/**
 * @brief RFM9x LoRa/FSK module driver
 */


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "em_core.h"
#include "spidrv.h"
#include "radio_template.h"
#include "radio_rfm9x.h"
#include "radio_rfm9x_regs.h"
#include "drv_debug.h"
#include "delay.h"
#include "bits.h"
#include "gpiointerrupt.h"
#include "irq.h"
#include "yield.h"

#define RFM9X_PRIVATE_SYNCWORD 0x12
#define RFM9X_PUBLIC_SYNWORD 0x34

extern const pio_map_t spi_mosi_map[];
extern const pio_map_t spi_miso_map[];
extern const pio_map_t spi_clk_map[];
extern const pio_map_t spi_cs_map[];


/**
 * @brief Assert chip select line for transceiver
 *
 * Note: internal function
 *
 * @param obj the transceiver object
 */
static inline void radio_rfm9x_cs_assert_pri(radio_rfm9x_t * obj)
{
    GPIO_PinOutClear(PIO_PORT(obj->cs), PIO_PIN(obj->cs));
}


/**
 * @brief De-assert chip select line for transceiver
 * @param obj the transceiver object
 */
static inline void radio_rfm9x_cs_deassert_pri(radio_rfm9x_t * obj)
{
    GPIO_PinOutSet(PIO_PORT(obj->cs), PIO_PIN(obj->cs));
}


/**
 * @brief Send data to transceiver
 * @param obj the transceiver object
 * @param addr internal register address
 * @param buffer generic data buffer (input)
 * @param size buffer size (in bytes)
 */
static inline void radio_rfm9x_write_pri(radio_rfm9x_t * obj, uint8_t addr, void * buffer, uint8_t size)
{
    CORE_ATOMIC_SECTION(
        // assert the line
        radio_rfm9x_cs_assert_pri(obj);

        // set write mode (MSB=1)
        addr |= 0x80;

        // transmit address
        SPIDRV_MTransmitB(&obj->spi_handle_data, &addr, 1);

        // transmit payload
        SPIDRV_MTransmitB(&obj->spi_handle_data, buffer, size);

        // deassert the line
        radio_rfm9x_cs_deassert_pri(obj);
    );
}


/**
 * @brief Read data from transceiver
 * @param obj the transceiver object
 * @param addr internal register address
 * @param buffer generic data buffer (output)
 * @param size number of bytes wanted to read from transceiver
 */
static inline void radio_rfm9x_read_pri(radio_rfm9x_t * obj, uint8_t addr, void * buffer, uint8_t size)
{
    CORE_ATOMIC_SECTION(
        // assert the line
        radio_rfm9x_cs_assert_pri(obj);

        // set read mode (MSB=0)
        addr &= 0x7F;

        // transmit address
        SPIDRV_MTransmitB(&obj->spi_handle_data, &addr, 1);

        // read data
        SPIDRV_MReceiveB(&obj->spi_handle_data, buffer, size);

        // deassert the line
        radio_rfm9x_cs_deassert_pri(obj);
    );
}


/**
 * @brief Write a word to internal register
 * @param obj the transceiver object
 * @param addr register address
 * @param value word
 */
static inline void radio_rfm9x_reg_write_pri(radio_rfm9x_t * obj, uint8_t addr, uint8_t value)
{
    radio_rfm9x_write_pri(obj, addr, &value, 1);
}


/**
 * @brief Read a word from internal register
 * @param obj the transceiver object
 * @param addr address of internal register
 * @return word
 */
static inline uint8_t radio_rfm9x_reg_read_pri(radio_rfm9x_t * obj, uint8_t addr)
{
    uint8_t reg = 0;
    radio_rfm9x_read_pri(obj, addr, &reg, 1);

    return reg;
}


/**
 * @brief Modify a word of a specific internal register (read-write operation)
 * @param obj the transceiver object
 * @param addr address of internal register
 * @param value word
 * @param mask bit mask for bits need to be changed in a word
 */
static inline void radio_rfm9x_reg_modify_pri(radio_rfm9x_t * obj, uint8_t addr, uint8_t value, uint8_t mask)
{
    // perform a write on modify operation
    uint8_t reg = 0;

    // read from module
    reg = radio_rfm9x_reg_read_pri(obj, addr);

    // set bit with corresponding bit mask
    BITS_MODIFY(reg, value, mask);

    // write back
    radio_rfm9x_reg_write_pri(obj, addr, reg);
}


/**
 * @brief Generically, set the transceiver operating mode
 * @param obj the transceiver object
 * @param opmode transceiver operating mode @see radio_rfm9x_op_t
 */
static inline void radio_rfm9x_set_opmode_pri(radio_rfm9x_t * obj, uint8_t opmode)
{
    radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, opmode, RH_RF95_MODE);
}


/**
 * @brief Read current transceiver operating mode
 * @param obj the transceiver object
 * @return current transceiver operating mode @see radio_rfm9x_op_t
 */
static inline uint8_t radio_rfm9x_get_opmode_pri(radio_rfm9x_t * obj)
{
    return (uint8_t) (radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_01_OP_MODE) & RH_RF95_MODE);
}


/**
 * @brief Tell if the channel is free by measuring channel RSSI
 * @param obj the transceiver object
 * @return True if the channel is free, False otherwise
 */
static inline bool radio_rfm9x_is_channel_available_pri(radio_rfm9x_t * obj)
{
    int16_t rssi = (int16_t) (RH_RSSI_OFFSET + radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_1B_RSSI_VALUE));

    return rssi <= RADIO_RFM9X_CHANNEL_RSSI_THRESHOLD;
}


/**
 * @brief DIO0 raising edge interrupt handler
 *
 * @param pin interrupt pin number
 * @param obj the transciever object
 */
static void radio_rfm9x_dio0_isr_pri(uint8_t pin, radio_rfm9x_t * obj)
{
    // read interrupt register
    volatile uint8_t reg_irq_flags = 0;
    reg_irq_flags = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_12_IRQ_FLAGS);

    // clear all flags
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_12_IRQ_FLAGS, 0xff);

    // rx_timeout error
    if (reg_irq_flags & RH_RF95_RX_TIMEOUT)
    {
        if (obj->base.on_rx_timeout_cb.callback)
            ((on_rx_timeout_isr_handler) obj->base.on_rx_timeout_cb.callback)(obj->base.on_rx_timeout_cb.args);
    }

    // rx done
    if (reg_irq_flags & RH_RF95_RX_DONE)
    {
        // get ready for message receive
        uint8_t buffer[RADIO_RFM9X_RW_BUFFER_SIZE];
        uint8_t size;

        // read available bytes from FIFO
        size = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_13_RX_NB_BYTES);

        // reset the fifo read pointer to the beginning of packet
        radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR,
                                  radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR)
        );
        radio_rfm9x_read_pri(obj, RH_RF95_REG_00_FIFO, buffer, size);

        // read snr and rssi
        int16_t rssi;
        int8_t snr;
        rssi = (int16_t) (RH_RSSI_OFFSET + radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_1A_PKT_RSSI_VALUE));
        snr = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_19_PKT_SNR_VALUE);
        if (snr & 0x80)
        {
            snr = (int8_t) ((~snr + 1) & 0xff) >> 2;
            snr = -snr;
        } else
        {
            snr = (int8_t) (snr & 0xff) >> 2;
        }

        // send received data to callback function
        if (obj->base.on_rx_done_cb.callback)
            ((on_rx_done_isr_handler) obj->base.on_rx_done_cb.callback)(
                    obj->base.on_rx_done_cb.args, buffer, size, rssi, snr);

#if USE_FREERTOS == 1
        // stop the rx timout timer
        xTimerStopFromISR(obj->rx_timeout_timer, NULL);
#endif
    }

    // crc error
    if (reg_irq_flags & RH_RF95_PAYLOAD_CRC_ERROR)
    {
        if (obj->base.on_rx_error_cb.callback)
            ((on_rx_error_isr_handler) obj->base.on_rx_error_cb.callback)(obj->base.on_rx_error_cb.args);
    }

    // valid header
    if (reg_irq_flags & RH_RF95_VALID_HEADER)
    {

    }

    // tx done
    if (reg_irq_flags & RH_RF95_TX_DONE)
    {
        // transmit done
        if (obj->base.on_tx_done_cb.callback)
            ((on_tx_done_isr_handler) obj->base.on_tx_done_cb.callback)(obj->base.on_tx_done_cb.args);

#if USE_FREERTOS == 1
        // stop the tx timeout timer
        xTimerStopFromISR(obj->tx_timeout_timer, NULL);
#endif
    }

    // cad done
    if (reg_irq_flags & RH_RF95_CAD_DONE)
    {

    }

    // FHSS change channel
    if (reg_irq_flags & RH_RF95_FHSS_CHANGE_CHANNEL)
    {

    }

    // CAD detected
    if (reg_irq_flags & RH_RF95_CAD_DETECTED)
    {

    }
}


#if USE_FREERTOS == 1
/**
 * @brief When Rx and Tx timer expires, calls corresponding ISRs and set the radio to default state (stdby)
 * @param xTimer the timer object
 */
static void radio_rfm9x_on_timer_timeout(TimerHandle_t xTimer)
{
    DRV_ASSERT(xTimer);

    xTimerStop(xTimer, portMAX_DELAY);

    radio_rfm9x_t * obj = (radio_rfm9x_t *) pvTimerGetTimerID(xTimer);

    switch (obj->base.opmode)
    {
        case RADIO_OPMODE_RX:
        {
            if (obj->base.on_rx_timeout_cb.callback)
                ((on_rx_timeout_handler_t) obj->base.on_rx_timeout_cb.callback)(obj->base.on_rx_timeout_cb.args);

            break;
        }

        case RADIO_OPMODE_TX:
        {
            // TODO: reset the radio and restore previous settings

            if (obj->base.on_tx_timeout_cb.callback)
                ((on_rx_timeout_handler_t) obj->base.on_tx_timeout_cb.callback)(obj->base.on_tx_timeout_cb.args);

            break;
        }

        default:
        {
            break;
        }
    }
}

#endif


void radio_rfm9x_hard_reset(radio_rfm9x_t * obj)
{
    DRV_ASSERT(obj);

    // TODO: check if delay module is initialized

    // hold reset line low for 100us
    GPIO_PinOutClear(PIO_PORT(obj->rst), PIO_PIN(obj->rst));
    delay_us(100);

    // release reset line and wait for another 5ms
    GPIO_PinOutSet(PIO_PORT(obj->rst), PIO_PIN(obj->rst));
    delay_ms(5);
}


void radio_rfm9x_set_channel(radio_rfm9x_t * obj, uint32_t freq)
{
    DRV_ASSERT(obj);

    // keep local copy
    obj->config.frequency = freq;

    freq = ( uint32_t )( ( double )freq / ( double ) RH_RF95_FSTEP );
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_06_FRF_MSB, (uint8_t) ((freq >> 16) & 0xff));
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_07_FRF_MID, (uint8_t) ((freq >> 8) & 0xff));
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_08_FRF_LSB, (uint8_t) (freq & 0xff));
}


void radio_rfm9x_set_modem(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem)
{
    DRV_ASSERT(obj);

    // backup previous transceiver opmode
    uint8_t prev_opmode = radio_rfm9x_get_opmode_pri(obj);

    // set to sleep mode to allow modem to be switched
    radio_rfm9x_set_opmode_pri(obj, RH_RF95_MODE_SLEEP);

    switch (modem)
    {
        case RADIO_RFM9X_MODEM_FSK:
        {
            radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, RH_RF95_FSK_MODE, 1 << 7);
            break;
        }

        case RADIO_RFM9X_MODEM_LORA:
        {
            radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_01_OP_MODE, RH_RF95_LONG_RANGE_MODE, 1 << 7);
            break;
        }

        default:
            DRV_ASSERT(false);
    }

    // restore previous op mode
    radio_rfm9x_set_opmode_pri(obj, prev_opmode);

    // keep local copy
    obj->config.modem = modem;
}


void radio_rfm9x_set_tx_power_use_rfo(radio_rfm9x_t * obj, int8_t power_dbm, bool use_rfo)
{
    DRV_ASSERT(obj);

    if (use_rfo)
    {
        if (power_dbm > 14) power_dbm = 14;
        if (power_dbm < -1) power_dbm = -1;

        radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_09_PA_CONFIG, (uint8_t) (RH_RF95_MAX_POWER | (power_dbm + 1)));
    }
    else
    {
        if (power_dbm > 23) power_dbm = 23;
        if (power_dbm < 5) power_dbm = 5;

        // For RH_RF95_PA_DAC_ENABLE, manual says '+20dBm on PA_BOOST when OutputPower=0xf'
        // RH_RF95_PA_DAC_ENABLE actually adds about 3dBm to all power levels. We will us it
        // for 21, 22 and 23dBm
        if (power_dbm > 20)
        {
            radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_ENABLE);
            power_dbm -= 3;
        }
        else
        {
            radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_4D_PA_DAC, RH_RF95_PA_DAC_DISABLE);
        }

        // RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
        // pin is connected, so must use PA_BOOST
        // Pout = 2 + OutputPower.
        // The documentation is pretty confusing on this topic: PaSelect says the max power is 20dBm,
        // but OutputPower claims it would be 17dBm.
        // My measurements show 20dBm is correct
        radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_09_PA_CONFIG, (uint8_t) (RH_RF95_PA_SELECT | (power_dbm - 5)));
    }


    // keep local copy
    obj->config.tx_power = power_dbm;
}


void radio_rfm9x_set_preamble_length(radio_rfm9x_t * obj, uint16_t bytes)
{
    DRV_ASSERT(obj);

    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_20_PREAMBLE_MSB, (uint8_t) (bytes >> 8));
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_21_PREAMBLE_LSB, (uint8_t) (bytes & 0xff));

    // keep local copy
    obj->config.preamble_length = bytes;
}


void radio_rfm9x_set_bandwidth(radio_rfm9x_t * obj, radio_rfm9x_bw_t bandwidth)
{
    DRV_ASSERT(obj);

    radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1D_MODEM_CONFIG1, ((uint8_t) bandwidth) << 4, 0xf0); // mask: 0b11110000

    // keep local copy
    obj->config.bandwidth = bandwidth;
}


void radio_rfm9x_set_coding_rate(radio_rfm9x_t * obj, radio_rfm9x_cr_t coding_rate)
{
    DRV_ASSERT(obj);

    radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1D_MODEM_CONFIG1, ((uint8_t) coding_rate) << 1, 0xe); // mask: 0b00001110

    // keep local copy
    obj->config.coding_rate = coding_rate;
}


void radio_rfm9x_set_implicit_header_mode_on(radio_rfm9x_t * obj, bool is_implicit_header)
{
    DRV_ASSERT(obj);

    radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1D_MODEM_CONFIG1, (uint8_t) (is_implicit_header ? 0x1 : 0x0), 0x1);

    // keep local copy
    obj->config.implicit_header = is_implicit_header;
}


void radio_rfm9x_set_spreading_factor(radio_rfm9x_t * obj, radio_rfm9x_sf_t spreading_factor)
{
    DRV_ASSERT(obj);

    radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1E_MODEM_CONFIG2, ((uint8_t) spreading_factor) << 4, 0xf0);

    // keep local copy
    obj->config.spreading_factor = spreading_factor;
}


void radio_rfm9x_set_crc_enable(radio_rfm9x_t * obj, bool crc_enable)
{
    DRV_ASSERT(obj);

    radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_1E_MODEM_CONFIG2, (uint8_t) (crc_enable ? 0x1 : 0x0) << 2, 0x4);

    // keep local copy
    obj->config.crc_enable = crc_enable;
}


void radio_rfm9x_set_lna(radio_rfm9x_t * obj, uint8_t lna_gain, bool boost_on)
{
    DRV_ASSERT(obj);

    // read lna settings
    uint8_t reg = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_0C_LNA);

    // apply boost_on option
    BITS_MODIFY(reg, (boost_on ? 0x1 : 0x0), 0x1);

    // apply lna gain
    if (lna_gain == 0x0 || lna_gain == 0x7)
    {
        return;
    }
    else
    {
        BITS_MODIFY(reg, lna_gain << 5, 0x7 << 5);
    }

    // write back
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0C_LNA, reg);
}


static void radio_rfm9x_set_opmode_stdby_pri(radio_rfm9x_t * obj)
{
    DRV_ASSERT(obj);

    if (obj->base.opmode != RADIO_OPMODE_IDLE)
    {
#if USE_FREERTOS == 1
        // stop all timers before entering standby state
        if (__IS_INTERRUPT())
        {
            xTimerStopFromISR(obj->rx_timeout_timer, NULL);
            xTimerStopFromISR(obj->tx_timeout_timer, NULL);
        }
        else
        {
            xTimerStop(obj->rx_timeout_timer, portMAX_DELAY);
            xTimerStop(obj->tx_timeout_timer, portMAX_DELAY);
        }
#endif
        radio_rfm9x_set_opmode_pri(obj, RH_RF95_MODE_STDBY);

        obj->base.opmode = RADIO_OPMODE_IDLE;
    }
}


static void radio_rfm9x_set_opmode_tx_timeout_pri(radio_rfm9x_t * obj, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);

    if (obj->base.opmode != RADIO_OPMODE_TX)
    {
        // set opmode and interrupt source
        radio_rfm9x_set_opmode_pri(obj, RH_RF95_MODE_TX);
        radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_40_DIO_MAPPING1, 0x01 << 6, 0xC0); // interrupt on tx done

#if USE_FREERTOS == 1
        if (timeout_ms != 0)
        {
            if (__IS_INTERRUPT())
            {
                xTimerChangePeriodFromISR(obj->tx_timeout_timer, pdMS_TO_TICKS(timeout_ms), NULL);
                xTimerStartFromISR(obj->tx_timeout_timer, NULL);
            }
            else
            {
                xTimerChangePeriod(obj->tx_timeout_timer, pdMS_TO_TICKS(timeout_ms), portMAX_DELAY);
                xTimerStart(obj->tx_timeout_timer, portMAX_DELAY);
            }
        }
#endif

        obj->base.opmode = RADIO_OPMODE_TX;
    }
}


static void radio_rfm9x_set_opmode_rx_timeout_pri(radio_rfm9x_t * obj, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);

    if (obj->base.opmode != RADIO_OPMODE_RX)
    {
        // set opmode and interrupt source
        radio_rfm9x_set_opmode_pri(obj, RH_RF95_MODE_RXCONTINUOUS);
        radio_rfm9x_reg_modify_pri(obj, RH_RF95_REG_40_DIO_MAPPING1, 0x00 << 6, 0xC0); // interrupt on rx done

#if USE_FREERTOS == 1
        if (timeout_ms != 0)
        {
            if (__IS_INTERRUPT())
            {
                xTimerChangePeriodFromISR(obj->rx_timeout_timer, pdMS_TO_TICKS(timeout_ms), NULL);
                xTimerStartFromISR(obj->rx_timeout_timer, NULL);
            }
            else
            {
                xTimerChangePeriod(obj->rx_timeout_timer, pdMS_TO_TICKS(timeout_ms), portMAX_DELAY);
                xTimerStart(obj->rx_timeout_timer, portMAX_DELAY);
            }
        }
#endif

        obj->base.opmode = RADIO_OPMODE_RX;
    }
}


static void radio_rfm9x_set_opmode_sleep_pri(radio_rfm9x_t * obj)
{
    DRV_ASSERT(obj);

    if (obj->base.opmode != RADIO_OPMODE_SLEEP)
    {
#if USE_FREERTOS == 1
        // stop all timers before entering sleep state
        if (__IS_INTERRUPT())
        {
            xTimerStopFromISR(obj->rx_timeout_timer, NULL);
            xTimerStopFromISR(obj->tx_timeout_timer, NULL);
        }
        else
        {
            xTimerStop(obj->rx_timeout_timer, portMAX_DELAY);
            xTimerStop(obj->tx_timeout_timer, portMAX_DELAY);
        }
#endif
        radio_rfm9x_set_opmode_pri(obj, RH_RF95_MODE_SLEEP);

        obj->base.opmode = RADIO_OPMODE_SLEEP;
    }
}


void radio_rfm9x_set_public_network(radio_rfm9x_t * obj, bool enable)
{
    DRV_ASSERT(obj);

    if (enable)
    {
         // Note: 0x39, aka SYNC register, is one of the undocumented register in RFM9x chip
        radio_rfm9x_reg_write_pri(obj, 0x39, RFM9X_PUBLIC_SYNWORD);
    }
    else
    {
        radio_rfm9x_reg_write_pri(obj, 0x39, RFM9X_PRIVATE_SYNCWORD);
    }

    obj->config.is_public_network = enable;
}


uint32_t radio_rfm9x_generate_random_number(radio_rfm9x_t * obj)
{
    DRV_ASSERT(obj);

    uint32_t rnd = 0;

    // backup previous transceiver opmode
    uint8_t prev_opmode = radio_rfm9x_get_opmode_pri(obj);

    // backup interrupt interrupts
    uint8_t prev_int_mask_reg = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_11_IRQ_FLAGS_MASK);

    // mask all interrupts
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_11_IRQ_FLAGS_MASK, 0xff);

    // set the transceiver to rx_continuous mode
    radio_rfm9x_set_opmode_pri(obj, RH_RF95_MODE_RXCONTINUOUS);

    for (uint8_t i = 0; i < 32; i++)
    {
        // Note: 0x2C, aka Wideband RSSI register, is one of the undocumented register in RFM9x chip
        rnd |= ((uint32_t) radio_rfm9x_reg_read_pri(obj, 0x2C) & 0x01) << i;
        delay_ms(1);
    }

    // restore interrupt masks
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_11_IRQ_FLAGS_MASK, prev_int_mask_reg);

    // restore previous op mode
    radio_rfm9x_set_opmode_pri(obj, prev_opmode);

    return rnd;
}


uint32_t radio_rfm9x_get_time_on_air(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem, uint8_t packet_length)
{
    DRV_ASSERT(obj);

    uint32_t time_on_air = 0;

    switch (modem)
    {
        case RADIO_RFM9X_MODEM_LORA:
        {
            // read current bandwidth
            uint32_t bw = 0;
            switch (obj->config.bandwidth)
            {
                case RADIO_RFM9X_BW_125K:
                    bw = 125000UL;
                    break;
                case RADIO_RFM9X_BW_250K:
                    bw = 250000UL;
                    break;
                case RADIO_RFM9X_BW_500K:
                    bw = 500000UL;
                    break;
                default:
                    DRV_ASSERT(false);
                    break;
            }

            // calculate premable duration
            double r_s = (double) bw / (1 << obj->config.spreading_factor);
            double t_s = 1 / r_s; // symbol period
            double t_preamble = (obj->config.preamble_length + 4.25) * t_s;

            // calculate payload period
            double t_payload =
                    (t_s * (8.0 +
                            ceil((8.0 * packet_length - 4.0 * obj->config.spreading_factor + (obj->config.implicit_header ? 24.0 : 44.0)) / (4.0 * obj->config.spreading_factor)) *
                                    obj->config.coding_rate + 4.0));

            // convert to ms
            time_on_air = (uint32_t) floor((t_preamble + t_payload) * 1e3 + 0.999);

            return time_on_air;
        }

        case RADIO_RFM9X_MODEM_FSK:
        {
            DRV_ASSERT(false);
            return 0;
        }
        default:
            DRV_ASSERT(false);
            return 0;
    }
}


void radio_rfm9x_set_max_payload_length(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem, uint8_t max_length)
{
    switch (modem)
    {
        case RADIO_RFM9X_MODEM_LORA:
        {
            radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, max_length);
            break;
        }
        case RADIO_RFM9X_MODEM_FSK:
        {
            DRV_ASSERT(false);
        }
        default:
            DRV_ASSERT(false);
            break;
    }

    // keep local copy
    obj->config.max_payload_length = max_length;
}


void radio_rfm9x_init(radio_rfm9x_t * obj,
                      pio_t rst, pio_t miso, pio_t mosi, pio_t clk, pio_t cs,
                      pio_t dio0, pio_t dio1, pio_t dio2, pio_t dio3, pio_t dio4, pio_t dio5
)
{
    uint32_t miso_loc;
    uint32_t mosi_loc;
    uint32_t clk_loc;
    uint32_t cs_loc;
    USART_TypeDef *usart_base;

    SPIDRV_Init_t spi_init_data;

    DRV_ASSERT(obj);

    // clear the memory
    memset(obj, 0x0, sizeof(radio_rfm9x_t));

    // copy variables object
    obj->rst = rst;
    obj->miso = miso;
    obj->mosi = mosi;
    obj->clk = clk;
    obj->cs = cs;
    obj->dio0 = dio0;

    // reset internal variable
    memset(&obj->config, 0x0, sizeof(obj->config));

    // configure default callbacks
    obj->base.radio_set_opmode_idle_cb.callback = radio_rfm9x_set_opmode_stdby_pri;
    obj->base.radio_set_opmode_idle_cb.args = obj;

    obj->base.radio_set_opmode_sleep_cb.callback = radio_rfm9x_set_opmode_sleep_pri;
    obj->base.radio_set_opmode_sleep_cb.args = obj;

    obj->base.radio_set_opmode_rx_timeout_cb.callback = radio_rfm9x_set_opmode_rx_timeout_pri;
    obj->base.radio_set_opmode_rx_timeout_cb.args = obj;

    obj->base.radio_set_opmode_tx_timeout_cb.callback = radio_rfm9x_set_opmode_tx_timeout_pri;
    obj->base.radio_set_opmode_tx_timeout_cb.args = obj;

    // register send/recv handlers
    obj->base.radio_send_cb.callback = radio_rfm9x_send_timeout;
    obj->base.radio_send_cb.args = obj;

    obj->base.radio_recv_cb.callback = radio_rfm9x_recv_timeout;
    obj->base.radio_recv_cb.args = obj;

#if USE_FREERTOS == 1
    // initialize timer
    obj->rx_timeout_timer = xTimerCreate("rfm_rx_t", pdMS_TO_TICKS(1), pdFALSE, obj, radio_rfm9x_on_timer_timeout);
    obj->tx_timeout_timer = xTimerCreate("rfm_tx_t", pdMS_TO_TICKS(1), pdFALSE, obj, radio_rfm9x_on_timer_timeout);

    DRV_ASSERT(obj->rx_timeout_timer);
    DRV_ASSERT(obj->tx_timeout_timer);
#endif

    /**
     * @brief Configure radio module
     */
    // find spi peripheral functions
    DRV_ASSERT(find_pin_function(spi_miso_map, obj->miso, (void **) &usart_base, &miso_loc));
    DRV_ASSERT(find_pin_function(spi_mosi_map, obj->mosi, NULL, &mosi_loc));
    DRV_ASSERT(find_pin_function(spi_clk_map, obj->clk, NULL, &clk_loc));
    DRV_ASSERT(find_pin_function(spi_cs_map, obj->cs, NULL, &cs_loc));

    // configure spi
    spi_init_data.port = usart_base;
    spi_init_data.portLocationTx = (uint8_t) mosi_loc;
    spi_init_data.portLocationRx = (uint8_t) miso_loc;
    spi_init_data.portLocationClk = (uint8_t) clk_loc;
    spi_init_data.portLocationCs = (uint8_t) cs_loc;
    spi_init_data.bitRate = 8000000;
    spi_init_data.frameLength = 8;
    spi_init_data.dummyTxValue = 0;
    spi_init_data.type = spidrvMaster;
    spi_init_data.bitOrder = spidrvBitOrderMsbFirst;
    spi_init_data.clockMode = spidrvClockMode0;
    spi_init_data.csControl = spidrvCsControlApplication;
    spi_init_data.slaveStartMode = spidrvSlaveStartImmediate;

    // initalize spi driver
    SPIDRV_Init(&obj->spi_handle_data, &spi_init_data);

    // configure pios
    GPIO_PinModeSet(PIO_PORT(obj->cs), PIO_PIN(obj->cs), gpioModePushPull, 1);
    GPIO_PinModeSet(PIO_PORT(obj->rst), PIO_PIN(obj->rst), gpioModePushPull, 1);
    GPIO_PinModeSet(PIO_PORT(obj->dio0), PIO_PIN(obj->dio0), gpioModeInput, 0);

    // configure port interrupt
    GPIOINT_CallbackRegisterWithArgs(PIO_PIN(obj->dio0), (GPIOINT_IrqCallbackPtrWithArgs_t) radio_rfm9x_dio0_isr_pri, (void *) obj);
    GPIO_IntConfig(PIO_PORT(obj->dio0), PIO_PIN(obj->dio0),
                   true /* raising edge */, false /* falling edge */, true /* enable now */
    );

    // perform hard reset
    radio_rfm9x_hard_reset(obj);

    // read hardware version, read 0 if radio is not present
    uint8_t hw_version = 0;
    hw_version = radio_rfm9x_reg_read_pri(obj, RH_RF95_REG_42_VERSION);
    DRV_ASSERT(hw_version);

    // configure pointer for FIFO
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0x00);
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);

    // reset fifo pointer
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

    // set public network
    radio_rfm9x_set_public_network(obj, true);

    // set stdby mode as default state after initialization
    radio_rfm9x_set_opmode_stdby_pri(obj);
}


void radio_rfm9x_send_timeout(radio_rfm9x_t * obj, void * buffer, uint8_t size, uint32_t timeout)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(buffer);
    DRV_ASSERT(size);

    // put the radio to idle state to allow fifo access
    radio_rfm9x_set_opmode_stdby_pri(obj);

    // set fifo pointer
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0x00);
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

    // declare data length
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_22_PAYLOAD_LENGTH, size);

    // transfer data from local to fifo
    radio_rfm9x_write_pri(obj, RH_RF95_REG_00_FIFO, buffer, size);

    // set in tx mode
    radio_rfm9x_set_opmode_tx_timeout_pri(obj, timeout);
}

void radio_rfm9x_recv_timeout(radio_rfm9x_t * obj, uint32_t timeout_ms)
{
    // put the radio to idle state
    radio_rfm9x_set_opmode_stdby_pri(obj);

    // reset fifo for receiving new packet
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0x00);
    radio_rfm9x_reg_write_pri(obj, RH_RF95_REG_0D_FIFO_ADDR_PTR, 0x00);

    // put the radio into active receive mode
    radio_rfm9x_set_opmode_rx_timeout_pri(obj, timeout_ms);
}
