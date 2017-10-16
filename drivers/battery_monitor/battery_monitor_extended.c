/**
 * @brief Extended battery monitor
 * @file battery_monitor_extended.c
 * @date 16, Oct, 2017
 * @author Ran Bao
 */

#include "battery_monitor_extended.h"
#include "battery_monitor.h"
#include "FreeRTOS.h"
#include "task.h"


static void battery_monitor_extended_daemon_thread(battery_monitor_extended_t * obj)
{
	// initialize the task tick handler
	portTickType xLastWakeTime;

	// get last execution time
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		obj->last_measured_voltage_mv = battery_monitor_read_mv((battery_monitor_t *) obj);
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(obj->measure_period_ms));
	}
}

void battery_monitor_extended_init(battery_monitor_extended_t * obj, pio_t probe, pio_t enable,
                                   uint32_t vref_mv, float divider_ratio, uint32_t measure_period_ms
)
{
	// initialize base class
	battery_monitor_init((battery_monitor_t *) obj, probe, enable, vref_mv, divider_ratio);

	// copy variable
	obj->measure_period_ms = measure_period_ms;

	// measure the voltage once to get an initial value
	obj->last_measured_voltage_mv = battery_monitor_read_mv((battery_monitor_t *) obj);

	// create thread
	xTaskCreate((void *) battery_monitor_extended_daemon_thread, "bat_mon", 200, obj, 2, &obj->battery_monitor_daemon_handler);
}
