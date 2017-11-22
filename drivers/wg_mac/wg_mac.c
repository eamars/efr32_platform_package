/**
 * @brief Simple PHY layer message transceiver
 * @date 21/Nov/2017
 * @author Ran Bao
 * @file wg_mac.h
 */


#if USE_FREERTOS == 1

#include "wg_mac.h"
#include "subg_packet_v2.h"

static void wg_mac_on_rx_done_isr(wg_mac_t * obj, void * msg, int32_t size, int32_t rssi, int32_t snr)
{
    wg_mac_msg_t rx_msg;

    DRV_ASSERT(size <= 0xff);

    // copy data
    rx_msg.size = (uint8_t) size;
    memcpy(rx_msg.buffer, msg, rx_msg.size);

    // copy rssi and snr
    obj->last_packet_rssi = (int16_t) rssi;
    obj->last_packet_snr = (int8_t) snr;

    // reset to rx to clear buffer
    radio_set_opmode_rx_timeout(obj->radio, 0);

    // send message to thread handler
    xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, NULL);
}

static void wg_mac_on_tx_done_isr(wg_mac_t * obj)
{
    // indicate the Tx is ready
    xSemaphoreGiveFromISR(obj->fsm_tx_done, NULL);
}

static void wg_mac_send_pri(wg_mac_t * obj, wg_mac_msg_t * msg, bool generate_seqid)
{
    if  (generate_seqid)
    {
        // give seq id
        subg_packet_v2_header_t * header = (subg_packet_v2_header_t *) msg->buffer;
        header->seq_id = obj->local_seq_id++;
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

    if (obj->retransmit.retry_counter >= obj->retransmit.max_retries)
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

                    // keep a copy of previously transmitted packet
                    memcpy(&obj->retransmit.prev_packet, &tx_msg, sizeof(wg_mac_msg_t));

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
                    // if the packet transmitted is an ACK, then we don't need to wait for ack for the ack
                    subg_packet_v2_cmd_t * cmd_packet = (subg_packet_v2_cmd_t *) obj->retransmit.prev_packet.buffer;
                    if (cmd_packet->header.packet_type == SUBG_PACKET_V2_TYPE_CMD &&
                        cmd_packet->command == SUBG_PACKET_V2_CMD_ACK)
                    {
                        obj->fsm_state = WG_MAC_IDLE;
                    }
                    else
                    {
                        // starts rx receive window
                        radio_set_opmode_rx_timeout(obj->radio, 0);

                        // starts rx timer
                        xTimerStart(obj->retransmit.rx_window_timer, portMAX_DELAY);

                        // setup done, then enter rx state
                        obj->fsm_state = WG_MAC_RX;
                    }
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
                    if (rx_msg.size < SUBG_PACKET_V2_HEADER_SIZE)
                    {
                        break;
                    }

                    subg_packet_v2_header_t * header = (subg_packet_v2_header_t *) rx_msg.buffer;

                    // validate the protocol version
                    if (header->protocol_version != SUBG_PACKET_V2_PROTOCOL_VER)
                    {
                        break;
                    }

                    // validate the destination
                    if (header->dest_id8 != obj->device_id8)
                    {
                        break;
                    }

                    // send filtered packet to upper layer (drop the packet if the upper layer failed to read from queue)
                    xQueueSend(obj->rx_queue, &rx_msg, 0);

                    // clear the retransmission if the receive packet is a command packet (any command packet will be
                    // treated as the acknowledgement)
                    if (header->packet_type == SUBG_PACKET_V2_TYPE_CMD)
                    {
                        xTimerStop(obj->retransmit.rx_window_timer, portMAX_DELAY);

                        // enter idle state
                        obj->fsm_state = WG_MAC_IDLE;

                        subg_packet_v2_cmd_t * cmd_packet = (subg_packet_v2_cmd_t *) rx_msg.buffer;

                        // if the acknowledgement is purely an acknowledgement, then we don't want to ack the packet
                        if (cmd_packet->command == SUBG_PACKET_V2_CMD_ACK)
                        {
                            break;
                        }
                    }

                    // ack the received packet
                    {
                        wg_mac_msg_t tx_msg;
                        tx_msg.size = sizeof(subg_packet_v2_cmd_t);
                        subg_packet_v2_cmd_t * ack_packet = (subg_packet_v2_cmd_t *) tx_msg.buffer;

                        // build packet
                        ack_packet->header.dest_id8 = header->src_id8;
                        ack_packet->header.src_id8 = obj->device_id8;
                        ack_packet->header.packet_type = (uint8_t) SUBG_PACKET_V2_TYPE_CMD;
                        ack_packet->header.protocol_version = SUBG_PACKET_V2_PROTOCOL_VER;

                        ack_packet->command = (uint8_t) SUBG_PACKET_V2_CMD_ACK;
                        ack_packet->payload = header->seq_id;

                        // transmit ack (we use a unique seqid for ack)
                        wg_mac_send_pri(obj, &tx_msg, true);
                    }
                }
            }
        }
    }
}

void wg_mac_init(wg_mac_t * obj, radio_t * radio, uint8_t device_id8)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(radio);

    // copy variables
    obj->radio = radio;
    obj->device_id8 = device_id8;
    obj->local_seq_id = 0;

    // setup retransmit
    obj->retransmit.rx_window_timer = xTimerCreate("rx_timer", pdMS_TO_TICKS(1000), pdFALSE, obj, wg_mac_on_rx_window_timeout);
    obj->retransmit.max_retries = WG_MAC_DEFAULT_MAX_RETRIES;

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

bool wg_mac_send_timeout(wg_mac_t * obj, wg_mac_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);

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
        if (xQueueSend(obj->tx_queue, &msg, pdMS_TO_TICKS(timeout_ms)) == pdFALSE)
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
