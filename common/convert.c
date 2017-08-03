/**
 * @file convert.c
 * @brief Type conversion module
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 *
 * Implementation for type conversion module, regardless the byte order
 */
#include <stdint.h>
#include <string.h>
#include "convert.h"


void uint32_to_uint8(uint32_t *in, uint8_t *out)
{
	memcpy(out, in, sizeof(uint32_t));
}

void uint8_to_uint32(uint8_t *in, uint32_t *out)
{
	memcpy(out, in, sizeof(uint32_t));
}

void uint16_to_uint8(uint16_t *in, uint8_t *out)
{
	memcpy(out, in, sizeof(uint16_t));
}

void uint8_to_uint16(uint8_t *in, uint16_t *out)
{
	memcpy(out, in, sizeof(uint16_t));
}

void uint64_to_uint8(uint64_t *in, uint8_t *out)
{
	memcpy(out, in, sizeof(uint64_t));
}

void uint8_to_uint64(uint8_t *in, uint64_t *out)
{
	memcpy(out, in, sizeof(uint64_t));
}
