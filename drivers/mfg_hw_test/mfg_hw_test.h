/**
 * @brief API for manufacture hardware test
 * @file mfg_hw_test.h
 * @date 29/Jan/2018
 * @author Ran Bao
 */

#ifndef MFG_HW_TEST_H_
#define MFG_HW_TEST_H_

#include <stdint.h>
#include "application_header.h"

typedef enum
{
    MFG_TEST_ALL = 0,
    MFG_TEST_WDOG,
    MFG_TEST_DCDC,
    MFG_TEST_HFXO,
    MFG_TEST_LFXO,
    MFG_TEST_GPIO,
    MFG_TEST_LED,
    MFG_TEST_BUTTON,
    MFG_TEST_UART,
    MFG_TEST_I2C,
    MFG_TEST_SPI,
    MFG_TEST_IMU,
    MFG_TEST_TEMP_SENSOR,
    MFG_TEST_ON_BOARD_RADIO,
    MFG_TEST_OFF_BOARD_RADIO,

    MFG_TEST_OK = 0xFE,
    MFG_TEST_ERROR = 0xFF,
} mfg_test_entry_list_e;

// MFG API
void mfg_hw_test_reboot_to_enter(uint32_t entry, bool contineous);
void mfg_hw_test_irq_register(IRQn_Type irq, void * handler, bool enable);

// MFG Test Entries
void mfg_hw_test_default(uint32_t boot_flags[4]);
void mfg_hw_test_wdog(uint32_t boot_flags[4]);
void mfg_hw_test_dcdc(uint32_t boot_flags[4]);
void mfg_hw_test_hfxo(uint32_t boot_flags[4]);
void mfg_hw_test_lfxo(uint32_t boot_flags[4]);
void mfg_hw_test_gpio(uint32_t boot_flags[4]);
void mfg_hw_test_led(uint32_t boot_flags[4]);
void mfg_hw_test_button(uint32_t boot_flags[4]);
void mfg_hw_test_uart(uint32_t boot_flags[4]);
void mfg_hw_test_i2c(uint32_t boot_flags[4]);
void mfg_hw_test_spi(uint32_t boot_flags[4]);
void mfg_hw_test_imu(uint32_t boot_flags[4]);
void mfg_hw_test_temp_sensor(uint32_t boot_flags[4]);
void mfg_hw_test_on_board_radio(uint32_t boot_flags[4]);
void mfg_hw_test_off_board_radio(uint32_t boot_flags[4]);


#endif // MFG_HW_TEST_H_
