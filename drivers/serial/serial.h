/**
 * @brief Wrapper for UARTDRV module, work with FreeRTOS
 * @author Ran Bao
 * @date 14/Dec/2017
 * @file serial/serial.h
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#if USE_FREERTOS == 1

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "uartdrv.h"
#include "io_device.h"
#include "pio_defs.h"

#include "FreeRTOS.h"
#include "semphr.h"


#define SERIAL_BUFFER_SIZE 512

typedef struct
{
    uint8_t buffer[SERIAL_BUFFER_SIZE];
    uint32_t read_idx;
    uint32_t write_idx;
    uint32_t available_bytes;

    SemaphoreHandle_t access_mutex;
} serial_buffer_t;

typedef struct
{
    io_device base;

    pio_t tx;
    pio_t rx;

    bool use_leuart;

    void * uart_engine;

    serial_buffer_t rx_buffer;
    serial_buffer_t tx_buffer;

    struct
    {
        SemaphoreHandle_t block_wait_sem;
        uint32_t more_to_read;
        uint32_t more_bytes_read;
    } block_read;

    struct
    {
        SemaphoreHandle_t block_wait_sem;
    } block_write;

} serial_t;

#ifdef __cplusplus
extern "C" {
#endif

void serial_init(serial_t *obj, pio_t tx, pio_t rx, uint32_t baud_rate, bool use_leuart);

ssize_t serial_write_timeout(serial_t * obj, void * buffer, size_t size, uint32_t timeout);
ssize_t serial_read_timeout(serial_t * obj, void * buffer, size_t size, uint32_t timeout);
ssize_t serial_read_blocking(serial_t * obj, void * buffer, size_t size);
ssize_t serial_read_non_blocking(serial_t * obj, void * buffer, size_t size);
ssize_t serial_write_blocking(serial_t * obj, void * buffer, size_t size);
ssize_t serial_write_non_blocking(serial_t * obj, void * buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // USE_FREERTOS == 1

#endif // SERIAL_H_
