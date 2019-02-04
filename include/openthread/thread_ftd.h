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
#include <openthread/thread.h>

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
 * This structure holds diagnostic information for a Thread Child
 *
 * `mFrameErrorRate` and `mMessageErrorRate` require `OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING` feature to be
 * enabled.
 *
 */
typedef struct
{
    otExtAddress mExtAddress;            ///< IEEE 802.15.4 Extended Address
    uint32_t     mTimeout;               ///< Timeout
    uint32_t     mAge;                   ///< Time last heard
    uint16_t     mRloc16;                ///< RLOC16
    uint16_t     mChildId;               ///< Child ID
    uint8_t      mNetworkDataVersion;    ///< Network Data Version
    uint8_t      mLinkQualityIn;         ///< Link Quality In
    int8_t       mAverageRssi;           ///< Average RSSI
    int8_t       mLastRssi;              ///< Last observed RSSI
    uint16_t     mFrameErrorRate;        ///< Frame error rate (0xffff->100%). Requires error tracking feature.
    uint16_t     mMessageErrorRate;      ///< (IPv6) msg error rate (0xffff->100%). Requires error tracking feature.
    bool         mRxOnWhenIdle : 1;      ///< rx-on-when-idle
    bool         mSecureDataRequest : 1; ///< Secure Data Requests
    bool         mFullThreadDevice : 1;  ///< Full Thread Device
    bool         mFullNetworkData : 1;   ///< Full Network Data
    bool         mIsStateRestoring : 1;  ///< Is in restoring state
} otChildInfo;

#define OT_CHILD_IP6_ADDRESS_ITERATOR_INIT 0 ///< Initializer for otChildIP6AddressIterator

typedef uint16_t otChildIp6AddressIterator; ///< Used to iterate through IPv6 addresses of a Thread Child entry.

/**
 * This structure represents an EID cache entry.
 *
 */
typedef struct otEidCacheEntry
{
    otIp6Address   mTarget;    ///< Target
    otShortAddress mRloc16;    ///< RLOC16
    uint8_t        mAge;       ///< Age (order of use, 0 indicates most recently used entry)
    bool           mValid : 1; ///< Indicates whether or not the cache entry is valid
} otEidCacheEntry;

/**
 * Get the maximum number of children currently allowed.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The maximum number of children currently allowed.
 *
 * @sa otThreadSetMaxAllowedChildren
 *
 */
OTAPI uint8_t OTCALL otThreadGetMaxAllowedChildren(otInstance *aInstance);

/**
 * Set the maximum number of children currently allowed.
 *
 * This parameter can only be set when Thread protocol operation has been stopped.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMaxChildren  The maximum allowed children.
 *
 * @retval  OT_ERROR_NONE           Successfully set the max.
 * @retval  OT_ERROR_INVALID_ARGS   If @p aMaxChildren is not in the range [1, OPENTHREAD_CONFIG_MAX_CHILDREN].
 * @retval  OT_ERROR_INVALID_STATE  If Thread isn't stopped.
 *
 * @sa otThreadGetMaxAllowedChildren, otThreadStop
 *
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
 * Upon becoming a router/leader the node attempts to use this Router Id. If the preferred Router Id is not set or if
 * it can not be used, a randomly generated router id is picked. This property can be set only when the device role is
 * either detached or disabled.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
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
 *
 */
OTAPI uint16_t OTCALL otThreadGetJoinerUdpPort(otInstance *aInstance);

/**
 * Set the Joiner UDP Port.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aJoinerUdpPort  The Joiner UDP Port number.
 *
 * @retval  OT_ERROR_NONE  Successfully set the Joiner UDP Port.
 *
 * @sa otThreadGetJoinerUdpPort
 *
 */
OTAPI otError OTCALL otThreadSetJoinerUdpPort(otInstance *aInstance, uint16_t aJoinerUdpPort);

/**
 * Set Steering data out of band.
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
otError otThreadSetSteeringData(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Get the CONTEXT_ID_REUSE_DELAY parameter used in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otThreadSetContextIdReuseDelay
 *
 */
OTAPI uint32_t OTCALL otThreadGetContextIdReuseDelay(otInstance *aInstance);

/**
 * Set the CONTEXT_ID_REUSE_DELAY parameter used in the Leader role.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDelay    The CONTEXT_ID_REUSE_DELAY value.
 *
 * @sa otThreadGetContextIdReuseDelay
 *
 */
OTAPI void OTCALL otThreadSetContextIdReuseDelay(otInstance *aInstance, uint32_t aDelay);

/**
 * Get the NETWORK_ID_TIMEOUT parameter used in the Router role.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The NETWORK_ID_TIMEOUT value.
 *
 * @sa otThreadSetNetworkIdTimeout
 *
 */
OTAPI uint8_t OTCALL otThreadGetNetworkIdTimeout(otInstance *aInstance);

/**
 * Set the NETWORK_ID_TIMEOUT parameter used in the Leader role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
 *
 * @sa otThreadGetNetworkIdTimeout
 *
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
 *
 */
OTAPI uint8_t OTCALL otThreadGetRouterUpgradeThreshold(otInstance *aInstance);

/**
 * Set the ROUTER_UPGRADE_THRESHOLD parameter used in the Leader role.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aThreshold  The ROUTER_UPGRADE_THRESHOLD value.
 *
 * @sa otThreadGetRouterUpgradeThreshold
 *
 */
OTAPI void OTCALL otThreadSetRouterUpgradeThreshold(otInstance *aInstance, uint8_t aThreshold);

/**
 * Release a Router ID that has been allocated by the device in the Leader role.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aRouterId  The Router ID to release. Valid range is [0, 62].
 *
 * @retval OT_ERROR_NONE           Successfully released the router id.
 * @retval OT_ERROR_INVALID_ARGS   @p aRouterId is not in the range [0, 62].
 * @retval OT_ERROR_INVALID_STATE  The device is not currently operating as a leader.
 * @retval OT_ERROR_NOT_FOUND      The router id is not currently allocated.
 *
 */
OTAPI otError OTCALL otThreadReleaseRouterId(otInstance *aInstance, uint8_t aRouterId);

/**
 * Attempt to become a router.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
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
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
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
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
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
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
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
OTAPI otError OTCALL otThreadGetChildInfoByIndex(otInstance *aInstance, uint8_t aChildIndex, otChildInfo *aChildInfo);

/**
 * This function gets the next IPv6 address (using an iterator) for a given child.
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in]     aChildIndex  The child index.
 * @param[inout]  aIterator    A pointer to the iterator. On success the iterator will be updated to point to next
 *                             entry in the list. To get the first IPv6 address the iterator should be set to
 *                             OT_CHILD_IP6_ADDRESS_ITERATOR_INIT.
 * @param[out]    aAddress     A pointer to an IPv6 address where the child's next address is placed (on success).
 *
 * @retval OT_ERROR_NONE          Successfully found the next IPv6 address (@p aAddress was successfully updated).
 * @retval OT_ERROR_NOT_FOUND     The child has no subsequent IPv6 address entry.
 * @retval OT_ERROR_INVALID_ARGS  @p aIterator or @p aAddress are NULL, or child at @p aChildIndex is not valid.
 *
 * @sa otThreadGetChildInfoByIndex
 *
 */
otError otThreadGetChildNextIp6Address(otInstance *               aInstance,
                                       uint8_t                    aChildIndex,
                                       otChildIp6AddressIterator *aIterator,
                                       otIp6Address *             aAddress);

/**
 * Get the current Router ID Sequence.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Router ID Sequence.
 *
 */
OTAPI uint8_t OTCALL otThreadGetRouterIdSequence(otInstance *aInstance);

/**
 * The function returns the maximum allowed router ID
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 *
 * @returns The maximum allowed router ID.
 *
 */
OTAPI uint8_t OTCALL otThreadGetMaxRouterId(otInstance *aInstance);

/**
 * The function retains diagnostic information for a given Thread Router.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[in]   aRouterId    The router ID or RLOC16 for a given router.
 * @param[out]  aRouterInfo  A pointer to where the router information is placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the router info for given id.
 * @retval OT_ERROR_NOT_FOUND     No router entry with the given id.
 * @retval OT_ERROR_INVALID_ARGS  @p aRouterInfo is NULL.
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
 *
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
 *
 */
OTAPI otError OTCALL otThreadSetPSKc(otInstance *aInstance, const uint8_t *aPSKc);

/**
 * Get the assigned parent priority.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 *
 * @returns The assigned parent priority value, -2 means not assigned.
 *
 * @sa otThreadSetParentPriority
 *
 */
OTAPI int8_t OTCALL otThreadGetParentPriority(otInstance *aInstance);

/**
 * Set the parent priority.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aParentPriority  The parent priority value.
 *
 * @retval OT_ERROR_NONE           Successfully set the parent priority.
 * @retval OT_ERROR_INVALID_ARGS   If the parent priority value is not among 1, 0, -1 and -2.
 *
 * @sa otThreadGetParentPriority
 */
OTAPI otError OTCALL otThreadSetParentPriority(otInstance *aInstance, int8_t aParentPriority);

/**
 * This enumeration defines the constants used in `otThreadChildTableCallback` to indicate whether a child is added or
 * removed.
 *
 */
typedef enum otThreadChildTableEvent
{
    OT_THREAD_CHILD_TABLE_EVENT_CHILD_ADDED,   ///< A child is being added.
    OT_THREAD_CHILD_TABLE_EVENT_CHILD_REMOVED, ///< A child is being removed.
} otThreadChildTableEvent;

/**
 * This function pointer is called to notify that a child is being added to or removed from child table.
 *
 * @param[in]  aEvent      A event flag indicating whether a child is being added or removed.
 * @param[in]  aChildInfo  A pointer to child information structure.
 *
 */
typedef void (*otThreadChildTableCallback)(otThreadChildTableEvent aEvent, const otChildInfo *aChildInfo);

/**
 * This function gets the child table callback function.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @returns  The callback function pointer.
 *
 */
otThreadChildTableCallback otThreadGetChildTableCallback(otInstance *aInstance);

/**
 * This function sets the child table callback function.
 *
 * The provided callback (if non-NULL) will be invoked when a child entry is being added/removed to/from the child
 * table. Subsequent calls to this method will overwrite the previous callback. Note that this callback in invoked
 * while the child table is being updated and always before the `otStateChangedCallback`.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aCallback  A pointer to callback handler function.
 *
 */
void otThreadSetChildTableCallback(otInstance *aInstance, otThreadChildTableCallback aCallback);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_THREAD_FTD_H_
