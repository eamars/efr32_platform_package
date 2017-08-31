//
// Created by Ran Bao on 31/08/17.
//

#ifndef I2CDRV_H
#define I2CDRV_H

#include <stdint.h>
#include <stdbool.h>

#include "em_i2c.h"
#include "pio_defs.h"


typedef struct
{
	bool initialized;
	I2C_TypeDef * base;
#ifdef ENABLE_I2C_DMA
	unsigned int tx_dma_ch;
	unsigned int rx_dma_ch;
#endif
} i2cdrv_t;


void i2cdrv_init(i2cdrv_t *obj, pio_t sda, pio_t scl, pio_t enable);



#endif //I2CDRV_H
