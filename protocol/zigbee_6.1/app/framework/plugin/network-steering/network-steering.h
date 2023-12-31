// Copyright 2015 Silicon Laboratories, Inc.

#ifndef SILABS_NETWORK_STEERING_H
#define SILABS_NETWORK_STEERING_H

// -----------------------------------------------------------------------------
// Constants

extern PGM uint8_t emAfNetworkSteeringPluginName[];

// -----------------------------------------------------------------------------
// Types

enum {
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_NONE                         = 0x00,
  // These next two states are only run if explicitly configured to do so
  // See emAfPluginNetworkSteeringSetConfiguredKey()
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_PRIMARY_CONFIGURED      = 0x01,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_SECONDARY_CONFIGURED    = 0x02,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_PRIMARY_INSTALL_CODE    = 0x03,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_SECONDARY_INSTALL_CODE  = 0x04,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_PRIMARY_CENTRALIZED     = 0x05,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_SECONDARY_CENTRALIZED   = 0x06,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_PRIMARY_DISTRIBUTED     = 0x07,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_SECONDARY_DISTRIBUTED   = 0x08,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_SCAN_FINISHED                = 0x09,

  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_UPDATE_TCLK                  = 0x10,
  EMBER_AF_PLUGIN_NETWORK_STEERING_STATE_VERIFY_TCLK                  = 0x20,
};
typedef uint8_t EmberAfPluginNetworkSteeringJoiningState;

enum {
  EMBER_AF_PLUGIN_NETWORK_STEERING_OPTIONS_NONE                       = 0x00,
  EMBER_AF_PLUGIN_NETWORK_STEERING_OPTIONS_NO_TCLK_UPDATE             = 0x01,
};
typedef uint8_t EmberAfPluginNetworkSteeringOptions;

extern EmberAfPluginNetworkSteeringOptions emAfPluginNetworkSteeringOptionsMask;

// -----------------------------------------------------------------------------
// Globals

/** @brief The first set of channels on which to search for joinable networks. */
extern uint32_t emAfPluginNetworkSteeringPrimaryChannelMask;
/** @brief The second set of channels on which to search for joinable networks. */
extern uint32_t emAfPluginNetworkSteeringSecondaryChannelMask;

// -----------------------------------------------------------------------------
// API

/** @brief Start
 *
 * Initiate a network-steering procedure.
 *
 * If the node is currently on a network, it will perform network steering for
 * node on a network, in which it opens up the network with a broadcasted
 * permit join message.
 *
 * If the node is not on a network, it will scan a series of primary channels
 * (see ::emAfPluginNetworkSteeringPrimaryChannelMask) to try to find possible
 * networks to join. If it is unable to join any of those networks, it will
 * try scanning on a set of secondary channels
 * (see ::emAfPluginNetworkSteeringSecondaryChannelMask). Upon completion of
 * this process, the plugin will call
 * ::emberAfPluginNetworkSteeringCompleteCallback with information regarding
 * the success or failure of the procedure.
 *
 * This procedure will try to join networks using install codes, the centralized
 * default key, and the distributed default key.
 *
 * @return An ::EmberStatus value that indicates the success or failure of
 * the initiating of the network steering process.
 */
EmberStatus emberAfPluginNetworkSteeringStart(void);

/** @brief Stop
 *
 * Stop the network steering procedure.
 *
 * @return An ::EmberStatus value that indicates the success or failure of
 * the initiating of the network steering process.
 */
EmberStatus emberAfPluginNetworkSteeringStop(void);

/** @brief Internal function to override the channel mask. */
void emAfPluginNetworkSteeringSetChannelMask(uint32_t mask, bool secondaryMask);

/** @brief Internal function to set extended PAN ID to search for. */
void emAfPluginNetworkSteeringSetExtendedPanIdFilter(uint8_t* extendedPanId,
                                                     bool turnFilterOn);

/** @brief Internal function to set a different key to use when joining. */
void emAfPluginNetworkSteeringSetConfiguredKey(uint8_t *key,
                                               bool useConfiguredKey);

#endif /* __NETWORK_STEERING_H__ */
