/**
 * @file bootloader.c
 * @brief Bootloader calls that is used to integrate with other applications
 * @author Ran Bao
 * @date March, 2017
 */
#include <stdbool.h>
#include "bootloader_api.h"

/**
 * Reboot the program to the bootloadfer
 * @param reboot_now if the reboot_now is set, the processor will be reset immediately
 */
void reboot_to_bootloader(bool reboot_now)
{

}

/**
 * Reboot to any given region on FLASH
 * @param region @see bootloader.h
 */
void reboot_to_region(bootloader_partition_t partition, bool reboot_now)
{

}

void reboot_to_addr(uint32_t vtor_addr, bool reboot_now)
{

}
