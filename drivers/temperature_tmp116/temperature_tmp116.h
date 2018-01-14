/**
 * @brief TMP116 temperature sensor driver
 * @file temperature_tmp116.h
 * @author Ran Bao, Connor Janie
 * @date Sept, 2017
 */

#ifndef TEMPERATURE_TMP116_H
#define TEMPERATURE_TMP116_H

#include <stdint.h>
#include <stdbool.h>

#include "i2cdrv.h"
#include "pio_defs.h"

#if TEMPERATURE_TMP116_USE_FREERTOS_DELAY == 1
	#include "FreeRTOS.h"
	#include "task.h"
#endif

#define TEMPERATURE_TMP116_HIGH_LIMIT_DEFAULT (50.0f) // degree
#define TEMPERATURE_TMP116_LOW_LIMIT_DEFAULT (-20.0f) // degree

#define TMP116_TEMPERATURE_RESOLUTION 0.0078125f


typedef struct
{
	i2cdrv_t * i2c_device;
	pio_t alert;
	uint16_t local_config_cache;
	bool is_local_config_cache_dirty;
} temperature_tmp116_t;

typedef enum
{
	TEMPERATURE_TMP116_CONTINOUS_CONVERSION_READ_BACK_MODE = 0x0,
	TEMPERATURE_TMP116_SHUTDOWN_MODE = 0x1,
	TEMPERATURE_TMP116_CONTINOUS_CONVERSION_MODE = 0x2,
	TEMPERATURE_TMP116_ONE_SHOT_MODE = 0x3
} temperature_tmp116_functional_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

void temperature_tmp116_init(temperature_tmp116_t * obj, i2cdrv_t * i2c_device, pio_t alert);
void temperature_tmp116_deinit(temperature_tmp116_t * obj);

void temperature_tmp116_set_high_limit(temperature_tmp116_t * obj, float high_threshold);
void temperature_tmp116_set_low_limit(temperature_tmp116_t * obj, float low_threshold);

float temperature_tmp116_get_high_limit(temperature_tmp116_t * obj);
float temperature_tmp116_get_low_limit(temperature_tmp116_t * obj);

int16_t temperature_tmp116_read_temperature_raw(temperature_tmp116_t * obj);
float temperature_tmp116_read_temperature(temperature_tmp116_t * obj);

void temperature_tmp116_pull_config(temperature_tmp116_t * obj);
void temperature_tmp116_push_config(temperature_tmp116_t * obj);

void temperature_tmp116_set_conversion_mode(temperature_tmp116_t * obj, temperature_tmp116_functional_mode_t mode);
static void temperature_tmp116_reader(temperature_tmp116_t * obj);

#ifdef __cplusplus
}
#endif

#endif // TEMPERATURE_TMP116_H
