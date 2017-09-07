/**
 * @brief TMP116 temperature sensor driver
 * @file temperature_tmp116.c
 * @author Ran Bao
 * @date Sept, 2017
 */

#include <stddef.h>

#include "em_device.h"
#include "em_gpio.h"
#include "em_cmu.h"

#include "i2cdrv.h"
#include "temperature_tmp116.h"
#include "gpiointerrupt.h"
#include "drv_debug.h"
#include "bits.h"

#define TMP116_TEMPERATURE_RESOLUTION 0.0078125f

/**
 * @brief Depending on ADD0 configuration, four possible slave address can be configured
 *
 * ADD0 --> GND: 0x0
 * ADD0 --> VCC: 0x1
 * ADD0 --> SDA: 0x2
 * ADD0 --> SCL: 0x3
 */
#define _TMP116_I2C_ADDR_SEL 0x0
#define _TMP116_I2C_ADDR_MASK 0x9
#define _TMP116_I2C_ADDR_SHIFT 3
#define TMP116_I2C_BASE_ADDR (_TMP116_I2C_ADDR_MASK << _TMP116_I2C_ADDR_SHIFT)
#define TMP116_I2C_ADDR (TMP116_I2C_BASE_ADDR | _TMP116_I2C_ADDR_SEL)

/**
 * @brief 16 bit temperature data
 */
#define TMP116_REG_ADDR_TEMPERATURE 0x00
#define _TMP116_REG_TEMPERATURE_SHIFT 0x0
#define _TMP116_REG_TEMPERATURE_MASK (0xFFFF << _TMP116_REG_TEMPERATURE_SHIFT)

/**
 * @brief Configurations
 */
#define TMP116_REG_ADDR_CONFIGURATION 0x01
#define _TMP116_REG_CONFIGURATION_HIGH_ALERT_SHIFT 0xF
#define _TMP116_REG_CONFIGURATION_HIGH_ALERT_MASK (1 << _TMP116_REG_CONFIGURATION_HIGH_ALERT_SHIFT)
#define _TMP116_REG_CONFIGURATION_LOW_ALERT_SHIFT 0xE
#define _TMP116_REG_CONFIGURATION_LOW_ALERT_MASK (1 << _TMP116_REG_CONFIGURATION_LOW_ALERT_SHIFT)
#define _TMP116_REG_CONFIGURATION_DATA_READY_SHIFT 0xD
#define _TMP116_REG_CONFIGURATION_DATA_READY_MASK (1 << _TMP116_REG_CONFIGURATION_DATA_READY_SHIFT)
#define _TMP116_REG_CONFIGURATION_EEPROM_BUSY_SHIFT 0xC
#define _TMP116_REG_CONFIGURATION_MOD_SHIFT 0xA
#define _TMP116_REG_CONFIGURATION_MOD_MASK (3 << _TMP116_REG_CONFIGURATION_MOD0_SHIFT)
#define _TMP116_REG_CONFIGURATION_CONV_SHIFT 0x7
#define _TMP116_REG_CONFIGURATION_CONV_MASK (7 << _TMP116_REG_CONFIGURATION_CONV0_SHIFT)
#define _TMP116_REG_CONFIGURATION_AVG_SHIFT 0x5
#define _TMP116_REG_CONFIGURATION_AVG_MASK (3 << _TMP116_REG_CONFIGURATION_AVG0_SHIFT)
#define _TMP116_REG_CONFIGURATION_TNA_SHIFT 0x4
#define _TMP116_REG_CONFIGURATION_TNA_MASK (1 << _TMP116_REG_CONFIGURATION_TNA_SHIFT)
#define _TMP116_REG_CONFIGURATION_POL_SHIFT 0x3
#define _TMP116_REG_CONFIGURATION_POL_MASK (1 << _TMP116_REG_CONFIGURATION_POL_SHIFT)
#define _TMP116_REG_CONFIGURATION_DRALERT_SHIFT 0x2
#define _TMP116_REG_CONFIGURATION_DRALERT_MASK (1 << _TMP116_REG_CONFIGURATION_DRALERT_SHIFT)

/**
 * @brief Temperathre high threshold
 */
#define TMP116_REG_ADDR_HIGH_LIMIT 0x02
#define _TMP116_REG_ADDR_HIGH_LIMIT_SHIFT 0x0
#define _TMP116_REG_ADDR_HIGH_LIMIT_MASK (0xFFFF << _TMP116_REG_ADDR_HIGH_LIMIT_SHIFT)

/**
 * @brief Temperature low threshold
 */
#define TMP116_REG_ADDR_LOW_LIMIT 0x03
#define _TMP116_REG_ADDR_LOW_LIMIT_SHIFT 0x0
#define _TMP116_REG_ADDR_LOW_LIMIT_MASK (0xFFFF << _TMP116_REG_ADDR_LOW_LIMIT_SHIFT)

/**
 * @brief EEPROM unlock register
 */
#define TMP116_REG_ADDR_EEPROM_UNLOCK 0x04
#define _TMP116_REG_EEPROM_UNLOCK_EUN_SHIFT 0xF
#define _TMP116_REG_EEPROM_UNLOCK_EUN_MASK (1 << _TMP116_REG_EEPROM_UNLOCK_EUN_SHIFT)
#define _TMP116_REG_EEPROM_UNLOCK_EEPROM_BUSY_SHIFT 0xE
#define _TMP116_REG_EEPROM_UNLOCK_EEPROM_BUSY_MASK (1 << _TMP116_REG_EEPROM_UNLOCK_EEPROM_BUSY_SHIFT)

/**
 * @brief Internal EEPROM
 */
#define TMP116_REG_ADDR_EEPROM1 0x05
#define TMP116_REG_ADDR_EEPROM2 0x06
#define TMP116_REG_ADDR_EEPROM3 0x07
#define TMP116_REG_ADDR_EEPROM4 0x08

/**
 * @brief Device ID
 */
#define TMP116_REG_ADDR_DEVICE_ID 0x0F
#define TMP116_REG_DEVICE_ID_MASK 0x0FFF
#define TMP116_REG_DEVICE_ID_SHIFT 0x0

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

	// do not reinitialize the device
	if (obj->initialized)
	{
		return;
	}

	// make sure i2c is initialized
	DRV_ASSERT(i2c_device->initialized);

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

	obj->initialized = true;

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
}

void temperature_tmp116_deinit(temperature_tmp116_t * obj)
{
	DRV_ASSERT(obj);

	// if the object is not initialized then we are not going to do anything
	if (!obj->initialized)
	{
		return;
	}

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
	obj->initialized = false;
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

	return temperature_tmp116_read_word_pri(obj, TMP116_REG_ADDR_TEMPERATURE);
}


float temperature_tmp116_read_temperature(temperature_tmp116_t * obj)
{
	return TMP116_TEMPERATURE_RESOLUTION * temperature_tmp116_read_temperature_raw(obj);
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
