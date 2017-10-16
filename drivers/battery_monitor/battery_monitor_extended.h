/**
 * @brief Extended battery monitor
 * @file battery_monitor_extended.c
 * @date 16, Oct, 2017
 * @author Ran Bao
 */

#ifndef BATTERY_MONITOR_EXTENDED_H
#define BATTERY_MONITOR_EXTENDED_H


#include "battery_monitor.h"

#include "FreeRTOS.h"
#include "task.h"

// subclass the battery monitor
typedef struct
{
	battery_monitor_t base;

	uint32_t last_measured_voltage_mv;
	TaskHandle_t battery_monitor_daemon_handler;
	uint32_t measure_period_ms;

} battery_monitor_extended_t;

void battery_monitor_extended_init(battery_monitor_extended_t * obj, pio_t probe, pio_t enable,
                                   uint32_t vref_mv, float divider_ratio, uint32_t measure_period_ms
);



#endif // BATTERY_MONITOR_EXTENDED_H
