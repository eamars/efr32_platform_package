// Copyright 2017 Silicon Laboratories, Inc.                                *80*

#ifndef __CHANNEL_MQTT_H
#define __CHANNEL_MQTT_H

/** @brief Function to set the heart beat interval
 *
 * Function to set the mqtt heart beat interval in milliseconds
 *
 * @param intervalMs heart beat interval in milliseconds
 */
void emberPluginGatewayRelayMqttSetHeartBeat(uint16_t intervalMs);

#endif
