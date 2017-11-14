// Copyright 2017 Silicon Laboratories, Inc.                                *80*

#include PLATFORM_HEADER //compiler/micro specifics, types

#include "app/framework/include/af.h"
#include "stack/include/ember-types.h"
#include "stack/include/error.h"

#include "hal/hal.h"
#include "app/framework/util/common.h"
#include "app/framework/plugin/device-table/device-table.h"
#include <string.h>
#include "app/framework/plugin-host/coap-server/coap-server.h"
#include "app/framework/plugin/device-table/device-table.h"
#include "app/framework/plugin-host/gateway-relay-coap/gateway-relay-coap-cbor.h"
#include "util/third_party/libcoap/include/coap/pdu.h"

static uint16_t returnCode = COAP_RESPONSE_OK;
#define RETURN_STRING_LENGTH 100

#define CLUSTER_STRING_LENGTH 6
static char clusterString[CLUSTER_STRING_LENGTH];

#define CLUSTER_LIST_START 11

#define ATTRIBIUTE_BUFFER_ATTRIBUTE_ID_LOW_BYTE 0
#define ATTRIBUTE_BUFFER_ATTRIBUTE_ID_HIGH_BYTE 1
#define ATTRIBUTE_BUFFER_SUCCESS_CODE 2
#define ATTRIBUTE_BUFFER_DATA_TYPE 3
#define ATTRIBUTE_BUFFER_DATA_START 4

// read attribute response code.
static void setUpErrorCode(uint16_t error);

// --------------------------------
void emAfGatewayRelayCoapProcessIncomingCommand(uint16_t cluster,
                                                uint8_t commandId,
                                                int16_t portCounter,
                                                uint16_t endpoint);

static void sendAttributeReportMessage(uint16_t device,
                                       uint8_t endpoint,
                                       uint16_t cluster,
                                       uint16_t attribute,
                                       uint32_t data);

void emberAfPluginGatewayRelayCoapInitCallback(void)
{
  setUpErrorCode(COAP_RESPONSE_NOT_FOUND);
}

// device-table callbacks and interface
void emberAfPluginDeviceTableInitialized(void)
{
  uint16_t i;
  // call to coap server to set up the channels.
  for (i = 0; i < PLUGIN_COAP_SERVER_MAX_PORTS; i++) {
    if (emberAfDeviceTableGetNodeIdFromIndex(i)
        != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID) {
      emberAfPluginCoapServerCreateDevice(i);
    }
  }
}

void emberAfPluginDeviceTableIndexRemovedCallback(uint16_t index)
{
  emberAfPluginCoapServerRemoveDevice(index);
}

void emberAfPluginDeviceTableIndexAddedCallback(uint16_t index)
{
  emberAfPluginCoapServerCreateDevice(index);
}

int emberAfPluginCoapServerCurrentPort(void);

static void handleAttributeRequest(uint16_t clusterId,
                                   int portCounter,
                                   uint16_t endpoint);

// response data
static uint8_t returnString[RETURN_STRING_LENGTH] = { 0, };

static void constructEndpointList(void)
{
  emAfPluginGatewayRelayCoapCborWriteInit(returnString, RETURN_STRING_LENGTH - 1);

  if (emAfPluginGatewayRelayCoapCborWriteArrayTag()) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteUint8(1)) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteBreak()) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }

  returnCode = COAP_RESPONSE_DATA;
}

static void constructClusterList(uint16_t deviceIndex)
{
  EmberAfPluginDeviceTableEntry *currentDevice;
  char direction;
  uint8_t i;

  currentDevice = emberAfDeviceTableFindDeviceTableEntry(deviceIndex);

  if (currentDevice->nodeId == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID) {
    setUpErrorCode(COAP_RESPONSE_NOT_FOUND);
    return;
  }

  emAfPluginGatewayRelayCoapCborWriteInit(returnString, RETURN_STRING_LENGTH - 1);

  emAfPluginGatewayRelayCoapCborWriteArrayTag();

  for (i = 0; i < EMBER_AF_PLUGIN_DEVICE_TABLE_CLUSTER_SIZE; i++) {
    if (currentDevice->clusterIds[i] == ZCL_NULL_CLUSTER_ID) {
      emAfPluginGatewayRelayCoapCborWriteBreak();
      returnCode = COAP_RESPONSE_DATA;
      return;
    }

    if (i >= currentDevice->clusterOutStartPosition) {
      direction = 'c';
    } else {
      direction = 's';
    }
    snprintf(clusterString,
             CLUSTER_STRING_LENGTH,
             "%c%x",
             direction,
             currentDevice->clusterIds[i]);
    if (emAfPluginGatewayRelayCoapCborWriteString(clusterString)) {
      setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
      return;
    }
  }
  emAfPluginGatewayRelayCoapCborWriteBreak();
  returnCode = COAP_RESPONSE_DATA;

  return;
}

// take a string of characters and make it into a list.
static void constructResourceList(char *list)
{
  uint8_t i;
  char resourceString[3];

  emAfPluginGatewayRelayCoapCborWriteInit(returnString, RETURN_STRING_LENGTH - 1);

  emAfPluginGatewayRelayCoapCborWriteArrayTag();
  resourceString[1] = 0;
  for (i = 0; i < strlen(list); i++) {
    resourceString[0] = list[i];
    emAfPluginGatewayRelayCoapCborWriteString(resourceString);
  }

  emAfPluginGatewayRelayCoapCborWriteBreak();

  returnCode = COAP_RESPONSE_DATA;
}

EmberStatus checkEndpoint(uint16_t endpoint, uint16_t portCounter)
{
  EmberAfPluginDeviceTableEntry *currentDevice
    = emberAfDeviceTableFindDeviceTableEntry(portCounter);

  if (currentDevice->endpoint != endpoint) {
    return EMBER_BAD_ARGUMENT;
  }

  return EMBER_SUCCESS;
}

static void setUpErrorCode(uint16_t error)
{
  returnCode = error;
  emAfPluginGatewayRelayCoapCborWriteInit(returnString, RETURN_STRING_LENGTH - 1);
}

static bool checkMethod(uint8_t method, uint8_t desiredMethod)
{
  if (method == 0
      || method == desiredMethod) {
    return false;
  } else {
    setUpErrorCode(COAP_RESPONSE_METHOD_NOT_ALLOWED);
    return true;
  }
}

uint16_t emberAfPluginGatewayRelayCoapReturnStringLength(void)
{
  //return strlen(returnString);
  return emAfPluginGatewayRelayCoapCborCurrentWriteDataLength();
}

uint16_t emberAfPluginGatewayRelayCoapReturnCode(void)
{
  return returnCode;
}

void emberAfPluginGatewayRelayCoapCopyReturnString(char *ptr)
{
  MEMCOPY((uint8_t *) ptr,
          (uint8_t *) returnString,
          emAfPluginGatewayRelayCoapCborCurrentWriteDataLength());

  returnString[0] = 0;
}

static bool getUint16Opttok(uint16_t *val16);

static void handleAttributeRequest(uint16_t clusterId, int portCounter, uint16_t endpoint)
{
  uint16_t attributeId;
  uint8_t attributeIdBuffer[2];

  if (!getUint16Opttok(&attributeId)) {
    setUpErrorCode(COAP_RESPONSE_NOT_IMPLEMENTED);
    return;
  }

  attributeIdBuffer[0] = LOW_BYTE(attributeId);
  attributeIdBuffer[1] = HIGH_BYTE(attributeId);

  emberAfFillCommandGlobalClientToServerReadAttributes(clusterId,
                                                       attributeIdBuffer,
                                                       2);
  if (portCounter < 0) {
    // message sent to 5683
    emberAfSetCommandEndpoints(1, 1);
    emberAfDeviceTableCommandIndexSend(endpoint - 1);
  } else {
    // port counter happens to be the index into the device table.
    emberAfSetCommandEndpoints(1, endpoint);
    emberAfDeviceTableCommandIndexSend((uint16_t) portCounter);
  }
}

bool emAfGatewayRelayCoapReadAttributesResponseCallback(EmberAfClusterId clusterId,
                                                        uint8_t* buffer,
                                                        uint16_t bufLen)
{
  uint16_t currentData;
  uint16_t currentAttribute;
  uint8_t status;

  // Note:  for now, we assume the device has a single endpoint.
  uint16_t nodeId = emberGetSender();
  uint16_t device = emberAfDeviceTableGetIndexFromNodeId(nodeId);
  EmberAfPluginDeviceTableEntry *dtEntry;
  uint8_t endpoint;

  if (device == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
    // don't have a valid index.  Need to return.
    return false;
  }

  dtEntry = emberAfDeviceTableFindDeviceTableEntry(device);
  endpoint = dtEntry->endpoint;

  currentAttribute =
    HIGH_LOW_TO_INT(buffer[ATTRIBUTE_BUFFER_ATTRIBUTE_ID_HIGH_BYTE],
                    buffer[ATTRIBIUTE_BUFFER_ATTRIBUTE_ID_LOW_BYTE]);
  status   = buffer[ATTRIBUTE_BUFFER_SUCCESS_CODE];

  if (status != 0x00) {
    return false;
  }

  currentData = buffer[ATTRIBUTE_BUFFER_DATA_START];

  if (bufLen > 5) {
    currentData <<= 8;
    currentData += buffer[5];
  }

  sendAttributeReportMessage(device,
                             endpoint,
                             clusterId,
                             currentAttribute,
                             currentData);

  return false;
}

bool emAfGatewayRelayCoapReportAttributesCallback(EmberAfClusterId clusterId,
                                                  uint8_t * buffer,
                                                  uint16_t bufLen)
{
  emAfGatewayRelayCoapReadAttributesResponseCallback(clusterId,
                                                     buffer,
                                                     bufLen);
  return false;
}

// ----------------------------------------------------------------------------
// gateway rely coap heartbeat code
EmberEventControl emberAfPluginGatewayRelayCoapHeartbeatEventControl;
static uint16_t heartbeatPeriodSeconds = 0;

static void sendAttributeReportMessage(uint16_t device,
                                       uint8_t endpoint,
                                       uint16_t cluster,
                                       uint16_t attribute,
                                       uint32_t data);

void emberAfPluginGatewayRelayCoapHeartbeatEventHandler(void)
{
  uint16_t i;
  EmberAfPluginDeviceTableEntry *pEntry;

  emberEventControlSetDelayMS(emberAfPluginGatewayRelayCoapHeartbeatEventControl,
                              1000 * (uint32_t) heartbeatPeriodSeconds);

  for (i = 0; i < PLUGIN_COAP_SERVER_MAX_PORTS; i++) {
    pEntry = emberAfDeviceTableFindDeviceTableEntry(i);

    if (pEntry->nodeId != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID) {
      sendAttributeReportMessage(i, pEntry->endpoint, 8, 0, 200);
    }
  }
}

void emberAfPluginGatewayRelayCoapStartHeartbeat(uint16_t seconds)
{
  heartbeatPeriodSeconds = seconds;

  emberEventControlSetActive(emberAfPluginGatewayRelayCoapHeartbeatEventControl);
}

void emberAfPluginGatewayRelayCoapStopHeartbeat(void)
{
  emberEventControlSetInactive(emberAfPluginGatewayRelayCoapHeartbeatEventControl);
}

// ----------------------------------------------------------------------------
// gateway relay coap construct messages code
#define BUFFER_LENGTH 50
static uint8_t outgoingMessageUri[BUFFER_LENGTH];
static uint8_t outgoingMessagePayload[BUFFER_LENGTH];

static void sendAttributeReportMessage(uint16_t device,
                                       uint8_t endpoint,
                                       uint16_t cluster,
                                       uint16_t attribute,
                                       uint32_t data)
{
  uint16_t bytesUsed = 0;
  uint8_t *buffer = outgoingMessageUri;

  sprintf((char *) buffer, "zcl/e/%d/s%x/n", endpoint, cluster);

  emAfPluginGatewayRelayCoapCborWriteInit(outgoingMessagePayload, BUFFER_LENGTH);

  if (emAfPluginGatewayRelayCoapCborWriteMapTag()) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteString("r")) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteUint32(0)) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteString("a")) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteMapTag()) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteUint16(attribute)) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteUint32(data)) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteBreak()) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }
  if (emAfPluginGatewayRelayCoapCborWriteBreak()) {
    setUpErrorCode(COAP_RESPONSE_INTERNAL_ERROR);
  }

  bytesUsed += emAfPluginGatewayRelayCoapCborCurrentWriteDataLength();

  emberAfPluginCoapServerSendMessage(buffer,
                                     outgoingMessagePayload,
                                     bytesUsed,
                                     device,
                                     COAP_REQUEST_POST);
}

// ----------------------------------------------------------------------------
// gateway relay coap parse Options
static uint16_t bufferLength;
static uint8_t returnToken[BUFFER_LENGTH];
static uint8_t *currentBuffer;
static uint16_t finger;
static uint8_t currentOption;

uint8_t *optTok(uint8_t *buffer, uint16_t length, uint8_t *option)
{
  uint8_t optionByte, optionLength, i;

  if (buffer != NULL) {
    currentBuffer = buffer;
    bufferLength = length;
    currentOption = COAP_OPTION_URI_PATH;
    finger = 0;
  }

  if (finger >= bufferLength) {
    return NULL;
  }

  optionByte = currentBuffer[finger++];

  if (optionByte == 0xff) {
    return NULL;
  }

  optionLength = optionByte & 0x0f;
  currentOption = currentOption + (optionByte >> 4);

  for (i = 0; i < optionLength; i++) {
    returnToken[i] = currentBuffer[finger++];
  }

  returnToken[i] = 0;
  *option = currentOption;
  return returnToken;
}

static bool getUint16Opttok(uint16_t *val16)
{
  uint32_t data = 0;
  uint8_t opt;
  uint8_t *ptr = optTok(NULL, 0, &opt);

  if (ptr == NULL) {
    return false;
  }

  sscanf((char *) ptr, "%x", &data);

  *val16 = (uint16_t) data;

  return true;
}

static bool getClusterOpttok(uint16_t *cluster, char *clientServer)
{
  uint32_t data;
  uint8_t opt;
  uint8_t *ptr = optTok(NULL, 0, &opt);

  if (ptr == NULL) {
    return false;
  }

  *clientServer = *ptr;

  ptr++;

  sscanf((char *) ptr, "%x", &data);

  *cluster = (uint16_t) data;

  return true;
}

static void printData(uint8_t * data, uint16_t length, char *formatString)
{
  uint16_t i;
  printf("Data: ");

  for (i = 0; i < length; i++) {
    printf(formatString, data[i]);
  }

  printf("\r\n");
}

bool emberAfGatewayRelayCoapParseDotdotMessage(uint8_t length,
                                               uint8_t *string,
                                               int portCounter,
                                               uint16_t method)
{
  uint8_t *ptr, option;
  uint16_t endpoint;
  uint16_t cluster;
  char clientServer;
  uint16_t commandId;

  uint16_t i, cborPayload = length, cborLength;

  printData(string, length, "%02x ");

  returnCode = COAP_RESPONSE_BAD_REQUEST; // bad request by default.

  for (i = 0; i < length; i++) {
    if (string[i] == 0xff) {
      cborPayload = i + 1;
      string[i] = ':';
      break;
    }
  }

  cborLength = length - cborPayload;

  if (cborLength > 0) {
    emAfPluginGatewayRelayCoapCborReadInit(string + cborPayload, cborLength);
  } else {
    emAfPluginGatewayRelayCoapCborReadInit(NULL, 0);
  }

  ptr = optTok(string, length, &option);

  if (ptr == NULL) {
    // respond with {0,"e"} here

    if (!checkMethod(method, COAP_REQUEST_GET)) {
      constructResourceList("e");
    }

    return false;
  }

  if (strncmp((char *) ptr, "e", 1) != 0) {
    // error:  only accept endpoint here.
    setUpErrorCode(404);
    return false;
  }

  if (!getUint16Opttok(&endpoint)) {
    // respond with endpoints list here
    if (!checkMethod(method, COAP_REQUEST_GET)) {
      constructEndpointList();
    }
    return false;
  }

  if (checkEndpoint(endpoint, portCounter)) {
    setUpErrorCode(404);
    return false;
  }

  if (!getClusterOpttok(&cluster, &clientServer)) {
    // respond with cluster list here
    if (!checkMethod(method, COAP_REQUEST_GET)) {
      constructClusterList(portCounter);
    }
    return false;
  }

  ptr = optTok(NULL, 0, &option);
  if (ptr == NULL) {
    if (!checkMethod(method, COAP_REQUEST_GET)) {
      constructResourceList("ac");
    }
    return false;
  }

  switch (ptr[0]) {
    case 'c':
      if (!getUint16Opttok(&commandId)) {
        if (!checkMethod(method, COAP_REQUEST_GET)) {
          setUpErrorCode(COAP_RESPONSE_NOT_IMPLEMENTED);
          return false;
        }
      } else {
        if (!checkMethod(method, COAP_REQUEST_POST)) {
          emAfGatewayRelayCoapProcessIncomingCommand(cluster,
                                                     commandId,
                                                     (uint16_t) portCounter,
                                                     endpoint);
          returnCode = COAP_RESPONSE_CREATED;
        }
      }
      break;
    case 'a':
      if (!checkMethod(method, COAP_REQUEST_GET)) {
        handleAttributeRequest(cluster, portCounter, endpoint);
        returnCode = COAP_RESPONSE_CREATED;
      }
      break;
    default:
      emberAfCorePrintln("Unknown payload: %s", ptr);
      break;
  }

  return true;
}
