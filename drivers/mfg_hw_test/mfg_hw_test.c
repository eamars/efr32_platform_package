/**
 * @brief API for manufacture hardware test
 * @file mfg_hw_test.c
 * @date 29/Jan/2018
 * @author Ran Bao
 */
#include "reset_info.h"
#include "mfg_hw_test.h"
#include "bootloader_api.h"

extern irq_handler_t mfg_test_vector_table[];

extern uint32_t __DYNAMIC_VTOR__begin;
extern uint32_t __DYNAMIC_VTOR__end;

void mfg_hw_test_reboot_to_enter(uint32_t entry)
{
    // use first boot flag to indicate the entry
    boot_info_map_t * boot_info_map = (boot_info_map_t *) reset_info_read();
    boot_info_map->boot_flags[0] = entry;

    // reboot to mfg vector table
    reboot_to_addr((uint32_t) mfg_test_vector_table);
}


void mfg_hw_test_irq_register(IRQn_Type irq, void * handler, bool enable)
{
    int32_t irq_id = irq + 16;

    // map the dynamic vector table
    irq_handler_t * dynamic_vectors = (irq_handler_t *) &__DYNAMIC_VTOR__begin;

    // disable the interrupt before entering
    NVIC_DisableIRQ(irq);

    // clear any pending ones
    NVIC_ClearPendingIRQ(irq);

    // replace the entry
    dynamic_vectors[irq_id] = handler;

    if (enable)
    {
        // enable the interrupt
        NVIC_EnableIRQ(irq);
    }
}
