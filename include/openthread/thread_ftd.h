/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *  This file defines the OpenThread Thread API (FTD only).
 */

#ifndef OPENTHREAD_THREAD_FTD_H_
#define OPENTHREAD_THREAD_FTD_H_

#include <openthread/link.h>
#include <openthread/message.h>
#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-router
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
 * @retval  OT_ERROR_NONE           Successfully set the max.
 * @retval  OT_ERROR_INVALID_ARGS   If @p aMaxChildren is not in the range [1, OPENTHREAD_CONFIG_MAX_CHILDREN].
 * @retval  OT_ERROR_INVALID_STATE  If Thread isn't stopped.
 *
 * @sa otThreadGetMaxAllowedChildren, otThreadStop
 */
OTAPI otError OTCALL otThreadSetMaxAllowedChildren(otInstance *aInstance, uint8_t aMaxChildren);

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
 * @retval OT_ERROR_NONE          Successfully set the preferred Router Id.
 * @retval OT_ERROR_INVALID_STATE Could not set (role is not detached or disabled)
 *
 */
OTAPI otError OTCALL otThreadSetPreferredRouterId(otInstance *aInstance, uint8_t aRouterId);

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
 * @param[in]  aWeight   The Thread Leader Weight value.
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
 * @retval  OT_ERROR_NONE  Successfully set the Joiner UDP Port.
 *
 * @sa otThreadGetJoinerUdpPort
 */
OTAPI otError OTCALL otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort);

/**
 * Set Steering data out of band
 *
 * Configuration option `OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB` should be set to enable setting of steering
 * data out of band. Otherwise calling this function does nothing and it returns `OT_ERROR_DISABLED_FEATURE`
 * error.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aExtAddress     Address used to update the steering data.
 *                             All zeros to clear the steering data (no steering data).
 *                             All 0xFFs to set steering data/bloom filter to accept/allow all.
 *                             A specific EUI64 which is then added to current steering data/bloom filter.
 *
 * @retval  OT_ERROR_NONE              Successfully set/updated the steering data.
 * @retval  OT_ERROR_DISABLED_FEATURE  Feature is disabled, not capable of setting steering data out of band.
 *
 */
otError otThreadSetSteeringData(otInstance *aInstance, otExtAddress *aExtAddress);

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
 * @retval OT_ERROR_NONE  Successfully released the Router ID specified by aRouterId.
 */
OTAPI otError OTCALL otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId);

/**
 * Attempt to become a router.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully begin attempt to become a router.
 * @retval OT_ERROR_INVALID_STATE  Thread is disabled.
 */
OTAPI otError OTCALL otThreadBecomeRouter(otInstance *aInstance);

/**
 * Become a leader and start a new partition.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully became a leader and started a new partition.
 * @retval OT_ERROR_INVALID_STATE  Thread is disabled.
 */
OTAPI otError OTCALL otThreadBecomeLeader(otInstance *aInstance);

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
 * The function retains diagnostic information for an attached Child by its Child ID or RLOC16.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[in]   aChildId    The Child ID or RLOC16 for the attached child.
 * @param[out]  aChildInfo  A pointer to where the child information is placed.
 *
 * @retval OT_ERROR_NONE          @p aChildInfo was successfully updated with the info for the given ID.
 * @retval OT_ERROR_NOT_FOUND     No valid child with this Child ID.
 * @retval OT_ERROR_INVALID_ARGS  If @p aChildInfo is NULL.
 *
 */
OTAPI otError OTCALL otThreadGetChildInfoById(otInstance *aInstance, uint16_t aChildId, otChildInfo *aChildInfo);

/**
 * The function retains diagnostic information for an attached Child by the internal table index.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[in]   aChildIndex  The table index.
 * @param[out]  aChildInfo   A pointer to where the child information is placed.
 *
 * @retval OT_ERROR_NONE             @p aChildInfo was successfully updated with the info for the given index.
 * @retval OT_ERROR_NOT_FOUND        No valid child at this index.
 * @retval OT_ERROR_INVALID_ARGS     Either @p aChildInfo is NULL, or @p aChildIndex is out of range (higher
 *                                   than max table index).
 *
 * @sa otGetMaxAllowedChildren
 *
 */
OTAPI otError OTCALL otThreadGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex,
                                                 otChildInfo *aChildInfo);

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
OTAPI otError OTCALL otThreadGetRouterInfo(otInstance *aInstance, uint16_t aRouterId, otRouterInfo *aRouterInfo);

/**
 * This function gets an EID cache entry.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[in]   aIndex    An index into the EID cache table.
 * @param[out]  aEntry    A pointer to where the EID information is placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the EID cache entry.
 * @retval OT_ERROR_INVALID_ARGS  @p aIndex was out of bounds or @p aEntry was NULL.
 *
 */
OTAPI otError OTCALL otThreadGetEidCacheEntry(otInstance *aInstance, uint8_t aIndex, otEidCacheEntry *aEntry);

/**
 * Get the thrPSKc.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 *
 * @returns A pointer to a buffer containing the thrPSKc.
 *
 * @sa otThreadSetPSKc
 */
OTAPI const uint8_t *OTCALL otThreadGetPSKc(otInstance *aInstance);

/**
 * Set the thrPSKc.
 *
 * This function will only succeed when Thread protocols are disabled.  A successful
 * call to this function will also invalidate the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aPSKc       A pointer to a buffer containing the thrPSKc.
 *
 * @retval OT_ERROR_NONE           Successfully set the thrPSKc.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otThreadGetPSKc
 */
OTAPI otError OTCALL otThreadSetPSKc(otInstance *aInstance, const uint8_t *aPSKc);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_THREAD_FTD_H_
