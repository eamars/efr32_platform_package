/**
 * @brief Network Co-Processor for EFR32 RAIL raw protocol
 * @author Ran Bao
 * @date 23/Nov/2017
 * @file wg_mac_ncp.h
 */

#ifndef WG_MAC_NCP_H_
#define WG_MAC_NCP_H_

#if USE_FREERTOS == 1

#define WG_MAC_NCP_MSG_BUFFER_SIZE 256
#define WG_MAC_NCP_MAX_CLIENT_COUNT 16
#define WG_MAC_NCP_QUEUE_LENGTH 4

#include <stdint.h>

#include "radio_template.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

typedef enum
{
    WG_MAC_NCP_RX,
    WG_MAC_NCP_TX
} wg_mac_ncp_state;

typedef enum
{
    WG_MAC_NCP_NO_ERROR,
    WG_MAC_NCP_UNKNOWN_ERROR,
    WG_MAC_NCP_INVALID_PACKET_LENGTH,
    WG_MAC_NCP_NO_CLIENT_SLOT_AVAILABLE,
    WG_MAC_NCP_NO_CLIENT_FOUND,
} wg_mac_ncp_error_code_t;

typedef struct
{
    uint8_t buffer[WG_MAC_NCP_MSG_BUFFER_SIZE]; // 1 byte reserved for data alignment
    uint16_t size;
    int32_t rssi;
    int32_t snr;
} wg_mac_ncp_msg_t;

typedef struct
{
    bool is_valid;
    uint8_t short_id;
    uint64_t device_eui64;
    uint32_t last_seen_sec;
    uint32_t next_retry_time_sec;

    wg_mac_ncp_msg_t prev_packet;
    bool prev_packet_acked;
    uint8_t retry_counter;
    uint8_t tx_seqid;
    uint8_t rx_seqid;

} wg_mac_ncp_client_t;

typedef struct
{
    uint64_t local_eui64;
    uint32_t max_heartbeat_period_sec;
    uint32_t ack_window_sec;
    uint8_t max_retries;
    bool forward_all_packets;
    bool auto_ack;
} wg_mac_ncp_config_t;

typedef struct
{
    // internal properties
    radio_t * radio;
    uint32_t sec_since_start;
    wg_mac_ncp_state state;

    // packet queue
    QueueHandle_t tx_queue_pri;
    QueueHandle_t tx_queue;
    QueueHandle_t rx_queue_pri;
    QueueHandle_t rx_queue;

    // queue set
    QueueSetHandle_t queue_set_pri;

    SemaphoreHandle_t tx_done_signal;

    // bookkeeper timer
    TimerHandle_t bookkeeping_timer;

    // bookkeeping fsm
    TaskHandle_t client_bookkeeping_thread;
    TaskHandle_t state_machine_thread;
    TaskHandle_t tx_queue_handler_thread;

    // config
    wg_mac_ncp_config_t config;

    // connected clients
    wg_mac_ncp_client_t clients[WG_MAC_NCP_MAX_CLIENT_COUNT];
} wg_mac_ncp_t;


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Default configuration for NCP
 */
extern const wg_mac_ncp_config_t wg_mac_ncp_default_config;

/**
 * @brief Initialize WG_MAC Network Co-Processor stack
 * @param obj the NCP object
 * @param radio the abstract radio object
 * @param config the setings. If NULL is passed the network stack will use default configuration instead @see wg_mac_ncp_default_config
 */
void wg_mac_ncp_init(wg_mac_ncp_t * obj, radio_t * radio, wg_mac_ncp_config_t * config);

/**
 * @brief Send bytes to transceiver
 * @param obj the NCP object
 * @param msg the message, contains destination address
 * @param timeout_ms timeout when waiting for place in queue
 * @return if the timeout is triggered
 */
bool wg_mac_ncp_send_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, uint32_t timeout_ms);
#define wg_mac_ncp_send(obj, msg) \
        wg_mac_ncp_send_timeout((obj), (msg), 0)
#define wg_mac_ncp_send_block(obj, msg) \
        wg_mac_ncp_send_timeout((obj), (msg), portMAX_DELAY)

/**
 * @brief Read a packet from receive queue
 * @param obj the NCP object
 * @param msg the message, contains source address
 * @param timeout_ms timeout when waiting to read from queue
 * @return if the timeout is triggered
 */
bool wg_mac_ncp_recv_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, uint32_t timeout_ms);
#define wg_mac_ncp_recv(obj, msg) \
        wg_mac_ncp_recv_timeout((obj), (msg), 0)
#define wg_mac_ncp(obj, msg) \
        wg_mac_ncp_recv_timeout((obj), (msg), portMAX_DELAY)

#ifdef __cplusplus
}
#endif

#endif // USE_FREERTOS == 1

#endif // WG_MAC_NCP_H_
