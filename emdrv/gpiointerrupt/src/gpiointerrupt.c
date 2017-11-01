/**
 * @brief Extended gpio interrupt library
 * @author Ran Bao
 * @date Oct, 2017
 * @file gpiointerrupt.c
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "drv_debug.h"
#include "em_gpio.h"
#include "em_core.h"
#include "gpiointerrupt.h"
#include "em_assert.h"
#include "em_common.h"

typedef struct
{
	void * callback;
	void * args;
	bool is_using_args;
} gpioint_callback_entry_t;

static gpioint_callback_entry_t gpioCallbacks[16] = { 0 };

static void GPIOINT_IRQDispatcher(uint32_t iflags)
{
	uint32_t irqIdx;
	void * callback;

	/* check for all flags set in IF register */
	while (iflags) {
		irqIdx = SL_CTZ(iflags);

		/* clear flag*/
		iflags &= ~(1 << irqIdx);

		if (gpioCallbacks[irqIdx].callback)
		{
			if (gpioCallbacks[irqIdx].is_using_args)
			{
				((GPIOINT_IrqCallbackPtrWithArgs_t) gpioCallbacks[irqIdx].callback)((uint8_t) irqIdx, gpioCallbacks[irqIdx].args);
			}
			else
			{
				((GPIOINT_IrqCallbackPtr_t) gpioCallbacks[irqIdx].callback)((uint8_t) irqIdx);
			}
		}
	}
}

void GPIOINT_Init(void)
{
	NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
	NVIC_EnableIRQ(GPIO_ODD_IRQn);
	NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

void GPIOINT_CallbackRegister(uint8_t pin, GPIOINT_IrqCallbackPtr_t callbackPtr)
{
	// make sure you haven't register the callback previously
	DRV_ASSERT(!gpioCallbacks[pin].callback);
	CORE_ATOMIC_SECTION(
			gpioCallbacks[pin].callback = (void *) callbackPtr;
			gpioCallbacks[pin].args = NULL;
			gpioCallbacks[pin].is_using_args = false;
	)
}

void GPIOINT_CallbackRegisterWithArgs(uint8_t pin, GPIOINT_IrqCallbackPtrWithArgs_t callbackPtr, void * args)
{
	// make sure you haven't register the callback previously
	DRV_ASSERT(!gpioCallbacks[pin].callback);
	CORE_ATOMIC_SECTION(
			gpioCallbacks[pin].callback = (void *) callbackPtr;
			gpioCallbacks[pin].args = args;
			gpioCallbacks[pin].is_using_args = true;
	)
}


void GPIO_EVEN_IRQHandler(void)
{
	uint32_t iflags;

	/* Get all even interrupts. */
	iflags = GPIO_IntGetEnabled() & 0x00005555;

	/* Clean only even interrupts. */
	GPIO_IntClear(iflags);

	GPIOINT_IRQDispatcher(iflags);
}


void GPIO_ODD_IRQHandler(void)
{
	uint32_t iflags;

	/* Get all odd interrupts. */
	iflags = GPIO_IntGetEnabled() & 0x0000AAAA;

	/* Clean only odd interrupts. */
	GPIO_IntClear(iflags);

	GPIOINT_IRQDispatcher(iflags);
}


void GPIOINT_CallbackUnRegister(uint8_t pin)
{
	// make sure you know what you are doing
	DRV_ASSERT(gpioCallbacks[pin].callback);
	CORE_ATOMIC_SECTION(
			gpioCallbacks[pin].callback = NULL;
	)
}
