/**
 * @brief Sub-GHz packet definitions V2
 * @see https://docs.google.com/document/d/1OpWyGidNAUjewc6FDjs2I4VpbvujCBs9gMa6KYZMaa0/edit
 */

#ifndef SUBG_PACKET_V2_H_
#define SUBG_PACKET_V2_H_


#include <stdint.h>

#define SUBG_PACKET_V2_PROTOCOL_VER (0x02)
#define SUBG_PACKET_V2_HEADER_SIZE (sizeof(subg_packet_v2_header_t))
#define SUBG_PACKET_V2_DATA_SIZE ((12) + SUBG_PACKET_V2_HEADER_SIZE)
#define SUBG_PACKET_V2_CMD_SIZE ((6) + SUBG_PACKET_V2_HEADER_SIZE)
#define SUBG_PACKET_V2_DEBUG_SIZE (255)
#define SUBG_PACKET_V2_DEBUG_MAX_PAYLOAD_LEN (SUBG_PACKET_V2_DEBUG_SIZE - SUBG_PACKET_V2_HEADER_SIZE - 1)

typedef enum
{
	SUBG_PACKET_V2_TYPE_DATA = 0x00,
	SUBG_PACKET_V2_TYPE_CMD = 0x01,
	SUBG_PACKET_V2_TYPE_DEBUG = 0x02
} sugb_packet_v2_type_list_t;

typedef enum
{
	SUBG_PACKET_V2_CMD_ACK = 0x00,
	SUBG_PACKET_V2_CMD_CAL = 0x01,
	SUBG_PACKET_V2_CMD_RST = 0x02
} subg_packet_v2_cmd_list_t;


typedef struct __attribute__ ((packed))
{
	uint8_t protocol_version;
	uint8_t src_id8;
	uint8_t dest_id8;
	uint8_t packet_type;
	uint8_t seq_id;
} subg_packet_v2_header_t;


typedef union
{
	uint8_t _raw_data[SUBG_PACKET_V2_DATA_SIZE];
	struct __attribute__ ((packed))
	{
		subg_packet_v2_header_t header;

		// payload
		int16_t lpk_rssi;
		int8_t lpk_snr;
		uint16_t battery_voltage_mv;
		int8_t temperature_imu;
		int8_t temperature_sensor;
		uint8_t door_state;
		int16_t compass_value;
		int16_t heading;
	};
} subg_packet_v2_data_t;

typedef union
{
	uint8_t _raw_data[SUBG_PACKET_V2_CMD_SIZE];

	struct __attribute__ ((packed))
	{
		subg_packet_v2_header_t header;

		// payload
		uint16_t command;
		uint32_t payload;
	};
} subg_packet_v2_cmd_t;

typedef union
{
	uint8_t _raw_data[SUBG_PACKET_V2_DEBUG_SIZE];

	struct __attribute__ ((packed))
	{
		subg_packet_v2_header_t header;

		// payload
		uint8_t payload_length;
		uint8_t payload[SUBG_PACKET_V2_DEBUG_MAX_PAYLOAD_LEN];
	};
} subg_packet_v2_debug_t;

#endif // SUBG_PACKET_V2_H_
