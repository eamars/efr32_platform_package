//
// Created by Ran Bao on 1/09/17.
//
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "i2cdrv.h"

#include "eeprom_cat24c16.h"

#define _CAT24C16_I2C_ADDR_MASK 0xA
#define _CAT24C16_I2C_ADDR_SHIFT 3
#define CAT24C16_I2C_BASE_ADDR (_CAT24C16_I2C_ADDR_MASK << _CAT24C16_I2C_ADDR_SHIFT)

static void eeprom_cat24c16_enable_pri(eeprom_cat24c16_t * obj)
{
	GPIO_PinOutSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable));
}

static void eeprom_cat24c16_disable_pri(eeprom_cat24c16_t * obj)
{
	GPIO_PinOutClear(PIO_PORT(obj->enable), PIO_PIN(obj->enable));
}

static void eeprom_cat24c16_get_address_pri(uint16_t eeprom_addr, uint8_t *slave_addr, uint8_t *internal_addr)
{
	/**
	 * @brief The internal layout of EEPROM can be summerized as:
	 *
	 * Section 0 (0-255 bytes)
	 * Section 1 (256-511 bytes)
	 * .
	 * .
	 * .
	 * Section 7 (... - 2047 bytes)
	 *
	 * Section number is used to make up slave address while internal address is the index in each section
	 */

	// figure out which section the eeprom address falls to
	// since eeprom addr ranges between 0 and 2048, section will always smaller or equal to 7
	uint8_t section = (uint8_t) (eeprom_addr / 256);

	// calculate internal register address
	// the internal address always ranges between 0 to 255
	uint8_t section_reg_index = (uint8_t) (eeprom_addr - section * 256);

	// assign return value
	*slave_addr = (uint8_t) (CAT24C16_I2C_BASE_ADDR | section);
	*internal_addr = (uint8_t) section_reg_index;
}

static void eeprom_cat24c16_ack_polling_pri(eeprom_cat24c16_t * obj, uint8_t slave_addr)
{
	I2C_TransferReturn_TypeDef ret;
	while ((ret = i2cdrv_master_write(obj->i2c_device, slave_addr, NULL, 0)) != i2cTransferDone)
	{

	}
}

void eeprom_cat24c16_init(eeprom_cat24c16_t * obj, i2cdrv_t * i2c_device, pio_t enable)
{
	// sanity check for pointers
	assert(obj);
	assert(i2c_device);

	// make sure i2c is initialized
	assert(i2c_device->initialized);

	// assign i2c object
	obj->i2c_device = i2c_device;

	// store enable pins
	obj->enable = enable;

	// configure load switch pins
	GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 0);
}

void eeprom_cat24c16_page_write(eeprom_cat24c16_t * obj, uint16_t location, uint8_t buffer[EEPROM_CAT24C16_BLOCK_SIZE])
{
	I2C_TransferReturn_TypeDef ret;

	// get corresponding slave address and internal address from location information
	uint8_t slave_addr = 0;
	uint8_t internal_addr = 0;
	eeprom_cat24c16_get_address_pri(location, &slave_addr, &internal_addr);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// enter mutex section
	xSemaphoreTake(obj->access_mutex, portMAX_DELAY);
#endif

	// perform a page write sequence operation
	eeprom_cat24c16_enable_pri(obj);

	// poll the eeprom until chip is ready
	eeprom_cat24c16_ack_polling_pri(obj, slave_addr);

	// start transferring data
	ret = i2cdrv_master_write_iaddr(obj->i2c_device, slave_addr, internal_addr, buffer, EEPROM_CAT24C16_BLOCK_SIZE);

	// transfer complete
	eeprom_cat24c16_disable_pri(obj);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// exit mutex section
	xSemaphoreGive(obj->access_mutex);
#endif


	assert(ret == i2cTransferDone);
}

void eeprom_cat24c16_byte_write(eeprom_cat24c16_t * obj, uint16_t location, uint8_t byte)
{
	I2C_TransferReturn_TypeDef ret;

	// get corresponding slave address and internal address from location information
	uint8_t slave_addr = 0;
	uint8_t internal_addr = 0;
	eeprom_cat24c16_get_address_pri(location, &slave_addr, &internal_addr);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// enter mutex section
	xSemaphoreTake(obj->access_mutex, portMAX_DELAY);
#endif

	// perform a page write sequence operation
	eeprom_cat24c16_enable_pri(obj);

	// poll the eeprom until chip is ready
	eeprom_cat24c16_ack_polling_pri(obj, slave_addr);

	// start transferring data
	ret = i2cdrv_master_write_iaddr(obj->i2c_device, slave_addr, internal_addr, &byte, 1);

	// transfer complete
	eeprom_cat24c16_disable_pri(obj);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// exit mutex section
	xSemaphoreGive(obj->access_mutex);
#endif

	assert(ret == i2cTransferDone);
}

void eeprom_cat24c16_selective_read(eeprom_cat24c16_t * obj, uint16_t location, uint16_t length, uint8_t * buffer)
{
	I2C_TransferReturn_TypeDef ret;

	// get corresponding slave address and internal address from location information
	uint8_t slave_addr = 0;
	uint8_t internal_addr = 0;
	eeprom_cat24c16_get_address_pri(location, &slave_addr, &internal_addr);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// enter mutex section
	xSemaphoreTake(obj->access_mutex, portMAX_DELAY);
#endif

	// perform a selective read operation
	eeprom_cat24c16_enable_pri(obj);

	// poll the eeprom until chip is ready
	eeprom_cat24c16_ack_polling_pri(obj, slave_addr);

	// start transferring data
	ret = i2cdrv_master_write_read(obj->i2c_device, slave_addr, &internal_addr, 1, buffer, length);

	// transfer complete
	eeprom_cat24c16_disable_pri(obj);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// exit mutex section
	xSemaphoreGive(obj->access_mutex);
#endif

	assert(ret == i2cTransferDone);
}
