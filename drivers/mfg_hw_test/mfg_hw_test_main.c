/**
 * @brief Manufacture Hardware Test implementation
 * @file mfg_hw_test_menu.c
 * @date 29/Jan/2018
 * @author Ran Bao
 */
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


void mfg_main(uint32_t * boot_flags)
{
    // clear the reset info
    reset_info_clear();

    uint32_t mfg_entry = boot_flags[0];
    switch (mfg_entry)
    {
        case 0:
        {
            // this is the first default entry for mfg mode.
            // to jump to next mfg mode, we can call with api
            mfg_hw_test_reboot_to_enter(mfg_entry + 1);

            // make sure the code will never go though this point
            break;
        }
        case 127:
        {
            // say 127 is the imu temperature coefficient calibration
            // do something
            // write to eeprom
            // reset
            break;
        }

        default:
            break;
    }

    while (1)
    {

    }
}

