/**
 * @brief Abstract interface for accessing EFR32 builtin radio module
 * @file radio_efr32.h
 * @author Ran Bao
 * @date 16/Nov/2017
 */

#ifndef RADIO_EFR32_H_
#define RADIO_EFR32_H_

#include <stdint.h>
#include <stdbool.h>

#include "radio_template.h"
#include "rail.h"

#if USE_FREERTOS == 1
#include "FreeRTOS.h"
#include "timers.h"
#endif


#define RADIO_EFR32_MAX_BUF_LEN 512
#define RADIO_EFR32_RX_FIFO_THRESHOLD 128
#define RADIO_EFR32_TX_FIFO_THRESHOLD 128

typedef struct
{

} radio_efr32_config_t;

typedef struct
{
    radio_t base;

    // low level RAIL handle
    RAIL_Handle_t rail_handle;

    // NOTE: the config need to be static since RAIL will keep referencing this value
    RAIL_Config_t rail_cfg;

    // NOTE: the rail data config need to be static as the program might refer to this data structure later
    RAIL_DataConfig_t data_config;

    uint8_t tx_buffer[RADIO_EFR32_MAX_BUF_LEN];

    uint8_t channel;

#if USE_FREERTOS == 1
    xTimerHandle rx_timeout_timer;
    xTimerHandle tx_timeout_timer;
#endif

} radio_efr32_t;

/**
 * @brief Initialize EFR32 on board subg radio with RAIL library
 * @param channelConfigs Radio configurations generated by Simplicity Studio
 * @param use_dcdc use DCDC power converter to power radio power amplifier
 * @return handle of efr32 radio (singleton)
 */
radio_efr32_t * radio_efr32_init(const RAIL_ChannelConfig_t *channelConfigs[], bool use_dcdc);

/**
 * @brief Set current channels between 0 to 20 (the channel spacing is defined in channelConfigs
 * @param obj the efr32 radio object
 * @param channel the channel
 */
void radio_efr32_set_channel(radio_efr32_t * obj, uint8_t channel);

/**
 * @brief Send data using radio with given timeout
 * @param obj the efr32 radio object
 * @param buffer data buffer
 * @param size data length in bytes
 * @param timeout_ms timeout measured in ms
 */
void radio_efr32_send_timeout(radio_efr32_t * obj, void * buffer, uint16_t size, uint32_t timeout_ms);

/**
 * @brief Put device in receive mode for given timeout
 * @param obj the efr32 radio object
 * @param timeout_ms timeout measured in ms
 */
void radio_efr32_recv_timeout(radio_efr32_t * obj, uint32_t timeout_ms);

/**
 * @brief Generate a true random number using radio
 * @param obj the efr32 radio object
 * @return generated random number
 */
uint32_t radio_efr32_generate_random_number(radio_efr32_t * obj);

/**
 * @brief Get current transmit power
 * @param obj the efr32 radio object
 * @return transmit power represented in dBm
 */
float radio_efr32_get_tx_pwr_dbm(radio_efr32_t * obj);

/**
 * @brief Set transmit power
 * @param obj the efr32 radio object
 * @param tx_pwr_dbm transmit power represented in dBm
 */
void radio_efr32_set_tx_pwr_dbm(radio_efr32_t * obj, float tx_pwr_dbm);

#endif
