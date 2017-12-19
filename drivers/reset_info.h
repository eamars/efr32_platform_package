/**
 * @brief Structure for reset info region at first 256bytes in SRAM
 * @author Ran Bao
 * @date 15/Dec/2017
 * @file reset_info.h
 */

#ifndef RESET_INFO_H_
#define RESET_INFO_H_


#include PLATFORM_HEADER
#include <stdint.h>
#include "bits.h"

#define RESET_INFO_SIGNATURE_VALID 0xF01Fu

#define ASSERT_FILE_NAME_LENGTH (32)
#define BACKTRACE_MAX_DEPTH (10)

typedef union
{
    uint32_t ICSR;

    struct __attribute__((packed))
    {
        uint32_t VECTACTIVE         : 9; // 8:0
        uint32_t                    : 2; // 10:9
        uint32_t RETOBASE           : 1; // 11
        uint32_t VECTPENDING        : 6; // 17:12
        uint32_t                    : 4; // 21:18
        uint32_t ISRPENDING         : 1; // 22
        uint32_t RESERVED_FOR_DEBUG : 1; // 23
        uint32_t                    : 1; // 24
        uint32_t PENDSTCLR          : 1; // 25
        uint32_t PENDSTSET          : 1; // 26
        uint32_t PENDSVCLR          : 1; // 27
        uint32_t PENDSVSET          : 1; // 28
        uint32_t                    : 2; // 30:29
        uint32_t NMIPENDSET         : 1; // 31
    };
} ICSR_t;

typedef union
{
    uint8_t MMFSR;

    struct __attribute__((packed))
    {
        uint8_t IACCVIOL            : 1;
        uint8_t DACCVIOL            : 1;
        uint8_t                     : 1;
        uint8_t MUNSTKERR           : 1;
        uint8_t MSTKERR             : 1;
        uint8_t MLSPERR             : 1;
        uint8_t                     : 1;
        uint8_t MMARVALID           : 1;
    };
} MMFSR_t;

typedef union
{
    uint8_t BFSR;

    struct __attribute__((packed))
    {
        uint8_t IBUSERR             : 1;
        uint8_t PRECISERR           : 1;
        uint8_t IMPRECISERR         : 1;
        uint8_t UNSTKERR            : 1;
        uint8_t STKERR              : 1;
        uint8_t LSPERR              : 1;
        uint8_t                     : 1;
        uint8_t BFARVALID           : 1;
    };
} BFSR_t;

typedef union
{
    uint16_t UFSR;

    struct __attribute__((packed))
    {
        uint16_t UNDEFINSTR         : 1;
        uint16_t INVSTATE           : 1;
        uint16_t INVPC              : 1;
        uint16_t NOCP               : 1;
        uint16_t                    : 4;
        uint16_t UNALIGNED          : 1;
        uint16_t DIVBYZERO          : 1;
        uint16_t                    : 6;
    };
} UFSR_t;

typedef union
{
    uint32_t CFSR;

    struct __attribute__((packed))
    {
        uint32_t MMFSR              : 8; // 7:0
        uint32_t BFSR               : 8; // 15:8
        uint32_t UFSR               : 16; // 31:16
    };
} CFSR_t;


typedef union
{
    uint32_t HFSR;

    struct __attribute__((packed))
    {
        uint32_t                    : 1;
        uint32_t VECTTBL            : 1;
        uint32_t                    : 28;
        uint32_t FORCED             : 1;
        uint32_t DEBUGEVT           : 1;
    };
} HFSR_t;

typedef union
{
    uint32_t XPSR;
    struct
    {
        uint32_t EXCPT          : 9;  // B0-8
        uint32_t ICIIT_LOW      : 7;  // B9-15
        uint32_t                : 8;  // B16-23
        uint32_t T              : 1;  // B24
        uint32_t ICIIT_HIGH     : 2;  // B25-26
        uint32_t Q              : 1;  // B27
        uint32_t V              : 1;  // B28
        uint32_t C              : 1;  // B29
        uint32_t Z              : 1;  // B30
        uint32_t N              : 1;  // B31
    };
} XPSR_t;

typedef union
{
    uint32_t SHCSR;
    struct
    {
        uint32_t MEMFAULTACT    : 1;  // B0
        uint32_t BUSFAULTACT    : 1;  // B1
        uint32_t                : 1;  // B2
        uint32_t USGFAULTACT    : 1;  // B3
        uint32_t                : 3;  // B4-6
        uint32_t SVCALLACT      : 1;  // B7
        uint32_t MONITORACT     : 1;  // B8
        uint32_t                : 1;  // B9
        uint32_t PENDSVACT      : 1;  // B10
        uint32_t SYSTICKACT     : 1;  // B11
        uint32_t USGFAULTPENDED : 1;  // B12
        uint32_t MEMFAULTPENDED : 1;  // B13
        uint32_t BUSFAULTPENDED : 1;  // B14
        uint32_t SVCALLPENDED   : 1;  // B15
        uint32_t MEMFAULTENA    : 1;  // B16
        uint32_t BUSFAULTENA    : 1;  // B17
        uint32_t USGFAULTENA    : 1;  // B18
        uint32_t                : 13; // B19-31
    };
} SHCSR_t;


typedef struct
{
    uint32_t MMSR;
} MMSR_t;


typedef struct
{
    uint32_t MMAR;
} MMAR_t;

typedef struct
{
    uint32_t BFAR;
} BFAR_t;

typedef union
{
    uint32_t AFSR;
    struct
    {
        uint32_t MISSED         : 1;  // B0
        uint32_t RESERVED       : 1;  // B1
        uint32_t PROTECTED      : 1;  // B2
        uint32_t WRONGSIZE      : 1;  // B3
        uint32_t                : 28; // B4-31
    };
} AFSR_t;


typedef struct __attribute__((packed))
{
    uint16_t reset_reason;
    uint16_t reset_signature;

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

} reset_info_t;


#endif // RESET_INFO_H_
