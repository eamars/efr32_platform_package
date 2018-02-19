/**
 * @brief The generic radio object template
 * @author Ran Bao
 * @date 13/Nov/2017
 * @file radio_template.h
 */

#ifndef RADIO_TEMPLATE_H_
#define RADIO_TEMPLATE_H_

#include <stdint.h>

#include "drv_debug.h"

typedef struct
{
    void * callback;
    void * args;
} radio_callback_t;

// specific radio callback functions
typedef void (*on_rx_done_handler_t)(void * obj, void * msg, int32_t size, int32_t rssi, int32_t snr);
typedef void (*on_rx_error_handler_t)(void * obj);
typedef void (*on_rx_timeout_handler_t)(void * obj);
typedef void (*on_tx_done_handler_t)(void * obj);
typedef void (*on_tx_timeout_handler_t)(void * obj);

// prototype for radio mode transitions
typedef void (*radio_set_opmode_idle_t)(void * obj);
typedef void (*radio_set_opmode_sleep_t)(void * obj);
typedef void (*radio_set_opmode_rx_timeout_t)(void * obj, uint32_t timeout_ms);
typedef void (*radio_set_opmode_tx_timeout_t)(void * obj, uint32_t timeout_ms);

// prototype for generic radio send/recv function
typedef void (*radio_send_timeout_t)(void * obj, void * buffer, uint16_t size, uint32_t timeout_ms);
typedef void (*radio_recv_timeout_t)(void * obj, uint32_t timeout_ms);

// prototype for setting/getting radio properties
typedef float (*radio_get_tx_power_t)(void * obj);
typedef void (*radio_set_tx_power_t)(void * obj, float power_dbm);

// generic radio opmode states
typedef enum
{
    RADIO_OPMODE_IDLE = 0,
    RADIO_OPMODE_SLEEP = 1,
    RADIO_OPMODE_TX = 2,
    RADIO_OPMODE_RX = 3
} radio_opmode_t;

// generic radio structure
typedef struct
{
    // define generic radio opmode
    radio_opmode_t opmode;

    // define radio callbacks
    radio_callback_t on_rx_done_cb;
    radio_callback_t on_rx_error_cb;
    radio_callback_t on_rx_timeout_cb;
    radio_callback_t on_tx_done_cb;
    radio_callback_t on_tx_timeout_cb;

    // define mode transition functions
    radio_callback_t radio_set_opmode_idle_cb;
    radio_callback_t radio_set_opmode_sleep_cb;
    radio_callback_t radio_set_opmode_tx_timeout_cb;
    radio_callback_t radio_set_opmode_rx_timeout_cb;

    // define generic send function
    radio_callback_t radio_send_cb;
    radio_callback_t radio_recv_cb;

    // define radio property configurations
    radio_callback_t radio_get_tx_power_cb;
    radio_callback_t radio_set_tx_power_cb;


} radio_t;

#ifdef __cplusplus
extern "C" {
#endif

static inline radio_opmode_t radio_get_opmode(radio_t * obj)
{
    return obj->opmode;
}

void radio_set_rx_done_handler(radio_t * obj, void * handler, void * args);
void radio_set_rx_error_handler(radio_t * obj, void * handler, void * args);
void radio_set_rx_timeout_handler(radio_t * obj, void * handler, void * args);
void radio_set_tx_done_handler(radio_t * obj, void * handler, void * args);
void radio_set_tx_timeout_handler(radio_t * obj, void * handler, void * args);

void radio_set_opmode_idle(radio_t * obj);
void radio_set_opmode_sleep(radio_t * obj);
void radio_set_opmode_rx_timeout(radio_t * obj, uint32_t timeout_ms);
void radio_set_opmode_tx_timeout(radio_t * obj, uint32_t timeout_ms);

void radio_send_timeout(radio_t * obj, void * buffer, uint16_t size, uint32_t timeout_ms);
void radio_recv_timeout(radio_t * obj, uint32_t timeout_ms);

float radio_get_tx_power(radio_t * obj);
void radio_set_tx_power(radio_t * obj, float power_dbm);

#ifdef __cplusplus
}
#endif

#endif // RADIO_TEMPLATE_H_
