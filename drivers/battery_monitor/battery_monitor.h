/**
 * @brief Battery monitor driver
 * @author Ran Bao
 * @file battery_monitor.h
 * @date 16, Oct, 2017
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "pio_defs.h"
#include "em_device.h"
#include "em_adc.h"

#if BATTERY_MONITOR_USE_MUTEX == 1
	#include "FreeRTOS.h"
	#include "semphr.h"
#endif

typedef struct
{
	pio_t enable;
	ADC_TypeDef * adc_base;
	uint32_t channel_pos;

	uint32_t vref_mv;
	float divider_ratio;

#if BATTERY_MONITOR_USE_MUTEX == 1
	SemaphoreHandle_t access_mutex;
#endif

} battery_monitor_t;

#ifdef __cplusplus
extern "C" {
#endif


void battery_monitor_init(battery_monitor_t *obj, pio_t probe, pio_t enable,
                          uint32_t vref_mv, float divider_ratio);

uint16_t battery_monitor_read_raw_pri(battery_monitor_t *obj);

uint32_t battery_monitor_read_mv(battery_monitor_t *obj);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_MONITOR_H
