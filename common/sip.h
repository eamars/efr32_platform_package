/**
 * @file sip.h
 * @brief Serial Interface Protocol module
 * @author Ran Bao
 * @date March, 2017
 *
 * Serial Interface Protocol for reading and verifing message over serial.
 * SIP uses hex decimal characters to transfer messages, allowing developers
 * to diagnose messages without using tools. The case is not sensitive.
 * Message Format:
 *
 * - 00: '<': starting symbol
 * - 01: 'x': command (up to 16 commands)
 * - 02: 'x': payload length (up to 256 bytes but we only use 20 of them)
 * - 03: 'x': payload 00 high
 * - 04: 'x': payload 00 low
 * - 05: 'x': payload 01 high
 * - 06: 'x': payload 01 low
 * - .
 * - .
 * - -1: '>': end symbol
 */

#ifndef SIP_H_
#define SIP_H_

#include <stdint.h>

#define SIP_FIXED_OVERHEAD 4
#define SIP_CMD_SIZE 16			// maximum 16 commands
#define SIP_PAYLOAD_LENGTH 256	// bytes
#define SIP_TOTAL_LENGTH 5 + SIP_PAYLOAD_LENGTH * 2		// 517 bytes

typedef void (*sip_cb_f) (void *);

typedef struct
{
	sip_cb_f function;
	void * args;
} sip_callback;

typedef enum
{
	NONE = 0,
	SFLAG,
	COMMAND,
	PAYLOAD_LENGTH_H,
	PAYLOAD_LENGTH_L,
	PAYLOAD_H,
	PAYLOAD_L,
	EFLAG
} sip_state_t;

typedef struct {
	// callback function table
	sip_callback cb_table[SIP_CMD_SIZE];
	uint8_t sip_buffer[SIP_TOTAL_LENGTH];

	// control variables
	// use uint16_t in case of overflow
	uint16_t payload_index;

	// decode state
	sip_state_t state;
} sip_t;



typedef enum
{
	SIP_PAYLOAD_0_S = 0,
	SIP_PAYLOAD_1_S = 1,
	SIP_PAYLOAD_2_S = 2,
	SIP_PAYLOAD_3_S = 3,
	SIP_PAYLOAD_4_S = 4,

	SIP_PAYLOAD_255_S = 255,

	SIP_CMD_S,
	SIP_PAYLOAD_LENGTH_S
} sip_symbol_t;

// initialize sip object
void sip_init(sip_t *handler);

// register callback function specific command
void sip_register_cb(sip_t *handler, uint8_t command, sip_cb_f function, void *args);

// set a specific value in sip object
void sip_set(sip_t *handler, sip_symbol_t symbol, uint8_t value);

// read a specific value from sip object
uint8_t sip_get(sip_t *handler, sip_symbol_t symbol);

// get payload pointer (dangerous)
uint8_t * sip_get_payload_ptr(sip_t *handler);

// get raw pointer of internal buffer
uint8_t * sip_get_raw_ptr(sip_t *handler);

// pump in data from serial or ble
void sip_poll(sip_t *handler, uint8_t ch);

// print debug information
void sip_debug_print(sip_t *handler);

#endif
