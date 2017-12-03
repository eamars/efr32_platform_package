/**
 * @brief Simple PHY layer message transceiver
 * @date 19, Oct, 2017
 * @author Ran Bao
 * @file lora_phy.h
 */

#if USE_FREERTOS == 1

#include "lora_phy.h"
#include "subg_packet_v2.h"

static void lora_phy_on_rx_done_isr(uint8_t * buffer, int16_t size, int16_t rssi, int8_t snr, lora_phy_t * obj)
{
    lora_phy_msg_t rx_msg;

    DRV_ASSERT(size <= 0xff);

    // copy data
    rx_msg.size = (uint8_t) size;
    memcpy(rx_msg.buffer, buffer, rx_msg.size);

    // copy rssi and snr
    obj->last_packet_rssi = rssi;
    obj->last_packet_snr = snr;

    // reset to rx state to clear buffer
    radio_rfm9x_set_opmode_rx(obj->radio);

    xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, NULL);

}

static void lora_phy_on_tx_done_isr(lora_phy_t * obj)
{
    // indicate the Tx is ready
    xSemaphoreGiveFromISR(obj->fsm_tx_done, NULL);
}

static void lora_phy_on_rx_error_isr(lora_phy_t * obj)
{

}

static void lora_phy_on_rx_timeout_isr(lora_phy_t * obj)
{

}

static void lora_phy_send_pri(lora_phy_t * obj, lora_phy_msg_t * msg, bool generate_seqid)
{
    if  (generate_seqid)
    {
        // give seq id
        subg_packet_v2_header_t * header = (subg_packet_v2_header_t *) msg->buffer;
        header->seq_id = obj->local_seq_id++;
    }

    // send to phy layer handler
    radio_rfm9x_send(obj->radio, msg->buffer, msg->size);
    obj->fsm_state = LORA_PHY_TX;
}


static void lora_phy_on_rx_window_timeout(TimerHandle_t xTimer)
{
    DRV_ASSERT(xTimer);
    xTimerStop(xTimer, 0);

    // read radio object
    lora_phy_t * obj = (lora_phy_t *) pvTimerGetTimerID(xTimer);

    // at the moment where this function is called, which indicates no proper response is received from node, we need
    // to start the retransmission

    // we can bypass the idle state to transmit packet
    obj->retransmit.retry_counter += 1;

    if (obj->retransmit.retry_counter >= LORA_PHY_MAX_RETRIES)
    {
        // reset to idle state
        obj->fsm_state = LORA_PHY_IDLE;
    }
    else
    {
        // for the retransmission, we don't generate a different seqid
        lora_phy_send_pri(obj, &obj->retransmit.prev_packet, false);
    }
}

static void lora_phy_fsm_thread(lora_phy_t * obj)
{
    while (1)
    {
        switch (obj->fsm_state)
        {
            case LORA_PHY_IDLE:
            {
                radio_rfm9x_set_opmode_sleep(obj->radio);

                // in the idle state the fsm will wait for any new message that is ready to transmit
                lora_phy_msg_t tx_msg;
                if (xQueueReceive(obj->tx_queue, &tx_msg, portMAX_DELAY))
                {
                    // set information for retransmission
                    obj->retransmit.retry_counter = 0;

                    // keep a copy of previously transmitted packet
                    memcpy(&obj->retransmit.prev_packet, &tx_msg, sizeof(lora_phy_msg_t));

                    // for the first attempt, we generate a unique seq id
                    lora_phy_send_pri(obj, &tx_msg, true);
                }

                break;
            }

            case LORA_PHY_TX:
            {
                // in tx state we are going to wait until tx is complete
                // stay in tx state until tx is done
                if (!xSemaphoreTake(obj->fsm_tx_done, pdMS_TO_TICKS(5000)))
                {
                    // fail to transmit, then enter idle state
                    obj->fsm_state = LORA_PHY_IDLE;

                    //DRV_ASSERT(false);
                }
                else
                {
                    // if the packet transmitted is an ACK, then we don't need to wait for ack for the ack
                    subg_packet_v2_cmd_t * cmd_packet = (subg_packet_v2_cmd_t *) obj->retransmit.prev_packet.buffer;
                    if (cmd_packet->header.packet_type == SUBG_PACKET_V2_TYPE_CMD &&
                            cmd_packet->command == SUBG_PACKET_V2_CMD_ACK)
                    {
                        obj->fsm_state = LORA_PHY_IDLE;
                    }
                    else
                    {
                        // starts rx receive window
                        radio_rfm9x_set_opmode_rx(obj->radio);

                        // starts rx timer
                        xTimerStart(obj->retransmit.rx_window_timer, portMAX_DELAY);

                        // setup done, then enter rx state
                        obj->fsm_state = LORA_PHY_RX;
                    }
                }

                break;
            }

            case LORA_PHY_RX:
            {
                // wait for packets to arrive
                lora_phy_msg_t rx_msg;
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
                        obj->fsm_state = LORA_PHY_IDLE;

                        subg_packet_v2_cmd_t * cmd_packet = (subg_packet_v2_cmd_t *) rx_msg.buffer;

                        // if the acknowledgement is purely an acknowledgement, then we don't want to ack the packet
                        if (cmd_packet->command == SUBG_PACKET_V2_CMD_ACK)
                        {
                            break;
                        }
                    }

                    // ack the received packet
                    {
                        lora_phy_msg_t tx_msg;
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
                        lora_phy_send_pri(obj, &tx_msg, true);
                    }
                }
            }
        }
    }
}

void lora_phy_init(lora_phy_t * obj, radio_rfm9x_t * radio, uint8_t device_id8)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(radio);

    // copy variables
    obj->radio = radio;
    obj->device_id8 = device_id8;
    obj->local_seq_id = 0;

    // setup retransmit
    obj->retransmit.rx_window_timer = xTimerCreate("rx_timer", pdMS_TO_TICKS(1000), pdFALSE, obj, lora_phy_on_rx_window_timeout);

    // set radio to sleep default state
    radio_rfm9x_set_opmode_sleep(obj->radio);

    // register callback function
    radio_rfm9x_set_rx_done_isr_callback(obj->radio, (void *) lora_phy_on_rx_done_isr, obj);
    radio_rfm9x_set_tx_done_isr_callback(obj->radio, (void *) lora_phy_on_tx_done_isr, obj);
    radio_rfm9x_set_rx_error_isr_callback(obj->radio, (void *) lora_phy_on_rx_error_isr, obj);
    radio_rfm9x_set_rx_timeout_isr_callback(obj->radio, (void *) lora_phy_on_rx_timeout_isr, obj);

    // configure state machine
    obj->fsm_state = LORA_PHY_IDLE;
    obj->tx_queue = xQueueCreate(4, sizeof(lora_phy_msg_t));
    obj->rx_queue_pri = xQueueCreate(4, sizeof(lora_phy_msg_t));
    obj->rx_queue = xQueueCreate(4, sizeof(lora_phy_msg_t));

    obj->fsm_tx_done = xSemaphoreCreateBinary();

    xTaskCreate((void *) lora_phy_fsm_thread, "lora_phy", 500, obj, 3, &obj->fsm_thread_handler);

    DRV_ASSERT(obj->tx_queue);
    DRV_ASSERT(obj->rx_queue_pri);
    DRV_ASSERT(obj->rx_queue);
    DRV_ASSERT(obj->fsm_tx_done);
}

bool lora_phy_send_timeout(lora_phy_t * obj, lora_phy_msg_t * msg, uint32_t timeout_ms)
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

bool lora_phy_recv_timeout(lora_phy_t * obj, lora_phy_msg_t * msg, uint32_t timeout_ms)
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
