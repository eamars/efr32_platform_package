/**
 * @brief MAC layer PDU and implementation
 * @date 18, Oct, 2017
 * @author Ran Bao
 * @file lorawan_mac.c
 */
#include <math.h>
#include <stdlib.h>
#include <drivers/radio_rfm9x/radio_rfm9x.h>

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

#define MAC_STATE_CHECK_TIMEOUT                     1000

#define JOIN_ACCEPT_DELAY1                          5000
#define JOIN_ACCEPT_DELAY2                          6000
#define MAX_FCNT_GAP                                16384
#define ADR_ACK_LIMIT                               64
#define ADR_ACK_DELAY                               32
#define ACK_TIMEOUT                                 1000 // random delay between 1 to 3 sec
#define SYNC_WORD                                   0x34
#define PREAMBLE_LEN                                8
#define MIC_LEN ( 4 )
#define LORAWAN_MAX_FOPTS_LENGTH                    15
#define LORAWAN_FRMPAYLOAD_OVERHEAD                 13 // MHDR(1) + FHDR(7) + Port(1) + MIC(4)

#if USE_BAND_868 == 1
static const lorawan_mac_params_t mac_params_defaults =
		{
				.channels_tx_power = LORAWAN_EU868_DEFAULT_TX_POWER,
				.channels_data_rate = LORAWAN_EU868_DEFAULT_DATARATE,
				.max_rx_window = LORAWAN_EU868_MAX_RX_WINDOW,
				.receive_delay_1 = LORAWAN_EU868_RECEIVE_DELAY1,
				.receive_delay_2 = LORAWAN_EU868_RECEIVE_DELAY2,
				.join_accept_delay_1 = LORAWAN_EU868_JOIN_ACCEPT_DELAY1,
				.join_accept_delay_2 = LORAWAN_EU868_JOIN_ACCEPT_DELAY2,
				.rx1_dr_offset = LORAWAN_EU868_DEFAULT_RX1_DR_OFFSET,
				.rx2_channel.frequency = LORAWAN_EU868_RX_WND_2_FREQ,
				.rx2_channel.data_rate = LORAWAN_EU868_RX_WND_2_DR,
				.uplink_dwell_time = 0,
				.downlink_dwell_time = 0,
				.max_eirp = LORAWAN_EU868_DEFAULT_MAX_EIRP,
				.antenna_gain = LORAWAN_EU868_DEFAULT_ANTENNA_GAIN,
				.channels_nb_rep = 1,
				.system_max_rx_error = 10,
				.min_rx_symbols = 6
		};
#endif

// function declearation
static void lorawan_mac_internal_reset_pri(lorawan_mac_t * obj);


static inline int32_t randr( int32_t min, int32_t max )
{
	return (int32_t) random() % (max - min + 1) + min;
}


static void lorawan_compute_rx_window_parameters_pri(lorawan_mac_t * obj, int8_t data_rate, uint8_t min_rx_symbols,
                                                     uint32_t rx_error, lorawan_rx_config_params_t * rx_config_param)
{
	double symbol_time = 0.0;

	// copy datarate
	rx_config_param->data_rate = (int8_t) MIN(data_rate, LORAWAN_EU868_RX_MAX_DATARATE);

	// get bandwidth from lookup table
	rx_config_param->bandwidth = lorawan_get_bandwidth(rx_config_param->data_rate);

	if (data_rate == DR_7)
	{
		// fsk mode
		symbol_time = lorawan_compute_symbol_time_fsk(rx_config_param->data_rate); // 1 symbol equals to 1 byte ( 8bits )
	}
	else
	{
		// lora mode
		symbol_time = lorawan_compute_symbol_time_lora(rx_config_param->data_rate);
	}

	// compute number of symbols
	rx_config_param->window_timeout =
			MAX(
					(uint32_t) ceil(((2 * min_rx_symbols - 8) * symbol_time + 2 * rx_error) / symbol_time),
					obj->mac_params.min_rx_symbols
			);

	rx_config_param->window_offset =
			(int32_t) ceil((4.0 * symbol_time) - ((rx_config_param->window_timeout * symbol_time) / 2.0) -
			               RADIO_RFM9X_WAKEUP_TIME);
}




static lorawan_mac_status_t lorawan_mac_send_frame_on_channel(lorawan_mac_t * obj, uint8_t channel)
{
	lorawan_tx_config_params_t tx_config;

	tx_config.channel = channel;
	tx_config.data_rate = obj->mac_params.channels_data_rate;
	tx_config.tx_power = obj->mac_params.channels_tx_power;
	tx_config.max_eirp = obj->mac_params.max_eirp;
	tx_config.antenna_gain = obj->mac_params.antenna_gain;
	tx_config.packet_len = obj->mac_buffer_packet_length;

	// TODO: limit the phy tx power
	int8_t phy_tx_power = lorawan_compute_phy_tx_power(tx_config.tx_power, tx_config.max_eirp, tx_config.antenna_gain);
	uint32_t bandwidth = lorawan_get_bandwidth(tx_config.data_rate);
	int8_t phy_dr = lorawan_get_phy_data_rate(tx_config.data_rate);
	uint32_t time_on_air = 0;


	// setup the radio frequency
	radio_rfm9x_set_channel(obj->radio, obj->channels[tx_config.channel].frequency);

	if (tx_config.data_rate == DR_7)
	{
		// TODO: high speed fsk
		DRV_ASSERT(false);
	}
	else
	{
		// set radio
		radio_rfm9x_set_modem(obj->radio, RADIO_RFM9X_MODEM_LORA);

		// set transmit power
		radio_rfm9x_set_tx_power(obj->radio, phy_tx_power);

		// set bandwidth
		if (bandwidth == 125000UL)
		{
			radio_rfm9x_set_bandwidth(obj->radio, RADIO_RFM9X_BW_125K);
		}
		else if (bandwidth == 250000UL)
		{
			radio_rfm9x_set_bandwidth(obj->radio, RADIO_RFM9X_BW_250K);
		}
		else if (bandwidth == 500000UL)
		{
			radio_rfm9x_set_bandwidth(obj->radio, RADIO_RFM9X_BW_500K);
		}
		else
		{
			DRV_ASSERT(false);
		}

		// set data rate
		radio_rfm9x_set_spreading_factor(obj->radio, (radio_rfm9x_sf_t) phy_dr);

		// set coding rate
		radio_rfm9x_set_coding_rate(obj->radio, RADIO_RFM9X_CR_4_5);

		// set preamble
		radio_rfm9x_set_preamble_length(obj->radio, PREAMBLE_LEN);

		// set maximum payload length
		radio_rfm9x_set_max_payload_length(obj->radio, RADIO_RFM9X_MODEM_LORA, (uint8_t) tx_config.packet_len);

		// calculate on the air time
		time_on_air = radio_rfm9x_get_time_on_air(obj->radio, RADIO_RFM9X_MODEM_LORA, (uint8_t) tx_config.packet_len);

		// TODO: setup mac layer status check timer

		if (obj->is_lorawan_network_joined == false)
		{
			obj->join_request_trials += 1;
		}
	}

	// transmit now
	radio_rfm9x_send(obj->radio, obj->mac_buffer, (uint8_t) obj->mac_buffer_packet_length);

	// set mac state
	STATE_SET(obj->mac_state, LORAWAN_MAC_TX_RUNNING);

	return LORAWAN_MAC_STATUS_OK;
}


static bool lorawan_rx_window_setup_pri(lorawan_mac_t * obj, lorawan_rx_config_params_t * rx_config, int8_t * data_rate)
{
	int8_t dr = rx_config->data_rate;
	uint8_t max_payload = 0;
	int8_t phy_dr = 0;
	uint32_t frequency = rx_config->frequency;

	if (obj->radio->radio_op_state != RADIO_RFM9X_OP_STDBY)
	{
		return false;
	}

	if (rx_config->window == 0)
	{
		// apply window 1 frequency
		frequency = obj->channels[rx_config->channel].frequency;

		// apply the alternative rx 1 window frequency if available
		if (obj->channels[rx_config->channel].rx1_frequency != 0)
			frequency = obj->channels[rx_config->channel].rx1_frequency;
	}

	// get the physical data rate
	phy_dr = lorawan_get_phy_data_rate(dr);

	// set frequency
	radio_rfm9x_set_channel(obj->radio, frequency);

	if (dr == DR_7)
	{
		// fsk
		DRV_ASSERT(false);
	}
	else
	{
		// select radio
		radio_rfm9x_set_modem(obj->radio, RADIO_RFM9X_MODEM_LORA);

		// select bandwidth
		radio_rfm9x_set_bandwidth(obj->radio, (radio_rfm9x_bw_t) (rx_config->bandwidth + 7));

		// set data rate
		radio_rfm9x_set_spreading_factor(obj->radio, (radio_rfm9x_sf_t) phy_dr);

		// set coding rate
		radio_rfm9x_set_coding_rate(obj->radio, RADIO_RFM9X_CR_4_5);

		// set preamble
		radio_rfm9x_set_preamble_length(obj->radio, PREAMBLE_LEN);
	}

	if (rx_config->repeater_support)
	{
		max_payload = lorawan_get_max_payload_of_data_rate_repeater(dr);
	}
	else
	{
		max_payload = lorawan_get_max_payload_of_data_rate(dr);
	}

	// set maximum payload length
	radio_rfm9x_set_max_payload_length(obj->radio, RADIO_RFM9X_MODEM_LORA, (uint8_t) (max_payload + LORAWAN_FRMPAYLOAD_OVERHEAD));

	*data_rate = dr;

	return true;
}


static void lorawan_mac_rx_window_timer1_handler_pri(lorawan_mac_t * obj)
{
	int8_t dr;

	obj->rx_slot = 0;

	obj->rx_window1_config.channel = obj->channel;
	obj->rx_window1_config.data_rate_offset = obj->mac_params.rx1_dr_offset;
	obj->rx_window1_config.downlink_dwell_time = obj->mac_params.downlink_dwell_time;
	obj->rx_window1_config.repeater_support = obj->repeater_support;
	obj->rx_window1_config.rx_continuous = false;
	obj->rx_window1_config.window = 0;

	if (obj->device_class == LORAWAN_CLASS_C)
	{
		radio_rfm9x_set_opmode_stdby(obj->radio);
	}


	// setup rx
	lorawan_rx_window_setup_pri(obj, &obj->rx_window1_config, &dr);

	// put radio to rx state
	radio_rfm9x_set_opmode_rx_timeout(obj->radio,
	                                  obj->rx_window1_config.rx_continuous ? 0 // continuous rx mode
	                                                                       : obj->mac_params.max_rx_window);
}

static void lorawan_mac_rx_window_timer2_handler_pri(lorawan_mac_t * obj)
{
	int8_t dr;
	obj->rx_window2_config.channel = obj->channel;
	obj->rx_window2_config.frequency = obj->mac_params.rx2_channel.frequency;
	obj->rx_window2_config.downlink_dwell_time = obj->mac_params.downlink_dwell_time;
	obj->rx_window2_config.repeater_support = obj->repeater_support;
	obj->rx_window2_config.window = 1;

	if (obj->device_class != LORAWAN_CLASS_C)
	{
		obj->rx_window2_config.rx_continuous = false;
	}
	else
	{
		obj->rx_window2_config.rx_continuous = true;
	}

	if (lorawan_rx_window_setup_pri(obj, &obj->rx_window1_config, &dr))
	{
		// put radio to rx state
		radio_rfm9x_set_opmode_rx_timeout(obj->radio,
		                                  obj->rx_window1_config.rx_continuous ? 0 // continuous rx mode
		                                                                       : obj->mac_params.max_rx_window);
		obj->rx_slot = obj->rx_window2_config.window;
	}
}

static void lorawan_mac_ack_handler_pri(lorawan_mac_t * obj)
{
	if (obj->node_ack_requested)
	{
		obj->ack_timeout_retry = true;
		STATE_CLEAR(obj->mac_state, LORAWAN_MAC_ACK_REQ);
	}

	if (obj->device_class == LORAWAN_CLASS_C)
	{
		obj->mac_flags.mac_done = 1;
	}
}

static void lorawan_mac_tx_delay_handler_pri(lorawan_mac_t * obj)
{
	lorawan_mac_header_t mac_header;
	lorawan_mac_fhdr_fctrl_t frame_control;

	uint16_t nb_trials;

	STATE_CLEAR(obj->mac_state, LORAWAN_MAC_TX_DELAYED);

	if (obj->mac_flags.mlme_req == 1 && obj->mlme_confirm.mlme_request == MLME_JOIN)
	{
		lorawan_mac_internal_reset_pri(obj);

		nb_trials = (uint16_t) (obj->join_request_trials + 1);

	}
}

static void lorawan_mac_state_check_handler_pri(lorawan_mac_t * obj)
{
	bool tx_timeout = false;

	if (obj->mac_flags.mac_done == 1)
	{
		if (STATE_CHECK(obj->mac_state, LORAWAN_MAC_RX_ABORT))
		{
			STATE_CLEAR(obj->mac_state, LORAWAN_MAC_RX_ABORT);
			STATE_CLEAR(obj->mac_state, LORAWAN_MAC_TX_RUNNING);
		}

		if (obj->mac_flags.mlme_req == 1 || obj->mac_flags.mcps_req == 1)
		{
			if (obj->mcps_confirm.status == LORAWAN_EVENT_INFO_STATUS_TX_TIMEOUT ||
					obj->mlme_confirm.status == LORAWAN_EVENT_INFO_STATUS_TX_TIMEOUT)
			{
				// stop transmit cycle due to tx_timeout
				STATE_CLEAR(obj->mac_state, LORAWAN_MAC_TX_RUNNING);
				obj->mac_commands_buffer_index = 0;
				obj->mcps_confirm.nb_retries = obj->ack_timeout_retries_counter;
				obj->mcps_confirm.ack_received = false;
				obj->mcps_confirm.tx_time_on_air = 0;
				tx_timeout = true;
			}
		}

		if (obj->node_ack_requested == false && tx_timeout == false)
		{
			if (obj->mac_flags.mlme_req == 1 || obj->mac_flags.mcps_req == 1)
			{
				if (obj->mac_flags.mlme_req == 1 && obj->mlme_confirm.mlme_request == MLME_JOIN)
				{
					// procedure for joining request
					obj->mlme_confirm.nb_retries = obj->join_request_trials;

					if (obj->mlme_confirm.status == LORAWAN_EVENT_INFO_STATUS_OK)
					{
						// node join successfully
						obj->uplink_counter = 0;
						obj->channels_nb_rep_counter = 0;
						STATE_CLEAR(obj->mac_state, LORAWAN_MAC_TX_RUNNING);
					}
					else
					{
						if (obj->join_request_trials >= obj->max_join_request_trials)
						{
							STATE_CLEAR(obj->mac_state, LORAWAN_MAC_TX_RUNNING);
						}
						else
						{
							obj->mac_flags.mac_done = 0;

							// send the same frame again

						}
					}
				}
			}
		}
	}
}

static inline void lorawan_mac_prepare_rx_done_abort_pri(lorawan_mac_t * obj)
{
	STATE_SET(obj->mac_state, LORAWAN_MAC_RX_ABORT);

	if (obj->node_ack_requested)
	{
		lorawan_mac_ack_handler_pri(obj);
	}

	obj->mac_flags.mcps_ind = 1;
	obj->mac_flags.mac_done = 1;

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

static void lorawan_mac_tx_delayed_timeout_callback(TimerHandle_t xTimer)
{
	DRV_ASSERT(xTimer);

	xTimerStop(xTimer, 0);

	// restore the timer id (which is used to store the parameter
	lorawan_mac_t * obj = (lorawan_mac_t *) pvTimerGetTimerID(xTimer);

	lorawan_mac_tx_delay_handler_pri(obj);
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
		lorawan_mac_rx_window_timer2_handler_pri(obj);
	}

	// at the moment the packet is transmitted, we are going to setup two rx slots, with corresponding delays
	if (obj->is_rx_windows_enabled)
	{
		// setup first time slot, with update-to-date rx_window_delay1
		xTimerChangePeriodFromISR(obj->rx_window1_timer, pdMS_TO_TICKS(obj->rx_window1_delay), NULL);
		xTimerStartFromISR(obj->rx_window1_timer, NULL);

		if (obj->device_class != LORAWAN_CLASS_C)
		{
			// setup the second time slot, with update-to-date rx_window_delay2
			xTimerChangePeriodFromISR(obj->rx_window2_timer, pdMS_TO_TICKS(obj->rx_window2_delay), NULL);
			xTimerStartFromISR(obj->rx_window2_timer, NULL);
		}

		if (obj->device_class == LORAWAN_CLASS_C || obj->node_ack_requested)
		{
			xTimerChangePeriodFromISR(obj->ack_timeout_timer,
			                          pdMS_TO_TICKS(obj->rx_window2_delay + randr(-LORAWAN_EU868_ACK_TIMEOUT_RND,
			                                                                      LORAWAN_EU868_ACK_TIMEOUT_RND)),
			                          NULL);
			xTimerStartFromISR(obj->ack_timeout_timer, NULL);
		}
	}
	else
	{
		// MCPS
		obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_OK;
		obj->mlme_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX2_TIMEOUT;

		if (obj->mac_flags.byte == 0)
		{
			obj->mac_flags.mcps_req = 1;
		}
		obj->mac_flags.mac_done = 1;
	}

	// verify if the last uplink was a join request
	if (obj->mac_flags.mlme_req == 1 && obj->mlme_confirm.mlme_request == MLME_JOIN) //  MlmeConfirm.MlmeRequest == MLME_JOIN
	{
		obj->last_tx_is_join_request = true;
	}
	else
	{
		obj->last_tx_is_join_request = false;
	}

	// store last tx channel
	obj->last_tx_channel = obj->channel;

	// TODO: Skip duty cycle part

	if (obj->node_ack_requested == false)
	{
		obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_OK;
		obj->channels_nb_rep_counter += 1;
	}
}

static void lorawan_mac_on_rx_error_isr(lorawan_mac_t * obj)
{
	if (obj->device_class != LORAWAN_CLASS_C)
	{
		radio_rfm9x_set_opmode_sleep(obj->radio);
	}
	else
	{
		lorawan_mac_rx_window_timer2_handler_pri(obj);
	}

	// error during rx1 window
	if (obj->rx_slot == 0)
	{
		if (obj->node_ack_requested)
		{
			obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX1_ERROR;
		}

		obj->mlme_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX1_ERROR;

		// if time >= rx_window2_delay
		// then set mac_done
	}
	else
	{
		// error during rx2 window
		if (obj->node_ack_requested)
		{
			obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX2_ERROR;
			obj->mac_flags.mac_done = 1;
		}
	}

}

static void lorawan_mac_on_rx_timeout_isr(lorawan_mac_t * obj)
{
	if (obj->device_class != LORAWAN_CLASS_C)
	{
		radio_rfm9x_set_opmode_sleep(obj->radio);
	}
	else
	{
		lorawan_mac_rx_window_timer2_handler_pri(obj);
	}

	if (obj->rx_slot == 0)
	{
		if (obj->node_ack_requested)
		{
			obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX1_TIMEOUT;
		}

		obj->mlme_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX1_TIMEOUT;

		// if time >= rx_window2_delay
		// then set mac_done
	}
	else
	{
		if (obj->node_ack_requested)
		{
			obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX2_TIMEOUT;
		}

		obj->mlme_confirm.status = LORAWAN_EVENT_INFO_STATUS_RX2_TIMEOUT;

		if (obj->device_class != LORAWAN_CLASS_C)
		{
			obj->mac_flags.mac_done = 1;
		}
	}
}


static void lorawan_mac_on_tx_timeout_handler(lorawan_mac_t * obj)
{
	if (obj->device_class != LORAWAN_CLASS_C)
	{
		radio_rfm9x_set_opmode_sleep(obj->radio);
	}
	else
	{
		lorawan_mac_rx_window_timer2_handler_pri(obj);
	}

	obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_TX_TIMEOUT;
	obj->mlme_confirm.status = LORAWAN_EVENT_INFO_STATUS_TX_TIMEOUT;
	obj->mac_flags.mac_done = 1;
}


static void lorawan_mac_rx_thread(lorawan_mac_t * obj)
{
	while (1)
	{
		// block read data from radio
		lorawan_mac_msg_ext_t rx_msg;
		DRV_ASSERT(xQueueReceive(obj->rx_queue_pri, &rx_msg, portMAX_DELAY));

		// configure mcps
		obj->mcps_confirm.ack_received = false;
		obj->mcps_indication.rssi = rx_msg.rssi;
		obj->mcps_indication.snr = rx_msg.snr;
		obj->mcps_indication.rx_slot = obj->rx_slot;
		obj->mcps_indication.port = 0;
		obj->mcps_indication.multicast = 0;
		obj->mcps_indication.frame_pending = 0;
		obj->mcps_indication.buffer = NULL;
		obj->mcps_indication.buffer_size = 0;
		obj->mcps_indication.rx_data = false;
		obj->mcps_indication.ack_received = false;
		obj->mcps_indication.downlink_counter = 0;
		obj->mcps_indication.mcps_indication = MCPS_UNCONFIRMED;

		// put the radio to sleep state and stop the rx2 timer
		radio_rfm9x_set_opmode_sleep(obj->radio);
		xTimerStop(obj->rx_window2_timer, portMAX_DELAY);

		// decode the packet
		uint8_t * packet_ptr = rx_msg.msg.buffer;

		lorawan_mac_header_t * mac_header;
		lorawan_mac_fhdr_fctrl_t * frame_control;

		mac_header = (lorawan_mac_header_t *) packet_ptr;
		packet_ptr += 1;

		// do things based on mac header type
		switch (mac_header->message_type)
		{
			case LORAWAN_MHDR_JOIN_ACCEPT:
			{
				if (obj->is_lorawan_network_joined)
				{
					obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_ERROR;
					lorawan_mac_prepare_rx_done_abort_pri(obj);
					return;
				}

				lorawan_mac_join_decrypt(rx_msg.msg.buffer + 1,
				                         (uint16_t) (rx_msg.msg.size - 1),
				                         obj->app_key,
				                         obj->mac_rx_payload + 1);
				obj->mac_rx_payload[0] = mac_header->byte;

				// compute mic
				uint32_t mic_cal = 0;
				lorawan_mac_join_compute_mic(obj->mac_rx_payload,
				                             (uint16_t) (rx_msg.msg.size - MIC_LEN),
				                             obj->app_key,
				                             &mic_cal);

				// read mic from rx_payload
				uint32_t mic_rx = 0;
				memcpy(&mic_rx, &obj->mac_rx_payload[rx_msg.msg.size - MIC_LEN] - MIC_LEN, MIC_LEN);

				if (mic_cal == mic_rx)
				{
					lorawan_mac_join_compute_session_key(obj->app_key,
					                                     obj->mac_rx_payload + 1,
					                                     obj->device_nonce,
					                                     obj->nwk_session_key,
					                                     obj->app_session_key);

					// read network id
					memcpy(&obj->network_id, &obj->mac_rx_payload[4], 3);

					// read device address
					memcpy(&obj->device_addr, &obj->mac_rx_payload[7], 4);

					// read dl settings
					obj->mac_params.rx1_dr_offset = (uint8_t) ((obj->mac_rx_payload[11] >> 4) & 0x07);
					obj->mac_params.rx2_channel.data_rate = (uint8_t) (obj->mac_rx_payload[11] & 0x0f);

					// read rx delay
					obj->mac_params.receive_delay_1 = (uint8_t) (obj->mac_rx_payload[12] & 0x0f);
					if (obj->mac_params.receive_delay_1 == 0)
						obj->mac_params.receive_delay_1 = 1;

					obj->mac_params.receive_delay_1 *= 1000;
					obj->mac_params.receive_delay_2 = obj->mac_params.receive_delay_1 + 1000;

					// cf list
					uint8_t * cf_list_payload = &obj->mac_rx_payload[13];
					uint8_t cf_list_size = (uint8_t) (rx_msg.msg.size - 17);


					if (cf_list_size == 16)
					{
						lorawan_channel_t new_channel;

						// setup default datarate range
						new_channel.data_rate_range.value = (DR_5 << 4) | DR_0;

						for (uint8_t i = 0, chan_idx = LORAWAN_EU868_NUMB_DEFAULT_CHANNELS;
						     chan_idx < LORAWAN_EU868_MAX_NB_CHANNELS;
						     i+=3, chan_idx+=1)
						{
							if (chan_idx < (LORAWAN_EU868_NUMB_CHANNELS_CF_LIST + LORAWAN_EU868_NUMB_DEFAULT_CHANNELS))
							{
								memcpy(&new_channel.frequency, &cf_list_payload[i], 3);
								new_channel.frequency *= 100;

								new_channel.rx1_frequency = 0;
							}
							else
							{
								new_channel.frequency = 0;
								new_channel.data_rate_range.value = 0;
								new_channel.rx1_frequency = 0;
							}

							if (new_channel.frequency != 0)
							{
								// TODO: add channel
							}
							else
							{
								// TODO: remove channel
							}
						}
					}

					obj->mlme_confirm.status = LORAWAN_EVENT_INFO_STATUS_OK;
					obj->is_lorawan_network_joined = true;

					obj->mac_params.channels_data_rate = mac_params_defaults.channels_data_rate;
				}
				else
				{
					obj->mlme_confirm.status = LORAWAN_EVENT_INFO_STATUS_JOIN_FAIL;
				}

				break;

			}

			case LORAWAN_MHDR_CONFIRMED_DATA_DOWN:
			{
				// NO BREAK
			}
			case LORAWAN_MHDR_UNCONFIRMED_DATA_DOWN:
			{
				// check the maximum packet length
				uint8_t max_packet_length = 0;
				if (obj->repeater_support)
				{
					max_packet_length = lorawan_get_max_payload_of_data_rate_repeater(
							obj->mcps_indication.rx_datarate);
				}
				else
				{
					max_packet_length = lorawan_get_max_payload_of_data_rate(
							obj->mcps_indication.rx_datarate);
				}

				if (MAX(0,
				        (int16_t)((int16_t) rx_msg.msg.size - (uint16_t) LORAWAN_FRMPAYLOAD_OVERHEAD)) > max_packet_length)
				{
					obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_ERROR;
					lorawan_mac_prepare_rx_done_abort_pri(obj);
					return;
				}

				// create variables
				uint8_t * nwk_session_key = obj->nwk_session_key;
				uint8_t * app_session_key = obj->app_session_key;
				uint32_t downlink_counter = obj->downlink_counter;
				uint32_t app_payload_start_idx;
				bool is_mic_ok = false;
				uint32_t mic_cal = 0;
				uint32_t mic_rx = 0;
				uint8_t port = 0xff;
				uint8_t frame_length;

				// copy address
				uint32_t address;
				memcpy(&address, packet_ptr, 4);
				packet_ptr += 4;

				// check the packet destination
				uint8_t multicast = false;
				if (address != obj->device_addr)
				{
					// TODO: we don't do with multicast for now
					if (!multicast)
					{
						// McpsIndication.Status = LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL;
						lorawan_mac_prepare_rx_done_abort_pri(obj);
						return;
					}
				}
				else
				{
					multicast = 0;
					nwk_session_key = obj->nwk_session_key;
					app_session_key = obj->app_session_key;
					downlink_counter = obj->downlink_counter;
				}

				// read frame_control
				frame_control = (lorawan_mac_fhdr_fctrl_t *) packet_ptr;
				packet_ptr += 1;

				// read application start address
				app_payload_start_idx = 8 + frame_control->foptslen;

				// read rx_mic
				memcpy(&mic_rx, &rx_msg.msg.buffer[rx_msg.msg.size - MIC_LEN], MIC_LEN);

				// read frame counter
				uint16_t frame_counter_prev = (uint16_t) downlink_counter;
				uint16_t frame_counter_diff = 0;
				uint16_t frame_counter = 0;

				memcpy(&frame_control, packet_ptr, 2);
				packet_ptr += 1;

				frame_counter_diff = frame_counter - frame_counter_prev;

				// check for frame counter roll over
				if (frame_counter_diff < (1 << 15))
				{
					downlink_counter += frame_counter_diff;
					lorawan_mac_compute_mic(rx_msg.msg.buffer,
					                        (uint16_t) (rx_msg.msg.size - MIC_LEN),
					                        nwk_session_key, address,
					                        LORAWAN_DOWN_LINK,
					                        downlink_counter,
					                        &mic_cal);
					if (mic_cal == mic_rx)
					{
						is_mic_ok = true;
					}
				}
				else
				{
					uint32_t frame_counter_roll_over = downlink_counter + 0x10000 + frame_counter_diff;
					lorawan_mac_compute_mic(rx_msg.msg.buffer,
					                        (uint16_t) (rx_msg.msg.size - MIC_LEN),
					                        nwk_session_key, address,
					                        LORAWAN_DOWN_LINK,
					                        frame_counter_roll_over,
					                        &mic_cal);
					if (mic_cal == mic_rx)
					{
						is_mic_ok = true;
						downlink_counter = frame_counter_roll_over;
					}
				}

				// check for maximum counter difference
				if (frame_counter_diff >= LORAWAN_EU868_MAX_FCNT_GAP)
				{
					obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS;
					obj->mcps_indication.downlink_counter = downlink_counter;
					lorawan_mac_prepare_rx_done_abort_pri(obj);
					return;
				}

				if (is_mic_ok)
				{
					obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_OK;
					obj->mcps_indication.multicast = multicast;
					obj->mcps_indication.frame_pending = frame_control->fpending;
					obj->mcps_indication.buffer = NULL;
					obj->mcps_indication.buffer_size = 0;
					obj->mcps_indication.downlink_counter = downlink_counter;

					obj->mcps_confirm.status = LORAWAN_EVENT_INFO_STATUS_OK;

					obj->adr_ack_counter = 0;
					obj->mac_commands_buffer_to_repeat_index = 0;

					if (multicast)
					{
						// TODO: We don't deal with multicast in this case
						obj->mcps_indication.mcps_indication = MCPS_MULTICAST;

						// TODO: Handle multicast
						DRV_ASSERT(false);
					}
					else
					{
						if (mac_header->message_type == LORAWAN_MHDR_CONFIRMED_DATA_DOWN)
						{
							obj->srv_ack_requested = true;
							obj->mcps_indication.mcps_indication = MCPS_CONFIRMED;

							if (obj->downlink_counter == downlink_counter && obj->downlink_counter != 0)
							{
								// duplicated confirmed downlink, skip indication
								obj->skip_indication = true;
							}
						}
						else
						{
							obj->srv_ack_requested = false;
							obj->mcps_indication.mcps_indication = MCPS_UNCONFIRMED;

							if (obj->downlink_counter == downlink_counter && obj->downlink_counter != 0)
							{
								obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_DOWNLINK_REPEATED;
								obj->mcps_indication.downlink_counter = downlink_counter;
								lorawan_mac_prepare_rx_done_abort_pri(obj);
								return;
							}
						}

						obj->downlink_counter = downlink_counter;
					}

					// before parsing the payload and mac command, we need to reset mac_command_buffer here.
					// error will be handled in on_mac_status_check_timer_ev
					if (obj->mcps_confirm.mcps_request == MCPS_CONFIRMED)
					{
						if (frame_control->ack == 1)
						{
							obj->mac_commands_buffer_index = 0;
						}
					}
					else
					{
						obj->mac_commands_buffer_index = 0;
					}

					// process payload and mac commands
					if (((rx_msg.msg.size - 4) - app_payload_start_idx) > 0)
					{
						port = rx_msg.msg.buffer[app_payload_start_idx++];
						frame_length = (uint8_t) ((rx_msg.msg.size - 4) - app_payload_start_idx);

						obj->mcps_indication.port = port;

						if (port == 0)
						{
							if (frame_control->foptslen == 0)
							{
								lorawan_mac_payload_decrypt(rx_msg.msg.buffer + app_payload_start_idx,
								                            frame_length,
								                            nwk_session_key,
								                            address,
								                            LORAWAN_DOWN_LINK,
								                            downlink_counter,
								                            obj->mac_rx_payload);

								// decode frame payload mac command
								lorawan_mac_process_mac_commands_pri(obj,
								                                     obj->mac_rx_payload,
								                                     0,
								                                     frame_length,
								                                     rx_msg.snr);

							}
							else
							{
								obj->skip_indication = true;
							}
						}
						else
						{
							if (frame_control->foptslen > 0)
							{
								// decode optional mac command, omit the fport
								lorawan_mac_process_mac_commands_pri(obj,
								                                     rx_msg.msg.buffer,
								                                     8,
								                                     (uint8_t) (app_payload_start_idx - 1),
								                                     rx_msg.snr);

							}

							lorawan_mac_payload_decrypt(rx_msg.msg.buffer + app_payload_start_idx,
							                            frame_length,
							                            app_session_key, address,
							                            LORAWAN_DOWN_LINK,
							                            downlink_counter,
							                            obj->mac_rx_payload);

							if (obj->skip_indication == false)
							{
								obj->mcps_indication.buffer = obj->mac_rx_payload;
								obj->mcps_indication.buffer_size = frame_length;
								obj->mcps_indication.rx_data = true;
							}
						}
					}

					else
					{
						if (frame_control->foptslen > 0)
						{
							// decode options field mac command
							lorawan_mac_process_mac_commands_pri(obj,
							                                     rx_msg.msg.buffer,
							                                     8,
							                                     (uint8_t) app_payload_start_idx,
							                                     rx_msg.snr);

						}
					}

					if (obj->skip_indication == false)
					{
						if (frame_control->ack == 1)
						{
							obj->mcps_confirm.ack_received = true;
							obj->mcps_indication.ack_received = true;

							// skip the acktimeout timer as no more retranmission are needed
							xTimerStop(obj->ack_timeout_timer, portMAX_DELAY);
						}
						else
						{
							obj->mcps_confirm.ack_received = false;

							if (obj->ack_timeout_retries_counter > obj->ack_timeout_retries)
							{
								// no more retransmission is needed
								xTimerStop(obj->ack_timeout_timer, portMAX_DELAY);
							}
						}
					}

					obj->mac_flags.mcps_ind = 1;
					obj->mac_flags.mcps_ind_skip = (uint8_t) obj->skip_indication;

				}
				else
				{
					obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_MIC_FAIL;
					lorawan_mac_prepare_rx_done_abort_pri(obj);
					return;
				}

				break;
			}

			case LORAWAN_MHDR_PROPRIETARY:
			{
				memcpy(obj->mac_rx_payload, packet_ptr, (size_t) rx_msg.msg.size);

				obj->mcps_indication.mcps_indication = MCPS_PROPRIETARY;
				obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_OK;
				obj->mcps_indication.buffer = obj->mac_rx_payload;
				obj->mcps_indication.buffer_size = (uint8_t) (rx_msg.msg.size - (packet_ptr - rx_msg.msg.buffer));

				obj->mac_flags.mcps_ind = 1;

				break;
			}

			default:
			{
				obj->mcps_indication.status = LORAWAN_EVENT_INFO_STATUS_ERROR;
				lorawan_mac_prepare_rx_done_abort_pri(obj);
				break;
			}
		}

		obj->mac_flags.mac_done = 1;

		// trigger on_mac_check_timer_event asap
		xTimerChangePeriod(obj->mac_state_check_timer, pdMS_TO_TICKS(1), portMAX_DELAY);
		xTimerStart(obj->mac_state_check_timer, portMAX_DELAY);
	}
}


static lorawan_mac_status_t lorawan_mac_prepare_frame_pri(lorawan_mac_t * obj, lorawan_mac_header_t * mac_header,
                                          lorawan_mac_fhdr_fctrl_t * fctrl,
                                          uint8_t fport, void * buffer, uint16_t size)
{
	// reset the packet length
	obj->mac_buffer_packet_length = 0;

	// record the tx_payload_length for further calculation
	obj->mac_tx_payload_length = size;

	// create a pointer to access internal mac buffer
	uint8_t * packet_ptr = obj->mac_buffer;

	// copy the mac header for all packet
	obj->mac_buffer[0] = mac_header->byte;

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
			lorawan_mac_join_compute_mic(obj->mac_buffer, (uint8_t) (packet_ptr - obj->mac_buffer),
			                             obj->app_key, &mic);

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
			if (obj->is_lorawan_network_joined == false)
			{
				return LORAWAN_MAC_STATUS_NO_NETWORK_JOINED;
			}

			// adr next request
			lorawan_adr_next_params_t adr_next;
			adr_next.update_channel_mask = true;
			adr_next.adr_enable = fctrl->adr;
			adr_next.adr_ack_counter = obj->adr_ack_counter;
			adr_next.data_rate = obj->mac_params.channels_data_rate;
			adr_next.tx_power = obj->mac_params.channels_tx_power;
			adr_next.uplink_dwell_time = obj->mac_params.uplink_dwell_time;

			fctrl->adrackreq = lorawan_adr_next(&adr_next,
			                                    &obj->mac_params.channels_data_rate,
			                                    &obj->mac_params.channels_tx_power,
			                                    &obj->adr_ack_counter);

			if (obj->srv_ack_requested)
			{
				obj->srv_ack_requested = false;
				fctrl->ack = 1;
			}

			// copy 4 byte device address
			memcpy(packet_ptr, &obj->device_addr, 4);
			packet_ptr += 4;

			// copy FCtrl
			fctrl->foptslen = 0;
			*packet_ptr = fctrl->byte;
			packet_ptr += 1;

			// copy uplink counter
			memcpy(packet_ptr, &obj->uplink_counter, 2);
			packet_ptr += 2;

			// copy mac command
			memcpy(&obj->mac_commands_buffer[obj->mac_commands_buffer_index],
			       obj->mac_commands_buffer_to_repeat,
			       obj->mac_commands_buffer_to_repeat_index);

			obj->mac_commands_buffer_index += obj->mac_commands_buffer_to_repeat_index;

			if (buffer != NULL && obj->mac_tx_payload_length > 0)
			{
				if (obj->mac_commands_in_next_tx == true)
				{
					if (obj->mac_commands_buffer_index <= LORAWAN_MAX_FOPTS_LENGTH)
					{
						fctrl->foptslen += obj->mac_commands_buffer_index;

						// update new fctrl
						obj->mac_buffer[0x05] = fctrl->byte;
						memcpy(packet_ptr, obj->mac_commands_buffer, obj->mac_commands_buffer_index);
						packet_ptr += obj->mac_commands_buffer_index;
					}
					else
					{
						obj->mac_tx_payload_length = obj->mac_commands_buffer_index;
						buffer = obj->mac_commands_buffer;
						fport = 0;
					}
				}
			}
			else
			{
				if (obj->mac_commands_buffer_index > 0 && obj->mac_commands_in_next_tx)
				{
					obj->mac_tx_payload_length = obj->mac_commands_buffer_index;
					buffer = obj->mac_commands_buffer;
					fport = 0;
				}
			}

			obj->mac_commands_in_next_tx = false;
			obj->mac_commands_buffer_to_repeat_index = 0; //TODO: ParseMacCommandsToRepeat
			if (obj->mac_commands_buffer_to_repeat_index > 0)
			{
				obj->mac_commands_in_next_tx = true;
			}

			if (buffer != NULL && obj->mac_tx_payload_length > 0)
			{
				// copy fport in the macpayload
				*packet_ptr = fport;
				packet_ptr += 1;

				if (fport == 0)
				{
					lorawan_mac_payload_encrypt(buffer, size, obj->nwk_session_key, obj->device_addr, LORAWAN_UP_LINK,
					                            obj->uplink_counter, packet_ptr);
				}
				else
				{
					lorawan_mac_payload_encrypt(buffer, size, obj->app_session_key, obj->device_addr, LORAWAN_UP_LINK,
					                            obj->uplink_counter, packet_ptr);
				}
			}

			packet_ptr += obj->mac_tx_payload_length;

			// calculate mic
			uint32_t mic;
			lorawan_mac_compute_mic(obj->mac_buffer, (uint16_t) (packet_ptr - obj->mac_buffer), obj->nwk_session_key,
			                        obj->device_addr, LORAWAN_UP_LINK, obj->uplink_counter, &mic);

			// copy mic
			memcpy(packet_ptr, &mic, 4);
			packet_ptr += 4;

			break;
		}

		case LORAWAN_MHDR_PROPRIETARY:
		{
			memcpy(packet_ptr, buffer, obj->mac_tx_payload_length);
			packet_ptr += size;

			break;
		}

		default:
			return LORAWAN_MAC_STATUS_SERVICE_UNKNOWN;
	}

	obj->mac_buffer_packet_length = (uint16_t) (packet_ptr - obj->mac_buffer);

	return LORAWAN_MAC_STATUS_OK;
}


static lorawan_mac_status_t lorawan_mac_schedule_tx_pri(lorawan_mac_t * obj)
{
	// due to the reason we don't have to adapt duty cycle, we can always use channel 0 for data transfer
	obj->mac_params.channels_data_rate = mac_params_defaults.channels_data_rate;

	lorawan_compute_rx_window_parameters_pri(obj,
	                                         lorawan_appy_data_rate_offset(obj->mac_params.downlink_dwell_time,
	                                                                       obj->mac_params.channels_data_rate,
	                                                                       obj->mac_params.rx1_dr_offset),
	                                         obj->mac_params.min_rx_symbols,
	                                         obj->mac_params.system_max_rx_error,
	                                         &obj->rx_window1_config);

	// compute rx2 parameter
	lorawan_compute_rx_window_parameters_pri(obj,
	                                         obj->mac_params.rx2_channel.data_rate,
	                                         obj->mac_params.min_rx_symbols,
	                                         obj->mac_params.system_max_rx_error,
	                                         &obj->rx_window2_config);

	if (obj->is_lorawan_network_joined == false)
	{
		obj->rx_window1_delay = obj->mac_params.join_accept_delay_1 + obj->rx_window1_config.window_offset;
		obj->rx_window2_delay = obj->mac_params.join_accept_delay_2 + obj->rx_window2_config.window_offset;
	}
	else
	{
		// TODO: validate payload length

		obj->rx_window1_delay = obj->mac_params.receive_delay_1 + obj->rx_window1_config.window_offset;
		obj->rx_window2_delay = obj->mac_params.receive_delay_2 + obj->rx_window2_config.window_offset;
	}

	// skip the duty cycle check
	// transmit now
	lorawan_mac_send_frame_on_channel(obj, obj->channel);


	return LORAWAN_MAC_STATUS_OK;
}


static void lorawan_mac_internal_reset_pri(lorawan_mac_t * obj)
{
	// reset mac MCPS status
	obj->mac_flags.byte = 0;

	// set default state for internal state machine
	obj->mac_state = LORAWAN_MAC_IDLE;

	// set default LoRaWAN device class to class A
	obj->device_class = LORAWAN_CLASS_A;

	obj->network_id = 0;

	// set join params
	obj->join_request_trials = 0;
	obj->max_join_request_trials = LORAWAN_MAX_JOIN_REQUEST_TRAILS;
	obj->repeater_support = false;

	// reset defaults from template
	obj->duty_cycle_on = LORAWAN_DUTY_CYCLE_ON;
	memcpy(&obj->mac_params, &mac_params_defaults, sizeof(lorawan_mac_params_t));

	// reset region specific params
	obj->channels[0] = (lorawan_channel_t) LORAWAN_EU868_LC1;
	obj->channels[1] = (lorawan_channel_t) LORAWAN_EU868_LC2;
	obj->channels[2] = (lorawan_channel_t) LORAWAN_EU868_LC3;

	// reset mac parameters
	obj->is_lorawan_network_joined = false;
	obj->uplink_counter = 0;
	obj->downlink_counter = 0;
	obj->adr_ack_counter = 0;

	obj->channels_nb_rep_counter = 0;
	obj->ack_timeout_retries = 1;
	obj->ack_timeout_retries_counter = 1;
	obj->ack_timeout_retry = false;

	obj->is_rx_windows_enabled = true;

	obj->node_ack_requested = false;
	obj->srv_ack_requested = false;
	obj->mac_commands_in_next_tx = false;
	obj->last_tx_is_join_request = false;
	obj->skip_indication = false;

	// skip multi cast

	// initialize channel index
	obj->channel = 0;
	obj->last_tx_channel = obj->channel;

	// local buffer
	obj->mac_commands_buffer_index = 0;
	obj->mac_commands_buffer_to_repeat_index = 0;

	obj->mac_buffer_packet_length = 0;

	// misc
	obj->adr_ctrl_on = false;

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
		radio_rfm9x_set_channel(obj->radio, 866000000UL);

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
	radio_rfm9x_set_tx_timeout_handler_callback(obj->radio, (void *) lorawan_mac_on_tx_timeout_handler, obj);

	// create timer instance
	obj->rx_window1_timer = xTimerCreate("rxw_t1", pdMS_TO_TICKS(1000), pdFALSE, obj, lorawan_mac_rx_window_timer1_callback);
	obj->rx_window2_timer = xTimerCreate("rxw_t2", pdMS_TO_TICKS(1000), pdFALSE, obj, lorawan_mac_rx_window_timer2_callback);
	obj->ack_timeout_timer = xTimerCreate("ack_t", pdMS_TO_TICKS(1000), pdFALSE, obj, lorawan_mac_ack_timeout_callback);
	obj->mac_state_check_timer = xTimerCreate("mac_chk", pdMS_TO_TICKS(1), pdFALSE, obj, lorawan_mac_state_check_timeout_callback);
	obj->tx_delayed_timer = xTimerCreate("txd", pdMS_TO_TICKS(1), pdFALSE, obj, lorawan_mac_tx_delayed_timeout_callback);

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
