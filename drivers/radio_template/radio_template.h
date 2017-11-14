/**
 * @brief The generic radio object template
 * @author Ran Bao
 * @date 13/Nov/2017
 * @file radio_template.h
 */

#ifndef RADIO_TEMPLATE_H_
#define RADIO_TEMPLATE_H_

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
typedef void (*radio_opmode_transition_t)(void * obj);

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
    // define radio callbacks
    radio_callback_t on_rx_done_cb;
    radio_callback_t on_rx_error_cb;
    radio_callback_t on_rx_timeout_cb;
    radio_callback_t on_tx_done_cb;
    radio_callback_t on_tx_timeout_cb;

    // define mode transition functions
    radio_callback_t radio_opmode_transition_functions[4];

    // define generic radio opmode
    radio_opmode_t opmode;

} radio_t;

#ifdef __cplusplus
extern "C" {
#endif

void radio_set_rx_done_handler(radio_t * obj, on_rx_done_handler_t handler, void * args);
void radio_set_rx_error_handler(radio_t * obj, on_rx_error_handler_t handler, void * args);
void radio_set_rx_timeout_handler(radio_t * obj, on_rx_timeout_handler_t handler, void * args);
void radio_set_tx_done_handler(radio_t * obj, on_tx_done_handler_t handler, void * args);
void radio_set_tx_timeout_handler(radio_t * obj, on_tx_timeout_handler_t handler, void * args);

void radio_set_opmode_handler(radio_t * obj, radio_opmode_t opmode, radio_opmode_transition_t handler, void * args);

void radio_set_opmode(radio_t * obj, radio_opmode_t opmode);
static inline radio_opmode_t radio_get_opmode(radio_t * obj)
{
    return obj->opmode;
}



#ifdef __cplusplus
}
#endif

#endif // RADIO_TEMPLATE_H_
