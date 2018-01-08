/**
 * @brief LEUART Interrupt Manager
 * @author Ran Bao
 * @date 8/Jan/2018
 */

#include <stdint.h>
#include <stdbool.h>

#include "leuartinterrupt.h"
#include "drv_debug.h"
#include "em_core.h"

typedef struct
{
    leuart_callback_t callback;
    void * arg;
} leuart_callback_struct_t;

// static variables
static leuart_callback_struct_t leuart_interrupt_cb_list[LEUART_COUNT];

static void leuart_interrupt_dispacher(uint32_t idx)
{
    if (leuart_interrupt_cb_list[idx].callback)
        leuart_interrupt_cb_list[idx].callback(leuart_interrupt_cb_list[idx].arg);
}

// static interrupt vectors
void LEUART0_IRQHandler(void)
{
    leuart_interrupt_dispacher(0);
}

void leuart_interrupt_callback_register_with_arg(uint32_t idx, leuart_callback_t callback, void * arg)
{
    // make sure there is no previously registered callback
    if (callback)
    DRV_ASSERT(!leuart_interrupt_cb_list[idx].callback);

    CORE_ATOMIC_SECTION(
            leuart_interrupt_cb_list[idx].callback = callback;
            leuart_interrupt_cb_list[idx].arg = arg;
    )
}
