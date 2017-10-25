/**
 * @brief LoRaWAN type definitions
 * @date 18, Oct, 2017
 * @author Ran Bao
 * @file lorawan_types.h
 */

#ifndef LORAWAN_TYPES_H_
#define LORAWAN_TYPES_H_

#include <stdint.h>
#include "radio_rfm9x.h"

#if USE_BAND_868 == 1
#include "lorawan_mac_868_defs.h"
#elif USE_BAND_434 == 1
#include "lorawan_mac_434_defs.h"
#else
#error "You should define your band before including lorawan_mac.h"
#endif


/**
 * @brief LoRaWAN MAC message type
 */
typedef enum
{
	LORAWAN_MHDR_JOIN_REQUEST = 0x0,
	LORAWAN_MHDR_JOIN_ACCEPT = 0x1,
	LORAWAN_MHDR_UNCONFIRMED_DATA_UP = 0x2,
	LORAWAN_MHDR_UNCONFIRMED_DATA_DOWN = 0x3,
	LORAWAN_MHDR_CONFIRMED_DATA_UP = 0x4,
	LORAWAN_MHDR_CONFIRMED_DATA_DOWN = 0x5,
	LORAWAN_MHDR_RFU = 0x6,
	LORAWAN_MHDR_PROPRIETARY = 0x7
} lorawan_mhdr_message_type_t;

typedef enum
{
	LORAWAN_CLASS_A,
	LORAWAN_CLASS_B,
	LORAWAN_CLASS_C
} lorawan_device_class_t;

typedef enum
{
	LORAWAN_R1 = 0x0
} lorawan_major_version_t;

typedef enum
{
	LORAWAN_MAC_CMD_LINK_CHECK_REQ = 0x02,
	LORAWAN_MAC_CMD_LINK_CHECK_ANS = 0x02,

	LORAWAN_MAC_CMD_LINK_ADR_REQ = 0x03,
	LORAWAN_MAC_CMD_LINK_ADR_ANS = 0x03,

	LORAWAN_MAC_CMD_DUTY_CYCLE_REQ = 0x04,
	LORAWAN_MAC_CMD_DUTY_CYCLE_ANS = 0x04,

	LORAWAN_MAC_CMD_RX_PARAM_SETUP_REQ = 0x05,
	LORAWAN_MAC_CMD_RX_PARAM_SETUP_ANS = 0x05,

	LORAWAN_MAC_CMD_DEV_STATUS_REQ = 0x06,
	LORAWAN_MAC_CMD_DEV_STATUS_ANS = 0x06,

	LORAWAN_MAC_CMD_NEW_CHANNEL_REQ = 0x07,
	LORAWAN_MAC_CMD_NEW_CHANNEL_ANS = 0x07,

	LORAWAN_MAC_CMD_TX_TIMING_SETUP_REQ = 0x08,
	LORAWAN_MAC_CMD_TX_TIMING_SETUP_ANS = 0x08,
} lorawan_mac_cmd_cid;

/**
 * @brief MHDR
 * MAC message header
 */
typedef union
{
	uint8_t byte;

	struct __attribute__((packed))
	{
		// major
		uint8_t major           : 2;

		// reserve for future usage
		uint8_t rfu             : 3;

		// message type
		uint8_t message_type    : 3;
	};

} lorawan_mac_header_t;


/**
 * @brief FCtrl
 * Frame header control octet
 */
typedef union
{
	uint8_t byte;

	struct __attribute__((packed))
	{
		uint8_t foptslen        : 4;
		uint8_t fpending        : 1;
		uint8_t ack             : 1;
		uint8_t adrackreq       : 1;

		// adaptive data rate
		uint8_t adr             : 1;
	};
} lorawan_mac_fhdr_fctrl_t;

typedef enum
{
	LORAWAN_UP_LINK = 0,
	LORAWAN_DOWN_LINK = 1,
} lorawan_mac_comm_direction_t;

typedef struct
{
	lorawan_data_rate_t data_rate;
	radio_rfm9x_bw_t bandwidth;
	uint32_t rx_window_timeout;
	int32_t rx_offset;
} lorawan_mac_rx_config_params_t;

typedef struct
{
	uint32_t frequency;
	lorawan_data_rate_t data_rate_min;
	lorawan_data_rate_t data_rate_max;
	uint8_t band;
} lorawan_mac_channel_params_t;

typedef struct
{
	uint32_t frequency;
	lorawan_data_rate_t data_rate;
} lorawan_mac_rx2_channel_params_t;

typedef struct
{
	lorawan_tx_power_t channel_tx_power;
	lorawan_data_rate_t channel_data_rate;
	uint32_t system_max_rx_error;
	uint8_t min_rx_symbols;
	uint8_t max_rx_symbols;
	uint32_t max_rx_window;
	uint32_t receive_delay_1;
	uint32_t receive_delay_2;
	uint32_t join_accept_delay_1;
	uint32_t join_accept_delay_2;
	uint8_t channels_nb_rep;
	uint8_t rx1_data_rate_offset;
	lorawan_mac_rx2_channel_params_t rx2_channel_params;
	uint16_t channel_masks[6];
} lorawan_mac_params_t;

typedef enum
{
	LORAWAN_MAC_IDLE                = 0x00,
	LORAWAN_MAC_TX_RUNNING          = 0x01,
	LORAWAN_MAC_RX                  = 0x02,
	LORAWAN_MAC_ACK_REQ             = 0x04,
	LORAWAN_MAC_ACK_RETRY           = 0x08,
	LORAWAN_MAC_TX_DELAYED          = 0x10,
	LORAWAN_MAC_TX_CONFIG           = 0x20,
	LORAWAN_MAC_RX_ABORT            = 0x40
} lorawan_mac_state_t;

typedef enum
{
	LORAWAN_MAC_STATUS_OK,
	LORAWAN_MAC_STATUS_BUSY,
	LORAWAN_MAC_STATUS_SERVICE_UNKNOWN,
	LORAWAN_MAC_STATUS_PARAMETER_INVALID,
	LORAWAN_MAC_STATUS_FREQUENCY_INVALID,
	LORAWAN_MAC_STATUS_DATARATE_INVALID,
	LORAWAN_MAC_STATUS_FREQUENCY_AND_DATARATE_INVALID,
	LORAWAN_MAC_STATUS_NO_NETWORK_JOINED,
	LORAWAN_MAC_STATUS_PAYLOAD_LENGTH_ERROR,
	LORAWAN_MAC_STATUS_COMMAND_LENGTH_ERROR,
	LORAWAN_MAC_STATUS_DEVICE_OFF
} lorawan_mac_status_t;

typedef struct
{
	uint8_t buffer[RADIO_RFM9X_RW_BUFFER_SIZE];
	int16_t size;
} lorawan_mac_msg_t;

/**
 * @brief Physical layer message payload
 */
typedef struct
{
	lorawan_mac_msg_t msg;
	int16_t rssi;
	int8_t snr;
} lorawan_mac_msg_ext_t;


#endif // LORAWAN_TYPES_H_
