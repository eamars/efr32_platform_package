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

    DRV_ASSERT(xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, NULL));

    obj->fsm_state = WG_MAC_FSM_RX_DONE;
    xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}

static void wg_mac_on_tx_done_isr(wg_mac_t * obj)
{
    // indicate the Tx is ready
    xSemaphoreGiveFromISR(obj->fsm_tx_done, NULL);

    obj->fsm_state = WG_MAC_FSM_TX_DONE;
    xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}

static void wg_mac_on_rx_error_isr(wg_mac_t * obj)
{
    obj->fsm_state = WG_MAC_FSM_RX_ERROR;
    xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}

static void wg_mac_on_rx_timeout_isr(wg_mac_t * obj)
{
    obj->fsm_state = WG_MAC_FSM_RX_TIMEOUT;
    xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}


static void lora_phy_on_rx_window_timeout(TimerHandle_t xTimer)
{
    DRV_ASSERT(xTimer);
    xTimerStop(xTimer, 0);

    // read radio object
    wg_mac_t *obj = (wg_mac_t *) pvTimerGetTimerID(xTimer);

    // if the receive window is still open, then put the device to sleep state, otherwise leave the device as is
    if (radio_get_opmode(obj->radio) == RADIO_OPMODE_RX)
    {
        radio_set_opmode_sleep(obj->radio);
    }
}

static void lora_phy_fsm_thread(wg_mac_t * obj)
{
    while (1)
    {
        switch (obj->fsm_state)
        {
            case WG_MAC_FSM_RX_IDLE:
            {
                // the state machine thread stopped as nothing is received or is required to be transmitted
                // the fsm will start running when event occurred, for example, radio ISR or packet is ready to be
                // transmitted
                xSemaphoreTake(obj->fsm_poll_event, portMAX_DELAY);

                // read data from tx queue, checking if data is queueing
                // if so, then transmit data
                wg_mac_msg_t tx_msg;
                if (xQueueReceive(obj->tx_queue, &tx_msg, 0))
                {
                    // give seq id
                    subg_packet_v2_header_t * header = (subg_packet_v2_header_t *) tx_msg.buffer;
                    header->seq_id = obj->local_seq_id++;

                    // send to phy layer handler
                    radio_send_timeout(obj->radio, tx_msg.buffer, tx_msg.size, 2000);
                    obj->fsm_state = WG_MAC_FSM_TX;
                }

                break;
            }

            case WG_MAC_FSM_TX:
            {
                // stay in tx state until tx is done
                if (!xSemaphoreTake(obj->fsm_tx_done, pdMS_TO_TICKS(5000)))
                {
                    // fail to transmit, then enter TX_TIMEOUT STATE
                    obj->fsm_state = WG_MAC_FSM_TX_TIMEOUT;
                }

                break;
            }

            case WG_MAC_FSM_TX_DONE:
            {
                // enter active receive mode for first receive window
                radio_set_opmode_rx_timeout(obj->radio, 1000);

                obj->fsm_state = WG_MAC_FSM_RX_IDLE;

                break;
            }

            case WG_MAC_FSM_RX_DONE:
            {
                // read message transmit from interrupt
                wg_mac_msg_t rx_msg;
                while (xQueueReceive(obj->rx_queue_pri, &rx_msg, 0))
                {
                    // filter the packet with wrong packet size
                    if (rx_msg.size < SUBG_PACKET_V2_HEADER_SIZE)
                    {
                        continue;
                    }

                    // filter the packet with wrong protocol version
                    subg_packet_v2_header_t * header = (subg_packet_v2_header_t *) rx_msg.buffer;
                    if (header->protocol_version < SUBG_PACKET_V2_PROTOCOL_VER)
                    {
                        continue;
                    }

                    // filter the packet with wrong destination
                    if (header->dest_id8 != obj->device_id8)
                    {
                        continue;
                    }

                    // send filtered packet to upper layer
                    xQueueSend(obj->rx_queue, &rx_msg, 0);

                    // do not ack the ack packet received
                    if (header->packet_type == SUBG_PACKET_V2_TYPE_CMD)
                    {
                        subg_packet_v2_cmd_t * cmd_packet = (subg_packet_v2_cmd_t *) rx_msg.buffer;
                        if (cmd_packet->command == SUBG_PACKET_V2_CMD_ACK)
                        {
                            continue;
                        }
                    }

                    // ack the received packet (both command and data, except for ack packet)
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

                    // transmit ack
                    wg_mac_send_block(obj, &tx_msg);
                }

                radio_set_opmode_sleep(obj->radio);

                obj->fsm_state = WG_MAC_FSM_RX_IDLE;

                break;
            }

            case WG_MAC_FSM_RX_ERROR:
            {
                // enter rx mode
                radio_set_opmode_sleep(obj->radio);
                obj->fsm_state = WG_MAC_FSM_RX_IDLE;

                break;
            }

            case WG_MAC_FSM_RX_TIMEOUT:
            {
                // enter rx mode
                radio_set_opmode_sleep(obj->radio);
                obj->fsm_state = WG_MAC_FSM_RX_IDLE;

                break;
            }

            case WG_MAC_FSM_TX_TIMEOUT:
            {
                // enter rx mode
                radio_set_opmode_sleep(obj->radio);
                obj->fsm_state = WG_MAC_FSM_RX_IDLE;

                break;
            }

            default:
            {
                DRV_ASSERT(false);
                break;
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

    // set radio to sleep default state
    radio_set_opmode_sleep(obj->radio);

    // register callback function
    radio_set_rx_done_handler(obj->radio, wg_mac_on_rx_done_isr, obj);
    radio_set_rx_done_handler(obj->radio, wg_mac_on_tx_done_isr, obj);
    radio_set_rx_error_handler(obj->radio, wg_mac_on_rx_error_isr, obj);
    radio_set_rx_timeout_handler(obj->radio, wg_mac_on_rx_timeout_isr, obj);

    // configure state machine
    obj->fsm_state = WG_MAC_FSM_RX_IDLE;
    obj->tx_queue = xQueueCreate(4, sizeof(wg_mac_msg_t));
    obj->rx_queue_pri = xQueueCreate(4, sizeof(wg_mac_msg_t));
    obj->rx_queue = xQueueCreate(4, sizeof(wg_mac_msg_t));

    obj->fsm_tx_done = xSemaphoreCreateBinary();
    obj->fsm_poll_event = xSemaphoreCreateBinary();

    xTaskCreate((void *) lora_phy_fsm_thread, "lora_phy", 500, obj, 3, &obj->fsm_thread_handler);

    DRV_ASSERT(obj->tx_queue);
    DRV_ASSERT(obj->rx_queue_pri);
    DRV_ASSERT(obj->rx_queue);
    DRV_ASSERT(obj->fsm_tx_done);
    DRV_ASSERT(obj->fsm_poll_event);
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

    // if the packet is put to the queue then I should run the fsm as soon as possible
    xSemaphoreGive(obj->fsm_poll_event);

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
