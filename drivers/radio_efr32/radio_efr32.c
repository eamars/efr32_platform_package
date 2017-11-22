/**
 * @brief Abstract interface for accessing EFR32 builtin radio module
 * @file radio_efr32.c
 * @author Ran Bao
 * @date 16/Nov/2017
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "radio_template.h"
#include "radio_efr32.h"
#include "drv_debug.h"
#include "pa_conversions_efr32.h"
#include "irq.h"

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
#if USE_FREERTOS == 1
        // stop the tx timeout timer
        xTimerStopFromISR(radio_efr32_singleton_instance.tx_timeout_timer, NULL);
#endif

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

#if USE_FREERTOS == 1
        // stop the rx timout timer
        xTimerStopFromISR(radio_efr32_singleton_instance.rx_timeout_timer, NULL);
#endif

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
}

static void radio_efr32_set_opmode_idle_pri(radio_efr32_t * obj)
{
    DRV_ASSERT(obj);

    if (obj->base.opmode != RADIO_OPMODE_IDLE)
    {
#if USE_FREERTOS == 1
        // stop all timers before entering idle state
        if (__IS_INTERRUPT())
        {
            xTimerStopFromISR(obj->rx_timeout_timer, NULL);
            xTimerStopFromISR(obj->tx_timeout_timer, NULL);
        }
        else
        {
            xTimerStop(obj->rx_timeout_timer, portMAX_DELAY);
            xTimerStop(obj->tx_timeout_timer, portMAX_DELAY);
        }
#endif

        RAIL_Idle(obj->rail_handle, RAIL_IDLE, true);

        obj->base.opmode = RADIO_OPMODE_IDLE;
    }
}

static void radio_efr32_set_opmode_sleep_pri(radio_efr32_t * obj)
{
    DRV_ASSERT(obj);

    if (obj->base.opmode != RADIO_OPMODE_SLEEP)
    {
#if USE_FREERTOS == 1
        // stop all timers before entering sleep state
        if (__IS_INTERRUPT())
        {
            xTimerStopFromISR(obj->rx_timeout_timer, NULL);
            xTimerStopFromISR(obj->tx_timeout_timer, NULL);
        }
        else
        {
            xTimerStop(obj->rx_timeout_timer, portMAX_DELAY);
            xTimerStop(obj->tx_timeout_timer, portMAX_DELAY);
        }
#endif

        RAIL_Idle(obj->rail_handle, RAIL_IDLE, true);

        obj->base.opmode = RADIO_OPMODE_SLEEP;
    }

}

static void radio_efr32_set_opmode_tx_timeout_pri(radio_efr32_t * obj, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);

    if (obj->base.opmode != RADIO_OPMODE_TX)
    {
        // call RAIL API to enter Tx mode
        DRV_ASSERT(RAIL_StartTx(obj->rail_handle, obj->channel, RAIL_TX_OPTIONS_DEFAULT, NULL) == RAIL_STATUS_NO_ERROR);

#if USE_FREERTOS == 1
        if (timeout_ms != 0)
        {
            if (__IS_INTERRUPT())
            {
                xTimerChangePeriodFromISR(obj->tx_timeout_timer, pdMS_TO_TICKS(timeout_ms), NULL);
                xTimerStartFromISR(obj->tx_timeout_timer, NULL);
            }
            else
            {
                xTimerChangePeriod(obj->tx_timeout_timer, pdMS_TO_TICKS(timeout_ms), portMAX_DELAY);
                xTimerStart(obj->tx_timeout_timer, portMAX_DELAY);
            }
        }
#endif

        obj->base.opmode = RADIO_OPMODE_TX;
    }
}

static void radio_efr32_set_opmode_rx_timeout_pri(radio_efr32_t * obj, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    
    if (obj->base.opmode != RADIO_OPMODE_RX)
    {
        DRV_ASSERT(RAIL_StartRx(obj->rail_handle, obj->channel, NULL) == RAIL_STATUS_NO_ERROR);

#if USE_FREERTOS == 1
        if (timeout_ms != 0)
        {
            if (__IS_INTERRUPT())
            {
                xTimerChangePeriodFromISR(obj->rx_timeout_timer, pdMS_TO_TICKS(timeout_ms), NULL);
                xTimerStartFromISR(obj->rx_timeout_timer, NULL);
            }
            else
            {
                xTimerChangePeriod(obj->rx_timeout_timer, pdMS_TO_TICKS(timeout_ms), portMAX_DELAY);
                xTimerStart(obj->rx_timeout_timer, portMAX_DELAY);
            }
        }
#endif

        obj->base.opmode = RADIO_OPMODE_RX;
    }

}

#if USE_FREERTOS == 1
static void radio_efr32_on_timer_timeout(TimerHandle_t xTimer)
{
    DRV_ASSERT(xTimer);

    xTimerStop(xTimer, portMAX_DELAY);

    radio_efr32_t * obj = (radio_efr32_t *) pvTimerGetTimerID(xTimer);

    switch (obj->base.opmode)
    {
        case RADIO_OPMODE_RX:
        {
            if (obj->base.on_rx_timeout_cb.callback)
                ((on_rx_timeout_handler_t) obj->base.on_rx_timeout_cb.callback)(obj->base.on_rx_timeout_cb.args);

            break;
        }

        case RADIO_OPMODE_TX:
        {
            if (obj->base.on_tx_timeout_cb.callback)
                ((on_rx_timeout_handler_t) obj->base.on_tx_timeout_cb.callback)(obj->base.on_tx_timeout_cb.args);

            break;
        }

        default:
        {
            break;
        }
    }
}
#endif

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
    radio_efr32_singleton_instance.rail_cfg.eventsCallback = radio_efr32_rail_interrupt_handler_pri;

    // get rail handler
    radio_efr32_singleton_instance.rail_handle = RAIL_Init(&radio_efr32_singleton_instance.rail_cfg, NULL);
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

    // set default channel
    radio_efr32_set_channel(&radio_efr32_singleton_instance, 0);

    // set default idle state
    radio_efr32_set_opmode_idle_pri(&radio_efr32_singleton_instance);

    // configure default callbacks
    radio_efr32_singleton_instance.base.radio_set_opmode_idle_cb.callback = radio_efr32_set_opmode_idle_pri;
    radio_efr32_singleton_instance.base.radio_set_opmode_idle_cb.args = &radio_efr32_singleton_instance;

    radio_efr32_singleton_instance.base.radio_set_opmode_sleep_cb.callback = radio_efr32_set_opmode_sleep_pri;
    radio_efr32_singleton_instance.base.radio_set_opmode_sleep_cb.args = &radio_efr32_singleton_instance;

    radio_efr32_singleton_instance.base.radio_set_opmode_rx_timeout_cb.callback = radio_efr32_set_opmode_rx_timeout_pri;
    radio_efr32_singleton_instance.base.radio_set_opmode_rx_timeout_cb.args = &radio_efr32_singleton_instance;

    radio_efr32_singleton_instance.base.radio_set_opmode_tx_timeout_cb.callback = radio_efr32_set_opmode_tx_timeout_pri;
    radio_efr32_singleton_instance.base.radio_set_opmode_tx_timeout_cb.args = &radio_efr32_singleton_instance;

    // register internal send callback function
    radio_efr32_singleton_instance.base.radio_send_cb.callback = radio_efr32_send_timeout;
    radio_efr32_singleton_instance.base.radio_send_cb.args = &radio_efr32_singleton_instance;

#if USE_FREERTOS == 1
    // initialize timer
    radio_efr32_singleton_instance.rx_timeout_timer = xTimerCreate(
            "efr_rx_t",
            pdMS_TO_TICKS(1),
            pdFALSE,
            &radio_efr32_singleton_instance,
            radio_efr32_on_timer_timeout
    );

    radio_efr32_singleton_instance.tx_timeout_timer = xTimerCreate(
            "efr_tx_t",
            pdMS_TO_TICKS(1),
            pdFALSE,
            &radio_efr32_singleton_instance,
            radio_efr32_on_timer_timeout
    );
#endif

    return &radio_efr32_singleton_instance;
}

void radio_efr32_set_channel(radio_efr32_t * obj, uint8_t channel)
{
    obj->channel = channel;
}

void radio_efr32_send_timeout(radio_efr32_t * obj, void * buffer, uint16_t size, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(obj->rail_handle);

    // set mode to idle preventing any unwanted error
    radio_efr32_set_opmode_idle_pri(obj);

    // write buffer
    memcpy(obj->tx_buffer, buffer, size);
    RAIL_SetTxFifo(obj->rail_handle, obj->tx_buffer, size, size);

    // set to tx mode
    radio_efr32_set_opmode_tx_timeout_pri(obj, timeout_ms);
}


uint32_t radio_efr32_generate_random_number(radio_efr32_t * obj)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(obj->rail_handle);

    uint32_t number = 0;

    DRV_ASSERT(RAIL_GetRadioEntropy(obj->rail_handle, (void *) &number, sizeof(uint32_t)));

    return number;
}
