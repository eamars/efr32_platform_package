// Copyright 2017 Silicon Laboratories, Inc.

#include "app/framework/include/af.h"
#include "app/framework/util/af-event.h"
#include "em4.h"

//these are implemented in /em4/plugin.properties
//these are declared in /idle-sleep/callbacks.info
void emberPluginEM4AboutToGoInEM4Callback(uint32_t sleepDurationMs)
{
  // this is called from idle-sleep-soc
  // a customer facing call back could be called here
  emberPluginEM4BeforeEM4Callback();//needs to be implemented by app
  // the following should be called after application code
  RTCCRamData input;
  input.outgoingNwkFrameCounter = emberGetSecurityFrameCounter();
  input.incomingParentNwkFrameCounter = emberGetParentIncomingNWKFrameCounter();
  input.outgoingLinkKeyFrameCounter = emberGetApsFrameCounter();
  input.incomingLinkKeyFrameCounter = emberGetIncomingTCLinkKeyFrameCounter();
  halBeforeEM4(sleepDurationMs, input);// it needs to set the timer on the hardware, and save the in/out NWK counter
}

void emberPluginEM4ComingBackFromEM4Callback(void)
{
  //this is called before application code
  RTCCRamData output = halAfterEM4();

  emberSetOutgoingNwkFrameCounter(output.outgoingNwkFrameCounter);
  EmberStatus status = emberSetParentAndIncomingNWKFrameCounter(output.incomingParentNwkFrameCounter);
  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("WARNING: undefined behaviour: Used EM4 sleep mode without/before saving the parent info to  the flash.");
  }
  emberSetOutgoingApsFrameCounter(output.outgoingLinkKeyFrameCounter);
  emberSetIncomingTCLinkKeyFrameCounter(output.incomingLinkKeyFrameCounter);
  // This is supposed to restore the outgoing network counter
  // and then also let the user know if he wants to restore something?
  // a customer facing call back could be called here
  emberPluginEM4AfterEM4Callback();//needs to be implemented by user app
}
