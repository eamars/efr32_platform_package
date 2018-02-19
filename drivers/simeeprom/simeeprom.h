/**
 * @file simeeprom.h
 * @author Ran Bao
 * @date 15/Jan/2018
 * @brief Simulated EEPROM
 */

#ifndef SIM_EEPROM_H_
#define SIM_EEPROM_H_

/**
 * @brief Define necessary types to make compiler to shutup
 */
#define DEFINETYPES
typedef uint16_t EmberNodeId;
typedef uint8_t EmberEUI64[8];
#include "stack/config/token-stack.h"
#undef DEFINETYPES

// start defining TOKENs
#define DEFINETOKENS
#define TOKEN_MFG(name, creator, iscnt, isidx, type, arraysize, ...)

/**
 * @brief Define TOKEN_COUNT
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  TOKEN_##name,
enum {
#include "stack/config/token-stack.h"
    TOKEN_COUNT
};
#undef TOKEN_DEF

/**
 * @brief Define TOKEN_SIZE
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  TOKEN_##name##_SIZE = sizeof(type),
enum {
#include "stack/config/token-stack.h"
};
#undef TOKEN_DEF


#undef TOKEN_MFG
#undef DEFINETOKENS
// finish defining TOKENs

typedef struct
{

} simeeprom_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize internal Simulated EEPROM
 */
void simeeprom_init(simeeprom_t * obj);

/**
 * @brief Read data from EEPROM
 * @param obj SimEEPROM object
 * @param token token id
 * @param data pointer to the data
 * @param len the length of data in bytes
 *
 * Note: the data you read might not be valid! You need some way to validate the retrieved data.
 */
void simeeprom_read(simeeprom_t * obj, uint8_t token, void *data, uint8_t len);

/**
 * @brief Fetch data pointer from EEPROM
 * @param obj SimEEPROM object
 * @param token token id
 * @param len the length of data in bytes
 * @return pointer to the data
 *
 * Note: the return value is READ ONLY!
 */
const void * simeeprom_read_ptr(simeeprom_t * obj, uint8_t token, uint8_t len);

/**
 * @brief Write data to EEPROM
 * @param obj SimEEPROM object
 * @param token token id
 * @param data pointer to the data
 * @param len the length of data in bytes
 */
void simeeprom_write(simeeprom_t * obj, uint8_t token, void *data, uint8_t len);

/**
 * @brief Increase the value for a self-increased counter
 * @param obj SimEEPROM object
 * @param token token id
 */
void simeeprom_self_increase(simeeprom_t * obj, uint8_t token);

/**
 * @brief Maintain the Simulated EEPROM page.
 * This function should be call periodically. In the worst case calling this function will result in interrupt unable
 * to serve for 21ms. (refer to the value specified in AN703)
 */
void simeeprom_bookkeeping(simeeprom_t * obj);

/**
 * @brief Erase entire Simulated EEPROM page
 */
void simeeprom_mass_erase(void);

#ifdef __cplusplus
}
#endif

#endif // SIM_EEPROM_H_
