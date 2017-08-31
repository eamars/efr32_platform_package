/**
 * @brief Pin definitions for EFR32
 */

#ifndef PIO_DEFS
#define PIO_DEFS

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "em_gpio.h"

/* EFM32 Pin Names
 * First 4 bits represent pin number, the remaining
 * bits represent port number (A = 0, B = 1, ...)
 */
typedef enum
{
	PA0 =  0 << 4, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
	PB0 =  1 << 4, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
	PC0 =  2 << 4, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
	PD0 =  3 << 4, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
	PE0 =  4 << 4, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
	PF0 =  5 << 4, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF10, PF11, PF12, PF13, PF14, PF15,
	PG0 =  6 << 4, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10, PG11, PG12, PG13, PG14, PG15,
	PH0 =  7 << 4, PH1, PH2, PH3, PH4, PH5, PH6, PH7, PH8, PH9, PH10, PH11, PH12, PH13, PH14, PH15,
	PI0 =  8 << 4, PI1, PI2, PI3, PI4, PI5, PI6, PI7, PI8, PI9, PI10, PI11, PI12, PI13, PI14, PI15,
	PJ0 =  9 << 4, PJ1, PJ2, PJ3, PJ4, PJ5, PJ6, PJ7, PJ8, PJ9, PJ10, PJ11, PJ12, PJ13, PJ14, PJ15,
	PK0 = 10 << 4, PK1, PK2, PK3, PK4, PK5, PK6, PK7, PK8, PK9, PK10, PK11, PK12, PK13, PK14, PK15,
	NC = (int) 0xFFFFFFFF
} pio_t;

typedef struct
{
	pio_t pio;
	void * base;
	uint32_t loc;
} pio_map_t;

/**
 * @Note: Treat function like a macro/constant (still debug friendly)
 */
#define PIO_PORT(x) get_pio_port((x))
#define PIO_PIN(x) get_pio_pin((x))
#define PIO_BIT(x) get_pio_pin((x))
#define PIO_DEF(port, pin) pio_define((port), (pin))

/**
 * @Note: Those simple functions are expected to be reduced to constant when optimized. We intended to keep those calculations
 * in the form of functions for better overview and debugging experience.
 */

static inline GPIO_Port_TypeDef get_pio_port(pio_t pio)
{
	assert(pio != NC);
	return (GPIO_Port_TypeDef) (pio >> 4);
}

static inline uint8_t get_pio_pin(pio_t pio)
{
	assert(pio != NC);
	return (uint8_t) (pio & 0xf);
}

static inline pio_t pio_define(uint32_t port, uint32_t pin)
{
	return (port << 4) | (pin & 0xf);
}

/**
 * @brief Find alternative function for PIO
 * @param table lookup table
 * @param pio input: lookup PIO
 * @param base output: alternative function base, valid only if the function returns true
 * @param loc output: alternative function routing location, valid only if function returns true
 * @return true if found, false otherwise
 */
static inline bool find_pin_function(const pio_map_t * table, pio_t pio, void ** base, uint32_t *loc)
{
	for (const pio_map_t * entry = table; entry->pio != NC; entry++)
	{
		if (pio == entry->pio)
		{
			*base = entry->base;
			*loc = entry->loc;

			return true;
		}
	}
	return false;
}

#endif /* PIN_DEFS */
