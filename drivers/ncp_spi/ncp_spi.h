/**
 * @brief SPI Slave handler for network co-processor
 * @author Ran Bao
 * @date 5/Dec/2017
 * @file ncp_spi.h
 */

#ifndef NCP_SPI_H_
#define NCP_SPI_H_

#include <stdint.h>
#include <stdbool.h>

#include "spidrv.h"
#include "pio_defs.h"
#include "pio_defs.h"

typedef enum
{
    NCP_SPI_STATE_IDLE,
    NCP_SPI_STATE_COMMAND,
    NCP_SPI_STATE_WAIT,
    NCP_SPI_STATE_RESPONSE
} ncp_spi_state_t;

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
    SPIDRV_HandleData_t spi_handle_data;

    // internal state
    ncp_spi_state_t state;



} ncp_spi_t;

void ncp_spi_init(ncp_spi_t * obj,pio_t miso, pio_t mosi, pio_t clk, pio_t cs, pio_t interrupt_out);

#endif // NCP_SPI_H_
