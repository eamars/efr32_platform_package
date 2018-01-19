/**
 *
 *  @file   wg_data_v1.h
 *  @brief  Protocol for communicating with WirelessGuard Hatch Outdoors
 *  @author David Barclay
 *  @date   January 2018
 *
 */


#ifndef _WG_DATA_V1_H
#define _WG_DATA_V1_H



#define WG_DATA_V1_PROTOCOL_VERSION 1

typedef enum {
    WG_DATA_PACKET_BASIC_REPORT = 0,   // Simple periodic data report (closed/temp/batlevel)
    WG_DATA_PACKET_EXTENDED_REPORT,    // More data than a basic DATA_REPORT
    WG_DATA_PACKET_DATA_REQUEST,       // Request a report packet from the device. 
    WG_DATA_PACKET_RESET_COMMAND,      // Reset the device according to parameters
    WG_DATA_PACKET_RESET_ACKNOWLEDGE,  // Follows a RESET_COMMAND before device reset
    WG_DATA_PACKET_CALIBRATE_COMMAND,  // Calibrate device sensors/state
    WG_DATA_PACKET_CALIBRATE_REPORT,   // Calibration data following a CALIBRATE_COMMAND
    WG_DATA_PACKET_SETTINGS_COMMAND,   // Configure device settings, e.g. report period/type
    WG_DATA_PACKET_SETTINGS_REPORT,    // Follows a SETTINGS_COMMAND, or settings request
    WG_DATA_PACKET_DEBUG_REPORT = 255,  // Extended boot debug message. Misc data.
} wg_data_v1_packet_type_t;

typedef struct {
    uint8_t protocol_version;
    uint8_t packet_type;
} wg_data_v1_packet_header_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t closed;
    int8_t temp;        // degrees celsius, -128,127
    uint8_t battery;    // scaled voltage
} wg_data_v1_basic_report_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t closed;
    int8_t temp;        // -128,127
    uint8_t battery;    // scaled voltage
    uint16_t voltage;   // Raw ADC reading (mV)
    /* ADD MORE DATA HERE */
} wg_data_v1_extended_report_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t request_type;   // Select report packet to request
} wg_data_v1_data_request_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    //TODO add precise calibration settings
    uint8_t calibration_mode;   // Select calibration mode
} wg_data_v1_calibrate_command_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    //TODO add calibration data to this struct __attribute__ ((packed)).
    uint8_t errors;
} wg_data_v1_calibrate_report_t;

typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    //TODO add device configuration fields
    int8_t tx_power;
} wg_data_v1_settings_command_t;


// Settings report  hould match settings command
typedef wg_data_v1_settings_command_t wg_data_v1_settings_report_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t reset_type;     // Select which reset to use (soft, hard, factory, etc.)
} wg_data_v1_reset_command_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    // Empty struct. Zero bytes
} wg_data_v1_reset_acknowledge_t;


typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    //TODO Add debug data here
} wg_data_v1_debug_report_t;



#endif // _WG_DATA_V1_H
