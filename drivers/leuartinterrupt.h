/**
 * @brief LEUART Interrupt Manager
 * @author Ran Bao
 * @date 21/Dec/2017
 */
#ifndef LEUARTINTERRUPT_H_
#define LEUARTINTERRUPT_H_

typedef void (*leuart_callback_t)(void * arg);

#ifdef __cplusplus
extern "C" {
#endif

void leuart_interrupt_callback_register_with_arg(uint32_t idx, leuart_callback_t callback, void * arg);

#ifdef __cplusplus
}
#endif

#endif // LEUARTINTERRUPT_H_
