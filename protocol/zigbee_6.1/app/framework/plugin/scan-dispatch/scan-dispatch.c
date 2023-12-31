//
// scan-dispatch.c
//
// Dispatching 802.15.4 scan results to interested parties.
//
// April 15, 2015
//
// Copyright 2015 Silicon Laboratories, Inc.                               *80*
//

#include "app/framework/include/af.h"

#include "scan-dispatch.h"

// -----------------------------------------------------------------------------
// Global variables

EmberEventControl emberAfPluginScanDispatchScanEventControl;

// -----------------------------------------------------------------------------
// Internal implementation elements

static uint8_t head = 0;
static uint8_t tail = 0;
static uint8_t count = 0;
static EmberAfPluginScanDispatchScanData handlerQueue[EMBER_AF_PLUGIN_SCAN_DISPATCH_SCAN_QUEUE_SIZE];
#define handlerQueueIsEmpty() (!count)
#define handlerQueueIsFull()  (count == EMBER_AF_PLUGIN_SCAN_DISPATCH_SCAN_QUEUE_SIZE)

// We save 12 bytes of text by having this as a function.
static uint8_t handlerQueueNextIndex(uint8_t i)
{
  return (++i % EMBER_AF_PLUGIN_SCAN_DISPATCH_SCAN_QUEUE_SIZE);
}

static void maybeCallNextHandler(uint8_t statusOrRssi,
                                 uint8_t channelOrLqi,
                                 EmberNetworkScanType scanType,
                                 bool isComplete,
                                 bool isFailure,
                                 EmberZigbeeNetwork *network)
{
  if (!handlerQueueIsEmpty()) {
    EmberAfPluginScanDispatchScanResults results = {
      .status = statusOrRssi,
      .rssi = statusOrRssi,
      .channel = channelOrLqi,
      .lqi = channelOrLqi,
      .network = network,
      .mask = (scanType
               | (isComplete
                  ? EM_AF_PLUGIN_SCAN_DISPATCH_SCAN_RESULTS_MASK_COMPLETE
                  : 0)
               | (isFailure
                  ? EM_AF_PLUGIN_SCAN_DISPATCH_SCAN_RESULTS_MASK_FAILURE
                  : 0)),
    };
    handlerQueue[head].handler(&results);
  }
}

// -----------------------------------------------------------------------------
// Public API

EmberStatus emberAfPluginScanDispatchScheduleScan(EmberAfPluginScanDispatchScanData *data)
{
  EmberStatus status = EMBER_ERR_FATAL;

  if (!handlerQueueIsFull()) {
    handlerQueue[tail] = *data;
    tail = handlerQueueNextIndex(tail);
    count++;
    status = EMBER_SUCCESS;
    emberEventControlSetActive(emberAfPluginScanDispatchScanEventControl);
  }

  return status;
}

void emberAfPluginScanDispatchClear(void)
{
  emberStopScan();
  head = tail = count = 0;
}

// -----------------------------------------------------------------------------
// Public callbacks

void emberAfPluginScanDispatchScanEventHandler(void)
{
  EmberStatus status;

  emberEventControlSetInactive(emberAfPluginScanDispatchScanEventControl);

  // If there is a handler in the queue, start a scan for it.
  // If we are already scanning, we should try again.
  // If there is an error, we abort the scan and tell the consumer.
  if (!handlerQueueIsEmpty()) {
    status = emberStartScan(handlerQueue[head].scanType,
                            handlerQueue[head].channelMask,
                            handlerQueue[head].duration);
    if (status != EMBER_MAC_SCANNING) {
      if (status != EMBER_SUCCESS) {
        maybeCallNextHandler(status,
                             0, // channel or LQI - meh
                             handlerQueue[head].scanType,
                             true,  // complete?
                             true,  // failure?
                             NULL); // network
        head = handlerQueueNextIndex(head);
        count--;
        emberEventControlSetActive(emberAfPluginScanDispatchScanEventControl);
      }
    }
  }
}

void emberAfEnergyScanResultCallback(uint8_t channel, int8_t rssi)
{
  maybeCallNextHandler((uint8_t)rssi,
                       channel,
                       EMBER_ENERGY_SCAN,
                       false,  // complete?
                       false,  // failure?
                       NULL);  // network
}

void emberAfNetworkFoundCallback(EmberZigbeeNetwork *networkFound,
                                 uint8_t lqi,
                                 int8_t rssi)
{
  maybeCallNextHandler((uint8_t)rssi,
                       lqi,
                       EMBER_ACTIVE_SCAN,
                       false, // complete?
                       false, // failure?
                       networkFound);
}

void emberAfScanCompleteCallback(uint8_t channel, EmberStatus status)
{
  maybeCallNextHandler(status,
                       channel,
                       handlerQueue[head].scanType,
                       true,  // complete?
                       false, // failure?
                       NULL); // network

  // The scan is done when the status is set to EMBER_SUCCESS.
  // See documentation for the emberScanCompleteHandler callback.
  if (status == EMBER_SUCCESS && !handlerQueueIsEmpty()) {
    head = handlerQueueNextIndex(head);
    count--;
    emberEventControlSetActive(emberAfPluginScanDispatchScanEventControl);
  }
}
