/**
 * @brief FreeRTOS wrapper for GPCRC module
 * @author Ran Bao
 * @date 17/Jan/2018
 * @file gpcrc.c
 *
 * It also works with non-freertos application
 */

#include "gpcrc.h"
#include "em_gpcrc.h"
#include "em_cmu.h"

void gpcrc_init(gpcrc_t * obj)
{
    obj->gpcrc_engine = GPCRC;

    // start clock
    CMU_ClockEnable(cmuClock_HFPER, true);
    CMU_ClockEnable(cmuClock_GPCRC, true);

#if USE_FREERTOS == 1
    obj->access_mutex = xSemaphoreCreateMutex();
#endif // #if USE_FREERTOS == 1
}


uint32_t gpcrc_crc32(gpcrc_t * obj, uint32_t polynomial, uint32_t init_value, void * buffer, size_t len)
{
    uint32_t checksum;
    GPCRC_Init_TypeDef init_data = GPCRC_INIT_DEFAULT;

    // configure the parameter
    init_data.crcPoly = polynomial;
    init_data.initValue = init_value;

#if USE_FREERTOS == 1
    xSemaphoreTake(obj->access_mutex, portMAX_DELAY);
#endif // #if USE_FREERTOS == 1

    GPCRC_Init(obj->gpcrc_engine, &init_data);
    GPCRC_Start(obj->gpcrc_engine);

    if (len % sizeof(uint32_t) == 0)
    {
        // we can utilize the memory
        uint32_t * data = buffer;
        for (size_t i = 0; i < len / 4; i += 1)
        {
            GPCRC_InputU32(obj->gpcrc_engine, data[i]);
        }
    }
    else if (len % sizeof(uint16_t) == 0)
    {
        uint16_t * data = buffer;
        for (size_t i = 0; i < len / 2; i += 1)
        {
            GPCRC_InputU16(obj->gpcrc_engine, data[i]);
        }
    }
    else
    {
        uint8_t * data = buffer;
        for (size_t i = 0; i < len; i += 1)
        {
            GPCRC_InputU8(obj->gpcrc_engine, data[i]);
        }
    }

    // according to crc32 spec the end result should be reverted
    checksum = ~GPCRC_DataRead(obj->gpcrc_engine);

#if USE_FREERTOS == 1
    xSemaphoreGive(obj->access_mutex);
#endif // #if USE_FREERTOS == 1

    return checksum;
}

uint16_t gpcrc_crc16(gpcrc_t * obj, uint16_t polynomial, uint16_t init_value, void * buffer, size_t len)
{
    // since the result from crc32 is reverted, the crc16 should revert it back
    return (uint16_t) ~gpcrc_crc32(obj, polynomial, init_value, buffer, len);
}
