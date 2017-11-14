// *******************************************************************
// * reporting default configuration.c
// *
// *
// * Copyright 2017 Silicon Laboratories, Inc.                              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/common.h"
#include "app/framework/util/attribute-storage.h"
#include "reporting.h"

#define REPORT_FAILED 0xFF

#if !(defined GENERATED_REPORTING_CONFIG_DEFAULTS)
  #include "test-reporting-config-header.h"
#else
PGM EmberAfPluginReportingEntry generatedReportingConfigDefaults[] = GENERATED_REPORTING_CONFIG_DEFAULTS;
  #define GENERATED_TABLE_SIZE (sizeof(generatedReportingConfigDefaults) / sizeof(EmberAfPluginReportingEntry))
#endif

// Load Default reporting configuration from generated table
void emberAfPluginReportingLoadReportingConfigDefaults(void)
{
  int tableSize = GENERATED_TABLE_SIZE;
  if (tableSize == 0) {
    return;
  }
  emberAfCorePrintln("GENERATED_TABLE_SIZE = %d, EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE=%d", tableSize, EMBER_AF_PLUGIN_REPORTING_TABLE_SIZE);
  for (int i = 0; i < tableSize; i++) {
    uint8_t reportEntry = emAfPluginReportingConditionallyAddReportingEntry((EmberAfPluginReportingEntry *)&generatedReportingConfigDefaults[i]);
    emberAfCorePrintln("Dflt Server Rpt reportEntry %d Config[%d] epId=%X, cId=%02X, aId=%02X, mnI=%02X, mxI=%02X, ch=%04X - [%p]",
                       reportEntry, i,
                       generatedReportingConfigDefaults[i].endpoint,
                       generatedReportingConfigDefaults[i].clusterId,
                       generatedReportingConfigDefaults[i].attributeId,
                       generatedReportingConfigDefaults[i].data.reported.minInterval,
                       generatedReportingConfigDefaults[i].data.reported.maxInterval,
                       generatedReportingConfigDefaults[i].data.reported.reportableChange,
                       reportEntry == 0 ? "Skiped" : "Set");
    assert(reportEntry != REPORT_FAILED);
  }
}

// Get default reporting values
bool emberAfPluginReportingGetReportingConfigDefaults(EmberAfPluginReportingEntry *defaultConfiguration)
{
  if (NULL == defaultConfiguration) {
    return false;
  }
  int tableSize = GENERATED_TABLE_SIZE;
  for (int i = 0; i < tableSize; i++) {
    // All of the serach parameters match then copy the default config.
    if ((defaultConfiguration->endpoint == generatedReportingConfigDefaults[i].endpoint)
        && (defaultConfiguration->clusterId == generatedReportingConfigDefaults[i].clusterId)
        && (defaultConfiguration->attributeId == generatedReportingConfigDefaults[i].attributeId)
        && (defaultConfiguration->mask == generatedReportingConfigDefaults[i].mask)
        && (defaultConfiguration->manufacturerCode == generatedReportingConfigDefaults[i].manufacturerCode)) {
      defaultConfiguration->direction = EMBER_ZCL_REPORTING_DIRECTION_REPORTED;
      defaultConfiguration->data = generatedReportingConfigDefaults[i].data;
      return true;
    }
  }
  return false;
}
