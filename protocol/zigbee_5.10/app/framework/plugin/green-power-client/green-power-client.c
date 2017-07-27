// *******************************************************************
// * green-power-client.c
// *
// *
// * Copyright 2014 by Silicon Laboratories. All rights reserved.           *80*
// * Jing Teng ( jing.teng@silabs.com )
// *******************************************************************

#include "enums.h"
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/common.h"
#include "green-power-client.h"
#include "stack/include/gp-types.h"
//#include "green-power-proxy-table.h"
#include "stack/gp/gp-proxy-table.h"
#include "app/framework/plugin/green-power-common/green-power-common.h"

//#include "stack/gp/gp.h"

#ifdef EZSP_HOST
  #define emberGpepIncomingMessageHandler ezspGpepIncomingMessageHandler
  #define emberDGpSentHandler ezspDGpSentHandler
#endif

bool __emIsGpTestDevice = false;

void emGpSecurityResponse(EmberStatus status,
                          uint8_t dgpHandle,
                          EmberGpAddress *addr,
                          EmberGpSecurityLevel gpdfSecurityLevel,
                          EmberGpKeyType gpdf_KeyType,
                          EmberKeyData gpdKey,
                          EmberGpSecurityFrameCounter gpdSecurityFrameCounter);

/*
   EmberStatus emberDGpSend(bool action,
                         bool useCca,
                         EmberGpAddress *addr,
                         uint8_t gpdCommandId,
                         uint8_t gpdAsduLength,
                         uint8_t const *gpdAsdu,
                         uint8_t gpepHandle,
                         uint16_t gpTxQueueEntryLifetimeMs);
 */

void emSetAddDelay(uint16_t delay);
bool emGpTxQueueFull(EmberGpAddress *addr);

static EmberAfGreenPowerClientCommissioningState commissioningState;
static EmberAfGreenPowerDuplicateFilter duplicateFilter;
EmberEventControl emberAfPluginGreenPowerClientExpireSeqNumEventControl;
EmberEventControl emberAfPluginGreenPowerClientExitCommissioningEventControl;
EmberEventControl emberAfPluginGreenPowerClientChannelEventControl;

#ifdef EZSP_HOST
static bool greenPowerActive = false;
#endif
void emberAfPluginGreenPowerClientInitCallback(void)
{
  MEMSET(&commissioningState,
         0x00,
         sizeof(EmberAfGreenPowerClientCommissioningState));
  MEMSET(&duplicateFilter, 0x00, sizeof(EmberAfGreenPowerDuplicateFilter));

  commissioningState.gppCommissioningWindow = EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_GPP_COMMISSIONING_WINDOW;

  // Write the max proxy table entries attribute, since we currently have no way
  // to configure it at compile time.
  uint8_t proxyTableSize = EMBER_GP_PROXY_TABLE_SIZE;
  EmberAfStatus status
    = emberAfWriteClientAttribute(GP_ENDPOINT,
                                  ZCL_GREEN_POWER_CLUSTER_ID,
                                  ZCL_GP_CLIENT_GPP_MAX_PROXY_TABLE_ENTRIES_ATTRIBUTE_ID,
                                  &proxyTableSize,
                                  ZCL_INT8U_ATTRIBUTE_TYPE);
  emberAfGreenPowerClusterPrintln("Writing proxy table size attribute: 0x%x",
                                  status);

#ifdef EZSP_HOST
  uint16_t configVal;
  if (ezspGetConfigurationValue(EZSP_CONFIG_GREEN_POWER_ACTIVE, &configVal) == EZSP_SUCCESS) {
    greenPowerActive = true;
  }
#endif
}

void emGpMakeAddr(EmberGpAddress *addr,
                  EmberGpApplicationId appId,
                  EmberGpSourceId srcId,
                  uint8_t *gpdIeee,
                  uint8_t endpoint)
{
  addr->applicationId = appId;
  if (appId == EMBER_GP_APPLICATION_SOURCE_ID) {
    addr->id.sourceId = srcId;
  } else if (appId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
    MEMCOPY(addr->id.gpdIeeeAddress, gpdIeee, EUI64_SIZE);
    addr->endpoint = endpoint;
  }
}

void emberAfGreenPowerClusterExitCommissioningMode()
{
  //emberAfPluginGreenPowerClientInitCallback();
  commissioningState.inCommissioningMode = false;
  emberAfGreenPowerClusterPrintln("Exiting commissioning mode for sink %2x", commissioningState.commissioningSink);
  // TODO: clean up gpTxQueue/gpTxBuffer
  //       remove sender information in stack.
}

void emberAfPluginGreenPowerClientChannelEventHandler()
{
  if (commissioningState.onTransmitChannel) {
    EmberGpAddress addr;
    addr.id.sourceId = 0;
    addr.applicationId = 0;
    emberAfGreenPowerClusterPrintln("returning to channel %d", commissioningState.onTransmitChannel);
    emberSetRadioChannel(commissioningState.onTransmitChannel);
    commissioningState.onTransmitChannel = 0;

    //clear the gpTxQueue
    emberDGpSend(false, false, &addr, 0, 0, NULL, 0, 0);
  }
  emberEventControlSetInactive(emberAfPluginGreenPowerClientChannelEventControl);
}

void emberAfPluginGreenPowerClientExitCommissioningEventHandler()
{
  emberAfGreenPowerClusterExitCommissioningMode();
  emberEventControlSetInactive(emberAfPluginGreenPowerClientExitCommissioningEventControl);
}

/*
 * Disable default response bit should be set per GP Spec 14-0563-08
 */
bool emberAfGreenPowerClusterGpProxyCommissioningModeCallback(uint8_t options,
                                                              uint16_t commissioningWindow,
                                                              uint8_t channel)
{
  bool enterCommissioningMode = (options
                                 & EMBER_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_ACTION);
  uint8_t exitMode = (options
                      & EMBER_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_EXIT_MODE)
                     >> EMBER_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_EXIT_MODE_OFFSET;
  bool chanPresent = (options
                      & EMBER_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_CHANNEL_PRESENT); // should always be 0
  bool unicastCommunication = (options
                               & EMBER_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_UNICAST_COMMUNICATION);

  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
  } else if (commissioningState.inCommissioningMode
             && (emberGetSender() != commissioningState.commissioningSink)) {
    // check if current message sender is same as sender that put us in
    // commissioning mode.
    // if not, drop message silently.
    emberAfGreenPowerClusterPrintln("Drop frame due to frame not sent by commisioning sink: %X", commissioningState.commissioningSink);
  } else if (enterCommissioningMode) {
    commissioningState.commissioningSink = emberGetSender();
    commissioningState.inCommissioningMode = true;
    commissioningState.gppCommissioningWindow = EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_GPP_COMMISSIONING_WINDOW;
    commissioningState.exitMode = (EmberAfGreenPowerClientCommissioningExitMode) exitMode;
    commissioningState.unicastCommunication = unicastCommunication;

    emberAfGreenPowerClusterPrintln("Entering commissioning mode for sink %2x", commissioningState.commissioningSink);

    if (commissioningState.exitMode & EMBER_AF_GPC_COMMISSIONING_EXIT_ON_COMMISSIONING_WINDOW_EXP) {
      // store the new commissioning window value and update our local default
      // one.
      emberEventControlSetDelayMS(emberAfPluginGreenPowerClientExitCommissioningEventControl, commissioningWindow * 1000);
    } else {
      emberEventControlSetDelayMS(emberAfPluginGreenPowerClientExitCommissioningEventControl, EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_GPP_COMMISSIONING_WINDOW * 1000);
    }

    if (chanPresent) {
      commissioningState.channel = channel;
      // TODO: goto channel
    } else {
      // TODO: goto operational channel
    }
  } else {
    // exit commissioning mode.
    emberAfGreenPowerClusterExitCommissioningMode();
  }

  return true;
}

bool emberAfGreenPowerClusterGpNotificationResponseCallback(uint8_t options,
                                                            uint32_t gpdSrcId,
                                                            uint8_t* gpdIeee,
                                                            uint32_t gpdSecurityFrameCounter)
{
  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
    goto kickout;
  }
  kickout: return false;
}

/*
 * Disable default response should be enabled in the sending message.
 */
bool emberAfGreenPowerClusterGpPairingCallback(uint32_t options, // actually a int24u.
                                               uint32_t gpdSrcId,
                                               uint8_t* gpdIeee,
                                               uint8_t endpoint,
                                               uint8_t* sinkIeeeAddress,
                                               uint16_t sinkNwkAddress,
                                               uint16_t sinkGroupId,
                                               uint8_t deviceId,
                                               uint32_t gpdSecurityFrameCounter,
                                               uint8_t* gpdKey,
                                               uint16_t assignedAlias,
                                               uint8_t forwardingRadius)
{
  uint8_t dummyArray[17];
  uint8_t securityLevel = (options & EMBER_AF_GP_PAIRING_OPTION_SECURITY_LEVEL)
                          >> EMBER_AF_GP_PAIRING_OPTION_SECURITY_LEVEL_OFFSET;
  uint8_t commMode = ((options & EMBER_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE)
                      >> EMBER_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET);
  uint8_t securityKeyType = ((options & EMBER_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE)
                             >> EMBER_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE_OFFSET);
  EmberGpAddress addr;
  bool createdPairing = false;

  //handle null args for EZSP
  if (gpdIeee  == NULL) {
    gpdIeee = dummyArray;
  }
  if (sinkIeeeAddress  == NULL) {
    sinkIeeeAddress = dummyArray;
  }
  if (gpdKey  == NULL) {
    gpdKey = dummyArray;
  }

  emGpMakeAddr(&addr,
               options & EMBER_AF_GP_PAIRING_OPTION_APPLICATION_ID,
               gpdSrcId,
               gpdIeee,
               endpoint);

  emberAfCorePrint("GP Pairing callback\n");

  // Commissioning: Step 21a: Proxy finalizes commissioning
  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Dropping frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
  } else if (securityLevel == EMBER_GP_SECURITY_LEVEL_RESERVED) {
    emberAfGreenPowerClusterPrintln("Dropping frame due to un-support security level: %X", securityLevel);
  } else if ((addr.applicationId != EMBER_GP_APPLICATION_SOURCE_ID)
             && (addr.applicationId != EMBER_GP_APPLICATION_IEEE_ADDRESS)) {
    emberAfGreenPowerClusterPrintln("Dropping frame due to un-support security level: %X", securityLevel);
  } else if ((addr.applicationId == EMBER_GP_APPLICATION_SOURCE_ID)
             && ((addr.id.sourceId == GP_GPD_SRC_ID_RESERVED_0)
                 || ((addr.id.sourceId >= GP_GPD_SRC_ID_RESERVED_FFFFFF9)
                     && (addr.id.sourceId <= GP_GPD_SRC_ID_RESERVED_FFFFFFE)))) {
    emberAfGreenPowerClusterPrintln("Dropping current frame due to appId/srcId values.");
  } else if ((addr.applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS)
             && ((emberAfMemoryByteCompare(addr.id.gpdIeeeAddress, EUI64_SIZE, 0)))) {
    emberAfGreenPowerClusterPrintln("Dropping current frame due to appId/Ieee values.");
  } else if ((options & EMBER_AF_GP_PAIRING_OPTION_ADD_SINK)
             && (options & EMBER_AF_GP_PAIRING_OPTION_REMOVE_GPD)) {
    emberAfGreenPowerClusterPrintln("Dropping current frame due to ADD+REMOVE together");
  } else if ((securityKeyType > EMBER_GP_SECURITY_KEY_GPD_OOB)
             &&  (securityKeyType < EMBER_GP_SECURITY_KEY_GPD_DERIVED)) {
    emberAfGreenPowerClusterPrintln("Dropping current frame due to Reserved Security Key Type");
  } else {
    // Step b:

    emberAfGreenPowerClusterPrintln(" PAIRING CREATION  SRC ID = %4x, Options = %u Sink nodeid = %2x", addr.id.sourceId, options, sinkNwkAddress);

    createdPairing = emberGpProxyTableProcessGpPairing(options,
                                                       &addr,
                                                       commMode,
                                                       sinkNwkAddress,
                                                       sinkGroupId,
                                                       assignedAlias,
                                                       sinkIeeeAddress,
                                                       (EmberKeyData *) gpdKey,
                                                       gpdSecurityFrameCounter,
                                                       forwardingRadius);

    // step c: optionally, exit commissioning mode
    if (createdPairing
        && (commissioningState.exitMode & EMBER_AF_GPC_COMMISSIONING_EXIT_ON_FIRST_PAIRING_SUCCESS)) {
      emberAfGreenPowerClusterExitCommissioningMode();
    }
  }

  return true;
}

bool emberAfGreenPowerClusterGpResponseCallback(uint8_t options,
                                                uint16_t tempMasterShortAddress,
                                                uint8_t tempMasterTxChannel,
                                                uint32_t gpdSrcId,
                                                uint8_t* gpdIeee,
                                                uint8_t endpoint,
                                                uint8_t gpdCommandId,
                                                uint8_t* gpdCommandPayload)
{
  EmberGpAddress addr;

  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
    goto kickout;
  }
  emberAfGreenPowerClusterPrintln("got GpResponse command: %1x", gpdCommandId);

  switch (gpdCommandId) {
    case EMBER_ZCL_GP_GPDF_CHANNEL_CONFIGURATION:
      addr.applicationId = EMBER_GP_APPLICATION_SOURCE_ID;
      addr.id.sourceId = 0x00000000;
      if (gpdSrcId != 0x00000000) {
        return true;
      }

      if (tempMasterShortAddress == emberGetNodeId()) {
        //Look at me, I'm the tempMaster now
        //TODO set firstToForward

        uint8_t payload = (emberAfGetRadioChannel() - 11) //channel ID
                          | 0x10;//basic
        emberAfGreenPowerClusterPrintln("calling dgpsend on channel response");
        emberDGpSend(true, //add
                     false,//no CCA
                     &addr,
                     EMBER_ZCL_GP_GPDF_CHANNEL_CONFIGURATION,
                     1, //length
                     &(payload),
                     0, //gpepHandle
                     0); //entryLifetimeMs

        commissioningState.onTransmitChannel = emberAfGetRadioChannel();
        emberSetRadioChannel(tempMasterTxChannel + 11);
        emberAfGreenPowerClusterPrintln("set radio channel to %d", tempMasterTxChannel + 11);
        emberEventControlSetDelayMS(emberAfPluginGreenPowerClientChannelEventControl, 5000);
      } else {
        //TODO clear firstToForward
        emberAfPluginGreenPowerClientChannelEventHandler();
        //emberDGpSend(false, false, &addr, 0, 0, NULL, 0, 0);
      }
      break;
    case EMBER_ZCL_GP_GPDF_COMMISSIONING_REPLY:
      //TODO IEEE GPD
    {
      EmberStatus result;
      EmberGpAddress addr;
      addr.applicationId = options & 0x03;
      if (addr.applicationId == EMBER_GP_APPLICATION_SOURCE_ID) {
        addr.id.sourceId = gpdSrcId;
      } else {
        addr.applicationId = EMBER_GP_APPLICATION_IEEE_ADDRESS;
        addr.endpoint = endpoint;
        MEMCOPY(addr.id.gpdIeeeAddress, gpdIeee, 8);
      }

      emberAfGreenPowerClusterPrintln("calling dgpsend on comm reply");
      result = emberDGpSend(true, //add
                            false, //skip cca
                            &addr,
                            EMBER_ZCL_GP_GPDF_COMMISSIONING_REPLY,
                            gpdCommandPayload[0],
                            gpdCommandPayload + 1,
                            0, 0
                            );
      if (result == EMBER_SUCCESS) {
        emberAfGreenPowerClusterPrintln("success");
      } else {
        emberAfGreenPowerClusterPrintln("fail %02x", result);
      }
    }

    break;
  }

  kickout: return false;
}

bool emberAfGreenPowerClusterGpSinkTableResponseCallback(uint8_t status,
                                                         uint8_t totalNumberofNonEmptySinkTableEntries,
                                                         uint8_t startIndex,
                                                         uint8_t entriesCount,
                                                         uint8_t* sinkTableEntry)
{
  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
    goto kickout;
  }
  kickout: return false;
}

uint16_t emberAfGreenPowerClientStoreProxyTableEntry(EmberGpProxyTableEntry *entry,
                                                     uint8_t *buffer)
{
  uint8_t *finger = buffer;
  //uint8_t securityLevel = entry->securityOptions & EMBER_AF_GP_PROXY_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL;
  //uint8_t securityKeyType = entry->securityOptions & EMBER_AF_GP_PROXY_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE;

  emberAfCopyInt16u(finger, 0, (uint16_t)(entry->options & 0xFFFF));
  finger += sizeof(uint16_t);
  if (entry->gpd.applicationId == EMBER_GP_APPLICATION_SOURCE_ID) {
    emberAfGreenPowerClusterPrintln("SourceID type");
    emberAfCopyInt32u(finger, 0, entry->gpd.id.sourceId);
    finger += sizeof(uint32_t);
  } else if (entry->gpd.applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
    emberAfGreenPowerClusterPrintln("IEEE type");
    MEMMOVE(finger, entry->gpd.id.gpdIeeeAddress, EUI64_SIZE);
    finger += EUI64_SIZE;
  }

  if (entry->gpd.applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
    emberAfCopyInt8u(finger, 0, entry->gpd.endpoint);
    finger += sizeof(uint8_t);
  }

  if (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS) {
    emberAfGreenPowerClusterPrintln("assigned alias %2x", entry->assignedAlias);
    emberAfCopyInt16u(finger, 0, entry->assignedAlias);
    finger += sizeof(uint16_t);
  }

  if (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_SECURITY_USE) {
    emberAfGreenPowerClusterPrintln("security optios %1x", entry->securityOptions);
    emberAfCopyInt8u(finger, 0, entry->securityOptions);
    finger += sizeof(uint8_t);
    emberAfGreenPowerClusterPrintln("security frame counter %4x", entry->gpdSecurityFrameCounter);
    emberAfCopyInt32u(finger, 0, entry->gpdSecurityFrameCounter);
    finger += sizeof(uint32_t);
    MEMMOVE(finger, entry->gpdKey.contents, EMBER_ENCRYPTION_KEY_SIZE);
    finger += EMBER_ENCRYPTION_KEY_SIZE;
  } else if (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_SEQUENCE_NUMBER_CAP) {
    emberAfGreenPowerClusterPrintln("security frame counter %4x", entry->gpdSecurityFrameCounter);
    emberAfCopyInt32u(finger, 0, entry->gpdSecurityFrameCounter);
    finger += sizeof(uint32_t);
  }

  // Lightweight sink address list
  if (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_LIGHTWEIGHT_UNICAST_GPS) {
    // let's count
    uint8_t index = 0;
    uint8_t *entryCount = finger;
    emberAfCopyInt8u(finger, 0, 0x00);
    finger += sizeof(uint8_t);
    for (index = 0; index < GP_SINK_LIST_ENTRIES; index++) {
      EmberGpSinkListEntry * sinkEntry = &entry->sinkList[index];

      if (sinkEntry->type == EMBER_GP_SINK_TYPE_LW_UNICAST) {
        MEMMOVE(finger, sinkEntry->target.unicast.sinkEUI, EUI64_SIZE);
        finger += EUI64_SIZE;
        emberAfCopyInt16u(finger, 0, sinkEntry->target.unicast.sinkNodeId);
        finger += sizeof(uint16_t);
        (*entryCount)++;
      } else {
        continue;
      }
    }
  }

  // Sink group list
  if (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_COMMISIONED_GROUP_GPS) {
    // let's count
    uint8_t index = 0;
    uint8_t *entryCount = finger;
    emberAfCopyInt8u(finger, 0, 0x00);
    finger += sizeof(uint8_t);
    for (index = 0; index < GP_SINK_LIST_ENTRIES; index++) {
      EmberGpSinkListEntry * sinkEntry = &entry->sinkList[index];

      if (sinkEntry->type == EMBER_GP_SINK_TYPE_GROUPCAST) {
        emberAfCopyInt16u(finger, 0, sinkEntry->target.groupcast.groupID);
        finger += sizeof(uint16_t);
        emberAfCopyInt16u(finger, 0, sinkEntry->target.groupcast.alias);
        finger += sizeof(uint16_t);
        (*entryCount)++;
      } else {
        continue;
      }
    }
  }

  emberAfCopyInt8u(finger, 0, entry->groupcastRadius);
  finger += sizeof(uint8_t);

  if ((entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_ENTRY_ACTIVE) == 0
      || (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_ENTRY_VALID) == 0) {
    emberAfCopyInt8u(finger, 0, entry->searchCounter);
    finger += sizeof(uint8_t);
  }

  if (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_EXTENSION) {
    emberAfCopyInt16u(finger, 0, (entry->options >> 16) & 0xFFFF);
    finger += sizeof(uint16_t);
  }

  // full unicast sink address list
  if (entry->options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_FULL_UNICAST_GPS) {
    // let's count
    uint8_t index = 0;
    uint8_t *len = emberAfPutInt8uInResp(0x00);
    for (index = 0; index < GP_SINK_LIST_ENTRIES; index++) {
      EmberGpSinkListEntry * sinkEntry = &entry->sinkList[index];

      if (sinkEntry->type == EMBER_GP_SINK_TYPE_LW_UNICAST) {
        MEMMOVE(finger, sinkEntry->target.unicast.sinkEUI, EUI64_SIZE);
        finger += EUI64_SIZE;
        emberAfCopyInt16u(finger, 0, sinkEntry->target.unicast.sinkNodeId);
        finger += sizeof(uint16_t);
        *len += EUI64_SIZE + sizeof(uint16_t);
      } else {
        continue;
      }
    }
  }

  return (uint16_t)(finger - buffer);
}

bool emberAfGreenPowerClusterGpProxyTableRequestCallback(uint8_t options,
                                                         uint32_t gpdSrcId,
                                                         uint8_t* gpdIeee,
                                                         uint8_t endpoint,
                                                         uint8_t index)
{
  uint8_t validEntriesCount = 0;
  uint8_t entryIndex = 0;
  uint8_t appId = options & EMBER_AF_GP_PROXY_TABLE_REQUEST_OPTIONS_APPLICATION_ID;
  uint8_t requestType = (options & EMBER_AF_GP_PROXY_TABLE_REQUEST_OPTIONS_REQUEST_TYPE)
                        >> EMBER_AF_GP_PROXY_TABLE_REQUEST_OPTIONS_REQUEST_TYPE_OFFSET;
  EmberGpProxyTableEntry entry;

  emberAfGreenPowerClusterPrintln("Got proxy table request with options %1x and index %1x", options, index);
  // only respond to unicast messages.
  if (emberAfCurrentCommand()->type != EMBER_INCOMING_UNICAST) {
    emberAfGreenPowerClusterPrintln("Not unicast");

    goto kickout;
  }

  if (EMBER_GP_PROXY_TABLE_SIZE == 0) {
    emberAfGreenPowerClusterPrintln("Unsup");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND);
    goto kickout;
  }

  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
    return false;
  }

  for (entryIndex = 0; entryIndex < EMBER_GP_PROXY_TABLE_SIZE; entryIndex++) {
    if (emberGpProxyTableGetEntry(entryIndex, &entry) != EMBER_SUCCESS) {
      return false;
    }

    if (entry.status != EMBER_GP_PROXY_TABLE_ENTRY_STATUS_UNUSED) {
      validEntriesCount++;
    }
  }

  if (requestType == EMBER_ZCL_GP_PROXY_TABLE_REQUEST_OPTIONS_REQUEST_TYPE_BY_GPD_ID) {
    EmberGpAddress addr;
    emGpMakeAddr(&addr,
                 appId,
                 gpdSrcId,
                 gpdIeee,
                 endpoint);
    entryIndex = emberGpProxyTableLookup(&addr);
    if (entryIndex == 0xFF) {
      emberAfFillCommandGreenPowerClusterGpProxyTableResponse(EMBER_ZCL_GP_PROXY_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                              validEntriesCount,
                                                              index,
                                                              0x00,
                                                              NULL,
                                                              0);
      emberAfSendResponse();
      goto kickout;
    } else {
      emberAfFillCommandGreenPowerClusterGpProxyTableResponse(EMBER_ZCL_GP_PROXY_TABLE_RESPONSE_STATUS_SUCCESS,
                                                              validEntriesCount,
                                                              0xff,
                                                              1,
                                                              NULL,
                                                              0);
      if (emberGpProxyTableGetEntry(entryIndex, &entry) != EMBER_SUCCESS) {
        return false;
      }

      appResponseLength
        += emberAfGreenPowerClientStoreProxyTableEntry(&entry,
                                                       appResponseData + appResponseLength);
      emberAfSendResponse();
      goto kickout;
    }
  } else if (requestType == EMBER_ZCL_GP_PROXY_TABLE_REQUEST_OPTIONS_REQUEST_TYPE_BY_INDEX) {
    if (index >= validEntriesCount) {
      //Nothing to send
      emberAfFillCommandGreenPowerClusterGpProxyTableResponse(EMBER_ZCL_GP_PROXY_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                              validEntriesCount,
                                                              index,
                                                              0x00,
                                                              NULL,
                                                              0);
      emberAfSendResponse();
      goto kickout;
    } else {
      emberAfFillCommandGreenPowerClusterGpProxyTableResponse(EMBER_ZCL_GP_PROXY_TABLE_RESPONSE_STATUS_SUCCESS,
                                                              validEntriesCount,
                                                              index,
                                                              validEntriesCount - index,
                                                              NULL,
                                                              0);
      validEntriesCount = 0;
      for (entryIndex = 0; entryIndex < EMBER_GP_PROXY_TABLE_SIZE; entryIndex++) {
        if (emberGpProxyTableGetEntry(entryIndex, &entry) != EMBER_SUCCESS) {
          return false;
        }

        if (entry.status != EMBER_GP_PROXY_TABLE_ENTRY_STATUS_UNUSED) {
          validEntriesCount++;
          if (validEntriesCount > index) {
            appResponseLength
              += emberAfGreenPowerClientStoreProxyTableEntry(&entry,
                                                             appResponseData + appResponseLength);
          }
        }
      }
      emberAfSendResponse();
      goto kickout;
    }
  }

  /*

     if (validEntriesCount == 0) {
     emberAfFillCommandGreenPowerClusterGpProxyTableResponse(EMBER_ZCL_GP_PROXY_TABLE_RESPONSE_STATUS_SUCCESS,
                                                            0x00,
                                                            index,
                                                            0x00,
                                                            NULL,
                                                            0);
     emberAfSendResponse();
     emberAfGreenPowerClusterPrintln("send empty");
     goto kickout;
     } else {
     if (requestType == EMBER_ZCL_GP_PROXY_TABLE_REQUEST_OPTIONS_REQUEST_TYPE_BY_GPD_ID) {
      if (index > validEntriesCount) {
        emberAfFillCommandGreenPowerClusterGpProxyTableResponse(EMBER_ZCL_GP_PROXY_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                validEntriesCount,
                                                                index,
                                                                0x00,
                                                                NULL,
                                                                0);
      } else {
        if (validEntryIndexAtIndex != 0xFF) {
          EmberGpProxyTableEntry entry;
          emberGpProxyTableGetEntry(validEntryIndexAtIndex, &entry);
          appResponseLength
   += emberAfGreenPowerClientStoreProxyTableEntry(&entry,
                                                           appResponseData);
        }
      }

     emberAfGreenPowerClusterPrintln("send full");
      emberAfSendResponse();
     }
     }

   */
  kickout:  return true;
}

bool emAfPluginGreenPowerClientRetrieveAttributeAndCraftResponse(uint8_t endpoint,
                                                                 EmberAfClusterId clusterId,
                                                                 EmberAfAttributeId attrId,
                                                                 uint8_t mask,
                                                                 uint16_t manufacturerCode,
                                                                 uint16_t readLength)
{
  if (endpoint != GP_ENDPOINT
      || clusterId != ZCL_GREEN_POWER_CLUSTER_ID
      || attrId != ZCL_GP_CLIENT_PROXY_TABLE_ATTRIBUTE_ID
      || mask != CLUSTER_MASK_CLIENT
      || manufacturerCode != EMBER_AF_NULL_MANUFACTURER_CODE) {
    return false;
  }

  emberAfPutInt16uInResp(attrId);
  emberAfPutInt8uInResp(EMBER_ZCL_STATUS_SUCCESS);
  emberAfPutInt8uInResp(ZCL_LONG_OCTET_STRING_ATTRIBUTE_TYPE);

  // The proxy table attribute is a long string ZCL attribute type, which means
  // it is encoded starting with a 2-byte length. We fill in the real length
  // after we have encoded the whole proxy table.
  uint8_t *finger = appResponseData + appResponseLength + 2;

  for (uint8_t i = 0; i < EMBER_GP_PROXY_TABLE_SIZE; i++) {
    EmberGpProxyTableEntry entry;
    if (emberGpProxyTableGetEntry(i, &entry) != EMBER_SUCCESS) {
      return false;
    }
    if (entry.status != EMBER_GP_PROXY_TABLE_ENTRY_STATUS_UNUSED) {
      finger += emberAfGreenPowerClientStoreProxyTableEntry(&entry, finger);
    }
  }

  // See comment above.
  uint16_t stringLength = (uint16_t)(finger
                                     - (appResponseData
                                        + appResponseLength
                                        + 2));
  emberAfPutInt16uInResp(stringLength);
  appResponseLength += stringLength;

  return true;
}

bool emberAfGreenPowerClusterAutoCommissioningCallback(GP_PARAMS)
{
  uint16_t options = 0;
  uint8_t UNUSED proxyIndex = 0xFF;
  EmberApsFrame *apsFrame = NULL;
  EmberStatus retval;

  if (!commissioningState.inCommissioningMode) {
    return true;
  }

  if (addr->applicationId == EMBER_GP_APPLICATION_SOURCE_ID
      && addr->id.sourceId == 0x00000000) {
    return true;
  }
  if (gpdCommandId == EMBER_ZCL_GP_GPDF_COMMISSIONING) {
    return true;
  }

  if (rxAfterTx) {
    return true;
  }

  /* TODO
     proxyIndex = emberGpProxyTableLookup(addr);
     if (proxyIndex != 0xFF) {
     //we have a proxy table entry for this GPD
     emGpProxyTableSetInRange(proxyIndex);
     } else {
     //we don't have a proxy table entry
     //optionally create an active invalid entry here
     //not required for proxy basic
     }
   */

  if (status == EMBER_NO_SECURITY) {
    options = 0;
  }
  //TODO handle other cases

  options |= addr->applicationId;
  options |= gpdfSecurityLevel
             << GP_COMMISSIONING_SECURITY_LEVEL_TO_OPTIONS_SHIFT;
  options |= gpdfSecurityKeyType
             << GP_COMMISSIONING_SECURITY_KEY_TYPE_TO_OPTIONS_SHIFT; //security key type
  options |= EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_PROXY_INFO_PRESENT;

  emberAfFillCommandGreenPowerClusterGpCommissioningNotificationSmart(options,
                                                                      addr->id.sourceId,
                                                                      addr->id.gpdIeeeAddress,
                                                                      addr->endpoint,
                                                                      sequenceNumber,
                                                                      gpdfSecurityLevel,
                                                                      gpdSecurityFrameCounter,
                                                                      gpdCommandId,
                                                                      gpdCommandPayloadLength,
                                                                      gpdCommandPayload,
                                                                      emberGetNodeId(),
                                                                      0xFF,
                                                                      mic);
  apsFrame = emberAfGetCommandApsFrame();
  apsFrame->sourceEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
  apsFrame->destinationEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
  emSetAddDelay(GP_DMIN_U);
  if (commissioningState.unicastCommunication) {
    retval = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                       commissioningState.commissioningSink);
  } else {
    emberAfGreenPowerClusterPrintln("sending broadcast with alias");
    apsFrame->sequence = sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET;
    apsFrame->options |= EMBER_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER;
    retval = emberAfSendCommandBroadcastWithAlias(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                                                  emGpdAlias(addr),
                                                  sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET);
  }
  emSetAddDelay(0);
  emberAfGreenPowerClusterPrintln("send returned %d", retval);
  return true;
}

uint8_t qualityBasedDelay(uint8_t gpdLink)
{
  uint8_t ourLqi = (gpdLink & 0xC0) >> 5;
  return ((3 - ourLqi) << 5);
}

bool emberAfGreenPowerClusterCommissioningGpdfCallback(GP_PARAMS)
{
  EmberApsFrame *apsFrame;

  // Per GP spec 14-0563-08:
  // Commissioning: Step 12a: Proxy receives commissioning command
  // If applicationId == 0b0000 and srcId == 0 or reserved range 0xfffffff9 - 0xffffffffe
  // OR applicationId == 0b0010 and srcId == 0, drop frame.
  if ((addr->applicationId == EMBER_GP_APPLICATION_SOURCE_ID)
      && (addr->id.sourceId == GP_GPD_SRC_ID_RESERVED_0)) {
    // silently drop frame
    goto kickout;
  }

  if ((addr->applicationId == EMBER_GP_APPLICATION_SOURCE_ID)
      && (addr->id.sourceId >=  GP_GPD_SRC_ID_RESERVED_FFFFFF9)
      && (addr->id.sourceId <=  GP_GPD_SRC_ID_RESERVED_FFFFFFE)) {
    // silently drop frame as in reserved range
    goto kickout;
  }

  if (addr->applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
    uint8_t i;
    for (i = 0; i < EUI64_SIZE; i++) {
      if (addr->id.gpdIeeeAddress[i]) {
        break;
      }
    }
    if (i == EUI64_SIZE) {
      // silently drop frame if EUI is all zeros
      goto kickout;
    }
  }

  if (autoCommissioning) {
    goto kickout;
  }

  // Step b
  if ((gpdfSecurityLevel == EMBER_GP_SECURITY_LEVEL_FC_MIC)
      || (gpdfSecurityLevel == EMBER_GP_SECURITY_LEVEL_FC_MIC_ENCRYPTED)) {
    if (status == EMBER_AUTH_FAILURE) {
      // fwd frame with Security processing failed bit set as part of GP
      // Commissioning Notification.
      goto send_notification;
    }
  }

  // Step c
  // RxAfterTx is true
  {
    bool frameFound = false;
    if (rxAfterTx && (gpdCommandId == EMBER_ZCL_GP_GPDF_COMMISSIONING)) {
      if (addr->applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
        // TODO: check endpoint
      }
    }

    if (frameFound) {
      // send at least one Commissioning Reply GPDF
      //XXX compare to where we do this now
    }
  }

  // Step d
  /* TODO
     {
     uint8_t gpProxyTableIndex = emberGpProxyTableLookup(addr);
     if (gpProxyTableIndex != 0xFF) {
      emGpProxyTableSetInRange(gpProxyTableIndex);
     } else {
       //basic proxy doesn't have to do anything in this situation
     }

     }
   */

  // Step e: sending GP Commissioning Notification
  send_notification: {
    uint16_t options = addr->applicationId;
    if (rxAfterTx) {
      options |= EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX;
    }

    if (status == EMBER_AUTH_FAILURE) {
      options |=
        EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_PROCESSING_FAILED;
      options |= (gpdfSecurityKeyType ? 0x4 : 0) // see 1.0 spec line 3040
                 << GP_COMMISSIONING_SECURITY_KEY_TYPE_TO_OPTIONS_SHIFT;
    } else {
      options |= gpdfSecurityKeyType
                 << GP_COMMISSIONING_SECURITY_KEY_TYPE_TO_OPTIONS_SHIFT;
    }

    options |= gpdfSecurityLevel
               << GP_COMMISSIONING_SECURITY_LEVEL_TO_OPTIONS_SHIFT;
    options |= EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_PROXY_INFO_PRESENT;

    UNUSED EmberStatus retval;

    if (commissioningState.unicastCommunication) {
      emberAfFillCommandGreenPowerClusterGpCommissioningNotificationSmart(options,
                                                                          addr->id.sourceId,
                                                                          addr->id.gpdIeeeAddress,
                                                                          addr->endpoint,
                                                                          sequenceNumber,
                                                                          gpdfSecurityLevel,
                                                                          gpdSecurityFrameCounter,
                                                                          gpdCommandId,
                                                                          gpdCommandPayloadLength,
                                                                          gpdCommandPayload,
                                                                          emberGetNodeId(),
                                                                          gpdLink,
                                                                          mic);
      if (rxAfterTx) {
        emSetAddDelay(GP_DMIN_B);
      } else {
        emSetAddDelay(GP_DMIN_U);
      }
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      apsFrame->destinationEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
      retval = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                         commissioningState.commissioningSink);
      emSetAddDelay(0);
      emberAfGreenPowerClusterPrintln("commission send to %d returned %d", commissioningState.commissioningSink, retval);
    } else {
      emberAfFillCommandGreenPowerClusterGpCommissioningNotificationSmart(options,
                                                                          addr->id.sourceId,
                                                                          addr->id.gpdIeeeAddress,
                                                                          addr->endpoint,
                                                                          sequenceNumber,
                                                                          gpdfSecurityLevel,
                                                                          gpdSecurityFrameCounter,
                                                                          gpdCommandId,
                                                                          gpdCommandPayloadLength,
                                                                          gpdCommandPayload,
                                                                          emberGetNodeId(),
                                                                          gpdLink,
                                                                          mic);
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      apsFrame->destinationEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
      apsFrame->sequence = sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET;
      apsFrame->options |= EMBER_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER;
      if (rxAfterTx) {
        emSetAddDelay(GP_DMIN_B + qualityBasedDelay(gpdLink));
      } else {
        emSetAddDelay(GP_DMIN_U);
      }

      retval = emberAfSendCommandBroadcastWithAlias(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                                                    emGpdAlias(addr),
                                                    sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET);
      emSetAddDelay(0);
      emberAfGreenPowerClusterPrintln("Broadcast send returned %d", retval);
    }
  }

  kickout: return true;
}

bool emberAfGreenPowerClusterDecommissioningGpdfCallback(GP_PARAMS)
{
  return emberAfGreenPowerClusterCommissioningGpdfCallback(GP_ARGS);
}

bool emberAfGreenPowerClusterSuccessGpdfCallback(GP_PARAMS)
{
  EmberApsFrame *apsFrame;
  EmberStatus retval;
  uint16_t options = addr->applicationId;
  emberAfGreenPowerClusterPrintln("got success");
  if (commissioningState.unicastCommunication) {
    emberAfFillCommandGreenPowerClusterGpCommissioningNotificationSmart(options,
                                                                        addr->id.sourceId,
                                                                        addr->id.gpdIeeeAddress,
                                                                        addr->endpoint,
                                                                        sequenceNumber,
                                                                        gpdfSecurityLevel,
                                                                        gpdSecurityFrameCounter,
                                                                        gpdCommandId,
                                                                        gpdCommandPayloadLength,
                                                                        gpdCommandPayload,
                                                                        emberGetNodeId(),
                                                                        0xFF,
                                                                        mic);
    emSetAddDelay(GP_DMIN_U);
    apsFrame = emberAfGetCommandApsFrame();
    apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
    apsFrame->destinationEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
    retval = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                       commissioningState.commissioningSink);
    emSetAddDelay(0);
    emberAfGreenPowerClusterPrintln("success send to %d returned %d", commissioningState.commissioningSink, retval);
  } else {
    emberAfFillCommandGreenPowerClusterGpCommissioningNotificationSmart(options,
                                                                        addr->id.sourceId,
                                                                        addr->id.gpdIeeeAddress,
                                                                        addr->endpoint,
                                                                        sequenceNumber,
                                                                        gpdfSecurityLevel,
                                                                        gpdSecurityFrameCounter,
                                                                        gpdCommandId,
                                                                        gpdCommandPayloadLength,
                                                                        gpdCommandPayload,
                                                                        emberGetNodeId(),
                                                                        0xFF,
                                                                        mic);
    apsFrame = emberAfGetCommandApsFrame();
    apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
    apsFrame->destinationEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
    apsFrame->sequence = sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET;
    apsFrame->options |= EMBER_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER;
    emSetAddDelay(GP_DMIN_U);
    retval = emberAfSendCommandBroadcastWithAlias(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                                                  emGpdAlias(addr),
                                                  sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET);
    emSetAddDelay(0);
    emberAfGreenPowerClusterPrintln("success send returned %d", retval);
  }
  return true;
}

bool emberAfGreenPowerClusterChannelRequestGpdfCallback(GP_PARAMS)
{
  if (!commissioningState.inCommissioningMode) {
    return true;
  }

  if (commissioningState.onTransmitChannel
      && commissioningState.onTransmitChannel != emberAfGetRadioChannel()) {
    // if we're on the operational channel, we forward'
    //send the request back
    //TODO switch to TX, and between after gpTxOffset, send the channel configuration gpdf
    //return to operational channel
  } else {
    //TODO: check for a reason not to forward (busy, etc)

    uint16_t options = 0; //always uses appID srcID becase srcID is absent was addr.applicationId;

    //rxAfter Tx always set for channel request
    options |=
      EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX;

    if (status == EMBER_AUTH_FAILURE) {
      options |=
        EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_PROCESSING_FAILED;
    }

    options |= gpdfSecurityLevel
               << GP_COMMISSIONING_SECURITY_LEVEL_TO_OPTIONS_SHIFT;
    options |= gpdfSecurityKeyType
               << GP_COMMISSIONING_SECURITY_KEY_TYPE_TO_OPTIONS_SHIFT;
    options |= EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_PROXY_INFO_PRESENT;

    EmberStatus retval;
    EmberApsFrame *apsFrame;

    emberAfGreenPowerClusterPrintln("sending GP CN from: %2x with payload %1x", emberGetNodeId(), gpdCommandPayload[0]);

    if (commissioningState.unicastCommunication) {
      emberAfFillCommandGreenPowerClusterGpCommissioningNotificationSmart(options,
                                                                          0x00000000, //addr.id.sourceId,
                                                                          NULL, //addr.id.gpdIeeeAddress,
                                                                          0, //addr.endpoint,
                                                                          sequenceNumber,
                                                                          gpdfSecurityLevel,
                                                                          gpdSecurityFrameCounter,
                                                                          gpdCommandId,
                                                                          gpdCommandPayloadLength,
                                                                          gpdCommandPayload,
                                                                          emberGetNodeId(),
                                                                          0xFF,
                                                                          mic);
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      apsFrame->destinationEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
      emSetAddDelay(GP_DMIN_B);
      retval = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                         commissioningState.commissioningSink);
      emSetAddDelay(0);
      emberAfGreenPowerClusterPrintln("send returned %d", retval);
    } else {
      emberAfFillCommandGreenPowerClusterGpCommissioningNotificationSmart(options,
                                                                          0x00000000, //addr.id.sourceId,
                                                                          NULL, //addr.id.gpdIeeeAddress,
                                                                          0, //addr.endpoint,
                                                                          sequenceNumber,
                                                                          gpdfSecurityLevel,
                                                                          gpdSecurityFrameCounter,
                                                                          gpdCommandId,
                                                                          gpdCommandPayloadLength,
                                                                          gpdCommandPayload,
                                                                          emberGetNodeId(),
                                                                          0xFF,
                                                                          mic);
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      apsFrame->destinationEndpoint = GP_ENDPOINT; //emberAfCurrentEndpoint();
      apsFrame->sequence = sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET;
      apsFrame->options |= EMBER_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER;
      emSetAddDelay(GP_DMIN_B + qualityBasedDelay(gpdLink));
      retval = emberAfSendCommandBroadcastWithAlias(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS,
                                                    emGpdAlias(addr),
                                                    sequenceNumber - EMBER_GP_COMMISSIONING_NOTIFICATION_SEQUENCE_NUMBER_OFFSET);
      emSetAddDelay(0);
      emberAfGreenPowerClusterPrintln("send returned %d", retval);
    }
  }

  return true;
}

bool emberAfGreenPowerClusterGpdfForwardCallback(GP_PARAMS)
{
  uint8_t i;
  uint16_t options = 0;
  uint16_t alias = 0;
  EmberApsFrame *apsFrame;

  if (!sinkList) {
    return true;
  }
  uint8_t proxyTableIndex = emberGpProxyTableLookup(addr);
  EmberGpProxyTableEntry entry;
  if (proxyTableIndex != 0xFF) {
#ifndef EZSP_HOST
    //TODO, we need to decide how to do this for host
    emGpProxyTableSetFirstToForward(proxyTableIndex);
    emGpProxyTableSetInRange(proxyTableIndex);    //TODO is this the right place for this?
#endif
    if (emberGpProxyTableGetEntry(proxyTableIndex, &entry) != EMBER_SUCCESS) {
      return true;
    }
  } else {
    return true;
  }

  options |= addr->applicationId;

  //Traverse the sink list to set the options flags
  for (i = 0; i < GP_SINK_LIST_ENTRIES; i++) {
    if (sinkList[i].type == EMBER_GP_SINK_TYPE_FULL_UNICAST
        || sinkList[i].type == EMBER_GP_SINK_TYPE_LW_UNICAST) {
      options |= EMBER_AF_GP_NOTIFICATION_OPTION_ALSO_UNICAST;
    } else if (sinkList[i].type == EMBER_GP_SINK_TYPE_GROUPCAST) {
      options |= EMBER_AF_GP_NOTIFICATION_OPTION_ALSO_COMMISSIONED_GROUP;
    }
  }

  if (entry.options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_DERIVED_GROUP_GPS) {
    options |= EMBER_AF_GP_NOTIFICATION_OPTION_ALSO_DERIVED_GROUP;
  }

  options |= gpdfSecurityLevel
             << EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET;

  options |= gpdfSecurityKeyType
             << EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE_OFFSET;

  //Basic proxy always sends queue full for data gpdfs (not commissioning)
  options |= EMBER_AF_GP_NOTIFICATION_OPTION_GP_TX_QUEUE_FULL;

  // Basic Proxy implementing the latest spec always sets this
  options |= EMBER_AF_GP_NOTIFICATION_OPTION_PROXY_INFO_PRESENT;

  // comm/decomm frames ignore rx after tx bit in operational mode
  if (((gpdCommandId != EMBER_ZCL_GP_GPDF_COMMISSIONING
        && gpdCommandId != EMBER_ZCL_GP_GPDF_DECOMMISSIONING)
       || commissioningState.inCommissioningMode)
      && rxAfterTx) {
    options |= EMBER_AF_GP_NOTIFICATION_OPTION_RX_AFTER_TX;
  }

  if (  gpdfSecurityLevel == EMBER_GP_SECURITY_KEY_NONE) {
    // If no security use the MAC sequence number instead
    gpdSecurityFrameCounter = (uint32_t)(sequenceNumber & 0x000000ff);
  }

  emberAfFillCommandGreenPowerClusterGpNotificationSmart(options,
                                                         addr->id.sourceId,
                                                         addr->id.gpdIeeeAddress,
                                                         addr->endpoint,
                                                         gpdSecurityFrameCounter,
                                                         gpdCommandId,
                                                         gpdCommandPayloadLength,
                                                         gpdCommandPayload,
                                                         emberGetNodeId(),
                                                         0xFF);

  for (i = 0; i < GP_SINK_LIST_ENTRIES; i++) {
    if (sinkList[i].type == EMBER_GP_SINK_TYPE_FULL_UNICAST
        || sinkList[i].type == EMBER_GP_SINK_TYPE_LW_UNICAST) {
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      apsFrame->destinationEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      if (rxAfterTx) {
        emSetAddDelay(GP_DMIN_B);
      } else {
        emSetAddDelay(GP_DMIN_U);
      }
      UNUSED EmberStatus retval = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT,
                                                            sinkList[i].target.unicast.sinkNodeId);
      emSetAddDelay(0);
      emberAfGreenPowerClusterPrintln("send uicast type %d to %2x returned %d", sinkList[i].type, sinkList[i].target.unicast.sinkNodeId, retval);
    } else if (sinkList[i].type == EMBER_GP_SINK_TYPE_GROUPCAST) {
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      apsFrame->destinationEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
      apsFrame->sequence = sequenceNumber - EMBER_GP_NOTIFICATION_COMMISSIONED_GROUPCAST_SEQUENCE_NUMBER_OFFSET;
      apsFrame->options |= EMBER_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER;
      apsFrame->radius = entry.groupcastRadius;
      if (rxAfterTx) {
        emSetAddDelay(GP_DMIN_B + qualityBasedDelay(gpdLink));
      } else {
        emSetAddDelay(GP_DMIN_U);
      }
      UNUSED EmberStatus retval = emberAfSendCommandMulticastWithAlias(sinkList[i].target.groupcast.groupID,
                                                                       (sinkList[i].target.groupcast.alias == 0xFFFF)
                                                                       ? emGpdAlias(addr)
                                                                       : sinkList[i].target.groupcast.alias,
                                                                       sequenceNumber - EMBER_GP_NOTIFICATION_COMMISSIONED_GROUPCAST_SEQUENCE_NUMBER_OFFSET);
      emSetAddDelay(0);
      emberAfGreenPowerClusterPrintln("Groupcast send returned %d", retval);
    }
  }
  if (entry.options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_DERIVED_GROUP_GPS) {
    apsFrame = emberAfGetCommandApsFrame();
    apsFrame->sourceEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
    apsFrame->destinationEndpoint = GP_ENDPOINT;  //emberAfCurrentEndpoint();
    apsFrame->sequence = sequenceNumber;
    apsFrame->options |= EMBER_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER;
    apsFrame->radius = entry.groupcastRadius;
    if (rxAfterTx) {
      emSetAddDelay(GP_DMIN_B + qualityBasedDelay(gpdLink));
    } else {
      emSetAddDelay(GP_DMIN_U);
    }

    //get derived alias for address alias and group ID alias
    if (entry.options & EMBER_AF_GP_PROXY_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS) {
      alias = entry.assignedAlias;
    } else {
      alias = emGpdAlias(addr);
    }

    UNUSED EmberStatus retval = emberAfSendCommandMulticastWithAlias((EmberMulticastId)emGpdAlias(addr),
                                                                     (EmberNodeId)alias,
                                                                     sequenceNumber);
    emSetAddDelay(0);
    emberAfGreenPowerClusterPrintln("Groupcast send returned %d", retval);
  }
  return true;
}

/*
 * Check if a EmberGpAddress entry is being used by checking each elemental
 * sequence number's corresponding expiration time field.
 */
bool emAfGreenPowerIsGpAddrUsed(EmberAfGreenPowerDuplicateFilter * filter,
                                uint8_t index)
{
  uint32_t * expirationTimes;
  uint8_t i;
  uint32_t curTime = halCommonGetInt32uMillisecondTick();

  if (index >= EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_ADDR_ENTRIES) {
    return true;
  }
  expirationTimes = (uint32_t *) &(filter->expirationTimes[index]);

  for (i = 0;
       i < EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_SEQ_NUM_ENTRIES_PER_ADDR;
       i++) {
    if (expirationTimes[i] > curTime) {
      return true;
    }
  }

  return false;
}

uint32_t emAfGreenPowerFindNextExpirationTime(EmberAfGreenPowerDuplicateFilter * filter)
{
  uint32_t expirationTime = MAX_INT32U_VALUE;
  uint8_t addrIndex;
  uint8_t entryIndex;
  uint32_t curTime = halCommonGetInt32uMillisecondTick();
  for (addrIndex = 0;
       addrIndex < EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_ADDR_ENTRIES;
       addrIndex++) {
    for (entryIndex = 0;
         entryIndex
         < EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_SEQ_NUM_ENTRIES_PER_ADDR;
         entryIndex++) {
      if ((filter->expirationTimes[addrIndex][entryIndex] > curTime)
          && (filter->expirationTimes[addrIndex][entryIndex] < expirationTime)) {
        expirationTime = filter->expirationTimes[addrIndex][entryIndex];
      }
    }
  }

  return expirationTime;
}

/*
 * return index to the entry with the smallest expiration date.
 */
uint8_t emAfGreenPowerFindEarliestExpirationTimeInAddr(EmberAfGreenPowerDuplicateFilter * filter,
                                                       uint8_t addrIndex)
{
  uint32_t * expirationTimes = filter->expirationTimes[addrIndex];
  uint32_t earliestTimeIndex = 0;
  uint32_t earliestTime = expirationTimes[0];
  uint8_t index;

  for (index = 0;
       index < EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_SEQ_NUM_ENTRIES_PER_ADDR;
       index++) {
    if (expirationTimes[index] < earliestTime) {
      earliestTime = expirationTimes[index];
      earliestTimeIndex = index;
    }
  }

  return earliestTimeIndex;
}

bool emAfGreenPowerAddRandomMacSeqNum(EmberAfGreenPowerDuplicateFilter * filter,
                                      uint8_t addrIndex,
                                      uint8_t randomSeqNum)
{
  uint8_t * seqNumList = NULL;
  uint32_t * expirationTimeList = NULL;
  uint8_t i;
  uint32_t curTime = halCommonGetInt32uMillisecondTick();
  uint8_t entryIndex;

  seqNumList = (filter->randomSeqNums[addrIndex]);
  expirationTimeList = (filter->expirationTimes[addrIndex]);

  // check if an existing sequence number with valid expiration date exists
  for (i = 0;
       i < EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_SEQ_NUM_ENTRIES_PER_ADDR;
       i++) {
    if ((randomSeqNum == seqNumList[i]) && (expirationTimeList[i] > curTime)) {
      emberAfGreenPowerClusterPrintln("drop frame due to active duplicate sequence number at addr[%d], entry[%d], expirationTime(0x%4X), curTime(0x%4X)", addrIndex, i, expirationTimeList[i], curTime);
      return false;
    }
  }

  entryIndex = emAfGreenPowerFindEarliestExpirationTimeInAddr(filter,
                                                              addrIndex);

  seqNumList[entryIndex] = randomSeqNum;
  expirationTimeList[entryIndex] = curTime + (2 * MILLISECOND_TICKS_PER_MINUTE);
  emberAfGreenPowerClusterPrintln("sequenceNumber(%d), expirationTime(0x%4X) is added to addr[%d], entry[%d] at curTime(0x%4X)", randomSeqNum, expirationTimeList[entryIndex], addrIndex, entryIndex, curTime);

  return true;
}

/* Helper function for handling duplicateFilter */
uint8_t emAfGreenPowerFindGpAddrIndex(EmberGpAddress * addr,
                                      EmberGpAddress * addrList,
                                      uint8_t sizeOfList)
{
  uint8_t index = 0;
  for (index = 0; index < sizeOfList; index++) {
    if (emGpAddressMatch(addr, &addrList[index])) {
      return index;
    }
  }

  return 0xFF;
}

uint8_t emAfGreenPowerAddGpAddr(EmberAfGreenPowerDuplicateFilter * filter,
                                EmberGpAddress * addr)
{
  uint8_t addrIndex;

  for (addrIndex = 0;
       addrIndex < EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_ADDR_ENTRIES;
       addrIndex++) {
    if (!emAfGreenPowerIsGpAddrUsed(&duplicateFilter, addrIndex)) {
      MEMCOPY(&filter->addrs[addrIndex], addr, sizeof(EmberGpAddress));
      return addrIndex;
    }
  }

  return 0xFF;
}

/*
 * Check if the incoming message is a duplicate message when the GPD's
 * Mac Sequence Number Capability indicate that random Mac Sequence Number is
 * used.
 *
 *
 * @return
 * If a duplicate mac sequence number has been found, we'll return true in
 * order to reject the message. Otherwise, we'll save the new random mac seq
 * number and return false to accept the message.
 *
 */
bool emAfGreenPowerFindDuplicateMacSeqNum(EmberGpAddress * addr,
                                          uint8_t randomSeqNum)
{
  bool added = false;
  uint8_t addrIndex = emAfGreenPowerFindGpAddrIndex(addr,
                                                    duplicateFilter.addrs,
                                                    EMBER_AF_PLUGIN_GREEN_POWER_CLIENT_MAX_ADDR_ENTRIES);
  if (addrIndex == 0xFF) {
    addrIndex = emAfGreenPowerAddGpAddr(&duplicateFilter, addr);
  }

  if (addrIndex == 0xFF) {
    emberAfGreenPowerClusterPrintln("Unable to store new GP addr", randomSeqNum);
    return false;
  }

  added = emAfGreenPowerAddRandomMacSeqNum(&duplicateFilter,
                                           addrIndex,
                                           randomSeqNum);
  return !added;
}

/*
 * This function implements the "Duplicate checking" and "Freshness check"
 * section of the GP implementation details.
 *
 * @return true if given sequenceNumber is valid and message should be
 *         accepted, false otherwise.
 */
bool emGpMessageChecking(EmberGpAddress *gpAddr, uint8_t sequenceNumber)
{
  /*
     uint8_t proxyTableIndex = emberGpProxyTableLookup(gpAddr);
     EmberGpProxyTableEntry entry;
     EmberGpSecurityLevel securityLevel;
     EmberGpGpdMacSeqNumCap macCap;

     if (proxyTableIndex == 0xFF) {
     return true;
     }

     emberGpProxyTableGetEntry(proxyTableIndex, &entry);
     securityLevel = (EmberGpSecurityLevel) ((entry.options >> 9) & 0x03);
     macCap = (EmberGpGpdMacSeqNumCap) ((entry.options >> 8) & 0x01);

     // Duplicate filtering / Freshness check
     if (securityLevel == EMBER_GP_SECURITY_LEVEL_NONE) {
     // filter by MAC Sequence Number
     if (macCap == EMBER_GP_GPD_MAC_SEQ_NUM_CAP_SEQUENTIAL) {
      uint8_t macSeqNum = (entry.gpdSecurityFrameCounter) & 0xFF;
      if (sequenceNumber <= macSeqNum) {
        emberAfGreenPowerClusterPrintln("Drop frame due to invalid sequence number. incoming(0x%4X), current(0x%4X)", sequenceNumber, macSeqNum);
        return false;
      }
     } else { // EMBER_GP_GPD_MAC_SEQ_NUM_CAP_RANDOM
      if (emAfGreenPowerFindDuplicateMacSeqNum(gpAddr, sequenceNumber)) {
        emberAfGreenPowerClusterPrintln("Drop frame due to duplicate sequence number: 0x%4X", sequenceNumber);
        return false;
      }
     }
     } else if ((securityLevel == EMBER_GP_SECURITY_LEVEL_FC_MIC)
   || (securityLevel == EMBER_GP_SECURITY_LEVEL_FC_MIC_ENCRYPTED)) {
     // filter base on GPD security frame counter
     if (sequenceNumber <= entry.gpdSecurityFrameCounter) {
      emberAfGreenPowerClusterPrintln("Drop frame due to lower sequence number: 0x%4X", sequenceNumber);
      return false;
     }
   */
  return true;
}

//GP-DATA.indication
void emberGpepIncomingMessageHandler(GP_PARAMS)
{
  emberAfGreenPowerClusterPrintln("Gpep Incoming command: %x with status %x SFC: %4x", gpdCommandId, status, gpdSecurityFrameCounter);
  if (commissioningState.onTransmitChannel
      && gpdCommandId != EMBER_ZCL_GP_GPDF_CHANNEL_REQUEST) {
    //drop everything but a channel request when we're off on a different channel
    return;
  }
  if (status == EMBER_SUCCESS
      || status == EMBER_NO_SECURITY
      || (status == EMBER_UNPROCESSED && commissioningState.inCommissioningMode)) {
    // duplicate filter
    if (emGpMessageChecking(addr, sequenceNumber)) {
      if (commissioningState.inCommissioningMode) {
        if (autoCommissioning) {
          emberAfGreenPowerClusterAutoCommissioningCallback(GP_ARGS);
        }
        switch (gpdCommandId) {
          case EMBER_ZCL_GP_GPDF_COMMISSIONING:
            emberAfGreenPowerClusterCommissioningGpdfCallback(GP_ARGS);
            break;
          case EMBER_ZCL_GP_GPDF_DECOMMISSIONING:
            emberAfGreenPowerClusterDecommissioningGpdfCallback(GP_ARGS);
            break;
          case EMBER_ZCL_GP_GPDF_SUCCESS:
            emberAfGreenPowerClusterSuccessGpdfCallback(GP_ARGS);
            break;
          case EMBER_ZCL_GP_GPDF_CHANNEL_REQUEST:
            emberAfGreenPowerClusterChannelRequestGpdfCallback(GP_ARGS);
            break;

          default:
            if (status == EMBER_UNPROCESSED) {
              status = EMBER_AUTH_FAILURE;
              emberAfGreenPowerClusterCommissioningGpdfCallback(GP_ARGS);
            } else {
              emberAfGreenPowerClusterGpdfForwardCallback(GP_ARGS);
            }
            break;
        }
      } else {
        //outside of commissioning mode, everything gets forwarded, including commissioning related messages
        emberAfGreenPowerClusterGpdfForwardCallback(GP_ARGS);
      }
    }
  }
}

void emberDGpSentHandler(EmberStatus status, uint8_t gpepHandle)
{
  if (status == EMBER_SUCCESS && commissioningState.onTransmitChannel) {
    emberAfGreenPowerClusterPrintln("returning to channel %d", commissioningState.onTransmitChannel);
    emberSetRadioChannel(commissioningState.onTransmitChannel);
    commissioningState.onTransmitChannel = 0;
  }
}

#ifdef EZSP_HOST
EmberStatus ezspConditionalGpProxyTableGetEntry(uint8_t proxyIndex, EmberGpProxyTableEntry *entry)
{
  if (greenPowerActive) {
    return ezspGpProxyTableGetEntry(proxyIndex, entry);
  } else {
    return EMBER_INVALID_CALL;
  }
}

uint8_t ezspConditionalGpProxyTableLookup(EmberGpAddress *addr)
{
  if (greenPowerActive) {
    return ezspGpProxyTableLookup(addr);
  } else {
    return 0xFF;
  }
}
#endif
