/**
 * @brief LoRaWAN 868 Band specification
 */
#ifndef LORAWAN_BAND_868_H_
#define LORAWAN_BAND_868_H_

#include <stdint.h>
#include "lorawan_band.h"
#include "lorawan_types.h"

/*!
 * LoRaMac maximum number of channels
 */
#define  LORAWAN_EU868_MAX_NB_CHANNELS                       16

/*!
 * Number of default channels
 */
#define  LORAWAN_EU868_NUMB_DEFAULT_CHANNELS                 3

/*!
 * Number of channels to apply for the CF list
 */
#define  LORAWAN_EU868_NUMB_CHANNELS_CF_LIST                 5

/*!
 * Minimal datarate that can be used by the node
 */
#define  LORAWAN_EU868_TX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define  LORAWAN_EU868_TX_MAX_DATARATE                       DR_7

/*!
 * Minimal datarate that can be used by the node
 */
#define  LORAWAN_EU868_RX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define  LORAWAN_EU868_RX_MAX_DATARATE                       DR_7

/*!
 * Default datarate used by the node
 */
#define  LORAWAN_EU868_DEFAULT_DATARATE                      DR_0

/*!
 * Minimal Rx1 receive datarate offset
 */
#define  LORAWAN_EU868_MIN_RX1_DR_OFFSET                     0

/*!
 * Maximal Rx1 receive datarate offset
 */
#define  LORAWAN_EU868_MAX_RX1_DR_OFFSET                     5

/*!
 * Default Rx1 receive datarate offset
 */
#define  LORAWAN_EU868_DEFAULT_RX1_DR_OFFSET                 0

/*!
 * Minimal Tx output power that can be used by the node
 */
#define  LORAWAN_EU868_MIN_TX_POWER                          TX_POWER_7

/*!
 * Maximal Tx output power that can be used by the node
 */
#define  LORAWAN_EU868_MAX_TX_POWER                          TX_POWER_0

/*!
 * Default Tx output power used by the node
 */
#define  LORAWAN_EU868_DEFAULT_TX_POWER                      TX_POWER_0

/*!
 * Default Max EIRP
 */
#define  LORAWAN_EU868_DEFAULT_MAX_EIRP                      16.0f

/*!
 * Default antenna gain
 */
#define  LORAWAN_EU868_DEFAULT_ANTENNA_GAIN                  2.15f

/*!
 * ADR Ack limit
 */
#define  LORAWAN_EU868_ADR_ACK_LIMIT                         64

/*!
 * ADR Ack delay
 */
#define  LORAWAN_EU868_ADR_ACK_DELAY                         32

/*!
 * Enabled or disabled the duty cycle
 */
#define  LORAWAN_EU868_DUTY_CYCLE_ENABLED                    1

/*!
 * Maximum RX window duration
 */
#define  LORAWAN_EU868_MAX_RX_WINDOW                         3000

/*!
 * Receive delay 1
 */
#define  LORAWAN_EU868_RECEIVE_DELAY1                        1000

/*!
 * Receive delay 2
 */
#define  LORAWAN_EU868_RECEIVE_DELAY2                        2000

/*!
 * Join accept delay 1
 */
#define  LORAWAN_EU868_JOIN_ACCEPT_DELAY1                    5000

/*!
 * Join accept delay 2
 */
#define  LORAWAN_EU868_JOIN_ACCEPT_DELAY2                    6000

/*!
 * Maximum frame counter gap
 */
#define  LORAWAN_EU868_MAX_FCNT_GAP                          16384

/*!
 * Ack timeout
 */
#define  LORAWAN_EU868_ACKTIMEOUT                            2000

/*!
 * Random ack timeout limits
 */
#define  LORAWAN_EU868_ACK_TIMEOUT_RND                       1000



/*!
 * Second reception window channel frequency definition.
 */
#define  LORAWAN_EU868_RX_WND_2_FREQ                         869525000

/*!
 * Second reception window channel datarate definition.
 */
#define  LORAWAN_EU868_RX_WND_2_DR                           DR_0

/*!
 * Maximum number of bands
 */
#define  LORAWAN_EU868_MAX_NB_BANDS                          5

/*!
 * Band 0 definition
 * { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define  LORAWAN_EU868_BAND0                                 { 100 , EU868_MAX_TX_POWER, 0,  0 } //  1.0 %

/*!
 * Band 1 definition
 * { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define  LORAWAN_EU868_BAND1                                 { 100 , EU868_MAX_TX_POWER, 0,  0 } //  1.0 %

/*!
 * Band 2 definition
 * Band = { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define  LORAWAN_EU868_BAND2                                 { 1000, EU868_MAX_TX_POWER, 0,  0 } //  0.1 %

/*!
 * Band 2 definition
 * Band = { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define  LORAWAN_EU868_BAND3                                 { 10  , EU868_MAX_TX_POWER, 0,  0 } // 10.0 %

/*!
 * Band 2 definition
 * Band = { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define  LORAWAN_EU868_BAND4                                 { 100 , EU868_MAX_TX_POWER, 0,  0 } //  1.0 %

/*!
 * LoRaMac default channel 1
 * Channel = { Frequency [Hz], RX1 Frequency [Hz], { ( ( DrMax << 4 ) | DrMin ) }, Band }
 */
#define  LORAWAN_EU868_LC1                                   { 866100000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 1 }

/*!
 * LoRaMac default channel 2
 * Channel = { Frequency [Hz], RX1 Frequency [Hz], { ( ( DrMax << 4 ) | DrMin ) }, Band }
 */
#define  LORAWAN_EU868_LC2                                   { 866300000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 1 }

/*!
 * LoRaMac default channel 3
 * Channel = { Frequency [Hz], RX1 Frequency [Hz], { ( ( DrMax << 4 ) | DrMin ) }, Band }
 */
#define  LORAWAN_EU868_LC3                                   { 866500000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 1 }

#define LORAWAN_EU868_CHANNELS_MASK_SIZE 1

/*!
 * LoRaMac channels which are allowed for the join procedure
 */
#define  LORAWAN_EU868_JOIN_CHANNELS                         ( uint16_t )( LC( 1 ) | LC( 2 ) | LC( 3 ) )

radio_rfm9x_bw_t lorawan_get_bandwidth(int8_t dr_idx);
uint8_t lorawan_get_max_payload_of_data_rate_repeater(int8_t dr_idx);
uint8_t lorawan_get_max_payload_of_data_rate(int8_t dr_idx);
int8_t lorawan_get_phy_data_rate(int8_t dr_idx);
double lorawan_compute_symbol_time_lora(int8_t dr_idx);
double lorawan_compute_symbol_time_fsk(int8_t dr_idx);
int8_t lorawan_appy_data_rate_offset(uint8_t downlink_dwell_time, int8_t dr, int8_t dr_offset);
int8_t lorawan_compute_phy_tx_power(int8_t tx_power_idx, float max_eirp, float antenna_gain);
uint8_t lorawan_adr_next(lorawan_adr_next_params_t * adr_next, int8_t * dr_out, int8_t * tx_power_out,
                         uint32_t * adr_ack_counter);
int8_t lorawan_get_alternate_dr(uint16_t nb_trials);

#endif
