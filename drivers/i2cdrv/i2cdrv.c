/**
 * @brief Implementation of I2C driver on EFR32 devices
 * @file i2cdrv.c
 * @author Ran Bao
 * @date Sept, 2017
 */

#include <stdint.h>
#include <stddef.h>

#include "em_device.h"
#include "em_cmu.h"
#include "em_i2c.h"
#include "ecode.h"

#include "i2cdrv.h"
#include "drv_debug.h"
#include "irq.h"

extern const pio_map_t i2c_scl_map[];
extern const pio_map_t i2c_sda_map[];

void i2cdrv_init(i2cdrv_t *obj, pio_t sda, pio_t scl, pio_t enable)
{
	// sanity check
	DRV_ASSERT(obj);

	Ecode_t ret;
	CMU_Clock_TypeDef clock;

	GPIO_Port_TypeDef sda_port;
	uint8_t sda_pin;
	GPIO_Port_TypeDef scl_port;
	uint8_t scl_pin;

	I2C_TypeDef *scl_base = NULL, *sda_base = NULL;
	uint32_t sda_loc = 0, scl_loc = 0;

	// store pins
	obj->sda = sda;
	obj->scl = scl;
	obj->enable = enable;

	// read port and pins
	sda_port = PIO_PORT(sda);
	sda_pin = PIO_PIN(sda);
	scl_port = PIO_PORT(scl);
	scl_pin = PIO_PIN(scl);

	// find base and pin location
	DRV_ASSERT(find_pin_function(i2c_scl_map, obj->scl, (void **) &scl_base, &scl_loc));
	DRV_ASSERT(find_pin_function(i2c_sda_map, obj->sda, (void **) &sda_base, &sda_loc));

	DRV_ASSERT(scl_base == sda_base);
	DRV_ASSERT(scl_base);

	// acquire base
	obj->base = scl_base;

	// acquire clock
	switch((uint32_t) obj->base)
	{
		case (uint32_t) I2C0:
			clock = cmuClock_I2C0;
			break;
		case (uint32_t) I2C1:
			clock = cmuClock_I2C1;
			break;
		default:
			DRV_ASSERT(false);
			break;
	}

	// enable I2C clock
	CMU_ClockEnable(clock, true);

	// enable GPIO clock
	CMU_ClockEnable(cmuClock_GPIO, true);

	// set open drain ports for sda and scl
	GPIO_PinModeSet(sda_port, sda_pin, gpioModeWiredAndPullUp, 1);
	GPIO_PinModeSet(scl_port, scl_pin, gpioModeWiredAndPullUp, 1);

	if (enable != NC)
	{
		// set I2C pull up source
		GPIO_PinModeSet(PIO_PORT(enable), PIO_PIN(enable), gpioModePushPull, 1);
	}

	// reset all slaves
	for (int i = 0; i < 9; i++)
	{
		GPIO_PinOutSet(scl_port, scl_pin);
		GPIO_PinOutClear(scl_port, scl_pin);
	}

	// set pin routing location
	obj->base->ROUTEPEN = (I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN);
	obj->base->ROUTELOC0 = (sda_loc << _I2C_ROUTELOC0_SDALOC_SHIFT) |
	                       (scl_loc << _I2C_ROUTELOC0_SCLLOC_SHIFT);

	// setup low level driver
	I2C_Init_TypeDef i2c_init_config;

	i2c_init_config.enable = true;
	i2c_init_config.master = true;
	i2c_init_config.freq = I2C_FREQ_STANDARD_MAX;
	i2c_init_config.refFreq = 0;
	i2c_init_config.clhr = i2cClockHLRStandard;

	// apply configuration to low level driver
	I2C_Init(obj->base, &i2c_init_config);

#if I2C_USE_DMA == 1
	// initialize DMA controller
	DMADRV_Init();

	ret = DMADRV_AllocateChannel(&obj->tx_dma_ch, NULL);
	assert(ret != ECODE_OK);

	ret = DMADRV_AllocateChannel(&obj->rx_dma_ch, NULL);
	assert(ret != ECODE_OK);
#endif

#if I2C_USE_MUTEX == 1
	// initialize mutex
	obj->access_mutex = xSemaphoreCreateMutex();
	assert(obj->access_mutex);
#endif
}

void i2cdrv_deinit(i2cdrv_t *obj)
{
	DRV_ASSERT(obj);

	// disable i2c peripheral
	i2cdrv_disable(obj);

	// reset pin location
	obj->base->ROUTEPEN = (I2C_ROUTEPEN_SCLPEN_DEFAULT | I2C_ROUTEPEN_SDAPEN_DEFAULT);
	obj->base->ROUTELOC0 = (I2C_ROUTELOC0_SCLLOC_DEFAULT | I2C_ROUTELOC0_SDALOC_DEFAULT);

#if I2C_USE_DMA == 1
	DMADRV_DeInit();
#endif

#if I2C_USE_MUTEX == 1
	assert(obj->access_mutex);
	vSemaphoreDelete(obj->access_mutex);

	obj->access_mutex = NULL;
#endif

	// revoke variables
	obj->sda = NC;
	obj->scl = NC;
	obj->enable = NC;
	obj->base = NULL;
}


void i2cdrv_enable(i2cdrv_t *obj)
{
	DRV_ASSERT(obj);

	// acquire clock
	CMU_Clock_TypeDef clock;
	switch((uint32_t) obj->base)
	{
		case (uint32_t) I2C0:
			clock = cmuClock_I2C0;
			break;
		case (uint32_t) I2C1:
			clock = cmuClock_I2C1;
			break;
		default:
			DRV_ASSERT(false);
			break;
	}

	// enable I2C clock
	CMU_ClockEnable(clock, true);

	// enable GPIO clock
	CMU_ClockEnable(cmuClock_GPIO, true);

	// set open drain ports for sda and scl
	GPIO_PinModeSet(PIO_PORT(obj->sda), PIO_PIN(obj->sda), gpioModeWiredAndPullUp, 1);
	GPIO_PinModeSet(PIO_PORT(obj->scl), PIO_PIN(obj->scl), gpioModeWiredAndPullUp, 1);

	if (obj->enable != NC)
	{
		// set I2C pull up source
		GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 1);
	}

	// enable I2C engine
	I2C_Enable(obj->base, true);
}


void i2cdrv_disable(i2cdrv_t *obj)
{
	DRV_ASSERT(obj);

	// acquire clock
	CMU_Clock_TypeDef clock;
	switch((uint32_t) obj->base)
	{
		case (uint32_t) I2C0:
			clock = cmuClock_I2C0;
			break;
		case (uint32_t) I2C1:
			clock = cmuClock_I2C1;
			break;
		default:
			DRV_ASSERT(false);
			break;
	}

	// disable I2C clock
	CMU_ClockEnable(clock, false);

	// disable sda and scl, leave master pull down
	GPIO_PinModeSet(PIO_PORT(obj->sda), PIO_PIN(obj->sda), gpioModeDisabled, 0);
	GPIO_PinModeSet(PIO_PORT(obj->scl), PIO_PIN(obj->scl), gpioModeDisabled, 0);

	if (obj->enable != NC)
	{
		// set I2C pull up source
		GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModeDisabled, 0);
	}

	// disable I2C engine
	I2C_Enable(obj->base, false);
}


/**
 * @brief Transfer data over i2c bus
 * @param obj i2c device object
 * @param seq determines transfer direction, data, etc...
 * @param timeout_cnt counter for timeout, 0 indicates no timeout
 * @return
 */
static I2C_TransferReturn_TypeDef i2cdrv_transfer_pri(i2cdrv_t *obj, I2C_TransferSeq_TypeDef * seq, uint32_t timeout_cnt)
{
	I2C_TransferReturn_TypeDef ret;
	bool is_timeout_enabled = (timeout_cnt != 0);

	// shift addr to the left by one
	// Layout details, A = address bit, X = don't care bit (set to 0):
	// 7 bit address - use format AAAA AAAX. (X -> R/W bits)
	// 10 bit address - use format XXXX XAAX AAAA AAAA
	seq->addr <<= 1;

#if I2C_USE_MUTEX == 1
	// enter mutex section
	if (__IS_INTERRUPT())
		xSemaphoreTakeFromISR(obj->access_mutex, NULL);
	else
		xSemaphoreTake(obj->access_mutex, portMAX_DELAY);
#endif

	// start transfer
	ret = I2C_TransferInit(obj->base, seq);

	// wait until finish, error or timeout
	while (ret == i2cTransferInProgress)
	{
		if (is_timeout_enabled)
		{
			timeout_cnt -= 1;
			if (timeout_cnt == 0) break;
		}
		ret = I2C_Transfer(obj->base);
	}

#if I2C_USE_MUTEX == 1
	// exit mutex section
	if (__IS_INTERRUPT())
		xSemaphoreGiveFromISR(obj->access_mutex, NULL);
	else
		xSemaphoreGive(obj->access_mutex);
#endif

	return ret;
}


I2C_TransferReturn_TypeDef i2cdrv_master_write_timeout(i2cdrv_t *obj, uint8_t slave_addr, void * buffer, uint16_t length, uint32_t timeout_cnt)
{
	DRV_ASSERT(obj);

	I2C_TransferSeq_TypeDef seq;
	I2C_TransferReturn_TypeDef ret;

	// setup transfer parameters
	seq.addr = slave_addr;
	seq.flags = I2C_FLAG_WRITE;
	seq.buf[0].data = buffer;
	seq.buf[0].len = length;

	ret = i2cdrv_transfer_pri(obj, &seq, timeout_cnt);

	return ret;
}

I2C_TransferReturn_TypeDef i2cdrv_master_read_timeout(i2cdrv_t *obj, uint8_t slave_addr, void * buffer, uint16_t length, uint32_t timeout_cnt)
{
	DRV_ASSERT(obj);

	I2C_TransferSeq_TypeDef seq;
	I2C_TransferReturn_TypeDef ret;

	// setup transfer parameters
	seq.addr = slave_addr;
	seq.flags = I2C_FLAG_READ;
	seq.buf[0].data = buffer;
	seq.buf[0].len = length;

	ret = i2cdrv_transfer_pri(obj, &seq, timeout_cnt);

	return ret;
}

I2C_TransferReturn_TypeDef i2cdrv_master_write_read_timeout(i2cdrv_t *obj, uint8_t slave_addr, void * write_buffer,
                                                    uint16_t write_length, void * read_buffer, uint16_t read_length,
                                                    uint32_t timeout_cnt)
{
	DRV_ASSERT(obj);

	I2C_TransferSeq_TypeDef seq;
	I2C_TransferReturn_TypeDef ret;

	// setup transfer parameters
	seq.addr = slave_addr;
	seq.flags = I2C_FLAG_WRITE_READ;
	seq.buf[0].data = write_buffer;
	seq.buf[0].len = write_length;
	seq.buf[1].data = read_buffer;
	seq.buf[1].len = read_length;

	ret = i2cdrv_transfer_pri(obj, &seq, timeout_cnt);

	return ret;
}

I2C_TransferReturn_TypeDef i2cdrv_master_write_iaddr_timeout(i2cdrv_t *obj, uint8_t slave_addr, uint8_t internal_addr, void *buffer, uint16_t length, uint32_t timeout_cnt)
{
	DRV_ASSERT(obj);

	I2C_TransferSeq_TypeDef seq;
	I2C_TransferReturn_TypeDef ret;

	// setup transfer parameters
	seq.addr = slave_addr;
	seq.flags = I2C_FLAG_WRITE_WRITE;

	// internal address can be implemented by setting first byte followed by slave address
	seq.buf[0].data = &internal_addr; // internal address is only used in current function scope
	seq.buf[0].len = 1;
	seq.buf[1].data = buffer;
	seq.buf[1].len = length;

	ret = i2cdrv_transfer_pri(obj , &seq, timeout_cnt);

	return ret;
}

I2C_TransferReturn_TypeDef i2cdrv_master_read_iaddr_timeout(i2cdrv_t *obj, uint8_t slave_addr, uint8_t internal_addr,
                                                            void * buffer, uint16_t length, uint32_t timeout_cnt)
{
	return i2cdrv_master_write_read_timeout(obj, slave_addr, &internal_addr, 1, buffer, length, timeout_cnt);
}
