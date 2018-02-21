/**
 * Manufacture and unit test for i2c module
 */

#include "i2cdrv.h"
#include "mfg_hw_test.h"

void mfg_hw_test_i2c(uint32_t boot_flags[4])
{
    uint32_t mfg_entry = boot_flags[0];
    bool continueous = (bool) boot_flags[1];

    if (!continueous)
    {
        boot_flags[2] = 0;
        boot_flags[3] = 0;
    }

    // test goes here


    if (continueous)
        mfg_hw_test_reboot_to_enter(mfg_entry + 1, true);
    else
        mfg_hw_test_reboot_to_enter(MFG_TEST_OK, false);
}
