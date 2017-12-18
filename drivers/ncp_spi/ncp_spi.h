/**
 * @brief SPI Slave handler for network co-processor
 * @author Ran Bao
 * @date 5/Dec/2017
 * @file ncp_spi.h
 */

#ifndef NCP_SPI_H_
#define NCP_SPI_H_

#if USE_FREERTOS == 1

#include <stdint.h>
#include <stdbool.h>

#include "spidrv.h"
#include "pio_defs.h"
#include "pio_defs.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define NCP_SPI_RX_BUFFER_SIZE 0xFF
#define NCP_SPI_TIMEOUT 0

#define NCP_SPI_FRAME_TERMINATOR 0xA7
#define NCP_SPI_VERSION          0x82

typedef enum
{
    NCP_SPI_STATE_IDLE,
    NCP_SPI_STATE_COMMAND,
    NCP_SPI_STATE_WAIT,
    NCP_SPI_STATE_RESPONSE
} ncp_spi_state_t;

typedef enum
{
    NCP_SPI_FRAME_PROTO_VER = 0x0A,
    NCP_SPI_FRAME_STATUS = 0x0B,
    NCP_SPI_FRAME_BOOTLOADER_FRAME = 0xFD,
    NCP_SPI_FRAME_EZSP_FRAME = 0xFE,
} ncp_spi_frame_type_t;

typedef struct
{
    // hardware pin
    struct
    {
        pio_t miso;
        pio_t mosi;
        pio_t cs;
        pio_t clk;
    } spi_slave_pin;

    pio_t interrupt_out;

    // spi driver handler
    SPIDRV_HandleData_t handle_data;

    // internal state
    ncp_spi_state_t state;

    uint8_t buffer[NCP_SPI_RX_BUFFER_SIZE];

    // semaphore that indicate the data is received
    TaskHandle_t data_recv_thread_handler;
    SemaphoreHandle_t spi_data_received;

} ncp_spi_t;

void ncp_spi_init(ncp_spi_t * obj, pio_t miso, pio_t mosi, pio_t clk, pio_t cs, pio_t interrupt_out);

#endif // USE_FREERTOS == 1

#endif // NCP_SPI_H_
