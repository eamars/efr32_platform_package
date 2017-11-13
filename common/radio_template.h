/**
 * @brief The generic radio object template
 * @author Ran Bao
 * @date 13/Nov/2017
 * @file radio_template.h
 */

#ifndef RADIO_TEMPLATE_H_
#define RADIO_TEMPLATE_H_

// generic callback storage object
typedef struct
{
    void * callback;
    void * args;
} radio_callback_t;

// specific radio callback functions
typedef void (*on_rx_done_handler)(void * obj, void * msg, int32_t size, int32_t rssi, int32_t snr);
typedef void (*on_rx_error_handler)(void * obj);
typedef void (*on_rx_timeout_handler)(void * obj);
typedef void (*on_tx_done_handler)(void * obj);
typedef void (*on_tx_timeout_handler)(void * obj);

// prototype for radio mode transitions
typedef void (*radio_opmode_transition_t)(void * obj);

// generic radio opmode states
typedef enum
{
    RADIO_OPMODE_IDLE = 0,
    RADIO_OPMODE_SLEEP,
    RADIO_OPMODE_TX,
    RADIO_OPMODE_RX
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
    radio_opmode_transition_t radio_set_opmode_sleep;
    radio_opmode_transition_t radio_set_opmode_idle;
    radio_opmode_transition_t radio_set_opmode_tx;
    radio_opmode_transition_t radio_set_opmode_rx;

    // define generic radio opmode
    radio_opmode_t opmode;

} radio_t;


#endif // RADIO_TEMPLATE_H_
