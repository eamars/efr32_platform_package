/**
 * @brief RFM9x LoRa/FSK module driver
 */

#if USE_FREERTOS == 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "spidrv.h"
#include "radio_rfm9x.h"
#include "radio_rfm9x_regs.h"
#include "drv_debug.h"
#include "delay.h"
#include "bits.h"
#include "gpiointerrupt.h"
#include "irq.h"
#include "yield.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

extern const pio_map_t spi_mosi_map[];
extern const pio_map_t spi_miso_map[];
extern const pio_map_t spi_clk_map[];
extern const pio_map_t spi_cs_map[];


/**
 * @brief Assert chip select line for transceiver
 *
 * Note: internal function
 *
 * @param obj the transceiver object
 */
static inline void radio_rfm9x_cs_assert_pri(radio_rfm9x_t * obj)
{
	// enter critical section
	if (__IS_INTERRUPT())
		DRV_ASSERT(xSemaphoreTakeFromISR(obj->spi_access_mutex, NULL));
	else
		DRV_ASSERT(xSemaphoreTake(obj->spi_access_mutex, portMAX_DELAY));

	GPIO_PinOutClear(PIO_PORT(obj->cs), PIO_PIN(obj->cs));
}


/**
 * @brief De-assert chip select line for transceiver
 * @param obj the transceiver object
 */
static inline void radio_rfm9x_cs_deassert_pri(radio_rfm9x_t * obj)
{
	GPIO_PinOutSet(PIO_PORT(obj->cs), PIO_PIN(obj->cs));

	// exit critical section
	if (__IS_INTERRUPT())
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		DRV_ASSERT(xSemaphoreGiveFromISR(obj->spi_access_mutex, &xHigherPriorityTaskWoken));
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
	else
		DRV_ASSERT(xSemaphoreGive(obj->spi_access_mutex));
}


/**
 * @brief Send data to transceiver
 * @param obj the transceiver object
 * @param addr internal register address
 * @param buffer generic data buffer (input)
 * @param size buffer size (in bytes)
 */
static inline void radio_rfm9x_write_pri(radio_rfm9x_t * obj, uint8_t addr, void * buffer, uint8_t size)
{
	// assert the line
	radio_rfm9x_cs_assert_pri(obj);

	// set write mode (MSB=1)
	addr |= 0x80;

	// transmit address
	SPIDRV_MTransmitB(&obj->spi_handle_data, &addr, 1);

	// transmit payload
	SPIDRV_MTransmitB(&obj->spi_handle_data, buffer, size);

	// deassert the line
	radio_rfm9x_cs_deassert_pri(obj);
}


/**
 * @brief Read data from transceiver
 * @param obj the transceiver object
 * @param addr internal register address
 * @param buffer generic data buffer (output)
 * @param size number of bytes wanted to read from transceiver
 */
static inline void radio_rfm9x_read_pri(radio_rfm9x_t * obj, uint8_t addr, void * buffer, uint8_t size)
{
	// assert the line
	radio_rfm9x_cs_assert_pri(obj);

	// set read mode (MSB=0)
	addr &= 0x7F;

	// transmit address
	SPIDRV_MTransmitB(&obj->spi_handle_data, &addr, 1);

	// read data
	SPIDRV_MReceiveB(&obj->spi_handle_data, buffer, size);

	// deassert the line
	radio_rfm9x_cs_deassert_pri(obj);
}


/**
 * @brief Write a word to internal register
 * @param obj the transceiver object
 * @param addr register address
 * @param value word
 */
static inline void radio_rfm9x_reg_write_pri(radio_rfm9x_t * obj, uint8_t addr, uint8_t value)
{
	radio_rfm9x_write_pri(obj, addr, &value, 1);
}


/**
 * @brief Read a word from internal register
 * @param obj the transceiver object
 * @param addr address of internal register
 * @return word
 */
static inline uint8_t radio_rfm9x_reg_read_pri(radio_rfm9x_t * obj, uint8_t addr)
{
	uint8_t reg = 0;
	radio_rfm9x_read_pri(obj, addr, &reg, 1);

	return reg;
}


/**
 * @brief Modify a word of a specific internal register (read-write operation)
 * @param obj the transceiver object
 * @param addr address of internal register
 * @param value word
 * @param mask bit mask for bits need to be changed in a word
 */
static inline void radio_rfm9x_reg_modify_pri(radio_rfm9x_t * obj, uint8_t addr, uint8_t value, uint8_t mask)
{
	// perform a write on modify operation
	uint8_t reg = 0;

	// read from module
	reg = radio_rfm9x_reg_read_pri(obj, addr);

	// set bit with corresponding bit mask
	BITS_MODIFY(reg, value, mask);

	// write back
	radio_rfm9x_reg_write_pri(obj, addr, reg);
}


/**
 * @brief Generically, set the transceiver operating mode
 * @param obj the transceiver object
 * @param opmode transceiver operating mode @see radio_rfm9x_op_t
 */
static inline void radio_rfm9x_set_opmode_pri(radio_rfm9x_t * obj, radio_rfm9x_op_t opmode)
{
	radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, (uint8_t) opmode, RH_RF95_MODE);
}


/**
 * @brief Read current transceiver operating mode
 * @param obj the transceiver object
 * @return current transceiver operating mode @see radio_rfm9x_op_t
 */
static inline radio_rfm9x_op_t radio_rfm9x_get_opmode_pri(radio_rfm9x_t * obj)
{
	return (radio_rfm9x_op_t) (radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_01_OP_MODE) & RH_RF95_MODE);
}


/**
 * @brief Toggle the transceiver mode to stand by
 *
 * Note: low power, ready to switch states, passive data sensing is not available
 *
 * @param obj the transciever
 */
static inline void radio_rfm9x_set_opmode_stdby_pri(radio_rfm9x_t * obj)
{
	if (obj->radio_op_state != RADIO_RFM9X_OP_STDBY)
	{
		radio_rfm9x_set_opmode_pri(obj, RADIO_RFM9X_OP_STDBY);

		obj->radio_op_state = RADIO_RFM9X_OP_STDBY;
	}
}


/**
 * @brief Toggle the transceiver mode to Tx (Transmit)
 * @param obj the transceiver object
 */
static inline void radio_rfm9x_set_opmode_tx_pri(radio_rfm9x_t * obj)
{
	if (obj->radio_op_state != RADIO_RFM9X_OP_TX)
	{
		// the application takes the responsible for

		// set opmode and interrupt source
		radio_rfm9x_set_opmode_pri(obj, RADIO_RFM9X_OP_TX);
		radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_40_DIO_MAPPING1, 0x01 << 6, 0xC0); // interrupt on tx done

		obj->radio_op_state = RADIO_RFM9X_OP_TX;
	}
}


/**
 * @brief Toggle the transceiver mode to Rx (active continuous receive)
 * @param obj the transceiver object
 */
static inline void radio_rfm9x_set_opmode_rx_pri(radio_rfm9x_t * obj)
{
	if (obj->radio_op_state != RADIO_RFM9X_OP_RX)
	{
		// set opmode and interrupt source
		radio_rfm9x_set_opmode_pri(obj, RADIO_RFM9X_OP_RX);
		radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_40_DIO_MAPPING1, 0x00 << 6, 0xC0); // interrupt on rx done

		obj->radio_op_state = RADIO_RFM9X_OP_RX;
	}
}


/**
 * @brief Toggle the transceiver mode to Sleep (low power mode)
 * @param obj the transceiver
 */
static inline void radio_rfm9x_set_opmode_sleep_pri(radio_rfm9x_t * obj)
{
	if (obj->radio_op_state != RADIO_RFM9X_OP_SLEEP)
	{
		radio_rfm9x_set_opmode_pri(obj, RADIO_RFM9X_OP_SLEEP);

		obj->radio_op_state = RADIO_RFM9X_OP_SLEEP;
	}
}


/**
 * @brief Tell if the channel is free by measuring channel RSSI
 * @param obj the transceiver object
 * @return True if the channel is free, False otherwise
 */
static inline bool radio_rfm9x_is_channel_available_pri(radio_rfm9x_t * obj)
{
	int16_t rssi = (int16_t) (RH_RSSI_OFFSET + radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_1B_RSSI_VALUE));

	return rssi <= RADIO_RFM9X_CHANNEL_RSSI_THRESHOLD;
}


/**
 * @brief DIO0 raising edge interrupt handler
 *
 * @param pin interrupt pin number
 * @param obj the transciever object
 */
static void radio_rfm9x_dio0_isr_pri(uint8_t pin, radio_rfm9x_t * obj)
{
	// read interrupt register
	volatile uint8_t reg_irq_flags = 0;
	reg_irq_flags = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_12_IRQ_FLAGS);

	// clear all flags
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_12_IRQ_FLAGS, 0xff);

	// handle the irq in the module
	switch (obj->radio_op_state)
	{
		case RADIO_RFM9X_OP_RX:
		{
			YIELD(
				// if crc error
				if (reg_irq_flags & RH_RF95_PAYLOAD_CRC_ERROR)
				{
					obj->fsm_state = RADIO_RFM9X_FSM_RX_ERROR;

					if (obj->on_rx_error_isr)
						obj->on_rx_error_isr();

					break;
				}

				// if rx timeout error
				if (reg_irq_flags & RH_RF95_RX_TIMEOUT)
				{
					obj->fsm_state = RADIO_RFM9X_FSM_RX_TIMEOUT;

					if (obj->on_rx_timeout_isr)
						obj->on_rx_timeout_isr();

					break;
				}

				// read data packet
				radio_rfm9x_msg_t rx_msg;

				// read available bytes from FIFO
				rx_msg.size = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_13_RX_NB_BYTES);

				// reset the fifo read pointer to the beginning of packet
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR,
				                          radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR)
				);
				radio_rfm9x_read_pri(obj, RH_RF95_REG_00_FIFO, rx_msg.buffer, rx_msg.size);

				// read snr and rssi
				obj->last_packet_rssi = (int16_t) (RH_RSSI_OFFSET + radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_1A_PKT_RSSI_VALUE));
				obj->last_packet_snr = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_19_PKT_SNR_VALUE);
				if (obj->last_packet_snr & 0x80)
				{
					obj->last_packet_snr = (int8_t) ((~obj->last_packet_snr + 1) & 0xff) >> 2;
					obj->last_packet_snr = -obj->last_packet_snr;
				}
				else
				{
					obj->last_packet_snr = (int8_t) (obj->last_packet_snr & 0xff) >> 2;
				}

				// send received data to queue
				DRV_ASSERT(xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, NULL));

				// TODO: optionally, we call interrupt handler directly
				// any modification to the rx buffer will be discarded
				if (obj->on_rx_done_isr)
					obj->on_rx_done_isr(&rx_msg, obj->last_packet_rssi, obj->last_packet_snr);

				// advance the state to RX, allow data to be transmitted or received
				obj->fsm_state = RADIO_RFM9X_FSM_RX_DONE;
			);

			// let the state machine runs
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			DRV_ASSERT(xSemaphoreGiveFromISR(obj->fsm_poll_event, &xHigherPriorityTaskWoken));
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

			break;
		}
		case RADIO_RFM9X_OP_TX:
		{
			YIELD(
				if (reg_irq_flags & RH_RF95_TX_DONE)
				{
					// transmit complete, advance the fsm state to tx_done
					obj->fsm_state = RADIO_RFM9X_FSM_TX_DONE;

					if (obj->on_tx_done_isr)
						obj->on_tx_done_isr();

					// indicate the Tx is ready
					BaseType_t xHigherPriorityTaskWoken = pdFALSE;
					DRV_ASSERT(xSemaphoreGiveFromISR(obj->fsm_tx_done, &xHigherPriorityTaskWoken));
					portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

					break;
				}
			);


			break;
		}
		default:
			break;
	}
}


static void radio_rfm9x_transceiver_fsm(radio_rfm9x_t * obj)
{
	while (1)
	{
		switch (obj->fsm_state)
		{
			case RADIO_RFM9X_FSM_RX_IDLE:
			{
				// the state machine thread stopped as nothing is received or is required to be transmitted
				// the fsm will start running when event occurred, for example, radio ISR or packet is ready to be
				// transmitted
				xSemaphoreTake(obj->fsm_poll_event, portMAX_DELAY);

				// read data from tx queue, checking if data is received
				// if so, then transmit data
				radio_rfm9x_msg_t tx_msg;
				if (xQueueReceive(obj->tx_queue, &tx_msg, 0))
				{
					// have data ready to transmit
#if 0
					// TODO: sense channel if free or not. Ideally this should be implemented by CAD.
					// For simplicity sake, we use channel rssi detection method
					while (!radio_rfm9x_is_channel_available_pri(obj))
					{
						vTaskDelay(pdMS_TO_TICKS(1000));
					}
#endif

					// set fifo pointer
					radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0x00);
					radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

					// declare data length
					radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_22_PAYLOAD_LENGTH, tx_msg.size);

					// transfer data from local to fifo
					radio_rfm9x_write_pri(obj, RH_RF95_REG_00_FIFO, tx_msg.buffer, tx_msg.size);

					// set in tx mode
					radio_rfm9x_set_opmode_tx_pri(obj);

					obj->fsm_state = RADIO_RFM9X_FSM_TX;
				}

				break;
			}

			case RADIO_RFM9X_FSM_TX:
			{
				// stay in tx state until tx is done
				if (!xSemaphoreTake(obj->fsm_tx_done, pdMS_TO_TICKS(5000)))
				{
					// fail to transmit, then enter TX_TIMEOUT STATE
					obj->fsm_state = RADIO_RFM9X_FSM_TX_TIMEOUT;

					if (obj->on_tx_timeout)
						obj->on_tx_timeout();
				}

				break;
			}

			case RADIO_RFM9X_FSM_RX_DONE:
			{
				// reset fifo pointer
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

				// read message received from interrupt
				radio_rfm9x_msg_t rx_msg;
				while (xQueueReceive(obj->rx_queue_pri, &rx_msg, 0))
				{
					xQueueSend(obj->rx_queue, &rx_msg, 0);
				}

				// enter rx_continous mode, the app state enters idle state
				radio_rfm9x_set_opmode_rx_pri(obj);
				obj->fsm_state = RADIO_RFM9X_FSM_RX_IDLE;
				break;
			}

			case RADIO_RFM9X_FSM_TX_DONE:
			{
				// since tx is complete, we can enter rx/idle state now
				// reset fifo pointer
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

				// set radio in rx_continous mode
				radio_rfm9x_set_opmode_rx_pri(obj);
				obj->fsm_state = RADIO_RFM9X_FSM_RX_IDLE;
				break;
			}

			case RADIO_RFM9X_FSM_RX_ERROR:
			{
				// reset fifo pointer
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

				// set radio in rx_continous mode
				radio_rfm9x_set_opmode_rx_pri(obj);
				obj->fsm_state = RADIO_RFM9X_FSM_RX_IDLE;

				break;
			}

			case RADIO_RFM9X_FSM_RX_TIMEOUT:
			{
				// reset fifo pointer
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

				// set radio in rx_continous mode
				radio_rfm9x_set_opmode_rx_pri(obj);
				obj->fsm_state = RADIO_RFM9X_FSM_RX_IDLE;

				break;
			}

			case RADIO_RFM9X_FSM_TX_TIMEOUT:
			{
				// reset fifo pointer
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);
				radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

				// set radio in rx_continous mode
				radio_rfm9x_set_opmode_rx_pri(obj);
				obj->fsm_state = RADIO_RFM9X_FSM_RX_IDLE;

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


void radio_rfm9x_hard_reset(radio_rfm9x_t * obj)
{
	DRV_ASSERT(obj);

	// hold reset line low for 100us
	GPIO_PinOutClear(PIO_PORT(obj->rst), PIO_PIN(obj->rst));
	delay_us(100);

	// release reset line and wait for another 5ms
	GPIO_PinOutSet(PIO_PORT(obj->rst), PIO_PIN(obj->rst));
	delay_ms(5);
}


void radio_rfm9x_init(radio_rfm9x_t * obj,
                      pio_t rst, pio_t miso, pio_t mosi, pio_t clk, pio_t cs,
                      pio_t dio0, pio_t dio1, pio_t dio2, pio_t dio3, pio_t dio4, pio_t dio5
)
{
	uint32_t miso_loc;
	uint32_t mosi_loc;
	uint32_t clk_loc;
	uint32_t cs_loc;
	USART_TypeDef *usart_base;

	SPIDRV_Init_t spi_init_data;

	DRV_ASSERT(obj);

	// clear the memory
	memset(obj, 0x0, sizeof(radio_rfm9x_t));

	// copy variables object
	obj->rst = rst;
	obj->miso = miso;
	obj->mosi = mosi;
	obj->clk = clk;
	obj->cs = cs;
	obj->dio0 = dio0;

	// configure state machine
	obj->fsm_state = RADIO_RFM9X_FSM_RX_IDLE;
	obj->tx_queue = xQueueCreate(4, sizeof(radio_rfm9x_msg_t));
	obj->rx_queue_pri = xQueueCreate(4, sizeof(radio_rfm9x_msg_t));
	obj->rx_queue = xQueueCreate(4, sizeof(radio_rfm9x_msg_t));

	obj->fsm_tx_done = xSemaphoreCreateBinary();
	obj->fsm_poll_event = xSemaphoreCreateBinary();

	// spawn a thread for each RFM9X module for handling data receive/transmit path
	xTaskCreate((void *) radio_rfm9x_transceiver_fsm, "rfm9x_fsm", 200, obj, 3, &obj->fsm_thread_handler);

	DRV_ASSERT(obj->tx_queue);
	DRV_ASSERT(obj->rx_queue_pri);
	DRV_ASSERT(obj->rx_queue);
	DRV_ASSERT(obj->fsm_tx_done);
	DRV_ASSERT(obj->fsm_poll_event);

	// Configure SPI driver
	obj->spi_access_mutex = xSemaphoreCreateMutex();

	// find spi peripheral functions
	DRV_ASSERT(find_pin_function(spi_miso_map, obj->miso, (void **) &usart_base, &miso_loc));
	DRV_ASSERT(find_pin_function(spi_mosi_map, obj->mosi, NULL, &mosi_loc));
	DRV_ASSERT(find_pin_function(spi_clk_map, obj->clk, NULL, &clk_loc));
	DRV_ASSERT(find_pin_function(spi_cs_map, obj->cs, NULL, &cs_loc));

	// configure spi
	spi_init_data.port = usart_base;
	spi_init_data.portLocationTx = (uint8_t) mosi_loc;
	spi_init_data.portLocationRx = (uint8_t) miso_loc;
	spi_init_data.portLocationClk = (uint8_t) clk_loc;
	spi_init_data.portLocationCs = (uint8_t) cs_loc;
	spi_init_data.bitRate = 8000000;
	spi_init_data.frameLength = 8;
	spi_init_data.dummyTxValue = 0;
	spi_init_data.type = spidrvMaster;
	spi_init_data.bitOrder = spidrvBitOrderMsbFirst;
	spi_init_data.clockMode = spidrvClockMode0;
	spi_init_data.csControl = spidrvCsControlApplication;
	spi_init_data.slaveStartMode = spidrvSlaveStartImmediate;

	// initalize spi driver
	SPIDRV_Init(&obj->spi_handle_data, &spi_init_data);

	// configure pios
	GPIO_PinModeSet(PIO_PORT(obj->cs), PIO_PIN(obj->cs), gpioModePushPull, 1);
	GPIO_PinModeSet(PIO_PORT(obj->rst), PIO_PIN(obj->rst), gpioModePushPull, 1);
	GPIO_PinModeSet(PIO_PORT(obj->dio0), PIO_PIN(obj->dio0), gpioModeInput, 0);

	// configure port interrupt
	GPIOINT_Init();
	GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->dio0), (GPIOINT_IrqCallbackPtrWithArgs_t) radio_rfm9x_dio0_isr_pri, (void *) obj);
	GPIO_ExtIntConfig(PIO_PORT(obj->dio0), PIO_PIN(obj->dio0), PIO_PIN(obj->dio0),
	                  true /* raising edge */, false /* falling edge */, true /* enable now */
	);

	/**
	 * @brief Configure radio module
	 */

	// perform hard reset
	radio_rfm9x_hard_reset(obj);

	// read hardware version, read 0 if radio is not present
	uint8_t hw_version = 0;
	hw_version = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_42_VERSION);
	DRV_ASSERT(hw_version);

	// configure pointer for FIFO
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0x00);
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);

	// reset fifo pointer
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

	// set rx mode
	radio_rfm9x_set_opmode_rx_pri(obj);
}


void radio_rfm9x_set_channel(radio_rfm9x_t * obj, uint32_t freq)
{
	DRV_ASSERT(obj);

	freq = ( uint32_t )( ( double )freq / ( double ) RH_RF95_FSTEP );
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_06_FRF_MSB, (uint8_t) ((freq >> 16) & 0xff));
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_07_FRF_MID, (uint8_t) ((freq >> 8) & 0xff));
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_08_FRF_LSB, (uint8_t) (freq & 0xff));
}


void radio_rfm9x_set_modem(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem)
{
	DRV_ASSERT(obj);

	// backup previous transceiver opmode
	radio_rfm9x_op_t prev_opmode = radio_rfm9x_get_opmode_pri(obj);

	// set to sleep mode to allow modem to be switched
	radio_rfm9x_set_opmode_pri(obj, RADIO_RFM9X_OP_SLEEP);

	switch (modem)
	{
		case RADIO_RFM9X_MODEM_FSK:
		{
			radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, RH_RF95_FSK_MODE, 1 << 7);
			break;
		}

		case RADIO_RFM9X_MODEM_LORA:
		{
			radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, RH_RF95_LONG_RANGE_MODE, 1 << 7);
			break;
		}

		default:
			DRV_ASSERT(false);
	}

	// restore previous op mode
	radio_rfm9x_set_opmode_pri(obj, prev_opmode);
}


void radio_rfm9x_set_tx_power_use_rfo(radio_rfm9x_t * obj, int8_t power_dbm, bool use_rfo)
{
	DRV_ASSERT(obj);

	if (use_rfo)
	{
		if (power_dbm > 14) power_dbm = 14;
		if (power_dbm < -1) power_dbm = -1;

		radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_09_PA_CONFIG, (uint8_t) (RH_RF95_MAX_POWER | (power_dbm + 1)));
	}
	else
	{
		if (power_dbm > 23) power_dbm = 23;
		if (power_dbm < 5) power_dbm = 5;

		// For RH_RF95_PA_DAC_ENABLE, manual says '+20dBm on PA_BOOST when OutputPower=0xf'
		// RH_RF95_PA_DAC_ENABLE actually adds about 3dBm to all power levels. We will us it
		// for 21, 22 and 23dBm
		if (power_dbm > 20)
		{
			radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_ENABLE);
			power_dbm -= 3;
		}
		else
		{
			radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_DISABLE);
		}

		// RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
		// pin is connected, so must use PA_BOOST
		// Pout = 2 + OutputPower.
		// The documentation is pretty confusing on this topic: PaSelect says the max power is 20dBm,
		// but OutputPower claims it would be 17dBm.
		// My measurements show 20dBm is correct
		radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_09_PA_CONFIG, (uint8_t) (RH_RF95_PA_SELECT | (power_dbm - 5)));
	}
}


void radio_rfm9x_set_preamble_length(radio_rfm9x_t * obj, uint16_t bytes)
{
	DRV_ASSERT(obj);

	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_20_PREAMBLE_MSB, (uint8_t) (bytes >> 8));
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_21_PREAMBLE_LSB, (uint8_t) (bytes & 0xff));
}


void radio_rfm9x_set_bandwidth(radio_rfm9x_t * obj, radio_rfm9x_bw_t bandwidth)
{
	DRV_ASSERT(obj);

	radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1D_MODEM_CONFIG1, ((uint8_t) bandwidth) << 4, 0xf0); // mask: 0b11110000
}


void radio_rfm9x_set_coding_rate(radio_rfm9x_t * obj, radio_rfm9x_cr_t coding_rate)
{
	DRV_ASSERT(obj);

	radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1D_MODEM_CONFIG1, ((uint8_t) coding_rate) << 1, 0xe); // mask: 0b00001110
}


void radio_rfm9x_set_implicit_header_mode_on(radio_rfm9x_t * obj, bool is_implicit_header)
{
	DRV_ASSERT(obj);

	radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1D_MODEM_CONFIG1, (uint8_t) (is_implicit_header ? 0x1 : 0x0), 0x1);
}


void radio_rfm9x_set_spreading_factor(radio_rfm9x_t * obj, radio_rfm9x_sf_t spreading_factor)
{
	DRV_ASSERT(obj);

	radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1E_MODEM_CONFIG2, ((uint8_t) spreading_factor) << 4, 0xf0);
}


void radio_rfm9x_set_crc_enable(radio_rfm9x_t * obj, bool crc_enable)
{
	DRV_ASSERT(obj);

	radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1E_MODEM_CONFIG2, (uint8_t) (crc_enable ? 0x1 : 0x0) << 2, 0x4);
}


void radio_rfm9x_set_lna(radio_rfm9x_t * obj, uint8_t lna_gain, bool boost_on)
{
	DRV_ASSERT(obj);

	// read lna settings
	uint8_t reg = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_0C_LNA);

	// apply boost_on option
	BITS_MODIFY(reg, (boost_on ? 0x1 : 0x0), 0x1);

	// apply lna gain
	if (lna_gain == 0x0 || lna_gain == 0x7)
	{
		return;
	}
	else
	{
		BITS_MODIFY(reg, lna_gain << 5, 0x7 << 5);
	}

	// write back
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0C_LNA, reg);
}


bool radio_rfm9x_send_timeout(radio_rfm9x_t * obj, radio_rfm9x_msg_t * msg, uint32_t timeout_ms)
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


bool radio_rfm9x_recv_timeout(radio_rfm9x_t * obj, radio_rfm9x_msg_t * msg, uint32_t timeout_ms)
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

#endif // USE_FREERTOS == 1
