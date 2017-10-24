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

typedef struct
{
	radio_rfm9x_t * radio;

	lorawan_device_class_t device_class;

	// addresses (4 bytes)
	uint32_t lora_mac_dev_addr;

	// reception window delay
	uint32_t rx_window_delay1;
	uint32_t rx_window_delay2;

	// timers
	xTimerHandle rx_window_timer1;
	xTimerHandle rx_window_timer2;
	xTimerHandle ack_timeout_timer;
	xTimerHandle mac_state_check_timer;

	// counters
	uint16_t uplink_counter;
	uint16_t downlink_counter;

	// states
	bool node_ack_requested;
	bool srv_ack_requested;
	bool mac_command_in_next_tx;

	bool is_lora_mac_network_joined;


} lorawan_mac_t;

void lorawan_mac_init(lorawan_mac_t * obj, radio_rfm9x_t * radio);


#endif // USE_FREERTOS == 1

#endif // LORAWAN_MAC_H_
