/**
 * @brief MAC layer PDU and implementation
 * @date 18, Oct, 2017
 * @author Ran Bao
 * @file lorawan_mac.c
 */

#include <drivers/radio_rfm9x/radio_rfm9x.h>
#include "lorawan_mac.h"
#include "drv_debug.h"
#include "lorawan_types.h"

static void lorawan_mac_rx_thread(lorawan_mac_t * obj)
{
	while (1)
	{
		radio_rfm9x_msg_t msg;

		radio_rfm9x_recv_block(obj->radio, &msg);

		// apply MAC header
		lorawan_mac_header_t * header = (lorawan_mac_header_t *) &msg.buffer;

		// do things based on mac header type
		switch (header->message_type)
		{
			case LORAWAN_MHDR_JOIN_REQUEST:
				break;
			case LORAWAN_MHDR_JOIN_ACCEPT:
				break;
			case LORAWAN_MHDR_UNCONFIRMED_DATA_UP:
				break;
			case LORAWAN_MHDR_UNCONFIRMED_DATA_DOWN:
				break;
			case LORAWAN_MHDR_CONFIRMED_DATA_UP:
				break;
			case LORAWAN_MHDR_CONFIRMED_DATA_DOWN:
				break;
			case LORAWAN_MHDR_RFU:
				break;
			case LORAWAN_MHDR_PROPRIETARY:
				break;
			default:
				DRV_ASSERT(false);
				break;
		}
	}
}

void lorawan_mac_init(lorawan_mac_t * obj, radio_rfm9x_t * radio)
{
	DRV_ASSERT(obj);
	DRV_ASSERT(radio);

	// copy variables
	obj->radio = radio;

	// create a thread for handling PHY payload
	xTaskCreate((void *) lorawan_mac_rx_thread, "mac_rx", 200, NULL, 2, &obj->rx_thread_handler);
}
