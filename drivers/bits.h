/**
 * @brief Bitwise operation library
 * @file bits.h
 * @author Ran Bao
 * @date Sept, 2017
 */

#ifndef BITS_H_H
#define BITS_H_H

#include <stdint.h>

#define BITS_MODIFY(dest, value, mask) \
	(dest) = ((dest) & ~(mask)) | ((value) & (mask));

#define BITS_SET(dest, value) \
	(dest) |= ((value))

#define BITS_CLEAR(dest, value) \
	(dest) &= ~((value))

#define BITS_TOGGLE(dest, value) \
	(dest) ^= (value)

#endif // BITS_H_H
