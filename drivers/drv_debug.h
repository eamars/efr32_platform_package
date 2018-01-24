/**
 * @brief Debug macro for drivers
 */

#ifndef DRV_DEBUG_H_
#define DRV_DEBUG_H_

#include BOARD_HEADER
#include PLATFORM_HEADER
#include <stdint.h>
#include <string.h>

#include "reset_info.h"

typedef struct
{
    uint32_t max_depth;
    uint32_t current_depth;
    uint32_t * ip_stack; // pointer to an array of addresses (represented as pointers to allow GDB to find original function name & offsets)
    uint32_t * code_stack;
} unwind_ctrl_t;

// assert
void assert_failed(const char * file, uint32_t line);
void assert_failed_func(const char * function_name);
void assert_failed_null();

void backtrace(void);

uint16_t system_crash_handler(void);

void factory_reset(void);

void system_reset(uint16_t reset_reason) __attribute__ ((deprecated));
void software_reset(uint16_t reset_reason);

void stack_reg_dump(const uint32_t * stack_addr);

void debug_breakpoint(void);



#ifndef DRV_ASSERT
#define DRV_ASSERT(x) while (1) {}
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
