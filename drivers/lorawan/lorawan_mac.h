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

#define STATE_SET(dest, value) BITS_SET((dest), (value))
#define STATE_CLEAR(dest, value) BITS_CLEAR((dest), (value))
#define STATE_CHECK(target, test) ((target) & (test))

typedef struct
{
	radio_rfm9x_t * radio;

	// internal state machine
	lorawan_mac_state_t state;

	// rx thread (to prevent data to be processed in interrupt handler that might take extreme long time)
	xQueueHandle rx_queue_pri;
	xTaskHandle rx_thread_handle;

	// timers
	xTimerHandle rx_window_timer1;
	xTimerHandle rx_window_timer2;
	xTimerHandle ack_timeout_timer;
	xTimerHandle mac_state_check_timer;

	// LoRaWAN device class
	lorawan_device_class_t device_class;

	// addresses (4 bytes)
	uint64_t device_eui64; // unique device identification
	uint64_t application_eui64; // unique application eui (why)
	uint32_t device_addr;

	// current rf channel
	uint8_t channel;

	// internal properties
	lorawan_mac_msg_t internal_message_buffer;
	lorawan_mac_cmd_msg_t internal_mac_command_message_buffer;
	uint16_t device_nonce;

	// reception window delay
	uint32_t rx_window_delay1;
	uint32_t rx_window_delay2;

	// counters
	uint16_t uplink_counter;
	uint16_t downlink_counter;

	// states
	bool node_ack_requested;
	bool ack_timeout_retry;
	bool srv_ack_requested;
	bool mac_command_in_next_tx;
	bool is_lora_mac_network_joined;
	bool rx_slot;
	bool adr_ctrl_on;

	// features
	bool repeater_support;

	// aes encryption/decription cipher network session key
	uint8_t network_session_key[16];
	uint8_t application_session_key[16];

	// rx parameters
	lorawan_mac_rx_config_params_t rx_window_param1;
	lorawan_mac_rx_config_params_t rx_window_param2;

	// mac related params
	lorawan_mac_params_t mac_params;


} lorawan_mac_t;

void lorawan_mac_init(lorawan_mac_t * obj, radio_rfm9x_t * radio, uint64_t * device_eui64, uint64_t * application_eui64);

lorawan_mac_status_t lorawan_mac_send(lorawan_mac_t * obj, lorawan_mac_header_t * mac_header, uint8_t fport, void * buffer, uint16_t size);

#endif // USE_FREERTOS == 1

#endif // LORAWAN_MAC_H_
