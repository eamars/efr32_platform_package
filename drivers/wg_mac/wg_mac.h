/**
 * @brief Simple PHY layer message transceiver
 * @date 21/Nov/2017
 * @author Ran Bao
 * @file wg_mac.h
 */
#ifndef WG_MAC_H_
#define WG_MAC_H_

#if USE_FREERTOS == 1

#include "radio_template.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"


#define WG_MAC_MSG_BUFFER_SIZE 0xff
#define WG_MAC_DEFAULT_TX_TIMEOUT_MS 5000
#define WG_MAC_DEFAULT_MAX_RETRIES 3


/**
 * @brief Internal LoRa state machine properties
 */
typedef enum
{
    WG_MAC_IDLE,
    WG_MAC_TX,
    WG_MAC_RX
} wg_mac_fsm_state_t;


/**
 * @brief Physical layer message payload
 */
typedef struct
{
    uint8_t buffer[WG_MAC_MSG_BUFFER_SIZE - 1]; // 1 byte reserved for data alignment
    uint8_t size;
} wg_mac_msg_t;

typedef struct
{
    xTimerHandle rx_window_timer;
    wg_mac_msg_t prev_packet;
    uint8_t retry_counter;
    uint8_t max_retries;
} wg_mac_retransmit_t;


typedef struct
{
    // transceiver driver
    radio_t * radio;
    uint8_t device_id8;
    uint8_t local_seq_id;

    // packet queue
    QueueHandle_t tx_queue;
    QueueHandle_t rx_queue_pri;
    QueueHandle_t rx_queue;

    // state machine
    wg_mac_fsm_state_t fsm_state;
    TaskHandle_t fsm_thread_handler;
    SemaphoreHandle_t fsm_tx_done;

    // link quality
    int16_t last_packet_rssi;
    int8_t last_packet_snr;

    // retransmit
    wg_mac_retransmit_t retransmit;
} wg_mac_t;


#ifdef __cplusplus
extern "C" {
#endif

void wg_mac_init(wg_mac_t * obj, radio_t * radio, uint8_t device_id8);

/**
 * @brief Send bytes to transceiver
 * @param obj the transceiver object
 * @param msg message buffer @see radio_rfm9x_msg_t. The message buffer have maximum payload length, @see RADIO_RFM9X_RW_BUFFER_SIZE
 * @param timeout_ms maximum block time, portMAX_DELAY if block forever
 * @return whether message is transmitted successfully within timeout, if maximum block time is configured
 */
bool wg_mac_send_timeout(wg_mac_t * obj, wg_mac_msg_t * msg, uint32_t timeout_ms);
#define wg_mac_send(obj, msg) \
		wg_mac_send_timeout((obj), (msg), 0)
#define wg_mac_send_block(obj, msg) \
		wg_mac_send_timeout((obj), (msg), portMAX_DELAY)

/**
 * @brief Receive bytes from transceiver
 * @param obj the transceiver object
 * @param msg message buffer @see radio_rfm9x_msg_t. The message buffer have maximum payload length, @see RADIO_RFM9X_RW_BUFFER_SIZE
 * @param timeout_ms maximum block time, portMAX_DELAY if block forever
 * @return whether received message is valid, if maximum block time is specified
 */
bool wg_mac_recv_timeout(wg_mac_t * obj, wg_mac_msg_t * msg, uint32_t timeout_ms);
#define wg_mac_recv(obj, msg) \
		wg_mac_recv_timeout((obj), (msg), 0)
#define wg_mac_recv_block(obj, msg) \
		wg_mac_recv_timeout((obj), (msg), portMAX_DELAY)

#ifdef __cplusplus
}
#endif

#endif // USE_FREERTOS == 1

#endif // WG_MAC_H_
