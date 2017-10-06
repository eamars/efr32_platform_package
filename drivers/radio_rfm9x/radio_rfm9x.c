/**
 * @brief RFM9x LoRa/FSK module driver
 */

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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern const pio_map_t spi_mosi_map[];
extern const pio_map_t spi_miso_map[];
extern const pio_map_t spi_clk_map[];
extern const pio_map_t spi_cs_map[];

/**
 * @brief WARNING: This ISR is singleton. On EFR platform all radio instance shares the same ISR
 * @param pin interrupt pin number
 */
static void radio_rfm9x_dio0_isr_pri(uint8_t pin, radio_rfm9x_t * obj)
{

}

static void radio_rfm9x_dio0_thread_handler_pri(radio_rfm9x_t * obj)
{
	while (1)
	{

	}
}

static inline void radio_rfm9x_cs_assert_pri(radio_rfm9x_t * obj)
{
	GPIO_PinOutClear(PIO_PORT(obj->cs), PIO_PIN(obj->cs));
}


static inline void radio_rfm9x_cs_deassert_pri(radio_rfm9x_t * obj)
{
	GPIO_PinOutSet(PIO_PORT(obj->cs), PIO_PIN(obj->cs));
}


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


static inline void radio_rfm9x_reg_write_pri(radio_rfm9x_t * obj, uint8_t addr, uint8_t value)
{
	radio_rfm9x_write_pri(obj, addr, &value, 1);
}


static inline uint8_t radio_rfm9x_reg_read_pri(radio_rfm9x_t * obj, uint8_t addr)
{
	uint8_t reg = 0;
	radio_rfm9x_read_pri(obj, addr, &reg, 1);

	return reg;
}


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


static inline void radio_rfm9x_set_opmode_pri(radio_rfm9x_t * obj, radio_rfm9x_op_t opmode)
{
	radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, (uint8_t) opmode, RH_RF95_MODE);
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

	// copy variables object
	obj->rst = rst;
	obj->miso = miso;
	obj->mosi = mosi;
	obj->clk = clk;
	obj->cs = cs;
	obj->dio0 = dio0;

	/**
	 * @brief Create a thread handler for each instance
	 */
	xTaskCreate((void *) radio_rfm9x_dio0_thread_handler_pri, "dio0", 200, (void *) obj, 2, &obj->dio0_thread_handle);

	/**
	 * @brief Configure SPI driver
	 */

	// find spi peripheral functions
	DRV_ASSERT(find_pin_function(spi_miso_map, miso, (void **) &usart_base, &miso_loc));
	DRV_ASSERT(find_pin_function(spi_mosi_map, mosi, NULL, &mosi_loc));
	DRV_ASSERT(find_pin_function(spi_clk_map, clk, NULL, &clk_loc));
	DRV_ASSERT(find_pin_function(spi_cs_map, cs, NULL, &cs_loc));

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

	// set radio to sleep mode and configure it to LoRa mode
	radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP);
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

	// set to sleep mode to allow modem to be switched
	radio_rfm9x_set_opmode_pri(obj, RADIO_RFM9X_OP_SLEEP);

	switch (modem)
	{
		case RADIO_RFM9X_MODEM_FSK:
		{
			radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, RH_RF95_FSK_MODE, 1 << 7);

			// configure DIO mapping for 0-3
			radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_40_DIO_MAPPING1, 0x00);

			// configure DIO mapping for 4-5
			radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_41_DIO_MAPPING2, 0x30);
			break;
		}

		case RADIO_RFM9X_MODEM_LORA:
		{
			radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, RH_RF95_LONG_RANGE_MODE, 1 << 7);

			// configure DIO mapping for 0-3
			radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_40_DIO_MAPPING1, 0x00);

			// configure DIO mapping for 4-5
			radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_41_DIO_MAPPING2, 0x00);
			break;
		}

		default:
			DRV_ASSERT(false);
	}
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
