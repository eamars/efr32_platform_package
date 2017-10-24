/**
 * @brief LoRaWAN type definitions
 * @date 18, Oct, 2017
 * @author Ran Bao
 * @file lorawan_types.h
 */

#ifndef LORAWAN_TYPES_H_
#define LORAWAN_TYPES_H_

#include <stdint.h>



/**
 * @brief LoRaWAN MAC message type
 */
typedef enum
{
	LORAWAN_MHDR_JOIN_REQUEST = 0x0,
	LORAWAN_MHDR_JOIN_ACCEPT = 0x1,
	LORAWAN_MHDR_UNCONFIRMED_DATA_UP = 0x2,
	LORAWAN_MHDR_UNCONFIRMED_DATA_DOWN = 0x3,
	LORAWAN_MHDR_CONFIRMED_DATA_UP = 0x4,
	LORAWAN_MHDR_CONFIRMED_DATA_DOWN = 0x5,
	LORAWAN_MHDR_RFU = 0x6,
	LORAWAN_MHDR_PROPRIETARY = 0x7
} lorawan_mhdr_message_type_t;

typedef enum
{
	LORAWAN_CLASS_A,
	LORAWAN_CLASS_B,
	LORAWAN_CLASS_C
} lorawan_device_class_t;

typedef enum
{
	LORAWAN_R1 = 0x0
} lorawan_major_version_t;

typedef enum
{
	LORAWAN_MAC_CMD_LINK_CHECK_REQ = 0x02,
	LORAWAN_MAC_CMD_LINK_CHECK_ANS = 0x02,

	LORAWAN_MAC_CMD_LINK_ADR_REQ = 0x03,
	LORAWAN_MAC_CMD_LINK_ADR_ANS = 0x03,

	LORAWAN_MAC_CMD_DUTY_CYCLE_REQ = 0x04,
	LORAWAN_MAC_CMD_DUTY_CYCLE_ANS = 0x04,

	LORAWAN_MAC_CMD_RX_PARAM_SETUP_REQ = 0x05,
	LORAWAN_MAC_CMD_RX_PARAM_SETUP_ANS = 0x05,

	LORAWAN_MAC_CMD_DEV_STATUS_REQ = 0x06,
	LORAWAN_MAC_CMD_DEV_STATUS_ANS = 0x06,

	LORAWAN_MAC_CMD_NEW_CHANNEL_REQ = 0x07,
	LORAWAN_MAC_CMD_NEW_CHANNEL_ANS = 0x07,

	LORAWAN_MAC_CMD_TX_TIMING_SETUP_REQ = 0x08,
	LORAWAN_MAC_CMD_TX_TIMING_SETUP_ANS = 0x08,
} lorawan_mac_cmd_cid;

/**
 * @brief MHDR
 * MAC message header
 */
typedef union
{
	uint8_t byte;

	struct __attribute__((packed))
	{
		// major
		uint8_t major           : 2;

		// reserve for future usage
		uint8_t rfu             : 3;

		// message type
		uint8_t message_type    : 3;
	};

} lorawan_mac_header_t;

/**
 * @brief FCtrl
 * Frame header control octet (uplink)
 */
typedef union
{
	uint8_t byte;

	struct __attribute__((packed))
	{
		uint8_t foptslen        : 4;
		uint8_t rfu             : 1;
		uint8_t ack             : 1;
		uint8_t adrackreq       : 1;

		// adaptive data rate
		uint8_t adr             : 1;
	};

} lorawan_mac_fhdr_fctrl_ul_t;


/**
 * @brief FCtrl
 * Frame header control octet (downlink)
 */
typedef union
{
	uint8_t byte;

	struct __attribute__((packed))
	{
		uint8_t foptslen        : 4;
		uint8_t fpending        : 1;
		uint8_t ack             : 1;
		uint8_t adrackreq       : 1;

		// adaptive data rate
		uint8_t adr             : 1;
	};
} lorawan_mac_fhdr_fctrl_dl_t;

typedef enum
{
	LORAWAN_UP_LINK = 0,
	LORAWAN_DOWN_LINK = 1,
} lorawan_mac_comm_direction_t;


#endif // LORAWAN_TYPES_H_
