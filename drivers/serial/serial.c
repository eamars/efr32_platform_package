/**
 * @brief Wrapper for UARTDRV module, work with FreeRTOS
 * @author Ran Bao
 * @date 14/Dec/2017
 * @file serial/serial.c
 */

#include "serial.h"
#include "drv_debug.h"
#include "uartdrv.h"

#include "FreeRTOS.h"
#include "queue.h"

extern const pio_map_t usart_tx_map[];
extern const pio_map_t usart_rx_map[];

extern const pio_map_t leuart_tx_map[];
extern const pio_map_t leuart_rx_map[];


static void on_uart_tx_done_pri(UARTDRV_Handle_t handle, Ecode_t transfer_status, uint8_t *data,
                                            UARTDRV_Count_t transfer_count)
{
    ssize_t transmitted = 0;

    if (ECODE_OK != transfer_status)
    {
        transmitted = -1;
    }
    else
    {
        transmitted = (ssize_t) transfer_count;
    }

    xQueueSendFromISR(((serial_t *) handle->parent)->on_tx_done_queue, &transmitted, NULL);
}

static void on_uart_rx_done_pri(UARTDRV_Handle_t handle, Ecode_t transfer_status, uint8_t *data,
                                            UARTDRV_Count_t transfer_count)
{
    ssize_t received = 0;

    if (ECODE_OK != transfer_status)
    {
        received = -1;
    }
    else
    {
        received = (ssize_t) transfer_count;
    }

    xQueueSendFromISR(((serial_t *) handle->parent)->on_rx_done_queue, &received, NULL);
}

void serial_init(serial_t * obj, pio_t tx, pio_t rx, uint32_t baud_rate, bool use_leuart)
{
    DRV_ASSERT(obj);

    // copy variables
    obj->tx = tx;
    obj->rx = rx;
    obj->use_leuart = use_leuart;

    // create queue
    obj->on_tx_done_queue = xQueueCreate(1, sizeof(size_t));
    obj->on_rx_done_queue = xQueueCreate(1, sizeof(size_t));

    // initialize fifo
    obj->rx_buffer_queue.head = 0;
    obj->rx_buffer_queue.tail = 0;
    obj->rx_buffer_queue.used = 0;
    obj->rx_buffer_queue.size = EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS;

    obj->tx_buffer_queue.head = 0;
    obj->tx_buffer_queue.tail = 0;
    obj->tx_buffer_queue.used = 0;
    obj->tx_buffer_queue.size = EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS;

    // configure uart
    if (use_leuart)
    {
        // find leuart engine and pin
        uint32_t tx_loc, rx_loc;
        LEUART_TypeDef * leuart_base;

        DRV_ASSERT(find_pin_function(leuart_tx_map, obj->tx, (void **) &leuart_base, &tx_loc));
        DRV_ASSERT(find_pin_function(leuart_rx_map, obj->rx, NULL, &rx_loc));

        // initialize LEUART data
        UARTDRV_InitLeuart_t init_data = {
                .port = leuart_base,
                .baudRate = baud_rate,
                .portLocationTx = (uint8_t) tx_loc,
                .portLocationRx = (uint8_t) rx_loc,
                .stopBits = leuartStopbits1,
                .parity = leuartNoParity,
                .fcType = uartdrvFlowControlNone,
                .rxQueue = (UARTDRV_Buffer_FifoQueue_t *)&obj->rx_buffer_queue,
                .txQueue = (UARTDRV_Buffer_FifoQueue_t *)&obj->tx_buffer_queue,
        };

        UARTDRV_InitLeuart(&obj->handle_data, &init_data);
    }
    else
    {
        // find leuart engine and pin
        uint32_t tx_loc, rx_loc;
        USART_TypeDef * usart_base;

        DRV_ASSERT(find_pin_function(usart_tx_map, obj->tx, (void **) &usart_base, &tx_loc));
        DRV_ASSERT(find_pin_function(usart_rx_map, obj->rx, NULL, &rx_loc));

        // initialize LEUART data
        UARTDRV_InitUart_t init_data = {
                .port = usart_base,
                .baudRate = baud_rate,
                .portLocationTx = (uint8_t) tx_loc,
                .portLocationRx = (uint8_t) rx_loc,
                .stopBits = usartStopbits1,
                .parity = usartNoParity,
                .oversampling = usartOVS16,
                .mvdis = false,
                .fcType = uartdrvFlowControlNone,
                .rxQueue = (UARTDRV_Buffer_FifoQueue_t *)&obj->rx_buffer_queue,
                .txQueue = (UARTDRV_Buffer_FifoQueue_t *)&obj->tx_buffer_queue,
        };

        UARTDRV_InitUart(&obj->handle_data, &init_data);
    }

    // register callback object to the uart handler
    obj->handle_data.parent = obj;
}

ssize_t serial_write_timeout(serial_t * obj, void * buffer, size_t size, uint32_t timeout)
{
    DRV_ASSERT(obj);

    ssize_t transmitted = 0;
    uint32_t block_ticks = 0;

    // calculate timeout
    if (timeout == portMAX_DELAY)
        block_ticks = timeout;
    else
        block_ticks = pdMS_TO_TICKS(timeout);

    // initialize a transmit request
    UARTDRV_Transmit(&obj->handle_data, buffer, size, on_uart_tx_done_pri);

    // block the current thread
    if (!xQueueReceive(obj->on_tx_done_queue, &transmitted, block_ticks))
    {
        // check bytes transmitted
        UARTDRV_Count_t bytes_sent, bytes_remaining;
        UARTDRV_GetTransmitStatus(&obj->handle_data, (void *) &buffer, &bytes_sent, &bytes_remaining);

        // abort any ongoing transmission
        UARTDRV_Abort(&obj->handle_data, uartdrvAbortTransmit);

        transmitted = (ssize_t) bytes_sent;
    }

    return transmitted;
}


ssize_t serial_read_timeout(serial_t * obj, void * buffer, size_t size, uint32_t timeout)
{
    DRV_ASSERT(obj);

    ssize_t received = 0;
    uint32_t block_ticks = 0;

    // calculate timeout
    if (timeout == portMAX_DELAY)
        block_ticks = timeout;
    else
        block_ticks = pdMS_TO_TICKS(timeout);

    // initialize a receive request
    UARTDRV_Receive(&obj->handle_data, buffer, size, on_uart_rx_done_pri);

    // block the current thread
    if (!xQueueReceive(obj->on_rx_done_queue, &received, block_ticks))
    {
        // check bytes received
        UARTDRV_Count_t bytes_received, bytes_remaining;
        UARTDRV_GetReceiveStatus(&obj->handle_data, (void *) &buffer, &bytes_received, &bytes_remaining);

        // abort any ongoing transmission
        UARTDRV_Abort(&obj->handle_data, uartdrvAbortReceive);

        received = (ssize_t) bytes_received;
    }

    return received;
}

ssize_t serial_read_blocking(serial_t * obj, void * buffer, size_t size)
{
    return serial_read_timeout(obj, buffer, size, portMAX_DELAY);
}

ssize_t serial_read_non_blocking(serial_t * obj, void * buffer, size_t size)
{
    return serial_read_timeout(obj, buffer, size, 0);
}

ssize_t serial_write_blocking(serial_t * obj, void * buffer, size_t size)
{
    return serial_write_timeout(obj, buffer, size, portMAX_DELAY);
}

ssize_t serial_write_non_blocking(serial_t * obj, void * buffer, size_t size)
{
    return serial_write_timeout(obj, buffer, size, 0);
}
