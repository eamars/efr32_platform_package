/**
 * @brief FreeRTOS wrapper for GPCRC module
 * @author Ran Bao
 * @date 17/Jan/2018
 * @file gpcrc.h
 *
 * It also works with non-freertos application
 */

#ifndef GPCRC_H_
#define GPCRC_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "em_gpcrc.h"

#if USE_FREERTOS == 1
#include "FreeRTOS.h"
#include "semphr.h"
#endif // USE_FREERTOS == 1

typedef struct
{
    GPCRC_TypeDef * gpcrc_engine;

#if USE_FREERTOS == 1
    SemaphoreHandle_t access_mutex;
#endif // USE_FREERTOS == 1
} gpcrc_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize GPCRC module
 * @param obj the gpcrc object
 */
void gpcrc_init(gpcrc_t * obj);

/**
 * @brief Initialize a CRC32 calculation
 * @param obj The GPCRC object
 * @param polynomial The CRC32 polynomial
 * @param init_value The initial value of CRC32, 0xFFFFFFFFUL is the standard CRC32 initial value
 * @param buffer data buffer
 * @param len The length of data buffer in bytes
 * @return CRC32 value
 *
 * For testing, gpcrc_crc32(obj, 0x04C11DB7UL, 0xFFFFFFFFUL, {0xC5}, 1) should give 0x390CD9B2UL
 */
uint32_t gpcrc_crc32(gpcrc_t * obj, uint32_t polynomial, uint32_t init_value, void * buffer, size_t len);

/**
 * @brief Initialize a CRC16 calculation
 * @param obj The GPCRC object
 * @param polynomial The CRC16 polynomial
 * @param init_value The initial value for CRC16
 * @param buffer data buffer
 * @param len The length of data buffer in bytes
 * @return CRC16 value
 *
 * For testing, gpcrc_crc16(&gpcrc, 0x8005, 0x0, {0xAB}, 1) should give 0xBF41
 */
uint16_t gpcrc_crc16(gpcrc_t * obj, uint16_t polynomial, uint16_t init_value, void * buffer, size_t len);

/**
 * @brief Initialize a CRC-CCITT calculation
 * @param obj The GPCRC object
 * @param buffer data buffer
 * @param len The length of data buffer in bytes
 * @return CRC checksum
 *
 * For testing, gpcrc_ccitt(&gpcrc, "123456789", strlen("123456789")) should give 0x29B1
 */
uint16_t gpcrc_ccitt(gpcrc_t * obj, void * buffer, size_t len);

#ifdef __cplusplus
}
#endif



#endif // GPCRC_H_
