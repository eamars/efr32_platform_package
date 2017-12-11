/**
 * @brief Network Co-Processor for EFR32 RAIL raw protocol
 * @author Ran Bao
 * @date 23/Nov/2017
 * @file wg_mac_ncp.h
 */

#if USE_FREERTOS == 1

#include <string.h>
#include <stdlib.h>

#include "wg_mac_ncp.h"
#include "drv_debug.h"
#include "subg_mac.h"
#include "irq.h"
#include "yield.h"

const wg_mac_ncp_config_t wg_mac_ncp_default_config = {
        .local_eui64 = 0,
        .max_heartbeat_period_sec = 21600, // by default the expire window for a client is 6 hours
        .ack_window_sec = 2,
        .max_retries = 3,
        .forward_all_packets = true,
        .auto_ack = true,
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

wg_mac_ncp_client_t * wg_mac_ncp_find_client_with_eui64(wg_mac_ncp_t * obj, uint64_t device_eui64)
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

wg_mac_ncp_client_t * wg_mac_ncp_find_client_with_short_id(wg_mac_ncp_t * obj, uint8_t short_id)
{
    for (uint8_t idx = 0; idx < WG_MAC_NCP_MAX_CLIENT_COUNT; idx++)
    {
        // look for sequence id for a specific client
        if (obj->clients[idx].is_valid)
        {
            if (obj->clients[idx].short_id == short_id)
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

static void wg_mac_ncp_send_pri(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, bool first_attempt, bool one_off)
{
    // look for the destination
    subg_mac_header_t * header = (subg_mac_header_t *) msg->buffer;

    if (one_off)
    {
        // assign a mock sequence id
        header->seqid = 0;

        // set id to local host
        header->src_id = (uint8_t) (obj->config.local_eui64 & 0xff);
    }
    else
    {
        wg_mac_ncp_client_t * client = wg_mac_ncp_find_client_with_short_id(obj, header->dest_id);

        if (!client)
        {
            // TODO: Notify the host that the host is attemping to send data to an unknown host
            DEBUG_PRINT("NCP: attempting to transmit to an unknown client [%d]\r\n", header->dest_id);

            DRV_ASSERT(false);
            return;
        }

        // generate sequence id, if requested
        if (first_attempt)
        {
            // assign
            header->seqid = obj->clients->tx_seqid++;

            // set id to local host
            header->src_id = (uint8_t) (obj->config.local_eui64 & 0xff);

            // make a copy of previously transmitted packet
            memcpy(&client->prev_packet, msg, sizeof(wg_mac_ncp_msg_t));

            // indicating the first attempt
            client->retry_counter = 1;
        }

        // for every packet that requires the ack, we calculate the next retransmission time
        bool requires_ack = true;
        if (header->packet_type == SUBG_MAC_PACKET_CMD)
        {
            // ACK packet don't need an ack
            subg_mac_cmd_header_t * cmd_header = (subg_mac_cmd_header_t *) msg->buffer;
            if (cmd_header->cmd_type == SUBG_MAC_PACKET_CMD_ACK)
            {
                requires_ack = false;
            }
        }
        else if (header->packet_type == SUBG_MAC_PACKET_DATA)
        {
            // unconfirmed data don't need an ack
            subg_mac_data_header_t * data_header = (subg_mac_data_header_t *) msg->buffer;
            if (data_header->data_type == SUBG_MAC_PACKET_DATA_UNCONFIRM)
            {
                requires_ack = false;
            }
        }

        if (requires_ack)
        {
            // the next retransmission time is calculated by: current time + retry_counter * ack_window
            client->next_retry_time_sec =
                    obj->sec_since_start + client->retry_counter * obj->config.ack_window_sec;

            client->prev_packet_acked = false;
        }
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


static bool is_id_unique(wg_mac_ncp_t * obj, uint8_t unique_id)
{
    // check if the generated id is the same as current device id
    if (unique_id == (uint8_t) (obj->config.local_eui64 & 0xff))
    {
        return false;
    }

    // check if device id presents in client list
    for (uint8_t idx = 0; idx < WG_MAC_NCP_MAX_CLIENT_COUNT; idx++)
    {
        // tell if the short id matches the unique id
        if (obj->clients[idx].is_valid)
        {
            if (obj->clients[idx].short_id == unique_id)
            {
                return false;
            }
        }
    }

    return true;
}

static uint8_t generate_unique_id(wg_mac_ncp_t * obj, uint64_t eui64)
{
    uint8_t unique_id = 0;

    do
    {
        // generate a random number
        unique_id = (uint8_t) (random() & 0xff);
    } while (!is_id_unique(obj, unique_id));

    return unique_id;
}


static wg_mac_ncp_error_code_t process_cmd_packet(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg)
{
    // if the message is smaller than a command packet, then quit
    if (msg->size < sizeof(subg_mac_cmd_header_t))
        return WG_MAC_NCP_INVALID_PACKET_LENGTH;

    // client information
    wg_mac_ncp_client_t * client = NULL;

    // map command header packet to the msg
    subg_mac_cmd_header_t * cmd_header = (subg_mac_cmd_header_t *) msg->buffer;

    switch (cmd_header->cmd_type)
    {
        case SUBG_MAC_PACKET_CMD_ACK:
        {
            // no special operation with ACK packet
            client = wg_mac_ncp_find_client_with_short_id(obj, cmd_header->mac_header.src_id);
            break;
        }
        case SUBG_MAC_PACKET_CMD_JOIN_REQ:
        {
            if (msg->size < sizeof(subg_mac_cmd_join_req_t))
                return WG_MAC_NCP_INVALID_PACKET_LENGTH;

            subg_mac_cmd_join_req_t * join_request = (subg_mac_cmd_join_req_t *) msg->buffer;

            // find existing client in the client library
            client = wg_mac_ncp_find_client_with_eui64(obj, join_request->eui64);

            // if the device is not found in existing device list, then create a new client
            if (!client)
            {
                client = wg_mac_ncp_find_available_client_slot(obj);

                // failed to create new client, then exit
                if (!client)
                {
                    return WG_MAC_NCP_NO_CLIENT_SLOT_AVAILABLE;
                }

                // a new client has been created
                memset(client, 0x0, sizeof(wg_mac_ncp_client_t));

                client->is_valid = true;
                client->short_id = generate_unique_id(obj, join_request->eui64); // generate a unique short id for the client
                client->device_eui64 = join_request->eui64;
                client->prev_packet_acked = true;
                client->retry_counter = 0;
                client->tx_seqid = 0;
                client->rx_seqid = (uint8_t) (join_request->cmd_header.mac_header.seqid - 1);

                DEBUG_PRINT("NCP: a new device [0x%08llx] joined, assigned with short_id [0x%02x]\r\n", client->device_eui64, client->short_id);

                // send a response message back
                wg_mac_ncp_msg_t tx_msg;
                tx_msg.size = SUBG_MAC_PACKET_CMD_JOIN_RESP_SIZE;

                // map
                subg_mac_cmd_join_resp_t * response_packet = (subg_mac_cmd_join_resp_t *) tx_msg.buffer;
                response_packet->cmd_header.mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
                response_packet->cmd_header.mac_header.src_id = 0;
                response_packet->cmd_header.mac_header.dest_id = join_request->cmd_header.mac_header.src_id;
                response_packet->cmd_header.mac_header.packet_type = SUBG_MAC_PACKET_CMD;
                response_packet->cmd_header.mac_header.seqid = 0;

                response_packet->cmd_header.cmd_type = SUBG_MAC_PACKET_CMD_JOIN_RESP;
                response_packet->allocated_device_id = client->short_id;
                response_packet->uplink_dest_id = (uint8_t) (obj->config.local_eui64 & 0xff);

                wg_mac_ncp_send_pri(obj, &tx_msg, true, true);
            }

            break;
        }
        case SUBG_MAC_PACKET_CMD_JOIN_RESP:
        {
            // do not process join response message as the message is transmitted from ncp
            break;
        }
        default:
            break;
    }

    // if there is a client, then do some bookkeeping stuff
    if (client)
    {
        // set the general attribute for the client
        client->last_seen_sec = obj->sec_since_start;
        client->rx_seqid = cmd_header->mac_header.seqid;
    }

    return WG_MAC_NCP_NO_ERROR;
}


static wg_mac_ncp_error_code_t process_data_packet(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg)
{
    // if the message is smaller than a data header, quit
    if (msg->size < sizeof(subg_mac_data_header_t))
        return WG_MAC_NCP_INVALID_PACKET_LENGTH;

    subg_mac_data_header_t * data_header = (subg_mac_data_header_t *) msg->buffer;

    // find client by the data header
    wg_mac_ncp_client_t * client = NULL;

    client = wg_mac_ncp_find_client_with_short_id(obj, data_header->mac_header.src_id);

    if (!client)
    {
        return WG_MAC_NCP_NO_CLIENT_FOUND;
    }

    // compare the sequence number of received data
    uint8_t seqid_diff = data_header->mac_header.seqid - client->rx_seqid;

    YIELD(
        if (seqid_diff == 0)
        {
            // TODO: repeated packet, this is likely caused by retransmission from the client
            // we are not going to report the packet to the upper layer
            DEBUG_PRINT("NCP: receive repeated packet with seqid [0x%x] from [0x%08llx], message won't deliver to upper layer\r\n",
                        data_header->mac_header.seqid, client->device_eui64);

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
        xQueueSend(obj->rx_queue, msg, 0);
    );

    // set the general attribute for the client
    client->last_seen_sec = obj->sec_since_start;
    client->rx_seqid = data_header->mac_header.seqid;

    // process acknowledgement
    wg_mac_ncp_msg_t tx_msg;

    tx_msg.size = sizeof(subg_mac_cmd_ack_t);

    // map subg_mac_cmd_ack_t to tx_msg
    subg_mac_cmd_ack_t * ack_packet = (subg_mac_cmd_ack_t *) tx_msg.buffer;

    ack_packet->cmd_header.mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
    ack_packet->cmd_header.mac_header.src_id = 0;
    ack_packet->cmd_header.mac_header.dest_id = client->short_id;
    ack_packet->cmd_header.mac_header.packet_type = SUBG_MAC_PACKET_CMD;
    ack_packet->cmd_header.mac_header.seqid = 0;
    ack_packet->cmd_header.cmd_type = SUBG_MAC_PACKET_CMD_ACK;
    ack_packet->ack_seqid = client->rx_seqid;
    ack_packet->ack_type = SUBG_MAC_PACKET_CMD_ACK_CONFIRM;

    wg_mac_ncp_send_pri(obj, &tx_msg, true, true);

    return WG_MAC_NCP_NO_ERROR;
}

static wg_mac_ncp_error_code_t process_routing_packet(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg)
{
    return WG_MAC_NCP_NO_ERROR;
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
                            wg_mac_ncp_send_pri(obj, &obj->clients[idx].prev_packet, false, false);
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
                    if (msg.size < SUBG_MAC_HEADER_SIZE)
                    {
                        break;
                    }

                    subg_mac_header_t * header = (subg_mac_header_t *) msg.buffer;

                    // validate the protocol version
                    if (header->magic_byte != SUBG_MAC_MAGIC_BYTE)
                    {
                        break;
                    }

                    // validate the destination (allow the broadcast as well)
                    // when forward_all_packet option is enabled, the destination of the packet will be ignored
                    if (header->dest_id != (uint8_t) (obj->config.local_eui64 & 0xff) &&
                            header->dest_id != SUBG_MAC_BROADCAST_ADDR_ID)
                    {
                        break;
                    }

                    // process packet with different type
                    switch(header->packet_type)
                    {
                        case SUBG_MAC_PACKET_CMD:
                        {
                            process_cmd_packet(obj, &msg);
                            break;
                        }
                        case SUBG_MAC_PACKET_DATA:
                        {
                            process_data_packet(obj, &msg);
                            break;
                        }
                        case SUBG_MAC_PACKET_ROUTING:
                        {
                            process_routing_packet(obj, &msg);
                            break;
                        }
                        default:
                            break;
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
            wg_mac_ncp_send_pri(obj, &tx_msg, true, false);
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
    xTaskCreate((void *) wg_mac_ncp_client_bookkeeping_thread, "ncp_b", 500, obj, 4, &obj->client_bookkeeping_thread);
    xTaskCreate((void *) wg_mac_ncp_state_machine_thread, "ncp_t", 500, obj, 3, &obj->state_machine_thread);
    xTaskCreate((void *) wg_mac_ncp_tx_queue_handler_thread, "ncp_q", 500, obj, 4, &obj->tx_queue_handler_thread);
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
        if (xQueueReceive(obj->rx_queue, msg, portMAX_DELAY))
        {
            return true;
        }
    }
    else
    {
        // attempt to read from queue
        if (xQueueReceive(obj->rx_queue, msg, pdMS_TO_TICKS(timeout_ms)))
        {
            return true;
        }
    }

    // timeout, nothing is received in rx queue
    return false;
}


#endif // #if USE_FREERTOS == 1
