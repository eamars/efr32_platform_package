/**
 * @file child.h
 * See @ref child for documentation.
 *
 * <!--Copyright 2004-2007 by Ember Corporation. All rights reserved.    *80*-->
 */

#ifndef SILABS_CHILD_H
#define SILABS_CHILD_H

/**
 * @addtogroup child
 * @brief EmberZNet API relating to end device children.
 *
 * See child.h for source code.
 * @{
 */

/** @brief Converts a child index to a node ID.
 *
 * @param childIndex  The index.
 *
 * @return The node ID of the child or ::EMBER_NULL_NODE_ID if there isn't a
 *         child at the childIndex specified.
 */
EmberNodeId emberChildId(uint8_t childIndex);

/** @brief This function returns radio power value of the child from the given
 * childIndex.
 * @param childIndex  The index.
 *
 * @return The power of the child or max radio power which is the power value
 *         provided by user while forming/joining a network if there isn't a
 *         child at the childIndex specified.
 */
int8_t emberChildPower(uint8_t childIndex);

/** @brief This function sets the radio power value for a given child index.
 *
 * @param childIndex The index
 * @param newPower The new power value
 */
void emberSetChildPower(uint8_t childIndex, int8_t newPower);

/** @brief Converts a node ID to a child index.
 *
 * @param childId  The node ID of the child.
 *
 * @return The child index or 0xFF if the node ID does not belong to a child.
 */
uint8_t emberChildIndex(EmberNodeId childId);

/** @brief If there is a child at 'index' this copies its EUI64 and
 * node type into the return variables and returns ::EMBER_SUCCESS.  If there
 * is no child at 'index' it returns ::EMBER_NOT_JOINED.  Possible child
 * indexes run from zero to  ::emberMaxChildCount() - 1.
 *
 * @param index            The index of the child of interest.
 *
 * @param childData        The child's data is copied into here.
 *
 * @return Returns ::EMBER_SUCCESS if a child is found at that index,
 *         ::EMBER_NOT_JOINED if not.
 */
EmberStatus emberGetChildData(uint8_t index,
                              EmberChildData *childData);

/** @brief This is called by the stack when a child joins or leaves.
 * 'Joining' is true if the child is joining and false if leaving.
 *
 * The index is the same as the value that should be passed to
 * ::emberGetChildData() to get this child's data.
 * Note that if the child is leaving ::emberGetChildData()
 * will now return ::EMBER_NOT_JOINED if called with this index.
 * If the application includes ::emberChildJoinHandler(), it must
 * define EMBER_APPLICATION_HAS_CHILD_JOIN_HANDLER in its
 * CONFIGURATION_HEADER
 *
 * @param index    The index of the child of interest.
 *
 * @param joining  True if the child is joining, false if the child is leaving.
 */
void emberChildJoinHandler(uint8_t index, bool joining);

/** @brief Function to request any pending data from the parent node.
 * This function allows an end device to query its parent for any pending data.
 *
 * End devices must call this function periodically to maintain contact with
 * their parent.  The parent will remove a mobile end device from its child
 * table if it has not received a poll from it within the last
 * ::EMBER_MOBILE_NODE_POLL_TIMEOUT quarter seconds.  It will remove a
 * sleepy or non-sleepy end device if it has not received a poll
 * from it within the last
 * ::EMBER_END_DEVICE_POLL_TIMEOUT << ::EMBER_END_DEVICE_POLL_TIMEOUT_SHIFT
 * seconds.
 *
 * If the end device has lost contact with its parent, ::emberPollForData()
 * calls ::emberRejoinNetwork(true) and returns ::EMBER_ERR_FATAL.
 *
 * The default values for the timeouts are set in
 * config/ember-configuration-defaults.h, and can be overridden in the
 * application's configuration header.
 *
 * @return An EmberStatus value:
 * - ::EMBER_SUCCESS - The poll message has been submitted for transmission.
 * - ::EMBER_INVALID_CALL - The node is not an end device.
 * - ::EMBER_NOT_JOINED - The node is not part of a network.
 * - ::EMBER_MAC_TRANSMIT_QUEUE_FULL - The MAC layer transmit queue is full.
 * - ::EMBER_NO_BUFFERS - There were no buffers available to create the
 *     data poll message.
 * - ::EMBER_ERR_FATAL - Too much time has elapsed since the
 *     last successful poll. A rejoin attempt has been initiated.
 *     This error is not "fatal". The command can be retried until successful.
 */
EmberStatus emberPollForData(void);

/** @ brief This is called by the stack when a data poll to the parent is
 * complete.
 *
 * If the application includes ::emberPollCompleteHandler(),
 * it must define EMBER_APPLICATION_HAS_POLL_COMPLETE_HANDLER within
 * its CONFIGURATION_HEADER
 *
 * @param status  An \c EmberStatus value:
 * - ::EMBER_SUCCESS - Data was received in response to the poll.
 * - ::EMBER_MAC_NO_DATA - No data was pending.
 * - ::EMBER_DELIVERY_FAILED - The poll message could not be sent.
 * - ::EMBER_MAC_NO_ACK_RECEIVED - The poll message was sent but
 *     not acknowledged by the parent.
 */
void emberPollCompleteHandler(EmberStatus status);

/** @brief Sets a flag to indicate that there is a message pending for a child.
 * The next time that the child polls, it will be informed that it has
 * a pending message. The message is sent from emberPollHandler, which is called
 * when the child requests the data.
 *
 * @param childId  The ID of the child that just polled for data.
 *
 * @return An ::EmberStatus value.
 * - ::EMBER_SUCCESS - The next time that the child polls, it will be informed
 *     that it has pending data.
 * - ::EMBER_NOT_JOINED - The child identified by childId is not our child
 *    (it is not in the PAN).
 */
EmberStatus emberSetMessageFlag(EmberNodeId childId);

/** @brief Clears a flag to indicate that there are no more messages for a
 * child. The next time the child polls, it will be informed that it does not
 * have any pending messages.
 *
 * @param childId  The ID of the child that no longer has pending messages.
 *
 * @return An ::EmberStatus value.
 * - ::EMBER_SUCCESS - The next time that the child polls, it will be informed
 *     that it does not have any pending messages.
 * - ::EMBER_NOT_JOINED - The child identified by childId is not our child
 *     (it is not in the PAN).
 */
EmberStatus emberClearMessageFlag(EmberNodeId childId);

/** @brief A callback that allows the application to send a message in
 * response to a poll from a child.
 *
 * This function is called when a child polls,
 * provided that the pending message flag is set for that child
 * (see ::emberSetMessageFlag(). The message should be sent to the child
 * using ::emberSendUnicast() with the ::EMBER_APS_OPTION_POLL_RESPONSE option.
 *
 * If the application includes ::emberPollHanlder(), it must
 * define EMBER_APPLICATION_HAS_POLL_HANDLER in its CONFIGURATION_HEADER.
 *
 * @param childId           The ID of the child that is requesting data.
 *
 * @param transmitExpected  true if the child is expecting an application-
 *     supplied data message.  false otherwise.
 *
 */
void emberPollHandler(EmberNodeId childId, bool transmitExpected);

#ifdef DOXYGEN_SHOULD_SKIP_THIS
/** @brief Returns the number of children the node currently has.
 *
 * @return number of children
 */
uint8_t emberChildCount(void);

/** @brief Returns the number of router children that the node
 * currently has.
 *
 * @return number of router children
 */
uint8_t emberRouterChildCount(void);

/** @brief Returns the maximum number of children for this node.
 * The return value is undefined for nodes that are not joined to a network.
 *
 * @return maximum number of children
 */
uint8_t emberMaxChildCount(void);

/** @brief Returns the maximum number of router children for this node.
 * The return value is undefined for nodes that are not joined to a network.
 *
 * @return maximum number of router children
 */
uint8_t emberMaxRouterChildCount(void);

/** @brief Returns the parent's node ID.  The return value is undefined
 * for nodes without parents (coordinators and nodes that are not joined
 * to a network).
 *
 * @return parent's node ID
 */
EmberNodeId emberGetParentNodeId(void);

/** @brief Returns the parent's EUI64.  The return value is undefined
 * for nodes without parents (coordinators and nodes that are not joined
 * to a network).
 *
 * @return parent's EUI64
 */
EmberEUI64 emberGetParentEui64(void);

#else   // Doxygen ignores the following

extern uint8_t emMaxEndDeviceChildren;    // maximum for this node
extern uint8_t emEndDeviceChildCount;     // how many we have
extern uint8_t emDefaultTimeoutValue;     // Default Timeout Enumeration.

// The '+ 0' prevents anyone from accidentally assigning to these.
#define emberChildCount()          (emEndDeviceChildCount + 0)
#define emberRouterChildCount()    0
#define emberMaxChildCount()       (emMaxEndDeviceChildren + 0)
#define emberMaxRouterChildCount() 0

// Implemented in ember-stack-common.c
uint32_t emberGetParentIncomingNwkFrameCounter(void);
EmberStatus emberSetParentIncomingNwkFrameCounter(uint32_t value);
EmberNodeId emberGetParentNodeId(void);
uint8_t* emberGetParentEui64(void);

#endif

/**@name Power Management
 */
//@{

/** @brief Defines tasks that prevent the stack from sleeping.
 */
enum {
  /** There are messages waiting for transmission. */
  EMBER_OUTGOING_MESSAGES = 0x0001,
  /** One or more incoming messages are being processed. */
  EMBER_INCOMING_MESSAGES = 0x0002,
  /** The radio is currently powered on.  On sleepy devices the radio is
   *  turned off when not in use.  On non-sleepy devices (::EMBER_COORDINATOR,
   *  ::EMBER_ROUTER, or ::EMBER_END_DEVICE) the radio is always on.
   */
  EMBER_RADIO_IS_ON = 0x0004,
  /** The transport layer has messages awaiting an ACK. */
  EMBER_TRANSPORT_ACTIVE = 0x0008,
  /** The ZigBee APS layer has messages awaiting an ACK. */
  EMBER_APS_LAYER_ACTIVE = 0x0010,
  /** The node is currently trying to associate with a ZigBee PRO network. */
  EMBER_ASSOCIATING = 0x0020,
  /** The node is currently touch linking. */
  EMBER_ZLL_TOUCH_LINKING = 0x0040,
  /** Network Timeout Request Event */
  EMBER_NETWORK_TIMEOUT_REQUEST = 0x0200,
  /** Sending Orphan Notification Event */
  EMBER_SEND_ORPHAN_NOTIFICATION = 0x0400,
};

/** @brief A mask of the tasks that prevent a device from sleeping.
 */
#define EMBER_HIGH_PRIORITY_TASKS \
  (EMBER_OUTGOING_MESSAGES | EMBER_INCOMING_MESSAGES | EMBER_RADIO_IS_ON)

/** @brief Returns a bitmask indicating the stack's current tasks.
 *
 *  The mask ::EMBER_HIGH_PRIORITY_TASKS defines which tasks are high
 *  priority.  Devices should not sleep if any high priority tasks are active.
 *  Active tasks that are not high priority are waiting for
 *  messages to arrive from other devices.  If there are active tasks,
 *  but no high priority ones, the device may sleep but should periodically
 *  wake up and call ::emberPollForData() in order to receive messages.  Parents
 *  will hold messages for ::EMBER_INDIRECT_TRANSMISSION_TIMEOUT milliseconds
 *  before discarding them.
 *
 * @return A bitmask of the stack's active tasks.
 */
uint16_t emberCurrentStackTasks(void);

/** @brief Indicates whether the stack is currently in a state where
 * there are no high priority tasks and may sleep.
 *
 * There may be tasks expecting incoming messages, in which case the device should
 * periodically wake up and call ::emberPollForData() in order to receive messages.
 * This function can only be called when the node type is
 * ::EMBER_SLEEPY_END_DEVICE or ::EMBER_MOBILE_END_DEVICE.
 *
 * @return true if the application may sleep but the stack may be expecting
 * incoming messages.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
bool emberOkToNap(void);
#else
#define emberOkToNap() \
  (!(emberCurrentStackTasks() & EMBER_HIGH_PRIORITY_TASKS))
#endif

/* check if parent token has been written
 * implemenented in association.c
 */
bool emberParentTokenSet(void);

/** @brief Indicates whether the stack currently has any tasks pending.
 *
 * If there are no pending tasks then ::emberTick() does not need to be called until
 * the next time a stack API function is called. This function can only be
 * called when the node type is ::EMBER_SLEEPY_END_DEVICE or
 * ::EMBER_MOBILE_END_DEVICE.
 *
 * @return true if the application may sleep for as long as it wishes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
bool emberOkToHibernate(void);
#else
#define emberOkToHibernate() (!emberCurrentStackTasks())
#endif

/* check the conditions for going to em4 mode */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
bool emberOkToGoToLowPowerMode(void);
#else
#define  emberOkToGoToLowPowerMode() (!emberCurrentStackTasks() && emberParentTokenSet())
#endif

/** @brief Indicates whether the stack is currently in a state that does not
 * require the application to periodically poll.
 *
 * @return true if the application may stop polling periodically.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
bool emberOkToLongPoll(void);
#else
#define emberOkToLongPoll() (!emberPendingAckedMessages())
#endif

/** @brief Immediately turns the radio power completely off.
 *
 * After calling this function, you must not call any other stack function
 * except emberStackPowerUp(). This is because all other stack
 * functions require that the radio is powered on for their
 * proper operation.
 */
void emberStackPowerDown(void);

/** @brief Initializes the radio.  Typically called coming out of deep sleep.
 *
 * For non-sleepy devices, also turns the radio on and leaves it in rx mode.
 */
void emberStackPowerUp(void);

// @} END Power Management

// @} END addtogroup

/** @brief sends a link power delta request to the parent
 */
EmberStatus emberSendLinkPowerDeltaRequest (void);

/**
 * <!-- HIDDEN
 * @page 2p5_to_3p0
 * <hr>
 * The file child.h is new and is described in @ref child.
 *
 * - <b>Items moved from ember.h</b>
 *   - emberChildCount()
 *   - emberChildId()
 *   - emberChildIndex()
 *   - emberChildJoinHandler()
 *   - emberClearMessageFlag()
 *   - emberCurrentStackTasks()
 *   - emberGetChildData()
 *   - emberGetParentEui64()
 *   - emberGetParentNodeId()
 *   - emberMaxChildCount()
 *   - emberMaxRouterChildCount()
 *   - emberOkToHibernate()
 *   - emberOkToNap()
 *   - emberPollCompleteHandler()
 *   - emberPollForData()
 *   - emberPollHandler()
 *   - emberRouterChildCount()
 *   - emberSetMessageFlag()
 *   - emberStackPowerDown()
 *   - emberStackPowerUp()
 *   .
 * .
 * HIDDEN -->
 */

#endif // SILABS_CHILD_H
