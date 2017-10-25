/**
 * @brief Definitions for band specific constants
 * @file lorawan_mac_868_defs.h
 * @date 25, Oct, 2017
 * @author Ran Bao
 */

#ifndef LORAWAN_MAC_868_DEFS_H_
#define LORAWAN_MAC_868_DEFS_H_

#ifndef USE_BAND_868
#error "You should include lorawan_mac.h instead of this file!"
#endif

#include <stdint.h>
#include "radio_rfm9x.h"

#define LORAWAN_MAX_NB_CHANNELS ( 16 )
#define LORAWAN_TX_MIN_DATARATE ( LORAWAN_DATARATE_0 )
#define LORAWAN_TX_MAX_DATARATE ( LORAWAN_DATARATE_7 )
#define LORAWAN_RX_MIN_DATARATE ( LORAWAN_DATARETE_0 )
#define LORAWAN_RX_MAX_DATARATE ( LORAWAN_DATARATE_7 )
#define LORAWAN_DEFAULT_DATARATE ( LORAWAN_DATARETE_0 )
#define LORAWAN_MIN_RX1_DATARATE_OFFSET ( 0 )
#define LORAWAN_MAX_RX1_DATARATE_OFFSET ( 5 )
#define LORAWAN_MIN_TX_POWER ( LORAWAN_TX_POWER_02_DBM )
#define LORAWAN_MAX_TX_POWER ( LORAWAN_TX_POWER_20_DBM )
#define LORAWAN_DEFAULT_TX_POWER ( LORAWAN_TX_POWER_14_DBM )

extern const radio_rfm9x_bw_t lorawan_mac_bandwidth_lookup_table[];
extern const radio_rfm9x_sf_t lorawan_mac_data_rate_lookup_table[];
extern const uint32_t lorawan_mac_channel_frequency_lookup_table[];
extern const uint32_t lorawan_mac_band_lookup_table[];

typedef enum
{
	LORAWAN_TX_POWER_20_DBM = 0,
	LORAWAN_TX_POWER_14_DBM = 1,
	LORAWAN_TX_POWER_11_DBM = 2,
	LORAWAN_TX_POWER_08_DBM = 3,
	LORAWAN_TX_POWER_05_DBM = 4,
	LORAWAN_TX_POWER_02_DBM = 5,
	LORAWAN_TX_POWER_RESERVED
} lorawan_tx_power_t;

typedef enum
{
	LORAWAN_DATARATE_0 = 0, // SF12, BW125
	LORAWAN_DATARATE_1 = 1, // SF11, BW125
	LORAWAN_DATARATE_2 = 2, // SF10, BW125
	LORAWAN_DATARATE_3 = 3, // SF9, BW125
	LORAWAN_DATARATE_4 = 4, // SF8, BW125
	LORAWAN_DATARATE_5 = 5, // SF7, BW125
	LORAWAN_DATARATE_6 = 6, // SF7, BW250
	LORAWAN_DATARATE_7 = 7, // FSK: 50kbps
	LORAWAN_DATARATE_RESERVED
} lorawan_data_rate_t;




#endif
