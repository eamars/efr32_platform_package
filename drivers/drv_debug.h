/**
 * @brief Debug macro for drivers
 */

#ifndef DRV_DEBUG_H_
#define DRV_DEBUG_H_

#include BOARD_HEADER

// assert
#ifndef DRV_ASSERT
#define DRV_ASSERT(x) {} while (1)
#endif

// print
#include <stdio.h>
#include "irq.h"
#include "yield.h"

#define DEBUG_PRINT(fmt, args...) YIELD(if (__IS_INTERRUPT()) DRV_ASSERT(false); else \
        fprintf(stderr, "DEBUG: %s:%d: " fmt, __func__, __LINE__, ##args);)

#define ERROR_PRINT(fmt, args...) YIELD(if (__IS_INTERRUPT()) DRV_ASSERT(false); else \
        fprintf(stderr, "ERROR: %s:%d: " fmt, __func__, __LINE__, ##args);)

#endif /* DRV_DEBUG_H_ */
