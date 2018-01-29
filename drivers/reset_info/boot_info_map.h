/**
 * @brief Structure for boot info region at first 256bytes in SRAM
 * @author Ran Bao
 * @date 9/Jan/2018
 * @file boot_info_map.h
 */
#ifndef RESET_INFO_EXCLUSIVE_INCLUDE
#error "You should include reset_info.h instead!"
#endif


typedef struct __attribute__((packed))
{
    reset_info_common_t reset_info_common;

    uint32_t prev_aat_addr;
    uint32_t next_aat_addr;
    uint32_t boot_flags[4];
} boot_info_map_t;
