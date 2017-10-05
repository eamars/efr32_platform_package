/**
 * @brief Delay module
 */

#ifndef DELAY_H_
#define DELAY_H_

#include <stdint.h>

void delay_init(void);

void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

#endif // DELAY_H_
