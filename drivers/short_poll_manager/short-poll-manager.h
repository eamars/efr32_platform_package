/**
 * @brief Synchronized short poll interval management across various applications
 * @file short-poll-manager.c
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef SHORT_POLL_MANAGER_H_
#define SHORT_POLL_MANAGER_H_

#include <stdint.h>

#define SHORT_POLL_HIGH_FREQ_MS 200

typedef struct
{
	uint16_t pushed_short_poll_interval_ms;
	uint16_t ref_counter;
} short_poll_t;


#ifdef __cplusplus
extern "C" {
#endif

void short_poll_init(short_poll_t *obj);
void short_poll_high_freq_set(short_poll_t *obj);
void short_poll_restore(short_poll_t *obj);

#ifdef __cplusplus
}
#endif


#endif /* SHORT_POLL_MANAGER_H_ */
