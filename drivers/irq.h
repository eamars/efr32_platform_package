/**
 * @brief Interrupt supplementary functions
 * @author Ran Bao
 * @date 9, Oct, 2017
 * @file irq.h
 */

#ifndef IRQ_H_
#define IRQ_H_

#include <stdbool.h>
#include <stdint.h>
#include "em_device.h"

typedef union
{
    uint32_t ICSR;

    struct __attribute__((packed))
    {
        uint32_t VECTACTIVE         : 9; // 8:0
        uint32_t RESERVED3          : 2; // 10:9
        uint32_t RETOBASE           : 1; // 11
        uint32_t VECTPENDING        : 6; // 17:12
        uint32_t RESERVED2          : 4; // 21:18
        uint32_t ISRPENDING         : 1; // 22
        uint32_t RESERVED_FOR_DEBUG : 1; // 23
        uint32_t RESERVED1          : 1; // 24
        uint32_t PENDSTCLR          : 1; // 25
        uint32_t PENDSTSET          : 1; // 26
        uint32_t PENDSVCLR          : 1; // 27
        uint32_t PENDSVSET          : 1; // 28
        uint32_t RESERVED0          : 2; // 30:29
        uint32_t NMIPENDSET         : 1; // 31
    };
} ICSR_t;

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
    uint8_t MMFSR;

    struct __attribute__((packed))
    {
        uint8_t IACCVIOL            : 1;
        uint8_t DACCVIOL            : 1;
        uint8_t RESERVED0           : 1;
        uint8_t MUNSTKERR           : 1;
        uint8_t MSTKERR             : 1;
        uint8_t MLSPERR             : 1;
        uint8_t RESERVED1           : 1;
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
        uint8_t RESERVED            : 1;
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
        uint16_t RESERVED0          : 4;
        uint16_t UNALIGNED          : 1;
        uint16_t DIVBYZERO          : 1;
        uint16_t RESERVED1          : 6;
    };
} UFSR_t;

/**
 * @brief Prototype for ISR handlers
 */
typedef void( *irq_handler_t )( void );

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tell if current coutine is running under interrupt mode
 * @return True if current code is running under interrupt mode
 */
#define  __IS_INTERRUPT() is_interrupt()
static inline bool is_interrupt(void)
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}


#ifdef __cplusplus
}
#endif

#endif // IRQ_H_
