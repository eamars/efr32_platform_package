/**
 * LoRaWAN sub layer
 */

#ifndef MCPS_H_
#define MCPS_H_

#include <stdint.h>

typedef enum
{
	LORAWAN_EVENT_INFO_STATUS_OK = 0,
	LORAWAN_EVENT_INFO_STATUS_ERROR,
	LORAWAN_EVENT_INFO_STATUS_TX_TIMEOUT,
	LORAWAN_EVENT_INFO_STATUS_RX1_TIMEOUT,
	LORAWAN_EVENT_INFO_STATUS_RX2_TIMEOUT,
	LORAWAN_EVENT_INFO_STATUS_RX1_ERROR,
	LORAWAN_EVENT_INFO_STATUS_RX2_ERROR,
	LORAWAN_EVENT_INFO_STATUS_JOIN_FAIL,
	LORAWAN_EVENT_INFO_STATUS_DOWNLINK_REPEATED,
	LORAWAN_EVENT_INFO_STATUS_TX_DR_PAYLOAD_SIZE_ERROR,
	LORAWAN_EVENT_INFO_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS,
	LORAWAN_EVENT_INFO_STATUS_ADDRESS_FAIL,
	LORAWAN_EVENT_INFO_STATUS_MIC_FAIL
} lorawan_mac_event_info_status_t;

typedef enum
{
	MCPS_UNCONFIRMED,
	MCPS_CONFIMRED,
	MCSS_MULTICAST,
	MCPS_PROPRIETARY
} mcps_t;

typedef struct
{
	uint8_t fport;
	void * buffer;
	uint16_t buffer_size;
	int8_t data_rate;
} mcps_req_unconfirmed_t;

typedef struct
{
	uint8_t fport;
	void * buffer;
	uint16_t buffer_size;
	int8_t data_rate;
	uint8_t nb_trials;
} mcps_req_confirmed_t;

typedef struct
{
	void * buffer;
	uint16_t buffer_size;
	int8_t data_rate;
} mcps_req_proprietary_t;

typedef struct
{
	mcps_t type;
	union
	{
		mcps_req_unconfirmed_t unconfirmed;
		mcps_req_confirmed_t confirmed;
		mcps_req_proprietary_t proprietary;
	} req;
} mcps_req_t;

typedef struct
{
	// holds the perviously performed mcps-request
	mcps_t mcps_request;
	lorawan_mac_event_info_status_t status;

	uint8_t datarate;
	int8_t tx_power;
	bool ack_received;
	uint8_t nb_retries;
	uint32_t tx_time_on_air;
	uint32_t uplink_counter;
	uint32_t uplink_frequency;
} mcps_confirm_t;

typedef struct
{
	mcps_t mcps_indication;
	lorawan_mac_event_info_status_t status;

	uint8_t multicast;
	uint8_t port;
	uint8_t rx_datarate;
	uint8_t frame_pending;
	uint8_t * buffer;
	uint8_t buffer_size;
	bool rx_data;
	int16_t rssi;
	uint8_t snr;
	uint8_t rx_slot;
	bool ack_received;
	uint32_t downlink_counter;
} mcps_indication;

typedef enum
{
	MLME_JOIN,
	MLME_LINK_CHECK,
	MLME_TXCW,
	MLME_TXCW_1
} mlme_t;

typedef struct
{
	uint8_t * dev_eui;
	uint8_t * app_eui;
	uint8_t * app_key;
	uint8_t nb_trials;
} mlme_req_join_t;

typedef struct
{
	uint16_t timeout;
	uint32_t frequency;
	uint8_t power;
} mlme_req_tx_cw_t;

typedef struct
{
	mlme_t type;
	union
	{
		mlme_req_join_t join;
		mlme_req_tx_cw_t txcw;
	} req;
} mlme_req_t;

typedef struct
{
	mlme_t mlme_request;
	lorawan_mac_event_info_status_t status;
	uint32_t tx_time_on_air;
	uint8_t demod_margin;
	uint8_t nb_gateways;
	uint8_t nb_retries;
} mlme_confirm_t;

typedef enum
{
	MIB_DEVICE_CLASS,
	MIB_NETWORK_JOINED,
	MIB_ADR,
	MIB_NET_ID,
	MIB_DEV_ADDR,
	MIB_NWK_SESSION_KEY,
	MIB_APP_SESSION_KEY,
	MIB_PUBLIC_NETWORK,
	MIB_REPEATER_SUPPORT,
	MIB_CHANNELS,
	MIB_RX2_CHANNEL,
	MIB_RX2_DEFAULT_CHANNEL,
	MIB_CHANNELS_MASK,
	MIB_CHANNELS_DEFAULT_MASK,
	MIB_CHANNELS_NB_REP,
	MIB_MAX_RX_WINDOW_DURATION,
	MIB_RECEIVE_DELAY_1,
	MIB_RECEIVE_DELAY_2,
	MIB_JOIN_ACCEPT_DELAY_1,
	MIB_JOIN_ACCEPT_DELAY_2,
	MIB_CHANNELS_DEFAULT_DATARATE,
	MIB_CHANNELS_DATARATE,
	MIB_CHANNELS_TX_POWER,
	MIB_CHANNELS_DEFAULT_TX_POWER,
	MIB_UPLINK_COUNTER,
	MIB_DOWNLINK_COUNTER,
	MIB_MULTICAST_CHANNEL,
	MIB_MIN_RX_SYMBOLS,
	MIB_ANTENNA_GAIN
} mib_t;

typedef struct
{
	lorawan_device_class_t device_class;
	bool is_network_joined;
	bool adr_enable;
	uint32_t net_id;
	uint32_t device_addr;
	uint8_t * nwk_session_key;
	uint8_t * app_session_key;
	bool enable_public_network;
	bool enable_repeater_support;
};

#endif // MCPS_H_
