/**
 * @brief Simple PHY layer message transceiver
 * @date 21/Nov/2017
 * @author Ran Bao
 * @file wg_mac.h
 */


#if USE_FREERTOS == 1

#include "wg_mac.h"
#include "subg_mac.h"
#include "utils.h"

const wg_mac_config_t wg_mac_default_config = {
        .local_eui64 = 0,
        .rx_window_timeout_ms = WG_MAC_DEFAULT_RX_WINDOW_TIMEOUT_MS,
        .tx_timeout_ms = WG_MAC_DEFAULT_TX_TIMEOUT_MS,
        .max_retransmit = WG_MAC_DEFAULT_MAX_RETRIES,
};

static void wg_mac_on_rx_done_isr(wg_mac_t * obj, void * msg, int32_t size, int32_t rssi, int32_t quality)
{
    wg_mac_raw_msg_t rx_msg;

    DRV_ASSERT(size <= 0xff);

    // copy data
    rx_msg.size = (uint8_t) size;
    memcpy(rx_msg.buffer, msg, rx_msg.size);

    // copy rssi and snr
    rx_msg.rssi = rssi;
    rx_msg.quality = quality;

    // send message to thread handler
    xQueueSendFromISR(obj->rx_raw_packet_queue, &rx_msg, NULL);
}

static void wg_mac_on_tx_done_isr(wg_mac_t * obj)
{
    // notify the Tx is complete
    xSemaphoreGiveFromISR(obj->fsm_tx_done, NULL);
}

static void wg_mac_send_raw_pri(wg_mac_t * obj, wg_mac_raw_msg_t * msg, bool generate_new_seqid)
{
    if (generate_new_seqid)
    {
        // give seq id
        subg_mac_header_t * header = (subg_mac_header_t *) msg->buffer;
        header->seqid = obj->local_seq_id++;

        // give src id and dest id
        if (!obj->link_state.is_network_joined)
        {
            // if the device is not joining the network, then use local eui64 mask with 0xFF
            header->src_id = (uint8_t) (obj->config.local_eui64 & 0xff);
            header->dest_id = SUBG_MAC_BROADCAST_ADDR_ID;
        }
        else
        {
            // otherwise use the allocated id
            header->src_id = obj->link_state.allocated_id;
            header->dest_id = obj->link_state.uplink_dest_id;
        }

        // keep a copy of previously transmitted packet
        memcpy(&obj->retransmit.prev_packet, msg, sizeof(wg_mac_raw_msg_t));
    }

    // send to phy layer handler
    radio_send_timeout(obj->radio, msg->buffer, msg->size, 0);
    obj->fsm_state = WG_MAC_TX;
}

static void wg_mac_on_rx_window_timeout(TimerHandle_t xTimer)
{
    DRV_ASSERT(xTimer);
    xTimerStop(xTimer, 0);

    // read radio object
    wg_mac_t * obj = (wg_mac_t *) pvTimerGetTimerID(xTimer);

    // if previous packet is acked, then I can safely enter the idle state and waiting for another transmission
    if (obj->retransmit.is_packet_clear)
    {
        obj->fsm_state = WG_MAC_IDLE;
    }
    else
    {
        // at the moment where this function is called, which indicates no proper response is received from node, we need
        // to start the retransmission

        // we can bypass the idle state to transmit packet
        obj->retransmit.retry_counter += 1;

        if (obj->retransmit.retry_counter >= obj->config.max_retransmit)
        {
            // if the network is joined previously, then trigger an event,
            // otherwise we do nothing.
            if (obj->link_state.is_network_joined)
            {
                // leave the network
                if (obj->callbacks.on_network_state_changed)
                    obj->callbacks.on_network_state_changed(obj, WG_MAC_NETWORK_LEFT);
            }

            // reset to idle state
            obj->fsm_state = WG_MAC_IDLE;
        }
        else
        {
            // for the retransmission, we don't generate a different seqid
            wg_mac_send_raw_pri(obj, &obj->retransmit.prev_packet, false);
        }
    }
}


static void send_ack_packet(wg_mac_t * obj, uint8_t seqid_to_ack)
{
    wg_mac_raw_msg_t tx_msg;

    tx_msg.size = sizeof(subg_mac_cmd_ack_t);

    // map subg_mac_cmd_ack_t to tx_msg
    subg_mac_cmd_ack_t * ack_packet = (subg_mac_cmd_ack_t *) tx_msg.buffer;

    ack_packet->cmd_header.mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
    ack_packet->cmd_header.mac_header.src_id = 0;
    ack_packet->cmd_header.mac_header.dest_id = obj->link_state.uplink_dest_id;
    ack_packet->cmd_header.mac_header.packet_type = SUBG_MAC_PACKET_CMD;
    ack_packet->cmd_header.mac_header.seqid = 0;
    ack_packet->cmd_header.cmd_type = SUBG_MAC_PACKET_CMD_ACK;
    ack_packet->ack_seqid = seqid_to_ack;
    ack_packet->ack_type = SUBG_MAC_PACKET_CMD_ACK_CONFIRM;
    ack_packet->extended_rx_window_ms = 0;

    wg_mac_send_raw_pri(obj, &tx_msg, true);
}


static wg_mac_error_code_t process_cmd_packet(wg_mac_t * obj, wg_mac_raw_msg_t * msg)
{
    // if the message is smaller than a command packet, then quit
    if (msg->size < sizeof(subg_mac_cmd_header_t))
        return WG_MAC_INVALID_PACKET_LENGTH;

    bool clear_pending = false;
    bool send_ack = false;
    bool network_joined = false;

    // map command header packet to the msg
    subg_mac_cmd_header_t * cmd_header = (subg_mac_cmd_header_t *) msg->buffer;

    switch (cmd_header->cmd_type)
    {
        case SUBG_MAC_PACKET_CMD_ACK:
        {
            if (msg->size < sizeof(subg_mac_cmd_ack_t))
                return WG_MAC_INVALID_PACKET_LENGTH;

            subg_mac_cmd_ack_t * ack_packet = (subg_mac_cmd_ack_t *) msg->buffer;

            switch(ack_packet->ack_type)
            {
                case SUBG_MAC_PACKET_CMD_ACK_CONFIRM:
                case SUBG_MAC_PACKET_CMD_ACK_REACHABLE:
                case SUBG_MAC_PACKET_CMD_ACK_UNREACHABLE:
                {
                    // check seqid of ack packet and compare the value with previous transmitted packet
                    subg_mac_header_t * header = (subg_mac_header_t *) obj->retransmit.prev_packet.buffer;
                    if (ack_packet->ack_seqid == header->seqid)
                        clear_pending = true;

                    break;
                }
                default:
                    break;
            }

            // extend the rx window if required
            if (ack_packet->extended_rx_window_ms)
            {
                xTimerStop(obj->retransmit.rx_window_timer, portMAX_DELAY);

                // select extended rx window timeout if present
                xTimerChangePeriod(obj->retransmit.rx_window_timer,
                                   pdMS_TO_TICKS(
                                           MAX(ack_packet->extended_rx_window_ms, obj->config.rx_window_timeout_ms)
                                   ),
                                   portMAX_DELAY);
                xTimerStart(obj->retransmit.rx_window_timer, portMAX_DELAY);
            }

            break;
        }
        case SUBG_MAC_PACKET_CMD_JOIN_REQ:
        {
            // the client shouldn't receive the join request message, skip
            break;
        }
        case SUBG_MAC_PACKET_CMD_JOIN_RESP:
        {
            if (msg->size < sizeof(subg_mac_cmd_join_resp_t))
            {
                return WG_MAC_INVALID_PACKET_LENGTH;
            }

            subg_mac_cmd_join_resp_t * response_packet = (subg_mac_cmd_join_resp_t *) msg->buffer;

            // the hub send me an allocated id!
            obj->link_state.is_network_joined = true;
            obj->link_state.allocated_id = response_packet->allocated_device_id;
            obj->link_state.uplink_dest_id = response_packet->uplink_dest_id;

            // clear the pending packet and send ack back
            clear_pending = true;
            send_ack = true;
            network_joined = true;

            break;
        }
        default:
            break;
    }

    if (clear_pending)
    {
        obj->retransmit.is_packet_clear = true;
    }

    if (send_ack)
    {
        send_ack_packet(obj, cmd_header->mac_header.seqid);
    }

    if (network_joined)
    {
        // fire callback to indicate the network state has changed
        // Note: unlike network leave, the join event will always be acknowledged to the user
        // since the device may request to join another network without notifying the previous
        // host
        if (obj->callbacks.on_network_state_changed)
            obj->callbacks.on_network_state_changed(obj, WG_MAC_NETWORK_JOINED);
    }

    return WG_MAC_NO_ERROR;
}


static wg_mac_error_code_t process_data_packet(wg_mac_t * obj, wg_mac_raw_msg_t * msg)
{
    if (msg->size < sizeof(subg_mac_data_header_t))
        return WG_MAC_INVALID_PACKET_LENGTH;

    wg_mac_downlink_msg_t downlink_msg;
    subg_mac_data_header_t * data_header = (subg_mac_data_header_t *) msg->buffer;

    // if data requires ack, then send one
    if (data_header->data_type == SUBG_MAC_PACKET_DATA_CONFIRM)
    {
        send_ack_packet(obj, data_header->mac_header.seqid);
    }

    // dump payload from raw packet
    downlink_msg.rssi = msg->rssi;
    downlink_msg.quality = msg->quality;
    downlink_msg.src_id = data_header->mac_header.src_id;
    downlink_msg.payload_size = data_header->payload_length;
    memcpy(downlink_msg.payload, &data_header->payload_start_place_holder, data_header->payload_length);

    // send data to upper layer
    xQueueSend(obj->rx_data_packet_queue, &downlink_msg, 0);

    return WG_MAC_NO_ERROR;
}


static void wg_mac_fsm_thread(wg_mac_t * obj)
{
    while (1)
    {
        switch (obj->fsm_state)
        {
            case WG_MAC_IDLE:
            {
                radio_set_opmode_sleep(obj->radio);

                // in idle state, the stack will listen to both tx_raw queue and tx_data queue
                // however, tx_raw queue has higher priority than tx_data queue
                QueueSetMemberHandle_t queue = xQueueSelectFromSet(obj->tx_queue_set, portMAX_DELAY);

                // set information for retransmission
                obj->retransmit.retry_counter = 0;
                obj->retransmit.is_packet_clear = false;

                if (queue == obj->tx_raw_packet_queue)
                {
                    wg_mac_raw_msg_t raw_msg;

                    // read from raw packet queue
                    if (!xQueueReceive(queue, &raw_msg, 0))
                    {
                        DRV_ASSERT(false);
                    }

                    // send to transceiver
                    wg_mac_send_raw_pri(obj, &raw_msg, true);
                }
                else if (queue == obj->tx_data_packet_queue)
                {
                    wg_mac_uplink_msg_t uplink_msg;

                    // read from tx data queue
                    if (!xQueueReceive(queue, &uplink_msg, 0))
                    {
                        DRV_ASSERT(false);
                    }

                    // build raw data
                    wg_mac_raw_msg_t raw_msg;
                    {
                        subg_mac_data_header_t * data_header = (subg_mac_data_header_t *) raw_msg.buffer;

                        data_header->mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
                        data_header->mac_header.src_id = 0;
                        data_header->mac_header.dest_id = 0;
                        data_header->mac_header.packet_type = SUBG_MAC_PACKET_DATA;
                        data_header->mac_header.seqid = 0;

                        data_header->data_type = uplink_msg.requires_ack ?
                                                 SUBG_MAC_PACKET_DATA_CONFIRM :
                                                 SUBG_MAC_PACKET_DATA_UNCONFIRM;
                        data_header->payload_length = uplink_msg.payload_size;

                        // copy payload
                        memcpy(&data_header->payload_start_place_holder, uplink_msg.payload, uplink_msg.payload_size);

                        // set mac layer payload size
                        raw_msg.size = (uint16_t) (SUBG_MAC_PACKET_DATA_HEADER_SIZE + data_header->payload_length);
                    }

                    // send to transceiver
                    wg_mac_send_raw_pri(obj, &raw_msg, true);
                    // xQueueSend(obj->tx_raw_packet_queue, &raw_msg, portMAX_DELAY);
                }
                else
                {
                    DRV_ASSERT(false);
                }

                break;
            }

            case WG_MAC_TX:
            {
                // in tx state we are going to wait until tx is complete
                // stay in tx state until tx is done
                if (xSemaphoreTake(obj->fsm_tx_done, pdMS_TO_TICKS(WG_MAC_DEFAULT_TX_TIMEOUT_MS)))
                {
                    // waiting for ack, unless specified
                    obj->retransmit.is_packet_clear = false;

                    // if previously doesn't require ack or the previous packet is an ack, we are not expecting an ack
                    subg_mac_header_t * header = (subg_mac_header_t *) obj->retransmit.prev_packet.buffer;
                    if (header->packet_type == SUBG_MAC_PACKET_CMD)
                    {
                        subg_mac_cmd_header_t * cmd_header = (subg_mac_cmd_header_t *) obj->retransmit.prev_packet.buffer;
                        if (cmd_header->cmd_type == SUBG_MAC_PACKET_CMD_ACK)
                        {
                            obj->retransmit.is_packet_clear = true;
                        }
                    }
                    else if (header->packet_type == SUBG_MAC_PACKET_DATA)
                    {
                        subg_mac_data_header_t * data_header = (subg_mac_data_header_t *) obj->retransmit.prev_packet.buffer;
                        if (data_header->data_type == SUBG_MAC_PACKET_DATA_UNCONFIRM)
                        {
                            obj->retransmit.is_packet_clear = true;
                        }
                    }

                    // for other packet that requires an ACK, open the receive window and waiting for ack packet
                    // starts rx receive window

                    // reset the rx window timer
                    uint32_t time_left = 0;
                    if (xTimerIsTimerActive(obj->retransmit.rx_window_timer))
                    {
                        // get the time left
                        time_left = (xTimerGetExpiryTime(obj->retransmit.rx_window_timer) - xTaskGetTickCount()) /
                                (configTICK_RATE_HZ / 1000);

                        // update previous timer
                        xTimerStop(obj->retransmit.rx_window_timer, portMAX_DELAY);
                    }

                    // create a new timer, based on the time left for the previous one
                    xTimerChangePeriod(obj->retransmit.rx_window_timer,
                                       pdMS_TO_TICKS(
                                            MAX(time_left, obj->config.rx_window_timeout_ms)
                                       ),
                                       portMAX_DELAY);
                    xTimerStart(obj->retransmit.rx_window_timer, portMAX_DELAY);

                    // fire the callback to indicate the transmit is done
                    if (obj->callbacks.on_raw_packet_transmitted)
                        obj->callbacks.on_raw_packet_transmitted(obj, &obj->retransmit.prev_packet);

                    // setup done, then enter rx state
                    obj->fsm_state = WG_MAC_RX;
                }
                else
                {
                    // fail to transmit, then enter idle state
                    obj->fsm_state = WG_MAC_IDLE;

                    DRV_ASSERT(false);
                }

                break;
            }

            case WG_MAC_RX:
            {
                // put the radio into receive mode in rx state
                if (radio_get_opmode(obj->radio) != RADIO_OPMODE_RX)
                    radio_recv_timeout(obj->radio, 0);

                // wait for packets to arrive
                wg_mac_raw_msg_t rx_msg;
                if (xQueueReceive(obj->rx_raw_packet_queue, &rx_msg, pdMS_TO_TICKS(100)))
                {
                    // validate the packet lenght
                    if (rx_msg.size < SUBG_MAC_HEADER_SIZE)
                    {
                        break;
                    }

                    subg_mac_header_t * header = (subg_mac_header_t *) rx_msg.buffer;

                    // validate the magic byte
                    if (header->magic_byte != SUBG_MAC_MAGIC_BYTE)
                    {
                        break;
                    }

                    // validate the destination, based on current link state
                    if (!obj->link_state.is_network_joined)
                    {
                        // if not joined, the destination will be local eui64 masked with 0xff
                        if (header->dest_id != (uint8_t) (obj->config.local_eui64 & 0xff))
                        {
                            break;
                        }
                    }
                    else
                    {
                        // if the network is joined, the destination will be allocated id
                        if (header->dest_id != obj->link_state.allocated_id)
                        {
                            break;
                        }
                    }

                    // fire the callback for debug purposes
                    if (obj->callbacks.on_raw_packet_received)
                        obj->callbacks.on_raw_packet_received(obj, &rx_msg);

                    // process packet
                    switch (header->packet_type)
                    {
                        case SUBG_MAC_PACKET_CMD:
                        {
                            process_cmd_packet(obj, &rx_msg);
                            break;
                        }
                        case SUBG_MAC_PACKET_DATA:
                        {
                            process_data_packet(obj, &rx_msg);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
    }
}

void wg_mac_init(wg_mac_t * obj, radio_t * radio, wg_mac_config_t * config)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(radio);

    // copy variables
    obj->radio = radio;
    obj->local_seq_id = 0;

    // if the user didn't supply the config, then use default copy
    if (config)
        memcpy(&obj->config, config, sizeof(wg_mac_config_t));
    else
        memcpy(&obj->config, &wg_mac_default_config, sizeof(wg_mac_config_t));

    // clear all callback functions to avoid errors
    memset(&obj->callbacks, 0x0, sizeof(obj->callbacks));

    // register radio callback function
    radio_set_rx_done_handler(obj->radio, wg_mac_on_rx_done_isr, obj);
    radio_set_tx_done_handler(obj->radio, wg_mac_on_tx_done_isr, obj);

    // initialize the link state
    obj->link_state.is_network_joined = false;
    obj->link_state.allocated_id = 0;
    obj->link_state.uplink_dest_id = 0;

    // setup retransmit
    obj->retransmit.retry_counter = 0;
    obj->retransmit.is_packet_clear = false;
    obj->retransmit.rx_window_timer = xTimerCreate(
            "rx_timer",
            pdMS_TO_TICKS(config->rx_window_timeout_ms),
            pdFALSE,
            obj,
            wg_mac_on_rx_window_timeout
    );

    // set radio to sleep default state
    radio_set_opmode_sleep(obj->radio);

    // configure state machine
    obj->fsm_state = WG_MAC_IDLE;
    obj->fsm_tx_done = xSemaphoreCreateBinary();
    DRV_ASSERT(obj->fsm_tx_done);

    // configure queue
    obj->rx_raw_packet_queue = xQueueCreate(WG_MAC_QUEUE_LENGTH, sizeof(wg_mac_raw_msg_t));
    obj->rx_data_packet_queue = xQueueCreate(WG_MAC_QUEUE_LENGTH, sizeof(wg_mac_downlink_msg_t));
    DRV_ASSERT(obj->rx_data_packet_queue);
    DRV_ASSERT(obj->rx_raw_packet_queue);

    obj->tx_raw_packet_queue = xQueueCreate(WG_MAC_QUEUE_LENGTH, sizeof(wg_mac_raw_msg_t));
    obj->tx_data_packet_queue = xQueueCreate(WG_MAC_QUEUE_LENGTH, sizeof(wg_mac_uplink_msg_t));
    DRV_ASSERT(obj->tx_raw_packet_queue);
    DRV_ASSERT(obj->tx_data_packet_queue);

    // create queue set
    obj->tx_queue_set = xQueueCreateSet(WG_MAC_QUEUE_LENGTH * 2);
    DRV_ASSERT(obj->tx_queue_set);

    xQueueAddToSet(obj->tx_raw_packet_queue, obj->tx_queue_set);
    xQueueAddToSet(obj->tx_data_packet_queue, obj->tx_queue_set);

    xTaskCreate((void *) wg_mac_fsm_thread, "wg_mac", 500, obj, 3, &obj->fsm_thread_handler);
}


void wg_mac_join_network(wg_mac_t * obj)
{
    DRV_ASSERT(obj);

    // form a join request packet
    wg_mac_raw_msg_t tx_msg;
    subg_mac_cmd_join_req_t * join_request = (subg_mac_cmd_join_req_t *) tx_msg.buffer;

    // set the packet length
    tx_msg.size = SUBG_MAC_PACKET_CMD_JOIN_REQ_SIZE;

    join_request->cmd_header.mac_header.magic_byte = SUBG_MAC_MAGIC_BYTE;
    join_request->cmd_header.mac_header.src_id = 0; // src id is managed by driver
    join_request->cmd_header.mac_header.dest_id = 0;
    join_request->cmd_header.mac_header.packet_type = SUBG_MAC_PACKET_CMD;
    join_request->cmd_header.mac_header.seqid = 0; // seqid is controlled by the driver
    join_request->cmd_header.cmd_type = SUBG_MAC_PACKET_CMD_JOIN_REQ;
    join_request->eui64 = obj->config.local_eui64;

    // send to local transmit queue
    xQueueSend(obj->tx_raw_packet_queue, &tx_msg, portMAX_DELAY);
}


bool wg_mac_send_timeout(wg_mac_t * obj, wg_mac_uplink_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);

    // send message buffer to the queue
    if (timeout_ms == portMAX_DELAY)
    {
        if (xQueueSend(obj->tx_data_packet_queue, msg, portMAX_DELAY) == pdFALSE)
        {
            return false;
        }
    }
    else
    {
        // wait for TX to get ready
        if (xQueueSend(obj->tx_data_packet_queue, msg, pdMS_TO_TICKS(timeout_ms)) == pdFALSE)
        {
            // timeout, indicating we failed to wait for transceiver to get ready
            return false;
        }
    }

    return true;
}

bool wg_mac_recv_timeout(wg_mac_t * obj, wg_mac_downlink_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);

    // allow user to wait forever
    if (timeout_ms == portMAX_DELAY)
    {
        if (xQueueReceive(obj->rx_data_packet_queue, msg, portMAX_DELAY) == pdFALSE)
        {
            return false;
        }
    }
    else
    {
        // attempt to read from queue
        if (xQueueReceive(obj->rx_data_packet_queue, msg, pdMS_TO_TICKS(timeout_ms)) == pdFALSE)
        {
            // timeout, nothing is received in rx queue
            return false;
        }
    }

    return true;
}

#endif // #if USE_FREERTOS == 1
