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

#include "uartdrv.h"
#include "io_device.h"
#include "pio_defs.h"

#include "FreeRTOS.h"
#include "queue.h"


typedef struct
{
    uint16_t head;
    uint16_t tail;
    volatile uint16_t used;
    uint16_t size;
    UARTDRV_Buffer_t fifo[EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS];
} serial_rx_buffer_t;

typedef struct
{
    uint16_t head;
    uint16_t tail;
    volatile uint16_t used;
    uint16_t size;
    UARTDRV_Buffer_t fifo[EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS];
} serial_tx_buffer_t;

typedef struct
{
    io_device base;

    UARTDRV_HandleData_t handle_data;

    pio_t tx;
    pio_t rx;

    bool use_leuart;

    xQueueHandle on_tx_done_queue;
    xQueueHandle on_rx_done_queue;

    volatile serial_rx_buffer_t rx_buffer_queue;
    volatile serial_tx_buffer_t tx_buffer_queue;

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
