/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *  This file defines the OpenThread Thread API.
 */

#ifndef OPENTHREAD_THREAD_H_
#define OPENTHREAD_THREAD_H_

#include "openthread/types.h"
#include "openthread/link.h"
#include "openthread/message.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup thread  Thread
 *
 * @brief
 *   This module includes functions that control Thread-specific functions.
 *
 * @{
 *
 */

/**
 * @addtogroup config  Configuration
 *
 * @brief
 *   This module includes functions for configuration.
 *
 * @{
 *
 */

/**
 * This function starts Thread protocol operation.
 *
 * The interface must be up when calling this function.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnabled  TRUE if Thread is enabled, FALSE otherwise.
 *
 * @retval kThreadError_None          Successfully started Thread protocol operation.
 * @retval kThreadError_InvalidState  The network interface was not not up.
 *
 */
OTAPI ThreadError OTCALL otThreadSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function queries if the Thread stack is configured to automatically start on reinitialization.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   It is configured to automatically start.
 * @retval FALSE  It is not configured to automatically start.
 *
 */
OTAPI bool OTCALL otThreadGetAutoStart(otInstance *aInstance);

/**
 * This function configures the Thread stack to automatically start on reinitialization.
 * It has no effect on the current Thread state.
 *
 * @param[in] aInstance             A pointer to an OpenThread instance.
 * @param[in] aStartAutomatically   TRUE to automatically start; FALSE to not automatically start.
 *
 */
OTAPI ThreadError OTCALL otThreadSetAutoStart(otInstance *aInstance, bool aStartAutomatically);

/**
 * This function indicates whether a node is the only router on the network.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   It is the only router in the network.
 * @retval FALSE  It is a child or is not a single router in the network.
 *
 */
OTAPI bool OTCALL otThreadIsSingleton(otInstance *aInstance);

/**
 * This function starts a Thread Discovery scan.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aScanChannels     A bit vector indicating which channels to scan (e.g. OT_CHANNEL_11_MASK).
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aPanId            The PAN ID filter (set to Broadcast PAN to disable filter).
 * @param[in]  aCallback         A pointer to a function called on receiving an MLE Discovery Response or scan completes.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval kThreadError_None  Accepted the Thread Discovery request.
 * @retval kThreadError_Busy  Already performing an Thread Discovery.
 *
 */
OTAPI ThreadError OTCALL otThreadDiscover(otInstance *aInstance, uint32_t aScanChannels, uint16_t aScanDuration,
                                          uint16_t aPanid,
                                          otHandleActiveScanResult aCallback, void *aCallbackContext);

/**
 * This function determines if an MLE Thread Discovery is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
OTAPI bool OTCALL otThreadIsDiscoverInProgress(otInstance *aInstance);

/**
 * @defgroup config-general  General
 *
 * @brief
 *   This module includes functions that manage configuration parameters for the Thread Child, Router, and Leader roles.
 *
 * @{
 *
 */

/**
 * Get the maximum number of children currently allowed.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The maximum number of children currently allowed.
 *
 * @sa otThreadSetMaxAllowedChildren
 */
OTAPI uint8_t OTCALL otThreadGetMaxAllowedChildren(otInstance *aInstance);

/**
 * Set the maximum number of children currently allowed.
 *
 * This parameter can only be set when Thread protocol operation
 * has been stopped.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMaxChildren  The maximum allowed children.
 *
 * @retval  kThreadErrorNone           Successfully set the max.
 * @retval  kThreadError_InvalidArgs   If @p aMaxChildren is not in the range [1, OPENTHREAD_CONFIG_MAX_CHILDREN].
 * @retval  kThreadError_InvalidState  If Thread isn't stopped.
 *
 * @sa otThreadGetMaxAllowedChildren, otThreadStop
 */
OTAPI ThreadError OTCALL otThreadSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren);

/**
 * Get the Thread Child Timeout used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Child Timeout value.
 *
 * @sa otThreadSetChildTimeout
 */
OTAPI uint32_t OTCALL otThreadGetChildTimeout(otInstance *aInstance);

/**
 * Set the Thread Child Timeout used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otThreadSetChildTimeout
 */
OTAPI void OTCALL otThreadSetChildTimeout(otInstance *aInstance, uint32_t aTimeout);

/**
 * Get the IEEE 802.15.4 Extended PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otThreadSetExtendedPanId
 */
OTAPI const uint8_t *OTCALL otThreadGetExtendedPanId(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 Extended PAN ID.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aExtendedPanId  A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otThreadGetExtendedPanId
 */
OTAPI void OTCALL otThreadSetExtendedPanId(otInstance *aInstance, const uint8_t *aExtendedPanId);

/**
 * This function returns a pointer to the Leader's RLOC.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderRloc  A pointer to where the Leader's RLOC will be written.
 *
 * @retval kThreadError_None         The Leader's RLOC was successfully written to @p aLeaderRloc.
 * @retval kThreadError_InvalidArgs  @p aLeaderRloc was NULL.
 * @retval kThreadError_Detached     Not currently attached to a Thread Partition.
 *
 */
OTAPI ThreadError OTCALL otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aLeaderRloc);

/**
 * Get the MLE Link Mode configuration.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The MLE Link Mode configuration.
 *
 * @sa otThreadSetLinkMode
 */
OTAPI otLinkModeConfig OTCALL otThreadGetLinkMode(otInstance *aInstance);

/**
 * Set the MLE Link Mode configuration.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the Link Mode configuration.
 *
 * @retval kThreadErrorNone  Successfully set the MLE Link Mode configuration.
 *
 * @sa otThreadGetLinkMode
 */
OTAPI ThreadError OTCALL otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig);

/**
 * Get the thrMasterKey.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[out]  aKeyLength  A pointer to an unsigned 8-bit value that the function will set to the number of bytes that
 *                          represent the thrMasterKey. Caller may set to NULL.
 *
 * @returns A pointer to a buffer containing the thrMasterKey.
 *
 * @sa otThreadSetMasterKey
 */
OTAPI const uint8_t *OTCALL otThreadGetMasterKey(otInstance *aInstance, uint8_t *aKeyLength);

/**
 * Set the thrMasterKey.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aKey        A pointer to a buffer containing the thrMasterKey.
 * @param[in]  aKeyLength  Number of bytes representing the thrMasterKey stored at aKey. Valid range is [0, 16].
 *
 * @retval kThreadErrorNone         Successfully set the thrMasterKey.
 * @retval kThreadErrorInvalidArgs  If aKeyLength is larger than 16.
 *
 * @sa otThreadGetMasterKey
 */
OTAPI ThreadError OTCALL otThreadSetMasterKey(otInstance *aInstance, const uint8_t *aKey, uint8_t aKeyLength);

/**
 * This function returns a pointer to the Mesh Local EID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local EID.
 *
 */
OTAPI const otIp6Address *OTCALL otThreadGetMeshLocalEid(otInstance *aInstance);

/**
 * This function returns a pointer to the Mesh Local Prefix.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local Prefix.
 *
 */
OTAPI const uint8_t *OTCALL otThreadGetMeshLocalPrefix(otInstance *aInstance);

/**
 * This function sets the Mesh Local Prefix.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aMeshLocalPrefix  A pointer to the Mesh Local Prefix.
 *
 * @retval kThreadError_None  Successfully set the Mesh Local Prefix.
 *
 */
OTAPI ThreadError OTCALL otThreadSetMeshLocalPrefix(otInstance *aInstance, const uint8_t *aMeshLocalPrefix);

/**
 * Get the Thread Network Name.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Network Name.
 *
 * @sa otThreadSetNetworkName
 */
OTAPI const char *OTCALL otThreadGetNetworkName(otInstance *aInstance);

/**
 * Set the Thread Network Name.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aNetworkName  A pointer to the Thread Network Name.
 *
 * @retval kThreadErrorNone  Successfully set the Thread Network Name.
 *
 * @sa otThreadGetNetworkName
 */
OTAPI ThreadError OTCALL otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName);

/**
 * This function indicates whether or not the Router Role is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   If the Router Role is enabled.
 * @retval FALSE  If the Router Role is not enabled.
 *
 */
OTAPI bool OTCALL otThreadIsRouterRoleEnabled(otInstance *aInstance);

/**
 * This function sets whether or not the Router Role is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aEnabled  TRUE if the Router Role is enabled, FALSE otherwise.
 *
 */
OTAPI void OTCALL otThreadSetRouterRoleEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Set the preferred Router Id.
 *
 * Upon becoming a router/leader the node attempts to use this Router Id. If the
 * preferred Router Id is not set or if it can not be used, a randomly generated
 * router id is picked. This property can be set only when the device role is
 * either detached or disabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aRouterId    The preferred Router Id.
 *
 * @retval kThreadError_None         Successfully set the preferred Router Id.
 * @retval kThreadError_InvalidState Could not set (role is not detached or disabled)
 *
 */
OTAPI ThreadError OTCALL otThreadSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId);

/**
 * @}
 */

/**
 * @defgroup config-router  Router/Leader
 *
 * @brief
 *   This module includes functions that manage configuration parameters for the Thread Router and Leader roles.
 *
 * @{
 *
 */

/**
 * Get the Thread Leader Weight used when operating in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Leader Weight value.
 *
 * @sa otThreadSetLeaderWeight
 */
OTAPI uint8_t OTCALL otThreadGetLocalLeaderWeight(otInstance *aInstance);

/**
 * Set the Thread Leader Weight used when operating in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aWeight   The Thread Leader Weight value..
 *
 * @sa otThreadGetLeaderWeight
 */
OTAPI void OTCALL otThreadSetLocalLeaderWeight(otInstance *aInstance, uint8_t aWeight);

/**
 * Get the Thread Leader Partition Id used when operating in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Leader Partition Id value.
 *
 */
OTAPI uint32_t OTCALL otThreadGetLocalLeaderPartitionId(otInstance *aInstance);

/**
 * Set the Thread Leader Partition Id used when operating in the Leader role.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPartitionId  The Thread Leader Partition Id value.
 *
 */
OTAPI void OTCALL otThreadSetLocalLeaderPartitionId(otInstance *aInstance, uint32_t aPartitionId);

/**
 * Get the Joiner UDP Port.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The Joiner UDP Port number.
 *
 * @sa otThreadSetJoinerUdpPort
 */
OTAPI uint16_t OTCALL otThreadGetJoinerUdpPort(otInstance *aInstance);

/**
 * Set the Joiner UDP Port
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aJoinerUdpPort  The Joiner UDP Port number.
 *
 * @retval  kThreadErrorNone   Successfully set the Joiner UDP Port.
 *
 * @sa otThreadGetJoinerUdpPort
 */
OTAPI ThreadError OTCALL otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort);

/**
 * @}
 */

/**
 * @defgroup config-test  Test
 *
 * @brief
 *   This module includes functions that manage configuration parameters required for Thread Certification testing.
 *
 * @{
 *
 */

/**
 * Get the CONTEXT_ID_REUSE_DELAY parameter used in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otThreadSetContextIdReuseDelay
 */
OTAPI uint32_t OTCALL otThreadGetContextIdReuseDelay(otInstance *aInstance);

/**
 * Set the CONTEXT_ID_REUSE_DELAY parameter used in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDelay    The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otThreadGetContextIdReuseDelay
 */
OTAPI void OTCALL otThreadSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay);

/**
 * Get the thrKeySequenceCounter.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySequenceCounter value.
 *
 * @sa otThreadSetKeySequenceCounter
 */
OTAPI uint32_t OTCALL otThreadGetKeySequenceCounter(otInstance *aInstance);

/**
 * Set the thrKeySequenceCounter.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySequenceCounter  The thrKeySequenceCounter value.
 *
 * @sa otThreadGetKeySequenceCounter
 */
OTAPI void OTCALL otThreadSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter);

/**
 * Get the thrKeySwitchGuardTime
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otThreadSetKeySwitchGuardTime
 */
OTAPI uint32_t OTCALL otThreadGetKeySwitchGuardTime(otInstance *aInstance);

/**
 * Set the thrKeySwitchGuardTime
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySwitchGuardTime  The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otThreadGetKeySwitchGuardTime
 */
OTAPI void OTCALL otThreadSetKeySwitchGuardTime(otInstance *aInstance, uint32_t aKeySwitchGuardTime);


/**
 * Get the NETWORK_ID_TIMEOUT parameter used in the Router role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The NETWORK_ID_TIMEOUT value.
 *
 * @sa otThreadSetNetworkIdTimeout
 */
OTAPI uint8_t OTCALL otThreadGetNetworkIdTimeout(otInstance *aInstance);

/**
 * Set the NETWORK_ID_TIMEOUT parameter used in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
 *
 * @sa otThreadGetNetworkIdTimeout
 */
OTAPI void OTCALL otThreadSetNetworkIdTimeout(otInstance *aInstance, uint8_t aTimeout);

/**
 * Get the ROUTER_UPGRADE_THRESHOLD parameter used in the REED role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The ROUTER_UPGRADE_THRESHOLD value.
 *
 * @sa otThreadSetRouterUpgradeThreshold
 */
OTAPI uint8_t OTCALL otThreadGetRouterUpgradeThreshold(otInstance *aInstance);

/**
 * Set the ROUTER_UPGRADE_THRESHOLD parameter used in the Leader role.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aThreshold  The ROUTER_UPGRADE_THRESHOLD value.
 *
 * @sa otThreadGetRouterUpgradeThreshold
 */
OTAPI void OTCALL otThreadSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold);

/**
 * Release a Router ID that has been allocated by the device in the Leader role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aRouterId  The Router ID to release. Valid range is [0, 62].
 *
 * @retval kThreadErrorNone  Successfully released the Router ID specified by aRouterId.
 */
OTAPI ThreadError OTCALL otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId);

/**
 * Detach from the Thread network.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone          Successfully detached from the Thread network.
 * @retval kThreadErrorInvalidState  Thread is disabled.
 */
OTAPI ThreadError OTCALL otThreadBecomeDetached(otInstance *aInstance);

/**
 * Attempt to reattach as a child.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aFilter   Identifies whether to join any, same, or better partition.
 *
 * @retval kThreadErrorNone          Successfully begin attempt to become a child.
 * @retval kThreadErrorInvalidState  Thread is disabled.
 */
OTAPI ThreadError OTCALL otThreadBecomeChild(otInstance *aInstance, otMleAttachFilter aFilter);

/**
 * Attempt to become a router.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone         Successfully begin attempt to become a router.
 * @retval kThreadErrorInvalidState Thread is disabled.
 */
OTAPI ThreadError OTCALL otThreadBecomeRouter(otInstance *aInstance);

/**
 * Become a leader and start a new partition.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone          Successfully became a leader and started a new partition.
 * @retval kThreadErrorInvalidState  Thread is disabled.
 */
OTAPI ThreadError OTCALL otThreadBecomeLeader(otInstance *aInstance);

/**
 * Get the ROUTER_DOWNGRADE_THRESHOLD parameter used in the Router role.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
 *
 * @sa otThreadSetRouterDowngradeThreshold
 */
OTAPI uint8_t OTCALL otThreadGetRouterDowngradeThreshold(otInstance *aInstance);

/**
 * Set the ROUTER_DOWNGRADE_THRESHOLD parameter used in the Leader role.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aThreshold  The ROUTER_DOWNGRADE_THRESHOLD value.
 *
 * @sa otThreadGetRouterDowngradeThreshold
 */
OTAPI void OTCALL otThreadSetRouterDowngradeThreshold(otInstance *aInstance, uint8_t aThreshold);

/**
 * Get the ROUTER_SELECTION_JITTER parameter used in the REED/Router role.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 *
 * @returns The ROUTER_SELECTION_JITTER value.
 *
 * @sa otThreadSetRouterSelectionJitter
 */
OTAPI uint8_t OTCALL otThreadGetRouterSelectionJitter(otInstance *aInstance);

/**
 * Set the ROUTER_SELECTION_JITTER parameter used in the REED/Router role.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aRouterJitter  The ROUTER_SELECTION_JITTER value.
 *
 * @sa otThreadGetRouterSelectionJitter
 */
OTAPI void OTCALL otThreadSetRouterSelectionJitter(otInstance *aInstance, uint8_t aRouterJitter);

/**
 * @}
 *
 */

/**
 * @}
 *
 */

/**
 * @addtogroup diags  Diagnostics
 *
 * @brief
 *   This module includes functions that expose internal state.
 *
 * @{
 *
 */

/**
 * The function retains diagnostic information for an attached Child by its Child ID or RLOC16.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[in]   aChildId    The Child ID or RLOC16 for the attached child.
 * @param[out]  aChildInfo  A pointer to where the child information is placed.
 *
 * @retavl kThreadError_None         @p aChildInfo was successfully updated with the info for the given ID.
 * @retval kThreadError_NotFound     No valid child with this Child ID.
 * @retavl kThreadError_InvalidArgs  If @p aChildInfo is NULL.
 *
 */
OTAPI ThreadError OTCALL otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo);

/**
 * The function retains diagnostic information for an attached Child by the internal table index.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[in]   aChildIndex  The table index.
 * @param[out]  aChildInfo   A pointer to where the child information is placed.
 *
 * @retavl kThreadError_None            @p aChildInfo was successfully updated with the info for the given index.
 * @retval kThreadError_NotFound        No valid child at this index.
 * @retavl kThreadError_InvalidArgs     Either @p aChildInfo is NULL, or @p aChildIndex is out of range (higher
 *                                      than max table index).
 *
 * @sa otGetMaxAllowedChildren
 *
 */
OTAPI ThreadError OTCALL otThreadGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex,
                                                     otChildInfo *aChildInfo);

/**
 * This function gets the next neighbor information. It is used to go through the entries of
 * the neighbor table.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the iterator context. To get the first neighbor entry
                             it should be set to OT_NEIGHBOR_INFO_ITERATOR_INIT.
 * @param[out]    aInfo      A pointer to where the neighbor information will be placed.
 *
 * @retval kThreadError_None         Successfully found the next neighbor entry in table.
 * @retval kThreadError_NotFound     No subsequent neighbor entry exists in the table.
 * @retval kThreadError_InvalidArgs  @p aIterator or @p aInfo was NULL.
 *
 */
OTAPI ThreadError OTCALL otThreadGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator,
                                                     otNeighborInfo *aInfo);

/**
 * Get the device role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval ::kDeviceRoleDisabled  The Thread stack is disabled.
 * @retval ::kDeviceRoleDetached  The device is not currently participating in a Thread network/partition.
 * @retval ::kDeviceRoleChild     The device is currently operating as a Thread Child.
 * @retval ::kDeviceRoleRouter    The device is currently operating as a Thread Router.
 * @retval ::kDeviceRoleLeader    The device is currently operating as a Thread Leader.
 */
OTAPI otDeviceRole OTCALL otThreadGetDeviceRole(otInstance *aInstance);

/**
 * This function gets an EID cache entry.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[in]   aIndex    An index into the EID cache table.
 * @param[out]  aEntry    A pointer to where the EID information is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the EID cache entry.
 * @retval kThreadError_InvalidArgs  @p aIndex was out of bounds or @p aEntry was NULL.
 *
 */
OTAPI ThreadError OTCALL otThreadGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry);

/**
 * This function get the Thread Leader Data.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderData  A pointer to where the leader data is placed.
 *
 * @retval kThreadError_None         Successfully retrieved the leader data.
 * @retval kThreadError_Detached     Not currently attached.
 * @retval kThreadError_InvalidArgs  @p aLeaderData is NULL.
 *
 */
OTAPI ThreadError OTCALL otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData);

/**
 * Get the Leader's Router ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Router ID.
 */
OTAPI uint8_t OTCALL otThreadGetLeaderRouterId(otInstance *aInstance);

/**
 * Get the Leader's Weight.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Weight.
 */
OTAPI uint8_t OTCALL otThreadGetLeaderWeight(otInstance *aInstance);

/**
 * Get the Partition ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Partition ID.
 */
OTAPI uint32_t OTCALL otThreadGetPartitionId(otInstance *aInstance);

/**
 * Get the RLOC16.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The RLOC16.
 */
OTAPI uint16_t OTCALL otThreadGetRloc16(otInstance *aInstance);

/**
 * Get the current Router ID Sequence.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Router ID Sequence.
 */
OTAPI uint8_t OTCALL otThreadGetRouterIdSequence(otInstance *aInstance);

/**
 * The function retains diagnostic information for a given Thread Router.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
 * @param[out]  aRouterInfo  A pointer to where the router information is placed.
 *
 */
OTAPI ThreadError OTCALL otThreadGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo);

/**
 * The function retrieves diagnostic information for a Thread Router as parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aParentInfo  A pointer to where the parent router information is placed.
 *
 */
OTAPI ThreadError OTCALL otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo);

/**
 * The function retrieves the average RSSI for the Thread Parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aParentRssi  A pointer to where the parent rssi should be placed.
 *
 */
OTAPI ThreadError OTCALL otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi);

/**
 * The function retrieves the RSSI of the last packet from the Thread Parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLastRssi    A pointer to where the last rssi should be placed.
 *
 * @retval kThreadError_None         Successfully retrieved the RSSI data.
 * @retval kThreadError_Failed       Unable to get RSSI data.
 * @retval kThreadError_InvalidArgs  @p aLastRssi is NULL.
 */
OTAPI ThreadError OTCALL otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi);

/**
 * This function pointer is called when Network Diagnostic Get response is received.
 *
 * @param[in]  aMessage      A pointer to the message buffer containing the received Network Diagnostic
 *                           Get response payload.
 * @param[in]  aMessageInfo  A pointer to the message info for @p aMessage.
 * @param[in]  aContext      A pointer to application-specific context.
 *
 */
typedef void (*otReceiveDiagnosticGetCallback)(otMessage *aMessage, const otMessageInfo *aMessageInfo,
                                               void *aContext);

/**
 * This function registers a callback to provide received raw Network Diagnostic Get response payload.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when Network Diagnostic Get response
 *                               is received or NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otThreadSetReceiveDiagnosticGetCallback(otInstance *aInstance, otReceiveDiagnosticGetCallback aCallback,
                                             void *aCallbackContext);

/**
 * Send a Network Diagnostic Get request.
 *
 * @param[in]  aDestination   A pointer to destination address.
 * @param[in]  aTlvTypes      An array of Network Diagnostic TLV types.
 * @param[in]  aCount         Number of types in aTlvTypes
 */
OTAPI ThreadError OTCALL otThreadSendDiagnosticGet(otInstance *aInstance, const otIp6Address *aDestination,
                                                   const uint8_t aTlvTypes[], uint8_t aCount);

/**
 * Send a Network Diagnostic Reset request.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aDestination   A pointer to destination address.
 * @param[in]  aTlvTypes      An array of Network Diagnostic TLV types. Currently only Type 9 is allowed.
 * @param[in]  aCount         Number of types in aTlvTypes
 */
OTAPI ThreadError OTCALL otThreadSendDiagnosticReset(otInstance *aInstance, const otIp6Address *aDestination,
                                                     const uint8_t aTlvTypes[], uint8_t aCount);

/**
 * @}
 *
 */

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_THREAD_H_
