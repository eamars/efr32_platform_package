/**
 * @brief USARTn Interrupt Manager
 * @author Ran Bao
 * @date 21/Dec/2017
 */
#ifndef USARTINTERRUPT_H_
#define USARTINTERRUPT_H_

typedef void (*usart_callback_t)(void * arg);

#ifdef __cplusplus
extern "C" {
#endif

void usart_interrupt_callback_register_with_arg(uint32_t idx, usart_callback_t rx_callback, void * rx_arg, usart_callback_t tx_callback, void * tx_arg);

#ifdef __cplusplus
}
#endif

#endif // USARTINTERRUPT_H_
