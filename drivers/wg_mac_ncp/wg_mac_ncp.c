/**
 * @brief Network Co-Processor for EFR32 RAIL raw protocol
 * @author Ran Bao
 * @date 23/Nov/2017
 * @file wg_mac_ncp.h
 */

#if USE_FREERTOS == 1

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "wg_mac_ncp.h"
#include "drv_debug.h"
#include "subg_mac.h"
#include "irq.h"
#include "yield.h"
#include "delay.h"

/**
 * @brief Define srandom function for gcc version below 5.0
 * GCC version below 5.0 does not have proper support for random() function. We
 * will use rand instead
 */
#if __GNUC__ < 5
#warning "You should upgrade your GCC to version 5.0 or above"
#define random(x) rand(x)
#endif

const wg_mac_ncp_config_t wg_mac_ncp_default_config = {
        .local_eui64 = 0,
        .max_heartbeat_period_sec = 21600, // by default the expire window for a client is 6 hours
        .ack_window_sec = 1,
        .max_retries = 3,
        .auto_ack = true,
        .downlink_extended_rx_window_ms = 5000,
        .enable_join_delay = true,
};

static void wg_mac_ncp_on_rx_done_handler(wg_mac_ncp_t * obj, void * msg, int32_t size, int32_t rssi, int32_t quality)
{
    wg_mac_ncp_raw_msg_t rx_msg;

    DRV_ASSERT(size <= 0xff);

    // copy data
    rx_msg.size = (uint8_t) size;
    memcpy(rx_msg.buffer, msg, rx_msg.size);
    rx_msg.rssi = rssi;
    rx_msg.quality = quality;

    // send message to thread handler
    xQueueSendFromISR(obj->rx_raw_packet_queue, &rx_msg, NULL);
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

static void wg_mac_ncp_send_raw_pri(wg_mac_ncp_t * obj, wg_mac_ncp_raw_msg_t * msg, bool generate_new_seqid, bool one_off)
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
            DRV_ASSERT(false);
            return;
        }

        // generate sequence id, if requested
        if (generate_new_seqid)
        {
            // assign
            header->seqid = client->tx_seqid++;

            // set id to local host
            header->src_id = (uint8_t) (obj->config.local_eui64 & 0xff);

            // make a copy of previously transmitted packet
            memcpy(&client->retransmit.prev_packet, msg, sizeof(wg_mac_ncp_raw_msg_t));

            // indicating the first attempt
            client->retransmit.retry_counter = 1;
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
            // the next retransmission time is calculated by: current time + ack_window
            client->next_retry_time_sec =
                    obj->sec_since_start + obj->config.ack_window_sec;

            client->retransmit.prev_packet_acked = false;
        }
    }

    // send to queue
    if (__IS_INTERRUPT())
    {
        if (!xQueueSendFromISR(obj->tx_raw_packet_queue, msg, NULL))
        {
            // tx_raw_packet_queue is full
            // DRV_ASSERT(false);
        }
    }
    else
    {
        if (!xQueueSend(obj->tx_raw_packet_queue, msg, 0))
        {
            // tx_raw_packet_queue is full
            // DRV_ASSERT(false);
        }
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


static wg_mac_ncp_error_code_t process_cmd_packet(wg_mac_ncp_t * obj, wg_mac_ncp_raw_msg_t * msg)
{
    // if the message is smaller than a command packet, then quit
    if (msg->size < sizeof(subg_mac_cmd_header_t))
        return WG_MAC_NCP_INVALID_PACKET_LENGTH;

    // client information
    wg_mac_ncp_client_t * client = NULL;

    // map command header packet to the msg
    subg_mac_cmd_header_t * cmd_header = (subg_mac_cmd_header_t *) msg->buffer;

    switch (((subg_mac_cmd_header_t *) msg->buffer)->cmd_type)
    {
        case SUBG_MAC_PACKET_CMD_ACK:
        {
            if (msg->size < sizeof(subg_mac_cmd_ack_t))
            {
                return WG_MAC_NCP_INVALID_PACKET_LENGTH;
            }

            subg_mac_cmd_ack_t * ack_packet = (subg_mac_cmd_ack_t *) msg->buffer;

            // decode the ack packet to figure out which packet is being acked
            switch (ack_packet->ack_type)
            {
                case SUBG_MAC_PACKET_CMD_ACK_REACHABLE: // INTERNATIONAL FALL THROUGH
                case SUBG_MAC_PACKET_CMD_ACK_CONFIRM:
                {
                    // find the client and compare it with the latest packet it transmitted
                    client = wg_mac_ncp_find_client_with_short_id(obj, ack_packet->cmd_header.mac_header.src_id);
                    break;
                }
                case SUBG_MAC_PACKET_CMD_ACK_UNREACHABLE:
                {
                    // TODO: how to find the correct client?
                    break;
                }
                default:
                {
                    break;
                }
            }


            if (client)
            {
                // clear the pending transmit packet
                if (!client->retransmit.prev_packet_acked)
                {
                    // matches the seqid
                    subg_mac_header_t * header = (subg_mac_header_t *) client->retransmit.prev_packet.buffer;
                    if (header->seqid == ack_packet->ack_seqid)
                        client->retransmit.prev_packet_acked = true;
                }

                // clear the downlink message if any
                if (client->downlink.downlink_state == WG_MAC_NCP_DOWNLINK_TRANSMITTING)
                {
                    // matches the seqid
                    subg_mac_header_t * header = (subg_mac_header_t *) client->downlink.downlink_packet.buffer;
                    if (header->seqid == ack_packet->ack_seqid)
                    {
                        client->downlink.downlink_state = WG_MAC_NCP_DOWNLINK_EMPTY;

                        // fire the callback to indicate the downlink packet has been transmitted
                        if (obj->callbacks.on_downlink_packet_success)
                            obj->callbacks.on_downlink_packet_success(obj, client, &client->downlink.downlink_packet);
                    }
                }
            }

            // NOTE: We don't care if the ack packet request to extend the receive window since NCP is designed
            // to open the rx window all the time

            break;
        }
        case SUBG_MAC_PACKET_CMD_JOIN_REQ:
        {
            if (msg->size < sizeof(subg_mac_cmd_join_req_t))
                return WG_MAC_NCP_INVALID_PACKET_LENGTH;

            subg_mac_cmd_join_req_t * join_request = (subg_mac_cmd_join_req_t *) msg->buffer;

            // find existing client in the client library
            // as the short id is self generated by client, we will ignore the short id in this case
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

                // initialize client default state
                client->is_valid = true;
                client->short_id = generate_unique_id(obj,
                                                      join_request->eui64); // generate a unique short id for the client
                client->device_eui64 = join_request->eui64;
                client->tx_seqid = 0;
                client->rx_seqid = (uint8_t) (join_request->cmd_header.mac_header.seqid - 1);
                client->retransmit.prev_packet_acked = true;
                client->retransmit.retry_counter = 0;
                client->downlink.downlink_state = WG_MAC_NCP_DOWNLINK_EMPTY;
            }

            // send a response message back
            wg_mac_ncp_raw_msg_t tx_msg;
            tx_msg.size = sizeof(subg_mac_cmd_join_resp_t);

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

            // delay response
            if (obj->config.enable_join_delay)
            {
                uint32_t delay = (uint32_t) lroundf(((-1.0f/13.0f) * msg->rssi + (30.0f/13.0f)));
                if (delay > 0)
                    delay_ms(pdMS_TO_TICKS(delay));
            }

            // send join response
            wg_mac_ncp_send_raw_pri(obj, &tx_msg, true, true);

            // TODO: At this point the device should stay in the pending state, in which case the device might joined other network
            // eventually, we need to remove such device from list if failed to send another join confirm message within a certain
            // timeout.

            break;
        }
        case SUBG_MAC_PACKET_CMD_JOIN_RESP:
        {
            // do not process join response message as the message is transmitted from ncp
            break;
        }
        case SUBG_MAC_PACKET_CMD_JOIN_CONFIRM:
        {
            if (msg->size < sizeof(subg_mac_cmd_join_confirm_t))
                return WG_MAC_NCP_INVALID_PACKET_LENGTH;

            subg_mac_cmd_join_confirm_t * join_confirm_packet = (subg_mac_cmd_join_confirm_t *) msg->buffer;

            // find existing client in the client library with short id (assume the device already send join request before
            client = wg_mac_ncp_find_client_with_short_id(obj, cmd_header->mac_header.src_id);

            if (client)
            {
                // send ack
                wg_mac_ncp_raw_msg_t tx_msg;

                tx_msg.size = sizeof(subg_mac_cmd_ack_t);

                // map subg_mac_cmd_ack_t to tx_msg
                subg_mac_cmd_ack_t * ack_packet = (subg_mac_cmd_ack_t *) tx_msg.buffer;

                ack_packet->cmd_header.mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
                ack_packet->cmd_header.mac_header.src_id = 0;
                ack_packet->cmd_header.mac_header.dest_id = client->short_id;
                ack_packet->cmd_header.mac_header.packet_type = SUBG_MAC_PACKET_CMD;
                ack_packet->cmd_header.mac_header.seqid = 0;
                ack_packet->cmd_header.cmd_type = SUBG_MAC_PACKET_CMD_ACK;
                ack_packet->ack_seqid = join_confirm_packet->cmd_header.mac_header.seqid;
                ack_packet->ack_type = SUBG_MAC_PACKET_CMD_ACK_CONFIRM;
                ack_packet->extended_rx_window_ms = 0; // no extra downlink for first packet

                // send ack message
                wg_mac_ncp_send_raw_pri(obj, &tx_msg, true, false);

                // fire the callback which indicates the device has joined the network
                if (obj->callbacks.on_client_joined)
                    obj->callbacks.on_client_joined(obj, client);
            }

            break;
        }
        default:
            break;
    }

    // if there is a client, update the information of such client
    if (client)
    {
        // set the general attribute for the client
        client->last_seen_sec = obj->sec_since_start;
        client->rx_seqid = cmd_header->mac_header.seqid;
    }

    return WG_MAC_NCP_NO_ERROR;
}


static wg_mac_ncp_error_code_t process_data_packet(wg_mac_ncp_t * obj, wg_mac_ncp_raw_msg_t * msg)
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

    // fire the callback for debug purpose (the application shouldn't rely on this callback
    if (obj->callbacks.on_packet_received)
        obj->callbacks.on_packet_received(obj, client, msg);

    // compare the sequence number of received data
    uint8_t seqid_diff = data_header->mac_header.seqid - client->rx_seqid;

    YIELD(
        if (seqid_diff == 0)
        {
            // repeated packet, this is likely caused by retransmission from the client
            // we are not going to report the packet to the upper layer
            if (obj->callbacks.on_repeated_message_recevied)
                obj->callbacks.on_repeated_message_recevied(obj, client, msg);

            break;
        }
        else if (seqid_diff > 1)
        {
            // have some missing packet however we don't care that much
            if (obj->callbacks.on_packet_missing)
                obj->callbacks.on_packet_missing(obj, client, seqid_diff);
        }
        else // diff == 1
        {
            // no missing packet
        }

        // deliver message to upper layer
        wg_mac_ncp_uplink_msg_t uplink_msg;

        uplink_msg.rssi = msg->rssi;
        uplink_msg.quality = msg->quality;
        uplink_msg.client = client;
        uplink_msg.payload_size = data_header->payload_length;
        memcpy(uplink_msg.payload, &data_header->payload_start_place_holder, data_header->payload_length);

        xQueueSend(obj->rx_data_packet_queue, &uplink_msg, 0);
    );

    // set the general attribute for the client
    client->last_seen_sec = obj->sec_since_start;
    client->rx_seqid = data_header->mac_header.seqid;

    // process acknowledgement
    wg_mac_ncp_raw_msg_t tx_msg;

    tx_msg.size = sizeof(subg_mac_cmd_ack_t);

    // map subg_mac_cmd_ack_t to tx_msg
    subg_mac_cmd_ack_t * ack_packet = (subg_mac_cmd_ack_t *) tx_msg.buffer;

    ack_packet->cmd_header.mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
    ack_packet->cmd_header.mac_header.src_id = 0;
    ack_packet->cmd_header.mac_header.dest_id = client->short_id;
    ack_packet->cmd_header.mac_header.packet_type = SUBG_MAC_PACKET_CMD;
    ack_packet->cmd_header.mac_header.seqid = 0;
    ack_packet->cmd_header.cmd_type = SUBG_MAC_PACKET_CMD_ACK;
    ack_packet->ack_seqid = data_header->mac_header.seqid;
    ack_packet->ack_type = SUBG_MAC_PACKET_CMD_ACK_CONFIRM;

    // extend the receiving window at client to make sure following packet can be received
    // the receive window periodic is specified in ACK packet
    if (client->downlink.downlink_state == WG_MAC_NCP_DOWNLINK_PENDING)
    {
        ack_packet->extended_rx_window_ms = obj->config.downlink_extended_rx_window_ms;
    }
    else
    {
        ack_packet->extended_rx_window_ms = 0;
    }

    // send ack message
    wg_mac_ncp_send_raw_pri(obj, &tx_msg, true, false);

    // for safety concern, we only transmit data on data packet rx window
    if (client->downlink.downlink_state == WG_MAC_NCP_DOWNLINK_PENDING)
    {
        // NOTE: @wg_mac_ncp_send_raw_pri will assign the downlink packet a unique seqid for the short period of time
        // The receiver thread will then compare the seqid with ack to make sure packet is delivered
        wg_mac_ncp_send_raw_pri(obj, &client->downlink.downlink_packet, true, false);

        // put the downlink state to transmitting, allowing retransmission to be complete
        client->downlink.downlink_state = WG_MAC_NCP_DOWNLINK_TRANSMITTING;

        // fire the callback to indicate the packet is attempting to transmit
        if (obj->callbacks.on_downlink_packet_init)
            obj->callbacks.on_downlink_packet_init(obj, client, &client->downlink.downlink_packet);
    }

    return WG_MAC_NCP_NO_ERROR;
}

static wg_mac_ncp_error_code_t process_routing_packet(wg_mac_ncp_t * obj, wg_mac_ncp_raw_msg_t * msg)
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
                if (!obj->clients[idx].retransmit.prev_packet_acked)
                {
                    // retransmit if it's the time to retransmit
                    if (obj->sec_since_start > obj->clients[idx].next_retry_time_sec)
                    {
                        obj->clients[idx].retransmit.retry_counter += 1;

                        // exceed the maximum retry threshold, then remove the device from list
                        if (obj->clients[idx].retransmit.retry_counter > obj->config.max_retries)
                        {
                            // mark the device as invalid
                            obj->clients[idx].is_valid = false;

                            // if the packet is downlink packet, then notify the packet loss
                            if (obj->clients[idx].downlink.downlink_state == WG_MAC_NCP_DOWNLINK_TRANSMITTING)
                            {
                                if (obj->callbacks.on_downlink_packet_fail)
                                    obj->callbacks.on_downlink_packet_fail(obj, &obj->clients[idx], &obj->clients[idx].downlink.downlink_packet);
                            }

                            // fire the callback to notify the client has been removed
                            if (obj->callbacks.on_client_left)
                                obj->callbacks.on_client_left(obj, &obj->clients[idx], WG_MAC_NCP_CLIENT_NO_RESPONSE);

                            // no further checking of the client
                            continue;
                        }
                        else
                        {
                            // retransmit the same packet
                            wg_mac_ncp_send_raw_pri(obj, &obj->clients[idx].retransmit.prev_packet, false, false);
                        }
                    }
                }

                // remove the device from list if unseen for a certain amount of time
                uint32_t unseen_period_sec = obj->sec_since_start - obj->clients[idx].last_seen_sec;

                if (unseen_period_sec > obj->config.max_heartbeat_period_sec)
                {
                    obj->clients[idx].is_valid = false;

                    // fire the callback to notify the client has been removed
                    if (obj->callbacks.on_client_left)
                        obj->callbacks.on_client_left(obj, &obj->clients[idx], WG_MAC_NCP_CLIENT_LOST);
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
                if (radio_get_opmode(obj->radio) != RADIO_OPMODE_RX)
                    radio_recv_timeout(obj->radio, 0);

                // listen on both tx_queue_pri and rx_queue_pri
                QueueSetMemberHandle_t queue = xQueueSelectFromSet(obj->queue_set_pri, portMAX_DELAY);

                if (queue == obj->tx_raw_packet_queue)
                {
                    wg_mac_ncp_raw_msg_t raw_msg;

                    // read from raw packet queue
                    if (!xQueueReceive(queue, &raw_msg, 0))
                    {
                        DRV_ASSERT(false);
                    }

                    // send to radio immediately
                    radio_send_timeout(obj->radio, raw_msg.buffer, raw_msg.size, 0);

                    // advance to Tx state waiting for tx_done
                    obj->state = WG_MAC_NCP_TX;
                }
                else if (queue == obj->rx_raw_packet_queue)
                {
                    wg_mac_ncp_raw_msg_t raw_msg;

                    // read from data queue
                    if (!xQueueReceive(queue, &raw_msg, 0))
                    {
                        DRV_ASSERT(false);
                    }

                    // validate packet length
                    if (raw_msg.size < SUBG_MAC_HEADER_SIZE)
                    {
                        break;
                    }

                    subg_mac_header_t * header = (subg_mac_header_t *) raw_msg.buffer;

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

                    // fire the callback for debug purposes
                    if (obj->callbacks.on_raw_packet_received)
                        obj->callbacks.on_raw_packet_received(obj, &raw_msg);

                    // process packet with different type
                    switch(header->packet_type)
                    {
                        case SUBG_MAC_PACKET_CMD:
                        {
                            process_cmd_packet(obj, &raw_msg);
                            break;
                        }
                        case SUBG_MAC_PACKET_DATA:
                        {
                            process_data_packet(obj, &raw_msg);
                            break;
                        }
                        case SUBG_MAC_PACKET_ROUTING:
                        {
                            process_routing_packet(obj, &raw_msg);
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


void wg_mac_ncp_init(wg_mac_ncp_t * obj, radio_t * radio, wg_mac_ncp_config_t * config, wg_mac_ncp_backup_t * backup)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(radio);

    // copy config
    obj->radio = radio;

    // make the radio sleep during setup
    radio_set_opmode_sleep(obj->radio);

    // reset sec since start
    obj->sec_since_start = 0;

    // put radio to rx state
    obj->state = WG_MAC_NCP_RX;

    // if the user didn't supply the config, then use default copy
    if (config)
        memcpy(&obj->config, config, sizeof(wg_mac_ncp_config_t));
    else
        memcpy(&obj->config, &wg_mac_ncp_default_config, sizeof(wg_mac_ncp_config_t));

    // restore backup data
    if (backup)
    {
        // continue from previous run
        obj->sec_since_start = backup->sec_since_start;

        // restore clients which are valid
        for (uint8_t client_idx = 0; client_idx < WG_MAC_NCP_MAX_CLIENT_COUNT; client_idx++)
        {
            // only copy the valid client
            if (backup->client_list[client_idx].is_valid)
            {
                obj->clients[client_idx].is_valid = true;
                obj->clients[client_idx].device_eui64 = backup->client_list[client_idx].device_eui64;
                obj->clients[client_idx].last_seen_sec = backup->client_list[client_idx].last_seen_sec;
                obj->clients[client_idx].short_id = backup->client_list[client_idx].short_id;
            }
        }
    }
    else
    {
        // initialize client list
        for (uint8_t client_idx = 0; client_idx < WG_MAC_NCP_MAX_CLIENT_COUNT; client_idx++)
        {
            obj->clients[client_idx].is_valid = false;
        }
    }

    // reset user-defined callbacks
    memset(&obj->callbacks, 0x0, sizeof(obj->callbacks));

    // register radio callback functions
    radio_set_rx_done_handler(obj->radio, wg_mac_ncp_on_rx_done_handler, obj);
    radio_set_tx_done_handler(obj->radio, wg_mac_ncp_on_tx_done_handler, obj);

    // create queue
    obj->tx_raw_packet_queue = xQueueCreate(WG_MAC_NCP_QUEUE_LENGTH, sizeof(wg_mac_ncp_raw_msg_t));
    DRV_ASSERT(obj->tx_raw_packet_queue);

    obj->rx_data_packet_queue = xQueueCreate(WG_MAC_NCP_QUEUE_LENGTH, sizeof(wg_mac_ncp_uplink_msg_t));
    obj->rx_raw_packet_queue = xQueueCreate(WG_MAC_NCP_QUEUE_LENGTH, sizeof(wg_mac_ncp_raw_msg_t));
    DRV_ASSERT(obj->rx_data_packet_queue);
    DRV_ASSERT(obj->rx_raw_packet_queue);

    // create queue set
    obj->queue_set_pri = xQueueCreateSet(WG_MAC_NCP_QUEUE_LENGTH * 2);
    xQueueAddToSet(obj->tx_raw_packet_queue, obj->queue_set_pri);
    xQueueAddToSet(obj->rx_raw_packet_queue, obj->queue_set_pri);

    obj->tx_done_signal = xSemaphoreCreateBinary();
    DRV_ASSERT(obj->tx_done_signal);

    // create book keeping thread (handles transceiver state)
    xTaskCreate((void *) wg_mac_ncp_client_bookkeeping_thread, "ncp_b", 500, obj, 4, &obj->client_bookkeeping_thread);
    xTaskCreate((void *) wg_mac_ncp_state_machine_thread, "ncp_t", 500, obj, 3, &obj->state_machine_thread);
}

wg_mac_ncp_error_code_t wg_mac_ncp_send_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_downlink_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);
    DRV_ASSERT(msg->client);

    if (!msg->client->is_valid)
    {
        return WG_MAC_NCP_NO_CLIENT_FOUND;
    }

    // the downlink message cannot be configured at transmitting state
    if (msg->client->downlink.downlink_state == WG_MAC_NCP_DOWNLINK_TRANSMITTING)
    {
        return WG_MAC_NCP_INVALID_CLIENT_STATE;
    }

    // the downlink message can only be transmitted right after the uplink message from
    // corresponding client. Each client has one slot for holding the most recent downlink
    // message.
    msg->client->downlink.downlink_state = WG_MAC_NCP_DOWNLINK_PENDING;

    // build message (directly map to the client's downlink packet buffer)
    {
        subg_mac_data_header_t * data_header =
                (subg_mac_data_header_t *) msg->client->downlink.downlink_packet.buffer;

        data_header->mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
        data_header->mac_header.src_id = 0;
        data_header->mac_header.dest_id = msg->client->short_id;
        data_header->mac_header.packet_type = SUBG_MAC_PACKET_DATA;
        data_header->mac_header.seqid = 0;

        data_header->data_type = SUBG_MAC_PACKET_DATA_CONFIRM;
        data_header->payload_length = msg->payload_size;

        memcpy(&data_header->payload_start_place_holder, msg->payload, msg->payload_size);

        // set mac layer payload size
        msg->client->downlink.downlink_packet.size =
                (uint16_t) (sizeof(subg_mac_data_header_t) - 1 + data_header->payload_length); // do not count for the place holder in the data header
    }

    return WG_MAC_NCP_NO_ERROR;
}


wg_mac_ncp_error_code_t wg_mac_ncp_recv_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_uplink_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);

    // allow user to wait forever
    if (timeout_ms == portMAX_DELAY)
    {
        if (xQueueReceive(obj->rx_data_packet_queue, msg, portMAX_DELAY))
        {
            return WG_MAC_NCP_NO_ERROR;
        }
    }
    else
    {
        // attempt to read from queue
        if (xQueueReceive(obj->rx_data_packet_queue, msg, pdMS_TO_TICKS(timeout_ms)))
        {
            return WG_MAC_NCP_NO_ERROR;
        }
    }

    // timeout, nothing is received in rx queue
    return WG_MAC_NCP_TIMEOUT;
}

#endif // #if USE_FREERTOS == 1
