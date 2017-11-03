
#include <math.h>
#include "lorawan_band_868.h"
#include "lorawan_types.h"

#if USE_BAND_868 == 1

static const uint32_t bandwidths[] =
		{ 125000, 125000, 125000, 125000, 125000, 125000, 250000, 0 };

static const uint8_t data_rates[] = { 12, 11, 10,  9,  8,  7,  7, 50 };

static const uint8_t max_payload_of_data_rate_repeater[] = { 51, 51, 51, 115, 222, 222, 222, 222 };

static const uint8_t max_payload_of_data_rate[] = { 51, 51, 51, 115, 242, 242, 242, 242 };

radio_rfm9x_bw_t lorawan_get_bandwidth(int8_t dr_idx)
{
	switch (bandwidths[dr_idx])
	{
		default:
		case 125000:
			return RADIO_RFM9X_BW_125K;
		case 250000:
			return RADIO_RFM9X_BW_250K;
		case 500000:
			return RADIO_RFM9X_BW_500K;
	}
}

uint8_t lorawan_get_max_payload_of_data_rate_repeater(int8_t dr_idx)
{
	return max_payload_of_data_rate_repeater[dr_idx];
}

uint8_t lorawan_get_max_payload_of_data_rate(int8_t dr_idx)
{
	return max_payload_of_data_rate[dr_idx];
}

int8_t lorawan_get_phy_data_rate(int8_t dr_idx)
{
	return data_rates[dr_idx];
}

double lorawan_compute_symbol_time_lora(int8_t dr_idx)
{
	return ( ( double )( 1 << data_rates[dr_idx] ) / ( double ) bandwidths[dr_idx] ) * 1000;
}

double lorawan_compute_symbol_time_fsk(int8_t dr_idx)
{
	return ( 8.0 / ( double ) data_rates[dr_idx] ); // 1 symbol equals 1 byte
}

int8_t lorawan_appy_data_rate_offset(uint8_t downlink_dwell_time, int8_t dr, int8_t dr_offset)
{
	int8_t datarate = dr - dr_offset;
	if (datarate < 0)
	{
		datarate = DR_0;
	}

	return datarate;
}

int8_t lorawan_compute_phy_tx_power(int8_t tx_power_idx, float max_eirp, float antenna_gain)
{
	return (int8_t) floorf((max_eirp - (tx_power_idx * 2U)) - antenna_gain);
}

uint8_t lorawan_adr_next(lorawan_adr_next_params_t * adr_next, int8_t * dr_out, int8_t * tx_power_out,
                         uint32_t * adr_ack_counter)
{
	uint8_t adr_ack_req = false;
	int8_t data_rate = adr_next->data_rate;
	int8_t tx_power = adr_next->tx_power;

	// report back the adr ack counter
	*adr_ack_counter = adr_next->adr_ack_counter;

	if (adr_next->adr_enable)
	{
		if (adr_next->data_rate == LORAWAN_EU868_TX_MIN_DATARATE)
		{
			*adr_ack_counter = 0;
			adr_ack_req = false;
		}
		else
		{
			if (adr_next->adr_ack_counter >= LORAWAN_EU868_ADR_ACK_LIMIT)
			{
				adr_ack_req = true;
				tx_power = LORAWAN_EU868_MAX_TX_POWER;
			}
			else
			{
				adr_ack_req = false;
			}

			if (adr_next->adr_ack_counter >= (LORAWAN_EU868_ADR_ACK_LIMIT + LORAWAN_EU868_ADR_ACK_DELAY))
			{
				if (adr_next->adr_ack_counter % LORAWAN_EU868_ADR_ACK_DELAY == 1)
				{
					if (data_rate != LORAWAN_EU868_TX_MIN_DATARATE)
					{
						data_rate -= 1;
					}
					else
					{
						adr_ack_req = false;
						if (adr_next->update_channel_mask)
						{
							// TODO: re-enable default channels
						}
					}
				}
			}
		}
	}

	*dr_out = data_rate;
	*tx_power_out = tx_power;
	return adr_ack_req;
}


int8_t lorawan_get_alternate_dr(uint16_t nb_trials)
{
	int8_t data_rate = 0;

	if (nb_trials % 48 == 0)
	{
		data_rate = DR_0;
	}
	else if (nb_trials % 32 == 0)
	{
		data_rate = DR_1;
	}
	else if (nb_trials % 24 == 0)
	{
		data_rate = DR_2;
	}
	else if (nb_trials % 16 == 0)
	{
		data_rate = DR_3;
	}
	else if (nb_trials % 8 == 0)
	{
		data_rate = DR_4;
	}
	else
	{
		data_rate = DR_5;
	}
	return data_rate;
}

#endif
