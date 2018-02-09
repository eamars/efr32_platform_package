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

radio_efr32_t * radio_efr32_init(const RAIL_ChannelConfig_t *channelConfigs[], bool use_dcdc);

void radio_efr32_set_channel(radio_efr32_t * obj, uint8_t channel);

void radio_efr32_send_timeout(radio_efr32_t * obj, void * buffer, uint16_t size, uint32_t timeout_ms);
void radio_efr32_recv_timeout(radio_efr32_t * obj, uint32_t timeout_ms);

uint32_t radio_efr32_generate_random_number(radio_efr32_t * obj);

#endif
