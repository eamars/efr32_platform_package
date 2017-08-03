/**
 * @file sip.c
 * @brief Implementation of Serial Interface Protocol
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 *
 * @see sip.h
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sip.h"

/**
 * @brief convert hex value to char
 * https://stackoverflow.com/questions/4085612/converting-an-int-to-a-2-byte-hex-value-in-c
 */
#define TO_HEX(value) (uint8_t)((value) <= 9 ? ('0' + (value)) : ('A' - 10 + (value)))

/**
 * Convert hex character decimal to unsigned integer
 * @param  ch input character (case insensitive)
 * @return    value of such character (between 0x0 - 0xf)
 */
uint8_t hexchar_to_uint8(uint8_t ch)
{
    uint8_t val = 0;

    if (ch >= '0' && ch <= '9')
    {
        val = ch - '0';
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        val  = ch - 'A';
        val += 10;
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        val = ch - 'a';
        val += 10;
    }

    return val;
}

/**
 * Intialize sip object, reset all variables
 * @param handler sip handler for accessing internal properties
 */
void sip_init(sip_t *handler)
{
	// initialize callback functions and arguments
	memset(handler->cb_table, 0x0, sizeof(sip_callback) * SIP_CMD_SIZE);

	// initialize buffers
	memset(handler->sip_buffer, '0', sizeof(uint8_t) * SIP_TOTAL_LENGTH);

	// initialize decode variables
	handler->payload_index = 0;
	handler->state = NONE;

	// set sip start and terminates
	handler->sip_buffer[0] = '<';
	handler->sip_buffer[SIP_TOTAL_LENGTH - 1] = '>';
}

/**
 * Bind function to specific command received from serial or ble
 * @param handler  sip handler for accessing internal properties
 * @param command  command index, ranges from 0x0 to 0xf
 * @param callback pointer to the callback function and corresponding argument, see definition of sip_callback
 */
void sip_register_cb(sip_t *handler, uint8_t command, sip_cb_f function, void *args)
{
	handler->cb_table[command].function = function;
	handler->cb_table[command].args = args;
}

/**
 * Set a specific value in sip buffer
 * @param handler sip handler for accessing internal properties
 * @param symbol  the name of a specific internal property
 * @param value   value to set
 */
void sip_set(sip_t *handler, sip_symbol_t symbol, uint8_t value)
{
	switch (symbol)
	{
		case SIP_CMD_S:
		{
			handler->sip_buffer[1] = TO_HEX((value & 0x0f));
			break;
		}

		case SIP_PAYLOAD_LENGTH_S:
		{
			handler->sip_buffer[2] = TO_HEX((value & 0xf0) >> 4);
			handler->sip_buffer[3] = TO_HEX((value & 0x0f));
			break;
		}

		default:
		{
			// set corresponding bytes in buffer
			handler->sip_buffer[symbol * 2 + 4] = TO_HEX((value & 0xf0) >> 4);
			handler->sip_buffer[symbol * 2 + 5] = TO_HEX((value & 0x0f));
			break;
		}
	}
}

/**
 * Read a specific from sip buffer
 * @param  handler sip handler for accessing internal properties
 * @param  symbol  the name of a specific internal property
 * @return         corresponding value to the internal property
 */
uint8_t sip_get(sip_t *handler, sip_symbol_t symbol)
{
	switch (symbol)
	{
		case SIP_CMD_S:
			return hexchar_to_uint8(handler->sip_buffer[1]);

		case SIP_PAYLOAD_LENGTH_S:
		{
			uint8_t high, low;
			high = hexchar_to_uint8(handler->sip_buffer[2]);
			low = hexchar_to_uint8(handler->sip_buffer[3]);
			return (high << 4) | low;
		}

		default:
		{
			// restore serialized values
			uint8_t high, low;

			high = hexchar_to_uint8(handler->sip_buffer[symbol * 2 + 4]);
			low = hexchar_to_uint8(handler->sip_buffer[symbol * 2 + 5]);
			return (high << 4) | low;
		}
	}
}

/**
 * Get pointer to payload directly.
 * Caution: using this function can be potentially dangerous, think carefully!
 * @param  handler sip handler for accessing internal properties
 * @return         pointer to the payload
 */
uint8_t * sip_get_payload_ptr(sip_t *handler)
{
	return &handler->sip_buffer[4];
}

/**
 * Get pointer to internal buffer directly
 * Caution: using this function can be potentially dangerous, you should know
 * what you are doing!
 * @param  handler sip handler for accessing internal properties
 * @return         pointer to the buffer
 */
uint8_t * sip_get_raw_ptr(sip_t *handler)
{
	return handler->sip_buffer;
}

/**
 * Sequentially read one character from external device
 * @param handler sip handler for accessing internal properties
 * @param ch      hex decimal character
 */
void sip_poll(sip_t *handler, uint8_t ch)
{
	// reset state to keep sync
	if (ch == '<')
	{
		handler->state = SFLAG;
		handler->payload_index = 0;
	}

	switch (handler->state)
	{
		case SFLAG:
			handler->state = COMMAND;
			break;
		case COMMAND:
			handler->sip_buffer[1] = ch;
			handler->state = PAYLOAD_LENGTH_H;
			break;
		case PAYLOAD_LENGTH_H:
			handler->sip_buffer[2] = ch;
			handler->state = PAYLOAD_LENGTH_L;
			break;
		case PAYLOAD_LENGTH_L:
			handler->sip_buffer[3] = ch;
			handler->state = PAYLOAD_H;
			break;
		case PAYLOAD_H:
			handler->sip_buffer[handler->payload_index * 2 + 4] = ch;
			handler->state = PAYLOAD_L;
			break;
		case PAYLOAD_L:
			handler->sip_buffer[handler->payload_index * 2 + 5] = ch;
			handler->payload_index += 1;

			// append ch to payload until reach the give length
			if (handler->payload_index < sip_get(handler, SIP_PAYLOAD_LENGTH_S))
			{
				handler->state = PAYLOAD_H;
			}
			else
			{
				handler->state = EFLAG;
			}
			break;
		case EFLAG:
			if (ch == '>')
			{
				// execute corresponding callback function
				uint8_t command = sip_get(handler, SIP_CMD_S);
				if (handler->cb_table[command].function != 0)
				{
					handler->cb_table[command].function(handler->cb_table[command].args);
				}
			}
			handler->state = NONE;
			break;
		case NONE:
			break;
		default:
			break;
	}
}
