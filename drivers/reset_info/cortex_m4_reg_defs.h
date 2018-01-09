/**
 * Cortex-M4 specific system control registers
 */
#include <stdint.h>

#ifndef RESET_INFO_EXCLUSIVE_INCLUDE
#error "You should include reset_info.h instead!"
#endif

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
