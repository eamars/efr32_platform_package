/**
 * @brief Battery monitor driver
 * @author Ran Bao
 * @file battery_monitor.c
 * @date 16, Oct, 2017
 */

#include "battery_monitor.h"
#include "drv_debug.h"
#include "irq.h"
#include "em_device.h"
#include "em_cmu.h"
#include "em_adc.h"
#include "delay.h"


#if BATTERY_MONITOR_USE_MUTEX == 1
	#include "FreeRTOS.h"
	#include "semphr.h"
#endif

extern const pio_map_t adc_map[];


void battery_monitor_init(battery_monitor_t * obj, pio_t probe, pio_t enable,
                          uint32_t vref_mv, float divider_ratio)
{
	DRV_ASSERT(obj);

	uint32_t adc_pos;

	// copy variables
	obj->enable = enable;
	obj->vref_mv = vref_mv;
	obj->divider_ratio = divider_ratio;

#if BATTERY_MONITOR_USE_MUTEX == 1
	obj->access_mutex = xSemaphoreCreateMutex();
#endif

	// find ADC pin function
	DRV_ASSERT(find_pin_function(adc_map, probe, (void **) &obj->adc_base, &obj->channel_pos));

	// enable GPIO clock
	CMU_ClockEnable(cmuClock_GPIO, true);

	// configure adc load switch
	GPIO_PinModeSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable), gpioModePushPull, 0);

	// initialize adc peripheral
	ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
	init.prescale = 4;
	CMU_ClockEnable(cmuClock_ADC0, true);

	ADC_Init(obj->adc_base, &init);

	// initialize single conversion
	ADC_InitSingle_TypeDef single_init = ADC_INITSINGLE_DEFAULT;

	// measure the input channel with vdd as reference
	single_init.reference = adcRefVDD;
	single_init.resolution = adcRes12Bit;
	single_init.acqTime = adcAcqTime32;

	ADC_InitSingle(obj->adc_base, &single_init);
}


uint16_t battery_monitor_read_raw_pri(battery_monitor_t * obj)
{
	DRV_ASSERT(obj);

#if BATTERY_MONITOR_USE_MUTEX == 1
	// get access to the ADC peripherals
	if (__IS_INTERRUPT())
		xSemaphoreTakeFromISR(obj->access_mutex, NULL);
	else
		xSemaphoreTake(obj->access_mutex, portMAX_DELAY);
#endif

	// stop existing single conversion
	obj->adc_base->CMD = ADC_CMD_SINGLESTOP;

	// Make sure we are checking the correct channel
#if defined (_ADC_SINGLECTRL_INPUTSEL_MASK)
	obj->adc_base->SINGLECTRL = (obj->adc_base->SINGLECTRL & ~_ADC_SINGLECTRL_INPUTSEL_MASK) | obj->channel_pos;
#elif _ADC_SINGLECTRL_POSSEL_MASK
	obj->adc_base->SINGLECTRL = (obj->adc_base->SINGLECTRL & ~_ADC_SINGLECTRL_POSSEL_MASK) | obj->channel_pos << _ADC_SINGLECTRL_POSSEL_SHIFT;
#endif

	// enable load switch
	GPIO_PinOutSet(PIO_PORT(obj->enable), PIO_PIN(obj->enable));

	// give some delay between enabling load switch and adc measurement
	delay_us(500);

	// start adc conversion
	ADC_Start(obj->adc_base, adcStartSingle);

	// wait for conversion
	while (obj->adc_base->STATUS & ADC_STATUS_SINGLEACT);

	// get result
	uint16_t value = (uint16_t) ADC_DataSingleGet(obj->adc_base);

	// disable load switch
	GPIO_PinOutClear(PIO_PORT(obj->enable), PIO_PIN(obj->enable));

#if BATTERY_MONITOR_USE_MUTEX == 1
	// release access to the ADC peripherals
	if (__IS_INTERRUPT())
		xSemaphoreGiveFromISR(obj->access_mutex, NULL);
	else
		xSemaphoreGive(obj->access_mutex);
#endif
	return value;
}


uint32_t battery_monitor_read_mv(battery_monitor_t * obj)
{
	DRV_ASSERT(obj);

	uint16_t raw_value = battery_monitor_read_raw_pri(obj);

	uint32_t voltage_mv = (uint32_t) (raw_value * obj->vref_mv / 4096 * obj->divider_ratio);

	return voltage_mv;
}
