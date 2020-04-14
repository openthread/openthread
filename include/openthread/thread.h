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
 *  This file defines the OpenThread Thread API (for both FTD and MTD).
 */

#ifndef OPENTHREAD_THREAD_H_
#define OPENTHREAD_THREAD_H_

#include <openthread/dataset.h>
#include <openthread/link.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @note
 *   The functions in this module require `OPENTHREAD_FTD=1` or `OPENTHREAD_MTD=1`.
 *
 * @{
 *
 */

/**
 * Maximum value length of Thread Base TLV.
 */
#define OT_NETWORK_BASE_TLV_MAX_LENGTH 254

#define OT_NETWORK_MAX_ROUTER_ID 62 ///< Maximum Router ID

/**
 * Represents a Thread device role.
 *
 */
typedef enum
{
    OT_DEVICE_ROLE_DISABLED = 0, ///< The Thread stack is disabled.
    OT_DEVICE_ROLE_DETACHED = 1, ///< Not currently participating in a Thread network/partition.
    OT_DEVICE_ROLE_CHILD    = 2, ///< The Thread Child role.
    OT_DEVICE_ROLE_ROUTER   = 3, ///< The Thread Router role.
    OT_DEVICE_ROLE_LEADER   = 4, ///< The Thread Leader role.
} otDeviceRole;

/**
 * This structure represents an MLE Link Mode configuration.
 */
typedef struct otLinkModeConfig
{
    bool mRxOnWhenIdle : 1;       ///< 1, if the sender has its receiver on when not transmitting. 0, otherwise.
    bool mSecureDataRequests : 1; ///< 1, if the sender uses IEEE 802.15.4 to secure all data requests. 0, otherwise.
    bool mDeviceType : 1;         ///< 1, if the sender is an FTD. 0, otherwise.
    bool mNetworkData : 1;        ///< 1, if the sender requires the full Network Data. 0, otherwise.
} otLinkModeConfig;

/**
 * This structure holds diagnostic information for a neighboring Thread node
 *
 */
typedef struct
{
    otExtAddress mExtAddress;            ///< IEEE 802.15.4 Extended Address
    uint32_t     mAge;                   ///< Time last heard
    uint16_t     mRloc16;                ///< RLOC16
    uint32_t     mLinkFrameCounter;      ///< Link Frame Counter
    uint32_t     mMleFrameCounter;       ///< MLE Frame Counter
    uint8_t      mLinkQualityIn;         ///< Link Quality In
    int8_t       mAverageRssi;           ///< Average RSSI
    int8_t       mLastRssi;              ///< Last observed RSSI
    uint16_t     mFrameErrorRate;        ///< Frame error rate (0xffff->100%). Requires error tracking feature.
    uint16_t     mMessageErrorRate;      ///< (IPv6) msg error rate (0xffff->100%). Requires error tracking feature.
    bool         mRxOnWhenIdle : 1;      ///< rx-on-when-idle
    bool         mSecureDataRequest : 1; ///< Secure Data Requests
    bool         mFullThreadDevice : 1;  ///< Full Thread Device
    bool         mFullNetworkData : 1;   ///< Full Network Data
    bool         mIsChild : 1;           ///< Is the neighbor a child
} otNeighborInfo;

#define OT_NEIGHBOR_INFO_ITERATOR_INIT 0 ///< Initializer for otNeighborInfoIterator.

typedef int16_t otNeighborInfoIterator; ///< Used to iterate through neighbor table.

/**
 * This structure represents the Thread Leader Data.
 *
 */
typedef struct otLeaderData
{
    uint32_t mPartitionId;       ///< Partition ID
    uint8_t  mWeighting;         ///< Leader Weight
    uint8_t  mDataVersion;       ///< Full Network Data Version
    uint8_t  mStableDataVersion; ///< Stable Network Data Version
    uint8_t  mLeaderRouterId;    ///< Leader Router ID
} otLeaderData;

/**
 * This structure holds diagnostic information for a Thread Router
 *
 */
typedef struct
{
    otExtAddress mExtAddress;          ///< IEEE 802.15.4 Extended Address
    uint16_t     mRloc16;              ///< RLOC16
    uint8_t      mRouterId;            ///< Router ID
    uint8_t      mNextHop;             ///< Next hop to router
    uint8_t      mPathCost;            ///< Path cost to router
    uint8_t      mLinkQualityIn;       ///< Link Quality In
    uint8_t      mLinkQualityOut;      ///< Link Quality Out
    uint8_t      mAge;                 ///< Time last heard
    bool         mAllocated : 1;       ///< Router ID allocated or not
    bool         mLinkEstablished : 1; ///< Link established with Router ID or not
} otRouterInfo;

/**
 * This structure represents the IP level counters.
 *
 */
typedef struct otIpCounters
{
    uint32_t mTxSuccess; ///< The number of IPv6 packets successfully transmitted.
    uint32_t mRxSuccess; ///< The number of IPv6 packets successfully received.
    uint32_t mTxFailure; ///< The number of IPv6 packets failed to transmit.
    uint32_t mRxFailure; ///< The number of IPv6 packets failed to receive.
} otIpCounters;

/**
 * This structure represents the Thread MLE counters.
 *
 */
typedef struct otMleCounters
{
    uint16_t mDisabledRole;                  ///< Number of times device entered OT_DEVICE_ROLE_DISABLED role.
    uint16_t mDetachedRole;                  ///< Number of times device entered OT_DEVICE_ROLE_DETACHED role.
    uint16_t mChildRole;                     ///< Number of times device entered OT_DEVICE_ROLE_CHILD role.
    uint16_t mRouterRole;                    ///< Number of times device entered OT_DEVICE_ROLE_ROUTER role.
    uint16_t mLeaderRole;                    ///< Number of times device entered OT_DEVICE_ROLE_LEADER role.
    uint16_t mAttachAttempts;                ///< Number of attach attempts while device was detached.
    uint16_t mPartitionIdChanges;            ///< Number of changes to partition ID.
    uint16_t mBetterPartitionAttachAttempts; ///< Number of attempts to attach to a better partition.

    /**
     * Number of times device changed its parents.
     *
     * Support for this counter requires the feature option OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH to
     * be enabled.
     *
     * A parent change can happen if device detaches from its current parent and attaches to a different one, or even
     * while device is attached when the periodic parent search feature is enabled  (please see option
     * OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE).
     *
     */
    uint16_t mParentChanges;
} otMleCounters;

/**
 * This structure represents the MLE Parent Response data.
 *
 */
typedef struct otThreadParentResponseInfo
{
    otExtAddress mExtAddr;      ///< IEEE 802.15.4 Extended Address of the Parent
    uint16_t     mRloc16;       ///< Short address of the Parent
    int8_t       mRssi;         ///< Rssi of the Parent
    int8_t       mPriority;     ///< Parent priority
    uint8_t      mLinkQuality3; ///< Parent Link Quality 3
    uint8_t      mLinkQuality2; ///< Parent Link Quality 2
    uint8_t      mLinkQuality1; ///< Parent Link Quality 1
    bool         mIsAttached;   ///< Is the node receiving parent response attached
} otThreadParentResponseInfo;

/**
 * This function starts Thread protocol operation.
 *
 * The interface must be up when calling this function.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnabled  TRUE if Thread is enabled, FALSE otherwise.
 *
 * @retval OT_ERROR_NONE           Successfully started Thread protocol operation.
 * @retval OT_ERROR_INVALID_STATE  The network interface was not not up.
 *
 */
otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function gets the Thread protocol version.
 *
 * @returns the Thread protocol version.
 *
 */
uint16_t otThreadGetVersion(void);

/**
 * This function indicates whether a node is the only router on the network.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   It is the only router in the network.
 * @retval FALSE  It is a child or is not a single router in the network.
 *
 */
bool otThreadIsSingleton(otInstance *aInstance);

/**
 * This function starts a Thread Discovery scan.
 *
 * @param[in]  aInstance              A pointer to an OpenThread instance.
 * @param[in]  aScanChannels          A bit vector indicating which channels to scan (e.g. OT_CHANNEL_11_MASK).
 * @param[in]  aPanId                 The PAN ID filter (set to Broadcast PAN to disable filter).
 * @param[in]  aJoiner                Value of the Joiner Flag in the Discovery Request TLV.
 * @param[in]  aEnableEui64Filtering  TRUE to filter responses on EUI-64, FALSE otherwise.
 * @param[in]  aCallback              A pointer to a function called on receiving an MLE Discovery Response or
 *                                    scan completes.
 * @param[in]  aCallbackContext       A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE  Accepted the Thread Discovery request.
 * @retval OT_ERROR_BUSY  Already performing an Thread Discovery.
 *
 */
otError otThreadDiscover(otInstance *             aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aPanId,
                         bool                     aJoiner,
                         bool                     aEnableEui64Filtering,
                         otHandleActiveScanResult aCallback,
                         void *                   aCallbackContext);

/**
 * This function determines if an MLE Thread Discovery is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 */
bool otThreadIsDiscoverInProgress(otInstance *aInstance);

/**
 * Get the Thread Child Timeout used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Child Timeout value in seconds.
 *
 * @sa otThreadSetChildTimeout
 *
 */
uint32_t otThreadGetChildTimeout(otInstance *aInstance);

/**
 * Set the Thread Child Timeout used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aTimeout  The timeout value in seconds.
 *
 * @sa otThreadGetChildTimeout
 *
 */
void otThreadSetChildTimeout(otInstance *aInstance, uint32_t aTimeout);

/**
 * Get the IEEE 802.15.4 Extended PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otThreadSetExtendedPanId
 *
 */
const otExtendedPanId *otThreadGetExtendedPanId(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 Extended PAN ID.
 *
 * This function can only be called while Thread protocols are disabled.  A successful
 * call to this function invalidates the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance       A pointer to an OpenThread instance.
 * @param[in]  aExtendedPanId  A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @retval OT_ERROR_NONE           Successfully set the Extended PAN ID.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otThreadGetExtendedPanId
 *
 */
otError otThreadSetExtendedPanId(otInstance *aInstance, const otExtendedPanId *aExtendedPanId);

/**
 * This function returns a pointer to the Leader's RLOC.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderRloc  A pointer to the Leader's RLOC.
 *
 * @retval OT_ERROR_NONE          The Leader's RLOC was successfully written to @p aLeaderRloc.
 * @retval OT_ERROR_INVALID_ARGS  @p aLeaderRloc was NULL.
 * @retval OT_ERROR_DETACHED      Not currently attached to a Thread Partition.
 *
 */
otError otThreadGetLeaderRloc(otInstance *aInstance, otIp6Address *aLeaderRloc);

/**
 * Get the MLE Link Mode configuration.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The MLE Link Mode configuration.
 *
 * @sa otThreadSetLinkMode
 *
 */
otLinkModeConfig otThreadGetLinkMode(otInstance *aInstance);

/**
 * Set the MLE Link Mode configuration.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the Link Mode configuration.
 *
 * @retval OT_ERROR_NONE  Successfully set the MLE Link Mode configuration.
 *
 * @sa otThreadGetLinkMode
 *
 */
otError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig);

/**
 * Get the thrMasterKey.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 *
 * @returns A pointer to a buffer containing the thrMasterKey.
 *
 * @sa otThreadSetMasterKey
 *
 */
const otMasterKey *otThreadGetMasterKey(otInstance *aInstance);

/**
 * Set the thrMasterKey.
 *
 * This function succeeds only when Thread protocols are disabled.  A successful
 * call to this function invalidates the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aKey        A pointer to a buffer containing the thrMasterKey.
 *
 * @retval OT_ERROR_NONE            Successfully set the thrMasterKey.
 * @retval OT_ERROR_INVALID_ARGS    If aKeyLength is larger than 16.
 * @retval OT_ERROR_INVALID_STATE   Thread protocols are enabled.
 *
 * @sa otThreadGetMasterKey
 *
 */
otError otThreadSetMasterKey(otInstance *aInstance, const otMasterKey *aKey);

/**
 * This function returns a pointer to the Thread Routing Locator (RLOC) address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Routing Locator (RLOC) address.
 *
 */
const otIp6Address *otThreadGetRloc(otInstance *aInstance);

/**
 * This function returns a pointer to the Mesh Local EID address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local EID address.
 *
 */
const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance);

/**
 * This function returns a pointer to the Mesh Local Prefix.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local Prefix.
 *
 */
const otMeshLocalPrefix *otThreadGetMeshLocalPrefix(otInstance *aInstance);

/**
 * This function sets the Mesh Local Prefix.
 *
 * This function succeeds only when Thread protocols are disabled.  A successful
 * call to this function invalidates the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aMeshLocalPrefix  A pointer to the Mesh Local Prefix.
 *
 * @retval OT_ERROR_NONE           Successfully set the Mesh Local Prefix.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 */
otError otThreadSetMeshLocalPrefix(otInstance *aInstance, const otMeshLocalPrefix *aMeshLocalPrefix);

/**
 * This function returns the Thread link-local IPv6 address.
 *
 * The Thread link local address is derived using IEEE802.15.4 Extended Address as Interface Identifier.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to Thread link-local IPv6 address.
 *
 */
const otIp6Address *otThreadGetLinkLocalIp6Address(otInstance *aInstance);

/**
 * Get the Thread Network Name.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Network Name.
 *
 * @sa otThreadSetNetworkName
 *
 */
const char *otThreadGetNetworkName(otInstance *aInstance);

/**
 * Set the Thread Network Name.
 *
 * This function succeeds only when Thread protocols are disabled.  A successful
 * call to this function invalidates the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aNetworkName  A pointer to the Thread Network Name.
 *
 * @retval OT_ERROR_NONE           Successfully set the Thread Network Name.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otThreadGetNetworkName
 *
 */
otError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName);

/**
 * Get the Thread Domain Name.
 *
 * This function is only availble since Thread 1.2.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Domain Name.
 *
 * @sa otThreadSetDomainName
 *
 */
const char *otThreadGetDomainName(otInstance *aInstance);

/**
 * Set the Thread Domain Name.
 *
 * This function is only availble since Thread 1.2.
 * This function succeeds only when Thread protocols are disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aDomainName   A pointer to the Thread Domain Name.
 *
 * @retval OT_ERROR_NONE           Successfully set the Thread Domain Name.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otThreadGetDomainName
 *
 */
otError otThreadSetDomainName(otInstance *aInstance, const char *aDomainName);

/**
 * Get the thrKeySequenceCounter.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySequenceCounter value.
 *
 * @sa otThreadSetKeySequenceCounter
 *
 */
uint32_t otThreadGetKeySequenceCounter(otInstance *aInstance);

/**
 * Set the thrKeySequenceCounter.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySequenceCounter  The thrKeySequenceCounter value.
 *
 * @sa otThreadGetKeySequenceCounter
 *
 */
void otThreadSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter);

/**
 * Get the thrKeySwitchGuardTime
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otThreadSetKeySwitchGuardTime
 *
 */
uint32_t otThreadGetKeySwitchGuardTime(otInstance *aInstance);

/**
 * Set the thrKeySwitchGuardTime
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySwitchGuardTime  The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otThreadGetKeySwitchGuardTime
 *
 */
void otThreadSetKeySwitchGuardTime(otInstance *aInstance, uint32_t aKeySwitchGuardTime);

/**
 * Detach from the Thread network.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully detached from the Thread network.
 * @retval OT_ERROR_INVALID_STATE  Thread is disabled.
 *
 */
otError otThreadBecomeDetached(otInstance *aInstance);

/**
 * Attempt to reattach as a child.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully begin attempt to become a child.
 * @retval OT_ERROR_INVALID_STATE  Thread is disabled.
 *
 */
otError otThreadBecomeChild(otInstance *aInstance);

/**
 * This function gets the next neighbor information. It is used to go through the entries of
 * the neighbor table.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the iterator context. To get the first neighbor entry
                             it should be set to OT_NEIGHBOR_INFO_ITERATOR_INIT.
 * @param[out]    aInfo      A pointer to the neighbor information.
 *
 * @retval OT_ERROR_NONE         Successfully found the next neighbor entry in table.
 * @retval OT_ERROR_NOT_FOUND     No subsequent neighbor entry exists in the table.
 * @retval OT_ERROR_INVALID_ARGS  @p aIterator or @p aInfo was NULL.
 *
 */
otError otThreadGetNextNeighborInfo(otInstance *aInstance, otNeighborInfoIterator *aIterator, otNeighborInfo *aInfo);

/**
 * Get the device role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_DEVICE_ROLE_DISABLED  The Thread stack is disabled.
 * @retval OT_DEVICE_ROLE_DETACHED  The device is not currently participating in a Thread network/partition.
 * @retval OT_DEVICE_ROLE_CHILD     The device is currently operating as a Thread Child.
 * @retval OT_DEVICE_ROLE_ROUTER    The device is currently operating as a Thread Router.
 * @retval OT_DEVICE_ROLE_LEADER    The device is currently operating as a Thread Leader.
 *
 */
otDeviceRole otThreadGetDeviceRole(otInstance *aInstance);

/**
 * This function get the Thread Leader Data.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderData  A pointer to where the leader data is placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the leader data.
 * @retval OT_ERROR_DETACHED      Not currently attached.
 *
 */
otError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData);

/**
 * Get the Leader's Router ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Router ID.
 *
 */
uint8_t otThreadGetLeaderRouterId(otInstance *aInstance);

/**
 * Get the Leader's Weight.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Weight.
 *
 */
uint8_t otThreadGetLeaderWeight(otInstance *aInstance);

/**
 * Get the Partition ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Partition ID.
 *
 */
uint32_t otThreadGetPartitionId(otInstance *aInstance);

/**
 * Get the RLOC16.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The RLOC16.
 *
 */
uint16_t otThreadGetRloc16(otInstance *aInstance);

/**
 * The function retrieves diagnostic information for a Thread Router as parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aParentInfo  A pointer to where the parent router information is placed.
 *
 */
otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo);

/**
 * The function retrieves the average RSSI for the Thread Parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aParentRssi  A pointer to where the parent RSSI should be placed.
 *
 */
otError otThreadGetParentAverageRssi(otInstance *aInstance, int8_t *aParentRssi);

/**
 * The function retrieves the RSSI of the last packet from the Thread Parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLastRssi    A pointer to where the last RSSI should be placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the RSSI data.
 * @retval OT_ERROR_FAILED        Unable to get RSSI data.
 * @retval OT_ERROR_INVALID_ARGS  @p aLastRssi is NULL.
 *
 */
otError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi);

/**
 * Get the IPv6 counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IPv6 counters.
 *
 */
const otIpCounters *otThreadGetIp6Counters(otInstance *aInstance);

/**
 * Reset the IPv6 counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 */
void otThreadResetIp6Counters(otInstance *aInstance);

/**
 * Get the Thread MLE counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread MLE counters.
 *
 */
const otMleCounters *otThreadGetMleCounters(otInstance *aInstance);

/**
 * Reset the Thread MLE counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 */
void otThreadResetMleCounters(otInstance *aInstance);

/**
 * This function pointer is called every time an MLE Parent Response message is received.
 *
 * @param[in]  aStats    pointer to a location on stack holding the stats data.
 * @param[in]  aContext  A pointer to callback client-specific context.
 *
 */
typedef void (*otThreadParentResponseCallback)(otThreadParentResponseInfo *aInfo, void *aContext);

/**
 * This function registers a callback to receive MLE Parent Response data.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aCallback  A pointer to a function that is called upon receiving an MLE Parent Response message.
 * @param[in]  aContext   A pointer to callback client-specific context.
 *
 */
void otThreadRegisterParentResponseCallback(otInstance *                   aInstance,
                                            otThreadParentResponseCallback aCallback,
                                            void *                         aContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_THREAD_H_
