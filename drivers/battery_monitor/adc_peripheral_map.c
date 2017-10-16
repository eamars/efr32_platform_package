/**
 * @brief Peripehral map for ADC
 */

#include <stddef.h>
#include "em_device.h"
#include "em_adc.h"
#include "pio_defs.h"

const pio_map_t adc_map[] = {
		{PA0,  ADC0, adcPosSelAPORT3XCH8},
		{PA1,  ADC0, adcPosSelAPORT4XCH9},
		{PA2,  ADC0, adcPosSelAPORT3XCH10},
		{PA3,  ADC0, adcPosSelAPORT4XCH11},
		{PA4,  ADC0, adcPosSelAPORT3XCH12},
		{PA5,  ADC0, adcPosSelAPORT4XCH13},

		{PB11, ADC0, adcPosSelAPORT4XCH27},
		{PB12, ADC0, adcPosSelAPORT3XCH28},
		{PB14, ADC0, adcPosSelAPORT3XCH30},
		{PB15, ADC0, adcPosSelAPORT4XCH31},

		{PC6,  ADC0, adcPosSelAPORT1XCH6},
		{PC7,  ADC0, adcPosSelAPORT2XCH7},
		{PC8,  ADC0, adcPosSelAPORT1XCH8},
		{PC9,  ADC0, adcPosSelAPORT2XCH9},
		{PC10, ADC0, adcPosSelAPORT1XCH10},
		{PC11, ADC0, adcPosSelAPORT2XCH11},

		{PD9,  ADC0, adcPosSelAPORT4XCH1},
		{PD10, ADC0, adcPosSelAPORT3XCH2},
		{PD11, ADC0, adcPosSelAPORT3YCH3},
		{PD12, ADC0, adcPosSelAPORT3XCH4},
		{PD13, ADC0, adcPosSelAPORT3YCH5},
		{PD14, ADC0, adcPosSelAPORT3XCH6},
		{PD15, ADC0, adcPosSelAPORT4XCH7},

		{PF0,  ADC0, adcPosSelAPORT1XCH16},
		{PF1,  ADC0, adcPosSelAPORT2XCH17},
		{PF2,  ADC0, adcPosSelAPORT1XCH18},
		{PF3,  ADC0, adcPosSelAPORT2XCH19},
		{PF4,  ADC0, adcPosSelAPORT1XCH20},
		{PF5,  ADC0, adcPosSelAPORT2XCH21},
		{PF6,  ADC0, adcPosSelAPORT1XCH22},
		{PF7,  ADC0, adcPosSelAPORT2XCH23},
		{NC ,  NULL   , 0}
};
