#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include <stdbool.h>
#include <stdint.h>
#include "stack32.h"

#define BL_PROTO_VER 0x0002

typedef enum
{
	FW_HEADER_OFFSET = 0x0,
	FW_VERSION_STRING_OFFSET = 0x20,
	FW_CODE_OFFSET = 0x40,
} fw_addresses_enum;

typedef enum
{
	FW_TOTAL_LENGTH_OFFSET = 0x0,
	FW_HEADER_LENGTH_OFFSET = 0x4,
	FW_CRC_8BIT_OFFSET = 0x5,
	FW_PARTITION_INDEX_OFFSET = 0x6,
	FW_MD5HASH_OFFSET = 0x10
} fw_header_addresses_enum;

typedef enum
{
	REGION_BOOTLOADER = 0x2,
	REGION_RESERVED = 0x3,
	REGION_A = 0x0,
	REGION_B = 0x1,
	REGION_PREV = 0xff,
	REGION_ERROR = 0xff,
} bootloader_partition_t;

/**
 * @brief Definition for boot to flag
 *
 * Note: bootloader_boot_to_flags_t is used exclusively by bootloader to determine default boot partition
 */
typedef enum
{
	BOOT_TO_DEFAULT = 0x0,
	BOOT_TO_BOOTLOADER = 0x1,
	BOOT_TO_REGION_A = 0x2,
	BOOT_TO_REGION_B = 0x3,
} bootloader_boot_to_flags_t;

typedef enum
{
	INST_SET_REGION = 0x0,              /* deprecated */
	INST_BRANCH_TO_REGION = 0x1,        /* deprecated */
	INST_COPY_REGION = 0x2,             /* deprecated */
	INST_FLUSH = 0x3,                   /* deprecated */

	/* Newly added */
	INST_SET_BASE_ADDR = 0x4,
	INST_BRANCH_TO_ADDR = 0x5,
	INST_PUSH_BASE_ADDR = 0x6,
	INST_POP_BASE_ADDR = 0x7,
	INST_ERASE_PAGES = 0x8,
	INST_ERASE_RANGE = 0x9,
	INST_QUERY_PROTO_VER = 0xa,
	INST_GET_BASE_ADDR = 0xb,

	INST_REBOOT = 0xd,
	INST_QUERY_DEVICE_INFO = 0xe,
	INST_QUERY_CHIP_INFO = 0xf
} bootloader_instruction_t;

typedef struct
{
	bootloader_boot_to_flags_t flash_fs_boot_code;
} bootloader_persist_storage;

typedef enum
{
	BOOTLOADER_WAIT_FOR_CONNECTIONS,
	BOOTLOADER_WAIT_FOR_BYTES
} bootloader_mode_t;

typedef struct
{
	// base address for flash (absolute base address for flash region)
	uint32_t base_addr;

	// current bootloader mode
	bootloader_mode_t bootloader_mode;

	// block size exponential (2^x)
	uint8_t block_size_exp;

	// address stack
	stack_t base_addr_stack;

}  bootloader_config_t;


#ifdef __cplusplus
extern "C" {
#endif

void reboot_to_bootloader(bool reboot_now);
void reboot_to_region(bootloader_partition_t region, bool reboot_now);

void reboot_to_addr(uint32_t vtor_addr, bool reboot_now);

/**
 * Read version string from partition and parse the string into different parts
 * @param region flash region
 * @param major major version number
 * @param minor minor version number
 * @param patch patch number
 * @param revision number of git commit since last major version
 * @return is version information trustworthy or not
 */
bool read_version_string(bootloader_partition_t partition, uint8_t *major, uint8_t *minor, uint8_t *patch, uint8_t *revision);

/**
 * @brief Read current partition index number @see bootloader_partition_t
 * @param partition output partition index
 * @return is partition index information trustworthy or not
 */
bool read_current_partition_index(bootloader_partition_t *partition);

/**
 * @brief Verify if the header is trustworthy
 * @param partition partition address
 * @return true or false
 */
bool is_partition_valid(char * partition);

#ifdef __cplusplus
}
#endif

#endif
