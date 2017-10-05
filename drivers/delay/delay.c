/**
 * @brief Delay module
 */

#include <stdbool.h>
#include "em_device.h"
#include "delay.h"
#include "ustimer.h"
#include "drv_debug.h"

#if USE_FREERTOS == 1
	#include "FreeRTOS.h"
	#include "task.h"
#endif // USE_FREERTOS


void delay_init(void)
{
	USTIMER_Init();
}

void delay_ms(uint32_t ms)
{
	// delay is prohibited in interrupt function
	if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0)
	{
		DRV_ASSERT(false);
	}

	// attempt to block the current thread by freertos system call
#if USE_FREERTOS == 1
	// block current thread only if freertos scheduler is running
	if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
		vTaskDelay(pdMS_TO_TICKS(ms));
	else
		USTIMER_DelayIntSafe(ms * 1000);
#else
	// otherwise use block wait
	USTIMER_DelayIntSafe(ms * 1000);
#endif
}

void delay_us(uint32_t us)
{
	if (us > 1000)
		delay_ms(us/1000);
	else
	{
		// delay is prohibited in interrupt function
		if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0)
		{
			DRV_ASSERT(false);
		}

		USTIMER_DelayIntSafe(us);
	}

}
