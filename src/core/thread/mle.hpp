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

#include <openthread/thread_ftd.h>

#include "coap/coap_message.hpp"
#include "common/callback.hpp"
#include "common/encoding.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "common/trickle_timer.hpp"
#include "crypto/aes_ccm.hpp"
#include "mac/mac.hpp"
#include "mac/mac_types.hpp"
#include "mac/wakeup_tx_scheduler.hpp"
#include "meshcop/dataset.hpp"
#include "meshcop/joiner_router.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/icmp6.hpp"
#include "net/udp6.hpp"
#include "thread/child.hpp"
#include "thread/child_table.hpp"
#include "thread/link_metrics.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/mle.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/neighbor_table.hpp"
#include "thread/network_data_types.hpp"
#include "thread/router.hpp"
#include "thread/router_table.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

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
 */

/**
 * Implements MLE functionality required by the Thread EndDevices, Router, and Leader roles.
 */
class Mle : public InstanceLocator, private NonCopyable
{
    friend class DiscoverScanner;
    friend class ot::Instance;
    friend class ot::Notifier;
    friend class ot::SupervisionListener;
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    friend class ot::LinkMetrics::Initiator;
#endif
    friend class ot::UnitTester;
#if OPENTHREAD_FTD
    friend class ot::TimeTicker;
    friend class Tmf::Agent;
#endif

public:
    typedef otDetachGracefullyCallback DetachCallback; ///< Callback to signal end of graceful detach.

    typedef otWakeupCallback WakeupCallback; ///< Callback to communicate the result of waking a Wake-up End Device

    /**
     * Initializes the MLE object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Mle(Instance &aInstance);

    /**
     * Enables MLE.
     *
     * @retval kErrorNone     Successfully enabled MLE.
     * @retval kErrorAlready  MLE was already enabled.
     */
    Error Enable(void);

    /**
     * Disables MLE.
     *
     * @retval kErrorNone     Successfully disabled MLE.
     */
    Error Disable(void);

    /**
     * Starts the MLE protocol operation.
     *
     * @retval kErrorNone           Successfully started the protocol operation.
     * @retval kErrorInvalidState   IPv6 interface is down or device is in raw-link mode.
     */
    Error Start(void) { return Start(kNormalAttach); }

    /**
     * Stops the MLE protocol operation.
     */
    void Stop(void) { Stop(kUpdateNetworkDatasets); }

    /**
     * Restores network information from non-volatile memory (if any).
     */
    void Restore(void);

    /**
     * Stores network information into non-volatile memory.
     */
    void Store(void);

    /**
     * Generates an MLE Announce message.
     *
     * @param[in]  aChannel        The channel to use when transmitting.
     */
    void SendAnnounce(uint8_t aChannel) { SendAnnounce(aChannel, kNormalAnnounce); }

    /**
     * Causes the Thread interface to detach from the Thread network.
     *
     * @retval kErrorNone          Successfully detached from the Thread network.
     * @retval kErrorInvalidState  MLE is Disabled.
     */
    Error BecomeDetached(void);

    /**
     * Causes the Thread interface to attempt an MLE attach.
     *
     * @retval kErrorNone          Successfully began the attach process.
     * @retval kErrorInvalidState  MLE is Disabled.
     * @retval kErrorBusy          An attach process is in progress.
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
     */
    Error DetachGracefully(DetachCallback aCallback, void *aContext) { return mDetacher.Detach(aCallback, aContext); }

    /**
     * Indicates whether or not the Thread device is attached to a Thread network.
     *
     * @retval TRUE   Attached to a Thread network.
     * @retval FALSE  Not attached to a Thread network.
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
     */
    bool IsAttaching(void) const { return (mAttachState != kAttachStateIdle); }

    /**
     * Returns the current Thread device role.
     *
     * @returns The current Thread device role.
     */
    DeviceRole GetRole(void) const { return mRole; }

    /**
     * Indicates whether device role is disabled.
     *
     * @retval TRUE   Device role is disabled.
     * @retval FALSE  Device role is not disabled.
     */
    bool IsDisabled(void) const { return (mRole == kRoleDisabled); }

    /**
     * Indicates whether device role is detached.
     *
     * @retval TRUE   Device role is detached.
     * @retval FALSE  Device role is not detached.
     */
    bool IsDetached(void) const { return (mRole == kRoleDetached); }

    /**
     * Indicates whether device role is child.
     *
     * @retval TRUE   Device role is child.
     * @retval FALSE  Device role is not child.
     */
    bool IsChild(void) const { return (mRole == kRoleChild); }

    /**
     * Indicates whether device role is router.
     *
     * @retval TRUE   Device role is router.
     * @retval FALSE  Device role is not router.
     */
    bool IsRouter(void) const { return (mRole == kRoleRouter); }

    /**
     * Indicates whether device role is leader.
     *
     * @retval TRUE   Device role is leader.
     * @retval FALSE  Device role is not leader.
     */
    bool IsLeader(void) const { return (mRole == kRoleLeader); }

    /**
     * Indicates whether device role is either router or leader.
     *
     * @retval TRUE   Device role is either router or leader.
     * @retval FALSE  Device role is neither router nor leader.
     */
    bool IsRouterOrLeader(void) const;

    /**
     * Returns the Device Mode as reported in the Mode TLV.
     *
     * @returns The Device Mode as reported in the Mode TLV.
     */
    DeviceMode GetDeviceMode(void) const { return mDeviceMode; }

    /**
     * Sets the Device Mode as reported in the Mode TLV.
     *
     * @param[in]  aDeviceMode  The device mode to set.
     *
     * @retval kErrorNone         Successfully set the Mode TLV.
     * @retval kErrorInvalidArgs  The mode combination specified in @p aMode is invalid.
     */
    Error SetDeviceMode(DeviceMode aDeviceMode);

    /**
     * Indicates whether or not the device is rx-on-when-idle.
     *
     * @returns TRUE if rx-on-when-idle, FALSE otherwise.
     */
    bool IsRxOnWhenIdle(void) const { return mDeviceMode.IsRxOnWhenIdle(); }

    /**
     * Indicates whether or not the device is a Full Thread Device.
     *
     * @returns TRUE if a Full Thread Device, FALSE otherwise.
     */
    bool IsFullThreadDevice(void) const { return mDeviceMode.IsFullThreadDevice(); }

    /**
     * Indicates whether or not the device is a Minimal End Device.
     *
     * @returns TRUE if the device is a Minimal End Device, FALSE otherwise.
     */
    bool IsMinimalEndDevice(void) const { return mDeviceMode.IsMinimalEndDevice(); }

    /**
     * Gets the Network Data type (full set or stable subset) that this device requests.
     *
     * @returns The Network Data type requested by this device.
     */
    NetworkData::Type GetNetworkDataType(void) const { return mDeviceMode.GetNetworkDataType(); }

    /**
     * Returns a pointer to the Mesh Local Prefix.
     *
     * @returns A reference to the Mesh Local Prefix.
     */
    const Ip6::NetworkPrefix &GetMeshLocalPrefix(void) const { return mMeshLocalPrefix; }

    /**
     * Sets the Mesh Local Prefix.
     *
     * @param[in]  aMeshLocalPrefix  A reference to the Mesh Local Prefix.
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
     */
    Error SetMeshLocalIid(const Ip6::InterfaceIdentifier &aMlIid);
#endif

    /**
     * Returns a reference to the Thread link-local address.
     *
     * The Thread link local address is derived using IEEE802.15.4 Extended Address as Interface Identifier.
     *
     * @returns A reference to the Thread link local address.
     */
    const Ip6::Address &GetLinkLocalAddress(void) const { return mLinkLocalAddress.GetAddress(); }

    /**
     * Updates the link local address.
     *
     * Call this method when the IEEE 802.15.4 Extended Address has changed.
     */
    void UpdateLinkLocalAddress(void);

    /**
     * Returns a reference to the link-local all Thread nodes multicast address.
     *
     * @returns A reference to the link-local all Thread nodes multicast address.
     */
    const Ip6::Address &GetLinkLocalAllThreadNodesAddress(void) const { return mLinkLocalAllThreadNodes.GetAddress(); }

    /**
     * Returns a reference to the realm-local all Thread nodes multicast address.
     *
     * @returns A reference to the realm-local all Thread nodes multicast address.
     */
    const Ip6::Address &GetRealmLocalAllThreadNodesAddress(void) const
    {
        return mRealmLocalAllThreadNodes.GetAddress();
    }

    /**
     * Gets the parent's RLOC16.
     *
     * @returns  The parent's RLOC16, or `kInvalidRloc16` if parent's state is not valid.
     */
    uint16_t GetParentRloc16(void) const;

    /**
     * Gets the parent when operating in End Device mode.
     *
     * @returns A reference to the parent.
     */
    Parent &GetParent(void) { return mParent; }

    /**
     * Gets the parent when operating in End Device mode.
     *
     * @returns A reference to the parent.
     */
    const Parent &GetParent(void) const { return mParent; }

    /**
     * The method retrieves information about the parent.
     *
     * @param[out] aParentInfo     Reference to a parent information structure.
     *
     * @retval kErrorNone          Successfully retrieved the parent info and updated @p aParentInfo.
     * @retval kErrorInvalidState  Device role is not child.
     */
    Error GetParentInfo(Router::Info &aParentInfo) const;

    /**
     * Get the parent candidate.
     *
     * The parent candidate is valid when attempting to attach to a new parent.
     */
    Parent &GetParentCandidate(void) { return mParentCandidate; }

    /**
     * Starts the process for child to search for a better parent while staying attached to its current
     * parent
     *
     * @retval kErrorNone          Successfully started the process to search for a better parent.
     * @retval kErrorInvalidState  Device role is not child.
     */
    Error SearchForBetterParent(void);

    /**
     * Indicates whether or not an IPv6 address is an RLOC.
     *
     * @retval TRUE   If @p aAddress is an RLOC.
     * @retval FALSE  If @p aAddress is not an RLOC.
     */
    bool IsRoutingLocator(const Ip6::Address &aAddress) const;

    /**
     * Indicates whether or not an IPv6 address is an ALOC.
     *
     * @retval TRUE   If @p aAddress is an ALOC.
     * @retval FALSE  If @p aAddress is not an ALOC.
     */
    bool IsAnycastLocator(const Ip6::Address &aAddress) const;

    /**
     * Indicates whether or not an IPv6 address is a Mesh Local Address.
     *
     * @retval TRUE   If @p aAddress is a Mesh Local Address.
     * @retval FALSE  If @p aAddress is not a Mesh Local Address.
     */
    bool IsMeshLocalAddress(const Ip6::Address &aAddress) const;

    /**
     * Returns the MLE Timeout value.
     *
     * @returns The MLE Timeout value in seconds.
     */
    uint32_t GetTimeout(void) const { return mTimeout; }

    /**
     * Sets the MLE Timeout value.
     *
     * @param[in]  aTimeout  The Timeout value in seconds.
     */
    void SetTimeout(uint32_t aTimeout) { SetTimeout(aTimeout, kSendChildUpdateToParent); }

    /**
     * Returns the RLOC16 assigned to the Thread interface.
     *
     * @returns The RLOC16 assigned to the Thread interface.
     */
    uint16_t GetRloc16(void) const { return mRloc16; }

    /**
     * Indicates whether or not this device is using a given RLOC16.
     *
     * @param[in] aRloc16   The RLOC16 to check.
     *
     * @retval TRUE   This device is using @p aRloc16.
     * @retval FALSE  This device is not using @p aRloc16.
     */
    bool HasRloc16(uint16_t aRloc16) const { return mRloc16 == aRloc16; }

    /**
     * Indicates whether or not this device RLOC16 matches a given Router ID.
     *
     * @param[in] aRouterId   The Router ID to check.
     *
     * @retval TRUE   This device's RLOC16 matches the @p aRouterId.
     * @retval FALSE  This device's RLOC16 does not match the @p aRouterId.
     */
    bool MatchesRouterId(uint8_t aRouterId) const { return RouterIdFromRloc16(mRloc16) == aRouterId; }

    /**
     * Indicates whether or not this device's RLOC16 shares the same Router ID with a given RLOC16.
     *
     * A shared Router ID implies that this device and the @ aRloc16 are either directly related as parent and child,
     * or are children of the same parent within the Thread network.
     *
     * @param[in] aRloc16   The RLOC16 to check.
     *
     * @retval TRUE   This device and @p aRloc16 have a matching router ID.
     * @retval FALSE  This device and @p aRloc16 do not have a matching router ID.
     */
    bool HasMatchingRouterIdWith(uint16_t aRloc16) const { return RouterIdMatch(mRloc16, aRloc16); }

    /**
     * Returns the mesh local RLOC IPv6 address assigned to the Thread interface.
     *
     * @returns The mesh local RLOC IPv6 address.
     */
    const Ip6::Address &GetMeshLocalRloc(void) const { return mMeshLocalRloc.GetAddress(); }

    /**
     * Returns the mesh local endpoint identifier (ML-EID) IPv6 address assigned to the Thread interface.
     *
     * @returns The ML-EID address.
     */
    const Ip6::Address &GetMeshLocalEid(void) const { return mMeshLocalEid.GetAddress(); }

    /**
     * Returns a reference to the ML-EID as a `Netif::UnicastAddress`.
     *
     * @returns A reference to the ML-EID.
     */
    Ip6::Netif::UnicastAddress &GetMeshLocalEidUnicastAddress(void) { return mMeshLocalEid; }

    /**
     * Returns the Router ID of the Leader.
     *
     * @returns The Router ID of the Leader.
     */
    uint8_t GetLeaderId(void) const { return mLeaderData.GetLeaderRouterId(); }

    /**
     * Returns the RLOC16 of the Leader.
     *
     * @returns The RLOC16 of the Leader.
     */
    uint16_t GetLeaderRloc16(void) const { return Rloc16FromRouterId(GetLeaderId()); }

    /**
     * Retrieves the Leader's RLOC.
     *
     * @param[out]  aAddress  A reference to an address to return the Leader's RLOC.
     */
    void GetLeaderRloc(Ip6::Address &aAddress) const;

    /**
     * Retrieves the Leader's ALOC.
     *
     * @param[out]  aAddress  A reference to an address to return the Leader's ALOC.
     */
    void GetLeaderAloc(Ip6::Address &aAddress) const;

    /**
     * Retrieves the Commissioner's ALOC for a given session ID.
     *
     * @param[in]   aSessionId      Commissioner session id.
     * @param[out]  aAddress        A reference to an address to return the Commissioner's ALOC.
     */
    void GetCommissionerAloc(uint16_t aSessionId, Ip6::Address &aAddress) const;

    /**
     * Retrieves the Service ALOC for given Service ID.
     *
     * @param[in]   aServiceId Service ID to get ALOC for.
     * @param[out]  aAddress   A reference to an address to return the Service ALOC.
     */
    void GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const;

    /**
     * Returns the most recently received Leader Data.
     *
     * @returns  A reference to the most recently received Leader Data.
     */
    const LeaderData &GetLeaderData(void);

    /**
     * Retrieves information about the MLE message queue used for delayed messages.
     *
     * @param[out] aQueueInfo     A `MessageQueue::Info` to populate with info about the MLE queue.
     */
    void GetMessageQueueInfo(MessageQueue::Info &aQueryInfo) const { mDelayedSender.GetQueueInfo(aQueryInfo); }

    /**
     * Gets the MLE counters.
     *
     * @returns A reference to the MLE counters.
     */
    const Counters &GetCounters(void);

    /**
     * Resets the MLE counters.
     */
    void ResetCounters(void);

    /**
     * Determines the current attach duration (number of seconds since the device last attached).
     *
     * @returns Current attach duration in seconds.
     */
    uint32_t GetCurrentAttachDuration(void) const;

#if OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE
    /**
     * Registers the client callback that is called when processing an MLE Parent Response message.
     *
     * @param[in]  aCallback A pointer to a function that is called to deliver MLE Parent Response data.
     * @param[in]  aContext  A pointer to application-specific context.
     */
    void RegisterParentResponseStatsCallback(otThreadParentResponseCallback aCallback, void *aContext)
    {
        mParentResponseCallback.Set(aCallback, aContext);
    }
#endif

    /**
     * Schedules a Child Update Request.
     */
    void ScheduleChildUpdateRequest(void);

    /**
     * Sends a Child Update Request to the parent.
     *
     * @retval kErrorNone     Successfully prepared and sent an MLE Child Update Request message.
     * @retval kErrorNoBufs   Insufficient buffers to construct the MLE Child Update Request message.
     */
    Error SendChildUpdateRequestToParent(void);

    /*
     * Indicates whether or not the device has restored the network information from
     * non-volatile settings after boot.
     *
     * @retval true  Successfully restored the network information.
     * @retval false No valid network information was found.
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
     */
    bool IsMulticastAddressMeshLocalPrefixBased(const Ip6::Netif::MulticastAddress &aAddress) const
    {
        return (&aAddress == &mLinkLocalAllThreadNodes) || (&aAddress == &mRealmLocalAllThreadNodes);
    }

    /**
     * Schedules a "Child Update Request" transmission if the device is an MTD child.
     *
     * For example, the `Slaac` class, which manages SLAAC addresses, calls this method to notify `Mle` that an
     * existing SLAAC address's Context ID has changed. This can occur due to Network Data updates where the same
     * on-mesh prefix receives a new Context ID.
     */
    void ScheduleChildUpdateRequestIfMtdChild(void);

#if OPENTHREAD_CONFIG_DYNAMIC_STORE_FRAME_AHEAD_COUNTER_ENABLE
    /**
     * Sets the store frame counter ahead.
     *
     * @param[in]  aStoreFrameCounterAhead  The store frame counter ahead to set.
     */
    void SetStoreFrameCounterAhead(uint32_t aStoreFrameCounterAhead)
    {
        mStoreFrameCounterAhead = aStoreFrameCounterAhead;
    }

    /**
     * Gets the current store frame counter ahead.
     *
     * @returns The current store frame counter ahead.
     */
    uint32_t GetStoreFrameCounterAhead(void) { return mStoreFrameCounterAhead; }
#endif // OPENTHREAD_CONFIG_DYNAMIC_STORE_FRAME_AHEAD_COUNTER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Gets the CSL timeout.
     *
     * @returns CSL timeout
     */
    uint32_t GetCslTimeout(void) const { return mCslTimeout; }

    /**
     * Sets the CSL timeout.
     *
     * @param[in]  aTimeout  The CSL timeout in seconds.
     */
    void SetCslTimeout(uint32_t aTimeout);

    /**
     * Calculates CSL metric of parent.
     *
     * @param[in] aCslAccuracy The CSL accuracy.
     *
     * @returns CSL metric.
     */
    uint64_t CalcParentCslMetric(const Mac::CslAccuracy &aCslAccuracy) const;

    /**
     * Indicates whether the device is connected to a parent which supports CSL.
     *
     * @retval TRUE   If parent supports CSL.
     * @retval FALSE  If parent does not support CSL.
     */
    bool IsCslSupported(void) const;
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    /**
     * Attempts to wake a Wake-up End Device.
     *
     * @param[in] aWedAddress The extended address of the Wake-up End Device.
     * @param[in] aIntervalUs An interval between consecutive wake-up frames (in microseconds).
     * @param[in] aDurationMs Duration of the wake-up sequence (in milliseconds).
     * @param[in] aCallback   A pointer to function that is called when the wake-up succeeds or fails.
     * @param[in] aContext    A pointer to callback application-specific context.
     *
     * @retval kErrorNone         Successfully started the wake-up.
     * @retval kErrorInvalidState Another wake-up request is still in progress.
     * @retval kErrorInvalidArgs  The wake-up interval or duration are invalid.
     */
    Error Wakeup(const Mac::ExtAddress &aWedAddress,
                 uint16_t               aIntervalUs,
                 uint16_t               aDurationMs,
                 WakeupCallback         aCallback,
                 void                  *aCallbackContext);
#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE

#if OPENTHREAD_FTD
    /**
     * Indicates whether or not the device is router-eligible.
     *
     * @retval true   If device is router-eligible.
     * @retval false  If device is not router-eligible.
     */
    bool IsRouterEligible(void) const;

    /**
     * Sets whether or not the device is router-eligible.
     *
     * If @p aEligible is false and the device is currently operating as a router, this call will cause the device to
     * detach and attempt to reattach as a child.
     *
     * @param[in]  aEligible  TRUE to configure device router-eligible, FALSE otherwise.
     *
     * @retval kErrorNone         Successfully set the router-eligible configuration.
     * @retval kErrorNotCapable   The device is not capable of becoming a router.
     */
    Error SetRouterEligible(bool aEligible);

    /**
     * Indicates whether a node is the only router on the network.
     *
     * @retval TRUE   It is the only router in the network.
     * @retval FALSE  It is a child or is not a single router in the network.
     */
    bool IsSingleton(void) const;

    /**
     * Generates an Address Solicit request for a Router ID.
     *
     * @param[in]  aStatus  The reason for requesting a Router ID.
     *
     * @retval kErrorNone           Successfully generated an Address Solicit message.
     * @retval kErrorNotCapable     Device is not capable of becoming a router
     * @retval kErrorInvalidState   Thread is not enabled
     */
    Error BecomeRouter(ThreadStatusTlv::Status aStatus);

    /**
     * Specifies the leader weight check behavior used in `BecomeLeader()`.
     */
    enum LeaderWeightCheck : uint8_t{
        kCheckLeaderWeight,  ///< Enforces that the local leader weight is greater than the current leader's weight.
        kIgnoreLeaderWeight, ///< Skips the leader weight check, attempting to become leader regardless.
    };

    /**
     * Attempts to become the leader and start a new partition.
     *
     * If the device is already attached, this method can be used to take over the leader role.
     *
     * @param[in] aMode             Specifies whether to enforce or ignore the leader weight check.
     *
     * @retval kErrorNone           Successfully became leader and started a new partition.
     * @retval kErrorInvalidState   The Thread interface is not enabled.
     * @retval kErrorNotCapable     The device is not router-eligible, or the leader weight check is enabled and the
     *                              local leader weight is less than or equal to the current leader's weight.
     */
    Error BecomeLeader(LeaderWeightCheck aMode);

#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
    /**
     * Gets the device properties which are used to determine the Leader Weight.
     *
     * @returns The current device properties.
     */
    const DeviceProperties &GetDeviceProperties(void) const { return mDeviceProperties; }

    /**
     * Sets the device properties which are then used to determine and set the Leader Weight.
     *
     * @param[in]  aDeviceProperties    The device properties.
     */
    void SetDeviceProperties(const DeviceProperties &aDeviceProperties);
#endif

    /**
     * Returns the Leader Weighting value for this Thread interface.
     *
     * @returns The Leader Weighting value for this Thread interface.
     */
    uint8_t GetLeaderWeight(void) const { return mLeaderWeight; }

    /**
     * Sets the Leader Weighting value for this Thread interface.
     *
     * Directly sets the Leader Weight to the new value replacing its previous value (which may have been
     * determined from a previous call to `SetDeviceProperties()`).
     *
     * @param[in]  aWeight  The Leader Weighting value.
     */
    void SetLeaderWeight(uint8_t aWeight) { mLeaderWeight = aWeight; }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

    /**
     * Returns the preferred Partition Id when operating in the Leader role for certification testing.
     *
     * @returns The preferred Partition Id value.
     */
    uint32_t GetPreferredLeaderPartitionId(void) const { return mPreferredLeaderPartitionId; }

    /**
     * Sets the preferred Partition Id when operating in the Leader role for certification testing.
     *
     * @param[in]  aPartitionId  The preferred Leader Partition Id.
     */
    void SetPreferredLeaderPartitionId(uint32_t aPartitionId) { mPreferredLeaderPartitionId = aPartitionId; }
#endif

    /**
     * Sets the preferred Router Id. Upon becoming a router/leader the node
     * attempts to use this Router Id. If the preferred Router Id is not set or if it
     * can not be used, a randomly generated router Id is picked.
     * This property can be set when he device role is detached or disabled.
     *
     * @param[in]  aRouterId             The preferred Router Id.
     *
     * @retval kErrorNone          Successfully set the preferred Router Id.
     * @retval kErrorInvalidState  Could not set (role is other than detached and disabled)
     */
    Error SetPreferredRouterId(uint8_t aRouterId);

    /**
     * Gets the Partition Id which the device joined successfully once.
     */
    uint32_t GetPreviousPartitionId(void) const { return mPreviousPartitionId; }

    /**
     * Sets the Partition Id which the device joins successfully.
     *
     * @param[in]  aPartitionId   The Partition Id.
     */
    void SetPreviousPartitionId(uint32_t aPartitionId) { mPreviousPartitionId = aPartitionId; }

    /**
     * Sets the Router Id.
     *
     * @param[in]  aRouterId   The Router Id.
     */
    void SetRouterId(uint8_t aRouterId);

    /**
     * Returns the NETWORK_ID_TIMEOUT value.
     *
     * @returns The NETWORK_ID_TIMEOUT value.
     */
    uint8_t GetNetworkIdTimeout(void) const { return mNetworkIdTimeout; }

    /**
     * Sets the NETWORK_ID_TIMEOUT value.
     *
     * @param[in]  aTimeout  The NETWORK_ID_TIMEOUT value.
     */
    void SetNetworkIdTimeout(uint8_t aTimeout) { mNetworkIdTimeout = aTimeout; }

    /**
     * Returns the ROUTER_SELECTION_JITTER value.
     *
     * @returns The ROUTER_SELECTION_JITTER value in seconds.
     */
    uint8_t GetRouterSelectionJitter(void) const { return mRouterRoleTransition.GetJitter(); }

    /**
     * Sets the ROUTER_SELECTION_JITTER value.
     *
     * @param[in] aRouterJitter  The router selection jitter value (in seconds).
     */
    void SetRouterSelectionJitter(uint8_t aRouterJitter) { mRouterRoleTransition.SetJitter(aRouterJitter); }

    /**
     * Indicates whether or not router role transition (upgrade from REED or downgrade to REED) is pending.
     *
     * @retval TRUE    Router role transition is pending.
     * @retval FALSE   Router role transition is not pending
     */
    bool IsRouterRoleTransitionPending(void) const { return mRouterRoleTransition.IsPending(); }

    /**
     * Returns the current timeout delay in seconds till router role transition (upgrade from REED or downgrade to
     * REED).
     *
     * @returns The timeout in seconds till router role transition, or zero if not pending role transition.
     */
    uint8_t GetRouterRoleTransitionTimeout(void) const { return mRouterRoleTransition.GetTimeout(); }

    /**
     * Returns the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_UPGRADE_THRESHOLD value.
     */
    uint8_t GetRouterUpgradeThreshold(void) const { return mRouterUpgradeThreshold; }

    /**
     * Sets the ROUTER_UPGRADE_THRESHOLD value.
     *
     * @param[in]  aThreshold  The ROUTER_UPGRADE_THRESHOLD value.
     */
    void SetRouterUpgradeThreshold(uint8_t aThreshold) { mRouterUpgradeThreshold = aThreshold; }

    /**
     * Returns the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @returns The ROUTER_DOWNGRADE_THRESHOLD value.
     */
    uint8_t GetRouterDowngradeThreshold(void) const { return mRouterDowngradeThreshold; }

    /**
     * Sets the ROUTER_DOWNGRADE_THRESHOLD value.
     *
     * @param[in]  aThreshold  The ROUTER_DOWNGRADE_THRESHOLD value.
     */
    void SetRouterDowngradeThreshold(uint8_t aThreshold) { mRouterDowngradeThreshold = aThreshold; }

    /**
     * Returns the MLE_CHILD_ROUTER_LINKS value.
     *
     * @returns The MLE_CHILD_ROUTER_LINKS value.
     */
    uint8_t GetChildRouterLinks(void) const { return mChildRouterLinks; }

    /**
     * Sets the MLE_CHILD_ROUTER_LINKS value.
     *
     * @param[in]  aChildRouterLinks  The MLE_CHILD_ROUTER_LINKS value.
     *
     * @retval kErrorNone          Successfully set the value.
     * @retval kErrorInvalidState  Thread protocols are enabled.
     */
    Error SetChildRouterLinks(uint8_t aChildRouterLinks);

    /**
     * Returns if the REED is expected to become Router soon.
     *
     * @retval TRUE   If the REED is going to become a Router soon.
     * @retval FALSE  If the REED is not going to become a Router soon.
     */
    bool WillBecomeRouterSoon(void) const;

    /**
     * Removes a link to a neighbor.
     *
     * @param[in]  aNeighbor  A reference to the neighbor object.
     */
    void RemoveNeighbor(Neighbor &aNeighbor);

    /**
     * Invalidates a direct link to a neighboring router (due to failed link-layer acks).
     *
     * @param[in]  aRouter  A reference to the router object.
     */
    void RemoveRouterLink(Router &aRouter);

    /**
     * Indicates whether or not the given Thread partition attributes are preferred.
     *
     * @param[in]  aSingletonA   Whether or not the Thread Partition A has a single router.
     * @param[in]  aLeaderDataA  A reference to Thread Partition A's Leader Data.
     * @param[in]  aSingletonB   Whether or not the Thread Partition B has a single router.
     * @param[in]  aLeaderDataB  A reference to Thread Partition B's Leader Data.
     *
     * @retval 1   If partition A is preferred.
     * @retval 0   If partition A and B have equal preference.
     * @retval -1  If partition B is preferred.
     */
    static int ComparePartitions(bool              aSingletonA,
                                 const LeaderData &aLeaderDataA,
                                 bool              aSingletonB,
                                 const LeaderData &aLeaderDataB);

    /**
     * Fills an ConnectivityTlv.
     *
     * @param[out]  aTlv  A reference to the tlv to be filled.
     */
    void FillConnectivityTlv(ConnectivityTlv &aTlv);

    /**
     * Schedule tx of MLE Advertisement message (unicast) to the given neighboring router after a random delay.
     *
     * @param[in] aRouter  The router to send the Advertisement to.
     *
     */
    void ScheduleUnicastAdvertisementTo(const Router &aRouter);

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    /**
     * Sets steering data out of band
     *
     * @param[in]  aExtAddress  Value used to set steering data
     *                          All zeros clears steering data
     *                          All 0xFFs sets steering data to 0xFF
     *                          Anything else is used to compute the bloom filter
     */
    void SetSteeringData(const Mac::ExtAddress *aExtAddress);
#endif

    /**
     * Gets the assigned parent priority.
     *
     * @returns The assigned parent priority value, -2 means not assigned.
     */
    int8_t GetAssignParentPriority(void) const { return mParentPriority; }

    /**
     * Sets the parent priority.
     *
     * @param[in]  aParentPriority  The parent priority value.
     *
     * @retval kErrorNone           Successfully set the parent priority.
     * @retval kErrorInvalidArgs    If the parent priority value is not among 1, 0, -1 and -2.
     */
    Error SetAssignParentPriority(int8_t aParentPriority);

    /**
     * Gets the longest MLE Timeout TLV for all active MTD children.
     *
     * @param[out]  aTimeout  A reference to where the information is placed.
     *
     * @retval kErrorNone           Successfully get the max child timeout
     * @retval kErrorInvalidState   Not an active router
     * @retval kErrorNotFound       NO MTD child
     */
    Error GetMaxChildTimeout(uint32_t &aTimeout) const;

    /**
     * Sets the callback that is called when processing an MLE Discovery Request message.
     *
     * @param[in]  aCallback A pointer to a function that is called to deliver MLE Discovery Request data.
     * @param[in]  aContext  A pointer to application-specific context.
     */
    void SetDiscoveryRequestCallback(otThreadDiscoveryRequestCallback aCallback, void *aContext)
    {
        mDiscoveryRequestCallback.Set(aCallback, aContext);
    }

    /**
     * Resets the MLE Advertisement Trickle timer interval.
     */
    void ResetAdvertiseInterval(void);

    /**
     * Updates the MLE Advertisement Trickle timer max interval (if timer is running).
     *
     * This is called when there is change in router table.
     */
    void UpdateAdvertiseInterval(void);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Generates an MLE Time Synchronization message.
     *
     * @retval kErrorNone     Successfully sent an MLE Time Synchronization message.
     * @retval kErrorNoBufs   Insufficient buffers to generate the MLE Time Synchronization message.
     */
    Error SendTimeSync(void);
#endif

    /**
     * Gets the maximum number of IP addresses that each MTD child may register with this device as parent.
     *
     * @returns The maximum number of IP addresses that each MTD child may register with this device as parent.
     */
    uint8_t GetMaxChildIpAddresses(void) const;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

    /**
     * Sets/restores the maximum number of IP addresses that each MTD child may register with this
     * device as parent.
     *
     * @param[in]  aMaxIpAddresses  The maximum number of IP addresses that each MTD child may register with this
     *                              device as parent. 0 to clear the setting and restore the default.
     *
     * @retval kErrorNone           Successfully set/cleared the number.
     * @retval kErrorInvalidArgs    If exceeds the allowed maximum number.
     */
    Error SetMaxChildIpAddresses(uint8_t aMaxIpAddresses);

    /**
     * Sets whether the device was commissioned using CCM.
     *
     * @param[in]  aEnabled  TRUE if the device was commissioned using CCM, FALSE otherwise.
     */
    void SetCcmEnabled(bool aEnabled) { mCcmEnabled = aEnabled; }

    /**
     * Sets whether the Security Policy TLV version-threshold for routing (VR field) is enabled.
     *
     * @param[in]  aEnabled  TRUE to enable Security Policy TLV version-threshold for routing, FALSE otherwise.
     */
    void SetThreadVersionCheckEnabled(bool aEnabled) { mThreadVersionCheckEnabled = aEnabled; }

    /**
     * Gets the current Interval Max value used by Advertisement trickle timer.
     *
     * @returns The Interval Max of Advertisement trickle timer in milliseconds.
     */
    uint32_t GetAdvertisementTrickleIntervalMax(void) const { return mAdvertiseTrickleTimer.GetIntervalMax(); }

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

#endif // OPENTHREAD_FTD

private:
    //------------------------------------------------------------------------------------------------------------------
    // Constants

    // All time intervals are in milliseconds
    static constexpr uint32_t kParentRequestRouterTimeout    = 750;  // Wait time after tx of Parent Req to routers
    static constexpr uint32_t kParentRequestReedTimeout      = 1250; // Wait timer after tx of Parent Req to REEDs
    static constexpr uint32_t kParentRequestDuplicateTimeout = 700;  // Min time to detect duplicate Parent Req rx
    static constexpr uint32_t kChildIdResponseTimeout        = 1250; // Wait time to receive Child ID Response
    static constexpr uint32_t kAttachStartJitter             = 50;   // Max jitter time added to start of attach
    static constexpr uint32_t kAnnounceProcessTimeout        = 250;  // Delay after Announce rx before processing
    static constexpr uint32_t kAnnounceTimeout               = 1400; // Total timeout for sending Announce messages
    static constexpr uint16_t kMinAnnounceDelay              = 80;   // Min delay between Announcement messages
    static constexpr uint32_t kParentResponseMaxDelayRouters = 500;  // Max response delay for Parent Req to routers
    static constexpr uint32_t kParentResponseMaxDelayAll     = 1000; // Max response delay for Parent Req to all
    static constexpr uint32_t kChildUpdateRequestDelay       = 100;  // Delay for aggregating Child Update Req
    static constexpr uint32_t kMaxLinkRequestDelayOnRouter   = 1000; // Max delay to tx Link Request on Adv rx
    static constexpr uint32_t kMinLinkRequestDelayOnChild    = 1500; // Min delay to tx Link Request on Adv rx (child)
    static constexpr uint32_t kMaxLinkRequestDelayOnChild    = 3000; // Max delay to tx Link Request on Adv rx (child)
    static constexpr uint32_t kMaxLinkAcceptDelay            = 1000; // Max delay to tx Link Accept for multicast Req
    static constexpr uint32_t kChildIdRequestTimeout         = 5000; // Max delay to rx a Child ID Req after Parent Res
    static constexpr uint32_t kLinkRequestTimeout            = 2000; // Max delay to rx a Link Accept
    static constexpr uint32_t kUnicastRetxDelay              = 1000; // Base delay for MLE unicast retx
    static constexpr uint32_t kMulticastRetxDelay            = 5000; // Base delay for MLE multicast retx
    static constexpr uint32_t kMulticastRetxDelayMin         = kMulticastRetxDelay * 9 / 10;  // 0.9 * base delay
    static constexpr uint32_t kMulticastRetxDelayMax         = kMulticastRetxDelay * 11 / 10; // 1.1 * base delay
    static constexpr uint32_t kAnnounceBackoffForPendingDataset = 60000; // Max delay left to block Announce processing.

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

    static constexpr uint8_t  kMleHopLimit                   = 255;
    static constexpr uint8_t  kMleSecurityTagSize            = 4;
    static constexpr uint32_t kDefaultStoreFrameCounterAhead = OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    static constexpr uint8_t  kMaxIpAddressesToRegister      = OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER;
    static constexpr uint32_t kDefaultChildTimeout           = OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT;
    static constexpr uint32_t kDefaultCslTimeout             = OPENTHREAD_CONFIG_CSL_TIMEOUT;

#if OPENTHREAD_FTD
    // Advertisement trickle timer constants - all times are in milliseconds.
    static constexpr uint32_t kAdvIntervalMin                = 1000;  // I_MIN
    static constexpr uint32_t kAdvIntervalNeighborMultiplier = 4000;  // Multiplier for I_MAX per router neighbor
    static constexpr uint32_t kAdvIntervalMaxLowerBound      = 12000; // Lower bound for I_MAX
    static constexpr uint32_t kAdvIntervalMaxUpperBound      = 32000; // Upper bound for I_MAX
    static constexpr uint32_t kReedAdvIntervalMin            = 570000;
    static constexpr uint32_t kReedAdvIntervalMax            = 630000;
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    static constexpr uint32_t kAdvIntervalMaxLogRoutes = 5000;
#endif

    static constexpr uint32_t kMaxUnicastAdvertisementDelay  = 1000;   // Max random delay for unciast Adv tx
    static constexpr uint32_t kMaxNeighborAge                = 100000; // Max neighbor age on router (in msec)
    static constexpr uint32_t kMaxNeighborAgeOnChild         = 150000; // Max neighbor age on FTD child (in msec)
    static constexpr uint32_t kMaxLeaderToRouterTimeout      = 90000;  // (in msec)
    static constexpr uint8_t  kMinDowngradeNeighbors         = 7;
    static constexpr uint8_t  kNetworkIdTimeout              = 120; // (in sec)
    static constexpr uint8_t  kRouterSelectionJitter         = 120; // (in sec)
    static constexpr uint8_t  kRouterDowngradeThreshold      = 23;
    static constexpr uint8_t  kRouterUpgradeThreshold        = 16;
    static constexpr uint16_t kDiscoveryMaxJitter            = 250; // Max jitter delay Discovery Responses (in msec).
    static constexpr uint16_t kUnsolicitedDataResponseJitter = 500; // Max delay for unsol Data Response (in msec).
    static constexpr uint8_t  kLeaderDowngradeExtraDelay     = 10;  // Extra delay to downgrade leader (in sec).
    static constexpr uint8_t  kDefaultLeaderWeight           = 64;
    static constexpr uint8_t  kAlternateRloc16Timeout        = 8; // Time to use alternate RLOC16 (in sec).

    // Threshold to accept a router upgrade request with reason
    // `kBorderRouterRequest` (number of BRs acting as router in
    // Network Data).
    static constexpr uint8_t kRouterUpgradeBorderRouterRequestThreshold = 2;

    static constexpr uint8_t kLinkRequestMinMargin    = OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN;
    static constexpr uint8_t kPartitionMergeMinMargin = OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN;
    static constexpr uint8_t kChildRouterLinks        = OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS;
    static constexpr uint8_t kMaxChildIpAddresses     = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD;

    // Constants for gradual router link establishment (on FTD child)
    struct GradualChildRouterLink
    {
        static constexpr uint8_t  kExtraChildRouterLinks   = OPENTHREAD_CONFIG_MLE_EXTRA_CHILD_ROUTER_LINKS_GRADUAL;
        static constexpr uint32_t kWaitDurationAfterAttach = 300;   // in seconds (5 minutes)
        static constexpr uint32_t kMinLinkRequestDelay     = 1500;  // in msec
        static constexpr uint32_t kMaxLinkRequestDelay     = 10000; // in msec
        static constexpr uint32_t kProbabilityPercentage   = 5;     // in percent
    };

    static constexpr uint8_t kMinCriticalChildrenCount = 6;

    static constexpr uint16_t kChildSupervisionDefaultIntervalForOlderVersion =
        OPENTHREAD_CONFIG_CHILD_SUPERVISION_OLDER_VERSION_CHILD_DEFAULT_INTERVAL;

    static constexpr int8_t kParentPriorityHigh        = 1;
    static constexpr int8_t kParentPriorityMedium      = 0;
    static constexpr int8_t kParentPriorityLow         = -1;
    static constexpr int8_t kParentPriorityUnspecified = -2;

#endif // OPENTHREAD_FTD

    //------------------------------------------------------------------------------------------------------------------
    // Enumerations

    enum AttachMode : uint8_t
    {
        kAnyPartition,    // Attach to any Thread partition.
        kSamePartition,   // Attach to the same Thread partition (when losing connectivity).
        kBetterPartition, // Attach to a better (i.e. higher weight/partition id) Thread partition.
        kDowngradeToReed, // Attach to the same Thread partition during downgrade process.
        kBetterParent,    // Attach to a better parent.
        kSelectedParent,  // Attach to a selected parent.
    };

    enum AttachState : uint8_t
    {
        kAttachStateIdle,           // Not currently searching for a parent.
        kAttachStateStart,          // Starting to look for a parent.
        kAttachStateParentRequest,  // Send Parent Request (current number tracked by `mParentRequestCounter`).
        kAttachStateAnnounce,       // Send Announce messages
        kAttachStateChildIdRequest, // Sending a Child ID Request message.
    };

    enum ReattachState : uint8_t
    {
        kReattachStop,    // Reattach process is disabled or finished
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
        kToSelectedRouter,  // Parent Request to a selected router (e.g., by `ParentSearch` module).
    };

    enum ChildUpdateRequestMode : uint8_t // Used in `SendChildUpdateRequest()`
    {
        kNormalChildUpdateRequest, // Normal Child Update Request.
        kAppendChallengeTlv,       // Append Challenge TLV to Child Update Request even if currently attached.
        kAppendZeroTimeout,        // Use zero timeout when appending Timeout TLV (used for graceful detach).
        kToRestoreChildRole,       // To restore previous child role (upon restart), re-establishing link with parent.
    };

    enum SecuritySuite : uint8_t
    {
        k154Security = 0,   // Security suite value indicating that MLE message is not secured.
        kNoSecurity  = 255, // Security suite value indicating that MLE message is secured.
    };

    enum TimeoutAction : uint8_t // Used as input in `SetTimeout()` to determine whether or not to update the parent.
    {
        kSendChildUpdateToParent,
        kDoNotSendChildUpdateToParent,
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
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        kTypeLinkMetricsManagementRequest,
        kTypeLinkMetricsManagementResponse,
        kTypeLinkProbe,
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        kTypeTimeSync,
#endif
    };

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    enum WedAttachState : uint8_t{
        kWedDetached,
        kWedAttaching,
        kWedAttached,
        kWedDetaching,
    };
#endif

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
        Error AppendLinkAndMleFrameCounterTlvs(void);
        Error AppendAddress16Tlv(uint16_t aRloc16);
        Error AppendNetworkDataTlv(NetworkData::Type aType);
        Error AppendTlvRequestTlv(const uint8_t *aTlvs, uint8_t aTlvsLength);
        Error AppendLeaderDataTlv(void);
        Error AppendScanMaskTlv(uint8_t aScanMask);
        Error AppendStatusTlv(StatusTlv::Status aStatus);
        Error AppendLinkMarginTlv(uint8_t aLinkMargin);
        Error AppendVersionTlv(void);
        Error AppendAddressRegistrationTlv(AddressRegistrationMode aMode = kAppendAllAddresses);
        Error AppendSupervisionIntervalTlvIfSleepyChild(void);
        Error AppendSupervisionIntervalTlv(uint16_t aInterval);
        Error AppendXtalAccuracyTlv(void);
        Error AppendActiveTimestampTlv(void);
        Error AppendPendingTimestampTlv(void);
        Error AppendActiveAndPendingTimestampTlvs(void);
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

    private:
        Error AppendAddressRegistrationEntry(const Ip6::Address &aAddress);
        Error AppendDatasetTlv(MeshCoP::Dataset::Type aDatasetType);
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class RxMessage : public Message
    {
    public:
        bool  ContainsTlv(Tlv::Type aTlvType) const;
        Error ReadModeTlv(DeviceMode &aMode) const;
        Error ReadVersionTlv(uint16_t &aVersion) const;
        Error ReadChallengeTlv(RxChallenge &aChallenge) const;
        Error ReadResponseTlv(RxChallenge &aResponse) const;
        Error ReadAndMatchResponseTlvWith(const TxChallenge &aChallenge) const;
        Error ReadFrameCounterTlvs(uint32_t &aLinkFrameCounter, uint32_t &aMleFrameCounter) const;
        Error ReadTlvRequestTlv(TlvList &aTlvList) const;
        Error ReadLeaderDataTlv(LeaderData &aLeaderData) const;
        Error ReadAndSetNetworkDataTlv(const LeaderData &aLeaderData) const;
        Error ReadAndSaveActiveDataset(const MeshCoP::Timestamp &aActiveTimestamp) const;
        Error ReadAndSavePendingDataset(const MeshCoP::Timestamp &aPendingTimestamp) const;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        Error ReadCslClockAccuracyTlv(Mac::CslAccuracy &aCslAccuracy) const;
#endif
#if OPENTHREAD_FTD
        Error ReadRouteTlv(RouteTlv &aRouteTlv) const;
#endif

    private:
        Error ReadChallengeOrResponse(uint8_t aTlvType, RxChallenge &aRxChallenge) const;
        Error ReadAndSaveDataset(MeshCoP::Dataset::Type aDatasetType, const MeshCoP::Timestamp &aTimestamp) const;
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

#if OPENTHREAD_FTD
    struct ParentResponseInfo
    {
        Mac::ExtAddress mChildExtAddress; // The child extended address.
        RxChallenge     mRxChallenge;     // The challenge from the Parent Request.
    };

    struct LinkAcceptInfo
    {
        Mac::ExtAddress mExtAddress;       // The neighbor/router extended address.
        TlvList         mRequestedTlvList; // The requested TLVs in Link Request.
        RxChallenge     mRxChallenge;      // The challenge in Link Request.
        uint8_t         mLinkMargin;       // Link margin of the received Link Request.
    };

    struct DiscoveryResponseInfo
    {
        Mac::PanId mPanId;
#if OPENTHREAD_CONFIG_MULTI_RADIO
        Mac::RadioType mRadioType;
#endif
    };

#endif // OPENTHREAD_FTD

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleDelayedSenderTimer(void) { mDelayedSender.HandleTimer(); }

    class DelayedSender : public InstanceLocator
    {
    public:
        explicit DelayedSender(Instance &aInstance);

        void Stop(void);

        void ScheduleDataRequest(const Ip6::Address &aDestination, uint32_t aDelay);
        void ScheduleChildUpdateRequestToParent(uint32_t aDelay);
#if OPENTHREAD_FTD
        void ScheduleParentResponse(const ParentResponseInfo &aInfo, uint32_t aDelay);
        void ScheduleAdvertisement(const Ip6::Address &aDestination, uint32_t aDelay);
        void ScheduleMulticastDataResponse(uint32_t aDelay);
        void ScheduleLinkRequest(const Router &aRouter, uint32_t aDelay);
        void RemoveScheduledLinkRequest(const Router &aRouter);
        bool HasAnyScheduledLinkRequest(const Router &aRouter) const;
        void ScheduleLinkAccept(const LinkAcceptInfo &aInfo, uint32_t aDelay);
        void ScheduleDiscoveryResponse(const Ip6::Address          &aDestination,
                                       const DiscoveryResponseInfo &aInfo,
                                       uint32_t                     aDelay);
#endif
        void RemoveScheduledChildUpdateRequestToParent(void);

        void HandleTimer(void);
        void GetQueueInfo(MessageQueue::Info &aQueueInfo) const { mSchedules.GetInfo(aQueueInfo); }

    private:
        typedef Message Schedule;

        struct Header
        {
            void ReadFrom(const Schedule &aSchedule) { IgnoreError(aSchedule.Read(/* aOffset */ 0, *this)); }

            TimeMilli    mSendTime;
            Ip6::Address mDestination;
            MessageType  mMessageType;
        };

        void AddSchedule(MessageType         aMessageType,
                         const Ip6::Address &aDestination,
                         uint32_t            aDelay,
                         const void         *aInfo,
                         uint16_t            aInfoSize);
        void Execute(const Schedule &aSchedule);
        bool HasMatchingSchedule(MessageType aMessageType, const Ip6::Address &aDestination) const;
        void RemoveMatchingSchedules(MessageType aMessageType, const Ip6::Address &aDestination);

        static bool Match(const Schedule &aSchedule, MessageType aMessageType, const Ip6::Address &aDestination);

        using DelayTimer = TimerMilliIn<Mle, &Mle::HandleDelayedSenderTimer>;

        MessageQueue mSchedules;
        DelayTimer   mTimer;
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
        static constexpr uint16_t kNotInUse = kInvalidRloc16;

        ServiceAloc(void);

        bool     IsInUse(void) const { return GetAloc16() != kNotInUse; }
        void     MarkAsNotInUse(void) { SetAloc16(kNotInUse); }
        uint16_t GetAloc16(void) const { return GetAddress().GetIid().GetLocator(); }
        void     SetAloc16(uint16_t aAloc16) { GetAddress().GetIid().SetLocator(aAloc16); }
    };
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleRoleRestorerTimer(void) { mPrevRoleRestorer.HandleTimer(); }

    class PrevRoleRestorer : public InstanceLocator
    {
        // Attempts to restore the device's previously saved role
        // (child/router/leader) after an MLE restart (e.g., after a
        // device reboot).
        //
        // If the previous role was router/leader, it sends multicast
        // Link Requests. If the role was child, it sends Child
        // Update Requests to the parent. It manages message
        // retransmissions and stops the restoration attempt if the
        // maximum number of attempts is exhausted, setting the
        // device to a detached state.
        //
        // To prevent synchronized transmissions when multiple devices
        // reboot at once, it adds a random delay (up to 25 ms)
        // before sending the first message.

    public:
        PrevRoleRestorer(Instance &aInstance);

        Error Start(void);
        void  Stop(void);
        bool  IsRestoringChildRole(void) const { return mState == kRestoringChildRole; }
        bool  IsRestoringRouterOrLeaderRole(void) const { return mState == kRestoringRouterOrLeaderRole; }
        void  HandleTimer(void);

#if OPENTHREAD_FTD
        void               GenerateRandomChallenge(void) { mChallenge.GenerateRandom(); }
        const TxChallenge &GetChallenge(void) const { return mChallenge; }
#endif

    private:
        static constexpr uint32_t kMaxStartDelay                = 25;
        static constexpr uint8_t  kMaxChildUpdatesToRestoreRole = kMaxChildKeepAliveAttempts;
        static constexpr uint32_t kChildUpdateRetxDelay         = kUnicastRetxDelay; /// 1000 msec
        static constexpr uint16_t kRetxJitter                   = 5;

        enum State : uint8_t
        {
            kIdle,
            kRestoringChildRole,
            kRestoringRouterOrLeaderRole,
        };

        void SetState(State aState);
        void SendChildUpdate(void);
#if OPENTHREAD_FTD
        void DetermineMaxLinkRequestAttempts(void);
        void SendMulticastLinkRequest(void);
#endif

        using DelayTimer = TimerMilliIn<Mle, &Mle::HandleRoleRestorerTimer>;

        State      mState;
        uint8_t    mAttempts;
        DelayTimer mTimer;
#if OPENTHREAD_FTD
        TxChallenge mChallenge;
#endif
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleDetacherTimer(void) { mDetacher.HandleTimer(); }

    class Detacher : public InstanceLocator
    {
        // Manages graceful detach process.

    public:
        explicit Detacher(Instance &aInstance);

        Error Detach(DetachCallback aCallback, void *aContext);
        void  HandleTimer(void);
        Error HandleChildUpdateResponse(uint32_t aTimeout);
        void  HandleStop(void);

    private:
        static constexpr uint32_t kTimeout = 1000;

        enum State : uint8_t
        {
            kIdle,
            kDetaching,
        };

        using DetachTimer = TimerMilliIn<Mle, &Mle::HandleDetacherTimer>;

        State                    mState;
        Callback<DetachCallback> mCallback;
        DetachTimer              mTimer;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleRetxTrackerTimer(void) { mRetxTracker.HandleTimer(); }

    class RetxTracker : public InstanceLocator
    {
        // Manages retransmissions of Child Update Request and Data
        // Request messages from a child to its parent. It also
        // handles periodic Child Update transmissions, as a
        // keep-alive on a rx-on (non-sleepy) child.

    public:
        explicit RetxTracker(Instance &aInstance);

        void Stop(void);
        void UpdateOnRoleChangeToChild(void);
        void UpdateOnChildUpdateRequestTx(void);
        void UpdateOnChildUpdateResponseRx(void);
        void UpdateOnDataRequestTx(void);
        void UpdateOnDataResponseRx(void);
        bool IsWaitingForDataResponse(void) const { return mDataRequest.mState == kWaitingForResponse; }
        void HandleTimer(void);

    private:
        static constexpr uint8_t  kMaxAttempts = kMaxChildKeepAliveAttempts;
        static constexpr uint32_t kRetxDelay   = kUnicastRetxDelay; /// 1000 msec
        static constexpr uint16_t kRetxJitter  = 5;

        enum State : uint8_t
        {
            kIdle,               // No pending tx
            kWaitingForResponse, // Message sent, waiting to receive response
            kSendingKeepAlive,   // Only applicable for `mChildUpdate` - keep alive
        };

        struct RetryInfo
        {
            void  Reset(void);
            void  IncrementAttempts(void);
            void  SetNextTxTime(uint32_t aDelay, uint16_t aJitter);
            void  Schedule(TimerMilli &aTimer) const;
            bool  ShouldSend(TimeMilli aNow) const;
            Error DetachIfMaxAttemptsReached(Mle &aMle) const;

            State     mState;
            uint8_t   mAttempts;
            TimeMilli mNextTxTime;
        };

        using RetxTimer = TimerMilliIn<Mle, &Mle::HandleRetxTrackerTimer>;

        void DetermineKeepAliveChildUpdateTxTime(void);
        void ScheduleTimer(void);

        RetryInfo mChildUpdate;
        RetryInfo mDataRequest;
        RetxTimer mTimer;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleAnnounceHandlerTimer(void) { mAnnounceHandler.HandleTimer(); }

    class AnnounceHandler : public InstanceLocator
    {
        // Handles received Announce messages with a newer timestamp on
        // a different channel and/or PAN ID. It may delay processing
        // to collect and handle subsequent Announce messages.
        //
        // This class also manages an 'announce attach' process, where
        // the device tries to attach using the parameters (channel and
        // PAN ID) from a processed Announce message.
        //
        // If the 'announce attach' is successful, this class handles
        // sending an Announce on the old channel to inform other
        // devices. This is done immediately after attaching as a child
        // or after an attempt to transition to the router role is
        // complete (whether successful or not).
        //
        // If the 'announce attach' fails, the class ensures the channel
        // and PAN ID are restored to their original values.

    public:
        explicit AnnounceHandler(Instance &aInstance);

        void Stop(void);
        void HandleAnnounce(RxInfo &aRxInfo);
        bool IsAnnounceAttaching(void) const { return mState == kStateAnnounceAttaching; }
        void HandleAnnounceAttachSuccess(void);
        void HandleAnnounceAttachFailure(void);
#if OPENTHREAD_FTD
        void HandleRouterRoleTransitionAttemptDone(void) { InformPreviousChannel(); }
#endif
        void HandleTimer(void);

    private:
        enum State : uint8_t
        {
            kStateIdle,
            kStateToAnnounceAttach,
            kStateAnnounceAttaching,
            kStateToInformPreviousChannel,
        };

        void StartAnnounceAttach(void);
        void InformPreviousChannel(void);

        using AnnouceTimer = TimerMilliIn<Mle, &Mle::HandleAnnounceHandlerTimer>;

        State        mState;
        uint8_t      mAlternateChannel;
        uint16_t     mAlternatePanId;
        uint64_t     mAlternateTimestamp;
        AnnouceTimer mTimer;
    };

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    void HandleParentSearchTimer(void) { mParentSearch.HandleTimer(); }

    class ParentSearch : public InstanceLocator
    {
    public:
        explicit ParentSearch(Instance &aInstance)
            : InstanceLocator(aInstance)
            , mEnabled(false)
            , mIsInBackoff(false)
            , mBackoffWasCanceled(false)
            , mRecentlyDetached(false)
            , mBackoffCancelTime(0)
            , mTimer(aInstance)
        {
        }

        void SetEnabled(bool aEnabled);
        bool IsEnabled(void) const { return mEnabled; }
        void UpdateState(void);
        void SetRecentlyDetached(void) { mRecentlyDetached = true; }
        void HandleTimer(void);
#if OPENTHREAD_FTD
        const Neighbor &GetSelectedParent(void) const { return *mSelectedParent; }
#endif

    private:
        // All timer intervals are converted to milliseconds.
        static constexpr uint32_t kCheckInterval   = (OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL * 1000u);
        static constexpr uint32_t kBackoffInterval = (OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL * 1000u);
        static constexpr uint32_t kJitterInterval  = (15 * 1000u);
        static constexpr int8_t   kRssThreshold    = OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD;

#if OPENTHREAD_FTD
        static constexpr int8_t kRssMarginOverParent = OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_MARGIN;

        Error SelectBetterParent(void);
        void  CompareAndUpdateSelectedParent(Router &aRouter);
#endif
        void StartTimer(void);

        using SearchTimer = TimerMilliIn<Mle, &Mle::HandleParentSearchTimer>;

        bool        mEnabled : 1;
        bool        mIsInBackoff : 1;
        bool        mBackoffWasCanceled : 1;
        bool        mRecentlyDetached : 1;
        TimeMilli   mBackoffCancelTime;
        SearchTimer mTimer;
#if OPENTHREAD_FTD
        Router *mSelectedParent;
#endif
    };
#endif // OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_FTD

    class RouterRoleTransition
    {
    public:
        RouterRoleTransition(void);

        bool    IsPending(void) const { return (mTimeout != 0); }
        void    StartTimeout(void);
        void    StopTimeout(void) { mTimeout = 0; }
        void    IncreaseTimeout(uint8_t aIncrement) { mTimeout += aIncrement; }
        uint8_t GetTimeout(void) const { return mTimeout; }
        bool    HandleTimeTick(void);
        uint8_t GetJitter(void) const { return mJitter; }
        void    SetJitter(uint8_t aJitter) { mJitter = aJitter; }

    private:
        uint8_t mTimeout;
        uint8_t mJitter;
    };

#endif

    //------------------------------------------------------------------------------------------------------------------
    // Methods

    Error      Start(StartMode aMode);
    void       Stop(StopMode aMode);
    Error      RestorePrevRole(void);
    TxMessage *NewMleMessage(Command aCommand);
    void       SetRole(DeviceRole aRole);
    void       Attach(AttachMode aMode);
    void       SetAttachState(AttachState aState);
    void       InitNeighbor(Neighbor &aNeighbor, const RxInfo &aRxInfo);
    void       ClearParentCandidate(void) { mParentCandidate.Clear(); }
    Error      SendDataRequestToParent(void);
    Error      SendDataRequest(const Ip6::Address &aDestination);
    void       HandleNotifierEvents(Events aEvents);
    void       HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void       ReestablishLinkWithNeighbor(Neighbor &aNeighbor);
    Error      SendChildUpdateRequestToParent(ChildUpdateRequestMode aMode);
    Error      SendChildUpdateResponse(const TlvList      &aTlvList,
                                       const RxChallenge  &aChallenge,
                                       const Ip6::Address &aDestination);
    void       SetRloc16(uint16_t aRloc16);
    void       SetStateDetached(void);
    void       SetStateChild(uint16_t aRloc16);
    void       SetLeaderData(uint32_t aPartitionId, uint8_t aWeighting, uint8_t aLeaderRouterId);
    void       SetLeaderData(const LeaderData &aLeaderData);
    void       SetTimeout(uint32_t aTimeout, TimeoutAction aAction);
    uint32_t   GenerateRandomDelay(uint32_t aMaxDelay) const;
    void       InformPreviousChannel(void);
    void       ScheduleMessageTransmissionTimer(void);
    void       HandleAttachTimer(void);
    void       ProcessKeySequence(RxInfo &aRxInfo);
    void       HandleAdvertisement(RxInfo &aRxInfo);
    void       HandleChildIdResponse(RxInfo &aRxInfo);
    void       HandleChildUpdateRequest(RxInfo &aRxInfo);
    void       HandleChildUpdateRequestOnChild(RxInfo &aRxInfo);
    void       HandleChildUpdateResponse(RxInfo &aRxInfo);
    void       HandleChildUpdateResponseOnChild(RxInfo &aRxInfo);
    void       HandleDataResponse(RxInfo &aRxInfo);
    void       HandleParentResponse(RxInfo &aRxInfo);
    Error      HandleLeaderData(RxInfo &aRxInfo);
    bool       HasUnregisteredAddress(void);
    uint32_t   GetAttachStartDelay(void) const;
    void       SendParentRequest(ParentRequestType aType);
    Error      SendChildIdRequest(void);
    void       HandleChildIdRequestTxDone(const Message &aMessage);
    Error      GetNextAnnounceChannel(uint8_t &aChannel) const;
    bool       HasMoreChannelsToAnnounce(void) const;
    bool       PrepareAnnounceState(void);
    void       SendAnnounce(uint8_t aChannel, AnnounceMode aMode);
    void       SendAnnounce(uint8_t aChannel, const Ip6::Address &aDestination, AnnounceMode aMode = kNormalAnnounce);
    uint32_t   Reattach(void);
    bool       HasAcceptableParentCandidate(void) const;
    Error      DetermineParentRequestType(ParentRequestType &aType) const;
    bool       IsBetterParent(uint16_t                aRloc16,
                              uint8_t                 aTwoWayLinkMargin,
                              const ConnectivityTlv  &aConnectivityTlv,
                              uint16_t                aVersion,
                              const Mac::CslAccuracy &aCslAccuracy);
    bool       IsNetworkDataNewer(const LeaderData &aLeaderData);
    Error      ProcessMessageSecurity(Crypto::AesCcm::Mode    aMode,
                                      Message                &aMessage,
                                      const Ip6::MessageInfo &aMessageInfo,
                                      uint16_t                aCmdOffset,
                                      const SecurityHeader   &aHeader);

    static void HandleChildIdRequestTxDone(const otMessage *aMessage, otError aError, void *aContext);

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
    void InformPreviousParent(void);
#endif

    void UpdateRoleTimeCounters(DeviceRole aRole);

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ServiceAloc *FindInServiceAlocs(uint16_t aAloc16);
    void         UpdateServiceAlocs(void);
#endif

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    void CheckTrelPeerAddrOnSecureMleRx(const Message &aMessage);
#endif

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    void HandleTimeSync(RxInfo &aRxInfo);
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
                          const LinkMetrics::Initiator::QueryInfo *aQueryInfo = nullptr);
#else
    Error       SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength);
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static void Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress);
    static void Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress, uint16_t aRloc);
#else
    static void Log(MessageAction, MessageType, const Ip6::Address &) {}
    static void Log(MessageAction, MessageType, const Ip6::Address &, uint16_t) {}
#endif

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    void HandleWedAttachTimer(void);
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

#if OPENTHREAD_FTD
    void     SetAlternateRloc16(uint16_t aRloc16);
    void     ClearAlternateRloc16(void);
    void     HandleDetachStart(void);
    void     HandleChildStart(AttachMode aMode);
    void     HandleSecurityPolicyChanged(void);
    void     HandleLinkRequest(RxInfo &aRxInfo);
    void     HandleLinkAccept(RxInfo &aRxInfo);
    void     HandleLinkAcceptAndRequest(RxInfo &aRxInfo);
    void     HandleLinkAcceptVariant(RxInfo &aRxInfo, MessageType aMessageType);
    Error    HandleAdvertisementOnFtd(RxInfo &aRxInfo, uint16_t aSourceAddress, const LeaderData &aLeaderData);
    void     HandleParentRequest(RxInfo &aRxInfo);
    void     HandleChildIdRequest(RxInfo &aRxInfo);
    void     HandleChildUpdateRequestOnParent(RxInfo &aRxInfo);
    void     HandleChildUpdateResponseOnParent(RxInfo &aRxInfo);
    void     HandleDataRequest(RxInfo &aRxInfo);
    void     HandleNetworkDataUpdateRouter(void);
    void     HandleDiscoveryRequest(RxInfo &aRxInfo);
    void     EstablishRouterLinkOnFtdChild(Router &aRouter, RxInfo &aRxInfo, uint8_t aLinkMargin);
    Error    ProcessRouteTlv(const RouteTlv &aRouteTlv, RxInfo &aRxInfo);
    Error    ReadAndProcessRouteTlvOnFtdChild(RxInfo &aRxInfo, uint8_t aParentId);
    void     StopAdvertiseTrickleTimer(void);
    uint32_t DetermineAdvertiseIntervalMax(void) const;
    Error    SendAddressSolicit(ThreadStatusTlv::Status aStatus);
    void     SendAddressSolicitResponse(const Coap::Message    &aRequest,
                                        ThreadStatusTlv::Status aResponseStatus,
                                        const Router           *aRouter,
                                        const Ip6::MessageInfo &aMessageInfo);
    void     SendAddressRelease(void);
    void     SendMulticastAdvertisement(void);
    void     SendAdvertisement(const Ip6::Address &aDestination);
    void     SendLinkRequest(Router *aRouter);
    Error    SendLinkAccept(const LinkAcceptInfo &aInfo);
    void     SendParentResponse(const ParentResponseInfo &aInfo);
    Error    SendChildIdResponse(Child &aChild);
    Error    SendChildUpdateRequestToChild(Child &aChild);
    void     SendChildUpdateResponseToChild(Child                  *aChild,
                                            const Ip6::MessageInfo &aMessageInfo,
                                            const TlvList          &aTlvList,
                                            const RxChallenge      &aChallenge);
    void     SendMulticastDataResponse(void);
    void     SendDataResponse(const Ip6::Address &aDestination,
                              const TlvList      &aTlvList,
                              const Message      *aRequestMessage = nullptr);
    Error    SendDiscoveryResponse(const Ip6::Address &aDestination, const DiscoveryResponseInfo &aInfo);
    void     SetStateRouter(uint16_t aRloc16);
    void     SetStateLeader(uint16_t aRloc16, LeaderStartMode aStartMode);
    void     SetStateRouterOrLeader(DeviceRole aRole, uint16_t aRloc16, LeaderStartMode aStartMode);
    void     StopLeader(void);
    void     SynchronizeChildNetworkData(void);
    Error    ProcessAddressRegistrationTlv(RxInfo &aRxInfo, Child &aChild);
    bool     HasNeighborWithGoodLinkQuality(void) const;
    void     HandlePartitionChange(void);
    void     SetChildStateToValid(Child &aChild);
    bool     HasChildren(void);
    void     RemoveChildren(void);
    bool     ShouldDowngrade(uint8_t aNeighborId, const RouteTlv &aRouteTlv) const;
    bool     NeighborHasComparableConnectivity(const RouteTlv &aRouteTlv, uint8_t aNeighborId) const;
    void     HandleAdvertiseTrickleTimer(void);
    void     HandleAddressSolicitResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult);
    void     HandleTimeTick(void);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    void SignalDuaAddressEvent(const Child &aChild, const Ip6::Address &aOldDua) const;
#endif

    static bool IsMessageMleSubType(const Message &aMessage);
    static bool IsMessageChildUpdateRequest(const Message &aMessage);
    static void HandleAdvertiseTrickleTimer(TrickleTimer &aTimer);
    static void HandleAddressSolicitResponse(void                *aContext,
                                             otMessage           *aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             otError              aResult);
#endif // OPENTHREAD_FTD

    //------------------------------------------------------------------------------------------------------------------
    // Variables

    using AttachTimer = TimerMilliIn<Mle, &Mle::HandleAttachTimer>;
    using MleSocket   = Ip6::Udp::SocketIn<Mle, &Mle::HandleUdpReceive>;
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    using WedAttachTimer = TimerMicroIn<Mle, &Mle::HandleWedAttachTimer>;
#endif

    static const otMeshLocalPrefix kMeshLocalPrefixInit;

    bool mRetrieveNewNetworkData : 1;
    bool mRequestRouteTlv : 1;
    bool mHasRestored : 1;
    bool mReceivedResponseFromParent : 1;
    bool mInitiallyAttachedAsSleepy : 1;

    DeviceRole              mRole;
    DeviceRole              mLastSavedRole;
    DeviceMode              mDeviceMode;
    AttachState             mAttachState;
    ReattachState           mReattachState;
    AttachMode              mAttachMode;
    AddressRegistrationMode mAddressRegistrationMode;

    uint8_t  mParentRequestCounter;
    uint8_t  mAnnounceChannel;
    uint16_t mRloc16;
    uint16_t mPreviousParentRloc;
    uint16_t mAttachCounter;
    uint16_t mAnnounceDelay;
    uint32_t mStoreFrameCounterAhead;
    uint32_t mTimeout;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    uint32_t mCslTimeout;
#endif
    uint32_t mLastAttachTime;
    uint64_t mLastUpdatedTimestamp;

    LeaderData       mLeaderData;
    Parent           mParent;
    NeighborTable    mNeighborTable;
    DelayedSender    mDelayedSender;
    TxChallenge      mParentRequestChallenge;
    ParentCandidate  mParentCandidate;
    MleSocket        mSocket;
    Counters         mCounters;
    PrevRoleRestorer mPrevRoleRestorer;
    Detacher         mDetacher;
    RetxTracker      mRetxTracker;
    AnnounceHandler  mAnnounceHandler;
#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    ParentSearch mParentSearch;
#endif
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ServiceAloc mServiceAlocs[kMaxServiceAlocs];
#endif
#if OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE
    Callback<otThreadParentResponseCallback> mParentResponseCallback;
#endif
    AttachTimer                  mAttachTimer;
    Ip6::NetworkPrefix           mMeshLocalPrefix;
    Ip6::Netif::UnicastAddress   mLinkLocalAddress;
    Ip6::Netif::UnicastAddress   mMeshLocalEid;
    Ip6::Netif::UnicastAddress   mMeshLocalRloc;
    Ip6::Netif::MulticastAddress mLinkLocalAllThreadNodes;
    Ip6::Netif::MulticastAddress mRealmLocalAllThreadNodes;

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    WakeupTxScheduler        mWakeupTxScheduler;
    WedAttachState           mWedAttachState;
    WedAttachTimer           mWedAttachTimer;
    Callback<WakeupCallback> mWakeupCallback;
#endif

#if OPENTHREAD_FTD

    bool mRouterEligible : 1;
    bool mAddressSolicitPending : 1;
    bool mAddressSolicitRejected : 1;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    bool mCcmEnabled : 1;
    bool mThreadVersionCheckEnabled : 1;
#endif

    uint8_t mRouterId;
    uint8_t mPreviousRouterId;
    uint8_t mNetworkIdTimeout;
    uint8_t mRouterUpgradeThreshold;
    uint8_t mRouterDowngradeThreshold;
    uint8_t mLeaderWeight;
    uint8_t mPreviousPartitionRouterIdSequence;
    uint8_t mPreviousPartitionIdTimeout;
    uint8_t mChildRouterLinks;
    uint8_t mAlternateRloc16Timeout;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t mMaxChildIpAddresses;
#endif
    int8_t   mParentPriority;
    uint32_t mPreviousPartitionIdRouter;
    uint32_t mPreviousPartitionId;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint32_t mPreferredLeaderPartitionId;
#endif

    TrickleTimer               mAdvertiseTrickleTimer;
    ChildTable                 mChildTable;
    RouterTable                mRouterTable;
    RouterRoleTransition       mRouterRoleTransition;
    Ip6::Netif::UnicastAddress mLeaderAloc;
#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
    DeviceProperties mDeviceProperties;
#endif
#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    MeshCoP::SteeringData mSteeringData;
#endif
    Callback<otThreadDiscoveryRequestCallback> mDiscoveryRequestCallback;

#endif // OPENTHREAD_FTD
};

#if OPENTHREAD_FTD
DeclareTmfHandler(Mle, kUriAddressSolicit);
DeclareTmfHandler(Mle, kUriAddressRelease);
#endif

} // namespace Mle

/**
 * @}
 */

} // namespace ot

#endif // MLE_HPP_
