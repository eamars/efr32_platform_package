/**
 * @brief Simple PHY layer message transceiver
 * @date 21/Nov/2017
 * @author Ran Bao
 * @file wg_mac.h
 */
#ifndef WG_MAC_H_
#define WG_MAC_H_

#if USE_FREERTOS == 1

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "radio_template.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

#include "subg_mac.h"

#define WG_MAC_MSG_BUFFER_SIZE 0xff // this value need to be greater than the sizeof biggest payload + overhead
#define WG_MAC_DEFAULT_TX_TIMEOUT_MS 5000
#define WG_MAC_DEFAULT_MAX_RETRIES 5
#define WG_MAC_DEFAULT_RX_WINDOW_TIMEOUT_MS 1000
#define WG_MAC_QUEUE_LENGTH 4

/**
 * @brief Internal LoRa state machine properties
 */
typedef enum
{
    WG_MAC_IDLE,
    WG_MAC_TX,
    WG_MAC_RX
} wg_mac_fsm_state_t;

typedef enum
{
    WG_MAC_NETWORK_JOINED,
    WG_MAC_NETWORK_LEFT
} wg_mac_network_state_t;

typedef struct
{
    uint8_t allocated_id;
    uint8_t uplink_dest_id;
    bool is_network_joined;
} wg_mac_link_state_t;

/**
 * @brief Physical layer message payload
 */
typedef struct
{
    uint8_t buffer[WG_MAC_MSG_BUFFER_SIZE]; // 1 byte reserved for data alignment
    uint16_t size;
    int32_t rssi;
    int32_t quality;
} wg_mac_raw_msg_t;

typedef struct
{
    uint16_t payload_size;
    uint8_t payload[SUBG_MAC_PACKET_MAX_DATA_PAYLOAD_SIZE];
    bool requires_ack;
} wg_mac_uplink_msg_t;

typedef struct
{
    int32_t rssi;
    int32_t quality;
    uint16_t payload_size;
    uint8_t payload[SUBG_MAC_PACKET_MAX_DATA_PAYLOAD_SIZE];
    uint8_t src_id;
} wg_mac_downlink_msg_t;

typedef struct
{
    uint64_t local_eui64;
    uint32_t rx_window_timeout_ms;
    uint32_t tx_timeout_ms;
    uint8_t max_retransmit;
} wg_mac_config_t;

typedef enum
{
    WG_MAC_NO_ERROR,
    WG_MAC_UNKNOWN_ERROR,
    WG_MAC_INVALID_PACKET_LENGTH,
} wg_mac_error_code_t;


typedef void (*wg_mac_on_network_state_changed)(void * obj, wg_mac_network_state_t state);
typedef void (*wg_mac_on_raw_packet_received)(void * obj, wg_mac_raw_msg_t *msg);


typedef struct
{
    // transceiver driver
    radio_t * radio;

    // transmit seqid (local copy, maintained by driver)
    uint8_t local_seq_id;

    // packet queue
    QueueHandle_t rx_data_packet_queue;
    QueueHandle_t rx_raw_packet_queue;

    QueueHandle_t tx_data_packet_queue;
    QueueHandle_t tx_raw_packet_queue;
    QueueSetHandle_t tx_queue_set;


    // state machine
    wg_mac_fsm_state_t fsm_state;
    TaskHandle_t fsm_thread_handler;
    SemaphoreHandle_t fsm_tx_done;

    // configuration
    wg_mac_config_t config;

    // link state
    wg_mac_link_state_t link_state;

    // retransmit
    struct
    {
        xTimerHandle rx_window_timer;
        wg_mac_raw_msg_t prev_packet;
        uint8_t retry_counter;
        bool is_packet_clear;
    } retransmit;

    struct
    {
        wg_mac_on_network_state_changed on_network_state_changed;

        // callback for debug purpose
        wg_mac_on_raw_packet_received on_raw_packet_received;
    } callbacks;
} wg_mac_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default configuration for wg_mac module
 */
extern const wg_mac_config_t wg_mac_default_config;

/**
 * @brief Initialize WG_MAC client network stack
 * @param obj the network stack object
 * @param radio the abstract radio object
 * @param config the settings. If NULL is passed the network stack will use default configuration instead @see wg_mac_default_config
 */
void wg_mac_init(wg_mac_t * obj, radio_t * radio, wg_mac_config_t * config);

/**
 * @brief Send join network request
 * @param obj the network stack object
 */
void wg_mac_join_network(wg_mac_t * obj);

/**
 * @brief Send bytes to transceiver
 * @param obj the transceiver object
 * @param msg message buffer @see radio_rfm9x_msg_t. The message buffer have maximum payload length, @see RADIO_RFM9X_RW_BUFFER_SIZE
 * @param timeout_ms maximum block time, portMAX_DELAY if block forever
 * @return whether message is transmitted successfully within timeout, if maximum block time is configured
 */
bool wg_mac_send_timeout(wg_mac_t * obj, wg_mac_uplink_msg_t * msg, uint32_t timeout_ms);
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
bool wg_mac_recv_timeout(wg_mac_t * obj, wg_mac_downlink_msg_t * msg, uint32_t timeout_ms);
#define wg_mac_recv(obj, msg) \
		wg_mac_recv_timeout((obj), (msg), 0)
#define wg_mac_recv_block(obj, msg) \
		wg_mac_recv_timeout((obj), (msg), portMAX_DELAY)

#ifdef __cplusplus
}
#endif

#endif // USE_FREERTOS == 1

#endif // WG_MAC_H_
