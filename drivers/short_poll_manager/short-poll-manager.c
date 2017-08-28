/**
 * @brief Synchronized short poll interval management across various applications
 * @file short-poll-manager.c
 * @author Ran Bao
 * @date Aug, 2017
 */

#include <string.h>
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"

#include "short-poll-manager.h"
#include "af.h"

void short_poll_init(short_poll_t *obj)
{
	memset(obj, 0x0, sizeof(short_poll_t));
}


void short_poll_high_freq_set(short_poll_t *obj)
{
	INTERRUPTS_OFF();

	// You cannot set a new value when there is another application using it
	assert(obj->ref_counter == 0);

	// store previous value
	obj->pushed_short_poll_interval_ms = emberAfGetShortPollIntervalMsCallback();

	// set a new value
	emberAfSetShortPollIntervalMsCallback(SHORT_POLL_HIGH_FREQ_MS);

	// increase the counter
	obj->ref_counter += 1;

	INTERRUPTS_ON();
}


void short_poll_restore(short_poll_t *obj)
{
	INTERRUPTS_OFF();
	// decrease the reference counter when the value is not zero
	// other application might still using the value
	if (obj->ref_counter > 0)
	{
		obj->ref_counter -= 1;
	}

	// if the current counter is zero, then restore previous configuration
	if (obj->ref_counter == 0)
	{
		emberAfSetShortPollIntervalMsCallback(obj->pushed_short_poll_interval_ms);
	}
	INTERRUPTS_ON();
}
