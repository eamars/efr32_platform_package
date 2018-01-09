/**
 * @file bootloader_api.c
 * @brief Bootloader calls that is used to integrate with other applications
 * @author Ran Bao
 * @date March, 2017
 */

#include PLATFORM_HEADER
#include <stdbool.h>

#include "em_device.h"

#include "reset_info.h"
#include "drv_debug.h"
#include "bootloader_api.h"
#include "application_header.h"

/**
 * @brief Execute interrupt vector table aligned at specific address
 * @param addr address of target interrupt vector table
 */
__attribute__ ((naked))
static void _binary_exec(void * r0 __attribute__((unused)))
{
    __ASM (
        "mov r1, r0\n"			// r0 is the first argument
        "ldr r0, [r1, #4]\n"	// load the address of static interrupt vector with offset 4 (Reset_handler)
        "ldr sp, [r1]\n"		// reset stack pointer
        "blx r0\n"				// branch to Reset_handler
    );
}

static void binary_exec(void * vtor_addr)
{
    // disable global interrupts
    __disable_irq();

    // disable irqs
    for (register uint32_t i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
    }
    for (register uint32_t i = 0; i < 8; i++)
    {
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // flush registers and memories
    __DSB();	// data memory barrier @see __DSB()
    __ISB();	// instruction memory barrier @__ISB()

    // change vector table
    SCB->VTOR = ((uint32_t) vtor_addr & SCB_VTOR_TBLOFF_Msk);

    // barriers
    __DSB();
    __ISB();

    // enable interrupts
    __enable_irq();

    // load stack and pc
    _binary_exec(vtor_addr);

    // should never run beyond this point
    while (1)
    {

    }
}

/**
 * Reboot the program to the bootloadfer
 * @param reboot_now if the reboot_now is set, the processor will be reset immediately
 */
void reboot_to_bootloader(void)
{
    boot_info_map_t * boot_info_map = (boot_info_map_t *) reset_info_read();

    // set current current application address
    boot_info_map->prev_aat_addr = (uint32_t) &__AAT__begin;

    // reset using debug interface
    system_reset(RESET_BOOTLOADER_BOOTLOAD);
}

void reboot_to_addr(uint32_t vtor_addr)
{
    boot_info_map_t * boot_info_map = (boot_info_map_t *) reset_info_read();

    // set current application address and next application address
    boot_info_map->prev_aat_addr = (uint32_t) &__AAT__begin;
    boot_info_map->next_aat_addr = vtor_addr;

    // reset using debug interface
    system_reset(RESET_BOOTLOADER_GO);
}

void branch_to_addr(uint32_t vtor_addr)
{
    binary_exec((void *) vtor_addr);
}

