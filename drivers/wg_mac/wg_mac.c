/**
 * @brief Simple PHY layer message transceiver
 * @date 21/Nov/2017
 * @author Ran Bao
 * @file wg_mac.h
 */


#if USE_FREERTOS == 1

#include "wg_mac.h"
#include "subg_mac.h"

const wg_mac_config_t wg_mac_default_config = {
        .local_eui64 = 0,
        .rx_window_timeout_ms = WG_MAC_DEFAULT_RX_WINDOW_TIMEOUT_MS,
        .tx_timeout_ms = WG_MAC_DEFAULT_TX_TIMEOUT_MS,
        .max_retransmit = WG_MAC_DEFAULT_MAX_RETRIES,
        .forwared_all_packets = false
};

static void wg_mac_on_rx_done_isr(wg_mac_t * obj, void * msg, int32_t size, int32_t rssi, int32_t snr)
{
    wg_mac_msg_t rx_msg;

    DRV_ASSERT(size <= 0xff);

    // copy data
    rx_msg.size = (uint8_t) size;
    memcpy(rx_msg.buffer, msg, rx_msg.size);

    // copy rssi and snr
    rx_msg.rssi = rssi;
    rx_msg.snr = snr;

    // continue receive packet unless interrupted
    radio_recv_timeout(obj->radio, 0);

    // send message to thread handler
    xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, NULL);
}

static void wg_mac_on_tx_done_isr(wg_mac_t * obj)
{
    // notify the Tx is complete
    xSemaphoreGiveFromISR(obj->fsm_tx_done, NULL);
}

static void wg_mac_send_pri(wg_mac_t * obj, wg_mac_msg_t * msg, bool first_attempt)
{
    if (first_attempt)
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
        memcpy(&obj->retransmit.prev_packet, msg, sizeof(wg_mac_msg_t));
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

    // at the moment where this function is called, which indicates no proper response is received from node, we need
    // to start the retransmission

    // we can bypass the idle state to transmit packet
    obj->retransmit.retry_counter += 1;

    if (obj->retransmit.retry_counter >= obj->config.max_retransmit)
    {
        // reset to idle state
        obj->fsm_state = WG_MAC_IDLE;
    }
    else
    {
        // for the retransmission, we don't generate a different seqid
        wg_mac_send_pri(obj, &obj->retransmit.prev_packet, false);
    }
}


static void send_ack_packet(wg_mac_t * obj, uint8_t seqid_to_ack)
{
    wg_mac_msg_t tx_msg;

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

    wg_mac_send_pri(obj, &tx_msg, true);
}


static wg_mac_error_code_t process_cmd_packet(wg_mac_t * obj, wg_mac_msg_t * msg)
{
    // if the message is smaller than a command packet, then quit
    if (msg->size < sizeof(subg_mac_cmd_header_t))
        return WG_MAC_INVALID_PACKET_LENGTH;

    bool clear_pending = false;
    bool send_ack = false;

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
                    // TODO: Check seqid of acked packet
                    clear_pending = true;
                    send_ack = false;
                    break;
                }
                default:
                    break;
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
        }
        default:
            break;
    }

    if (clear_pending)
    {
        xTimerStop(obj->retransmit.rx_window_timer, portMAX_DELAY);

        obj->fsm_state = WG_MAC_IDLE;
    }

    if (send_ack)
    {
        send_ack_packet(obj, cmd_header->mac_header.seqid);
    }

    return WG_MAC_NO_ERROR;
}

static wg_mac_error_code_t process_data_packet(wg_mac_t * obj, wg_mac_msg_t * msg)
{
    if (msg->size < sizeof(subg_mac_data_header_t))
        return WG_MAC_INVALID_PACKET_LENGTH;

    subg_mac_data_header_t * data_header = (subg_mac_data_header_t *) msg->buffer;

    // send data to upper layer
    xQueueSend(obj->rx_queue, msg, 0);

    // if data requires ack, then send one
    if (data_header->data_type == SUBG_MAC_PACKET_DATA_CONFIRM)
    {
        send_ack_packet(obj, data_header->mac_header.seqid);
    }

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

                // in the idle state the fsm will wait for any new message that is ready to transmit
                wg_mac_msg_t tx_msg;
                if (xQueueReceive(obj->tx_queue, &tx_msg, portMAX_DELAY))
                {
                    // set information for retransmission
                    obj->retransmit.retry_counter = 0;

                    // for the first attempt, we generate a unique seq id
                    wg_mac_send_pri(obj, &tx_msg, true);
                }

                break;
            }

            case WG_MAC_TX:
            {
                // in tx state we are going to wait until tx is complete
                // stay in tx state until tx is done
                if (!xSemaphoreTake(obj->fsm_tx_done, pdMS_TO_TICKS(WG_MAC_DEFAULT_TX_TIMEOUT_MS)))
                {
                    // fail to transmit, then enter idle state
                    obj->fsm_state = WG_MAC_IDLE;

                    DRV_ASSERT(false);
                }
                else
                {
                    // if previously doesn't require ack or the previous packet is an ack, we are not expecting an ack
                    subg_mac_header_t * header = (subg_mac_header_t *) obj->retransmit.prev_packet.buffer;
                    if (header->packet_type == SUBG_MAC_PACKET_CMD)
                    {
                        subg_mac_cmd_header_t * cmd_header = (subg_mac_cmd_header_t *) obj->retransmit.prev_packet.buffer;
                        if (cmd_header->cmd_type == SUBG_MAC_PACKET_CMD_ACK)
                        {
                            obj->fsm_state = WG_MAC_IDLE;
                            break;
                        }
                    }
                    else if (header->packet_type == SUBG_MAC_PACKET_DATA)
                    {
                        subg_mac_data_header_t * data_header = (subg_mac_data_header_t *) obj->retransmit.prev_packet.buffer;
                        if (data_header->data_type == SUBG_MAC_PACKET_DATA_UNCONFIRM)
                        {
                            obj->fsm_state = WG_MAC_IDLE;
                            break;
                        }
                    }

                    // for other packet that requires an ACK, open the receive window and waiting for ack packet
                    // starts rx receive window
                    radio_recv_timeout(obj->radio, 0);

                    // starts rx timer
                    xTimerChangePeriod(obj->retransmit.rx_window_timer, pdMS_TO_TICKS(obj->config.rx_window_timeout_ms),
                                       portMAX_DELAY);
                    xTimerStart(obj->retransmit.rx_window_timer, portMAX_DELAY);

                    // setup done, then enter rx state
                    obj->fsm_state = WG_MAC_RX;
                }

                break;
            }

            case WG_MAC_RX:
            {
                // wait for packets to arrive
                wg_mac_msg_t rx_msg;
                if (xQueueReceive(obj->rx_queue_pri, &rx_msg, pdMS_TO_TICKS(100)))
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

    // initialize the link state
    obj->link_state.is_network_joined = false;
    obj->link_state.allocated_id = 0;
    obj->link_state.uplink_dest_id = 0;

    // setup retransmit
    obj->retransmit.retry_counter = 0;
    obj->retransmit.rx_window_timer = xTimerCreate(
            "rx_timer",
            pdMS_TO_TICKS(obj-config->rx_window_timeout_ms),
            pdFALSE,
            obj,
            wg_mac_on_rx_window_timeout
    );

    // set radio to sleep default state
    radio_set_opmode_sleep(obj->radio);

    // register callback function
    radio_set_rx_done_handler(obj->radio, wg_mac_on_rx_done_isr, obj);
    radio_set_tx_done_handler(obj->radio, wg_mac_on_tx_done_isr, obj);

    // configure state machine
    obj->fsm_state = WG_MAC_IDLE;
    obj->tx_queue = xQueueCreate(4, sizeof(wg_mac_msg_t));
    obj->rx_queue_pri = xQueueCreate(4, sizeof(wg_mac_msg_t));
    obj->rx_queue = xQueueCreate(4, sizeof(wg_mac_msg_t));

    obj->fsm_tx_done = xSemaphoreCreateBinary();

    xTaskCreate((void *) wg_mac_fsm_thread, "wg_mac", 500, obj, 3, &obj->fsm_thread_handler);

    DRV_ASSERT(obj->tx_queue);
    DRV_ASSERT(obj->rx_queue_pri);
    DRV_ASSERT(obj->rx_queue);
    DRV_ASSERT(obj->fsm_tx_done);
}


void wg_mac_join_network(wg_mac_t * obj)
{
    DRV_ASSERT(obj);

    // form a join request packet
    wg_mac_msg_t tx_msg;
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
    wg_mac_send_timeout(obj, &tx_msg, portMAX_DELAY);
}


bool wg_mac_send_timeout(wg_mac_t * obj, wg_mac_msg_t * msg, uint32_t timeout_ms)
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

bool wg_mac_recv_timeout(wg_mac_t * obj, wg_mac_msg_t * msg, uint32_t timeout_ms)
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

#endif // #if USE_FREERTOS == 1
