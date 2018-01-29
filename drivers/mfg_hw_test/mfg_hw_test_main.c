/**
 * @brief Manufacture Hardware Test implementation
 * @file mfg_hw_test_menu.c
 * @date 29/Jan/2018
 * @author Ran Bao
 */
#include "em_device.h"
#include "reset_info.h"


void mfg_generic_irq_handler(uint32_t ipsr)
{
    IRQn_Type irqno = (IRQn_Type) ((ipsr & 0x1ff) - 16);

    while (1)
    {

    }
}

void hw_test_0(void)
{

}

void hw_test_1(void)
{

}

void mfg_main(void)
{
    // clear the reset info
    reset_info_clear();

    // trigger a divide by 0 error
//    uint32_t a = 5/0;

    while (1)
    {

    }
}

