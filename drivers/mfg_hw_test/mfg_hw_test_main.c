/**
 * @brief Manufacture Hardware Test implementation
 * @file mfg_hw_test_menu.c
 * @date 29/Jan/2018
 * @author Ran Bao
 */
#include <string.h>
#include "em_device.h"
#include "reset_info.h"
#include "mfg_hw_test.h"

void mfg_generic_irq_handler(uint32_t ipsr)
{
    IRQn_Type irqno = (IRQn_Type) ((ipsr & 0x1ff) - 16);

    while (1)
    {

    }
}

void mfg_hw_test_wdog(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_dcdc(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_hfxo(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_lfxo(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_gpio(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_led(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_button(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_uart(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_i2c(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_spi(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_imu(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_temp_sensor(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_on_board_radio(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));
void mfg_hw_test_off_board_radio(uint32_t boot_flags[4]) __attribute__((weak, alias("mfg_hw_test_default")));


void mfg_main(uint32_t * boot_flags_ptr)
{
    // keep a copy of boot flags
    {
        uint32_t boot_flags[4];
        memcpy(&boot_flags, boot_flags_ptr, sizeof(boot_flags));

        // clear the reset info
        reset_info_clear();

        // restore the boot flag
        memcpy(boot_flags_ptr, boot_flags, sizeof(boot_flags));
    }

    uint32_t mfg_entry = boot_flags_ptr[0];

    switch (mfg_entry)
    {
        case MFG_TEST_ALL:
        {
            // this is the first default entry for mfg mode.
            // to jump to next mfg mode, we can call with api
            boot_flags_ptr[2] = 0;
            boot_flags_ptr[3] = 0;

            mfg_hw_test_reboot_to_enter(mfg_entry + 1, true);

            // make sure the code will never go though this point
            break;
        }
        case MFG_TEST_WDOG:
        {
            mfg_hw_test_wdog(boot_flags_ptr);
            break;
        }
        case MFG_TEST_DCDC:
        {
            mfg_hw_test_dcdc(boot_flags_ptr);
            break;
        }
        case MFG_TEST_HFXO:
        {
            mfg_hw_test_hfxo(boot_flags_ptr);
            break;
        }
        case MFG_TEST_LFXO:
        {
            mfg_hw_test_lfxo(boot_flags_ptr);
            break;
        }
        case MFG_TEST_GPIO:
        {
            mfg_hw_test_gpio(boot_flags_ptr);
            break;
        }
        case MFG_TEST_LED:
        {
            mfg_hw_test_led(boot_flags_ptr);
            break;
        }
        case MFG_TEST_BUTTON:
        {
            mfg_hw_test_button(boot_flags_ptr);
            break;
        }
        case MFG_TEST_UART:
        {
            mfg_hw_test_uart(boot_flags_ptr);
            break;
        }
        case MFG_TEST_I2C:
        {
            mfg_hw_test_i2c(boot_flags_ptr);
            break;
        }
        case MFG_TEST_SPI:
        {
            mfg_hw_test_spi(boot_flags_ptr);
            break;
        }
        case MFG_TEST_IMU:
        {
            mfg_hw_test_imu(boot_flags_ptr);
            break;
        }
        case MFG_TEST_TEMP_SENSOR:
        {
            mfg_hw_test_temp_sensor(boot_flags_ptr);
            break;
        }
        case MFG_TEST_ON_BOARD_RADIO:
        {
            mfg_hw_test_on_board_radio(boot_flags_ptr);
            break;
        }
        case MFG_TEST_OFF_BOARD_RADIO:
        {
            mfg_hw_test_off_board_radio(boot_flags_ptr);
            break;
        }
        case MFG_TEST_OK:
        {
            while (1)
            {

            }
            break;
        }
        case MFG_TEST_ERROR:
        {
            while (1)
            {

            }
            break;
        }
        default:
        {
            mfg_hw_test_reboot_to_enter(MFG_TEST_OK, false);
            break;
        }
    }


    while (1)
    {

    }
}

void mfg_hw_test_default(uint32_t boot_flags[4])
{
    uint32_t mfg_entry = boot_flags[0];

    mfg_hw_test_reboot_to_enter(mfg_entry + 1, true);
}
