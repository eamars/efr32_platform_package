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

#include <stdint.h>
#include <stdbool.h>

#define WG_DATA_V1_PROTOCOL_VERSION 1

#define WG_DATA_RESET_TYPE_SOFT 0
#define WG_DATA_RESET_TYPE_HARD 1


typedef enum {
    WG_DATA_PACKET_BASIC_REPORT = 0,    // Simple data report (closed/temp/batlevel)
    WG_DATA_PACKET_EXTENDED_REPORT,     // More data than a basic BASIC_REPORT
    WG_DATA_PACKET_DATA_REQUEST,        // Request a report packet from the device.
    WG_DATA_PACKET_RESET_COMMAND,       // Reset the device according to parameters
    WG_DATA_PACKET_RESET_ACKNOWLEDGE,   // Follows a RESET_COMMAND before device reset
    WG_DATA_PACKET_CALIBRATE_COMMAND,   // Calibrate device sensors/state
    WG_DATA_PACKET_CALIBRATE_REPORT,    // Calibration data following a CALIBRATE_COMMAND
    WG_DATA_PACKET_SETTINGS_COMMAND,    // Configure device settings, e.g. WG_DATA_SETTING_ID_REPORT_CONFIG
    WG_DATA_PACKET_SETTINGS_REPORT,     // Follows a SETTINGS_COMMAND, or settings request
    WG_DATA_PACKET_SETTINGS_REQUEST,    // Request settings from the device
    WG_DATA_PACKET_DEBUG_REPORT = 255,  // Extended boot debug message. Misc data.
} wg_data_v1_packet_type_t;

typedef enum {
    WG_DATA_CALIBRATE_IMU_VECTOR = 0,
    WG_DATA_CALIBRATE_IMU_TEMP = 1,
} wg_data_v1_calibrate_type_t;

typedef enum {
    WG_DATA_SETTING_ID_REPORT_CONFIG    = 0,  // Config for data reports (type,period,etc.)
    WG_DATA_SETTING_ID_LED_MODE         = 1,  // LED configuration (on,off,radio,etc.)
    WG_DATA_SETTING_ID_IMU_TEMP_COEFF   = 2,  // Config for IMU temperature coefficients (float)(x,y,z)
    WG_DATA_SETTING_ID_RADIO_CONFIG     = 3,  // Config for the radio driver (txpwr,freq,etc.)
} wg_data_v1_setting_id_t;

typedef struct __attribute__ ((packed)) {
    uint8_t report_type;            // Which report type to generate (basic, extended)
    uint8_t report_fastmode;        // Whether to report fast or slow
    uint32_t report_period_slow;    // Time between reports (seconds), minimum 5s
    uint32_t report_period_fast;    // Time between fast mode reports (seconds)
} wg_data_v1_setting_report_config_t;

typedef struct __attribute__ ((packed)) {
    uint8_t led_mode;       // Which mode to configure the LED flashing
    uint32_t led_period;    // Period for LED flashing (milliseconds)
} wg_data_v1_setting_led_mode_t;

typedef struct __attribute__ ((packed)) {
    float x_temp_coeff;
    float y_temp_coeff;
    float z_temp_coeff;
} wg_data_v1_setting_imu_temp_coeff_t;

typedef struct __attribute__ ((packed)) {
    uint8_t radio_txpower;  // transmit power for radio
} wg_data_v1_setting_radio_config_t;

typedef struct __attribute__ ((packed)) {
    uint8_t protocol_version;
    uint8_t packet_type;
} wg_data_v1_packet_header_t;


// WG_DATA_PACKET_BASIC_REPORT - Simple data report (closed/temp/batlevel)
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t closed;
    int8_t temp;        // degrees celsius, -128,127
    uint8_t battery;    // scaled voltage
} wg_data_v1_basic_report_t;


// WG_DATA_PACKET_EXTENDED_REPORT - More data than a BASIC_REPORT
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t closed;
    int8_t temp;        // -128,127
    uint8_t battery;    // scaled voltage
    uint16_t voltage;   // Raw ADC reading (mV)
    int16_t mag_raw_x;  // Raw magnetometer X value
    int16_t mag_raw_y;  // Raw magnetometer X value
    int16_t mag_raw_z;  // Raw magnetometer Z value
    uint32_t vector;    // IMU calculated vector

    struct __attribute__ ((packed))
    {
        int16_t x_origin;
        int16_t y_origin;
        int16_t z_origin;

        int16_t x_origin_compensated;
        int16_t y_origin_compensated;
        int16_t z_origin_compensated;

        uint32_t vector;
        uint16_t vector_threshold_closed;
        uint16_t vector_threshold_open;
        int8_t calibration_temp;
        float x_tmp_coef;
        float y_tmp_coef;
        float z_tmp_coef;
        /* ADD MORE DATA HERE */
    } origin;

    /* ADD MORE DATA HERE */
} wg_data_v1_extended_report_t;


// WG_DATA_PACKET_DATA_REQUEST - Request a report packet from the device.
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t request_type;   // Select report packet to request
} wg_data_v1_data_request_t;

// WG_DATA_PACKET_RESET_COMMAND - Reset the device according to parameters
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t reset_type;     // Select which reset to use (soft, hard, factory, etc.)
} wg_data_v1_reset_command_t;

// WG_DATA_PACKET_RESET_ACKNOWLEDGE - Follows a RESET_COMMAND before device reset
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    // Empty struct. Zero bytes
} wg_data_v1_reset_acknowledge_t;

// WG_DATA_PACKET_CALIBRATE_COMMAND - Calibrate device sensors/state
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    //TODO add precise calibration settings
    uint8_t calibration_mode;   // Select calibration mode
} wg_data_v1_calibrate_command_t;

// WG_DATA_PACKET_CALIBRATE_REPORT - Calibration data following a CALIBRATE_COMMAND
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    //TODO add calibration data to this struct __attribute__ ((packed)).
    uint8_t errors;
} wg_data_v1_calibrate_report_t;

// WG_DATA_PACKET_SETTINGS_COMMAND - Configure device settings, e.g. WG_DATA_SETTING_ID_REPORT_CONFIG
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t settings_id;    // ID of setting that we are configuring
    uint8_t payload[16];    // Values to use for setting (maximum 16 bytes)
} wg_data_v1_settings_command_t;

// WG_DATA_PACKET_SETTINGS_REPORT - Follows a SETTINGS_COMMAND, or SETTINGS_REQUEST
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    uint8_t settings_id;    // ID of setting that we are reporting
    uint8_t payload[16];    // Current values for setting (maximum 16 bytes)
} wg_data_v1_settings_report_t;

// WG_DATA_PACKET_SETTINGS_REQUEST - Request settings from the device
typedef struct wg_data_v1_settings_request_t {
    wg_data_v1_packet_header_t header;
    uint8_t settings_id;        // ID of setting that we are requesting
} wg_data_v1_settings_request_t;

// WG_DATA_PACKET_DEBUG_REPORT - Extended boot debug message. Misc data.
typedef struct __attribute__ ((packed)) {
    wg_data_v1_packet_header_t header;
    //TODO Add debug data here
    uint16_t reset_reason;
    uint32_t rmu_reset_reason;
    struct __attribute__((packed))
    {
        char assert_filename[32];
        uint32_t assert_line;
    } assert_info;
} wg_data_v1_debug_report_t;



#endif // _WG_DATA_V1_H
