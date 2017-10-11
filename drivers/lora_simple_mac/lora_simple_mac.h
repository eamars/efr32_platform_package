/**
 * @brief LoRa simple MAC layer protocol implementation
 * @author Ran Bao
 * @date 12, Oct, 2017
 * @file lora_simple_mac.h
 */

#ifndef LORA_SIMPLE_MAC_H_
#define LORA_SIMPLE_MAC_H_

#include "radio_rfm9x.h"
#include "subg_packet.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

typedef void (*mac_packet_rx_done_callback) (subg_packet_t * packet);
typedef void (*mac_packet_tx_done_callback) (void);

typedef struct
{
	radio_rfm9x_t * transceiver;

	// hardware id
	uint8_t device_id8;

	// tx and rx callbacks
	mac_packet_tx_done_callback tx_done_callback;
	mac_packet_rx_done_callback rx_done_callback;

	// rx daemon thread handler
	TaskHandle_t tx_thread_handler;

	// retransmission timer
	TimerHandle_t re_tx_timer;

} lora_simple_mac_t;



void lora_simple_mac_init(lora_simple_mac_t * obj, radio_rfm9x_t * transceiver,
                          uint8_t device_id8,
                          mac_packet_tx_done_callback tx_done_callback,
                          mac_packet_rx_done_callback rx_done_callback
);

void lora_simple_mac_send_packet(lora_simple_mac_t * obj, subg_packet_t * packet);


#endif // LORA_SIMPLE_MAC_H_
