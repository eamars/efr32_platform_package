/**
 * @brief Simple PHY layer message transceiver
 * @date 19, Oct, 2017
 * @author Ran Bao
 * @file lora_phy.h
 */

#ifndef LORA_PHY_H
#define LORA_PHY_H

#if USE_FREERTOS == 1


#include "radio_rfm9x.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#define LORA_PHY_MAX_RETRIES 3

/**
 * @brief Internal LoRa state machine properties
 */
typedef enum
{
    LORA_PHY_IDLE,
    LORA_PHY_TX,
    LORA_PHY_RX
} lora_phy_fsm_state_t;


/**
 * @brief Physical layer message payload
 */
typedef struct
{
    uint8_t buffer[RADIO_RFM9X_RW_BUFFER_SIZE - 1]; // 1 byte reserved for data alignment
    uint8_t size;
} lora_phy_msg_t;

typedef struct
{
    xTimerHandle rx_window_timer;
    lora_phy_msg_t prev_packet;
    uint8_t retry_counter;

} lora_phy_retransmit_t;

typedef struct
{
    // transceiver driver
    radio_rfm9x_t * radio;
    uint8_t device_id8;
    uint8_t local_seq_id;

    // packet queue
    QueueHandle_t tx_queue;
    QueueHandle_t rx_queue_pri;
    QueueHandle_t rx_queue;

    // state machine
    lora_phy_fsm_state_t fsm_state;
    TaskHandle_t fsm_thread_handler;
    SemaphoreHandle_t fsm_tx_done;

    // link quality
    int16_t last_packet_rssi;
    int8_t last_packet_snr;

    // retransmit
    lora_phy_retransmit_t retransmit;
} lora_phy_t;


#ifdef __cplusplus
extern "C" {
#endif

void lora_phy_init(lora_phy_t * obj, radio_rfm9x_t * radio, uint8_t device_id8);

/**
 * @brief Send bytes to transceiver
 * @param obj the transceiver object
 * @param msg message buffer @see radio_rfm9x_msg_t. The message buffer have maximum payload length, @see RADIO_RFM9X_RW_BUFFER_SIZE
 * @param timeout_ms maximum block time, portMAX_DELAY if block forever
 * @return whether message is transmitted successfully within timeout, if maximum block time is configured
 */
bool lora_phy_send_timeout(lora_phy_t * obj, lora_phy_msg_t * msg, uint32_t timeout_ms);
#define lora_phy_send(obj, msg) \
        lora_phy_send_timeout((obj), (msg), RADIO_RFM9X_DEFAULT_TX_TIMEOUT)
#define lora_phy_send_block(obj, msg) \
        lora_phy_send_timeout((obj), (msg), portMAX_DELAY)

/**
 * @brief Receive bytes from transceiver
 * @param obj the transceiver object
 * @param msg message buffer @see radio_rfm9x_msg_t. The message buffer have maximum payload length, @see RADIO_RFM9X_RW_BUFFER_SIZE
 * @param timeout_ms maximum block time, portMAX_DELAY if block forever
 * @return whether received message is valid, if maximum block time is specified
 */
bool lora_phy_recv_timeout(lora_phy_t * obj, lora_phy_msg_t * msg, uint32_t timeout_ms);
#define lora_phy_recv(obj, msg) \
        lora_phy_recv_timeout((obj), (msg), RADIO_RFM9X_DEFAULT_RX_TIMEOUT)
#define lora_phy_recv_block(obj, msg) \
        lora_phy_recv_timeout((obj), (msg), portMAX_DELAY)

#ifdef __cplusplus
}
#endif

#endif // USE_FREERTOS == 1

#endif // LORA_PHY_H
