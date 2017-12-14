/**
 * @brief Peripehral map for USART
 */

#include <stddef.h>
#include "em_device.h"
#include "pio_defs.h"

const pio_map_t usart_tx_map[] = {

        {PB6 , USART3, 10},
        {PD15, USART1, 23},
        {PA2,  USART1, 2},
        {PC10, USART0, 15},
        {PA6,  USART2, 1},
        {NC  , NULL   , 0}
};

const pio_map_t usart_rx_map[] = {
        {PB7 , USART3, 10},
        {PD14, USART1, 21},
        {PA1,  USART1, 0},
        {PC9,  USART0, 13},
        {PA7,  USART2, 1},
        {NC  , NULL   , 0}
};

const pio_map_t usart_clk_map[] = {

        {PD12, USART1, 18},
        {PA3,  USART1, 1},
        {PD15, USART0, 21},
        {PA8,  USART2, 1},
        {NC  , NULL   , 0}
};

const pio_map_t usart_cs_map[] = {

        {PD13, USART1, 18},
        {PA4,  USART1, 1},
        {PD14, USART0, 19},
        {PA9,  USART2, 1},
        {NC  , NULL   , 0}
};
