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
#include "thread/link_metrics.hpp"
#include "thread/link_metrics_tlvs.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/mle_types.hpp"
#include "thread/neighbor_table.hpp"
#include "thread/network_data_types.hpp"
#include "thread/topology.hpp"

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

/**
 * This class implements MLE functionality required by the Thread EndDevices, Router, and Leader roles.
 *
 */
class Mle : public InstanceLocator, private NonCopyable
{
    friend class DiscoverScanner;
    friend class ot::Notifier;
    friend class ot::SupervisionListener;
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    friend class ot::LinkMetrics::Initiator;
#endif

public:
    /**
     * This constructor initializes the MLE object.
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
    const Ip6::NetworkPrefix &GetMeshLocalPrefix(void) const { return mMeshLocal16.GetAddress().GetPrefix(); }

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
     * Applies the Mesh Local Prefix.
     *
     */
    void ApplyMeshLocalPrefix(void);

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

protected:
    /**
     * MLE Command Types.
     *
     */
    enum Command : uint8_t
    {
        kCommandLinkRequest                   = 0,  ///< Link Request
        kCommandLinkAccept                    = 1,  ///< Link Accept
        kCommandLinkAcceptAndRequest          = 2,  ///< Link Accept and Reject
        kCommandLinkReject                    = 3,  ///< Link Reject
        kCommandAdvertisement                 = 4,  ///< Advertisement
        kCommandUpdate                        = 5,  ///< Update
        kCommandUpdateRequest                 = 6,  ///< Update Request
        kCommandDataRequest                   = 7,  ///< Data Request
        kCommandDataResponse                  = 8,  ///< Data Response
        kCommandParentRequest                 = 9,  ///< Parent Request
        kCommandParentResponse                = 10, ///< Parent Response
        kCommandChildIdRequest                = 11, ///< Child ID Request
        kCommandChildIdResponse               = 12, ///< Child ID Response
        kCommandChildUpdateRequest            = 13, ///< Child Update Request
        kCommandChildUpdateResponse           = 14, ///< Child Update Response
        kCommandAnnounce                      = 15, ///< Announce
        kCommandDiscoveryRequest              = 16, ///< Discovery Request
        kCommandDiscoveryResponse             = 17, ///< Discovery Response
        kCommandLinkMetricsManagementRequest  = 18, ///< Link Metrics Management Request
        kCommandLinkMetricsManagementResponse = 19, ///< Link Metrics Management Response
        kCommandLinkProbe                     = 20, ///< Link Probe
        kCommandTimeSync                      = 99, ///< Time Sync (when OPENTHREAD_CONFIG_TIME_SYNC_ENABLE enabled)
    };

    /**
     * Attach mode.
     *
     */
    enum AttachMode : uint8_t
    {
        kAnyPartition,       ///< Attach to any Thread partition.
        kSamePartition,      ///< Attach to the same Thread partition (attempt 1 when losing connectivity).
        kSamePartitionRetry, ///< Attach to the same Thread partition (attempt 2 when losing connectivity).
        kBetterPartition,    ///< Attach to a better (i.e. higher weight/partition id) Thread partition.
        kDowngradeToReed,    ///< Attach to the same Thread partition during downgrade process.
        kBetterParent,       ///< Attach to a better parent.
    };

    /**
     * States during attach (when searching for a parent).
     *
     */
    enum AttachState : uint8_t
    {
        kAttachStateIdle,            ///< Not currently searching for a parent.
        kAttachStateProcessAnnounce, ///< Waiting to process a received Announce (to switch channel/pan-id).
        kAttachStateStart,           ///< Starting to look for a parent.
        kAttachStateParentRequest,   ///< Send Parent Request (current number tracked by `mParentRequestCounter`).
        kAttachStateAnnounce,        ///< Send Announce messages
        kAttachStateChildIdRequest,  ///< Sending a Child ID Request message.
    };

    /**
     * States when reattaching network using stored dataset
     *
     */
    enum ReattachState : uint8_t
    {
        kReattachStop,    ///< Reattach process is disabled or finished
        kReattachStart,   ///< Start reattach process
        kReattachActive,  ///< Reattach using stored Active Dataset
        kReattachPending, ///< Reattach using stored Pending Dataset
    };

    static constexpr uint16_t kMleMaxResponseDelay = 1000u; ///< Max delay before responding to a multicast request.

    /**
     * This enumeration type is used in `AppendAddressRegistrationTlv()` to determine which addresses to include in the
     * appended Address Registration TLV.
     *
     */
    enum AddressRegistrationMode : uint8_t
    {
        kAppendAllAddresses,  ///< Append all addresses (unicast/multicast) in Address Registration TLV.
        kAppendMeshLocalOnly, ///< Only append the Mesh Local (ML-EID) address in Address Registration TLV.
    };

    /**
     * This enumeration represents the message actions used in `Log()` methods.
     *
     */
    enum MessageAction : uint8_t
    {
        kMessageSend,
        kMessageReceive,
        kMessageDelay,
        kMessageRemoveDelayed,
    };

    /**
     * This enumeration represents message types used in `Log()` methods.
     *
     */
    enum MessageType : uint8_t
    {
        kTypeAdvertisement,
        kTypeAnnounce,
        kTypeChildIdRequest,
        kTypeChildIdRequestShort,
        kTypeChildIdResponse,
        kTypeChildUpdateRequestOfParent,
        kTypeChildUpdateResponseOfParent,
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

    static constexpr uint8_t kMaxTlvListSize = 32; ///< Maximum number of TLVs in a `TlvList`.

    /**
     * This type represents a list of TLVs (array of TLV types).
     *
     */
    class TlvList : public Array<uint8_t, kMaxTlvListSize>
    {
    public:
        /**
         * This constructor initializes the `TlvList` as empty.
         *
         */
        TlvList(void) = default;

        /**
         * Checks if a given TLV type is not already present in the list and adds it in the list.
         *
         * If the list is full, this method logs it as a warning.
         *
         * @param[in] aTlvType   The TLV type to add to the list.
         *
         */
        void Add(uint8_t aTlvType);

        /**
         * Adds elements from a given list to this TLV list (if not already present in the list).
         *
         * @param[in] aTlvList   The TLV list to add elements from.
         *
         */
        void AddElementsFrom(const TlvList &aTlvList);
    };

    /**
     * This type represents a Challenge (or Response) data.
     *
     */
    struct Challenge
    {
        uint8_t mBuffer[kMaxChallengeSize]; ///< Buffer containing the challenge/response byte sequence.
        uint8_t mLength;                    ///< Challenge length (in bytes).

        /**
         * Generates a cryptographically secure random sequence to populate the challenge data.
         *
         */
        void GenerateRandom(void);

        /**
         * Indicates whether the Challenge matches a given buffer.
         *
         * @param[in] aBuffer   A pointer to a buffer to compare with the Challenge.
         * @param[in] aLength   Length of @p aBuffer (in bytes).
         *
         * @retval TRUE  If the Challenge matches the given buffer.
         * @retval FALSE If the Challenge does not match the given buffer.
         *
         */
        bool Matches(const uint8_t *aBuffer, uint8_t aLength) const;

        /**
         * Indicates whether two Challenge data byte sequences are equal or not.
         *
         * @param[in] aOther   Another Challenge data to compare.
         *
         * @retval TRUE  If the two Challenges match.
         * @retval FALSE If the two Challenges do not match.
         *
         */
        bool operator==(const Challenge &aOther) const { return Matches(aOther.mBuffer, aOther.mLength); }
    };

    /**
     * This class represents an MLE Tx message.
     *
     */
    class TxMessage : public Message
    {
    public:
        /**
         * Appends a Source Address TLV to the message.
         *
         * @retval kErrorNone    Successfully appended the Source Address TLV.
         * @retval kErrorNoBufs  Insufficient buffers available to append the Source Address TLV.
         *
         */
        Error AppendSourceAddressTlv(void);

        /**
         * Appends a Mode TLV to the message.
         *
         * @param[in]  aMode     The Device Mode.
         *
         * @retval kErrorNone    Successfully appended the Mode TLV.
         * @retval kErrorNoBufs  Insufficient buffers available to append the Mode TLV.
         *
         */
        Error AppendModeTlv(DeviceMode aMode);

        /**
         * Appends a Timeout TLV to the message.
         *
         * @param[in]  aTimeout  The Timeout value.
         *
         * @retval kErrorNone    Successfully appended the Timeout TLV.
         * @retval kErrorNoBufs  Insufficient buffers available to append the Timeout TLV.
         *
         */
        Error AppendTimeoutTlv(uint32_t aTimeout);

        /**
         * Appends a Challenge TLV to the message.
         *
         * @param[in]  aChallenge        A pointer to the Challenge value.
         * @param[in]  aChallengeLength  The length of the Challenge value in bytes.
         *
         * @retval kErrorNone    Successfully appended the Challenge TLV.
         * @retval kErrorNoBufs  Insufficient buffers available to append the Challenge TLV.
         *
         */
        Error AppendChallengeTlv(const uint8_t *aChallenge, uint8_t aChallengeLength);

        /**
         * Appends a Challenge TLV to the message.
         *
         * @param[in] aChallenge A reference to the Challenge data.
         *
         * @retval kErrorNone    Successfully appended the Challenge TLV.
         * @retval kErrorNoBufs  Insufficient buffers available to append the Challenge TLV.
         *
         */
        Error AppendChallengeTlv(const Challenge &aChallenge);

        /**
         * Appends a Response TLV to the message.
         *
         * @param[in] aResponse  A reference to the Response data.
         *
         * @retval kErrorNone    Successfully appended the Response TLV.
         * @retval kErrorNoBufs  Insufficient buffers available to append the Response TLV.
         *
         */
        Error AppendResponseTlv(const Challenge &aResponse);

        /**
         * Appends a Link Frame Counter TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Link Frame Counter TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Link Frame Counter TLV.
         *
         */
        Error AppendLinkFrameCounterTlv(void);

        /**
         * Appends an MLE Frame Counter TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Frame Counter TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the MLE Frame Counter TLV.
         *
         */
        Error AppendMleFrameCounterTlv(void);

        /**
         * Appends an Address16 TLV to the message.
         *
         * @param[in]  aRloc16    The RLOC16 value.
         *
         * @retval kErrorNone     Successfully appended the Address16 TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Address16 TLV.
         *
         */
        Error AppendAddress16Tlv(uint16_t aRloc16);

        /**
         * Appends a Network Data TLV to the message.
         *
         * @param[in]  aType      The Network Data type to append, full set or stable subset.
         *
         * @retval kErrorNone     Successfully appended the Network Data TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Network Data TLV.
         *
         */
        Error AppendNetworkDataTlv(NetworkData::Type aType);

        /**
         * Appends a TLV Request TLV to the message.
         *
         * @param[in]  aTlvs        A pointer to the list of TLV types.
         * @param[in]  aTlvsLength  The number of TLV types in @p aTlvs
         *
         * @retval kErrorNone     Successfully appended the TLV Request TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the TLV Request TLV.
         *
         */
        Error AppendTlvRequestTlv(const uint8_t *aTlvs, uint8_t aTlvsLength);

        /**
         * Appends a TLV Request TLV to the message.
         *
         * @tparam kArrayLength     The TLV array length.
         *
         * @param[in]  aTlvArray    A reference to an array of TLV types of @p kArrayLength length.
         *
         * @retval kErrorNone     Successfully appended the TLV Request TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the TLV Request TLV.
         *
         */
        template <uint8_t kArrayLength> Error AppendTlvRequestTlv(const uint8_t (&aTlvArray)[kArrayLength])
        {
            return AppendTlvRequestTlv(aTlvArray, kArrayLength);
        }

        /**
         * Appends a Leader Data TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Leader Data TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Leader Data TLV.
         *
         */
        Error AppendLeaderDataTlv(void);

        /**
         * Appends a Scan Mask TLV to th message.
         *
         * @param[in]  aScanMask  The Scan Mask value.
         *
         * @retval kErrorNone     Successfully appended the Scan Mask TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Scan Mask TLV.
         *
         */
        Error AppendScanMaskTlv(uint8_t aScanMask);

        /**
         * Appends a Status TLV to the message.
         *
         * @param[in] aStatus     The Status value.
         *
         * @retval kErrorNone     Successfully appended the Status TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Status TLV.
         *
         */
        Error AppendStatusTlv(StatusTlv::Status aStatus);

        /**
         * Appends a Link Margin TLV to the message.
         *
         * @param[in] aLinkMargin The Link Margin value.
         *
         * @retval kErrorNone     Successfully appended the Link Margin TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Link Margin TLV.
         *
         */
        Error AppendLinkMarginTlv(uint8_t aLinkMargin);

        /**
         * Appends a Version TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Version TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Version TLV.
         *
         */
        Error AppendVersionTlv(void);

        /**
         * Appends an Address Registration TLV to the message.
         *
         * @param[in]  aMode      Determines which addresses to include in the TLV (see `AddressRegistrationMode`).
         *
         * @retval kErrorNone     Successfully appended the Address Registration TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Address Registration TLV.
         *
         */
        Error AppendAddressRegistrationTlv(AddressRegistrationMode aMode = kAppendAllAddresses);

        /**
         * Appends a Supervision Interval TLV to the message.
         *
         * @param[in]  aInterval  The interval value.
         *
         * @retval kErrorNone    Successfully appended the Supervision Interval TLV.
         * @retval kErrorNoBufs  Insufficient buffers available to append the Supervision Interval TLV.
         *
         */
        Error AppendSupervisionIntervalTlv(uint16_t aInterval);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        /**
         * Appends a Time Request TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Time Request TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Time Request TLV.
         *
         */
        Error AppendTimeRequestTlv(void);

        /**
         * Appends a Time Parameter TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Time Parameter TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Time Parameter TLV.
         *
         */
        Error AppendTimeParameterTlv(void);
#endif
        /**
         * Appends a XTAL Accuracy TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the XTAL Accuracy TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the XTAl Accuracy TLV.
         *
         */
        Error AppendXtalAccuracyTlv(void);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        /**
         * Appends a CSL Channel TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the CSL Channel TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the CSL Channel TLV.
         *
         */
        Error AppendCslChannelTlv(void);

        /**
         * Appends a CSL Sync Timeout TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the CSL Timeout TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the CSL Timeout TLV.
         *
         */
        Error AppendCslTimeoutTlv(void);
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        /**
         * Appends a CSL Clock Accuracy TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the CSL Accuracy TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the CSL Accuracy TLV.
         *
         */
        Error AppendCslClockAccuracyTlv(void);
#endif

        /**
         * Appends a Active Timestamp TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Active Timestamp TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Active Timestamp TLV.
         *
         */
        Error AppendActiveTimestampTlv(void);

        /**
         * Appends a Pending Timestamp TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Pending Timestamp TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Pending Timestamp TLV.
         *
         */
        Error AppendPendingTimestampTlv(void);

#if OPENTHREAD_FTD
        /**
         * Appends a Route TLV to the message.
         *
         * @param[in] aNeighbor   A pointer to the intended destination  (can be `nullptr`).
         *
         * @retval kErrorNone     Successfully appended the Route TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Route TLV.
         *
         */
        Error AppendRouteTlv(Neighbor *aNeighbor = nullptr);

        /**
         * Appends a Active Dataset TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Active Dataset TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Active Dataset TLV.
         *
         */
        Error AppendActiveDatasetTlv(void);

        /**
         * Appends a Pending Dataset TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Pending Dataset TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Pending Dataset TLV.
         *
         */
        Error AppendPendingDatasetTlv(void);

        /**
         * Appends a Connectivity TLV to the message.
         *
         * @retval kErrorNone     Successfully appended the Connectivity TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Connectivity TLV.
         *
         */
        Error AppendConnectivityTlv(void);

        /**
         * Appends a Address Registration TLV to the message with addresses from a given child.
         *
         * @param[in] aChild  The child to include its list of addresses in the Address Registration TLV.
         *
         * @retval kErrorNone     Successfully appended the Connectivity TLV.
         * @retval kErrorNoBufs   Insufficient buffers available to append the Connectivity TLV.
         *
         */
        Error AppendAddressRegistrationTlv(Child &aChild);
#endif // OPENTHREAD_FTD

        /**
         * Submits the MLE message to the UDP socket to be sent.
         *
         * @param[in]  aDestination  A reference to the IPv6 address of the destination.
         *
         * @retval kErrorNone     Successfully submitted the MLE message.
         * @retval kErrorNoBufs   Insufficient buffers to form the rest of the MLE message.
         *
         */
        Error SendTo(const Ip6::Address &aDestination);

        /**
         * Enqueues the message to be sent after a given delay.
         *
         * @param[in]  aDestination         The IPv6 address of the recipient of the message.
         * @param[in]  aDelay               The delay in milliseconds before transmission of the message.
         *
         * @retval kErrorNone     Successfully queued the message to transmit after the delay.
         * @retval kErrorNoBufs   Insufficient buffers to queue the message.
         *
         */
        Error SendAfterDelay(const Ip6::Address &aDestination, uint16_t aDelay);

    private:
        Error AppendCompressedAddressEntry(uint8_t aContextId, const Ip6::Address &aAddress);
        Error AppendAddressEntry(const Ip6::Address &aAddress);
    };

    /**
     * This class represents an MLE Rx message.
     *
     */
    class RxMessage : public Message
    {
    public:
        /**
         * Reads Challenge TLV from the message.
         *
         * @param[out] aChallenge        A reference to the Challenge data where to output the read value.
         *
         * @retval kErrorNone       Successfully read the Challenge TLV.
         * @retval kErrorNotFound   Challenge TLV was not found in the message.
         * @retval kErrorParse      Challenge TLV was found but could not be parsed.
         *
         */
        Error ReadChallengeTlv(Challenge &aChallenge) const;

        /**
         * Reads Response TLV from the message.
         *
         * @param[out] aResponse        A reference to the Response data where to output the read value.
         *
         * @retval kErrorNone       Successfully read the Response TLV.
         * @retval kErrorNotFound   Response TLV was not found in the message.
         * @retval kErrorParse      Response TLV was found but could not be parsed.
         *
         */
        Error ReadResponseTlv(Challenge &aResponse) const;

        /**
         * Reads Link and MLE Frame Counters from the message.
         *
         * Link Frame Counter TLV must be present in the message and its value is read into @p aLinkFrameCounter. If MLE
         * Frame Counter TLV is present in the message, its value is read into @p aMleFrameCounter. If the MLE Frame
         * Counter TLV is not present in the message, then @p aMleFrameCounter is set to the same value as
         * @p aLinkFrameCounter.
         *
         * @param[out] aLinkFrameCounter  A reference to an `uint32_t` to output the Link Frame Counter.
         * @param[out] aMleFrameCounter   A reference to an `uint32_t` to output the MLE Frame Counter.
         *
         * @retval kErrorNone       Successfully read the counters.
         * @retval kErrorNotFound   Link Frame Counter TLV was not found in the message.
         * @retval kErrorParse      TLVs are not well-formed.
         *
         */
        Error ReadFrameCounterTlvs(uint32_t &aLinkFrameCounter, uint32_t &aMleFrameCounter) const;

        /**
         * Reads TLV Request TLV from the message.
         *
         * @param[out] aTlvList     A reference to output the read list of requested TLVs.
         *
         * @retval kErrorNone       Successfully read the TLV.
         * @retval kErrorNotFound   TLV was not found in the message.
         * @retval kErrorParse      TLV was found but could not be parsed.
         *
         */
        Error ReadTlvRequestTlv(TlvList &aTlvList) const;

        /**
         * Reads Leader Data TLV from a message.
         *
         * @param[out] aLeaderData     A reference to output the Leader Data.
         *
         * @retval kErrorNone       Successfully read the TLV.
         * @retval kErrorNotFound   TLV was not found in the message.
         * @retval kErrorParse      TLV was found but could not be parsed.
         *
         */
        Error ReadLeaderDataTlv(LeaderData &aLeaderData) const;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        /**
         * Reads CSL Clock Accuracy TLV from a message.
         *
         * @param[out] aCslAccuracy A reference to output the CSL accuracy.
         *
         * @retval kErrorNone       Successfully read the TLV.
         * @retval kErrorNotFound   TLV was not found in the message.
         * @retval kErrorParse      TLV was found but could not be parsed.
         *
         */
        Error ReadCslClockAccuracyTlv(Mac::CslAccuracy &aCslAccuracy) const;
#endif

#if OPENTHREAD_FTD
        /**
         * Reads and validates Route TLV from a message.
         *
         * @param[out] aRouteTlv    A reference to output the read Route TLV.
         *
         * @retval kErrorNone       Successfully read and validated the Route TLV.
         * @retval kErrorNotFound   TLV was not found in the message.
         * @retval kErrorParse      TLV was found but could not be parsed or is not valid.
         *
         */
        Error ReadRouteTlv(RouteTlv &aRouteTlv) const;
#endif

    private:
        Error ReadChallengeOrResponse(uint8_t aTlvType, Challenge &aBuffer) const;
    };

    /**
     * This structure represents a received MLE message containing additional information about the message (e.g.
     * key sequence, neighbor from which it was received).
     *
     */
    struct RxInfo
    {
        /**
         * This enumeration represents a received MLE message class.
         *
         */
        enum Class : uint8_t
        {
            kUnknown,              ///< Unknown (default value, also indicates MLE message parse error).
            kAuthoritativeMessage, ///< Authoritative message (larger received key seq MUST be adopted).
            kPeerMessage,          ///< Peer message (adopt only if from a known neighbor and is greater by one).
        };

        /**
         * This constructor initializes the `RxInfo`.
         *
         * @param[in] aMessage       The received MLE message.
         * @param[in] aMessageInfo   The `Ip6::MessageInfo` associated with message.
         *
         */
        RxInfo(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
            : mMessage(static_cast<RxMessage &>(aMessage))
            , mMessageInfo(aMessageInfo)
            , mFrameCounter(0)
            , mKeySequence(0)
            , mNeighbor(nullptr)
            , mClass(kUnknown)
        {
        }

        /**
         * Indicates whether the `mNeighbor` (neighbor from which message was received) is non-null and
         * in valid state.
         *
         * @retval TRUE  If `mNeighbor` is non-null and in valid state.
         * @retval FALSE If `mNeighbor` is `nullptr` or not in valid state.
         *
         */
        bool IsNeighborStateValid(void) const { return (mNeighbor != nullptr) && mNeighbor->IsStateValid(); }

        RxMessage              &mMessage;      ///< The MLE message.
        const Ip6::MessageInfo &mMessageInfo;  ///< The `MessageInfo` associated with the message.
        uint32_t                mFrameCounter; ///< The frame counter from aux security header.
        uint32_t                mKeySequence;  ///< The key sequence from aux security header.
        Neighbor               *mNeighbor;     ///< Neighbor from which message was received (can be `nullptr`).
        Class                   mClass;        ///< The message class (authoritative, peer, or unknown).
    };

    /**
     * Allocates and initializes new MLE message for a given command.
     *
     * @param[in] aCommand   The MLE command.
     *
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
     *
     */
    TxMessage *NewMleMessage(Command aCommand);

    /**
     * Sets the device role.
     *
     * @param[in] aRole A device role.
     *
     */
    void SetRole(DeviceRole aRole);

    /**
     * Causes the Thread interface to attempt an MLE attach.
     *
     * @param[in]  aMode  Indicates what partitions to attach to.
     *
     */
    void Attach(AttachMode aMode);

    /**
     * Sets the attach state
     *
     * @param[in] aState An attach state
     *
     */
    void SetAttachState(AttachState aState);

    /**
     * Initializes a given @p aNeighbor with information from @p aRxInfo.
     *
     * Updates the following properties on @p aNeighbor from @p aRxInfo:
     *
     * - Sets the Extended MAC address from `MessageInfo` peer IPv6 address IID.
     * - Clears the `GetLinkInfo()` and adds RSS from the `GetThreadLinkInfo()`.
     * - Resets the link failure counter (`ResetLinkFailures()`).
     * - Sets the "last heard" time to now (`SetLastHeard()`).
     *
     * @param[in,out] aNeighbor   The `Neighbor` to initialize.
     * @param[in]     aRxInfo     The `RxtInfo` to use for initialization.
     *
     */
    void InitNeighbor(Neighbor &aNeighbor, const RxInfo &aRxInfo);

    /**
     * Clears the parent candidate.
     *
     */
    void ClearParentCandidate(void) { mParentCandidate.Clear(); }

    /**
     * Checks if the destination is reachable.
     *
     * @param[in]  aMeshDest   The RLOC16 of the destination.
     * @param[in]  aIp6Header  The IPv6 header of the message.
     *
     * @retval kErrorNone      The destination is reachable.
     * @retval kErrorNoRoute   The destination is not reachable and the message should be dropped.
     *
     */
    Error CheckReachability(uint16_t aMeshDest, const Ip6::Header &aIp6Header);

    /**
     * Returns the next hop towards an RLOC16 destination.
     *
     * @param[in]  aDestination  The RLOC16 of the destination.
     *
     * @returns A RLOC16 of the next hop if a route is known, kInvalidRloc16 otherwise.
     *
     */
    Mac::ShortAddress GetNextHop(uint16_t aDestination) const;

    /**
     * Generates an MLE Data Request message.
     *
     * @param[in]  aDestination      The IPv6 destination address.
     *
     * @retval kErrorNone     Successfully generated an MLE Data Request message.
     * @retval kErrorNoBufs   Insufficient buffers to generate the MLE Data Request message.
     *
     */
    Error SendDataRequest(const Ip6::Address &aDestination);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    /**
     * Generates an MLE Data Request message which request Link Metrics Report TLV.
     *
     * @param[in]  aDestination      A reference to the IPv6 address of the destination.
     * @param[in]  aQueryInfo        A Link Metrics query info.
     *
     * @retval kErrorNone     Successfully generated an MLE Data Request message.
     * @retval kErrorNoBufs   Insufficient buffers to generate the MLE Data Request message.
     *
     */
    Error SendDataRequestForLinkMetricsReport(const Ip6::Address                      &aDestination,
                                              const LinkMetrics::Initiator::QueryInfo &aQueryInfo);
#endif

    /**
     * Generates an MLE Child Update Request message.
     *
     * @retval kErrorNone    Successfully generated an MLE Child Update Request message.
     * @retval kErrorNoBufs  Insufficient buffers to generate the MLE Child Update Request message.
     *
     */
    Error SendChildUpdateRequest(void);

    /**
     * Generates an MLE Child Update Response message.
     *
     * @param[in]  aTlvList      A list of requested TLV types.
     * @param[in]  aChallenge    The Challenge for the response.
     *
     * @retval kErrorNone     Successfully generated an MLE Child Update Response message.
     * @retval kErrorNoBufs   Insufficient buffers to generate the MLE Child Update Response message.
     *
     */
    Error SendChildUpdateResponse(const TlvList &aTlvList, const Challenge &aChallenge);

    /**
     * Sets the RLOC16 assigned to the Thread interface.
     *
     * @param[in]  aRloc16  The RLOC16 to set.
     *
     */
    void SetRloc16(uint16_t aRloc16);

    /**
     * Sets the Device State to Detached.
     *
     */
    void SetStateDetached(void);

    /**
     * Sets the Device State to Child.
     *
     */
    void SetStateChild(uint16_t aRloc16);

    /**
     * Sets the Leader's Partition ID, Weighting, and Router ID values.
     *
     * @param[in]  aPartitionId     The Leader's Partition ID value.
     * @param[in]  aWeighting       The Leader's Weighting value.
     * @param[in]  aLeaderRouterId  The Leader's Router ID value.
     *
     */
    void SetLeaderData(uint32_t aPartitionId, uint8_t aWeighting, uint8_t aLeaderRouterId);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    /**
     * Emits a log message with an IPv6 address.
     *
     * @param[in]  aAction     The message action (send/receive/delay, etc).
     * @param[in]  aType       The message type.
     * @param[in]  aAddress    The IPv6 address of the peer.
     *
     */
    static void Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress);

    /**
     * Emits a log message with an IPv6 address and RLOC16.
     *
     * @param[in]  aAction     The message action (send/receive/delay, etc).
     * @param[in]  aType       The message type.
     * @param[in]  aAddress    The IPv6 address of the peer.
     * @param[in]  aRloc       The RLOC16.
     *
     */
    static void Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress, uint16_t aRloc);
#else
    static void Log(MessageAction, MessageType, const Ip6::Address &) {}
    static void Log(MessageAction, MessageType, const Ip6::Address &, uint16_t) {}
#endif // #if OT_SHOULD_LOG_AT( OT_LOG_LEVEL_INFO)

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    /**
     * Emits a log message indicating an error in processing of a message.
     *
     * Note that log message is emitted only if there is an error, i.e., @p aError is not `kErrorNone`. The log
     * message will have the format "Failed to process {aMessageString} : {ErrorString}".
     *
     * @param[in]  aType      The message type.
     * @param[in]  aError     The error in processing of the message.
     *
     */
    static void LogProcessError(MessageType aType, Error aError);

    /**
     * Emits a log message indicating an error when sending a message.
     *
     * Note that log message is emitted only if there is an error, i.e. @p aError is not `kErrorNone`. The log
     * message will have the format "Failed to send {Message Type} : {ErrorString}".
     *
     * @param[in]  aType    The message type.
     * @param[in]  aError   The error in sending the message.
     *
     */
    static void LogSendError(MessageType aType, Error aError);
#else
    static void LogProcessError(MessageType, Error) {}
    static void LogSendError(MessageType, Error) {}
#endif // #if OT_SHOULD_LOG_AT( OT_LOG_LEVEL_WARN)

    /**
     * Triggers MLE Announce on previous channel after the Thread device successfully
     * attaches and receives the new Active Commissioning Dataset if needed.
     *
     * MTD would send Announce immediately after attached.
     * FTD would delay to send Announce after tried to become Router or decided to stay in REED role.
     *
     */
    void InformPreviousChannel(void);

    /**
     * Indicates whether or not in announce attach process.
     *
     * @retval true if attaching/attached on the announced parameters, false otherwise.
     *
     */
    bool IsAnnounceAttach(void) const { return mAlternatePanId != Mac::kPanIdBroadcast; }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)
    /**
     * Converts an `AttachMode` enumeration value into a human-readable string.
     *
     * @param[in] aMode An attach mode
     *
     * @returns A human-readable string corresponding to the attach mode.
     *
     */
    static const char *AttachModeToString(AttachMode aMode);

    /**
     * Converts an `AttachState` enumeration value into a human-readable string.
     *
     * @param[in] aState An attach state
     *
     * @returns A human-readable string corresponding to the attach state.
     *
     */
    static const char *AttachStateToString(AttachState aState);

    /**
     * Converts a `ReattachState` enumeration value into a human-readable string.
     *
     * @param[in] aState A reattach state
     *
     * @returns A human-readable string corresponding to the reattach state.
     *
     */
    static const char *ReattachStateToString(ReattachState aState);
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    /**
     * Sends a Link Metrics Management Request message.
     *
     * @param[in]  aDestination  A reference to the IPv6 address of the destination.
     * @param[in]  aSubTlv       A reference to the sub-TLV to include.
     *
     * @retval kErrorNone     Successfully sent a Link Metrics Management Request.
     * @retval kErrorNoBufs   Insufficient buffers to generate the MLE Link Metrics Management Request message.
     *
     */
    Error SendLinkMetricsManagementRequest(const Ip6::Address &aDestination, const ot::Tlv &aSubTlv);

    /**
     * Sends an MLE Link Probe message.
     *
     * @param[in]  aDestination  A reference to the IPv6 address of the destination.
     * @param[in]  aSeriesId     The Series ID [1, 254] which the Probe message targets at.
     * @param[in]  aBuf          A pointer to the data payload.
     * @param[in]  aLength       The length of the data payload in Link Probe TLV, [0, 64].
     *
     * @retval kErrorNone         Successfully sent a Link Metrics Management Request.
     * @retval kErrorNoBufs       Insufficient buffers to generate the MLE Link Metrics Management Request message.
     * @retval kErrorInvalidArgs  Series ID is not a valid value, not within range [1, 254].
     *
     */
    Error SendLinkProbe(const Ip6::Address &aDestination, uint8_t aSeriesId, uint8_t *aBuf, uint8_t aLength);

#endif

    void ScheduleMessageTransmissionTimer(void);

private:
    // Declare early so we can use in as `TimerMilli` callbacks.
    void HandleAttachTimer(void);
    void HandleDelayedResponseTimer(void);
    void HandleMessageTransmissionTimer(void);

protected:
    using AttachTimer = TimerMilliIn<Mle, &Mle::HandleAttachTimer>;
    using DelayTimer  = TimerMilliIn<Mle, &Mle::HandleDelayedResponseTimer>;
    using MsgTxTimer  = TimerMilliIn<Mle, &Mle::HandleMessageTransmissionTimer>;

    Ip6::Netif::UnicastAddress mLeaderAloc; ///< Leader anycast locator

    LeaderData    mLeaderData;                 ///< Last received Leader Data TLV.
    bool          mRetrieveNewNetworkData : 1; ///< Indicating new Network Data is needed if set.
    bool          mRequestRouteTlv : 1;        ///< Request Route TLV when sending Data Request.
    DeviceRole    mRole;                       ///< Current Thread role.
    Parent        mParent;                     ///< Parent information.
    NeighborTable mNeighborTable;              ///< The neighbor table.
    DeviceMode    mDeviceMode;                 ///< Device mode setting.
    AttachState   mAttachState;                ///< The attach state.
    uint8_t       mParentRequestCounter;       ///< Number of parent requests while in `kAttachStateParentRequest`.
    ReattachState mReattachState;              ///< Reattach state
    uint16_t      mAttachCounter;              ///< Attach attempt counter.
    uint16_t      mAnnounceDelay;              ///< Delay in between sending Announce messages during attach.
    AttachTimer   mAttachTimer;                ///< The timer for driving the attach process.
    DelayTimer    mDelayedResponseTimer;       ///< The timer to delay MLE responses.
    MsgTxTimer    mMessageTransmissionTimer;   ///< The timer for (re-)sending of MLE messages (e.g. Child Update).
#if OPENTHREAD_FTD
    uint8_t mLinkRequestAttempts; ///< Number of remaining link requests to send after reset.
    bool    mWasLeader;           ///< Indicating if device was leader before reset.
#endif

private:
    static constexpr uint8_t kMleHopLimit        = 255;
    static constexpr uint8_t kMleSecurityTagSize = 4; // Security tag size in bytes.

    // Parameters for "attach backoff" feature (CONFIG_ENABLE_ATTACH_BACKOFF) - Intervals are in milliseconds.
    static constexpr uint32_t kAttachBackoffMinInterval = OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_MINIMUM_INTERVAL;
    static constexpr uint32_t kAttachBackoffMaxInterval = OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_MAXIMUM_INTERVAL;
    static constexpr uint32_t kAttachBackoffJitter      = OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_JITTER_INTERVAL;
    static constexpr uint32_t kAttachBackoffDelayToResetCounter =
        OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_DELAY_TO_RESET_BACKOFF_INTERVAL;

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

    static constexpr uint32_t kDetachGracefullyTimeout = 1000;

    static constexpr uint32_t kStoreFrameCounterAhead   = OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    static constexpr uint8_t  kMaxIpAddressesToRegister = OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER;
    static constexpr uint32_t kDefaultCslTimeout        = OPENTHREAD_CONFIG_CSL_TIMEOUT;

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

    struct DelayedResponseMetadata
    {
        Error AppendTo(Message &aMessage) const { return aMessage.Append(*this); }
        void  ReadFrom(const Message &aMessage);
        void  RemoveFrom(Message &aMessage) const;

        Ip6::Address mDestination; // IPv6 address of the message destination.
        TimeMilli    mSendTime;    // Time when the message shall be sent.
    };

    OT_TOOL_PACKED_BEGIN
    class SecurityHeader
    {
    public:
        void InitSecurityControl(void) { mSecurityControl = kKeyIdMode2Mic32; }
        bool IsSecurityControlValid(void) const { return (mSecurityControl == kKeyIdMode2Mic32); }

        uint32_t GetFrameCounter(void) const { return Encoding::LittleEndian::HostSwap32(mFrameCounter); }
        void     SetFrameCounter(uint32_t aCounter) { mFrameCounter = Encoding::LittleEndian::HostSwap32(aCounter); }

        uint32_t GetKeyId(void) const { return Encoding::BigEndian::HostSwap32(mKeySource); }
        void     SetKeyId(uint32_t aKeySequence)
        {
            mKeySource = Encoding::BigEndian::HostSwap32(aKeySequence);
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

    class ParentCandidate : public Parent
    {
    public:
        void Init(Instance &aInstance) { Parent::Init(aInstance); }
        void Clear(void);
        void CopyTo(Parent &aParent) const;

        Challenge  mChallenge;
        int8_t     mPriority;
        uint8_t    mLinkQuality3;
        uint8_t    mLinkQuality2;
        uint8_t    mLinkQuality1;
        uint16_t   mSedBufferSize;
        uint8_t    mSedDatagramCount;
        uint8_t    mLinkMargin;
        LeaderData mLeaderData;
        bool       mIsSingleton;
    };

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
        void     ApplyMeshLocalPrefix(const Ip6::NetworkPrefix &aPrefix) { GetAddress().SetPrefix(aPrefix); }
    };
#endif

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

    Error       Start(StartMode aMode);
    void        Stop(StopMode aMode);
    void        HandleNotifierEvents(Events aEvents);
    void        SendDelayedResponse(TxMessage &aMessage, const DelayedResponseMetadata &aMetadata);
    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void        ReestablishLinkWithNeighbor(Neighbor &aNeighbor);
    static void HandleDetachGracefullyTimer(Timer &aTimer);
    void        HandleDetachGracefullyTimer(void);
    bool        IsDetachingGracefully(void) { return mDetachGracefullyTimer.IsRunning(); }
    Error       SendChildUpdateRequest(ChildUpdateRequestMode aMode);
    Error       SendDataRequestAfterDelay(const Ip6::Address &aDestination, uint16_t aDelay);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    Error SendDataRequest(const Ip6::Address                      &aDestination,
                          const uint8_t                           *aTlvs,
                          uint8_t                                  aTlvsLength,
                          uint16_t                                 aDelay,
                          const LinkMetrics::Initiator::QueryInfo *aQueryInfo = nullptr);
#else
    Error SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength, uint16_t aDelay);
#endif

#if OPENTHREAD_FTD
    static void HandleDetachGracefullyAddressReleaseResponse(void                *aContext,
                                                             otMessage           *aMessage,
                                                             const otMessageInfo *aMessageInfo,
                                                             Error                aResult);
    void        HandleDetachGracefullyAddressReleaseResponse(void);
#endif

    void HandleAdvertisement(RxInfo &aRxInfo);
    void HandleChildIdResponse(RxInfo &aRxInfo);
    void HandleChildUpdateRequest(RxInfo &aRxInfo);
    void HandleChildUpdateResponse(RxInfo &aRxInfo);
    void HandleDataResponse(RxInfo &aRxInfo);
    void HandleParentResponse(RxInfo &aRxInfo);
    void HandleAnnounce(RxInfo &aRxInfo);
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    void HandleLinkMetricsManagementRequest(RxInfo &aRxInfo);
    void HandleLinkProbe(RxInfo &aRxInfo);
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    void HandleLinkMetricsManagementResponse(RxInfo &aRxInfo);
#endif
    Error HandleLeaderData(RxInfo &aRxInfo);
    void  ProcessAnnounce(void);
    bool  HasUnregisteredAddress(void);

    uint32_t GetAttachStartDelay(void) const;
    void     SendParentRequest(ParentRequestType aType);
    Error    SendChildIdRequest(void);
    Error    GetNextAnnounceChannel(uint8_t &aChannel) const;
    bool     HasMoreChannelsToAnnounce(void) const;
    bool     PrepareAnnounceState(void);
    void     SendAnnounce(uint8_t aChannel, AnnounceMode aMode);
    void     SendAnnounce(uint8_t aChannel, const Ip6::Address &aDestination, AnnounceMode aMode = kNormalAnnounce);
    void RemoveDelayedMessage(Message::SubType aSubType, MessageType aMessageType, const Ip6::Address *aDestination);
    void RemoveDelayedDataRequestMessage(const Ip6::Address &aDestination);
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    Error SendLinkMetricsManagementResponse(const Ip6::Address &aDestination, LinkMetrics::Status aStatus);
#endif
    uint32_t Reattach(void);
    bool     HasAcceptableParentCandidate(void) const;
    Error    DetermineParentRequestType(ParentRequestType &aType) const;

    bool IsBetterParent(uint16_t                aRloc16,
                        LinkQuality             aLinkQuality,
                        uint8_t                 aLinkMargin,
                        const ConnectivityTlv  &aConnectivityTlv,
                        uint16_t                aVersion,
                        const Mac::CslAccuracy &aCslAccuracy);
    bool IsNetworkDataNewer(const LeaderData &aLeaderData);

    Error ProcessMessageSecurity(Crypto::AesCcm::Mode    aMode,
                                 Message                &aMessage,
                                 const Ip6::MessageInfo &aMessageInfo,
                                 uint16_t                aCmdOffset,
                                 const SecurityHeader   &aHeader);

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ServiceAloc *FindInServiceAlocs(uint16_t aAloc16);
    void         UpdateServiceAlocs(void);
#endif

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
    void InformPreviousParent(void);
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    static void        LogError(MessageAction aAction, MessageType aType, Error aError);
    static const char *MessageActionToString(MessageAction aAction);
    static const char *MessageTypeToString(MessageType aType);
    static const char *MessageTypeActionToSuffixString(MessageType aType, MessageAction aAction);
#endif

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    void UpdateRoleTimeCounters(DeviceRole aRole);
#endif

    using DetachGracefullyTimer = TimerMilliIn<Mle, &Mle::HandleDetachGracefullyTimer>;

    MessageQueue mDelayedResponses;

    Challenge mParentRequestChallenge;

    AttachMode      mAttachMode;
    ParentCandidate mParentCandidate;

    uint8_t                 mChildUpdateAttempts;
    ChildUpdateRequestState mChildUpdateRequestState;
    uint8_t                 mDataRequestAttempts;
    DataRequestState        mDataRequestState;

    AddressRegistrationMode mAddressRegistrationMode;

    bool mHasRestored;
    bool mReceivedResponseFromParent;
    bool mInitiallyAttachedAsSleepy;

    Ip6::Udp::Socket mSocket;
    uint32_t         mTimeout;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    uint32_t mCslTimeout;
#endif

    uint16_t mRloc16;
    uint16_t mPreviousParentRloc;

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    ParentSearch mParentSearch;
#endif

    uint8_t  mAnnounceChannel;
    uint8_t  mAlternateChannel;
    uint16_t mAlternatePanId;
    uint64_t mAlternateTimestamp;

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    ServiceAloc mServiceAlocs[kMaxServiceAlocs];
#endif

    Counters mCounters;
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    uint64_t mLastUpdatedTimestamp;
#endif

    static const otMeshLocalPrefix sMeshLocalPrefixInit;

    Ip6::Netif::UnicastAddress   mLinkLocal64;
    Ip6::Netif::UnicastAddress   mMeshLocal64;
    Ip6::Netif::UnicastAddress   mMeshLocal16;
    Ip6::Netif::MulticastAddress mLinkLocalAllThreadNodes;
    Ip6::Netif::MulticastAddress mRealmLocalAllThreadNodes;

    DetachGracefullyTimer                mDetachGracefullyTimer;
    Callback<otDetachGracefullyCallback> mDetachGracefullyCallback;

#if OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE
    Callback<otThreadParentResponseCallback> mParentResponseCallback;
#endif
};

} // namespace Mle

/**
 * @}
 *
 */

} // namespace ot

#endif // MLE_HPP_
