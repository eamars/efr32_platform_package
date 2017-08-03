/**
 * @file common/debug.h
 * @brief Debug print macro
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date April, 2017
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG
#include <stdio.h>
#define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d: " fmt, __func__, __LINE__, ##args)
#define DEBUG_FPRINT(io, fmt, args...) fprintf(io, "DEBUG: %s:%d: " fmt, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...) do {} while (0)
#deifne DEBUG_FPRINT(io, fmt, args...) do {} while (0)
#endif /* DEBUG */



#endif /* DEBUG_H_ */
