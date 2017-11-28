/*
 * Implementation for generic radio operation
 */

#include <stdbool.h>

#include "radio_template.h"
#include "drv_debug.h"


void radio_set_rx_done_handler(radio_t * obj, void * handler, void * args)
{
    obj->on_rx_done_cb.callback = handler;
    obj->on_rx_done_cb.args = args;
}


void radio_set_rx_error_handler(radio_t * obj, void * handler, void * args)
{
    obj->on_rx_error_cb.callback = handler;
    obj->on_rx_error_cb.args = args;
}


void radio_set_rx_timeout_handler(radio_t * obj, void * handler, void * args)
{
    obj->on_rx_timeout_cb.callback = handler;
    obj->on_rx_timeout_cb.args = args;
}


void radio_set_tx_done_handler(radio_t * obj, void * handler, void * args)
{
    obj->on_tx_done_cb.callback = handler;
    obj->on_tx_done_cb.args = args;
}


void radio_set_tx_timeout_handler(radio_t * obj, void * handler, void * args)
{
    obj->on_tx_timeout_cb.callback = handler;
    obj->on_tx_timeout_cb.args = args;
}


void radio_set_opmode_idle(radio_t * obj)
{
    if (obj->radio_set_opmode_idle_cb.callback)
    {
        ((radio_set_opmode_idle_t) obj->radio_set_opmode_idle_cb.callback)(obj->radio_set_opmode_idle_cb.args);
    }
    else
    {
        DRV_ASSERT(false);
    }

}

void radio_set_opmode_sleep(radio_t * obj)
{
    if (obj->radio_set_opmode_sleep_cb.callback)
    {
        ((radio_set_opmode_sleep_t) obj->radio_set_opmode_sleep_cb.callback)(obj->radio_set_opmode_sleep_cb.args);
    }
    else
    {
        DRV_ASSERT(false);
    }
}

void radio_set_opmode_rx_timeout(radio_t * obj, uint32_t timeout_ms)
{
    if (obj->radio_set_opmode_rx_timeout_cb.callback)
    {
        ((radio_set_opmode_rx_timeout_t) obj->radio_set_opmode_rx_timeout_cb.callback)(obj->radio_set_opmode_rx_timeout_cb.args, timeout_ms);
    }
    else
    {
        DRV_ASSERT(false);
    }
}

void radio_set_opmode_tx_timeout(radio_t * obj, uint32_t timeout_ms)
{
    if (obj->radio_set_opmode_tx_timeout_cb.callback)
    {
        ((radio_set_opmode_tx_timeout_t) obj->radio_set_opmode_tx_timeout_cb.callback)(obj->radio_set_opmode_tx_timeout_cb.args, timeout_ms);
    }
    else
    {
        DRV_ASSERT(false);
    }
}


void radio_send_timeout(radio_t * obj, void * buffer, uint16_t size, uint32_t timeout_ms)
{
    // execute internal send function
    if (obj->radio_send_cb.callback)
    {
        ((radio_send_timeout_t) obj->radio_send_cb.callback)(obj->radio_send_cb.args, buffer, size, timeout_ms);
    }
    else
    {
        DRV_ASSERT(false);
    }
}

void radio_recv_timeout(radio_t * obj, uint32_t timeout_ms)
{
    // execute internal recv function
    if (obj->radio_recv_cb.callback)
    {
        ((radio_recv_timeout_t) obj->radio_recv_cb.callback)(obj->radio_recv_cb.args, timeout_ms);
    }
    else
    {
        DRV_ASSERT(false);
    }
}
