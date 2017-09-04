//
// Created by Ran Bao on 31/08/17.
//

#ifndef I2CDRV_H
#define I2CDRV_H

#include <stdint.h>
#include <stdbool.h>

#include "em_i2c.h"
#include "pio_defs.h"

#define I2CDRV_DEFAULT_TRANSFER_TIMEOUT 300000UL


#if I2C_USE_DMA == 1
	#include "dmadrv.h"
#endif


#if I2C_USE_MUTEX == 1
	#include "FreeRTOS.h"
	#include "semphr.h"
#endif


typedef struct
{
	bool initialized;
	I2C_TypeDef * base;

	pio_t sda;
	pio_t scl;
	pio_t enable;
#if I2C_USE_DMA == 1
	unsigned int tx_dma_ch;
	unsigned int rx_dma_ch;
#endif

#if I2C_USE_MUTEX == 1
	SemaphoreHandle_t access_mutex;
#endif

} i2cdrv_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize I2C driver
 * @param obj I2C instance
 * @param sda sda pin
 * @param scl scl pin
 * @param enable enable pin
 */
void i2cdrv_init(i2cdrv_t *obj, pio_t sda, pio_t scl, pio_t enable);

/**
 * @brief Initialize a write transfer to I2C slave
 * @param obj I2C instance
 * @param slave_addr slave address, 7 bit, not shifted
 * @param buffer data to transmit
 * @param length length of data in bytes
 * @param timeout_cnt timeout counter, 0 for no timeout
 * @return
 */
I2C_TransferReturn_TypeDef i2cdrv_master_write_timeout(i2cdrv_t *obj, uint8_t slave_addr, void * buffer, uint16_t length,
                                                       uint32_t timeout_cnt);
#define i2cdrv_master_write(obj, slave_addr, buffer, length) \
		i2cdrv_master_write_timeout((obj), (slave_addr), (buffer), (length), I2CDRV_DEFAULT_TRANSFER_TIMEOUT)

/**
 * @brief Initialize a read transfer to I2C slave
 * @param obj I2C instance
 * @param slave_addr slave address, 7 bit, not shifted
 * @param buffer data to receive
 * @param length expected number of bytes to receive
 * @param timeout_cnt timeout counter, 0 for no timeout
 * @return
 */
I2C_TransferReturn_TypeDef i2cdrv_master_read_timeout(i2cdrv_t *obj, uint8_t slave_addr, void * buffer, uint16_t length,
                                                      uint32_t timeout_cnt);
#define i2cdrv_master_read(obj, slave_addr, buffer, length) \
		i2cdrv_master_read_timeout((obj), (slave_addr), (buffer), (length), I2CDRV_DEFAULT_TRANSFER_TIMEOUT)

/**
 * @brief Initialize a write then read transfer to slave
 *
 * @Note: this function is the combination of @see i2cdrv_master_write and @i2cdrv_master_read but, in the real world,
 * it might be faster than calling those two functions
 *
 * @param obj I2C instance
 * @param slave_addr slave address, 7 bit, not shifted
 * @param write_buffer data to transmit
 * @param write_length length of transmit data in bytes
 * @param read_buffer data to receive
 * @param read_length expected number of bytes to receive
 * @param timeout_cnt timeout counter, 0 for no timeout
 * @return
 */
I2C_TransferReturn_TypeDef i2cdrv_master_write_read_timeout(i2cdrv_t *obj, uint8_t slave_addr, void * write_buffer,
                                                            uint16_t write_length, void * read_buffer, uint16_t read_length,
                                                            uint32_t timeout_cnt);
#define i2cdrv_master_write_read(obj, slave_addr, write_buffer, write_length, read_buffer, read_length) \
		i2cdrv_master_write_read_timeout((obj), (slave_addr), (write_buffer), (write_length), (read_buffer), (read_length), I2CDRV_DEFAULT_TRANSFER_TIMEOUT)

/**
 * @brief Initialize a write transfer to slave with internal register address specified (required by some i2c slaves)
 * @param obj I2C instance
 * @param slave_addr slave address, 7 bit, not shifted
 * @param internal_addr internal register address
 * @param buffer data to transmit
 * @param length length of data in bytes
 * @param timeout_cnt timeout counter, 0 for no timeout
 * @return
 */
I2C_TransferReturn_TypeDef i2cdrv_master_write_iaddr_timeout(i2cdrv_t *obj, uint8_t slave_addr, uint8_t internal_addr,
                                                             void * buffer, uint16_t length, uint32_t timeout_cnt);
#define i2cdrv_master_write_iaddr(obj, slave_addr, internal_addr, buffer, length) \
		i2cdrv_master_write_iaddr_timeout((obj), (slave_addr), (internal_addr), (buffer), (length), I2CDRV_DEFAULT_TRANSFER_TIMEOUT)

#ifdef __cplusplus
}
#endif

#endif //I2CDRV_H
