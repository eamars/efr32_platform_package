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
	INST_SET_BASE_ADDR = 0x0,
	INST_GET_BASE_ADDR = 0x1,
	INST_PUSH_BASE_ADDR = 0x2,
	INST_POP_BASE_ADDR = 0x3,

	INST_SET_BLOCK_EXP = 0x4,
	INST_GET_BLOCK_EXP = 0x5,

	INST_BRANCH_TO_ADDR = 0x6,

	INST_ERASE_PAGES = 0x7,
	INST_ERASE_RANGE = 0x8,

	INST_REBOOT = 0x9,

	INST_QUERY_PROTO_VER = 0xfc,
	INST_QUERY_DEVICE_INFO = 0xfd,
	INST_QUERY_CHIP_INFO = 0xfe,

	INST_INVALID = 0xff
} bootloader_instruction_t;


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
void reboot_to_addr(uint32_t vtor_addr, bool reboot_now);

void branch_to_addr(uint32_t vtor_addr);


#ifdef __cplusplus
}
#endif

#endif
