/**
 * @brief WG Mac protocol running under Sub-GHz band (Centered at 866MHz)
 * @author Ran Bao
 * @date 7/Dec/2017
 * @file subg_mac.h
 *
 * For more information, visit:
 * @see https://docs.google.com/a/wirelessguard.co.nz/document/d/16_lXap3jrpjHero1t4Y5XJ2KB_WJ8xmIGJc0uXTiyAY/edit?usp=sharing
 */

#ifndef SUBG_MAC_H_
#define SUBG_MAC_H_

#include <stdint.h>

#define SUBG_MAC_MAGIC_BYTE 0x03
#define SUBG_MAC_HEADER_SIZE (5)
#define SUBG_MAC_CMD_HEADER_SIZE (SUBG_MAC_HEADER_SIZE + 1)
#define SUBG_MAC_PACKET_CMD_ACK_SIZE (SUBG_MAC_CMD_HEADER_SIZE + 2)
#define SUBG_MAC_PACKET_CMD_JOIN_REQ_SIZE (SUBG_MAC_CMD_HEADER_SIZE + 8)
#define SUBG_MAC_PACKET_CMD_JOIN_RESP_SIZE (SUBG_MAC_CMD_HEADER_SIZE + 2)
#define SUBG_MAC_PACKET_DATA_HEADER_SIZE (SUBG_MAC_HEADER_SIZE + 3)
#define SUBG_MAC_PACKET_ROUTING_HEADER_SIZE (SUBG_MAC_HEADER_SIZE + 17)
#define SUBG_MAC_PACKET_MAX_DATA_PAYLOAD_SIZE 128

#define SUBG_MAC_BROADCAST_ADDR_ID 0xFF

typedef enum
{
    SUBG_MAC_PACKET_CMD = 0,
    SUBG_MAC_PACKET_DATA = 1,
    SUBG_MAC_PACKET_ROUTING = 2
} subg_mac_packet_type_e;

typedef enum
{
    SUBG_MAC_PACKET_CMD_ACK = 0,
    SUBG_MAC_PACKET_CMD_JOIN_REQ = 1,
    SUBG_MAC_PACKET_CMD_JOIN_RESP = 2,
} subg_mac_packet_cmd_type_e;

typedef enum
{
    SUBG_MAC_PACKET_CMD_ACK_CONFIRM = 0,
    SUBG_MAC_PACKET_CMD_ACK_REACHABLE = 1,
    SUBG_MAC_PACKET_CMD_ACK_UNREACHABLE = 2,
} subg_mac_packet_cmd_ack_type_e;

typedef enum
{
    SUBG_MAC_PACKET_DATA_CONFIRM = 0,       // requires ack
    SUBG_MAC_PACKET_DATA_UNCONFIRM = 1      // doesn't requires ack
} subg_mac_packet_data_type_e;

typedef union
{
    uint8_t _raw_data[SUBG_MAC_HEADER_SIZE];
    struct __attribute__((packed))
    {
        uint8_t magic_byte;
        uint8_t src_id;
        uint8_t dest_id;
        uint8_t packet_type;
        uint8_t seqid;
    };
} subg_mac_header_t;


typedef union
{
    uint8_t _raw_data[SUBG_MAC_CMD_HEADER_SIZE];
    struct __attribute__((packed))
    {
        subg_mac_header_t mac_header;
        uint8_t cmd_type;
    };
} subg_mac_cmd_header_t;

typedef union
{
    uint8_t _raw_data[SUBG_MAC_PACKET_CMD_ACK_SIZE];
    struct __attribute__((packed))
    {
        subg_mac_cmd_header_t cmd_header;
        uint8_t ack_type;
        uint8_t ack_seqid;
    };
} subg_mac_cmd_ack_t;

typedef union
{
    uint8_t _raw_data[SUBG_MAC_PACKET_CMD_JOIN_REQ_SIZE];
    struct __attribute__((packed))
    {
        subg_mac_cmd_header_t cmd_header;
        uint64_t eui64;
    };
} subg_mac_cmd_join_req_t;

typedef union
{
    uint8_t _raw_data[SUBG_MAC_PACKET_CMD_JOIN_RESP_SIZE];
    struct __attribute__((packed))
    {
        subg_mac_cmd_header_t cmd_header;
        uint8_t allocated_device_id;       // src id for following transmission
        uint8_t uplink_dest_id;            // dest id for following transmission
    };
} subg_mac_cmd_join_resp_t;

typedef union
{
    uint8_t _raw_data[SUBG_MAC_PACKET_DATA_HEADER_SIZE];
    struct __attribute__((packed))
    {
        subg_mac_header_t mac_header;
        uint8_t data_type;
        uint16_t payload_length;
        uint8_t payload_start_place_holder; // this byte do not count in the header size since this byte shares the first byte with the payload
    };
} subg_mac_data_header_t;

typedef union
{
    uint8_t _raw_data[SUBG_MAC_PACKET_ROUTING_HEADER_SIZE];
    struct __attribute__((packed))
    {
        subg_mac_header_t mac_header;
        uint64_t src_eui64;
        uint64_t dest_eui64;
        uint8_t instruction;    // TODO: request or response
    };
} subg_mac_routing_header_t;


#endif // SUBG_MAC_H_
