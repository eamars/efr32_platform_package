/**
 * @file atcommand.c
 * @brief AT command parser
 * @author David Barclay (david.barclay@wirelessguard.co.nz)
 * @date December, 2017
 *
 * AT command parser to take a buffer and split it into its command components
 */

#include "atcommand.h"

#include <string.h>

const char *atcmd_cmd_hello = "AT";
const char *atcmd_cmd_echo = "ATE";
const char *atcmd_cmd_info = "ATI";
const char *atcmd_cmd_reset = "ATZ";
const char *atcmd_cmd_cmd = "AT+";

const char *atcmd_command_echo = "ECHO";
const char *atcmd_command_reset = "RESET";

// Converts commands to upper case and returns total length
uint8_t preparebuffer (char *buffer)
{
    uint8_t i = 0;
    uint8_t seen_equals = 0;
    
    // Iterate through buffer, convert to upper until equals sign
    while ((i < ATCMD_MAX_CMDLEN) && (buffer[i] != '\0')) {
        if (buffer[i] == '=') {
            seen_equals = 1;
        }
        if (!seen_equals && (buffer[i] >= 'a') && (buffer[i] <= 'z')) {
            buffer[i] = buffer[i] - ('A' - 'a');
        }
        i++;
    }

    // If buffer is not null terminated
    if ((i == ATCMD_MAX_CMDLEN) && (buffer[i-1] != '\0')) {
        i = 0;
    }

    return i;
}


int8_t atcmd_parse_header (atcmd_t *atcmd, char *buffer, uint8_t buflen)
{
    if (strncmp(atcmd_cmd_hello, buffer, 2) != 0) {
        return -1;
    }
    
    switch (buffer[2]) {
        case '\0':
        case '=':
        case '?':
            atcmd->cmd = ATCMD_CMD_HELLO;
            break;
        case 'E':
            atcmd->cmd = ATCMD_CMD_ECHO;
            break;
        case 'I':
            atcmd->cmd = ATCMD_CMD_INFO;
            break;
        case 'Z':
            atcmd->cmd = ATCMD_CMD_RESET;
            break;
        case '+':
            atcmd->cmd = ATCMD_CMD_COMMAND;
            break;
        default:
            atcmd->cmd = 0;
            atcmd->args[0] = '\0';
            return -1;
    }

    return 0;
}


// Determnine the type of command in buffer and store in atcmd.
// Returns 0 on success.
int8_t atcmd_parse_type (atcmd_t *atcmd, char *buffer, uint8_t buflen)
{
    char *qm_pos = 0;
    char *eq_pos = 0;

    switch (atcmd->cmd) {
        case ATCMD_CMD_HELLO:
            atcmd->type = ATCMD_TYPE_NULL;
            break;
        case ATCMD_CMD_INFO:
        case ATCMD_CMD_ECHO:
        case ATCMD_CMD_RESET:
            switch (buffer[3]) {
                case '\0':
                    atcmd->type = ATCMD_TYPE_NULL;
                    break;
                case '?':
                    atcmd->type = ATCMD_TYPE_GET;
                    break;
                case '=':
                    atcmd->type = ATCMD_TYPE_RUN;
                    break;
                default:
                    atcmd->type = 0;
                    return 2; // Unrecognized character at end of basic cmd.
            }
        case ATCMD_CMD_COMMAND: // Special case with verbose arguments.
            // Find the first occurrence of '=' or '?'. Else no arguments
            eq_pos = strstr(buffer, "=");
            qm_pos = strstr(buffer, "?");
            if ((eq_pos != NULL) && (qm_pos != NULL)) {
                // Both '=' and '?' found. Use fist occurence.
                atcmd->type = (eq_pos < qm_pos) ? 
                    ATCMD_TYPE_RUN : ATCMD_TYPE_GET;
            }
            else if (eq_pos == NULL) { 
                // Only '?' found
                atcmd->type = ATCMD_TYPE_GET;
            }
            else if (qm_pos == NULL) {
                // Only '=' found
                atcmd->type = ATCMD_TYPE_RUN;
            }
            else {
                // Neither '?' or '=' found.
                atcmd->type = ATCMD_TYPE_NULL;
            }
        default:
            atcmd->type = 0;
            return 1;
    }

    return 0;
}

int8_t atcmd_parse_args (atcmd_t *atcmd, char *buffer, uint8_t buflen)
{
    switch (atcmd->type) {
        case ATCMD_TYPE_NULL:
            atcmd->args[0] = '\0';
            break;
        case ATCMD_TYPE_GET:
        case ATCMD_TYPE_RUN:
            memcpy(atcmd->args, buffer, buflen);
            break;
    }

    return 0;
}

int8_t command_from_buffer (atcmd_t *atcmd, char *buffer, uint8_t buflen)
{
    // BUFFER MAY NOT BE NULL TERMINATED!!!
    if (buflen == 0) {
        return 1;
    }
    else if (strncmp(buffer, atcmd_command_echo, buflen) == 0) {
        atcmd->command = ATCMD_COMMAND_ECHO;
    }
    else if (strncmp(buffer, atcmd_command_reset, buflen) == 0) {
        atcmd->command = ATCMD_COMMAND_RESET;
    }
    else {
        atcmd->command = ATCMD_COMMAND_NONE;
        return 2;
    }

    return 0;
}

int8_t parse_command_args (atcmd_t *atcmd, char *buffer, uint8_t buflen)
{
    int8_t rc = 0;
    char *substr = 0;
    int8_t pos = 0;

    switch (atcmd->type) {
        case ATCMD_TYPE_NULL:
            // Simply compare the entire buffer up to the null terminator
            rc = command_from_buffer(atcmd, buffer, buflen);
            break;
        case ATCMD_TYPE_GET:
            // Compare buffer up to '?'
            substr = strstr(buffer, "?");
            if (substr == NULL) {
                return 1;
            }
            pos = buffer - substr;
            rc = command_from_buffer(atcmd, buffer, pos);
            rc |= atcmd_parse_args(atcmd, substr+1, buflen - (pos + 1));
            break;
        case ATCMD_TYPE_RUN:
            // Compare buffer up to '='
            substr = strstr(buffer, "=");
            if (substr == 0) {
                return 1;
            }
            pos = buffer - substr;
            rc = command_from_buffer(atcmd, buffer, pos);
            rc |= atcmd_parse_args(atcmd, substr+1, buflen - (pos + 1));
            break;
    }

    return rc;
}

int8_t atcmd_parse_command_args (atcmd_t *atcmd, char *buffer, uint8_t buflen)
{
    int8_t rc = 0;
    
    switch (atcmd->cmd) {
        case ATCMD_CMD_HELLO: // AT
            rc = atcmd_parse_args(atcmd, buffer + 2, buflen - 2);
            break;
        case ATCMD_CMD_COMMAND:
            rc = parse_command_args(atcmd, buffer + 3, buflen - 3);
            break;
        default:
            rc = atcmd_parse_args(atcmd, buffer + 3, buflen - 3);
            break;
    }

    return rc;
}


int8_t atcmd_parse_buffer (atcmd_t *atcmd, char *buffer)
{
    uint8_t i = 0;
    int8_t rc = 0;
    char c = 0;
    uint8_t len = 0;
    
    // Get buffer total length
    len = preparebuffer(buffer);
    if (len < 2) {
        return 1;
    }

    rc = atcmd_parse_header(atcmd, buffer, len);
    if (rc != 0) {
        return 1 << 4 | rc;
    }

    rc = atcmd_parse_type(atcmd, buffer, len);
    if (rc != 0) {
        return 2 << 4 | rc;
    }

    rc = atcmd_parse_args(atcmd, buffer, len);
    if (rc != 0) {
        return 3 << 4 | rc;
    }

    return 0;
}