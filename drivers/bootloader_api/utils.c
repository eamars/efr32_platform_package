/**
 * @file query.c
 * @brief Handle query requests
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date May, 2017
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "dscrc8.h"
#include "bootloader_api.h"


/**
 * @brief Read current partition index number @see bootloader_partition_t
 * @param partition output partition index
 * @return is partition index information trustworthy or not
 */
bool read_current_partition_index(bootloader_partition_t *partition)
{
	return true;
}

/**
 * Read version string from partition and parse the string into different parts
 * @param region flash region
 * @param major major version number
 * @param minor minor version number
 * @param patch patch number
 * @param revision number of git commit since last major version
 * @return is version information trustworthy or not
 */
bool read_version_string(bootloader_partition_t partition, uint8_t *major, uint8_t *minor, uint8_t *patch, uint8_t *revision)
{
	return true;
}

bool is_partition_valid(char * partition)
{
	return true;
}
