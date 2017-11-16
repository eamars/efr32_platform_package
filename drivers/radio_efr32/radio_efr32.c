/**
 * @brief Abstract interface for accessing EFR32 builtin radio module
 * @file radio_efr32.c
 * @author Ran Bao
 * @date 16/Nov/2017
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <drivers/radio_template/radio_template.h>

#include "radio_efr32.h"
#include "drv_debug.h"
#include "pa_conversions_efr32.h"

#define PA_RAMP                                   (10)
#define PA_2P4_LOWPOWER                           (0)
#define PA_DEFAULT_POWER                                  (252)
#define PA_DCDC_VOLTAGE                                (1800)
#define PA_VBAT_VOLTAGE                                (1800)

static radio_efr32_t radio_efr32_singleton_instance;
static bool radio_efr32_initialized = false;

static void radio_efr32_rail_interrupt_handler_pri(RAIL_Handle_t rail_handle, RAIL_Events_t events)
{
    if (events & RAIL_EVENT_TX_PACKET_SENT)
    {
        if (radio_efr32_singleton_instance.base.on_tx_done_cb.callback)
            ((on_tx_done_handler_t) radio_efr32_singleton_instance.base.on_tx_done_cb.callback)(
                    radio_efr32_singleton_instance.base.on_tx_done_cb.args
            );
    }

    if (events & RAIL_EVENT_RX_FRAME_ERROR)
    {
        if (radio_efr32_singleton_instance.base.on_rx_error_cb.callback)
            ((on_rx_error_handler_t) radio_efr32_singleton_instance.base.on_rx_error_cb.callback)(
                    radio_efr32_singleton_instance.base.on_rx_error_cb.args
            );
    }

    if (events & RAIL_EVENT_RX_PACKET_RECEIVED)
    {

    }
}

radio_efr32_t * radio_efr32_init(const RAIL_ChannelConfig_t *channelConfigs[], bool use_dcdc)
{
    // oops, someone trying to initialize instance twice!
    if (radio_efr32_initialized)
    {
        DRV_ASSERT(false);
        return &radio_efr32_singleton_instance;
    }

    // first time invoking this function
    radio_efr32_initialized = true;

    // initialize the instance
    memset(&radio_efr32_singleton_instance, 0x0, sizeof(radio_efr32_t));

    // initialize RAIL
    {
        RAIL_Config_t rail_cfg = {
                .eventsCallback = radio_efr32_rail_interrupt_handler_pri,
        };

        // get rail handler
        radio_efr32_singleton_instance.rail_handle = RAIL_Init(&rail_cfg, NULL);
        DRV_ASSERT(radio_efr32_singleton_instance.rail_handle);

        // configure available channels
        RAIL_ConfigChannels(radio_efr32_singleton_instance.rail_handle, channelConfigs[0], NULL);

        // configure callback events
        DRV_ASSERT(RAIL_ConfigEvents(
                radio_efr32_singleton_instance.rail_handle,
                RAIL_EVENTS_ALL,
                RAIL_EVENT_TX_PACKET_SENT | RAIL_EVENT_RX_PACKET_RECEIVED | RAIL_EVENT_RX_FRAME_ERROR
        ) == RAIL_STATUS_NO_ERROR);

        // configure power amplifier
        RAIL_TxPowerCurvesConfig_t tx_power_curves_config;
        RAIL_TxPowerConfig_t tx_power_config;

        if (use_dcdc)
        {
            RAIL_DECLARE_TX_POWER_VBAT_CURVES(piecewiseSegments, curvesSg, curves24Hp, curves24Lp);
            tx_power_curves_config.piecewiseSegments = piecewiseSegments;
            tx_power_curves_config.txPower24HpCurves = curves24Hp;
            tx_power_curves_config.txPower24LpCurves = curves24Lp;
            tx_power_curves_config.txPowerSgCurves = curvesSg;

            tx_power_config.mode = RAIL_TX_POWER_MODE_SUBGIG;
            tx_power_config.voltage = PA_VBAT_VOLTAGE;
            tx_power_config.rampTime = PA_RAMP;
        }
        else
        {
            RAIL_DECLARE_TX_POWER_DCDC_CURVES(piecewiseSegments, curvesSg, curves24Hp, curves24Lp);
            tx_power_curves_config.piecewiseSegments = piecewiseSegments;
            tx_power_curves_config.txPower24HpCurves = curves24Hp;
            tx_power_curves_config.txPower24LpCurves = curves24Lp;
            tx_power_curves_config.txPowerSgCurves = curvesSg;

            tx_power_config.mode = RAIL_TX_POWER_MODE_SUBGIG;
            tx_power_config.voltage = PA_DCDC_VOLTAGE;
            tx_power_config.rampTime = PA_RAMP;
        }

        DRV_ASSERT(RAIL_InitTxPowerCurves(&tx_power_curves_config) == RAIL_STATUS_NO_ERROR);
        DRV_ASSERT(RAIL_ConfigTxPower(
                radio_efr32_singleton_instance.rail_handle,
                &tx_power_config
        ) == RAIL_STATUS_NO_ERROR);

        // set transmit power
        DRV_ASSERT(RAIL_SetTxPower(radio_efr32_singleton_instance.rail_handle, PA_DEFAULT_POWER) == RAIL_STATUS_NO_ERROR);

        // set default transition state
        RAIL_StateTransitions_t transitions = {
                .success = RAIL_RF_STATE_RX,
                .error = RAIL_RF_STATE_RX
        };

        RAIL_SetRxTransitions(radio_efr32_singleton_instance.rail_handle, &transitions);
        RAIL_SetTxTransitions(radio_efr32_singleton_instance.rail_handle, &transitions);

        // set radio to idle state
        RAIL_Idle(radio_efr32_singleton_instance.rail_handle, RAIL_IDLE, true);
    }

    return &radio_efr32_singleton_instance;
}


