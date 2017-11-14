//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.
#include <stdint.h>
#include <stdio.h>
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/plugin/network-creator-security/network-creator-security.h"
#include "app/framework/plugin/network-creator/network-creator.h"
#include "app/framework/plugin/reporting/reporting.h"
#include "rtos_bluetooth.h"
#include "gatt_db.h"
#include "app/framework/plugin/dmp-ui-demo/dmp-ui.h"

#define DMP_DEBUG
//#undef DMP_DEBUG

/* Write response codes*/
#define ES_WRITE_OK                         0
#define ES_ERR_CCCD_CONF                    0x81
#define ES_ERR_PROC_IN_PROGRESS             0x80
#define ES_NO_CONNECTION                    0xFF
// Advertisement data
#define UINT16_TO_BYTES(n)        ((uint8_t) (n)), ((uint8_t)((n) >> 8))
#define UINT16_TO_BYTE0(n)        ((uint8_t) (n))
#define UINT16_TO_BYTE1(n)        ((uint8_t) ((n) >> 8))
// Ble TX test macros and functions
#define BLE_TX_TEST_DATA_SIZE   2
// We need to put the device name into a scan response packet,
// since it isn't included in the 'standard' beacons -
// I've included the flags, since certain apps seem to expect them
#define DEVNAME "DMP%02X%02X"
#define DEVNAME_LEN 8  // incl term null
#define UUID_LEN 16 // 128-bit UUID

#define LE_GAP_MAX_DISCOVERABLE_MODE   0x04
#define LE_GAP_MAX_CONNECTABLE_MODE    0x03
#define LE_GAP_MAX_DISCOVERY_MODE      0x02
#define BLE_INDICATION_TIMEOUT         30000
#define BUTTON_LONG_PRESS_TIME_MSEC    3000

static void readLightState(uint8_t connection);
static void readTriggerSource(uint8_t connection);
static void writeLightState(uint8_t connection, uint8array *writeValue);
static void enableBleAdvertisements(void);
static uint8_t bleConnectionInfoTableFindUnused(void);
static void bleConnectionInfoTablePrintEntry(uint8_t index);
static void printBleAddress(uint8_t *address);

// Event function forward declarations
void bleEventHandler(void);
void bleTxEventHandler(void);
void lcdMainMenuDisplayEventHandler(void);
void lcdPermitJoinEventHandler(void);
void buttonEventHandler(void);
void bleAdvertsEventHandler(void);
void bleIndicateRespEventHandler(void);

typedef enum {
  GAT_SERVER_CLIENT_CONFIG = 1,
  GAT_SERVER_CONFIRMATION,
}BLE_GAT_STATUS_FLAG;

typedef enum {
  GAT_DISABLED,
  GAT_RECEIVE_NOTIFICATION,
  GAT_RECEIVE_INDICATION,
}BLE_GAT_CLIENT_CONFIG_FLAG;

static volatile boolean ble_client_Confirm = FALSE;
static BLE_GAT_CLIENT_CONFIG_FLAG ble_lightState_config = 0;
static BLE_GAT_CLIENT_CONFIG_FLAG ble_lastEvent_config = 0;
static uint8_t ble_lightState = 0;
static uint8_t ble_lastEvent = 3;
// TODO: we should have a single connection with a client, stored
// in 'currentConnection' - call on attribute changed callback
static uint16_t currentConnection;
static boolean bleConnected = false;
static uint8_t lastButton;
static bool longPress = false;
static uint16_t bleNotificationsPeriodMs;
static DmpUiLightDirection_t lightDirection = DMP_UI_DIRECTION_INVALID;
// Event control struct declarations
EmberEventControl identifyEventControl;
EmberEventControl networkEventControl;
EmberEventControl bleEventControl;
EmberEventControl bleTxEventControl;
EmberEventControl zigbeeEventControl;
EmberEventControl lcdMainMenuDisplayEventControl;
EmberEventControl lcdPermitJoinEventControl;
EmberEventControl buttonEventControl;
EmberEventControl bleIndicateControl;

struct {
  bool inUse;
  bool isMaster;
  uint8_t connectionHandle;
  uint8_t bondingHandle;
  uint8_t remoteAddress[6];
} bleConnectionTable[EMBER_AF_PLUGIN_BLE_MAX_CONNECTIONS];

/** GATT Server Attribute User Read Configuration.
 *  Structure to register handler functions to user read events. */
typedef struct {
  uint16 charId;                        /**< ID of the Characteristic. */
  void (*fctn)(uint8_t connection); /**< Handler function. */
} AppCfgGattServerUserReadRequest_t;

/** GATT Server Attribute Value Write Configuration.
 *  Structure to register handler functions to characteristic write events. */
typedef struct {
  uint16 charId;                        /**< ID of the Characteristic. */
  /**< Handler function. */
  void (*fctn)(uint8_t connection, uint8array * writeValue);
} AppCfgGattServerUserWriteRequest_t;

// iBeacon structure and data
static struct {
  uint8_t flagsLen;     /* Length of the Flags field. */
  uint8_t flagsType;    /* Type of the Flags field. */
  uint8_t flags;        /* Flags field. */
  uint8_t mandataLen;   /* Length of the Manufacturer Data field. */
  uint8_t mandataType;  /* Type of the Manufacturer Data field. */
  uint8_t compId[2];    /* Company ID field. */
  uint8_t beacType[2];  /* Beacon Type field. */
  uint8_t uuid[16];     /* 128-bit Universally Unique Identifier (UUID). The UUID is an identifier for the company using the beacon*/
  uint8_t majNum[2];    /* Beacon major number. Used to group related beacons. */
  uint8_t minNum[2];    /* Beacon minor number. Used to specify individual beacons within a group.*/
  uint8_t txPower;      /* The Beacon's measured RSSI at 1 meter distance in dBm. See the iBeacon specification for measurement guidelines. */
}
iBeaconData
  = {
  /* Flag bits - See Bluetooth 4.0 Core Specification , Volume 3, Appendix C, 18.1 for more details on flags. */
  2,  /* length  */
  0x01, /* type */
  0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */

  /* Manufacturer specific data */
  26,  /* length of field*/
  0xFF, /* type of field */

  /* The first two data octets shall contain a company identifier code from
   * the Assigned Numbers - Company Identifiers document */
  { UINT16_TO_BYTES(0x004C) },

  /* Beacon type */
  /* 0x0215 is iBeacon */
  { UINT16_TO_BYTE1(0x0215), UINT16_TO_BYTE0(0x0215) },

  /* 128 bit / 16 byte UUID - generated specially for the DMP Demo */
  { 0x00, 0x47, 0xe7, 0x0a, 0x5d, 0xc1, 0x47, 0x25, 0x87, 0x99, 0x83, 0x05, 0x44, 0xae, 0x04, 0xf6 },

  /* Beacon major number - not used for this application */
  { UINT16_TO_BYTE1(0), UINT16_TO_BYTE0(0) },

  /* Beacon minor number  - not used for this application*/
  { UINT16_TO_BYTE1(0), UINT16_TO_BYTE0(0) },

  /* The Beacon's measured RSSI at 1 meter distance in dBm */
  /* 0xC3 is -61dBm */
  // TBD: check?
  0xC3
  };

static struct {
  uint8_t flagsLen;          /**< Length of the Flags field. */
  uint8_t flagsType;         /**< Type of the Flags field. */
  uint8_t flags;             /**< Flags field. */
  uint8_t serLen;            /**< Length of Complete list of 16-bit Service UUIDs. */
  uint8_t serType;           /**< Complete list of 16-bit Service UUIDs. */
  uint8_t serviceList[2];    /**< Complete list of 16-bit Service UUIDs. */
  uint8_t serDataLength;     /**< Length of Service Data. */
  uint8_t serDataType;       /**< Type of Service Data. */
  uint8_t uuid[2];           /**< 16-bit Eddystone UUID. */
  uint8_t frameType;         /**< Frame type. */
  uint8_t txPower;           /**< The Beacon's measured RSSI at 0 meter distance in dBm. */
  uint8_t urlPrefix;         /**< URL prefix type. */
  uint8_t url[10];           /**< URL. */
} eddystone_data = {
  /* Flag bits - See Bluetooth 4.0 Core Specification , Volume 3, Appendix C, 18.1 for more details on flags. */
  2,  /* length  */
  0x01, /* type */
  0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */
  /* Service field length */
  0x03,
  /* Service field type */
  0x03,
  /* 16-bit Eddystone UUID */
  { UINT16_TO_BYTES(0xFEAA) },
  /* Eddystone-TLM Frame length */
  0x10,
  /* Service Data data type value */
  0x16,
  /* 16-bit Eddystone UUID */
  { UINT16_TO_BYTES(0xFEAA) },
  /* Eddystone-URL Frame type */
  0x10,
  /* Tx power */
  0x00,
  /* URL prefix - standard */
  0x00,
  /* URL */
  { 's', 'i', 'l', 'a', 'b', 's', '.', 'c', 'o', 'm' }
};

static struct {
  uint16_t txDelayMs;
  uint8_t connHandle;
  uint16_t characteristicHandle;
  uint8_t size;
} bleTxTestParams;

struct responseData_t{
  uint8_t flagsLen;          /**< Length of the Flags field. */
  uint8_t flagsType;         /**< Type of the Flags field. */
  uint8_t flags;             /**< Flags field. */
  uint8_t shortNameLen;      /**< Length of Shortened Local Name. */
  uint8_t shortNameType;     /**< Shortened Local Name. */
  uint8_t shortName[DEVNAME_LEN]; /**< Shortened Local Name. */
  uint8_t uuidLength;        /**< Length of UUID. */
  uint8_t uuidType;          /**< Type of UUID. */
  uint8_t uuid[UUID_LEN];    /**< 128-bit UUID. */
};

static struct responseData_t responseData = {
  2,  /* length (incl type) */
  0x01, /* type */
  0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */
  DEVNAME_LEN + 1,        // length of local name (incl type)
  0x08,               // shortened local name
  { 'D', 'M', '0', '0', ':', '0', '0' },
  UUID_LEN + 1,           // length of UUID data (incl type)
  0x06,               // incomplete list of service UUID's
  // custom service UUID for silabs lamp in little-endian format
  { 0xc9, 0x1b, 0x80, 0x3d, 0x61, 0x50, 0x0c, 0x97, 0x8d, 0x45, 0x19, 0x7d, 0x96, 0x5b, 0xe5, 0xba }
};

static const AppCfgGattServerUserReadRequest_t appCfgGattServerUserReadRequest[] =
{
  { gattdb_light_state, readLightState },
  { gattdb_trigger_source, readTriggerSource },
  { 0, NULL }
};

static const AppCfgGattServerUserWriteRequest_t appCfgGattServerUserWriteRequest[] =
{
  { gattdb_light_state, writeLightState },
  { 0, NULL }
};

size_t appCfgGattServerUserReadRequestSize = COUNTOF(appCfgGattServerUserReadRequest) - 1;
size_t appCfgGattServerUserWriteRequestSize = COUNTOF(appCfgGattServerUserWriteRequest) - 1;

/**
 * Custom CLI.  This command tree is executed by typing "custom <command>"
 * See app/util/serial/command-interpreter2.h for more detail on writing commands.
 **/
/*  Example sub-menu */
//  extern void doSomethingFunction(void);
//  static EmberCommandEntry customSubMenu[] = {
//    emberCommandEntryAction("do-something", doSomethingFunction, "", "Do something description"),
//    emberCommandEntryTerminator()
//  };
//  extern void actionFunction(void);

static void readLightState(uint8_t connection)
{
#if defined(DMP_DEBUG)
  emberAfCorePrintln("Light state = %d\r\n", ble_lightState);
#endif //DMP_DEBUG
  /* Send response to read request */
  gecko_cmd_gatt_server_send_user_read_response(connection, gattdb_light_state, 0,
                                                sizeof(ble_lightState), &ble_lightState);
}

static void readTriggerSource(uint8_t connection)
{
#if defined(DMP_DEBUG)
  emberAfCorePrintln("Last event = %d\r\n", ble_lastEvent);
#endif //DMP_DEBUG
  /* Send response to read request */
  gecko_cmd_gatt_server_send_user_read_response(connection, gattdb_trigger_source, 0,
                                                sizeof(ble_lastEvent), &ble_lastEvent);
}

static void writeLightState(uint8_t connection, uint8array *writeValue)
{
#if defined(DMP_DEBUG)
  emberAfCorePrintln("Light state write; %d\r\n", writeValue->data[0]);
#endif //DMP_DEBUG
  lightDirection = DMP_UI_DIRECTION_BLUETOOTH;
  emberAfWriteAttribute(emberAfPrimaryEndpoint(),
                        ZCL_ON_OFF_CLUSTER_ID,
                        ZCL_ON_OFF_ATTRIBUTE_ID,
                        CLUSTER_MASK_SERVER,
                        (int8u *)&writeValue->data[0],
                        ZCL_BOOLEAN_ATTRIBUTE_TYPE);
  gecko_cmd_gatt_server_send_user_write_response(
    connection,
    gattdb_light_state,
    ES_WRITE_OK
    );
}

void bleIndicateRespEventHandler(void)
{
  emberAfCorePrintln("ble indication timer expired... disable the GATT transactions\r\n");
  emberEventControlSetInactive(bleIndicateControl);
  ble_client_Confirm = FALSE;
  ble_lightState_config = GAT_DISABLED;
  ble_lastEvent_config  = GAT_DISABLED;
}

static void notifyLight(uint8_t connection, uint8_t lightState)
{
  ble_lightState = lightState;
  if (ble_lightState_config == GAT_RECEIVE_INDICATION) {
#if defined(DMP_DEBUG)
    emberAfCorePrintln("notifyLight : ble_lightState_config = %d,ble_client_Confirm = %d\r\n", ble_lightState_config, ble_client_Confirm);
#endif //DMP_DEBUG
    if (!ble_client_Confirm) {
#if defined(DMP_DEBUG)
      emberAfCorePrintln("Light state = %d\r\n", lightState);
#endif //DMP_DEBUG
      /* Send notification/indication data */
      gecko_cmd_gatt_server_send_characteristic_notification(connection, gattdb_light_state, sizeof(lightState), &lightState);
      ble_client_Confirm = TRUE;
      emberEventControlSetDelayMS(bleIndicateControl, BLE_INDICATION_TIMEOUT);
    }
  }
}

static void notifyTriggerSource(uint8_t connection, uint8_t triggerSource)
{
  if (ble_lastEvent_config == GAT_RECEIVE_INDICATION) {
    if (!ble_client_Confirm) {
#if defined(DMP_DEBUG)
      emberAfCorePrintln("notifyTriggerSource : ble_lastEvent_config = %d, ble_client_Confirm = %d\r\n", ble_lastEvent_config, ble_client_Confirm);
#endif //DMP_DEBUG
#if defined(DMP_DEBUG)
      emberAfCorePrintln("Last event = %d\r\n", triggerSource);
#endif //DMP_DEBUG
      /* Send notification/indication data */
      gecko_cmd_gatt_server_send_characteristic_notification(connection, gattdb_trigger_source, sizeof(triggerSource), &triggerSource);
      ble_client_Confirm = TRUE;
      emberEventControlSetDelayMS(bleIndicateControl, BLE_INDICATION_TIMEOUT);
    }
  }
}

void TgToggleOnoffAttribute(void)
{
  EmberStatus status;
  int8u data;
  status =  emberAfReadAttribute(emberAfPrimaryEndpoint(),
                                 ZCL_ON_OFF_CLUSTER_ID,
                                 ZCL_ON_OFF_ATTRIBUTE_ID,
                                 CLUSTER_MASK_SERVER,
                                 (int8u*)&data,
                                 sizeof(data),
                                 NULL);

  if (status == EMBER_ZCL_STATUS_SUCCESS) {
    if (data == 0x00) {
      data = 0x01;
    } else {
      data = 0x00;
    }

    lightDirection = DMP_UI_DIRECTION_SWITCH;
  } else {
#if defined(DMP_DEBUG)
    emberAfAppPrintln("read onoff attr: 0x%x", status);
#endif //DMP_DEBUG
  }

  status = emberAfWriteAttribute(emberAfPrimaryEndpoint(),
                                 ZCL_ON_OFF_CLUSTER_ID,
                                 ZCL_ON_OFF_ATTRIBUTE_ID,
                                 CLUSTER_MASK_SERVER,
                                 (int8u *)&data,
                                 ZCL_BOOLEAN_ATTRIBUTE_TYPE);
#if defined(DMP_DEBUG)
  emberAfAppPrintln("write to onoff attr: 0x%x", status);
#endif //DMP_DEBUG
}

void TgSetDefaultReportEntry(void)
{
  EmberAfPluginReportingEntry reportingEntry;
  emberAfClearReportTableCallback();
  reportingEntry.direction = EMBER_ZCL_REPORTING_DIRECTION_REPORTED;
  reportingEntry.endpoint = emberAfPrimaryEndpoint();
  reportingEntry.clusterId = ZCL_ON_OFF_CLUSTER_ID;
  reportingEntry.attributeId = ZCL_ON_OFF_ATTRIBUTE_ID;
  reportingEntry.mask = CLUSTER_MASK_SERVER;
  reportingEntry.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;
  reportingEntry.data.reported.minInterval = 0x0001;
  reportingEntry.data.reported.maxInterval = 0x0002;
  reportingEntry.data.reported.reportableChange = 0; // onoff is boolean type so it is unused
  emberAfPluginReportingConfigureReportedAttribute(&reportingEntry);
}

EmberStatus TgPjoinAndIdentifying(uint16_t identifyTime)
{
  EmberStatus status;
  EmberEUI64 wildcardEui64 = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
  EmberKeyData centralizedKey = { 0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39 };
  emberAddTransientLinkKey(wildcardEui64, &centralizedKey);
  status = emberPermitJoining(identifyTime);
  emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                              ZCL_IDENTIFY_CLUSTER_ID,
                              ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                              (uint8_t *)&identifyTime,
                              sizeof(identifyTime));

  return status;
}

void buttonEventHandler(void)
{
  EmberStatus status;
  emberEventControlSetInactive(buttonEventControl);

  EmberNetworkStatus state = emberAfNetworkState();
  if (lastButton == BUTTON0) {
    TgToggleOnoffAttribute();
  } else if (lastButton == BUTTON1) {
    if (state != EMBER_JOINED_NETWORK) {
      dmpUiDisplayZigBeeState(DMP_UI_FORMING);
      status = emberAfPluginNetworkCreatorStart(true); // centralized
#if defined(DMP_DEBUG)
      emberAfCorePrintln("%p network %p: 0x%X",
                         "Form centralized",
                         "start",
                         status);
#endif //DMP_DEBUG
    } else { // joined on NWK
      if (longPress == false) {
        if (emberGetPermitJoining()) {
          emberPermitJoining(0);
        } else {
          if (TgPjoinAndIdentifying(180) == EMBER_SUCCESS) {
            dmpUiZigBeePjoin(DMP_UI_PJOIN_EVENT_DURATION);
  #if defined(DMP_DEBUG)
            emberAfAppPrintln("pJoin for 180 sec: 0x%x", status);
  #endif //DMP_DEBUG
          }
        }
      } else {
        status = emberLeaveNetwork();
        emberClearBindingTable();
#if defined(DMP_DEBUG)
        emberAfAppPrintln("leave NWK: 0x%x", status);
#endif //DMP_DEBUG
      }
    }
  }
}

void lcdMainMenuDisplayEventHandler(void)
{
  emberEventControlSetInactive(lcdMainMenuDisplayEventControl);
  dmpUiLightOff();
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
boolean emberAfStackStatusCallback(EmberStatus status)
{
  switch (status) {
    case EMBER_NETWORK_UP:
      dmpUiDisplayZigBeeState(DMP_UI_NETWORK_UP);
      if (TgPjoinAndIdentifying(180) == EMBER_SUCCESS) {
        dmpUiZigBeePjoin(DMP_UI_PJOIN_EVENT_DURATION);
#if defined(DMP_DEBUG)
        emberAfCorePrintln("%p network %p: 0x%X", "Open", "for joining", status);
#endif //DMP_DEBUG
      }
      break;

    case EMBER_NETWORK_DOWN:
      emberEventControlSetInactive(lcdPermitJoinEventControl);
      dmpUiDisplayZigBeeState(DMP_UI_NO_NETWORK);
      longPress = false;
      break;
  }

  return false;
}

static void bleConnectionInfoTableInit(void)
{
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_BLE_MAX_CONNECTIONS; i++) {
    bleConnectionTable[i].inUse = false;
  }
}

static uint8_t bleConnectionInfoTableLookup(uint8_t connHandle)
{
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_BLE_MAX_CONNECTIONS; i++) {
    if (bleConnectionTable[i].inUse
        && bleConnectionTable[i].connectionHandle == connHandle) {
      return i;
    }
  }
  return 0xFF;
}

/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup. Any
 * code that you would normally put into the top of the application's main()
 * routine should be put into this function. This is called before the clusters,
 * plugins, and the network are initialized so some functionality is not yet
 * available.
        Note: No callback in the Application Framework is
 * associated with resource cleanup. If you are implementing your application on
 * a Unix host where resource cleanup is a consideration, we expect that you
 * will use the standard Posix system calls, including the use of atexit() and
 * handlers for signals such as SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If
 * you use the signal() function to register your signal handler, please mind
 * the returned value which may be an Application Framework function. If the
 * return value is non-null, please make sure that you call the returned
 * function from your handler to avoid negating the resource cleanup of the
 * Application Framework itself.
 *
 */
void emberAfMainInitCallback(void)
{
  dmpUiInit();
  dmpUiDisplayHelp();
  emberEventControlSetDelayMS(lcdMainMenuDisplayEventControl, 10000);
  TgSetDefaultReportEntry();
  bleConnectionInfoTableInit();
}

/** @brief On/off Cluster Server Init
 *
 * Server Init
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfOnOffClusterServerInitCallback(int8u endpoint)
{
}

/** @brief On/off Cluster Server Attribute Changed
 *
 * Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfOnOffClusterServerAttributeChangedCallback(int8u endpoint,
                                                       EmberAfAttributeId attributeId)
{
  EmberStatus status;
  int8u data;

  if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID) {
    status =  emberAfReadAttribute(endpoint,
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   CLUSTER_MASK_SERVER,
                                   (int8u*)&data,
                                   sizeof(data),
                                   NULL);

    if (status == EMBER_ZCL_STATUS_SUCCESS) {
      if (data == 0x00) {
        halClearLed(BOARDLED0);
        halClearLed(BOARDLED1);
        dmpUiLightOff();
        notifyLight(currentConnection, 0);
      } else {
        halSetLed(BOARDLED0);
        halSetLed(BOARDLED1);
        notifyLight(currentConnection, 1);
        dmpUiLightOn();
      }
      if ( (lightDirection == DMP_UI_DIRECTION_BLUETOOTH)
           || (lightDirection == DMP_UI_DIRECTION_SWITCH) ) {
        dmpUiUpdateDirection(lightDirection);
      } else {
        lightDirection = DMP_UI_DIRECTION_ZIGBEE;
        dmpUiUpdateDirection(lightDirection);
      }
      ble_lastEvent = lightDirection;
      lightDirection = DMP_UI_DIRECTION_INVALID;

      if (ble_lastEvent != DMP_UI_DIRECTION_INVALID) {
        if ( (ble_lightState_config != GAT_RECEIVE_INDICATION) && (ble_lastEvent_config == GAT_RECEIVE_INDICATION)) {
          notifyTriggerSource(currentConnection, ble_lastEvent);
        }
      }
    }
  } else {
  }
}

/** @brief Join
 *
 * This callback is called by the ZLL Commissioning plugin when a joinable
 * network has been found. If the application returns true, the plugin will
 * attempt to join the network. Otherwise, the plugin will ignore the network
 * and continue searching. Applications can use this callback to implement a
 * network blacklist. Note that this callback is not called during touch
 * linking.
 *
 * @param networkFound   Ver.: always
 * @param lqi   Ver.: always
 * @param rssi   Ver.: always
 */
bool emberAfPluginZllCommissioningJoinCallback(EmberZigbeeNetwork *networkFound,
                                               uint8_t lqi,
                                               int8_t rssi)
{
  return true;
}

/** @brief Ok To Sleep
 *
 * This function is called by the Idle/Sleep plugin before sleeping. It is
 * called with interrupts disabled. The application should return true if the
 * device may sleep or false otherwise.
 *
 * @param durationMs The maximum duration in milliseconds that the device will
 * sleep. Ver.: always
 */
bool emberAfPluginIdleSleepOkToSleepCallback(uint32_t durationMs)
{
  return true;
}

/** @brief Wake Up
 *
 * This function is called by the Idle/Sleep plugin after sleeping.
 *
 * @param durationMs The duration in milliseconds that the device slept.
 * Ver.: always
 */
void emberAfPluginIdleSleepWakeUpCallback(uint32_t durationMs)
{
}

/** @brief Ok To Idle
 *
 * This function is called by the Idle/Sleep plugin before idling. It is called
 * with interrupts disabled. The application should return true if the device
 * may idle or false otherwise.
 *
 */
bool emberAfPluginIdleSleepOkToIdleCallback(void)
{
  return true;
}

/** @brief Active
 *
 * This function is called by the Idle/Sleep plugin after idling.
 *
 */
void emberAfPluginIdleSleepActiveCallback(void)
{
}

/** @brief Get Group Name
 *
 * This function returns the name of a group with the provided group ID, should
 * it exist.
 *
 * @param endpoint Endpoint Ver.: always
 * @param groupId Group ID Ver.: always
 * @param groupName Group Name Ver.: always
 */
void emberAfPluginGroupsServerGetGroupNameCallback(uint8_t endpoint,
                                                   uint16_t groupId,
                                                   uint8_t *groupName)
{
}

/** @brief Set Group Name
 *
 * This function sets the name of a group with the provided group ID.
 *
 * @param endpoint Endpoint Ver.: always
 * @param groupId Group ID Ver.: always
 * @param groupName Group Name Ver.: always
 */
void emberAfPluginGroupsServerSetGroupNameCallback(uint8_t endpoint,
                                                   uint16_t groupId,
                                                   uint8_t *groupName)
{
}

/** @brief Group Names Supported
 *
 * This function returns whether or not group names are supported.
 *
 * @param endpoint Endpoint Ver.: always
 */
bool emberAfPluginGroupsServerGroupNamesSupportedCallback(uint8_t endpoint)
{
  return false;
}

/** @brief Initial Security State
 *
 * This function is called by the ZLL Commissioning plugin to determine the
 * initial security state to be used by the device. The application must
 * populate the ::EmberZllInitialSecurityState structure with a configuration
 * appropriate for the network being formed, joined, or started. Once the
 * device forms, joins, or starts a network, the same security configuration
 * will remain in place until the device leaves the network.
 *
 * @param securityState The security configuration to be populated by the
 * application and ultimately set in the stack. Ver.: always
 */
void emberAfPluginZllCommissioningInitialSecurityStateCallback(EmberZllInitialSecurityState *securityState)
{
}

/** @brief Touch Link Complete
 *
 * This function is called by the ZLL Commissioning plugin when touch linking
 * completes.
 *
 * @param networkInfo The ZigBee and ZLL-specific information about the network
 * and target. Ver.: always
 * @param deviceInformationRecordCount The number of sub-device information
 * records for the target. Ver.: always
 * @param deviceInformationRecordList The list of sub-device information
 * records for the target. Ver.: always
 */
void emberAfPluginZllCommissioningTouchLinkCompleteCallback(const EmberZllNetwork *networkInfo,
                                                            uint8_t deviceInformationRecordCount,
                                                            const EmberZllDeviceInfoRecord *deviceInformationRecordList)
{
}

/** @brief Touch Link Failed
 *
 * This function is called by the ZLL Commissioning plugin if touch linking
 * fails.
 *
 * @param status The reason the touch link failed. Ver.: always
 */
void emberAfPluginZllCommissioningTouchLinkFailedCallback(EmberAfZllCommissioningStatus status)
{
}

/** @brief Group Identifier Count
 *
 * This function is called by the ZLL Commissioning plugin to determine the
 * number of group identifiers in use by a specific endpoint on the device. The
 * total number of group identifiers on the device, which are shared by all
 * endpoints, is defined by ::EMBER_ZLL_GROUP_ADDRESSES.
 *
 * @param endpoint The endpoint for which the group identifier count is
 * requested. Ver.: always
 */
uint8_t emberAfPluginZllCommissioningGroupIdentifierCountCallback(uint8_t endpoint)
{
  return 0x00;
}

/** @brief Group Identifier
 *
 * This function is called by the ZLL Commissioning plugin to obtain
 * information about the group identifiers in use by a specific endpoint on the
 * device. The application should populate the record with information about
 * the group identifier and return true. If no information is available for the
 * given endpoint and index, the application should return false.
 *
 * @param endpoint The endpoint for which the group identifier is requested.
 * Ver.: always
 * @param index The index of the group on the endpoint. Ver.: always
 * @param record The group information record. Ver.: always
 */
bool emberAfPluginZllCommissioningGroupIdentifierCallback(uint8_t endpoint,
                                                          uint8_t index,
                                                          EmberAfPluginZllCommissioningGroupInformationRecord *record)
{
  return false;
}

/** @brief Endpoint Information Count
 *
 * This function is called by the ZLL Commissioning plugin to determine the
 * number of remote endpoints controlled by a specific endpoint on the local
 * device.
 *
 * @param endpoint The local endpoint for which the remote endpoint information
 * count is requested. Ver.: always
 */
uint8_t emberAfPluginZllCommissioningEndpointInformationCountCallback(uint8_t endpoint)
{
  return 0x00;
}

/** @brief Endpoint Information
 *
 * This function is called by the ZLL Commissioning plugin to obtain
 * information about the remote endpoints controlled by a specific endpoint on
 * the local device. The application should populate the record with
 * information about the remote endpoint and return true. If no information is
 * available for the given endpoint and index, the application should return
 * false.
 *
 * @param endpoint The local endpoint for which the remote endpoint information
 * is requested. Ver.: always
 * @param index The index of the remote endpoint information on the local
 * endpoint. Ver.: always
 * @param record The endpoint information record. Ver.: always
 */
bool emberAfPluginZllCommissioningEndpointInformationCallback(uint8_t endpoint,
                                                              uint8_t index,
                                                              EmberAfPluginZllCommissioningEndpointInformationRecord *record)
{
  return false;
}

/** @brief Identify
 *
 * This function is called by the ZLL Commissioning plugin to notify the
 * application that it should take an action to identify itself. This typically
 * occurs when an Identify Request is received via inter-PAN messaging.
 *
 * @param durationS If the duration is zero, the device should exit identify
 * mode. If the duration is 0xFFFF, the device should remain in identify mode
 * for the default time. Otherwise, the duration specifies the length of time
 * in seconds that the device should remain in identify mode. Ver.: always
 */
void emberAfPluginZllCommissioningIdentifyCallback(uint16_t durationS)
{
}

/** @brief Reset To Factory New
 *
 * This function is called by the ZLL Commissioning plugin when a request to
 * reset to factory new is received. The plugin will leave the network, reset
 * attributes managed by the framework to their default values, and clear the
 * group and scene tables. The application should perform any other necessary
 * reset-related operations in this callback, including resetting any
 * externally-stored attributes.
 *
 */
void emberAfPluginZllCommissioningResetToFactoryNewCallback(void)
{
}

/** @brief Off With Effect
 *
 * This callback is called by the ZLL On/Off Server plugin whenever an
 * OffWithEffect command is received. The application should implement the
 * effect and variant requested in the command and return
 * ::EMBER_ZCL_STATUS_SUCCESS if successful or an appropriate error status
 * otherwise.
 *
 * @param endpoint   Ver.: always
 * @param effectId   Ver.: always
 * @param effectVariant   Ver.: always
 */
EmberAfStatus emberAfPluginZllOnOffServerOffWithEffectCallback(uint8_t endpoint,
                                                               uint8_t effectId,
                                                               uint8_t effectVariant)
{
  return EMBER_ZCL_STATUS_SUCCESS;
}

/** @brief
 *
 * This function is called from the BLE stack to notify the application of a
 * stack event.
 */
void emberAfPluginBleEventCallback(struct gecko_cmd_packet* evt)
{
  switch (BGLIB_MSG_ID(evt->header)) {
    /* This event indicates that a remote GATT client is attempting to read a value of an
     *  attribute from the local GATT database, where the attribute was defined in the GATT
     *  XML firmware configuration file to have type="user". */

    case gecko_evt_gatt_server_user_read_request_id:
      for (int i = 0; i < appCfgGattServerUserReadRequestSize; i++) {
        if ( (appCfgGattServerUserReadRequest[i].charId
              == evt->data.evt_gatt_server_user_read_request.characteristic)
             && (appCfgGattServerUserReadRequest[i].fctn) ) {
          appCfgGattServerUserReadRequest[i].fctn(evt->data.evt_gatt_server_user_read_request.connection);
        }
      }
      break;

    /* This event indicates that a remote GATT client is attempting to write a value of an
     * attribute in to the local GATT database, where the attribute was defined in the GATT
     * XML firmware configuration file to have type="user".  */

    case gecko_evt_gatt_server_user_write_request_id:
      for (int i = 0; i < appCfgGattServerUserWriteRequestSize; i++) {
        if ( (appCfgGattServerUserWriteRequest[i].charId
              == evt->data.evt_gatt_server_characteristic_status.characteristic)
             && (appCfgGattServerUserWriteRequest[i].fctn) ) {
          appCfgGattServerUserWriteRequest[i].fctn(evt->data.evt_gatt_server_user_read_request.connection,
                                                   &(evt->data.evt_gatt_server_attribute_value.value));
        }
      }
      break;

    case gecko_evt_system_boot_id:
    {
      struct gecko_msg_system_hello_rsp_t *hello_rsp;
      struct gecko_msg_system_get_bt_address_rsp_t *get_address_rsp;

      // Call these two APIs upon boot for testing purposes.
      hello_rsp = gecko_cmd_system_hello();
      get_address_rsp = gecko_cmd_system_get_bt_address();
#if defined(DMP_DEBUG)
      emberAfCorePrintln("BLE hello: %s",
                         (hello_rsp->result == bg_err_success) ? "success" : "error");
      emberAfCorePrint("BLE address: ");
      printBleAddress(get_address_rsp->address.addr);
      emberAfCorePrintln("");
#endif //DMP_DEBUG
      // start advertising
      enableBleAdvertisements();
    }
    break;
    case gecko_evt_gatt_server_characteristic_status_id:
    {
      bool isTimerRunning = 0;
      struct gecko_msg_gatt_server_characteristic_status_evt_t *StatusEvt =
        (struct gecko_msg_gatt_server_characteristic_status_evt_t*)&(evt->data);
      emberAfCorePrintln("GAT_SERVER_CONFIRMATION= %d\r\n", StatusEvt->status_flags);
      emberAfCorePrintln("characteristic= %d , GAT_SERVER_CLIENT_CONFIG_FLAG = %d\r\n", StatusEvt->characteristic, StatusEvt->client_config_flags);
      if (StatusEvt->status_flags == GAT_SERVER_CONFIRMATION) {
        ble_client_Confirm = FALSE;
        emberEventControlSetInactive(bleIndicateControl);
        if ( (ble_lightState_config == GAT_RECEIVE_INDICATION) && (ble_lastEvent_config == GAT_RECEIVE_INDICATION)) {
          if (StatusEvt->characteristic == gattdb_light_state) {
            notifyTriggerSource(currentConnection, ble_lastEvent);
          }
        }
      } else if (StatusEvt->status_flags == GAT_SERVER_CLIENT_CONFIG) {
        if (StatusEvt->characteristic == gattdb_light_state) {
          ble_lightState_config = StatusEvt->client_config_flags;
        } else if (StatusEvt->characteristic == gattdb_trigger_source) {
          ble_lastEvent_config = StatusEvt->client_config_flags;
        }
        if ((ble_lastEvent_config == GAT_RECEIVE_INDICATION) || (ble_lightState_config == GAT_RECEIVE_INDICATION)) {
          ble_client_Confirm = FALSE;
          emberEventControlSetInactive(bleIndicateControl);
        }
      }
#if defined(DMP_DEBUG)
      emberAfCorePrintln("SERVER : ble_lightState_config= %d , ble_lastEvent_config = %d\r\n", ble_lightState_config, ble_lastEvent_config);
#endif //DMP_DEBUG
    }
    break;
    case gecko_evt_le_gap_scan_request_id:
    {
      emberAfCorePrintln("received a scan request from %X:%X:%X:%X:%X:%X:\r\n",
                         evt->data.evt_le_gap_scan_request.address.addr[5],
                         evt->data.evt_le_gap_scan_request.address.addr[4],
                         evt->data.evt_le_gap_scan_request.address.addr[3],
                         evt->data.evt_le_gap_scan_request.address.addr[2],
                         evt->data.evt_le_gap_scan_request.address.addr[1],
                         evt->data.evt_le_gap_scan_request.address.addr[0]);
    }
    break;
    case gecko_evt_le_connection_bt5_opened_id:
    {
      emberAfCorePrintln("BT5 : gecko_evt_le_connection_bt5_opened_id \n");
      struct gecko_msg_le_connection_bt5_opened_evt_t *conn_evt =
        (struct gecko_msg_le_connection_bt5_opened_evt_t*)&(evt->data);
      uint8_t index = bleConnectionInfoTableFindUnused();
      assert(index < 0xFF);

      bleConnectionTable[index].inUse = true;
      bleConnectionTable[index].isMaster = (conn_evt->master > 0);
      bleConnectionTable[index].connectionHandle = conn_evt->connection;
      bleConnectionTable[index].bondingHandle = conn_evt->bonding;
      memcpy(bleConnectionTable[index].remoteAddress, conn_evt->address.addr, 6);

      // for a server, we have a single connection
      currentConnection = conn_evt->connection;
      bleConnected = true;

      // restart advertising, but non-connectable
 #if defined(DMP_DEBUG)
      emberAfCorePrintln("BLE connection opened");
 #endif//DMP_DEBUG
      bleConnectionInfoTablePrintEntry(index);
    }
    break;
    case gecko_evt_le_connection_opened_id:
    {
      emberAfCorePrintln("BT4 :gecko_evt_le_connection_opened_id \n");
    }
    break;
    case gecko_evt_le_connection_closed_id:
    {
      struct gecko_msg_le_connection_closed_evt_t *conn_evt =
        (struct gecko_msg_le_connection_closed_evt_t*)&(evt->data);
      uint8_t index = bleConnectionInfoTableLookup(conn_evt->connection);
      assert(index < 0xFF);

      bleConnectionTable[index].inUse = false;
      currentConnection = 0;   // ??
      bleConnected = false;

      // restart advertising, set connectable
      enableBleAdvertisements();

      dmpUiBluetoothConnected(false);
#if defined(DMP_DEBUG)
      emberAfCorePrintln("BLE connection closed, handle=0x%x, reason=0x%2x",
                         conn_evt->connection, conn_evt->reason);
#endif //DMP_DEBUG
    }
    break;
    case gecko_evt_le_gap_scan_response_id:
    {
      struct gecko_msg_le_gap_scan_response_evt_t *scan_evt =
        (struct gecko_msg_le_gap_scan_response_evt_t*)&(evt->data);
#if defined(DMP_DEBUG)
      emberAfCorePrint("Scan response, address type=0x%x, address: ",
                       scan_evt->address_type);
      printBleAddress(scan_evt->address.addr);
      emberAfCorePrintln("");
#endif //DMP_DEBUG
    }
    break;
    case gecko_evt_sm_list_bonding_entry_id:
    {
      struct gecko_msg_sm_list_bonding_entry_evt_t * bonding_entry_evt =
        (struct gecko_msg_sm_list_bonding_entry_evt_t*)&(evt->data);
#if defined(DMP_DEBUG)
      emberAfCorePrint("Bonding handle=0x%x, address type=0x%x, address: ",
                       bonding_entry_evt->bonding, bonding_entry_evt->address_type);
      printBleAddress(bonding_entry_evt->address.addr);
      emberAfCorePrintln("");
#endif //DMP_DEBUG
    }
    break;
    case gecko_evt_gatt_service_id:
    {
      struct gecko_msg_gatt_service_evt_t* service_evt =
        (struct gecko_msg_gatt_service_evt_t*)&(evt->data);
      uint8_t i;
#if defined(DMP_DEBUG)
      emberAfCorePrintln("GATT service, conn_handle=0x%x, service_handle=0x%4x",
                         service_evt->connection, service_evt->service);
      emberAfCorePrint("UUID=[");
      for (i = 0; i < service_evt->uuid.len; i++) {
        emberAfCorePrint("0x%x ", service_evt->uuid.data[i]);
      }
      emberAfCorePrintln("]");
#endif //DMP_DEBUG
    }
    break;
    case gecko_evt_gatt_characteristic_id:
    {
      struct gecko_msg_gatt_characteristic_evt_t* char_evt =
        (struct gecko_msg_gatt_characteristic_evt_t*)&(evt->data);
      uint8_t i;
#if defined(DMP_DEBUG)
      emberAfCorePrintln("GATT characteristic, conn_handle=0x%x, char_handle=0x%2x, properties=0x%x",
                         char_evt->connection, char_evt->characteristic, char_evt->properties);
      emberAfCorePrint("UUID=[");
      for (i = 0; i < char_evt->uuid.len; i++) {
        emberAfCorePrint("0x%x ", char_evt->uuid.data[i]);
      }
      emberAfCorePrintln("]");
#endif //DMP_DEBUG
    }
    break;
    case gecko_evt_gatt_characteristic_value_id:
    {
      struct gecko_msg_gatt_characteristic_value_evt_t* char_evt =
        (struct gecko_msg_gatt_characteristic_value_evt_t*)&(evt->data);
      uint8_t i;

      if (char_evt->att_opcode == gatt_handle_value_indication) {
        gecko_cmd_gatt_send_characteristic_confirmation(char_evt->connection);
      }
#if defined(DMP_DEBUG)
      emberAfCorePrintln("GATT (client) characteristic value, handle=0x%x, characteristic=0x%2x, att_op_code=0x%x",
                         char_evt->connection,
                         char_evt->characteristic,
                         char_evt->att_opcode);
      emberAfCorePrint("value=[");
      for (i = 0; i < char_evt->value.len; i++) {
        emberAfCorePrint("0x%x ", char_evt->value.data[i]);
      }
      emberAfCorePrintln("]");
#endif //DMP_DEBUG
    }
    break;
    case gecko_evt_gatt_server_attribute_value_id:
    {
      struct gecko_msg_gatt_server_attribute_value_evt_t* attr_evt =
        (struct gecko_msg_gatt_server_attribute_value_evt_t*)&(evt->data);
      EmberStatus status;
      uint8_t i;
#if defined(DMP_DEBUG)
      emberAfCorePrintln("GATT (server) attribute value, handle=0x%x, attribute=0x%2x, att_op_code=0x%x",
                         attr_evt->connection,
                         attr_evt->attribute,
                         attr_evt->att_opcode);
      emberAfCorePrint("value=[");
      for (i = 0; i < attr_evt->value.len; i++) {
        emberAfCorePrint("0x%x ", attr_evt->value.data[i]);
      }
      emberAfCorePrintln("]");
#endif //DMP_DEBUG
      // Forward the attribute over the ZigBee network.
      emberAfWriteAttribute(emberAfPrimaryEndpoint(),
                            ZCL_ON_OFF_CLUSTER_ID,
                            ZCL_ON_OFF_ATTRIBUTE_ID,
                            CLUSTER_MASK_SERVER,
                            (int8u *)attr_evt->value.data,
                            ZCL_BOOLEAN_ATTRIBUTE_TYPE);

      lightDirection = DMP_UI_DIRECTION_BLUETOOTH;
    }
    break;
    case gecko_evt_le_connection_parameters_id:
    {
      struct gecko_msg_le_connection_parameters_evt_t* param_evt =
        (struct gecko_msg_le_connection_parameters_evt_t*)&(evt->data);
#if defined(DMP_DEBUG)
      emberAfCorePrintln("BLE connection parameters are updated, handle=0x%x, interval=0x%2x, latency=0x%2x, timeout=0x%2x, security=0x%x, txsize=0x%2x",
                         param_evt->connection,
                         param_evt->interval,
                         param_evt->latency,
                         param_evt->timeout,
                         param_evt->security_mode,
                         param_evt->txsize);
#endif //DMP_DEBUG
      dmpUiBluetoothConnected(true);
    }
    break;
    case gecko_evt_gatt_procedure_completed_id:
    {
      struct gecko_msg_gatt_procedure_completed_evt_t* proc_comp_evt =
        (struct gecko_msg_gatt_procedure_completed_evt_t*)&(evt->data);
#if defined(DMP_DEBUG)
      emberAfCorePrintln("BLE procedure completed, handle=0x%x, result=0x%2x",
                         proc_comp_evt->connection,
                         proc_comp_evt->result);
#endif //DMP_DEBUG
    }
    break;
  }
}

/** @brief Message Sent
 *
 * This function is called by the application framework from the message sent
 * handler, when it is informed by the stack regarding the message sent status.
 * All of the values passed to the emberMessageSentHandler are passed on to this
 * callback. This provides an opportunity for the application to verify that its
 * message has been sent successfully and take the appropriate action. This
 * callback should return a bool value of true or false. A value of true
 * indicates that the message sent notification has been handled and should not
 * be handled by the application framework.
 *
 * @param type   Ver.: always
 * @param indexOrDestination   Ver.: always
 * @param apsFrame   Ver.: always
 * @param msgLen   Ver.: always
 * @param message   Ver.: always
 * @param status   Ver.: always
 */
boolean emberAfMessageSentCallback(EmberOutgoingMessageType type,
                                   int16u indexOrDestination,
                                   EmberApsFrame* apsFrame,
                                   int16u msgLen,
                                   int8u* message,
                                   EmberStatus status)
{
  return false;
}

/** @brief Hal Button Isr
 *
 * This callback is called by the framework whenever a button is pressed on the
 * device. This callback is called within ISR context.
 *
 * @param button The button which has changed state, either BUTTON0 or BUTTON1
 * as defined in the appropriate BOARD_HEADER.  Ver.: always
 * @param state The new state of the button referenced by the button parameter,
 * either ::BUTTON_PRESSED if the button has been pressed or ::BUTTON_RELEASED
 * if the button has been released.  Ver.: always
 */
void emberAfHalButtonIsrCallback(int8u button,
                                 int8u state)
{
  static uint16_t buttonPressTime;
  uint16_t currentTime = 0;
  if (state == BUTTON_PRESSED) {
    if (button == BUTTON1) {
      buttonPressTime = halCommonGetInt16uMillisecondTick();
    }
  } else if (state == BUTTON_RELEASED) {
    if (button == BUTTON1) {
      currentTime = halCommonGetInt16uMillisecondTick();

      if ((currentTime - buttonPressTime) > BUTTON_LONG_PRESS_TIME_MSEC) {
        longPress = true;
      }
    }
    lastButton = button;
    emberEventControlSetActive(buttonEventControl);
  }
}

/** @brief Pre Command Received
 *
 * This callback is the second in the Application Framework's message processing
 * chain. At this point in the processing of incoming over-the-air messages, the
 * application has determined that the incoming message is a ZCL command. It
 * parses enough of the message to populate an EmberAfClusterCommand struct. The
 * Application Framework defines this struct value in a local scope to the
 * command processing but also makes it available through a global pointer
 * called emberAfCurrentCommand, in app/framework/util/util.c. When command
 * processing is complete, this pointer is cleared.
 *
 * @param cmd   Ver.: always
 */
boolean emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd)
{
  return false;
}

/** @brief Pre Message Received
 *
 * This callback is the first in the Application Framework's message processing
 * chain. The Application Framework calls it when a message has been received
 * over the air but has not yet been parsed by the ZCL command-handling code. If
 * you wish to parse some messages that are completely outside the ZCL
 * specification or are not handled by the Application Framework's command
 * handling code, you should intercept them for parsing in this callback.

 *   This callback returns a Boolean value indicating whether or not the message
 * has been handled. If the callback returns a value of true, then the
 * Application Framework assumes that the message has been handled and it does
 * nothing else with it. If the callback returns a value of false, then the
 * application framework continues to process the message as it would with any
 * incoming message.
        Note:   This callback receives a pointer to an
 * incoming message struct. This struct allows the application framework to
 * provide a unified interface between both Host devices, which receive their
 * message through the ezspIncomingMessageHandler, and SoC devices, which
 * receive their message through emberIncomingMessageHandler.
 *
 * @param incomingMessage   Ver.: always
 */
boolean emberAfPreMessageReceivedCallback(EmberAfIncomingMessage* incomingMessage)
{
  return false;
}

/** @brief Pre Message Send
 *
 * This function is called by the framework when it is about to pass a message
 * to the stack primitives for sending.   This message may or may not be ZCL,
 * ZDO, or some other protocol.  This is called prior to
        any ZigBee
 * fragmentation that may be done.  If the function returns true it is assumed
 * the callback has consumed and processed the message.  The callback must also
 * set the EmberStatus status code to be passed back to the caller.  The
 * framework will do no further processing on the message.
        If the
 * function returns false then it is assumed that the callback has not processed
 * the mesasge and the framework will continue to process accordingly.
 *
 * @param messageStruct The structure containing the parameters of the APS
 * message to be sent.  Ver.: always
 * @param status A pointer to the status code value that will be returned to the
 * caller.  Ver.: always
 */
boolean emberAfPreMessageSendCallback(EmberAfMessageStruct* messageStruct,
                                      EmberStatus* status)
{
  return false;
}

/** @brief Trust Center Join
 *
 * This callback is called from within the application framework's
 * implementation of emberTrustCenterJoinHandler or ezspTrustCenterJoinHandler.
 * This callback provides the same arguments passed to the
 * TrustCenterJoinHandler. For more information about the TrustCenterJoinHandler
 * please see documentation included in stack/include/trust-center.h.
 *
 * @param newNodeId   Ver.: always
 * @param newNodeEui64   Ver.: always
 * @param parentOfNewNode   Ver.: always
 * @param status   Ver.: always
 * @param decision   Ver.: always
 */
void emberAfTrustCenterJoinCallback(EmberNodeId newNodeId,
                                    EmberEUI64 newNodeEui64,
                                    EmberNodeId parentOfNewNode,
                                    EmberDeviceUpdate status,
                                    EmberJoinDecision decision)
{
  if (status == EMBER_DEVICE_LEFT) {
    for (uint8_t i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
      EmberBindingTableEntry entry;
      emberGetBinding(i, &entry);
      if ((entry.type == EMBER_UNICAST_BINDING)
          && (entry.clusterId == ZCL_ON_OFF_CLUSTER_ID)
          && ((MEMCOMPARE(entry.identifier, newNodeEui64, EUI64_SIZE) == 0))) {
        emberDeleteBinding(i);
#if defined(DMP_DEBUG)
        emberAfAppPrintln("deleted binding entry: %d", i);
#endif //DMP_DEBUG
        break;
      }
    }
  } else if (status == EMBER_STANDARD_SECURITY_UNSECURED_JOIN) {
    emberPermitJoining(0);
  }
}

/** @brief Pre ZDO Message Received
 *
 * This function passes the application an incoming ZDO message and gives the
 * appictation the opportunity to handle it. By default, this callback returns
 * false indicating that the incoming ZDO message has not been handled and
 * should be handled by the Application Framework.
 *
 * @param emberNodeId   Ver.: always
 * @param apsFrame   Ver.: always
 * @param message   Ver.: always
 * @param length   Ver.: always
 */
boolean emberAfPreZDOMessageReceivedCallback(EmberNodeId emberNodeId,
                                             EmberApsFrame* apsFrame,
                                             int8u* message,
                                             int16u length)
{
  return false;
}

void bleEventHandler(void)
{
  emberEventControlSetDelayMS(bleEventControl, bleNotificationsPeriodMs);
}

#ifdef ENABLE_CUSTOM_COMMANDS
static void printBleConnectionTableCommand(void)
{
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_BLE_MAX_CONNECTIONS; i++) {
    if (bleConnectionTable[i].inUse) {
      bleConnectionInfoTablePrintEntry(i);
    }
  }
}

static void enableBleNotificationsCommand(void)
{
  bleNotificationsPeriodMs = emberUnsignedCommandArgument(0);
#if defined(DMP_DEBUG)
  emberAfCorePrintln("BLE notifications enabled");
#endif //DMP_DEBUG
  bleEventHandler();
}

static void disableBleNotificationsCommand(void)
{
#if defined(DMP_DEBUG)
  emberAfCorePrintln("BLE notifications disabled");
#endif //DMP_DEBUG
  emberEventControlSetInactive(bleEventControl);
}
#endif

void bleTxEventHandler(void);

EmberCommandEntry emberAfCustomCommands[] = {
#ifdef ENABLE_CUSTOM_COMMANDS
  emberCommandEntryAction("print-ble-connections",
                          printBleConnectionTableCommand,
                          "",
                          "Print BLE connections info"),
  emberCommandEntryAction("enable-ble-notifications",
                          enableBleNotificationsCommand,
                          "v",
                          "Enable BLE temperature notifications"),
  emberCommandEntryAction("disable-ble-notifications",
                          disableBleNotificationsCommand,
                          "",
                          "Disable BLE temperature notifications"),
#endif //ENABLE_CUSTOM_COMMANDS
  emberCommandEntryTerminator(),
};

//------------------------------------------------------------------------------
// BLE connection info table functions
static uint8_t bleConnectionInfoTableFindUnused(void)
{
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_BLE_MAX_CONNECTIONS; i++) {
    if (!bleConnectionTable[i].inUse) {
      return i;
    }
  }
  return 0xFF;
}

static void bleConnectionInfoTablePrintEntry(uint8_t index)
{
  assert(index < EMBER_AF_PLUGIN_BLE_MAX_CONNECTIONS
         && bleConnectionTable[index].inUse);

#if defined(DMP_DEBUG)
  emberAfCorePrintln("**** Connection Info ****");
  emberAfCorePrintln("connection handle 0x%x",
                     bleConnectionTable[index].connectionHandle);
  emberAfCorePrintln("bonding handle = 0x%x",
                     bleConnectionTable[index].bondingHandle);
  emberAfCorePrintln("local node is %s",
                     (bleConnectionTable[index].isMaster) ? "master" : "slave");
  emberAfCorePrint("remote address: ");
  printBleAddress(bleConnectionTable[index].remoteAddress);
  emberAfCorePrintln("");
  emberAfCorePrintln("*************************");
#endif //DMP_DEBUG
}

#if defined(DMP_DEBUG)
static void printBleAddress(uint8_t *address)
{
  emberAfCorePrint("[%x %x %x %x %x %x]",
                   address[5], address[4], address[3],
                   address[2], address[1], address[0]);
}
#endif //DMP_DEBUG

//------------------------------------------------------------------------------
void bleTxEventHandler(void)
{
  uint8_t txData[BLE_TX_TEST_DATA_SIZE];
  uint8_t i;

  for (i = 0; i < BLE_TX_TEST_DATA_SIZE; i++) {
    txData[i] = i;
  }

  gecko_cmd_gatt_write_characteristic_value_without_response(bleTxTestParams.connHandle,
                                                             bleTxTestParams.characteristicHandle,
                                                             BLE_TX_TEST_DATA_SIZE,
                                                             txData);

  emberEventControlSetDelayMS(bleTxEventControl, bleTxTestParams.txDelayMs);
}

/** @brief
 *
 * This function is called at init time. The following fields will be
 * overwritten by the framework:
 *  - max_connections
 *  - heap
 *  - heap_size
 */
void emberAfPluginBleGetConfigCallback(gecko_configuration_t* config)
{
  // Change the BLE configuration here if needed
  config->bluetooth.max_advertisers = 3;
}

static void BeaconAdvertisements(void)
{
  static uint8_t *advData;
  static uint8_t advDataLen;

  advData = (uint8_t*)&iBeaconData;
  advDataLen = sizeof(iBeaconData);
  /* Set custom advertising data */
  gecko_cmd_le_gap_bt5_set_adv_data(1, 0, advDataLen, advData);
  gecko_cmd_le_gap_bt5_set_adv_parameters(1, 160, 160, 7, 0);
  gecko_cmd_le_gap_bt5_set_mode(1, le_gap_user_data, le_gap_non_connectable, 0, le_gap_non_resolvable);

  advData = (uint8_t*)&eddystone_data;
  advDataLen = sizeof(eddystone_data);
  /* Set custom advertising data */
  gecko_cmd_le_gap_bt5_set_adv_data(2, 0, advDataLen, advData);
  gecko_cmd_le_gap_bt5_set_adv_parameters(2, 160, 160, 7, 0);
  gecko_cmd_le_gap_bt5_set_mode(2, le_gap_user_data, le_gap_non_connectable, 0, le_gap_non_resolvable);
}

static void enableBleAdvertisements(void)
{
  /* set transmit power to 0 dBm */
  gecko_cmd_system_set_tx_power(0);

  /* Create the device name based on the 16-bit device ID */
  uint16_t devId;
  struct gecko_msg_system_get_bt_address_rsp_t* btAddr;
  btAddr = gecko_cmd_system_get_bt_address();
  devId = *((uint16*)(btAddr->address.addr));

  // Copy to the local GATT database - this will be used by the BLE stack
  // to put the local device name into the advertisements, but only if we are
  // using default advertisements
  static char devName[DEVNAME_LEN];
  snprintf(devName, DEVNAME_LEN, DEVNAME, devId >> 8, devId & 0xff);
#if defined(DMP_DEBUG)
  emberAfCorePrintln("devName = %s", devName);
#endif // DMP_DEBUG
  gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name,
                                              0,
                                              strlen(devName),
                                              (uint8_t *)devName);

  dmpUiSetBleDeviceName(devName);

  // Copy the shortened device name to the response data, overwriting
  // the default device name which is set at compile time
  MEMCOPY(((uint8_t*)&responseData) + 5, devName, 8);

  // Set the response data
  struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t *rsp;
  rsp = gecko_cmd_le_gap_bt5_set_adv_data(0, 0, sizeof(responseData), (uint8_t*)&responseData);
#if defined(DMP_DEBUG)
  emberAfCorePrintln("set_adv_params: results = 0x%x", rsp->result);
#endif //DMP_DEBUG

  // Set nominal 100ms advertising interval, so we just get
  // a single beacon of each type
  gecko_cmd_le_gap_bt5_set_adv_parameters(0, 160, 160, 7, 1);
  /* Start advertising in user mode and enable connections, if we are
   * not already connected */
  if (bleConnected) {
    gecko_cmd_le_gap_bt5_set_mode(0, le_gap_user_data, le_gap_non_connectable, 0, le_gap_identity_address);
  } else {
    gecko_cmd_le_gap_bt5_set_mode(0, le_gap_user_data, le_gap_undirected_connectable, 0, le_gap_identity_address);
  }
 #if defined(DMP_DEBUG)
  emberAfCorePrintln("BLE custom advertisements enabled");
 #endif // DMP_DEBUG
  BeaconAdvertisements();
}
