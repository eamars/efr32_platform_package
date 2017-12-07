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
#define WG_MAC_DEFAULT_RX_WINDOW_TIMEOUT_MS 1000


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
    uint8_t buffer[WG_MAC_MSG_BUFFER_SIZE]; // 1 byte reserved for data alignment
    uint16_t size;
    int32_t rssi;
    int32_t snr;
} wg_mac_msg_t;

typedef struct
{
    uint64_t local_eui64;
    uint32_t rx_window_timeout_ms;
    uint32_t tx_timeout_ms;
    uint8_t max_retransmit;
    bool forwared_all_packets;
} wg_mac_config_t;

typedef enum
{
    WG_MAC_NO_ERROR,
    WG_MAC_UNKNOWN_ERROR,
    WG_MAC_INVALID_PACKET_LENGTH,
} wg_mac_error_code_t;


typedef struct
{
    // transceiver driver
    radio_t * radio;

    // transmit seqid (local copy, maintained by driver)
    uint8_t local_seq_id;

    // packet queue
    QueueHandle_t tx_queue;
    QueueHandle_t rx_queue_pri;
    QueueHandle_t rx_queue;

    // state machine
    wg_mac_fsm_state_t fsm_state;
    TaskHandle_t fsm_thread_handler;
    SemaphoreHandle_t fsm_tx_done;

    // configuration
    wg_mac_config_t config;

    // link state
    struct
    {
        bool is_network_joined;
        uint8_t allocated_id;
        uint8_t uplink_dest_id;
    } link_state;

    // retransmit
    struct
    {
        xTimerHandle rx_window_timer;
        wg_mac_msg_t prev_packet;
        uint8_t retry_counter;
    } retransmit;
} wg_mac_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default configuration for wg_mac module
 */
extern const wg_mac_config_t wg_mac_default_config;

void wg_mac_init(wg_mac_t * obj, radio_t * radio, wg_mac_config_t * config);

void wg_mac_join_network(wg_mac_t * obj);

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
