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
    MFG_TEST_I2C,
    MFG_TEST_IMU,
} mfg_test_entry_list_e;

void mfg_hw_test_reboot_to_enter(uint32_t entry);

#endif // MFG_HW_TEST_H_
