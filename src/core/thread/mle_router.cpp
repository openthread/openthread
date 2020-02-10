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

#if OPENTHREAD_FTD

#include "mle_router.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop.hpp"
#include "net/icmp6.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"
#include "thread/time_sync_service.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Mle {

MleRouter::MleRouter(Instance &aInstance)
    : Mle(aInstance)
    , mAdvertiseTimer(aInstance, &MleRouter::HandleAdvertiseTimer, NULL, this)
    , mStateUpdateTimer(aInstance, &MleRouter::HandleStateUpdateTimer, this)
    , mAddressSolicit(OT_URI_PATH_ADDRESS_SOLICIT, &MleRouter::HandleAddressSolicit, this)
    , mAddressRelease(OT_URI_PATH_ADDRESS_RELEASE, &MleRouter::HandleAddressRelease, this)
    , mChildTable(aInstance)
    , mRouterTable(aInstance)
    , mNeighborTableChangedCallback(NULL)
    , mChallengeTimeout(0)
    , mNextChildId(kMaxChildId)
    , mNetworkIdTimeout(kNetworkIdTimeout)
    , mRouterUpgradeThreshold(kRouterUpgradeThreshold)
    , mRouterDowngradeThreshold(kRouterDowngradeThreshold)
    , mLeaderWeight(kLeaderWeight)
    , mFixedLeaderPartitionId(0)
    , mRouterEligible(true)
    , mAddressSolicitPending(false)
    , mPreviousPartitionIdRouter(0)
    , mPreviousPartitionId(0)
    , mPreviousPartitionRouterIdSequence(0)
    , mPreviousPartitionIdTimeout(0)
    , mRouterSelectionJitter(kRouterSelectionJitter)
    , mRouterSelectionJitterTimeout(0)
    , mParentPriority(kParentPriorityUnspecified)
{
    mDeviceMode.Set(mDeviceMode.Get() | DeviceMode::kModeFullThreadDevice | DeviceMode::kModeFullNetworkData);

    SetRouterId(kInvalidRouterId);
}

void MleRouter::HandlePartitionChange(void)
{
    mPreviousPartitionId               = mLeaderData.GetPartitionId();
    mPreviousPartitionRouterIdSequence = mRouterTable.GetRouterIdSequence();
    mPreviousPartitionIdTimeout        = GetNetworkIdTimeout();

    Get<AddressResolver>().Clear();
    Get<Coap::Coap>().AbortTransaction(&MleRouter::HandleAddressSolicitResponse, this);
    mRouterTable.Clear();
}

bool MleRouter::IsRouterEligible(void) const
{
    return mRouterEligible && IsFullThreadDevice();
}

otError MleRouter::SetRouterEligible(bool aEligible)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsFullThreadDevice() || !aEligible, error = OT_ERROR_NOT_CAPABLE);

    mRouterEligible = aEligible;

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        break;

    case OT_DEVICE_ROLE_CHILD:
        Get<Mac::Mac>().SetBeaconEnabled(mRouterEligible);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if (!mRouterEligible)
        {
            BecomeDetached();
        }

        break;
    }

exit:
    return error;
}

otError MleRouter::BecomeRouter(ThreadStatusTlv::Status aStatus)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mRole != OT_DEVICE_ROLE_ROUTER, error = OT_ERROR_NONE);
    VerifyOrExit(IsRouterEligible(), error = OT_ERROR_NOT_CAPABLE);

    otLogInfoMle("Attempt to become router");

    Get<MeshForwarder>().SetRxOnWhenIdle(true);
    mRouterSelectionJitterTimeout = 0;

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DETACHED:
        SuccessOrExit(error = SendLinkRequest(NULL));
        mStateUpdateTimer.Start(kStateUpdatePeriod);
        break;

    case OT_DEVICE_ROLE_CHILD:
        SuccessOrExit(error = SendAddressSolicit(aStatus));
        break;

    default:
        assert(false);
        break;
    }

exit:
    return error;
}

otError MleRouter::BecomeLeader(void)
{
    otError  error = OT_ERROR_NONE;
    Router * router;
    uint32_t partitionId;
    uint8_t  leaderId;

    VerifyOrExit(!Get<MeshCoP::ActiveDataset>().IsPartiallyComplete(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mRole != OT_DEVICE_ROLE_LEADER, error = OT_ERROR_NONE);
    VerifyOrExit(IsRouterEligible(), error = OT_ERROR_NOT_CAPABLE);

    mRouterTable.Clear();

    partitionId = mFixedLeaderPartitionId ? mFixedLeaderPartitionId : Random::NonCrypto::GetUint32();

    leaderId = IsRouterIdValid(mPreviousRouterId) ? mPreviousRouterId
                                                  : Random::NonCrypto::GetUint8InRange(0, kMaxRouterId + 1);

    SetLeaderData(partitionId, mLeaderWeight, leaderId);

    router = mRouterTable.Allocate(leaderId);
    assert(router != NULL);

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
    Get<Coap::Coap>().RemoveResource(mAddressSolicit);
    Get<Coap::Coap>().RemoveResource(mAddressRelease);
    Get<MeshCoP::ActiveDataset>().StopLeader();
    Get<MeshCoP::PendingDataset>().StopLeader();
    StopAdvertiseTimer();
    Get<NetworkData::Leader>().Stop();
    Get<ThreadNetif>().UnsubscribeAllRoutersMulticast();
}

void MleRouter::HandleDetachStart(void)
{
    mRouterTable.ClearNeighbors();
    StopLeader();
    mStateUpdateTimer.Stop();
}

otError MleRouter::HandleChildStart(AttachMode aMode)
{
    otError error = OT_ERROR_NONE;

    mRouterSelectionJitterTimeout = 1 + Random::NonCrypto::GetUint8InRange(0, mRouterSelectionJitter);

    StopLeader();
    mStateUpdateTimer.Start(kStateUpdatePeriod);

    if (mRouterEligible)
    {
        Get<Mac::Mac>().SetBeaconEnabled(true);
    }

    Get<ThreadNetif>().SubscribeAllRoutersMulticast();

    VerifyOrExit(IsRouterIdValid(mPreviousRouterId), error = OT_ERROR_INVALID_STATE);

    switch (aMode)
    {
    case kAttachSameDowngrade:
        SendAddressRelease();

        // reset children info if any
        if (HasChildren())
        {
            RemoveChildren();
        }

        // reset routerId info
        SetRouterId(kInvalidRouterId);
        break;

    case kAttachSame1:
    case kAttachSame2:
        if (HasChildren())
        {
            BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        }

        break;

    case kAttachAny:
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

        // fall through
    case kAttachBetter:
        if (HasChildren() && mPreviousPartitionIdRouter != mLeaderData.GetPartitionId())
        {
            BecomeRouter(ThreadStatusTlv::kParentPartitionChange);
        }

        break;
    }

exit:

    if (mRouterTable.GetActiveRouterCount() >= mRouterUpgradeThreshold &&
        (!IsRouterIdValid(mPreviousRouterId) || !HasChildren()))
    {
        SetRouterId(kInvalidRouterId);
    }

    return error;
}

void MleRouter::SetStateRouter(uint16_t aRloc16)
{
    SetRloc16(aRloc16);

    SetRole(OT_DEVICE_ROLE_ROUTER);
    SetAttachState(kAttachStateIdle);
    mAttachCounter = 0;
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    StopAdvertiseTimer();
    ResetAdvertiseInterval();

    Get<ThreadNetif>().SubscribeAllRoutersMulticast();
    mPreviousPartitionIdRouter = mLeaderData.GetPartitionId();
    Get<NetworkData::Leader>().Stop();
    Get<Ip6::Ip6>().SetForwardingEnabled(true);
    Get<Ip6::Mpl>().SetTimerExpirations(kMplRouterDataMessageTimerExpirations);
    Get<Mac::Mac>().SetBeaconEnabled(true);

    // remove children that do not have matching RLOC16
    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone(); iter++)
    {
        if (RouterIdFromRloc16(iter.GetChild()->GetRloc16()) != mRouterId)
        {
            RemoveNeighbor(*iter.GetChild());
        }
    }
}

void MleRouter::SetStateLeader(uint16_t aRloc16)
{
    SetRloc16(aRloc16);

    SetRole(OT_DEVICE_ROLE_LEADER);
    SetAttachState(kAttachStateIdle);
    mAttachCounter = 0;
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    StopAdvertiseTimer();
    ResetAdvertiseInterval();
    AddLeaderAloc();

    Get<ThreadNetif>().SubscribeAllRoutersMulticast();
    mPreviousPartitionIdRouter = mLeaderData.GetPartitionId();
    mStateUpdateTimer.Start(kStateUpdatePeriod);

    Get<NetworkData::Leader>().Start();
    Get<MeshCoP::ActiveDataset>().StartLeader();
    Get<MeshCoP::PendingDataset>().StartLeader();
    Get<Coap::Coap>().AddResource(mAddressSolicit);
    Get<Coap::Coap>().AddResource(mAddressRelease);
    Get<Ip6::Ip6>().SetForwardingEnabled(true);
    Get<Ip6::Mpl>().SetTimerExpirations(kMplRouterDataMessageTimerExpirations);
    Get<Mac::Mac>().SetBeaconEnabled(true);
    Get<AddressResolver>().Clear();

    // remove children that do not have matching RLOC16
    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone(); iter++)
    {
        if (RouterIdFromRloc16(iter.GetChild()->GetRloc16()) != mRouterId)
        {
            RemoveNeighbor(*iter.GetChild());
        }
    }

    otLogNoteMle("Leader partition id 0x%x", mLeaderData.GetPartitionId());
}

bool MleRouter::HandleAdvertiseTimer(TrickleTimer &aTimer)
{
    return aTimer.GetOwner<MleRouter>().HandleAdvertiseTimer();
}

bool MleRouter::HandleAdvertiseTimer(void)
{
    bool continueTrickle = true;

    VerifyOrExit(IsRouterEligible(), continueTrickle = false);

    SendAdvertisement();

exit:
    return continueTrickle;
}

void MleRouter::StopAdvertiseTimer(void)
{
    mAdvertiseTimer.Stop();
}

void MleRouter::ResetAdvertiseInterval(void)
{
    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER);

    if (!mAdvertiseTimer.IsRunning())
    {
        mAdvertiseTimer.Start(Time::SecToMsec(kAdvertiseIntervalMin), Time::SecToMsec(kAdvertiseIntervalMax),
                              TrickleTimer::kModeNormal);
    }

    mAdvertiseTimer.IndicateInconsistent();

exit:
    return;
}

otError MleRouter::SendAdvertisement(void)
{
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message = NULL;

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

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandAdvertisement));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        assert(false);
        break;

    case OT_DEVICE_ROLE_CHILD:
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        SuccessOrExit(error = AppendRoute(*message));
        break;
    }

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);
    SuccessOrExit(error = SendMessage(*message, destination));

    LogMleMessage("Send Advertisement", destination);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendLinkRequest(Neighbor *aNeighbor)
{
    static const uint8_t detachedTlvs[]      = {Tlv::kAddress16, Tlv::kRoute};
    static const uint8_t routerTlvs[]        = {Tlv::kLinkMargin};
    static const uint8_t validNeighborTlvs[] = {Tlv::kLinkMargin, Tlv::kRoute};
    otError              error               = OT_ERROR_NONE;
    Message *            message;
    Ip6::Address         destination;

    destination.Clear();

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandLinkRequest));
    SuccessOrExit(error = AppendVersion(*message));

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        assert(false);
        break;

    case OT_DEVICE_ROLE_DETACHED:
        SuccessOrExit(error = AppendTlvRequest(*message, detachedTlvs, sizeof(detachedTlvs)));
        break;

    case OT_DEVICE_ROLE_CHILD:
        SuccessOrExit(error = AppendSourceAddress(*message));
        SuccessOrExit(error = AppendLeaderData(*message));
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if (aNeighbor == NULL || !aNeighbor->IsStateValid())
        {
            SuccessOrExit(error = AppendTlvRequest(*message, routerTlvs, sizeof(routerTlvs)));
        }
        else
        {
            SuccessOrExit(error = AppendTlvRequest(*message, validNeighborTlvs, sizeof(validNeighborTlvs)));
        }

        SuccessOrExit(error = AppendSourceAddress(*message));
        SuccessOrExit(error = AppendLeaderData(*message));
        break;
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = AppendTimeRequest(*message));
#endif

    if (aNeighbor == NULL)
    {
        Random::Crypto::FillBuffer(mChallenge, sizeof(mChallenge));

        mChallengeTimeout = (((2 * kMaxResponseDelay) + kStateUpdatePeriod - 1) / kStateUpdatePeriod);

        SuccessOrExit(error = AppendChallenge(*message, mChallenge, sizeof(mChallenge)));
        destination.mFields.m8[0]  = 0xff;
        destination.mFields.m8[1]  = 0x02;
        destination.mFields.m8[15] = 2;
    }
    else
    {
        if (!aNeighbor->IsStateValid())
        {
            aNeighbor->GenerateChallenge();
            SuccessOrExit(error = AppendChallenge(*message, aNeighbor->GetChallenge(), aNeighbor->GetChallengeSize()));
        }
        else
        {
            uint8_t challenge[ChallengeTlv::kMaxSize];

            Random::Crypto::FillBuffer(challenge, sizeof(challenge));
            SuccessOrExit(error = AppendChallenge(*message, challenge, sizeof(challenge)));
        }

        destination.mFields.m16[0] = HostSwap16(0xfe80);
        destination.SetIid(aNeighbor->GetExtAddress());
    }

    SuccessOrExit(error = SendMessage(*message, destination));

    LogMleMessage("Send Link Request", destination);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::HandleLinkRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor)
{
    otError          error    = OT_ERROR_NONE;
    Neighbor *       neighbor = NULL;
    ChallengeTlv     challenge;
    VersionTlv       version;
    LeaderDataTlv    leaderData;
    SourceAddressTlv sourceAddress;
    TlvRequestTlv    tlvRequest;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    TimeRequestTlv timeRequest;
#endif

    LogMleMessage("Receive Link Request", aMessageInfo.GetPeerAddr());

    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(!IsAttaching(), error = OT_ERROR_INVALID_STATE);

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid(), error = OT_ERROR_PARSE);

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == OT_ERROR_NONE)
    {
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), error = OT_ERROR_INVALID_STATE);
    }

    // Source Address
    if (Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress) == OT_ERROR_NONE)
    {
        uint16_t rloc16;

        VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

        rloc16 = sourceAddress.GetRloc16();

        if (IsActiveRouter(rloc16))
        {
            Mac::ExtAddress macAddr;

            aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);

            neighbor = mRouterTable.GetRouter(RouterIdFromRloc16(rloc16));
            VerifyOrExit(neighbor != NULL, error = OT_ERROR_PARSE);
            VerifyOrExit(!neighbor->IsStateLinkRequest(), error = OT_ERROR_ALREADY);

            if (!neighbor->IsStateValid())
            {
                const otThreadLinkInfo *linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());

                neighbor->SetExtAddress(macAddr);
                neighbor->GetLinkInfo().Clear();
                neighbor->GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
                neighbor->ResetLinkFailures();
                neighbor->SetLastHeard(TimerMilli::GetNow());
                neighbor->SetState(Neighbor::kStateLinkRequest);
            }
            else
            {
                VerifyOrExit(neighbor->GetExtAddress() == macAddr);
            }
        }
    }
    else
    {
        // lack of source address indicates router coming out of reset
        VerifyOrExit(aNeighbor && aNeighbor->IsStateValid() && IsActiveRouter(aNeighbor->GetRloc16()),
                     error = OT_ERROR_DROP);
        neighbor = aNeighbor;
    }

    // TLV Request
    if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == OT_ERROR_NONE)
    {
        VerifyOrExit(tlvRequest.IsValid(), error = OT_ERROR_PARSE);
    }
    else
    {
        tlvRequest.SetLength(0);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (neighbor != NULL)
    {
        if (Tlv::GetTlv(aMessage, Tlv::kTimeRequest, sizeof(timeRequest), timeRequest) == OT_ERROR_NONE)
        {
            neighbor->SetTimeSyncEnabled(true);
        }
        else
        {
            neighbor->SetTimeSyncEnabled(false);
        }
    }
#endif

    SuccessOrExit(error = SendLinkAccept(aMessageInfo, neighbor, tlvRequest, challenge));

exit:
    return error;
}

otError MleRouter::SendLinkAccept(const Ip6::MessageInfo &aMessageInfo,
                                  Neighbor *              aNeighbor,
                                  const TlvRequestTlv &   aTlvRequest,
                                  const ChallengeTlv &    aChallenge)
{
    otError                 error        = OT_ERROR_NONE;
    const otThreadLinkInfo *linkInfo     = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    static const uint8_t    routerTlvs[] = {Tlv::kLinkMargin};
    Message *               message;
    Header::Command         command;
    uint8_t                 linkMargin;

    command = (aNeighbor == NULL || aNeighbor->IsStateValid()) ? Header::kCommandLinkAccept
                                                               : Header::kCommandLinkAcceptAndRequest;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, command));
    SuccessOrExit(error = AppendVersion(*message));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendResponse(*message, aChallenge.GetChallenge(), aChallenge.GetChallengeLength()));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));

    // always append a link margin, regardless of whether or not it was requested
    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);

    SuccessOrExit(error = AppendLinkMargin(*message, linkMargin));

    if (aNeighbor != NULL && IsActiveRouter(aNeighbor->GetRloc16()))
    {
        SuccessOrExit(error = AppendLeaderData(*message));
    }

    for (uint8_t i = 0; i < aTlvRequest.GetLength(); i++)
    {
        switch (aTlvRequest.GetTlvs()[i])
        {
        case Tlv::kRoute:
            SuccessOrExit(error = AppendRoute(*message));
            break;

        case Tlv::kAddress16:
            VerifyOrExit(aNeighbor != NULL, error = OT_ERROR_DROP);
            SuccessOrExit(error = AppendAddress16(*message, aNeighbor->GetRloc16()));
            break;

        case Tlv::kLinkMargin:
            break;

        default:
            ExitNow(error = OT_ERROR_DROP);
        }
    }

    if (aNeighbor != NULL && !aNeighbor->IsStateValid())
    {
        aNeighbor->GenerateChallenge();

        SuccessOrExit(error = AppendChallenge(*message, aNeighbor->GetChallenge(), aNeighbor->GetChallengeSize()));
        SuccessOrExit(error = AppendTlvRequest(*message, routerTlvs, sizeof(routerTlvs)));
        aNeighbor->SetLastHeard(TimerMilli::GetNow());
        aNeighbor->SetState(Neighbor::kStateLinkRequest);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aNeighbor != NULL && aNeighbor->IsTimeSyncEnabled())
    {
        message->SetTimeSync(true);
    }
#endif

    if (aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(error = AddDelayedResponse(*message, aMessageInfo.GetPeerAddr(),
                                                 1 + Random::NonCrypto::GetUint16InRange(0, kMaxResponseDelay)));

        LogMleMessage("Delay Link Accept", aMessageInfo.GetPeerAddr());
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aMessageInfo.GetPeerAddr()));

        LogMleMessage("Send Link Accept", aMessageInfo.GetPeerAddr());
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::HandleLinkAccept(const Message &         aMessage,
                                    const Ip6::MessageInfo &aMessageInfo,
                                    uint32_t                aKeySequence,
                                    Neighbor *              aNeighbor)
{
    otError error = HandleLinkAccept(aMessage, aMessageInfo, aKeySequence, aNeighbor, false);

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Link Accept: %s", otThreadErrorToString(error));
    }

    return error;
}

otError MleRouter::HandleLinkAcceptAndRequest(const Message &         aMessage,
                                              const Ip6::MessageInfo &aMessageInfo,
                                              uint32_t                aKeySequence,
                                              Neighbor *              aNeighbor)
{
    otError error = HandleLinkAccept(aMessage, aMessageInfo, aKeySequence, aNeighbor, true);

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Link Accept and Request: %s", otThreadErrorToString(error));
    }

    return error;
}

otError MleRouter::HandleLinkAccept(const Message &         aMessage,
                                    const Ip6::MessageInfo &aMessageInfo,
                                    uint32_t                aKeySequence,
                                    Neighbor *              aNeighbor,
                                    bool                    aRequest)
{
    static const uint8_t dataRequestTlvs[] = {Tlv::kNetworkData};

    otError                 error    = OT_ERROR_NONE;
    const otThreadLinkInfo *linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    Router *                router;
    Neighbor::State         neighborState;
    Mac::ExtAddress         macAddr;
    VersionTlv              version;
    ResponseTlv             response;
    SourceAddressTlv        sourceAddress;
    LinkFrameCounterTlv     linkFrameCounter;
    MleFrameCounterTlv      mleFrameCounter;
    uint8_t                 routerId;
    Address16Tlv            address16;
    RouteTlv                route;
    LeaderDataTlv           leaderData;
    LinkMarginTlv           linkMargin;

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    if (aRequest)
    {
        LogMleMessage("Receive Link Accept and Request", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());
    }
    else
    {
        LogMleMessage("Receive Link Accept", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());
    }

    VerifyOrExit(IsActiveRouter(sourceAddress.GetRloc16()), error = OT_ERROR_PARSE);

    routerId      = RouterIdFromRloc16(sourceAddress.GetRloc16());
    router        = mRouterTable.GetRouter(routerId);
    neighborState = (router != NULL) ? router->GetState() : Neighbor::kStateInvalid;

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid(), error = OT_ERROR_PARSE);

    // verify response
    switch (neighborState)
    {
    case Neighbor::kStateLinkRequest:
        VerifyOrExit(memcmp(router->GetChallenge(), response.GetResponse(), router->GetChallengeSize()) == 0,
                     error = OT_ERROR_SECURITY);
        break;

    case Neighbor::kStateInvalid:
        VerifyOrExit((mChallengeTimeout > 0) && (memcmp(mChallenge, response.GetResponse(), sizeof(mChallenge)) == 0),
                     error = OT_ERROR_SECURITY);

    case Neighbor::kStateValid:
        break;

    default:
        ExitNow(error = OT_ERROR_SECURITY);
    }

    // Remove stale neighbors
    if (aNeighbor && aNeighbor->GetRloc16() != sourceAddress.GetRloc16())
    {
        RemoveNeighbor(*aNeighbor);
    }

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid(), error = OT_ERROR_PARSE);

    // Link-Layer Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter), linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) == OT_ERROR_NONE)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), error = OT_ERROR_PARSE);
    }
    else
    {
        mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    // Link Margin
    if (Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMargin), linkMargin) == OT_ERROR_NONE)
    {
        VerifyOrExit(linkMargin.IsValid(), error = OT_ERROR_PARSE);
    }
    else
    {
        // Link Margin TLV may be skipped in Router Synchronization process after Reset
        VerifyOrExit(mRole == OT_DEVICE_ROLE_DETACHED, error = OT_ERROR_NOT_FOUND);

        // Wait for an MLE Advertisement to establish a routing cost to the neighbor
        linkMargin.SetLinkMargin(0);
    }

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        assert(false);
        break;

    case OT_DEVICE_ROLE_DETACHED:
        // Address16
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kAddress16, sizeof(address16), address16));
        VerifyOrExit(address16.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(GetRloc16() == address16.GetRloc16(), error = OT_ERROR_DROP);

        // Leader Data
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
        SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

        // Route
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route));
        VerifyOrExit(route.IsValid(), error = OT_ERROR_PARSE);
        mRouterTable.Clear();
        SuccessOrExit(error = ProcessRouteTlv(route));
        router = mRouterTable.GetRouter(routerId);
        VerifyOrExit(router != NULL);

        if (mLeaderData.GetLeaderRouterId() == RouterIdFromRloc16(GetRloc16()))
        {
            SetStateLeader(GetRloc16());
        }
        else
        {
            SetStateRouter(GetRloc16());
        }

        mRetrieveNewNetworkData = true;
        SendDataRequest(aMessageInfo.GetPeerAddr(), dataRequestTlvs, sizeof(dataRequestTlvs), 0);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        Get<TimeSync>().HandleTimeSyncMessage(aMessage);
#endif
        break;

    case OT_DEVICE_ROLE_CHILD:
        VerifyOrExit(router != NULL);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        VerifyOrExit(router != NULL);

        // Leader Data
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());

        if (mRetrieveNewNetworkData ||
            (static_cast<int8_t>(leaderData.GetDataVersion() - Get<NetworkData::Leader>().GetVersion()) > 0))
        {
            SendDataRequest(aMessageInfo.GetPeerAddr(), dataRequestTlvs, sizeof(dataRequestTlvs), 0);
        }

        // Route (optional)
        if (Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE)
        {
            VerifyOrExit(route.IsValid(), error = OT_ERROR_PARSE);
            SuccessOrExit(error = ProcessRouteTlv(route));
            UpdateRoutes(route, routerId);
        }

        // update routing table
        if (routerId != mRouterId && !IsRouterIdValid(router->GetNextHop()))
        {
            ResetAdvertiseInterval();
        }

        break;
    }

    // finish link synchronization
    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);
    router->SetExtAddress(macAddr);
    router->SetRloc16(sourceAddress.GetRloc16());
    router->SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    router->SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    router->SetLastHeard(TimerMilli::GetNow());
    router->SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                     DeviceMode::kModeFullNetworkData));
    router->GetLinkInfo().Clear();
    router->GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
    router->SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin.GetLinkMargin()));
    router->ResetLinkFailures();
    router->SetState(Neighbor::kStateValid);
    router->SetKeySequence(aKeySequence);

    Signal(OT_NEIGHBOR_TABLE_EVENT_ROUTER_ADDED, *router);

    if (aRequest)
    {
        ChallengeTlv  challenge;
        TlvRequestTlv tlvRequest;

        // Challenge
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
        VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);

        // TLV Request
        if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == OT_ERROR_NONE)
        {
            VerifyOrExit(tlvRequest.IsValid(), error = OT_ERROR_PARSE);
        }
        else
        {
            tlvRequest.SetLength(0);
        }

        SuccessOrExit(error = SendLinkAccept(aMessageInfo, router, tlvRequest, challenge));
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

    // NULL aRouterId indicates non-existing next hop, hence return kMaxRouteCost for it.
    VerifyOrExit(router != NULL);

    rval = mRouterTable.GetLinkCost(*router);

exit:
    return rval;
}

otError MleRouter::SetRouterSelectionJitter(uint8_t aRouterJitter)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aRouterJitter > 0, error = OT_ERROR_INVALID_ARGS);

    mRouterSelectionJitter = aRouterJitter;

exit:
    return error;
}

otError MleRouter::ProcessRouteTlv(const RouteTlv &aRoute)
{
    otError error = OT_ERROR_NONE;

    mRouterTable.ProcessTlv(aRoute);

    if (mRole == OT_DEVICE_ROLE_ROUTER && !mRouterTable.IsAllocated(mRouterId))
    {
        BecomeDetached();
        error = OT_ERROR_NO_ROUTE;
    }

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

int MleRouter::ComparePartitions(bool                 aSingletonA,
                                 const LeaderDataTlv &aLeaderDataA,
                                 bool                 aSingletonB,
                                 const LeaderDataTlv &aLeaderDataB)
{
    int rval = 0;

    if (aSingletonA != aSingletonB)
    {
        ExitNow(rval = aSingletonB ? 1 : -1);
    }

    if (aLeaderDataA.GetWeighting() != aLeaderDataB.GetWeighting())
    {
        ExitNow(rval = aLeaderDataA.GetWeighting() > aLeaderDataB.GetWeighting() ? 1 : -1);
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

otError MleRouter::HandleAdvertisement(const Message &         aMessage,
                                       const Ip6::MessageInfo &aMessageInfo,
                                       Neighbor *              aNeighbor)
{
    otError                 error    = OT_ERROR_NONE;
    const otThreadLinkInfo *linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    uint8_t linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
    Mac::ExtAddress  macAddr;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv    leaderData;
    RouteTlv         route;
    uint32_t         partitionId;
    Router *         router;
    uint8_t          routerId;
    uint8_t          routerCount;

    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    // Route Data (optional)
    if (Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE)
    {
        VerifyOrExit(route.IsValid(), error = OT_ERROR_PARSE);
    }
    else
    {
        // mark that a Route TLV was not included
        route.SetLength(0);
    }

    partitionId = leaderData.GetPartitionId();

    if (partitionId != mLeaderData.GetPartitionId())
    {
        otLogNoteMle("Different partition (peer:%u, local:%u)", leaderData.GetPartitionId(),
                     mLeaderData.GetPartitionId());

        VerifyOrExit(linkMargin >= OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN, error = OT_ERROR_LINK_MARGIN_LOW);

        if (route.IsValid() && IsFullThreadDevice() && (mPreviousPartitionIdTimeout > 0) &&
            (partitionId == mPreviousPartitionId))
        {
            VerifyOrExit((static_cast<int8_t>(route.GetRouterIdSequence() - mPreviousPartitionRouterIdSequence) > 0),
                         error = OT_ERROR_DROP);
        }

        if (mRole == OT_DEVICE_ROLE_CHILD && (aNeighbor == &mParent || !IsFullThreadDevice()))
        {
            ExitNow();
        }

        if (ComparePartitions(IsSingleton(route), leaderData, IsSingleton(), mLeaderData) > 0
#if OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED
            // if time sync is required, it will only migrate to a better network which also enables time sync.
            && aMessage.GetTimeSyncSeq() != OT_TIME_SYNC_INVALID_SEQ
#endif
        )
        {
            BecomeChild(kAttachBetter);
        }

        ExitNow(error = OT_ERROR_DROP);
    }
    else if (leaderData.GetLeaderRouterId() != GetLeaderId())
    {
        VerifyOrExit(aNeighbor && aNeighbor->IsStateValid());

        if (mRole != OT_DEVICE_ROLE_CHILD)
        {
            otLogInfoMle("Leader ID mismatch");
            BecomeDetached();
            error = OT_ERROR_DROP;
        }

        ExitNow();
    }

    VerifyOrExit(IsActiveRouter(sourceAddress.GetRloc16()) && route.IsValid());
    routerId = RouterIdFromRloc16(sourceAddress.GetRloc16());

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    Get<TimeSync>().HandleTimeSyncMessage(aMessage);
#endif

    if (IsFullThreadDevice() && (aNeighbor && aNeighbor->IsStateValid()) &&
        ((mRouterTable.GetActiveRouterCount() == 0) ||
         (static_cast<int8_t>(route.GetRouterIdSequence() - mRouterTable.GetRouterIdSequence()) > 0)))
    {
        bool processRouteTlv = false;

        switch (mRole)
        {
        case OT_DEVICE_ROLE_DISABLED:
        case OT_DEVICE_ROLE_DETACHED:
            break;

        case OT_DEVICE_ROLE_CHILD:
            if (sourceAddress.GetRloc16() == mParent.GetRloc16())
            {
                processRouteTlv = true;
            }
            else
            {
                router = mRouterTable.GetRouter(routerId);

                if (router != NULL && router->IsStateValid())
                {
                    processRouteTlv = true;
                }
            }

            break;

        case OT_DEVICE_ROLE_ROUTER:
        case OT_DEVICE_ROLE_LEADER:
            processRouteTlv = true;
            break;
        }

        if (processRouteTlv)
        {
            SuccessOrExit(error = ProcessRouteTlv(route));
        }
    }

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        ExitNow();

    case OT_DEVICE_ROLE_CHILD:
        if (aNeighbor == &mParent)
        {
            // MLE Advertisement from parent
            router = &mParent;

            if (mParent.GetRloc16() != sourceAddress.GetRloc16())
            {
                BecomeDetached();
                ExitNow(error = OT_ERROR_NO_ROUTE);
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

                if (leader != NULL)
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
            VerifyOrExit(router != NULL);

            if (IsFullThreadDevice() && !router->IsStateValid() && !router->IsStateLinkRequest() &&
                (mRouterTable.GetActiveLinkCount() < OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS))
            {
                router->SetExtAddress(macAddr);
                router->GetLinkInfo().Clear();
                router->GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
                router->ResetLinkFailures();
                router->SetLastHeard(TimerMilli::GetNow());
                router->SetState(Neighbor::kStateLinkRequest);
                SendLinkRequest(router);
                ExitNow(error = OT_ERROR_NO_ROUTE);
            }
        }

        router->SetLastHeard(TimerMilli::GetNow());

        ExitNow();

    case OT_DEVICE_ROLE_ROUTER:
        router = mRouterTable.GetRouter(routerId);
        VerifyOrExit(router != NULL);

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

        // fall through

    case OT_DEVICE_ROLE_LEADER:
        router = mRouterTable.GetRouter(routerId);
        VerifyOrExit(router != NULL);

        // Send unicast link request if no link to router and no unicast/multicast link request in progress
        if (!router->IsStateValid() && !router->IsStateLinkRequest() && (mChallengeTimeout == 0) &&
            (linkMargin >= OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN))
        {
            router->SetExtAddress(macAddr);
            router->GetLinkInfo().Clear();
            router->GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
            router->ResetLinkFailures();
            router->SetLastHeard(TimerMilli::GetNow());
            router->SetState(Neighbor::kStateLinkRequest);
            SendLinkRequest(router);
            ExitNow(error = OT_ERROR_NO_ROUTE);
        }

        router->SetLastHeard(TimerMilli::GetNow());
        break;
    }

    UpdateRoutes(route, routerId);

exit:
    if (aNeighbor && aNeighbor->GetRloc16() != sourceAddress.GetRloc16())
    {
        // Remove stale neighbors
        RemoveNeighbor(*aNeighbor);
    }

    return error;
}

void MleRouter::UpdateRoutes(const RouteTlv &aRoute, uint8_t aRouterId)
{
    Router *neighbor;
    bool    resetAdvInterval = false;
    bool    changed          = false;

    neighbor = mRouterTable.GetRouter(aRouterId);
    VerifyOrExit(neighbor != NULL);

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

        if (router == NULL || router->GetRloc16() == GetRloc16() || router == neighbor)
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

        if (nextHop == NULL || nextHop == neighbor)
        {
            // route has no next hop or next hop is neighbor (sender)

            if (cost + mRouterTable.GetLinkCost(*neighbor) < kMaxRouteCost)
            {
                if (nextHop == NULL && mRouterTable.GetLinkCost(*router) >= kMaxRouteCost)
                {
                    resetAdvInterval = true;
                }

                router->SetNextHop(aRouterId);
                router->SetCost(cost);
                changed = true;
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

#if (OPENTHREAD_CONFIG_LOG_MLE && (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO))

    VerifyOrExit(changed);
    otLogInfoMle("Route table updated");

    for (RouterTable::Iterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        Router &router = *iter.GetRouter();

        otLogInfoMle("    %04x -> %04x, cost:%d %d, lqin:%d, lqout:%d, link:%s", router.GetRloc16(),
                     (router.GetNextHop() == kInvalidRouterId) ? 0xffff : Rloc16FromRouterId(router.GetNextHop()),
                     router.GetCost(), mRouterTable.GetLinkCost(router), router.GetLinkInfo().GetLinkQuality(),
                     router.GetLinkQualityOut(),
                     router.GetRloc16() == GetRloc16() ? "device" : (router.IsStateValid() ? "yes" : "no"));
    }

#else
    OT_UNUSED_VARIABLE(changed);
#endif

exit:
    return;
}

bool MleRouter::UpdateLinkQualityOut(const RouteTlv &aRoute, Router &aNeighbor, bool &aResetAdvInterval)
{
    bool    changed = false;
    uint8_t linkQuality;
    uint8_t myRouterId;
    uint8_t myRouteCount;
    uint8_t oldLinkCost;
    Router *nextHop;

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
    if (nextHop == NULL && (oldLinkCost >= kMaxRouteCost) != (mRouterTable.GetLinkCost(aNeighbor) >= kMaxRouteCost))
    {
        aResetAdvInterval = true;
    }
    changed = true;

exit:
    return changed;
}

otError MleRouter::HandleParentRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                 error    = OT_ERROR_NONE;
    const otThreadLinkInfo *linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    Mac::ExtAddress         macAddr;
    VersionTlv              version;
    ScanMaskTlv             scanMask;
    ChallengeTlv            challenge;
    Router *                leader;
    Child *                 child;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    TimeRequestTlv timeRequest;
#endif

    LogMleMessage("Receive Parent Request", aMessageInfo.GetPeerAddr());

    VerifyOrExit(IsRouterEligible(), error = OT_ERROR_INVALID_STATE);

    // A Router MUST NOT send an MLE Parent Response if:

    // 0. It is detached or attempting to another partition
    VerifyOrExit((mRole != OT_DEVICE_ROLE_DETACHED) && !IsAttaching(), error = OT_ERROR_DROP);

    // 1. It has no available Child capacity (if Max Child Count minus
    // Child Count would be equal to zero)
    // ==> verified below when allocating a child entry

    // 2. It is disconnected from its Partition (that is, it has not
    // received an updated ID sequence number within LEADER_TIMEOUT
    // seconds)
    VerifyOrExit(mRouterTable.GetLeaderAge() < mNetworkIdTimeout, error = OT_ERROR_DROP);

    // 3. Its current routing path cost to the Leader is infinite.
    leader = mRouterTable.GetLeader();
    assert(leader != NULL);

    VerifyOrExit(mRole == OT_DEVICE_ROLE_LEADER || GetLinkCost(GetLeaderId()) < kMaxRouteCost ||
                     (mRole == OT_DEVICE_ROLE_CHILD && leader->GetCost() + 1 < kMaxRouteCost) ||
                     (leader->GetCost() + GetLinkCost(leader->GetNextHop()) < kMaxRouteCost),
                 error = OT_ERROR_DROP);

    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid(), error = OT_ERROR_PARSE);

    // Scan Mask
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kScanMask, sizeof(scanMask), scanMask));
    VerifyOrExit(scanMask.IsValid(), error = OT_ERROR_PARSE);

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        ExitNow();

    case OT_DEVICE_ROLE_CHILD:
        VerifyOrExit(scanMask.IsEndDeviceFlagSet());
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        VerifyOrExit(scanMask.IsRouterFlagSet());
        break;
    }

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);

    child = mChildTable.FindChild(macAddr, Child::kInStateAnyExceptInvalid);

    if (child == NULL)
    {
        VerifyOrExit((child = mChildTable.GetNewChild()) != NULL);

        // MAC Address
        child->SetExtAddress(macAddr);
        child->GetLinkInfo().Clear();
        child->GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
        child->ResetLinkFailures();
        child->SetState(Neighbor::kStateParentRequest);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        if (Tlv::GetTlv(aMessage, Tlv::kTimeRequest, sizeof(timeRequest), timeRequest) == OT_ERROR_NONE)
        {
            child->SetTimeSyncEnabled(true);
        }
        else
        {
            child->SetTimeSyncEnabled(false);
        }
#endif
    }
    else if (TimerMilli::GetNow() - child->GetLastHeard() < kParentRequestRouterTimeout - kParentRequestDuplicateMargin)
    {
        ExitNow(error = OT_ERROR_DUPLICATED);
    }

    if (!child->IsStateValidOrRestoring())
    {
        child->SetLastHeard(TimerMilli::GetNow());
        child->SetTimeout(Time::MsecToSec(kMaxChildIdRequestTimeout));
    }

    SendParentResponse(child, challenge, !scanMask.IsEndDeviceFlagSet());

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Parent Request: %s", otThreadErrorToString(error));
    }

    return error;
}

void MleRouter::HandleStateUpdateTimer(Timer &aTimer)
{
    aTimer.GetOwner<MleRouter>().HandleStateUpdateTimer();
}

void MleRouter::HandleStateUpdateTimer(void)
{
    bool routerStateUpdate = false;

    VerifyOrExit(IsFullThreadDevice());

    mStateUpdateTimer.Start(kStateUpdatePeriod);

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

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        assert(false);
        break;

    case OT_DEVICE_ROLE_DETACHED:
        if (mChallengeTimeout == 0)
        {
            BecomeDetached();
            ExitNow();
        }

        break;

    case OT_DEVICE_ROLE_CHILD:
        if (routerStateUpdate)
        {
            if (mRouterTable.GetActiveRouterCount() < mRouterUpgradeThreshold)
            {
                // upgrade to Router
                BecomeRouter(ThreadStatusTlv::kTooFewRouters);
            }
            else
            {
                // send announce after decided to stay in REED if needed
                InformPreviousChannel();
            }

            if (!mAdvertiseTimer.IsRunning())
            {
                SendAdvertisement();

                mAdvertiseTimer.Start(Time::SecToMsec(kReedAdvertiseInterval),
                                      Time::SecToMsec(kReedAdvertiseInterval + kReedAdvertiseJitter),
                                      TrickleTimer::kModePlainTimer);
            }

            ExitNow();
        }

        // fall through

    case OT_DEVICE_ROLE_ROUTER:
        // verify path to leader
        otLogDebgMle("network id timeout = %d", mRouterTable.GetLeaderAge());

        if ((mRouterTable.GetActiveRouterCount() > 0) && (mRouterTable.GetLeaderAge() >= mNetworkIdTimeout))
        {
            otLogInfoMle("Router ID Sequence timeout");
            BecomeChild(kAttachSame1);
        }

        if (routerStateUpdate && mRouterTable.GetActiveRouterCount() > mRouterDowngradeThreshold)
        {
            // downgrade to REED
            otLogNoteMle("Downgrade to REED");
            BecomeChild(kAttachSameDowngrade);
        }

        break;

    case OT_DEVICE_ROLE_LEADER:
        break;
    }

    // update children state
    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateAnyExceptInvalid); !iter.IsDone(); iter++)
    {
        Child &  child   = *iter.GetChild();
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
            assert(false);
            break;
        }

        if (TimerMilli::GetNow() - child.GetLastHeard() >= timeout)
        {
            otLogInfoMle("Child timeout expired");
            RemoveNeighbor(child);
        }
        else if ((mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER) && child.IsStateRestored())
        {
            SendChildUpdateRequest(child);
        }
    }

    // update router state
    for (RouterTable::Iterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        Router & router = *iter.GetRouter();
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
                otLogInfoMle("Router timeout expired");
                RemoveNeighbor(router);
                continue;
            }

#else

            if (age >= Time::SecToMsec(kMaxNeighborAge))
            {
                if (age < Time::SecToMsec(kMaxNeighborAge) + kMaxTransmissionCount * kUnicastRetransmissionDelay)
                {
                    otLogInfoMle("Router timeout expired");
                    SendLinkRequest(&router);
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
                otLogInfoMle("Link Request timeout expired");
                RemoveNeighbor(router);
                continue;
            }
        }

        if (GetRole() == OT_DEVICE_ROLE_LEADER)
        {
            if (mRouterTable.GetRouter(router.GetNextHop()) == NULL &&
                mRouterTable.GetLinkCost(router) >= kMaxRouteCost && age >= Time::SecToMsec(kMaxLeaderToRouterTimeout))
            {
                otLogInfoMle("Router ID timeout expired (no route)");
                mRouterTable.Release(router.GetRouterId());
            }
        }
    }

    mRouterTable.ProcessTimerTick();

    SynchronizeChildNetworkData();

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (mRole == OT_DEVICE_ROLE_LEADER || mRole == OT_DEVICE_ROLE_ROUTER)
    {
        Get<TimeSync>().ProcessTimeSync();
    }
#endif

exit:
    return;
}

void MleRouter::SendParentResponse(Child *aChild, const ChallengeTlv &aChallenge, bool aRoutersOnlyRequest)
{
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message;
    uint16_t     delay;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetDirectTransmission();

    SuccessOrExit(error = AppendHeader(*message, Header::kCommandParentResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));
    SuccessOrExit(error = AppendResponse(*message, aChallenge.GetChallenge(), aChallenge.GetChallengeLength()));
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aChild->IsTimeSyncEnabled())
    {
        SuccessOrExit(error = AppendTimeParameter(*message));
    }
#endif

    aChild->GenerateChallenge();

    SuccessOrExit(error = AppendChallenge(*message, aChild->GetChallenge(), aChild->GetChallengeSize()));
    error = AppendLinkMargin(*message, aChild->GetLinkInfo().GetLinkMargin(Get<Mac::Mac>().GetNoiseFloor()));
    SuccessOrExit(error);

    SuccessOrExit(error = AppendConnectivity(*message));
    SuccessOrExit(error = AppendVersion(*message));

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild->GetExtAddress());

    if (aRoutersOnlyRequest)
    {
        delay = 1 + Random::NonCrypto::GetUint16InRange(0, kParentResponseMaxDelayRouters);
    }
    else
    {
        delay = 1 + Random::NonCrypto::GetUint16InRange(0, kParentResponseMaxDelayAll);
    }

    SuccessOrExit(error = AddDelayedResponse(*message, destination, delay));

    LogMleMessage("Delay Parent Response", destination);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError MleRouter::UpdateChildAddresses(const Message &aMessage, uint16_t aOffset, Child &aChild)
{
    otError                  error = OT_ERROR_NONE;
    AddressRegistrationEntry entry;
    Ip6::Address             address;
    Lowpan::Context          context;
    Tlv                      tlv;
    uint8_t                  registeredCount = 0;
    uint8_t                  storedCount     = 0;
    uint16_t                 offset          = 0;
    uint16_t                 end             = 0;

    VerifyOrExit(aMessage.Read(aOffset, sizeof(tlv), &tlv) == sizeof(tlv), error = OT_ERROR_PARSE);
    VerifyOrExit(tlv.GetLength() <= (aMessage.GetLength() - aOffset - sizeof(tlv)), error = OT_ERROR_PARSE);

    offset = aOffset + sizeof(tlv);
    end    = offset + tlv.GetLength();
    aChild.ClearIp6Addresses();

    while (offset < end)
    {
        uint8_t len;

        // read out the control field
        VerifyOrExit(aMessage.Read(offset, 1, &entry) == 1, error = OT_ERROR_PARSE);

        len = entry.GetLength();

        VerifyOrExit(aMessage.Read(offset, len, &entry) == len, error = OT_ERROR_PARSE);

        offset += len;
        registeredCount++;

        if (entry.IsCompressed())
        {
            if (Get<NetworkData::Leader>().GetContext(entry.GetContextId(), context) != OT_ERROR_NONE)
            {
                otLogWarnMle("Failed to get context %d for compressed address from child 0x%04x", entry.GetContextId(),
                             aChild.GetRloc16());
                continue;
            }

            memcpy(&address, context.mPrefix, BitVectorBytes(context.mPrefixLength));
            address.SetIid(entry.GetIid());
        }
        else
        {
            address = *entry.GetIp6Address();
        }

        // We try to accept/add as many IPv6 addresses as possible.
        // "Child ID/Update Response" will indicate the accepted
        // addresses.
        error = aChild.AddIp6Address(GetInstance(), address);

        if (error == OT_ERROR_NONE)
        {
            storedCount++;
            otLogInfoMle("Child 0x%04x IPv6 address[%d]=%s", aChild.GetRloc16(), storedCount,
                         address.ToString().AsCString());
        }
        else
        {
            otLogWarnMle("Error %s adding IPv6 address %s to child 0x%04x", otThreadErrorToString(error),
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

        for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone(); iter++)
        {
            if (iter.GetChild() == &aChild)
            {
                continue;
            }

            IgnoreReturnValue(iter.GetChild()->RemoveIp6Address(GetInstance(), address));
        }

        // Clear EID-to-RLOC cache for the unicast address registered by the child.
        Get<AddressResolver>().Remove(address);
    }

    if (registeredCount == 0)
    {
        otLogInfoMle("Child 0x%04x has no registered IPv6 address", aChild.GetRloc16());
    }
    else
    {
        otLogInfoMle("Child 0x%04x has %d registered IPv6 address%s, %d address%s stored", aChild.GetRloc16(),
                     registeredCount, (registeredCount == 1) ? "" : "es", storedCount, (storedCount == 1) ? "" : "es");
    }

    error = OT_ERROR_NONE;

exit:
    return error;
}

otError MleRouter::HandleChildIdRequest(const Message &         aMessage,
                                        const Ip6::MessageInfo &aMessageInfo,
                                        uint32_t                aKeySequence)
{
    otError                 error    = OT_ERROR_NONE;
    const otThreadLinkInfo *linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    Mac::ExtAddress         macAddr;
    ResponseTlv             response;
    LinkFrameCounterTlv     linkFrameCounter;
    MleFrameCounterTlv      mleFrameCounter;
    ModeTlv                 mode;
    TimeoutTlv              timeout;
    TlvRequestTlv           tlvRequest;
    ActiveTimestampTlv      activeTimestamp;
    PendingTimestampTlv     pendingTimestamp;
    Child *                 child;
    Router *                router;
    uint8_t                 numTlvs;
    uint16_t                addressRegistrationOffset = 0;

    LogMleMessage("Receive Child ID Request", aMessageInfo.GetPeerAddr());

    VerifyOrExit(IsRouterEligible(), error = OT_ERROR_INVALID_STATE);

    // only process message when operating as a child, router, or leader
    VerifyOrExit(IsAttached(), error = OT_ERROR_INVALID_STATE);

    // Find Child
    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);

    child = mChildTable.FindChild(macAddr, Child::kInStateAnyExceptInvalid);
    VerifyOrExit(child != NULL, error = OT_ERROR_ALREADY);

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid() &&
                     memcmp(response.GetResponse(), child->GetChallenge(), child->GetChallengeSize()) == 0,
                 error = OT_ERROR_SECURITY);

    // Remove existing MLE messages
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleGeneral);
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleChildIdRequest);
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleChildUpdateRequest);
    Get<MeshForwarder>().RemoveMessages(*child, Message::kSubTypeMleDataResponse);

    // Link-Layer Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter), linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) == OT_ERROR_NONE)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), error = OT_ERROR_PARSE);
    }
    else
    {
        mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    // Mode
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMode, sizeof(mode), mode));
    VerifyOrExit(mode.IsValid(), error = OT_ERROR_PARSE);

    // Timeout
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout));
    VerifyOrExit(timeout.IsValid(), error = OT_ERROR_PARSE);

    // TLV Request
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest));
    VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= Child::kMaxRequestTlvs, error = OT_ERROR_PARSE);

    // Active Timestamp
    activeTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);
    }

    // Pending Timestamp
    pendingTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);
    }

    if (!mode.GetMode().IsFullThreadDevice())
    {
        SuccessOrExit(error = Tlv::GetOffset(aMessage, Tlv::kAddressRegistration, addressRegistrationOffset));
        SuccessOrExit(error = UpdateChildAddresses(aMessage, addressRegistrationOffset, *child));
    }

    // Remove from router table
    router = mRouterTable.GetRouter(macAddr);
    if (router != NULL)
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
    child->SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    child->SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    child->SetKeySequence(aKeySequence);
    child->SetDeviceMode(mode.GetMode());
    child->GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
    child->SetTimeout(timeout.GetTimeout());

    if (mode.GetMode().IsFullNetworkData())
    {
        child->SetNetworkDataVersion(mLeaderData.GetDataVersion());
    }
    else
    {
        child->SetNetworkDataVersion(mLeaderData.GetStableDataVersion());
    }

    child->ClearRequestTlvs();

    for (numTlvs = 0; numTlvs < tlvRequest.GetLength(); numTlvs++)
    {
        child->SetRequestTlv(numTlvs, tlvRequest.GetTlvs()[numTlvs]);
    }

    if (activeTimestamp.GetLength() == 0 || Get<MeshCoP::ActiveDataset>().Compare(activeTimestamp) != 0)
    {
        child->SetRequestTlv(numTlvs++, Tlv::kActiveDataset);
    }

    if (pendingTimestamp.GetLength() == 0 || Get<MeshCoP::PendingDataset>().Compare(pendingTimestamp) != 0)
    {
        child->SetRequestTlv(numTlvs++, Tlv::kPendingDataset);
    }

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        assert(false);
        break;

    case OT_DEVICE_ROLE_CHILD:
        child->SetState(Neighbor::kStateChildIdRequest);
        BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        SuccessOrExit(error = SendChildIdResponse(*child));
        break;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Child ID Request: %s", otThreadErrorToString(error));
    }

    return error;
}

otError MleRouter::HandleChildUpdateRequest(const Message &         aMessage,
                                            const Ip6::MessageInfo &aMessageInfo,
                                            uint32_t                aKeySequence)
{
    static const uint8_t kMaxResponseTlvs = 10;

    otError         error = OT_ERROR_NONE;
    Mac::ExtAddress macAddr;
    ModeTlv         mode;
    ChallengeTlv    challenge;
    LeaderDataTlv   leaderData;
    TimeoutTlv      timeout;
    Child *         child;
    DeviceMode      oldMode;
    TlvRequestTlv   tlvRequest;
    uint8_t         tlvs[kMaxResponseTlvs];
    uint8_t         tlvslength                = 0;
    uint16_t        addressRegistrationOffset = 0;
    bool            childDidChange            = false;

    LogMleMessage("Receive Child Update Request from child", aMessageInfo.GetPeerAddr());

    // Mode
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMode, sizeof(mode), mode));
    VerifyOrExit(mode.IsValid(), error = OT_ERROR_PARSE);

    // Challenge
    challenge.SetLength(0);
    if (Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge) == OT_ERROR_NONE)
    {
        VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);
        tlvs[tlvslength++] = Tlv::kResponse;
    }

    // Find Child
    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);
    child = mChildTable.FindChild(macAddr, Child::kInStateAnyExceptInvalid);

    tlvs[tlvslength++] = Tlv::kSourceAddress;

    // Not proceed if the Child Update Request is from the peer which is not the device's child or
    // which was the device's child but becomes invalid.
    if (child == NULL || child->IsStateInvalid())
    {
        // For invalid non-sleepy child, Send Child Update Response with status TLV (error)
        if (mode.GetMode().IsRxOnWhenIdle())
        {
            tlvs[tlvslength++] = Tlv::kStatus;
            SendChildUpdateResponse(NULL, aMessageInfo, tlvs, tlvslength, challenge);
        }

        ExitNow();
    }

    oldMode = child->GetDeviceMode();
    child->SetDeviceMode(mode.GetMode());

    tlvs[tlvslength++] = Tlv::kMode;

    // Parent MUST include Leader Data TLV in Child Update Response
    tlvs[tlvslength++] = Tlv::kLeaderData;

    if (challenge.IsValid())
    {
        tlvs[tlvslength++] = Tlv::kMleFrameCounter;
        tlvs[tlvslength++] = Tlv::kLinkFrameCounter;
    }

    // Ip6 Address TLV
    if (Tlv::GetOffset(aMessage, Tlv::kAddressRegistration, addressRegistrationOffset) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = UpdateChildAddresses(aMessage, addressRegistrationOffset, *child));
        tlvs[tlvslength++] = Tlv::kAddressRegistration;
    }

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == OT_ERROR_NONE)
    {
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
    }

    // Timeout
    if (Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout) == OT_ERROR_NONE)
    {
        VerifyOrExit(timeout.IsValid(), error = OT_ERROR_PARSE);

        if (child->GetTimeout() != timeout.GetTimeout())
        {
            child->SetTimeout(timeout.GetTimeout());
            childDidChange = true;
        }

        tlvs[tlvslength++] = Tlv::kTimeout;
    }

    // TLV Request
    if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == OT_ERROR_NONE)
    {
        uint8_t            tlv;
        TlvRequestIterator iterator = TLVREQUESTTLV_ITERATOR_INIT;

        VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= (kMaxResponseTlvs - tlvslength),
                     error = OT_ERROR_PARSE);

        while (tlvRequest.GetNextTlv(iterator, tlv) == OT_ERROR_NONE)
        {
            // Here skips Tlv::kLeaderData because it has already been included by default
            if (tlv != Tlv::kLeaderData)
            {
                tlvs[tlvslength++] = tlv;
            }
        }
    }

    child->SetLastHeard(TimerMilli::GetNow());

    if (oldMode != child->GetDeviceMode())
    {
        otLogNoteMle("Child 0x%04x mode change 0x%02x -> 0x%02x [%s]", child->GetRloc16(), oldMode.Get(),
                     child->GetDeviceMode().Get(), child->GetDeviceMode().ToString().AsCString());

        childDidChange = true;

        // The `IndirectSender::HandleChildModeChange()` needs to happen
        // after "Child Update" message is fully parsed to ensure that
        // any registered IPv6 addresses included in the "Child Update"
        // are added to the child.

        Get<IndirectSender>().HandleChildModeChange(*child, oldMode);
    }

    if (child->IsStateRestoring())
    {
        SetChildStateToValid(*child);
        child->SetKeySequence(aKeySequence);
    }
    else if (child->IsStateValid())
    {
        if (childDidChange)
        {
            StoreChild(*child);
        }
    }

    SendChildUpdateResponse(child, aMessageInfo, tlvs, tlvslength, challenge);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Child Update Request from child: %s", otThreadErrorToString(error));
    }

    return error;
}

otError MleRouter::HandleChildUpdateResponse(const Message &         aMessage,
                                             const Ip6::MessageInfo &aMessageInfo,
                                             uint32_t                aKeySequence,
                                             Neighbor *              aNeighbor)
{
    otError                 error    = OT_ERROR_NONE;
    const otThreadLinkInfo *linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    SourceAddressTlv        sourceAddress;
    TimeoutTlv              timeout;
    ResponseTlv             response;
    StatusTlv               status;
    LinkFrameCounterTlv     linkFrameCounter;
    MleFrameCounterTlv      mleFrameCounter;
    LeaderDataTlv           leaderData;
    Child *                 child;
    uint16_t                addressRegistrationOffset = 0;

    if ((aNeighbor == NULL) || IsActiveRouter(aNeighbor->GetRloc16()))
    {
        LogMleMessage("Receive Child Update Response from unknown child", aMessageInfo.GetPeerAddr());
        ExitNow(error = OT_ERROR_NOT_FOUND);
    }

    child = static_cast<Child *>(aNeighbor);

    // Response
    if (Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response) == OT_ERROR_NONE)
    {
        VerifyOrExit(response.IsValid() &&
                         memcmp(response.GetResponse(), child->GetChallenge(), child->GetChallengeSize()) == 0,
                     error = OT_ERROR_SECURITY);
    }
    else
    {
        VerifyOrExit(child->IsStateValid(), error = OT_ERROR_SECURITY);
    }

    LogMleMessage("Receive Child Update Response from child", aMessageInfo.GetPeerAddr(), child->GetRloc16());

    // Source Address
    if (Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress) == OT_ERROR_NONE)
    {
        VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

        if (child->GetRloc16() != sourceAddress.GetRloc16())
        {
            RemoveNeighbor(*child);
            ExitNow();
        }
    }

    // Status
    if (Tlv::GetTlv(aMessage, Tlv::kStatus, sizeof(status), status) == OT_ERROR_NONE)
    {
        VerifyOrExit(status.IsValid(), error = OT_ERROR_PARSE);

        if (status.GetStatus() == StatusTlv::kError)
        {
            RemoveNeighbor(*child);
            ExitNow();
        }
    }

    // Link-Layer Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter), linkFrameCounter) == OT_ERROR_NONE)
    {
        VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);
        child->SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) == OT_ERROR_NONE)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), error = OT_ERROR_PARSE);
        child->SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    }

    // Timeout
    if (Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout) == OT_ERROR_NONE)
    {
        VerifyOrExit(timeout.IsValid(), error = OT_ERROR_PARSE);
        child->SetTimeout(timeout.GetTimeout());
    }

    // Ip6 Address
    if (Tlv::GetOffset(aMessage, Tlv::kAddressRegistration, addressRegistrationOffset) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = UpdateChildAddresses(aMessage, addressRegistrationOffset, *child));
    }

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == OT_ERROR_NONE)
    {
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

        if (child->IsFullNetworkData())
        {
            child->SetNetworkDataVersion(leaderData.GetDataVersion());
        }
        else
        {
            child->SetNetworkDataVersion(leaderData.GetStableDataVersion());
        }
    }

    SetChildStateToValid(*child);
    child->SetLastHeard(TimerMilli::GetNow());
    child->SetKeySequence(aKeySequence);
    child->GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Child Update Response from child: %s", otThreadErrorToString(error));
    }

    return error;
}

otError MleRouter::HandleDataRequest(const Message &         aMessage,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     const Neighbor *        aNeighbor)
{
    otError             error = OT_ERROR_NONE;
    TlvRequestTlv       tlvRequest;
    ActiveTimestampTlv  activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    uint8_t             tlvs[4];
    uint8_t             numTlvs;

    LogMleMessage("Receive Data Request", aMessageInfo.GetPeerAddr());

    VerifyOrExit(aNeighbor && aNeighbor->IsStateValid(), error = OT_ERROR_SECURITY);

    // TLV Request
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest));
    VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= sizeof(tlvs), error = OT_ERROR_PARSE);

    // Active Timestamp
    activeTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);
    }

    // Pending Timestamp
    pendingTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);
    }

    memset(tlvs, Tlv::kInvalid, sizeof(tlvs));
    memcpy(tlvs, tlvRequest.GetTlvs(), tlvRequest.GetLength());
    numTlvs = tlvRequest.GetLength();

    if (activeTimestamp.GetLength() == 0 || Get<MeshCoP::ActiveDataset>().Compare(activeTimestamp))
    {
        tlvs[numTlvs++] = Tlv::kActiveDataset;
    }

    if (pendingTimestamp.GetLength() == 0 || Get<MeshCoP::PendingDataset>().Compare(pendingTimestamp))
    {
        tlvs[numTlvs++] = Tlv::kPendingDataset;
    }

    SendDataResponse(aMessageInfo.GetPeerAddr(), tlvs, numTlvs, 0);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Data Request: %s", otThreadErrorToString(error));
    }

    return error;
}

void MleRouter::HandleNetworkDataUpdateRouter(void)
{
    static const uint8_t tlvs[] = {Tlv::kNetworkData};
    Ip6::Address         destination;
    uint16_t             delay;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER);

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);

    delay =
        (mRole == OT_DEVICE_ROLE_LEADER) ? 0 : Random::NonCrypto::GetUint16InRange(0, kUnsolicitedDataResponseJitter);
    SendDataResponse(destination, tlvs, sizeof(tlvs), delay);

    SynchronizeChildNetworkData();

exit:
    return;
}

void MleRouter::SynchronizeChildNetworkData(void)
{
    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER);

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValid); !iter.IsDone(); iter++)
    {
        Child & child = *iter.GetChild();
        uint8_t version;

        if (child.IsRxOnWhenIdle())
        {
            continue;
        }

        if (child.IsFullNetworkData())
        {
            version = Get<NetworkData::Leader>().GetVersion();
        }
        else
        {
            version = Get<NetworkData::Leader>().GetStableVersion();
        }

        if (child.GetNetworkDataVersion() == version)
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

    mSteeringData.Init();

    if ((aExtAddress == NULL) || (*aExtAddress == nullExtAddr))
    {
        // Clear steering data
        mSteeringData.Clear();
    }
    else if (*aExtAddress == allowAnyExtAddr)
    {
        // Set steering data to 0xFF
        mSteeringData.SetLength(1);
        mSteeringData.Set();
    }
    else
    {
        Mac::ExtAddress joinerId;

        // compute Joiner ID
        MeshCoP::ComputeJoinerId(*aExtAddress, joinerId);
        // compute Bloom Filter
        mSteeringData.ComputeBloomFilter(joinerId);
    }
}
#endif // OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE

otError MleRouter::HandleDiscoveryRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                      error = OT_ERROR_NONE;
    Tlv                          tlv;
    MeshCoP::Tlv                 meshcopTlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    MeshCoP::ExtendedPanIdTlv    extPanId;
    uint16_t                     offset;
    uint16_t                     end;

    LogMleMessage("Receive Discovery Request", aMessageInfo.GetPeerAddr());

    // only Routers and REEDs respond
    VerifyOrExit(IsRouterEligible(), error = OT_ERROR_INVALID_STATE);

    // find MLE Discovery TLV
    VerifyOrExit(Tlv::GetOffset(aMessage, Tlv::kDiscovery, offset) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
    aMessage.Read(offset, sizeof(tlv), &tlv);

    offset += sizeof(tlv);
    end = offset + sizeof(tlv) + tlv.GetLength();

    while (offset < end)
    {
        aMessage.Read(offset, sizeof(meshcopTlv), &meshcopTlv);

        switch (meshcopTlv.GetType())
        {
        case MeshCoP::Tlv::kDiscoveryRequest:
            aMessage.Read(offset, sizeof(discoveryRequest), &discoveryRequest);
            VerifyOrExit(discoveryRequest.IsValid(), error = OT_ERROR_PARSE);

            if (discoveryRequest.IsJoiner())
            {
#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE

                if (!mSteeringData.IsCleared())
                {
                    break;
                }
                else // if steering data is not set out of band, fall back to network data
#endif               // OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
                {
                    VerifyOrExit(Get<NetworkData::Leader>().IsJoiningEnabled());
                }
            }

            break;

        case MeshCoP::Tlv::kExtendedPanId:
            aMessage.Read(offset, sizeof(extPanId), &extPanId);
            VerifyOrExit(extPanId.IsValid(), error = OT_ERROR_PARSE);
            VerifyOrExit(memcmp(&Get<Mac::Mac>().GetExtendedPanId(), &extPanId.GetExtendedPanId(), OT_EXT_PAN_ID_SIZE));

            break;

        default:
            break;
        }

        offset += sizeof(meshcopTlv) + meshcopTlv.GetLength();
    }

    error = SendDiscoveryResponse(aMessageInfo.GetPeerAddr(), aMessage.GetPanId());

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Discovery Request: %s", otThreadErrorToString(error));
    }

    return error;
}

otError MleRouter::SendDiscoveryResponse(const Ip6::Address &aDestination, uint16_t aPanId)
{
    otError                       error = OT_ERROR_NONE;
    Message *                     message;
    uint16_t                      startOffset;
    Tlv                           tlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::ExtendedPanIdTlv     extPanId;
    MeshCoP::NetworkNameTlv       networkName;
    MeshCoP::JoinerUdpPortTlv     joinerUdpPort;
    MeshCoP::Tlv *                steeringData;
    uint16_t                      delay;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleDiscoverResponse);
    message->SetPanId(aPanId);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDiscoveryResponse));

    // Discovery TLV
    tlv.SetType(Tlv::kDiscovery);
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

    startOffset = message->GetLength();

    // Discovery Response TLV
    discoveryResponse.Init();
    discoveryResponse.SetVersion(kThreadVersion);

    if (Get<KeyManager>().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_NATIVE_COMMISSIONING)
    {
        MeshCoP::CommissionerUdpPortTlv commissionerUdpPort;

        commissionerUdpPort.Init();
        commissionerUdpPort.SetUdpPort(MeshCoP::kBorderAgentUdpPort);
        SuccessOrExit(error = commissionerUdpPort.AppendTo(*message));

        discoveryResponse.SetNativeCommissioner(true);
    }
    else
    {
        discoveryResponse.SetNativeCommissioner(false);
    }

    SuccessOrExit(error = discoveryResponse.AppendTo(*message));

    // Extended PAN ID TLV
    extPanId.Init();
    extPanId.SetExtendedPanId(Get<Mac::Mac>().GetExtendedPanId());
    SuccessOrExit(error = extPanId.AppendTo(*message));

    // Network Name TLV
    networkName.Init();
    networkName.SetNetworkName(Get<Mac::Mac>().GetNetworkName().GetAsData());
    SuccessOrExit(error = networkName.AppendTo(*message));

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE

    // If steering data is set out of band, use that value.
    // Otherwise use the one from commissioning data.
    if (!mSteeringData.IsCleared())
    {
        SuccessOrExit(error = mSteeringData.AppendTo(*message));
    }
    else
#endif // OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    {
        // Steering Data TLV
        steeringData = Get<NetworkData::Leader>().GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);

        if (steeringData != NULL)
        {
            SuccessOrExit(error = steeringData->AppendTo(*message));
        }
    }

    // Joiner UDP Port TLV
    joinerUdpPort.Init();
    joinerUdpPort.SetUdpPort(Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort());
    SuccessOrExit(error = joinerUdpPort.AppendTo(*message));

    tlv.SetLength(static_cast<uint8_t>(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    delay = Random::NonCrypto::GetUint16InRange(0, kDiscoveryMaxJitter + 1);

    SuccessOrExit(error = AddDelayedResponse(*message, aDestination, delay));

    LogMleMessage("Delay Discovery Response", aDestination);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Discovery Response: %s", otThreadErrorToString(error));

        if (message != NULL)
        {
            message->Free();
        }
    }

    return error;
}

otError MleRouter::SendChildIdResponse(Child &aChild)
{
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildIdResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    if (aChild.GetRloc16() == 0)
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

        } while (mChildTable.FindChild(rloc16, Child::kInStateAnyExceptInvalid) != NULL);

        // allocate Child ID
        aChild.SetRloc16(rloc16);
    }

    SuccessOrExit(error = AppendAddress16(*message, aChild.GetRloc16()));

    for (uint8_t i = 0; i < Child::kMaxRequestTlvs; i++)
    {
        switch (aChild.GetRequestTlv(i))
        {
        case Tlv::kNetworkData:
            SuccessOrExit(error = AppendNetworkData(*message, !aChild.IsFullNetworkData()));
            break;

        case Tlv::kRoute:
            SuccessOrExit(error = AppendRoute(*message));
            break;

        case Tlv::kActiveDataset:
            SuccessOrExit(error = AppendActiveDataset(*message));
            break;

        case Tlv::kPendingDataset:
            SuccessOrExit(error = AppendPendingDataset(*message));
            break;

        default:
            break;
        }
    }

    if (!aChild.IsFullThreadDevice())
    {
        SuccessOrExit(error = AppendChildAddresses(*message, aChild));
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

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    LogMleMessage("Send Child ID Response", destination, aChild.GetRloc16());

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendChildUpdateRequest(Child &aChild)
{
    static const uint8_t tlvs[] = {Tlv::kTimeout, Tlv::kAddressRegistration};
    otError              error  = OT_ERROR_NONE;
    Ip6::Address         destination;
    Message *            message;

    if (!aChild.IsRxOnWhenIdle())
    {
        uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

        for (message = Get<MeshForwarder>().GetSendQueue().GetHead(); message; message = message->GetNext())
        {
            if (message->GetChildMask(childIndex) && message->GetSubType() == Message::kSubTypeMleChildUpdateRequest)
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

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleChildUpdateRequest);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateRequest));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendNetworkData(*message, !aChild.IsFullNetworkData()));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    if (!aChild.IsStateValid())
    {
        SuccessOrExit(error = AppendTlvRequest(*message, tlvs, sizeof(tlvs)));
        aChild.GenerateChallenge();
        SuccessOrExit(error = AppendChallenge(*message, aChild.GetChallenge(), aChild.GetChallengeSize()));
    }

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    if (aChild.IsRxOnWhenIdle())
    {
        // only try to send a single Child Update Request message to an rx-on-when-idle child
        aChild.SetState(Child::kStateChildUpdateRequest);
    }

    LogMleMessage("Send Child Update Request to child", destination, aChild.GetRloc16());

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void MleRouter::SendChildUpdateResponse(Child *                 aChild,
                                        const Ip6::MessageInfo &aMessageInfo,
                                        const uint8_t *         aTlvs,
                                        uint8_t                 aTlvsLength,
                                        const ChallengeTlv &    aChallenge)
{
    otError  error = OT_ERROR_NONE;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateResponse));

    for (int i = 0; i < aTlvsLength; i++)
    {
        switch (aTlvs[i])
        {
        case Tlv::kStatus:
            SuccessOrExit(error = AppendStatus(*message, StatusTlv::kError));
            break;

        case Tlv::kAddressRegistration:
            SuccessOrExit(error = AppendChildAddresses(*message, *aChild));
            break;

        case Tlv::kLeaderData:
            SuccessOrExit(error = AppendLeaderData(*message));
            break;

        case Tlv::kMode:
            SuccessOrExit(error = AppendMode(*message, aChild->GetDeviceMode()));
            break;

        case Tlv::kNetworkData:
            SuccessOrExit(error = AppendNetworkData(*message, !aChild->IsFullNetworkData()));
            SuccessOrExit(error = AppendActiveTimestamp(*message));
            SuccessOrExit(error = AppendPendingTimestamp(*message));
            break;

        case Tlv::kResponse:
            SuccessOrExit(error = AppendResponse(*message, aChallenge.GetChallenge(), aChallenge.GetChallengeLength()));
            break;

        case Tlv::kSourceAddress:
            SuccessOrExit(error = AppendSourceAddress(*message));
            break;

        case Tlv::kTimeout:
            SuccessOrExit(error = AppendTimeout(*message, aChild->GetTimeout()));
            break;

        case Tlv::kMleFrameCounter:
            SuccessOrExit(error = AppendMleFrameCounter(*message));
            break;

        case Tlv::kLinkFrameCounter:
            SuccessOrExit(error = AppendLinkFrameCounter(*message));
            break;
        }
    }

    SuccessOrExit(error = SendMessage(*message, aMessageInfo.GetPeerAddr()));

    if (aChild == NULL)
    {
        LogMleMessage("Send Child Update Response to child", aMessageInfo.GetPeerAddr());
    }
    else
    {
        LogMleMessage("Send Child Update Response to child", aMessageInfo.GetPeerAddr(), aChild->GetRloc16());
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError MleRouter::SendDataResponse(const Ip6::Address &aDestination,
                                    const uint8_t *     aTlvs,
                                    uint8_t             aTlvsLength,
                                    uint16_t            aDelay)
{
    otError   error   = OT_ERROR_NONE;
    Message * message = NULL;
    Neighbor *neighbor;
    bool      stableOnly;

    if (mRetrieveNewNetworkData)
    {
        otLogInfoMle("Suppressing Data Response - waiting for new network data");
        ExitNow();
    }

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleDataResponse);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDataResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    for (int i = 0; i < aTlvsLength; i++)
    {
        switch (aTlvs[i])
        {
        case Tlv::kNetworkData:
            neighbor   = GetNeighbor(aDestination);
            stableOnly = neighbor != NULL ? !neighbor->IsFullNetworkData() : false;
            SuccessOrExit(error = AppendNetworkData(*message, stableOnly));
            break;

        case Tlv::kActiveDataset:
            SuccessOrExit(error = AppendActiveDataset(*message));
            break;

        case Tlv::kPendingDataset:
            SuccessOrExit(error = AppendPendingDataset(*message));
            break;
        }
    }

    if (aDelay)
    {
        // Remove MLE Data Responses from Send Message Queue.
        Get<MeshForwarder>().RemoveDataResponseMessages();

        // Remove multicast MLE Data Response from Delayed Message Queue.
        RemoveDelayedDataResponseMessage();

        SuccessOrExit(error = AddDelayedResponse(*message, aDestination, aDelay));
        LogMleMessage("Delay Data Response", aDestination);
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aDestination));
        LogMleMessage("Send Data Response", aDestination);
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

bool MleRouter::IsMinimalChild(uint16_t aRloc16)
{
    bool rval = false;

    if (RouterIdFromRloc16(aRloc16) == RouterIdFromRloc16(Get<Mac::Mac>().GetShortAddress()))
    {
        Neighbor *neighbor;

        neighbor = GetNeighbor(aRloc16);

        rval = (neighbor != NULL) && (!neighbor->IsFullThreadDevice());
    }

    return rval;
}

void MleRouter::RemoveNeighbor(Neighbor &aNeighbor)
{
    if (&aNeighbor == &mParent)
    {
        if (mRole == OT_DEVICE_ROLE_CHILD)
        {
            BecomeDetached();
        }
    }
    else if (!IsActiveRouter(aNeighbor.GetRloc16()))
    {
        if (aNeighbor.IsStateValidOrRestoring())
        {
            Signal(OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED, aNeighbor);
        }

        Get<IndirectSender>().ClearAllMessagesForSleepyChild(static_cast<Child &>(aNeighbor));
        Get<NetworkData::Leader>().SendServerDataNotification(aNeighbor.GetRloc16());

        if (aNeighbor.IsFullThreadDevice())
        {
            // Clear all EID-to-RLOC entries associated with the child.
            Get<AddressResolver>().Remove(aNeighbor.GetRloc16());
        }

        RemoveStoredChild(aNeighbor.GetRloc16());
    }
    else if (aNeighbor.IsStateValid())
    {
        Signal(OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED, aNeighbor);
        mRouterTable.RemoveNeighbor(static_cast<Router &>(aNeighbor));
    }

    aNeighbor.GetLinkInfo().Clear();
    aNeighbor.SetState(Neighbor::kStateInvalid);
}

Neighbor *MleRouter::GetNeighbor(uint16_t aAddress)
{
    Neighbor *rval = NULL;

    if (aAddress == Mac::kShortAddrBroadcast || aAddress == Mac::kShortAddrInvalid)
    {
        ExitNow();
    }

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        break;

    case OT_DEVICE_ROLE_DETACHED:
    case OT_DEVICE_ROLE_CHILD:
        rval = Mle::GetNeighbor(aAddress);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        rval = mChildTable.FindChild(aAddress, Child::kInStateValidOrRestoring);
        VerifyOrExit(rval == NULL);

        rval = mRouterTable.GetNeighbor(aAddress);
        break;
    }

exit:
    return rval;
}

Neighbor *MleRouter::GetNeighbor(const Mac::ExtAddress &aAddress)
{
    Neighbor *rval = NULL;

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        break;

    case OT_DEVICE_ROLE_DETACHED:
    case OT_DEVICE_ROLE_CHILD:
        rval = Mle::GetNeighbor(aAddress);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        rval = mChildTable.FindChild(aAddress, Child::kInStateValidOrRestoring);
        VerifyOrExit(rval == NULL);

        rval = mRouterTable.GetNeighbor(aAddress);

        if (rval != NULL)
        {
            ExitNow();
        }

        if (IsAttaching())
        {
            rval = Mle::GetNeighbor(aAddress);
        }

        break;
    }

exit:
    return rval;
}

Neighbor *MleRouter::GetNeighbor(const Mac::Address &aAddress)
{
    Neighbor *rval = NULL;

    switch (aAddress.GetType())
    {
    case Mac::Address::kTypeShort:
        rval = GetNeighbor(aAddress.GetShort());
        break;

    case Mac::Address::kTypeExtended:
        rval = GetNeighbor(aAddress.GetExtended());
        break;

    default:
        break;
    }

    return rval;
}

Neighbor *MleRouter::GetNeighbor(const Ip6::Address &aAddress)
{
    Mac::Address    macAddr;
    Lowpan::Context context;
    Child *         child;
    Neighbor *      rval = NULL;

    if (aAddress.IsLinkLocal())
    {
        if (aAddress.mFields.m16[4] == HostSwap16(0x0000) && aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
            aAddress.mFields.m16[6] == HostSwap16(0xfe00))
        {
            macAddr.SetShort(HostSwap16(aAddress.mFields.m16[7]));
        }
        else
        {
            aAddress.ToExtAddress(macAddr);
        }

        ExitNow(rval = GetNeighbor(macAddr));
    }

    if (Get<NetworkData::Leader>().GetContext(aAddress, context) != OT_ERROR_NONE)
    {
        context.mContextId = 0xff;
    }

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone(); iter++)
    {
        child = iter.GetChild();

        if (context.mContextId == kMeshLocalPrefixContextId && aAddress.mFields.m16[4] == HostSwap16(0x0000) &&
            aAddress.mFields.m16[5] == HostSwap16(0x00ff) && aAddress.mFields.m16[6] == HostSwap16(0xfe00) &&
            aAddress.mFields.m16[7] == HostSwap16(child->GetRloc16()))
        {
            ExitNow(rval = child);
        }

        if (child->HasIp6Address(GetInstance(), aAddress))
        {
            ExitNow(rval = child);
        }
    }

    VerifyOrExit(context.mContextId == kMeshLocalPrefixContextId, rval = NULL);

    if (aAddress.mFields.m16[4] == HostSwap16(0x0000) && aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
        aAddress.mFields.m16[6] == HostSwap16(0xfe00))
    {
        rval = mRouterTable.GetNeighbor(HostSwap16(aAddress.mFields.m16[7]));
    }

exit:
    return rval;
}

Neighbor *MleRouter::GetRxOnlyNeighborRouter(const Mac::Address &aAddress)
{
    Neighbor *rval = NULL;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_CHILD, rval = NULL);

    switch (aAddress.GetType())
    {
    case Mac::Address::kTypeShort:
        rval = mRouterTable.GetNeighbor(aAddress.GetShort());
        break;

    case Mac::Address::kTypeExtended:
        rval = mRouterTable.GetNeighbor(aAddress.GetExtended());
        break;

    default:
        break;
    }

exit:
    return rval;
}

uint16_t MleRouter::GetNextHop(uint16_t aDestination)
{
    uint8_t       destinationId = RouterIdFromRloc16(aDestination);
    uint8_t       routeCost;
    uint8_t       linkCost;
    uint16_t      rval = Mac::kShortAddrInvalid;
    const Router *router;
    const Router *nextHop;

    if (mRole == OT_DEVICE_ROLE_CHILD)
    {
        ExitNow(rval = Mle::GetNextHop(aDestination));
    }

    // The frame is destined to a child
    if (destinationId == mRouterId)
    {
        ExitNow(rval = aDestination);
    }

    router = mRouterTable.GetRouter(destinationId);
    VerifyOrExit(router != NULL);

    linkCost  = GetLinkCost(destinationId);
    routeCost = GetRouteCost(aDestination);

    if ((routeCost + GetLinkCost(router->GetNextHop())) < linkCost)
    {
        nextHop = mRouterTable.GetRouter(router->GetNextHop());
        VerifyOrExit(nextHop != NULL && !nextHop->IsStateInvalid());

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

    VerifyOrExit(router != NULL && mRouterTable.GetRouter(router->GetNextHop()) != NULL);

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
    VerifyOrExit(router != NULL && mRouterTable.GetRouter(router->GetNextHop()) != NULL);

    rval = router->GetCost();

exit:
    return rval;
}

otError MleRouter::SetPreferredRouterId(uint8_t aRouterId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit((mRole == OT_DEVICE_ROLE_DETACHED) || (mRole == OT_DEVICE_ROLE_DISABLED),
                 error = OT_ERROR_INVALID_STATE);

    mPreviousRouterId = aRouterId;

exit:
    return error;
}

void MleRouter::SetRouterId(uint8_t aRouterId)
{
    mRouterId         = aRouterId;
    mPreviousRouterId = mRouterId;
}

otError MleRouter::GetChildInfoById(uint16_t aChildId, otChildInfo &aChildInfo)
{
    otError  error = OT_ERROR_NONE;
    Child *  child;
    uint16_t rloc16;

    if ((aChildId & ~kMaxChildId) != 0)
    {
        aChildId = ChildIdFromRloc16(aChildId);
    }

    rloc16 = Get<Mac::Mac>().GetShortAddress() | aChildId;
    child  = mChildTable.FindChild(rloc16, Child::kInStateAnyExceptInvalid);
    VerifyOrExit(child != NULL, error = OT_ERROR_NOT_FOUND);

    error = GetChildInfo(*child, aChildInfo);

exit:
    return error;
}

otError MleRouter::GetChildInfoByIndex(uint16_t aChildIndex, otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;
    Child * child = NULL;

    child = mChildTable.GetChildAtIndex(aChildIndex);
    VerifyOrExit(child != NULL, error = OT_ERROR_NOT_FOUND);

    error = GetChildInfo(*child, aChildInfo);

exit:
    return error;
}

otError MleRouter::GetChildNextIp6Address(uint16_t                   aChildIndex,
                                          Child::Ip6AddressIterator &aIterator,
                                          Ip6::Address &             aAddress)
{
    otError error = OT_ERROR_NONE;
    Child * child = NULL;

    child = mChildTable.GetChildAtIndex(aChildIndex);
    VerifyOrExit(child != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(child->IsStateValidOrRestoring(), error = OT_ERROR_INVALID_ARGS);

    error = child->GetNextIp6Address(GetInstance(), aIterator, aAddress);

exit:
    return error;
}

void MleRouter::RestoreChildren(void)
{
    otError  error          = OT_ERROR_NONE;
    bool     foundDuplicate = false;
    uint16_t numChildren    = 0;

    for (Settings::ChildInfoIterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        Child *                    child;
        const Settings::ChildInfo &childInfo = iter.GetChildInfo();

        child = mChildTable.FindChild(childInfo.GetExtAddress(), Child::kInStateAnyExceptInvalid);

        if (child == NULL)
        {
            VerifyOrExit((child = mChildTable.GetNewChild()) != NULL, error = OT_ERROR_NO_BUFS);
        }
        else
        {
            foundDuplicate = true;
        }

        child->Clear();

        child->SetExtAddress(childInfo.GetExtAddress());
        child->GetLinkInfo().Clear();
        child->SetRloc16(childInfo.GetRloc16());
        child->SetTimeout(childInfo.GetTimeout());
        child->SetDeviceMode(DeviceMode(childInfo.GetMode()));
        child->SetState(Neighbor::kStateRestored);
        child->SetLastHeard(TimerMilli::GetNow());
        Get<IndirectSender>().SetChildUseShortAddress(*child, true);
        numChildren++;
    }

exit:

    if (foundDuplicate || (numChildren > mChildTable.GetMaxChildren()) || (error != OT_ERROR_NONE))
    {
        // If there is any error, e.g., there are more saved children
        // in non-volatile settings than could be restored or there are
        // duplicate entries with same extended address, refresh the stored
        // children info to ensure that the non-volatile settings remain
        // consistent with the child table.

        RefreshStoredChildren();
    }
}

otError MleRouter::RemoveStoredChild(uint16_t aChildRloc16)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (Settings::ChildInfoIterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        if (iter.GetChildInfo().GetRloc16() == aChildRloc16)
        {
            error = iter.Delete();
            ExitNow();
        }
    }

exit:
    return error;
}

otError MleRouter::StoreChild(const Child &aChild)
{
    Settings::ChildInfo childInfo;

    IgnoreReturnValue(RemoveStoredChild(aChild.GetRloc16()));

    childInfo.Init();
    childInfo.SetExtAddress(aChild.GetExtAddress());
    childInfo.SetTimeout(aChild.GetTimeout());
    childInfo.SetRloc16(aChild.GetRloc16());
    childInfo.SetMode(aChild.GetDeviceMode().Get());

    return Get<Settings>().AddChildInfo(childInfo);
}

otError MleRouter::RefreshStoredChildren(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = Get<Settings>().DeleteChildInfo());

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateAnyExceptInvalid); !iter.IsDone(); iter++)
    {
        SuccessOrExit(error = StoreChild(*iter.GetChild()));
    }

exit:
    return error;
}

otError MleRouter::GetChildInfo(Child &aChild, otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aChild.IsStateValidOrRestoring(), error = OT_ERROR_NOT_FOUND);

    memset(&aChildInfo, 0, sizeof(aChildInfo));
    aChildInfo.mExtAddress         = aChild.GetExtAddress();
    aChildInfo.mTimeout            = aChild.GetTimeout();
    aChildInfo.mRloc16             = aChild.GetRloc16();
    aChildInfo.mChildId            = ChildIdFromRloc16(aChild.GetRloc16());
    aChildInfo.mNetworkDataVersion = aChild.GetNetworkDataVersion();
    aChildInfo.mAge                = Time::MsecToSec(TimerMilli::GetNow() - aChild.GetLastHeard());
    aChildInfo.mLinkQualityIn      = aChild.GetLinkInfo().GetLinkQuality();
    aChildInfo.mAverageRssi        = aChild.GetLinkInfo().GetAverageRss();
    aChildInfo.mLastRssi           = aChild.GetLinkInfo().GetLastRss();
    aChildInfo.mFrameErrorRate     = aChild.GetLinkInfo().GetFrameErrorRate();
    aChildInfo.mMessageErrorRate   = aChild.GetLinkInfo().GetMessageErrorRate();
    aChildInfo.mRxOnWhenIdle       = aChild.IsRxOnWhenIdle();
    aChildInfo.mSecureDataRequest  = aChild.IsSecureDataRequest();
    aChildInfo.mFullThreadDevice   = aChild.IsFullThreadDevice();
    aChildInfo.mFullNetworkData    = aChild.IsFullNetworkData();
    aChildInfo.mIsStateRestoring   = aChild.IsStateRestoring();

exit:
    return error;
}

void MleRouter::GetNeighborInfo(Neighbor &aNeighbor, otNeighborInfo &aNeighInfo)
{
    aNeighInfo.mExtAddress        = aNeighbor.GetExtAddress();
    aNeighInfo.mAge               = Time::MsecToSec(TimerMilli::GetNow() - aNeighbor.GetLastHeard());
    aNeighInfo.mRloc16            = aNeighbor.GetRloc16();
    aNeighInfo.mLinkFrameCounter  = aNeighbor.GetLinkFrameCounter();
    aNeighInfo.mMleFrameCounter   = aNeighbor.GetMleFrameCounter();
    aNeighInfo.mLinkQualityIn     = aNeighbor.GetLinkInfo().GetLinkQuality();
    aNeighInfo.mAverageRssi       = aNeighbor.GetLinkInfo().GetAverageRss();
    aNeighInfo.mLastRssi          = aNeighbor.GetLinkInfo().GetLastRss();
    aNeighInfo.mFrameErrorRate    = aNeighbor.GetLinkInfo().GetFrameErrorRate();
    aNeighInfo.mMessageErrorRate  = aNeighbor.GetLinkInfo().GetMessageErrorRate();
    aNeighInfo.mRxOnWhenIdle      = aNeighbor.IsRxOnWhenIdle();
    aNeighInfo.mSecureDataRequest = aNeighbor.IsSecureDataRequest();
    aNeighInfo.mFullThreadDevice  = aNeighbor.IsFullThreadDevice();
    aNeighInfo.mFullNetworkData   = aNeighbor.IsFullNetworkData();
}

otError MleRouter::GetNextNeighborInfo(otNeighborInfoIterator &aIterator, otNeighborInfo &aNeighInfo)
{
    otError   error    = OT_ERROR_NONE;
    Neighbor *neighbor = NULL;
    int16_t   index;

    memset(&aNeighInfo, 0, sizeof(aNeighInfo));

    // Non-negative iterator value gives the Child index into child table

    if (aIterator >= 0)
    {
        for (index = aIterator;; index++)
        {
            Child *child = mChildTable.GetChildAtIndex(static_cast<uint16_t>(index));

            if (child == NULL)
            {
                break;
            }

            if (child->IsStateValid())
            {
                neighbor            = child;
                aNeighInfo.mIsChild = true;
                index++;
                aIterator = index;
                ExitNow();
            }
        }

        aIterator = 0;
    }

    // Negative iterator value gives the current index into mRouters array

    for (index = -aIterator; index <= kMaxRouterId; index++)
    {
        Router *router = mRouterTable.GetRouter(static_cast<uint8_t>(index));

        if (router != NULL && router->IsStateValid())
        {
            neighbor            = router;
            aNeighInfo.mIsChild = false;
            index++;
            aIterator = -index;
            ExitNow();
        }
    }

    aIterator = -index;
    error     = OT_ERROR_NOT_FOUND;

exit:

    if (neighbor != NULL)
    {
        GetNeighborInfo(*neighbor, aNeighInfo);
    }

    return error;
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
    VerifyOrExit(router != NULL);

    // invalidate next hop
    router->SetNextHop(kInvalidRouterId);
    ResetAdvertiseInterval();

exit:
    return;
}

otError MleRouter::CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    Ip6::MessageInfo messageInfo;
    otError          error = OT_ERROR_NONE;

    if (mRole == OT_DEVICE_ROLE_CHILD)
    {
        error = Mle::CheckReachability(aMeshSource, aMeshDest, aIp6Header);
        ExitNow();
    }

    if (aMeshDest == Get<Mac::Mac>().GetShortAddress())
    {
        // mesh destination is this device
        if (Get<ThreadNetif>().IsUnicastAddress(aIp6Header.GetDestination()))
        {
            // IPv6 destination is this device
            ExitNow();
        }
        else if (GetNeighbor(aIp6Header.GetDestination()) != NULL)
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

    messageInfo.GetPeerAddr()                = GetMeshLocal16();
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(aMeshSource);

    Get<Ip6::Icmp>().SendError(Ip6::IcmpHeader::kTypeDstUnreach, Ip6::IcmpHeader::kCodeDstUnreachNoRoute, messageInfo,
                               aIp6Header);

    error = OT_ERROR_DROP;

exit:
    return error;
}

otError MleRouter::SendAddressSolicit(ThreadStatusTlv::Status aStatus)
{
    otError                error = OT_ERROR_NONE;
    ThreadExtMacAddressTlv macAddr64Tlv;
    ThreadRloc16Tlv        rlocTlv;
    ThreadStatusTlv        statusTlv;
    Ip6::MessageInfo       messageInfo;
    Coap::Message *        message = NULL;

    VerifyOrExit(!mAddressSolicitPending);

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_ADDRESS_SOLICIT));
    SuccessOrExit(error = message->SetPayloadMarker());

    macAddr64Tlv.Init();
    macAddr64Tlv.SetMacAddr(Get<Mac::Mac>().GetExtAddress());
    SuccessOrExit(error = macAddr64Tlv.AppendTo(*message));

    if (IsRouterIdValid(mPreviousRouterId))
    {
        rlocTlv.Init();
        rlocTlv.SetRloc16(Rloc16FromRouterId(mPreviousRouterId));
        SuccessOrExit(error = rlocTlv.AppendTo(*message));
    }

    statusTlv.Init();
    statusTlv.SetStatus(aStatus);
    SuccessOrExit(error = statusTlv.AppendTo(*message));

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = AppendXtalAccuracy(*message));
#endif

    SuccessOrExit(error = GetLeaderAddress(messageInfo.GetPeerAddr()));
    messageInfo.SetSockAddr(GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(
        error = Get<Coap::Coap>().SendMessage(*message, messageInfo, &MleRouter::HandleAddressSolicitResponse, this));
    mAddressSolicitPending = true;

    LogMleMessage("Send Address Solicit", messageInfo.GetPeerAddr());

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendAddressRelease(void)
{
    otError                error = OT_ERROR_NONE;
    ThreadRloc16Tlv        rlocTlv;
    ThreadExtMacAddressTlv macAddr64Tlv;
    Ip6::MessageInfo       messageInfo;
    Coap::Message *        message;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_ADDRESS_RELEASE));
    SuccessOrExit(error = message->SetPayloadMarker());

    rlocTlv.Init();
    rlocTlv.SetRloc16(Rloc16FromRouterId(mRouterId));
    SuccessOrExit(error = rlocTlv.AppendTo(*message));

    macAddr64Tlv.Init();
    macAddr64Tlv.SetMacAddr(Get<Mac::Mac>().GetExtAddress());
    SuccessOrExit(error = macAddr64Tlv.AppendTo(*message));

    messageInfo.SetSockAddr(GetMeshLocal16());
    SuccessOrExit(error = GetLeaderAddress(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    LogMleMessage("Send Address Release", messageInfo.GetPeerAddr());

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void MleRouter::HandleAddressSolicitResponse(void *               aContext,
                                             otMessage *          aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             otError              aResult)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicitResponse(
        static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void MleRouter::HandleAddressSolicitResponse(Coap::Message *         aMessage,
                                             const Ip6::MessageInfo *aMessageInfo,
                                             otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    ThreadStatusTlv     statusTlv;
    ThreadRloc16Tlv     rlocTlv;
    ThreadRouterMaskTlv routerMaskTlv;
    uint8_t             routerId;
    Router *            router;
    Router *            leader;

    mAddressSolicitPending = false;

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage != NULL && aMessage != NULL);

    VerifyOrExit(aMessage->GetCode() == OT_COAP_CODE_CHANGED);

    LogMleMessage("Receive Address Reply", aMessageInfo->GetPeerAddr());

    SuccessOrExit(ThreadTlv::GetTlv(*aMessage, ThreadTlv::kStatus, sizeof(statusTlv), statusTlv));
    VerifyOrExit(statusTlv.IsValid());

    if (statusTlv.GetStatus() != statusTlv.kSuccess)
    {
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

    SuccessOrExit(ThreadTlv::GetTlv(*aMessage, ThreadTlv::kRloc16, sizeof(rlocTlv), rlocTlv));
    VerifyOrExit(rlocTlv.IsValid());
    routerId = RouterIdFromRloc16(rlocTlv.GetRloc16());

    SuccessOrExit(ThreadTlv::GetTlv(*aMessage, ThreadTlv::kRouterMask, sizeof(routerMaskTlv), routerMaskTlv));
    VerifyOrExit(routerMaskTlv.IsValid());

    // assign short address
    SetRouterId(routerId);

    SetStateRouter(Rloc16FromRouterId(mRouterId));
    mRouterTable.Clear();
    mRouterTable.ProcessTlv(routerMaskTlv);

    router = mRouterTable.GetRouter(routerId);
    VerifyOrExit(router != NULL);

    router->SetExtAddress(Get<Mac::Mac>().GetExtAddress());
    router->SetCost(0);

    router = mRouterTable.GetRouter(mParent.GetRouterId());
    VerifyOrExit(router != NULL);

    // Keep link to the parent in order to respond to Parent Requests before new link is established.
    *router = mParent;
    router->SetNextHop(kInvalidRouterId);
    router->SetCost(0);

    leader = mRouterTable.GetLeader();
    assert(leader != NULL);

    if (leader != router)
    {
        // Keep route path to the Leader reported by the parent before it is updated.
        leader->SetCost(mParentLeaderCost);
        leader->SetNextHop(RouterIdFromRloc16(mParent.GetRloc16()));
    }

    // send link request
    SendLinkRequest(NULL);

    // send child id responses
    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateChildIdRequest); !iter.IsDone(); iter++)
    {
        SendChildIdResponse(*iter.GetChild());
    }

exit:

    // send announce after received address solicit reply if needed
    InformPreviousChannel();
}

void MleRouter::HandleAddressSolicit(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicit(*static_cast<Coap::Message *>(aMessage),
                                                             *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void MleRouter::HandleAddressSolicit(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                error = OT_ERROR_NONE;
    ThreadExtMacAddressTlv macAddr64Tlv;
    ThreadRloc16Tlv        rlocTlv;
    ThreadStatusTlv        statusTlv;
    Router *               router = NULL;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    XtalAccuracyTlv xtalAccuracyTlv;
#endif

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST,
                 error = OT_ERROR_PARSE);

    LogMleMessage("Receive Address Solicit", aMessageInfo.GetPeerAddr());

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kExtMacAddress, sizeof(macAddr64Tlv), macAddr64Tlv));
    VerifyOrExit(macAddr64Tlv.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kStatus, sizeof(statusTlv), statusTlv));
    VerifyOrExit(statusTlv.IsValid(), error = OT_ERROR_PARSE);

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // In a time sync enabled network, all routers' xtal accuracy must be less than the threshold.
    if (Tlv::GetTlv(aMessage, Tlv::kXtalAccuracy, sizeof(xtalAccuracyTlv), xtalAccuracyTlv) != OT_ERROR_NONE ||
        xtalAccuracyTlv.GetXtalAccuracy() > Get<TimeSync>().GetXtalThreshold())
    {
        ExitNow(router = NULL);
    }
#endif

    // see if allocation already exists
    router = mRouterTable.GetRouter(macAddr64Tlv.GetMacAddr());

    if (router != NULL)
    {
        ExitNow();
    }

    // check the request reason
    switch (statusTlv.GetStatus())
    {
    case ThreadStatusTlv::kTooFewRouters:
        VerifyOrExit(mRouterTable.GetActiveRouterCount() < mRouterUpgradeThreshold);
        break;

    case ThreadStatusTlv::kHaveChildIdRequest:
    case ThreadStatusTlv::kParentPartitionChange:
        break;

    default:
        ExitNow(error = OT_ERROR_PARSE);
        break;
    }

    if (ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rlocTlv), rlocTlv) == OT_ERROR_NONE)
    {
        VerifyOrExit(rlocTlv.IsValid(), error = OT_ERROR_PARSE);
        router = mRouterTable.Allocate(RouterIdFromRloc16(rlocTlv.GetRloc16()));
    }

    // allocate new router id
    if (router == NULL)
    {
        router = mRouterTable.Allocate();
    }
    else
    {
        otLogInfoMle("router id requested and provided!");
    }

    if (router != NULL)
    {
        router->SetExtAddress(macAddr64Tlv.GetMacAddr());
    }
    else
    {
        otLogInfoMle("router address unavailable!");
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        SendAddressSolicitResponse(aMessage, router, aMessageInfo);
    }
}

void MleRouter::SendAddressSolicitResponse(const Coap::Message &   aRequest,
                                           const Router *          aRouter,
                                           const Ip6::MessageInfo &aMessageInfo)
{
    otError             error = OT_ERROR_NONE;
    ThreadStatusTlv     statusTlv;
    ThreadRouterMaskTlv routerMaskTlv;
    ThreadRloc16Tlv     rlocTlv;
    Coap::Message *     message;

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    statusTlv.Init();
    statusTlv.SetStatus(aRouter == NULL ? statusTlv.kNoAddressAvailable : statusTlv.kSuccess);
    SuccessOrExit(error = statusTlv.AppendTo(*message));

    if (aRouter != NULL)
    {
        rlocTlv.Init();
        rlocTlv.SetRloc16(aRouter->GetRloc16());
        SuccessOrExit(error = rlocTlv.AppendTo(*message));

        routerMaskTlv.Init();
        routerMaskTlv.SetIdSequence(mRouterTable.GetRouterIdSequence());
        routerMaskTlv.ClearAssignedRouterIdMask();

        for (uint8_t routerId = 0; routerId <= kMaxRouterId; routerId++)
        {
            if (mRouterTable.IsAllocated(routerId))
            {
                routerMaskTlv.SetAssignedRouterId(routerId);
            }
        }

        SuccessOrExit(error = routerMaskTlv.AppendTo(*message));
    }

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo));

    LogMleMessage("Send Address Reply", aMessageInfo.GetPeerAddr());

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void MleRouter::HandleAddressRelease(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<MleRouter *>(aContext)->HandleAddressRelease(*static_cast<Coap::Message *>(aMessage),
                                                             *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void MleRouter::HandleAddressRelease(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadRloc16Tlv        rlocTlv;
    ThreadExtMacAddressTlv macAddr64Tlv;
    uint8_t                routerId;
    Router *               router;

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST);

    LogMleMessage("Receive Address Release", aMessageInfo.GetPeerAddr());

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rlocTlv), rlocTlv));
    VerifyOrExit(rlocTlv.IsValid());

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kExtMacAddress, sizeof(macAddr64Tlv), macAddr64Tlv));
    VerifyOrExit(macAddr64Tlv.IsValid());

    routerId = RouterIdFromRloc16(rlocTlv.GetRloc16());
    router   = mRouterTable.GetRouter(routerId);

    VerifyOrExit(router != NULL && router->GetExtAddress() == macAddr64Tlv.GetMacAddr());

    mRouterTable.Release(routerId);

    SuccessOrExit(Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo));

    LogMleMessage("Send Address Release Reply", aMessageInfo.GetPeerAddr());

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
    cost   = (leader != NULL) ? leader->GetCost() : static_cast<uint8_t>(kMaxRouteCost);

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        cost = static_cast<uint8_t>(kMaxRouteCost);
        break;

    case OT_DEVICE_ROLE_CHILD:
        switch (mParent.GetLinkInfo().GetLinkQuality())
        {
        case 1:
            aTlv.SetLinkQuality1(aTlv.GetLinkQuality1() + 1);
            break;

        case 2:
            aTlv.SetLinkQuality2(aTlv.GetLinkQuality2() + 1);
            break;

        case 3:
            aTlv.SetLinkQuality3(aTlv.GetLinkQuality3() + 1);
            break;
        }

        cost += LinkQualityToCost(mParent.GetLinkInfo().GetLinkQuality());
        break;

    case OT_DEVICE_ROLE_ROUTER:
        if (leader != NULL)
        {
            cost += GetLinkCost(leader->GetNextHop());

            if (!IsRouterIdValid(leader->GetNextHop()) || GetLinkCost(GetLeaderId()) < cost)
            {
                cost = GetLinkCost(GetLeaderId());
            }
        }

        break;

    case OT_DEVICE_ROLE_LEADER:
        cost = 0;
        break;
    }

    aTlv.SetActiveRouters(mRouterTable.GetActiveRouterCount());

    for (RouterTable::Iterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        Router &router = *iter.GetRouter();
        uint8_t linkQuality;

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
        case 1:
            aTlv.SetLinkQuality1(aTlv.GetLinkQuality1() + 1);
            break;

        case 2:
            aTlv.SetLinkQuality2(aTlv.GetLinkQuality2() + 1);
            break;

        case 3:
            aTlv.SetLinkQuality3(aTlv.GetLinkQuality3() + 1);
            break;
        }
    }

    aTlv.SetLeaderCost((cost < kMaxRouteCost) ? cost : static_cast<uint8_t>(kMaxRouteCost));
    aTlv.SetIdSequence(mRouterTable.GetRouterIdSequence());
    aTlv.SetSedBufferSize(OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE);
    aTlv.SetSedDatagramCount(OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT);
}

otError MleRouter::AppendConnectivity(Message &aMessage)
{
    ConnectivityTlv tlv;

    tlv.Init();
    FillConnectivityTlv(tlv);

    return tlv.AppendTo(aMessage);
}

otError MleRouter::AppendChildAddresses(Message &aMessage, Child &aChild)
{
    otError                   error;
    Ip6::Address              address;
    Child::Ip6AddressIterator iterator;
    Tlv                       tlv;
    AddressRegistrationEntry  entry;
    Lowpan::Context           context;
    uint8_t                   length      = 0;
    uint16_t                  startOffset = aMessage.GetLength();

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    while (aChild.GetNextIp6Address(GetInstance(), iterator, address) == OT_ERROR_NONE)
    {
        if (address.IsMulticast() || Get<NetworkData::Leader>().GetContext(address, context) != OT_ERROR_NONE)
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

        SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
        length += entry.GetLength();
    }

    tlv.SetLength(length);
    aMessage.Write(startOffset, sizeof(tlv), &tlv);

exit:
    return error;
}

void MleRouter::FillRouteTlv(RouteTlv &aTlv)
{
    uint8_t routerCount = 0;

    aTlv.SetRouterIdSequence(mRouterTable.GetRouterIdSequence());
    aTlv.ClearRouterIdMask();

    for (RouterTable::Iterator iter(GetInstance()); !iter.IsDone(); iter++, routerCount++)
    {
        Router &router = *iter.GetRouter();

        aTlv.SetRouterId(router.GetRouterId());

        if (router.GetRloc16() == GetRloc16())
        {
            aTlv.SetLinkQualityIn(routerCount, 0);
            aTlv.SetLinkQualityOut(routerCount, 0);
            aTlv.SetRouteCost(routerCount, 1);
        }
        else
        {
            Router *nextHop;
            uint8_t linkCost;
            uint8_t routeCost;

            linkCost = mRouterTable.GetLinkCost(router);
            nextHop  = mRouterTable.GetRouter(router.GetNextHop());

            if (nextHop == NULL)
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
    }

    aTlv.SetRouteDataLength(routerCount);
}

otError MleRouter::AppendRoute(Message &aMessage)
{
    RouteTlv tlv;

    tlv.Init();
    FillRouteTlv(tlv);

    return tlv.AppendTo(aMessage);
}

otError MleRouter::AppendActiveDataset(Message &aMessage)
{
    return Get<MeshCoP::ActiveDataset>().AppendMleDatasetTlv(aMessage);
}

otError MleRouter::AppendPendingDataset(Message &aMessage)
{
    return Get<MeshCoP::PendingDataset>().AppendMleDatasetTlv(aMessage);
}

bool MleRouter::HasMinDowngradeNeighborRouters(void)
{
    uint8_t linkQuality;
    uint8_t routerCount = 0;

    for (RouterTable::Iterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        Router &router = *iter.GetRouter();

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
    bool rval = true;

    // process local neighbor routers
    for (RouterTable::Iterator iter(GetInstance()); !iter.IsDone(); iter++)
    {
        Router &router           = *iter.GetRouter();
        uint8_t localLinkQuality = 0;
        uint8_t peerLinkQuality  = 0;
        uint8_t routerCount      = 0;

        if (router.GetRouterId() == mRouterId)
        {
            routerCount++;
            continue;
        }

        // check if neighbor is valid
        if (router.IsStateValid())
        {
            // if neighbor is just peer
            if (router.GetRouterId() == aRouterId)
            {
                routerCount++;
                continue;
            }

            localLinkQuality = router.GetLinkInfo().GetLinkQuality();

            if (localLinkQuality > router.GetLinkQualityOut())
            {
                localLinkQuality = router.GetLinkQualityOut();
            }

            if (localLinkQuality >= 2)
            {
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
                else
                {
                    ExitNow(rval = false);
                }
            }
        }

        routerCount++;
    }

exit:
    return rval;
}

void MleRouter::SetChildStateToValid(Child &aChild)
{
    VerifyOrExit(!aChild.IsStateValid());

    aChild.SetState(Neighbor::kStateValid);
    StoreChild(aChild);
    Signal(OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED, aChild);

exit:
    return;
}

bool MleRouter::HasChildren(void)
{
    return mChildTable.HasChildren(Child::kInStateValidOrAttaching);
}

void MleRouter::RemoveChildren(void)
{
    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone(); iter++)
    {
        RemoveNeighbor(*iter.GetChild());
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

otError MleRouter::SetAssignParentPriority(int8_t aParentPriority)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aParentPriority <= kParentPriorityHigh && aParentPriority >= kParentPriorityUnspecified,
                 error = OT_ERROR_INVALID_ARGS);

    mParentPriority = aParentPriority;

exit:
    return error;
}

otError MleRouter::GetMaxChildTimeout(uint32_t &aTimeout) const
{
    otError error = OT_ERROR_NOT_FOUND;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValid); !iter.IsDone(); iter++)
    {
        Child &child = *iter.GetChild();

        if (child.IsFullThreadDevice())
        {
            continue;
        }

        if (child.GetTimeout() > aTimeout)
        {
            aTimeout = child.GetTimeout();
        }

        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

void MleRouter::Signal(otNeighborTableEvent aEvent, Neighbor &aNeighbor)
{
    if (mNeighborTableChangedCallback != NULL)
    {
        otNeighborTableEntryInfo info;
        otError                  error;

        info.mInstance = &GetInstance();

        switch (aEvent)
        {
        case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:
        case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED:
            error = GetChildInfo(static_cast<Child &>(aNeighbor), info.mInfo.mChild);
            assert(error == OT_ERROR_NONE);
            break;

        case OT_NEIGHBOR_TABLE_EVENT_ROUTER_ADDED:
        case OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED:
            GetNeighborInfo(aNeighbor, info.mInfo.mRouter);
            break;
        }

        mNeighborTableChangedCallback(aEvent, &info);
    }

    switch (aEvent)
    {
    case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:
        Get<Notifier>().Signal(OT_CHANGED_THREAD_CHILD_ADDED);
        break;

    case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED:
        Get<Notifier>().Signal(OT_CHANGED_THREAD_CHILD_REMOVED);
        break;

    default:
        break;
    }
}

bool MleRouter::HasSleepyChildrenSubscribed(const Ip6::Address &aAddress)
{
    bool rval = false;

    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone(); iter++)
    {
        Child &child = *iter.GetChild();

        if (child.IsRxOnWhenIdle())
        {
            continue;
        }

        if (IsSleepyChildSubscribed(aAddress, child))
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

bool MleRouter::IsSleepyChildSubscribed(const Ip6::Address &aAddress, Child &aChild)
{
    return aChild.IsStateValidOrRestoring() && !aChild.IsRxOnWhenIdle() &&
           aChild.HasIp6Address(GetInstance(), aAddress);
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
void MleRouter::HandleTimeSync(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, const Neighbor *aNeighbor)
{
    LogMleMessage("Receive Time Sync", aMessageInfo.GetPeerAddr());

    VerifyOrExit(aNeighbor && aNeighbor->IsStateValid());

    Get<TimeSync>().HandleTimeSyncMessage(aMessage);

exit:
    return;
}

otError MleRouter::SendTimeSync(void)
{
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message = NULL;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandTimeSync));

    message->SetTimeSync(true);

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);
    SuccessOrExit(error = SendMessage(*message, destination));

    LogMleMessage("Send Time Sync", destination);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

} // namespace Mle
} // namespace ot

#endif // OPENTHREAD_FTD
