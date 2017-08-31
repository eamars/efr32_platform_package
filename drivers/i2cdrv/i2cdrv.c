//
// Created by Ran Bao on 31/08/17.
//
#include <stdint.h>
#include <assert.h>
#include <stddef.h>

#include "em_i2c.h"

#include "em_device.h"
#include "em_cmu.h"

#include "dmadrv.h"

#include "i2cdrv.h"
#include "pio_defs.h"


extern bool halConfigRegisterGpio(GPIO_Port_TypeDef port,
                                  uint8_t pin,
                                  GPIO_Mode_TypeDef pUpMode,
                                  uint8_t pUpOut,
                                  GPIO_Mode_TypeDef pDownMode,
                                  uint8_t pDownOut);


/************I2C SCL***********/
const pio_map_t i2c_scl_map[] = {
		/* I2C0 */
		{PA1,  I2C0,  0},
		{PA2,  I2C0,  1},
		{PA3,  I2C0,  2},
		{PA4,  I2C0,  3},
		{PA5,  I2C0,  4},
		{PB11, I2C0,  5},
		{PB12, I2C0,  6},
		{PB13, I2C0,  7},
		{PB14, I2C0,  8},
		{PB15, I2C0,  9},
		{PC6,  I2C0, 10},
		{PC7,  I2C0, 11},
		{PC8,  I2C0, 12},
		{PC9,  I2C0, 13},
		{PC10, I2C0, 14},
		{PC11, I2C0, 15},
		{PD9,  I2C0, 16},
		{PD10, I2C0, 17},
		{PD11, I2C0, 18},
		{PD12, I2C0, 19},
		{PD13, I2C0, 20},
		{PD14, I2C0, 21},
		{PD15, I2C0, 22},
		{PF0,  I2C0, 23},
		{PF1,  I2C0, 24},
		{PF2,  I2C0, 25},
		{PF3,  I2C0, 26},
		{PF4,  I2C0, 27},
		{PF5,  I2C0, 28},
		{PF6,  I2C0, 29},
		{PF7,  I2C0, 30},
		{PA0,  I2C0, 31},

		{PA7, I2C1, 0},
		{PA8, I2C1, 1},
		{PA9, I2C1, 2},
		{PI2, I2C1, 3},
		{PI3, I2C1, 4},
		{PB6, I2C1, 5},
		{PB7, I2C1, 6},
		{PB8, I2C1, 7},
		{PB9, I2C1, 8},
		{PB10, I2C1, 9},
		{PJ14, I2C1, 10},
		{PJ15, I2C1, 11},
		{PC0, I2C1, 12},
		{PC1, I2C1, 13},
		{PC2, I2C1, 14},
		{PC3, I2C1, 15},
		{PC4, I2C1, 16},
		{PC5, I2C1, 17},
		{PF8, I2C1, 20},
		{PF9, I2C1, 21},
		{PF10, I2C1, 22},
		{PF11, I2C1, 23},
		{PF12, I2C1, 24},
		{PF13, I2C1, 25},
		{PF14, I2C1, 26},
		{PF15, I2C1, 27},
		{PK0, I2C1, 28},
		{PK1, I2C1, 29},
		{PK2, I2C1, 30},
		{PA6, I2C1, 31},

		{NC  , 0   , 0}
};

/************I2C SDA***********/
const pio_map_t i2c_sda_map[] = {
		/* I2C0 */
		{PA0,  I2C0,  0},
		{PA1,  I2C0,  1},
		{PA2,  I2C0,  2},
		{PA3,  I2C0,  3},
		{PA4,  I2C0,  4},
		{PA5,  I2C0,  5},
		{PB11, I2C0,  6},
		{PB12, I2C0,  7},
		{PB13, I2C0,  8},
		{PB14, I2C0,  9},
		{PB15, I2C0, 10},
		{PC6,  I2C0, 11},
		{PC7,  I2C0, 12},
		{PC8,  I2C0, 13},
		{PC9,  I2C0, 14},
		{PC10, I2C0, 15},
		{PC11, I2C0, 16},
		{PD9,  I2C0, 17},
		{PD10, I2C0, 18},
		{PD11, I2C0, 19},
		{PD12, I2C0, 20},
		{PD13, I2C0, 21},
		{PD14, I2C0, 22},
		{PD15, I2C0, 23},
		{PF0,  I2C0, 24},
		{PF1,  I2C0, 25},
		{PF2,  I2C0, 26},
		{PF3,  I2C0, 27},
		{PF4,  I2C0, 28},
		{PF5,  I2C0, 29},
		{PF6,  I2C0, 30},
		{PF7,  I2C0, 31},

		{PA6, I2C1, 0},
		{PA7, I2C1, 1},
		{PA8, I2C1, 2},
		{PA9, I2C1, 3},
		{PI2, I2C1, 4},
		{PI3, I2C1, 5},
		{PB6, I2C1, 6},
		{PB7, I2C1, 7},
		{PB8, I2C1, 8},
		{PB9, I2C1, 9},
		{PB10, I2C1, 10},
		{PJ14, I2C1, 11},
		{PJ15, I2C1, 12},
		{PC0, I2C1, 13},
		{PC1, I2C1, 14},
		{PC2, I2C1, 15},
		{PC3, I2C1, 16},
		{PC4, I2C1, 17},
		{PC5, I2C1, 18},
		{PF8, I2C1, 21},
		{PF9, I2C1, 22},
		{PF10, I2C1, 23},
		{PF11, I2C1, 24},
		{PF12, I2C1, 25},
		{PF13, I2C1, 26},
		{PF14, I2C1, 27},
		{PF15, I2C1, 28},
		{PK0, I2C1, 29},
		{PK1, I2C1, 30},
		{PK2, I2C1, 31},

		/* Not connected */

		{NC  , 0   , 0}
};

void i2cdrv_init(i2cdrv_t *obj, pio_t sda, pio_t scl, pio_t enable)
{
	if (obj->initialized)
	{
		return;
	}

	Ecode_t ret;

	// sanity check
	assert(obj);

	CMU_Clock_TypeDef clock = cmuClock_HFPER;

	// read port and pins
	GPIO_Port_TypeDef sda_port = PIO_PORT(sda);
	uint8_t sda_pin = PIO_PIN(sda);
	GPIO_Port_TypeDef scl_port = PIO_PORT(scl);
	uint8_t scl_pin = PIO_PIN(scl);

	// find base and location
	I2C_TypeDef *scl_base, *sda_base, *base;
	uint32_t sda_loc, scl_loc;

	// find base and pin location
	find_pin_function(i2c_scl_map, scl, (void **) &scl_base, &scl_loc);
	find_pin_function(i2c_sda_map, sda, (void **) &sda_base, &sda_loc);

	assert(scl_base == sda_base);

	// assign base
	base = scl_base;
	assert(base);

	// acquire base
	obj->base = base;

	// acquire clock
	switch((uint32_t) base)
	{
		case (uint32_t) I2C0:
			clock = cmuClock_I2C0;
			break;
		case (uint32_t) I2C1:
			clock = cmuClock_I2C0;
			break;
		default:
			assert(false);
			break;
	}

	// enable the clock
	CMU_ClockEnable(clock, true);

	// set open drain ports for sda and scl
	halConfigRegisterGpio(sda_port, sda_pin,
	                      gpioModeWiredAndPullUp, 1,
	                      gpioModeWiredAndPullUp, 1
	                      // gpioModeDisabled, 0
	);
	halConfigRegisterGpio(scl_port, scl_pin,
	                      gpioModeWiredAndPullUp, 1,
	                      gpioModeWiredAndPullUp, 1
	                      // gpioModeDisabled, 0
	);

	// set enable pin output high when idle and output low when sleep, if enable pin is used
	if (enable != NC)
	{
		halConfigRegisterGpio(PIO_PORT(enable), PIO_PIN(enable),
		                      gpioModePushPull, 1,
		                      gpioModePushPull, 1
		                      // gpioModePushPull, 0
		);
	}

	// reset all slaves
	for (int i = 0; i < 9; i++)
	{
		GPIO_PinOutSet(scl_port, scl_pin);
		GPIO_PinOutClear(scl_port, scl_pin);
	}

	// set pin routing location
	obj->base->ROUTEPEN = (I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN);
	obj->base->ROUTELOC0 = (sda_loc << _I2C_ROUTELOC0_SDALOC_SHIFT) |
			(scl_loc << _I2C_ROUTELOC0_SCLLOC_SHIFT);

	// setup low level driver
	I2C_Init_TypeDef i2c_init_config;

	i2c_init_config.enable = true;
	i2c_init_config.master = true;
	i2c_init_config.freq = I2C_FREQ_STANDARD_MAX;
	i2c_init_config.refFreq = 0;
	i2c_init_config.clhr = i2cClockHLRStandard;

	// apply configuration to low level driver
	I2C_Init(obj->base, &i2c_init_config);

#ifdef ENABLE_I2C_DMA
	// initialize DMA controller
	DMADRV_Init();

	ret = DMADRV_AllocateChannel(&obj->tx_dma_ch, NULL);
	assert(ret != ECODE_OK);

	ret = DMADRV_AllocateChannel(&obj->rx_dma_ch, NULL);
	assert(ret != ECODE_OK);
#endif
	obj->initialized = true;
}
