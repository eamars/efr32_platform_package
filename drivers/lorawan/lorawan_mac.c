/**
 * @brief MAC layer PDU and implementation
 * @date 18, Oct, 2017
 * @author Ran Bao
 * @file lorawan_mac.c
 */
#include <math.h>
#include <stdlib.h>

#include "lorawan_mac.h"
#include "drv_debug.h"
#include "lorawan_types.h"
#include "lorawan_mac_crypto.h"
#include "utils.h"

/*!
 * Class A&B receive delay 1 in ms
 */
#define RECEIVE_DELAY1                              1000

/*!
 * Class A&B receive delay 2 in ms
 */
#define RECEIVE_DELAY2                              2000

#define JOIN_ACCEPT_DELAY1                          5000
#define JOIN_ACCEPT_DELAY2                          6000
#define MAX_FCNT_GAP                                16384
#define ADR_ACK_LIMIT                               64
#define ADR_ACK_DELAY                               32
#define ACK_TIMEOUT                                 1000 // random delay between 1 to 3 sec
#define SYNC_WORD                                   0x34
#define PREAMBLE_LEN                                8
#define MIC_LEN ( 4 )
#define LORAWAN_MAX_FCNT_GAP 16384


static void lorawan_compute_rx_window_parameters_pri(lorawan_mac_t * obj, lorawan_data_rate_t data_rate, uint32_t rx_error,
                                                     lorawan_mac_rx_config_params_t * rx_config_param)
{
	double symbol_time = 0.0;

	// copy datarate
	rx_config_param->data_rate = data_rate;

	// get bandwidth from lookup table
	rx_config_param->bandwidth = lorawan_mac_bandwidth_lookup_table[data_rate];

	if (data_rate == LORAWAN_DATARATE_7)
	{
		// fsk mode
		symbol_time = (1.0 / (double) lorawan_mac_data_rate_lookup_table[data_rate]) * 8.0; // 1 symbol equals to 1 byte ( 8bits )
	}
	else
	{
		// lora mode
		symbol_time =
				((double) (1 << lorawan_mac_data_rate_lookup_table[data_rate]) /
				 (double) lorawan_mac_bandwidth_lookup_table[data_rate]) * 1e3;
	}

	// compute number of symbols
	rx_config_param->rx_window_timeout =
			MAX(
					(uint32_t) ceil(((2 * obj->mac_params.min_rx_symbols - 8) * symbol_time + 2 * rx_error) / symbol_time),
					obj->mac_params.min_rx_symbols
			);

	rx_config_param->rx_offset =
			(int32_t) ceil((4.0 * symbol_time) - ((rx_config_param->rx_window_timeout * symbol_time) / 2.0) -
			               RADIO_RFM9X_WAKEUP_TIME);
}


static bool lorawan_mac_rx_window_setup_pri(lorawan_mac_t * obj, uint32_t freq, lorawan_data_rate_t data_rate, radio_rfm9x_bw_t bw,
                                        uint32_t timeout, bool rx_contineous)
{
	if (obj->radio->radio_op_state == RADIO_RFM9X_OP_STDBY)
	{
		// set frequency
		radio_rfm9x_set_channel(obj->radio, freq);

		if (data_rate == LORAWAN_DATARATE_7)
		{
			// fsk
			DRV_ASSERT(false);
		}
		else
		{
			// lora

			// set bandwidth
			radio_rfm9x_set_bandwidth(obj->radio, bw);

			// set spreading factor
			radio_rfm9x_set_spreading_factor(obj->radio, lorawan_mac_data_rate_lookup_table[data_rate]);
		}

		// set rx mode
		radio_rfm9x_set_opmode_rx_timeout(obj->radio, rx_contineous ? 0 : obj->mac_params.max_rx_window);

		return true;
	}

	return false;
}

static inline void lorawan_mac_rx_window_timer1_handler_pri(lorawan_mac_t * obj)
{
	if (obj->device_class == LORAWAN_CLASS_C)
	{
		radio_rfm9x_set_opmode_stdby(obj->radio);
#if USE_BAND_868 == 1
		lorawan_mac_rx_window_setup_pri(obj,
		                                lorawan_mac_channel_frequency_lookup_table[obj->channel],
			                            obj->rx_window_param1.data_rate,
			                            obj->rx_window_param1.bandwidth,
			                            obj->rx_window_param1.rx_window_timeout,
			                            false);
#else
#error "You should define a frequency band!"
#endif
	}

	radio_rfm9x_set_opmode_rx(obj->radio);
}

static inline void lorawan_mac_rx_window_timer2_handler_pri(lorawan_mac_t * obj)
{
	bool rx_contineous = false;

	if (obj->device_class == LORAWAN_CLASS_C)
	{
		rx_contineous = true;
	}

	if (lorawan_mac_rx_window_setup_pri(obj,
		                                obj->mac_params.rx2_channel_params.frequency,
		                                obj->rx_window_param2.data_rate,
		                                obj->rx_window_param2.bandwidth,
		                                obj->rx_window_param2.rx_window_timeout,
		                                rx_contineous)
	)
	{
		obj->rx_slot = true;
	}
}

static inline void lorawan_mac_ack_handler_pri(lorawan_mac_t * obj)
{
	if (obj->node_ack_requested)
	{
		obj->ack_timeout_retry = true;
		// TODO: change loramac state
	}
	if (obj->device_class == LORAWAN_CLASS_C)
	{
		// TODO: mac done
	}
}

static void lorawan_mac_state_check_handler_pri(lorawan_mac_t * obj)
{

}

static inline void lorawan_mac_prepare_rx_done_abort_pri(lorawan_mac_t * obj)
{
	STATE_SET(obj->state, LORAWAN_MAC_RX_ABORT);

	if (obj->node_ack_requested)
	{
		lorawan_mac_ack_handler_pri(obj);
	}

	// trigger mac state check
	xTimerChangePeriod(obj->mac_state_check_timer, pdMS_TO_TICKS(1), portMAX_DELAY);
	xTimerStart(obj->mac_state_check_timer, portMAX_DELAY);
}

static void lorawan_mac_rx_window_timer1_callback(TimerHandle_t xTimer)
{
	DRV_ASSERT(xTimer);

	xTimerStop(xTimer, 0);

	// restore the timer id (which is used to store the parameter
	lorawan_mac_t * obj = (lorawan_mac_t *) pvTimerGetTimerID(xTimer);

	lorawan_mac_rx_window_timer1_handler_pri(obj);
}

static void lorawan_mac_rx_window_timer2_callback(TimerHandle_t xTimer)
{
	DRV_ASSERT(xTimer);

	xTimerStop(xTimer, 0);

	// restore the timer id (which is used to store the parameter
	lorawan_mac_t * obj = (lorawan_mac_t *) pvTimerGetTimerID(xTimer);

	lorawan_mac_rx_window_timer2_handler_pri(obj);
}

static void lorawan_mac_ack_timeout_callback(TimerHandle_t xTimer)
{
	DRV_ASSERT(xTimer);

	xTimerStop(xTimer, 0);

	// restore the timer id (which is used to store the parameter
	lorawan_mac_t * obj = (lorawan_mac_t *) pvTimerGetTimerID(xTimer);

	lorawan_mac_ack_handler_pri(obj);
}

static void lorawan_mac_state_check_timeout_callback(TimerHandle_t xTimer)
{
	DRV_ASSERT(xTimer);

	xTimerStop(xTimer, 0);

	// restore the timer id (which is used to store the parameter
	lorawan_mac_t * obj = (lorawan_mac_t *) pvTimerGetTimerID(xTimer);

	lorawan_mac_state_check_handler_pri(obj);
}

static void lorawan_mac_process_mac_commands_pri(lorawan_mac_t * obj, uint8_t * payload, uint8_t mac_index, uint8_t cmd_size, int8_t snr)
{

}

static void lorawan_mac_on_rx_done_isr(uint8_t * msg, int16_t size, int16_t rssi, int8_t snr, lorawan_mac_t * obj)
{
	// encapsulate packet in lorawan_mac_msg
	lorawan_mac_msg_ext_t rx_msg;

	// copy variables
	rx_msg.msg.size = size;
	rx_msg.rssi = rssi;
	rx_msg.snr  = snr;
	memcpy(rx_msg.msg.buffer, msg, (size_t) rx_msg.msg.size);

	// send to thread
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(obj->rx_queue_pri, &rx_msg, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

static void lorawan_mac_on_tx_done_isr(lorawan_mac_t * obj)
{
	if (obj->device_class != LORAWAN_CLASS_C)
	{
		radio_rfm9x_set_opmode_sleep(obj->radio);
	}
	else
	{
		lorawan_mac_rx_window_timer2_callback(NULL);
	}

	// at the moment the packet is transmitted, we are going to setup two rx slots, with corresponding delays

	// setup first time slot, with update-to-date rx_window_delay1
	xTimerChangePeriodFromISR(obj->rx_window_timer1, pdMS_TO_TICKS(obj->rx_window_delay1), NULL);
	xTimerStartFromISR(obj->rx_window_timer1, NULL);

	if (obj->device_class != LORAWAN_CLASS_C)
	{
		// setup the second time slot, with update-to-date rx_window_delay2
		xTimerChangePeriodFromISR(obj->rx_window_timer2, pdMS_TO_TICKS(obj->rx_window_delay2), NULL);
		xTimerStartFromISR(obj->rx_window_timer2, NULL);
	}

	if (obj->device_class == LORAWAN_CLASS_C || obj->node_ack_requested == true)
	{
		// TODO: Implement ack timeout timer with correct delay time
		// RxWindow2Delay + ACK_TIMEOUT + randr( -ACK_TIMEOUT_RND, ACK_TIMEOUT_RND )
		xTimerChangePeriodFromISR(obj->ack_timeout_timer, pdMS_TO_TICKS(obj->rx_window_delay2), NULL);
		xTimerStartFromISR(obj->rx_window_timer2, NULL);
	}

	// TODO: calculate backoff time
}

static void lorawan_mac_on_rx_error_isr(lorawan_mac_t * obj)
{

}

static void lorawan_mac_on_rx_timeout_isr(lorawan_mac_t * obj)
{

}


static void lorawan_mac_rx_thread(lorawan_mac_t * obj)
{
	while (1)
	{
		// block read data from radio
		lorawan_mac_msg_ext_t rx_msg;
		DRV_ASSERT(xQueueReceive(obj->rx_queue_pri, &rx_msg, portMAX_DELAY));

		// TODO; The most data processing can actually be done in thread.
		uint8_t * raw_packet_buffer = rx_msg.msg.buffer;

		// counters
		uint16_t downlink_counter;

		// decoded payload
		uint8_t decoded_payload[RADIO_RFM9X_RW_BUFFER_SIZE];

		// multicast
		bool multicast = false;

		// put the radio to the sleep state
		radio_rfm9x_set_opmode_sleep(obj->radio);

		// stop the rx timer
		xTimerStopFromISR(obj->rx_window_timer2, NULL);

		// apply header to the received buffer
		lorawan_mac_header_t * mac_header = (lorawan_mac_header_t *) raw_packet_buffer;
		raw_packet_buffer += 1;

		// do things based on mac header type
		switch (mac_header->message_type)
		{
			case LORAWAN_MHDR_JOIN_ACCEPT:
			{
				uint32_t calc_mic;

				// TODO: These values should be placed in the object
				uint8_t * lorawan_app_key = NULL;

				// device nonce is a random value extracted by issuing a sequence of RSSI measurement
				uint16_t lorawan_mac_dev_nonce = 0;

				if (obj->is_lora_mac_network_joined)
				{
					// oops we have already joined the network
					lorawan_mac_prepare_rx_done_abort_pri(obj);
					return;
				}

				lorawan_mac_join_decrypt(rx_msg.msg.buffer + 1, (uint16_t) (rx_msg.msg.size - 1), lorawan_app_key, decoded_payload + 1);

				decoded_payload[0] = mac_header->byte;

				lorawan_mac_join_compute_mic(decoded_payload, (uint16_t) (rx_msg.msg.size - MIC_LEN), lorawan_app_key, &calc_mic);

				// get rx_mic
				uint32_t * rx_mic = (uint32_t *) &rx_msg.msg.buffer[rx_msg.msg.size - MIC_LEN];

				if (*rx_mic == calc_mic)
				{
					lorawan_mac_join_compute_session_key(lorawan_app_key, decoded_payload + 1, lorawan_mac_dev_nonce,
					                                     obj->network_session_key, obj->application_session_key);
					// TODO: get lora mac net id
					// TODO: get lora mac device addr
					// TODO: read DL settings

					// TODO: read rx delay

				}

				break;
			}

			case LORAWAN_MHDR_CONFIRMED_DATA_DOWN:
			case LORAWAN_MHDR_UNCONFIRMED_DATA_DOWN:
			{
				// decode frame header
				uint32_t * dest_dev_addr = (uint32_t *) (raw_packet_buffer);
				raw_packet_buffer += 4;

				// tell if the destination is the local device
				if (*dest_dev_addr != obj->device_addr)
				{
					// TODO: multicast?
					multicast = true;
				}
				else
				{
					// TODO:
					multicast = false;
				}

				// read frame control
				lorawan_mac_fhdr_fctrl_t * fctrl = (lorawan_mac_fhdr_fctrl_t *) raw_packet_buffer;
				raw_packet_buffer += 1;

				// read 2 byte fcnt
				uint16_t * fcnt = (uint16_t *) raw_packet_buffer;
				raw_packet_buffer += 2;

				// read fopts
				uint8_t * fopts = (uint8_t *) raw_packet_buffer;
				raw_packet_buffer += fctrl->foptslen;

				// read frame payload (the length of frame payload is calculated by the length of payload - existing fields
				uint8_t * frmpayload = raw_packet_buffer;

				// read 4 byte mic
				uint32_t * rx_mic = (uint32_t *) &rx_msg.msg.buffer[rx_msg.msg.size - MIC_LEN];

				// calculate downlink counter different
				uint16_t counter_diff = *fcnt - obj->downlink_counter;

				// test if roll over
				if (counter_diff < (1 << 15))
				{
					obj->downlink_counter += counter_diff;
				}
				else
				{
					obj->downlink_counter = (uint16_t) (obj->downlink_counter + 0x10000 + counter_diff);
				}

				uint32_t calc_mic = 0;
				lorawan_mac_compute_mic(rx_msg.msg.buffer, (uint16_t) (rx_msg.msg.size - MIC_LEN), obj->network_session_key, *dest_dev_addr,
				                        LORAWAN_DOWN_LINK, obj->downlink_counter, &calc_mic);

				// check for the maximum allowed counter difference
				if (counter_diff >= LORAWAN_MAX_FCNT_GAP)
				{
					// too many frames were lost
					lorawan_mac_prepare_rx_done_abort_pri(obj);
					return;
				}

				if (*rx_mic == calc_mic)
				{
					// TODO: make it global
					bool server_ack_requested = false;

					// confirm data
					if (mac_header->message_type == LORAWAN_MHDR_CONFIRMED_DATA_DOWN)
					{
						server_ack_requested = true;

						// TODO: check downlink counter
					}
					else if (mac_header->message_type == LORAWAN_MHDR_UNCONFIRMED_DATA_DOWN)
					{
						server_ack_requested = false;

						// TODO: check downlink counter
					}
					else
					{
						DRV_ASSERT(false);
					}

					// TODO: set downlink counter
					// TODO: MCPS confirm?

					// process payload and mac commands
					uint8_t port = frmpayload[0];
					uint8_t frame_length = (uint8_t) (rx_msg.msg.size - MIC_LEN - (frmpayload - rx_msg.msg.buffer));

					if (((rx_msg.msg.size - 4) - (frmpayload - rx_msg.msg.buffer)) > 0 )
					{
						// if the port is 0, it indicates that the FRMPayload contains MAC command only
						if (port == 0)
						{
							if (fctrl->foptslen == 0)
							{
								lorawan_mac_payload_decrypt(frmpayload + 1, frame_length, obj->network_session_key,
								                            *dest_dev_addr,
								                            LORAWAN_DOWN_LINK, obj->downlink_counter, decoded_payload);

								// process mac command
								lorawan_mac_process_mac_commands_pri(obj, decoded_payload, 0, frame_length, rx_msg.snr);
							} else
							{
								// TODO: skip indication
							}
						} else
						{
							// for those port > 0
							if (fctrl->foptslen > 0)
							{
								// decode options field MAC command, omit the fport
								lorawan_mac_process_mac_commands_pri(obj, rx_msg.msg.buffer, 8, (uint8_t) (frmpayload - rx_msg.msg.buffer - 1),
								                                     rx_msg.snr);
							}
							lorawan_mac_payload_decrypt(frmpayload, frame_length, obj->application_session_key,
							                            *dest_dev_addr,
							                            LORAWAN_DOWN_LINK, obj->downlink_counter, decoded_payload);
						}
					}
					else
					{
						if (fctrl->foptslen > 0)
						{
							lorawan_mac_process_mac_commands_pri(obj, rx_msg.msg.buffer, 8, (uint8_t) (frmpayload - rx_msg.msg.buffer - 1), rx_msg.snr);
						}
					}

					// check if the frame is ack
					if (fctrl->ack)
					{
						xTimerStop(obj->ack_timeout_timer, portMAX_DELAY);
					}
					else
					{
						// TODO: need to stop the timer as no more retransmissions are need
						xTimerStop(obj->ack_timeout_timer, portMAX_DELAY);
					}
				}
				else
				{
					// mic mismatch
					lorawan_mac_prepare_rx_done_abort_pri(obj);
					return;
				}

				// set mac state check timer
				xTimerChangePeriod(obj->mac_state_check_timer, pdMS_TO_TICKS(1), portMAX_DELAY);
				xTimerStart(obj->mac_state_check_timer, portMAX_DELAY);

				break;
			}

			case LORAWAN_MHDR_JOIN_REQUEST:
			{
				break;
			}

			case LORAWAN_MHDR_UNCONFIRMED_DATA_UP:
			{
				break;
			}

			case LORAWAN_MHDR_CONFIRMED_DATA_UP:
			{
				break;
			}



			case LORAWAN_MHDR_RFU:
			{
				break;
			}

			case LORAWAN_MHDR_PROPRIETARY:
			{
				break;
			}

			default:
			{
				DRV_ASSERT(false);
				break;
			}
		}
	}
}


static lorawan_mac_status_t lorawan_mac_prepare_frame_pri(lorawan_mac_t * obj, lorawan_mac_header_t * mac_header,
                                          lorawan_mac_fhdr_fctrl_t * fctrl,
                                          uint8_t fport, void * buffer, uint16_t size)
{
	uint8_t * packet_ptr = obj->internal_message_buffer.buffer;

	obj->internal_message_buffer.size = size;

	obj->internal_message_buffer.buffer[0] = mac_header->byte;
	packet_ptr += 1;

	switch(mac_header->message_type)
	{
		case LORAWAN_MHDR_JOIN_REQUEST:
		{
			memcpy(packet_ptr, &obj->application_eui64, 8);
			packet_ptr += 8;

			memcpy(packet_ptr, &obj->device_eui64, 8);
			packet_ptr += 8;

			// read a random number to fill in device nonce
			obj->device_nonce = (uint16_t) random();

			memcpy(packet_ptr, &obj->device_nonce, 2);
			packet_ptr += 2;

			// compute mic
			uint32_t mic = 0;
			lorawan_mac_join_compute_mic(obj->internal_message_buffer.buffer, (uint8_t) (packet_ptr - obj->internal_message_buffer.buffer),
			                             obj->application_session_key, &mic);

			memcpy(packet_ptr, &mic, 4);
			packet_ptr += 4;

			break;
		}

		case LORAWAN_MHDR_CONFIRMED_DATA_UP:
		{
			obj->node_ack_requested = true;

			// NO BREAK!
		}

		case LORAWAN_MHDR_UNCONFIRMED_DATA_UP:
		{
			if (!obj->is_lora_mac_network_joined)
			{
				return LORAWAN_MAC_STATUS_NO_NETWORK_JOINED;
			}

			fctrl->adrackreq = adr_next_data_rate();

			if (obj->srv_ack_requested)
			{
				obj->srv_ack_requested = false;
				fctrl->ack = 1;
			}

			memcpy(packet_ptr, &obj->device_addr, 4);
			packet_ptr += 4;

			*packet_ptr = fctrl->byte;
			packet_ptr += 1;

			memcpy(packet_ptr, &obj->uplink_counter, 2);
			packet_ptr += 2;

			// copy mac command
			if (obj->internal_message_buffer.size > 0)
			{

			}

		}
	}

	return LORAWAN_MAC_STATUS_OK;
}


static lorawan_mac_status_t lorawan_mac_schedule_tx_pri(lorawan_mac_t * obj)
{
	return LORAWAN_MAC_STATUS_OK;
}


static void lorawan_mac_internal_reset_pri(lorawan_mac_t * obj)
{
	// set default state for internal state machine
	obj->state = LORAWAN_MAC_IDLE;

	// set default LoRaWAN device class to class A
	obj->device_class = LORAWAN_CLASS_A;

	// reset rf channel
	obj->channel = 0;

	// TODO: reset lorawan device address
	obj->device_addr = 0x0;

	// reset internal buffer
	memset(&obj->internal_message_buffer, 0x0, sizeof(lorawan_mac_msg_t));

	// reset device nonce
	obj->device_nonce = 0;

	// TODO: reset rx window delay
	obj->rx_window_delay1 = RECEIVE_DELAY1;
	obj->rx_window_delay2 = RECEIVE_DELAY2;

	// reset counters
	obj->uplink_counter = 0;
	obj->downlink_counter = 0;

	// reset states
	obj->node_ack_requested = false;
	obj->ack_timeout_retry = false;
	obj->srv_ack_requested = false;
	obj->mac_command_in_next_tx = false;
	obj->is_lora_mac_network_joined = false;
	obj->rx_slot = false;
	obj->adr_ctrl_on = false;

	// features
	obj->repeater_support = false;

	// reset encryption keys
	memset(obj->network_session_key, 0x0, sizeof(obj->network_session_key));
	memset(obj->application_session_key, 0x0, sizeof(obj->application_session_key));

	// TODO: reset rx windows parameters
	obj->rx_window_param1;
	obj->rx_window_param2;

	// TODO: reset mac parameters
	obj->mac_params;

}


void lorawan_mac_init(lorawan_mac_t * obj, radio_rfm9x_t * radio, uint64_t * device_eui64, uint64_t * application_eui64)
{
	DRV_ASSERT(obj);
	DRV_ASSERT(radio);

	// copy variables
	{
		obj->radio = radio;
		memcpy(&obj->device_eui64, device_eui64, sizeof(uint64_t));
		memcpy(&obj->application_eui64, application_eui64, sizeof(uint64_t));
	}


	// initialize radio
	{
		// configure the radio
		radio_rfm9x_set_modem(obj->radio, RADIO_RFM9X_MODEM_LORA);

		// set bandwidth
		radio_rfm9x_set_bandwidth(obj->radio, RADIO_RFM9X_BW_125K);

		// set coding rate
		radio_rfm9x_set_coding_rate(obj->radio, RADIO_RFM9X_CR_4_5);

		// set spreading factor
		radio_rfm9x_set_spreading_factor(obj->radio, RADIO_RFM9X_SF_128);

		// enable crc
		radio_rfm9x_set_crc_enable(obj->radio, true);

		// set preamble length
		radio_rfm9x_set_preamble_length(obj->radio, 8);

		// set operating frequency
		radio_rfm9x_set_channel(obj->radio, 867000000UL);

		// set tx power
		radio_rfm9x_set_tx_power(obj->radio, 23);

		// generate a random number as seed
		srandom(radio_rfm9x_generate_random_number(obj->radio));

		// set public network
		radio_rfm9x_set_public_network(obj->radio, true);

		// set the radio to sleep state
		radio_rfm9x_set_opmode_sleep(obj->radio);
	}



	// reset variables to its default value
	lorawan_mac_internal_reset_pri(obj);

	radio_rfm9x_set_rx_done_isr_callback(obj->radio, (void *) lorawan_mac_on_rx_done_isr, obj);
	radio_rfm9x_set_tx_done_isr_callback(obj->radio, (void *) lorawan_mac_on_tx_done_isr, obj);
	radio_rfm9x_set_rx_timeout_isr_callback(obj->radio, (void *) lorawan_mac_on_rx_timeout_isr, obj);
	radio_rfm9x_set_rx_error_isr_callback(obj->radio, (void *) lorawan_mac_on_rx_error_isr, obj);

	// create timer instance
	obj->rx_window_timer1 = xTimerCreate("rxw_t1", pdMS_TO_TICKS(obj->rx_window_delay1), pdFALSE, obj, lorawan_mac_rx_window_timer1_callback);
	obj->rx_window_timer2 = xTimerCreate("rxw_t2", pdMS_TO_TICKS(obj->rx_window_delay2), pdFALSE, obj, lorawan_mac_rx_window_timer2_callback);
	obj->ack_timeout_timer = xTimerCreate("ack_t", pdMS_TO_TICKS(obj->rx_window_delay2), pdFALSE, obj, lorawan_mac_ack_timeout_callback);
	obj->mac_state_check_timer = xTimerCreate("mac_chk", pdMS_TO_TICKS(1), pdFALSE, obj, lorawan_mac_state_check_timeout_callback);

	// create rx queue for internal data transaction
	obj->rx_queue_pri = xQueueCreate(2, sizeof(lorawan_mac_msg_t));

	// create rx thread handler
	xTaskCreate((void *) lorawan_mac_rx_thread, "mac_rx", 400, obj, 3, &obj->rx_thread_handle);
}



lorawan_mac_status_t lorawan_mac_send(lorawan_mac_t * obj, lorawan_mac_header_t * mac_header, uint8_t fport, void * buffer, uint16_t size)
{
	lorawan_mac_status_t status = LORAWAN_MAC_STATUS_PARAMETER_INVALID;

	// create a frame control byte
	lorawan_mac_fhdr_fctrl_t fctrl;
	fctrl.byte = 0;

	// set fctrl
	fctrl.foptslen = 0,
	fctrl.fpending = 0;
	fctrl.ack = 0;
	fctrl.adrackreq = 0;
	fctrl.adr = (uint8_t) (obj->adr_ctrl_on ? 1 : 0);

	// prepare frame
	if ((status = lorawan_mac_prepare_frame_pri(obj, mac_header, &fctrl, fport, buffer, size)) != LORAWAN_MAC_STATUS_OK)
	{
		return status;
	}

	if ((status = lorawan_mac_schedule_tx_pri(obj)) != LORAWAN_MAC_STATUS_OK)
	{
		return status;
	}

	return status;
}
