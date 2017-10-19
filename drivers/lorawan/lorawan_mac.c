/**
 * @brief MAC layer PDU and implementation
 * @date 18, Oct, 2017
 * @author Ran Bao
 * @file lorawan_mac.c
 */

#include "lorawan_mac.h"
#include "drv_debug.h"
#include "lorawan_types.h"


/*!
 * Class A&B receive delay 1 in ms
 */
#define RECEIVE_DELAY1                              1000

/*!
 * Class A&B receive delay 2 in ms
 */
#define RECEIVE_DELAY2                              2000


static void lorawan_mac_rx_window_timer1_callback(TimerHandle_t xTimer)
{
	DRV_ASSERT(xTimer);

	xTimerStop(xTimer, 0);

	// restore the timer id (which is used to store the parameter
	lorawan_mac_t * obj = (lorawan_mac_t *) pvTimerGetTimerID(xTimer);

}

static void lorawan_mac_rx_window_timer2_callback(TimerHandle_t xTimer)
{
	DRV_ASSERT(xTimer);

	xTimerStop(xTimer, 0);

	// restore the timer id (which is used to store the parameter
	lorawan_mac_t * obj = (lorawan_mac_t *) pvTimerGetTimerID(xTimer);
}


static void lorawan_mac_on_rx_done_isr(uint8_t * msg, int16_t size, int16_t rssi, int8_t snr, lorawan_mac_t * obj)
{

}

static void lorawan_mac_on_tx_done_isr(lorawan_mac_t * obj)
{
	// TODO: put the radio in sleep state

	// at the moment the packet is transmitted, we are going to setup two rx slots, with corresponding delays

	// setup first time slot, with update-to-date rx_window_delay1
	xTimerChangePeriodFromISR(obj->rx_window_timer1, pdMS_TO_TICKS(obj->rx_window_delay1), NULL);
	xTimerStartFromISR(obj->rx_window_timer1, NULL);

	// setup the second time slot, with update-to-date rx_window_delay2
	xTimerChangePeriodFromISR(obj->rx_window_timer2, pdMS_TO_TICKS(obj->rx_window_delay2), NULL);
	xTimerStartFromISR(obj->rx_window_timer2, NULL);


}

static void lorawan_mac_on_tx_timeout_thread(lorawan_mac_t * obj)
{
	// TODO: put the radio in sleep state
}

static void lorawan_mac_on_rx_error_isr(lorawan_mac_t * obj)
{

}

static void lorawan_mac_on_rx_timeout_isr(lorawan_mac_t * obj)
{

}

void lorawan_mac_init(lorawan_mac_t * obj, radio_rfm9x_t * radio)
{
	DRV_ASSERT(obj);
	DRV_ASSERT(radio);

	// copy variables
	obj->radio = radio;
	obj->rx_window_delay1 = RECEIVE_DELAY1;
	obj->rx_window_delay2 = RECEIVE_DELAY2;

	radio_rfm9x_set_rx_done_isr_callback(obj->radio, (void *) lorawan_mac_on_rx_done_isr, obj);
	radio_rfm9x_set_tx_done_isr_callback(obj->radio, (void *) lorawan_mac_on_tx_done_isr, obj);
	radio_rfm9x_set_tx_timeout_thread_callback(obj->radio, (void *) lorawan_mac_on_tx_timeout_thread, obj);

	// create timer instance
	obj->rx_window_timer1 = xTimerCreate("rxw_t1", pdMS_TO_TICKS(obj->rx_window_delay1), pdFALSE, obj, lorawan_mac_rx_window_timer1_callback);
	obj->rx_window_timer2 = xTimerCreate("rxw_t2", pdMS_TO_TICKS(obj->rx_window_delay2), pdFALSE, obj, lorawan_mac_rx_window_timer2_callback);
}
