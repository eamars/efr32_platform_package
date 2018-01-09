/**
 * Common implementation for reset info region
 */
#ifndef RESET_INFO_EXCLUSIVE_INCLUDE
#error "You should include reset_info.h instead!"
#endif

#include <stdint.h>

typedef union
{
    uint32_t RMU_RESET_CAUSE;

    struct __attribute__((packed))
    {
        uint32_t PORST      : 1;
        uint32_t            : 1;
        uint32_t AVDDBOD    : 1;
        uint32_t DVDDBOD    : 1;
        uint32_t DECBOD     : 1;
        uint32_t            : 3;
        uint32_t EXTRST     : 1;
        uint32_t LOCKUPRST  : 1;
        uint32_t SYSREQRST  : 1;
        uint32_t WDOGRST    : 1;
        uint32_t            : 4;
        uint32_t EM4RST     : 1;
        uint32_t            : 15;
    };
} rmu_reset_cause_t;

typedef struct __attribute__((packed))
{
    uint16_t reset_reason;
    uint16_t signature;
    rmu_reset_cause_t rmu_reset_cause;
} reset_info_common_t;
