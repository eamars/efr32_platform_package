/**
 * @brief Implementation of I2C EEPROM driver
 * @file eeprom_cat24c16.h
 * @author Ran Bao
 * @date Sept, 2017
 */

#ifndef EEPROM_CAT24C16_H
#define EEPROM_CAT24C16_H

#include <stdint.h>
#include <stdbool.h>

#include "i2cdrv.h"
#include "pio_defs.h"

/**
 * @brief Thread safe read/write operation guarded by mutex
 *
 * If this module is included in a FreeRTOS working environment, please set EEPROM_CAT24C16_USE_MUTEX to 1 in build script
 */
#if EEPROM_CAT24C16_USE_MUTEX == 1
	#include "FreeRTOS.h"
	#include "semphr.h"
#endif

/**
 * @brief Each page have 16 bytes, as specified in datasheet
 */
#define EEPROM_CAT24C16_BLOCK_SIZE 16
#define EEPROM_CAT24C16_PAGE_SIZE EEPROM_CAT24C16_BLOCK_SIZE

typedef struct
{
	i2cdrv_t * i2c_device;
	pio_t enable;
	bool initialized;

#if EEPROM_CAT24C16_USE_MUTEX == 1
	SemaphoreHandle_t access_mutex;
#endif

} eeprom_cat24c16_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize CAT24C16 EEPROM chip
 * @param obj EEPROM device object
 * @param i2c_device I2C device instance
 * @param enable load switch pin
 */
void eeprom_cat24c16_init(eeprom_cat24c16_t * obj, i2cdrv_t * i2c_device, pio_t enable);

/**
 * @brief Deinitialize CAT24C16 EEPROM
 * @param obj EEPROM device object
 */
void eeprom_cat24c16_deinit(eeprom_cat24c16_t * obj);

/**
 * @brief Write a page to EEPROM chip, block if writing is not complete. The page size is defined in @see EEPROM_CAT24C16_PAGE_SIZE
 * @param obj EEPROM device object
 * @param location internal address of EEPROM (0-2048 bytes)
 * @param buffer pointer to the data
 */
void eeprom_cat24c16_page_write(eeprom_cat24c16_t * obj, uint16_t location, void * buffer);

/**
 * @brief Write a byte to EEPROM chip
 * @param obj EEPROM device object
 * @param location internal address of EEPROM (0-2048 bytes)
 * @param byte 1 byte data
 */
void eeprom_cat24c16_byte_write(eeprom_cat24c16_t * obj, uint16_t location, uint8_t byte);

/**
 * @brief Read data from random location in EEPROM
 * @param obj EEPROM device object
 * @param location internal address of EEPROM (0-2048 bytes)
 * @param length number of bytes expected to read
 * @param buffer received data buffer
 */
void eeprom_cat24c16_selective_read(eeprom_cat24c16_t * obj, uint16_t location, uint16_t length, void * buffer);

#ifdef __cplusplus
}
#endif


#endif //EFR32_FIRMWARE_EEPROM_CAT24C16_H
