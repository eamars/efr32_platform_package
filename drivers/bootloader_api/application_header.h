/**
 * @brief Application Header defintion
 * @file application_header.h
 * @author Ran Bao
 * @date Aug, 2017
 *
 * The application header definition is derived from AAT module by Silicon Labs
 */

#ifndef APPLICATION_HEADER_H_
#define APPLICATION_HEADER_H_

#include <stdint.h>
#include "irq.h"

#define APPLICATION_HEADER_VERSION_1 0x1UL
#define BASIC_HEADER_VERSION 0x0001
#define EXTENDED_HEADER_VERSION 0x00000001UL

#define APP_ADDRESS_TABLE_TYPE          (0x0AA7)
#define BOOTLOADER_ADDRESS_TABLE_TYPE   (0x0BA7)
#define RAMEXE_ADDRESS_TABLE_TYPE       (0x0EA7)

typedef struct __attribute__ ((packed))
{
	uint32_t * stack_top;
	irq_handler_t reset_handler;
	irq_handler_t nmi_handler;
	irq_handler_t hardfault_handler;

	// Application Address Table or Bootloader Address Table
	uint16_t type;

	// Version: first byte: major version, second byte: minor version
	uint16_t version;

	// Address to the full interrupt vector table
	irq_handler_t * vector_table;
} BasicApplicationHeaderTable_t;

typedef struct __attribute__ ((packed))
{
	// Same structure for every application header shared across Silicon Labs product
	BasicApplicationHeaderTable_t basic_application_header_table;

	// Crc32 for basic application header table
	uint32_t basic_app_header_table_crc;

	// Extended application header table version
	uint32_t ext_header_version;

	// Application total size
	uint32_t app_total_size;

	// Header size
	uint32_t header_size;

	// Application version
	union
	{
		uint32_t app_version;

		// TODO: the byte order is important!
		struct __attribute__ ((packed))
		{
			uint8_t app_version_build;
			uint8_t app_version_patch;
			uint8_t app_version_minor;
			uint8_t app_version_major;
		};
	};

	// timestamp for build
	uint32_t timestamp;

} ExtendedApplicationHeaderTableV1_t;

typedef ExtendedApplicationHeaderTableV1_t ExtendedApplicationHeaderTable_t;

#endif /* APPLICATION_HEADER_H_ */
