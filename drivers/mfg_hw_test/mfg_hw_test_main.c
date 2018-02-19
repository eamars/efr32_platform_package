/**
 * @brief Manufacture Hardware Test implementation
 * @file mfg_hw_test_menu.c
 * @date 29/Jan/2018
 * @author Ran Bao
 */
#include "em_device.h"
#include "reset_info.h"
#include "mfg_hw_test.h"

extern void mfg_imu_test(uint32_t * boot_flags);

void mfg_generic_irq_handler(uint32_t ipsr)
{
    IRQn_Type irqno = (IRQn_Type) ((ipsr & 0x1ff) - 16);

    while (1)
    {

    }
}


void mfg_main(uint32_t * boot_flags)
{
    uint32_t mfg_entry = boot_flags[0];

    // clear the reset info
    reset_info_clear();

    switch (mfg_entry)
    {
        case MFG_TEST_ALL:
        {
            // this is the first default entry for mfg mode.
            // to jump to next mfg mode, we can call with api
            mfg_hw_test_reboot_to_enter(mfg_entry + 1);

            // make sure the code will never go though this point
            break;
        }
        case MFG_TEST_I2C:
        {
            mfg_hw_test_reboot_to_enter(mfg_entry + 1);
            break;
        }
        case MFG_TEST_IMU:
        {
            mfg_imu_test(boot_flags);
            mfg_hw_test_reboot_to_enter(mfg_entry + 1);
            break;
        }

        default:
        {
            break;
        }
    }


    while (1)
    {

    }
}

