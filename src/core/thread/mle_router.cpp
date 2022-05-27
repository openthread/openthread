/*
 *  Copyright (c) 2016, The OpenThread Authors.  All rights reserved.
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
 *   This file implements MLE functionality required for the Thread Router and Leader roles.
 */

#include "mle_router.hpp"

#if OPENTHREAD_FTD

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/random.hpp"
#include "common/serial_number.hpp"
#include "common/settings.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop.hpp"
#include "net/icmp6.hpp"
#include "thread/key_manager.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/time_sync_service.hpp"
#include "thread/uri_paths.hpp"
#include "utils/otns.hpp"

namespace ot {
namespace Mle {

RegisterLogModule("Mle");

MleRouter::MleRouter(Instance &aInstance)
    : Mle(aInstance)
    , mAdvertiseTrickleTimer(aInstance, MleRouter::HandleAdvertiseTrickleTimer)
    , mAddressSolicit(UriPath::kAddressSolicit, &MleRouter::HandleAddressSolicit, this)
    , mAddressRelease(UriPath::kAddressRelease, &MleRouter::HandleAddressRelease, this)
    , mChildTable(aInstance)
    , mRouterTable(aInstance)
    , mChallengeTimeout(0)
    , mNextChildId(kMaxChildId)
    , mNetworkIdTimeout(kNetworkIdTimeout)
    , mRouterUpgradeThreshold(kRouterUpgradeThreshold)
    , mRouterDowngradeThreshold(kRouterDowngradeThreshold)
    , mLeaderWeight(kLeaderWeight)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mPreferredLeaderPartitionId(0)
    , mCcmEnabled(false)
    , mThreadVersionCheckEnabled(true)
#endif
    , mRouterEligible(true)
    , mAddressSolicitPending(false)
    , mAddressSolicitRejected(false)
    , mPreviousPartitionIdRouter(0)
    , mPreviousPartitionId(0)
    , mPreviousPartitionRouterIdSequence(0)
    , mPreviousPartitionIdTimeout(0)
    , mRouterSelectionJitter(kRouterSelectionJitter)
    , mRouterSelectionJitterTimeout(0)
    , mLinkRequestDelay(0)
    , mParentPriority(kParentPriorityUnspecified)
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    , mBackboneRouterRegistrationDelay(0)
#endif
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mMaxChildIpAddresses(0)
#endif
    , mDiscoveryRequestCallback(nullptr)
    , mDiscoveryRequestCallbackContext(nullptr)
{
    mDeviceMode.Set(mDeviceMode.Get() | DeviceMode::kModeFullThreadDevice | DeviceMode::kModeFullNetworkData);

    SetRouterId(kInvalidRouterId);

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    mSteeringData.Clear();
#endif
}

void MleRouter::HandlePartitionChange(void)
{
    mPreviousPartitionId               = mLeaderData.GetPartitionId();
    mPreviousPartitionRouterIdSequence = mRouterTable.GetRouterIdSequence();
    mPreviousPartitionIdTimeout        = GetNetworkIdTimeout();

    Get<AddressResolver>().Clear();
    IgnoreError(Get<Tmf::Agent>().AbortTransaction(&MleRouter::HandleAddressSolicitResponse, this));
    mRouterTable.Clear();
}

bool MleRouter::IsRouterEligible(void) const
{
    bool                  rval      = false;
    const SecurityPolicy &secPolicy = Get<KeyManager>().GetSecurityPolicy();

    VerifyOrExit(mRouterEligible && IsFullThreadDevice());

#if OPENTHREAD_CONFIG_THREAD_VERSION == OT_THREAD_VERSION_1_1
    VerifyOrExit(secPolicy.mRoutersEnabled);
#else
    if (secPolicy.mCommercialCommissioningEnabled)
    {
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        VerifyOrExit(mCcmEnabled || secPolicy.mNonCcmRoutersEnabled);
#else
        VerifyOrExit(secPolicy.mNonCcmRoutersEnabled);
#endif
    }
    if (!secPolicy.mRoutersEnabled)
    {
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        VerifyOrExit(!mThreadVersionCheckEnabled ||
                     secPolicy.mVersionThresholdForRouting + SecurityPolicy::kVersionThresholdOffsetVersion <=
                         kThreadVersion);
#else
        VerifyOrExit(secPolicy.mVersionThresholdForRouting + SecurityPolicy::kVersionThresholdOffsetVersion <=
                     kThreadVersion);
#endif
    }
#endif

    rval = true;

exit:
    return rval;
}

Error MleRouter::SetRouterEligible(bool aEligible)
{
    Error error = kErrorNone;

    VerifyOrExit(IsFullThreadDevice() || !aEligible, error = kErrorNotCapable);

    mRouterEligible = aEligible;

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        break;

    case kRoleChild:
        Get<Mac::Mac>().SetBeaconEnabled(mRouterEligible);
        break;

    case kRoleRouter:
    case kRoleLeader:
        if (!mRouterEligible)
        {
            IgnoreError(BecomeDetached());
        }

        break;
    }

exit:
    return error;
}

Error MleRouter::BecomeRouter(ThreadStatusTlv::Status aStatus)
{
    Error error = kErrorNone;

    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsRouter(), error = kErrorNone);
    VerifyOrExit(IsRouterEligible(), error = kErrorNotCapable);

    LogInfo("Attempt to become router");

    Get<MeshForwarder>().SetRxOnWhenIdle(true);
    mRouterSelectionJitterTimeout = 0;
    mLinkRequestDelay             = 0;

    switch (mRole)
    {
    case kRoleDetached:
        SuccessOrExit(error = SendLinkRequest(nullptr));
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMleRouter);
        break;

    case kRoleChild:
        SuccessOrExit(error = SendAddressSolicit(aStatus));
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

exit:
    return error;
}

Error MleRouter::BecomeLeader(void)
{
    Error    error = kErrorNone;
    Router * router;
    uint32_t partitionId;
    uint8_t  leaderId;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t minRouterId;
    uint8_t maxRouterId;
#endif

    VerifyOrExit(!Get<MeshCoP::ActiveDatasetManager>().IsPartiallyComplete(), error = kErrorInvalidState);
    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsLeader(), error = kErrorNone);
    VerifyOrExit(IsRouterEligible(), error = kErrorNotCapable);

    mRouterTable.Clear();

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    partitionId = mPreferredLeaderPartitionId ? mPreferredLeaderPartitionId : Random::NonCrypto::GetUint32();
#else
    partitionId = Random::NonCrypto::GetUint32();
#endif

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    mRouterTable.GetRouterIdRange(minRouterId, maxRouterId);
    if (IsRouterIdValid(mPreviousRouterId) && minRouterId <= mPreviousRouterId && mPreviousRouterId <= maxRouterId)
    {
        leaderId = mPreviousRouterId;
    }
    else
    {
        leaderId = Random::NonCrypto::GetUint8InRange(minRouterId, maxRouterId + 1);
    }
#else
    leaderId    = IsRouterIdValid(mPreviousRouterId) ? mPreviousRouterId
                                                  : Random::NonCrypto::GetUint8InRange(0, kMaxRouterId + 1);
#endif

    SetLeaderData(partitionId, mLeaderWeight, leaderId);

    router = mRouterTable.Allocate(leaderId);
    OT_ASSERT(router != nullptr);

    SetRouterId(leaderId);
    router->SetExtAddress(Get<Mac::Mac>().GetExtAddress());

    Get<NetworkData::Leader>().Reset();
    Get<MeshCoP::Leader>().SetEmptyCommissionerData();

    SetStateLeader(Rloc16FromRouterId(leaderId));

exit:
    return error;
}

void MleRouter::StopLeader(void)
{
    Get<Tmf::Agent>().RemoveResource(mAddressSolicit);
    Get<Tmf::Agent>().RemoveResource(mAddressRelease);
    Get<MeshCoP::ActiveDatasetManager>().StopLeader();
    Get<MeshCoP::PendingDatasetManager>().StopLeader();
    StopAdvertiseTrickleTimer();
    Get<NetworkData::Leader>().Stop();
    Get<ThreadNetif>().UnsubscribeAllRoutersMulticast();
}

void MleRouter::HandleDetachStart(void)
{
    mRouterTable.ClearNeighbors();
    StopLeader();
    Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMleRouter);
}

void MleRouter::HandleChildStart(AttachMode aMode)
{
    mLinkRequestDelay       = 0;
    mAddressSolicitRejected = false;

    mRouterSelectionJitterTimeout = 1 + Random::NonCrypto::GetUint8InRange(0, mRouterSelectionJitter);

    StopLeader();
    Get<TimeTicker>().RegisterReceiver(TimeTicker::kMleRouter);

    if (mRouterEligible)
    {
        Get<Mac::Mac>().SetBeaconEnabled(true);
    }

    Get<ThreadNetif>().SubscribeAllRoutersMulticast();

    VerifyOrExit(IsRouterIdValid(mPreviousRouterId));

    switch (aMode)
    {
    case kDowngradeToReed:
        SendAddressRelease();

        // reset children info if any
        if (HasChildren())
        {
            RemoveChildren();
        }

        // reset routerId info
        SetRouterId(kInvalidRouterId);
        break;

    case kSamePartition:
    case kSamePartitionRetry:
        if (HasChildren())
        {
            IgnoreError(BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest));
        }

        break;

    case kAnyPartition:
    case kBetterParent:
        // If attach was started due to receiving MLE Announce Messages, all rx-on-when-idle devices would
        // start attach immediately when receiving such Announce message as in Thread 1.1 specification,
        // Section 4.8.1,
        // "If the received value is newer and the channel and/or PAN ID in the Announce message differ
        //  from those currently in use, the receiving device attempts to attach using the channel and
        //  PAN ID received from the Announce message."
        //
        // That is, Parent-child relationship is highly unlikely to be kept in the new partition, so here
        // removes all children, leaving whether to become router according to the new partition status.
        if (IsAnnounceAttach() && HasChildren())
        {
            RemoveChildren();
        }

        OT_FALL_THROUGH;

    case kBetterPartition:
        if (HasChildren() && mPreviousPartitionIdRouter != mLeaderData.GetPartitionId())
        {
            IgnoreError(BecomeRouter(ThreadStatusTlv::kParentPartitionChange));
        }

        break;
    }

exit:

    if (mRouterTable.GetActiveRouterCount() >= mRouterUpgradeThreshold &&
        (!IsRouterIdValid(mPreviousRouterId) || !HasChildren()))
    {
        SetRouterId(kInvalidRouterId);
    }
}

void MleRouter::SetStateRouter(uint16_t aRloc16)
{
    SetRloc16(aRloc16);

    SetRole(kRoleRouter);
    SetAttachState(kAttachStateIdle);
    mAttachCounter = 0;
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    StopAdvertiseTrickleTimer();
    ResetAdvertiseInterval();

    Get<ThreadNetif>().SubscribeAllRoutersMulticast();
    mPreviousPartitionIdRouter = mLeaderData.GetPartitionId();
    Get<NetworkData::Leader>().Stop();
    Get<Ip6::Ip6>().SetForwardingEnabled(true);
    Get<Ip6::Mpl>().SetTimerExpirations(kMplRouterDataMessageTimerExpirations);
    Get<Mac::Mac>().SetBeaconEnabled(true);

    // remove children that do not have matching RLOC16
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
    {
        if (RouterIdFromRloc16(child.GetRloc16()) != mRouterId)
        {
            RemoveNeighbor(child);
        }
    }
}

void MleRouter::SetStateLeader(uint16_t aRloc16)
{
    IgnoreError(Get<MeshCoP::ActiveDatasetManager>().Restore());
    IgnoreError(Get<MeshCoP::PendingDatasetManager>().Restore());
    SetRloc16(aRloc16);

    SetRole(kRoleLeader);
    SetAttachState(kAttachStateIdle);
    mAttachCounter = 0;
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    StopAdvertiseTrickleTimer();
    ResetAdvertiseInterval();
    IgnoreError(GetLeaderAloc(mLeaderAloc.GetAddress()));
    Get<ThreadNetif>().AddUnicastAddress(mLeaderAloc);

    Get<ThreadNetif>().SubscribeAllRoutersMulticast();
    mPreviousPartitionIdRouter = mLeaderData.GetPartitionId();
    Get<TimeTicker>().RegisterReceiver(TimeTicker::kMleRouter);

    Get<NetworkData::Leader>().Start();
    Get<MeshCoP::ActiveDatasetManager>().StartLeader();
    Get<MeshCoP::PendingDatasetManager>().StartLeader();
    Get<Tmf::Agent>().AddResource(mAddressSolicit);
    Get<Tmf::Agent>().AddResource(mAddressRelease);
    Get<Ip6::Ip6>().SetForwardingEnabled(true);
    Get<Ip6::Mpl>().SetTimerExpirations(kMplRouterDataMessageTimerExpirations);
    Get<Mac::Mac>().SetBeaconEnabled(true);
    Get<AddressResolver>().Clear();

    // remove children that do not have matching RLOC16
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
    {
        if (RouterIdFromRloc16(child.GetRloc16()) != mRouterId)
        {
            RemoveNeighbor(child);
        }
    }

    LogNote("Leader partition id 0x%x", mLeaderData.GetPartitionId());
}

void MleRouter::HandleAdvertiseTrickleTimer(TrickleTimer &aTimer)
{
    aTimer.Get<MleRouter>().HandleAdvertiseTrickleTimer();
}

void MleRouter::HandleAdvertiseTrickleTimer(void)
{
    VerifyOrExit(IsRouterEligible(), mAdvertiseTrickleTimer.Stop());

    SendAdvertisement();

exit:
    return;
}

void MleRouter::StopAdvertiseTrickleTimer(void)
{
    mAdvertiseTrickleTimer.Stop();
}

void MleRouter::ResetAdvertiseInterval(void)
{
    VerifyOrExit(IsRouterOrLeader());

    if (!mAdvertiseTrickleTimer.IsRunning())
    {
        mAdvertiseTrickleTimer.Start(TrickleTimer::kModeTrickle, Time::SecToMsec(kAdvertiseIntervalMin),
                                     Time::SecToMsec(kAdvertiseIntervalMax));
    }

    mAdvertiseTrickleTimer.IndicateInconsistent();

exit:
    return;
}

void MleRouter::SendAdvertisement(void)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage *  message = nullptr;

    // Suppress MLE Advertisements when trying to attach to a better partition.
    //
    // Without this suppression, a device may send an MLE Advertisement before receiving the MLE Child ID Response.
    // The candidate parent then removes the attaching device because the Source Address TLV includes an RLOC16 that
    // indicates a Router role (i.e. a Child ID equal to zero).
    VerifyOrExit(!IsAttaching());

    // Suppress MLE Advertisements when transitioning to the router role.
    //
    // When trying to attach to a new partition, sending out advertisements as a REED can cause already-attached
    // children to detach.
    VerifyOrExit(!mAddressSolicitPending);

    // Suppress MLE Advertisements before sending multicast Link Request.
    //
    // Before sending the multicast Link Request message, no links have been established to neighboring routers.
    VerifyOrExit(mLinkRequestDelay == 0);

    VerifyOrExit((message = NewMleMessage(kCommandAdvertisement)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);

    case kRoleChild:
        break;

    case kRoleRouter:
    case kRoleLeader:
        SuccessOrExit(error = message->AppendRouteTlv());
        break;
    }

    destination.SetToLinkLocalAllNodesMulticast();
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeAdvertisement, destination);

exit:
    FreeMessageOnError(message, error);
    LogSendError(kTypeAdvertisement, error);
}

Error MleRouter::SendLinkRequest(Neighbor *aNeighbor)
{
    static const uint8_t detachedTlvs[]      = {Tlv::kAddress16, Tlv::kRoute};
    static const uint8_t routerTlvs[]        = {Tlv::kLinkMargin};
    static const uint8_t validNeighborTlvs[] = {Tlv::kLinkMargin, Tlv::kRoute};
    Error                error               = kErrorNone;
    TxMessage *          message             = nullptr;
    Ip6::Address         destination;

    VerifyOrExit(mLinkRequestDelay == 0 && mChallengeTimeout == 0);

    destination.Clear();

    VerifyOrExit((message = NewMleMessage(kCommandLinkRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendVersionTlv());

    switch (mRole)
    {
    case kRoleDisabled:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);

    case kRoleDetached:
        SuccessOrExit(error = message->AppendTlvRequestTlv(detachedTlvs, sizeof(detachedTlvs)));
        break;

    case kRoleChild:
        SuccessOrExit(error = message->AppendSourceAddressTlv());
        SuccessOrExit(error = message->AppendLeaderDataTlv());
        break;

    case kRoleRouter:
    case kRoleLeader:
        if (aNeighbor == nullptr || !aNeighbor->IsStateValid())
        {
            SuccessOrExit(error = message->AppendTlvRequestTlv(routerTlvs, sizeof(routerTlvs)));
        }
        else
        {
            SuccessOrExit(error = message->AppendTlvRequestTlv(validNeighborTlvs, sizeof(validNeighborTlvs)));
        }

        SuccessOrExit(error = message->AppendSourceAddressTlv());
        SuccessOrExit(error = message->AppendLeaderDataTlv());
        break;
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = message->AppendTimeRequestTlv());
#endif

    if (aNeighbor == nullptr)
    {
        mChallenge.GenerateRandom();
        mChallengeTimeout = (((2 * kMaxResponseDelay) + kStateUpdatePeriod - 1) / kStateUpdatePeriod);

        SuccessOrExit(error = message->AppendChallengeTlv(mChallenge));
        destination.SetToLinkLocalAllRoutersMulticast();
    }
    else
    {
        if (!aNeighbor->IsStateValid())
        {
            aNeighbor->GenerateChallenge();
            SuccessOrExit(error =
                              message->AppendChallengeTlv(aNeighbor->GetChallenge(), aNeighbor->GetChallengeSize()));
        }
        else
        {
            Challenge challenge;

            challenge.GenerateRandom();
            SuccessOrExit(error = message->AppendChallengeTlv(challenge));
        }

        destination.SetToLinkLocalAddress(aNeighbor->GetExtAddress());
    }

    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeLinkRequest, destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void MleRouter::HandleLinkRequest(RxInfo &aRxInfo)
{
    Error         error    = kErrorNone;
    Neighbor *    neighbor = nullptr;
    Challenge     challenge;
    uint16_t      version;
    LeaderData    leaderData;
    uint16_t      sourceAddress;
    RequestedTlvs requestedTlvs;

    Log(kMessageReceive, kTypeLinkRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(IsRouterOrLeader(), error = kErrorInvalidState);

    VerifyOrExit(!IsAttaching(), error = kErrorInvalidState);

    // Challenge
    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

    // Version
    SuccessOrExit(error = Tlv::Find<VersionTlv>(aRxInfo.mMessage, version));
    VerifyOrExit(version >= OT_THREAD_VERSION_1_1, error = kErrorParse);

    // Leader Data
    switch (aRxInfo.mMessage.ReadLeaderDataTlv(leaderData))
    {
    case kErrorNone:
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), error = kErrorInvalidState);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    // Source Address
    switch (Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress))
    {
    case kErrorNone:
        if (IsActiveRouter(sourceAddress))
        {
            Mac::ExtAddress extAddr;

            aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);

            neighbor = mRouterTable.GetRouter(RouterIdFromRloc16(sourceAddress));
            VerifyOrExit(neighbor != nullptr, error = kErrorParse);
            VerifyOrExit(!neighbor->IsStateLinkRequest(), error = kErrorAlready);

            if (!neighbor->IsStateValid())
            {
                neighbor->SetExtAddress(extAddr);
                neighbor->GetLinkInfo().Clear();
                neighbor->GetLinkInfo().AddRss(aRxInfo.mMessageInfo.GetThreadLinkInfo()->GetRss());
                neighbor->ResetLinkFailures();
                neighbor->SetLastHeard(TimerMilli::GetNow());
                neighbor->SetState(Neighbor::kStateLinkRequest);
            }
            else
            {
                VerifyOrExit(neighbor->GetExtAddress() == extAddr);
            }
        }

        break;

    case kErrorNotFound:
        // lack of source address indicates router coming out of reset
        VerifyOrExit(aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid() &&
                         IsActiveRouter(aRxInfo.mNeighbor->GetRloc16()),
                     error = kErrorDrop);
        neighbor = aRxInfo.mNeighbor;
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // TLV Request
    switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvs))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        requestedTlvs.mNumTlvs = 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (neighbor != nullptr)
    {
        neighbor->SetTimeSyncEnabled(Tlv::Find<TimeRequestTlv>(aRxInfo.mMessage, nullptr, 0) == kErrorNone);
    }
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (neighbor != nullptr)
    {
        neighbor->ClearLastRxFragmentTag();
    }
#endif

    SuccessOrExit(error = SendLinkAccept(aRxInfo.mMessageInfo, neighbor, requestedTlvs, challenge));

exit:
    LogProcessError(kTypeLinkRequest, error);
}

Error MleRouter::SendLinkAccept(const Ip6::MessageInfo &aMessageInfo,
                                Neighbor *              aNeighbor,
                                const RequestedTlvs &   aRequestedTlvs,
                                const Challenge &       aChallenge)
{
    Error                error        = kErrorNone;
    static const uint8_t routerTlvs[] = {Tlv::kLinkMargin};
    TxMessage *          message;
    Command              command;
    uint8_t              linkMargin;

    command = (aNeighbor == nullptr || aNeighbor->IsStateValid()) ? kCommandLinkAccept : kCommandLinkAcceptAndRequest;

    VerifyOrExit((message = NewMleMessage(command)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendVersionTlv());
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendResponseTlv(aChallenge));
    SuccessOrExit(error = message->AppendLinkFrameCounterTlv());
    SuccessOrExit(error = message->AppendMleFrameCounterTlv());

    // always append a link margin, regardless of whether or not it was requested
    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(),
                                                         aMessageInfo.GetThreadLinkInfo()->GetRss());

    SuccessOrExit(error = message->AppendLinkMarginTlv(linkMargin));

    if (aNeighbor != nullptr && IsActiveRouter(aNeighbor->GetRloc16()))
    {
        SuccessOrExit(error = message->AppendLeaderDataTlv());
    }

    for (uint8_t i = 0; i < aRequestedTlvs.mNumTlvs; i++)
    {
        switch (aRequestedTlvs.mTlvs[i])
        {
        case Tlv::kRoute:
            SuccessOrExit(error = message->AppendRouteTlv(aNeighbor));
            break;

        case Tlv::kAddress16:
            VerifyOrExit(aNeighbor != nullptr, error = kErrorDrop);
            SuccessOrExit(error = message->AppendAddress16Tlv(aNeighbor->GetRloc16()));
            break;

        case Tlv::kLinkMargin:
            break;

        default:
            ExitNow(error = kErrorDrop);
        }
    }

    if (aNeighbor != nullptr && !aNeighbor->IsStateValid())
    {
        aNeighbor->GenerateChallenge();

        SuccessOrExit(error = message->AppendChallengeTlv(aNeighbor->GetChallenge(), aNeighbor->GetChallengeSize()));
        SuccessOrExit(error = message->AppendTlvRequestTlv(routerTlvs, sizeof(routerTlvs)));
        aNeighbor->SetLastHeard(TimerMilli::GetNow());
        aNeighbor->SetState(Neighbor::kStateLinkRequest);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aNeighbor != nullptr && aNeighbor->IsTimeSyncEnabled())
    {
        message->SetTimeSync(true);
    }
#endif

    if (aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(error = message->SendAfterDelay(aMessageInfo.GetPeerAddr(),
                                                      1 + Random::NonCrypto::GetUint16InRange(0, kMaxResponseDelay)));

        Log(kMessageDelay, kTypeLinkAccept, aMessageInfo.GetPeerAddr());
    }
    else
    {
        SuccessOrExit(error = message->SendTo(aMessageInfo.GetPeerAddr()));

        Log(kMessageSend, kTypeLinkAccept, aMessageInfo.GetPeerAddr());
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

void MleRouter::HandleLinkAccept(RxInfo &aRxInfo)
{
    Error error = HandleLinkAccept(aRxInfo, false);

    LogProcessError(kTypeLinkAccept, error);
}

void MleRouter::HandleLinkAcceptAndRequest(RxInfo &aRxInfo)
{
    Error error = HandleLinkAccept(aRxInfo, true);

    LogProcessError(kTypeLinkAcceptAndRequest, error);
}

Error MleRouter::HandleLinkAccept(RxInfo &aRxInfo, bool aRequest)
{
    static const uint8_t dataRequestTlvs[] = {Tlv::kNetworkData};

    Error           error = kErrorNone;
    Router *        router;
    Neighbor::State neighborState;
    Mac::ExtAddress extAddr;
    uint16_t        version;
    Challenge       response;
    uint16_t        sourceAddress;
    uint32_t        linkFrameCounter;
    uint32_t        mleFrameCounter;
    uint8_t         routerId;
    uint16_t        address16;
    RouteTlv        routeTlv;
    LeaderData      leaderData;
    uint8_t         linkMargin;

    // Source Address
    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, aRequest ? kTypeLinkAcceptAndRequest : kTypeLinkAccept, aRxInfo.mMessageInfo.GetPeerAddr(),
        sourceAddress);

    VerifyOrExit(IsActiveRouter(sourceAddress), error = kErrorParse);

    routerId      = RouterIdFromRloc16(sourceAddress);
    router        = mRouterTable.GetRouter(routerId);
    neighborState = (router != nullptr) ? router->GetState() : Neighbor::kStateInvalid;

    // Response
    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));

    // verify response
    switch (neighborState)
    {
    case Neighbor::kStateLinkRequest:
        VerifyOrExit(response.Matches(router->GetChallenge(), router->GetChallengeSize()), error = kErrorSecurity);
        break;

    case Neighbor::kStateInvalid:
        VerifyOrExit((mChallengeTimeout > 0) && (response == mChallenge), error = kErrorSecurity);

        OT_FALL_THROUGH;

    case Neighbor::kStateValid:
        break;

    default:
        ExitNow(error = kErrorSecurity);
    }

    // Remove stale neighbors
    if (aRxInfo.mNeighbor && aRxInfo.mNeighbor->GetRloc16() != sourceAddress)
    {
        RemoveNeighbor(*aRxInfo.mNeighbor);
    }

    // Version
    SuccessOrExit(error = Tlv::Find<VersionTlv>(aRxInfo.mMessage, version));
    VerifyOrExit(version >= OT_THREAD_VERSION_1_1, error = kErrorParse);

    // Link and MLE Frame Counters
    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));

    // Link Margin
    switch (Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMargin))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        // Link Margin TLV may be skipped in Router Synchronization process after Reset
        VerifyOrExit(IsDetached(), error = kErrorNotFound);
        // Wait for an MLE Advertisement to establish a routing cost to the neighbor
        linkMargin = 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    switch (mRole)
    {
    case kRoleDisabled:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);

    case kRoleDetached:
        // Address16
        SuccessOrExit(error = Tlv::Find<Address16Tlv>(aRxInfo.mMessage, address16));
        VerifyOrExit(GetRloc16() == address16, error = kErrorDrop);

        // Leader Data
        SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));
        SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

        // Route
        mRouterTable.Clear();
        SuccessOrExit(error = ProcessRouteTlv(aRxInfo));
        router = mRouterTable.GetRouter(routerId);
        VerifyOrExit(router != nullptr);

        if (mLeaderData.GetLeaderRouterId() == RouterIdFromRloc16(GetRloc16()))
        {
            SetStateLeader(GetRloc16());
        }
        else
        {
            SetStateRouter(GetRloc16());
        }

        mRetrieveNewNetworkData = true;
        IgnoreError(SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr(), dataRequestTlvs, sizeof(dataRequestTlvs), 0));

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);
#endif
        break;

    case kRoleChild:
        VerifyOrExit(router != nullptr);
        break;

    case kRoleRouter:
    case kRoleLeader:
        VerifyOrExit(router != nullptr);

        // Leader Data
        SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());

        if (mRetrieveNewNetworkData ||
            SerialNumber::IsGreater(leaderData.GetDataVersion(NetworkData::kFullSet),
                                    Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet)))
        {
            IgnoreError(
                SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr(), dataRequestTlvs, sizeof(dataRequestTlvs), 0));
        }

        // Route (optional)
        switch (error = ProcessRouteTlv(aRxInfo, routeTlv))
        {
        case kErrorNone:
            UpdateRoutes(routeTlv, routerId);
            // Need to update router after ProcessRouteTlv
            router = mRouterTable.GetRouter(routerId);
            OT_ASSERT(router != nullptr);
            break;

        case kErrorNotFound:
            error = kErrorNone;
            break;

        default:
            ExitNow();
        }

        // update routing table
        if (routerId != mRouterId && !IsRouterIdValid(router->GetNextHop()))
        {
            ResetAdvertiseInterval();
        }

        break;
    }

    // finish link synchronization
    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);
    router->SetExtAddress(extAddr);
    router->SetRloc16(sourceAddress);
    router->GetLinkFrameCounters().SetAll(linkFrameCounter);
    router->SetLinkAckFrameCounter(linkFrameCounter);
    router->SetMleFrameCounter(mleFrameCounter);
    router->SetLastHeard(TimerMilli::GetNow());
    router->SetVersion(static_cast<uint8_t>(version));
    router->SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                     DeviceMode::kModeFullNetworkData));
    router->GetLinkInfo().Clear();
    router->GetLinkInfo().AddRss(aRxInfo.mMessageInfo.GetThreadLinkInfo()->GetRss());
    router->SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin));
    router->ResetLinkFailures();
    router->SetState(Neighbor::kStateValid);
    router->SetKeySequence(aRxInfo.mKeySequence);

    mNeighborTable.Signal(NeighborTable::kRouterAdded, *router);

    if (aRequest)
    {
        Challenge     challenge;
        RequestedTlvs requestedTlvs;

        // Challenge
        SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

        // TLV Request
        switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvs))
        {
        case kErrorNone:
            break;
        case kErrorNotFound:
            requestedTlvs.mNumTlvs = 0;
            break;
        default:
            ExitNow(error = kErrorParse);
        }

        SuccessOrExit(error = SendLinkAccept(aRxInfo.mMessageInfo, router, requestedTlvs, challenge));
    }

exit:
    return error;
}

uint8_t MleRouter::LinkQualityToCost(uint8_t aLinkQuality)
{
    uint8_t rval;

    switch (aLinkQuality)
    {
    case 1:
        rval = kLinkQuality1LinkCost;
        break;

    case 2:
        rval = kLinkQuality2LinkCost;
        break;

    case 3:
        rval = kLinkQuality3LinkCost;
        break;

    default:
        rval = kLinkQuality0LinkCost;
        break;
    }

    return rval;
}

uint8_t MleRouter::GetLinkCost(uint8_t aRouterId)
{
    uint8_t rval = kMaxRouteCost;
    Router *router;

    router = mRouterTable.GetRouter(aRouterId);

    // `nullptr` aRouterId indicates non-existing next hop, hence return kMaxRouteCost for it.
    VerifyOrExit(router != nullptr);

    rval = mRouterTable.GetLinkCost(*router);

exit:
    return rval;
}

Error MleRouter::SetRouterSelectionJitter(uint8_t aRouterJitter)
{
    Error error = kErrorNone;

    VerifyOrExit(aRouterJitter > 0, error = kErrorInvalidArgs);

    mRouterSelectionJitter = aRouterJitter;

exit:
    return error;
}

Error MleRouter::ProcessRouteTlv(RxInfo &aRxInfo)
{
    RouteTlv routeTlv;

    return ProcessRouteTlv(aRxInfo, routeTlv);
}

Error MleRouter::ProcessRouteTlv(RxInfo &aRxInfo, RouteTlv &aRouteTlv)
{
    // This method processes Route TLV in a received MLE message
    // (from `RxInfo`). In case of success, `aRouteTlv` is updated
    // to return the read/processed route TLV from the message.
    // If the message contains no Route TLV, `kErrorNotFound` is
    // returned.
    //
    // During processing of Route TLV, the entries in the router table
    // may shuffle. This method ensures that the `aRxInfo.mNeighbor`
    // (which indicates the neighbor from which the MLE message was
    // received) is correctly updated to point to the same neighbor
    // (in case `mNeighbor` was pointing to a router entry from the
    // `RouterTable`).

    Error    error;
    uint16_t neighborRloc16 = Mac::kShortAddrInvalid;

    if ((aRxInfo.mNeighbor != nullptr) && Get<RouterTable>().Contains(*aRxInfo.mNeighbor))
    {
        neighborRloc16 = aRxInfo.mNeighbor->GetRloc16();
    }

    SuccessOrExit(error = Tlv::FindTlv(aRxInfo.mMessage, aRouteTlv));

    VerifyOrExit(aRouteTlv.IsValid(), error = kErrorParse);

    Get<RouterTable>().UpdateRouterIdSet(aRouteTlv.GetRouterIdSequence(), aRouteTlv.GetRouterIdMask());

    if (IsRouter() && !Get<RouterTable>().IsAllocated(mRouterId))
    {
        IgnoreError(BecomeDetached());
        error = kErrorNoRoute;
    }

    if (neighborRloc16 != Mac::kShortAddrInvalid)
    {
        aRxInfo.mNeighbor = Get<RouterTable>().GetNeighbor(neighborRloc16);
    }

exit:
    return error;
}

bool MleRouter::IsSingleton(void)
{
    bool rval = true;

    if (IsAttached() && IsRouterEligible())
    {
        // not a singleton if any other routers exist
        if (mRouterTable.GetActiveRouterCount() > 1)
        {
            ExitNow(rval = false);
        }
    }

exit:
    return rval;
}

int MleRouter::ComparePartitions(bool              aSingletonA,
                                 const LeaderData &aLeaderDataA,
                                 bool              aSingletonB,
                                 const LeaderData &aLeaderDataB)
{
    int rval = 0;

    if (aLeaderDataA.GetWeighting() != aLeaderDataB.GetWeighting())
    {
        ExitNow(rval = aLeaderDataA.GetWeighting() > aLeaderDataB.GetWeighting() ? 1 : -1);
    }

    if (aSingletonA != aSingletonB)
    {
        ExitNow(rval = aSingletonB ? 1 : -1);
    }

    if (aLeaderDataA.GetPartitionId() != aLeaderDataB.GetPartitionId())
    {
        ExitNow(rval = aLeaderDataA.GetPartitionId() > aLeaderDataB.GetPartitionId() ? 1 : -1);
    }

exit:
    return rval;
}

bool MleRouter::IsSingleton(const RouteTlv &aRouteTlv)
{
    bool    rval  = true;
    uint8_t count = 0;

    // REEDs do not include a Route TLV and indicate not a singleton
    if (!aRouteTlv.IsValid())
    {
        ExitNow(rval = false);
    }

    // Check if 2 or more active routers
    for (uint8_t routerId = 0; routerId <= kMaxRouterId; routerId++)
    {
        if (aRouteTlv.IsRouterIdSet(routerId) && (++count >= 2))
        {
            ExitNow(rval = false);
        }
    }

exit:
    return rval;
}

Error MleRouter::HandleAdvertisement(RxInfo &aRxInfo)
{
    Error                 error    = kErrorNone;
    const ThreadLinkInfo *linkInfo = aRxInfo.mMessageInfo.GetThreadLinkInfo();
    uint8_t linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->GetRss());
    Mac::ExtAddress extAddr;
    uint16_t        sourceAddress = Mac::kShortAddrInvalid;
    LeaderData      leaderData;
    RouteTlv        route;
    uint32_t        partitionId;
    Router *        router;
    uint8_t         routerId;
    uint8_t         routerCount;

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);

    // Source Address
    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    // Leader Data
    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

    // Route Data (optional)
    if (Tlv::FindTlv(aRxInfo.mMessage, route) == kErrorNone)
    {
        VerifyOrExit(route.IsValid(), error = kErrorParse);
    }
    else
    {
        // mark that a Route TLV was not included
        route.SetLength(0);
    }

    partitionId = leaderData.GetPartitionId();

    if (partitionId != mLeaderData.GetPartitionId())
    {
        LogNote("Different partition (peer:%u, local:%u)", partitionId, mLeaderData.GetPartitionId());

        VerifyOrExit(linkMargin >= OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN, error = kErrorLinkMarginLow);

        if (route.IsValid() && IsFullThreadDevice() && (mPreviousPartitionIdTimeout > 0) &&
            (partitionId == mPreviousPartitionId))
        {
            VerifyOrExit(SerialNumber::IsGreater(route.GetRouterIdSequence(), mPreviousPartitionRouterIdSequence),
                         error = kErrorDrop);
        }

        if (IsChild() && (aRxInfo.mNeighbor == &mParent || !IsFullThreadDevice()))
        {
            ExitNow();
        }

        if (ComparePartitions(IsSingleton(route), leaderData, IsSingleton(), mLeaderData) > 0
#if OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED
            // if time sync is required, it will only migrate to a better network which also enables time sync.
            && aRxInfo.mMessage.GetTimeSyncSeq() != OT_TIME_SYNC_INVALID_SEQ
#endif
        )
        {
            Attach(kBetterPartition);
        }

        ExitNow(error = kErrorDrop);
    }
    else if (leaderData.GetLeaderRouterId() != GetLeaderId())
    {
        VerifyOrExit(aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid());

        if (!IsChild())
        {
            LogInfo("Leader ID mismatch");
            IgnoreError(BecomeDetached());
            error = kErrorDrop;
        }

        ExitNow();
    }

    VerifyOrExit(IsActiveRouter(sourceAddress) && route.IsValid());
    routerId = RouterIdFromRloc16(sourceAddress);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);
#endif

    if (IsFullThreadDevice() && (aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid()) &&
        ((mRouterTable.GetActiveRouterCount() == 0) ||
         SerialNumber::IsGreater(route.GetRouterIdSequence(), mRouterTable.GetRouterIdSequence())))
    {
        bool processRouteTlv = false;

        switch (mRole)
        {
        case kRoleDisabled:
        case kRoleDetached:
            break;

        case kRoleChild:
            if (sourceAddress == mParent.GetRloc16())
            {
                processRouteTlv = true;
            }
            else
            {
                router = mRouterTable.GetRouter(routerId);

                if (router != nullptr && router->IsStateValid())
                {
                    processRouteTlv = true;
                }
            }

            break;

        case kRoleRouter:
        case kRoleLeader:
            processRouteTlv = true;
            break;
        }

        if (processRouteTlv)
        {
            SuccessOrExit(error = ProcessRouteTlv(aRxInfo));
        }
    }

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        ExitNow();

    case kRoleChild:
        if (aRxInfo.mNeighbor == &mParent)
        {
            // MLE Advertisement from parent
            router = &mParent;

            if (mParent.GetRloc16() != sourceAddress)
            {
                IgnoreError(BecomeDetached());
                ExitNow(error = kErrorDetached);
            }

            if (IsFullThreadDevice())
            {
                Router *leader;

                if ((mRouterSelectionJitterTimeout == 0) &&
                    (mRouterTable.GetActiveRouterCount() < mRouterUpgradeThreshold))
                {
                    mRouterSelectionJitterTimeout = 1 + Random::NonCrypto::GetUint8InRange(0, mRouterSelectionJitter);
                    ExitNow();
                }

                leader = mRouterTable.GetLeader();

                if (leader != nullptr)
                {
                    for (uint8_t id = 0, routeCount = 0; id <= kMaxRouterId; id++)
                    {
                        if (!route.IsRouterIdSet(id))
                        {
                            continue;
                        }

                        if (id != GetLeaderId())
                        {
                            routeCount++;
                            continue;
                        }

                        if (route.GetRouteCost(routeCount) > 0)
                        {
                            leader->SetNextHop(id);
                            leader->SetCost(route.GetRouteCost(routeCount));
                        }
                        else
                        {
                            leader->SetNextHop(kInvalidRouterId);
                            leader->SetCost(0);
                        }

                        break;
                    }
                }
            }
        }
        else
        {
            // MLE Advertisement not from parent, but from some other neighboring router
            router = mRouterTable.GetRouter(routerId);
            VerifyOrExit(router != nullptr);

            if (IsFullThreadDevice() && !router->IsStateValid() && !router->IsStateLinkRequest() &&
                (mRouterTable.GetActiveLinkCount() < OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS))
            {
                router->SetExtAddress(extAddr);
                router->GetLinkInfo().Clear();
                router->GetLinkInfo().AddRss(linkInfo->GetRss());
                router->ResetLinkFailures();
                router->SetLastHeard(TimerMilli::GetNow());
                router->SetState(Neighbor::kStateLinkRequest);
                IgnoreError(SendLinkRequest(router));
                ExitNow(error = kErrorNoRoute);
            }
        }

        router->SetLastHeard(TimerMilli::GetNow());

        ExitNow();

    case kRoleRouter:
        if (mLinkRequestDelay > 0 && route.IsRouterIdSet(mRouterId))
        {
            mLinkRequestDelay = 0;
            IgnoreError(SendLinkRequest(nullptr));
        }

        router = mRouterTable.GetRouter(routerId);
        VerifyOrExit(router != nullptr);

        // check current active router number
        routerCount = 0;

        for (uint8_t id = 0; id <= kMaxRouterId; id++)
        {
            if (route.IsRouterIdSet(id))
            {
                routerCount++;
            }
        }

        if (routerCount > mRouterDowngradeThreshold && mRouterSelectionJitterTimeout == 0 &&
            HasMinDowngradeNeighborRouters() && HasSmallNumberOfChildren() &&
            HasOneNeighborWithComparableConnectivity(route, routerId))
        {
            mRouterSelectionJitterTimeout = 1 + Random::NonCrypto::GetUint8InRange(0, mRouterSelectionJitter);
        }

        OT_FALL_THROUGH;

    case kRoleLeader:
        router = mRouterTable.GetRouter(routerId);
        VerifyOrExit(router != nullptr);

        // Send unicast link request if no link to router and no unicast/multicast link request in progress
        if (!router->IsStateValid() && !router->IsStateLinkRequest() && (mChallengeTimeout == 0) &&
            (linkMargin >= OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN))
        {
            router->SetExtAddress(extAddr);
            router->GetLinkInfo().Clear();
            router->GetLinkInfo().AddRss(linkInfo->GetRss());
            router->ResetLinkFailures();
            router->SetLastHeard(TimerMilli::GetNow());
            router->SetState(Neighbor::kStateLinkRequest);
            IgnoreError(SendLinkRequest(router));
            ExitNow(error = kErrorNoRoute);
        }

        router->SetLastHeard(TimerMilli::GetNow());
        break;
    }

    UpdateRoutes(route, routerId);

exit:
    if (aRxInfo.mNeighbor && aRxInfo.mNeighbor->GetRloc16() != sourceAddress)
    {
        // Remove stale neighbors
        RemoveNeighbor(*aRxInfo.mNeighbor);
    }

    return error;
}

void MleRouter::UpdateRoutes(const RouteTlv &aRoute, uint8_t aRouterId)
{
    Router *neighbor;
    bool    resetAdvInterval = false;
    bool    changed          = false;

    neighbor = mRouterTable.GetRouter(aRouterId);
    VerifyOrExit(neighbor != nullptr);

    // update link quality out to neighbor
    changed = UpdateLinkQualityOut(aRoute, *neighbor, resetAdvInterval);

    // update routes
    for (uint8_t routerId = 0, routeCount = 0; routerId <= kMaxRouterId; routerId++)
    {
        Router *router;
        Router *nextHop;
        uint8_t oldNextHop;
        uint8_t cost;

        if (!aRoute.IsRouterIdSet(routerId))
        {
            continue;
        }

        router = mRouterTable.GetRouter(routerId);

        if (router == nullptr || router->GetRloc16() == GetRloc16() || router == neighbor)
        {
            routeCount++;
            continue;
        }

        oldNextHop = router->GetNextHop();
        nextHop    = mRouterTable.GetRouter(oldNextHop);

        cost = aRoute.GetRouteCost(routeCount);

        if (cost == 0)
        {
            cost = kMaxRouteCost;
        }

        if (nextHop == nullptr || nextHop == neighbor)
        {
            // router has no next hop or next hop is neighbor (sender)

            if (cost + mRouterTable.GetLinkCost(*neighbor) < kMaxRouteCost)
            {
                if (nextHop == nullptr && mRouterTable.GetLinkCost(*router) >= kMaxRouteCost)
                {
                    resetAdvInterval = true;
                }

                if (router->GetNextHop() != aRouterId)
                {
                    router->SetNextHop(aRouterId);
                    changed = true;
                }

                if (router->GetCost() != cost)
                {
                    router->SetCost(cost);
                    changed = true;
                }
            }
            else if (nextHop == neighbor)
            {
                if (mRouterTable.GetLinkCost(*router) >= kMaxRouteCost)
                {
                    resetAdvInterval = true;
                }

                router->SetNextHop(kInvalidRouterId);
                router->SetCost(0);
                router->SetLastHeard(TimerMilli::GetNow());
                changed = true;
            }
        }
        else
        {
            uint8_t curCost = router->GetCost() + mRouterTable.GetLinkCost(*nextHop);
            uint8_t newCost = cost + mRouterTable.GetLinkCost(*neighbor);

            if (newCost < curCost)
            {
                router->SetNextHop(aRouterId);
                router->SetCost(cost);
                changed = true;
            }
        }

        routeCount++;
    }

    if (resetAdvInterval)
    {
        ResetAdvertiseInterval();
    }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

    VerifyOrExit(changed);
    LogInfo("Route table updated");

    for (Router &router : Get<RouterTable>().Iterate())
    {
        LogInfo("    %04x -> %04x, cost:%d %d, lqin:%d, lqout:%d, link:%s", router.GetRloc16(),
                (router.GetNextHop() == kInvalidRouterId) ? 0xffff : Rloc16FromRouterId(router.GetNextHop()),
                router.GetCost(), mRouterTable.GetLinkCost(router), router.GetLinkInfo().GetLinkQuality(),
                router.GetLinkQualityOut(),
                router.GetRloc16() == GetRloc16() ? "device" : ToYesNo(router.IsStateValid()));
    }

#else
    OT_UNUSED_VARIABLE(changed);
#endif

exit:
    return;
}

bool MleRouter::UpdateLinkQualityOut(const RouteTlv &aRoute, Router &aNeighbor, bool &aResetAdvInterval)
{
    bool        changed = false;
    LinkQuality linkQuality;
    uint8_t     myRouterId;
    uint8_t     myRouteCount;
    uint8_t     oldLinkCost;
    Router *    nextHop;

    myRouterId = RouterIdFromRloc16(GetRloc16());
    VerifyOrExit(aRoute.IsRouterIdSet(myRouterId));

    myRouteCount = 0;
    for (uint8_t routerId = 0; routerId < myRouterId; routerId++)
    {
        myRouteCount += aRoute.IsRouterIdSet(routerId);
    }

    linkQuality = aRoute.GetLinkQualityIn(myRouteCount);
    VerifyOrExit(aNeighbor.GetLinkQualityOut() != linkQuality);

    oldLinkCost = mRouterTable.GetLinkCost(aNeighbor);

    aNeighbor.SetLinkQualityOut(linkQuality);
    nextHop = mRouterTable.GetRouter(aNeighbor.GetNextHop());

    // reset MLE advertisement timer if neighbor route cost changed to or from infinite
    if (nextHop == nullptr && (oldLinkCost >= kMaxRouteCost) != (mRouterTable.GetLinkCost(aNeighbor) >= kMaxRouteCost))
    {
        aResetAdvInterval = true;
    }
    changed = true;

exit:
    return changed;
}

void MleRouter::HandleParentRequest(RxInfo &aRxInfo)
{
    Error           error = kErrorNone;
    Mac::ExtAddress extAddr;
    uint16_t        version;
    uint8_t         scanMask;
    Challenge       challenge;
    Router *        leader;
    Child *         child;
    uint8_t         modeBitmask;
    DeviceMode      mode;

    Log(kMessageReceive, kTypeParentRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(IsRouterEligible(), error = kErrorInvalidState);

    // A Router/REED MUST NOT send an MLE Parent Response if:

    // 0. It is detached or attempting to another partition
    VerifyOrExit(!IsDetached() && !IsAttaching(), error = kErrorDrop);

    // 1. It has no available Child capacity (if Max Child Count minus
    // Child Count would be equal to zero)
    // ==> verified below when allocating a child entry

    // 2. It is disconnected from its Partition (that is, it has not
    // received an updated ID sequence number within LEADER_TIMEOUT
    // seconds)
    VerifyOrExit(mRouterTable.GetLeaderAge() < mNetworkIdTimeout, error = kErrorDrop);

    // 3. Its current routing path cost to the Leader is infinite.
    leader = mRouterTable.GetLeader();
    OT_ASSERT(leader != nullptr);

    VerifyOrExit(IsLeader() || GetLinkCost(GetLeaderId()) < kMaxRouteCost ||
                     (IsChild() && leader->GetCost() + 1 < kMaxRouteCost) ||
                     (leader->GetCost() + GetLinkCost(leader->GetNextHop()) < kMaxRouteCost),
                 error = kErrorDrop);

    // 4. It is a REED and there are already `kMaxRouters` active routers in
    // the network (because Leader would reject any further address solicit).
    // ==> Verified below when checking the scan mask.

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);

    // Version
    SuccessOrExit(error = Tlv::Find<VersionTlv>(aRxInfo.mMessage, version));
    VerifyOrExit(version >= OT_THREAD_VERSION_1_1, error = kErrorParse);

    // Scan Mask
    SuccessOrExit(error = Tlv::Find<ScanMaskTlv>(aRxInfo.mMessage, scanMask));

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        ExitNow();

    case kRoleChild:
        VerifyOrExit(ScanMaskTlv::IsEndDeviceFlagSet(scanMask));
        VerifyOrExit(mRouterTable.GetActiveRouterCount() < kMaxRouters, error = kErrorDrop);
        break;

    case kRoleRouter:
    case kRoleLeader:
        VerifyOrExit(ScanMaskTlv::IsRouterFlagSet(scanMask));
        break;
    }

    // Challenge
    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

    child = mChildTable.FindChild(extAddr, Child::kInStateAnyExceptInvalid);

    if (child == nullptr)
    {
        VerifyOrExit((child = mChildTable.GetNewChild()) != nullptr, error = kErrorNoBufs);

        // MAC Address
        child->SetExtAddress(extAddr);
        child->GetLinkInfo().Clear();
        child->GetLinkInfo().AddRss(aRxInfo.mMessageInfo.GetThreadLinkInfo()->GetRss());
        child->ResetLinkFailures();
        child->SetState(Neighbor::kStateParentRequest);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        child->SetTimeSyncEnabled(Tlv::Find<TimeRequestTlv>(aRxInfo.mMessage, nullptr, 0) == kErrorNone);
#endif
        if (Tlv::Find<ModeTlv>(aRxInfo.mMessage, modeBitmask) == kErrorNone)
        {
            mode.Set(modeBitmask);
            child->SetDeviceMode(mode);
            child->SetVersion(static_cast<uint8_t>(version));
        }
    }
    else if (TimerMilli::GetNow() - child->GetLastHeard() < kParentRequestRouterTimeout - kParentRequestDuplicateMargin)
    {
        ExitNow(error = kErrorDuplicated);
    }

    if (!child->IsStateValidOrRestoring())
    {
        child->SetLastHeard(TimerMilli::GetNow());
        child->SetTimeout(Time::MsecToSec(kMaxChildIdRequestTimeout));
    }

    SendParentResponse(child, challenge, !ScanMaskTlv::IsEndDeviceFlagSet(scanMask));

exit:
    LogProcessError(kTypeParentRequest, error);
}

bool MleRouter::HasNeighborWithGoodLinkQuality(void) const
{
    bool    haveNeighbor = true;
    uint8_t linkMargin;

    linkMargin =
        LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), mParent.GetLinkInfo().GetLastRss());

    if (linkMargin >= OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN)
    {
        ExitNow();
    }

    for (Router &router : Get<RouterTable>().Iterate())
    {
        if (!router.IsStateValid())
        {
            continue;
        }

        linkMargin =
            LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), router.GetLinkInfo().GetLastRss());

        if (linkMargin >= OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN)
        {
            ExitNow();
        }
    }

    haveNeighbor = false;

exit:
    return haveNeighbor;
}

void MleRouter::HandleTimeTick(void)
{
    bool routerStateUpdate = false;

    VerifyOrExit(IsFullThreadDevice(), Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMleRouter));

    if (mLinkRequestDelay > 0 && --mLinkRequestDelay == 0)
    {
        IgnoreError(SendLinkRequest(nullptr));
    }

    if (mChallengeTimeout > 0)
    {
        mChallengeTimeout--;
    }

    if (mPreviousPartitionIdTimeout > 0)
    {
        mPreviousPartitionIdTimeout--;
    }

    if (mRouterSelectionJitterTimeout > 0)
    {
        mRouterSelectionJitterTimeout--;

        if (mRouterSelectionJitterTimeout == 0)
        {
            routerStateUpdate = true;
        }
    }
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    // Delay register only when `mRouterSelectionJitterTimeout` is 0,
    // that is, when the device has decided to stay as REED or Router.
    else if (mBackboneRouterRegistrationDelay > 0)
    {
        mBackboneRouterRegistrationDelay--;

        if (mBackboneRouterRegistrationDelay == 0)
        {
            // If no Backbone Router service after jitter, try to register its own Backbone Router Service.
            if (!Get<BackboneRouter::Leader>().HasPrimary())
            {
                if (Get<BackboneRouter::Local>().AddService() == kErrorNone)
                {
                    Get<NetworkData::Notifier>().HandleServerDataUpdated();
                }
            }
        }
    }
#endif

    switch (mRole)
    {
    case kRoleDisabled:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);

    case kRoleDetached:
        if (mChallengeTimeout == 0)
        {
            IgnoreError(BecomeDetached());
            ExitNow();
        }

        break;

    case kRoleChild:
        if (routerStateUpdate)
        {
            if (mRouterTable.GetActiveRouterCount() < mRouterUpgradeThreshold && HasNeighborWithGoodLinkQuality())
            {
                // upgrade to Router
                IgnoreError(BecomeRouter(ThreadStatusTlv::kTooFewRouters));
            }
            else
            {
                // send announce after decided to stay in REED if needed
                InformPreviousChannel();
            }

            if (!mAdvertiseTrickleTimer.IsRunning())
            {
                SendAdvertisement();

                mAdvertiseTrickleTimer.Start(TrickleTimer::kModePlainTimer, Time::SecToMsec(kReedAdvertiseInterval),
                                             Time::SecToMsec(kReedAdvertiseInterval + kReedAdvertiseJitter));
            }

            ExitNow();
        }

        OT_FALL_THROUGH;

    case kRoleRouter:
        // verify path to leader
        LogDebg("network id timeout = %d", mRouterTable.GetLeaderAge());

        if ((mRouterTable.GetActiveRouterCount() > 0) && (mRouterTable.GetLeaderAge() >= mNetworkIdTimeout))
        {
            LogInfo("Router ID Sequence timeout");
            Attach(kSamePartition);
        }

        if (routerStateUpdate && mRouterTable.GetActiveRouterCount() > mRouterDowngradeThreshold)
        {
            LogNote("Downgrade to REED");
            Attach(kDowngradeToReed);
        }

        break;

    case kRoleLeader:
        break;
    }

    // update children state
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        uint32_t timeout = 0;

        switch (child.GetState())
        {
        case Neighbor::kStateInvalid:
        case Neighbor::kStateChildIdRequest:
            continue;

        case Neighbor::kStateParentRequest:
        case Neighbor::kStateValid:
        case Neighbor::kStateRestored:
        case Neighbor::kStateChildUpdateRequest:
            timeout = Time::SecToMsec(child.GetTimeout());
            break;

        case Neighbor::kStateParentResponse:
        case Neighbor::kStateLinkRequest:
            OT_ASSERT(false);
            OT_UNREACHABLE_CODE(break);
        }

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        if (child.IsCslSynchronized() &&
            TimerMilli::GetNow() - child.GetCslLastHeard() >= Time::SecToMsec(child.GetCslTimeout()))
        {
            LogInfo("Child CSL synchronization expired");
            child.SetCslSynchronized(false);
            Get<CslTxScheduler>().Update();
        }
#endif

        if (TimerMilli::GetNow() - child.GetLastHeard() >= timeout)
        {
            LogInfo("Child timeout expired");
            RemoveNeighbor(child);
        }
        else if (IsRouterOrLeader() && child.IsStateRestored())
        {
            IgnoreError(SendChildUpdateRequest(child));
        }
    }

    // update router state
    for (Router &router : Get<RouterTable>().Iterate())
    {
        uint32_t age;

        if (router.GetRloc16() == GetRloc16())
        {
            router.SetLastHeard(TimerMilli::GetNow());
            continue;
        }

        age = TimerMilli::GetNow() - router.GetLastHeard();

        if (router.IsStateValid())
        {
#if OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT == 0

            if (age >= Time::SecToMsec(kMaxNeighborAge))
            {
                LogInfo("Router timeout expired");
                RemoveNeighbor(router);
                continue;
            }

#else

            if (age >= Time::SecToMsec(kMaxNeighborAge))
            {
                if (age < Time::SecToMsec(kMaxNeighborAge) + kMaxTransmissionCount * kUnicastRetransmissionDelay)
                {
                    LogInfo("Router timeout expired");
                    IgnoreError(SendLinkRequest(&router));
                }
                else
                {
                    RemoveNeighbor(router);
                    continue;
                }
            }

#endif
        }
        else if (router.IsStateLinkRequest())
        {
            if (age >= kMaxLinkRequestTimeout)
            {
                LogInfo("Link Request timeout expired");
                RemoveNeighbor(router);
                continue;
            }
        }

        if (IsLeader())
        {
            if (mRouterTable.GetRouter(router.GetNextHop()) == nullptr &&
                mRouterTable.GetLinkCost(router) >= kMaxRouteCost && age >= Time::SecToMsec(kMaxLeaderToRouterTimeout))
            {
                LogInfo("Router ID timeout expired (no route)");
                IgnoreError(mRouterTable.Release(router.GetRouterId()));
            }
        }
    }

    mRouterTable.HandleTimeTick();

    SynchronizeChildNetworkData();

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (IsRouterOrLeader())
    {
        Get<TimeSync>().ProcessTimeSync();
    }
#endif

exit:
    return;
}

void MleRouter::SendParentResponse(Child *aChild, const Challenge &aChallenge, bool aRoutersOnlyRequest)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage *  message;
    uint16_t     delay;

    VerifyOrExit((message = NewMleMessage(kCommandParentResponse)) != nullptr, error = kErrorNoBufs);
    message->SetDirectTransmission();

    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());
    SuccessOrExit(error = message->AppendLinkFrameCounterTlv());
    SuccessOrExit(error = message->AppendMleFrameCounterTlv());
    SuccessOrExit(error = message->AppendResponseTlv(aChallenge));
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aChild->IsTimeSyncEnabled())
    {
        SuccessOrExit(error = message->AppendTimeParameterTlv());
    }
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    if (aChild->IsThreadVersionCslCapable())
    {
        SuccessOrExit(error = message->AppendCslClockAccuracyTlv());
    }
#endif

    aChild->GenerateChallenge();

    SuccessOrExit(error = message->AppendChallengeTlv(aChild->GetChallenge(), aChild->GetChallengeSize()));
    error = message->AppendLinkMarginTlv(aChild->GetLinkInfo().GetLinkMargin());
    SuccessOrExit(error);

    SuccessOrExit(error = message->AppendConnectivityTlv());
    SuccessOrExit(error = message->AppendVersionTlv());

    destination.SetToLinkLocalAddress(aChild->GetExtAddress());

    if (aRoutersOnlyRequest)
    {
        delay = 1 + Random::NonCrypto::GetUint16InRange(0, kParentResponseMaxDelayRouters);
    }
    else
    {
        delay = 1 + Random::NonCrypto::GetUint16InRange(0, kParentResponseMaxDelayAll);
    }

    SuccessOrExit(error = message->SendAfterDelay(destination, delay));

    Log(kMessageDelay, kTypeParentResponse, destination);

exit:
    FreeMessageOnError(message, error);
    LogSendError(kTypeParentResponse, error);
}

uint8_t MleRouter::GetMaxChildIpAddresses(void) const
{
    uint8_t num = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (mMaxChildIpAddresses != 0)
    {
        num = mMaxChildIpAddresses;
    }
#endif

    return num;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
Error MleRouter::SetMaxChildIpAddresses(uint8_t aMaxIpAddresses)
{
    Error error = kErrorNone;

    VerifyOrExit(aMaxIpAddresses <= OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD, error = kErrorInvalidArgs);

    mMaxChildIpAddresses = aMaxIpAddresses;

exit:
    return error;
}
#endif

Error MleRouter::UpdateChildAddresses(const Message &aMessage, uint16_t aOffset, Child &aChild)
{
    Error                    error = kErrorNone;
    AddressRegistrationEntry entry;
    Ip6::Address             address;
    Lowpan::Context          context;
    Tlv                      tlv;
    uint8_t                  registeredCount = 0;
    uint8_t                  storedCount     = 0;
    uint16_t                 offset          = 0;
    uint16_t                 end             = 0;
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    Ip6::Address        oldDua;
    const Ip6::Address *oldDuaPtr = nullptr;
    bool                hasDua    = false;
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    Ip6::Address oldMlrRegisteredAddresses[OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD - 1];
    uint16_t     oldMlrRegisteredAddressNum = 0;
#endif

    SuccessOrExit(error = aMessage.Read(aOffset, tlv));
    VerifyOrExit(tlv.GetLength() <= (aMessage.GetLength() - aOffset - sizeof(tlv)), error = kErrorParse);

    offset = aOffset + sizeof(tlv);
    end    = offset + tlv.GetLength();

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    if ((oldDuaPtr = aChild.GetDomainUnicastAddress()) != nullptr)
    {
        oldDua = *oldDuaPtr;
    }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    // Retrieve registered multicast addresses of the Child
    if (aChild.HasAnyMlrRegisteredAddress())
    {
        OT_ASSERT(aChild.IsStateValid());

        for (const Ip6::Address &childAddress :
             aChild.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            if (aChild.GetAddressMlrState(childAddress) == kMlrStateRegistered)
            {
                oldMlrRegisteredAddresses[oldMlrRegisteredAddressNum++] = childAddress;
            }
        }
    }
#endif

    aChild.ClearIp6Addresses();

    while (offset < end)
    {
        uint8_t len;

        // read out the control field
        SuccessOrExit(error = aMessage.Read(offset, &entry, sizeof(uint8_t)));

        len = entry.GetLength();

        SuccessOrExit(error = aMessage.Read(offset, &entry, len));

        offset += len;
        registeredCount++;

        if (entry.IsCompressed())
        {
            if (Get<NetworkData::Leader>().GetContext(entry.GetContextId(), context) != kErrorNone)
            {
                LogWarn("Failed to get context %d for compressed address from child 0x%04x", entry.GetContextId(),
                        aChild.GetRloc16());
                continue;
            }

            address.Clear();
            address.SetPrefix(context.mPrefix);
            address.SetIid(entry.GetIid());
        }
        else
        {
            address = entry.GetIp6Address();
        }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if (mMaxChildIpAddresses > 0 && storedCount >= mMaxChildIpAddresses)
        {
            // Skip remaining address registration entries but keep logging skipped addresses.
            error = kErrorNoBufs;
        }
        else
#endif
        {
            // We try to accept/add as many IPv6 addresses as possible.
            // "Child ID/Update Response" will indicate the accepted
            // addresses.
            error = aChild.AddIp6Address(address);
        }

        if (error == kErrorNone)
        {
            storedCount++;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
            if (Get<BackboneRouter::Leader>().IsDomainUnicast(address))
            {
                hasDua = true;

                if (oldDuaPtr != nullptr)
                {
                    Get<DuaManager>().UpdateChildDomainUnicastAddress(
                        aChild, oldDua != address ? ChildDuaState::kChanged : ChildDuaState::kUnchanged);
                }
                else
                {
                    Get<DuaManager>().UpdateChildDomainUnicastAddress(aChild, ChildDuaState::kAdded);
                }
            }
#endif

            LogInfo("Child 0x%04x IPv6 address[%d]=%s", aChild.GetRloc16(), storedCount,
                    address.ToString().AsCString());
        }
        else
        {
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
            if (Get<BackboneRouter::Leader>().IsDomainUnicast(address))
            {
                // if not able to store DUA, then assume child does not have one
                hasDua = false;
            }
#endif

            LogWarn("Error %s adding IPv6 address %s to child 0x%04x", ErrorToString(error),
                    address.ToString().AsCString(), aChild.GetRloc16());
        }

        if (address.IsMulticast())
        {
            continue;
        }

        // We check if the same address is in-use by another child, if so
        // remove it. This implements "last-in wins" duplicate address
        // resolution policy.
        //
        // Duplicate addresses can occur if a previously attached child
        // attaches to same parent again (after a reset, memory wipe) using
        // a new random extended address before the old entry in the child
        // table is timed out and then trying to register its globally unique
        // IPv6 address as the new child.

        for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
        {
            if (&child == &aChild)
            {
                continue;
            }

            IgnoreError(child.RemoveIp6Address(address));
        }

        // Clear EID-to-RLOC cache for the unicast address registered by the child.
        Get<AddressResolver>().Remove(address);
    }
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    // Dua is removed
    if (oldDuaPtr != nullptr && !hasDua)
    {
        Get<DuaManager>().UpdateChildDomainUnicastAddress(aChild, ChildDuaState::kRemoved);
    }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    Get<MlrManager>().UpdateProxiedSubscriptions(aChild, oldMlrRegisteredAddresses, oldMlrRegisteredAddressNum);
#endif

    if (registeredCount == 0)
    {
        LogInfo("Child 0x%04x has no registered IPv6 address", aChild.GetRloc16());
    }
    else
    {
        LogInfo("Child 0x%04x has %d registered IPv6 address%s, %d address%s stored", aChild.GetRloc16(),
                registeredCount, (registeredCount == 1) ? "" : "es", storedCount, (storedCount == 1) ? "" : "es");
    }

    error = kErrorNone;

exit:
    return error;
}

void MleRouter::HandleChildIdRequest(RxInfo &aRxInfo)
{
    Error              error = kErrorNone;
    Mac::ExtAddress    extAddr;
    uint16_t           version;
    Challenge          response;
    uint32_t           linkFrameCounter;
    uint32_t           mleFrameCounter;
    uint8_t            modeBitmask;
    DeviceMode         mode;
    uint32_t           timeout;
    RequestedTlvs      requestedTlvs;
    MeshCoP::Timestamp timestamp;
    bool               needsActiveDatasetTlv;
    bool               needsPendingDatasetTlv;
    Child *            child;
    Router *           router;
    uint8_t            numTlvs;
    uint16_t           addressRegistrationOffset = 0;

    Log(kMessageReceive, kTypeChildIdRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(IsRouterEligible(), error = kErrorInvalidState);

    // only process message when operating as a child, router, or leader
    VerifyOrExit(IsAttached(), error = kErrorInvalidState);

    // Find Child
    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);

    child = mChildTable.FindChild(extAddr, Child::kInStateAnyExceptInvalid);
    VerifyOrExit(child != nullptr, error = kErrorAlready);

    // Version
    SuccessOrExit(error = Tlv::Find<VersionTlv>(aRxInfo.mMessage, version));
    VerifyOrExit(version >= OT_THREAD_VERSION_1_1, error = kErrorParse);

    // Response
    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));
    VerifyOrExit(response.Matches(child->GetChallenge(), child->GetChallengeSize()), error = kErrorSecurity);

    // Remove existing MLE messages
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleGeneral);
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleChildIdRequest);
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleChildUpdateRequest);
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleDataResponse);

    // Link-Layer and MLE Frame Counters
    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));

    // Mode
    SuccessOrExit(error = Tlv::Find<ModeTlv>(aRxInfo.mMessage, modeBitmask));
    mode.Set(modeBitmask);

    // Timeout
    SuccessOrExit(error = Tlv::Find<TimeoutTlv>(aRxInfo.mMessage, timeout));

    // TLV Request
    SuccessOrExit(error = aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvs));
    VerifyOrExit(requestedTlvs.mNumTlvs <= Child::kMaxRequestTlvs, error = kErrorParse);

    // Active Timestamp
    needsActiveDatasetTlv = true;
    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        needsActiveDatasetTlv =
            (MeshCoP::Timestamp::Compare(&timestamp, Get<MeshCoP::ActiveDatasetManager>().GetTimestamp()) != 0);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    // Pending Timestamp
    needsPendingDatasetTlv = true;
    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        needsPendingDatasetTlv =
            (MeshCoP::Timestamp::Compare(&timestamp, Get<MeshCoP::PendingDatasetManager>().GetTimestamp()) != 0);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    if (!mode.IsFullThreadDevice())
    {
        SuccessOrExit(error =
                          Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kAddressRegistration, addressRegistrationOffset));
        SuccessOrExit(error = UpdateChildAddresses(aRxInfo.mMessage, addressRegistrationOffset, *child));
    }

    // Remove from router table
    router = mRouterTable.GetRouter(extAddr);
    if (router != nullptr)
    {
        // The `router` here can be invalid
        RemoveNeighbor(*router);
    }

    if (!child->IsStateValid())
    {
        child->SetState(Neighbor::kStateChildIdRequest);
    }
    else
    {
        RemoveNeighbor(*child);
    }

    child->SetLastHeard(TimerMilli::GetNow());
    child->GetLinkFrameCounters().SetAll(linkFrameCounter);
    child->SetLinkAckFrameCounter(linkFrameCounter);
    child->SetMleFrameCounter(mleFrameCounter);
    child->SetKeySequence(aRxInfo.mKeySequence);
    child->SetDeviceMode(mode);
    child->SetVersion(static_cast<uint8_t>(version));
    child->GetLinkInfo().AddRss(aRxInfo.mMessageInfo.GetThreadLinkInfo()->GetRss());
    child->SetTimeout(timeout);
#if OPENTHREAD_CONFIG_MULTI_RADIO
    child->ClearLastRxFragmentTag();
#endif

    child->SetNetworkDataVersion(mLeaderData.GetDataVersion(mode.GetNetworkDataType()));
    child->ClearRequestTlvs();

    for (numTlvs = 0; numTlvs < requestedTlvs.mNumTlvs; numTlvs++)
    {
        child->SetRequestTlv(numTlvs, requestedTlvs.mTlvs[numTlvs]);
    }

    if (needsActiveDatasetTlv)
    {
        child->SetRequestTlv(numTlvs++, Tlv::kActiveDataset);
    }

    if (needsPendingDatasetTlv)
    {
        child->SetRequestTlv(numTlvs++, Tlv::kPendingDataset);
    }

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);

    case kRoleChild:
        child->SetState(Neighbor::kStateChildIdRequest);
        IgnoreError(BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest));
        break;

    case kRoleRouter:
    case kRoleLeader:
        SuccessOrExit(error = SendChildIdResponse(*child));
        break;
    }

exit:
    LogProcessError(kTypeChildIdRequest, error);
}

void MleRouter::HandleChildUpdateRequest(RxInfo &aRxInfo)
{
    static const uint8_t kMaxResponseTlvs = 10;

    Error           error = kErrorNone;
    Mac::ExtAddress extAddr;
    uint8_t         modeBitmask;
    DeviceMode      mode;
    Challenge       challenge;
    LeaderData      leaderData;
    uint32_t        timeout;
    Child *         child;
    DeviceMode      oldMode;
    RequestedTlvs   requestedTlvs;
    uint8_t         tlvs[kMaxResponseTlvs];
    uint8_t         tlvslength                = 0;
    uint16_t        addressRegistrationOffset = 0;
    bool            childDidChange            = false;

    Log(kMessageReceive, kTypeChildUpdateRequestOfChild, aRxInfo.mMessageInfo.GetPeerAddr());

    // Mode
    SuccessOrExit(error = Tlv::Find<ModeTlv>(aRxInfo.mMessage, modeBitmask));
    mode.Set(modeBitmask);

    // Challenge
    switch (aRxInfo.mMessage.ReadChallengeTlv(challenge))
    {
    case kErrorNone:
        tlvs[tlvslength++] = Tlv::kResponse;
        break;
    case kErrorNotFound:
        challenge.mLength = 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    tlvs[tlvslength++] = Tlv::kSourceAddress;

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);
    child = mChildTable.FindChild(extAddr, Child::kInStateAnyExceptInvalid);

    if (child == nullptr)
    {
        // For invalid non-sleepy child, send Child Update Response with
        // Status TLV (error).
        if (mode.IsRxOnWhenIdle())
        {
            tlvs[tlvslength++] = Tlv::kStatus;
            SendChildUpdateResponse(nullptr, aRxInfo.mMessageInfo, tlvs, tlvslength, challenge);
        }

        ExitNow();
    }

    // Ignore "Child Update Request" from a child that is present in the
    // child table but it is not yet in valid state. For example, a
    // child which is being restored (due to parent reset) or is in the
    // middle of the attach process (in `kStateParentRequest` or
    // `kStateChildIdRequest`).

    VerifyOrExit(child->IsStateValid());

    oldMode = child->GetDeviceMode();
    child->SetDeviceMode(mode);

    tlvs[tlvslength++] = Tlv::kMode;

    // Parent MUST include Leader Data TLV in Child Update Response
    tlvs[tlvslength++] = Tlv::kLeaderData;

    if (challenge.mLength != 0)
    {
        tlvs[tlvslength++] = Tlv::kMleFrameCounter;
        tlvs[tlvslength++] = Tlv::kLinkFrameCounter;
    }

    // IPv6 Address TLV
    if (Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kAddressRegistration, addressRegistrationOffset) == kErrorNone)
    {
        SuccessOrExit(error = UpdateChildAddresses(aRxInfo.mMessage, addressRegistrationOffset, *child));
        tlvs[tlvslength++] = Tlv::kAddressRegistration;
    }

    // Leader Data
    switch (aRxInfo.mMessage.ReadLeaderDataTlv(leaderData))
    {
    case kErrorNone:
        child->SetNetworkDataVersion(leaderData.GetDataVersion(child->GetNetworkDataType()));
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    // Timeout
    switch (Tlv::Find<TimeoutTlv>(aRxInfo.mMessage, timeout))
    {
    case kErrorNone:
        if (child->GetTimeout() != timeout)
        {
            child->SetTimeout(timeout);
            childDidChange = true;
        }

        tlvs[tlvslength++] = Tlv::kTimeout;
        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // TLV Request
    switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvs))
    {
    case kErrorNone:
        VerifyOrExit(requestedTlvs.mNumTlvs <= (kMaxResponseTlvs - tlvslength), error = kErrorParse);
        for (uint8_t i = 0; i < requestedTlvs.mNumTlvs; i++)
        {
            // Skip LeaderDataTlv since it is already included by default.
            if (requestedTlvs.mTlvs[i] != Tlv::kLeaderData)
            {
                tlvs[tlvslength++] = requestedTlvs.mTlvs[i];
            }
        }
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    if (child->IsCslSynchronized())
    {
        CslChannelTlv cslChannel;
        uint32_t      cslTimeout;

        if (Tlv::Find<CslTimeoutTlv>(aRxInfo.mMessage, cslTimeout) == kErrorNone)
        {
            child->SetCslTimeout(cslTimeout);
            // MUST include CSL accuracy TLV when request includes CSL timeout
            tlvs[tlvslength++] = Tlv::kCslClockAccuracy;
        }

        if (Tlv::FindTlv(aRxInfo.mMessage, cslChannel) == kErrorNone)
        {
            child->SetCslChannel(static_cast<uint8_t>(cslChannel.GetChannel()));
        }
        else
        {
            // Set CSL Channel unspecified.
            child->SetCslChannel(0);
        }
    }
#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

    child->SetLastHeard(TimerMilli::GetNow());

    if (oldMode != child->GetDeviceMode())
    {
        LogNote("Child 0x%04x mode change 0x%02x -> 0x%02x [%s]", child->GetRloc16(), oldMode.Get(),
                child->GetDeviceMode().Get(), child->GetDeviceMode().ToString().AsCString());

        childDidChange = true;

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        if (child->IsRxOnWhenIdle())
        {
            // Clear CSL synchronization state
            child->SetCslSynchronized(false);
        }
#endif

        // The `IndirectSender::HandleChildModeChange()` needs to happen
        // after "Child Update" message is fully parsed to ensure that
        // any registered IPv6 addresses included in the "Child Update"
        // are added to the child.

        Get<IndirectSender>().HandleChildModeChange(*child, oldMode);
    }

    if (childDidChange)
    {
        IgnoreError(mChildTable.StoreChild(*child));
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    // We clear the fragment tag only if the "Child Update Request" is
    // from a detached child trying to restore its link with its
    // parent which is indicated by the presence of Challenge TLV in
    // the message.
    if (challenge.mLength != 0)
    {
        child->ClearLastRxFragmentTag();
    }
#endif

    SendChildUpdateResponse(child, aRxInfo.mMessageInfo, tlvs, tlvslength, challenge);

exit:
    LogProcessError(kTypeChildUpdateRequestOfChild, error);
}

void MleRouter::HandleChildUpdateResponse(RxInfo &aRxInfo)
{
    Error      error = kErrorNone;
    uint16_t   sourceAddress;
    uint32_t   timeout;
    Challenge  response;
    uint8_t    status;
    uint32_t   linkFrameCounter;
    uint32_t   mleFrameCounter;
    LeaderData leaderData;
    Child *    child;
    uint16_t   addressRegistrationOffset = 0;

    if ((aRxInfo.mNeighbor == nullptr) || IsActiveRouter(aRxInfo.mNeighbor->GetRloc16()))
    {
        Log(kMessageReceive, kTypeChildUpdateResponseOfUnknownChild, aRxInfo.mMessageInfo.GetPeerAddr());
        ExitNow(error = kErrorNotFound);
    }

    child = static_cast<Child *>(aRxInfo.mNeighbor);

    // Response
    switch (aRxInfo.mMessage.ReadResponseTlv(response))
    {
    case kErrorNone:
        VerifyOrExit(response.Matches(child->GetChallenge(), child->GetChallengeSize()), error = kErrorSecurity);
        break;
    case kErrorNotFound:
        VerifyOrExit(child->IsStateValid(), error = kErrorSecurity);
        break;
    default:
        ExitNow(error = kErrorNone);
    }

    Log(kMessageReceive, kTypeChildUpdateResponseOfChild, aRxInfo.mMessageInfo.GetPeerAddr(), child->GetRloc16());

    // Source Address
    switch (Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress))
    {
    case kErrorNone:
        if (child->GetRloc16() != sourceAddress)
        {
            RemoveNeighbor(*child);
            ExitNow();
        }

        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // Status
    switch (Tlv::Find<ThreadStatusTlv>(aRxInfo.mMessage, status))
    {
    case kErrorNone:
        VerifyOrExit(status != StatusTlv::kError, RemoveNeighbor(*child));
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    // Link-Layer Frame Counter

    switch (Tlv::Find<LinkFrameCounterTlv>(aRxInfo.mMessage, linkFrameCounter))
    {
    case kErrorNone:
        child->GetLinkFrameCounters().SetAll(linkFrameCounter);
        child->SetLinkAckFrameCounter(linkFrameCounter);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    // MLE Frame Counter
    switch (Tlv::Find<MleFrameCounterTlv>(aRxInfo.mMessage, mleFrameCounter))
    {
    case kErrorNone:
        child->SetMleFrameCounter(mleFrameCounter);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorNone);
    }

    // Timeout
    switch (Tlv::Find<TimeoutTlv>(aRxInfo.mMessage, timeout))
    {
    case kErrorNone:
        child->SetTimeout(timeout);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    // IPv6 Address
    if (Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kAddressRegistration, addressRegistrationOffset) == kErrorNone)
    {
        SuccessOrExit(error = UpdateChildAddresses(aRxInfo.mMessage, addressRegistrationOffset, *child));
    }

    // Leader Data
    switch (aRxInfo.mMessage.ReadLeaderDataTlv(leaderData))
    {
    case kErrorNone:
        child->SetNetworkDataVersion(leaderData.GetDataVersion(child->GetNetworkDataType()));
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    SetChildStateToValid(*child);
    child->SetLastHeard(TimerMilli::GetNow());
    child->SetKeySequence(aRxInfo.mKeySequence);
    child->GetLinkInfo().AddRss(aRxInfo.mMessageInfo.GetThreadLinkInfo()->GetRss());

exit:
    LogProcessError(kTypeChildUpdateResponseOfChild, error);
}

void MleRouter::HandleDataRequest(RxInfo &aRxInfo)
{
    Error              error = kErrorNone;
    RequestedTlvs      requestedTlvs;
    MeshCoP::Timestamp timestamp;
    uint8_t            tlvs[4];
    uint8_t            numTlvs;

    Log(kMessageReceive, kTypeDataRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid(), error = kErrorSecurity);

    // TLV Request
    SuccessOrExit(error = aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvs));
    VerifyOrExit(requestedTlvs.mNumTlvs <= sizeof(tlvs), error = kErrorParse);

    memset(tlvs, Tlv::kInvalid, sizeof(tlvs));
    memcpy(tlvs, requestedTlvs.mTlvs, requestedTlvs.mNumTlvs);
    numTlvs = requestedTlvs.mNumTlvs;

    // Active Timestamp
    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        if (MeshCoP::Timestamp::Compare(&timestamp, Get<MeshCoP::ActiveDatasetManager>().GetTimestamp()) == 0)
        {
            break;
        }

        OT_FALL_THROUGH;

    case kErrorNotFound:
        tlvs[numTlvs++] = Tlv::kActiveDataset;
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // Pending Timestamp
    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        if (MeshCoP::Timestamp::Compare(&timestamp, Get<MeshCoP::PendingDatasetManager>().GetTimestamp()) == 0)
        {
            break;
        }

        OT_FALL_THROUGH;

    case kErrorNotFound:
        tlvs[numTlvs++] = Tlv::kPendingDataset;
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    SendDataResponse(aRxInfo.mMessageInfo.GetPeerAddr(), tlvs, numTlvs, 0, &aRxInfo.mMessage);

exit:
    LogProcessError(kTypeDataRequest, error);
}

void MleRouter::HandleNetworkDataUpdateRouter(void)
{
    static const uint8_t tlvs[] = {Tlv::kNetworkData};
    Ip6::Address         destination;
    uint16_t             delay;

    VerifyOrExit(IsRouterOrLeader());

    destination.SetToLinkLocalAllNodesMulticast();

    delay = IsLeader() ? 0 : Random::NonCrypto::GetUint16InRange(0, kUnsolicitedDataResponseJitter);
    SendDataResponse(destination, tlvs, sizeof(tlvs), delay);

    SynchronizeChildNetworkData();

exit:
    return;
}

void MleRouter::SynchronizeChildNetworkData(void)
{
    VerifyOrExit(IsRouterOrLeader());

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (child.IsRxOnWhenIdle())
        {
            continue;
        }

        if (child.GetNetworkDataVersion() == Get<NetworkData::Leader>().GetVersion(child.GetNetworkDataType()))
        {
            continue;
        }

        SuccessOrExit(SendChildUpdateRequest(child));
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
void MleRouter::SetSteeringData(const Mac::ExtAddress *aExtAddress)
{
    Mac::ExtAddress nullExtAddr;
    Mac::ExtAddress allowAnyExtAddr;

    nullExtAddr.Clear();
    allowAnyExtAddr.Fill(0xff);

    if ((aExtAddress == nullptr) || (*aExtAddress == nullExtAddr))
    {
        mSteeringData.Clear();
    }
    else if (*aExtAddress == allowAnyExtAddr)
    {
        mSteeringData.SetToPermitAllJoiners();
    }
    else
    {
        Mac::ExtAddress joinerId;

        mSteeringData.Init();
        MeshCoP::ComputeJoinerId(*aExtAddress, joinerId);
        mSteeringData.UpdateBloomFilter(joinerId);
    }
}
#endif // OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE

void MleRouter::HandleDiscoveryRequest(RxInfo &aRxInfo)
{
    Error                        error = kErrorNone;
    Tlv                          tlv;
    MeshCoP::Tlv                 meshcopTlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    MeshCoP::ExtendedPanId       extPanId;
    uint16_t                     offset;
    uint16_t                     end;

    Log(kMessageReceive, kTypeDiscoveryRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    discoveryRequest.SetLength(0);

    // only Routers and REEDs respond
    VerifyOrExit(IsRouterEligible(), error = kErrorInvalidState);

    // find MLE Discovery TLV
    VerifyOrExit(Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kDiscovery, offset) == kErrorNone, error = kErrorParse);
    IgnoreError(aRxInfo.mMessage.Read(offset, tlv));

    offset += sizeof(tlv);
    end = offset + sizeof(tlv) + tlv.GetLength();

    while (offset < end)
    {
        IgnoreError(aRxInfo.mMessage.Read(offset, meshcopTlv));

        switch (meshcopTlv.GetType())
        {
        case MeshCoP::Tlv::kDiscoveryRequest:
            IgnoreError(aRxInfo.mMessage.Read(offset, discoveryRequest));
            VerifyOrExit(discoveryRequest.IsValid(), error = kErrorParse);

            break;

        case MeshCoP::Tlv::kExtendedPanId:
            SuccessOrExit(error = Tlv::Read<MeshCoP::ExtendedPanIdTlv>(aRxInfo.mMessage, offset, extPanId));
            VerifyOrExit(Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId() != extPanId, error = kErrorDrop);

            break;

        default:
            break;
        }

        offset += sizeof(meshcopTlv) + meshcopTlv.GetLength();
    }

    if (discoveryRequest.IsValid())
    {
        if (mDiscoveryRequestCallback != nullptr)
        {
            otThreadDiscoveryRequestInfo info;

            aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(AsCoreType(&info.mExtAddress));
            info.mVersion  = discoveryRequest.GetVersion();
            info.mIsJoiner = discoveryRequest.IsJoiner();

            mDiscoveryRequestCallback(&info, mDiscoveryRequestCallbackContext);
        }

        if (discoveryRequest.IsJoiner())
        {
#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
            if (!mSteeringData.IsEmpty())
            {
            }
            else // if steering data is not set out of band, fall back to network data
#endif
            {
                VerifyOrExit(Get<NetworkData::Leader>().IsJoiningEnabled(), error = kErrorSecurity);
            }
        }
    }

    error = SendDiscoveryResponse(aRxInfo.mMessageInfo.GetPeerAddr(), aRxInfo.mMessage);

exit:
    LogProcessError(kTypeDiscoveryRequest, error);
}

Error MleRouter::SendDiscoveryResponse(const Ip6::Address &aDestination, const Message &aDiscoverRequestMessage)
{
    Error                         error = kErrorNone;
    TxMessage *                   message;
    uint16_t                      startOffset;
    Tlv                           tlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::NetworkNameTlv       networkName;
    uint16_t                      delay;

    VerifyOrExit((message = NewMleMessage(kCommandDiscoveryResponse)) != nullptr, error = kErrorNoBufs);
    message->SetPanId(aDiscoverRequestMessage.GetPanId());
#if OPENTHREAD_CONFIG_MULTI_RADIO
    // Send the MLE Discovery Response message on same radio link
    // from which the "MLE Discover Request" message was received.
    message->SetRadioType(aDiscoverRequestMessage.GetRadioType());
#endif

    // Discovery TLV
    tlv.SetType(Tlv::kDiscovery);
    SuccessOrExit(error = message->Append(tlv));

    startOffset = message->GetLength();

    // Discovery Response TLV
    discoveryResponse.Init();
    discoveryResponse.SetVersion(kThreadVersion);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    if (Get<KeyManager>().GetSecurityPolicy().mNativeCommissioningEnabled)
    {
        SuccessOrExit(
            error = Tlv::Append<MeshCoP::CommissionerUdpPortTlv>(*message, Get<MeshCoP::BorderAgent>().GetUdpPort()));

        discoveryResponse.SetNativeCommissioner(true);
    }
    else
#endif
    {
        discoveryResponse.SetNativeCommissioner(false);
    }

    if (Get<KeyManager>().GetSecurityPolicy().mCommercialCommissioningEnabled)
    {
        discoveryResponse.SetCommercialCommissioningMode(true);
    }

    SuccessOrExit(error = discoveryResponse.AppendTo(*message));

    // Extended PAN ID TLV
    SuccessOrExit(
        error = Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()));

    // Network Name TLV
    networkName.Init();
    networkName.SetNetworkName(Get<MeshCoP::NetworkNameManager>().GetNetworkName().GetAsData());
    SuccessOrExit(error = networkName.AppendTo(*message));

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    // If steering data is set out of band, use that value.
    // Otherwise use the one from commissioning data.
    if (!mSteeringData.IsEmpty())
    {
        SuccessOrExit(error = Tlv::Append<MeshCoP::SteeringDataTlv>(*message, mSteeringData.GetData(),
                                                                    mSteeringData.GetLength()));
    }
    else
#endif
    {
        const MeshCoP::Tlv *steeringData;

        steeringData = Get<NetworkData::Leader>().GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);

        if (steeringData != nullptr)
        {
            SuccessOrExit(error = steeringData->AppendTo(*message));
        }
    }

    SuccessOrExit(
        error = Tlv::Append<MeshCoP::JoinerUdpPortTlv>(*message, Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort()));

    tlv.SetLength(static_cast<uint8_t>(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), tlv);

    delay = Random::NonCrypto::GetUint16InRange(0, kDiscoveryMaxJitter + 1);

    SuccessOrExit(error = message->SendAfterDelay(aDestination, delay));

    Log(kMessageDelay, kTypeDiscoveryResponse, aDestination);

exit:
    FreeMessageOnError(message, error);
    LogProcessError(kTypeDiscoveryResponse, error);
    return error;
}

Error MleRouter::SendChildIdResponse(Child &aChild)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage *  message;

    VerifyOrExit((message = NewMleMessage(kCommandChildIdResponse)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());
    SuccessOrExit(error = message->AppendActiveTimestampTlv());
    SuccessOrExit(error = message->AppendPendingTimestampTlv());

    if ((aChild.GetRloc16() == 0) || !RouterIdMatch(aChild.GetRloc16(), GetRloc16()))
    {
        uint16_t rloc16;

        // pick next Child ID that is not being used
        do
        {
            mNextChildId++;

            if (mNextChildId > kMaxChildId)
            {
                mNextChildId = kMinChildId;
            }

            rloc16 = Get<Mac::Mac>().GetShortAddress() | mNextChildId;

        } while (mChildTable.FindChild(rloc16, Child::kInStateAnyExceptInvalid) != nullptr);

        // allocate Child ID
        aChild.SetRloc16(rloc16);
    }

    SuccessOrExit(error = message->AppendAddress16Tlv(aChild.GetRloc16()));

    for (uint8_t i = 0; i < Child::kMaxRequestTlvs; i++)
    {
        switch (aChild.GetRequestTlv(i))
        {
        case Tlv::kNetworkData:
            SuccessOrExit(error = message->AppendNetworkDataTlv(aChild.GetNetworkDataType()));
            break;

        case Tlv::kRoute:
            SuccessOrExit(error = message->AppendRouteTlv());
            break;

        case Tlv::kActiveDataset:
            SuccessOrExit(error = message->AppendActiveDatasetTlv());
            break;

        case Tlv::kPendingDataset:
            SuccessOrExit(error = message->AppendPendingDatasetTlv());
            break;

        default:
            break;
        }
    }

    if (!aChild.IsFullThreadDevice())
    {
        SuccessOrExit(error = message->AppendAddresseRegisterationTlv(aChild));
    }

    SetChildStateToValid(aChild);

    if (!aChild.IsRxOnWhenIdle())
    {
        Get<IndirectSender>().SetChildUseShortAddress(aChild, false);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aChild.IsTimeSyncEnabled())
    {
        message->SetTimeSync(true);
    }
#endif

    destination.SetToLinkLocalAddress(aChild.GetExtAddress());
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeChildIdResponse, destination, aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error MleRouter::SendChildUpdateRequest(Child &aChild)
{
    static const uint8_t tlvs[] = {Tlv::kTimeout, Tlv::kAddressRegistration};
    Error                error  = kErrorNone;
    Ip6::Address         destination;
    TxMessage *          message = nullptr;

    if (!aChild.IsRxOnWhenIdle())
    {
        uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

        for (const Message &msg : Get<MeshForwarder>().GetSendQueue())
        {
            if (msg.GetChildMask(childIndex) && msg.GetSubType() == Message::kSubTypeMleChildUpdateRequest)
            {
                // No need to send the resync "Child Update Request" to the sleepy child
                // if there is one already queued.
                if (aChild.IsStateRestoring())
                {
                    ExitNow();
                }

                // Remove queued outdated "Child Update Request" when there is newer Network Data is to send.
                Get<MeshForwarder>().RemoveMessages(aChild, Message::kSubTypeMleChildUpdateRequest);
                break;
            }
        }
    }

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());
    SuccessOrExit(error = message->AppendNetworkDataTlv(aChild.GetNetworkDataType()));
    SuccessOrExit(error = message->AppendActiveTimestampTlv());
    SuccessOrExit(error = message->AppendPendingTimestampTlv());

    if (!aChild.IsStateValid())
    {
        SuccessOrExit(error = message->AppendTlvRequestTlv(tlvs, sizeof(tlvs)));
        aChild.GenerateChallenge();
        SuccessOrExit(error = message->AppendChallengeTlv(aChild.GetChallenge(), aChild.GetChallengeSize()));
    }

    destination.SetToLinkLocalAddress(aChild.GetExtAddress());
    SuccessOrExit(error = message->SendTo(destination));

    if (aChild.IsRxOnWhenIdle())
    {
        // only try to send a single Child Update Request message to an rx-on-when-idle child
        aChild.SetState(Child::kStateChildUpdateRequest);
    }

    Log(kMessageSend, kTypeChildUpdateRequestOfChild, destination, aChild.GetRloc16());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void MleRouter::SendChildUpdateResponse(Child *                 aChild,
                                        const Ip6::MessageInfo &aMessageInfo,
                                        const uint8_t *         aTlvs,
                                        uint8_t                 aTlvsLength,
                                        const Challenge &       aChallenge)
{
    Error      error = kErrorNone;
    TxMessage *message;

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateResponse)) != nullptr, error = kErrorNoBufs);

    for (int i = 0; i < aTlvsLength; i++)
    {
        switch (aTlvs[i])
        {
        case Tlv::kStatus:
            SuccessOrExit(error = message->AppendStatusTlv(StatusTlv::kError));
            break;

        case Tlv::kAddressRegistration:
            SuccessOrExit(error = message->AppendAddresseRegisterationTlv(*aChild));
            break;

        case Tlv::kLeaderData:
            SuccessOrExit(error = message->AppendLeaderDataTlv());
            break;

        case Tlv::kMode:
            SuccessOrExit(error = message->AppendModeTlv(aChild->GetDeviceMode()));
            break;

        case Tlv::kNetworkData:
            SuccessOrExit(error = message->AppendNetworkDataTlv(aChild->GetNetworkDataType()));
            SuccessOrExit(error = message->AppendActiveTimestampTlv());
            SuccessOrExit(error = message->AppendPendingTimestampTlv());
            break;

        case Tlv::kResponse:
            SuccessOrExit(error = message->AppendResponseTlv(aChallenge));
            break;

        case Tlv::kSourceAddress:
            SuccessOrExit(error = message->AppendSourceAddressTlv());
            break;

        case Tlv::kTimeout:
            SuccessOrExit(error = message->AppendTimeoutTlv(aChild->GetTimeout()));
            break;

        case Tlv::kMleFrameCounter:
            SuccessOrExit(error = message->AppendMleFrameCounterTlv());
            break;

        case Tlv::kLinkFrameCounter:
            SuccessOrExit(error = message->AppendLinkFrameCounterTlv());
            break;

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        case Tlv::kCslClockAccuracy:
            if (!aChild->IsRxOnWhenIdle())
            {
                SuccessOrExit(error = message->AppendCslClockAccuracyTlv());
            }
            break;
#endif
        }
    }

    SuccessOrExit(error = message->SendTo(aMessageInfo.GetPeerAddr()));

    if (aChild == nullptr)
    {
        Log(kMessageSend, kTypeChildUpdateResponseOfChild, aMessageInfo.GetPeerAddr());
    }
    else
    {
        Log(kMessageSend, kTypeChildUpdateResponseOfChild, aMessageInfo.GetPeerAddr(), aChild->GetRloc16());
    }

exit:
    FreeMessageOnError(message, error);
}

void MleRouter::SendDataResponse(const Ip6::Address &aDestination,
                                 const uint8_t *     aTlvs,
                                 uint8_t             aTlvsLength,
                                 uint16_t            aDelay,
                                 const Message *     aRequestMessage)
{
    OT_UNUSED_VARIABLE(aRequestMessage);

    Error      error   = kErrorNone;
    TxMessage *message = nullptr;
    Neighbor * neighbor;

    if (mRetrieveNewNetworkData)
    {
        LogInfo("Suppressing Data Response - waiting for new network data");
        ExitNow();
    }

    VerifyOrExit((message = NewMleMessage(kCommandDataResponse)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());
    SuccessOrExit(error = message->AppendActiveTimestampTlv());
    SuccessOrExit(error = message->AppendPendingTimestampTlv());

    for (int i = 0; i < aTlvsLength; i++)
    {
        switch (aTlvs[i])
        {
        case Tlv::kNetworkData:
            neighbor = mNeighborTable.FindNeighbor(aDestination);
            SuccessOrExit(error = message->AppendNetworkDataTlv((neighbor != nullptr) ? neighbor->GetNetworkDataType()
                                                                                      : NetworkData::kFullSet));
            break;

        case Tlv::kActiveDataset:
            SuccessOrExit(error = message->AppendActiveDatasetTlv());
            break;

        case Tlv::kPendingDataset:
            SuccessOrExit(error = message->AppendPendingDatasetTlv());
            break;

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        case Tlv::kLinkMetricsReport:
            OT_ASSERT(aRequestMessage != nullptr);
            neighbor = mNeighborTable.FindNeighbor(aDestination);
            VerifyOrExit(neighbor != nullptr, error = kErrorInvalidState);
            SuccessOrExit(error = Get<LinkMetrics::LinkMetrics>().AppendReport(*message, *aRequestMessage, *neighbor));
            break;
#endif
        }
    }

    if (aDelay)
    {
        // Remove MLE Data Responses from Send Message Queue.
        Get<MeshForwarder>().RemoveDataResponseMessages();

        // Remove multicast MLE Data Response from Delayed Message Queue.
        RemoveDelayedDataResponseMessage();

        SuccessOrExit(error = message->SendAfterDelay(aDestination, aDelay));
        Log(kMessageDelay, kTypeDataResponse, aDestination);
    }
    else
    {
        SuccessOrExit(error = message->SendTo(aDestination));
        Log(kMessageSend, kTypeDataResponse, aDestination);
    }

exit:
    FreeMessageOnError(message, error);
    LogSendError(kTypeDataResponse, error);
}

bool MleRouter::IsMinimalChild(uint16_t aRloc16)
{
    bool rval = false;

    if (RouterIdFromRloc16(aRloc16) == RouterIdFromRloc16(Get<Mac::Mac>().GetShortAddress()))
    {
        Neighbor *neighbor;

        neighbor = mNeighborTable.FindNeighbor(aRloc16);

        rval = (neighbor != nullptr) && (!neighbor->IsFullThreadDevice());
    }

    return rval;
}

void MleRouter::RemoveRouterLink(Router &aRouter)
{
    switch (mRole)
    {
    case kRoleChild:
        if (&aRouter == &mParent)
        {
            IgnoreError(BecomeDetached());
        }
        break;

#if OPENTHREAD_FTD
    case kRoleRouter:
    case kRoleLeader:
        mRouterTable.RemoveRouterLink(aRouter);
        break;
#endif

    default:
        break;
    }
}

void MleRouter::RemoveNeighbor(Neighbor &aNeighbor)
{
    VerifyOrExit(!aNeighbor.IsStateInvalid());

    if (&aNeighbor == &mParent)
    {
        if (IsChild())
        {
            IgnoreError(BecomeDetached());
        }
    }
    else if (&aNeighbor == &mParentCandidate)
    {
        mParentCandidate.Clear();
    }
    else if (!IsActiveRouter(aNeighbor.GetRloc16()))
    {
        OT_ASSERT(mChildTable.GetChildIndex(static_cast<Child &>(aNeighbor)) < kMaxChildren);

        if (aNeighbor.IsStateValidOrRestoring())
        {
            mNeighborTable.Signal(NeighborTable::kChildRemoved, aNeighbor);
        }

        Get<IndirectSender>().ClearAllMessagesForSleepyChild(static_cast<Child &>(aNeighbor));

        if (aNeighbor.IsFullThreadDevice())
        {
            // Clear all EID-to-RLOC entries associated with the child.
            Get<AddressResolver>().Remove(aNeighbor.GetRloc16());
        }

        mChildTable.RemoveStoredChild(static_cast<Child &>(aNeighbor));
    }
    else if (aNeighbor.IsStateValid())
    {
        OT_ASSERT(mRouterTable.Contains(aNeighbor));

        mNeighborTable.Signal(NeighborTable::kRouterRemoved, aNeighbor);
        mRouterTable.RemoveRouterLink(static_cast<Router &>(aNeighbor));
    }

    aNeighbor.GetLinkInfo().Clear();
    aNeighbor.SetState(Neighbor::kStateInvalid);
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    aNeighbor.RemoveAllForwardTrackingSeriesInfo();
#endif

exit:
    return;
}

uint16_t MleRouter::GetNextHop(uint16_t aDestination)
{
    uint8_t       destinationId = RouterIdFromRloc16(aDestination);
    uint8_t       routeCost;
    uint8_t       linkCost;
    uint16_t      rval = Mac::kShortAddrInvalid;
    const Router *router;
    const Router *nextHop;

    if (IsChild())
    {
        ExitNow(rval = Mle::GetNextHop(aDestination));
    }

    // The frame is destined to a child
    if (destinationId == mRouterId)
    {
        ExitNow(rval = aDestination);
    }

    router = mRouterTable.GetRouter(destinationId);
    VerifyOrExit(router != nullptr);

    linkCost  = GetLinkCost(destinationId);
    routeCost = GetRouteCost(aDestination);

    if ((routeCost + GetLinkCost(router->GetNextHop())) < linkCost)
    {
        nextHop = mRouterTable.GetRouter(router->GetNextHop());
        VerifyOrExit(nextHop != nullptr && !nextHop->IsStateInvalid());

        rval = Rloc16FromRouterId(router->GetNextHop());
    }
    else if (linkCost < kMaxRouteCost)
    {
        rval = Rloc16FromRouterId(destinationId);
    }

exit:
    return rval;
}

uint8_t MleRouter::GetCost(uint16_t aRloc16)
{
    uint8_t routerId = RouterIdFromRloc16(aRloc16);
    uint8_t cost     = GetLinkCost(routerId);
    Router *router   = mRouterTable.GetRouter(routerId);
    uint8_t routeCost;

    VerifyOrExit(router != nullptr && mRouterTable.GetRouter(router->GetNextHop()) != nullptr);

    routeCost = GetRouteCost(aRloc16) + GetLinkCost(router->GetNextHop());

    if (cost > routeCost)
    {
        cost = routeCost;
    }

exit:
    return cost;
}

uint8_t MleRouter::GetRouteCost(uint16_t aRloc16) const
{
    uint8_t       rval = kMaxRouteCost;
    const Router *router;

    router = mRouterTable.GetRouter(RouterIdFromRloc16(aRloc16));
    VerifyOrExit(router != nullptr && mRouterTable.GetRouter(router->GetNextHop()) != nullptr);

    rval = router->GetCost();

exit:
    return rval;
}

Error MleRouter::SetPreferredRouterId(uint8_t aRouterId)
{
    Error error = kErrorNone;

    VerifyOrExit(IsDetached() || IsDisabled(), error = kErrorInvalidState);

    mPreviousRouterId = aRouterId;

exit:
    return error;
}

void MleRouter::SetRouterId(uint8_t aRouterId)
{
    mRouterId         = aRouterId;
    mPreviousRouterId = mRouterId;
}

void MleRouter::ResolveRoutingLoops(uint16_t aSourceMac, uint16_t aDestRloc16)
{
    Router *router;

    if (aSourceMac != GetNextHop(aDestRloc16))
    {
        ExitNow();
    }

    // loop exists
    router = mRouterTable.GetRouter(RouterIdFromRloc16(aDestRloc16));
    VerifyOrExit(router != nullptr);

    // invalidate next hop
    router->SetNextHop(kInvalidRouterId);
    ResetAdvertiseInterval();

exit:
    return;
}

Error MleRouter::CheckReachability(uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    Error error = kErrorNone;

    if (IsChild())
    {
        error = Mle::CheckReachability(aMeshDest, aIp6Header);
        ExitNow();
    }

    if (aMeshDest == Get<Mac::Mac>().GetShortAddress())
    {
        // mesh destination is this device
        if (Get<ThreadNetif>().HasUnicastAddress(aIp6Header.GetDestination()))
        {
            // IPv6 destination is this device
            ExitNow();
        }
        else if (mNeighborTable.FindNeighbor(aIp6Header.GetDestination()) != nullptr)
        {
            // IPv6 destination is an RFD child
            ExitNow();
        }
    }
    else if (RouterIdFromRloc16(aMeshDest) == mRouterId)
    {
        // mesh destination is a child of this device
        if (mChildTable.FindChild(aMeshDest, Child::kInStateValidOrRestoring))
        {
            ExitNow();
        }
    }
    else if (GetNextHop(aMeshDest) != Mac::kShortAddrInvalid)
    {
        // forwarding to another router and route is known
        ExitNow();
    }

    error = kErrorNoRoute;

exit:
    return error;
}

Error MleRouter::SendAddressSolicit(ThreadStatusTlv::Status aStatus)
{
    Error            error = kErrorNone;
    Tmf::MessageInfo messageInfo(GetInstance());
    Coap::Message *  message = nullptr;

    VerifyOrExit(!mAddressSolicitPending);

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(UriPath::kAddressSolicit);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<ThreadExtMacAddressTlv>(*message, Get<Mac::Mac>().GetExtAddress()));

    if (IsRouterIdValid(mPreviousRouterId))
    {
        SuccessOrExit(error = Tlv::Append<ThreadRloc16Tlv>(*message, Rloc16FromRouterId(mPreviousRouterId)));
    }

    SuccessOrExit(error = Tlv::Append<ThreadStatusTlv>(*message, aStatus));

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = Tlv::Append<XtalAccuracyTlv>(*message, otPlatTimeGetXtalAccuracy()));
#endif

    SuccessOrExit(error = messageInfo.SetSockAddrToRlocPeerAddrToLeaderRloc());

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, &HandleAddressSolicitResponse, this));
    mAddressSolicitPending = true;

    Log(kMessageSend, kTypeAddressSolicit, messageInfo.GetPeerAddr());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void MleRouter::SendAddressRelease(void)
{
    Error            error = kErrorNone;
    Tmf::MessageInfo messageInfo(GetInstance());
    Coap::Message *  message;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(UriPath::kAddressRelease);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<ThreadRloc16Tlv>(*message, Rloc16FromRouterId(mRouterId)));
    SuccessOrExit(error = Tlv::Append<ThreadExtMacAddressTlv>(*message, Get<Mac::Mac>().GetExtAddress()));

    SuccessOrExit(error = messageInfo.SetSockAddrToRlocPeerAddrToLeaderRloc());

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    Log(kMessageSend, kTypeAddressRelease, messageInfo.GetPeerAddr());

exit:
    FreeMessageOnError(message, error);
    LogSendError(kTypeAddressRelease, error);
}

void MleRouter::HandleAddressSolicitResponse(void *               aContext,
                                             otMessage *          aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             Error                aResult)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicitResponse(AsCoapMessagePtr(aMessage),
                                                                     AsCoreTypePtr(aMessageInfo), aResult);
}

void MleRouter::HandleAddressSolicitResponse(Coap::Message *         aMessage,
                                             const Ip6::MessageInfo *aMessageInfo,
                                             Error                   aResult)
{
    uint8_t             status;
    uint16_t            rloc16;
    ThreadRouterMaskTlv routerMaskTlv;
    uint8_t             routerId;
    Router *            router;
    Router *            leader;

    mAddressSolicitPending = false;

    VerifyOrExit(aResult == kErrorNone && aMessage != nullptr && aMessageInfo != nullptr);

    VerifyOrExit(aMessage->GetCode() == Coap::kCodeChanged);

    Log(kMessageReceive, kTypeAddressReply, aMessageInfo->GetPeerAddr());

    SuccessOrExit(Tlv::Find<ThreadStatusTlv>(*aMessage, status));

    if (status != ThreadStatusTlv::kSuccess)
    {
        mAddressSolicitRejected = true;

        if (IsRouterIdValid(mPreviousRouterId))
        {
            if (HasChildren())
            {
                RemoveChildren();
            }

            SetRouterId(kInvalidRouterId);
        }

        ExitNow();
    }

    SuccessOrExit(Tlv::Find<ThreadRloc16Tlv>(*aMessage, rloc16));
    routerId = RouterIdFromRloc16(rloc16);

    SuccessOrExit(Tlv::FindTlv(*aMessage, routerMaskTlv));
    VerifyOrExit(routerMaskTlv.IsValid());

    // assign short address
    SetRouterId(routerId);

    SetStateRouter(Rloc16FromRouterId(mRouterId));
    mRouterTable.Clear();
    mRouterTable.UpdateRouterIdSet(routerMaskTlv.GetIdSequence(), routerMaskTlv.GetAssignedRouterIdMask());

    router = mRouterTable.GetRouter(routerId);
    VerifyOrExit(router != nullptr);

    router->SetExtAddress(Get<Mac::Mac>().GetExtAddress());
    router->SetCost(0);

    router = mRouterTable.GetRouter(mParent.GetRouterId());
    VerifyOrExit(router != nullptr);

    // Keep link to the parent in order to respond to Parent Requests before new link is established.
    *router = mParent;
    router->SetState(Neighbor::kStateValid);
    router->SetNextHop(kInvalidRouterId);
    router->SetCost(0);

    leader = mRouterTable.GetLeader();
    OT_ASSERT(leader != nullptr);

    if (leader != router)
    {
        // Keep route path to the Leader reported by the parent before it is updated.
        leader->SetCost(mParentLeaderCost);
        leader->SetNextHop(RouterIdFromRloc16(mParent.GetRloc16()));
    }

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateChildIdRequest))
    {
        IgnoreError(SendChildIdResponse(child));
    }

    mLinkRequestDelay = kMulticastLinkRequestDelay;

exit:
    // Send announce after received address solicit reply if needed
    InformPreviousChannel();
}

bool MleRouter::IsExpectedToBecomeRouterSoon(void) const
{
    static constexpr uint8_t kMaxDelay = 10;

    return IsRouterEligible() && IsChild() && !mAddressSolicitRejected &&
           ((GetRouterSelectionJitterTimeout() != 0 && GetRouterSelectionJitterTimeout() <= kMaxDelay) ||
            mAddressSolicitPending);
}

void MleRouter::HandleAddressSolicit(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicit(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void MleRouter::HandleAddressSolicit(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                   error          = kErrorNone;
    ThreadStatusTlv::Status responseStatus = ThreadStatusTlv::kNoAddressAvailable;
    Router *                router         = nullptr;
    Mac::ExtAddress         extAddress;
    uint16_t                rloc16;
    uint8_t                 status;

    VerifyOrExit(aMessage.IsConfirmablePostRequest(), error = kErrorParse);

    Log(kMessageReceive, kTypeAddressSolicit, aMessageInfo.GetPeerAddr());

    SuccessOrExit(error = Tlv::Find<ThreadExtMacAddressTlv>(aMessage, extAddress));
    SuccessOrExit(error = Tlv::Find<ThreadStatusTlv>(aMessage, status));

    switch (Tlv::Find<ThreadRloc16Tlv>(aMessage, rloc16))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        rloc16 = Mac::kShortAddrInvalid;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    {
        uint16_t xtalAccuracy;

        SuccessOrExit(Tlv::Find<XtalAccuracyTlv>(aMessage, xtalAccuracy));
        VerifyOrExit(xtalAccuracy <= Get<TimeSync>().GetXtalThreshold());
    }
#endif

    // Check if allocation already exists
    router = mRouterTable.GetRouter(extAddress);

    if (router != nullptr)
    {
        responseStatus = ThreadStatusTlv::kSuccess;
        ExitNow();
    }

    switch (status)
    {
    case ThreadStatusTlv::kTooFewRouters:
        VerifyOrExit(mRouterTable.GetActiveRouterCount() < mRouterUpgradeThreshold);
        break;

    case ThreadStatusTlv::kHaveChildIdRequest:
    case ThreadStatusTlv::kParentPartitionChange:
        break;

    case ThreadStatusTlv::kBorderRouterRequst:
        if ((mRouterTable.GetActiveRouterCount() >= mRouterUpgradeThreshold) &&
            (Get<NetworkData::Leader>().CountBorderRouters(NetworkData::kRouterRoleOnly) >=
             kRouterUpgradeBorderRouterRequestThreshold))
        {
            LogInfo("Rejecting BR %s router role req - have %d BR routers", extAddress.ToString().AsCString(),
                    kRouterUpgradeBorderRouterRequestThreshold);
            ExitNow();
        }
        break;

    default:
        responseStatus = ThreadStatusTlv::kUnrecognizedStatus;
        ExitNow();
    }

    if (rloc16 != Mac::kShortAddrInvalid)
    {
        router = mRouterTable.Allocate(RouterIdFromRloc16(rloc16));

        if (router != nullptr)
        {
            LogInfo("Router id %d requested and provided!", RouterIdFromRloc16(rloc16));
        }
    }

    if (router == nullptr)
    {
        router = mRouterTable.Allocate();
        VerifyOrExit(router != nullptr);
    }

    router->SetExtAddress(extAddress);
    responseStatus = ThreadStatusTlv::kSuccess;

exit:
    if (error == kErrorNone)
    {
        SendAddressSolicitResponse(aMessage, responseStatus, router, aMessageInfo);
    }
}

void MleRouter::SendAddressSolicitResponse(const Coap::Message &   aRequest,
                                           ThreadStatusTlv::Status aResponseStatus,
                                           const Router *          aRouter,
                                           const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Message *message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);

    VerifyOrExit(message != nullptr);

    SuccessOrExit(Tlv::Append<ThreadStatusTlv>(*message, aResponseStatus));

    if (aRouter != nullptr)
    {
        ThreadRouterMaskTlv routerMaskTlv;

        SuccessOrExit(Tlv::Append<ThreadRloc16Tlv>(*message, aRouter->GetRloc16()));

        routerMaskTlv.Init();
        routerMaskTlv.SetIdSequence(mRouterTable.GetRouterIdSequence());
        routerMaskTlv.SetAssignedRouterIdMask(mRouterTable.GetRouterIdSet());

        SuccessOrExit(routerMaskTlv.AppendTo(*message));
    }

    SuccessOrExit(Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));
    message = nullptr;

    Log(kMessageSend, kTypeAddressReply, aMessageInfo.GetPeerAddr());

exit:
    FreeMessage(message);
}

void MleRouter::HandleAddressRelease(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<MleRouter *>(aContext)->HandleAddressRelease(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void MleRouter::HandleAddressRelease(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t        rloc16;
    Mac::ExtAddress extAddress;
    uint8_t         routerId;
    Router *        router;

    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    Log(kMessageReceive, kTypeAddressRelease, aMessageInfo.GetPeerAddr());

    SuccessOrExit(Tlv::Find<ThreadRloc16Tlv>(aMessage, rloc16));
    SuccessOrExit(Tlv::Find<ThreadExtMacAddressTlv>(aMessage, extAddress));

    routerId = RouterIdFromRloc16(rloc16);
    router   = mRouterTable.GetRouter(routerId);

    VerifyOrExit((router != nullptr) && (router->GetExtAddress() == extAddress));

    IgnoreError(mRouterTable.Release(routerId));

    SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

    Log(kMessageSend, kTypeAddressReleaseReply, aMessageInfo.GetPeerAddr());

exit:
    return;
}

void MleRouter::FillConnectivityTlv(ConnectivityTlv &aTlv)
{
    Router *leader;
    uint8_t cost;
    int8_t  parentPriority = kParentPriorityMedium;

    if (mParentPriority != kParentPriorityUnspecified)
    {
        parentPriority = mParentPriority;
    }
    else
    {
        uint16_t numChildren = mChildTable.GetNumChildren(Child::kInStateValid);
        uint16_t maxAllowed  = mChildTable.GetMaxChildrenAllowed();

        if ((maxAllowed - numChildren) < (maxAllowed / 3))
        {
            parentPriority = kParentPriorityLow;
        }
        else
        {
            parentPriority = kParentPriorityMedium;
        }
    }

    aTlv.SetParentPriority(parentPriority);

    // compute leader cost and link qualities
    aTlv.SetLinkQuality1(0);
    aTlv.SetLinkQuality2(0);
    aTlv.SetLinkQuality3(0);

    leader = mRouterTable.GetLeader();
    cost   = (leader != nullptr) ? leader->GetCost() : static_cast<uint8_t>(kMaxRouteCost);

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        cost = static_cast<uint8_t>(kMaxRouteCost);
        break;

    case kRoleChild:
        switch (mParent.GetLinkInfo().GetLinkQuality())
        {
        case kLinkQuality0:
            break;

        case kLinkQuality1:
            aTlv.SetLinkQuality1(aTlv.GetLinkQuality1() + 1);
            break;

        case kLinkQuality2:
            aTlv.SetLinkQuality2(aTlv.GetLinkQuality2() + 1);
            break;

        case kLinkQuality3:
            aTlv.SetLinkQuality3(aTlv.GetLinkQuality3() + 1);
            break;
        }

        cost += LinkQualityToCost(mParent.GetLinkInfo().GetLinkQuality());
        break;

    case kRoleRouter:
        if (leader != nullptr)
        {
            cost += GetLinkCost(leader->GetNextHop());

            if (!IsRouterIdValid(leader->GetNextHop()) || GetLinkCost(GetLeaderId()) < cost)
            {
                cost = GetLinkCost(GetLeaderId());
            }
        }

        break;

    case kRoleLeader:
        cost = 0;
        break;
    }

    aTlv.SetActiveRouters(mRouterTable.GetActiveRouterCount());

    for (Router &router : Get<RouterTable>().Iterate())
    {
        LinkQuality linkQuality;

        if (router.GetRloc16() == GetRloc16())
        {
            // skip self
            continue;
        }

        if (!router.IsStateValid())
        {
            // skip non-neighbor routers
            continue;
        }

        linkQuality = router.GetLinkInfo().GetLinkQuality();

        if (linkQuality > router.GetLinkQualityOut())
        {
            linkQuality = router.GetLinkQualityOut();
        }

        switch (linkQuality)
        {
        case kLinkQuality0:
            break;

        case kLinkQuality1:
            aTlv.SetLinkQuality1(aTlv.GetLinkQuality1() + 1);
            break;

        case kLinkQuality2:
            aTlv.SetLinkQuality2(aTlv.GetLinkQuality2() + 1);
            break;

        case kLinkQuality3:
            aTlv.SetLinkQuality3(aTlv.GetLinkQuality3() + 1);
            break;
        }
    }

    aTlv.SetLeaderCost((cost < kMaxRouteCost) ? cost : static_cast<uint8_t>(kMaxRouteCost));
    aTlv.SetIdSequence(mRouterTable.GetRouterIdSequence());
    aTlv.SetSedBufferSize(OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE);
    aTlv.SetSedDatagramCount(OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT);
}

Error Mle::TxMessage::AppendConnectivityTlv(void)
{
    ConnectivityTlv tlv;

    tlv.Init();
    Get<MleRouter>().FillConnectivityTlv(tlv);

    return tlv.AppendTo(*this);
}

Error Mle::TxMessage::AppendAddresseRegisterationTlv(Child &aChild)
{
    Error                    error;
    Tlv                      tlv;
    AddressRegistrationEntry entry;
    Lowpan::Context          context;
    uint8_t                  length      = 0;
    uint16_t                 startOffset = GetLength();

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = Append(tlv));

    for (const Ip6::Address &address : aChild.IterateIp6Addresses())
    {
        if (address.IsMulticast() || Get<NetworkData::Leader>().GetContext(address, context) != kErrorNone)
        {
            // uncompressed entry
            entry.SetUncompressed();
            entry.SetIp6Address(address);
        }
        else if (context.mContextId != kMeshLocalPrefixContextId)
        {
            // compressed entry
            entry.SetContextId(context.mContextId);
            entry.SetIid(address.GetIid());
        }
        else
        {
            continue;
        }

        SuccessOrExit(error = AppendBytes(&entry, entry.GetLength()));
        length += entry.GetLength();
    }

    tlv.SetLength(length);
    Write(startOffset, tlv);

exit:
    return error;
}

void MleRouter::FillRouteTlv(RouteTlv &aTlv, Neighbor *aNeighbor)
{
    uint8_t     routerIdSequence = mRouterTable.GetRouterIdSequence();
    RouterIdSet routerIdSet      = mRouterTable.GetRouterIdSet();
    uint8_t     routerCount;

    if (aNeighbor && IsActiveRouter(aNeighbor->GetRloc16()))
    {
        // Sending a Link Accept message that may require truncation
        // of Route64 TLV

        routerCount = mRouterTable.GetActiveRouterCount();

        if (routerCount > kLinkAcceptMaxRouters)
        {
            for (uint8_t routerId = 0; routerId <= kMaxRouterId; routerId++)
            {
                if (routerCount <= kLinkAcceptMaxRouters)
                {
                    break;
                }

                if ((routerId == RouterIdFromRloc16(GetRloc16())) || (routerId == aNeighbor->GetRouterId()) ||
                    (routerId == GetLeaderId()))
                {
                    // Route64 TLV must contain this device and the
                    // neighboring router to ensure that at least this
                    // link can be established.
                    continue;
                }

                if (routerIdSet.Contains(routerId))
                {
                    routerIdSet.Remove(routerId);
                    routerCount--;
                }
            }

            // Ensure that the neighbor will process the current
            // Route64 TLV in a subsequent message exchange
            routerIdSequence -= kLinkAcceptSequenceRollback;
        }
    }

    aTlv.SetRouterIdSequence(routerIdSequence);
    aTlv.SetRouterIdMask(routerIdSet);

    routerCount = 0;

    for (Router &router : Get<RouterTable>().Iterate())
    {
        if (!routerIdSet.Contains(router.GetRouterId()))
        {
            continue;
        }

        if (router.GetRloc16() == GetRloc16())
        {
            aTlv.SetLinkQualityIn(routerCount, kLinkQuality0);
            aTlv.SetLinkQualityOut(routerCount, kLinkQuality0);
            aTlv.SetRouteCost(routerCount, 1);
        }
        else
        {
            Router *nextHop;
            uint8_t linkCost;
            uint8_t routeCost;

            linkCost = mRouterTable.GetLinkCost(router);
            nextHop  = mRouterTable.GetRouter(router.GetNextHop());

            if (nextHop == nullptr)
            {
                routeCost = linkCost;
            }
            else
            {
                routeCost = router.GetCost() + mRouterTable.GetLinkCost(*nextHop);

                if (linkCost < routeCost)
                {
                    routeCost = linkCost;
                }
            }

            if (routeCost >= kMaxRouteCost)
            {
                routeCost = 0;
            }

            aTlv.SetRouteCost(routerCount, routeCost);
            aTlv.SetLinkQualityOut(routerCount, router.GetLinkQualityOut());
            aTlv.SetLinkQualityIn(routerCount, router.GetLinkInfo().GetLinkQuality());
        }

        routerCount++;
    }

    aTlv.SetRouteDataLength(routerCount);
}

Error Mle::TxMessage::AppendRouteTlv(Neighbor *aNeighbor)
{
    RouteTlv tlv;

    tlv.Init();
    Get<MleRouter>().FillRouteTlv(tlv, aNeighbor);

    return tlv.AppendTo(*this);
}

Error Mle::TxMessage::AppendActiveDatasetTlv(void)
{
    return Get<MeshCoP::ActiveDatasetManager>().AppendMleDatasetTlv(*this);
}

Error Mle::TxMessage::AppendPendingDatasetTlv(void)
{
    return Get<MeshCoP::PendingDatasetManager>().AppendMleDatasetTlv(*this);
}

bool MleRouter::HasMinDowngradeNeighborRouters(void)
{
    uint8_t linkQuality;
    uint8_t routerCount = 0;

    for (Router &router : Get<RouterTable>().Iterate())
    {
        if (!router.IsStateValid())
        {
            continue;
        }

        linkQuality = router.GetLinkInfo().GetLinkQuality();

        if (linkQuality > router.GetLinkQualityOut())
        {
            linkQuality = router.GetLinkQualityOut();
        }

        if (linkQuality >= 2)
        {
            routerCount++;
        }
    }

    return routerCount >= kMinDowngradeNeighbors;
}

bool MleRouter::HasOneNeighborWithComparableConnectivity(const RouteTlv &aRoute, uint8_t aRouterId)
{
    uint8_t routerCount = 0;
    bool    rval        = true;

    // process local neighbor routers
    for (Router &router : Get<RouterTable>().Iterate())
    {
        uint8_t localLinkQuality;
        uint8_t peerLinkQuality;

        if (!router.IsStateValid() || router.GetRouterId() == mRouterId || router.GetRouterId() == aRouterId)
        {
            routerCount++;
            continue;
        }

        localLinkQuality = router.GetLinkInfo().GetLinkQuality();

        if (localLinkQuality > router.GetLinkQualityOut())
        {
            localLinkQuality = router.GetLinkQualityOut();
        }

        if (localLinkQuality < 2)
        {
            routerCount++;
            continue;
        }

        // check if this neighbor router is in peer Route64 TLV
        if (!aRoute.IsRouterIdSet(router.GetRouterId()))
        {
            ExitNow(rval = false);
        }

        // get the peer's two-way link quality to this router
        peerLinkQuality = aRoute.GetLinkQualityIn(routerCount);

        if (peerLinkQuality > aRoute.GetLinkQualityOut(routerCount))
        {
            peerLinkQuality = aRoute.GetLinkQualityOut(routerCount);
        }

        // compare local link quality to this router with peer's
        if (peerLinkQuality >= localLinkQuality)
        {
            routerCount++;
            continue;
        }

        ExitNow(rval = false);
    }

exit:
    return rval;
}

void MleRouter::SetChildStateToValid(Child &aChild)
{
    VerifyOrExit(!aChild.IsStateValid());

    aChild.SetState(Neighbor::kStateValid);
    IgnoreError(mChildTable.StoreChild(aChild));

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    Get<MlrManager>().UpdateProxiedSubscriptions(aChild, nullptr, 0);
#endif

    mNeighborTable.Signal(NeighborTable::kChildAdded, aChild);

exit:
    return;
}

bool MleRouter::HasChildren(void)
{
    return mChildTable.HasChildren(Child::kInStateValidOrAttaching);
}

void MleRouter::RemoveChildren(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
    {
        RemoveNeighbor(child);
    }
}

bool MleRouter::HasSmallNumberOfChildren(void)
{
    uint16_t numChildren = 0;
    uint8_t  routerCount = mRouterTable.GetActiveRouterCount();

    VerifyOrExit(routerCount > mRouterDowngradeThreshold);

    numChildren = mChildTable.GetNumChildren(Child::kInStateValid);

    return numChildren < (routerCount - mRouterDowngradeThreshold) * 3;

exit:
    return false;
}

Error MleRouter::SetAssignParentPriority(int8_t aParentPriority)
{
    Error error = kErrorNone;

    VerifyOrExit(aParentPriority <= kParentPriorityHigh && aParentPriority >= kParentPriorityUnspecified,
                 error = kErrorInvalidArgs);

    mParentPriority = aParentPriority;

exit:
    return error;
}

Error MleRouter::GetMaxChildTimeout(uint32_t &aTimeout) const
{
    Error error = kErrorNotFound;

    aTimeout = 0;

    VerifyOrExit(IsRouterOrLeader(), error = kErrorInvalidState);

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (child.IsFullThreadDevice())
        {
            continue;
        }

        if (child.GetTimeout() > aTimeout)
        {
            aTimeout = child.GetTimeout();
        }

        error = kErrorNone;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
void MleRouter::HandleTimeSync(RxInfo &aRxInfo)
{
    Log(kMessageReceive, kTypeTimeSync, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid());

    Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);

exit:
    return;
}

Error MleRouter::SendTimeSync(void)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage *  message = nullptr;

    VerifyOrExit((message = NewMleMessage(kCommandTimeSync)) != nullptr, error = kErrorNoBufs);

    message->SetTimeSync(true);

    destination.SetToLinkLocalAllNodesMulticast();
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeTimeSync, destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

} // namespace Mle
} // namespace ot

#endif // OPENTHREAD_FTD
