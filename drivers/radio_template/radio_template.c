/*
 * Implementation for generic radio operation
 */

#include <stdbool.h>

#include "radio_template.h"
#include "drv_debug.h"


void radio_set_rx_done_handler(radio_t * obj, on_rx_done_handler_t handler, void * args)
{
    obj->on_rx_done_cb.callback = handler;
    obj->on_rx_done_cb.args = args;
}


void radio_set_rx_error_handler(radio_t * obj, on_rx_error_handler_t handler, void * args)
{
    obj->on_rx_error_cb.callback = handler;
    obj->on_rx_error_cb.args = args;
}


void radio_set_rx_timeout_handler(radio_t * obj, on_rx_timeout_handler_t handler, void * args)
{
    obj->on_rx_timeout_cb.callback = handler;
    obj->on_rx_timeout_cb.args = args;
}


void radio_set_tx_done_handler(radio_t * obj, on_tx_done_handler_t handler, void * args)
{
    obj->on_tx_done_cb.callback = handler;
    obj->on_tx_done_cb.args = args;
}


void radio_set_tx_timeout_handler(radio_t * obj, on_tx_timeout_handler_t handler, void * args)
{
    obj->on_tx_timeout_cb.callback = handler;
    obj->on_tx_timeout_cb.args = args;
}


void radio_set_opmode_handler(radio_t * obj, radio_opmode_t opmode, radio_opmode_transition_t handler, void * args)
{
    obj->radio_opmode_transition_functions[opmode].callback = handler;
    obj->radio_opmode_transition_functions[opmode].args = args;
}


void radio_set_opmode(radio_t * obj, radio_opmode_t opmode)
{
    obj->opmode = opmode;

    // execute radio state transition function
    if (obj->radio_opmode_transition_functions[opmode].callback)
        ((radio_opmode_transition_t) obj->radio_opmode_transition_functions[opmode].callback)(
                obj->radio_opmode_transition_functions[opmode].args
        );
    else
        DRV_ASSERT(false);
}
