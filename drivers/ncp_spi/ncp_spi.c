/**
 * @brief SPI Slave handler for network co-processor
 * @author Ran Bao
 * @date 5/Dec/2017
 * @file ncp_spi.c
 */

#if USE_FREERTOS == 1

#include "ncp_spi.h"
#include "pio_defs.h"
#include "drv_debug.h"
#include "gpiointerrupt.h"

extern const pio_map_t spi_mosi_map[];
extern const pio_map_t spi_miso_map[];
extern const pio_map_t spi_clk_map[];
extern const pio_map_t spi_cs_map[];

static inline bool is_cs_asserted_pri(ncp_spi_t * obj)
{
    return (GPIO_PinInGet(PIO_PORT(obj->spi_slave_pin.cs), PIO_PIN(obj->spi_slave_pin.cs)) == 0);
}

static inline bool is_cs_negated_pri(ncp_spi_t * obj)
{
    return (GPIO_PinInGet(PIO_PORT(obj->spi_slave_pin.cs), PIO_PIN(obj->spi_slave_pin.cs)) != 0);
}

static inline void interrupt_out_set(ncp_spi_t * obj)
{
    GPIO_PinOutSet(PIO_PORT(obj->interrupt_out), PIO_PIN(obj->interrupt_out));
}

static inline void interrupt_out_clear(ncp_spi_t * obj)
{
    GPIO_PinOutClear(PIO_PORT(obj->interrupt_out), PIO_PIN(obj->interrupt_out));
}

static void transfer_done_singleton(SPIDRV_Handle_t handle, Ecode_t transferStatus, int itemsTransferred)
{

}

static void cs_isr_pri(uint8_t pin, ncp_spi_t * obj)
{
    // ignore falling edge interrupt where the host just started transmitting data
    if (is_cs_asserted_pri(obj))
    {
        return;
    }

    // start a DMA request to transfer data from spi to local buffer
    SPIDRV_SReceive(&obj->spi_handle_data, obj->buffer, NCP_SPI_RX_BUFFER_SIZE, NULL, NCP_SPI_TIMEOUT);

    // on rising edge, indicate the thread data is about ready to read
    xSemaphoreGiveFromISR(obj->spi_data_received, NULL);
}



static void slave_data_receive_thread(ncp_spi_t * obj)
{
    while (1)
    {
        // indefinitely block on semaphore
        if (xSemaphoreTake(obj->spi_data_received, portMAX_DELAY))
        {
            int item_transfered = 0, item_remaining = 0;
            SPIDRV_GetTransferStatus(&obj->spi_handle_data, &item_transfered, &item_remaining);

            // if any data is received
            if (item_transfered > 1)
            {
                DEBUG_PRINT("Transfered: %d, Remaining: %d\r\n\r\n", item_transfered, item_remaining);
                switch (obj->buffer[0])
                {
                    case NCP_SPI_FRAME_PROTO_VER:
                    {
                        DEBUG_PRINT("FRAME_PROTOCOL_VER requested\r\n\r\n");
                        // abort current transmission
                        SPIDRV_AbortTransfer(&obj->spi_handle_data);

                        // check for frame terminator
                        if (obj->buffer[1] == NCP_SPI_FRAME_TERMINATOR)
                        {
                            // set transmit buffer
                            obj->buffer[0] = NCP_SPI_VERSION;
                            obj->buffer[1] = NCP_SPI_FRAME_TERMINATOR;

                            // transfer the bytes
                            SPIDRV_STransmit(&obj->spi_handle_data, obj->buffer, 2, NULL, NCP_SPI_TIMEOUT);
                        }
                        break;
                    }
                    default:
                        break;
                }

            }
        }
    }
}

void ncp_spi_init(ncp_spi_t * obj,pio_t miso, pio_t mosi, pio_t clk, pio_t cs, pio_t interrupt_out)
{
    DRV_ASSERT(obj);

    // copy variables
    obj->spi_slave_pin.mosi = mosi;
    obj->spi_slave_pin.miso = miso;
    obj->spi_slave_pin.clk  = clk;
    obj->spi_slave_pin.cs   = cs;
    obj->interrupt_out = interrupt_out;

    // set default state
    obj->state = NCP_SPI_STATE_IDLE;

    // create semaphore that indicates the data is ready
    obj->spi_data_received = xSemaphoreCreateBinary();

    // create thread that handles receive data from the interrupt handler
    xTaskCreate((void *) slave_data_receive_thread, "spi_slave_t", 300, obj, 3, &obj->data_recv_thread_handler);

    // configure spi
    SPIDRV_Init_t spi_init_data;
    USART_TypeDef *usart_base;

    // find spi periperhal functions
    uint32_t mosi_loc, miso_loc, clk_loc, cs_loc;
    DRV_ASSERT(find_pin_function(spi_mosi_map, obj->spi_slave_pin.mosi, (void **) &usart_base, &mosi_loc));
    DRV_ASSERT(find_pin_function(spi_miso_map, obj->spi_slave_pin.miso, NULL, &miso_loc));
    DRV_ASSERT(find_pin_function(spi_clk_map, obj->spi_slave_pin.clk, NULL, &clk_loc));
    DRV_ASSERT(find_pin_function(spi_cs_map, obj->spi_slave_pin.cs, NULL, &cs_loc));

    // reset spi status
    SPIDRV_DeInit(&obj->spi_handle_data);

    // configure spi
    spi_init_data.port = usart_base;
    spi_init_data.portLocationTx = (uint8_t) mosi_loc;
    spi_init_data.portLocationRx = (uint8_t) miso_loc;
    spi_init_data.portLocationClk = (uint8_t) clk_loc;
    spi_init_data.portLocationCs = (uint8_t) cs_loc;
    spi_init_data.bitRate = 0;
    spi_init_data.frameLength = 8;
    spi_init_data.dummyTxValue = 0xff;
    spi_init_data.type = spidrvSlave;
    spi_init_data.bitOrder = spidrvBitOrderMsbFirst;
    spi_init_data.clockMode = spidrvClockMode0;
    spi_init_data.csControl = spidrvCsControlAuto;
    spi_init_data.slaveStartMode = spidrvSlaveStartImmediate;

    // initialize spi instance
    SPIDRV_Init(&obj->spi_handle_data, &spi_init_data);

    // initialize host interrupt (interrupt out) as a GPIO output
    GPIO_PinModeSet(PIO_PORT(obj->interrupt_out), PIO_PIN(obj->interrupt_out), gpioModePushPull, 1);

    // initialize chip select (cs) as interrupt input
    GPIOINT_Init();
    GPIO_PinModeSet(PIO_PORT(obj->spi_slave_pin.cs), PIO_PORT(obj->spi_slave_pin.cs), gpioModeInputPullFilter, 1);
    GPIOINT_CallbackRegisterWithArgs(
            PIO_PIN(obj->spi_slave_pin.cs),
            (GPIOINT_IrqCallbackPtrWithArgs_t) cs_isr_pri,
            (void *) obj
    );
    GPIO_IntConfig(PIO_PORT(obj->spi_slave_pin.cs), PIO_PIN(obj->spi_slave_pin.cs),
                   true /* raising edge */, true /* falling edge */, true /* enable now */
    );

    // compensate noise and crosstalk
    // on some hardware configurations there is a lot of noise and bootloading can fail
    // due to crosstalk. to avoid this, the slewrate is lowered here from 6 to 4, and the
    // drivestrength is lowered from 10mA to 1mA. if noise related errors still occur,
    // the slewrate can be lowered further
    GPIO_SlewrateSet(PIO_PORT(obj->interrupt_out), 4, 4);
    GPIO_DriveStrengthSet(PIO_PORT(obj->interrupt_out), gpioDriveStrengthWeakAlternateStrong);
}


#endif // USE_FREERTOS == 1
