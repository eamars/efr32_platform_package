/**
 * @brief Implementation of SKY66112-11 Zigbee power amplifier
 * @file pa_sky66112.c
 * @author Ran Bao
 * @date Sept, 2017
 */

#include "pa_sky66112.h"
#include "drv_debug.h"
#include "prs.h"
#include "em_cmu.h"

void pa_sky66112_init(pa_sky66112_t * obj, pio_t ant_sel, pio_t csd, pio_t cps, pio_t crx, pio_t ctx, pio_t chl)
{
	DRV_ASSERT(obj);

	// copy variables
	obj->ant_sel = ant_sel;
	obj->csd = csd;
	obj->cps = cps;
	obj->crx = crx;
	obj->ctx = ctx;
	obj->chl = chl;
	obj->ctx_prs_ch = 0;

	// enable GPIO clock
	CMU_ClockEnable(cmuClock_GPIO, true);

	// set gpios as outputs
	GPIO_PinModeSet(PIO_PORT(obj->ant_sel), PIO_PIN(obj->ant_sel), gpioModePushPull, 0);
	GPIO_PinModeSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd), gpioModePushPull, 0);
	GPIO_PinModeSet(PIO_PORT(obj->cps), PIO_PIN(obj->cps), gpioModePushPull, 0);
	GPIO_PinModeSet(PIO_PORT(obj->crx), PIO_PIN(obj->crx), gpioModePushPull, 0);
	GPIO_PinModeSet(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx), gpioModePushPull, 0);
	GPIO_PinModeSet(PIO_PORT(obj->chl), PIO_PIN(obj->chl), gpioModePushPull, 0);

	// by default the pa enters bypass mode
	pa_sky66112_set_mode(obj, PA_RECEIVE_BYPASS_MODE);
	pa_sky66112_set_mode(obj, PA_TRANSMIT_BYPASS_MODE);
}


void pa_sky66112_set_mode(pa_sky66112_t * obj, pa_mode_t mode)
{
	DRV_ASSERT(obj);

	// store mode
	obj->current_mode = mode;

	switch (mode)
	{
		case PA_SLEEP_MODE_0:
		{
			// As specified in datasheet, all controls must be at Vdd or 0v to achieve t the specified sleep current
			// clear csd
			GPIO_PinOutClear(PIO_PORT(obj->csd), PIO_PIN(obj->csd));
			break;
		}
		case PA_RECEIVE_LNA_MODE:
		{
			// set csd
			GPIO_PinOutSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd));

			// clear cps
			GPIO_PinOutClear(PIO_PORT(obj->cps), PIO_PIN(obj->cps));

			// set crx
			GPIO_PinOutSet(PIO_PORT(obj->crx), PIO_PIN(obj->crx));

			// clear ctx
			GPIO_PinOutClear(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx));

			break;
		}
		case PA_TRANSMIT_HP_MODE:
		{
			// set csd
			GPIO_PinOutSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd));

			// clear cps
			GPIO_PinOutClear(PIO_PORT(obj->cps), PIO_PIN(obj->cps));

			// set ctx
			GPIO_PinOutSet(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx));

			// set chl
			GPIO_PinOutSet(PIO_PORT(obj->chl), PIO_PIN(obj->chl));

			break;
		}
		case PA_TRANSMIT_LP_MODE:
		{
			// set csd
			GPIO_PinOutSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd));

			// clear cps
			GPIO_PinOutClear(PIO_PORT(obj->cps), PIO_PIN(obj->cps));

			// set ctx
			GPIO_PinOutSet(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx));

			// clear chl
			GPIO_PinOutClear(PIO_PORT(obj->chl), PIO_PIN(obj->chl));

			break;
		}
		case PA_RECEIVE_BYPASS_MODE:
		{
			// set csd
			GPIO_PinOutSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd));

			// set cps
			GPIO_PinOutSet(PIO_PORT(obj->cps), PIO_PIN(obj->cps));

			// set crx
			GPIO_PinOutSet(PIO_PORT(obj->crx), PIO_PIN(obj->crx));

			// clear ctx
			GPIO_PinOutClear(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx));

			break;
		}
		case PA_TRANSMIT_BYPASS_MODE:
		{
			// set csd
			GPIO_PinOutSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd));

			// set cps
			GPIO_PinOutSet(PIO_PORT(obj->cps), PIO_PIN(obj->cps));

			// set ctx
			GPIO_PinOutSet(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx));

			break;
		}
		case PA_SLEEP_MODE_1:
		{
			// set csd
			GPIO_PinOutSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd));

			// clear crx
			GPIO_PinOutClear(PIO_PORT(obj->crx), PIO_PIN(obj->crx));

			// clear ctx
			GPIO_PinOutClear(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx));

			break;
		}
		default:
		{
			DRV_ASSERT(false);
			break;
		}
	}
}


pa_mode_t pa_sky66112_get_mode(pa_sky66112_t * obj)
{
	DRV_ASSERT(obj);

	return obj->current_mode;
}


void pa_sky66112_set_antenna(pa_sky66112_t * obj, pa_ant_t antenna)
{
	DRV_ASSERT(obj);

	// store antenna configuration
	obj->current_antenna = antenna;

	switch(antenna)
	{
		case PA_ANT1:
		{
			GPIO_PinOutClear(PIO_PORT(obj->ant_sel), PIO_PIN(obj->ant_sel));
			break;
		}
		case PA_ANT2:
		{
			GPIO_PinOutSet(PIO_PORT(obj->ant_sel), PIO_PIN(obj->ant_sel));
			break;
		}
		default:
		{
			DRV_ASSERT(false);
			break;
		}
	}
}


pa_ant_t pa_sky66112_get_antenna(pa_sky66112_t * obj)
{
	DRV_ASSERT(obj);
	return obj->current_antenna;
}

void pa_sky66112_connect_rac(pa_sky66112_t * obj, uint8_t ctx_prs_ch, uint8_t ctx_prs_loc)
{
	DRV_ASSERT(obj);

	obj->ctx_prs_ch = ctx_prs_ch;

	/**
	 * @brief set default op mode : RXLNA
	 * 				CSD		CPS		CRX		CTX		CHL
	 * RX_LNA:		1		0		1		0		X(1)
	 * TX_HP: 		1		0		X(1)	1		1
	 *
	 * CTX is the only GPIO we need to toggle
	 */

	// set csd
	GPIO_PinOutSet(PIO_PORT(obj->csd), PIO_PIN(obj->csd));

	// clear cps
	GPIO_PinOutClear(PIO_PORT(obj->cps), PIO_PIN(obj->cps));

	// set crx
	GPIO_PinOutSet(PIO_PORT(obj->crx), PIO_PIN(obj->crx));

	// clear ctx
	GPIO_PinOutClear(PIO_PORT(obj->ctx), PIO_PIN(obj->ctx));

	// set chl
	GPIO_PinOutSet(PIO_PORT(obj->chl), PIO_PIN(obj->chl));

	/**
	 * @brief configure PRS for CTX pin
	 *
	 * When the radio is about to transmit, CTX is toggled high to enter TRANSMIT_HP_MODE. When done, the CTX is restored
	 * back to 0, entering RECEIVE_LNA_MODE.
	 */
	halConfigurePrs(ctx_prs_ch, ctx_prs_loc,
	                ((PRS_RAC_PAEN & _PRS_CH_CTRL_SOURCESEL_MASK) >> _PRS_CH_CTRL_SOURCESEL_SHIFT),
	                ((PRS_RAC_PAEN & _PRS_CH_CTRL_SIGSEL_MASK) >> _PRS_CH_CTRL_SIGSEL_SHIFT),
	                false
	);
}

/**
 * @brief Disconnect the Radio and the power amplifier
 * @param obj power amplifier object
 */
void pa_sky66112_disconnect_rac(pa_sky66112_t * obj)
{
	DRV_ASSERT(obj);

	// disable prs channel
	BUS_RegBitWrite(&PRS->ROUTEPEN,
	                _PRS_ROUTEPEN_CH0PEN_SHIFT + obj->ctx_prs_ch,
	                0);

	// enter default bypass mode
	pa_sky66112_set_mode(obj, PA_RECEIVE_BYPASS_MODE);
	pa_sky66112_set_mode(obj, PA_TRANSMIT_BYPASS_MODE);
}
