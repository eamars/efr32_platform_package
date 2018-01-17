/**
 * @file simeeprom.h
 * @author Ran Bao
 * @date 15/Jan/2018
 * @brief Simulated EEPROM
 */

#ifndef SIM_EEPROM_H_
#define SIM_EEPROM_H_

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
 * @param id token id
 * @param data pointer to the data
 * @param len the length of data in bytes
 *
 * Note: the data you read might not be valid! You need some way to validate the retrieved data.
 */
void simeeprom_read(simeeprom_t * obj, uint8_t id, void *data, uint8_t len);

/**
 * @brief Write data to EEPROM
 * @param id token id
 * @param data pointer to the data
 * @param len the length of data in bytes
 */
void simeeprom_write(simeeprom_t * obj, uint8_t id, void *data, uint8_t len);

/**
 * @brief Maintain the Simulated EEPROM page.
 * This function should be call periodically. In the worst case calling this function will result in interrupt unable
 * to serve for 21ms. (refer to the value specified in AN703)
 */
void simeeprom_bookkeeping(simeeprom_t * obj);

#ifdef __cplusplus
}
#endif

#endif // SIM_EEPROM_H_
