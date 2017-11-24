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


const float percent_table[101] = {
    2705.4,   2996.9,   3204.9,   3349.0,   3445.4,   3506.8,   3543.3,   3563.0,   3571.6,   3573.8,   3572.8,   3570.9,   3569.4,   3569.4,   3571.1,   3574.7,
    3580.1,   3586.9,   3594.8,   3603.4,   3612.2,   3621.0,   3629.3,   3637.0,   3643.8,   3649.7,   3654.5,   3658.3,   3661.1,   3663.0,   3664.2,   3664.7,
    3664.8,   3664.6,   3664.2,   3663.9,   3663.8,   3663.9,   3664.4,   3665.4,   3666.9,   3669.0,   3671.6,   3674.7,   3678.4,   3682.6,   3687.1,   3692.0,
    3697.2,   3702.6,   3708.1,   3713.6,   3719.2,   3724.8,   3730.3,   3735.7,   3741.1,   3746.3,   3751.5,   3756.7,   3761.9,   3767.1,   3772.5,   3778.0,
    3783.7,   3789.7,   3796.1,   3802.7,   3809.7,   3817.0,   3824.6,   3832.6,   3840.7,   3849.1,   3857.5,   3866.0,   3874.3,   3882.5,   3890.4,   3898.0,
    3905.1,   3911.9,   3918.2,   3924.1,   3929.8,   3935.3,   3940.8,   3946.6,   3952.8,   3959.9,   3967.9,   3977.1,   3987.7,   3999.6,   4012.6,   4026.1,
    4039.2,   4050.3,   4057.2,   4056.9,   1000000
 };


static void battery_monitor_extended_daemon_thread(battery_monitor_extended_t * obj)
{
    uint8_t i = 0;
	// initialize the task tick handler
	portTickType xLastWakeTime;

	// get last execution time
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		obj->last_measured_voltage_mv = battery_monitor_read_mv((battery_monitor_t *) obj);
        for (i=0; percent_table[i] < 	obj->last_measured_voltage_mv; i++)
        {
          obj->bat_percentage = i + 1;
        }

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
