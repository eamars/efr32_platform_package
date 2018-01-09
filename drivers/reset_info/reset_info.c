/**
 * @brief Structure for reset info region at first 256bytes in SRAM
 * @author Ran Bao
 * @date 9/Jan/2017
 * @file reset_info.h
 */
#include PLATFORM_HEADER
#include <string.h>
#include "reset_info.h"

#define RESET_INFO_SIGNATURE_VALID 0xF01Fu

reset_info_t * reset_info_read(void)
{
    return (reset_info_t *) &__RESETINFO__begin;
}

void reset_info_clear(void)
{
    memset(&__RESETINFO__begin, 0x0, sizeof(reset_info_t));
}

bool reset_info_get_valid(void)
{
    return ((reset_info_common_t *) reset_info_read())->signature == RESET_INFO_SIGNATURE_VALID;
}

void reset_info_set_valid(void)
{
    ((reset_info_common_t *) reset_info_read())->signature = RESET_INFO_SIGNATURE_VALID;
}

uint16_t reset_info_get_reset_reason(void)
{
    return ((reset_info_common_t *) reset_info_read())->reset_reason;
}

void reset_info_set_reset_reason(uint16_t reset_reason)
{
    ((reset_info_common_t *) reset_info_read())->reset_reason = reset_reason;
}

rmu_reset_cause_t reset_info_get_rmu_reset_cause(void)
{
    return ((reset_info_common_t *) reset_info_read())->rmu_reset_cause;
}
