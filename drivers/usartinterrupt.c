/**
 * @brief USARTn Interrupt Manager
 * @author Ran Bao
 * @date 21/Dec/2017
 */

#include <stdint.h>
#include <stdbool.h>

#include "usartinterrupt.h"
#include "drv_debug.h"
#include "em_core.h"

typedef struct
{
    usart_callback_t rx_callback;
    void * rx_arg;

    usart_callback_t tx_callback;
    void * tx_arg;
} usart_callback_struct_t;

// static variables
static usart_callback_struct_t usart_interrupt_cb_list[USART_COUNT];

static void usart_interrupt_dispacher(uint32_t idx, bool is_rx)
{
    if (is_rx)
    {
        if (usart_interrupt_cb_list[idx].rx_callback)
            usart_interrupt_cb_list[idx].rx_callback(usart_interrupt_cb_list[idx].rx_arg);
    }
    else
    {
        if (usart_interrupt_cb_list[idx].tx_callback)
            usart_interrupt_cb_list[idx].tx_callback(usart_interrupt_cb_list[idx].tx_arg);
    }

}

// static interrupt vectors
void USART0_RX_IRQHandler(void)
{
    usart_interrupt_dispacher(0, true);
}

void USART1_RX_IRQHandler(void)
{
    usart_interrupt_dispacher(1, true);
}

void USART2_RX_IRQHandler(void)
{
    usart_interrupt_dispacher(2, true);
}

void USART3_RX_IRQHandler(void)
{
    usart_interrupt_dispacher(3, true);
}

void USART0_TX_IRQHandler(void)
{
    usart_interrupt_dispacher(0, false);
}

void USART1_TX_IRQHandler(void)
{
    usart_interrupt_dispacher(1, false);
}

void USART2_TX_IRQHandler(void)
{
    usart_interrupt_dispacher(2, false);
}

void USART3_TX_IRQHandler(void)
{
    usart_interrupt_dispacher(3, false);
}

void usart_interrupt_callback_register_with_arg(uint32_t idx, usart_callback_t rx_callback, void * rx_arg, usart_callback_t tx_callback, void * tx_arg)
{
    // make sure there is no previously registered callback
    if (rx_callback)
        DRV_ASSERT(!usart_interrupt_cb_list[idx].rx_callback);
    if (tx_callback)
        DRV_ASSERT(!usart_interrupt_cb_list[idx].tx_callback);

    CORE_ATOMIC_SECTION(
            usart_interrupt_cb_list[idx].rx_callback = rx_callback;
            usart_interrupt_cb_list[idx].rx_arg = rx_arg;

            usart_interrupt_cb_list[idx].tx_callback = tx_callback;
            usart_interrupt_cb_list[idx].tx_arg = tx_arg;
    )
}
