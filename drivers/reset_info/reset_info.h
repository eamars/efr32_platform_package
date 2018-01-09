/**
 * @brief Structure for reset info region at first 256bytes in SRAM
 * @author Ran Bao
 * @date 9/Jan/2018
 * @file reset_info.h
 */

#ifndef RESET_INFO_H_
#define RESET_INFO_H_

#include <stdint.h>
#include <stdbool.h>
#include "micro/micro.h"

#define ASSERT_FILE_NAME_LENGTH (32)
#define BACKTRACE_MAX_DEPTH (10)

#undef RESET_INFO_EXCLUSIVE_INCLUDE
#define RESET_INFO_EXCLUSIVE_INCLUDE
#include "reset_info_map.h"
#include "boot_info_map.h"
#undef RESET_INFO_EXCLUSIVE_INCLUDE

typedef union
{
    reset_info_map_t reset_info_map;
    boot_info_map_t boot_info_map;

    // and maybe more
} reset_info_t;

reset_info_t * reset_info_read(void);
void reset_info_clear(void);
bool reset_info_get_valid(void);
void reset_info_set_valid(void);

uint16_t reset_info_get_reset_reason(void);
void reset_info_set_reset_reason(uint16_t reset_reason);
rmu_reset_cause_t reset_info_get_rmu_reset_cause(void);

#endif // RESET_INFO_H_
