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

#define RADIO_EFR32_MAX_BUF_LEN 0xff

typedef struct
{
    radio_t base;

    RAIL_Handle_t rail_handle;

    uint8_t buffer[RADIO_EFR32_MAX_BUF_LEN];

    uint8_t channel;
} radio_efr32_t;

radio_efr32_t * radio_efr32_init(const RAIL_ChannelConfig_t *channelConfigs[], bool use_dcdc);

void radio_efr32_set_channel(radio_efr32_t * obj, uint8_t channel);

void radio_efr32_send(radio_efr32_t * obj, void * buffer, uint8_t size);

#endif
