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
#include "common/locator_getters.hpp"
#include "common/num_utils.hpp"
#include "common/random.hpp"
#include "common/serial_number.hpp"
#include "common/settings.hpp"
#include "instance/instance.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop.hpp"
#include "net/icmp6.hpp"
#include "thread/key_manager.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/time_sync_service.hpp"
#include "thread/uri_paths.hpp"
#include "thread/version.hpp"
#include "utils/otns.hpp"

namespace ot {
namespace Mle {

RegisterLogModule("Mle");

MleRouter::MleRouter(Instance &aInstance)
    : Mle(aInstance)
    , mAdvertiseTrickleTimer(aInstance, MleRouter::HandleAdvertiseTrickleTimer)
    , mChildTable(aInstance)
    , mRouterTable(aInstance)
    , mChallengeTimeout(0)
    , mNextChildId(kMaxChildId)
    , mNetworkIdTimeout(kNetworkIdTimeout)
    , mRouterUpgradeThreshold(kRouterUpgradeThreshold)
    , mRouterDowngradeThreshold(kRouterDowngradeThreshold)
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
    , mChildRouterLinks(kChildRouterLinks)
    , mParentPriority(kParentPriorityUnspecified)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mMaxChildIpAddresses(0)
#endif
{
    mDeviceMode.Set(mDeviceMode.Get() | DeviceMode::kModeFullThreadDevice | DeviceMode::kModeFullNetworkData);

#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
    mLeaderWeight = mDeviceProperties.CalculateLeaderWeight();
#else
    mLeaderWeight = kDefaultLeaderWeight;
#endif

    mLeaderAloc.InitAsThreadOriginMeshLocal();

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

void MleRouter::HandleSecurityPolicyChanged(void)
{
    // If we are currently router or leader and no longer eligible to
    // be a router (due to security policy change), we start jitter
    // timeout to downgrade.

    VerifyOrExit(IsRouterOrLeader() && !IsRouterEligible());

    VerifyOrExit(!mRouterRoleTransition.IsPending());

    mRouterRoleTransition.StartTimeout();

    if (IsLeader())
    {
        mRouterRoleTransition.IncreaseTimeout(kLeaderDowngradeExtraDelay);
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE
void MleRouter::SetDeviceProperties(const DeviceProperties &aDeviceProperties)
{
    mDeviceProperties = aDeviceProperties;
    mDeviceProperties.ClampWeightAdjustment();
    SetLeaderWeight(mDeviceProperties.CalculateLeaderWeight());
}
#endif

Error MleRouter::BecomeRouter(ThreadStatusTlv::Status aStatus)
{
    Error error = kErrorNone;

    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsRouter(), error = kErrorNone);
    VerifyOrExit(IsRouterEligible(), error = kErrorNotCapable);

    LogInfo("Attempt to become router");

    Get<MeshForwarder>().SetRxOnWhenIdle(true);
    mRouterRoleTransition.StopTimeout();

    switch (mRole)
    {
    case kRoleDetached:
        // If router had more than `kMinCriticalChildrenCount` children
        // or was a leader prior to reset we treat the multicast Link
        // Request as a critical message.
        mLinkRequestAttempts =
            (mWasLeader || mChildTable.GetNumChildren(Child::kInStateValidOrRestoring) >= kMinCriticalChildrenCount)
                ? kMaxCriticalTxCount
                : kMaxTxCount;

        SuccessOrExit(error = SendLinkRequest(nullptr));
        mLinkRequestAttempts--;
        ScheduleMessageTransmissionTimer();
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMleRouter);
        break;

    case kRoleChild:
        SuccessOrExit(error = SendAddressSolicit(aStatus));
        break;

    default:
        OT_ASSERT(false);
    }

exit:
    return error;
}

Error MleRouter::BecomeLeader(void)
{
    Error    error = kErrorNone;
    Router  *router;
    uint32_t partitionId;
    uint8_t  leaderId;

#if OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT
    VerifyOrExit(!Get<MeshCoP::ActiveDatasetManager>().IsPartiallyComplete(), error = kErrorInvalidState);
#else
    VerifyOrExit(Get<MeshCoP::ActiveDatasetManager>().IsComplete(), error = kErrorInvalidState);
#endif
    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsLeader(), error = kErrorNone);
    VerifyOrExit(IsRouterEligible(), error = kErrorNotCapable);

    mRouterTable.Clear();

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    {
        uint8_t minId;
        uint8_t maxId;

        mRouterTable.GetRouterIdRange(minId, maxId);
        partitionId = mPreferredLeaderPartitionId ? mPreferredLeaderPartitionId : Random::NonCrypto::GetUint32();
        leaderId    = (IsRouterIdValid(mPreviousRouterId) && minId <= mPreviousRouterId && mPreviousRouterId <= maxId)
                          ? mPreviousRouterId
                          : Random::NonCrypto::GetUint8InRange(minId, maxId + 1);
    }
#else
    partitionId = Random::NonCrypto::GetUint32();
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

    SetStateLeader(Rloc16FromRouterId(leaderId), kStartingAsLeader);

exit:
    return error;
}

void MleRouter::StopLeader(void)
{
    StopAdvertiseTrickleTimer();
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
    mAddressSolicitRejected = false;

    mRouterRoleTransition.StartTimeout();

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

        if (HasChildren())
        {
            RemoveChildren();
        }

        SetRouterId(kInvalidRouterId);
        break;

    case kSamePartition:
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
    // The `aStartMode` is ignored when used with `kRoleRouter`
    SetStateRouterOrLeader(kRoleRouter, aRloc16, /* aStartMode */ kStartingAsLeader);
}

void MleRouter::SetStateLeader(uint16_t aRloc16, LeaderStartMode aStartMode)
{
    SetStateRouterOrLeader(kRoleLeader, aRloc16, aStartMode);
}

void MleRouter::SetStateRouterOrLeader(DeviceRole aRole, uint16_t aRloc16, LeaderStartMode aStartMode)
{
    if (aRole == kRoleLeader)
    {
        IgnoreError(Get<MeshCoP::ActiveDatasetManager>().Restore());
        IgnoreError(Get<MeshCoP::PendingDatasetManager>().Restore());
    }

    SetRloc16(aRloc16);

    SetRole(aRole);

    SetAttachState(kAttachStateIdle);
    mAttachCounter = 0;
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    StopAdvertiseTrickleTimer();
    ResetAdvertiseInterval();

    Get<ThreadNetif>().SubscribeAllRoutersMulticast();
    mPreviousPartitionIdRouter = mLeaderData.GetPartitionId();
    Get<Mac::Mac>().SetBeaconEnabled(true);

    if (aRole == kRoleLeader)
    {
        IgnoreError(GetLeaderAloc(mLeaderAloc.GetAddress()));
        Get<ThreadNetif>().AddUnicastAddress(mLeaderAloc);
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMleRouter);
        Get<NetworkData::Leader>().Start(aStartMode);
        Get<MeshCoP::ActiveDatasetManager>().StartLeader();
        Get<MeshCoP::PendingDatasetManager>().StartLeader();
        Get<AddressResolver>().Clear();
    }

    // Remove children that do not have matching RLOC16
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
    {
        if (RouterIdFromRloc16(child.GetRloc16()) != mRouterId)
        {
            RemoveNeighbor(child);
        }
    }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    Get<Mac::Mac>().UpdateCsl();
#endif

    LogNote("Partition ID 0x%lx", ToUlong(mLeaderData.GetPartitionId()));
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

void MleRouter::StopAdvertiseTrickleTimer(void) { mAdvertiseTrickleTimer.Stop(); }

uint32_t MleRouter::DetermineAdvertiseIntervalMax(void) const
{
    uint32_t interval;

#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    interval = kAdvIntervalMaxLogRoutes;
#else
    // Determine the interval based on the number of router neighbors
    // with link quality 2 or higher.

    interval = (Get<RouterTable>().GetNeighborCount(kLinkQuality2) + 1) * kAdvIntervalNeighborMultiplier;
    interval = Clamp(interval, kAdvIntervalMaxLowerBound, kAdvIntervalMaxUpperBound);
#endif

    return interval;
}

void MleRouter::UpdateAdvertiseInterval(void)
{
    if (IsRouterOrLeader() && mAdvertiseTrickleTimer.IsRunning())
    {
        mAdvertiseTrickleTimer.SetIntervalMax(DetermineAdvertiseIntervalMax());
    }
}

void MleRouter::ResetAdvertiseInterval(void)
{
    VerifyOrExit(IsRouterOrLeader());

    if (!mAdvertiseTrickleTimer.IsRunning())
    {
        mAdvertiseTrickleTimer.Start(TrickleTimer::kModeTrickle, kAdvIntervalMin, DetermineAdvertiseIntervalMax());
    }

    mAdvertiseTrickleTimer.IndicateInconsistent();

exit:
    return;
}

void MleRouter::SendAdvertisement(void)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage   *message = nullptr;

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

    VerifyOrExit((message = NewMleMessage(kCommandAdvertisement)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());

    switch (mRole)
    {
    case kRoleChild:
        break;

    case kRoleRouter:
    case kRoleLeader:
        SuccessOrExit(error = message->AppendRouteTlv());
        break;

    case kRoleDisabled:
    case kRoleDetached:
        OT_ASSERT(false);
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
    static const uint8_t kDetachedTlvs[]      = {Tlv::kAddress16, Tlv::kRoute};
    static const uint8_t kRouterTlvs[]        = {Tlv::kLinkMargin};
    static const uint8_t kValidNeighborTlvs[] = {Tlv::kLinkMargin, Tlv::kRoute};

    Error        error   = kErrorNone;
    TxMessage   *message = nullptr;
    Ip6::Address destination;

    VerifyOrExit(mChallengeTimeout == 0);

    destination.Clear();

    VerifyOrExit((message = NewMleMessage(kCommandLinkRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendVersionTlv());

    switch (mRole)
    {
    case kRoleDetached:
        SuccessOrExit(error = message->AppendTlvRequestTlv(kDetachedTlvs));
        break;

    case kRoleChild:
        SuccessOrExit(error = message->AppendSourceAddressTlv());
        SuccessOrExit(error = message->AppendLeaderDataTlv());
        break;

    case kRoleRouter:
    case kRoleLeader:
        if (aNeighbor == nullptr || !aNeighbor->IsStateValid())
        {
            SuccessOrExit(error = message->AppendTlvRequestTlv(kRouterTlvs));
        }
        else
        {
            SuccessOrExit(error = message->AppendTlvRequestTlv(kValidNeighborTlvs));
        }

        SuccessOrExit(error = message->AppendSourceAddressTlv());
        SuccessOrExit(error = message->AppendLeaderDataTlv());
        break;

    case kRoleDisabled:
        OT_ASSERT(false);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = message->AppendTimeRequestTlv());
#endif

    if (aNeighbor == nullptr)
    {
        mChallenge.GenerateRandom();
        mChallengeTimeout = kChallengeTimeout;

        SuccessOrExit(error = message->AppendChallengeTlv(mChallenge));
        destination.SetToLinkLocalAllRoutersMulticast();
    }
    else
    {
        if (!aNeighbor->IsStateValid())
        {
            aNeighbor->GenerateChallenge();
            SuccessOrExit(error = message->AppendChallengeTlv(aNeighbor->GetChallenge()));
        }
        else
        {
            TxChallenge challenge;

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
    Error       error    = kErrorNone;
    Neighbor   *neighbor = nullptr;
    RxChallenge challenge;
    uint16_t    version;
    LeaderData  leaderData;
    uint16_t    sourceAddress;
    TlvList     requestedTlvList;

    Log(kMessageReceive, kTypeLinkRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(IsRouterOrLeader(), error = kErrorInvalidState);

    VerifyOrExit(!IsAttaching(), error = kErrorInvalidState);

    // Challenge
    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

    // Version
    SuccessOrExit(error = Tlv::Find<VersionTlv>(aRxInfo.mMessage, version));
    VerifyOrExit(version >= kThreadVersion1p1, error = kErrorParse);

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
            neighbor = mRouterTable.FindRouterByRloc16(sourceAddress);
            VerifyOrExit(neighbor != nullptr, error = kErrorParse);
            VerifyOrExit(!neighbor->IsStateLinkRequest(), error = kErrorAlready);

            if (!neighbor->IsStateValid())
            {
                InitNeighbor(*neighbor, aRxInfo);
                neighbor->SetState(Neighbor::kStateLinkRequest);
            }
            else
            {
                Mac::ExtAddress extAddr;

                aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);
                VerifyOrExit(neighbor->GetExtAddress() == extAddr);
            }
        }

        break;

    case kErrorNotFound:
        // lack of source address indicates router coming out of reset
        VerifyOrExit(aRxInfo.IsNeighborStateValid() && IsActiveRouter(aRxInfo.mNeighbor->GetRloc16()),
                     error = kErrorDrop);
        neighbor = aRxInfo.mNeighbor;
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // TLV Request
    switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvList))
    {
    case kErrorNone:
    case kErrorNotFound:
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

    aRxInfo.mClass = RxInfo::kPeerMessage;
    ProcessKeySequence(aRxInfo);

    SuccessOrExit(error = SendLinkAccept(aRxInfo.mMessageInfo, neighbor, requestedTlvList, challenge));

exit:
    LogProcessError(kTypeLinkRequest, error);
}

Error MleRouter::SendLinkAccept(const Ip6::MessageInfo &aMessageInfo,
                                Neighbor               *aNeighbor,
                                const TlvList          &aRequestedTlvList,
                                const RxChallenge      &aChallenge)
{
    static const uint8_t kRouterTlvs[] = {Tlv::kLinkMargin};

    Error      error = kErrorNone;
    TxMessage *message;
    Command    command;
    uint8_t    linkMargin;

    command = (aNeighbor == nullptr || aNeighbor->IsStateValid()) ? kCommandLinkAccept : kCommandLinkAcceptAndRequest;

    VerifyOrExit((message = NewMleMessage(command)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendVersionTlv());
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendResponseTlv(aChallenge));
    SuccessOrExit(error = message->AppendLinkFrameCounterTlv());
    SuccessOrExit(error = message->AppendMleFrameCounterTlv());

    // always append a link margin, regardless of whether or not it was requested
    linkMargin = Get<Mac::Mac>().ComputeLinkMargin(aMessageInfo.GetThreadLinkInfo()->GetRss());

    SuccessOrExit(error = message->AppendLinkMarginTlv(linkMargin));

    if (aNeighbor != nullptr && IsActiveRouter(aNeighbor->GetRloc16()))
    {
        SuccessOrExit(error = message->AppendLeaderDataTlv());
    }

    for (uint8_t tlvType : aRequestedTlvList)
    {
        switch (tlvType)
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

        SuccessOrExit(error = message->AppendChallengeTlv(aNeighbor->GetChallenge()));
        SuccessOrExit(error = message->AppendTlvRequestTlv(kRouterTlvs));
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
                                                      1 + Random::NonCrypto::GetUint16InRange(0, kMaxLinkAcceptDelay)));

        Log(kMessageDelay, (command == kCommandLinkAccept) ? kTypeLinkAccept : kTypeLinkAcceptAndRequest,
            aMessageInfo.GetPeerAddr());
    }
    else
    {
        SuccessOrExit(error = message->SendTo(aMessageInfo.GetPeerAddr()));

        Log(kMessageSend, (command == kCommandLinkAccept) ? kTypeLinkAccept : kTypeLinkAcceptAndRequest,
            aMessageInfo.GetPeerAddr());
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
    Error           error = kErrorNone;
    Router         *router;
    Neighbor::State neighborState;
    uint16_t        version;
    RxChallenge     response;
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
    router        = mRouterTable.FindRouterById(routerId);
    neighborState = (router != nullptr) ? router->GetState() : Neighbor::kStateInvalid;

    // Response
    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));

    // verify response
    switch (neighborState)
    {
    case Neighbor::kStateLinkRequest:
        VerifyOrExit(response == router->GetChallenge(), error = kErrorSecurity);
        break;

    case Neighbor::kStateInvalid:
        VerifyOrExit((mLinkRequestAttempts > 0 || mChallengeTimeout > 0) && (response == mChallenge),
                     error = kErrorSecurity);

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
    VerifyOrExit(version >= kThreadVersion1p1, error = kErrorParse);

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
    case kRoleDetached:
        // Address16
        SuccessOrExit(error = Tlv::Find<Address16Tlv>(aRxInfo.mMessage, address16));
        VerifyOrExit(GetRloc16() == address16, error = kErrorDrop);

        // Leader Data
        SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));
        SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

        // Route
        mRouterTable.Clear();
        SuccessOrExit(error = aRxInfo.mMessage.ReadRouteTlv(routeTlv));
        SuccessOrExit(error = ProcessRouteTlv(routeTlv, aRxInfo));
        router = mRouterTable.FindRouterById(routerId);
        VerifyOrExit(router != nullptr);

        if (mLeaderData.GetLeaderRouterId() == RouterIdFromRloc16(GetRloc16()))
        {
            SetStateLeader(GetRloc16(), kRestoringLeaderRoleAfterReset);
        }
        else
        {
            SetStateRouter(GetRloc16());
        }

        mLinkRequestAttempts    = 0; // completed router sync after reset, no more link request to retransmit
        mRetrieveNewNetworkData = true;
        IgnoreError(SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr()));

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
            IgnoreError(SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr()));
        }

        // Route (optional)
        switch (aRxInfo.mMessage.ReadRouteTlv(routeTlv))
        {
        case kErrorNone:
            VerifyOrExit(routeTlv.IsRouterIdSet(routerId), error = kErrorParse);

            if (mRouterTable.IsRouteTlvIdSequenceMoreRecent(routeTlv))
            {
                SuccessOrExit(error = ProcessRouteTlv(routeTlv, aRxInfo));
                router = mRouterTable.FindRouterById(routerId);
                OT_ASSERT(router != nullptr);
            }

            mRouterTable.UpdateRoutes(routeTlv, routerId);
            break;

        case kErrorNotFound:
            break;

        default:
            ExitNow(error = kErrorParse);
        }

        if (routerId != mRouterId && !IsRouterIdValid(router->GetNextHop()))
        {
            ResetAdvertiseInterval();
        }

        break;

    case kRoleDisabled:
        OT_ASSERT(false);
    }

    // finish link synchronization
    InitNeighbor(*router, aRxInfo);
    router->SetRloc16(sourceAddress);
    router->GetLinkFrameCounters().SetAll(linkFrameCounter);
    router->SetLinkAckFrameCounter(linkFrameCounter);
    router->SetMleFrameCounter(mleFrameCounter);
    router->SetVersion(version);
    router->SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                     DeviceMode::kModeFullNetworkData));
    router->SetLinkQualityOut(LinkQualityForLinkMargin(linkMargin));
    router->SetState(Neighbor::kStateValid);
    router->SetKeySequence(aRxInfo.mKeySequence);

    mNeighborTable.Signal(NeighborTable::kRouterAdded, *router);

    aRxInfo.mClass = RxInfo::kAuthoritativeMessage;
    ProcessKeySequence(aRxInfo);

    if (aRequest)
    {
        RxChallenge challenge;
        TlvList     requestedTlvList;

        // Challenge
        SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

        // TLV Request
        switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvList))
        {
        case kErrorNone:
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }

        SuccessOrExit(error = SendLinkAccept(aRxInfo.mMessageInfo, router, requestedTlvList, challenge));
    }

exit:
    return error;
}

Error MleRouter::ProcessRouteTlv(const RouteTlv &aRouteTlv, RxInfo &aRxInfo)
{
    // This method processes `aRouteTlv` read from an MLE message.
    //
    // During processing of Route TLV, the entries in the router table
    // may shuffle. This method ensures that the `aRxInfo.mNeighbor`
    // (which indicates the neighbor from which the MLE message was
    // received) is correctly updated to point to the same neighbor
    // (in case `mNeighbor` was pointing to a router entry from the
    // `RouterTable`).

    Error    error          = kErrorNone;
    uint16_t neighborRloc16 = Mac::kShortAddrInvalid;

    if ((aRxInfo.mNeighbor != nullptr) && Get<RouterTable>().Contains(*aRxInfo.mNeighbor))
    {
        neighborRloc16 = aRxInfo.mNeighbor->GetRloc16();
    }

    mRouterTable.UpdateRouterIdSet(aRouteTlv.GetRouterIdSequence(), aRouteTlv.GetRouterIdMask());

    if (IsRouter() && !mRouterTable.IsAllocated(mRouterId))
    {
        IgnoreError(BecomeDetached());
        error = kErrorNoRoute;
    }

    if (neighborRloc16 != Mac::kShortAddrInvalid)
    {
        aRxInfo.mNeighbor = Get<NeighborTable>().FindNeighbor(neighborRloc16);
    }

    return error;
}

Error MleRouter::ReadAndProcessRouteTlvOnFed(RxInfo &aRxInfo, uint8_t aParentId)
{
    // This method reads and processes Route TLV from message on an
    // FED if message contains one. It returns `kErrorNone` when
    // successfully processed or if there is no Route TLV in the
    // message.
    //
    // It MUST be used only when device is acting as a child and
    // for a message received from device's current parent.

    Error    error = kErrorNone;
    RouteTlv routeTlv;

    VerifyOrExit(IsFullThreadDevice());

    switch (aRxInfo.mMessage.ReadRouteTlv(routeTlv))
    {
    case kErrorNone:
        SuccessOrExit(error = ProcessRouteTlv(routeTlv, aRxInfo));
        mRouterTable.UpdateRoutesOnFed(routeTlv, aParentId);
        mRequestRouteTlv = false;
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

exit:
    return error;
}

bool MleRouter::IsSingleton(void) const
{
    bool isSingleton = true;

    VerifyOrExit(IsAttached() && IsRouterEligible());
    isSingleton = (mRouterTable.GetActiveRouterCount() <= 1);

exit:
    return isSingleton;
}

int MleRouter::ComparePartitions(bool              aSingletonA,
                                 const LeaderData &aLeaderDataA,
                                 bool              aSingletonB,
                                 const LeaderData &aLeaderDataB)
{
    int rval = 0;

    rval = ThreeWayCompare(aLeaderDataA.GetWeighting(), aLeaderDataB.GetWeighting());
    VerifyOrExit(rval == 0);

    // Not being a singleton is better.
    rval = ThreeWayCompare(!aSingletonA, !aSingletonB);
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aLeaderDataA.GetPartitionId(), aLeaderDataB.GetPartitionId());

exit:
    return rval;
}

Error MleRouter::HandleAdvertisement(RxInfo &aRxInfo, uint16_t aSourceAddress, const LeaderData &aLeaderData)
{
    // This method processes a received MLE Advertisement message on
    // an FTD device. It is called from `Mle::HandleAdvertisement()`
    // only when device is attached (in child, router, or leader roles)
    // and `IsFullThreadDevice()`.
    //
    // - `aSourceAddress` is the read value from `SourceAddressTlv`.
    // - `aLeaderData` is the read value from `LeaderDataTlv`.

    Error    error      = kErrorNone;
    uint8_t  linkMargin = Get<Mac::Mac>().ComputeLinkMargin(aRxInfo.mMessageInfo.GetThreadLinkInfo()->GetRss());
    RouteTlv routeTlv;
    Router  *router;
    uint8_t  routerId;

    switch (aRxInfo.mMessage.ReadRouteTlv(routeTlv))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        routeTlv.SetLength(0); // Mark that a Route TLV was not included.
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Handle Partition ID mismatch

    if (aLeaderData.GetPartitionId() != mLeaderData.GetPartitionId())
    {
        LogNote("Different partition (peer:%lu, local:%lu)", ToUlong(aLeaderData.GetPartitionId()),
                ToUlong(mLeaderData.GetPartitionId()));

        VerifyOrExit(linkMargin >= kPartitionMergeMinMargin, error = kErrorLinkMarginLow);

        if (routeTlv.IsValid() && (mPreviousPartitionIdTimeout > 0) &&
            (aLeaderData.GetPartitionId() == mPreviousPartitionId))
        {
            VerifyOrExit(SerialNumber::IsGreater(routeTlv.GetRouterIdSequence(), mPreviousPartitionRouterIdSequence),
                         error = kErrorDrop);
        }

        if (IsChild() && (aRxInfo.mNeighbor == &mParent))
        {
            ExitNow();
        }

        if (ComparePartitions(routeTlv.IsSingleton(), aLeaderData, IsSingleton(), mLeaderData) > 0
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

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Handle Leader Router ID mismatch

    if (aLeaderData.GetLeaderRouterId() != GetLeaderId())
    {
        VerifyOrExit(aRxInfo.IsNeighborStateValid());

        if (!IsChild())
        {
            LogInfo("Leader ID mismatch");
            IgnoreError(BecomeDetached());
            error = kErrorDrop;
        }

        ExitNow();
    }

    VerifyOrExit(IsActiveRouter(aSourceAddress) && routeTlv.IsValid());
    routerId = RouterIdFromRloc16(aSourceAddress);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);
#endif

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Process `RouteTlv`

    if (aRxInfo.IsNeighborStateValid() && mRouterTable.IsRouteTlvIdSequenceMoreRecent(routeTlv))
    {
        SuccessOrExit(error = ProcessRouteTlv(routeTlv, aRxInfo));
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Update routers as a child

    if (IsChild())
    {
        if (aRxInfo.mNeighbor == &mParent)
        {
            // MLE Advertisement from parent
            router = &mParent;

            if (mParent.GetRloc16() != aSourceAddress)
            {
                IgnoreError(BecomeDetached());
                ExitNow(error = kErrorDetached);
            }

            if (!mRouterRoleTransition.IsPending() && (mRouterTable.GetActiveRouterCount() < mRouterUpgradeThreshold))
            {
                mRouterRoleTransition.StartTimeout();
            }

            mRouterTable.UpdateRoutesOnFed(routeTlv, routerId);
        }
        else
        {
            // MLE Advertisement not from parent, but from some other neighboring router
            router = mRouterTable.FindRouterById(routerId);
            VerifyOrExit(router != nullptr);

            if (!router->IsStateValid() && !router->IsStateLinkRequest() &&
                (mRouterTable.GetNeighborCount(kLinkQuality1) < mChildRouterLinks))
            {
                InitNeighbor(*router, aRxInfo);
                router->SetState(Neighbor::kStateLinkRequest);
                IgnoreError(SendLinkRequest(router));
                ExitNow(error = kErrorNoRoute);
            }
        }

        router->SetLastHeard(TimerMilli::GetNow());

        ExitNow();
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Update routers as a router or leader.

    if (IsRouter() && ShouldDowngrade(routerId, routeTlv))
    {
        mRouterRoleTransition.StartTimeout();
    }

    router = mRouterTable.FindRouterById(routerId);
    VerifyOrExit(router != nullptr);

    if (!router->IsStateValid() && aRxInfo.IsNeighborStateValid() && Get<ChildTable>().Contains(*aRxInfo.mNeighbor))
    {
        // The Adv is from a former child that is now acting as a router,
        // we copy the info from child entry and update the RLOC16.

        *static_cast<Neighbor *>(router) = *aRxInfo.mNeighbor;
        router->SetRloc16(Rloc16FromRouterId(routerId));
        router->SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                         DeviceMode::kModeFullNetworkData));

        mNeighborTable.Signal(NeighborTable::kRouterAdded, *router);

        // Change the cache entries associated with the former child
        // from using the old RLOC16 to its new RLOC16.
        Get<AddressResolver>().ReplaceEntriesForRloc16(aRxInfo.mNeighbor->GetRloc16(), router->GetRloc16());
    }

    // Send unicast link request if no link to router and no unicast/multicast link request in progress
    if (!router->IsStateValid() && !router->IsStateLinkRequest() && (mChallengeTimeout == 0) &&
        (linkMargin >= kLinkRequestMinMargin))
    {
        InitNeighbor(*router, aRxInfo);
        router->SetState(Neighbor::kStateLinkRequest);
        IgnoreError(SendLinkRequest(router));
        ExitNow(error = kErrorNoRoute);
    }

    router->SetLastHeard(TimerMilli::GetNow());

    mRouterTable.UpdateRoutes(routeTlv, routerId);

exit:
    if (aRxInfo.mNeighbor && aRxInfo.mNeighbor->GetRloc16() != aSourceAddress)
    {
        // Remove stale neighbors
        RemoveNeighbor(*aRxInfo.mNeighbor);
    }

    return error;
}

void MleRouter::HandleParentRequest(RxInfo &aRxInfo)
{
    Error           error = kErrorNone;
    Mac::ExtAddress extAddr;
    uint16_t        version;
    uint8_t         scanMask;
    RxChallenge     challenge;
    Child          *child;
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
    VerifyOrExit(mRouterTable.GetPathCostToLeader() < kMaxRouteCost, error = kErrorDrop);

    // 4. It is a REED and there are already `kMaxRouters` active routers in
    // the network (because Leader would reject any further address solicit).
    // ==> Verified below when checking the scan mask.

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);

    // Version
    SuccessOrExit(error = Tlv::Find<VersionTlv>(aRxInfo.mMessage, version));
    VerifyOrExit(version >= kThreadVersion1p1, error = kErrorParse);

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
        InitNeighbor(*child, aRxInfo);
        child->SetState(Neighbor::kStateParentRequest);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        child->SetTimeSyncEnabled(Tlv::Find<TimeRequestTlv>(aRxInfo.mMessage, nullptr, 0) == kErrorNone);
#endif
        if (Tlv::Find<ModeTlv>(aRxInfo.mMessage, modeBitmask) == kErrorNone)
        {
            mode.Set(modeBitmask);
            child->SetDeviceMode(mode);
            child->SetVersion(version);
        }
    }
    else if (TimerMilli::GetNow() - child->GetLastHeard() < kParentRequestRouterTimeout - kParentRequestDuplicateMargin)
    {
        ExitNow(error = kErrorDuplicated);
    }

    if (!child->IsStateValidOrRestoring())
    {
        child->SetLastHeard(TimerMilli::GetNow());
        child->SetTimeout(Time::MsecToSec(kChildIdRequestTimeout));
    }

    aRxInfo.mClass = RxInfo::kPeerMessage;
    ProcessKeySequence(aRxInfo);

    SendParentResponse(child, challenge, !ScanMaskTlv::IsEndDeviceFlagSet(scanMask));

exit:
    LogProcessError(kTypeParentRequest, error);
}

bool MleRouter::HasNeighborWithGoodLinkQuality(void) const
{
    bool    haveNeighbor = true;
    uint8_t linkMargin;

    linkMargin = Get<Mac::Mac>().ComputeLinkMargin(mParent.GetLinkInfo().GetLastRss());

    if (linkMargin >= kLinkRequestMinMargin)
    {
        ExitNow();
    }

    for (const Router &router : Get<RouterTable>())
    {
        if (!router.IsStateValid())
        {
            continue;
        }

        linkMargin = Get<Mac::Mac>().ComputeLinkMargin(router.GetLinkInfo().GetLastRss());

        if (linkMargin >= kLinkRequestMinMargin)
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
    bool roleTransitionTimeoutExpired = false;

    VerifyOrExit(IsFullThreadDevice(), Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMleRouter));

    if (mChallengeTimeout > 0)
    {
        mChallengeTimeout--;
    }

    if (mPreviousPartitionIdTimeout > 0)
    {
        mPreviousPartitionIdTimeout--;
    }

    roleTransitionTimeoutExpired = mRouterRoleTransition.HandleTimeTick();

    switch (mRole)
    {
    case kRoleDetached:
        if (mChallengeTimeout == 0 && mLinkRequestAttempts == 0)
        {
            IgnoreError(BecomeDetached());
            ExitNow();
        }

        break;

    case kRoleChild:
        if (roleTransitionTimeoutExpired)
        {
            if (mRouterTable.GetActiveRouterCount() < mRouterUpgradeThreshold && HasNeighborWithGoodLinkQuality())
            {
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

                mAdvertiseTrickleTimer.Start(TrickleTimer::kModePlainTimer, kReedAdvIntervalMin, kReedAdvIntervalMax);
            }

            ExitNow();
        }

        OT_FALL_THROUGH;

    case kRoleRouter:
        LogDebg("network id timeout = %lu", ToUlong(mRouterTable.GetLeaderAge()));

        if ((mRouterTable.GetActiveRouterCount() > 0) && (mRouterTable.GetLeaderAge() >= mNetworkIdTimeout))
        {
            LogInfo("Router ID Sequence timeout");
            Attach(kSamePartition);
        }

        if (roleTransitionTimeoutExpired && mRouterTable.GetActiveRouterCount() > mRouterDowngradeThreshold)
        {
            LogNote("Downgrade to REED");
            Attach(kDowngradeToReed);
        }

        OT_FALL_THROUGH;

    case kRoleLeader:
        if (roleTransitionTimeoutExpired && !IsRouterEligible())
        {
            LogInfo("No longer router eligible");
            IgnoreError(BecomeDetached());
        }

        break;

    case kRoleDisabled:
        OT_ASSERT(false);
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
    for (Router &router : Get<RouterTable>())
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

            if (age >= kMaxNeighborAge)
            {
                LogInfo("Router timeout expired");
                RemoveNeighbor(router);
                continue;
            }

#else

            if (age >= kMaxNeighborAge)
            {
                if (age < kMaxNeighborAge + kMaxTxCount * kUnicastRetxDelay)
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
            if (age >= kLinkRequestTimeout)
            {
                LogInfo("Link Request timeout expired");
                RemoveNeighbor(router);
                continue;
            }
        }

        if (IsLeader())
        {
            if (mRouterTable.FindNextHopOf(router) == nullptr && mRouterTable.GetLinkCost(router) >= kMaxRouteCost &&
                age >= kMaxLeaderToRouterTimeout)
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

void MleRouter::SendParentResponse(Child *aChild, const RxChallenge &aChallenge, bool aRoutersOnlyRequest)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage   *message;
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
    SuccessOrExit(error = message->AppendChallengeTlv(aChild->GetChallenge()));
    SuccessOrExit(error = message->AppendLinkMarginTlv(aChild->GetLinkInfo().GetLinkMargin()));
    SuccessOrExit(error = message->AppendConnectivityTlv());
    SuccessOrExit(error = message->AppendVersionTlv());

    destination.SetToLinkLocalAddress(aChild->GetExtAddress());

    delay = 1 + Random::NonCrypto::GetUint16InRange(0, aRoutersOnlyRequest ? kParentResponseMaxDelayRouters
                                                                           : kParentResponseMaxDelayAll);

    SuccessOrExit(error = message->SendAfterDelay(destination, delay));

    Log(kMessageDelay, kTypeParentResponse, destination);

exit:
    FreeMessageOnError(message, error);
    LogSendError(kTypeParentResponse, error);
}

uint8_t MleRouter::GetMaxChildIpAddresses(void) const
{
    uint8_t num = kMaxChildIpAddresses;

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

    VerifyOrExit(aMaxIpAddresses <= kMaxChildIpAddresses, error = kErrorInvalidArgs);

    mMaxChildIpAddresses = aMaxIpAddresses;

exit:
    return error;
}
#endif

Error MleRouter::ProcessAddressRegistrationTlv(RxInfo &aRxInfo, Child &aChild)
{
    Error    error;
    uint16_t offset;
    uint16_t endOffset;
    uint8_t  count       = 0;
    uint8_t  storedCount = 0;
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    bool         hasOldDua = false;
    bool         hasNewDua = false;
    Ip6::Address oldDua;
#endif
#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    MlrManager::MlrAddressArray oldMlrRegisteredAddresses;
#endif

    SuccessOrExit(error =
                      Tlv::FindTlvValueStartEndOffsets(aRxInfo.mMessage, Tlv::kAddressRegistration, offset, endOffset));

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    {
        const Ip6::Address *duaAddress = aChild.GetDomainUnicastAddress();

        if (duaAddress != nullptr)
        {
            oldDua    = *duaAddress;
            hasOldDua = true;
        }
    }
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    // Retrieve registered multicast addresses of the Child
    if (aChild.HasAnyMlrRegisteredAddress())
    {
        OT_ASSERT(aChild.IsStateValid());

        for (const Ip6::Address &childAddress :
             aChild.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            if (aChild.GetAddressMlrState(childAddress) == kMlrStateRegistered)
            {
                IgnoreError(oldMlrRegisteredAddresses.PushBack(childAddress));
            }
        }
    }
#endif

    aChild.ClearIp6Addresses();

    while (offset < endOffset)
    {
        uint8_t      controlByte;
        Ip6::Address address;

        // Read out the control byte (first byte in entry)
        SuccessOrExit(error = aRxInfo.mMessage.Read(offset, controlByte));
        offset++;
        count++;

        address.Clear();

        if (AddressRegistrationTlv::IsEntryCompressed(controlByte))
        {
            // Compressed entry contains IID with the 64-bit prefix
            // determined from 6LoWPAN context identifier (from
            // the control byte).

            uint8_t         contextId = AddressRegistrationTlv::GetContextId(controlByte);
            Lowpan::Context context;

            VerifyOrExit(offset + sizeof(Ip6::InterfaceIdentifier) <= endOffset, error = kErrorParse);
            IgnoreError(aRxInfo.mMessage.Read(offset, address.GetIid()));
            offset += sizeof(Ip6::InterfaceIdentifier);

            if (Get<NetworkData::Leader>().GetContext(contextId, context) != kErrorNone)
            {
                LogWarn("Failed to get context %u for compressed address from child 0x%04x", contextId,
                        aChild.GetRloc16());
                continue;
            }

            address.SetPrefix(context.mPrefix);
        }
        else
        {
            // Uncompressed entry contains the full IPv6 address.

            VerifyOrExit(offset + sizeof(Ip6::Address) <= endOffset, error = kErrorParse);
            IgnoreError(aRxInfo.mMessage.Read(offset, address));
            offset += sizeof(Ip6::Address);
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
            LogInfo("Child 0x%04x IPv6 address[%u]=%s", aChild.GetRloc16(), storedCount,
                    address.ToString().AsCString());
        }
        else
        {
            LogWarn("Error %s adding IPv6 address %s to child 0x%04x", ErrorToString(error),
                    address.ToString().AsCString(), aChild.GetRloc16());
        }

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
        if (Get<BackboneRouter::Leader>().IsDomainUnicast(address))
        {
            if (error == kErrorNone)
            {
                DuaManager::ChildDuaAddressEvent event;

                hasNewDua = true;

                if (hasOldDua)
                {
                    event = (oldDua != address) ? DuaManager::kAddressChanged : DuaManager::kAddressUnchanged;
                }
                else
                {
                    event = DuaManager::kAddressAdded;
                }

                Get<DuaManager>().HandleChildDuaAddressEvent(aChild, event);
            }
            else
            {
                // if not able to store DUA, then assume child does not have one
                hasNewDua = false;
            }
        }
#endif

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
        Get<AddressResolver>().RemoveEntryForAddress(address);
    }
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    if (hasOldDua && !hasNewDua)
    {
        Get<DuaManager>().HandleChildDuaAddressEvent(aChild, DuaManager::kAddressRemoved);
    }
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    Get<MlrManager>().UpdateProxiedSubscriptions(aChild, oldMlrRegisteredAddresses);
#endif

    if (count == 0)
    {
        LogInfo("Child 0x%04x has no registered IPv6 address", aChild.GetRloc16());
    }
    else
    {
        LogInfo("Child 0x%04x has %u registered IPv6 address%s, %u address%s stored", aChild.GetRloc16(), count,
                (count == 1) ? "" : "es", storedCount, (storedCount == 1) ? "" : "es");
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
    RxChallenge        response;
    uint32_t           linkFrameCounter;
    uint32_t           mleFrameCounter;
    uint8_t            modeBitmask;
    DeviceMode         mode;
    uint32_t           timeout;
    TlvList            tlvList;
    MeshCoP::Timestamp timestamp;
    Child             *child;
    Router            *router;
    uint16_t           supervisionInterval;

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
    VerifyOrExit(version >= kThreadVersion1p1, error = kErrorParse);

    // Response
    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));
    VerifyOrExit(response == child->GetChallenge(), error = kErrorSecurity);

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

    // Requested TLVs
    SuccessOrExit(error = aRxInfo.mMessage.ReadTlvRequestTlv(tlvList));

    // Supervision interval
    switch (Tlv::Find<SupervisionIntervalTlv>(aRxInfo.mMessage, supervisionInterval))
    {
    case kErrorNone:
        tlvList.Add(Tlv::kSupervisionInterval);
        break;
    case kErrorNotFound:
        supervisionInterval = (version <= kThreadVersion1p3) ? kChildSupervisionDefaultIntervalForOlderVersion : 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

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
        tlvList.Add(Tlv::kActiveDataset);
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
        tlvList.Add(Tlv::kPendingDataset);
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    VerifyOrExit(tlvList.GetLength() <= Child::kMaxRequestTlvs, error = kErrorParse);

    if (!mode.IsFullThreadDevice())
    {
        SuccessOrExit(error = ProcessAddressRegistrationTlv(aRxInfo, *child));
    }

    // Remove from router table
    router = mRouterTable.FindRouter(extAddr);

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
    child->SetVersion(version);
    child->GetLinkInfo().AddRss(aRxInfo.mMessageInfo.GetThreadLinkInfo()->GetRss());
    child->SetTimeout(timeout);
    child->SetSupervisionInterval(supervisionInterval);
#if OPENTHREAD_CONFIG_MULTI_RADIO
    child->ClearLastRxFragmentTag();
#endif

    child->SetNetworkDataVersion(mLeaderData.GetDataVersion(mode.GetNetworkDataType()));

    // We already checked above that `tlvList` will fit in
    // `child` entry (with `Child::kMaxRequestTlvs` TLVs).

    child->ClearRequestTlvs();

    for (uint8_t index = 0; index < tlvList.GetLength(); index++)
    {
        child->SetRequestTlv(index, tlvList[index]);
    }

    aRxInfo.mClass = RxInfo::kAuthoritativeMessage;
    ProcessKeySequence(aRxInfo);

    switch (mRole)
    {
    case kRoleChild:
        child->SetState(Neighbor::kStateChildIdRequest);
        IgnoreError(BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest));
        break;

    case kRoleRouter:
    case kRoleLeader:
        SuccessOrExit(error = SendChildIdResponse(*child));
        break;

    case kRoleDisabled:
    case kRoleDetached:
        OT_ASSERT(false);
    }

exit:
    LogProcessError(kTypeChildIdRequest, error);
}

void MleRouter::HandleChildUpdateRequest(RxInfo &aRxInfo)
{
    Error           error = kErrorNone;
    Mac::ExtAddress extAddr;
    uint8_t         modeBitmask;
    DeviceMode      mode;
    RxChallenge     challenge;
    LeaderData      leaderData;
    uint32_t        timeout;
    uint16_t        supervisionInterval;
    Child          *child;
    DeviceMode      oldMode;
    TlvList         requestedTlvList;
    TlvList         tlvList;
    bool            childDidChange = false;

    Log(kMessageReceive, kTypeChildUpdateRequestOfChild, aRxInfo.mMessageInfo.GetPeerAddr());

    // Mode
    SuccessOrExit(error = Tlv::Find<ModeTlv>(aRxInfo.mMessage, modeBitmask));
    mode.Set(modeBitmask);

    // Challenge
    switch (aRxInfo.mMessage.ReadChallengeTlv(challenge))
    {
    case kErrorNone:
        tlvList.Add(Tlv::kResponse);
        break;
    case kErrorNotFound:
        challenge.Clear();
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    tlvList.Add(Tlv::kSourceAddress);

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);
    child = mChildTable.FindChild(extAddr, Child::kInStateAnyExceptInvalid);

    if (child == nullptr)
    {
        // For invalid non-sleepy child, send Child Update Response with
        // Status TLV (error).
        if (mode.IsRxOnWhenIdle())
        {
            tlvList.Add(Tlv::kStatus);
            SendChildUpdateResponse(nullptr, aRxInfo.mMessageInfo, tlvList, challenge);
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

    tlvList.Add(Tlv::kMode);

    // Parent MUST include Leader Data TLV in Child Update Response
    tlvList.Add(Tlv::kLeaderData);

    if (!challenge.IsEmpty())
    {
        tlvList.Add(Tlv::kMleFrameCounter);
        tlvList.Add(Tlv::kLinkFrameCounter);
    }

    // IPv6 Address TLV
    switch (ProcessAddressRegistrationTlv(aRxInfo, *child))
    {
    case kErrorNone:
        tlvList.Add(Tlv::kAddressRegistration);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
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

        tlvList.Add(Tlv::kTimeout);
        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // Supervision interval
    switch (Tlv::Find<SupervisionIntervalTlv>(aRxInfo.mMessage, supervisionInterval))
    {
    case kErrorNone:
        tlvList.Add(Tlv::kSupervisionInterval);
        break;

    case kErrorNotFound:
        supervisionInterval =
            (child->GetVersion() <= kThreadVersion1p3) ? kChildSupervisionDefaultIntervalForOlderVersion : 0;
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    child->SetSupervisionInterval(supervisionInterval);

    // TLV Request
    switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvList))
    {
    case kErrorNone:
        tlvList.AddElementsFrom(requestedTlvList);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    if (child->IsCslSynchronized())
    {
        ChannelTlvValue cslChannelTlvValue;
        uint32_t        cslTimeout;

        switch (Tlv::Find<CslTimeoutTlv>(aRxInfo.mMessage, cslTimeout))
        {
        case kErrorNone:
            child->SetCslTimeout(cslTimeout);
            // MUST include CSL accuracy TLV when request includes CSL timeout
            tlvList.Add(Tlv::kCslClockAccuracy);
            break;
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorNone);
        }

        if (Tlv::Find<CslChannelTlv>(aRxInfo.mMessage, cslChannelTlvValue) == kErrorNone)
        {
            // Special value of zero is used to indicate that
            // CSL channel is not specified.
            child->SetCslChannel(static_cast<uint8_t>(cslChannelTlvValue.GetChannel()));
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
    if (!challenge.IsEmpty())
    {
        child->ClearLastRxFragmentTag();
    }
#endif

    SendChildUpdateResponse(child, aRxInfo.mMessageInfo, tlvList, challenge);

    aRxInfo.mClass = RxInfo::kPeerMessage;

exit:
    LogProcessError(kTypeChildUpdateRequestOfChild, error);
}

void MleRouter::HandleChildUpdateResponse(RxInfo &aRxInfo)
{
    Error       error = kErrorNone;
    uint16_t    sourceAddress;
    uint32_t    timeout;
    RxChallenge response;
    uint8_t     status;
    uint32_t    linkFrameCounter;
    uint32_t    mleFrameCounter;
    LeaderData  leaderData;
    Child      *child;

    if ((aRxInfo.mNeighbor == nullptr) || IsActiveRouter(aRxInfo.mNeighbor->GetRloc16()) ||
        !Get<ChildTable>().Contains(*aRxInfo.mNeighbor))
    {
        Log(kMessageReceive, kTypeChildUpdateResponseOfUnknownChild, aRxInfo.mMessageInfo.GetPeerAddr());
        ExitNow(error = kErrorNotFound);
    }

    child = static_cast<Child *>(aRxInfo.mNeighbor);

    // Response
    switch (aRxInfo.mMessage.ReadResponseTlv(response))
    {
    case kErrorNone:
        VerifyOrExit(response == child->GetChallenge(), error = kErrorSecurity);
        break;
    case kErrorNotFound:
        VerifyOrExit(child->IsStateValid(), error = kErrorSecurity);
        response.Clear();
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
    switch (Tlv::Find<StatusTlv>(aRxInfo.mMessage, status))
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

    {
        uint16_t supervisionInterval;

        switch (Tlv::Find<SupervisionIntervalTlv>(aRxInfo.mMessage, supervisionInterval))
        {
        case kErrorNone:
            child->SetSupervisionInterval(supervisionInterval);
            break;
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }
    }

    // IPv6 Address
    switch (ProcessAddressRegistrationTlv(aRxInfo, *child))
    {
    case kErrorNone:
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
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

    aRxInfo.mClass = response.IsEmpty() ? RxInfo::kPeerMessage : RxInfo::kAuthoritativeMessage;

exit:
    LogProcessError(kTypeChildUpdateResponseOfChild, error);
}

void MleRouter::HandleDataRequest(RxInfo &aRxInfo)
{
    Error              error = kErrorNone;
    TlvList            tlvList;
    MeshCoP::Timestamp timestamp;

    Log(kMessageReceive, kTypeDataRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.IsNeighborStateValid(), error = kErrorSecurity);

    // TLV Request
    SuccessOrExit(error = aRxInfo.mMessage.ReadTlvRequestTlv(tlvList));

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
        tlvList.Add(Tlv::kActiveDataset);
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
        tlvList.Add(Tlv::kPendingDataset);
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    aRxInfo.mClass = RxInfo::kPeerMessage;
    ProcessKeySequence(aRxInfo);

    SendDataResponse(aRxInfo.mMessageInfo.GetPeerAddr(), tlvList, /* aDelay */ 0, &aRxInfo.mMessage);

exit:
    LogProcessError(kTypeDataRequest, error);
}

void MleRouter::HandleNetworkDataUpdateRouter(void)
{
    Ip6::Address destination;
    uint16_t     delay;
    TlvList      tlvList;

    VerifyOrExit(IsRouterOrLeader());

    destination.SetToLinkLocalAllNodesMulticast();
    tlvList.Add(Tlv::kNetworkData);

    delay = IsLeader() ? 0 : Random::NonCrypto::GetUint16InRange(0, kUnsolicitedDataResponseJitter);
    SendDataResponse(destination, tlvList, delay);

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
    MeshCoP::Tlv                 meshcopTlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequestTlv;
    MeshCoP::ExtendedPanId       extPanId;
    uint16_t                     offset;
    uint16_t                     end;

    Log(kMessageReceive, kTypeDiscoveryRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    discoveryRequestTlv.SetLength(0);

    // only Routers and REEDs respond
    VerifyOrExit(IsRouterEligible(), error = kErrorInvalidState);

    SuccessOrExit(error = Tlv::FindTlvValueStartEndOffsets(aRxInfo.mMessage, Tlv::kDiscovery, offset, end));

    while (offset < end)
    {
        IgnoreError(aRxInfo.mMessage.Read(offset, meshcopTlv));

        switch (meshcopTlv.GetType())
        {
        case MeshCoP::Tlv::kDiscoveryRequest:
            IgnoreError(aRxInfo.mMessage.Read(offset, discoveryRequestTlv));
            VerifyOrExit(discoveryRequestTlv.IsValid(), error = kErrorParse);

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

    if (discoveryRequestTlv.IsValid())
    {
        if (mDiscoveryRequestCallback.IsSet())
        {
            otThreadDiscoveryRequestInfo info;

            aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(AsCoreType(&info.mExtAddress));
            info.mVersion  = discoveryRequestTlv.GetVersion();
            info.mIsJoiner = discoveryRequestTlv.IsJoiner();

            mDiscoveryRequestCallback.Invoke(&info);
        }

        if (discoveryRequestTlv.IsJoiner())
        {
#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
            if (!mSteeringData.IsEmpty())
            {
            }
            else // if steering data is not set out of band, fall back to network data
#endif
            {
                VerifyOrExit(Get<NetworkData::Leader>().IsJoiningAllowed(), error = kErrorSecurity);
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
    TxMessage                    *message;
    uint16_t                      startOffset;
    Tlv                           tlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponseTlv;
    MeshCoP::NetworkNameTlv       networkNameTlv;
    uint16_t                      delay;

    VerifyOrExit((message = NewMleMessage(kCommandDiscoveryResponse)) != nullptr, error = kErrorNoBufs);
    message->SetDirectTransmission();
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
    discoveryResponseTlv.Init();
    discoveryResponseTlv.SetVersion(kThreadVersion);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    if (Get<KeyManager>().GetSecurityPolicy().mNativeCommissioningEnabled)
    {
        SuccessOrExit(
            error = Tlv::Append<MeshCoP::CommissionerUdpPortTlv>(*message, Get<MeshCoP::BorderAgent>().GetUdpPort()));

        discoveryResponseTlv.SetNativeCommissioner(true);
    }
    else
#endif
    {
        discoveryResponseTlv.SetNativeCommissioner(false);
    }

    if (Get<KeyManager>().GetSecurityPolicy().mCommercialCommissioningEnabled)
    {
        discoveryResponseTlv.SetCommercialCommissioningMode(true);
    }

    SuccessOrExit(error = discoveryResponseTlv.AppendTo(*message));

    // Extended PAN ID TLV
    SuccessOrExit(
        error = Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()));

    // Network Name TLV
    networkNameTlv.Init();
    networkNameTlv.SetNetworkName(Get<MeshCoP::NetworkNameManager>().GetNetworkName().GetAsData());
    SuccessOrExit(error = networkNameTlv.AppendTo(*message));

    SuccessOrExit(error = message->AppendSteeringDataTlv());

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
    TxMessage   *message;

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

        case Tlv::kSupervisionInterval:
            SuccessOrExit(error = message->AppendSupervisionIntervalTlv(aChild.GetSupervisionInterval()));
            break;

        default:
            break;
        }
    }

    if (!aChild.IsFullThreadDevice())
    {
        SuccessOrExit(error = message->AppendAddressRegistrationTlv(aChild));
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
    static const uint8_t kTlvs[] = {Tlv::kTimeout, Tlv::kAddressRegistration};

    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage   *message = nullptr;

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
        SuccessOrExit(error = message->AppendTlvRequestTlv(kTlvs));
        aChild.GenerateChallenge();
        SuccessOrExit(error = message->AppendChallengeTlv(aChild.GetChallenge()));
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

void MleRouter::SendChildUpdateResponse(Child                  *aChild,
                                        const Ip6::MessageInfo &aMessageInfo,
                                        const TlvList          &aTlvList,
                                        const RxChallenge      &aChallenge)
{
    Error      error = kErrorNone;
    TxMessage *message;

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateResponse)) != nullptr, error = kErrorNoBufs);

    for (uint8_t tlvType : aTlvList)
    {
        // Add all TLV types that do not depend on `child`

        switch (tlvType)
        {
        case Tlv::kStatus:
            SuccessOrExit(error = message->AppendStatusTlv(StatusTlv::kError));
            break;

        case Tlv::kLeaderData:
            SuccessOrExit(error = message->AppendLeaderDataTlv());
            break;

        case Tlv::kResponse:
            SuccessOrExit(error = message->AppendResponseTlv(aChallenge));
            break;

        case Tlv::kSourceAddress:
            SuccessOrExit(error = message->AppendSourceAddressTlv());
            break;

        case Tlv::kMleFrameCounter:
            SuccessOrExit(error = message->AppendMleFrameCounterTlv());
            break;

        case Tlv::kLinkFrameCounter:
            SuccessOrExit(error = message->AppendLinkFrameCounterTlv());
            break;
        }

        // Make sure `child` is not null before adding TLV types
        // that can depend on it.

        if (aChild == nullptr)
        {
            continue;
        }

        switch (tlvType)
        {
        case Tlv::kAddressRegistration:
            SuccessOrExit(error = message->AppendAddressRegistrationTlv(*aChild));
            break;

        case Tlv::kMode:
            SuccessOrExit(error = message->AppendModeTlv(aChild->GetDeviceMode()));
            break;

        case Tlv::kNetworkData:
            SuccessOrExit(error = message->AppendNetworkDataTlv(aChild->GetNetworkDataType()));
            SuccessOrExit(error = message->AppendActiveTimestampTlv());
            SuccessOrExit(error = message->AppendPendingTimestampTlv());
            break;

        case Tlv::kTimeout:
            SuccessOrExit(error = message->AppendTimeoutTlv(aChild->GetTimeout()));
            break;

        case Tlv::kSupervisionInterval:
            SuccessOrExit(error = message->AppendSupervisionIntervalTlv(aChild->GetSupervisionInterval()));
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
                                 const TlvList      &aTlvList,
                                 uint16_t            aDelay,
                                 const Message      *aRequestMessage)
{
    OT_UNUSED_VARIABLE(aRequestMessage);

    Error      error   = kErrorNone;
    TxMessage *message = nullptr;
    Neighbor  *neighbor;

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

    for (uint8_t tlvType : aTlvList)
    {
        switch (tlvType)
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

        case Tlv::kRoute:
            SuccessOrExit(error = message->AppendRouteTlv());
            break;

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        case Tlv::kLinkMetricsReport:
            OT_ASSERT(aRequestMessage != nullptr);
            neighbor = mNeighborTable.FindNeighbor(aDestination);
            VerifyOrExit(neighbor != nullptr, error = kErrorInvalidState);
            SuccessOrExit(error = Get<LinkMetrics::Subject>().AppendReport(*message, *aRequestMessage, *neighbor));
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

    case kRoleRouter:
    case kRoleLeader:
        mRouterTable.RemoveRouterLink(aRouter);
        break;

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
    else if (&aNeighbor == &GetParentCandidate())
    {
        ClearParentCandidate();
    }
    else if (!IsActiveRouter(aNeighbor.GetRloc16()))
    {
        OT_ASSERT(mChildTable.Contains(aNeighbor));

        if (aNeighbor.IsStateValidOrRestoring())
        {
            mNeighborTable.Signal(NeighborTable::kChildRemoved, aNeighbor);
        }

        Get<IndirectSender>().ClearAllMessagesForSleepyChild(static_cast<Child &>(aNeighbor));

        if (aNeighbor.IsFullThreadDevice())
        {
            // Clear all EID-to-RLOC entries associated with the child.
            Get<AddressResolver>().RemoveEntriesForRloc16(aNeighbor.GetRloc16());
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
    router = mRouterTable.FindRouterByRloc16(aDestRloc16);
    VerifyOrExit(router != nullptr);

    // invalidate next hop
    router->SetNextHopToInvalid();
    ResetAdvertiseInterval();

exit:
    return;
}

Error MleRouter::CheckReachability(uint16_t aMeshDest, const Ip6::Header &aIp6Header)
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
    Coap::Message   *message = nullptr;

    VerifyOrExit(!mAddressSolicitPending);

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriAddressSolicit);
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
    Coap::Message   *message;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriAddressRelease);
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

void MleRouter::HandleAddressSolicitResponse(void                *aContext,
                                             otMessage           *aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             Error                aResult)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicitResponse(AsCoapMessagePtr(aMessage),
                                                                     AsCoreTypePtr(aMessageInfo), aResult);
}

void MleRouter::HandleAddressSolicitResponse(Coap::Message          *aMessage,
                                             const Ip6::MessageInfo *aMessageInfo,
                                             Error                   aResult)
{
    uint8_t             status;
    uint16_t            rloc16;
    ThreadRouterMaskTlv routerMaskTlv;
    uint8_t             routerId;
    Router             *router;

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

    // We keep the router table next hop and cost as what we had as a
    // REED, i.e., our parent was the next hop towards all other
    // routers and we tracked its cost towards them. As FED, we may
    // have established links with a subset of neighboring routers.
    // We ensure to clear these links to avoid using them (since will
    // be rejected by the neighbor).

    mRouterTable.ClearNeighbors();

    mRouterTable.UpdateRouterIdSet(routerMaskTlv.GetIdSequence(), routerMaskTlv.GetAssignedRouterIdMask());

    router = mRouterTable.FindRouterById(routerId);
    VerifyOrExit(router != nullptr);
    router->SetExtAddress(Get<Mac::Mac>().GetExtAddress());
    router->SetNextHopToInvalid();

    // Ensure we have our parent as a neighboring router, copying the
    // `mParent` entry.

    router = mRouterTable.FindRouterById(mParent.GetRouterId());
    VerifyOrExit(router != nullptr);
    router->SetFrom(mParent);
    router->SetState(Neighbor::kStateValid);
    router->SetNextHopToInvalid();

    // Ensure we have a next hop and cost towards leader.
    if (mRouterTable.GetPathCostToLeader() >= kMaxRouteCost)
    {
        Router *leader = mRouterTable.GetLeader();

        OT_ASSERT(leader != nullptr);
        leader->SetNextHopAndCost(RouterIdFromRloc16(mParent.GetRloc16()), mParent.GetLeaderCost());
    }

    // We send a unicast Link Request to our former parent if its
    // version is earlier than 1.3. This is to address a potential
    // compatibility issue with some non-OpenThread stacks which may
    // ignore MLE Advertisements from a former/existing child.

    if (mParent.GetVersion() < kThreadVersion1p3)
    {
        IgnoreError(SendLinkRequest(&mParent));
    }

    // We send an Advertisement to inform our former parent of our
    // newly allocated Router ID. This will cause the parent to
    // reset its advertisement trickle timer which can help speed
    // up the dissemination of the new Router ID to other routers.
    // This can also help with quicker link establishment with our
    // former parent and other routers.
    SendAdvertisement();

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateChildIdRequest))
    {
        IgnoreError(SendChildIdResponse(child));
    }

exit:
    // Send announce after received address solicit reply if needed
    InformPreviousChannel();
}

Error MleRouter::SetChildRouterLinks(uint8_t aChildRouterLinks)
{
    Error error = kErrorNone;

    VerifyOrExit(IsDisabled(), error = kErrorInvalidState);
    mChildRouterLinks = aChildRouterLinks;
exit:
    return error;
}

bool MleRouter::IsExpectedToBecomeRouterSoon(void) const
{
    static constexpr uint8_t kMaxDelay = 10;

    return IsRouterEligible() && IsChild() && !mAddressSolicitRejected &&
           ((mRouterRoleTransition.IsPending() && mRouterRoleTransition.GetTimeout() <= kMaxDelay) ||
            mAddressSolicitPending);
}

template <> void MleRouter::HandleTmf<kUriAddressSolicit>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                   error          = kErrorNone;
    ThreadStatusTlv::Status responseStatus = ThreadStatusTlv::kNoAddressAvailable;
    Router                 *router         = nullptr;
    Mac::ExtAddress         extAddress;
    uint16_t                rloc16;
    uint8_t                 status;

    VerifyOrExit(mRole == kRoleLeader, error = kErrorInvalidState);

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
    router = mRouterTable.FindRouter(extAddress);

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

    case ThreadStatusTlv::kBorderRouterRequest:
        if ((mRouterTable.GetActiveRouterCount() >= mRouterUpgradeThreshold) &&
            (Get<NetworkData::Leader>().CountBorderRouters(NetworkData::kRouterRoleOnly) >=
             kRouterUpgradeBorderRouterRequestThreshold))
        {
            LogInfo("Rejecting BR %s router role req - have %u BR routers", extAddress.ToString().AsCString(),
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
            LogInfo("Router id %u requested and provided!", RouterIdFromRloc16(rloc16));
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

void MleRouter::SendAddressSolicitResponse(const Coap::Message    &aRequest,
                                           ThreadStatusTlv::Status aResponseStatus,
                                           const Router           *aRouter,
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
        mRouterTable.GetRouterIdSet(routerMaskTlv.GetAssignedRouterIdMask());

        SuccessOrExit(routerMaskTlv.AppendTo(*message));
    }

    SuccessOrExit(Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));
    message = nullptr;

    Log(kMessageSend, kTypeAddressReply, aMessageInfo.GetPeerAddr());

    // If assigning a new RLOC16 (e.g., on promotion of a child to
    // router role) we clear any address cache entries associated
    // with the old RLOC16.

    if ((aResponseStatus == ThreadStatusTlv::kSuccess) && (aRouter != nullptr))
    {
        uint16_t oldRloc16;

        VerifyOrExit(IsRoutingLocator(aMessageInfo.GetPeerAddr()));
        oldRloc16 = aMessageInfo.GetPeerAddr().GetIid().GetLocator();

        VerifyOrExit(oldRloc16 != aRouter->GetRloc16());
        Get<AddressResolver>().RemoveEntriesForRloc16(oldRloc16);
    }

exit:
    FreeMessage(message);
}

template <> void MleRouter::HandleTmf<kUriAddressRelease>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t        rloc16;
    Mac::ExtAddress extAddress;
    uint8_t         routerId;
    Router         *router;

    VerifyOrExit(mRole == kRoleLeader);

    VerifyOrExit(aMessage.IsConfirmablePostRequest());

    Log(kMessageReceive, kTypeAddressRelease, aMessageInfo.GetPeerAddr());

    SuccessOrExit(Tlv::Find<ThreadRloc16Tlv>(aMessage, rloc16));
    SuccessOrExit(Tlv::Find<ThreadExtMacAddressTlv>(aMessage, extAddress));

    routerId = RouterIdFromRloc16(rloc16);
    router   = mRouterTable.FindRouterById(routerId);

    VerifyOrExit((router != nullptr) && (router->GetExtAddress() == extAddress));

    IgnoreError(mRouterTable.Release(routerId));

    SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

    Log(kMessageSend, kTypeAddressReleaseReply, aMessageInfo.GetPeerAddr());

exit:
    return;
}

void MleRouter::FillConnectivityTlv(ConnectivityTlv &aTlv)
{
    int8_t parentPriority = kParentPriorityMedium;

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

    aTlv.SetLinkQuality1(0);
    aTlv.SetLinkQuality2(0);
    aTlv.SetLinkQuality3(0);

    if (IsChild())
    {
        aTlv.IncrementLinkQuality(mParent.GetLinkQualityIn());
    }

    for (const Router &router : Get<RouterTable>())
    {
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

        aTlv.IncrementLinkQuality(router.GetTwoWayLinkQuality());
    }

    aTlv.SetActiveRouters(mRouterTable.GetActiveRouterCount());
    aTlv.SetLeaderCost(Min(mRouterTable.GetPathCostToLeader(), kMaxRouteCost));
    aTlv.SetIdSequence(mRouterTable.GetRouterIdSequence());
    aTlv.SetSedBufferSize(OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE);
    aTlv.SetSedDatagramCount(OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT);
}

bool MleRouter::ShouldDowngrade(uint8_t aNeighborId, const RouteTlv &aRouteTlv) const
{
    // Determine whether all conditions are satisfied for the router
    // to downgrade after receiving info for a neighboring router
    // with Router ID `aNeighborId` along with its `aRouteTlv`.

    bool    shouldDowngrade   = false;
    uint8_t activeRouterCount = mRouterTable.GetActiveRouterCount();
    uint8_t count;

    VerifyOrExit(IsRouter());
    VerifyOrExit(mRouterTable.IsAllocated(aNeighborId));

    VerifyOrExit(!mRouterRoleTransition.IsPending());

    VerifyOrExit(activeRouterCount > mRouterDowngradeThreshold);

    // Check that we have at least `kMinDowngradeNeighbors`
    // neighboring routers with two-way link quality of 2 or better.

    count = 0;

    for (const Router &router : mRouterTable)
    {
        if (!router.IsStateValid() || (router.GetTwoWayLinkQuality() < kLinkQuality2))
        {
            continue;
        }

        count++;

        if (count >= kMinDowngradeNeighbors)
        {
            break;
        }
    }

    VerifyOrExit(count >= kMinDowngradeNeighbors);

    // Check that we have fewer children than three times the number
    // of excess routers (defined as the difference between number of
    // active routers and `mRouterDowngradeThreshold`).

    count = activeRouterCount - mRouterDowngradeThreshold;
    VerifyOrExit(mChildTable.GetNumChildren(Child::kInStateValid) < count * 3);

    // Check that the neighbor has as good or better-quality links to
    // same routers.

    VerifyOrExit(NeighborHasComparableConnectivity(aRouteTlv, aNeighborId));

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    // Check if we are eligible to be router due to being a BR.
    VerifyOrExit(!Get<NetworkData::Notifier>().IsEligibleForRouterRoleUpgradeAsBorderRouter());
#endif

    shouldDowngrade = true;

exit:
    return shouldDowngrade;
}

bool MleRouter::NeighborHasComparableConnectivity(const RouteTlv &aRouteTlv, uint8_t aNeighborId) const
{
    // Check whether the neighboring router with Router ID `aNeighborId`
    // (along with its `aRouteTlv`) has as good or better-quality links
    // to all our neighboring routers which have a two-way link quality
    // of two or better.

    bool isComparable = true;

    for (uint8_t routerId = 0, index = 0; routerId <= kMaxRouterId;
         index += aRouteTlv.IsRouterIdSet(routerId) ? 1 : 0, routerId++)
    {
        const Router *router;
        LinkQuality   localLinkQuality;
        LinkQuality   peerLinkQuality;

        if ((routerId == mRouterId) || (routerId == aNeighborId))
        {
            continue;
        }

        router = mRouterTable.FindRouterById(routerId);

        if ((router == nullptr) || !router->IsStateValid())
        {
            continue;
        }

        localLinkQuality = router->GetTwoWayLinkQuality();

        if (localLinkQuality < kLinkQuality2)
        {
            continue;
        }

        // `router` is our neighbor with two-way link quality of
        // at least two. Check that `aRouteTlv` has as good or
        // better-quality link to it as well.

        if (!aRouteTlv.IsRouterIdSet(routerId))
        {
            ExitNow(isComparable = false);
        }

        peerLinkQuality = Min(aRouteTlv.GetLinkQualityIn(index), aRouteTlv.GetLinkQualityOut(index));

        if (peerLinkQuality < localLinkQuality)
        {
            ExitNow(isComparable = false);
        }
    }

exit:
    return isComparable;
}

void MleRouter::SetChildStateToValid(Child &aChild)
{
    VerifyOrExit(!aChild.IsStateValid());

    aChild.SetState(Neighbor::kStateValid);
    IgnoreError(mChildTable.StoreChild(aChild));

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    Get<MlrManager>().UpdateProxiedSubscriptions(aChild, MlrManager::MlrAddressArray());
#endif

    mNeighborTable.Signal(NeighborTable::kChildAdded, aChild);

exit:
    return;
}

bool MleRouter::HasChildren(void) { return mChildTable.HasChildren(Child::kInStateValidOrAttaching); }

void MleRouter::RemoveChildren(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
    {
        RemoveNeighbor(child);
    }
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

    VerifyOrExit(aRxInfo.IsNeighborStateValid());

    aRxInfo.mClass = RxInfo::kPeerMessage;

    Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);

exit:
    return;
}

Error MleRouter::SendTimeSync(void)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage   *message = nullptr;

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

//----------------------------------------------------------------------------------------------------------------------
// RouterRoleTransition

MleRouter::RouterRoleTransition::RouterRoleTransition(void)
    : mTimeout(0)
    , mJitter(kRouterSelectionJitter)
{
}

void MleRouter::RouterRoleTransition::StartTimeout(void)
{
    mTimeout = 1 + Random::NonCrypto::GetUint8InRange(0, mJitter);
}

bool MleRouter::RouterRoleTransition::HandleTimeTick(void)
{
    bool expired = false;

    VerifyOrExit(mTimeout > 0);
    mTimeout--;
    expired = (mTimeout == 0);

exit:
    return expired;
}

} // namespace Mle
} // namespace ot

#endif // OPENTHREAD_FTD
