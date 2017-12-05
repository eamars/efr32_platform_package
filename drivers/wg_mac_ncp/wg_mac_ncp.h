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

typedef struct
{
    uint8_t buffer[WG_MAC_NCP_MSG_BUFFER_SIZE]; // 1 byte reserved for data alignment
    uint16_t size;
    bool requires_ack;
} wg_mac_ncp_msg_t;

typedef struct
{
    bool is_valid;
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
    bool enable_beacon;
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

/**
 * @brief Default configuration for NCP
 */
extern const wg_mac_ncp_config_t wg_mac_ncp_default_config;

#endif // USE_FREERTOS == 1


#ifdef __cplusplus
extern "C" {
#endif


void wg_mac_ncp_init(wg_mac_ncp_t * obj, radio_t * radio, wg_mac_ncp_config_t * config);
bool wg_mac_ncp_send_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, uint32_t timeout_ms);
bool wg_mac_ncp_recv_timeout(wg_mac_ncp_t * obj, wg_mac_ncp_msg_t * msg, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif


#endif // WG_MAC_NCP_H_
