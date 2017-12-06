/**
 * @brief Network Co-Processor for EFR32 RAIL raw protocol
 * @author Ran Bao
 * @date 23/Nov/2017
 * @file wg_mac_ncp.h
 */

#include <string.h>

#include "wg_mac_ncp.h"
#include "drv_debug.h"
#include "subg_packet_v2.h"
#include "irq.h"
#include "yield.h"

const wg_mac_ncp_config_t wg_mac_ncp_default_config = {
        .local_eui64 = 0,
        .max_heartbeat_period_sec = 21600, // by default the expire window for a client is 6 hours
        .ack_window_sec = 2,
        .max_retries = 3,
        .forward_all_packets = true,
        .auto_ack = true,
        .enable_beacon = false
};

static void wg_mac_ncp_on_rx_done_handler(wg_mac_ncp_t * obj, void * msg, int32_t size, int32_t rssi, int32_t snr)
{
    wg_mac_ncp_msg_t rx_msg;

    DRV_ASSERT(size <= 0xff);

    // copy data
    rx_msg.size = (uint8_t) size;
    memcpy(rx_msg.buffer, msg, rx_msg.size);
    rx_msg.rssi = rssi;
    rx_msg.snr = snr;

    // enter rx state again
    radio_recv_timeout(obj->radio, 0);

    // send message to thread handler
    xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, NULL);
}

static void wg_mac_ncp_on_tx_done_handler(wg_mac_ncp_t * obj)
{
    xSemaphoreGiveFromISR(obj->tx_done_signal, NULL);
}

wg_mac_ncp_client_t * wg_mac_ncp_find_client(wg_mac_ncp_t * obj, uint64_t device_eui64)
{
    for (uint8_t idx = 0; idx < WG_MAC_NCP_MAX_CLIENT_COUNT; idx++)
    {
        // look for sequence id for a specific client
        if (obj->clients[idx].is_valid)
        {
            if (obj->clients[idx].device_eui64 == device_eui64)
            {
                return &obj->clients[idx];
            }
        }
    }
    return NULL;
}

wg_mac_ncp_client_t * wg_mac_ncp_find_available_client_slot(wg_mac_ncp_t * obj)
{
    for (uint8_t idx = 0; idx < WG_MAC_NCP_MAX_CLIENT_COUNT; idx++)
    {
        // look for first invalid slot
        if (!obj->clients[idx].is_valid)
        {
            return &obj->clients[idx];
        }
    }
    return NULL;
}

static void wg_mac_ncp_send_pri(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, bool first_attempt)
{
    // look for the destination
    subg_packet_v2_header_t * header = (subg_packet_v2_header_t *) msg->buffer;
    wg_mac_ncp_client_t * client = wg_mac_ncp_find_client(obj, header->dest_id8);

    if (!client)
    {
        // TODO: Notify the host that the host is attemping to send data to an unknown host
        DEBUG_PRINT("NCP: attempting to transmit to an unknown client [%d]\r\n", header->dest_id8);

        DRV_ASSERT(false);
        return;
    }

    // generate sequence id, if requested
    if (first_attempt)
    {
        // assign
        header->seq_id = obj->clients->tx_seqid++;

        // set id to local host
        header->src_id8 = (uint8_t) obj->config.local_eui64;

        // make a copy of previously transmitted packet
        memcpy(&client->prev_packet, msg, sizeof(wg_mac_ncp_msg_t));

        // indicating the first attempt
        client->retry_counter = 1;
    }

    // for every packet that requires the ack, we calculate the next retransmission time
    if (msg->requires_ack)
    {
        // the next retransmission time is calculated by: current time + retry_counter * ack_window
        client->next_retry_time_sec =
                obj->sec_since_start + client->retry_counter * obj->config.ack_window_sec;

        client->prev_packet_acked = false;
    }

    // send to the queue
    if (__IS_INTERRUPT())
    {
        xQueueSendFromISR(obj->tx_queue_pri, msg, NULL);
    }
    else
    {
        xQueueSend(obj->tx_queue_pri, msg, portMAX_DELAY);
    }
}

static void wg_mac_ncp_client_bookkeeping_thread(wg_mac_ncp_t * obj)
{
    // initialize the task tick handler
    portTickType xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        // increase internal timer
        obj->sec_since_start += 1;

        // go through all valid clients
        for (uint8_t idx = 0; idx < WG_MAC_NCP_MAX_CLIENT_COUNT; idx++)
        {
            if (obj->clients[idx].is_valid)
            {
                // if the device didn't ack me yet
                if (!obj->clients[idx].prev_packet_acked)
                {
                    // retransmit if it's the time to retransmit
                    if (obj->sec_since_start > obj->clients[idx].next_retry_time_sec)
                    {
                        obj->clients[idx].retry_counter += 1;

                        // exceed the maximum retry threshold, then remove the device from list
                        if (obj->clients[idx].retry_counter > obj->config.max_retries)
                        {
                            // TODO: report to host that a device is unresponsive
                            DEBUG_PRINT("NCP: a client [0x%08llx] failed to ACK, removed from device list\r\n", obj->clients[idx].device_eui64);

                            // mark the device as invalid
                            obj->clients[idx].is_valid = false;

                            // no further checking of the client
                            continue;
                        }
                        else
                        {
                            // retransmit the same packet
                            wg_mac_ncp_send_pri(obj, &obj->clients[idx].prev_packet, false);
                        }
                    }

                }

                // remove the device from list if unseen for a certain amount of time
                uint32_t unseen_period_sec = obj->sec_since_start - obj->clients[idx].last_seen_sec;

                if (unseen_period_sec > obj->config.max_heartbeat_period_sec)
                {
                    obj->clients[idx].is_valid = false;
                }
            }
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}


static void wg_mac_ncp_state_machine_thread(wg_mac_ncp_t * obj)
{
    while (1)
    {
        switch (obj->state)
        {
            case WG_MAC_NCP_RX:
            {
                // put the radio to rx state before entering sleep
                radio_recv_timeout(obj->radio, 0);

                // listen on both tx_queue_pri and rx_queue_pri
                QueueSetMemberHandle_t queue = xQueueSelectFromSet(obj->queue_set_pri, portMAX_DELAY);

                wg_mac_ncp_msg_t msg;

                // read from queue
                xQueueReceive(queue, &msg, 0);

                if (queue == obj->tx_queue_pri)
                {
                    // send to radio phy layer
                    radio_send_timeout(obj->radio, msg.buffer, msg.size, 0);

                    // advance to tx state
                    obj->state = WG_MAC_NCP_TX;
                }
                else if (queue == obj->rx_queue_pri)
                {
                    // validate packet length
                    if (msg.size < SUBG_PACKET_V2_HEADER_SIZE)
                    {
                        break;
                    }

                    subg_packet_v2_header_t * header = (subg_packet_v2_header_t *) msg.buffer;

                    // validate the protocol version
                    if (header->protocol_version != SUBG_PACKET_V2_PROTOCOL_VER)
                    {
                        break;
                    }

                    // TODO: accept beacon signal before validating the destination

                    // validate the destination
                    // when forward_all_packet option is enabled, the destination of the packet will be ignored
                    if (header->dest_id8 != obj->config.local_eui64)
                    {
                        if (obj->config.forward_all_packets)
                        {
                            xQueueSend(obj->rx_queue, &msg, 0);
                        }
                        break;
                    }

                    // update the client info
                    wg_mac_ncp_client_t * client = wg_mac_ncp_find_client(obj, header->src_id8);

                    // if not found, then create a new one
                    if (!client)
                    {
                        client = wg_mac_ncp_find_available_client_slot(obj);
                        if (!client)
                        {
                            // TODO: Notify there is not slot available for new client
                            DRV_ASSERT(false);
                            break;
                        }
                        else
                        {
                            // setup a new client
                            memset(client, 0x0, sizeof(wg_mac_ncp_client_t));

                            client->is_valid = true;
                            client->device_eui64 = header->src_id8;
                            client->prev_packet_acked = true;
                            client->tx_seqid = 0;
                            client->rx_seqid = (uint8_t) (header->seq_id - 1);
                            client->retry_counter = 0;

                            // TODO: report the host that a new device joint the network
                            DEBUG_PRINT("NCP: a new device [0x%08llx] joint\r\n", client->device_eui64);
                        }
                    }

                    // compare the sequence id of the newly received packet
                    uint8_t seqid_diff = header->seq_id - client->rx_seqid;

                    YIELD(
                        if (seqid_diff == 0)
                        {
                            // TODO: repeated packet, this is likely caused by retransmission from the client
                            // we are not going to report the packet to the upper layer
                            DEBUG_PRINT("NCP: receive repeated packet with seqid [0x%x] from [0x%08llx], message won't deliver to upper layer\r\n",
                                        header->seq_id, client->device_eui64);

                            break;
                        }
                        else if (seqid_diff > 1)
                        {
                            // have some missing packet however we don't care that much
                            DEBUG_PRINT("NCP: have [%d] packet missing from [0x%08llx]\r\n", seqid_diff, client->device_eui64);
                        }
                        else // diff == 1
                        {
                            // no missing packet
                        }

                        // TODO: Send the packet to host
                        xQueueSend(obj->rx_queue, &msg, 0);
                    );

                    // set general attribute for the client
                    client->last_seen_sec = obj->sec_since_start;
                    client->rx_seqid = header->seq_id;

                    // tell the type of the packet
                    if (header->packet_type == SUBG_PACKET_V2_TYPE_CMD)
                    {
                        // map to a command packet
                        subg_packet_v2_cmd_t * cmd_packet = (subg_packet_v2_cmd_t *) msg.buffer;

                        // TODO: Only a dedicated ACK packet will clear the ACK on host
                        if (cmd_packet->command == SUBG_PACKET_V2_CMD_ACK)
                        {
                            // indicate the packet is acked
                            client->prev_packet_acked = true;

                            // reset the retry counter
                            client->retry_counter = 0;
                        }
                    }
                    else
                    {
                        if (!client->prev_packet_acked)
                        {
                            // TODO: Notify the host that previous packet is not acked
                            // DRV_ASSERT(false);
                        }

                        // ack the packet
                        if (obj->config.auto_ack)
                        {
                            wg_mac_ncp_msg_t tx_msg;
                            tx_msg.size = sizeof(subg_packet_v2_cmd_t);
                            tx_msg.requires_ack = false;
                            subg_packet_v2_cmd_t * ack_packet = (subg_packet_v2_cmd_t *) tx_msg.buffer;

                            // build packet
                            ack_packet->header.dest_id8 = header->src_id8;
                            ack_packet->header.src_id8 = (uint8_t) (obj->config.local_eui64 & 0xff);
                            ack_packet->header.packet_type = (uint8_t) SUBG_PACKET_V2_TYPE_CMD;
                            ack_packet->header.protocol_version = SUBG_PACKET_V2_PROTOCOL_VER;

                            ack_packet->command = (uint8_t) SUBG_PACKET_V2_CMD_ACK;
                            ack_packet->payload = header->seq_id;

                            wg_mac_ncp_send_pri(obj, &tx_msg, true);
                        }
                    }
                }
                else
                {
                    DRV_ASSERT(false);
                }

                break;
            }
            case WG_MAC_NCP_TX:
            {
                if (!xSemaphoreTake(obj->tx_done_signal, pdMS_TO_TICKS(5000)))
                {
                    // failed to transmit
                    obj->state = WG_MAC_NCP_RX;

                    DRV_ASSERT(false);
                }
                else
                {
                    // transmit complete, return to rx state
                    obj->state = WG_MAC_NCP_RX;
                }
                break;
            }
        }
    }
}

static void wg_mac_ncp_tx_queue_handler_thread(wg_mac_ncp_t * obj)
{
    // wait on tx_queue
    while (1)
    {
        wg_mac_ncp_msg_t tx_msg;
        if (xQueueReceive(obj->tx_queue, &tx_msg, portMAX_DELAY))
        {
            // for the message with first transmission, we always generate a seqid
            wg_mac_ncp_send_pri(obj, &tx_msg, true);
        }
    }
}

void wg_mac_ncp_init(wg_mac_ncp_t * obj, radio_t * radio, wg_mac_ncp_config_t * config)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(radio);

    // copy config
    obj->radio = radio;

    // make the radio sleep during setup
    radio_set_opmode_sleep(obj->radio);

    // if the user didn't supply the config, then use default copy
    if (config)
        memcpy(&obj->config, config, sizeof(wg_mac_ncp_config_t));
    else
        memcpy(&obj->config, &wg_mac_ncp_default_config, sizeof(wg_mac_ncp_config_t));

    // initialize client list
    for (uint8_t client_idx = 0; client_idx < WG_MAC_NCP_MAX_CLIENT_COUNT; client_idx++)
    {
        obj->clients[client_idx].is_valid = false;
    }

    // register callback functions
    radio_set_rx_done_handler(obj->radio, wg_mac_ncp_on_rx_done_handler, obj);
    radio_set_tx_done_handler(obj->radio, wg_mac_ncp_on_tx_done_handler, obj);

    // create queue
    obj->tx_queue_pri = xQueueCreate(WG_MAC_NCP_QUEUE_LENGTH, sizeof(wg_mac_ncp_msg_t));
    obj->tx_queue = xQueueCreate(WG_MAC_NCP_QUEUE_LENGTH, sizeof(wg_mac_ncp_msg_t));
    obj->rx_queue_pri = xQueueCreate(WG_MAC_NCP_QUEUE_LENGTH, sizeof(wg_mac_ncp_msg_t));
    obj->rx_queue = xQueueCreate(WG_MAC_NCP_QUEUE_LENGTH, sizeof(wg_mac_ncp_msg_t));

    // create queue set
    obj->queue_set_pri = xQueueCreateSet(WG_MAC_NCP_QUEUE_LENGTH * 2);
    xQueueAddToSet(obj->tx_queue_pri, obj->queue_set_pri);
    xQueueAddToSet(obj->rx_queue_pri, obj->queue_set_pri);

    obj->tx_done_signal = xSemaphoreCreateBinary();

    // put radio to rx state
    obj->state = WG_MAC_NCP_RX;

    // create book keeping thread (handles transceiver state)
    xTaskCreate((void *) wg_mac_ncp_client_bookkeeping_thread, "ncp_b", 400, obj, 4, &obj->client_bookkeeping_thread);
    xTaskCreate((void *) wg_mac_ncp_state_machine_thread, "ncp_t", 500, obj, 3, &obj->state_machine_thread);
    xTaskCreate((void *) wg_mac_ncp_tx_queue_handler_thread, "ncp_q", 400, obj, 4, &obj->tx_queue_handler_thread);
}

bool wg_mac_ncp_send_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);

    // send message buffer to the queue
    if (timeout_ms == portMAX_DELAY)
    {
        if (xQueueSend(obj->tx_queue, msg, portMAX_DELAY) == pdFALSE)
        {
            return false;
        }
    }
    else
    {
        // wait for TX to get ready
        if (xQueueSend(obj->tx_queue, msg, pdMS_TO_TICKS(timeout_ms)) == pdFALSE)
        {
            // timeout, indicating we failed to wait for transceiver to get ready
            return false;
        }
    }

    return true;
}


bool wg_mac_ncp_recv_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);

    // allow user to wait forever
    if (timeout_ms == portMAX_DELAY)
    {
        if (xQueueReceive(obj->rx_queue, msg, portMAX_DELAY) == pdFALSE)
        {
            return false;
        }
    }
    else
    {
        // attempt to read from queue
        if (xQueueReceive(obj->rx_queue, msg, pdMS_TO_TICKS(timeout_ms)) == pdFALSE)
        {
            // timeout, nothing is received in rx queue
            return false;
        }
    }

    return true;
}
