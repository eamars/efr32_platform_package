/**
 * @brief Structure for reset info region at first 256bytes in SRAM
 * @author Ran Bao
 * @date 15/Dec/2017
 * @file reset_info.h
 */


#ifndef RESET_INFO_EXCLUSIVE_INCLUDE
#error "You should include reset_info.h instead!"
#endif

#include <stdint.h>
#include "reset_info_common.h"
#include "cortex_m4_reg_defs.h"

typedef struct __attribute__((packed))
{
    reset_info_common_t reset_info_common;

    /**
     * @brief Core general propose registers
     *
     * Note: when fault isr occurred, only R0, R1, R2, R3, R12, LR, PC, XPSR and SP value is reliable
     */
    struct __attribute__((packed))
    {
        uint32_t R0;
        uint32_t R1;
        uint32_t R2;
        uint32_t R3;
        uint32_t R4;
        uint32_t R5;
        uint32_t R6;
        uint32_t R7;
        uint32_t R8;
        uint32_t R9;
        uint32_t R10;
        uint32_t R11;
        uint32_t R12;
        uint32_t LR;
        uint32_t SP;
        uint32_t MSP;
        uint32_t PSP;
        uint32_t PC;
        XPSR_t XPSR;
    } core_registers;

    /**
     * @brief System Control Block
     *
     * Contains key runtime registers
     */
    struct __attribute__((packed))
    {
        ICSR_t icsr;
        SHCSR_t shcsr;
        CFSR_t cfsr;
        HFSR_t hfsr;
        MMAR_t mmar;
        BFAR_t bfar;
        AFSR_t afsr;
    } scb_info;

    /**
     * @brief Assert information
     *
     * Note: Valid only if RESET_CRASH_ASSERT is set in reset_reason
     */
    struct __attribute__((packed))
    {
        const char * assert_filename_path_ptr;
        uint32_t assert_line;
        char assert_filename[ASSERT_FILE_NAME_LENGTH];
    } assert_info;

    /**
     * @brief Function call stack
     *
     * Note: no reliable function call before the fault interrupt for current version
     */
    uint32_t ip_stack[BACKTRACE_MAX_DEPTH];

} reset_info_map_t;



