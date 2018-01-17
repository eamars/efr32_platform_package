/**
 * @file simeeprom.c
 * @author Ran Bao
 * @date 15/Jan/2018
 * @brief Simulated EEPROM
 *
 * @see sim-eeprom.c in plugin folder for more information
 */
#include PLATFORM_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "drv_debug.h"
#include "simeeprom.h"
#include "sim-eeprom.h" // << silabs' code

#define BYTES_TO_WORDS(x) (((x) + 1) / 2)
#define COUNTER_TOKEN_PAD        50

#ifndef EMBER_CHILD_TABLE_SIZE
#define EMBER_CHILD_TABLE_SIZE                    0
#endif
#define EMBER_CHILD_TABLE_TOKEN_SIZE                EMBER_CHILD_TABLE_SIZE
#define SIMEE_SIZE_B        8192
#define SIMEE_BTS_SIZE_B    2104

#define DEFINETYPES
typedef uint16_t EmberNodeId;
typedef uint8_t EmberEUI64[8];
#include "stack/config/token-stack.h"
#undef DEFINETYPES


#define DEFINETOKENS
#define TOKEN_MFG(name, creator, iscnt, isidx, type, arraysize, ...)


/**
 * @brief Define tokenArraySize
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  arraysize,
const uint8_t tokenArraySize[] = {
#include "stack/config/token-stack.h"
};
#undef TOKEN_DEF

/**
 * @brief Define tokenSize
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  sizeof(type),
const uint8_t tokenSize[] = {
#include "stack/config/token-stack.h"
};
#undef TOKEN_DEF

/**
 * @brief Define tokenIsCnt
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  iscnt,
const bool tokenIsCnt[] = {
#include "stack/config/token-stack.h"
};
#undef TOKEN_DEF

/**
 * @brief Define tokenCreators
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  creator,
const uint16_t tokenCreators[] = {
#include "stack/config/token-stack.h"
};
#undef TOKEN_DEF

/**
 * @brief Define tokenDefaults
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  const type TOKEN_##name##_DEFAULTS = __VA_ARGS__;
#include "stack/config/token-stack.h"
#undef TOKEN_DEF

#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  ((void *)&TOKEN_##name##_DEFAULTS),
const void * const tokenDefaults[] = {
#include "stack/config/token-stack.h"
};
#undef TOKEN_DEF


/**
 * @brief Define ptrCache and PTR_CACHE_SIZE
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  + (((arraysize) < 127 /*fundamental limit (3)*/)                   \
     ? ((arraysize) + (((arraysize) == 1) ? 0 : 1))                  \
     : -10000 /*force negative array length compile-time error*/)
//value of all index counts added together
const uint16_t PTR_CACHE_SIZE = (0
#include "stack/config/token-stack.h"
);
//the ptrCache definition - used to provide efficient access, based upon
//ID and index, to the freshest data in the Simulated EEPROM.
uint16_t ptrCache[0    //Compiler error here means too many elements in an array token
#include "stack/config/token-stack.h"
];
#undef TOKEN_DEF


/**
 * @brief Define sizeCache
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  + ((sizeof(type) < 255 /*fundamental limit (2)*/)                  \
     ? 1 : -10000 /*large negative will force compile-time error*/)
const uint8_t sizeCache[0    //Compiler error here means a token's size is too large
#include "stack/config/token-stack.h"
] = {
#undef TOKEN_DEF
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  (BYTES_TO_WORDS(sizeof(type))                                      \
   + ((iscnt) ? BYTES_TO_WORDS(COUNTER_TOKEN_PAD) : 0)),
#include "stack/config/token-stack.h"
        };
#undef TOKEN_DEF


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
 * @brief Define totalTokenStorage
 */
#define TOKEN_DEF(name, creator, iscnt, isidx, type, arraysize, ...) \
  + ((arraysize) * (1 /*info word*/ + BYTES_TO_WORDS(sizeof(type))   \
                    + ((!!(iscnt)) * BYTES_TO_WORDS(COUNTER_TOKEN_PAD))))
const uint32_t totalTokenStorage[(0   //Compiler error here means total token storage exceeds limit
                                  + (TOKEN_COUNT * 2)
#include "stack/config/token-stack.h"
                                 ) <= (SIMEE_BTS_SIZE_B / 2)/*fundamental limit (4)*/
                                 ? 1 : -1/*negative forces compile-time error*/
] = { (TOKEN_COUNT * 2)
      #include "stack/config/token-stack.h"
        };
#undef TOKEN_DEF


#undef TOKEN_MFG
#undef DEFINETOKENS

/**
 * @brief Define Symbols
 */
VAR_AT_SEGMENT(NO_STRIPPING uint8_t simulated_eeprom_storage_static[SIMEE_SIZE_B], __SIMEE__);

/**
 * @brief Dummy declaration for simulated eeprom storage
 *
 * The alias variable simulatedEepromStorage shares the same memory with simulated_eeprom_storage_static
 */
extern uint8_t simulatedEepromStorage[] __attribute__((alias("simulated_eeprom_storage_static")));

const uint8_t REAL_PAGES_PER_VIRTUAL = ((SIMEE_SIZE_HW / FLASH_PAGE_SIZE_HW) / 2);
const uint16_t REAL_PAGE_SIZE = FLASH_PAGE_SIZE_HW;
const uint16_t LEFT_BASE = SIMEE_BASE_ADDR_HW;
const uint16_t LEFT_TOP = ((SIMEE_BASE_ADDR_HW + (FLASH_PAGE_SIZE_HW
                                                  * ((SIMEE_SIZE_HW / FLASH_PAGE_SIZE_HW) / 2))) - 1);
const uint16_t RIGHT_BASE = (SIMEE_BASE_ADDR_HW + (FLASH_PAGE_SIZE_HW
                                                   * ((SIMEE_SIZE_HW / FLASH_PAGE_SIZE_HW) / 2)));
const uint16_t RIGHT_TOP = (SIMEE_BASE_ADDR_HW + (SIMEE_SIZE_HW - 1));
const uint16_t ERASE_CRITICAL_THRESHOLD = (SIMEE_SIZE_HW / 4);
const uint16_t ID_COUNT = TOKEN_COUNT;

bool checkForSimEe2DataExistence(uint32_t simEe2offset);
uint8_t halInternalSimEeStartupCore(bool forceRebuildAll, uint8_t *lookupCache);

static void checkForSimEe2(void)
{
    bool simEe2Found = false;

    simEe2Found = checkForSimEe2DataExistence(0x7000);
    DRV_ASSERT(simEe2Found == false);
    simEe2Found = checkForSimEe2DataExistence(0x4000);
    DRV_ASSERT(simEe2Found == false);
    simEe2Found = checkForSimEe2DataExistence(0x1000);
    DRV_ASSERT(simEe2Found == false);
}


__attribute__((weak))
uint8_t halInternalFlashWrite(uint32_t address, uint16_t * data, uint32_t length)
{
    DRV_ASSERT(false);
    return 0;
}

__attribute__((weak))
uint8_t halInternalFlashErase(uint8_t eraseType, uint32_t address)
{
    DRV_ASSERT(false);
    return 0;
}

void halSimEepromCallback(uint8_t status)
{
    switch (status)
    {
        case 0x43: // EMBER_SIM_EEPROM_ERASE_PAGE_GREEN
        {
            halSimEepromErasePage();
            break;
        }
        case 0x44: // EMBER_SIM_EEPROM_ERASE_PAGE_RED // INTERNATIONALLY FALL THROUGH
        case 0x45: // EMBER_SIM_EEPROM_FULL:
        {
            if (halSimEepromPagesRemainingToBeErased() > 0)
            {
                while (halSimEepromErasePage());

                break;
            }

            // INTERNATIONALLY FALL THROUGH
        }
        case 0x46: // EMBER_ERR_FLASH_WRITE_INHIBITED // INTERNATIONALLY FALL THROUGH
        case 0x47: // EMBER_ERR_FLASH_VERIFY_FAILED
        {
            // TODO: Implement this
            DRV_ASSERT(false);
            break;
        }
        case 0x4d: // EMBER_SIM_EEPROM_REPAIRING
        {
            break;
        }
        default:
        {
            DRV_ASSERT(false);
            break;
        }
    }
}

uint8_t halInternalSimEeStartup(bool forceRebuildAll)
{
    uint8_t lookupCache[0  //Compiler error here means too many tokens declared
                        + (TOKEN_COUNT < 255/*fundamental limit (1)*/
                           ? TOKEN_COUNT : -1 /*force compile-time error*/)];
    uint32_t indexSkip = TOKEN_COUNT;

    memset(lookupCache, 0xff, ID_COUNT);
    memset(ptrCache, 0xff, PTR_CACHE_SIZE * sizeof(uint16_t));

    for (uint8_t i = 0; i < TOKEN_COUNT; i++)
    {
        uint32_t size = (uint32_t) BYTES_TO_WORDS(tokenSize[i]);
        uint32_t arraySize = tokenArraySize[i];
        if (tokenIsCnt[i])
        {
            size += BYTES_TO_WORDS(COUNTER_TOKEN_PAD);
        }

        DEBUG_PRINT("Creator: 0x%02x, Words: %d\r\n", tokenCreators[i], (uint16_t)(arraySize * (1 + size)));

        if (arraySize != 1)
        {
            ptrCache[1] = (uint16_t) indexSkip;
            indexSkip += arraySize;
        }
    }

    DEBUG_PRINT("SimEE data: %d words of %d max ,tokenCount: %d\r\n", (uint16_t) *totalTokenStorage, (uint16_t)(SIMEE_BTS_SIZE_B/2), (uint16_t)TOKEN_COUNT);
    checkForSimEe2();

    return halInternalSimEeStartupCore(forceRebuildAll, lookupCache);
}

void simeeprom_init(simeeprom_t * obj)
{
    DRV_ASSERT(halInternalSimEeInit() == 0);

    simeeprom_bookkeeping(obj);
}

void simeeprom_read(simeeprom_t * obj, uint8_t id, void * data, uint8_t len)
{
    halInternalSimEeGetData(data, id, 0, len);
}

void simeeprom_write(simeeprom_t * obj, uint8_t id, void * data, uint8_t len)
{
    halInternalSimEeSetData(id, data, 0, len);
}

void simeeprom_bookkeeping(simeeprom_t * obj)
{
    while (halSimEepromErasePage());
}
