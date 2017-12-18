/**
 * @file atcommand.h
 * @brief AT command parser
 * @author David Barclay (david.barclay@wirelessguard.co.nz)
 * @date December, 2017
 *
 * AT command parser to take a buffer and split it into its command components
 */

/**
 * AT - "hello"
 * ATE - toggle echo
 * ATI - device/firmware information
 * ATZ - soft reset
 * AT+{COMMAND}
 * 
 * Command structure:
 *  Run command (no arguments):
 *      AT+{AT_CMD_NAME}
 *  Run command (with arguments):
 *      AT+{AT_CMD_NAME}={AT_CMD_ARGS}
 *  Get value:
 *      AT+{AT_CMD_NAME}?
 * 
 * Command responses:
 *      TX: "AT\r\n"
 *      RX: "OK\r\n"
 * 
 *      TX: "ATE=1\r\n"
 *      RX: "OK\r\n"
 * 
 *      TX: "ATE?\r\n"
 *      RX: "1\r\n"
 *      RX: "OK\r\n"
 * 
 * Commands:
 *  - ECHO={STRING}
 *  - RESET
 *  - RESET={RESET_STATE}
 */

#ifndef _ATCOMMAND_H
#define _ATCOMMAND_H

#include <stdint.h>
#include <stdbool.h>

#define ATCMD_MAX_CMDLEN    127


typedef enum {
    ATCMD_CMD_HELLO = 1,
    ATCMD_CMD_ECHO,
    ATCMD_CMD_INFO,
    ATCMD_CMD_RESET,
    ATCMD_CMD_COMMAND
} atcmd_cmd_t;

typedef enum {
    ATCMD_TYPE_NULL = 0, // ATE
    ATCMD_TYPE_GET,      // ATE?
    ATCMD_TYPE_RUN      // ATE=0
} atcmd_type_t;

typedef enum {
    ATCMD_COMMAND_NONE = 0,
    ATCMD_COMMAND_ECHO,
    ATCMD_COMMAND_RESET
} atcmd_command_t;

typedef struct {
    atcmd_cmd_t cmd; // Base command
    atcmd_type_t type; // Command type (\0?=)
    atcmd_command_t command; // Extended command type
    char args[112]; // Up to 112 characters in the args
} atcmd_t;


// Parses a null terminated buffer for the first AT command in it. 
// Ignores trailing '\r','\n'
int8_t atcmd_parse_buffer (atcmd_t *cmd, char *buffer);

#endif //_ATCOMMAND_H