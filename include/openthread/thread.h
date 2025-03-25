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
 */

#define OT_THREAD_VERSION_INVALID 0 ///< Invalid Thread version
#define OT_THREAD_VERSION_1_1 2     ///< Thread Version 1.1
#define OT_THREAD_VERSION_1_2 3     ///< Thread Version 1.2
#define OT_THREAD_VERSION_1_3 4     ///< Thread Version 1.3
#define OT_THREAD_VERSION_1_3_1 5   ///< Thread Version 1.3.1 (alias for 1.4)
#define OT_THREAD_VERSION_1_4 5     ///< Thread Version 1.4

/**
 * Maximum value length of Thread Base TLV.
 */
#define OT_NETWORK_BASE_TLV_MAX_LENGTH 254

#define OT_NETWORK_MAX_ROUTER_ID 62 ///< Maximum Router ID

/**
 * Represents a Thread device role.
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
 * Represents an MLE Link Mode configuration.
 */
typedef struct otLinkModeConfig
{
    bool mRxOnWhenIdle : 1; ///< 1, if the sender has its receiver on when not transmitting. 0, otherwise.
    bool mDeviceType : 1;   ///< 1, if the sender is an FTD. 0, otherwise.
    bool mNetworkData : 1;  ///< 1, if the sender requires the full Network Data. 0, otherwise.
} otLinkModeConfig;

/**
 * Holds diagnostic information for a neighboring Thread node
 */
typedef struct
{
    otExtAddress mExtAddress;           ///< IEEE 802.15.4 Extended Address
    uint32_t     mAge;                  ///< Seconds since last heard
    uint32_t     mConnectionTime;       ///< Seconds since link establishment (requires `CONFIG_UPTIME_ENABLE`)
    uint16_t     mRloc16;               ///< RLOC16
    uint32_t     mLinkFrameCounter;     ///< Link Frame Counter
    uint32_t     mMleFrameCounter;      ///< MLE Frame Counter
    uint8_t      mLinkQualityIn;        ///< Link Quality In
    int8_t       mAverageRssi;          ///< Average RSSI
    int8_t       mLastRssi;             ///< Last observed RSSI
    uint8_t      mLinkMargin;           ///< Link Margin
    uint16_t     mFrameErrorRate;       ///< Frame error rate (0xffff->100%). Requires error tracking feature.
    uint16_t     mMessageErrorRate;     ///< (IPv6) msg error rate (0xffff->100%). Requires error tracking feature.
    uint16_t     mVersion;              ///< Thread version of the neighbor
    bool         mRxOnWhenIdle : 1;     ///< rx-on-when-idle
    bool         mFullThreadDevice : 1; ///< Full Thread Device
    bool         mFullNetworkData : 1;  ///< Full Network Data
    bool         mIsChild : 1;          ///< Is the neighbor a child
} otNeighborInfo;

#define OT_NEIGHBOR_INFO_ITERATOR_INIT 0 ///< Initializer for otNeighborInfoIterator.

typedef int16_t otNeighborInfoIterator; ///< Used to iterate through neighbor table.

/**
 * Represents the Thread Leader Data.
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
 * Holds diagnostic information for a Thread Router
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
    uint8_t      mVersion;             ///< Thread version

    /**
     * Parent CSL parameters are only relevant when OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE is enabled.
     */
    uint8_t mCslClockAccuracy; ///< CSL clock accuracy, in ± ppm
    uint8_t mCslUncertainty;   ///< CSL uncertainty, in ±10 us
} otRouterInfo;

/**
 * Represents the IP level counters.
 */
typedef struct otIpCounters
{
    uint32_t mTxSuccess; ///< The number of IPv6 packets successfully transmitted.
    uint32_t mRxSuccess; ///< The number of IPv6 packets successfully received.
    uint32_t mTxFailure; ///< The number of IPv6 packets failed to transmit.
    uint32_t mRxFailure; ///< The number of IPv6 packets failed to receive.
} otIpCounters;

/**
 * Represents the Thread MLE counters.
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
    uint16_t mBetterParentAttachAttempts;    ///< Number of attempts to attach to find a better parent (parent search).

    uint64_t mDisabledTime; ///< Number of milliseconds device has been in OT_DEVICE_ROLE_DISABLED role.
    uint64_t mDetachedTime; ///< Number of milliseconds device has been in OT_DEVICE_ROLE_DETACHED role.
    uint64_t mChildTime;    ///< Number of milliseconds device has been in OT_DEVICE_ROLE_CHILD role.
    uint64_t mRouterTime;   ///< Number of milliseconds device has been in OT_DEVICE_ROLE_ROUTER role.
    uint64_t mLeaderTime;   ///< Number of milliseconds device has been in OT_DEVICE_ROLE_LEADER role.
    uint64_t mTrackedTime;  ///< Number of milliseconds tracked by previous counters.

    /**
     * Number of times device changed its parent.
     *
     * A parent change can happen if device detaches from its current parent and attaches to a different one, or even
     * while device is attached when the periodic parent search feature is enabled  (please see option
     * OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE).
     */
    uint16_t mParentChanges;
} otMleCounters;

/**
 * Represents the MLE Parent Response data.
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
 * This callback informs the application that the detaching process has finished.
 *
 * @param[in] aContext A pointer to application-specific context.
 */
typedef void (*otDetachGracefullyCallback)(void *aContext);

/**
 * Informs the application about the result of waking a Wake-up End Device.
 *
 * @param[in] aError   OT_ERROR_NONE    Indicates that the Wake-up End Device has been added as a neighbor.
 *                     OT_ERROR_FAILED  Indicates that the Wake-up End Device has not received a wake-up frame, or it
 *                                      has failed the MLE procedure.
 * @param[in] aContext A pointer to application-specific context.
 */
typedef void (*otWakeupCallback)(otError aError, void *aContext);

/**
 * Starts Thread protocol operation.
 *
 * The interface must be up when calling this function.
 *
 * Calling this function with @p aEnabled set to FALSE stops any ongoing processes of detaching started by
 * otThreadDetachGracefully(). Its callback will be called.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnabled  TRUE if Thread is enabled, FALSE otherwise.
 *
 * @retval OT_ERROR_NONE           Successfully started Thread protocol operation.
 * @retval OT_ERROR_INVALID_STATE  The network interface was not up.
 */
otError otThreadSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Gets the Thread protocol version.
 *
 * The constants `OT_THREAD_VERSION_*` define the numerical version values.
 *
 * @returns the Thread protocol version.
 */
uint16_t otThreadGetVersion(void);

/**
 * Indicates whether a node is the only router on the network.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   It is the only router in the network.
 * @retval FALSE  It is a child or is not a single router in the network.
 */
bool otThreadIsSingleton(otInstance *aInstance);

/**
 * Starts a Thread Discovery scan.
 *
 * @note A successful call to this function enables the rx-on-when-idle mode for the entire scan procedure.
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
 * @retval OT_ERROR_NONE           Successfully started a Thread Discovery Scan.
 * @retval OT_ERROR_INVALID_STATE  The IPv6 interface is not enabled (netif is not up).
 * @retval OT_ERROR_NO_BUFS        Could not allocate message for Discovery Request.
 * @retval OT_ERROR_BUSY           Thread Discovery Scan is already in progress.
 */
otError otThreadDiscover(otInstance              *aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aPanId,
                         bool                     aJoiner,
                         bool                     aEnableEui64Filtering,
                         otHandleActiveScanResult aCallback,
                         void                    *aCallbackContext);

/**
 * Determines if an MLE Thread Discovery is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 */
bool otThreadIsDiscoverInProgress(otInstance *aInstance);

/**
 * Sets the Thread Joiner Advertisement when discovering Thread network.
 *
 * Thread Joiner Advertisement is used to allow a Joiner to advertise its own application-specific information
 * (such as Vendor ID, Product ID, Discriminator, etc.) via a newly-proposed Joiner Advertisement TLV,
 * and to make this information available to Commissioners or Commissioner Candidates without human interaction.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aOui             The Vendor IEEE OUI value that will be included in the Joiner Advertisement. Only the
 *                              least significant 3 bytes will be used, and the most significant byte will be ignored.
 * @param[in]  aAdvData         A pointer to the AdvData that will be included in the Joiner Advertisement.
 * @param[in]  aAdvDataLength   The length of AdvData in bytes.
 *
 * @retval OT_ERROR_NONE         Successfully set Joiner Advertisement.
 * @retval OT_ERROR_INVALID_ARGS Invalid AdvData.
 */
otError otThreadSetJoinerAdvertisement(otInstance    *aInstance,
                                       uint32_t       aOui,
                                       const uint8_t *aAdvData,
                                       uint8_t        aAdvDataLength);

#define OT_JOINER_ADVDATA_MAX_LENGTH 64 ///< Maximum AdvData Length of Joiner Advertisement

/**
 * Gets the Thread Child Timeout (in seconds) used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Thread Child Timeout value in seconds.
 *
 * @sa otThreadSetChildTimeout
 */
uint32_t otThreadGetChildTimeout(otInstance *aInstance);

/**
 * Sets the Thread Child Timeout (in seconds) used when operating in the Child role.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aTimeout  The timeout value in seconds.
 *
 * @sa otThreadGetChildTimeout
 */
void otThreadSetChildTimeout(otInstance *aInstance, uint32_t aTimeout);

/**
 * Gets the IEEE 802.15.4 Extended PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
 *
 * @sa otThreadSetExtendedPanId
 */
const otExtendedPanId *otThreadGetExtendedPanId(otInstance *aInstance);

/**
 * Sets the IEEE 802.15.4 Extended PAN ID.
 *
 * @note Can only be called while Thread protocols are disabled. A successful
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
 */
otError otThreadSetExtendedPanId(otInstance *aInstance, const otExtendedPanId *aExtendedPanId);

/**
 * Returns a pointer to the Leader's RLOC.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderRloc  A pointer to the Leader's RLOC.
 *
 * @retval OT_ERROR_NONE          The Leader's RLOC was successfully written to @p aLeaderRloc.
 * @retval OT_ERROR_INVALID_ARGS  @p aLeaderRloc was NULL.
 * @retval OT_ERROR_DETACHED      Not currently attached to a Thread Partition.
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
 */
otError otThreadSetLinkMode(otInstance *aInstance, otLinkModeConfig aConfig);

/**
 * Get the Thread Network Key.
 *
 * @param[in]   aInstance     A pointer to an OpenThread instance.
 * @param[out]  aNetworkKey   A pointer to an `otNetworkKey` to return the Thread Network Key.
 *
 * @sa otThreadSetNetworkKey
 */
void otThreadGetNetworkKey(otInstance *aInstance, otNetworkKey *aNetworkKey);

/**
 * Get the `otNetworkKeyRef` for Thread Network Key.
 *
 * Requires the build-time feature `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` to be enabled.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 *
 * @returns Reference to the Thread Network Key stored in memory.
 *
 * @sa otThreadSetNetworkKeyRef
 */
otNetworkKeyRef otThreadGetNetworkKeyRef(otInstance *aInstance);

/**
 * Set the Thread Network Key.
 *
 * Succeeds only when Thread protocols are disabled.  A successful
 * call to this function invalidates the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aKey        A pointer to a buffer containing the Thread Network Key.
 *
 * @retval OT_ERROR_NONE            Successfully set the Thread Network Key.
 * @retval OT_ERROR_INVALID_STATE   Thread protocols are enabled.
 *
 * @sa otThreadGetNetworkKey
 */
otError otThreadSetNetworkKey(otInstance *aInstance, const otNetworkKey *aKey);

/**
 * Set the Thread Network Key as a `otNetworkKeyRef`.
 *
 * Succeeds only when Thread protocols are disabled.  A successful
 * call to this function invalidates the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * Requires the build-time feature `OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE` to be enabled.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aKeyRef     Reference to the Thread Network Key.
 *
 * @retval OT_ERROR_NONE            Successfully set the Thread Network Key.
 * @retval OT_ERROR_INVALID_STATE   Thread protocols are enabled.
 *
 * @sa otThreadGetNetworkKeyRef
 */
otError otThreadSetNetworkKeyRef(otInstance *aInstance, otNetworkKeyRef aKeyRef);

/**
 * Gets the Thread Routing Locator (RLOC) address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Routing Locator (RLOC) address.
 */
const otIp6Address *otThreadGetRloc(otInstance *aInstance);

/**
 * Gets the Mesh Local EID address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local EID address.
 */
const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance);

/**
 * Returns a pointer to the Mesh Local Prefix.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Mesh Local Prefix.
 */
const otMeshLocalPrefix *otThreadGetMeshLocalPrefix(otInstance *aInstance);

/**
 * Sets the Mesh Local Prefix.
 *
 * Succeeds only when Thread protocols are disabled.  A successful
 * call to this function invalidates the Active and Pending Operational Datasets in
 * non-volatile memory.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aMeshLocalPrefix  A pointer to the Mesh Local Prefix.
 *
 * @retval OT_ERROR_NONE           Successfully set the Mesh Local Prefix.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 */
otError otThreadSetMeshLocalPrefix(otInstance *aInstance, const otMeshLocalPrefix *aMeshLocalPrefix);

/**
 * Gets the Thread link-local IPv6 address.
 *
 * The Thread link local address is derived using IEEE802.15.4 Extended Address as Interface Identifier.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to Thread link-local IPv6 address.
 */
const otIp6Address *otThreadGetLinkLocalIp6Address(otInstance *aInstance);

/**
 * Gets the Thread Link-Local All Thread Nodes multicast address.
 *
 * The address is a link-local Unicast Prefix-Based Multicast Address [RFC 3306], with:
 *   - flgs set to 3 (P = 1 and T = 1)
 *   - scop set to 2
 *   - plen set to 64
 *   - network prefix set to the Mesh Local Prefix
 *   - group ID set to 1
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to Thread Link-Local All Thread Nodes multicast address.
 */
const otIp6Address *otThreadGetLinkLocalAllThreadNodesMulticastAddress(otInstance *aInstance);

/**
 * Gets the Thread Realm-Local All Thread Nodes multicast address.
 *
 * The address is a realm-local Unicast Prefix-Based Multicast Address [RFC 3306], with:
 *   - flgs set to 3 (P = 1 and T = 1)
 *   - scop set to 3
 *   - plen set to 64
 *   - network prefix set to the Mesh Local Prefix
 *   - group ID set to 1
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to Thread Realm-Local All Thread Nodes multicast address.
 */
const otIp6Address *otThreadGetRealmLocalAllThreadNodesMulticastAddress(otInstance *aInstance);

/**
 * Retrieves the Service ALOC for given Service ID.
 *
 * @param[in]   aInstance     A pointer to an OpenThread instance.
 * @param[in]   aServiceId    Service ID to get ALOC for.
 * @param[out]  aServiceAloc  A pointer to output the Service ALOC. MUST NOT BE NULL.
 *
 * @retval OT_ERROR_NONE      Successfully retrieved the Service ALOC.
 * @retval OT_ERROR_DETACHED  The Thread interface is not currently attached to a Thread Partition.
 */
otError otThreadGetServiceAloc(otInstance *aInstance, uint8_t aServiceId, otIp6Address *aServiceAloc);

/**
 * Get the Thread Network Name.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Network Name.
 *
 * @sa otThreadSetNetworkName
 */
const char *otThreadGetNetworkName(otInstance *aInstance);

/**
 * Set the Thread Network Name.
 *
 * Succeeds only when Thread protocols are disabled.  A successful
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
 */
otError otThreadSetNetworkName(otInstance *aInstance, const char *aNetworkName);

/**
 * Gets the Thread Domain Name.
 *
 * @note Available since Thread 1.2.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread Domain Name.
 *
 * @sa otThreadSetDomainName
 */
const char *otThreadGetDomainName(otInstance *aInstance);

/**
 * Sets the Thread Domain Name. Only succeeds when Thread protocols are disabled.
 *
 * @note Available since Thread 1.2.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aDomainName   A pointer to the Thread Domain Name.
 *
 * @retval OT_ERROR_NONE           Successfully set the Thread Domain Name.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otThreadGetDomainName
 */
otError otThreadSetDomainName(otInstance *aInstance, const char *aDomainName);

/**
 * Sets or clears the Interface Identifier manually specified for the Thread Domain Unicast Address.
 *
 * Available when `OPENTHREAD_CONFIG_DUA_ENABLE` is enabled.
 *
 * @note Only available since Thread 1.2.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aIid        A pointer to the Interface Identifier to set or NULL to clear.
 *
 * @retval OT_ERROR_NONE           Successfully set/cleared the Interface Identifier.
 * @retval OT_ERROR_INVALID_ARGS   The specified Interface Identifier is reserved.
 *
 * @sa otThreadGetFixedDuaInterfaceIdentifier
 */
otError otThreadSetFixedDuaInterfaceIdentifier(otInstance *aInstance, const otIp6InterfaceIdentifier *aIid);

/**
 * Gets the Interface Identifier manually specified for the Thread Domain Unicast Address.
 *
 * Available when `OPENTHREAD_CONFIG_DUA_ENABLE` is enabled.
 *
 * @note Only available since Thread 1.2.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Interface Identifier which was set manually, or NULL if none was set.
 *
 * @sa otThreadSetFixedDuaInterfaceIdentifier
 */
const otIp6InterfaceIdentifier *otThreadGetFixedDuaInterfaceIdentifier(otInstance *aInstance);

/**
 * Gets the thrKeySequenceCounter.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySequenceCounter value.
 *
 * @sa otThreadSetKeySequenceCounter
 */
uint32_t otThreadGetKeySequenceCounter(otInstance *aInstance);

/**
 * Sets the thrKeySequenceCounter.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySequenceCounter  The thrKeySequenceCounter value.
 *
 * @sa otThreadGetKeySequenceCounter
 */
void otThreadSetKeySequenceCounter(otInstance *aInstance, uint32_t aKeySequenceCounter);

/**
 * Gets the thrKeySwitchGuardTime (in hours).
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otThreadSetKeySwitchGuardTime
 */
uint16_t otThreadGetKeySwitchGuardTime(otInstance *aInstance);

/**
 * Sets the thrKeySwitchGuardTime (in hours).
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aKeySwitchGuardTime  The thrKeySwitchGuardTime value (in hours).
 *
 * @sa otThreadGetKeySwitchGuardTime
 */
void otThreadSetKeySwitchGuardTime(otInstance *aInstance, uint16_t aKeySwitchGuardTime);

/**
 * Detach from the Thread network.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully detached from the Thread network.
 * @retval OT_ERROR_INVALID_STATE  Thread is disabled.
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
 */
otError otThreadBecomeChild(otInstance *aInstance);

/**
 * Gets the next neighbor information. It is used to go through the entries of
 * the neighbor table.
 *
 * @param[in]      aInstance  A pointer to an OpenThread instance.
 * @param[in,out]  aIterator  A pointer to the iterator context. To get the first neighbor entry
                              it should be set to OT_NEIGHBOR_INFO_ITERATOR_INIT.
 * @param[out]     aInfo      A pointer to the neighbor information.
 *
 * @retval OT_ERROR_NONE         Successfully found the next neighbor entry in table.
 * @retval OT_ERROR_NOT_FOUND     No subsequent neighbor entry exists in the table.
 * @retval OT_ERROR_INVALID_ARGS  @p aIterator or @p aInfo was NULL.
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
 */
otDeviceRole otThreadGetDeviceRole(otInstance *aInstance);

/**
 * Convert the device role to human-readable string.
 *
 * @param[in] aRole   The device role to convert.
 *
 * @returns A string representing @p aRole.
 */
const char *otThreadDeviceRoleToString(otDeviceRole aRole);

/**
 * Get the Thread Leader Data.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aLeaderData  A pointer to where the leader data is placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the leader data.
 * @retval OT_ERROR_DETACHED      Not currently attached.
 */
otError otThreadGetLeaderData(otInstance *aInstance, otLeaderData *aLeaderData);

/**
 * Get the Leader's Router ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Router ID.
 */
uint8_t otThreadGetLeaderRouterId(otInstance *aInstance);

/**
 * Get the Leader's Weight.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Leader's Weight.
 */
uint8_t otThreadGetLeaderWeight(otInstance *aInstance);

/**
 * Get the Partition ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Partition ID.
 */
uint32_t otThreadGetPartitionId(otInstance *aInstance);

/**
 * Get the RLOC16.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The RLOC16.
 */
uint16_t otThreadGetRloc16(otInstance *aInstance);

/**
 * The function retrieves diagnostic information for a Thread Router as parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aParentInfo  A pointer to where the parent router information is placed.
 */
otError otThreadGetParentInfo(otInstance *aInstance, otRouterInfo *aParentInfo);

/**
 * The function retrieves the average RSSI for the Thread Parent.
 *
 * @param[in]   aInstance    A pointer to an OpenThread instance.
 * @param[out]  aParentRssi  A pointer to where the parent RSSI should be placed.
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
 */
otError otThreadGetParentLastRssi(otInstance *aInstance, int8_t *aLastRssi);

/**
 * Starts the process for child to search for a better parent while staying attached to its current parent.
 *
 * Must be used when device is attached as a child.
 *
 * @retval OT_ERROR_NONE           Successfully started the process to search for a better parent.
 * @retval OT_ERROR_INVALID_STATE  Device role is not child.
 */
otError otThreadSearchForBetterParent(otInstance *aInstance);

/**
 * Gets the IPv6 counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IPv6 counters.
 */
const otIpCounters *otThreadGetIp6Counters(otInstance *aInstance);

/**
 * Resets the IPv6 counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
void otThreadResetIp6Counters(otInstance *aInstance);

/**
 * Gets the time-in-queue histogram for messages in the TX queue.
 *
 * Requires `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE`.
 *
 * Histogram of the time-in-queue of messages in the transmit queue is collected. The time-in-queue is tracked for
 * direct transmissions only and is measured as the duration from when a message is added to the transmit queue until
 * it is passed to the MAC layer for transmission or dropped.
 *
 * The histogram is returned as an array of `uint32_t` values with `aNumBins` entries. The first entry in the array
 * (at index 0) represents the number of messages with a time-in-queue less than `aBinInterval`. The second entry
 * represents the number of messages with a time-in-queue greater than or equal to `aBinInterval`, but less than
 * `2 * aBinInterval`. And so on. The last entry represents the number of messages with time-in-queue  greater than or
 * equal to `(aNumBins - 1) * aBinInterval`.
 *
 * The collected statistics can be reset by calling `otThreadResetTimeInQueueStat()`. The histogram information is
 * collected since the OpenThread instance was initialized or since the last time statistics collection was reset by
 * calling the `otThreadResetTimeInQueueStat()`.
 *
 * Pointers @p aNumBins and @p aBinInterval MUST NOT be NULL.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[out] aNumBins       Pointer to return the number of bins in histogram (array length).
 * @param[out] aBinInterval   Pointer to return the histogram bin interval length in milliseconds.
 *
 * @returns A pointer to an array of @p aNumBins entries representing the collected histogram info.
 */
const uint32_t *otThreadGetTimeInQueueHistogram(otInstance *aInstance, uint16_t *aNumBins, uint32_t *aBinInterval);

/**
 * Gets the maximum time-in-queue for messages in the TX queue.
 *
 * Requires `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE`.
 *
 * The time-in-queue is tracked for direct transmissions only and is measured as the duration from when a message is
 * added to the transmit queue until it is passed to the MAC layer for transmission or dropped.
 *
 * The collected statistics can be reset by calling `otThreadResetTimeInQueueStat()`.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 *
 * @returns The maximum time-in-queue in milliseconds for all messages in the TX queue (so far).
 */
uint32_t otThreadGetMaxTimeInQueue(otInstance *aInstance);

/**
 * Resets the TX queue time-in-queue statistics.
 *
 * Requires `OPENTHREAD_CONFIG_TX_QUEUE_STATISTICS_ENABLE`.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 */
void otThreadResetTimeInQueueStat(otInstance *aInstance);

/**
 * Gets the Thread MLE counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the Thread MLE counters.
 */
const otMleCounters *otThreadGetMleCounters(otInstance *aInstance);

/**
 * Resets the Thread MLE counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
void otThreadResetMleCounters(otInstance *aInstance);

/**
 * Gets the current attach duration (number of seconds since the device last attached).
 *
 * If the device is not currently attached, zero will be returned.
 *
 * Unlike the role-tracking variables in `otMleCounters`, which track the cumulative time the device is in each role,
 * this function tracks the time since the last successful attachment, indicating how long the device has been
 * connected to the Thread mesh (regardless of its role, whether acting as a child, router, or leader).
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @returns The number of seconds since last attached.
 */
uint32_t otThreadGetCurrentAttachDuration(otInstance *aInstance);

/**
 * Pointer is called every time an MLE Parent Response message is received.
 *
 * This is used in `otThreadRegisterParentResponseCallback()`.
 *
 * @param[in]  aInfo     A pointer to a location on stack holding the stats data.
 * @param[in]  aContext  A pointer to callback client-specific context.
 */
typedef void (*otThreadParentResponseCallback)(otThreadParentResponseInfo *aInfo, void *aContext);

/**
 * Registers a callback to receive MLE Parent Response data.
 *
 * Requires `OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE`.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aCallback  A pointer to a function that is called upon receiving an MLE Parent Response message.
 * @param[in]  aContext   A pointer to callback client-specific context.
 */
void otThreadRegisterParentResponseCallback(otInstance                    *aInstance,
                                            otThreadParentResponseCallback aCallback,
                                            void                          *aContext);

/**
 * Represents the Thread Discovery Request data.
 */
typedef struct otThreadDiscoveryRequestInfo
{
    otExtAddress mExtAddress;   ///< IEEE 802.15.4 Extended Address of the requester
    uint8_t      mVersion : 4;  ///< Thread version.
    bool         mIsJoiner : 1; ///< Whether is from joiner.
} otThreadDiscoveryRequestInfo;

/**
 * Pointer is called every time an MLE Discovery Request message is received.
 *
 * @param[in]  aInfo     A pointer to the Discovery Request info data.
 * @param[in]  aContext  A pointer to callback application-specific context.
 */
typedef void (*otThreadDiscoveryRequestCallback)(const otThreadDiscoveryRequestInfo *aInfo, void *aContext);

/**
 * Sets a callback to receive MLE Discovery Request data.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aCallback  A pointer to a function that is called upon receiving an MLE Discovery Request message.
 * @param[in]  aContext   A pointer to callback application-specific context.
 */
void otThreadSetDiscoveryRequestCallback(otInstance                      *aInstance,
                                         otThreadDiscoveryRequestCallback aCallback,
                                         void                            *aContext);

/**
 * Pointer type defines the callback to notify the outcome of a `otThreadLocateAnycastDestination()`
 * request.
 *
 * @param[in] aContext            A pointer to an arbitrary context (provided when callback is registered).
 * @param[in] aError              The error when handling the request. OT_ERROR_NONE indicates success.
 *                                OT_ERROR_RESPONSE_TIMEOUT indicates a destination could not be found.
 *                                OT_ERROR_ABORT indicates the request was aborted.
 * @param[in] aMeshLocalAddress   A pointer to the mesh-local EID of the closest destination of the anycast address
 *                                when @p aError is OT_ERROR_NONE, NULL otherwise.
 * @param[in] aRloc16             The RLOC16 of the destination if found, otherwise invalid RLOC16 (0xfffe).
 */
typedef void (*otThreadAnycastLocatorCallback)(void               *aContext,
                                               otError             aError,
                                               const otIp6Address *aMeshLocalAddress,
                                               uint16_t            aRloc16);

/**
 * Requests the closest destination of a given anycast address to be located.
 *
 * Is only available when `OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE` is enabled.
 *
 * If a previous request is ongoing, a subsequent call to this function will cancel and replace the earlier request.
 *
 * @param[in] aInstance         A pointer to an OpenThread instance.
 * @param[in] aAnycastAddress   The anycast address to locate. MUST NOT be NULL.
 * @param[in] aCallback         The callback function to report the result.
 * @param[in] aContext          An arbitrary context used with @p aCallback.
 *
 * @retval OT_ERROR_NONE          The request started successfully. @p aCallback will be invoked to report the result.
 * @retval OT_ERROR_INVALID_ARGS  The @p aAnycastAddress is not a valid anycast address or @p aCallback is NULL.
 * @retval OT_ERROR_NO_BUFS       Out of buffer to prepare and send the request message.
 */
otError otThreadLocateAnycastDestination(otInstance                    *aInstance,
                                         const otIp6Address            *aAnycastAddress,
                                         otThreadAnycastLocatorCallback aCallback,
                                         void                          *aContext);

/**
 * Indicates whether an anycast locate request is currently in progress.
 *
 * Is only available when `OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE` is enabled.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns TRUE if an anycast locate request is currently in progress, FALSE otherwise.
 */
bool otThreadIsAnycastLocateInProgress(otInstance *aInstance);

/**
 * Sends a Proactive Address Notification (ADDR_NTF.ntf) message.
 *
 * Is only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aDestination  The destination to send the ADDR_NTF.ntf message.
 * @param[in]  aTarget       The target address of the ADDR_NTF.ntf message.
 * @param[in]  aMlIid        The ML-IID of the ADDR_NTF.ntf message.
 */
void otThreadSendAddressNotification(otInstance               *aInstance,
                                     otIp6Address             *aDestination,
                                     otIp6Address             *aTarget,
                                     otIp6InterfaceIdentifier *aMlIid);

/**
 * Sends a Proactive Backbone Notification (PRO_BB.ntf) message on the Backbone link.
 *
 * Is only available when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
 *
 * @param[in]  aInstance                    A pointer to an OpenThread instance.
 * @param[in]  aTarget                      The target address of the PRO_BB.ntf message.
 * @param[in]  aMlIid                       The ML-IID of the PRO_BB.ntf message.
 * @param[in]  aTimeSinceLastTransaction    Time since last transaction (in seconds).
 *
 * @retval OT_ERROR_NONE           Successfully sent PRO_BB.ntf on backbone link.
 * @retval OT_ERROR_NO_BUFS        If insufficient message buffers available.
 */
otError otThreadSendProactiveBackboneNotification(otInstance               *aInstance,
                                                  otIp6Address             *aTarget,
                                                  otIp6InterfaceIdentifier *aMlIid,
                                                  uint32_t                  aTimeSinceLastTransaction);

/**
 * Notifies other nodes in the network (if any) and then stops Thread protocol operation.
 *
 * It sends an Address Release if it's a router, or sets its child timeout to 0 if it's a child.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aCallback A pointer to a function that is called upon finishing detaching.
 * @param[in] aContext  A pointer to callback application-specific context.
 *
 * @retval OT_ERROR_NONE Successfully started detaching.
 * @retval OT_ERROR_BUSY Detaching is already in progress.
 */
otError otThreadDetachGracefully(otInstance *aInstance, otDetachGracefullyCallback aCallback, void *aContext);

#define OT_DURATION_STRING_SIZE 21 ///< Recommended size for string representation of `uint32_t` duration in seconds.

/**
 * Converts an `uint32_t` duration (in seconds) to a human-readable string.
 *
 * Requires `OPENTHREAD_CONFIG_UPTIME_ENABLE` to be enabled.
 *
 * The string follows the format "<hh>:<mm>:<ss>" for hours, minutes, seconds (if duration is shorter than one day) or
 * "<dd>d.<hh>:<mm>:<ss>" (if longer than a day).
 *
 * If the resulting string does not fit in @p aBuffer (within its @p aSize characters), the string will be truncated
 * but the outputted string is always null-terminated.
 *
 * Is intended for use with `mAge` or `mConnectionTime` in `otNeighborInfo` or `otChildInfo` structures.
 *
 * @param[in]  aDuration A duration interval in seconds.
 * @param[out] aBuffer   A pointer to a char array to output the string.
 * @param[in]  aSize     The size of @p aBuffer (in bytes). Recommended to use `OT_DURATION_STRING_SIZE`.
 */
void otConvertDurationInSecondsToString(uint32_t aDuration, char *aBuffer, uint16_t aSize);

/**
 * Sets the store frame counter ahead.
 *
 * Requires `OPENTHREAD_CONFIG_DYNAMIC_STORE_FRAME_AHEAD_COUNTER_ENABLE` to be enabled.
 *
 * The OpenThread stack stores the MLE and MAC security frame counter values in non-volatile storage,
 * ensuring they persist across device resets. These saved values are set to be ahead of their current
 * values by the "frame counter ahead" value.
 *
 * @param[in] aInstance                  A pointer to an OpenThread instance.
 * @param[in] aStoreFrameCounterAhead    The store frame counter ahead to set.
 */
void otThreadSetStoreFrameCounterAhead(otInstance *aInstance, uint32_t aStoreFrameCounterAhead);

/**
 * Gets the store frame counter ahead.
 *
 * Requires `OPENTHREAD_CONFIG_DYNAMIC_STORE_FRAME_AHEAD_COUNTER_ENABLE` to be enabled.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The current store frame counter ahead.
 */
uint32_t otThreadGetStoreFrameCounterAhead(otInstance *aInstance);

/**
 * Attempts to wake a Wake-up End Device.
 *
 * Requires `OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE` to be enabled.
 *
 * The wake-up starts with transmitting a wake-up frame sequence to the Wake-up End Device.
 * During the wake-up sequence, and for a short time after the last wake-up frame is sent, the Wake-up Coordinator keeps
 * its receiver on to be able to receive an initial mesh link establishment message from the WED.
 *
 * @warning The functionality implemented by this function is still in the design phase.
 *          Consequently, the prototype and semantics of this function are subject to change.
 *
 * @param[in] aInstance         A pointer to an OpenThread instance.
 * @param[in] aWedAddress       The extended address of the Wake-up End Device.
 * @param[in] aWakeupIntervalUs An interval between consecutive wake-up frames (in microseconds).
 * @param[in] aWakeupDurationMs Duration of the wake-up sequence (in milliseconds).
 * @param[in] aCallback         A pointer to function that is called when the wake-up succeeds or fails.
 * @param[in] aCallbackContext  A pointer to callback application-specific context.
 *
 * @retval OT_ERROR_NONE          Successfully started the wake-up.
 * @retval OT_ERROR_INVALID_STATE Another attachment request is still in progress.
 * @retval OT_ERROR_INVALID_ARGS  The wake-up interval or duration are invalid.
 */
otError otThreadWakeup(otInstance         *aInstance,
                       const otExtAddress *aWedAddress,
                       uint16_t            aWakeupIntervalUs,
                       uint16_t            aWakeupDurationMs,
                       otWakeupCallback    aCallback,
                       void               *aCallbackContext);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_THREAD_H_
