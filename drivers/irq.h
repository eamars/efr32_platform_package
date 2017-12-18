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
