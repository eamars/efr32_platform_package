/**
 * @file bootloader_api.c
 * @brief Bootloader calls that is used to integrate with other applications
 * @author Ran Bao
 * @date March, 2017
 */

#include <stdbool.h>

#include "em_device.h"

#include "btl_reset.h"
#include "btl_reset_info.h"

#include "bootloader_api.h"
#include "application_header.h"


extern uint32_t __CRASHINFO__begin;
extern uint32_t __AAT__begin;

/**
 * @brief Execute interrupt vector table aligned at specific address
 * @param addr address of target interrupt vector table
 */
__attribute__ ((naked))
static void _binary_exec(void *addr __attribute__((unused)))
{
	__ASM (
		"mov r1, r0\n"			// r0 is the first argument
		"ldr r0, [r1, #4]\n"	// load the address of static interrupt vector with offset 4 (Reset_handler)
		"ldr sp, [r1]\n"		// reset stack pointer
		"blx r0\n"				// branch to Reset_handler
	);
}

static void binary_exec(void *addr)
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
	SCB->VTOR = ((uint32_t) addr & SCB_VTOR_TBLOFF_Msk);

	// barriers
	__DSB();
	__ISB();

	// enable interrupts
	__enable_irq();

	// load stack and pc
	_binary_exec(addr);

	// should never run beyond this point
	while (1)
	{

	}
}

/**
 * Reboot the program to the bootloadfer
 * @param reboot_now if the reboot_now is set, the processor will be reset immediately
 */
void reboot_to_bootloader(bool reboot_now)
{
	// set field in reset info indicates that we want to boot to application
	// map reset cause structure to the begin of crash info memory
	BootloaderResetCause_t * reset_cause = (BootloaderResetCause_t *) &__CRASHINFO__begin;

	reset_cause->signature = BOOTLOADER_RESET_SIGNATURE_VALID;
	reset_cause->reason = BOOTLOADER_RESET_REASON_BOOTLOAD;

	// software reset
	NVIC_SystemReset();
}

void reboot_to_addr(uint32_t app_addr, bool reboot_now)
{
	// set field in reset info indicates that we want to boot to application
	// map reset cause structure to the begin of crash info memory
	ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__CRASHINFO__begin;

	reset_cause->basicResetCause.signature = BOOTLOADER_RESET_SIGNATURE_VALID;
	reset_cause->basicResetCause.reason = BOOTLOADER_RESET_REASON_GO;

	// store the address that we want to run
	reset_cause->app_signature = APP_SIGNATURE;
	reset_cause->app_addr = app_addr;

	if (reboot_now)
	{
		// software reset, SRAM is retained
		NVIC_SystemReset();
	}
}

void branch_to_addr(uint32_t vtor_addr)
{
	binary_exec((void *) vtor_addr);
}


/**
 * @brief As required by ZStack OTA plugins
 */
#include PLATFORM_HEADER
#include "app/framework/include/af.h"
#include "stack/include/ember-types.h"
#include "stack/include/error.h"
#include "hal/hal.h"
#include "hal/micro/cortexm3/common/bootloader-common.h"
#include "hal/micro/cortexm3/common/ebl.h"
#include "hal/micro/cortexm3/memmap.h"

// Common bootloader interface
#include "api/btl_interface.h"

const HalEepromInformationType * halAppBootloaderInfo(void)
{
	return NULL;
}

uint8_t halAppBootloaderInit(void)
{
	return EEPROM_SUCCESS;
}

uint8_t halAppBootloaderWriteRawStorage(uint32_t address, const uint8_t *data, uint16_t len)
{
	emberAfCorePrint("WriteAddr: %ld, Len: %d\r\n", address, len);
	return EEPROM_SUCCESS;
}

bool halAppBootloaderStorageBusy(void)
{
	return false;
}

void halAppBootloaderShutdown(void)
{

}


BlExtendedType halBootloaderGetInstalledType(void)
{
	return BL_EXT_TYPE_NULL;
}

void halGetExtendedBootloaderVersion(uint32_t* getEmberVersion, uint32_t* customerVersion)
{

}

BlBaseType halBootloaderGetType(void)
{
	return BL_TYPE_STANDALONE;
}

uint16_t halGetBootloaderVersion(void)
{
	return BL_PROTO_VER;
}

EmberStatus halAppBootloaderInstallNewImage(void)
{
	while (1)
	{

	}

	return EMBER_ERR_FATAL;
}

void halAppBootloaderImageIsValidReset(void)
{

}

uint16_t halAppBootloaderImageIsValid(void)
{
	return BL_IMAGE_IS_VALID_CONTINUE;
}

uint8_t halAppBootloaderReadRawStorage(uint32_t address, uint8_t *data, uint16_t len)
{
	emberAfCorePrint("ReadAddr: %ld, Len: %d\r\n", address, len);
	return EEPROM_SUCCESS;
}


void emberAfOtaClientVersionInfoCallback(EmberAfOtaImageId* currentImageInfo,
                                         uint16_t* hardwareVersion)
{
	// read version number from application header
	ExtendedApplicationHeaderTable_t * header = (ExtendedApplicationHeaderTable_t *) &__AAT__begin;

	memset(currentImageInfo, 0, sizeof(EmberAfOtaImageId));
	currentImageInfo->manufacturerId  = EMBER_AF_MANUFACTURER_CODE;
	currentImageInfo->imageTypeId     = EMBER_AF_PLUGIN_OTA_CLIENT_POLICY_IMAGE_TYPE_ID;
	currentImageInfo->firmwareVersion = header->app_version;

	if (hardwareVersion != NULL) {
		*hardwareVersion = 0xFFFF;
	}
}
