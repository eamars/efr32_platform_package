/**
 * @brief Extended gpio interrupt library
 * @author Ran Bao
 * @date Oct, 2017
 * @file gpiointerrupt.h
 */

#ifndef GPIOINTERRUPT_H_
#define GPIOINTERRUPT_H

#include "em_device.h"


typedef void (*GPIOINT_IrqCallbackPtr_t)(uint8_t pin);
typedef void (*GPIOINT_IrqCallbackPtrWithArgs_t)(uint8_t pin, void * args);

#ifdef __cplusplus
extern "C" {
#endif

void GPIOINT_Init(void);
void GPIOINT_CallbackRegister(uint8_t pin, GPIOINT_IrqCallbackPtr_t callbackPtr);
void GPIOINT_CallbackRegisterWithArgs(uint8_t pin, GPIOINT_IrqCallbackPtrWithArgs_t callbackPtr, void * args);
void GPIOINT_CallbackUnRegister(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif // #define GPIOINTERRUPT_H
