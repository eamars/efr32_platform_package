//
// Created by Ran Bao on 1/09/17.
//

#ifndef EEPROM_CAT24C16_H
#define EEPROM_CAT24C16_H

#include <stdint.h>
#include "i2cdrv.h"
#include "pio_defs.h"


#if EEPROM_CAT24C16_USE_MUTEX == 1
	#include "FreeRTOS.h"
	#include "semphr.h"
#endif

#define EEPROM_CAT24C16_BLOCK_SIZE 16

typedef struct
{
	i2cdrv_t * i2c_device;
	pio_t enable;

#if EEPROM_CAT24C16_USE_MUTEX == 1
	SemaphoreHandle_t access_mutex;
#endif

} eeprom_cat24c16_t;

#ifdef __cplusplus
extern "C" {
#endif


void eeprom_cat24c16_init(eeprom_cat24c16_t * obj, i2cdrv_t * i2c_device, pio_t enable);
void eeprom_cat24c16_page_write(eeprom_cat24c16_t * obj, uint16_t location, uint8_t buffer[EEPROM_CAT24C16_BLOCK_SIZE]);
void eeprom_cat24c16_byte_write(eeprom_cat24c16_t * obj, uint16_t location, uint8_t byte);
void eeprom_cat24c16_selective_read(eeprom_cat24c16_t * obj, uint16_t location, uint16_t length, uint8_t buffer[EEPROM_CAT24C16_BLOCK_SIZE]);

#ifdef __cplusplus
}
#endif


#endif //EFR32_FIRMWARE_EEPROM_CAT24C16_H
