/**
 * @brief Simple PHY layer message transceiver
 * @date 19, Oct, 2017
 * @author Ran Bao
 * @file lora_phy.h
 */

#if USE_FREERTOS == 1

#include "lora_phy.h"

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

	DRV_ASSERT(xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, NULL));

	obj->fsm_state = LORA_PHY_FSM_RX_DONE;
	xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}

static void lora_phy_on_tx_done_isr(lora_phy_t * obj)
{
	// indicate the Tx is ready
	xSemaphoreGiveFromISR(obj->fsm_tx_done, NULL);

	obj->fsm_state = LORA_PHY_FSM_TX_DONE;
	xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}

static void lora_phy_on_rx_error_isr(lora_phy_t * obj)
{
	obj->fsm_state = LORA_PHY_FSM_RX_ERROR;
	xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}

static void lora_phy_on_rx_timeout_isr(lora_phy_t * obj)
{
	obj->fsm_state = LORA_PHY_FSM_RX_TIMEOUT;
	xSemaphoreGiveFromISR(obj->fsm_poll_event, NULL);
}

static void lora_phy_fsm_thread(lora_phy_t * obj)
{
	while (1)
	{
		switch (obj->fsm_state)
		{
			case LORA_PHY_FSM_RX_IDLE:
			{
				// the state machine thread stopped as nothing is received or is required to be transmitted
				// the fsm will start running when event occurred, for example, radio ISR or packet is ready to be
				// transmitted
				xSemaphoreTake(obj->fsm_poll_event, portMAX_DELAY);

				// read data from tx queue, checking if data is queueing
				// if so, then transmit data
				lora_phy_msg_t tx_msg;
				if (xQueueReceive(obj->tx_queue, &tx_msg, 0))
				{
					radio_rfm9x_send(obj->radio, tx_msg.buffer, tx_msg.size);
					obj->fsm_state = LORA_PHY_FSM_TX;
				}

				break;
			}

			case LORA_PHY_FSM_TX:
			{
				// stay in tx state until tx is done
				if (!xSemaphoreTake(obj->fsm_tx_done, pdMS_TO_TICKS(5000)))
				{
					// fail to transmit, then enter TX_TIMEOUT STATE
					obj->fsm_state = LORA_PHY_FSM_TX_TIMEOUT;
				}

				break;
			}

			case LORA_PHY_FSM_TX_DONE:
			{
				// enter rx mode
				radio_rfm9x_set_opmode_sleep(obj->radio);
				obj->fsm_state = LORA_PHY_FSM_RX_IDLE;
			}

			case LORA_PHY_FSM_RX_DONE:
			{
				// read message transmit from interrupt
				lora_phy_msg_t rx_msg;
				while (xQueueReceive(obj->rx_queue_pri, &rx_msg, 0))
				{
					xQueueSend(obj->rx_queue, &rx_msg, 0);
				}

				// enter rx mode
				radio_rfm9x_set_opmode_sleep(obj->radio);
				obj->fsm_state = LORA_PHY_FSM_RX_IDLE;

				break;
			}

			case LORA_PHY_FSM_RX_ERROR:
			{
				// enter rx mode
				radio_rfm9x_set_opmode_sleep(obj->radio);
				obj->fsm_state = LORA_PHY_FSM_RX_IDLE;

				break;
			}

			case LORA_PHY_FSM_RX_TIMEOUT:
			{
				// enter rx mode
				radio_rfm9x_set_opmode_sleep(obj->radio);
				obj->fsm_state = LORA_PHY_FSM_RX_IDLE;

				break;
			}

			case LORA_PHY_FSM_TX_TIMEOUT:
			{
				// enter rx mode
				radio_rfm9x_set_opmode_sleep(obj->radio);
				obj->fsm_state = LORA_PHY_FSM_RX_IDLE;

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

void lora_phy_init(lora_phy_t * obj, radio_rfm9x_t * radio)
{
	DRV_ASSERT(obj);
	DRV_ASSERT(radio);

	// copy variables
	obj->radio = radio;

	// set radio to sleep default state
	radio_rfm9x_set_opmode_sleep(obj->radio);

	// register callback function
	radio_rfm9x_set_rx_done_isr_callback(obj->radio, (void *) lora_phy_on_rx_done_isr, obj);
	radio_rfm9x_set_tx_done_isr_callback(obj->radio, (void *) lora_phy_on_tx_done_isr, obj);
	radio_rfm9x_set_rx_error_isr_callback(obj->radio, (void *) lora_phy_on_rx_error_isr, obj);
	radio_rfm9x_set_rx_timeout_isr_callback(obj->radio, (void *) lora_phy_on_rx_timeout_isr, obj);

	// configure state machine
	obj->fsm_state = LORA_PHY_FSM_RX_IDLE;
	obj->tx_queue = xQueueCreate(4, sizeof(lora_phy_msg_t));
	obj->rx_queue_pri = xQueueCreate(4, sizeof(lora_phy_msg_t));
	obj->rx_queue = xQueueCreate(4, sizeof(lora_phy_msg_t));

	obj->fsm_tx_done = xSemaphoreCreateBinary();
	obj->fsm_poll_event = xSemaphoreCreateBinary();

	xTaskCreate((void *) lora_phy_fsm_thread, "lora_phy", 500, obj, 3, &obj->fsm_thread_handler);

	DRV_ASSERT(obj->tx_queue);
	DRV_ASSERT(obj->rx_queue_pri);
	DRV_ASSERT(obj->rx_queue);
	DRV_ASSERT(obj->fsm_tx_done);
	DRV_ASSERT(obj->fsm_poll_event);
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

	// if the packet is put to the queue then I should run the fsm as soon as possible
	xSemaphoreGive(obj->fsm_poll_event);

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
