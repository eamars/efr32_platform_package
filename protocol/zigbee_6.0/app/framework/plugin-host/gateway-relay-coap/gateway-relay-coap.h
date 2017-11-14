// Copyright 2017 Silicon Laboratories, Inc.                                *80*

#ifndef __GATEWAY_RELAY_COAP_H
#define __GATEWAY_RELAY_COAP_H

/** @brief Parse incoming dotdot message
 *
 * This API will parse the incoming gateway relay coap message and, if necessary,
 * send the resulting ZCL command to the appropriate device.
 *
 * @param length The length of the payload.
 *
 * @param *string Pointer to the payload (after the "zcl" option has been
 * stripped out).
 *
 * @param portCounter The number of the port on which the message was received.
 * This will be used to determine to which device a zcl command should be sent,
 * if necessary.
 *
 * @param method The COAP method used to send the message.  This is checked
 * during parsing because certain methods are only allowed to perform certain
 * functions.
 *
 */
bool emberAfGatewayRelayCoapParseDotdotMessage(uint8_t length,
                                               uint8_t *string,
                                               int portCounter,
                                               uint16_t method);

/** @brief Returns the length of the return string.
 *
 * The gateway relay coap pluign will create a response string.  The coap server
 * needs to know the length of that string for the COAP response methods.  This
 * api provides that length.
 *
 */
uint16_t emberAfPluginGatewayRelayCoapReturnStringLength(void);

/** @brief Copy the response string into the data in pointer.
 *
 * Gateway relay coap plugin may generate a response string.  If so, the coap
 * server will need to obtain it using this api.
 *
 * @param *ptr Pointer to the response string.
 *
 */
void emberAfPluginGatewayRelayCoapCopyReturnString(char *ptr);

/** @brief Returns the COAP response code from the gateway relay coap plugin.
 *
 * The coap server needs to know what response (if any) to send as part of the
 * coap ACK.  This is generated by parsing the message in the gateway-relay-coap
 * plugin.  Use this method to communicate it.
 *
 */
uint16_t emberAfPluginGatewayRelayCoapReturnCode(void);

#endif
