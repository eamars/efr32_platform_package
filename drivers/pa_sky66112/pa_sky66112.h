/**
 * @brief Implementation of SKY66112-11 Zigbee power amplifier
 * @file pa_sky66112.h
 * @author Ran Bao
 * @date Sept, 2017
 */

#ifndef PA_SKY66112_H_
#define PA_SKY66112_H_

#include <stdint.h>
#include "pio_defs.h"

typedef enum
{
	PA_SLEEP_MODE_0 = 0,
	PA_RECEIVE_LNA_MODE = 1,
	PA_TRANSMIT_HP_MODE = 2,
	PA_TRANSMIT_LP_MODE = 3,
	PA_RECEIVE_BYPASS_MODE = 4,
	PA_TRANSMIT_BYPASS_MODE = 5,
	PA_SLEEP_MODE_1 = 6
} pa_mode_t;

typedef enum
{
	PA_ANT1 = 0,
	PA_ANT2 = 1
} pa_ant_t;

typedef struct
{
	pio_t ant_sel;
	pio_t csd;
	pio_t cps;
	pio_t crx;
	pio_t ctx;
	pio_t chl;

	pa_mode_t current_mode;
	pa_ant_t current_antenna;
} pa_sky66112_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize SKY66112-11 power amplifier module
 * @param obj power amplifier object
 * @param ant_sel antenna select pin
 * @param csd CSD pin
 * @param cps CPS pin
 * @param crx CRX pin
 * @param ctx CTX pin
 * @param chl CHL pin
 */
void pa_sky66112_init(pa_sky66112_t * obj, pio_t ant_sel, pio_t csd, pio_t cps, pio_t crx, pio_t ctx, pio_t chl);

/**
 * @brief Set power amplifier operation mode
 * @param obj power amplifier object
 * @param mode operation mode @see pa_sky66112_t
 */
void pa_sky66112_set_mode(pa_sky66112_t * obj, pa_mode_t mode);

/**
 * @brief Get current power amplifier operation mode
 * @param obj power amplifier object
 * @return current power amplifier configuration (cached version)
 */
pa_mode_t pa_sky66112_get_mode(pa_sky66112_t * obj);

/**
 * @brief Select antenna attached to power amplifier
 * @param obj power amplifier object
 * @param antenna antenna to choose @see pa_ant_t
 */
void pa_sky66112_set_antenna(pa_sky66112_t * obj, pa_ant_t antenna);

/**
 * @brief Get current operational antenna
 * @param obj power amplifier object
 * @return antenna that is currently using (cached version)
 */
pa_ant_t pa_sky66112_get_antenna(pa_sky66112_t * obj);


#ifdef __cplusplus
}
#endif

#endif // PA_SKY66112_H_
