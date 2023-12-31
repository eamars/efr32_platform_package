// MAC Mode Device Sample Application
//
// Copyright 2017 Silicon Laboratories, Inc.                                *80*

#if !defined(UNIX_HOST) && !defined(CORTEXM3_EMBER_MICRO)
#include "em_device.h"
#include "em_chip.h"
#include "bsp.h"
#endif

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER

#include "stack/include/ember.h"
#ifndef UNIX_HOST
#include "heartbeat/heartbeat.h"
#endif
#include "hal/hal.h"
#include "command-interpreter/command-interpreter2.h"
#include "debug-print/debug-print.h"

#define NETWORK_UP_LED BOARDLED0

static EmberMessageOptions txOptions = EMBER_OPTIONS_ACK_REQUESTED;

//------------------------------------------------------------------------------
// Callbacks

void emberAfChildJoinCallback(EmberNodeType nodeType,
                              EmberNodeId nodeId)
{
  emberAfCorePrintln("Node joined with short address 0x%2x", nodeId);
}

// MAC mode incoming message handler
void emberAfIncomingMacMessageCallback(EmberIncomingMacMessage *message)
{
  uint8_t i;
  if (message->macFrame.srcAddress.mode == EMBER_MAC_ADDRESS_MODE_SHORT) {
    emberAfCorePrint("MAC RX: Data from 0x%2x:{",
                     message->macFrame.srcAddress.addr.shortAddress);
  } else if (message->macFrame.srcAddress.mode == EMBER_MAC_ADDRESS_MODE_NONE) {
    emberAfCorePrint("MAC RX: Data from unspecified address: {");
  } else {
    // print long address
    emberAfCorePrint("MAC RX: Data from ");
    for ( i = 0; i < EUI64_SIZE; i++ ) {
      emberAfCorePrint("%x", message->macFrame.srcAddress.addr.longAddress[i]);
    }
    emberAfCorePrint(":{");
  }
  for ( i = 0; i < message->length; i++ ) {
    emberAfCorePrint(" %x", message->payload[i]);
  }
  emberAfCorePrintln("}");
}

// MAC mode message sent handler
void emberAfMacMessageSentCallback(EmberStatus status,
                                   EmberOutgoingMacMessage *message)
{
  if ( status != EMBER_SUCCESS ) {
    emberAfCorePrintln("MAC TX: 0x%x", status);
  }
}

// This callback is called when the application starts and can be used to
// perform any additional initialization required at system startup.
void emberAfMainInitCallback(void)
{
  emberAfCorePrintln("Powered UP");
  emberAfCorePrintln("\n%p>", EMBER_AF_DEVICE_NAME);

  emberNetworkInit();
}

// This callback is called in each iteration of the main application loop and
// can be used to perform periodic functions.
void emberAfMainTickCallback(void)
{
  #ifndef UNIX_HOST
  if (emberStackIsUp()) {
    halSetLed(NETWORK_UP_LED);
  } else {
    halClearLed(NETWORK_UP_LED);
  }
  #endif
}

void emberAfStackStatusCallback(EmberStatus status)
{
  switch ( status ) {
    case EMBER_NETWORK_UP:
      emberAfCorePrintln("Network up");
      break;
    case EMBER_NETWORK_DOWN:
      emberAfCorePrintln("Network down");
      break;
    default:
      emberAfCorePrintln("Stack status: 0x%x", status);
      break;
  }
}

void emberAfIncomingBeaconCallback(EmberPanId panId,
                                   EmberNodeId nodeId,
                                   int8_t rssi,
                                   bool permitJoining,
                                   uint8_t payloadLength,
                                   uint8_t *payload)
{
  emberAfCorePrint("BEACON: panId 0x%2X nodeId 0x%2X payload {", panId, nodeId);
  emberAfCorePrintBuffer(payload, payloadLength, FALSE);
  emberAfCorePrintln("}");
}

void emberAfEnergyScanCompleteCallback(int8_t mean,
                                       int8_t min,
                                       int8_t max,
                                       uint16_t variance)
{
  emberAfCorePrintln("Energy scan complete, mean=%d min=%d max=%d var=%d",
                     mean, min, max, variance);
}

void emberAfActiveScanCompleteCallback(void)
{
  emberAfCorePrintln("Active scan complete");
}

// ------------------------ CLI commands ---------------------------------------
void setSecurityKeyCommand(void)
{
  EmberKeyData key;
  emberCopyKeyArgument(0, &key);

  if (emberSetSecurityKey(&key) == EMBER_SUCCESS) {
    uint8_t i;

    emberAfCorePrint("Security key set {");
    for (i = 0; i < EMBER_ENCRYPTION_KEY_SIZE; i++) {
      emberAfCorePrint("%x", key.contents[i]);
    }
    emberAfCorePrintln("}");
  } else {
    emberAfCorePrintln("Security key set failed");
  }
}

void setBeaconPayloadCommand(void)
{
  EmberStatus status;
  uint8_t length;
  uint8_t *contents = emberStringCommandArgument(0, &length);

  status = emberSetApplicationBeaconPayload(length, contents);

  emberAfCorePrint("Set beacon payload: {");
  emberAfCorePrintBuffer(contents, length, FALSE);
  emberAfCorePrintln("}: status=0x%x", status);
}

void joinCommissionedCommand(void)
{
  EmberNetworkParameters parameters;
  EmberStatus status;
  uint8_t nodeType = emberUnsignedCommandArgument(0);
  EmberNodeId nodeId = emberUnsignedCommandArgument(1);
  parameters.panId = emberUnsignedCommandArgument(2);
  parameters.radioTxPower = emberSignedCommandArgument(3);
  parameters.radioChannel = emberUnsignedCommandArgument(4);

  status = emberJoinCommissioned(nodeType, nodeId, &parameters);

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Node parameters commissioned");
  } else {
    emberAfCorePrintln("Commissioning failed, status 0x%X", status);
  }
}

void joinNetworkCommand(void)
{
  EmberNetworkParameters parameters;
  EmberStatus status;
  uint8_t nodeType = emberUnsignedCommandArgument(0);
  parameters.panId = emberUnsignedCommandArgument(1);
  parameters.radioTxPower = emberSignedCommandArgument(2);
  parameters.radioChannel = emberUnsignedCommandArgument(3);

  status = emberJoinNetwork(nodeType, &parameters);

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Started the joining process");
  } else {
    emberAfCorePrintln("Join network failed, status 0x%X", status);
  }
}

void setPermitJoinCommand(void)
{
  uint8_t duration = emberUnsignedCommandArgument(0);
  EmberStatus status = emberPermitJoining(duration);

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Permit join set 0x%X", duration);
  } else {
    emberAfCorePrintln("Permit join failed");
  }
}

void setAllocateAddressFlagCommand(void)
{
  bool allocateAddress = (emberUnsignedCommandArgument(0) > 0);
  EmberStatus status = emberMacSetAllocateAddressFlag(allocateAddress);

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Allocate address flag set %d", allocateAddress);
  } else {
    emberAfCorePrintln("Allocate address flag set failed, 0x%x", status);
  }
}

void setOptionsCommand(void)
{
  txOptions = emberUnsignedCommandArgument(0);
  emberAfCorePrintln("Send options set: 0x%x", txOptions);
}

void pollCommand(void)
{
  EmberStatus status = emberPollForData();

  emberAfCorePrintln("Poll status 0x%x", status);
}

void setPollDestinationCommand(void)
{
  EmberStatus status;
  EmberMacAddress destAddr;

  destAddr.addr.shortAddress = emberUnsignedCommandArgument(0);
  destAddr.mode = EMBER_MAC_ADDRESS_MODE_SHORT;

  if (destAddr.addr.shortAddress == EMBER_NULL_NODE_ID) {
    emberCopyEui64Argument(1, destAddr.addr.longAddress);
    destAddr.mode = EMBER_MAC_ADDRESS_MODE_LONG;
  }

  status = emberSetPollDestinationAddress(&destAddr);

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Poll address set");
  } else {
    emberAfCorePrintln("Poll address set failed, 0x%x", status);
  }
}

// Params:
// 0: a "nibble mask" indicating which fields are specified, specifically:
//    0x000F - source ID mode (0x00 = none, 0x02 = short, 0x03 = long)
//    0x00F0 - destination ID mode (0x00 = none, 0x02 = short, 0x03 = long)
//    0x0F00 - the source pan ID is specified (0x01) or not (0x00).
//    0xF000 - the destination pan ID is specified (0x01) or not (0x00).
// 1: the source short ID (if specified)
// 2: the source long ID (if specified)
// 3: the destination short ID (if specified)
// 4: the destination long ID (if specified)
// 5: the source PAN ID (if specified)
// 6: the destination PAN ID (if specified)
// 7: MAC payload length
void sendCommand(void)
{
  EmberStatus status;
  EmberMacFrame macFrame;
  uint8_t length;
  uint8_t argLength;
  uint16_t macFrameInfo = emberUnsignedCommandArgument(0);
  EmberNodeId shortSrcId = emberUnsignedCommandArgument(1);
  EmberNodeId shortDestId = emberUnsignedCommandArgument(3);
  EmberPanId srcPanId = emberUnsignedCommandArgument(5);
  EmberPanId dstPanId = emberUnsignedCommandArgument(6);
  uint8_t *message = emberStringCommandArgument(7, &length);

  if ((macFrameInfo & 0x000F) == EMBER_MAC_ADDRESS_MODE_SHORT) {
    macFrame.srcAddress.addr.shortAddress = shortSrcId;
    macFrame.srcAddress.mode = EMBER_MAC_ADDRESS_MODE_SHORT;
  } else if ((macFrameInfo & 0x000F) == EMBER_MAC_ADDRESS_MODE_LONG) {
    emberStringCommandArgument(2, &argLength);
    assert(argLength == EUI64_SIZE);
    emberCopyEui64Argument(2, macFrame.srcAddress.addr.longAddress);
    macFrame.srcAddress.mode = EMBER_MAC_ADDRESS_MODE_LONG;
  } else {
    macFrame.srcAddress.mode = EMBER_MAC_ADDRESS_MODE_NONE;
  }

  if (((macFrameInfo & 0x00F0) >> 4) == EMBER_MAC_ADDRESS_MODE_SHORT) {
    macFrame.dstAddress.addr.shortAddress = shortDestId;
    macFrame.dstAddress.mode = EMBER_MAC_ADDRESS_MODE_SHORT;
  } else if (((macFrameInfo & 0x00F0) >> 4) == EMBER_MAC_ADDRESS_MODE_LONG) {
    emberStringCommandArgument(4, &argLength);
    assert(argLength == EUI64_SIZE);
    emberCopyEui64Argument(4, macFrame.dstAddress.addr.longAddress);
    macFrame.dstAddress.mode = EMBER_MAC_ADDRESS_MODE_LONG;
  } else {
    macFrame.dstAddress.mode = EMBER_MAC_ADDRESS_MODE_NONE;
  }

  if (macFrameInfo & 0x0F00) {
    macFrame.srcPanId = srcPanId;
    macFrame.srcPanIdSpecified = true;
  } else {
    macFrame.srcPanIdSpecified = false;
  }

  if (macFrameInfo & 0xF000) {
    macFrame.dstPanId = dstPanId;
    macFrame.dstPanIdSpecified = true;
  } else {
    macFrame.dstPanIdSpecified = false;
  }

  status = emberMacMessageSend(&macFrame,
                               0x00, // messageTag
                               length,
                               message,
                               txOptions);

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("MAC frame submitted");
  } else {
    emberAfCorePrintln("MAC frame submission failed, status=0x%x",
                       status);
  }
}

void infoCommand(void)
{
  emberAfCorePrintln("%p:", EMBER_AF_DEVICE_NAME);
  emberAfCorePrintln("  network state: 0x%x", emberNetworkState());
  emberAfCorePrintln("      node type: 0x%x", emberGetNodeType());
  emberAfCorePrintln("          eui64: >%x%x%x%x%x%x%x%x",
                     emberGetEui64()[7],
                     emberGetEui64()[6],
                     emberGetEui64()[5],
                     emberGetEui64()[4],
                     emberGetEui64()[3],
                     emberGetEui64()[2],
                     emberGetEui64()[1],
                     emberGetEui64()[0]);
  emberAfCorePrintln("        node id: 0x%2x", emberGetNodeId());
  emberAfCorePrintln("         pan id: 0x%2x", emberGetPanId());
  emberAfCorePrintln("        channel: %d", (uint16_t)emberGetRadioChannel());
  emberAfCorePrintln("          power: %d", (int16_t)emberGetRadioPower());
  emberAfCorePrintln("     TX options: MAC acks %s, security %s, priority %s",
                     ((txOptions & EMBER_OPTIONS_ACK_REQUESTED) ? "enabled" : "disabled"),
                     ((txOptions & EMBER_OPTIONS_SECURITY_ENABLED) ? "enabled" : "disabled"),
                     ((txOptions & EMBER_OPTIONS_HIGH_PRIORITY) ? "enabled" : "disabled"));
}

void purgeIndirectCommand(void)
{
  EmberStatus status = emberPurgeIndirectMessages();

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Purge indirect success");
  } else {
    emberAfCorePrintln("Purge indirect failed, 0x%x", status);
  }
}

void activeScanCommand(void)
{
  EmberStatus status;
  uint8_t channelToScan = emberUnsignedCommandArgument(0);
  status = emberStartActiveScan(channelToScan);
  emberAfCorePrintln("Start active scanning: channel %d, status=0x%x", channelToScan, status);
}

void energyScanCommand(void)
{
  EmberStatus status;
  uint8_t channelToScan = emberUnsignedCommandArgument(0);
  uint8_t samples = emberUnsignedCommandArgument(1);
  status = emberStartEnergyScan(channelToScan, samples);
  emberAfCorePrintln("Start energy scanning: channel %d, samples %d, status=0x%x", channelToScan, samples, status);
}

#ifdef UNIX_HOST
void setPhyModeCommand(void)
{
  uint8_t mode = emberUnsignedCommandArgument(0);
  EmberStatus status = emberSetPhyMode(mode);

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("Phy mode change failed, status=0x%x", status);
  } else {
    emberAfCorePrintln("Phy mode changed successfully");
  }
}

void getPhyModeCommand(void)
{
  uint8_t mode = emberGetPhyMode();

  if (mode == 0) {
    emberAfCorePrintln("Phy mode: SubGHz");
  } else if (mode == 1) {
    emberAfCorePrintln("Phy mode: 2.4GHz");
  } else {
    emberAfCorePrintln("Invalid image");
  }
}
#endif
