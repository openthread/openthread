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

#include "instance/instance.hpp"

namespace ot {
namespace Mle {

RegisterLogModule("Mle");

MleRouter::MleRouter(Instance &aInstance)
    : Mle(aInstance)
    , mRouterEligible(true)
    , mAddressSolicitPending(false)
    , mAddressSolicitRejected(false)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mCcmEnabled(false)
    , mThreadVersionCheckEnabled(true)
#endif
    , mNetworkIdTimeout(kNetworkIdTimeout)
    , mRouterUpgradeThreshold(kRouterUpgradeThreshold)
    , mRouterDowngradeThreshold(kRouterDowngradeThreshold)
    , mPreviousPartitionRouterIdSequence(0)
    , mPreviousPartitionIdTimeout(0)
    , mChildRouterLinks(kChildRouterLinks)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mMaxChildIpAddresses(0)
#endif
    , mParentPriority(kParentPriorityUnspecified)
    , mNextChildId(kMaxChildId)
    , mPreviousPartitionIdRouter(0)
    , mPreviousPartitionId(0)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mPreferredLeaderPartitionId(0)
#endif
    , mAdvertiseTrickleTimer(aInstance, MleRouter::HandleAdvertiseTrickleTimer)
    , mChildTable(aInstance)
    , mRouterTable(aInstance)
    , mRouterRoleRestorer(aInstance)
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

    if (!IsFullThreadDevice())
    {
        VerifyOrExit(!aEligible, error = kErrorNotCapable);
    }

    VerifyOrExit(aEligible != mRouterEligible);

    mRouterEligible = aEligible;

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        break;

    case kRoleChild:
        if (mRouterEligible)
        {
            mRouterRoleTransition.StartTimeout();
        }

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
        mRouterRoleRestorer.Start(mLastSavedRole);
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

Error MleRouter::BecomeLeader(bool aCheckWeight)
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

    if (aCheckWeight && IsAttached())
    {
        VerifyOrExit(mLeaderWeight > mLeaderData.GetWeighting(), error = kErrorNotCapable);
    }

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

        // If attach was initiated due to receiving an MLE Announce
        // message, all rx-on-when-idle devices will immediately
        // attempt to attach as well. This aligns with the Thread 1.1
        // specification (Section 4.8.1):
        //
        // "If the received value is newer and the channel and/or PAN
        //  ID in the Announce message differ from those currently in
        //  use, the receiving device attempts to attach using the
        //  channel and PAN ID received from the Announce message."
        //
        // Since parent-child relationships are unlikely to persist in
        // the new partition, we remove all children here. The
        // decision to become router is determined based on the new
        // partition's status.

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
    Get<TimeTicker>().RegisterReceiver(TimeTicker::kMleRouter);

    if (aRole == kRoleLeader)
    {
        GetLeaderAloc(mLeaderAloc.GetAddress());
        Get<ThreadNetif>().AddUnicastAddress(mLeaderAloc);
        Get<NetworkData::Leader>().Start(aStartMode);
        Get<MeshCoP::ActiveDatasetManager>().StartLeader();
        Get<MeshCoP::PendingDatasetManager>().StartLeader();
        Get<AddressResolver>().Clear();
    }

    // Remove children that do not have a matching RLOC16
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

    // Suppress MLE Advertisements when trying to attach to a better
    // partition. Without this, a candidate parent might incorrectly
    // interpret this advertisement (Source Address TLV containing an
    // RLOC16 indicating device is acting as router) and reject the
    // attaching device.

    VerifyOrExit(!IsAttaching());

    // Suppress MLE Advertisements when attempting to transition to
    // router role. Advertisements as a REED while attaching to a new
    // partition can cause existing children to detach
    // unnecessarily.

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
        mRouterRoleRestorer.GenerateRandomChallenge();
        SuccessOrExit(error = message->AppendChallengeTlv(mRouterRoleRestorer.GetChallenge()));
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

    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

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

    switch (Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress))
    {
    case kErrorNone:
        if (IsRouterRloc16(sourceAddress))
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
        // A missing source address indicates that the router was
        // recently reset.
        VerifyOrExit(aRxInfo.IsNeighborStateValid() && IsRouterRloc16(aRxInfo.mNeighbor->GetRloc16()),
                     error = kErrorDrop);
        neighbor = aRxInfo.mNeighbor;
        break;

    default:
        ExitNow(error = kErrorParse);
    }

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

    SuccessOrExit(error = SendLinkAccept(aRxInfo, neighbor, requestedTlvList, challenge));

exit:
    LogProcessError(kTypeLinkRequest, error);
}

Error MleRouter::SendLinkAccept(const RxInfo      &aRxInfo,
                                Neighbor          *aNeighbor,
                                const TlvList     &aRequestedTlvList,
                                const RxChallenge &aChallenge)
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
    SuccessOrExit(error = message->AppendLinkAndMleFrameCounterTlvs());

    linkMargin = Get<Mac::Mac>().ComputeLinkMargin(aRxInfo.mMessage.GetAverageRss());
    SuccessOrExit(error = message->AppendLinkMarginTlv(linkMargin));

    if (aNeighbor != nullptr && IsRouterRloc16(aNeighbor->GetRloc16()))
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

    if (aRxInfo.mMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(error = message->SendAfterDelay(aRxInfo.mMessageInfo.GetPeerAddr(),
                                                      1 + Random::NonCrypto::GetUint16InRange(0, kMaxLinkAcceptDelay)));

        Log(kMessageDelay, (command == kCommandLinkAccept) ? kTypeLinkAccept : kTypeLinkAcceptAndRequest,
            aRxInfo.mMessageInfo.GetPeerAddr());
    }
    else
    {
        SuccessOrExit(error = message->SendTo(aRxInfo.mMessageInfo.GetPeerAddr()));

        Log(kMessageSend, (command == kCommandLinkAccept) ? kTypeLinkAccept : kTypeLinkAcceptAndRequest,
            aRxInfo.mMessageInfo.GetPeerAddr());
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
    bool            shouldUpdateRoutes = false;

    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, aRequest ? kTypeLinkAcceptAndRequest : kTypeLinkAccept, aRxInfo.mMessageInfo.GetPeerAddr(),
        sourceAddress);

    VerifyOrExit(IsRouterRloc16(sourceAddress), error = kErrorParse);

    routerId      = RouterIdFromRloc16(sourceAddress);
    router        = mRouterTable.FindRouterById(routerId);
    neighborState = (router != nullptr) ? router->GetState() : Neighbor::kStateInvalid;

    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));

    switch (neighborState)
    {
    case Neighbor::kStateLinkRequest:
        VerifyOrExit(response == router->GetChallenge(), error = kErrorSecurity);
        break;

    case Neighbor::kStateInvalid:
        VerifyOrExit(mRouterRoleRestorer.IsActive() && (response == mRouterRoleRestorer.GetChallenge()),
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

    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));

    switch (Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMargin))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        // The Link Margin TLV may be omitted after a reset. We wait
        // for MLE Advertisements to establish the routing cost to
        // the neighbor
        VerifyOrExit(IsDetached(), error = kErrorNotFound);
        linkMargin = 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    switch (mRole)
    {
    case kRoleDetached:
        SuccessOrExit(error = Tlv::Find<Address16Tlv>(aRxInfo.mMessage, address16));
        VerifyOrExit(GetRloc16() == address16, error = kErrorDrop);

        SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));
        SetLeaderData(leaderData);

        mRouterTable.Clear();
        SuccessOrExit(error = aRxInfo.mMessage.ReadRouteTlv(routeTlv));
        SuccessOrExit(error = ProcessRouteTlv(routeTlv, aRxInfo));
        router = mRouterTable.FindRouterById(routerId);
        VerifyOrExit(router != nullptr);

        if (GetLeaderRloc16() == GetRloc16())
        {
            SetStateLeader(GetRloc16(), kRestoringLeaderRoleAfterReset);
        }
        else
        {
            SetStateRouter(GetRloc16());
        }

        mRouterRoleRestorer.Stop();
        mRetrieveNewNetworkData = true;
        IgnoreError(SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr()));
        shouldUpdateRoutes = true;

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

        SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());

        if (mRetrieveNewNetworkData ||
            SerialNumber::IsGreater(leaderData.GetDataVersion(NetworkData::kFullSet),
                                    Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet)))
        {
            IgnoreError(SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr()));
        }

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
            shouldUpdateRoutes = true;
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

    if (shouldUpdateRoutes)
    {
        mRouterTable.UpdateRoutes(routeTlv, routerId);
    }

    aRxInfo.mClass = RxInfo::kAuthoritativeMessage;
    ProcessKeySequence(aRxInfo);

    if (aRequest)
    {
        RxChallenge challenge;
        TlvList     requestedTlvList;

        SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

        switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvList))
        {
        case kErrorNone:
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }

        SuccessOrExit(error = SendLinkAccept(aRxInfo, router, requestedTlvList, challenge));
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
    uint16_t neighborRloc16 = kInvalidRloc16;

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

    if (neighborRloc16 != kInvalidRloc16)
    {
        aRxInfo.mNeighbor = Get<NeighborTable>().FindNeighbor(neighborRloc16);
    }

    return error;
}

Error MleRouter::ReadAndProcessRouteTlvOnFtdChild(RxInfo &aRxInfo, uint8_t aParentId)
{
    // This method reads and processes Route TLV from message on an
    // FTD child if message contains one. It returns `kErrorNone`
    // when successfully processed or if there is no Route TLV in
    // the message.
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
        mRouterTable.UpdateRouterOnFtdChild(routeTlv, aParentId);
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

Error MleRouter::HandleAdvertisementOnFtd(RxInfo &aRxInfo, uint16_t aSourceAddress, const LeaderData &aLeaderData)
{
    // This method processes a received MLE Advertisement message on
    // an FTD device. It is called from `Mle::HandleAdvertisement()`
    // only when device is attached (in child, router, or leader roles)
    // and `IsFullThreadDevice()`.
    //
    // - `aSourceAddress` is the read value from `SourceAddressTlv`.
    // - `aLeaderData` is the read value from `LeaderDataTlv`.

    Error    error      = kErrorNone;
    uint8_t  linkMargin = Get<Mac::Mac>().ComputeLinkMargin(aRxInfo.mMessage.GetAverageRss());
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
            // Allow a better partition if it also enables time sync.
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

    VerifyOrExit(IsRouterRloc16(aSourceAddress) && routeTlv.IsValid());
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

            mRouterTable.UpdateRouterOnFtdChild(routeTlv, routerId);
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

    // Send unicast link request if no link to router and no
    // unicast/multicast link request in progress

    if (!router->IsStateValid() && !router->IsStateLinkRequest() && (linkMargin >= kLinkRequestMinMargin))
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

    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

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

    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(challenge));

    child = mChildTable.FindChild(extAddr, Child::kInStateAnyExceptInvalid);

    if (child == nullptr)
    {
        VerifyOrExit((child = mChildTable.GetNewChild()) != nullptr, error = kErrorNoBufs);

        InitNeighbor(*child, aRxInfo);
        child->SetState(Neighbor::kStateParentRequest);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        child->SetTimeSyncEnabled(Tlv::Find<TimeRequestTlv>(aRxInfo.mMessage, nullptr, 0) == kErrorNone);
#endif
        if (aRxInfo.mMessage.ReadModeTlv(mode) == kErrorNone)
        {
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

    SendParentResponse(*child, challenge, !ScanMaskTlv::IsEndDeviceFlagSet(scanMask));

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

    if (mPreviousPartitionIdTimeout > 0)
    {
        mPreviousPartitionIdTimeout--;
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Role transitions

    roleTransitionTimeoutExpired = mRouterRoleTransition.HandleTimeTick();

    switch (mRole)
    {
    case kRoleDetached:
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
        LogDebg("Leader age %lu", ToUlong(mRouterTable.GetLeaderAge()));

        if ((mRouterTable.GetActiveRouterCount() > 0) && (mRouterTable.GetLeaderAge() >= mNetworkIdTimeout))
        {
            LogInfo("Leader age timeout");
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

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Update `ChildTable`

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
            LogInfo("Child 0x%04x CSL synchronization expired", child.GetRloc16());
            child.SetCslSynchronized(false);
            Get<CslTxScheduler>().Update();
        }
#endif

        if (TimerMilli::GetNow() - child.GetLastHeard() >= timeout)
        {
            LogInfo("Child 0x%04x timeout expired", child.GetRloc16());
            RemoveNeighbor(child);
        }
        else if (IsRouterOrLeader() && child.IsStateRestored())
        {
            IgnoreError(SendChildUpdateRequest(child));
        }
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Update `RouterTable`

    for (Router &router : Get<RouterTable>())
    {
        uint32_t age;

        if (router.GetRloc16() == GetRloc16())
        {
            router.SetLastHeard(TimerMilli::GetNow());
            continue;
        }

        age = TimerMilli::GetNow() - router.GetLastHeard();

        if (router.IsStateValid() && (age >= kMaxNeighborAge))
        {
#if OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
            if (age < kMaxNeighborAge + kMaxTxCount * kUnicastRetxDelay)
            {
                LogInfo("No Adv from router 0x%04x - sending Link Request", router.GetRloc16());
                IgnoreError(SendLinkRequest(&router));
            }
            else
#endif
            {
                LogInfo("Router 0x%04x timeout expired", router.GetRloc16());
                RemoveNeighbor(router);
                continue;
            }
        }

        if (router.IsStateLinkRequest() && (age >= kLinkRequestTimeout))
        {
            LogInfo("Router 0x%04x - Link Request timeout expired", router.GetRloc16());
            RemoveNeighbor(router);
            continue;
        }

        if (IsLeader() && (mRouterTable.FindNextHopOf(router) == nullptr) &&
            (mRouterTable.GetLinkCost(router) >= kMaxRouteCost) && (age >= kMaxLeaderToRouterTimeout))
        {
            LogInfo("Router 0x%04x ID timeout expired (no route)", router.GetRloc16());
            IgnoreError(mRouterTable.Release(router.GetRouterId()));
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

void MleRouter::SendParentResponse(Child &aChild, const RxChallenge &aChallenge, bool aRoutersOnlyRequest)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage   *message;
    uint16_t     delay;

    VerifyOrExit((message = NewMleMessage(kCommandParentResponse)) != nullptr, error = kErrorNoBufs);
    message->SetDirectTransmission();

    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());
    SuccessOrExit(error = message->AppendLinkAndMleFrameCounterTlvs());
    SuccessOrExit(error = message->AppendResponseTlv(aChallenge));
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aChild.IsTimeSyncEnabled())
    {
        SuccessOrExit(error = message->AppendTimeParameterTlv());
    }
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    if (aChild.IsThreadVersionCslCapable())
    {
        SuccessOrExit(error = message->AppendCslClockAccuracyTlv());
    }
#endif
    aChild.GenerateChallenge();
    SuccessOrExit(error = message->AppendChallengeTlv(aChild.GetChallenge()));
    SuccessOrExit(error = message->AppendLinkMarginTlv(aChild.GetLinkInfo().GetLinkMargin()));
    SuccessOrExit(error = message->AppendConnectivityTlv());
    SuccessOrExit(error = message->AppendVersionTlv());

    destination.SetToLinkLocalAddress(aChild.GetExtAddress());

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
    Error       error;
    OffsetRange offsetRange;
    uint8_t     count       = 0;
    uint8_t     storedCount = 0;
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    Ip6::Address oldDua;
#endif
#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    MlrManager::MlrAddressArray oldMlrRegisteredAddresses;
#endif

    OT_UNUSED_VARIABLE(storedCount);

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aRxInfo.mMessage, Tlv::kAddressRegistration, offsetRange));

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    if (aChild.GetDomainUnicastAddress(oldDua) != kErrorNone)
    {
        oldDua.Clear();
    }
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    if (aChild.HasAnyMlrRegisteredAddress())
    {
        OT_ASSERT(aChild.IsStateValid());

        for (const Child::Ip6AddrEntry &addrEntry : aChild.GetIp6Addresses())
        {
            if (!addrEntry.IsMulticastLargerThanRealmLocal())
            {
                continue;
            }

            if (addrEntry.GetMlrState(aChild) == kMlrStateRegistered)
            {
                IgnoreError(oldMlrRegisteredAddresses.PushBack(addrEntry));
            }
        }
    }
#endif

    aChild.ClearIp6Addresses();

    while (!offsetRange.IsEmpty())
    {
        uint8_t      controlByte;
        Ip6::Address address;

        // Read out the control byte (first byte in entry)
        SuccessOrExit(error = aRxInfo.mMessage.Read(offsetRange, controlByte));
        offsetRange.AdvanceOffset(sizeof(uint8_t));
        count++;

        address.Clear();

        if (AddressRegistrationTlv::IsEntryCompressed(controlByte))
        {
            // Compressed entry contains IID with the 64-bit prefix
            // determined from 6LoWPAN context identifier (from
            // the control byte).

            uint8_t         contextId = AddressRegistrationTlv::GetContextId(controlByte);
            Lowpan::Context context;

            IgnoreError(aRxInfo.mMessage.Read(offsetRange, address.GetIid()));
            offsetRange.AdvanceOffset(sizeof(Ip6::InterfaceIdentifier));

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

            IgnoreError(aRxInfo.mMessage.Read(offsetRange, address));
            offsetRange.AdvanceOffset(sizeof(Ip6::Address));
        }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if (mMaxChildIpAddresses > 0 && storedCount >= mMaxChildIpAddresses)
        {
            // Skip remaining address registration entries but keep logging
            // skipped addresses.
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
    SignalDuaAddressEvent(aChild, oldDua);
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

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
void MleRouter::SignalDuaAddressEvent(const Child &aChild, const Ip6::Address &aOldDua) const
{
    DuaManager::ChildDuaAddressEvent event = DuaManager::kAddressUnchanged;
    Ip6::Address                     newDua;

    if (aChild.GetDomainUnicastAddress(newDua) == kErrorNone)
    {
        if (aOldDua.IsUnspecified())
        {
            event = DuaManager::kAddressAdded;
        }
        else if (aOldDua != newDua)
        {
            event = DuaManager::kAddressChanged;
        }
    }
    else
    {
        // Child has no DUA address. If there was no old DUA, no need
        // to signal.

        VerifyOrExit(!aOldDua.IsUnspecified());

        event = DuaManager::kAddressRemoved;
    }

    Get<DuaManager>().HandleChildDuaAddressEvent(aChild, event);

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

bool MleRouter::IsMessageMleSubType(const Message &aMessage) { return aMessage.IsSubTypeMle(); }

bool MleRouter::IsMessageChildUpdateRequest(const Message &aMessage)
{
    return aMessage.IsMleCommand(kCommandChildUpdateRequest);
}

void MleRouter::HandleChildIdRequest(RxInfo &aRxInfo)
{
    Error              error = kErrorNone;
    Mac::ExtAddress    extAddr;
    uint16_t           version;
    uint32_t           linkFrameCounter;
    uint32_t           mleFrameCounter;
    DeviceMode         mode;
    uint32_t           timeout;
    TlvList            tlvList;
    MeshCoP::Timestamp timestamp;
    Child             *child;
    Router            *router;
    uint16_t           supervisionInterval;

    Log(kMessageReceive, kTypeChildIdRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(IsRouterEligible(), error = kErrorInvalidState);

    VerifyOrExit(IsAttached(), error = kErrorInvalidState);

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);

    child = mChildTable.FindChild(extAddr, Child::kInStateAnyExceptInvalid);
    VerifyOrExit(child != nullptr, error = kErrorAlready);

    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

    SuccessOrExit(error = aRxInfo.mMessage.ReadAndMatchResponseTlvWith(child->GetChallenge()));

    Get<MeshForwarder>().RemoveMessagesForChild(*child, IsMessageMleSubType);

    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));

    SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));

    SuccessOrExit(error = Tlv::Find<TimeoutTlv>(aRxInfo.mMessage, timeout));

    SuccessOrExit(error = aRxInfo.mMessage.ReadTlvRequestTlv(tlvList));

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

    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        if (timestamp == Get<MeshCoP::ActiveDatasetManager>().GetTimestamp())
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

    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        if (timestamp == Get<MeshCoP::PendingDatasetManager>().GetTimestamp())
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

    router = mRouterTable.FindRouter(extAddr);

    if (router != nullptr)
    {
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
    child->GetLinkInfo().AddRss(aRxInfo.mMessage.GetAverageRss());
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

void MleRouter::HandleChildUpdateRequestOnParent(RxInfo &aRxInfo)
{
    Error           error = kErrorNone;
    Mac::ExtAddress extAddr;
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

    SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));

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
    tlvList.Add(Tlv::kLinkMargin);

    // Parent MUST include Leader Data TLV in Child Update Response
    tlvList.Add(Tlv::kLeaderData);

    if (!challenge.IsEmpty())
    {
        tlvList.Add(Tlv::kMleFrameCounter);
        tlvList.Add(Tlv::kLinkFrameCounter);
    }

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

void MleRouter::HandleChildUpdateResponseOnParent(RxInfo &aRxInfo)
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

    if ((aRxInfo.mNeighbor == nullptr) || IsRouterRloc16(aRxInfo.mNeighbor->GetRloc16()) ||
        !Get<ChildTable>().Contains(*aRxInfo.mNeighbor))
    {
        Log(kMessageReceive, kTypeChildUpdateResponseOfUnknownChild, aRxInfo.mMessageInfo.GetPeerAddr());
        ExitNow(error = kErrorNotFound);
    }

    child = static_cast<Child *>(aRxInfo.mNeighbor);

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

    switch (ProcessAddressRegistrationTlv(aRxInfo, *child))
    {
    case kErrorNone:
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

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
    child->GetLinkInfo().AddRss(aRxInfo.mMessage.GetAverageRss());

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

    SuccessOrExit(error = aRxInfo.mMessage.ReadTlvRequestTlv(tlvList));

    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        if (timestamp == Get<MeshCoP::ActiveDatasetManager>().GetTimestamp())
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

    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        if (timestamp == Get<MeshCoP::PendingDatasetManager>().GetTimestamp())
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
    Tlv::ParsedInfo              tlvInfo;
    MeshCoP::DiscoveryRequestTlv discoveryRequestTlv;
    MeshCoP::ExtendedPanId       extPanId;
    OffsetRange                  offsetRange;

    Log(kMessageReceive, kTypeDiscoveryRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    discoveryRequestTlv.SetLength(0);

    VerifyOrExit(IsRouterEligible(), error = kErrorInvalidState);

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aRxInfo.mMessage, Tlv::kDiscovery, offsetRange));

    for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(tlvInfo.GetSize()))
    {
        SuccessOrExit(error = tlvInfo.ParseFrom(aRxInfo.mMessage, offsetRange));

        if (tlvInfo.mIsExtended)
        {
            continue;
        }

        switch (tlvInfo.mType)
        {
        case MeshCoP::Tlv::kDiscoveryRequest:
            SuccessOrExit(error = aRxInfo.mMessage.Read(offsetRange, discoveryRequestTlv));
            VerifyOrExit(discoveryRequestTlv.IsValid(), error = kErrorParse);

            break;

        case MeshCoP::Tlv::kExtendedPanId:
            SuccessOrExit(
                error = Tlv::Read<MeshCoP::ExtendedPanIdTlv>(aRxInfo.mMessage, offsetRange.GetOffset(), extPanId));
            VerifyOrExit(Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId() != extPanId, error = kErrorDrop);

            break;

        default:
            break;
        }
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
    uint16_t                      delay;

    VerifyOrExit((message = NewMleMessage(kCommandDiscoveryResponse)) != nullptr, error = kErrorNoBufs);
    message->SetDirectTransmission();
    message->SetPanId(aDiscoverRequestMessage.GetPanId());
#if OPENTHREAD_CONFIG_MULTI_RADIO
    // Send the MLE Discovery Response message on same radio link
    // from which the "MLE Discover Request" message was received.
    message->SetRadioType(aDiscoverRequestMessage.GetRadioType());
#endif

    tlv.SetType(Tlv::kDiscovery);
    SuccessOrExit(error = message->Append(tlv));

    startOffset = message->GetLength();

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

    SuccessOrExit(
        error = Tlv::Append<MeshCoP::ExtendedPanIdTlv>(*message, Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()));

    SuccessOrExit(error = Tlv::Append<MeshCoP::NetworkNameTlv>(
                      *message, Get<MeshCoP::NetworkNameManager>().GetNetworkName().GetAsCString()));

    SuccessOrExit(error = message->AppendSteeringDataTlv());

    SuccessOrExit(
        error = Tlv::Append<MeshCoP::JoinerUdpPortTlv>(*message, Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort()));

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_4)
    if (!Get<MeshCoP::NetworkNameManager>().IsDefaultDomainNameSet())
    {
        SuccessOrExit(error = Tlv::Append<MeshCoP::ThreadDomainNameTlv>(
                          *message, Get<MeshCoP::NetworkNameManager>().GetDomainName().GetAsCString()));
    }
#endif

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
    SuccessOrExit(error = message->AppendActiveAndPendingTimestampTlvs());

    if ((aChild.GetRloc16() == 0) || !HasMatchingRouterIdWith(aChild.GetRloc16()))
    {
        uint16_t rloc16;

        // Pick next Child ID that is not being used
        do
        {
            mNextChildId++;

            if (mNextChildId > kMaxChildId)
            {
                mNextChildId = kMinChildId;
            }

            rloc16 = Get<Mac::Mac>().GetShortAddress() | mNextChildId;

        } while (mChildTable.FindChild(rloc16, Child::kInStateAnyExceptInvalid) != nullptr);

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

    if (!aChild.IsRxOnWhenIdle() && aChild.IsStateRestoring())
    {
        // No need to send the resync "Child Update Request"
        // to the sleepy child if there is one already
        // queued.

        VerifyOrExit(!Get<IndirectSender>().HasQueuedMessageForSleepyChild(aChild, IsMessageChildUpdateRequest));
    }

    Get<MeshForwarder>().RemoveMessagesForChild(aChild, IsMessageChildUpdateRequest);

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());
    SuccessOrExit(error = message->AppendNetworkDataTlv(aChild.GetNetworkDataType()));
    SuccessOrExit(error = message->AppendActiveAndPendingTimestampTlvs());

    if (aChild.IsStateValid())
    {
        SuccessOrExit(error = message->AppendLinkMarginTlv(aChild.GetLinkInfo().GetLinkMargin()));
    }
    else
    {
        SuccessOrExit(error = message->AppendTlvRequestTlv(kTlvs));

        if (!aChild.IsStateRestored())
        {
            // A random challenge is generated and saved when `aChild`
            // is first initialized in `kStateRestored`. We will use
            // the saved challenge here. This prevents overwriting
            // the saved challenge when the child is also detached
            // and happens to send a "Parent Request" in the window
            // where the parent transitions to the router/leader role
            // and before the parent sends the "Child Update Request".
            // This ensures that the same random challenge is
            // included in both "Parent Response" and "Child Update
            // Response," guaranteeing proper acceptance of the
            // child's "Child ID request".

            aChild.GenerateChallenge();
        }

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
            SuccessOrExit(error = message->AppendActiveAndPendingTimestampTlvs());
            break;

        case Tlv::kTimeout:
            SuccessOrExit(error = message->AppendTimeoutTlv(aChild->GetTimeout()));
            break;

        case Tlv::kLinkMargin:
            SuccessOrExit(error = message->AppendLinkMarginTlv(aChild->GetLinkInfo().GetLinkMargin()));
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
    SuccessOrExit(error = message->AppendActiveAndPendingTimestampTlvs());

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
        Get<MeshForwarder>().RemoveDataResponseMessages();

        mDelayedSender.RemoveDataResponseMessage();

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
    else if (IsChildRloc16(aNeighbor.GetRloc16()))
    {
        OT_ASSERT(mChildTable.Contains(aNeighbor));

        if (aNeighbor.IsStateValidOrRestoring())
        {
            mNeighborTable.Signal(NeighborTable::kChildRemoved, aNeighbor);
        }

        Get<IndirectSender>().ClearAllMessagesForSleepyChild(static_cast<Child &>(aNeighbor));

        if (aNeighbor.IsFullThreadDevice())
        {
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

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderRloc();

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

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderRloc();

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

    SetRouterId(routerId);

    SetStateRouter(Rloc16FromRouterId(mRouterId));

    // We keep the router table next hop and cost as what we had as a
    // REED, i.e., our parent was the next hop towards all other
    // routers and we tracked its cost towards them. As an FTD child,
    // we may have established links with a subset of neighboring routers.
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
        rloc16 = kInvalidRloc16;
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

    if (rloc16 != kInvalidRloc16)
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
    // with the old RLOC16 unless the sender is a direct child. For
    // direct children, we retain the cache entries to allow
    // association with the promoted router's new RLOC16 upon
    // receiving its Link Advertisement.

    if ((aResponseStatus == ThreadStatusTlv::kSuccess) && (aRouter != nullptr))
    {
        uint16_t oldRloc16;

        VerifyOrExit(IsRoutingLocator(aMessageInfo.GetPeerAddr()));
        oldRloc16 = aMessageInfo.GetPeerAddr().GetIid().GetLocator();

        VerifyOrExit(oldRloc16 != aRouter->GetRloc16());
        VerifyOrExit(!RouterIdMatch(oldRloc16, GetRloc16()));
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
            continue;
        }

        if (!router.IsStateValid())
        {
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

//----------------------------------------------------------------------------------------------------------------------
// RouterRoleRestorer

MleRouter::RouterRoleRestorer::RouterRoleRestorer(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mAttempts(0)
{
}

void MleRouter::RouterRoleRestorer::Start(DeviceRole aPreviousRole)
{
    // If the device was previously the leader or had more than
    // `kMinCriticalChildrenCount` children, we use more link
    // request attempts.

    mAttempts = 0;

    switch (aPreviousRole)
    {
    case kRoleRouter:
        if (Get<MleRouter>().mChildTable.GetNumChildren(Child::kInStateValidOrRestoring) < kMinCriticalChildrenCount)
        {
            mAttempts = kMaxTxCount;
            break;
        }

        OT_FALL_THROUGH;

    case kRoleLeader:
        mAttempts = kMaxCriticalTxCount;
        break;

    case kRoleChild:
    case kRoleDetached:
    case kRoleDisabled:
        break;
    }

    SendMulticastLinkRequest();
}

void MleRouter::RouterRoleRestorer::HandleTimer(void)
{
    if (mAttempts > 0)
    {
        mAttempts--;
    }

    SendMulticastLinkRequest();
}

void MleRouter::RouterRoleRestorer::SendMulticastLinkRequest(void)
{
    uint32_t delay;

    VerifyOrExit(Get<Mle>().IsDetached(), mAttempts = 0);

    if (mAttempts == 0)
    {
        IgnoreError(Get<Mle>().BecomeDetached());
        ExitNow();
    }

    IgnoreError(Get<MleRouter>().SendLinkRequest(nullptr));

    delay = (mAttempts == 1) ? kLinkRequestTimeout
                             : Random::NonCrypto::GetUint32InRange(kMulticastRetxDelayMin, kMulticastRetxDelayMax);

    Get<Mle>().mAttachTimer.Start(delay);

exit:
    return;
}

} // namespace Mle
} // namespace ot

#endif // OPENTHREAD_FTD
