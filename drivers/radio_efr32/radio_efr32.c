/**
 * @brief Abstract interface for accessing EFR32 builtin radio module
 * @file radio_efr32.c
 * @author Ran Bao
 * @date 16/Nov/2017
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "em_core.h"
#include "radio_efr32.h"
#include "drv_debug.h"
#include "pa_conversions_efr32.h"

#define PA_RAMP                                   (10)
#define PA_2P4_LOWPOWER                           (0)
#define PA_DEFAULT_POWER                          (252)
#define PA_DCDC_VOLTAGE                           (1800)
#define PA_VBAT_VOLTAGE                           (1800)

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
        RAIL_RxPacketInfo_t packet_info;
        RAIL_RxPacketDetails_t packet_details;
        RAIL_RxPacketHandle_t packet_handle;

        // get packet handle
        packet_handle = RAIL_GetRxPacketInfo(rail_handle, RAIL_RX_PACKET_HANDLE_NEWEST, &packet_info);

        // get packet details
        RAIL_GetRxPacketDetails(rail_handle, packet_handle, &packet_details);

        if (packet_info.packetStatus != RAIL_RX_PACKET_READY_SUCCESS && packet_info.packetStatus != RAIL_RX_PACKET_READY_CRC_ERROR)
        {
            // some non crc error case
            DRV_ASSERT(false);
            return;
        }

        // send data to callback function
        if (radio_efr32_singleton_instance.base.on_tx_done_cb.callback)
        {
            // make a copy of buffer on current stack (avoid changes directly applied to the data buffer)
            uint8_t received_data[0xff];
            memcpy(received_data, packet_info.firstPortionData, packet_info.firstPortionBytes);
            memcpy(
                    received_data + packet_info.firstPortionBytes,
                    packet_info.lastPortionData,
                    packet_info.packetBytes - packet_info.firstPortionBytes // get bytes for second portion
            );

            ((on_rx_done_handler_t) radio_efr32_singleton_instance.base.on_rx_done_cb.callback)(
                    radio_efr32_singleton_instance.base.on_rx_done_cb.args,
                    received_data,
                    packet_info.packetBytes,
                    packet_details.rssi,
                    packet_details.lqi
            );
        }
    }
    else
        DRV_ASSERT(false);
}

static void radio_efr32_set_opmode_idle_pri(radio_efr32_t * obj)
{
    obj->base.opmode = RADIO_OPMODE_IDLE;

    RAIL_Idle(obj->rail_handle, RAIL_IDLE, true);
}

static void radio_efr32_set_opmode_sleep_pri(radio_efr32_t * obj)
{
    obj->base.opmode = RADIO_OPMODE_SLEEP;

    RAIL_Idle(obj->rail_handle, RAIL_IDLE, true);
}

static void radio_efr32_set_opmode_tx_pri(radio_efr32_t * obj)
{
    obj->base.opmode = RADIO_OPMODE_TX;

    DRV_ASSERT(RAIL_StartTx(obj->rail_handle, obj->channel, RAIL_TX_OPTIONS_DEFAULT, NULL) == RAIL_STATUS_NO_ERROR);
}

static void radio_efr32_set_opmode_rx_pri(radio_efr32_t * obj)
{
    obj->base.opmode = RADIO_OPMODE_RX;

    DRV_ASSERT(RAIL_StartRx(obj->rail_handle, obj->channel, NULL) == RAIL_STATUS_NO_ERROR);
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
            RAIL_EVENT_TX_PACKET_SENT |
                    RAIL_EVENT_RX_PACKET_RECEIVED |
                    RAIL_EVENT_RX_FRAME_ERROR
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

    // configure power curve and output power
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

    // configure default callbacks
    radio_set_opmode_handler(
            &radio_efr32_singleton_instance.base,
            RADIO_OPMODE_IDLE,
            (radio_opmode_transition_t) radio_efr32_set_opmode_idle_pri,
            &radio_efr32_singleton_instance
    );

    radio_set_opmode_handler(
            &radio_efr32_singleton_instance.base,
            RADIO_OPMODE_SLEEP,
            (radio_opmode_transition_t) radio_efr32_set_opmode_idle_pri,
            &radio_efr32_singleton_instance
    );

    radio_set_opmode_handler(
            &radio_efr32_singleton_instance.base,
            RADIO_OPMODE_TX,
            (radio_opmode_transition_t) radio_efr32_set_opmode_tx_pri,
            &radio_efr32_singleton_instance
    );

    radio_set_opmode_handler(
            &radio_efr32_singleton_instance.base,
            RADIO_OPMODE_RX,
            (radio_opmode_transition_t) radio_efr32_set_opmode_rx_pri,
            &radio_efr32_singleton_instance
    );

    // set default channel
    radio_efr32_set_channel(&radio_efr32_singleton_instance, 0);

    // set default idle state
    radio_efr32_set_opmode_idle_pri(&radio_efr32_singleton_instance);

    return &radio_efr32_singleton_instance;
}

void radio_efr32_set_channel(radio_efr32_t * obj, uint8_t channel)
{
    obj->channel = channel;
}

void radio_efr32_send(radio_efr32_t * obj, void * buffer, uint8_t size)
{
    radio_efr32_set_opmode_idle_pri(obj);

    // write buffer
    memcpy(obj->tx_buffer, buffer, size);

    RAIL_SetTxFifo(obj->rail_handle, obj->tx_buffer, size, size);

    // set to tx mode
    radio_efr32_set_opmode_tx_pri(obj);
}
