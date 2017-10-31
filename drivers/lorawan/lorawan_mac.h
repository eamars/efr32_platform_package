/**
 * @brief MAC layer PDU and implementation
 * @date 18, Oct, 2017
 * @author Ran Bao
 * @file lorawan_mac.h
 */

#ifndef LORAWAN_MAC_H_
#define LORAWAN_MAC_H_

#if USE_FREERTOS == 1

#include <stdint.h>

#include "radio_rfm9x.h"
#include "lorawan_types.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "bits.h"

#if USE_BAND_868 == 1
#include "lorawan_band_868.h"
#else
#error "You need to define at least one band"
#endif

#define STATE_SET(dest, value) BITS_SET((dest), (value))
#define STATE_CLEAR(dest, value) BITS_CLEAR((dest), (value))
#define STATE_CHECK(target, test) ((target) & (test))

#define LORAWAN_MAX_JOIN_REQUEST_TRAILS (1)
#define LORAWAN_DUTY_CYCLE_ON false
#define LORAWAN_MAC_COMMAND_MAX_LENGTH                 128

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

typedef union
{
	uint8_t byte;

	struct
	{
		uint8_t mcps_req : 1;
		uint8_t mcps_ind : 1;
		uint8_t mcps_ind_skip : 1;
		uint8_t mlme_req : 1;
		uint8_t mac_done : 1;
	};
} lorawan_mac_flags_t;

typedef struct
{
	uint32_t frequency;
	uint8_t data_rate;
} lorawan_rx2_channel_params_t;

typedef struct
{
	int8_t channels_tx_power;
	int8_t channels_data_rate;
	uint32_t system_max_rx_error;
	uint8_t min_rx_symbols;
	uint8_t max_rx_symbols;
	uint32_t max_rx_window;
	uint32_t receive_delay_1;
	uint32_t receive_delay_2;
	uint32_t join_accept_delay_1;
	uint32_t join_accept_delay_2;
	uint8_t channels_nb_rep;
	uint8_t rx1_dr_offset;
	lorawan_rx2_channel_params_t rx2_channel;
	uint8_t uplink_dwell_time;
	uint8_t downlink_dwell_time;

	float max_eirp;
	float antenna_gain;
} lorawan_mac_params_t;

typedef struct
{
	int8_t value;

	struct
	{
		int8_t min : 4;
		int8_t max : 4;
	};
} lorawan_data_rate_range_t;

typedef struct
{
	uint32_t frequency;
	uint32_t rx1_frequency;
	lorawan_data_rate_range_t data_rate_range;
	uint8_t band;
} lorawan_channel_t;

typedef struct
{
	uint8_t channel;
	int8_t data_rate;
	uint8_t bandwidth;
	int8_t data_rate_offset;
	uint32_t frequency;
	uint32_t window_timeout;
	int32_t window_offset;
	uint8_t downlink_dwell_time;

	bool repeater_support;
	bool rx_continuous;
	uint8_t window;
} lorawan_rx_config_params_t;

typedef struct
{
	uint8_t channel;
	int8_t data_rate;
	int8_t tx_power;
	float max_eirp;
	float antenna_gain;
	uint16_t packet_len;
} lorawan_tx_config_params_t;

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

typedef struct
{
	radio_rfm9x_t * radio;

	// rx thread (to prevent data to be processed in interrupt handler that might take extreme long time)
	xQueueHandle rx_queue_pri;
	xTaskHandle rx_thread_handle;

	// timers
	xTimerHandle rx_window1_timer;
	xTimerHandle rx_window2_timer;
	xTimerHandle ack_timeout_timer;
	xTimerHandle mac_state_check_timer;

	// addresses (4 bytes)
	uint64_t device_eui64; // unique device identification
	uint64_t application_eui64; // unique application eui (why)
	uint32_t device_addr;
	uint32_t network_id; // 3 byte

	// internal state machine
	lorawan_mac_state_t mac_state;
	lorawan_mac_flags_t mac_flags;

	// LoRaWAN device class
	lorawan_device_class_t device_class;

	// parameters
	lorawan_mac_params_t mac_params;

	uint8_t join_request_trials;
	uint8_t max_join_request_trails;
	bool repeater_support;
	bool duty_cycle_on;
	bool is_lorawan_network_joined;
	uint32_t uplink_counter;
	uint32_t downlink_counter;
	uint32_t adr_ack_counter;
	uint8_t channels_nb_rep_counter;
	uint8_t ack_timeout_retries;
	uint8_t ack_timeout_retries_counter;
	bool ack_timeout_retry;
	bool is_rx_windows_enabled;
	bool node_ack_requested;
	bool srv_ack_requested;
	bool mac_commands_in_next_tx;
	uint8_t channel; // channel index;
	uint8_t last_tx_channel;
	bool last_tx_is_join_request;

	uint32_t rx_window1_delay;
	uint32_t rx_window2_delay;

	bool adr_ctrl_on;

	lorawan_channel_t channels[LORAWAN_EU868_MAX_NB_CHANNELS];

	uint8_t rx_slot;
	lorawan_rx_config_params_t rx_window1_config;
	lorawan_rx_config_params_t rx_window2_config;

	uint16_t device_nonce;

	uint8_t * app_key;
	uint8_t * nwk_session_key;
	uint8_t * app_session_key;

	// local buffer
	uint8_t mac_commands_buffer[LORAWAN_MAC_COMMAND_MAX_LENGTH];
	uint8_t mac_commands_buffer_index;

	uint8_t mac_commands_buffer_to_repeat[LORAWAN_MAC_COMMAND_MAX_LENGTH];
	uint8_t mac_commands_buffer_to_repeat_index;

	uint8_t mac_buffer[RADIO_RFM9X_RW_BUFFER_SIZE];
	uint16_t mac_buffer_packet_length;

	uint16_t mac_tx_payload_length;

	uint8_t mac_rx_payload[RADIO_RFM9X_RW_BUFFER_SIZE];


} lorawan_mac_t;

void lorawan_mac_init(lorawan_mac_t * obj, radio_rfm9x_t * radio, uint64_t * device_eui64, uint64_t * application_eui64);

lorawan_mac_status_t lorawan_mac_send(lorawan_mac_t * obj, lorawan_mac_header_t * mac_header, uint8_t fport, void * buffer, uint16_t size);

#endif // USE_FREERTOS == 1

#endif // LORAWAN_MAC_H_
