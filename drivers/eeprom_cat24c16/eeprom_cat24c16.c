/**
 * @brief Implementation of I2C EEPROM driver
 * @file eeprom_cat24c16.c
 * @author Ran Bao
 * @date Sept, 2017
 */
#include <stdint.h>
#include <stddef.h>

#include "eeprom_cat24c16.h"
#include "drv_debug.h"
#include "i2cdrv.h"


#define _CAT24C16_I2C_ADDR_MASK 0xA
#define _CAT24C16_I2C_ADDR_SHIFT 3
#define CAT24C16_I2C_BASE_ADDR (_CAT24C16_I2C_ADDR_MASK << _CAT24C16_I2C_ADDR_SHIFT)

/**
 * @brief Enable load switch for EEPROM chip
 *
 * Note: Need to invoke @see eeprom_cat24c16_ack_polling_pri before writing to the chip
 * @param obj EEPROM device object
 */
static void eeprom_cat24c16_enable_pri(eeprom_cat24c16_t * obj)
{
	GPIO_PinOutSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable));
}

/**
 * @brief Disable load switch for EEPROM chip
 *
 * Note: Need to invoke @eeprom_cat24c16_ack_polling_pri to make sure the write cycle is complete before shutting
 * down EEPROM chip
 * @param obj EEPROM device object
 */
static void eeprom_cat24c16_disable_pri(eeprom_cat24c16_t * obj)
{
	GPIO_PinOutClear(PIO_PORT(obj->enable), PIO_PIN(obj->enable));
}

/**
 * @brief Calculate slave address and internal register address with given data address
 * @param eeprom_addr input eeprom data address
 * @param slave_addr output i2c slave address
 * @param internal_addr output internal register address
 */
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

/**
 * @brief Polling the I2C slave until ACK is received
 * @param obj EEPROM device object
 * @param slave_addr i2c slave address
 */
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
	DRV_ASSERT(obj);
	DRV_ASSERT(i2c_device);

	// assign i2c object
	obj->i2c_device = i2c_device;

	// store enable pins
	obj->enable = enable;

	// configure load switch pins
	GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 0);
}

void eeprom_cat24c16_deinit(eeprom_cat24c16_t * obj)
{
	DRV_ASSERT(obj);

	// disable load switch
	GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModeDisabled, 0);

	// revoke assigned variables
	obj->i2c_device = NULL;
	obj->enable = NC;
}

void eeprom_cat24c16_page_write(eeprom_cat24c16_t * obj, uint16_t location, void * buffer)
{
	DRV_ASSERT(obj);

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

	// poll the eeprom until chip is ready (allow data to be written into the memory)
	eeprom_cat24c16_ack_polling_pri(obj, slave_addr);

	// transfer complete
	eeprom_cat24c16_disable_pri(obj);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// exit mutex section
	xSemaphoreGive(obj->access_mutex);
#endif


	DRV_ASSERT(ret == i2cTransferDone);
}

void eeprom_cat24c16_byte_write(eeprom_cat24c16_t * obj, uint16_t location, uint8_t byte)
{
	DRV_ASSERT(obj);

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

	// poll the eeprom until chip is ready (allow data to be written into the memory)
	eeprom_cat24c16_ack_polling_pri(obj, slave_addr);

	// transfer complete
	eeprom_cat24c16_disable_pri(obj);

#if EEPROM_CAT24C16_USE_MUTEX == 1
	// exit mutex section
	xSemaphoreGive(obj->access_mutex);
#endif

	DRV_ASSERT(ret == i2cTransferDone);
}

void eeprom_cat24c16_selective_read(eeprom_cat24c16_t * obj, uint16_t location, uint16_t length, void * buffer)
{
	DRV_ASSERT(obj);

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

	DRV_ASSERT(ret == i2cTransferDone);
}
