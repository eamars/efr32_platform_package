// *******************************************************************
// * load-control-event-table.h
// *
// * The Demand Response Load Control Event Table is responsible
// * for keeping track of all load control events scheduled
// * by the Energy Service Provider. This module provides
// * interfaces used to schedule and inform load shedding
// * devices of scheduled events.
// *
// * Any code that uses this event table is responsible for
// * providing four things:
// *   1. frequent calls to eventTableTick(), one per millisecond
// *      will do. These calls are used to drive the
// *      table's timing mechanism.
// *   2. A way to get the real time by implementing
// *      getCurrentTime(uint32_t *currentTime);
// *   3. An implementation of eventAction which
// *      will be called whenever event status changes
// *
// * The load control event table expects that currentTime, startTime
// * and randomization are provided in seconds. And that duration is
// * provided in minutes.
// *
// * The implementing code is responsible for over the
// * air communication based on event status changes
// * reported through the eventAction interface
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#ifndef SILABS_LOAD_CONTROL_EVENT_TABLE_H
#define SILABS_LOAD_CONTROL_EVENT_TABLE_H

// include global header for public LoadControlEvent struct
#include "../../include/af.h"

#define RANDOMIZE_START_TIME_FLAG     1
#define RANDOMIZE_DURATION_TIME_FLAG  2

#define CANCEL_WITH_RANDOMIZATION        1

// Table entry status
enum {
  ENTRY_VOID,
  ENTRY_SCHEDULED,
  ENTRY_STARTED,
  ENTRY_IS_SUPERSEDED_EVENT,
  ENTRY_IS_CANCELLED_EVENT
};

enum {
  EVENT_OPT_FLAG_OPT_IN                           = 0x01,
  EVENT_OPT_FLAG_PARTIAL                          = 0x02
};

// EVENT TABLE API

void afLoadControlEventTableInit(uint8_t endpoint);

/**
 * This function is used to schedule events in the
 * load control event table. The interface expects that
 * the user will populate a LoadControlEvent with all the
 * necessary fields and will pass a pointer to this event.
 * The passed event will be copied into the event table so it does
 * not need to survive the call to scheduleEvent.
 *
 * A call to this function always generates an event response OTA
 *
 */
void emAfScheduleLoadControlEvent(uint8_t endpoint,
                                  EmberAfLoadControlEvent *event);

/**
 * Tells the Event table when a tick has taken place. This
 * function should be called by the cluster that uses the
 * event table.
 **/
void emAfLoadControlEventTableTick(uint8_t endpoint);

/**
 * Used to cancel all events in the event table
 *
 * @return A bool value indicating that a response was
 * generated for this action.
 **/
bool emAfCancelAllLoadControlEvents(uint8_t endpoint,
                                    uint8_t cancelControl);

/**
 * Used to cancel an event in the event table
 *
 * A call to this function always generates an event response OTA.
 **/
void emAfCancelLoadControlEvent(uint8_t endpoint,
                                uint32_t eventId,
                                uint8_t cancelControl,
                                uint32_t effectiveTime);

/**
 * Used to schedule a call to cancel an event. Takes a
 * LoadControlEvent struct which is used to encapsulate
 * the necessary information for the scheduled cancel.
 *
 * The scheduleCancelEvent function only uses three fields
 * from the LoadControlEvent struct, eventId, startTime,
 * and eventControl, as follows:
 *
 * eventId: The eventId of the event to cancel.
 *
 * startTime: The starttime of the event should
 * be the same as the effective time of the
 * cancel and will be used to schedule the
 * call to cancel.
 *
 * eventControl: The eventControl of the event should
 * be the same as the cancelControl passed when sending the
 * initial cancel event call.
 *
 * NOTE: A call to cancelAllEvents() will wipe out
 * this scheduled cancel as well as all other events which
 * should be ok, since the event scheduled to cancel would
 * be affected as well by the cancel all call anyway.
 **/
void afScheduleCancelEvent(EmberAfLoadControlEvent *e);

/**
 * An interface for opting in and out of an event
 */
void emAfLoadControlEventOptInOrOut(uint8_t endpoint,
                                    uint32_t eventId,
                                    bool optIn);

// The module using this table is responsible for providing the following
// functions.
/**
 * Called by the event table when an events status has
 * changed. This handle should be used to inform the
 * ESP(s) and or react to scheduled events. The event
 * table will take care of clearing itself if the event
 * is completed.
 **/
void emberAfEventAction(EmberAfLoadControlEvent *event,
                        uint8_t eventStatus,
                        uint8_t sequenceNumber,
                        uint8_t esiIndex);

/** A routine that prints a message indicating
 *  an attempt to append a signature to the event status message
 *  has failed.
 **/
void emAfNoteSignatureFailure(void);

/** Prints the load control event table
**/
void emAfLoadControlEventTablePrint(uint8_t endpoint);

/**
 * Initialization function.
 */
void emAfLoadControlEventTableInit(uint8_t endpoint);

/*
 * Clear the event table.
 */
void emAfLoadControlEventTableClear(uint8_t endpoint);

#endif //__LOAD_CONTROL_EVENT_TABLE_H__
