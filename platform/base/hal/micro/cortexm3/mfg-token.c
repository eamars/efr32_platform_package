/** @file hal/micro/cortexm3/mfg-token.c
 * @brief Cortex-M3 Manufacturing-Token system
 *
 * <!-- Copyright 2014 Silicon Laboratories, Inc.                        *80*-->
 */
#include PLATFORM_HEADER
#include "include/error.h"
#include "hal/micro/cortexm3/flash.h"
#include "mfg-token.h"




#define DEFINETOKENS
#define TOKEN_MFG(name, creator, iscnt, isidx, type, arraysize, ...) \
  const uint16_t TOKEN_##name = TOKEN_##name##_ADDRESS;
  #include "hal/micro/cortexm3/token-manufacturing.h"
#undef TOKEN_DEF
#undef TOKEN_MFG
#undef DEFINETOKENS








static const uint8_t nullEui[] = { 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU };

static void getMfgTokenData(void *data,
                            uint16_t token,
                            uint8_t index,
                            uint8_t len)
{
  uint8_t *ram = (uint8_t*)data;

  //0x7F is a non-indexed token.  Remap to 0 for the address calculation
  index = (index == 0x7FU) ? 0U : index;

  //read from the Information Blocks.  The token ID is only the
  //bottom 16bits of the token's actual address.  Since the info blocks
  //exist in the range FIB_BOTTOM-DATA_BIG_INFO_END, we need
  //to OR the ID with FIB_BOTTOM to get the real address.
  uint32_t realAddress = (FIB_BOTTOM | token) + (len * index);
  uint8_t *flash = (uint8_t *)realAddress;






















  MEMCOPY(ram, flash, len);
}

void halInternalGetMfgTokenData(void *data,
                                uint16_t token,
                                uint8_t index,
                                uint8_t len)
{
  if (len == 0U) {
    return; // Nothing to do...
  }
  if (token == MFG_EUI_64_LOCATION) {
    //There are two EUI64's stored in the Info Blocks, Ember and Custom.
    //0x0A00 is the address used by the generic EUI64 token, and it is
    //token.c's responbility to pick the returned EUI64 from either Ember
    //or Custom.  Return the Custom EUI64 if it is not all FF's, otherwise
    //return the Ember EUI64.
    if (len > sizeof(nullEui)) {
      len = sizeof(nullEui);
    }
    getMfgTokenData(data, MFG_CUSTOM_EUI_64_LOCATION, 0x7FU, len);
    if (MEMCOMPARE(data, nullEui, len) != 0) {
      return; // Return CUSTOM EUI
    }
    token = MFG_EMBER_EUI_64_LOCATION;
  }
  getMfgTokenData(data, token, index, len);
}

void halInternalSetMfgTokenData(uint16_t token, void *data, uint8_t len)
{
  EmberStatus flashStatus;
  uint32_t realAddress = (FIB_BOTTOM | token);
  uint8_t * flash = (uint8_t *)realAddress;
  uint32_t i;

  //The flash library (and hardware) requires the address and length to both
  //be multiples of 16bits.  Since this API is only valid for writing to
  //the CIB, verify that the token+len falls within the CIB.
  assert((token & 1) != 1);
  assert((len & 1) != 1);
  assert((realAddress >= CIB_BOTTOM) && ((realAddress + len - 1) <= CIB_TOP));

  //CIB manufacturing tokens can only be written by on-chip code if the token
  //is currently unprogrammed.  Verify the entire token is unwritten.  The
  //flash library performs a similar check, but verifying here ensures that
  //the entire token is unprogrammed and will prevent partial writes.
  for (i = 0; i < len; i++) {
    assert(flash[i] == 0xFF);
  }

  //Remember, the flash library operates in 16bit quantities, but the
  //token system operates in 8bit quantities.  Hence the divide by 2.
  flashStatus = halInternalFlashWrite(realAddress, data, (len / 2));
  assert(flashStatus == EMBER_SUCCESS);
}
