//
// test-harness-z3-ota-server.c
//
// April 21, 2017
//
// ZigBee 3.0 ota server test harness functionality
//

#include "app/framework/include/af.h"

#include "test-harness-z3-core.h"
#include "app/framework/plugin/ota-server/ota-server.h"
#include "app/framework/plugin/ota-storage-common/ota-storage.h"
// -----------------------------------------------------------------------------
static EmberAfOtaImageId id;
EmberAfOtaImageId emberAfOtaStorageSearchCallback(int16u manufacturerId,
                                                  int16u imageTypeId,
                                                  const int16u* hardwareVersion)
{
  (void)manufacturerId;
  (void)imageTypeId;
  (void)hardwareVersion;
  return id;
}

void emAfOtaLoadFileCommand(void)
{
  // TODO: dummy file for the test harness
}
bool emberAfIsOtaImageIdValid(const EmberAfOtaImageId* idToCompare)
{
  // TODO: The id should be compared for a valid transfer, based on the
  // discussion in the event.
  (void)idToCompare;
  return true;
}

// OTA Server commands
// plugin test-harness z3 ota-server activate<shortAddress:2> <manufacturereCode:2> <imageType:2> <fileVersion:4>
//static EmberAfOtaImageId otaImageId;
void emAfPluginTestHarnessZ3OtaServerActivateCommand(void)
{
  EmberStatus status = EMBER_INVALID_CALL;
#ifndef EZSP_HOST
  //TODO: EmberNodeId shortAddress = (EmberNodeId)emberUnsignedCommandArgument(0);
  //TODO: uint8_t endpoint = (uint8_t)emberUnsignedCommandArgument(1);
  id.manufacturerId = (uint16_t)emberUnsignedCommandArgument(2);
  id.imageTypeId = (uint16_t)emberUnsignedCommandArgument(3);
  id.firmwareVersion = (uint32_t)emberUnsignedCommandArgument(4);
  status = EMBER_SUCCESS;
#endif /* EZSP_HOST */

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Ota Server activate",
                     status);
}

// plugin test-harness z3 ota-server deactivate
void emAfPluginTestHarnessZ3OtaServerDeActivateCommand(void)
{
  EmberStatus status = EMBER_INVALID_CALL;
#ifndef EZSP_HOST
  EmberAfOtaImageId InvalidId = {
    INVALID_MANUFACTURER_ID,
    INVALID_DEVICE_ID,
    INVALID_FIRMWARE_VERSION,
    INVALID_EUI64,
  };
  id = InvalidId;
  status = EMBER_SUCCESS;
#endif /* EZSP_HOST */

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Ota Server Deactivate",
                     status);
}
