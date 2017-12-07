/**************************************************************************//**
 * Copyright 2017 Silicon Laboratories, Inc.
 *
 *****************************************************************************/
//
// *** Generated file. Do not edit! ***
//
// Description: Convert an enumerated value into a human-readable string.

#include PLATFORM_HEADER
#include "stack/include/ember-types.h"
#include "csp-enum.h"

const char *decodeFrameId(uint16_t value)
{
  switch (value) {
// Core
    case EMBER_SET_PHY_MODE_COMMAND_IDENTIFIER:
      return "EMBER_SET_PHY_MODE_COMMAND_IDENTIFIER";
    case EMBER_SET_PHY_MODE_RETURN_IDENTIFIER:
      return "EMBER_SET_PHY_MODE_RETURN_IDENTIFIER";
    case EMBER_GET_PHY_MODE_COMMAND_IDENTIFIER:
      return "EMBER_GET_PHY_MODE_COMMAND_IDENTIFIER";
    case EMBER_GET_PHY_MODE_RETURN_IDENTIFIER:
      return "EMBER_GET_PHY_MODE_RETURN_IDENTIFIER";
    case EMBER_NETWORK_STATE_COMMAND_IDENTIFIER:
      return "EMBER_NETWORK_STATE_COMMAND_IDENTIFIER";
    case EMBER_NETWORK_STATE_RETURN_IDENTIFIER:
      return "EMBER_NETWORK_STATE_RETURN_IDENTIFIER";
    case EMBER_STACK_IS_UP_COMMAND_IDENTIFIER:
      return "EMBER_STACK_IS_UP_COMMAND_IDENTIFIER";
    case EMBER_STACK_IS_UP_RETURN_IDENTIFIER:
      return "EMBER_STACK_IS_UP_RETURN_IDENTIFIER";
    case EMBER_SET_SECURITY_KEY_COMMAND_IDENTIFIER:
      return "EMBER_SET_SECURITY_KEY_COMMAND_IDENTIFIER";
    case EMBER_SET_SECURITY_KEY_RETURN_IDENTIFIER:
      return "EMBER_SET_SECURITY_KEY_RETURN_IDENTIFIER";
    case EMBER_GET_COUNTER_COMMAND_IDENTIFIER:
      return "EMBER_GET_COUNTER_COMMAND_IDENTIFIER";
    case EMBER_GET_COUNTER_RETURN_IDENTIFIER:
      return "EMBER_GET_COUNTER_RETURN_IDENTIFIER";
    case EMBER_SET_RADIO_CHANNEL_COMMAND_IDENTIFIER:
      return "EMBER_SET_RADIO_CHANNEL_COMMAND_IDENTIFIER";
    case EMBER_SET_RADIO_CHANNEL_RETURN_IDENTIFIER:
      return "EMBER_SET_RADIO_CHANNEL_RETURN_IDENTIFIER";
    case EMBER_GET_RADIO_CHANNEL_COMMAND_IDENTIFIER:
      return "EMBER_GET_RADIO_CHANNEL_COMMAND_IDENTIFIER";
    case EMBER_GET_RADIO_CHANNEL_RETURN_IDENTIFIER:
      return "EMBER_GET_RADIO_CHANNEL_RETURN_IDENTIFIER";
    case EMBER_SET_RADIO_POWER_COMMAND_IDENTIFIER:
      return "EMBER_SET_RADIO_POWER_COMMAND_IDENTIFIER";
    case EMBER_SET_RADIO_POWER_RETURN_IDENTIFIER:
      return "EMBER_SET_RADIO_POWER_RETURN_IDENTIFIER";
    case EMBER_GET_RADIO_POWER_COMMAND_IDENTIFIER:
      return "EMBER_GET_RADIO_POWER_COMMAND_IDENTIFIER";
    case EMBER_GET_RADIO_POWER_RETURN_IDENTIFIER:
      return "EMBER_GET_RADIO_POWER_RETURN_IDENTIFIER";
    case EMBER_SET_RADIO_POWER_MODE_COMMAND_IDENTIFIER:
      return "EMBER_SET_RADIO_POWER_MODE_COMMAND_IDENTIFIER";
    case EMBER_SET_RADIO_POWER_MODE_RETURN_IDENTIFIER:
      return "EMBER_SET_RADIO_POWER_MODE_RETURN_IDENTIFIER";
    case EMBER_SET_MAC_PARAMS_COMMAND_IDENTIFIER:
      return "EMBER_SET_MAC_PARAMS_COMMAND_IDENTIFIER";
    case EMBER_SET_MAC_PARAMS_RETURN_IDENTIFIER:
      return "EMBER_SET_MAC_PARAMS_RETURN_IDENTIFIER";
    case EMBER_CURRENT_STACK_TASKS_COMMAND_IDENTIFIER:
      return "EMBER_CURRENT_STACK_TASKS_COMMAND_IDENTIFIER";
    case EMBER_CURRENT_STACK_TASKS_RETURN_IDENTIFIER:
      return "EMBER_CURRENT_STACK_TASKS_RETURN_IDENTIFIER";
    case EMBER_OK_TO_NAP_COMMAND_IDENTIFIER:
      return "EMBER_OK_TO_NAP_COMMAND_IDENTIFIER";
    case EMBER_OK_TO_NAP_RETURN_IDENTIFIER:
      return "EMBER_OK_TO_NAP_RETURN_IDENTIFIER";
    case EMBER_OK_TO_HIBERNATE_COMMAND_IDENTIFIER:
      return "EMBER_OK_TO_HIBERNATE_COMMAND_IDENTIFIER";
    case EMBER_OK_TO_HIBERNATE_RETURN_IDENTIFIER:
      return "EMBER_OK_TO_HIBERNATE_RETURN_IDENTIFIER";
    case EMBER_GET_EUI64_COMMAND_IDENTIFIER:
      return "EMBER_GET_EUI64_COMMAND_IDENTIFIER";
    case EMBER_GET_EUI64_RETURN_IDENTIFIER:
      return "EMBER_GET_EUI64_RETURN_IDENTIFIER";
    case EMBER_IS_LOCAL_EUI64_COMMAND_IDENTIFIER:
      return "EMBER_IS_LOCAL_EUI64_COMMAND_IDENTIFIER";
    case EMBER_IS_LOCAL_EUI64_RETURN_IDENTIFIER:
      return "EMBER_IS_LOCAL_EUI64_RETURN_IDENTIFIER";
    case EMBER_GET_NODE_ID_COMMAND_IDENTIFIER:
      return "EMBER_GET_NODE_ID_COMMAND_IDENTIFIER";
    case EMBER_GET_NODE_ID_RETURN_IDENTIFIER:
      return "EMBER_GET_NODE_ID_RETURN_IDENTIFIER";
    case EMBER_GET_PAN_ID_COMMAND_IDENTIFIER:
      return "EMBER_GET_PAN_ID_COMMAND_IDENTIFIER";
    case EMBER_GET_PAN_ID_RETURN_IDENTIFIER:
      return "EMBER_GET_PAN_ID_RETURN_IDENTIFIER";
    case EMBER_GET_PARENT_ID_COMMAND_IDENTIFIER:
      return "EMBER_GET_PARENT_ID_COMMAND_IDENTIFIER";
    case EMBER_GET_PARENT_ID_RETURN_IDENTIFIER:
      return "EMBER_GET_PARENT_ID_RETURN_IDENTIFIER";
    case EMBER_GET_NODE_TYPE_COMMAND_IDENTIFIER:
      return "EMBER_GET_NODE_TYPE_COMMAND_IDENTIFIER";
    case EMBER_GET_NODE_TYPE_RETURN_IDENTIFIER:
      return "EMBER_GET_NODE_TYPE_RETURN_IDENTIFIER";
    case EMBER_GET_CSP_VERSION_COMMAND_IDENTIFIER:
      return "EMBER_GET_CSP_VERSION_COMMAND_IDENTIFIER";
    case EMBER_GET_CSP_VERSION_RETURN_IDENTIFIER:
      return "EMBER_GET_CSP_VERSION_RETURN_IDENTIFIER";
    case EMBER_CALIBRATE_CURRENT_CHANNEL_COMMAND_IDENTIFIER:
      return "EMBER_CALIBRATE_CURRENT_CHANNEL_COMMAND_IDENTIFIER";
    case CB_STACK_STATUS_COMMAND_IDENTIFIER:
      return "CB_STACK_STATUS_COMMAND_IDENTIFIER";
    case CB_CHILD_JOIN_COMMAND_IDENTIFIER:
      return "CB_CHILD_JOIN_COMMAND_IDENTIFIER";
    case CB_RADIO_NEEDS_CALIBRATING_COMMAND_IDENTIFIER:
      return "CB_RADIO_NEEDS_CALIBRATING_COMMAND_IDENTIFIER";
// Messaging
    case EMBER_MESSAGE_SEND_COMMAND_IDENTIFIER:
      return "EMBER_MESSAGE_SEND_COMMAND_IDENTIFIER";
    case EMBER_MESSAGE_SEND_RETURN_IDENTIFIER:
      return "EMBER_MESSAGE_SEND_RETURN_IDENTIFIER";
    case EMBER_POLL_FOR_DATA_COMMAND_IDENTIFIER:
      return "EMBER_POLL_FOR_DATA_COMMAND_IDENTIFIER";
    case EMBER_POLL_FOR_DATA_RETURN_IDENTIFIER:
      return "EMBER_POLL_FOR_DATA_RETURN_IDENTIFIER";
    case EMBER_MAC_MESSAGE_SEND_COMMAND_IDENTIFIER:
      return "EMBER_MAC_MESSAGE_SEND_COMMAND_IDENTIFIER";
    case EMBER_MAC_MESSAGE_SEND_RETURN_IDENTIFIER:
      return "EMBER_MAC_MESSAGE_SEND_RETURN_IDENTIFIER";
    case EMBER_MAC_SET_ALLOCATE_ADDRESS_FLAG_COMMAND_IDENTIFIER:
      return "EMBER_MAC_SET_ALLOCATE_ADDRESS_FLAG_COMMAND_IDENTIFIER";
    case EMBER_MAC_SET_ALLOCATE_ADDRESS_FLAG_RETURN_IDENTIFIER:
      return "EMBER_MAC_SET_ALLOCATE_ADDRESS_FLAG_RETURN_IDENTIFIER";
    case EMBER_SET_POLL_DESTINATION_ADDRESS_COMMAND_IDENTIFIER:
      return "EMBER_SET_POLL_DESTINATION_ADDRESS_COMMAND_IDENTIFIER";
    case EMBER_SET_POLL_DESTINATION_ADDRESS_RETURN_IDENTIFIER:
      return "EMBER_SET_POLL_DESTINATION_ADDRESS_RETURN_IDENTIFIER";
    case EMBER_PURGE_INDIRECT_MESSAGES_COMMAND_IDENTIFIER:
      return "EMBER_PURGE_INDIRECT_MESSAGES_COMMAND_IDENTIFIER";
    case EMBER_PURGE_INDIRECT_MESSAGES_RETURN_IDENTIFIER:
      return "EMBER_PURGE_INDIRECT_MESSAGES_RETURN_IDENTIFIER";
    case CB_MESSAGE_SENT_COMMAND_IDENTIFIER:
      return "CB_MESSAGE_SENT_COMMAND_IDENTIFIER";
    case CB_INCOMING_MESSAGE_COMMAND_IDENTIFIER:
      return "CB_INCOMING_MESSAGE_COMMAND_IDENTIFIER";
    case CB_INCOMING_MAC_MESSAGE_COMMAND_IDENTIFIER:
      return "CB_INCOMING_MAC_MESSAGE_COMMAND_IDENTIFIER";
    case CB_MAC_MESSAGE_SENT_COMMAND_IDENTIFIER:
      return "CB_MAC_MESSAGE_SENT_COMMAND_IDENTIFIER";
// Network Management
    case EMBER_NETWORK_INIT_COMMAND_IDENTIFIER:
      return "EMBER_NETWORK_INIT_COMMAND_IDENTIFIER";
    case EMBER_NETWORK_INIT_RETURN_IDENTIFIER:
      return "EMBER_NETWORK_INIT_RETURN_IDENTIFIER";
    case EMBER_START_ACTIVE_SCAN_COMMAND_IDENTIFIER:
      return "EMBER_START_ACTIVE_SCAN_COMMAND_IDENTIFIER";
    case EMBER_START_ACTIVE_SCAN_RETURN_IDENTIFIER:
      return "EMBER_START_ACTIVE_SCAN_RETURN_IDENTIFIER";
    case EMBER_START_ENERGY_SCAN_COMMAND_IDENTIFIER:
      return "EMBER_START_ENERGY_SCAN_COMMAND_IDENTIFIER";
    case EMBER_START_ENERGY_SCAN_RETURN_IDENTIFIER:
      return "EMBER_START_ENERGY_SCAN_RETURN_IDENTIFIER";
    case EMBER_SET_APPLICATION_BEACON_PAYLOAD_COMMAND_IDENTIFIER:
      return "EMBER_SET_APPLICATION_BEACON_PAYLOAD_COMMAND_IDENTIFIER";
    case EMBER_SET_APPLICATION_BEACON_PAYLOAD_RETURN_IDENTIFIER:
      return "EMBER_SET_APPLICATION_BEACON_PAYLOAD_RETURN_IDENTIFIER";
    case EMBER_FORM_NETWORK_COMMAND_IDENTIFIER:
      return "EMBER_FORM_NETWORK_COMMAND_IDENTIFIER";
    case EMBER_FORM_NETWORK_RETURN_IDENTIFIER:
      return "EMBER_FORM_NETWORK_RETURN_IDENTIFIER";
    case EMBER_JOIN_NETWORK_EXTENDED_COMMAND_IDENTIFIER:
      return "EMBER_JOIN_NETWORK_EXTENDED_COMMAND_IDENTIFIER";
    case EMBER_JOIN_NETWORK_EXTENDED_RETURN_IDENTIFIER:
      return "EMBER_JOIN_NETWORK_EXTENDED_RETURN_IDENTIFIER";
    case EMBER_JOIN_NETWORK_COMMAND_IDENTIFIER:
      return "EMBER_JOIN_NETWORK_COMMAND_IDENTIFIER";
    case EMBER_JOIN_NETWORK_RETURN_IDENTIFIER:
      return "EMBER_JOIN_NETWORK_RETURN_IDENTIFIER";
    case EMBER_PERMIT_JOINING_COMMAND_IDENTIFIER:
      return "EMBER_PERMIT_JOINING_COMMAND_IDENTIFIER";
    case EMBER_PERMIT_JOINING_RETURN_IDENTIFIER:
      return "EMBER_PERMIT_JOINING_RETURN_IDENTIFIER";
    case EMBER_JOIN_COMMISSIONED_COMMAND_IDENTIFIER:
      return "EMBER_JOIN_COMMISSIONED_COMMAND_IDENTIFIER";
    case EMBER_JOIN_COMMISSIONED_RETURN_IDENTIFIER:
      return "EMBER_JOIN_COMMISSIONED_RETURN_IDENTIFIER";
    case EMBER_RESET_NETWORK_STATE_COMMAND_IDENTIFIER:
      return "EMBER_RESET_NETWORK_STATE_COMMAND_IDENTIFIER";
    case EMBER_GET_STANDALONE_BOOTLOADER_INFO_COMMAND_IDENTIFIER:
      return "EMBER_GET_STANDALONE_BOOTLOADER_INFO_COMMAND_IDENTIFIER";
    case EMBER_LAUNCH_STANDALONE_BOOTLOADER_COMMAND_IDENTIFIER:
      return "EMBER_LAUNCH_STANDALONE_BOOTLOADER_COMMAND_IDENTIFIER";
    case EMBER_FREQUENCY_HOPPING_START_SERVER_COMMAND_IDENTIFIER:
      return "EMBER_FREQUENCY_HOPPING_START_SERVER_COMMAND_IDENTIFIER";
    case EMBER_FREQUENCY_HOPPING_START_SERVER_RETURN_IDENTIFIER:
      return "EMBER_FREQUENCY_HOPPING_START_SERVER_RETURN_IDENTIFIER";
    case EMBER_FREQUENCY_HOPPING_START_CLIENT_COMMAND_IDENTIFIER:
      return "EMBER_FREQUENCY_HOPPING_START_CLIENT_COMMAND_IDENTIFIER";
    case EMBER_FREQUENCY_HOPPING_START_CLIENT_RETURN_IDENTIFIER:
      return "EMBER_FREQUENCY_HOPPING_START_CLIENT_RETURN_IDENTIFIER";
    case EMBER_FREQUENCY_HOPPING_STOP_COMMAND_IDENTIFIER:
      return "EMBER_FREQUENCY_HOPPING_STOP_COMMAND_IDENTIFIER";
    case EMBER_FREQUENCY_HOPPING_STOP_RETURN_IDENTIFIER:
      return "EMBER_FREQUENCY_HOPPING_STOP_RETURN_IDENTIFIER";
    case EMBER_SET_AUXILIARY_ADDRESS_FILTERING_ENTRY_COMMAND_IDENTIFIER:
      return "EMBER_SET_AUXILIARY_ADDRESS_FILTERING_ENTRY_COMMAND_IDENTIFIER";
    case EMBER_SET_AUXILIARY_ADDRESS_FILTERING_ENTRY_RETURN_IDENTIFIER:
      return "EMBER_SET_AUXILIARY_ADDRESS_FILTERING_ENTRY_RETURN_IDENTIFIER";
    case EMBER_GET_AUXILIARY_ADDRESS_FILTERING_ENTRY_COMMAND_IDENTIFIER:
      return "EMBER_GET_AUXILIARY_ADDRESS_FILTERING_ENTRY_COMMAND_IDENTIFIER";
    case EMBER_GET_AUXILIARY_ADDRESS_FILTERING_ENTRY_RETURN_IDENTIFIER:
      return "EMBER_GET_AUXILIARY_ADDRESS_FILTERING_ENTRY_RETURN_IDENTIFIER";
    case CB_INCOMING_BEACON_COMMAND_IDENTIFIER:
      return "CB_INCOMING_BEACON_COMMAND_IDENTIFIER";
    case CB_ACTIVE_SCAN_COMPLETE_COMMAND_IDENTIFIER:
      return "CB_ACTIVE_SCAN_COMPLETE_COMMAND_IDENTIFIER";
    case CB_ENERGY_SCAN_COMPLETE_COMMAND_IDENTIFIER:
      return "CB_ENERGY_SCAN_COMPLETE_COMMAND_IDENTIFIER";
    case CB_GET_STANDALONE_BOOTLOADER_INFO_COMMAND_IDENTIFIER:
      return "CB_GET_STANDALONE_BOOTLOADER_INFO_COMMAND_IDENTIFIER";
    case CB_LAUNCH_STANDALONE_BOOTLOADER_COMMAND_IDENTIFIER:
      return "CB_LAUNCH_STANDALONE_BOOTLOADER_COMMAND_IDENTIFIER";
    case CB_FREQUENCY_HOPPING_START_CLIENT_COMPLETE_COMMAND_IDENTIFIER:
      return "CB_FREQUENCY_HOPPING_START_CLIENT_COMPLETE_COMMAND_IDENTIFIER";
// EMBER_TEST
    case EMBER_ECHO_COMMAND_IDENTIFIER:
      return "EMBER_ECHO_COMMAND_IDENTIFIER";
    case CB_ECHO_COMMAND_IDENTIFIER:
      return "CB_ECHO_COMMAND_IDENTIFIER";
// APP_USES_SOFTWARE_FLOW_CONTROL
    case EMBER_START_XON_XOFF_TEST_COMMAND_IDENTIFIER:
      return "EMBER_START_XON_XOFF_TEST_COMMAND_IDENTIFIER";
  }
  return "*** Not enumerated. ***";
}