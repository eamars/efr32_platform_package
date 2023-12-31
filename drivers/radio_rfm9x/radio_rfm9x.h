/**
 * @brief RFM9x LoRa/FSK module driver
 */

#ifndef RADIO_RFM9X_H_
#define RADIO_RFM9X_H_

#include <stdint.h>
#include <stdbool.h>

#include "radio_template.h"
#include "spidrv.h"
#include "pio_defs.h"
#include "radio_rfm9x_regs.h"

#if USE_FREERTOS == 1
#include "FreeRTOS.h"
#include "timers.h"
#endif


// The crystal oscillator frequency of the module
#define RH_RF95_FXOSC 32000000.0

// The Frequency Synthesizer step = RH_RF95_FXOSC / 2^^19
#define RH_RF95_FSTEP  (RH_RF95_FXOSC / 524288)

#define RH_RSSI_OFFSET (-137)

// Maximum a packet can be copied from chip to local (reserve 1 byte for memory alignment)
#define RADIO_RFM9X_RW_BUFFER_SIZE (0xFFUL)

#define RADIO_RFM9X_DEFAULT_RX_TIMEOUT (2000) // ms
#define RADIO_RFM9X_DEFAULT_TX_TIMEOUT (2000) // ms

#define RADIO_RFM9X_CHANNEL_RSSI_THRESHOLD (-60) // dBm

// radio wakeup time from sleep
#define RADIO_RFM9X_WAKEUP_TIME 1 // ms


/**
 * @brief RFM9X supports (G)FSK or LoRa frontend mode
 */
typedef enum
{
    RADIO_RFM9X_MODEM_FSK = 0,
    RADIO_RFM9X_MODEM_LORA = 1
} radio_rfm9x_modem_t;


/**
 * @brief Configurable bandwidth
 */
typedef enum
{
    RADIO_RFM9X_BW_7K8 = 0x0,
    RADIO_RFM9X_BW_10K4 = 0x1,
    RADIO_RFM9X_BW_15K6 = 0x2,
    RADIO_RFM9X_BW_20K8 = 0x3,
    RADIO_RFM9X_BW_32K25 = 0x4,
    RADIO_RFM9X_BW_41K7 = 0x5,
    RADIO_RFM9X_BW_62K5 = 0x6,
    RADIO_RFM9X_BW_125K = 0x7,
    RADIO_RFM9X_BW_250K = 0x8,
    RADIO_RFM9X_BW_500K = 0x9
} radio_rfm9x_bw_t;


/**
 * @brief Configurable coding rate
 */
typedef enum
{
    RADIO_RFM9X_CR_4_5 = 1,
    RADIO_RFM9X_CR_4_6 = 2,
    RADIO_RFM9X_CR_4_7 = 3,
    RADIO_RFM9X_CR_4_8 = 4
} radio_rfm9x_cr_t;


/**
 * @brief Configurable spreading factor (LoRa)
 */
typedef enum
{
    // expressed in the power of 2
    RADIO_RFM9X_SF_64 = 6,
    RADIO_RFM9X_SF_128 = 7,
    RADIO_RFM9X_SF_256 = 8,
    RADIO_RFM9X_SF_512 = 9,
    RADIO_RFM9X_SF_1024 = 10,
    RADIO_RFM9X_SF_2048 = 11,
    RADIO_RFM9X_SF_4096 = 12
} radio_rfm9x_sf_t;


/**
 * @brief Generic callback function scheme
 */
typedef struct
{
    void * callback_function;
    void * args;
} radio_rfm9x_callback_t;


typedef void (*on_rx_done_isr_handler)(void * args, void * msg, uint16_t size, int16_t rssi, int8_t snr) ;
typedef void (*on_tx_done_isr_handler)(void * args);
typedef void (*on_rx_error_isr_handler)(void * args);
typedef void (*on_rx_timeout_isr_handler)(void * args);
#if USE_FREERTOS == 1
typedef void (*on_tx_timeout_handler)(void * args);
#endif

/**
 * @brief RFM9X transceiver object
 */
typedef struct
{
    radio_t base;

    // hardware pins
    pio_t rst;
    pio_t miso;
    pio_t mosi;
    pio_t clk;
    pio_t cs;
    pio_t dio0;

    // SPI driver
    SPIDRV_HandleData_t spi_handle_data;

    struct
    {
        uint32_t frequency;
        radio_rfm9x_modem_t modem;
        int8_t tx_power;
        uint16_t preamble_length;
        radio_rfm9x_bw_t bandwidth;
        radio_rfm9x_cr_t coding_rate;
        radio_rfm9x_sf_t spreading_factor;
        bool crc_enable;
        bool implicit_header;
        bool is_public_network;
        uint8_t max_payload_length;
    } config;

#if USE_FREERTOS == 1
    xTimerHandle rx_timeout_timer;
    xTimerHandle tx_timeout_timer;
#endif

} radio_rfm9x_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize RFM9x LoRa/FSK module
 * @param obj the transceiver object
 * @param rst reset pin
 * @param miso master in slave out
 * @param mosi master out slave in
 * @param clk SPI clock
 * @param cs SPI slave select
 * @param dio0 transceivier interrupt in 0
 * @param dio1 transceivier interrupt in 1
 * @param dio2 transceivier interrupt in 2
 * @param dio3 transceivier interrupt in 3
 * @param dio4 transceivier interrupt in 4
 * @param dio5 transceivier interrupt in 5
 */
void radio_rfm9x_init(radio_rfm9x_t * obj,
                      pio_t rst, pio_t miso, pio_t mosi, pio_t clk, pio_t cs,
                      pio_t dio0, pio_t dio1, pio_t dio2, pio_t dio3, pio_t dio4, pio_t dio5
);

/**
 * @brief Perform a hard reset on RFM9x transceiver module
 *
 * Note: The thread is excepted to be block for more than 5ms
 *
 * @param obj the transceiver object
 */
void radio_rfm9x_hard_reset(radio_rfm9x_t * obj);

/**
 * @brief Configure transceiver center channel
 * @param obj the transceiver object
 * @param freq center frequency, measured in Hz
 */
void radio_rfm9x_set_channel(radio_rfm9x_t * obj, uint32_t freq);

/**
 * @brief Configure transmit power from the transceiver
 * @param obj the transceiver object
 * @param power_dbm transmit power, measured in dBm
 * @param use_rfo do not use power amplifier
 */
void radio_rfm9x_set_tx_power_use_rfo(radio_rfm9x_t * obj, int8_t power_dbm, bool use_rfo);

float radio_rfm9x_get_tx_power(radio_rfm9x_t * obj);
void radio_rfm9x_set_tx_power(radio_rfm9x_t * obj, float power_dbm);

/**
 * @brief Configure modem mode
 * @param obj the transceiver object
 * @param modem (G)FSK or LoRa (spread spectrum) @see radio_rfm9x_modem_t
 */
void radio_rfm9x_set_modem(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem);

/**
 * @brief Configure LoRa packet preamble length
 * @param obj the transceiver object
 * @param bytes number of bytes in preamble
 */
void radio_rfm9x_set_preamble_length(radio_rfm9x_t * obj, uint16_t bytes);

/**
 * @brief Configure radio bandwidth. Note, in the lower band (169MHz), the signal bandwidth 250KHz and 500KHz are not available
 * @param obj transceiver object
 * @param bandwidth desired bandwidth
 */
void radio_rfm9x_set_bandwidth(radio_rfm9x_t * obj, radio_rfm9x_bw_t bandwidth);

/**
 * @brief Configure coding rate
 * @param obj the transceiver object
 * @param coding_rate desired coding rate, @see radio_rfm9x_cr_t
 */
void radio_rfm9x_set_coding_rate(radio_rfm9x_t * obj, radio_rfm9x_cr_t coding_rate);

/**
 * @brief Configure the implicit header mode
 * @param obj the transceiver object
 * @param is_implicit_header whether to enable implicit header
 */
void radio_rfm9x_set_implicit_header_mode_on(radio_rfm9x_t * obj, bool is_implicit_header);

/**
 * @brief Configure LoRa spreading factor
 * @param obj the transceiver object
 * @param spreading_factor desired spreading factor @see radio_rfm9x_sf_t
 */
void radio_rfm9x_set_spreading_factor(radio_rfm9x_t * obj, radio_rfm9x_sf_t spreading_factor);

/**
 * @brief Configure CRC validation for the payload
 * @param obj the transceiver object
 * @param crc_enable whether to check CRC for the payload
 */
void radio_rfm9x_set_crc_enable(radio_rfm9x_t * obj, bool crc_enable);

/**
 * @brief Configure transceiver LNA
 * @param obj the transceiver object
 * @param lna_gain LNA gain settings, please refer to datasheet. 0x0 indicates no changes
 * @param boost_on true if turn LNA boost on, 150% LNA current
 */
void radio_rfm9x_set_lna(radio_rfm9x_t * obj, uint8_t lna_gain, bool boost_on);

/**
 * @brief Write data to transceiver and set transceiver mode to transmit
 * @param obj the transceiver object
 * @param buffer data
 * @param size the length of data
 * @param timeout_ms timeout in ms
 */
void radio_rfm9x_send_timeout(radio_rfm9x_t * obj, void * buffer, uint8_t size, uint32_t timeout_ms);

/**
 * @brief Enter the data receiving mode for certain amount of time
 * @param obj the transceiver object
 * @param timeout_ms timeout in ms
 */
void radio_rfm9x_recv_timeout(radio_rfm9x_t * obj, uint32_t timeout_ms);

/**
 * @brief Set public network access
 *
 * Note: For some reason this API is not mentioned anywhere in RFM9x datasheet. Please use with caution!
 *
 * @param obj the transceiver
 */
void radio_rfm9x_set_public_network(radio_rfm9x_t * obj, bool enable);

/**
 * @brief Generate random number from the radio antenna
 * @param obj the transceiver
 * @return a 32bit random number
 */
uint32_t radio_rfm9x_generate_random_number(radio_rfm9x_t * obj);

/**
 * @brief Calculate time on air for given packet length with current configuration
 * @param obj the transceiver
 * @param modem LoRa or FSK modem
 * @param packet_length the packet length
 * @return time on air in ms
 */
uint32_t radio_rfm9x_get_time_on_air(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem, uint8_t packet_length);

/**
 * @brief Set the maximum number of bytes in threshold. Once exceeded, throw a CRC error
 * @param obj the transceiver
 * @param modem LoRa or FSK modem
 * @param max_length the maximum packet length
 */
void radio_rfm9x_set_max_payload_length(radio_rfm9x_t * obj, radio_rfm9x_modem_t modem, uint8_t max_length);


#ifdef __cplusplus
}
#endif


#endif // RADIO_RFM9X_H_
