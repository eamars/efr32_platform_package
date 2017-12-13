/**
 * @file bootloader_api.h
 * @brief Bootloader calls that is used to integrate with other applications
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef BOOTLOADER_API_H_
#define BOOTLOADER_API_H_

#include <stdbool.h>
#include <stdint.h>

#include "stack32.h"
#include "irq.h"
#include "btl_reset_info.h"

#define BL_PROTO_VER 0x0002
#define APP_SIGNATURE 0xc91a2d2e

typedef enum
{
    INST_SET_BASE_ADDR = 0x0,
    INST_GET_BASE_ADDR = 0x1,
    INST_PUSH_BASE_ADDR = 0x2,
    INST_POP_BASE_ADDR = 0x3,

    INST_SET_BLOCK_EXP = 0x4,
    INST_GET_BLOCK_EXP = 0x5,

    INST_BRANCH_TO_ADDR = 0x6,

    INST_ERASE_PAGES = 0x7,
    INST_ERASE_RANGE = 0x8,

    INST_REBOOT = 0x9,

    INST_QUERY_PROTO_VER = 0xfc,
    INST_QUERY_DEVICE_INFO = 0xfd,
    INST_QUERY_CHIP_INFO = 0xfe,

    INST_INVALID = 0xff
} bootloader_instruction_t;


typedef struct
{
    // base address for flash (absolute base address for flash region)
    uint32_t base_addr;

    // block size exponential (2^x)
    uint8_t block_size_exp;

    // address stack
    stack_t base_addr_stack;
} bootloader_config_t;

typedef union
{
    uint32_t RMU_RESET_CAUSE;

    struct __attribute__((packed))
    {
        uint32_t PORST      : 1;
        uint32_t RESERVED1  : 1;
        uint32_t AVDDBOD    : 1;
        uint32_t DVDDBOD    : 1;
        uint32_t DECBOD     : 1;
        uint32_t RESERVED2  : 3;
        uint32_t EXTRST     : 1;
        uint32_t LOCKUPRST  : 1;
        uint32_t SYSREQRST  : 1;
        uint32_t WDOGRST    : 1;
        uint32_t RESERVED3  : 4;
        uint32_t EM4RST     : 1;
        uint32_t RESERVED   : 15;
    };
} rmu_reset_cause_t;

typedef struct
{
    BootloaderResetCause_t basicResetCause;

    /// Signature indicating following application address is valid
    uint32_t app_signature;

    /// Address for the application
    uint32_t app_addr;

    /// RMU reset reason
    rmu_reset_cause_t rmu_reset_cause;
} ExtendedBootloaderResetCause_t;


#ifdef __cplusplus
extern "C" {
#endif

void reboot_to_bootloader(bool reboot_now);
void reboot_to_addr(uint32_t app_addr, bool reboot_now);
void branch_to_addr(uint32_t app_addr);


#ifdef __cplusplus
}
#endif

#endif
