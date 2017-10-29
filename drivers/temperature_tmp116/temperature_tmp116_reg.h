//
// Created by Ran Bao on 30/10/17.
//

#ifndef TEMPERATURE_TMP116_REG_H_H
#define TEMPERATURE_TMP116_REG_H_H

/**
 * @brief Depending on ADD0 configuration, four possible slave address can be configured
 *
 * ADD0 --> GND: 0x0
 * ADD0 --> VCC: 0x1
 * ADD0 --> SDA: 0x2
 * ADD0 --> SCL: 0x3
 */
#define _TMP116_I2C_ADDR_SEL 0x0
#define _TMP116_I2C_ADDR_MASK 0x9
#define _TMP116_I2C_ADDR_SHIFT 3
#define TMP116_I2C_BASE_ADDR (_TMP116_I2C_ADDR_MASK << _TMP116_I2C_ADDR_SHIFT)
#define TMP116_I2C_ADDR (TMP116_I2C_BASE_ADDR | _TMP116_I2C_ADDR_SEL)

/**
 * @brief 16 bit temperature data
 */
#define TMP116_REG_ADDR_TEMPERATURE 0x00
#define _TMP116_REG_TEMPERATURE_SHIFT 0x0
#define _TMP116_REG_TEMPERATURE_MASK (0xFFFF << _TMP116_REG_TEMPERATURE_SHIFT)

/**
 * @brief Configurations
 */
#define TMP116_REG_ADDR_CONFIGURATION 0x01
#define _TMP116_REG_CONFIGURATION_HIGH_ALERT_SHIFT 0xF
#define _TMP116_REG_CONFIGURATION_HIGH_ALERT_MASK (1 << _TMP116_REG_CONFIGURATION_HIGH_ALERT_SHIFT)
#define _TMP116_REG_CONFIGURATION_LOW_ALERT_SHIFT 0xE
#define _TMP116_REG_CONFIGURATION_LOW_ALERT_MASK (1 << _TMP116_REG_CONFIGURATION_LOW_ALERT_SHIFT)
#define _TMP116_REG_CONFIGURATION_DATA_READY_SHIFT 0xD
#define _TMP116_REG_CONFIGURATION_DATA_READY_MASK (1 << _TMP116_REG_CONFIGURATION_DATA_READY_SHIFT)
#define _TMP116_REG_CONFIGURATION_EEPROM_BUSY_SHIFT 0xC
#define _TMP116_REG_CONFIGURATION_MOD_SHIFT 0xA
#define _TMP116_REG_CONFIGURATION_MOD_MASK (3 << _TMP116_REG_CONFIGURATION_MOD0_SHIFT)
#define _TMP116_REG_CONFIGURATION_CONV_SHIFT 0x7
#define _TMP116_REG_CONFIGURATION_CONV_MASK (7 << _TMP116_REG_CONFIGURATION_CONV0_SHIFT)
#define _TMP116_REG_CONFIGURATION_AVG_SHIFT 0x5
#define _TMP116_REG_CONFIGURATION_AVG_MASK (3 << _TMP116_REG_CONFIGURATION_AVG0_SHIFT)
#define _TMP116_REG_CONFIGURATION_TNA_SHIFT 0x4
#define _TMP116_REG_CONFIGURATION_TNA_MASK (1 << _TMP116_REG_CONFIGURATION_TNA_SHIFT)
#define _TMP116_REG_CONFIGURATION_POL_SHIFT 0x3
#define _TMP116_REG_CONFIGURATION_POL_MASK (1 << _TMP116_REG_CONFIGURATION_POL_SHIFT)
#define _TMP116_REG_CONFIGURATION_DRALERT_SHIFT 0x2
#define _TMP116_REG_CONFIGURATION_DRALERT_MASK (1 << _TMP116_REG_CONFIGURATION_DRALERT_SHIFT)

/**
 * @brief Temperathre high threshold
 */
#define TMP116_REG_ADDR_HIGH_LIMIT 0x02
#define _TMP116_REG_ADDR_HIGH_LIMIT_SHIFT 0x0
#define _TMP116_REG_ADDR_HIGH_LIMIT_MASK (0xFFFF << _TMP116_REG_ADDR_HIGH_LIMIT_SHIFT)

/**
 * @brief Temperature low threshold
 */
#define TMP116_REG_ADDR_LOW_LIMIT 0x03
#define _TMP116_REG_ADDR_LOW_LIMIT_SHIFT 0x0
#define _TMP116_REG_ADDR_LOW_LIMIT_MASK (0xFFFF << _TMP116_REG_ADDR_LOW_LIMIT_SHIFT)

/**
 * @brief EEPROM unlock register
 */
#define TMP116_REG_ADDR_EEPROM_UNLOCK 0x04
#define _TMP116_REG_EEPROM_UNLOCK_EUN_SHIFT 0xF
#define _TMP116_REG_EEPROM_UNLOCK_EUN_MASK (1 << _TMP116_REG_EEPROM_UNLOCK_EUN_SHIFT)
#define _TMP116_REG_EEPROM_UNLOCK_EEPROM_BUSY_SHIFT 0xE
#define _TMP116_REG_EEPROM_UNLOCK_EEPROM_BUSY_MASK (1 << _TMP116_REG_EEPROM_UNLOCK_EEPROM_BUSY_SHIFT)

/**
 * @brief Internal EEPROM
 */
#define TMP116_REG_ADDR_EEPROM1 0x05
#define TMP116_REG_ADDR_EEPROM2 0x06
#define TMP116_REG_ADDR_EEPROM3 0x07
#define TMP116_REG_ADDR_EEPROM4 0x08

/**
 * @brief Device ID
 */
#define TMP116_REG_ADDR_DEVICE_ID 0x0F
#define TMP116_REG_DEVICE_ID_MASK 0x0FFF
#define TMP116_REG_DEVICE_ID_SHIFT 0x0

#endif // TEMPERATURE_TMP116_REG_H_H
