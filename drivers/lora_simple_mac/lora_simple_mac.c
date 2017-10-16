/**
 * @brief LoRa simple MAC layer protocol implementation
 * @author Ran Bao
 * @date 12, Oct, 2017
 * @file lora_simple_mac.c
 */

#if USE_FREERTOS == 1

#include "lora_simple_mac.h"
#include "drv_debug.h"
#include "subg_packet.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

static void lora_simple_mac_rx_handler_thread(lora_simple_mac_t * obj)
{
	while (1)
	{
		radio_rfm9x_msg_t rx_msg;

		// block receiving data
		radio_rfm9x_recv_block(obj->transceiver, &rx_msg);

		switch (rx_msg.size)
		{
			case sizeof(subg_packet_header_t):
			{
				// This is an ack message
				subg_packet_header_t * header = (subg_packet_header_t *) &rx_msg;

				if (header->device_id8 == obj->device_id8)
				{
					// this is my ack, clear the ack for previous packet
					if (obj->tx_done_callback) obj->tx_done_callback();
				}

				// otherwise just drop the packet
				break;
			}
			case sizeof(subg_packet_t):
			{
				// message, need to tell the receiver
				subg_packet_t * packet = (subg_packet_t *) &rx_msg;

				if (packet->header.device_id8 == obj->device_id8)
				{
					// this is my packet process this!
					if (obj->rx_done_callback) obj->rx_done_callback(packet);
				}

				// otherwise just drop the packet
				break;
			}
			default:
				// drop other message
				break;
		}
	}
}


void lora_simple_mac_init(lora_simple_mac_t * obj, radio_rfm9x_t * transceiver,
                          uint8_t device_id8,
                          mac_packet_tx_done_callback tx_done_callback,
                          mac_packet_rx_done_callback rx_done_callback
)
{
	DRV_ASSERT(obj);
	DRV_ASSERT(transceiver);

	// copy variables
	obj->transceiver = transceiver;
	obj->device_id8 = device_id8;
	obj->tx_done_callback = tx_done_callback;
	obj->rx_done_callback = rx_done_callback;
	obj->re_tx_timer = NULL;

	// create a thread for rx handler
	xTaskCreate((void *) lora_simple_mac_rx_handler_thread, "lora_rx", 200, obj, 2, &obj->tx_thread_handler);
}


void lora_simple_mac_send_packet(lora_simple_mac_t * obj, subg_packet_t * packet)
{
	DRV_ASSERT(obj);
	DRV_ASSERT(packet);

	// block send a packet
	radio_rfm9x_msg_t tx_msg;

	tx_msg.size = sizeof(subg_packet_t);
	memcpy(tx_msg.buffer, packet, tx_msg.size);

	// block transmit the packet
	if (!radio_rfm9x_send(obj->transceiver, &tx_msg))
	{
		radio_rfm9x_hard_reset(obj->transceiver);
	}
}


#endif // USE_FREERTOS == 1
