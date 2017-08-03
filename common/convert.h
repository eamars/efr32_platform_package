/**
 * @file convert.h
 * @brief Type conversion module
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 *
 * Type conversion
 */

#ifndef CONVERT_H_
#define CONVERT_H_

#include <stdint.h>

/**
 * Convert one uint32_t value to four uint8_t value
 * @param in  input uint32_t pointer
 * @param out output uint8_t pointer
 */
void uint32_to_uint8(uint32_t *in, uint8_t *out);

/**
 * Convert four uint8_t value into one uint32_t value
 * @param in  input uint8_t pointer
 * @param out output uint32_t pointer
 */
void uint8_to_uint32(uint8_t *in, uint32_t *out);

void uint16_to_uint8(uint16_t *in, uint8_t *out);

void uint8_to_uint16(uint8_t *in, uint16_t *out);

void uint64_to_uint8(uint64_t *in, uint8_t *out);

void uint8_to_uint64(uint8_t *in, uint64_t *out);

#endif
