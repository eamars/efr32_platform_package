// This file is generated by Simplicity Studio. Please do not edit manually.
//
//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.
#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_EMBER_TYPES

#include EMBER_AF_API_XNCP
#include EMBER_AF_API_EVENT
#include EMBER_AF_API_BYTE_UTILITIES
#include EMBER_AF_API_HAL

#include "led-protocol.h"

/* This sample application demostrates an NCP using a custom protocol to
 * communicate with the host. As an example protocol, the NCP has defined
 * commands so that the host can control an LED on the NCP's RCM.  See
 * led-protocol.h for details.
 *
 * The host sends custom EZSP commands to the NCP, and the NCP acts on them
 * based on the functionality in the code found below.
 * This sample application is meant to be paired with the XncpSensorHost
 * sample application in the ZCL Application Framework V2.
 */

// -----------------------------------------------------------------------------
// Declarations

#define XNCP_VERSION_NUMBER  0x1234
#define XNCP_MANUFACTURER_ID 0xABCD

// LED state.
static uint8_t ledState;
static uint32_t ledEventHandlerDelayMS = MILLISECOND_TICKS_PER_SECOND;
#define LED (BOARDLED0)

// -----------------------------------------------------------------------------
// LED event

EmberEventControl ledEventControl;

// LED event handler.
void ledEventHandler(void)
{
  switch(ledState) {
  case LED_PROTOCOL_COMMAND_CLEAR_LED:
    halClearLed(LED);
    break;
  case LED_PROTOCOL_COMMAND_SET_LED:
    halSetLed(LED);
    break;
  case LED_PROTOCOL_COMMAND_STROBE_LED:
    halToggleLed(LED);
    break;
  default:
    ; // TODO: handler error.
  }
  
  emberEventControlSetDelayMS(ledEventControl, ledEventHandlerDelayMS);
}

// -----------------------------------------------------------------------------
// Callbacks

/** @brief Main Init
 *
 * This function is called when the application starts and can be used to
 * perform any additional initialization required at system startup.
 */
void emberAfMainInitCallback(void)
{
  emberEventControlSetActive(ledEventControl);
}

/** @brief Incoming Custom EZSP Message Callback
 *
 * This function is called when the NCP receives a custom EZSP message from the
 * HOST.  The message length and payload is passed to the callback in the first
 * two arguments.  The implementation can then fill in the replyPayload and set
 * the replayPayloadLength to the number of bytes in the replyPayload.
 * See documentation for the function ezspCustomFrame on sending these messages
 * from the HOST.
 *
 * @param messageLength The length of the messagePayload.
 * @param messagePayload The custom message that was sent from the HOST.
 * Ver.: always
 * @param replyPayloadLength The length of the replyPayload.  This needs to be
 * set by the implementation in order for a properly formed respose to be sent
 * back to the HOST. Ver.: always
 * @param replyPayload The custom message to send back to the HOST in respose
 * to the custom message. Ver.: always
 *
 * @return An ::EmberStatus indicating the result of the custom message
 * handling.  This returned status is always the first byte of the EZSP
 * response.
 */ 
EmberStatus emberAfPluginXncpIncomingCustomFrameCallback(uint8_t messageLength,
                                                         uint8_t *messagePayload,
                                                         uint8_t *replyPayloadLength,
                                                         uint8_t *replyPayload)
{
  // First byte is the command ID.
  uint8_t commandId = messagePayload[LED_PROTOCOL_COMMAND_INDEX];
  EmberStatus status = EMBER_SUCCESS;
  *replyPayloadLength = 0;
  
  switch (commandId) {
  case LED_PROTOCOL_COMMAND_SET_LED:
    ledState = LED_PROTOCOL_COMMAND_SET_LED;
    break;
  case LED_PROTOCOL_COMMAND_CLEAR_LED:
    ledState = LED_PROTOCOL_COMMAND_CLEAR_LED;
    break;
  case LED_PROTOCOL_COMMAND_STROBE_LED:
    ledState = LED_PROTOCOL_COMMAND_STROBE_LED;
    break;
  case LED_PROTOCOL_COMMAND_GET_LED:
    replyPayload[0] = ledState;
    *replyPayloadLength += sizeof(ledState);
    break;
  case LED_PROTOCOL_COMMAND_GET_FREQ:
    emberStoreLowHighInt32u(replyPayload, ledEventHandlerDelayMS);
    *replyPayloadLength += sizeof(ledEventHandlerDelayMS);
    break;
  case LED_PROTOCOL_COMMAND_SET_FREQ: {
    // Get the frequency parameter from one index past the command.
    // A frequency of 0 is invalid, as mentioned in the protocol.
    // The message should be longer than 4 bytes, since it should consist of
    // a command and a 32-bit integer.
    uint32_t frequency = emberFetchLowHighInt32u(messagePayload + 1);
    if (frequency || messageLength < 4) {
      ledEventHandlerDelayMS = frequency;
    } else {
      status = EMBER_BAD_ARGUMENT;
    }
  }
    break;
  default:
    status = EMBER_INVALID_CALL;
  }
  
  return status;
}

/** @brief Get XNCP Information
 *
 * This callback enables users to communicate the version number and
 * manufacturer ID of their NCP application to the framework. This information
 * is needed for the EZSP command frame called getXncpInfo. This callback will
 * be called when that frame is received so that the application can report
 * its version number and manufacturer ID to be sent back to the HOST.
 *
 * @param versionNumber The version number of the NCP application.
 * @param manufacturerId The manufacturer ID of the NCP application.
 */
void emberAfPluginXncpGetXncpInformation(uint16_t *manufacturerId,
                                         uint16_t *versionNumber)
{
  *versionNumber = XNCP_VERSION_NUMBER;
  *manufacturerId = XNCP_MANUFACTURER_ID;
}
