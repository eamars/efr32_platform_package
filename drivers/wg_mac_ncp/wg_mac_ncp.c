/**
 * @brief Network Co-Processor for EFR32 RAIL raw protocol
 * @author Ran Bao
 * @date 23/Nov/2017
 * @file wg_mac_ncp.h
 */

#include <string.h>

#include "wg_mac_ncp.h"
#include "drv_debug.h"

static const wg_mac_ncp_config_t wg_mac_ncp_default_config = {
        .local_eui64 = 0,
        .max_heartbeat_period_sec = 3600,
        .ack_window_ms = 5000,
        .max_retries = 3,
        .forward_all_packets = true,
};

static void wg_mac_ncp_on_rx_done_handler(wg_mac_ncp_t * obj)
{

}

static void wg_mac_ncp_on_tx_done_handler(wg_mac_ncp_t * obj)
{
    xSemaphoreGiveFromISR(obj->tx_done_signal, NULL);
}

static void wg_mac_ncp_send_pri(wg_mac_ncp_t * obj, wg_mac_msg_t * msg, bool generate_seqid)
{
    // TODO: Need internal queue
}

static void wg_mac_ncp_bookkeeping_handler_pri(TimerHandle_t xTimer)
{
    DRV_ASSERT(xTimer);

    // read object
    wg_mac_ncp_t * obj = (wg_mac_ncp_t *) pvTimerGetTimerID(xTimer);

    // increase internal timer
    obj->sec_since_start += 1;

    // remove the device from list if unseen for a certain amount of time
    for (uint8_t idx = 0; idx < WG_MAC_NCP_MAX_CLIENT_COUNT; idx++)
    {
        if (obj->clients[idx].is_valid)
        {
            uint32_t unseen_period = obj->sec_since_start - obj->clients[idx].last_seen_sec;

            // if the device not ack me yet
            if (!obj->clients[idx].prev_packet_acked)
            {
                if (unseen_period > obj->config.ack_window_ms)
                {
                    obj->clients[idx].retry_counter += 1;

                    // exceed the maximum retry threshold, then remove the device from list
                    if (obj->clients[idx].retry_counter >= obj->config.max_retries)
                    {
                        // TODO: report to host that a device is irresponsible

                        // mark the device as invalid
                        obj->clients[idx].is_valid = false;
                    }
                    else
                    {
                        // retransmit the same packet
                        wg_mac_ncp_send_pri(obj, &obj->clients[idx].prev_packet, false);
                    }
                }
            }

            // remove the device that failed to periodically report to host
            if (unseen_period > obj->config.max_heartbeat_period_sec)
            {
                // TODO: report to host that a device is lost

                // mark the device as invalid
                obj->clients[idx].is_valid = false;
            }


        }
    }
}


static void wg_mac_ncp_bookkeeping_thread(wg_mac_ncp_t * obj)
{
    while (1)
    {
        switch (obj->state)
        {
            case WG_MAC_NCP_RX:
            {
                break;
            }
            case WG_MAC_NCP_TX:
            {
                break;
            }
        }
    }
}

void wg_mac_ncp_init(wg_mac_ncp_t * obj, radio_t * radio, wg_mac_ncp_config_t * config)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(radio);

    // copy config
    obj->radio = radio;

    // make the radio sleep during setup
    radio_set_opmode_sleep(obj->radio);

    // if the user didn't supply the config, then use default copy
    if (config)
        memcpy(&obj->config, config, sizeof(wg_mac_ncp_config_t));
    else
        memcpy(&obj->config, &wg_mac_ncp_default_config, sizeof(wg_mac_ncp_config_t));

    // initialize client list
    for (uint8_t client_idx = 0; client_idx < WG_MAC_NCP_MAX_CLIENT_COUNT; client_idx++)
    {
        obj->clients[client_idx].is_valid = false;
    }

    // register callback functions
    radio_set_rx_done_handler(obj->radio, wg_mac_ncp_on_rx_done_handler, obj);
    radio_set_tx_done_handler(obj->radio, wg_mac_ncp_on_tx_done_handler, obj);

    // create queue
    obj->tx_queue_pri = xQueueCreate(4, sizeof(wg_mac_msg_t));
    obj->tx_queue = xQueueCreate(4, sizeof(wg_mac_msg_t));
    obj->rx_queue_pri = xQueueCreate(4, sizeof(wg_mac_msg_t));
    obj->rx_queue = xQueueCreate(4, sizeof(wg_mac_msg_t));

    obj->tx_done_signal = xSemaphoreCreateBinary();

    // create book keeping timer (1sec interval)
    obj->bookkeeping_timer = xTimerCreate("ncp_bk", pdMS_TO_TICKS(1000), pdTRUE, obj, wg_mac_ncp_bookkeeping_handler_pri);
    xTimerStart(obj->bookkeeping_timer, portMAX_DELAY);

    // put radio to rx state
    obj->state = WG_MAC_NCP_RX;
    radio_set_opmode_rx_timeout(obj->radio, 0);

    // create book keeping thread (handles transceiver state)
    xTaskCreate((void *) wg_mac_ncp_bookkeeping_thread, "ncp_t", 500, obj, 3, &obj->bookkeeping_thread);
}

bool wg_mac_ncp_send_timeout(wg_mac_ncp_t * obj, wg_mac_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);

    // send message buffer to the queue
    if (timeout_ms == portMAX_DELAY)
    {
        if (xQueueSend(obj->tx_queue, msg, portMAX_DELAY) == pdFALSE)
        {
            return false;
        }
    }
    else
    {
        // wait for TX to get ready
        if (xQueueSend(obj->tx_queue, &msg, pdMS_TO_TICKS(timeout_ms)) == pdFALSE)
        {
            // timeout, indicating we failed to wait for transceiver to get ready
            return false;
        }
    }

    return true;
}


bool wg_mac_ncp_recv_timeout(wg_mac_ncp_t * obj, wg_mac_msg_t * msg, uint32_t timeout_ms)
{
    DRV_ASSERT(obj);
    DRV_ASSERT(msg);

    // allow user to wait forever
    if (timeout_ms == portMAX_DELAY)
    {
        if (xQueueReceive(obj->rx_queue, msg, portMAX_DELAY) == pdFALSE)
        {
            return false;
        }
    }
    else
    {
        // attempt to read from queue
        if (xQueueReceive(obj->rx_queue, msg, pdMS_TO_TICKS(timeout_ms)) == pdFALSE)
        {
            // timeout, nothing is received in rx queue
            return false;
        }
    }

    return true;
}
