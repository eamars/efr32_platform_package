/**
 * @brief Peripehral map for LEUART
 */

#include <stddef.h>
#include "em_device.h"
#include "pio_defs.h"

const pio_map_t leuart_tx_map[] = {

        {PC10, LEUART0, 15},
        {PA0 , LEUART0, 0},
        {NC  , NULL   , 0}
};

const pio_map_t leuart_rx_map[] = {

        {PC9, LEUART0, 13},
        {PA1, LEUART0,  0},
        {NC  , NULL   , 0}
};
