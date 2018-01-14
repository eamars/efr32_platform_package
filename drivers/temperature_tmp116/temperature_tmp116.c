/**
 * @brief TMP116 temperature sensor driver
 * @file temperature_tmp116.c
 * @author Ran Bao, Connor Janie
 * @date Sept, 2017
 */

#include <stddef.h>

#include "em_device.h"
#include "em_gpio.h"
#include "em_cmu.h"

#include "i2cdrv.h"
#include "temperature_tmp116_reg.h"
#include "temperature_tmp116.h"
#include "gpiointerrupt.h"
#include "drv_debug.h"
#include "bits.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"




static uint16_t temperature_tmp116_read_word_pri(temperature_tmp116_t * obj, uint8_t reg)
{
	uint16_t word;

	I2C_TransferReturn_TypeDef ret;
	ret = i2cdrv_master_read_iaddr(obj->i2c_device, TMP116_I2C_ADDR, reg, &word, sizeof(uint16_t));

	DRV_ASSERT(ret == i2cTransferDone);

	return word;
}

static void temperature_tmp116_write_word_pri(temperature_tmp116_t * obj, uint8_t reg, uint16_t word)
{
	I2C_TransferReturn_TypeDef ret;
	ret = i2cdrv_master_write_iaddr(obj->i2c_device, TMP116_I2C_ADDR, reg, &word, sizeof(uint16_t));

	DRV_ASSERT(ret == i2cTransferDone);
}

static void temperature_tmp116_alert_isr_pri(uint8_t pin)
{

}

void temperature_tmp116_init(temperature_tmp116_t * obj, i2cdrv_t * i2c_device, pio_t alert)
{
	DRV_ASSERT(obj);
	DRV_ASSERT(i2c_device);

	// reset dirty pin
	obj->is_local_config_cache_dirty = false;

	// assign i2c object
	obj->i2c_device = i2c_device;

	// store alert pin
	obj->alert = alert;

	// enable gpio clock
	CMU_ClockEnable(cmuClock_GPIO, true);

	// configure interrupt manager
	GPIOINT_Init();
	GPIOINT_CallbackRegister(PIO_PIN(obj->alert),temperature_tmp116_alert_isr_pri);

	// configure port interrupt
	GPIO_PinModeSet(PIO_PORT(obj->alert), PIO_PIN(obj->alert),
	                gpioModeInput, 1 // <-- TMP_ALERT is open drained with pull up resister
	);
	GPIO_ExtIntConfig(PIO_PORT(obj->alert), PIO_PIN(obj->alert), PIO_PIN(obj->alert),
	                  false, true, true
	);

	// apply default settings
	// read configuration from sensor
	temperature_tmp116_pull_config(obj);

	// set default high threshold
	temperature_tmp116_set_high_limit(obj, TEMPERATURE_TMP116_HIGH_LIMIT_DEFAULT);
	temperature_tmp116_set_low_limit(obj, TEMPERATURE_TMP116_LOW_LIMIT_DEFAULT);

	// enable alert mode (ALERT is then used to notify when temperature exceeded the threshold)
	BITS_CLEAR(obj->local_config_cache, 1 << _TMP116_REG_CONFIGURATION_TNA_SHIFT);

	// set ALERT line active low
	BITS_CLEAR(obj->local_config_cache, 1 << _TMP116_REG_CONFIGURATION_POL_SHIFT);

	// configure ALERT to represent alert event
	BITS_CLEAR(obj->local_config_cache, 1 << _TMP116_REG_CONFIGURATION_DRALERT_SHIFT);

	temperature_tmp116_push_config(obj);

	// creates the temperature adjusting queue (this is here instead of the init as the temp code needs the int to initialised first)
	xTaskCreate((void *) temperature_tmp116_reader, "temp_reader", 200, obj, 2, NULL);
}

void temperature_tmp116_deinit(temperature_tmp116_t * obj)
{
	DRV_ASSERT(obj);

	// disable the interrupt
	GPIO_ExtIntConfig(PIO_PORT(obj->alert), PIO_PIN(obj->alert), PIO_PIN(obj->alert),
	                  false, true, false
	);
	GPIO_PinModeSet(PIO_PORT(obj->alert), PIO_PIN(obj->alert),
	                gpioModeDisabled, 1 // <-- TMP_ALERT is open drained with pull up resister
	);

	// unregister from interrupt manager
	GPIOINT_CallbackUnRegister(PIO_PIN(obj->alert));

	// revoke assigned variables
	obj->i2c_device = NULL;
	obj->alert = NC;
}

void temperature_tmp116_set_high_limit(temperature_tmp116_t * obj, float high_threshold)
{
	DRV_ASSERT(obj);

	// convert temperature to bytes
	int16_t threshold = (int16_t) (high_threshold / TMP116_TEMPERATURE_RESOLUTION);

	// write to sensor
	temperature_tmp116_write_word_pri(obj, TMP116_REG_ADDR_HIGH_LIMIT, (uint16_t) threshold);
}


void temperature_tmp116_set_low_limit(temperature_tmp116_t * obj, float low_threshold)
{
	DRV_ASSERT(obj);

	// convert temperature to bytes
	int16_t threshold = (int16_t) (low_threshold / TMP116_TEMPERATURE_RESOLUTION);

	// write to sensor
	temperature_tmp116_write_word_pri(obj, TMP116_REG_ADDR_LOW_LIMIT, (uint16_t) threshold);
}


float temperature_tmp116_get_high_limit(temperature_tmp116_t * obj)
{
	DRV_ASSERT(obj);

	return TMP116_TEMPERATURE_RESOLUTION * temperature_tmp116_read_word_pri(obj, TMP116_REG_ADDR_HIGH_LIMIT);
}


float temperature_tmp116_get_low_limit(temperature_tmp116_t * obj)
{
	DRV_ASSERT(obj);

	return TMP116_TEMPERATURE_RESOLUTION * temperature_tmp116_read_word_pri(obj, TMP116_REG_ADDR_LOW_LIMIT);
}


int16_t temperature_tmp116_read_temperature_raw(temperature_tmp116_t * obj)
{
	DRV_ASSERT(obj);

	int16_t raw_temp = temperature_tmp116_read_word_pri(obj, TMP116_REG_ADDR_TEMPERATURE);

	int16_t raw_temp_shifted = (raw_temp >> 8) | (raw_temp << 8);

	return raw_temp_shifted;
}


float temperature_tmp116_read_temperature(temperature_tmp116_t * obj)
{
	float temp = TMP116_TEMPERATURE_RESOLUTION * temperature_tmp116_read_temperature_raw(obj);

	return temp;
}


void temperature_tmp116_set_conversion_mode(temperature_tmp116_t * obj, temperature_tmp116_functional_mode_t mode)
{
	DRV_ASSERT(obj);

	// TODO: Finish this
}


void temperature_tmp116_pull_config(temperature_tmp116_t * obj)
{
	DRV_ASSERT(obj);

	// you should never pull changes when local cache is dirty
	DRV_ASSERT(!obj->is_local_config_cache_dirty);

	obj->local_config_cache = temperature_tmp116_read_word_pri(obj, TMP116_REG_ADDR_CONFIGURATION);
}


void temperature_tmp116_push_config(temperature_tmp116_t * obj)
{
	DRV_ASSERT(obj);

	// only transmit data to sensor if the local cache is dirty
	if (obj->is_local_config_cache_dirty)
	{
		temperature_tmp116_write_word_pri(obj, TMP116_REG_ADDR_CONFIGURATION, obj->local_config_cache);
		obj->is_local_config_cache_dirty = false;
	}
}

static void temperature_tmp116_reader(temperature_tmp116_t * obj)
{
	portTickType xLastWakeTime;

	float temperature = 0;

	xLastWakeTime = xTaskGetTickCount();
	while(1)
	{
		temperature = temperature_tmp116_read_temperature(obj);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
	}


}
