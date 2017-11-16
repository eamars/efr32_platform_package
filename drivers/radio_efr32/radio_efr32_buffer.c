/**
 * @brief Radio buffer management module for onboard EFR32 radio
 * @author Silicon Labs
 * @date 13/Nov/2017
 * @file radio_efr32_buffer.c
 */

#include <stdint.h>
#include "rail.h"
#include "buffer_pool_allocator.h"


/// Rely on the pool allocator's allocate function to get memory
void *RAILCb_AllocateMemory(uint32_t size)
{
    return memoryAllocate(size);
}

/// Use the pool allocator's free function to return the memory to the pool
void RAILCb_FreeMemory(void *ptr)
{
    memoryFree(ptr);
}

/// Get the memory pointer for this handle and offset into it as requested
void *RAILCb_BeginWriteMemory(void *handle,
                              uint32_t offset,
                              uint32_t *available)
{
    return ((uint8_t*)memoryPtrFromHandle(handle)) + offset;
}

/// We don't need to track the completion of a memory write so do nothing
void RAILCb_EndWriteMemory(void *handle, uint32_t offset, uint32_t size)
{
}
