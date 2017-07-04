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

#define WPP_NAME "mle_router.tmh"

#include <openthread/config.h>

#include "mle_router.hpp"

#include <openthread/platform/random.h>
#include <openthread/platform/settings.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"
#include "mac/mac_frame.hpp"
#include "net/icmp6.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Mle {

MleRouter::MleRouter(ThreadNetif &aThreadNetif):
    Mle(aThreadNetif),
    mAdvertiseTimer(aThreadNetif.GetIp6(), &MleRouter::HandleAdvertiseTimer, NULL, this),
    mStateUpdateTimer(aThreadNetif.GetIp6(), &MleRouter::HandleStateUpdateTimer, this),
    mAddressSolicit(OT_URI_PATH_ADDRESS_SOLICIT, &MleRouter::HandleAddressSolicit, this),
    mAddressRelease(OT_URI_PATH_ADDRESS_RELEASE, &MleRouter::HandleAddressRelease, this),
    mRouterIdSequence(0),
    mRouterIdSequenceLastUpdated(0),
    mMaxChildrenAllowed(kMaxChildren),
    mChallengeTimeout(0),
    mNextChildId(kMaxChildId),
    mNetworkIdTimeout(kNetworkIdTimeout),
    mRouterUpgradeThreshold(kRouterUpgradeThreshold),
    mRouterDowngradeThreshold(kRouterDowngradeThreshold),
    mLeaderWeight(kLeaderWeight),
    mFixedLeaderPartitionId(0),
    mRouterRoleEnabled(true),
    mIsRouterRestoringChildren(false),
    mPreviousPartitionId(0),
    mRouterSelectionJitter(kRouterSelectionJitter),
    mRouterSelectionJitterTimeout(0),
    mParentPriority(kParentPriorityUnspecified)
{
    mDeviceMode |= ModeTlv::kModeFFD | ModeTlv::kModeFullNetworkData;

    memset(mChildren, 0, sizeof(mChildren));
    memset(mRouters, 0, sizeof(mRouters));

    SetRouterId(kInvalidRouterId);
}

bool MleRouter::IsRouterRoleEnabled(void) const
{
    return mRouterRoleEnabled && (mDeviceMode & ModeTlv::kModeFFD);
}

void MleRouter::SetRouterRoleEnabled(bool aEnabled)
{
    mRouterRoleEnabled = aEnabled;

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        break;

    case OT_DEVICE_ROLE_CHILD:
        GetNetif().GetMac().SetBeaconEnabled(mRouterRoleEnabled);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if (!mRouterRoleEnabled)
        {
            BecomeDetached();
        }

        break;
    }
}

uint8_t MleRouter::AllocateRouterId(void)
{
    uint8_t rval = kInvalidRouterId;

    // count available router ids
    uint8_t numAvailable = 0;
    uint8_t numAllocated = 0;

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].IsAllocated())
        {
            numAllocated++;
        }
        else if (mRouters[i].IsReclaimDelay() == false)
        {
            numAvailable++;
        }
    }

    VerifyOrExit(numAllocated < kMaxRouters && numAvailable > 0, rval = kInvalidRouterId);

    // choose available router id at random
    uint8_t freeBit;
    freeBit = otPlatRandomGet() % numAvailable;

    // allocate router id
    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].IsAllocated() || mRouters[i].IsReclaimDelay())
        {
            continue;
        }

        if (freeBit == 0)
        {
            rval = AllocateRouterId(i);
            ExitNow();
        }

        freeBit--;
    }

exit:
    return rval;
}

uint8_t MleRouter::AllocateRouterId(uint8_t aRouterId)
{
    uint8_t rval = kInvalidRouterId;
    Router *router;

    router = GetRouter(aRouterId);
    assert(router != NULL);

    VerifyOrExit(!router->IsAllocated(), rval = kInvalidRouterId);

    // init router state
    router->SetAllocated(true);
    router->SetLastHeard(Timer::GetNow());
    router->ClearExtAddress();

    // bump sequence number
    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = Timer::GetNow();
    rval = aRouterId;

    otLogInfoMle(GetInstance(), "add router id %d", aRouterId);

exit:
    return rval;
}

otError MleRouter::ReleaseRouterId(uint8_t aRouterId)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Router *router = GetRouter(aRouterId);

    VerifyOrExit(router != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mRole == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);

    otLogInfoMle(GetInstance(), "delete router id %d", aRouterId);
    router->SetAllocated(false);
    router->SetReclaimDelay(true);
    router->SetState(Neighbor::kStateInvalid);
    router->SetNextHop(kInvalidRouterId);

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].GetNextHop() == aRouterId)
        {
            mRouters[i].SetNextHop(kInvalidRouterId);
            mRouters[i].SetCost(0);
        }
    }

    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = Timer::GetNow();
    netif.GetAddressResolver().Remove(aRouterId);
    netif.GetNetworkDataLeader().RemoveBorderRouter(GetRloc16(aRouterId));
    ResetAdvertiseInterval();

exit:
    return error;
}

uint32_t MleRouter::GetLeaderAge(void) const
{
    return Timer::MsecToSec(Timer::GetNow() - mRouterIdSequenceLastUpdated);
}

otError MleRouter::BecomeRouter(ThreadStatusTlv::Status aStatus)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mRole != OT_DEVICE_ROLE_ROUTER, error = OT_ERROR_NONE);
    VerifyOrExit(IsRouterRoleEnabled(), error = OT_ERROR_NOT_CAPABLE);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        mRouters[i].SetAllocated(false);
        mRouters[i].SetReclaimDelay(false);
        mRouters[i].SetState(Neighbor::kStateInvalid);
        mRouters[i].SetNextHop(kInvalidRouterId);
    }

    mAdvertiseTimer.Stop();
    netif.GetAddressResolver().Clear();
    netif.GetMeshForwarder().SetRxOnWhenIdle(true);
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
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    uint8_t routerId;
    Router *router;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mRole != OT_DEVICE_ROLE_LEADER, error = OT_ERROR_NONE);
    VerifyOrExit(IsRouterRoleEnabled(), error = OT_ERROR_NOT_CAPABLE);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        mRouters[i].SetAllocated(false);
        mRouters[i].SetReclaimDelay(false);
        mRouters[i].SetState(Neighbor::kStateInvalid);
        mRouters[i].SetNextHop(kInvalidRouterId);
    }

    routerId = IsRouterIdValid(mPreviousRouterId) ? AllocateRouterId(mPreviousRouterId) : AllocateRouterId();
    router = GetRouter(routerId);
    VerifyOrExit(router != NULL, error = OT_ERROR_NO_BUFS);

    SetRouterId(routerId);

    router->SetExtAddress(*netif.GetMac().GetExtAddress());
    mAdvertiseTimer.Stop();
    netif.GetAddressResolver().Clear();

    if (mFixedLeaderPartitionId != 0)
    {
        SetLeaderData(mFixedLeaderPartitionId, mLeaderWeight, mRouterId);
    }
    else
    {
        SetLeaderData(otPlatRandomGet(), mLeaderWeight, mRouterId);
    }

    mRouterIdSequence = static_cast<uint8_t>(otPlatRandomGet());

    netif.GetNetworkDataLeader().Reset();
    netif.GetLeader().SetEmptyCommissionerData();

    SuccessOrExit(error = SetStateLeader(GetRloc16(mRouterId)));

exit:
    return error;
}

void MleRouter::StopLeader(void)
{
    ThreadNetif &netif = GetNetif();

    netif.GetCoap().RemoveResource(mAddressSolicit);
    netif.GetCoap().RemoveResource(mAddressRelease);
    netif.GetActiveDataset().StopLeader();
    netif.GetPendingDataset().StopLeader();
    mAdvertiseTimer.Stop();
    netif.GetNetworkDataLeader().Stop();
    netif.UnsubscribeAllRoutersMulticast();
}

otError MleRouter::HandleDetachStart(void)
{
    otError error = OT_ERROR_NONE;

    GetNetif().GetAddressResolver().Clear();

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        mRouters[i].SetState(Neighbor::kStateInvalid);
    }

    StopLeader();
    mStateUpdateTimer.Stop();

    return error;
}

otError MleRouter::HandleChildStart(AttachMode aMode)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    mRouterIdSequenceLastUpdated = Timer::GetNow();
    mRouterSelectionJitterTimeout = (otPlatRandomGet() % mRouterSelectionJitter) + 1;

    StopLeader();
    mStateUpdateTimer.Start(kStateUpdatePeriod);

    if (mRouterRoleEnabled)
    {
        netif.GetMac().SetBeaconEnabled(true);
    }

    netif.SubscribeAllRoutersMulticast();

    VerifyOrExit(IsRouterIdValid(mPreviousRouterId), error = OT_ERROR_INVALID_STATE);

    switch (aMode)
    {
    case kAttachSame1:
    case kAttachSame2:

        // downgrade
        if (GetActiveRouterCount() > mRouterDowngradeThreshold)
        {
            SendAddressRelease();

            // reset children info if any
            if (HasChildren())
            {
                RemoveChildren();
            }

            // reset routerId info
            SetRouterId(kInvalidRouterId);
        }
        else if (HasChildren())
        {
            BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        }

        break;

    case kAttachAny:
    case kAttachBetter:
        if (HasChildren() &&
            mPreviousPartitionId != mLeaderData.GetPartitionId())
        {
            BecomeRouter(ThreadStatusTlv::kParentPartitionChange);
        }

        break;
    }

exit:

    if (GetActiveRouterCount() >= mRouterUpgradeThreshold &&
        (!IsRouterIdValid(mPreviousRouterId) || !HasChildren()))
    {
        SetRouterId(kInvalidRouterId);
    }

    return error;
}

otError MleRouter::SetStateRouter(uint16_t aRloc16)
{
    ThreadNetif &netif = GetNetif();

    if (mRole != OT_DEVICE_ROLE_ROUTER)
    {
        netif.SetStateChangedFlags(OT_CHANGED_THREAD_ROLE);
    }

    SetRloc16(aRloc16);
    mRole = OT_DEVICE_ROLE_ROUTER;
    mParentRequestState = kParentIdle;
    mParentRequestTimer.Stop();
    ResetAdvertiseInterval();

    netif.SubscribeAllRoutersMulticast();
    mRouters[mRouterId].SetNextHop(mRouterId);
    mPreviousPartitionId = mLeaderData.GetPartitionId();
    netif.GetNetworkDataLeader().Stop();
    mStateUpdateTimer.Start(kStateUpdatePeriod);
    netif.GetIp6().SetForwardingEnabled(true);
    netif.GetIp6().mMpl.SetTimerExpirations(kMplRouterDataMessageTimerExpirations);
    netif.GetMac().SetBeaconEnabled(true);

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() == Neighbor::kStateRestored)
        {
            mIsRouterRestoringChildren = true;
            break;
        }
    }

    otLogInfoMle(GetInstance(), "Mode -> Router");
    return OT_ERROR_NONE;
}

otError MleRouter::SetStateLeader(uint16_t aRloc16)
{
    ThreadNetif &netif = GetNetif();

    if (mRole != OT_DEVICE_ROLE_LEADER)
    {
        netif.SetStateChangedFlags(OT_CHANGED_THREAD_ROLE);
    }

    SetRloc16(aRloc16);
    mRole = OT_DEVICE_ROLE_LEADER;
    mParentRequestState = kParentIdle;
    mParentRequestTimer.Stop();
    ResetAdvertiseInterval();
    AddLeaderAloc();

    netif.SubscribeAllRoutersMulticast();
    mRouters[mRouterId].SetNextHop(mRouterId);
    mPreviousPartitionId = mLeaderData.GetPartitionId();
    mStateUpdateTimer.Start(kStateUpdatePeriod);
    mRouters[mRouterId].SetLastHeard(Timer::GetNow());

    netif.GetNetworkDataLeader().Start();
    netif.GetActiveDataset().StartLeader();
    netif.GetPendingDataset().StartLeader();
    netif.GetCoap().AddResource(mAddressSolicit);
    netif.GetCoap().AddResource(mAddressRelease);
    netif.GetIp6().SetForwardingEnabled(true);
    netif.GetIp6().mMpl.SetTimerExpirations(kMplRouterDataMessageTimerExpirations);
    netif.GetMac().SetBeaconEnabled(true);

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() == Neighbor::kStateRestored)
        {
            mIsRouterRestoringChildren = true;
            break;
        }
    }

    otLogInfoMle(GetInstance(), "Mode -> Leader %d", mLeaderData.GetPartitionId());
    return OT_ERROR_NONE;
}

bool MleRouter::HandleAdvertiseTimer(TrickleTimer &aTimer)
{
    return GetOwner(aTimer).HandleAdvertiseTimer();
}

bool MleRouter::HandleAdvertiseTimer(void)
{
    if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
    {
        return false;
    }

    SendAdvertisement();

    return true;
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
        mAdvertiseTimer.Start(
            Timer::SecToMsec(kAdvertiseIntervalMin),
            Timer::SecToMsec(kAdvertiseIntervalMax),
            TrickleTimer::kModeNormal);
    }

    mAdvertiseTimer.IndicateInconsistent();

exit:
    return;
}

otError MleRouter::SendAdvertisement(void)
{
    otError error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *message;

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

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle(GetInstance(), "Sent advertisement");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendLinkRequest(Neighbor *aNeighbor)
{
    static const uint8_t detachedTlvs[] = {Tlv::kAddress16, Tlv::kRoute};
    static const uint8_t routerTlvs[] = {Tlv::kLinkMargin};
    otError error = OT_ERROR_NONE;
    Message *message;
    Ip6::Address destination;

    memset(&destination, 0, sizeof(destination));

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
        SuccessOrExit(error = AppendTlvRequest(*message, routerTlvs, sizeof(routerTlvs)));
        SuccessOrExit(error = AppendSourceAddress(*message));
        SuccessOrExit(error = AppendLeaderData(*message));
        break;
    }

    if (aNeighbor == NULL)
    {
        for (uint8_t i = 0; i < sizeof(mChallenge); i++)
        {
            mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
        }

        mChallengeTimeout = (((2 * kMaxResponseDelay) + kStateUpdatePeriod - 1) / kStateUpdatePeriod);

        SuccessOrExit(error = AppendChallenge(*message, mChallenge, sizeof(mChallenge)));
        destination.mFields.m8[0] = 0xff;
        destination.mFields.m8[1] = 0x02;
        destination.mFields.m8[15] = 2;
    }
    else
    {
        aNeighbor->GenerateChallenge();

        SuccessOrExit(error = AppendChallenge(*message, aNeighbor->GetChallenge(), aNeighbor->GetChallengeSize()));
        destination.mFields.m16[0] = HostSwap16(0xfe80);
        destination.SetIid(aNeighbor->GetExtAddress());
    }

    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle(GetInstance(), "Sent link request");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::HandleLinkRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Neighbor *neighbor = NULL;
    Mac::ExtAddress macAddr;
    ChallengeTlv challenge;
    VersionTlv version;
    LeaderDataTlv leaderData;
    SourceAddressTlv sourceAddress;
    TlvRequestTlv tlvRequest;
    uint16_t rloc16;

    otLogInfoMle(GetInstance(), "Received link request");

    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(mParentRequestState == kParentIdle, error = OT_ERROR_INVALID_STATE);

    macAddr.Set(aMessageInfo.GetPeerAddr());

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid() && version.GetVersion() >= kVersion, error = OT_ERROR_PARSE);

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == OT_ERROR_NONE)
    {
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), error = OT_ERROR_INVALID_STATE);
    }

    // Source Address
    if (Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress) == OT_ERROR_NONE)
    {
        VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

        rloc16 = sourceAddress.GetRloc16();

        if ((neighbor = GetNeighbor(macAddr)) != NULL && neighbor->GetRloc16() != rloc16)
        {
            // remove stale neighbors
            RemoveNeighbor(*neighbor);
            neighbor = NULL;
        }

        if (IsActiveRouter(rloc16))
        {
            // source is a router
            neighbor = GetRouter(GetRouterId(rloc16));
            VerifyOrExit(neighbor != NULL, error = OT_ERROR_PARSE);

            if (neighbor->GetState() != Neighbor::kStateValid)
            {
                const ThreadMessageInfo *threadMessageInfo =
                    static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());

                neighbor->SetExtAddress(macAddr);
                neighbor->GetLinkInfo().Clear();
                neighbor->GetLinkInfo().AddRss(GetNetif().GetMac().GetNoiseFloor(), threadMessageInfo->mRss);
                neighbor->ResetLinkFailures();
                neighbor->SetState(Neighbor::kStateLinkRequest);
            }
            else
            {
                VerifyOrExit(memcmp(&neighbor->GetExtAddress(), &macAddr, sizeof(macAddr)) == 0);
            }
        }
        else
        {
            // source is not a router
            neighbor = NULL;
        }
    }
    else
    {
        // lack of source address indicates router coming out of reset
        VerifyOrExit((neighbor = GetNeighbor(macAddr)) != NULL &&
                     neighbor->GetState() == Neighbor::kStateValid &&
                     IsActiveRouter(neighbor->GetRloc16()),
                     error = OT_ERROR_DROP);
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

    SuccessOrExit(error = SendLinkAccept(aMessageInfo, neighbor, tlvRequest, challenge));

exit:
    return error;
}

otError MleRouter::SendLinkAccept(const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor,
                                  const TlvRequestTlv &aTlvRequest, const ChallengeTlv &aChallenge)
{
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    static const uint8_t routerTlvs[] = {Tlv::kLinkMargin};
    Message *message;
    Header::Command command;
    uint8_t linkMargin;

    command = (aNeighbor == NULL || aNeighbor->GetState() == Neighbor::kStateValid) ?
              Header::kCommandLinkAccept : Header::kCommandLinkAcceptAndRequest;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, command));
    SuccessOrExit(error = AppendVersion(*message));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendResponse(*message, aChallenge.GetChallenge(), aChallenge.GetLength()));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));

    // always append a link margin, regardless of whether or not it was requested
    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(GetNetif().GetMac().GetNoiseFloor(), threadMessageInfo->mRss);

    // add for certification testing
    if (isAssignLinkQuality && aNeighbor != NULL &&
        (memcmp(&aNeighbor->GetExtAddress(), mAddr64.m8, OT_EXT_ADDRESS_SIZE) == 0))
    {
        linkMargin = mAssignLinkMargin;
    }

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

    if (aNeighbor != NULL && aNeighbor->GetState() != Neighbor::kStateValid)
    {
        aNeighbor->GenerateChallenge();

        SuccessOrExit(error = AppendChallenge(*message, aNeighbor->GetChallenge(), aNeighbor->GetChallengeSize()));
        SuccessOrExit(error = AppendTlvRequest(*message, routerTlvs, sizeof(routerTlvs)));
        aNeighbor->SetState(Neighbor::kStateLinkRequest);
    }

    if (aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(error = AddDelayedResponse(*message, aMessageInfo.GetPeerAddr(),
                                                 (otPlatRandomGet() % kMaxResponseDelay) + 1));

        otLogInfoMle(GetInstance(), "Delayed link accept");
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aMessageInfo.GetPeerAddr()));

        otLogInfoMle(GetInstance(), "Sent link accept");
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::HandleLinkAccept(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                    uint32_t aKeySequence)
{
    otLogInfoMle(GetInstance(), "Received link accept");
    return HandleLinkAccept(aMessage, aMessageInfo, aKeySequence, false);
}

otError MleRouter::HandleLinkAcceptAndRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                              uint32_t aKeySequence)
{
    otLogInfoMle(GetInstance(), "Received link accept and request");
    return HandleLinkAccept(aMessage, aMessageInfo, aKeySequence, true);
}

otError MleRouter::HandleLinkAccept(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                    uint32_t aKeySequence, bool aRequest)
{
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    Router *router;
    Neighbor *neighbor;
    Mac::ExtAddress macAddr;
    VersionTlv version;
    ResponseTlv response;
    SourceAddressTlv sourceAddress;
    LinkFrameCounterTlv linkFrameCounter;
    MleFrameCounterTlv mleFrameCounter;
    uint8_t routerId;
    Address16Tlv address16;
    RouteTlv route;
    LeaderDataTlv leaderData;
    LinkMarginTlv linkMargin;
    ChallengeTlv challenge;
    TlvRequestTlv tlvRequest;

    macAddr.Set(aMessageInfo.GetPeerAddr());

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid(), error = OT_ERROR_PARSE);

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid(), error = OT_ERROR_PARSE);

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    // Remove stale neighbors
    if ((neighbor = GetNeighbor(macAddr)) != NULL &&
        neighbor->GetRloc16() != sourceAddress.GetRloc16())
    {
        RemoveNeighbor(*neighbor);
    }

    // Link-Layer Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter),
                                      linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) ==
        OT_ERROR_NONE)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), error = OT_ERROR_PARSE);
    }
    else
    {
        mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    VerifyOrExit(IsActiveRouter(sourceAddress.GetRloc16()), error = OT_ERROR_PARSE);

    routerId = GetRouterId(sourceAddress.GetRloc16());
    router = GetRouter(routerId);

    VerifyOrExit(router != NULL, error = OT_ERROR_PARSE);

    // verify response
    switch (router->GetState())
    {
    case Neighbor::kStateLinkRequest:
        VerifyOrExit(memcmp(router->GetChallenge(), response.GetResponse(), router->GetChallengeSize()) == 0,
                     error = OT_ERROR_SECURITY);
        break;

    case Neighbor::kStateInvalid:
    case Neighbor::kStateValid:
        VerifyOrExit((mChallengeTimeout > 0) && (memcmp(mChallenge, response.GetResponse(), sizeof(mChallenge)) == 0),
                     error = OT_ERROR_SECURITY);
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_STATE);
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

        // Route
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route));
        VerifyOrExit(route.IsValid(), error = OT_ERROR_PARSE);
        SuccessOrExit(error = ProcessRouteTlv(route));

        // Leader Data
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
        SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

        if (mLeaderData.GetLeaderRouterId() == GetRouterId(GetRloc16()))
        {
            SetStateLeader(GetRloc16());
        }
        else
        {
            static const uint8_t tlvs[] = {Tlv::kNetworkData};
            SetStateRouter(GetRloc16());
            mRetrieveNewNetworkData = true;
            SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), 0);
        }

        break;

    case OT_DEVICE_ROLE_CHILD:
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMargin), linkMargin));
        VerifyOrExit(linkMargin.IsValid(), error = OT_ERROR_PARSE);
        router->SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin.GetLinkMargin()));
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        // Leader Data
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());

        // Link Margin
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMargin), linkMargin));
        VerifyOrExit(linkMargin.IsValid(), error = OT_ERROR_PARSE);
        router->SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin.GetLinkMargin()));

        // update routing table
        if (routerId != mRouterId && !IsRouterIdValid(router->GetNextHop()))
        {
            ResetAdvertiseInterval();
        }

        break;
    }

    // finish link synchronization
    router->SetExtAddress(macAddr);
    router->SetRloc16(sourceAddress.GetRloc16());
    router->SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    router->SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    router->SetLastHeard(Timer::GetNow());
    router->SetDeviceMode(ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData);
    router->GetLinkInfo().Clear();
    router->GetLinkInfo().AddRss(GetNetif().GetMac().GetNoiseFloor(), threadMessageInfo->mRss);
    router->ResetLinkFailures();
    router->SetState(Neighbor::kStateValid);
    router->SetKeySequence(aKeySequence);

    if (aRequest)
    {
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

Child *MleRouter::NewChild(void)
{
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() == Neighbor::kStateInvalid)
        {
            return &mChildren[i];
        }
    }

    return NULL;
}

Child *MleRouter::FindChild(uint16_t aChildId)
{
    Child *rval = NULL;

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() != Neighbor::kStateInvalid &&
            GetChildId(mChildren[i].GetRloc16()) == aChildId)
        {
            ExitNow(rval = &mChildren[i]);
        }
    }

exit:
    return rval;
}

Child *MleRouter::FindChild(const Mac::ExtAddress &aAddress)
{
    Child *rval = NULL;

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() != Neighbor::kStateInvalid &&
            memcmp(&mChildren[i].GetExtAddress(), &aAddress, sizeof(mChildren[i].GetExtAddress())) == 0)
        {
            ExitNow(rval = &mChildren[i]);
        }
    }

exit:
    return rval;
}

uint8_t MleRouter::LinkQualityToCost(uint8_t aLinkQuality)
{
    switch (aLinkQuality)
    {
    case 1:
        return kLinkQuality1LinkCost;

    case 2:
        return kLinkQuality2LinkCost;

    case 3:
        return kLinkQuality3LinkCost;

    default:
        return kLinkQuality0LinkCost;
    }
}

uint8_t MleRouter::GetLinkCost(uint8_t aRouterId)
{
    uint8_t rval = kMaxRouteCost;
    Router *router;

    router = GetRouter(aRouterId);

    // NULL aRouterId indicates non-existing next hop, hence return kMaxRouteCost for it.
    VerifyOrExit(aRouterId != mRouterId && router != NULL && router->GetState() == Neighbor::kStateValid);

    rval = router->GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor());

    if (rval > router->GetLinkQualityOut())
    {
        rval = router->GetLinkQualityOut();
    }

    // add for certification testing
    if (isAssignLinkQuality && (memcmp(&router->GetExtAddress(), mAddr64.m8, OT_EXT_ADDRESS_SIZE) == 0))
    {
        rval = mAssignLinkQuality;
    }

    rval = LinkQualityToCost(rval);

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

    mRouterIdSequence = aRoute.GetRouterIdSequence();
    mRouterIdSequenceLastUpdated = Timer::GetNow();

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        bool old = mRouters[i].IsAllocated();
        mRouters[i].SetAllocated(aRoute.IsRouterIdSet(i));

        if (old && !mRouters[i].IsAllocated())
        {
            mRouters[i].SetNextHop(kInvalidRouterId);
            GetNetif().GetAddressResolver().Remove(i);
        }
    }

    if (mRole == OT_DEVICE_ROLE_ROUTER && !mRouters[mRouterId].IsAllocated())
    {
        BecomeDetached();
        ExitNow(error = OT_ERROR_NO_ROUTE);
    }

exit:
    return error;
}

bool MleRouter::IsSingleton(void)
{
    bool rval = true;

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        ExitNow(rval = true);
        break;

    case OT_DEVICE_ROLE_CHILD:
        ExitNow(rval = ((mDeviceMode & ModeTlv::kModeFFD) == 0));
        break;

    case OT_DEVICE_ROLE_ROUTER:
        ExitNow(rval = false);
        break;

    case OT_DEVICE_ROLE_LEADER:

        // not a singleton if any other routers exist
        for (int i = 0; i <= kMaxRouterId; i++)
        {
            if (i != mRouterId && mRouters[i].IsAllocated())
            {
                ExitNow(rval = false);
            }
        }

        // not a singleton if any children are REEDs
        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].GetState() == Neighbor::kStateValid && mChildren[i].IsFullThreadDevice())
            {
                ExitNow(rval = false);
            }
        }

        break;
    }

exit:
    return rval;
}

int MleRouter::ComparePartitions(bool aSingletonA, const LeaderDataTlv &aLeaderDataA,
                                 bool aSingletonB, const LeaderDataTlv &aLeaderDataB)
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

uint8_t MleRouter::GetActiveRouterCount(void) const
{
    uint8_t rval = 0;

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].IsAllocated())
        {
            rval++;
        }
    }

    return rval;
}

otError MleRouter::HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    Mac::ExtAddress macAddr;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv leaderData;
    RouteTlv route;
    uint32_t partitionId;
    Router *router;
    Neighbor *neighbor;
    uint8_t routerId;
    uint8_t routerCount;

    macAddr.Set(aMessageInfo.GetPeerAddr());

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    // Remove stale neighbors
    if ((neighbor = GetNeighbor(macAddr)) != NULL &&
        neighbor->GetRloc16() != sourceAddress.GetRloc16())
    {
        RemoveNeighbor(*neighbor);
    }

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    // Route Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route));
    VerifyOrExit(route.IsValid(), error = OT_ERROR_PARSE);

    partitionId = leaderData.GetPartitionId();

    if (partitionId != mLeaderData.GetPartitionId())
    {
        otLogDebgMle(GetInstance(), "different partition! %d %d %d %d",
                     leaderData.GetWeighting(), partitionId,
                     mLeaderData.GetWeighting(), mLeaderData.GetPartitionId());

        if (partitionId == mLastPartitionId && (mDeviceMode & ModeTlv::kModeFFD))
        {
            VerifyOrExit((static_cast<int8_t>(route.GetRouterIdSequence() - mLastPartitionRouterIdSequence) > 0),
                         error = OT_ERROR_DROP);
        }

        if (mRole == OT_DEVICE_ROLE_CHILD &&
            (memcmp(&mParent.GetExtAddress(), &macAddr, sizeof(macAddr)) == 0 || !(mDeviceMode & ModeTlv::kModeFFD)))
        {
            ExitNow();
        }

        routerCount = 0;

        for (uint8_t i = 0; i <= kMaxRouterId; i++)
        {
            if (route.IsRouterIdSet(i))
            {
                routerCount++;
            }
        }

        if (ComparePartitions(routerCount <= 1, leaderData, IsSingleton(), mLeaderData) > 0)
        {
            otLogDebgMle(GetInstance(), "trying to migrate");
            BecomeChild(kAttachBetter);
        }

        ExitNow(error = OT_ERROR_DROP);
    }
    else if (leaderData.GetLeaderRouterId() != GetLeaderId())
    {
        if (mRole != OT_DEVICE_ROLE_CHILD)
        {
            BecomeDetached();
            error = OT_ERROR_DROP;
        }

        ExitNow();
    }

    VerifyOrExit(IsActiveRouter(sourceAddress.GetRloc16()));
    routerId = GetRouterId(sourceAddress.GetRloc16());
    router = GetRouter(routerId);
    VerifyOrExit(router != NULL, error = OT_ERROR_PARSE);

    if ((mDeviceMode & ModeTlv::kModeFFD) &&
        static_cast<int8_t>(route.GetRouterIdSequence() - mRouterIdSequence) > 0)
    {
        bool processRouteTlv = false;

        switch (mRole)
        {
        case OT_DEVICE_ROLE_DISABLED:
        case OT_DEVICE_ROLE_DETACHED:
            break;

        case OT_DEVICE_ROLE_CHILD:
            if ((sourceAddress.GetRloc16() == mParent.GetRloc16()) ||
                (router->GetState() == Neighbor::kStateValid))
            {
                processRouteTlv = true;
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
        if ((sourceAddress.GetRloc16() == mParent.GetRloc16() || router->GetState() == Neighbor::kStateValid) &&
            (mDeviceMode & ModeTlv::kModeFFD) &&
            (mRouterSelectionJitterTimeout == 0) &&
            (GetActiveRouterCount() < mRouterUpgradeThreshold))
        {
            mRouterSelectionJitterTimeout = (otPlatRandomGet() % mRouterSelectionJitter) + 1;
            ExitNow();
        }

        if (memcmp(&mParent.GetExtAddress(), &macAddr, sizeof(mParent.GetExtAddress())) == 0)
        {
            router = &mParent;

            if (mParent.GetRloc16() != sourceAddress.GetRloc16())
            {
                BecomeDetached();
                ExitNow(error = OT_ERROR_NO_ROUTE);
            }

            if (mDeviceMode & ModeTlv::kModeFFD)
            {
                for (uint8_t i = 0, routeCount = 0; i <= kMaxRouterId; i++)
                {
                    if (route.IsRouterIdSet(i) == false)
                    {
                        continue;
                    }

                    if (i != GetLeaderId())
                    {
                        routeCount++;
                        continue;
                    }

                    if (route.GetRouteCost(routeCount) > 0)
                    {
                        mRouters[GetLeaderId()].SetNextHop(routerId);
                        mRouters[GetLeaderId()].SetCost(route.GetRouteCost(routeCount));
                    }
                    else
                    {
                        mRouters[GetLeaderId()].SetNextHop(kInvalidRouterId);
                        mRouters[GetLeaderId()].SetCost(0);
                    }

                    break;
                }
            }
        }
        else if ((mDeviceMode & ModeTlv::kModeFFD) && (router->GetState() != Neighbor::kStateValid))
        {
            router->SetExtAddress(macAddr);
            router->GetLinkInfo().Clear();
            router->GetLinkInfo().AddRss(netif.GetMac().GetNoiseFloor(), threadMessageInfo->mRss);
            router->ResetLinkFailures();
            router->SetState(Neighbor::kStateLinkRequest);
            SendLinkRequest(router);
            ExitNow(error = OT_ERROR_NO_ROUTE);
        }

        router->SetLastHeard(Timer::GetNow());

        ExitNow();

    case OT_DEVICE_ROLE_ROUTER:
        // check current active router number
        routerCount = 0;

        for (uint8_t i = 0; i <= kMaxRouterId; i++)
        {
            if (route.IsRouterIdSet(i))
            {
                routerCount++;
            }
        }

        if (routerCount > mRouterDowngradeThreshold &&
            mRouterSelectionJitterTimeout == 0 &&
            HasMinDowngradeNeighborRouters() &&
            HasSmallNumberOfChildren() &&
            HasOneNeighborwithComparableConnectivity(route, routerId))
        {
            mRouterSelectionJitterTimeout = (otPlatRandomGet() % mRouterSelectionJitter) + 1;
        }

    // fall through

    case OT_DEVICE_ROLE_LEADER:

        // router is not in list, reject
        if (!router->IsAllocated())
        {
            ExitNow(error = OT_ERROR_NO_ROUTE);
        }

        // Send link request if no link to router
        if (router->GetState() != Neighbor::kStateValid)
        {
            router->SetExtAddress(macAddr);
            router->GetLinkInfo().Clear();
            router->GetLinkInfo().AddRss(netif.GetMac().GetNoiseFloor(), threadMessageInfo->mRss);
            router->ResetLinkFailures();
            router->SetState(Neighbor::kStateLinkRequest);
            router->SetDataRequestPending(false);
            SendLinkRequest(router);
            ExitNow(error = OT_ERROR_NO_ROUTE);
        }

        router->SetLastHeard(Timer::GetNow());
        break;
    }

    UpdateRoutes(route, routerId);

#if OPENTHREAD_ENABLE_BORDER_ROUTER
    netif.GetNetworkDataLocal().SendServerDataNotification();
#endif

exit:
    return error;
}

void MleRouter::UpdateRoutes(const RouteTlv &aRoute, uint8_t aRouterId)
{
    uint8_t curCost;
    uint8_t newCost;
    uint8_t oldNextHop;
    uint8_t cost;
    uint8_t linkQuality;
    bool update;

    // update routes
    do
    {
        update = false;

        for (uint8_t i = 0, routeCount = 0; i <= kMaxRouterId; i++)
        {
            if (aRoute.IsRouterIdSet(i) == false)
            {
                continue;
            }

            if (mRouters[i].IsAllocated() == false)
            {
                routeCount++;
                continue;
            }

            if (i == mRouterId)
            {
                linkQuality = aRoute.GetLinkQualityIn(routeCount);

                if (mRouters[aRouterId].GetLinkQualityOut() != linkQuality)
                {
                    mRouters[aRouterId].SetLinkQualityOut(linkQuality);
                    update = true;
                }
            }
            else
            {
                oldNextHop = mRouters[i].GetNextHop();

                if (i == aRouterId)
                {
                    cost = 0;
                }
                else
                {
                    cost = aRoute.GetRouteCost(routeCount);

                    if (cost == 0)
                    {
                        cost = kMaxRouteCost;
                    }
                }

                if (!IsRouterIdValid(mRouters[i].GetNextHop()) || mRouters[i].GetNextHop() == aRouterId)
                {
                    // route has no nexthop or nexthop is neighbor (sender)

                    if (i != aRouterId)
                    {
                        if (cost + GetLinkCost(aRouterId) <= kMaxRouteCost)
                        {
                            if (!IsRouterIdValid(mRouters[i].GetNextHop()) && GetLinkCost(i) >= kMaxRouteCost)
                            {
                                ResetAdvertiseInterval();
                            }

                            mRouters[i].SetNextHop(aRouterId);
                            mRouters[i].SetCost(cost);
                        }
                        else if (mRouters[i].GetNextHop() == aRouterId)
                        {
                            if (GetLinkCost(i) >= kMaxRouteCost)
                            {
                                ResetAdvertiseInterval();
                            }

                            mRouters[i].SetNextHop(kInvalidRouterId);
                            mRouters[i].SetCost(0);
                            mRouters[i].SetLastHeard(Timer::GetNow());
                        }
                    }
                }
                else
                {
                    curCost = mRouters[i].GetCost() + GetLinkCost(mRouters[i].GetNextHop());
                    newCost = cost + GetLinkCost(aRouterId);

                    if (newCost < curCost && i != aRouterId)
                    {
                        mRouters[i].SetNextHop(aRouterId);
                        mRouters[i].SetCost(cost);
                    }
                }

                update |= mRouters[i].GetNextHop() != oldNextHop;
            }

            routeCount++;
        }
    }
    while (update);

#if 1

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].IsAllocated() == false || !IsRouterIdValid(mRouters[i].GetNextHop()))
        {
            continue;
        }

        otLogDebgMle(GetInstance(),
                     "%x: %x %d %d %d %d",
                     GetRloc16(i),
                     GetRloc16(mRouters[i].GetNextHop()),
                     mRouters[i].GetCost(),
                     GetLinkCost(i), mRouters[i].GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor()),
                     mRouters[i].GetLinkQualityOut());
    }

#endif
}

otError MleRouter::HandleParentRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    Mac::ExtAddress macAddr;
    VersionTlv version;
    ScanMaskTlv scanMask;
    ChallengeTlv challenge;
    Child *child;

    otLogInfoMle(GetInstance(), "Received parent request");

    // A Router MUST NOT send an MLE Parent Response if:

    // 1. It has no available Child capacity (if Max Child Count minus
    // Child Count would be equal to zero)
    // ==> verified below when allocating a child entry

    // 2. It is disconnected from its Partition (that is, it has not
    // received an updated ID sequence number within LEADER_TIMEOUT
    // seconds
    VerifyOrExit(GetLeaderAge() < mNetworkIdTimeout, error = OT_ERROR_DROP);

    // 3. Its current routing path cost to the Leader is infinite.
    VerifyOrExit(mRole == OT_DEVICE_ROLE_LEADER ||
                 GetLinkCost(GetLeaderId()) < kMaxRouteCost ||
                 (mRole == OT_DEVICE_ROLE_CHILD && mRouters[GetLeaderId()].GetCost() + 1 < kMaxRouteCost) ||
                 (mRouters[GetLeaderId()].GetCost() + GetLinkCost(mRouters[GetLeaderId()].GetNextHop()) < kMaxRouteCost),
                 error = OT_ERROR_DROP);

    macAddr.Set(aMessageInfo.GetPeerAddr());

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid() && version.GetVersion() >= kVersion, error = OT_ERROR_PARSE);

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

    child = FindChild(macAddr);

    if (child != NULL && !child->IsFullThreadDevice())
    {
        // Parent Request from a MTD child means that the child had detached. It can be safely removed.
        RemoveNeighbor(*child);
        child = NULL;
    }

    if (child == NULL)
    {
        VerifyOrExit((child = NewChild()) != NULL);

        memset(child, 0, sizeof(*child));

        // MAC Address
        child->SetExtAddress(macAddr);
        child->GetLinkInfo().Clear();
        child->GetLinkInfo().AddRss(GetNetif().GetMac().GetNoiseFloor(), threadMessageInfo->mRss);
        child->ResetLinkFailures();
        child->SetState(Neighbor::kStateParentRequest);
        child->SetDataRequestPending(false);

        child->SetLastHeard(Timer::GetNow());
        child->SetTimeout(Timer::MsecToSec(kMaxChildIdRequestTimeout));
    }

    SuccessOrExit(error = SendParentResponse(child, challenge, !scanMask.IsEndDeviceFlagSet()));

exit:
    return error;
}

void MleRouter::HandleStateUpdateTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleStateUpdateTimer();
}

void MleRouter::HandleStateUpdateTimer(void)
{
    bool routerStateUpdate = false;

    if (mChallengeTimeout > 0)
    {
        mChallengeTimeout--;
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
        BecomeDetached();
        ExitNow();

    case OT_DEVICE_ROLE_CHILD:
        if (routerStateUpdate)
        {
            if (GetActiveRouterCount() < mRouterUpgradeThreshold)
            {
                // upgrade to Router
                BecomeRouter(ThreadStatusTlv::kTooFewRouters);
            }
            else if (!mAdvertiseTimer.IsRunning())
            {
                SendAdvertisement();

                mAdvertiseTimer.Start(
                    Timer::SecToMsec(kReedAdvertiseInterval),
                    Timer::SecToMsec(kReedAdvertiseInterval + kReedAdvertiseJitter),
                    TrickleTimer::kModePlainTimer);
            }

            ExitNow();
        }

    // fall through

    case OT_DEVICE_ROLE_ROUTER:
        // verify path to leader
        otLogDebgMle(GetInstance(), "network id timeout = %d", GetLeaderAge());

        if (GetLeaderAge() >= mNetworkIdTimeout)
        {
            BecomeChild(kAttachSame1);
        }

        if (routerStateUpdate && GetActiveRouterCount() > mRouterDowngradeThreshold)
        {
            // downgrade to REED
            BecomeChild(kAttachSame1);
        }

        break;

    case OT_DEVICE_ROLE_LEADER:

        // update router id sequence
        if (GetLeaderAge() >= kRouterIdSequencePeriod)
        {
            mRouterIdSequence++;
            mRouterIdSequenceLastUpdated = Timer::GetNow();
        }

        break;
    }

    if (mIsRouterRestoringChildren)
    {
        bool hasRestoringChildren = false;

        for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].GetState() == Neighbor::kStateRestored)
            {
                SendChildUpdateRequest(&mChildren[i]);
                hasRestoringChildren = true;
            }
        }

        // no child to restore
        if (!hasRestoringChildren)
        {
            mIsRouterRestoringChildren = false;
        }
    }

    // update children state
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        uint32_t timeout = 0;

        switch (mChildren[i].GetState())
        {
        case Neighbor::kStateInvalid:
        case Neighbor::kStateChildIdRequest:
            continue;

        case Neighbor::kStateParentRequest:
        case Neighbor::kStateValid:
        case Neighbor::kStateRestored:
        case Neighbor::kStateChildUpdateRequest:
            timeout = Timer::SecToMsec(mChildren[i].GetTimeout());
            break;

        case Neighbor::kStateLinkRequest:
            assert(false);
            break;
        }

        if ((Timer::GetNow() - mChildren[i].GetLastHeard()) >= timeout)
        {
            RemoveNeighbor(mChildren[i]);
        }
    }

    // update router state
    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].GetState() == Neighbor::kStateValid)
        {
            if ((Timer::GetNow() - mRouters[i].GetLastHeard()) >= Timer::SecToMsec(kMaxNeighborAge))
            {
                RemoveNeighbor(mRouters[i]);
            }
        }

        if (mRole == OT_DEVICE_ROLE_LEADER)
        {
            if (mRouters[i].IsAllocated())
            {
                if (!IsRouterIdValid(mRouters[i].GetNextHop()) &&
                    GetLinkCost(i) >= kMaxRouteCost &&
                    (Timer::GetNow() - mRouters[i].GetLastHeard()) >= Timer::SecToMsec(kMaxLeaderToRouterTimeout))
                {
                    ReleaseRouterId(i);
                }
            }
            else if (mRouters[i].IsReclaimDelay())
            {
                if ((Timer::GetNow() - mRouters[i].GetLastHeard()) >=
                    Timer::SecToMsec((kMaxLeaderToRouterTimeout + kRouterIdReuseDelay)))
                {
                    mRouters[i].SetReclaimDelay(false);
                }
            }
        }
    }

    mStateUpdateTimer.Start(kStateUpdatePeriod);

exit:
    return;
}

otError MleRouter::SendParentResponse(Child *aChild, const ChallengeTlv &challenge, bool aRoutersOnlyRequest)
{
    otError error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *message;
    uint16_t delay;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandParentResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));
    SuccessOrExit(error = AppendResponse(*message, challenge.GetChallenge(), challenge.GetLength()));

    aChild->GenerateChallenge();

    SuccessOrExit(error = AppendChallenge(*message, aChild->GetChallenge(), aChild->GetChallengeSize()));

    if (isAssignLinkQuality &&
        (memcmp(mAddr64.m8, &aChild->GetExtAddress(), OT_EXT_ADDRESS_SIZE) == 0))
    {
        // use assigned one to ensure the link quality
        SuccessOrExit(error = AppendLinkMargin(*message, mAssignLinkMargin));
    }
    else
    {
        error = AppendLinkMargin(*message, aChild->GetLinkInfo().GetLinkMargin(GetNetif().GetMac().GetNoiseFloor()));
        SuccessOrExit(error);
    }

    SuccessOrExit(error = AppendConnectivity(*message));
    SuccessOrExit(error = AppendVersion(*message));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild->GetExtAddress());

    if (aRoutersOnlyRequest)
    {
        delay = (otPlatRandomGet() % kParentResponseMaxDelayRouters) + 1;
    }
    else
    {
        delay = (otPlatRandomGet() % kParentResponseMaxDelayAll) + 1;
    }

    SuccessOrExit(error = AddDelayedResponse(*message, destination, delay));

    otLogInfoMle(GetInstance(), "Delayed Parent Response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return OT_ERROR_NONE;
}

otError MleRouter::UpdateChildAddresses(const AddressRegistrationTlv &aTlv, Child &aChild)
{
    const AddressRegistrationEntry *entry;
    Ip6::Address address;
    Lowpan::Context context;
    Child *child;
    uint8_t count;
    uint8_t index;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    aChild.ClearIp6Addresses();

    for (count = 0; count < Child::kMaxIp6AddressPerChild; count++)
    {
        if ((entry = aTlv.GetAddressEntry(count)) == NULL)
        {
            break;
        }

        if (entry->IsCompressed())
        {
            // xxx check if context id exists
            GetNetif().GetNetworkDataLeader().GetContext(entry->GetContextId(), context);
            memcpy(&address, context.mPrefix, BitVectorBytes(context.mPrefixLength));
            address.SetIid(entry->GetIid());
        }
        else
        {
            address = *entry->GetIp6Address();
        }

        aChild.GetIp6Address(count) = address;

        otLogInfoMle(GetInstance(), "Child 0x%04x IPv6 address[%d]=%s", aChild.GetRloc16(), count,
                     address.ToString(stringBuffer, sizeof(stringBuffer)));

        // We check if the same address is in-use by another child, if so
        // remove it. This implements "last-in wins" duplicate address
        // resolution policy.
        //
        // Duplicate addresses can occur if a previously attached child
        // attaches to same parent again (after a reset, memory wipe) using
        // a new random extended address before the old entry in the child
        // table is timed out and then trying to register its globally unique
        // IPv6 address as the new child.

        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            child = &mChildren[i];

            if (!child->IsStateValidOrRestoring() || (child == &aChild))
            {
                continue;
            }

            if (child->FindIp6Address(address, &index) == OT_ERROR_NONE)
            {
                child->RemoveIp6Address(index);
            }
        }
    }

    if (count == 0)
    {
        otLogInfoMle(GetInstance(), "Child 0x%04x has no registered IPv6 address", aChild.GetRloc16());
    }
    else
    {
        otLogInfoMle(GetInstance(), "Child 0x%04x has %d registered IPv6 address%s", aChild.GetRloc16(), count,
                     (count == 1) ? "" : "es");
    }

    OT_UNUSED_VARIABLE(stringBuffer);

    return OT_ERROR_NONE;
}

otError MleRouter::HandleChildIdRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                        uint32_t aKeySequence)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    Mac::ExtAddress macAddr;
    ResponseTlv response;
    LinkFrameCounterTlv linkFrameCounter;
    MleFrameCounterTlv mleFrameCounter;
    ModeTlv mode;
    TimeoutTlv timeout;
    AddressRegistrationTlv address;
    TlvRequestTlv tlvRequest;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    Child *child;
    uint8_t numTlvs;

    otLogInfoMle(GetInstance(), "Received Child ID Request");

    // only process message when operating as a child, router, or leader
    VerifyOrExit(mRole >= OT_DEVICE_ROLE_CHILD, error = OT_ERROR_INVALID_STATE);

    // Find Child
    macAddr.Set(aMessageInfo.GetPeerAddr());

    VerifyOrExit((child = FindChild(macAddr)) != NULL, error = OT_ERROR_ALREADY);

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid() &&
                 memcmp(response.GetResponse(), child->GetChallenge(), child->GetChallengeSize()) == 0,
                 error = OT_ERROR_SECURITY);

    // Link-Layer Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter),
                                      linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) ==
        OT_ERROR_NONE)
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

    // Ip6 Address
    address.SetLength(0);

    if ((mode.GetMode() & ModeTlv::kModeFFD) == 0)
    {
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kAddressRegistration, sizeof(address), address));
        VerifyOrExit(address.IsValid(), error = OT_ERROR_PARSE);
    }

    // TLV Request
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest));
    VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= Child::kMaxRequestTlvs,
                 error = OT_ERROR_PARSE);

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

    // Remove from router table
    for (int i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].GetState() != Neighbor::kStateInvalid &&
            memcmp(&mRouters[i].GetExtAddress(), &macAddr, sizeof(macAddr)) == 0)
        {
            RemoveNeighbor(mRouters[i]);
            break;
        }
    }

    if (child->GetState() != Neighbor::kStateValid)
    {
        child->SetState(Neighbor::kStateChildIdRequest);
    }
    else
    {
        RemoveStoredChild(child->GetRloc16());

        if (!child->IsRxOnWhenIdle())
        {
            netif.GetMeshForwarder().ClearChildIndirectMessages(*child);
        }
    }


    child->SetLastHeard(Timer::GetNow());
    child->SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    child->SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    child->SetKeySequence(aKeySequence);
    child->SetDeviceMode(mode.GetMode());
    child->GetLinkInfo().AddRss(netif.GetMac().GetNoiseFloor(), threadMessageInfo->mRss);
    child->SetTimeout(timeout.GetTimeout());

    if (mode.GetMode() & ModeTlv::kModeFullNetworkData)
    {
        child->SetNetworkDataVersion(mLeaderData.GetDataVersion());
    }
    else
    {
        child->SetNetworkDataVersion(mLeaderData.GetStableDataVersion());
    }

    UpdateChildAddresses(address, *child);

    child->ClearRequestTlvs();

    for (numTlvs = 0; numTlvs < tlvRequest.GetLength(); numTlvs++)
    {
        child->SetRequestTlv(numTlvs, tlvRequest.GetTlvs()[numTlvs]);
    }

    if (activeTimestamp.GetLength() == 0 ||
        netif.GetActiveDataset().Compare(activeTimestamp) != 0)
    {
        child->SetRequestTlv(numTlvs++, Tlv::kActiveDataset);
    }

    if (pendingTimestamp.GetLength() == 0 ||
        netif.GetPendingDataset().Compare(pendingTimestamp) != 0)
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
        SuccessOrExit(error = SendChildIdResponse(child));
        break;
    }

exit:
    return error;
}

otError MleRouter::HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                            uint32_t aKeySequence)
{
    static const uint8_t kMaxResponseTlvs = 10;

    otError error = OT_ERROR_NONE;
    Mac::ExtAddress macAddr;
    ModeTlv mode;
    ChallengeTlv challenge;
    AddressRegistrationTlv address;
    LeaderDataTlv leaderData;
    TimeoutTlv timeout;
    Child *child;
    TlvRequestTlv tlvRequest;
    uint8_t tlvs[kMaxResponseTlvs];
    uint8_t tlvslength = 0;

    otLogInfoMle(GetInstance(), "Received Child Update Request from child");

    // Mode
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMode, sizeof(mode), mode));
    VerifyOrExit(mode.IsValid(), error = OT_ERROR_PARSE);

    // Find Child
    macAddr.Set(aMessageInfo.GetPeerAddr());
    child = FindChild(macAddr);

    tlvs[tlvslength++] = Tlv::kSourceAddress;

    // Not proceed if the Child Update Request is from the peer which is not the device's child or
    // which was the device's child but becomes invalid.
    if (child == NULL || child->GetState() == Neighbor::kStateInvalid)
    {
        // For invalid non-sleepy child, Send Child Update Response with status TLV (error)
        if (mode.GetMode() & ModeTlv::kModeRxOnWhenIdle)
        {
            tlvs[tlvslength++] = Tlv::kStatus;
            SendChildUpdateResponse(NULL, aMessageInfo, tlvs, tlvslength, NULL);
        }

        ExitNow();
    }

    child->SetDeviceMode(mode.GetMode());
    tlvs[tlvslength++] = Tlv::kMode;

    // Parent MUST include Leader Data TLV in Child Update Response
    tlvs[tlvslength++] = Tlv::kLeaderData;

    // Challenge
    if (Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge) == OT_ERROR_NONE)
    {
        VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);
        tlvs[tlvslength++] = Tlv::kResponse;
        tlvs[tlvslength++] = Tlv::kMleFrameCounter;
        tlvs[tlvslength++] = Tlv::kLinkFrameCounter;
    }

    // Ip6 Address TLV
    if (Tlv::GetTlv(aMessage, Tlv::kAddressRegistration, sizeof(address), address) == OT_ERROR_NONE)
    {
        VerifyOrExit(address.IsValid(), error = OT_ERROR_PARSE);
        UpdateChildAddresses(address, *child);
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
        child->SetTimeout(timeout.GetTimeout());
        tlvs[tlvslength++] = Tlv::kTimeout;
    }

    // TLV Request
    if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == OT_ERROR_NONE)
    {
        uint8_t tlv;
        TlvRequestIterator iterator =  TLVREQUESTTLV_ITERATOR_INIT;

        VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= (Child::kMaxRequestTlvs - tlvslength),
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

    child->SetLastHeard(Timer::GetNow());

    if (child->IsStateRestoring())
    {
        SetChildStateToValid(child);
        child->SetKeySequence(aKeySequence);
    }

    SendChildUpdateResponse(child, aMessageInfo, tlvs, tlvslength, &challenge);

exit:
    return error;
}

otError MleRouter::HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                             uint32_t aKeySequence)
{
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    Mac::ExtAddress macAddr;
    SourceAddressTlv sourceAddress;
    TimeoutTlv timeout;
    AddressRegistrationTlv address;
    ResponseTlv response;
    LinkFrameCounterTlv linkFrameCounter;
    MleFrameCounterTlv mleFrameCounter;
    LeaderDataTlv leaderData;
    Child *child;

    otLogInfoMle(GetInstance(), "Received Child Update Response from child");

    // Find Child
    macAddr.Set(aMessageInfo.GetPeerAddr());

    VerifyOrExit((child = FindChild(macAddr)) != NULL, error = OT_ERROR_NOT_FOUND);

    // Source Address
    if (Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress) == OT_ERROR_NONE)
    {
        VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(child->GetRloc16() == sourceAddress.GetRloc16(), error = OT_ERROR_PARSE);
    }

    // Response
    if (Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response) == OT_ERROR_NONE)
    {
        VerifyOrExit(response.IsValid() &&
                     memcmp(response.GetResponse(), child->GetChallenge(), child->GetChallengeSize()) == 0,
                     error = OT_ERROR_SECURITY);
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
    if (Tlv::GetTlv(aMessage, Tlv::kAddressRegistration, sizeof(address), address) == OT_ERROR_NONE)
    {
        VerifyOrExit(address.IsValid(), error = OT_ERROR_PARSE);
        UpdateChildAddresses(address, *child);
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

    SetChildStateToValid(child);
    child->SetLastHeard(Timer::GetNow());
    child->SetKeySequence(aKeySequence);
    child->GetLinkInfo().AddRss(GetNetif().GetMac().GetNoiseFloor(), threadMessageInfo->mRss);

exit:
    return error;
}

otError MleRouter::HandleDataRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    TlvRequestTlv tlvRequest;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    uint8_t tlvs[4];
    uint8_t numTlvs;

    otLogInfoMle(GetInstance(), "Received Data Request");

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

    if (activeTimestamp.GetLength() == 0 ||
        netif.GetActiveDataset().Compare(activeTimestamp))
    {
        tlvs[numTlvs++] = Tlv::kActiveDataset;
    }

    if (pendingTimestamp.GetLength() == 0 ||
        netif.GetPendingDataset().Compare(pendingTimestamp))
    {
        tlvs[numTlvs++] = Tlv::kPendingDataset;
    }

    SendDataResponse(aMessageInfo.GetPeerAddr(), tlvs, numTlvs, 0);

exit:
    return error;
}

otError MleRouter::HandleNetworkDataUpdateRouter(void)
{
    static const uint8_t tlvs[] = {Tlv::kNetworkData};
    ThreadNetif &netif = GetNetif();
    Ip6::Address destination;
    uint16_t delay;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);

    delay = (mRole == OT_DEVICE_ROLE_LEADER) ? 0 : (otPlatRandomGet() % kUnsolicitedDataResponseJitter);
    SendDataResponse(destination, tlvs, sizeof(tlvs), delay);

    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        Child *child = &mChildren[i];

        if (child->GetState() != Neighbor::kStateValid || child->IsRxOnWhenIdle())
        {
            continue;
        }

        memset(&destination, 0, sizeof(destination));
        destination.mFields.m16[0] = HostSwap16(0xfe80);
        destination.SetIid(child->GetExtAddress());

        if (child->IsFullNetworkData())
        {
            if (child->GetNetworkDataVersion() != netif.GetNetworkDataLeader().GetVersion())
            {
                SendDataResponse(destination, tlvs, sizeof(tlvs), 0);
            }
        }
        else
        {
            if (child->GetNetworkDataVersion() != netif.GetNetworkDataLeader().GetStableVersion())
            {
                SendDataResponse(destination, tlvs, sizeof(tlvs), 0);
            }
        }
    }

exit:
    return OT_ERROR_NONE;
}

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
otError MleRouter::SetSteeringData(otExtAddress *aExtAddress)
{
    otError error = OT_ERROR_NONE;
    uint8_t nullExtAddr[OT_EXT_ADDRESS_SIZE];
    uint8_t allowAnyExtAddr[OT_EXT_ADDRESS_SIZE];

    memset(nullExtAddr, 0, sizeof(nullExtAddr));
    memset(allowAnyExtAddr, 0xFF, sizeof(allowAnyExtAddr));

    mSteeringData.Init();

    if ((aExtAddress == NULL) || (memcmp(aExtAddress, &nullExtAddr, sizeof(nullExtAddr)) == 0))
    {
        // Clear steering data
        mSteeringData.Clear();
    }
    else if (memcmp(aExtAddress, &allowAnyExtAddr, sizeof(allowAnyExtAddr)) == 0)
    {
        // Set steering data to 0xFF
        mSteeringData.SetLength(1);
        mSteeringData.Set();
    }
    else
    {
        // Set bloom filter with the extended address passed in
        mSteeringData.ComputeBloomFilter(aExtAddress);
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

otError MleRouter::HandleDiscoveryRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Tlv tlv;
    MeshCoP::Tlv meshcopTlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    MeshCoP::ExtendedPanIdTlv extPanId;
    uint16_t offset;
    uint16_t end;

    otLogInfoMle(GetInstance(), "Received discovery request");

    // only Routers and REEDs respond
    VerifyOrExit((mDeviceMode & ModeTlv::kModeFFD) != 0, error = OT_ERROR_INVALID_STATE);

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
#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

                if (!mSteeringData.IsCleared())
                {
                    break;
                }
                else // if steering data is not set out of band, fall back to network data
#endif // OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
                {
                    VerifyOrExit(netif.GetNetworkDataLeader().IsJoiningEnabled(), error = OT_ERROR_NOT_CAPABLE);
                }
            }

            break;

        case MeshCoP::Tlv::kExtendedPanId:
            aMessage.Read(offset, sizeof(extPanId), &extPanId);
            VerifyOrExit(extPanId.IsValid(), error = OT_ERROR_PARSE);
            VerifyOrExit(memcmp(netif.GetMac().GetExtendedPanId(), extPanId.GetExtendedPanId(), OT_EXT_PAN_ID_SIZE),
                         error = OT_ERROR_DROP);
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
        otLogWarnMleErr(GetInstance(), error, "Failed to process Discovery Request");
    }

    return error;
}

otError MleRouter::SendDiscoveryResponse(const Ip6::Address &aDestination, uint16_t aPanId)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Message *message;
    uint16_t startOffset;
    Tlv tlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::ExtendedPanIdTlv extPanId;
    MeshCoP::NetworkNameTlv networkName;
    MeshCoP::JoinerUdpPortTlv joinerUdpPort;
    MeshCoP::Tlv *steeringData;
    uint16_t delay;

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
    discoveryResponse.SetVersion(kVersion);

    if (netif.GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_NATIVE_COMMISSIONING)
    {
        discoveryResponse.SetNativeCommissioner(true);
    }
    else
    {
        discoveryResponse.SetNativeCommissioner(false);
    }

    SuccessOrExit(error = message->Append(&discoveryResponse, sizeof(discoveryResponse)));

    // Extended PAN ID TLV
    extPanId.Init();
    extPanId.SetExtendedPanId(netif.GetMac().GetExtendedPanId());
    SuccessOrExit(error = message->Append(&extPanId, sizeof(extPanId)));

    // Network Name TLV
    networkName.Init();
    networkName.SetNetworkName(netif.GetMac().GetNetworkName());
    SuccessOrExit(error = message->Append(&networkName, sizeof(tlv) + networkName.GetLength()));

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

    // If steering data is set out of band, use that value.
    // Otherwise use the one from commissioning data.
    if (!mSteeringData.IsCleared())
    {
        SuccessOrExit(error = message->Append(&mSteeringData, sizeof(mSteeringData) + mSteeringData.GetLength()));
    }
    else
#endif // OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    {
        // Steering Data TLV
        steeringData = netif.GetNetworkDataLeader().GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);

        if (steeringData != NULL)
        {
            SuccessOrExit(error = message->Append(steeringData, sizeof(*steeringData) + steeringData->GetLength()));
        }
    }

    // Joiner UDP Port TLV
    joinerUdpPort.Init();
    joinerUdpPort.SetUdpPort(netif.GetJoinerRouter().GetJoinerUdpPort());
    SuccessOrExit(error = message->Append(&joinerUdpPort, sizeof(tlv) + joinerUdpPort.GetLength()));

    tlv.SetLength(static_cast<uint8_t>(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    delay = otPlatRandomGet() % (kDiscoveryMaxJitter + 1);

    SuccessOrExit(error = AddDelayedResponse(*message, aDestination, delay));

    otLogInfoMle(GetInstance(), "Sent discovery response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendChildIdResponse(Child *aChild)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildIdResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    if (aChild->GetState() != Neighbor::kStateValid)
    {
        // pick next Child ID that is not being used
        do
        {
            mNextChildId++;

            if (mNextChildId > kMaxChildId)
            {
                mNextChildId = kMinChildId;
            }
        }
        while (FindChild(mNextChildId) != NULL);

        // allocate Child ID
        aChild->SetRloc16(netif.GetMac().GetShortAddress() | mNextChildId);
    }

    SuccessOrExit(error = AppendAddress16(*message, aChild->GetRloc16()));

    for (uint8_t i = 0; i < Child::kMaxRequestTlvs; i++)
    {
        switch (aChild->GetRequestTlv(i))
        {
        case Tlv::kNetworkData:
            SuccessOrExit(error = AppendNetworkData(*message, !aChild->IsFullNetworkData()));
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

    if (!aChild->IsFullThreadDevice())
    {
        SuccessOrExit(error = AppendChildAddresses(*message, *aChild));
    }

    SetChildStateToValid(aChild);

    if (!aChild->IsRxOnWhenIdle())
    {
        netif.GetMeshForwarder().GetSourceMatchController().SetSrcMatchAsShort(*aChild, false);
    }

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild->GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle(GetInstance(), "Sent Child ID Response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendChildUpdateRequest(Child *aChild)
{
    static const uint8_t tlvs[] = {Tlv::kTimeout, Tlv::kAddressRegistration};
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *message;

    if (!aChild->IsRxOnWhenIdle())
    {
        uint8_t childIndex = netif.GetMle().GetChildIndex(*aChild);

        // No need to send "Child Update Request" to the sleepy child if there is one already queued for it.
        for (message = netif.GetMeshForwarder().GetSendQueue().GetHead(); message; message = message->GetNext())
        {
            if (message->GetChildMask(childIndex) && message->GetSubType() == Message::kSubTypeMleChildUpdateRequest)
            {
                ExitNow();
            }
        }
    }

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleChildUpdateRequest);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateRequest));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendNetworkData(*message, !aChild->IsFullNetworkData()));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));
    SuccessOrExit(error = AppendTlvRequest(*message, tlvs, sizeof(tlvs)));

    aChild->GenerateChallenge();

    SuccessOrExit(error = AppendChallenge(*message, aChild->GetChallenge(), aChild->GetChallengeSize()));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild->GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    if (aChild->IsRxOnWhenIdle())
    {
        // only try to send a single Child Update Request message to an rx-on-when-idle child
        aChild->SetState(Child::kStateChildUpdateRequest);
    }

    otLogInfoMle(GetInstance(), "Sent Child Update Request to child");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendChildUpdateResponse(Child *aChild, const Ip6::MessageInfo &aMessageInfo,
                                           const uint8_t *aTlvs, uint8_t aTlvslength,
                                           const ChallengeTlv *aChallenge)
{
    otError error = OT_ERROR_NONE;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateResponse));

    for (int i = 0; i < aTlvslength; i++)
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
            SuccessOrExit(error = AppendResponse(*message, aChallenge->GetChallenge(), aChallenge->GetLength()));
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

    otLogInfoMle(GetInstance(), "Sent Child Update Response to child");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return OT_ERROR_NONE;
}

otError MleRouter::SendDataResponse(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength,
                                    uint16_t aDelay)
{
    otError error = OT_ERROR_NONE;
    Message *message;
    Neighbor *neighbor;
    bool stableOnly;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
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
            neighbor = GetNeighbor(aDestination);
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
        SuccessOrExit(error = AddDelayedResponse(*message, aDestination, aDelay));
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aDestination));
    }

    otLogInfoMle(GetInstance(), "Sent Data Response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

Child *MleRouter::GetChild(uint16_t aAddress)
{
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].IsStateValidOrRestoring() && mChildren[i].GetRloc16() == aAddress)
        {
            return &mChildren[i];
        }
    }

    return NULL;
}

Child *MleRouter::GetChild(const Mac::ExtAddress &aAddress)
{
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].IsStateValidOrRestoring() &&
            memcmp(&mChildren[i].GetExtAddress(), &aAddress, sizeof(aAddress)) == 0)
        {
            return &mChildren[i];
        }
    }

    return NULL;
}

Child *MleRouter::GetChild(const Mac::Address &aAddress)
{
    switch (aAddress.mLength)
    {
    case sizeof(aAddress.mShortAddress):
        return GetChild(aAddress.mShortAddress);

    case sizeof(aAddress.mExtAddress):
        return GetChild(aAddress.mExtAddress);
    }

    return NULL;
}

uint8_t MleRouter::GetChildIndex(const Child &child)
{
    return static_cast<uint8_t>(&child - mChildren);
}

Child *MleRouter::GetChildren(uint8_t *numChildren)
{
    if (numChildren != NULL)
    {
        *numChildren = mMaxChildrenAllowed;
    }

    return mChildren;
}

otError MleRouter::SetMaxAllowedChildren(uint8_t aMaxChildren)
{
    otError error = OT_ERROR_NONE;

    // Ensure the value is between 1 and kMaxChildren
    VerifyOrExit(aMaxChildren > 0 && aMaxChildren <= kMaxChildren, error = OT_ERROR_INVALID_ARGS);

    // Do not allow setting max children if MLE is running
    VerifyOrExit(mRole == OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    // Save the value
    mMaxChildrenAllowed = aMaxChildren;

exit:
    return error;
}

otError MleRouter::RemoveNeighbor(const Mac::Address &aAddress)
{
    otError error = OT_ERROR_NONE;
    Neighbor *neighbor;

    VerifyOrExit((neighbor = GetNeighbor(aAddress)) != NULL, error = OT_ERROR_NOT_FOUND);
    SuccessOrExit(error = RemoveNeighbor(*neighbor));

exit:
    return error;
}

otError MleRouter::RemoveNeighbor(Neighbor &aNeighbor)
{
    ThreadNetif &netif = GetNetif();

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        break;

    case OT_DEVICE_ROLE_CHILD:
        if (&aNeighbor == &mParent)
        {
            BecomeDetached();
        }

        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if (aNeighbor.IsStateValidOrRestoring() && !IsActiveRouter(aNeighbor.GetRloc16()))
        {
            aNeighbor.SetState(Neighbor::kStateInvalid);
            netif.GetMeshForwarder().UpdateIndirectMessages();
            netif.SetStateChangedFlags(OT_CHANGED_THREAD_CHILD_REMOVED);
            netif.GetNetworkDataLeader().SendServerDataNotification(aNeighbor.GetRloc16());
            RemoveStoredChild(aNeighbor.GetRloc16());
        }
        else if ((aNeighbor.GetState() == Neighbor::kStateValid) && IsActiveRouter(aNeighbor.GetRloc16()))
        {
            Router &routerToRemove = static_cast<Router &>(aNeighbor);

            routerToRemove.SetLinkQualityOut(0);
            routerToRemove.SetLastHeard(Timer::GetNow());

            for (uint8_t j = 0; j <= kMaxRouterId; j++)
            {
                if (mRouters[j].GetNextHop() == GetRouterId(routerToRemove.GetRloc16()))
                {
                    mRouters[j].SetNextHop(kInvalidRouterId);
                    mRouters[j].SetCost(0);

                    if (GetLinkCost(j) >= kMaxRouteCost)
                    {
                        ResetAdvertiseInterval();
                    }
                }
            }

            if (routerToRemove.GetNextHop() == kInvalidRouterId)
            {
                ResetAdvertiseInterval();
            }
        }

        break;
    }

    aNeighbor.GetLinkInfo().Clear();
    aNeighbor.SetState(Neighbor::kStateInvalid);

    return OT_ERROR_NONE;
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
        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].IsStateValidOrRestoring() && mChildren[i].GetRloc16() == aAddress)
            {
                ExitNow(rval = &mChildren[i]);
            }
        }

        for (int i = 0; i <= kMaxRouterId; i++)
        {
            if (i == mRouterId)
            {
                continue;
            }

            if (mRouters[i].GetState() == Neighbor::kStateValid && mRouters[i].GetRloc16() == aAddress)
            {
                ExitNow(rval = &mRouters[i]);
            }
        }

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
        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].IsStateValidOrRestoring() &&
                memcmp(&mChildren[i].GetExtAddress(), &aAddress, sizeof(aAddress)) == 0)
            {
                ExitNow(rval = &mChildren[i]);
            }
        }

        for (int i = 0; i <= kMaxRouterId; i++)
        {
            if (i == mRouterId)
            {
                continue;
            }

            if (mRouters[i].GetState() == Neighbor::kStateValid &&
                memcmp(&mRouters[i].GetExtAddress(), &aAddress, sizeof(aAddress)) == 0)
            {
                ExitNow(rval = &mRouters[i]);
            }
        }

        if (mParentRequestState != kParentIdle)
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

    switch (aAddress.mLength)
    {
    case sizeof(aAddress.mShortAddress):
        rval = GetNeighbor(aAddress.mShortAddress);
        break;

    case sizeof(aAddress.mExtAddress):
        rval = GetNeighbor(aAddress.mExtAddress);
        break;

    default:
        break;
    }

    return rval;
}

Neighbor *MleRouter::GetNeighbor(const Ip6::Address &aAddress)
{
    Mac::Address macaddr;
    Lowpan::Context context;
    Child *child;
    Router *router;
    Neighbor *rval = NULL;

    if (aAddress.IsLinkLocal())
    {
        if (aAddress.mFields.m16[4] == HostSwap16(0x0000) &&
            aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
            aAddress.mFields.m16[6] == HostSwap16(0xfe00))
        {
            macaddr.mLength = sizeof(macaddr.mShortAddress);
            macaddr.mShortAddress = HostSwap16(aAddress.mFields.m16[7]);
        }
        else
        {
            macaddr.mLength = sizeof(macaddr.mExtAddress);
            macaddr.mExtAddress.Set(aAddress);
        }

        ExitNow(rval = GetNeighbor(macaddr));
    }

    if (GetNetif().GetNetworkDataLeader().GetContext(aAddress, context) != OT_ERROR_NONE)
    {
        context.mContextId = 0xff;
    }

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        child = &mChildren[i];

        if (!child->IsStateValidOrRestoring())
        {
            continue;
        }

        if (context.mContextId == 0 &&
            aAddress.mFields.m16[4] == HostSwap16(0x0000) &&
            aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
            aAddress.mFields.m16[6] == HostSwap16(0xfe00) &&
            aAddress.mFields.m16[7] == HostSwap16(child->GetRloc16()))
        {
            ExitNow(rval = child);
        }

        if (child->FindIp6Address(aAddress, NULL) == OT_ERROR_NONE)
        {
            ExitNow(rval = child);
        }
    }

    VerifyOrExit(context.mContextId == 0, rval = NULL);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        router = &mRouters[i];

        if (router->GetState() != Neighbor::kStateValid || i == mRouterId)
        {
            continue;
        }

        if (aAddress.mFields.m16[4] == HostSwap16(0x0000) &&
            aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
            aAddress.mFields.m16[6] == HostSwap16(0xfe00) &&
            aAddress.mFields.m16[7] == HostSwap16(router->GetRloc16()))
        {
            ExitNow(rval = router);
        }
    }

exit:
    return rval;
}

uint16_t MleRouter::GetNextHop(uint16_t aDestination)
{
    uint8_t destinationId = GetRouterId(aDestination);
    uint8_t routeCost;
    uint8_t linkCost;
    uint16_t rval = Mac::kShortAddrInvalid;
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

    router = GetRouter(destinationId);
    VerifyOrExit(router != NULL);

    linkCost = GetLinkCost(destinationId);
    routeCost = GetRouteCost(aDestination);

    if ((routeCost + GetLinkCost(router->GetNextHop())) < linkCost)
    {
        nextHop = GetRouter(router->GetNextHop());
        VerifyOrExit(nextHop != NULL && nextHop->GetState() != Neighbor::kStateInvalid);

        rval = GetRloc16(router->GetNextHop());
    }
    else if (linkCost < kMaxRouteCost)
    {
        rval = GetRloc16(destinationId);
    }

exit:
    return rval;
}

uint8_t MleRouter::GetCost(uint16_t aRloc16)
{
    uint8_t routerId = GetRouterId(aRloc16);
    uint8_t cost = GetLinkCost(routerId);
    Router *router = GetRouter(routerId);
    uint8_t routeCost;

    VerifyOrExit(router != NULL && GetRouter(router->GetNextHop()) != NULL);

    routeCost = GetRouteCost(aRloc16) + GetLinkCost(GetRouter(routerId)->GetNextHop());

    if (cost > routeCost)
    {
        cost = routeCost;
    }

exit:
    return cost;
}

uint8_t MleRouter::GetRouteCost(uint16_t aRloc16) const
{
    uint8_t rval = kMaxRouteCost;
    const Router *router;

    router = GetRouter(GetRouterId(aRloc16));
    VerifyOrExit(router != NULL && GetRouter(router->GetNextHop()) != NULL);

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
    mRouterId = aRouterId;
    mPreviousRouterId = mRouterId;
}

Router *MleRouter::GetRouters(uint8_t *aNumRouters)
{
    if (aNumRouters != NULL)
    {
        *aNumRouters = kMaxRouterId + 1;
    }

    return mRouters;
}

Router *MleRouter::GetRouter(uint8_t aRouterId)
{
    Router *rval = NULL;

    VerifyOrExit(aRouterId <= kMaxRouterId);

    rval = &mRouters[aRouterId];

exit:
    return rval;
}

const Router *MleRouter::GetRouter(uint8_t aRouterId) const
{
    const Router *rval = NULL;

    VerifyOrExit(aRouterId <= kMaxRouterId);

    rval = &mRouters[aRouterId];

exit:
    return rval;
}

otError MleRouter::GetChildInfoById(uint16_t aChildId, otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;
    Child *child;

    if ((aChildId & ~kMaxChildId) != 0)
    {
        aChildId = GetChildId(aChildId);
    }

    VerifyOrExit((child = FindChild(aChildId)) != NULL, error = OT_ERROR_NOT_FOUND);
    error = GetChildInfo(*child, aChildInfo);

exit:
    return error;
}

otError MleRouter::GetChildInfoByIndex(uint8_t aChildIndex, otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aChildIndex < mMaxChildrenAllowed, error = OT_ERROR_INVALID_ARGS);
    error = GetChildInfo(mChildren[aChildIndex], aChildInfo);

exit:
    return error;
}

otError MleRouter::RestoreChildren(void)
{
    otError error = OT_ERROR_NONE;

    for (uint8_t i = 0; ; i++)
    {
        Child *child;
        Settings::ChildInfo childInfo;
        uint16_t length;

        length = sizeof(childInfo);
        SuccessOrExit(error = otPlatSettingsGet(GetInstance(), Settings::kKeyChildInfo, i,
                                                reinterpret_cast<uint8_t *>(&childInfo), &length));
        VerifyOrExit(length >= sizeof(childInfo), error = OT_ERROR_PARSE);

        VerifyOrExit((child = NewChild()) != NULL, error = OT_ERROR_NO_BUFS);
        memset(child, 0, sizeof(*child));

        child->SetExtAddress(*static_cast<Mac::ExtAddress *>(&childInfo.mExtAddress));
        child->SetRloc16(childInfo.mRloc16);
        child->SetTimeout(childInfo.mTimeout);
        child->SetDeviceMode(childInfo.mMode);
        child->SetState(Neighbor::kStateRestored);
        child->SetLastHeard(Timer::GetNow());
        GetNetif().GetMeshForwarder().GetSourceMatchController().SetSrcMatchAsShort(*child, true);
    }

exit:
    return error;
}

otError MleRouter::RemoveStoredChild(uint16_t aChildRloc16)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (uint8_t i = 0; i < kMaxChildren; i++)
    {
        Settings::ChildInfo childInfo;
        uint16_t length = sizeof(childInfo);

        SuccessOrExit(error = otPlatSettingsGet(GetInstance(), Settings::kKeyChildInfo, i,
                                                reinterpret_cast<uint8_t *>(&childInfo), &length));
        VerifyOrExit(length == sizeof(childInfo), error = OT_ERROR_PARSE);

        if (childInfo.mRloc16 == aChildRloc16)
        {
            error = otPlatSettingsDelete(GetNetif().GetInstance(), Settings::kKeyChildInfo, i);
            ExitNow();
        }
    }

exit:
    return error;
}

otError MleRouter::StoreChild(uint16_t aChildRloc16)
{
    otError error = OT_ERROR_NONE;
    Child *child;
    Settings::ChildInfo childInfo;

    VerifyOrExit((child = FindChild(GetChildId(aChildRloc16))) != NULL, error = OT_ERROR_NOT_FOUND);

    IgnoreReturnValue(RemoveStoredChild(aChildRloc16));

    memset(&childInfo, 0, sizeof(childInfo));
    memcpy(&childInfo.mExtAddress, &child->GetExtAddress(), sizeof(childInfo.mExtAddress));

    childInfo.mTimeout = child->GetTimeout();
    childInfo.mRloc16  = child->GetRloc16();
    childInfo.mMode    = child->GetDeviceMode();

    error = otPlatSettingsAdd(GetInstance(), Settings::kKeyChildInfo, reinterpret_cast<uint8_t *>(&childInfo),
                              sizeof(childInfo));

exit:
    return error;
}

otError MleRouter::RefreshStoredChildren(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = otPlatSettingsDelete(GetInstance(), Settings::kKeyChildInfo, -1));

    for (uint8_t i = 0; i < kMaxChildren; i++)
    {
        if (mChildren[i].GetState() != Neighbor::kStateInvalid)
        {
            SuccessOrExit(error = StoreChild(mChildren[i].GetRloc16()));
        }
    }

exit:
    return error;
}

otError MleRouter::GetChildInfo(Child &aChild, otChildInfo &aChildInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aChild.GetState() == Neighbor::kStateValid, error = OT_ERROR_NOT_FOUND);

    memset(&aChildInfo, 0, sizeof(aChildInfo));
    memcpy(&aChildInfo.mExtAddress, &aChild.GetExtAddress(), sizeof(aChildInfo.mExtAddress));

    aChildInfo.mTimeout            = aChild.GetTimeout();
    aChildInfo.mRloc16             = aChild.GetRloc16();
    aChildInfo.mChildId            = GetChildId(aChild.GetRloc16());
    aChildInfo.mNetworkDataVersion = aChild.GetNetworkDataVersion();
    aChildInfo.mAge                = Timer::MsecToSec(Timer::GetNow() - aChild.GetLastHeard());
    aChildInfo.mLinkQualityIn      = aChild.GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor());
    aChildInfo.mAverageRssi        = aChild.GetLinkInfo().GetAverageRss();
    aChildInfo.mLastRssi           = aChild.GetLinkInfo().GetLastRss();

    aChildInfo.mRxOnWhenIdle      = aChild.IsRxOnWhenIdle();
    aChildInfo.mSecureDataRequest = aChild.IsSecureDataRequest();
    aChildInfo.mFullFunction      = aChild.IsFullThreadDevice();
    aChildInfo.mFullNetworkData   = aChild.IsFullNetworkData();

exit:
    return error;
}

otError MleRouter::GetRouterInfo(uint16_t aRouterId, otRouterInfo &aRouterInfo)
{
    otError error = OT_ERROR_NONE;
    Router *router;
    uint8_t routerId;

    if (aRouterId > kMaxRouterId && IsActiveRouter(aRouterId))
    {
        routerId = GetRouterId(aRouterId);
    }
    else
    {
        routerId = static_cast<uint8_t>(aRouterId);
    }

    router = GetRouter(routerId);
    VerifyOrExit(router != NULL, error = OT_ERROR_INVALID_ARGS);

    memcpy(&aRouterInfo.mExtAddress, &router->GetExtAddress(), sizeof(aRouterInfo.mExtAddress));

    aRouterInfo.mAllocated       = router->IsAllocated();
    aRouterInfo.mRouterId        = routerId;
    aRouterInfo.mRloc16          = GetRloc16(routerId);
    aRouterInfo.mNextHop         = router->GetNextHop();
    aRouterInfo.mLinkEstablished = router->GetState() == Neighbor::kStateValid;
    aRouterInfo.mPathCost        = router->GetCost();
    aRouterInfo.mLinkQualityIn   = router->GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor());
    aRouterInfo.mLinkQualityOut  = router->GetLinkQualityOut();
    aRouterInfo.mAge             = static_cast<uint8_t>(Timer::MsecToSec(Timer::GetNow() - router->GetLastHeard()));

exit:
    return error;
}

otError MleRouter::GetNextNeighborInfo(otNeighborInfoIterator &aIterator, otNeighborInfo &aNeighInfo)
{
    otError error = OT_ERROR_NONE;
    Neighbor *neighbor = NULL;
    int16_t index;

    memset(&aNeighInfo, 0, sizeof(aNeighInfo));

    // Non-negative iterator value gives the current index into mChildren array

    if (aIterator >= 0)
    {
        for (index = aIterator; index < mMaxChildrenAllowed; index++)
        {
            if (mChildren[index].GetState() == Neighbor::kStateValid)
            {
                neighbor = &mChildren[index];
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
        if (mRouters[index].GetState() == Neighbor::kStateValid)
        {
            neighbor = &mRouters[index];
            aNeighInfo.mIsChild = false;
            index++;
            aIterator = -index;
            ExitNow();
        }
    }

    aIterator = -index;
    error = OT_ERROR_NOT_FOUND;

exit:

    if (neighbor != NULL)
    {
        memcpy(&aNeighInfo.mExtAddress, &neighbor->GetExtAddress(), sizeof(aNeighInfo.mExtAddress));
        aNeighInfo.mAge = Timer::MsecToSec(Timer::GetNow() - neighbor->GetLastHeard());
        aNeighInfo.mRloc16 = neighbor->GetRloc16();
        aNeighInfo.mLinkFrameCounter = neighbor->GetLinkFrameCounter();
        aNeighInfo.mMleFrameCounter = neighbor->GetMleFrameCounter();
        aNeighInfo.mLinkQualityIn = neighbor->GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor());
        aNeighInfo.mAverageRssi = neighbor->GetLinkInfo().GetAverageRss();
        aNeighInfo.mLastRssi = neighbor->GetLinkInfo().GetLastRss();
        aNeighInfo.mRxOnWhenIdle = neighbor->IsRxOnWhenIdle();
        aNeighInfo.mSecureDataRequest = neighbor->IsSecureDataRequest();
        aNeighInfo.mFullFunction = neighbor->IsFullThreadDevice();
        aNeighInfo.mFullNetworkData = neighbor->IsFullNetworkData();
    }

    return error;
}

void MleRouter::ResolveRoutingLoops(uint16_t aSourceMac, uint16_t aDestRloc16)
{
    if (aSourceMac == GetNextHop(aDestRloc16))
    {
        // loop detected
        Router *router = GetRouter(GetRouterId(aDestRloc16));
        assert(router != NULL);

        // invalidate next hop
        router->SetNextHop(kInvalidRouterId);
        ResetAdvertiseInterval();
    }
}

otError MleRouter::CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    ThreadNetif &netif = GetNetif();
    Ip6::MessageInfo messageInfo;

    if (mRole == OT_DEVICE_ROLE_CHILD)
    {
        return Mle::CheckReachability(aMeshSource, aMeshDest, aIp6Header);
    }

    if (aMeshDest == netif.GetMac().GetShortAddress())
    {
        // mesh destination is this device
        if (netif.IsUnicastAddress(aIp6Header.GetDestination()))
        {
            // IPv6 destination is this device
            return OT_ERROR_NONE;
        }
        else if (GetNeighbor(aIp6Header.GetDestination()) != NULL)
        {
            // IPv6 destination is an RFD child
            return OT_ERROR_NONE;
        }
    }
    else if (GetRouterId(aMeshDest) == mRouterId)
    {
        // mesh destination is a child of this device
        if (GetChild(aMeshDest))
        {
            return OT_ERROR_NONE;
        }
    }
    else if (GetNextHop(aMeshDest) != Mac::kShortAddrInvalid)
    {
        // forwarding to another router and route is known
        return OT_ERROR_NONE;
    }

    messageInfo.GetPeerAddr() = GetMeshLocal16();
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(aMeshSource);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());

    netif.GetIp6().mIcmp.SendError(Ip6::IcmpHeader::kTypeDstUnreach,
                                   Ip6::IcmpHeader::kCodeDstUnreachNoRoute,
                                   messageInfo, aIp6Header);

    return OT_ERROR_DROP;
}

otError MleRouter::SendAddressSolicit(ThreadStatusTlv::Status aStatus)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    ThreadExtMacAddressTlv macAddr64Tlv;
    ThreadRloc16Tlv rlocTlv;
    ThreadStatusTlv statusTlv;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_ADDRESS_SOLICIT);
    header.SetPayloadMarker();

    VerifyOrExit((message = netif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    macAddr64Tlv.Init();
    macAddr64Tlv.SetMacAddr(*netif.GetMac().GetExtAddress());
    SuccessOrExit(error = message->Append(&macAddr64Tlv, sizeof(macAddr64Tlv)));

    if (IsRouterIdValid(mPreviousRouterId))
    {
        rlocTlv.Init();
        rlocTlv.SetRloc16(GetRloc16(mPreviousRouterId));
        SuccessOrExit(error = message->Append(&rlocTlv, sizeof(rlocTlv)));
    }

    statusTlv.Init();
    statusTlv.SetStatus(aStatus);
    SuccessOrExit(error = message->Append(&statusTlv, sizeof(statusTlv)));

    SuccessOrExit(error = GetLeaderAddress(messageInfo.GetPeerAddr()));
    messageInfo.SetSockAddr(GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo,
                                                      &MleRouter::HandleAddressSolicitResponse, this));

    otLogInfoMle(GetInstance(), "Sent address solicit to %04x", HostSwap16(messageInfo.GetPeerAddr().mFields.m16[7]));

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError MleRouter::SendAddressRelease(void)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header header;
    ThreadRloc16Tlv rlocTlv;
    ThreadExtMacAddressTlv macAddr64Tlv;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_ADDRESS_RELEASE);
    header.SetPayloadMarker();

    VerifyOrExit((message = netif.GetCoap().NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    rlocTlv.Init();
    rlocTlv.SetRloc16(GetRloc16(mRouterId));
    SuccessOrExit(error = message->Append(&rlocTlv, sizeof(rlocTlv)));

    macAddr64Tlv.Init();
    macAddr64Tlv.SetMacAddr(*netif.GetMac().GetExtAddress());
    SuccessOrExit(error = message->Append(&macAddr64Tlv, sizeof(macAddr64Tlv)));

    messageInfo.SetSockAddr(GetMeshLocal16());
    SuccessOrExit(error = GetLeaderAddress(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMle(GetInstance(), "Sent address release");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void MleRouter::HandleAddressSolicitResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                             const otMessageInfo *aMessageInfo, otError aResult)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicitResponse(static_cast<Coap::Header *>(aHeader),
                                                                     static_cast<Message *>(aMessage),
                                                                     static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                     aResult);
}

void MleRouter::HandleAddressSolicitResponse(Coap::Header *aHeader, Message *aMessage,
                                             const Ip6::MessageInfo *aMessageInfo, otError aResult)
{
    OT_UNUSED_VARIABLE(aResult);
    OT_UNUSED_VARIABLE(aMessageInfo);

    ThreadStatusTlv statusTlv;
    ThreadRloc16Tlv rlocTlv;
    ThreadRouterMaskTlv routerMaskTlv;
    uint8_t routerId;
    Router *router;
    bool old;

    VerifyOrExit(aResult == OT_ERROR_NONE && aHeader != NULL && aMessage != NULL);

    VerifyOrExit(aHeader->GetCode() == OT_COAP_CODE_CHANGED);

    otLogInfoMle(GetInstance(), "Received address reply");

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
    routerId = GetRouterId(rlocTlv.GetRloc16());
    router = GetRouter(routerId);
    VerifyOrExit(router != NULL);

    SuccessOrExit(ThreadTlv::GetTlv(*aMessage, ThreadTlv::kRouterMask, sizeof(routerMaskTlv), routerMaskTlv));
    VerifyOrExit(routerMaskTlv.IsValid());

    // if allocated routerId is different from previous routerId
    if (IsRouterIdValid(mPreviousRouterId) && routerId != mPreviousRouterId)
    {
        // reset children info if any
        if (HasChildren())
        {
            RemoveChildren();
        }
    }

    // assign short address
    SetRouterId(routerId);

    SuccessOrExit(SetStateRouter(GetRloc16(mRouterId)));

    router->SetCost(0);

    // copy router id information
    mRouterIdSequence = routerMaskTlv.GetIdSequence();
    mRouterIdSequenceLastUpdated = Timer::GetNow();

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        old = mRouters[i].IsAllocated();
        mRouters[i].SetAllocated(routerMaskTlv.IsAssignedRouterIdSet(i));

        if (old && !mRouters[i].IsAllocated())
        {
            GetNetif().GetAddressResolver().Remove(i);
        }
    }

    // Keep route path to the Leader reported by the parent before it is updated.
    if (mRouters[GetLeaderId()].GetCost() == 0)
    {
        mRouters[GetLeaderId()].SetCost(mParentLeaderCost);
    }

    mRouters[GetLeaderId()].SetNextHop(GetRouterId(mParent.GetRloc16()));

    // Keep link to the parent in order to response to Parent Requests before new link is established.
    mRouters[GetRouterId(mParent.GetRloc16())] = mParent;
    mRouters[GetRouterId(mParent.GetRloc16())].SetAllocated(true);

    // send link request
    SendLinkRequest(NULL);

    // send child id responses
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        switch (mChildren[i].GetState())
        {
        case Neighbor::kStateChildIdRequest:
            SendChildIdResponse(&mChildren[i]);
            break;

        case Neighbor::kStateLinkRequest:
            assert(false);
            break;

        case Neighbor::kStateInvalid:
        case Neighbor::kStateParentRequest:
        case Neighbor::kStateValid:
        case Neighbor::kStateRestored:
        case Neighbor::kStateChildUpdateRequest:
            break;
        }
    }

exit:
    return;
}

void MleRouter::HandleAddressSolicit(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicit(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void MleRouter::HandleAddressSolicit(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    ThreadExtMacAddressTlv macAddr64Tlv;
    ThreadRloc16Tlv rlocTlv;
    ThreadStatusTlv statusTlv;
    uint8_t routerId = kInvalidRouterId;
    Router *router = NULL;

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE && aHeader.GetCode() == OT_COAP_CODE_POST,
                 error = OT_ERROR_PARSE);

    otLogInfoMle(GetInstance(), "Received address solicit");

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kExtMacAddress, sizeof(macAddr64Tlv), macAddr64Tlv));
    VerifyOrExit(macAddr64Tlv.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kStatus, sizeof(statusTlv), statusTlv));
    VerifyOrExit(statusTlv.IsValid(), error = OT_ERROR_PARSE);

    // see if allocation already exists
    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].IsAllocated() &&
            memcmp(&mRouters[i].GetExtAddress(), macAddr64Tlv.GetMacAddr(), sizeof(mRouters[i].GetExtAddress())) == 0)
        {
            ExitNow(routerId = i);
        }
    }

    // check the request reason
    switch (statusTlv.GetStatus())
    {
    case ThreadStatusTlv::kTooFewRouters:
        VerifyOrExit(GetActiveRouterCount() < mRouterUpgradeThreshold);
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
        // specific Router ID requested
        VerifyOrExit(rlocTlv.IsValid(), error = OT_ERROR_PARSE);
        routerId = GetRouterId(rlocTlv.GetRloc16());
        router = GetRouter(routerId);

        if (router != NULL)
        {
            if (router->IsAllocated() &&
                memcmp(&router->GetExtAddress(), macAddr64Tlv.GetMacAddr(), sizeof(router->GetExtAddress())))
            {
                // requested Router ID is allocated to another device
                routerId = kInvalidRouterId;
            }
            else if (!router->IsAllocated() && router->IsReclaimDelay())
            {
                // requested Router ID is deallocated but within ID_REUSE_DELAY period
                routerId = kInvalidRouterId;
            }
            else
            {
                routerId = AllocateRouterId(routerId);
            }
        }
    }

    // allocate new router id
    if (!IsRouterIdValid(routerId))
    {
        routerId = AllocateRouterId();
    }
    else
    {
        otLogInfoMle(GetInstance(), "router id requested and provided!");
    }

    router = GetRouter(routerId);

    if (router != NULL)
    {
        router->SetExtAddress(*macAddr64Tlv.GetMacAddr());
    }
    else
    {
        otLogInfoMle(GetInstance(), "router address unavailable!");
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        SendAddressSolicitResponse(aHeader, routerId, aMessageInfo);
    }
}

void MleRouter::SendAddressSolicitResponse(const Coap::Header &aRequestHeader, uint8_t aRouterId,
                                           const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    ThreadStatusTlv statusTlv;
    ThreadRouterMaskTlv routerMaskTlv;
    ThreadRloc16Tlv rlocTlv;
    Message *message;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = netif.GetCoap().NewMessage(responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    statusTlv.Init();
    statusTlv.SetStatus(!IsRouterIdValid(aRouterId) ? statusTlv.kNoAddressAvailable : statusTlv.kSuccess);
    SuccessOrExit(error = message->Append(&statusTlv, sizeof(statusTlv)));

    if (IsRouterIdValid(aRouterId))
    {
        rlocTlv.Init();
        rlocTlv.SetRloc16(GetRloc16(aRouterId));
        SuccessOrExit(error = message->Append(&rlocTlv, sizeof(rlocTlv)));

        routerMaskTlv.Init();
        routerMaskTlv.SetIdSequence(mRouterIdSequence);
        routerMaskTlv.ClearAssignedRouterIdMask();

        for (uint8_t i = 0; i <= kMaxRouterId; i++)
        {
            if (mRouters[i].IsAllocated())
            {
                routerMaskTlv.SetAssignedRouterId(i);
            }
        }

        SuccessOrExit(error = message->Append(&routerMaskTlv, sizeof(routerMaskTlv)));
    }

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMle(GetInstance(), "Sent address reply");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void MleRouter::HandleAddressRelease(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo)
{
    static_cast<MleRouter *>(aContext)->HandleAddressRelease(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void MleRouter::HandleAddressRelease(Coap::Header &aHeader, Message &aMessage,
                                     const Ip6::MessageInfo &aMessageInfo)
{
    ThreadRloc16Tlv rlocTlv;
    ThreadExtMacAddressTlv macAddr64Tlv;
    uint8_t routerId;
    Router *router;

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE &&
                 aHeader.GetCode() == OT_COAP_CODE_POST);

    otLogInfoMle(GetInstance(), "Received address release");

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rlocTlv), rlocTlv));
    VerifyOrExit(rlocTlv.IsValid());

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kExtMacAddress, sizeof(macAddr64Tlv), macAddr64Tlv));
    VerifyOrExit(macAddr64Tlv.IsValid());

    routerId = GetRouterId(rlocTlv.GetRloc16());
    router = GetRouter(routerId);

    VerifyOrExit(router != NULL &&
                 memcmp(&router->GetExtAddress(), macAddr64Tlv.GetMacAddr(), sizeof(router->GetExtAddress())) == 0);

    ReleaseRouterId(routerId);

    SuccessOrExit(GetNetif().GetCoap().SendEmptyAck(aHeader, aMessageInfo));

    otLogInfoMle(GetInstance(), "Sent address release response");

exit:
    return;
}

void MleRouter::FillConnectivityTlv(ConnectivityTlv &aTlv)
{
    ConnectivityTlv &tlv = aTlv;
    uint8_t cost;
    uint8_t linkQuality;
    uint8_t numChildren = 0;
    int8_t parentPriority = kParentPriorityMedium;
    int8_t noiseFloor = GetNetif().GetMac().GetNoiseFloor();

    if (mParentPriority != kParentPriorityUnspecified)
    {
        parentPriority = mParentPriority;
    }
    else
    {
        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].GetState() == Neighbor::kStateValid)
            {
                numChildren++;
            }
        }

        if ((mMaxChildrenAllowed - numChildren) < (mMaxChildrenAllowed / 3))
        {
            parentPriority = kParentPriorityLow;
        }
        else
        {
            parentPriority = kParentPriorityMedium;
        }
    }

    tlv.SetParentPriority(parentPriority);

    // compute leader cost and link qualities
    tlv.SetLinkQuality1(0);
    tlv.SetLinkQuality2(0);
    tlv.SetLinkQuality3(0);

    cost = mRouters[GetLeaderId()].GetCost();

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        assert(false);
        break;

    case OT_DEVICE_ROLE_CHILD:
        switch (mParent.GetLinkInfo().GetLinkQuality(noiseFloor))
        {
        case 1:
            tlv.SetLinkQuality1(tlv.GetLinkQuality1() + 1);
            break;

        case 2:
            tlv.SetLinkQuality2(tlv.GetLinkQuality2() + 1);
            break;

        case 3:
            tlv.SetLinkQuality3(tlv.GetLinkQuality3() + 1);
            break;
        }

        cost += LinkQualityToCost(mParent.GetLinkInfo().GetLinkQuality(noiseFloor));
        break;

    case OT_DEVICE_ROLE_ROUTER:
        cost += GetLinkCost(mRouters[GetLeaderId()].GetNextHop());

        if (!IsRouterIdValid(mRouters[GetLeaderId()].GetNextHop()) || GetLinkCost(GetLeaderId()) < cost)
        {
            cost = GetLinkCost(GetLeaderId());
        }

        break;

    case OT_DEVICE_ROLE_LEADER:
        cost = 0;
        break;
    }

    tlv.SetActiveRouters(0);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].IsAllocated())
        {
            tlv.SetActiveRouters(tlv.GetActiveRouters() + 1);
        }

        if (mRouters[i].GetState() != Neighbor::kStateValid || i == mRouterId)
        {
            continue;
        }

        linkQuality = mRouters[i].GetLinkInfo().GetLinkQuality(noiseFloor);

        if (linkQuality > mRouters[i].GetLinkQualityOut())
        {
            linkQuality = mRouters[i].GetLinkQualityOut();
        }

        switch (linkQuality)
        {
        case 1:
            tlv.SetLinkQuality1(tlv.GetLinkQuality1() + 1);
            break;

        case 2:
            tlv.SetLinkQuality2(tlv.GetLinkQuality2() + 1);
            break;

        case 3:
            tlv.SetLinkQuality3(tlv.GetLinkQuality3() + 1);
            break;
        }
    }

    tlv.SetLeaderCost((cost < kMaxRouteCost) ? cost : static_cast<uint8_t>(kMaxRouteCost));
    tlv.SetIdSequence(mRouterIdSequence);
    tlv.SetSedBufferSize(1280);
    tlv.SetSedDatagramCount(1);
}

otError MleRouter::AppendConnectivity(Message &aMessage)
{
    otError error;
    ConnectivityTlv tlv;

    tlv.Init();
    FillConnectivityTlv(tlv);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

exit:
    return error;
}

otError MleRouter::AppendChildAddresses(Message &aMessage, Child &aChild)
{
    ThreadNetif &netif = GetNetif();
    otError error;
    Tlv tlv;
    AddressRegistrationEntry entry;
    Lowpan::Context context;
    uint8_t length = 0;
    uint8_t startOffset = static_cast<uint8_t>(aMessage.GetLength());

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    for (uint8_t i = 0; i < Child::kMaxIp6AddressPerChild; i++)
    {
        if (aChild.GetIp6Address(i).IsUnspecified())
        {
            break;
        }

        if (netif.GetNetworkDataLeader().GetContext(aChild.GetIp6Address(i), context) == OT_ERROR_NONE)
        {
            // compressed entry
            entry.SetContextId(context.mContextId);
            entry.SetIid(aChild.GetIp6Address(i).GetIid());
        }
        else
        {
            // uncompressed entry
            entry.SetUncompressed();
            entry.SetIp6Address(aChild.GetIp6Address(i));
        }

        SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
        length += entry.GetLength();
    }

    tlv.SetLength(length);
    aMessage.Write(startOffset, sizeof(tlv), &tlv);

exit:
    return error;
}

void MleRouter::FillRouteTlv(RouteTlv &tlv)
{

    uint8_t routeCount = 0;
    uint8_t linkCost;
    uint8_t cost;

    tlv.SetRouterIdSequence(mRouterIdSequence);
    tlv.ClearRouterIdMask();

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].IsAllocated() == false)
        {
            continue;
        }

        tlv.SetRouterId(i);

        if (i == mRouterId)
        {
            tlv.SetLinkQualityIn(routeCount, 0);
            tlv.SetLinkQualityOut(routeCount, 0);
            tlv.SetRouteCost(routeCount, 1);
        }
        else
        {
            linkCost = GetLinkCost(i);

            if (!IsRouterIdValid(mRouters[i].GetNextHop()))
            {
                cost = linkCost;
            }
            else
            {
                cost = mRouters[i].GetCost() + GetLinkCost(mRouters[i].GetNextHop());

                if (linkCost < cost)
                {
                    cost = linkCost;
                }
            }

            if (cost >= kMaxRouteCost)
            {
                cost = 0;
            }

            tlv.SetRouteCost(routeCount, cost);
            tlv.SetLinkQualityOut(routeCount, mRouters[i].GetLinkQualityOut());

            if (isAssignLinkQuality &&
                (memcmp(&mRouters[i].GetExtAddress(), mAddr64.m8, OT_EXT_ADDRESS_SIZE) == 0))
            {
                tlv.SetLinkQualityIn(routeCount, mAssignLinkQuality);
            }
            else
            {
                tlv.SetLinkQualityIn(routeCount,
                                     mRouters[i].GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor()));
            }
        }

        routeCount++;
    }

    tlv.SetRouteDataLength(routeCount);
}

otError MleRouter::AppendRoute(Message &aMessage)
{
    otError error;
    RouteTlv tlv;
    tlv.Init();
    FillRouteTlv(tlv);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(Tlv) + tlv.GetLength()));
exit:
    return error;
}

otError MleRouter::AppendActiveDataset(Message &aMessage)
{
    return GetNetif().GetActiveDataset().AppendMleDatasetTlv(aMessage);
}

otError MleRouter::AppendPendingDataset(Message &aMessage)
{
    return GetNetif().GetPendingDataset().AppendMleDatasetTlv(aMessage);
}

bool MleRouter::HasMinDowngradeNeighborRouters(void)
{
    return GetMinDowngradeNeighborRouters() >= kMinDowngradeNeighbors;
}

bool MleRouter::HasOneNeighborwithComparableConnectivity(const RouteTlv &aRoute, uint8_t aRouterId)
{
    bool rval = true;
    uint8_t localLinkQuality = 0;
    uint8_t peerLinkQuality = 0;
    uint8_t routerCount = 0;

    // process local neighbor routers
    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (i == mRouterId)
        {
            routerCount++;
            continue;
        }

        // check if neighbor is valid
        if (mRouters[i].GetState() == Neighbor::kStateValid)
        {
            // if neighbor is just peer
            if (i == aRouterId)
            {
                routerCount++;
                continue;
            }

            localLinkQuality = mRouters[i].GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor());

            if (localLinkQuality > mRouters[i].GetLinkQualityOut())
            {
                localLinkQuality = mRouters[i].GetLinkQualityOut();
            }

            if (localLinkQuality >= 2)
            {
                // check if this neighbor router is in peer Route64 TLV
                if (aRoute.IsRouterIdSet(i) == false)
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

            routerCount++;
        }
    }

exit:
    return rval;
}

void MleRouter::SetChildStateToValid(Child *aChild)
{
    VerifyOrExit(aChild->GetState() != Neighbor::kStateValid);

    aChild->SetState(Neighbor::kStateValid);
    GetNetif().SetStateChangedFlags(OT_CHANGED_THREAD_CHILD_ADDED);
    StoreChild(aChild->GetRloc16());

exit:
    return;
}

bool MleRouter::HasChildren(void)
{
    bool hasChildren = false;

    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() == Neighbor::kStateRestored ||
            mChildren[i].GetState() >= Neighbor::kStateChildIdRequest)
        {
            ExitNow(hasChildren = true);
        }
    }

exit:
    return hasChildren;
}

void MleRouter::RemoveChildren(void)
{
    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        switch (mChildren[i].GetState())
        {
        case Neighbor::kStateValid:
            GetNetif().SetStateChangedFlags(OT_CHANGED_THREAD_CHILD_REMOVED);

        // Fall-through to next case

        case Neighbor::kStateChildUpdateRequest:
        case Neighbor::kStateRestored:
            RemoveStoredChild(mChildren[i].GetRloc16());
            break;

        default:
            break;
        }

        mChildren[i].SetState(Neighbor::kStateInvalid);
    }
}

bool MleRouter::HasSmallNumberOfChildren(void)
{
    uint8_t numChildren = 0;
    uint8_t routerCount = GetActiveRouterCount();

    VerifyOrExit(routerCount > mRouterDowngradeThreshold);

    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() == Neighbor::kStateValid)
        {
            numChildren++;
        }
    }

    return numChildren < (routerCount - mRouterDowngradeThreshold) * 3;

exit:
    return false;
}

uint8_t MleRouter::GetMinDowngradeNeighborRouters(void)
{
    uint8_t linkQuality;
    uint8_t routerCount = 0;

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].GetState() != Neighbor::kStateValid)
        {
            continue;
        }

        linkQuality = mRouters[i].GetLinkInfo().GetLinkQuality(GetNetif().GetMac().GetNoiseFloor());

        if (linkQuality > mRouters[i].GetLinkQualityOut())
        {
            linkQuality = mRouters[i].GetLinkQualityOut();
        }

        if (linkQuality >= 2)
        {
            routerCount++;
        }
    }

    return routerCount;
}

int8_t MleRouter::GetAssignParentPriority(void) const
{
    return mParentPriority;
}

otError MleRouter::SetAssignParentPriority(int8_t aParentPriority)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aParentPriority <= kParentPriorityHigh &&
                 aParentPriority >= kParentPriorityUnspecified, error = OT_ERROR_INVALID_ARGS);

    mParentPriority = aParentPriority;

exit:
    return error;
}

MleRouter &MleRouter::GetOwner(const Context &aContext)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    MleRouter &mle = *static_cast<MleRouter *>(aContext.GetContext());
#else
    MleRouter &mle = otGetThreadNetif().GetMle();
    OT_UNUSED_VARIABLE(aContext);
#endif
    return mle;
}

otError MleRouter::GetMaxChildTimeout(uint32_t &aTimeout) const
{
    otError error = OT_ERROR_NOT_FOUND;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);

    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].GetState() != Neighbor::kStateValid)
        {
            continue;
        }

        if (mChildren[i].IsFullThreadDevice())
        {
            continue;
        }

        if (mChildren[i].GetTimeout() > aTimeout)
        {
            aTimeout = mChildren[i].GetTimeout();
        }

        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

}  // namespace Mle
}  // namespace ot

#endif // OPENTHREAD_FTD

