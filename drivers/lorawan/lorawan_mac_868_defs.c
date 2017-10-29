/**
 * @brief Definitions for band specific constants
 * @file lorawan_mac_868_defs.c
 * @date 25, Oct, 2017
 * @author Ran Bao
 */

#include <stdint.h>
#include "lorawan_mac_868_defs.h"
#include "radio_rfm9x.h"

const radio_rfm9x_bw_t lorawan_mac_bandwidth_lookup_table[] =
{
    RADIO_RFM9X_BW_125K,
    RADIO_RFM9X_BW_125K,
    RADIO_RFM9X_BW_125K,
    RADIO_RFM9X_BW_125K,
    RADIO_RFM9X_BW_125K,
    RADIO_RFM9X_BW_125K,
    RADIO_RFM9X_BW_250K,
    RADIO_RFM9X_BW_500K
};

const radio_rfm9x_sf_t lorawan_mac_data_rate_lookup_table[] =
{
    RADIO_RFM9X_SF_4096, // 12
    RADIO_RFM9X_SF_2048, // 11
    RADIO_RFM9X_SF_1024, // 10
    RADIO_RFM9X_SF_512,  // 9
    RADIO_RFM9X_SF_256,  // 8
    RADIO_RFM9X_SF_128,  // 7
    RADIO_RFM9X_SF_128,  // 7
    RADIO_RFM9X_SF_64    // TODO: invalid?
};

const uint32_t lorawan_mac_channel_frequency_lookup_table[LORAWAN_MAX_NB_CHANNELS] =
{
    864100000UL,
    864300000UL,
    864500000UL
};


const uint32_t lorawan_mac_band_lookup_table[] =
{
    100,
    100,
    1000,
    10,
    100
};
