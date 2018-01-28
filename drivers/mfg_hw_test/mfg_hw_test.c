/**
 * @brief API for manufacture hardware test
 * @file mfg_hw_test.c
 * @date 29/Jan/2018
 * @author Ran Bao
 */

#include "mfg_hw_test.h"
#include "bootloader_api.h"

extern irq_handler_t mfg_test_vector_table[];

void mfg_hw_test_reboot_to_enter(void)
{
    reboot_to_addr((uint32_t) mfg_test_vector_table);
}
