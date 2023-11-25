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
 *   This file includes definitions for MLE functionality required by the Thread Child, Router, and Leader roles.
 */

#ifndef MLE_HPP_
#define MLE_HPP_

#include "openthread-core-config.h"

#include "common/callback.hpp"
#include "common/encoding.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "crypto/aes_ccm.hpp"
#include "mac/mac.hpp"
#include "meshcop/joiner_router.hpp"
#include "meshcop/meshcop.hpp"
#include "net/udp6.hpp"
#include "thread/child.hpp"
#include "thread/link_metrics.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/neighbor_table.hpp"
#include "thread/network_data_types.hpp"
#include "thread/router.hpp"

namespace ot {

/**
 * @addtogroup core-mle MLE
 *
 * @brief
 *   This module includes definitions for the MLE protocol.
 *
 * @{
 *
 * @defgroup core-mle-core Core
 * @defgroup core-mle-router Router
 * @defgroup core-mle-tlvs TLVs
 *
 * @}
 */

class SupervisionListener;
class UnitTester;

/**
 * @namespace ot::Mle
 *
 * @brief
 *   This namespace includes definitions for the MLE protocol.
 */

namespace Mle {

/**
 * @addtogroup core-mle-core
 *
 * @brief
 *   This module includes definitions for MLE functionality required by the Thread Child, Router, and Leader roles.
 *
 * @{
 *
 */

#if OPENTHREAD_FTD
class MleRouter;
#endif

/**
 * Implements MLE functionality required by the Thread EndDevices, Router, and Leader roles.
 *
 */
class Mle : public InstanceLocator, private NonCopyable
{
#if OPENTHREAD_FTD
    friend class MleRouter;
#endif
    friend class DiscoverScanner;
    friend class ot::Instance;
    friend class ot::Notifier;
    friend class ot::SupervisionListener;
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    friend class ot::LinkMetrics::Initiator;
#endif
    friend class ot::UnitTester;

public:
    /**
     * Initializes the MLE object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Mle(Instance &aInstance);

    /**
     * Enables MLE.
     *
     * @retval kErrorNone     Successfully enabled MLE.
     * @retval kErrorAlready  MLE was already enabled.
     *
     */
    Error Enable(void);

    /**
     * Disables MLE.
     *
     * @retval kErrorNone     Successfully disabled MLE.
     *
     */
    Error Disable(void);

    /**
     * Starts the MLE protocol operation.
     *
     * @retval kErrorNone           Successfully started the protocol operation.
     * @retval kErrorInvalidState   IPv6 interface is down or device is in raw-link mode.
     *
     */
    Error Start(void) { return Start(kNormalAttach); }

    /**
     * Stops the MLE protocol operation.
     *
     */
    void Stop(void) { Stop(kUpdateNetworkDatasets); }

    /**
     * Restores network information from non-volatile memory (if any).
     *
     */
    void Restore(void);

    /**
     * Stores network information into non-volatile memory.
     *
     * @retval kErrorNone      Successfully store the network information.
     * @retval kErrorNoBufs    Could not store the network information due to insufficient memory space.
     *
     */
    Error Store(void);

    /**
     * Generates an MLE Announce message.
     *
     * @param[in]  aChannel        The channel to use when transmitting.
     *
     */
    void SendAnnounce(uint8_t aChannel) { SendAnnounce(aChannel, kNormalAnnounce); }

    /**
     * Causes the Thread interface to detach from the Thread network.
     *
     * @retval kErrorNone          Successfully detached from the Thread network.
     * @retval kErrorInvalidState  MLE is Disabled.
     *
     */
    Error BecomeDetached(void);

    /**
     * Causes the Thread interface to attempt an MLE attach.
     *
     * @retval kErrorNone          Successfully began the attach process.
     * @retval kErrorInvalidState  MLE is Disabled.
     * @retval kErrorBusy          An attach process is in progress.
     *
     */
    Error BecomeChild(void);

    /**
     * Notifies other nodes in the network (if any) and then stops Thread protocol operation.
     *
     * It sends an Address Release if it's a router, or sets its child timeout to 0 if it's a child.
     *
     * @param[in] aCallback A pointer to a function that is called upon finishing detaching.
     * @param[in] aContext  A pointer to callback application-specific context.
     *
     * @retval kErrorNone   Successfully started detaching.
     * @retval kErrorBusy   Detaching is already in progress.
     *
     */
    Error DetachGracefully(otDetachGracefullyCallback aCallback, void *aContext);

    /**
     * Indicates whether or not the Thread device is attached to a Thread network.
     *
     * @retval TRUE   Attached to a Thread network.
     * @retval FALSE  Not attached to a Thread network.
     *
     */
    bool IsAttached(void) const;

    /**
     * Indicates whether device is currently attaching or not.
     *
     * Note that an already attached device may also be in attaching state. Examples of this include a leader/router
     * trying to attach to a better partition, or a child trying to find a better parent (when feature
     * `OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE` is enabled).
     *
     * @retval TRUE   Device is currently trying to attach.
     * @retval FALSE  Device is not in middle of attach process.
     *
     */
    bool IsAttaching(void) const { return (mAttachState != kAttachStateIdle); }

    /**
     * Returns the current Thread device role.
     *
     * @returns The current Thread device role.
     *
     */
    DeviceRole GetRole(void) const { return mRole; }

    /**
     * Indicates whether device role is disabled.
     *
     * @retval TRUE   Device role is disabled.
     * @retval FALSE  Device role is not disabled.
     *
     */
    bool IsDisabled(void) const { return (mRole == kRoleDisabled); }

    /**
     * Indicates whether device role is detached.
     *
     * @retval TRUE   Device role is detached.
     * @retval FALSE  Device role is not detached.
     *
     */
    bool IsDetached(void) const { return (mRole == kRoleDetached); }

    /**
     * Indicates whether device role is child.
     *
     * @retval TRUE   Device role is child.
     * @retval FALSE  Device role is not child.
     *
     */
    bool IsChild(void) const { return (mRole == kRoleChild); }

    /**
     * Indicates whether device role is router.
     *
     * @retval TRUE   Device role is router.
     * @retval FALSE  Device role is not router.
     *
     */
    bool IsRouter(void) const { return (mRole == kRoleRouter); }

    /**
     * Indicates whether device role is leader.
     *
     * @retval TRUE   Device role is leader.
     * @retval FALSE  Device role is not leader.
     *
     */
    bool IsLeader(void) const { return (mRole == kRoleLeader); }

    /**
     * Indicates whether device role is either router or leader.
     *
     * @retval TRUE   Device role is either router or leader.
     * @retval FALSE  Device role is neither router nor leader.
     *
     */
    bool IsRouterOrLeader(void) const;

    /**
     * Returns the Device Mode as reported in the Mode TLV.
     *
     * @returns The Device Mode as reported in the Mode TLV.
     *
     */
    DeviceMode GetDeviceMode(void) const { return mDeviceMode; }

    /**
     * Sets the Device Mode as reported in the Mode TLV.
     *
     * @param[in]  aDeviceMode  The device mode to set.
     *
     * @retval kErrorNone         Successfully set the Mode TLV.
     * @retval kErrorInvalidArgs  The mode combination specified in @p aMode is invalid.
     *
     */
    Error SetDeviceMode(DeviceMode aDeviceMode);

    /**
     * Indicates whether or not the device is rx-on-when-idle.
     *
     * @returns TRUE if rx-on-when-idle, FALSE otherwise.
     *
     */
    bool IsRxOnWhenIdle(void) const { return mDeviceMode.IsRxOnWhenIdle(); }

    /**
     * Indicates whether or not the device is a Full Thread Device.
     *
     * @returns TRUE if a Full Thread Device, FALSE otherwise.
     *
     */
    bool IsFullThreadDevice(void) const { return mDeviceMode.IsFullThreadDevice(); }

    /**
     * Indicates whether or not the device is a Minimal End Device.
     *
     * @returns TRUE if the device is a Minimal End Device, FALSE otherwise.
     *
     */
    bool IsMinimalEndDevice(void) const { return mDeviceMode.IsMinimalEndDevice(); }

    /**
     * Gets the Network Data type (full set or stable subset) that this device requests.
     *
     * @returns The Network Data type requested by this device.
     *
     */
    NetworkData::Type GetNetworkDataType(void) const { return mDeviceMode.GetNetworkDataType(); }

    /**
     * Returns a pointer to the Mesh Local Prefix.
     *
     * @returns A reference to the Mesh Local Prefix.
     *
     */
    const Ip6::NetworkPrefix &GetMeshLocalPrefix(void) const { return mMeshLocalPrefix; }

    /**
     * Sets the Mesh Local Prefix.
     *
     * @param[in]  aMeshLocalPrefix  A reference to the Mesh Local Prefix.
     *
     */
    void SetMeshLocalPrefix(const Ip6::NetworkPrefix &aMeshLocalPrefix);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * Sets the Mesh Local IID.
     *
     * Available only when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
     *
     * @param[in] aMlIid  The Mesh Local IID.
     *
     * @retval kErrorNone           Successfully configured Mesh Local IID.
     * @retval kErrorInvalidState   If the Thread stack is already enabled.
     *
     */
    Error SetMeshLocalIid(const Ip6::InterfaceIdentifier &aMlIid);
#endif

    /**
     * Returns a reference to the Thread link-local address.
     *
     * The Thread link local address is derived using IEEE802.15.4 Extended Address as Interface Identifier.
     *
     * @returns A reference to the Thread link local address.
     *
     */
    const Ip6::Address &GetLinkLocalAddress(void) const { return mLinkLocal64.GetAddress(); }

    /**
     * Updates the link local address.
     *
     * Call this method when the IEEE 802.15.4 Extended Address has changed.
     *
     */
    void UpdateLinkLocalAddress(void);

    /**
     * Returns a reference to the link-local all Thread nodes multicast address.
     *
     * @returns A reference to the link-local all Thread nodes multicast address.
     *
     */
    const Ip6::Address &GetLinkLocalAllThreadNodesAddress(void) const { return mLinkLocalAllThreadNodes.GetAddress(); }

    /**
     * Returns a reference to the realm-local all Thread nodes multicast address.
     *
     * @returns A reference to the realm-local all Thread nodes multicast address.
     *
     */
    const Ip6::Address &GetRealmLocalAllThreadNodesAddress(void) const
    {
        return mRealmLocalAllThreadNodes.GetAddress();
    }

    /**
     * Gets the parent when operating in End Device mode.
     *
     * @returns A reference to the parent.
     *
     */
    Parent &GetParent(void) { return mParent; }

    /**
     * Gets the parent when operating in End Device mode.
     *
     * @returns A reference to the parent.
     *
     */
    const Parent &GetParent(void) const { return mParent; }

    /**
     * The method retrieves information about the parent.
     *
     * @param[out] aParentInfo     Reference to a parent information structure.
     *
     * @retval kErrorNone          Successfully retrieved the parent info and updated @p aParentInfo.
     * @retval kErrorInvalidState  Device role is not child.
     *
     */
    Error GetParentInfo(Router::Info &aParentInfo) const;

    /**
     * Get the parent candidate.
     *
     * The parent candidate is valid when attempting to attach to a new parent.
     *
     */
    Parent &GetParentCandidate(void) { return mParentCandidate; }

    /**
     * Starts the process for child to search for a better parent while staying attached to its current
     * parent
     *
     * @retval kErrorNone          Successfully started the process to search for a better parent.
     * @retval kErrorInvalidState  Device role is not child.
     *
     */
    Error SearchForBetterParent(void);

    /**
     * Indicates whether or not an IPv6 address is an RLOC.
     *
     * @retval TRUE   If @p aAddress is an RLOC.
     * @retval FALSE  If @p aAddress is not an RLOC.
     *
     */
    bool IsRoutingLocator(const Ip6::Address &aAddress) const;

    /**
     * Indicates whether or not an IPv6 address is an ALOC.
     *
     * @retval TRUE   If @p aAddress is an ALOC.
     * @retval FALSE  If @p aAddress is not an ALOC.
     *
     */
    bool IsAnycastLocator(const Ip6::Address &aAddress) const;

    /**
     * Indicates whether or not an IPv6 address is a Mesh Local Address.
     *
     * @retval TRUE   If @p aAddress is a Mesh Local Address.
     * @retval FALSE  If @p aAddress is not a Mesh Local Address.
     *
     */
    bool IsMeshLocalAddress(const Ip6::Address &aAddress) const;

    /**
     * Returns the MLE Timeout value.
     *
     * @returns The MLE Timeout value in seconds.
     *
     */
    uint32_t GetTimeout(void) const { return mTimeout; }

    /**
     * Sets the MLE Timeout value.
     *
     * @param[in]  aTimeout  The Timeout value in seconds.
     *
     */
    void SetTimeout(uint32_t aTimeout);

    /**
     * Returns the RLOC16 assigned to the Thread interface.
     *
     * @returns The RLOC16 assigned to the Thread interface.
     *
     */
    uint16_t GetRloc16(void) const { return mRloc16; }

    /**
     * Returns a reference to the RLOC assigned to the Thread interface.
     *
     * @returns A reference to the RLOC assigned to the Thread interface.
     *
     */
    const Ip6::Address &GetMeshLocal16(void) const { return mMeshLocal16.GetAddress(); }

    /**
     * Returns a reference to the ML-EID assigned to the Thread interface.
     *
     * @returns A reference to the ML-EID assigned to the Thread interface.
     *
     */
    const Ip6::Address &GetMeshLocal64(void) const { return mMeshLocal64.GetAddress(); }

    /**
     * Returns a reference to the ML-EID as a `Netif::UnicastAddress`.
     *
     * @returns A reference to the ML-EID.
     *
     */
    Ip6::Netif::UnicastAddress &GetMeshLocal64UnicastAddress(void) { return mMeshLocal64; }

    /**
     * Returns the Router ID of the Leader.
     *
     * @returns The Router ID of the Leader.
     *
     */
    uint8_t GetLeaderId(void) const { return mLeaderData.GetLeaderRouterId(); }

    /**
     * Retrieves the Leader's RLOC.
     *
     * @param[out]  aAddress  A reference to the Leader's RLOC.
     *
     * @retval kErrorNone      Successfully retrieved the Leader's RLOC.
     * @retval kErrorDetached  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    Error GetLeaderAddress(Ip6::Address &aAddress) const;

    /**
     * Retrieves the Leader's ALOC.
     *
     * @param[out]  aAddress  A reference to the Leader's ALOC.
     *
     * @retval kErrorNone      Successfully retrieved the Leader's ALOC.
     * @retval kErrorDetached  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    Error GetLeaderAloc(Ip6::Address &aAddress) const { return GetLocatorAddress(aAddress, kAloc16Leader); }

    /**
     * Computes the Commissioner's ALOC.
     *
     * @param[out]  aAddress        A reference to the Commissioner's ALOC.
     * @param[in]   aSessionId      Commissioner session id.
     *
     * @retval kErrorNone      Successfully retrieved the Commissioner's ALOC.
     * @retval kErrorDetached  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    Error GetCommissionerAloc(Ip6::Address &aAddress, uint16_t aSessionId) const
    {
        return GetLocatorAddress(aAddress, CommissionerAloc16FromId(aSessionId));
    }

    /**
     * Retrieves the Service ALOC for given Service ID.
     *
     * @param[in]   aServiceId Service ID to get ALOC for.
     * @param[out]  aAddress   A reference to the Service ALOC.
     *
     * @retval kErrorNone      Successfully retrieved the Service ALOC.
     * @retval kErrorDetached  The Thread interface is not currently attached to a Thread Partition.
     *
     */
    Error GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const;

    /**
     * Returns the most recently received Leader Data.
     *
     * @returns  A reference to the most recently received Leader Data.
     *
     */
    const LeaderData &GetLeaderData(void);

    /**
     * Returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     *
     */
    const MessageQueue &GetMessageQueue(void) const { return mDelayedResponses; }

    /**
     * Frees multicast MLE Data Response from Delayed Message Queue if any.
     *
     */
    void RemoveDelayedDataResponseMessage(void);

    /**
     * Gets the MLE counters.
     *
     * @returns A reference to the MLE counters.
     *
     */
    const Counters &GetCounters(void)
    {
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
        UpdateRoleTimeCounters(mRole);
#endif
        return mCounters;
    }

    /**
     * Resets the MLE counters.
     *
     */
    void ResetCounters(void);

#if OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE
    /**
     * Registers the client callback that is called when processing an MLE Parent Response message.
     *
     * @param[in]  aCallback A pointer to a function that is called to deliver MLE Parent Response data.
     * @param[in]  aContext  A pointer to application-specific context.
     *
     */
    void RegisterParentResponseStatsCallback(otThreadParentResponseCallback aCallback, void *aContext)
    {
        mParentResponseCallback.Set(aCallback, aContext);
    }
#endif
    /**
     * Notifies MLE whether the Child ID Request message was transmitted successfully.
     *
     * @param[in]  aMessage  The transmitted message.
     *
     */
    void HandleChildIdRequestTxDone(Message &aMessage);

    /**
     * Requests MLE layer to prepare and send a shorter version of Child ID Request message by only
     * including the mesh-local IPv6 address in the Address Registration TLV.
     *
     * Should be called when a previous MLE Child ID Request message would require fragmentation at 6LoWPAN
     * layer.
     *
     */
    void RequestShorterChildIdRequest(void);

    /**
     * Gets the RLOC or ALOC of a given RLOC16 or ALOC16.
     *
     * @param[out]  aAddress  A reference to the RLOC or ALOC.
     * @param[in]   aLocator  RLOC16 or ALOC16.
     *
     * @retval kErrorNone      If got the RLOC or ALOC successfully.
     * @retval kErrorDetached  If device is detached.
     *
     */
    Error GetLocatorAddress(Ip6::Address &aAddress, uint16_t aLocator) const;

    /**
     * Schedules a Child Update Request.
     *
     */
    void ScheduleChildUpdateRequest(void);

    /*
     * Indicates whether or not the device has restored the network information from
     * non-volatile settings after boot.
     *
     * @retval true  Successfully restored the network information.
     * @retval false No valid network information was found.
     *
     */
    bool HasRestored(void) const { return mHasRestored; }

    /**
     * Indicates whether or not a given netif multicast address instance is a prefix-based address added by MLE and
     * uses the mesh local prefix.
     *
     * @param[in] aAddress   A `Netif::MulticastAddress` address instance.
     *
     * @retval TRUE   If @p aAddress is a prefix-based address which uses the mesh local prefix.
     * @retval FALSE  If @p aAddress is not a prefix-based address which uses the mesh local prefix.
     *
     */
    bool IsMulticastAddressMeshLocalPrefixBased(const Ip6::Netif::MulticastAddress &aAddress) const
    {
        return (&aAddress == &mLinkLocalAllThreadNodes) || (&aAddress == &mRealmLocalAllThreadNodes);
    }

    /**
     * Determines the next hop towards an RLOC16 destination.
     *
     * @param[in]  aDestination  The RLOC16 of the destination.
     *
     * @returns A RLOC16 of the next hop if a route is known, kInvalidRloc16 otherwise.
     *
     */
    uint16_t GetNextHop(uint16_t aDestination) const;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Gets the CSL timeout.
     *
     * @returns CSL timeout
     *
     */
    uint32_t GetCslTimeout(void) const { return mCslTimeout; }

    /**
     * Sets the CSL timeout.
     *
     * @param[in]  aTimeout  The CSL timeout in seconds.
     *
     */
    void SetCslTimeout(uint32_t aTimeout);

    /**
     * Calculates CSL metric of parent.
     *
     * @param[in] aCslAccuracy The CSL accuracy.
     *
     * @returns CSL metric.
     *
     */
    uint64_t CalcParentCslMetric(const Mac::CslAccuracy &aCslAccuracy) const;

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

private:
    //------------------------------------------------------------------------------------------------------------------
    // Constants

    // All time intervals are in milliseconds
    static constexpr uint32_t kParentRequestRouterTimeout     = 750;  // Wait time after tx of Parent Req to routers
    static constexpr uint32_t kParentRequestReedTimeout       = 1250; // Wait timer after tx of Parent Req to REEDs
    static constexpr uint32_t kParentRequestDuplicateMargin   = 50;   // Margin to detect duplicate received Parent Req
    static constexpr uint32_t kChildIdResponseTimeout         = 1250; // Wait time to receive Child ID Response
    static constexpr uint32_t kAttachStartJitter              = 50;   // Max jitter time added to start of attach
    static constexpr uint32_t kAnnounceProcessTimeout         = 250;  // Delay after Announce rx before processing
    static constexpr uint32_t kAnnounceTimeout                = 1400; // Total timeout for sending Announce messages
    static constexpr uint16_t kMinAnnounceDelay               = 80;   // Min delay between Announcement messages
    static constexpr uint32_t kParentResponseMaxDelayRouters  = 500;  // Max response delay for Parent Req to routers
    static constexpr uint32_t kParentResponseMaxDelayAll      = 1000; // Max response delay for Parent Req to all
    static constexpr uint32_t kChildUpdateRequestPendingDelay = 100;  // Delay for aggregating Child Update Req
    static constexpr uint32_t kMaxLinkAcceptDelay             = 1000; // Max delay to tx Link Accept for multicast Req
    static constexpr uint32_t kChildIdRequestTimeout          = 5000; // Max delay to rx a Child ID Req after Parent Res
    static constexpr uint32_t kLinkRequestTimeout             = 2000; // Max delay to rx a Link Accept
    static constexpr uint32_t kDetachGracefullyTimeout        = 1000; // Timeout for graceful detach
    static constexpr uint32_t kUnicastRetxDelay               = 1000; // Base delay for MLE unicast retx
    static constexpr uint32_t kMulticastRetxDelay             = 5000; // Base delay for MLE multicast retx
    static constexpr uint32_t kMulticastRetxDelayMin          = kMulticastRetxDelay * 9 / 10;  // 0.9 * base delay
    static constexpr uint32_t kMulticastRetxDelayMax          = kMulticastRetxDelay * 11 / 10; // 1.1 * base delay

    static constexpr uint8_t kMaxTxCount                = 3; // Max tx count for MLE message
    static constexpr uint8_t kMaxCriticalTxCount        = 6; // Max tx count for critical MLE message
    static constexpr uint8_t kMaxChildKeepAliveAttempts = 4; // Max keep alive attempts before reattach

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Attach backoff feature (CONFIG_ENABLE_ATTACH_BACKOFF) - Intervals are in milliseconds.

    static constexpr uint32_t kAttachBackoffMinInterval = OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_MINIMUM_INTERVAL;
    static constexpr uint32_t kAttachBackoffMaxInterval = OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_MAXIMUM_INTERVAL;
    static constexpr uint32_t kAttachBackoffJitter      = OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_JITTER_INTERVAL;
    static constexpr uint32_t kAttachBackoffDelayToResetCounter =
        OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_DELAY_TO_RESET_BACKOFF_INTERVAL;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Number of Parent Requests in first and next attach cycles

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_3
    // First attach cycle includes two Parent Requests to routers, followed by four to routers and REEDs.
    static constexpr uint8_t kFirstAttachCycleTotalParentRequests       = 6;
    static constexpr uint8_t kFirstAttachCycleNumParentRequestToRouters = 2;
#else
    // First attach cycle in Thread 1.1/1.2 includes a Parent Requests to routers, followed by one to routers and REEDs.
    static constexpr uint8_t kFirstAttachCycleTotalParentRequests       = 2;
    static constexpr uint8_t kFirstAttachCycleNumParentRequestToRouters = 1;
#endif

    // Next attach cycles includes one Parent Request to routers, followed by one to routers and REEDs.
    static constexpr uint8_t kNextAttachCycleTotalParentRequests       = 2;
    static constexpr uint8_t kNextAttachCycleNumParentRequestToRouters = 1;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    static constexpr uint8_t kMaxServiceAlocs = OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS + 1;
#else
    static constexpr uint8_t kMaxServiceAlocs = OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS;
#endif

    static constexpr uint8_t  kMleHopLimit              = 255;
    static constexpr uint8_t  kMleSecurityTagSize       = 4;
    static constexpr uint32_t kStoreFrameCounterAhead   = OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    static constexpr uint8_t  kMaxIpAddressesToRegister = OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER;
    static constexpr uint32_t kDefaultChildTimeout      = OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT;
    static constexpr uint32_t kDefaultCslTimeout        = OPENTHREAD_CONFIG_CSL_TIMEOUT;

    //------------------------------------------------------------------------------------------------------------------
    // Enumerations

    enum Command : uint8_t
    {
        kCommandLinkRequest                   = 0,
        kCommandLinkAccept                    = 1,
        kCommandLinkAcceptAndRequest          = 2,
        kCommandLinkReject                    = 3,
        kCommandAdvertisement                 = 4,
        kCommandUpdate                        = 5,
        kCommandUpdateRequest                 = 6,
        kCommandDataRequest                   = 7,
        kCommandDataResponse                  = 8,
        kCommandParentRequest                 = 9,
        kCommandParentResponse                = 10,
        kCommandChildIdRequest                = 11,
        kCommandChildIdResponse               = 12,
        kCommandChildUpdateRequest            = 13,
        kCommandChildUpdateResponse           = 14,
        kCommandAnnounce                      = 15,
        kCommandDiscoveryRequest              = 16,
        kCommandDiscoveryResponse             = 17,
        kCommandLinkMetricsManagementRequest  = 18,
        kCommandLinkMetricsManagementResponse = 19,
        kCommandLinkProbe                     = 20,
        kCommandTimeSync                      = 99,
    };

    enum AttachMode : uint8_t
    {
        kAnyPartition,    // Attach to any Thread partition.
        kSamePartition,   // Attach to the same Thread partition (when losing connectivity).
        kBetterPartition, // Attach to a better (i.e. higher weight/partition id) Thread partition.
        kDowngradeToReed, // Attach to the same Thread partition during downgrade process.
        kBetterParent,    // Attach to a better parent.
    };

    enum AttachState : uint8_t
    {
        kAttachStateIdle,            // Not currently searching for a parent.
        kAttachStateProcessAnnounce, // Waiting to process a received Announce (to switch channel/pan-id).
        kAttachStateStart,           // Starting to look for a parent.
        kAttachStateParentRequest,   // Send Parent Request (current number tracked by `mParentRequestCounter`).
        kAttachStateAnnounce,        // Send Announce messages
        kAttachStateChildIdRequest,  // Sending a Child ID Request message.
    };

    enum ReattachState : uint8_t
    {
        kReattachStop,    // Reattach process is disabled or finished
        kReattachStart,   // Start reattach process
        kReattachActive,  // Reattach using stored Active Dataset
        kReattachPending, // Reattach using stored Pending Dataset
    };

    static constexpr uint16_t kMleMaxResponseDelay = 1000u; // Max delay before responding to a multicast request.

    enum AddressRegistrationMode : uint8_t // Used by `AppendAddressRegistrationTlv()`
    {
        kAppendAllAddresses,  // Append all addresses (unicast/multicast) in Address Registration TLV.
        kAppendMeshLocalOnly, // Only append the Mesh Local (ML-EID) address in Address Registration TLV.
    };

    enum StartMode : uint8_t // Used in `Start()`.
    {
        kNormalAttach,
        kAnnounceAttach, // Try to attach on the announced thread network with newer active timestamp.
    };

    enum StopMode : uint8_t // Used in `Stop()`.
    {
        kKeepNetworkDatasets,
        kUpdateNetworkDatasets,
    };

    enum AnnounceMode : uint8_t // Used in `SendAnnounce()`
    {
        kNormalAnnounce,
        kOrphanAnnounce,
    };

    enum ParentRequestType : uint8_t
    {
        kToRouters,         // Parent Request to routers only.
        kToRoutersAndReeds, // Parent Request to all routers and REEDs.
    };

    enum ChildUpdateRequestState : uint8_t
    {
        kChildUpdateRequestNone,    // No pending or active Child Update Request.
        kChildUpdateRequestPending, // Pending Child Update Request due to relative OT_CHANGED event.
        kChildUpdateRequestActive,  // Child Update Request has been sent and Child Update Response is expected.
    };

    enum ChildUpdateRequestMode : uint8_t // Used in `SendChildUpdateRequest()`
    {
        kNormalChildUpdateRequest, // Normal Child Update Request.
        kAppendChallengeTlv,       // Append Challenge TLV to Child Update Request even if currently attached.
        kAppendZeroTimeout,        // Use zero timeout when appending Timeout TLV (used for graceful detach).
    };

    enum DataRequestState : uint8_t
    {
        kDataRequestNone,   // Not waiting for a Data Response.
        kDataRequestActive, // Data Request has been sent, Data Response is expected.
    };

    enum SecuritySuite : uint8_t
    {
        k154Security = 0,   // Security suite value indicating that MLE message is not secured.
        kNoSecurity  = 255, // Security suite value indicating that MLE message is secured.
    };

    enum MessageAction : uint8_t
    {
        kMessageSend,
        kMessageReceive,
        kMessageDelay,
        kMessageRemoveDelayed,
    };

    enum MessageType : uint8_t
    {
        kTypeAdvertisement,
        kTypeAnnounce,
        kTypeChildIdRequest,
        kTypeChildIdRequestShort,
        kTypeChildIdResponse,
        kTypeChildUpdateRequestAsChild,
        kTypeChildUpdateResponseAsChild,
        kTypeDataRequest,
        kTypeDataResponse,
        kTypeDiscoveryRequest,
        kTypeDiscoveryResponse,
        kTypeGenericDelayed,
        kTypeGenericUdp,
        kTypeParentRequestToRouters,
        kTypeParentRequestToRoutersReeds,
        kTypeParentResponse,
#if OPENTHREAD_FTD
        kTypeAddressRelease,
        kTypeAddressReleaseReply,
        kTypeAddressReply,
        kTypeAddressSolicit,
        kTypeChildUpdateRequestOfChild,
        kTypeChildUpdateResponseOfChild,
        kTypeChildUpdateResponseOfUnknownChild,
        kTypeLinkAccept,
        kTypeLinkAcceptAndRequest,
        kTypeLinkReject,
        kTypeLinkRequest,
        kTypeParentRequest,
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        kTypeTimeSync,
#endif
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        kTypeLinkMetricsManagementRequest,
        kTypeLinkMetricsManagementResponse,
        kTypeLinkProbe,
#endif
    };

    //------------------------------------------------------------------------------------------------------------------
    // Nested types

    static constexpr uint8_t kMaxTlvListSize = 32; // Maximum number of TLVs in a `TlvList`.

    class TlvList : public Array<uint8_t, kMaxTlvListSize>
    {
    public:
        TlvList(void) = default;

        void Add(uint8_t aTlvType);
        void AddElementsFrom(const TlvList &aTlvList);
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class TxMessage : public Message
    {
    public:
        Error AppendSourceAddressTlv(void);
        Error AppendModeTlv(DeviceMode aMode);
        Error AppendTimeoutTlv(uint32_t aTimeout);
        Error AppendChallengeTlv(const TxChallenge &aChallenge);
        Error AppendResponseTlv(const RxChallenge &aResponse);
        Error AppendLinkFrameCounterTlv(void);
        Error AppendMleFrameCounterTlv(void);
        Error AppendAddress16Tlv(uint16_t aRloc16);
        Error AppendNetworkDataTlv(NetworkData::Type aType);
        Error AppendTlvRequestTlv(const uint8_t *aTlvs, uint8_t aTlvsLength);
        Error AppendLeaderDataTlv(void);
        Error AppendScanMaskTlv(uint8_t aScanMask);
        Error AppendStatusTlv(StatusTlv::Status aStatus);
        Error AppendLinkMarginTlv(uint8_t aLinkMargin);
        Error AppendVersionTlv(void);
        Error AppendAddressRegistrationTlv(AddressRegistrationMode aMode = kAppendAllAddresses);
        Error AppendSupervisionIntervalTlv(uint16_t aInterval);
        Error AppendXtalAccuracyTlv(void);
        Error AppendActiveTimestampTlv(void);
        Error AppendPendingTimestampTlv(void);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        Error AppendTimeRequestTlv(void);
        Error AppendTimeParameterTlv(void);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        Error AppendCslChannelTlv(void);
        Error AppendCslTimeoutTlv(void);
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        Error AppendCslClockAccuracyTlv(void);
#endif
#if OPENTHREAD_FTD
        Error AppendRouteTlv(Neighbor *aNeighbor = nullptr);
        Error AppendActiveDatasetTlv(void);
        Error AppendPendingDatasetTlv(void);
        Error AppendConnectivityTlv(void);
        Error AppendSteeringDataTlv(void);
        Error AppendAddressRegistrationTlv(Child &aChild);
#endif
        template <uint8_t kArrayLength> Error AppendTlvRequestTlv(const uint8_t (&aTlvArray)[kArrayLength])
        {
            return AppendTlvRequestTlv(aTlvArray, kArrayLength);
        }

        Error SendTo(const Ip6::Address &aDestination);
        Error SendAfterDelay(const Ip6::Address &aDestination, uint16_t aDelay);

    private:
        Error AppendCompressedAddressEntry(uint8_t aContextId, const Ip6::Address &aAddress);
        Error AppendAddressEntry(const Ip6::Address &aAddress);
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class RxMessage : public Message
    {
    public:
        Error ReadChallengeTlv(RxChallenge &aChallenge) const;
        Error ReadResponseTlv(RxChallenge &aResponse) const;
        Error ReadFrameCounterTlvs(uint32_t &aLinkFrameCounter, uint32_t &aMleFrameCounter) const;
        Error ReadTlvRequestTlv(TlvList &aTlvList) const;
        Error ReadLeaderDataTlv(LeaderData &aLeaderData) const;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        Error ReadCslClockAccuracyTlv(Mac::CslAccuracy &aCslAccuracy) const;
#endif
#if OPENTHREAD_FTD
        Error ReadRouteTlv(RouteTlv &aRouteTlv) const;
#endif

    private:
        Error ReadChallengeOrResponse(uint8_t aTlvType, RxChallenge &aRxChallenge) const;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct RxInfo
    {
        enum Class : uint8_t
        {
            kUnknown,              // Unknown (default value, also indicates MLE message parse error).
            kAuthoritativeMessage, // Authoritative message (larger received key seq MUST be adopted).
            kPeerMessage,          // Peer message (adopt only if from a known neighbor and is greater by one).
        };

        RxInfo(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
            : mMessage(static_cast<RxMessage &>(aMessage))
            , mMessageInfo(aMessageInfo)
            , mFrameCounter(0)
            , mKeySequence(0)
            , mNeighbor(nullptr)
            , mClass(kUnknown)
        {
        }

        bool IsNeighborStateValid(void) const { return (mNeighbor != nullptr) && mNeighbor->IsStateValid(); }

        RxMessage              &mMessage;      // The MLE message.
        const Ip6::MessageInfo &mMessageInfo;  // The `MessageInfo` associated with the message.
        uint32_t                mFrameCounter; // The frame counter from aux security header.
        uint32_t                mKeySequence;  // The key sequence from aux security header.
        Neighbor               *mNeighbor;     // Neighbor from which message was received (can be `nullptr`).
        Class                   mClass;        // The message class (authoritative, peer, or unknown).
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct DelayedResponseMetadata
    {
        Error AppendTo(Message &aMessage) const { return aMessage.Append(*this); }
        void  ReadFrom(const Message &aMessage);
        void  RemoveFrom(Message &aMessage) const;

        Ip6::Address mDestination; // IPv6 address of the message destination.
        TimeMilli    mSendTime;    // Time when the message shall be sent.
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    OT_TOOL_PACKED_BEGIN
    class SecurityHeader
    {
    public:
        void InitSecurityControl(void) { mSecurityControl = kKeyIdMode2Mic32; }
        bool IsSecurityControlValid(void) const { return (mSecurityControl == kKeyIdMode2Mic32); }

        uint32_t GetFrameCounter(void) const { return LittleEndian::HostSwap32(mFrameCounter); }
        void     SetFrameCounter(uint32_t aCounter) { mFrameCounter = LittleEndian::HostSwap32(aCounter); }

        uint32_t GetKeyId(void) const { return BigEndian::HostSwap32(mKeySource); }
        void     SetKeyId(uint32_t aKeySequence)
        {
            mKeySource = BigEndian::HostSwap32(aKeySequence);
            mKeyIndex  = (aKeySequence & 0x7f) + 1;
        }

    private:
        static constexpr uint8_t kKeyIdMode2Mic32 =
            static_cast<uint8_t>(Mac::Frame::kKeyIdMode2) | static_cast<uint8_t>(Mac::Frame::kSecurityEncMic32);

        uint8_t  mSecurityControl;
        uint32_t mFrameCounter;
        uint32_t mKeySource;
        uint8_t  mKeyIndex;
    } OT_TOOL_PACKED_END;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class ParentCandidate : public Parent
    {
    public:
        void Init(Instance &aInstance) { Parent::Init(aInstance); }
        void Clear(void);
        void CopyTo(Parent &aParent) const;

        RxChallenge mRxChallenge;
        int8_t      mPriority;
        uint8_t     mLinkQuality3;
        uint8_t     mLinkQuality2;
        uint8_t     mLinkQuality1;
        uint16_t    mSedBufferSize;
        uint8_t     mSedDatagramCount;
        uint8_t     mLinkMargin;
        LeaderData  mLeaderData;
        bool        mIsSingleton;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    class ServiceAloc : public Ip6::Netif::UnicastAddress
    {
    public:
        static constexpr uint16_t kNotInUse = Mac::kShortAddrInvalid;

        ServiceAloc(void);

        bool     IsInUse(void) const { return GetAloc16() != kNotInUse; }
        void     MarkAsNotInUse(void) { SetAloc16(kNotInUse); }
        uint16_t GetAloc16(void) const { return GetAddress().GetIid().GetLocator(); }
        void     SetAloc16(uint16_t aAloc16) { GetAddress().GetIid().SetLocator(aAloc16); }
    };
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    void HandleParentSearchTimer(void) { mParentSearch.HandleTimer(); }

    class ParentSearch : public InstanceLocator
    {
    public:
        explicit ParentSearch(Instance &aInstance)
            : InstanceLocator(aInstance)
            , mIsInBackoff(false)
            , mBackoffWasCanceled(false)
            , mRecentlyDetached(false)
            , mBackoffCancelTime(0)
            , mTimer(aInstance)
        {
        }

        void StartTimer(void);
        void UpdateState(void);
        void SetRecentlyDetached(void) { mRecentlyDetached = true; }
        void HandleTimer(void);

    private:
        // All timer intervals are converted to milliseconds.
        static constexpr uint32_t kCheckInterval   = (OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL * 1000u);
        static constexpr uint32_t kBackoffInterval = (OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL * 1000u);
        static constexpr uint32_t kJitterInterval  = (15 * 1000u);
        static constexpr int8_t   kRssThreshold    = OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD;

        using SearchTimer = TimerMilliIn<Mle, &Mle::HandleParentSearchTimer>;

        bool        mIsInBackoff : 1;
        bool        mBackoffWasCanceled : 1;
        bool        mRecentlyDetached : 1;
        TimeMilli   mBackoffCancelTime;
        SearchTimer mTimer;
    };
#endif // OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE

    //------------------------------------------------------------------------------------------------------------------
    // Methods

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    static void HandleDetachGracefullyTimer(Timer &aTimer);

    Error      Start(StartMode aMode);
    void       Stop(StopMode aMode);
    TxMessage *NewMleMessage(Command aCommand);
    void       SetRole(DeviceRole aRole);
    void       Attach(AttachMode aMode);
    void       SetAttachState(AttachState aState);
    void       InitNeighbor(Neighbor &aNeighbor, const RxInfo &aRxInfo);
    void       ClearParentCandidate(void) { mParentCandidate.Clear(); }
    Error      CheckReachability(uint16_t aMeshDest, const Ip6::Header &aIp6Header);
    Error      SendDataRequest(const Ip6::Address &aDestination);
    void       HandleNotifierEvents(Events aEvents);
    void       SendDelayedResponse(TxMessage &aMessage, const DelayedResponseMetadata &aMetadata);
    void       HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void       ReestablishLinkWithNeighbor(Neighbor &aNeighbor);
    void       HandleDetachGracefullyTimer(void);
    bool       IsDetachingGracefully(void) { return mDetachGracefullyTimer.IsRunning(); }
    Error      SendChildUpdateRequest(ChildUpdateRequestMode aMode);
    Error      SendDataRequestAfterDelay(const Ip6::Address &aDestination, uint16_t aDelay);
    Error      SendChildUpdateRequest(void);
    Error      SendChildUpdateResponse(const TlvList      &aTlvList,
                                       const RxChallenge  &aChallenge,
                                       const Ip6::Address &aDestination);
    void       SetRloc16(uint16_t aRloc16);
    void       SetStateDetached(void);
    void       SetStateChild(uint16_t aRloc16);
    void       SetLeaderData(uint32_t aPartitionId, uint8_t aWeighting, uint8_t aLeaderRouterId);
    void       InformPreviousChannel(void);
    bool       IsAnnounceAttach(void) const { return mAlternatePanId != Mac::kPanIdBroadcast; }
    void       ScheduleMessageTransmissionTimer(void);
    void       HandleAttachTimer(void);
    void       HandleDelayedResponseTimer(void);
    void       HandleMessageTransmissionTimer(void);
    void       ProcessKeySequence(RxInfo &aRxInfo);
    void       HandleAdvertisement(RxInfo &aRxInfo);
    void       HandleChildIdResponse(RxInfo &aRxInfo);
    void       HandleChildUpdateRequest(RxInfo &aRxInfo);
    void       HandleChildUpdateResponse(RxInfo &aRxInfo);
    void       HandleDataResponse(RxInfo &aRxInfo);
    void       HandleParentResponse(RxInfo &aRxInfo);
    void       HandleAnnounce(RxInfo &aRxInfo);
    Error      HandleLeaderData(RxInfo &aRxInfo);
    void       ProcessAnnounce(void);
    bool       HasUnregisteredAddress(void);
    uint32_t   GetAttachStartDelay(void) const;
    void       SendParentRequest(ParentRequestType aType);
    Error      SendChildIdRequest(void);
    Error      GetNextAnnounceChannel(uint8_t &aChannel) const;
    bool       HasMoreChannelsToAnnounce(void) const;
    bool       PrepareAnnounceState(void);
    void       SendAnnounce(uint8_t aChannel, AnnounceMode aMode);
    void       SendAnnounce(uint8_t aChannel, const Ip6::Address &aDestination, AnnounceMode aMode = kNormalAnnounce);
    uint32_t   Reattach(void);
    bool       HasAcceptableParentCandidate(void) const;
    Error      DetermineParentRequestType(ParentRequestType &aType) const;
    bool       IsBetterParent(uint16_t                aRloc16,
                              LinkQuality             aLinkQuality,
                              uint8_t                 aLinkMargin,
                              const ConnectivityTlv  &aConnectivityTlv,
                              uint16_t                aVersion,
                              const Mac::CslAccuracy &aCslAccuracy);
    bool       IsNetworkDataNewer(const LeaderData &aLeaderData);
    Error      ProcessMessageSecurity(Crypto::AesCcm::Mode    aMode,
                                      Message                &aMessage,
                                      const Ip6::MessageInfo &aMessageInfo,
                                      uint16_t                aCmdOffset,
                                      const SecurityHeader   &aHeader);
    void RemoveDelayedMessage(Message::SubType aSubType, MessageType aMessageType, const Ip6::Address *aDestination);
    void RemoveDelayedDataRequestMessage(const Ip6::Address &aDestination);

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
    void InformPreviousParent(void);
#endif

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    void UpdateRoleTimeCounters(DeviceRole aRole);
#endif

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ServiceAloc *FindInServiceAlocs(uint16_t aAloc16);
    void         UpdateServiceAlocs(void);
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    void  HandleLinkMetricsManagementRequest(RxInfo &aRxInfo);
    void  HandleLinkProbe(RxInfo &aRxInfo);
    Error SendLinkMetricsManagementResponse(const Ip6::Address &aDestination, LinkMetrics::Status aStatus);
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    void  HandleLinkMetricsManagementResponse(RxInfo &aRxInfo);
    Error SendDataRequestForLinkMetricsReport(const Ip6::Address                      &aDestination,
                                              const LinkMetrics::Initiator::QueryInfo &aQueryInfo);
    Error SendLinkMetricsManagementRequest(const Ip6::Address &aDestination, const ot::Tlv &aSubTlv);
    Error SendLinkProbe(const Ip6::Address &aDestination, uint8_t aSeriesId, uint8_t *aBuf, uint8_t aLength);
    Error SendDataRequest(const Ip6::Address                      &aDestination,
                          const uint8_t                           *aTlvs,
                          uint8_t                                  aTlvsLength,
                          uint16_t                                 aDelay,
                          const LinkMetrics::Initiator::QueryInfo *aQueryInfo = nullptr);
#else
    Error SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength, uint16_t aDelay);
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static void Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress);
    static void Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress, uint16_t aRloc);
#else
    static void Log(MessageAction, MessageType, const Ip6::Address &) {}
    static void Log(MessageAction, MessageType, const Ip6::Address &, uint16_t) {}
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)
    static const char *AttachModeToString(AttachMode aMode);
    static const char *AttachStateToString(AttachState aState);
    static const char *ReattachStateToString(ReattachState aState);
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    static void        LogError(MessageAction aAction, MessageType aType, Error aError);
    static const char *MessageActionToString(MessageAction aAction);
    static const char *MessageTypeToString(MessageType aType);
    static const char *MessageTypeActionToSuffixString(MessageType aType, MessageAction aAction);
    static void        LogProcessError(MessageType aType, Error aError);
    static void        LogSendError(MessageType aType, Error aError);
#else
    static void LogProcessError(MessageType, Error) {}
    static void LogSendError(MessageType, Error) {}
#endif

    //------------------------------------------------------------------------------------------------------------------
    // Variables

    using DetachGracefullyTimer = TimerMilliIn<Mle, &Mle::HandleDetachGracefullyTimer>;
    using AttachTimer           = TimerMilliIn<Mle, &Mle::HandleAttachTimer>;
    using DelayTimer            = TimerMilliIn<Mle, &Mle::HandleDelayedResponseTimer>;
    using MsgTxTimer            = TimerMilliIn<Mle, &Mle::HandleMessageTransmissionTimer>;

    static const otMeshLocalPrefix kMeshLocalPrefixInit;

    bool mRetrieveNewNetworkData : 1;
    bool mRequestRouteTlv : 1;
    bool mHasRestored : 1;
    bool mReceivedResponseFromParent : 1;
    bool mInitiallyAttachedAsSleepy : 1;
#if OPENTHREAD_FTD
    bool mWasLeader : 1;
#endif

    DeviceRole              mRole;
    DeviceMode              mDeviceMode;
    AttachState             mAttachState;
    ReattachState           mReattachState;
    AttachMode              mAttachMode;
    DataRequestState        mDataRequestState;
    AddressRegistrationMode mAddressRegistrationMode;
    ChildUpdateRequestState mChildUpdateRequestState;

    uint8_t mParentRequestCounter;
    uint8_t mChildUpdateAttempts;
    uint8_t mDataRequestAttempts;
    uint8_t mAnnounceChannel;
    uint8_t mAlternateChannel;
#if OPENTHREAD_FTD
    uint8_t mLinkRequestAttempts;
#endif
    uint16_t mRloc16;
    uint16_t mPreviousParentRloc;
    uint16_t mAttachCounter;
    uint16_t mAnnounceDelay;
    uint16_t mAlternatePanId;
    uint32_t mTimeout;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    uint32_t mCslTimeout;
#endif
    uint64_t mAlternateTimestamp;
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    uint64_t mLastUpdatedTimestamp;
#endif

    LeaderData       mLeaderData;
    Parent           mParent;
    NeighborTable    mNeighborTable;
    MessageQueue     mDelayedResponses;
    TxChallenge      mParentRequestChallenge;
    ParentCandidate  mParentCandidate;
    Ip6::Udp::Socket mSocket;
    Counters         mCounters;
#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    ParentSearch mParentSearch;
#endif
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ServiceAloc mServiceAlocs[kMaxServiceAlocs];
#endif
    Callback<otDetachGracefullyCallback> mDetachGracefullyCallback;
#if OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE
    Callback<otThreadParentResponseCallback> mParentResponseCallback;
#endif
    AttachTimer                  mAttachTimer;
    DelayTimer                   mDelayedResponseTimer;
    MsgTxTimer                   mMessageTransmissionTimer;
    DetachGracefullyTimer        mDetachGracefullyTimer;
    Ip6::NetworkPrefix           mMeshLocalPrefix;
    Ip6::Netif::UnicastAddress   mLinkLocal64;
    Ip6::Netif::UnicastAddress   mMeshLocal64;
    Ip6::Netif::UnicastAddress   mMeshLocal16;
    Ip6::Netif::MulticastAddress mLinkLocalAllThreadNodes;
    Ip6::Netif::MulticastAddress mRealmLocalAllThreadNodes;
};

} // namespace Mle

/**
 * @}
 *
 */

} // namespace ot

#endif // MLE_HPP_
