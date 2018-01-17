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


static void gpcrc_feed(gpcrc_t * obj, void * buffer, size_t len)
{
    // attempt to access data aligned, for the rest of bytes, feed byte by byte
    size_t aligned_data_start = len / sizeof(uint32_t);
    size_t unaligned_data_start = len - (len % sizeof(uint32_t));

    // feed aligned data
    for (size_t i = 0; i < aligned_data_start; i++)
    {
        GPCRC_InputU32(obj->gpcrc_engine, ((uint32_t *) buffer)[i]);
    }

    // feed unaligned data
    for (size_t i = unaligned_data_start; i < len; i++)
    {
        GPCRC_InputU8(obj->gpcrc_engine, ((uint8_t *) buffer)[i]);
    }
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

    gpcrc_feed(obj, buffer, len);

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

uint16_t gpcrc_ccitt(gpcrc_t * obj, void * buffer, size_t len)
{
    uint16_t checksum;
    GPCRC_Init_TypeDef init_data = GPCRC_INIT_DEFAULT;

    // configure the parameter
    init_data.crcPoly = 0x1021; // x16 + x12 + x5 + 1
    init_data.initValue = 0xFFFF;
    init_data.reverseBits = true;

#if USE_FREERTOS == 1
    xSemaphoreTake(obj->access_mutex, portMAX_DELAY);
#endif // #if USE_FREERTOS == 1

    GPCRC_Init(obj->gpcrc_engine, &init_data);
    GPCRC_Start(obj->gpcrc_engine);

    gpcrc_feed(obj, buffer, len);

    checksum = (uint16_t) GPCRC_DataReadBitReversed(obj->gpcrc_engine);

#if USE_FREERTOS == 1
    xSemaphoreGive(obj->access_mutex);
#endif // #if USE_FREERTOS == 1

    return checksum;
}
