/**
 * @brief Sub-GHz packet definitions
 * @see https://docs.google.com/document/d/14sDZyo9l24STt2ZkljEb2scW_F_XmbcTaAfJaVU1s0E/edit
 */

#ifndef SUBG_PACKET_H_
#define SUBG_PACKET_H_


#include <stdint.h>


#define SUBG_PACKET_TOTAL_LEN (12) // bytes

typedef union
{
	// byte interface
	uint8_t _raw_data[SUBG_PACKET_TOTAL_LEN];

	// struct interface
	struct __attribute__ ((packed))
	{
		uint8_t version;
		uint8_t device_id8;
		int16_t lpk_rssi;
		int8_t lpk_snr;
		int8_t battery_percentage;
		int8_t temperature;
		uint8_t door_state;
		uint16_t compass_value;
		uint16_t heading;
	};

} subg_packet_t;


#endif // SUBG_PACKET_H_
