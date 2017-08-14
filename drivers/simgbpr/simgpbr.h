/**
 * @brief Simulated General Purpose Backup Register (GPBR)
 * @file simgpbr.h
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef SIMGPBR_H_
#define SIMGPBR_H_

#include <stdint.h>
#include <stdbool.h>

#define SIMGPBR_DATA_SIZE 16
#define SIMGPBR_MAGIC_DATA_SIZE 2
#define SIMGPBR_SIZE (SIMGPBR_DATA_SIZE + SIMGPBR_MAGIC_DATA_SIZE)
#define SIMGPBR_WORD_LEN 4

#ifdef __cplusplus
extern "C" {
#endif

void simgpbr_rebuild(void);
bool simgbpr_is_valid(void);
uint32_t simgpbr_get(uint32_t index);
void simgpbr_set(uint32_t index, uint32_t data);

#ifdef __cplusplus
}
#endif

#endif
