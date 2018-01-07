/**
 * @brief Wrapper for UARTDRV module, work with FreeRTOS
 * @author Ran Bao
 * @date 14/Dec/2017
 * @file serial/serial.c
 */
#if USE_FREERTOS == 1

#include "serial.h"
#include "drv_debug.h"
#include "uartdrv.h"
#include "bits.h"
#include "usartinterrupt.h"

#include "FreeRTOS.h"
#include "queue.h"

// external variables
extern const pio_map_t usart_tx_map[];
extern const pio_map_t usart_rx_map[];

extern const pio_map_t leuart_tx_map[];
extern const pio_map_t leuart_rx_map[];

void serial_tx_irq_handler(serial_t * obj)
{
    // clear the tx interrupt
    volatile uint32_t IF = ((USART_TypeDef *) obj->uart_engine)->IF;

    if (BITS_CHECK(((USART_TypeDef *) obj->uart_engine)->STATUS, USART_STATUS_TXBL))
    {
        if (obj->tx_buffer.available_bytes > 0)
        {
            ((USART_TypeDef *) obj->uart_engine)->TXDATA = obj->tx_buffer.buffer[obj->tx_buffer.read_idx++];

            if (obj->tx_buffer.read_idx == SERIAL_BUFFER_SIZE)
            {
                obj->tx_buffer.read_idx = 0;
            }

            obj->tx_buffer.available_bytes -= 1;
        }
        else // if (obj->tx_buffer.available_bytes == 0)
        {
            // write complete
            if (obj->block_write.block_wait_sem)
            {
                xSemaphoreGiveFromISR(obj->block_write.block_wait_sem, NULL);

                // disable txbl interrupt
                USART_IntDisable(obj->uart_engine, USART_IF_TXBL);
            }
        }
    }
}

void serial_rx_irq_handler(serial_t * obj)
{
    uint8_t byte;

    while (BITS_CHECK(((USART_TypeDef *) obj->uart_engine)->STATUS, USART_STATUS_RXDATAV))
    {
        byte = (uint8_t) ((USART_TypeDef *) obj->uart_engine)->RXDATA;

        // store the byte to local buffer
        obj->rx_buffer.buffer[obj->rx_buffer.write_idx++] = byte;
        obj->rx_buffer.available_bytes += 1;

        // check for overflow
        if (obj->rx_buffer.write_idx == SERIAL_BUFFER_SIZE)
            obj->rx_buffer.write_idx = 0;

        // check for underflow
        if (obj->rx_buffer.read_idx == obj->rx_buffer.write_idx)
            obj->rx_buffer.available_bytes = 0;
        else
        {
            // signal other thread to indicate the bytes is ready
            if (obj->block_read.block_wait_sem)
            {
                obj->block_read.more_bytes_read += 1;
                if (obj->block_read.more_bytes_read == obj->block_read.more_to_read)
                {
                    xSemaphoreGiveFromISR(obj->block_read.block_wait_sem, NULL);
                }
            }
        }
    }
}


void serial_init(serial_t * obj, pio_t tx, pio_t rx, uint32_t baud_rate, bool use_leuart)
{
    DRV_ASSERT(obj);

    // copy variables
    obj->tx = tx;
    obj->rx = rx;
    obj->use_leuart = use_leuart;

    // initialize buffer
    memset(&obj->rx_buffer, 0x0, sizeof(serial_buffer_t));
    memset(&obj->tx_buffer, 0x0, sizeof(serial_buffer_t));

    // create semaphore
    memset(&obj->block_read, 0x0, sizeof(obj->block_read));
    obj->rx_buffer.access_mutex = xSemaphoreCreateMutex();
    obj->tx_buffer.access_mutex = xSemaphoreCreateMutex();
    DRV_ASSERT(obj->rx_buffer.access_mutex);
    DRV_ASSERT(obj->tx_buffer.access_mutex);

    // enable gpio clocks allow configuration
    CMU_ClockEnable(cmuClock_GPIO, true);

    // configure physical port as output and input
    GPIO_PinModeSet(PIO_PORT(tx), PIO_PIN(tx), gpioModePushPull, 1);
    GPIO_PinModeSet(PIO_PORT(rx), PIO_PIN(rx), gpioModeInputPull, 1);

    if (use_leuart)
    {
        uint32_t tx_loc, rx_loc;

        DRV_ASSERT(find_pin_function(leuart_tx_map, obj->tx, &obj->uart_engine, &tx_loc));
        DRV_ASSERT(find_pin_function(leuart_rx_map, obj->rx, NULL, &rx_loc));

        // TODO: Implement LEUART
        DRV_ASSERT(false);
    }
    else
    {
        uint32_t tx_loc, rx_loc;
        CMU_Clock_TypeDef peripheral_clock;
        uint32_t usart_idx;
        IRQn_Type usart_rx_irqn;
        IRQn_Type usart_tx_irqn;

        DRV_ASSERT(find_pin_function(usart_tx_map, obj->tx, &obj->uart_engine, &tx_loc));
        DRV_ASSERT(find_pin_function(usart_rx_map, obj->rx, NULL, &rx_loc));

        // find clocks and register object
        switch ((uint32_t) obj->uart_engine)
        {
            case (uint32_t) USART0:
            {
                peripheral_clock = cmuClock_USART0;
                usart_idx = 0;
                usart_rx_irqn = USART0_RX_IRQn;
                usart_tx_irqn = USART0_TX_IRQn;
                break;
            }
            case (uint32_t) USART1:
            {
                peripheral_clock = cmuClock_USART1;
                usart_idx = 1;
                usart_rx_irqn = USART1_RX_IRQn;
                usart_tx_irqn = USART1_TX_IRQn;
                break;
            }
            case (uint32_t) USART2:
            {
                peripheral_clock = cmuClock_USART2;
                usart_idx = 2;
                usart_rx_irqn = USART2_RX_IRQn;
                usart_tx_irqn = USART2_TX_IRQn;
                break;
            }
            case (uint32_t) USART3:
            {
                peripheral_clock = cmuClock_USART3;
                usart_idx = 3;
                usart_rx_irqn = USART3_RX_IRQn;
                usart_tx_irqn = USART3_TX_IRQn;
                break;
            }
            default:
            {
                peripheral_clock = cmuClock_USART0;
                usart_idx = 0;
                usart_rx_irqn = USART0_RX_IRQn;
                usart_tx_irqn = USART0_TX_IRQn;
                DRV_ASSERT(false);
                break;
            }
        }

        // enable peripheral clock
        CMU_ClockEnable(cmuClock_HFPER, true);
        CMU_ClockEnable(peripheral_clock, true);

        USART_InitAsync_TypeDef init_data = USART_INITASYNC_DEFAULT;
        USART_InitAsync(obj->uart_engine, &init_data);

        // enable IO and location
        BITS_SET(((USART_TypeDef *) obj->uart_engine)->ROUTEPEN, (USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN));
        BITS_MODIFY(
                ((USART_TypeDef *) obj->uart_engine)->ROUTELOC0,
                (tx_loc << _USART_ROUTELOC0_TXLOC_SHIFT) | (rx_loc << _USART_ROUTELOC0_RXLOC_SHIFT),
                _USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK
        );

        // register interrupt callback
        usart_interrupt_callback_register_with_arg(usart_idx, (void *) serial_rx_irq_handler, obj, (void *) serial_tx_irq_handler, obj);

        // discard false frames
        BITS_SET(((USART_TypeDef *) obj->uart_engine)->CMD, USART_CMD_CLEARRX | USART_CMD_CLEARTX);

        // enable interrupt with conditions
        USART_IntEnable(obj->uart_engine, USART_IF_RXDATAV);

        // clear any interrupt from source
        USART_IntClear(obj->uart_engine, 0xFFFFFFFF);

        // enable NVIC interrupt
        NVIC_ClearPendingIRQ(usart_rx_irqn);
        NVIC_EnableIRQ(usart_rx_irqn);
        NVIC_ClearPendingIRQ(usart_tx_irqn);
        NVIC_EnableIRQ(usart_tx_irqn);

        // enable usart
        USART_Enable(obj->uart_engine, usartEnable);
    }
}

ssize_t serial_write_timeout(serial_t * obj, void * buffer, size_t size, uint32_t timeout)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(size <= SERIAL_BUFFER_SIZE);

    ssize_t transmitted = 0;
    uint32_t block_ticks = 0;

    // calculate timeout
    if (timeout == portMAX_DELAY)
        block_ticks = timeout;
    else
        block_ticks = pdMS_TO_TICKS(timeout);

    // write to tx buffer
    uint8_t * byte_array = (uint8_t *) buffer;

    // enter critical section
    xSemaphoreTake(obj->tx_buffer.access_mutex, portMAX_DELAY);

    // go through ch one by one
    for (size_t ch_idx = 0; ch_idx < size; ch_idx++)
    {
        obj->tx_buffer.buffer[obj->tx_buffer.write_idx++] = byte_array[ch_idx];
        obj->tx_buffer.available_bytes += 1;

        // check for overflow
        if (obj->tx_buffer.write_idx == SERIAL_BUFFER_SIZE)
            obj->tx_buffer.write_idx = 0;

        // check for underflow
        if (obj->tx_buffer.read_idx == obj->tx_buffer.write_idx)
            obj->tx_buffer.available_bytes;
    }

    // block wait until transmit done
    DRV_ASSERT(obj->block_write.block_wait_sem == NULL);
    obj->block_write.block_wait_sem = xSemaphoreCreateBinary();

    // enable interrupt
    USART_IntEnable(obj->uart_engine, USART_IF_TXBL);

    // wait until complete
    if (!xSemaphoreTake(obj->block_write.block_wait_sem, block_ticks))
    {
        // failed to transmit before timeout, then disable the interrupt manually
        USART_IntDisable(obj->uart_engine, USART_IF_TXBL);

        // calculate byte transfered
        transmitted = size - obj->tx_buffer.available_bytes;

        // clear the data that failed to transmit
        obj->tx_buffer.available_bytes = 0;
        obj->tx_buffer.read_idx = obj->tx_buffer.write_idx;
    }
    else
    {
        // calculate byte transfered
        transmitted = size - obj->tx_buffer.available_bytes;
    }

    // delete semaphore
    vSemaphoreDelete(obj->block_write.block_wait_sem);
    obj->block_write.block_wait_sem = NULL;

    // leave critical section
    xSemaphoreGive(obj->tx_buffer.access_mutex);

    return transmitted;
}


ssize_t serial_read_timeout(serial_t * obj, void * buffer, size_t size, uint32_t timeout)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(size <= SERIAL_BUFFER_SIZE);

    volatile uint32_t available = obj->rx_buffer.available_bytes;

    // calculate how many bytes still need to read from ring buffer
    if (size > available)
    {
        // create semaphore
        DRV_ASSERT(obj->block_read.block_wait_sem == NULL);
        obj->block_read.block_wait_sem = xSemaphoreCreateBinary();

        // setup read threshold
        obj->block_read.more_to_read = size - available;
        obj->block_read.more_bytes_read = 0;

        // calculate timeout
        uint32_t block_time = 0;
        if (timeout == portMAX_DELAY)
            block_time = timeout;
        else
            block_time = pdMS_TO_TICKS(timeout);

        // block the thread until timeout or enough data received
        xSemaphoreTake(obj->block_read.block_wait_sem, block_time);

        // delete semaphore
        vSemaphoreDelete(obj->block_read.block_wait_sem);
        obj->block_read.block_wait_sem = NULL;
    }

    // find out the current available bytes
    uint32_t bytes_to_read;
    available = obj->rx_buffer.available_bytes;

    bytes_to_read = size <= available ? size : available;

    // if there is no bytes to read, then exit
    if (bytes_to_read == 0)
    {
        return 0;
    }

    // read from circular buffer
    xSemaphoreTake(obj->rx_buffer.access_mutex, portMAX_DELAY);

    // transfer data
    uint8_t * byte_output_ptr = buffer;
    for (uint32_t bytes_left = bytes_to_read; bytes_left != 0; bytes_left--)
    {
        *(byte_output_ptr++) = obj->rx_buffer.buffer[obj->rx_buffer.read_idx++];
        if (obj->rx_buffer.read_idx == SERIAL_BUFFER_SIZE)
        {
            obj->rx_buffer.read_idx = 0;
        }
    }

    obj->rx_buffer.available_bytes -= bytes_to_read;

    // release
    xSemaphoreGive(obj->rx_buffer.access_mutex);

    return (ssize_t) bytes_to_read;
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

#endif // #if USE_FREERTOS == 1
