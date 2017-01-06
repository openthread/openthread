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
 *   This file implements MLE functionality required for the Thread Router and Leader roles.
 */

#define WPP_NAME "mle_router.tmh"

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/encoding.hpp>
#include <common/settings.hpp>
#include <mac/mac_frame.hpp>
#include <net/icmp6.hpp>
#include <platform/random.h>
#include <platform/settings.h>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Mle {

MleRouter::MleRouter(ThreadNetif &aThreadNetif):
    Mle(aThreadNetif),
    mAdvertiseTimer(aThreadNetif.GetIp6().mTimerScheduler, &MleRouter::HandleAdvertiseTimer, NULL, this),
    mStateUpdateTimer(aThreadNetif.GetIp6().mTimerScheduler, &MleRouter::HandleStateUpdateTimer, this),
    mDelayedResponseTimer(aThreadNetif.GetIp6().mTimerScheduler, &MleRouter::HandleDelayedResponseTimer, this),
    mChildUpdateRequestTimer(aThreadNetif.GetIp6().mTimerScheduler, &MleRouter::HandleChildUpdateRequestTimer, this),
    mAddressSolicit(OPENTHREAD_URI_ADDRESS_SOLICIT, &MleRouter::HandleAddressSolicit, this),
    mAddressRelease(OPENTHREAD_URI_ADDRESS_RELEASE, &MleRouter::HandleAddressRelease, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mCoapClient(aThreadNetif.GetCoapClient())
{
    mChallengeTimeout = 0;
    mNextChildId = kMaxChildId;
    mRouterIdSequence = 0;
    memset(mChildren, 0, sizeof(mChildren));
    memset(mRouters, 0, sizeof(mRouters));

    mNetworkIdTimeout = kNetworkIdTimeout;
    mRouterUpgradeThreshold = kRouterUpgradeThreshold;
    mRouterDowngradeThreshold = kRouterDowngradeThreshold;
    mLeaderWeight = kLeaderWeight;
    mFixedLeaderPartitionId = 0;
    mMaxChildrenAllowed = kMaxChildren;

    SetRouterId(kInvalidRouterId);
    mPreviousPartitionId = 0;
    mRouterIdSequenceLastUpdated = 0;
    mRouterRoleEnabled = true;
}

bool MleRouter::IsRouterRoleEnabled(void) const
{
    return mRouterRoleEnabled && (mDeviceMode & ModeTlv::kModeFFD);
}

void MleRouter::SetRouterRoleEnabled(bool aEnabled)
{
    mRouterRoleEnabled = aEnabled;

    if (!mRouterRoleEnabled && (mDeviceState == kDeviceStateRouter || mDeviceState == kDeviceStateLeader))
    {
        BecomeDetached();
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
        if (mRouters[i].mAllocated)
        {
            numAllocated++;
        }
        else if (mRouters[i].mReclaimDelay == false)
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
        if (mRouters[i].mAllocated || mRouters[i].mReclaimDelay)
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

    VerifyOrExit(!router->mAllocated, rval = kInvalidRouterId);

    // init router state
    router->mAllocated = true;
    router->mLastHeard = Timer::GetNow();
    memset(&router->mMacAddr, 0, sizeof(router->mMacAddr));

    // bump sequence number
    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = Timer::GetNow();
    rval = aRouterId;

    otLogInfoMle("add router id %d", aRouterId);

exit:
    return rval;
}

ThreadError MleRouter::ReleaseRouterId(uint8_t aRouterId)
{
    ThreadError error = kThreadError_None;
    Router *router = GetRouter(aRouterId);

    VerifyOrExit(router != NULL, error = kThreadError_InvalidArgs);
    VerifyOrExit(mDeviceState == kDeviceStateLeader, error = kThreadError_InvalidState);

    otLogInfoMle("delete router id %d", aRouterId);
    router->mAllocated = false;
    router->mReclaimDelay = true;
    router->mState = Neighbor::kStateInvalid;
    router->mNextHop = kInvalidRouterId;

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mNextHop == aRouterId)
        {
            mRouters[i].mNextHop = kInvalidRouterId;
            mRouters[i].mCost = 0;
        }
    }

    mRouterIdSequence++;
    mRouterIdSequenceLastUpdated = Timer::GetNow();
    mAddressResolver.Remove(aRouterId);
    mNetworkData.RemoveBorderRouter(GetRloc16(aRouterId));
    ResetAdvertiseInterval();

exit:
    return error;
}

uint32_t MleRouter::GetLeaderAge(void) const
{
    return Timer::MsecToSec(Timer::GetNow() - mRouterIdSequenceLastUpdated);
}

ThreadError MleRouter::BecomeRouter(ThreadStatusTlv::Status aStatus)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mDeviceState != kDeviceStateDisabled, error = kThreadError_InvalidState);
    VerifyOrExit(mDeviceState != kDeviceStateRouter, error = kThreadError_None);
    VerifyOrExit(IsRouterRoleEnabled(), error = kThreadError_NotCapable);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        mRouters[i].mAllocated = false;
        mRouters[i].mReclaimDelay = false;
        mRouters[i].mState = Neighbor::kStateInvalid;
        mRouters[i].mNextHop = kInvalidRouterId;
    }

    mAdvertiseTimer.Stop();
    mAddressResolver.Clear();
    mRouterSelectionJitterTimeout = 0;

    switch (mDeviceState)
    {
    case kDeviceStateDetached:
        SuccessOrExit(error = SendLinkRequest(NULL));
        mStateUpdateTimer.Start(kStateUpdatePeriod);
        break;

    case kDeviceStateChild:
        SuccessOrExit(error = SendAddressSolicit(aStatus));
        break;

    default:
        assert(false);
        break;
    }

exit:
    return error;
}

ThreadError MleRouter::BecomeLeader(void)
{
    ThreadError error = kThreadError_None;
    uint8_t routerId;
    Router *router;

    VerifyOrExit(mDeviceState != kDeviceStateDisabled, error = kThreadError_InvalidState);
    VerifyOrExit(mDeviceState != kDeviceStateLeader, error = kThreadError_None);
    VerifyOrExit(IsRouterRoleEnabled(), error = kThreadError_NotCapable);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        mRouters[i].mAllocated = false;
        mRouters[i].mReclaimDelay = false;
        mRouters[i].mState = Neighbor::kStateInvalid;
        mRouters[i].mNextHop = kInvalidRouterId;
    }

    mAdvertiseTimer.Stop();
    mStateUpdateTimer.Start(kStateUpdatePeriod);
    mAddressResolver.Clear();

    routerId = IsRouterIdValid(mPreviousRouterId) ? AllocateRouterId(mPreviousRouterId) : AllocateRouterId();
    router = GetRouter(routerId);
    VerifyOrExit(router != NULL, error = kThreadError_NoBufs);

    SetRouterId(routerId);

    memcpy(&router->mMacAddr, mMac.GetExtAddress(), sizeof(router->mMacAddr));

    if (mFixedLeaderPartitionId != 0)
    {
        SetLeaderData(mFixedLeaderPartitionId, mLeaderWeight, mRouterId);
    }
    else
    {
        SetLeaderData(otPlatRandomGet(), mLeaderWeight, mRouterId);
    }

    mRouterIdSequence = static_cast<uint8_t>(otPlatRandomGet());

    mNetworkData.Reset();

    SuccessOrExit(error = SetStateLeader(GetRloc16(mRouterId)));

    SuccessOrExit(error = AddLeaderAloc());

    ResetAdvertiseInterval();

exit:
    return error;
}

void MleRouter::StopLeader(void)
{
    mCoapServer.RemoveResource(mAddressSolicit);
    mCoapServer.RemoveResource(mAddressRelease);
    mNetif.GetActiveDataset().StopLeader();
    mNetif.GetPendingDataset().StopLeader();
    mAdvertiseTimer.Stop();
    mNetworkData.Stop();
    mNetif.UnsubscribeAllRoutersMulticast();
}

ThreadError MleRouter::HandleDetachStart(void)
{
    ThreadError error = kThreadError_None;

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        mRouters[i].mState = Neighbor::kStateInvalid;
    }

    StopLeader();
    mStateUpdateTimer.Stop();

    return error;
}

ThreadError MleRouter::HandleChildStart(otMleAttachFilter aFilter)
{
    mRouterIdSequenceLastUpdated = Timer::GetNow();

    StopLeader();
    mStateUpdateTimer.Start(kStateUpdatePeriod);

    mAdvertiseTimer.Start(
        Timer::SecToMsec(kReedAdvertiseInterval),
        Timer::SecToMsec(kReedAdvertiseInterval + kReedAdvertiseJitter),
        TrickleTimer::kModePlainTimer);
    mNetif.SubscribeAllRoutersMulticast();

    VerifyOrExit(IsRouterIdValid(mPreviousRouterId), ;);

    switch (aFilter)
    {
    case kMleAttachSamePartition:

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

    case kMleAttachAnyPartition:
    case kMleAttachBetterPartition:
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
        SendAdvertisement();
    }

    return kThreadError_None;
}

ThreadError MleRouter::SetStateRouter(uint16_t aRloc16)
{
    if (mDeviceState != kDeviceStateRouter)
    {
        mNetif.SetStateChangedFlags(OT_NET_ROLE);
    }

    SetRloc16(aRloc16);
    mDeviceState = kDeviceStateRouter;
    mParentRequestState = kParentIdle;
    mParentRequestTimer.Stop();

    mNetif.SubscribeAllRoutersMulticast();
    mRouters[mRouterId].mNextHop = mRouterId;
    mPreviousPartitionId = mLeaderData.GetPartitionId();
    mNetworkData.Stop();
    mStateUpdateTimer.Start(kStateUpdatePeriod);
    mNetif.GetIp6().SetForwardingEnabled(true);
    mNetif.GetIp6().mMpl.SetTimerExpirations(kMplRouterDataMessageTimerExpirations);

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].mState == Neighbor::kStateRestored)
        {
            mChildUpdateRequestTimer.Start(kChildUpdateRequestPeriod);
            break;
        }
    }

    otLogInfoMle("Mode -> Router");
    return kThreadError_None;
}

ThreadError MleRouter::SetStateLeader(uint16_t aRloc16)
{
    if (mDeviceState != kDeviceStateLeader)
    {
        mNetif.SetStateChangedFlags(OT_NET_ROLE);
    }

    SetRloc16(aRloc16);
    mDeviceState = kDeviceStateLeader;
    mParentRequestState = kParentIdle;
    mParentRequestTimer.Stop();

    mNetif.SubscribeAllRoutersMulticast();
    mRouters[mRouterId].mNextHop = mRouterId;
    mPreviousPartitionId = mLeaderData.GetPartitionId();
    mRouters[mRouterId].mLastHeard = Timer::GetNow();

    mNetworkData.Start();
    mNetif.GetActiveDataset().StartLeader();
    mNetif.GetPendingDataset().StartLeader();
    mCoapServer.AddResource(mAddressSolicit);
    mCoapServer.AddResource(mAddressRelease);
    mNetif.GetIp6().SetForwardingEnabled(true);
    mNetif.GetIp6().mMpl.SetTimerExpirations(kMplRouterDataMessageTimerExpirations);

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].mState == Neighbor::kStateRestored)
        {
            mChildUpdateRequestTimer.Start(kChildUpdateRequestPeriod);
            break;
        }
    }

    otLogInfoMle("Mode -> Leader %d", mLeaderData.GetPartitionId());
    return kThreadError_None;
}

uint8_t MleRouter::GetNetworkIdTimeout(void) const
{
    return mNetworkIdTimeout;
}

void MleRouter::SetNetworkIdTimeout(uint8_t aTimeout)
{
    mNetworkIdTimeout = aTimeout;
}

uint8_t MleRouter::GetRouterUpgradeThreshold(void) const
{
    return mRouterUpgradeThreshold;
}

void MleRouter::SetRouterUpgradeThreshold(uint8_t aThreshold)
{
    mRouterUpgradeThreshold = aThreshold;
}

uint8_t MleRouter::GetRouterDowngradeThreshold(void) const
{
    return mRouterDowngradeThreshold;
}

void MleRouter::SetRouterDowngradeThreshold(uint8_t aThreshold)
{
    mRouterDowngradeThreshold = aThreshold;
}

bool MleRouter::HandleAdvertiseTimer(void *aContext)
{
    MleRouter *obj = static_cast<MleRouter *>(aContext);
    return obj->HandleAdvertiseTimer();
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

void MleRouter::ResetAdvertiseInterval(void)
{
    assert(GetDeviceState() == kDeviceStateRouter ||
           GetDeviceState() == kDeviceStateLeader);

    if (!mAdvertiseTimer.IsRunning())
    {
        mAdvertiseTimer.Start(
            Timer::SecToMsec(kAdvertiseIntervalMin),
            Timer::SecToMsec(kAdvertiseIntervalMax),
            TrickleTimer::kModeNormal);
    }

    mAdvertiseTimer.IndicateInconsistent();
}

ThreadError MleRouter::SendAdvertisement(void)
{
    ThreadError error = kThreadError_None;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandAdvertisement));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));

    switch (GetDeviceState())
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        assert(false);
        break;

    case kDeviceStateChild:
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        SuccessOrExit(error = AppendRoute(*message));
        break;
    }

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("Sent advertisement");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError MleRouter::SendLinkRequest(Neighbor *aNeighbor)
{
    static const uint8_t detachedTlvs[] = {Tlv::kAddress16, Tlv::kRoute};
    static const uint8_t routerTlvs[] = {Tlv::kLinkMargin};
    ThreadError error = kThreadError_None;
    Message *message;
    Ip6::Address destination;

    memset(&destination, 0, sizeof(destination));

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandLinkRequest));
    SuccessOrExit(error = AppendVersion(*message));

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
        assert(false);
        break;

    case kDeviceStateDetached:
        SuccessOrExit(error = AppendTlvRequest(*message, detachedTlvs, sizeof(detachedTlvs)));
        break;

    case kDeviceStateChild:
        SuccessOrExit(error = AppendSourceAddress(*message));
        SuccessOrExit(error = AppendLeaderData(*message));
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
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
        for (uint8_t i = 0; i < sizeof(aNeighbor->mPending.mChallenge); i++)
        {
            aNeighbor->mPending.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
        }

        SuccessOrExit(error = AppendChallenge(*message, aNeighbor->mPending.mChallenge,
                                              sizeof(aNeighbor->mPending.mChallenge)));
        destination.mFields.m16[0] = HostSwap16(0xfe80);
        destination.SetIid(aNeighbor->mMacAddr);
    }

    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("Sent link request");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError MleRouter::HandleLinkRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Neighbor *neighbor = NULL;
    Mac::ExtAddress macAddr;
    ChallengeTlv challenge;
    VersionTlv version;
    LeaderDataTlv leaderData;
    SourceAddressTlv sourceAddress;
    TlvRequestTlv tlvRequest;
    uint16_t rloc16;

    otLogInfoMle("Received link request");

    VerifyOrExit(GetDeviceState() == kDeviceStateRouter ||
                 GetDeviceState() == kDeviceStateLeader, ;);

    VerifyOrExit(mParentRequestState == kParentIdle, ;);

    macAddr.Set(aMessageInfo.GetPeerAddr());

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = kThreadError_Parse);

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid() && version.GetVersion() == kVersion, error = kThreadError_Parse);

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == kThreadError_None)
    {
        VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), ;);
    }

    // Source Address
    if (Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress) == kThreadError_None)
    {
        VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

        rloc16 = sourceAddress.GetRloc16();

        if ((neighbor = GetNeighbor(macAddr)) != NULL && neighbor->mValid.mRloc16 != rloc16)
        {
            // remove stale neighbors
            RemoveNeighbor(*neighbor);
            neighbor = NULL;
        }

        if (IsActiveRouter(rloc16))
        {
            // source is a router
            neighbor = GetRouter(GetRouterId(rloc16));
            VerifyOrExit(neighbor != NULL, error = kThreadError_Parse);

            if (neighbor->mState != Neighbor::kStateValid)
            {
                const ThreadMessageInfo *threadMessageInfo =
                    static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());

                memcpy(&neighbor->mMacAddr, &macAddr, sizeof(neighbor->mMacAddr));
                neighbor->mLinkInfo.Clear();
                neighbor->mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
                neighbor->mLinkFailures = 0;
                neighbor->mState = Neighbor::kStateLinkRequest;
            }
            else
            {
                VerifyOrExit(memcmp(&neighbor->mMacAddr, &macAddr, sizeof(neighbor->mMacAddr)) == 0, ;);
            }
        }
    }
    else
    {
        // lack of source address indicates router coming out of reset
        VerifyOrExit((neighbor = GetNeighbor(macAddr)) != NULL, error = kThreadError_Drop);
    }

    // TLV Request
    if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == kThreadError_None)
    {
        VerifyOrExit(tlvRequest.IsValid(), error = kThreadError_Parse);
    }
    else
    {
        tlvRequest.SetLength(0);
    }

    SuccessOrExit(error = SendLinkAccept(aMessageInfo, neighbor, tlvRequest, challenge));

exit:
    return error;
}

ThreadError MleRouter::SendLinkAccept(const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor,
                                      const TlvRequestTlv &aTlvRequest, const ChallengeTlv &aChallenge)
{
    ThreadError error = kThreadError_None;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    static const uint8_t routerTlvs[] = {Tlv::kLinkMargin};
    Message *message;
    Header::Command command;
    uint8_t linkMargin;

    command = (aNeighbor == NULL || aNeighbor->mState == Neighbor::kStateValid) ?
              Header::kCommandLinkAccept : Header::kCommandLinkAcceptAndRequest;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, command));
    SuccessOrExit(error = AppendVersion(*message));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendResponse(*message, aChallenge.GetChallenge(), aChallenge.GetLength()));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));

    // always append a link margin, regardless of whether or not it was requested
    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(mMac.GetNoiseFloor(), threadMessageInfo->mRss);

    // add for certification testing
    if (isAssignLinkQuality &&
        (memcmp(aNeighbor->mMacAddr.m8, mAddr64.m8, OT_EXT_ADDRESS_SIZE) == 0))
    {
        linkMargin = mAssignLinkMargin;
    }

    SuccessOrExit(error = AppendLinkMargin(*message, linkMargin));

    if (aNeighbor != NULL && IsActiveRouter(aNeighbor->mValid.mRloc16))
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
            VerifyOrExit(aNeighbor != NULL, error = kThreadError_Drop);
            SuccessOrExit(error = AppendAddress16(*message, aNeighbor->mValid.mRloc16));
            break;

        case Tlv::kLinkMargin:
            break;

        default:
            ExitNow(error = kThreadError_Drop);
        }
    }

    if (aNeighbor != NULL && aNeighbor->mState != Neighbor::kStateValid)
    {
        for (uint8_t i = 0; i < sizeof(aNeighbor->mPending.mChallenge); i++)
        {
            aNeighbor->mPending.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
        }

        SuccessOrExit(error = AppendChallenge(*message, aNeighbor->mPending.mChallenge,
                                              sizeof(aNeighbor->mPending.mChallenge)));
        SuccessOrExit(error = AppendTlvRequest(*message, routerTlvs, sizeof(routerTlvs)));
        aNeighbor->mState = Neighbor::kStateLinkRequest;
    }

    if (aMessageInfo.GetSockAddr().IsMulticast())
    {
        SuccessOrExit(error = AddDelayedResponse(*message, aMessageInfo.GetPeerAddr(),
                                                 (otPlatRandomGet() % kMaxResponseDelay) + 1));

        otLogInfoMle("Delayed link accept");
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aMessageInfo.GetPeerAddr()));

        otLogInfoMle("Sent link accept");
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError MleRouter::HandleLinkAccept(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                        uint32_t aKeySequence)
{
    otLogInfoMle("Received link accept");
    return HandleLinkAccept(aMessage, aMessageInfo, aKeySequence, false);
}

ThreadError MleRouter::HandleLinkAcceptAndRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                                  uint32_t aKeySequence)
{
    otLogInfoMle("Received link accept and request");
    return HandleLinkAccept(aMessage, aMessageInfo, aKeySequence, true);
}

ThreadError MleRouter::HandleLinkAccept(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                        uint32_t aKeySequence, bool aRequest)
{
    ThreadError error = kThreadError_None;
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
    VerifyOrExit(version.IsValid(), error = kThreadError_Parse);

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid(), error = kThreadError_Parse);

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

    // Remove stale neighbors
    if ((neighbor = GetNeighbor(macAddr)) != NULL &&
        neighbor->mValid.mRloc16 != sourceAddress.GetRloc16())
    {
        RemoveNeighbor(*neighbor);
    }

    // Link-Layer Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter),
                                      linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = kThreadError_Parse);

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) ==
        kThreadError_None)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), error = kThreadError_Parse);
    }
    else
    {
        mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    VerifyOrExit(IsActiveRouter(sourceAddress.GetRloc16()), error = kThreadError_Parse);

    routerId = GetRouterId(sourceAddress.GetRloc16());
    router = GetRouter(routerId);

    VerifyOrExit(router != NULL, error = kThreadError_Parse);

    // verify response
    switch (router->mState)
    {
    case Neighbor::kStateLinkRequest:
        VerifyOrExit(memcmp(router->mPending.mChallenge, response.GetResponse(), sizeof(router->mPending.mChallenge)) == 0,
                     error = kThreadError_Error);
        break;

    case Neighbor::kStateInvalid:
        VerifyOrExit((mChallengeTimeout > 0) && (memcmp(mChallenge, response.GetResponse(), sizeof(mChallenge)) == 0),
                     error = kThreadError_Error);
        break;

    default:
        ExitNow(error = kThreadError_InvalidState);
    }

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
        assert(false);
        break;

    case kDeviceStateDetached:
        // Address16
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kAddress16, sizeof(address16), address16));
        VerifyOrExit(address16.IsValid(), error = kThreadError_Parse);
        VerifyOrExit(GetRloc16() == address16.GetRloc16(), error = kThreadError_Drop);

        // Route
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route));
        VerifyOrExit(route.IsValid(), error = kThreadError_Parse);
        SuccessOrExit(error = ProcessRouteTlv(route));

        // Leader Data
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
        VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);
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
            SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs));
        }

        break;

    case kDeviceStateChild:
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMargin), linkMargin));
        VerifyOrExit(linkMargin.IsValid(), error = kThreadError_Parse);
        router->mLinkQualityOut = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin.GetLinkMargin());
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        // Leader Data
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
        VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);
        VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), ;);

        // Link Margin
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMargin), linkMargin));
        VerifyOrExit(linkMargin.IsValid(), error = kThreadError_Parse);
        router->mLinkQualityOut = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin.GetLinkMargin());

        // update routing table
        if (routerId != mRouterId && !IsRouterIdValid(router->mNextHop))
        {
            router->mNextHop = routerId;
            ResetAdvertiseInterval();
        }

        break;
    }

    // finish link synchronization
    memcpy(&router->mMacAddr, &macAddr, sizeof(router->mMacAddr));
    router->mValid.mRloc16 = sourceAddress.GetRloc16();
    router->mValid.mLinkFrameCounter = linkFrameCounter.GetFrameCounter();
    router->mValid.mMleFrameCounter = mleFrameCounter.GetFrameCounter();
    router->mLastHeard = Timer::GetNow();
    router->mMode = ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData;
    router->mLinkInfo.Clear();
    router->mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
    router->mLinkFailures = 0;
    router->mState = Neighbor::kStateValid;
    router->mKeySequence = aKeySequence;

    if (aRequest)
    {
        // Challenge
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
        VerifyOrExit(challenge.IsValid(), error = kThreadError_Parse);

        // TLV Request
        if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == kThreadError_None)
        {
            VerifyOrExit(tlvRequest.IsValid(), error = kThreadError_Parse);
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
        if (mChildren[i].mState == Neighbor::kStateInvalid)
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
        if (mChildren[i].mState != Neighbor::kStateInvalid &&
            GetChildId(mChildren[i].mValid.mRloc16) == aChildId)
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
        if (mChildren[i].mState != Neighbor::kStateInvalid &&
            memcmp(&mChildren[i].mMacAddr, &aAddress, sizeof(mChildren[i].mMacAddr)) == 0)
        {
            ExitNow(rval = &mChildren[i]);
        }
    }

exit:
    return rval;
}

uint8_t MleRouter::LqiToCost(uint8_t aLqi)
{
    switch (aLqi)
    {
    case 1:
        return kLqi1LinkCost;

    case 2:
        return kLqi2LinkCost;

    case 3:
        return kLqi3LinkCost;

    default:
        return kLqi0LinkCost;
    }
}

uint8_t MleRouter::GetLinkCost(uint8_t aRouterId)
{
    uint8_t rval = kMaxRouteCost;
    Router *router;

    router = GetRouter(aRouterId);

    // NULL aRouterId indicates non-existing next hop, hence return kMaxRouteCost for it.
    VerifyOrExit(aRouterId != mRouterId && router != NULL && router->mState == Neighbor::kStateValid,);

    rval = router->mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor());

    if (rval > router->mLinkQualityOut)
    {
        rval = router->mLinkQualityOut;
    }

    // add for certification testing
    if (isAssignLinkQuality && (memcmp(router->mMacAddr.m8, mAddr64.m8, OT_EXT_ADDRESS_SIZE) == 0))
    {
        rval = mAssignLinkQuality;
    }

    rval = LqiToCost(rval);

exit:
    return rval;
}

ThreadError MleRouter::ProcessRouteTlv(const RouteTlv &aRoute)
{
    ThreadError error = kThreadError_None;

    mRouterIdSequence = aRoute.GetRouterIdSequence();
    mRouterIdSequenceLastUpdated = Timer::GetNow();

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        bool old = mRouters[i].mAllocated;
        mRouters[i].mAllocated = aRoute.IsRouterIdSet(i);

        if (old && !mRouters[i].mAllocated)
        {
            mRouters[i].mNextHop = kInvalidRouterId;
            mAddressResolver.Remove(i);
        }
    }

    if (GetDeviceState() == kDeviceStateRouter && !mRouters[mRouterId].mAllocated)
    {
        BecomeDetached();
        ExitNow(error = kThreadError_NoRoute);
    }

exit:
    return error;
}

bool MleRouter::IsSingleton(void)
{
    bool rval = true;

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        ExitNow(rval = true);
        break;

    case kDeviceStateChild:
        ExitNow(rval = ((mDeviceMode & ModeTlv::kModeFFD) == 0));
        break;

    case kDeviceStateRouter:
        ExitNow(rval = false);
        break;

    case kDeviceStateLeader:

        // not a singleton if any other routers exist
        for (int i = 0; i <= kMaxRouterId; i++)
        {
            if (i != mRouterId && mRouters[i].mAllocated)
            {
                ExitNow(rval = false);
            }
        }

        // not a singleton if any children are REEDs
        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].mState == Neighbor::kStateValid && (mChildren[i].mMode & ModeTlv::kModeFFD))
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
        if (mRouters[i].mAllocated)
        {
            rval++;
        }
    }

    return rval;
}

ThreadError MleRouter::HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
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
    VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

    // Remove stale neighbors
    if ((neighbor = GetNeighbor(macAddr)) != NULL &&
        neighbor->mValid.mRloc16 != sourceAddress.GetRloc16())
    {
        RemoveNeighbor(*neighbor);
    }

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

    // Route Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route));
    VerifyOrExit(route.IsValid(), error = kThreadError_Parse);

    partitionId = leaderData.GetPartitionId();

    if (partitionId != mLeaderData.GetPartitionId())
    {
        otLogDebgMle("different partition! %d %d %d %d",
                     leaderData.GetWeighting(), partitionId,
                     mLeaderData.GetWeighting(), mLeaderData.GetPartitionId());

        if (GetDeviceState() == kDeviceStateChild &&
            (memcmp(&mParent.mMacAddr, &macAddr, sizeof(mParent.mMacAddr)) == 0 || !(mDeviceMode & ModeTlv::kModeFFD)))
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
            otLogDebgMle("trying to migrate");
            BecomeChild(kMleAttachBetterPartition);
        }

        ExitNow(error = kThreadError_Drop);
    }
    else if (leaderData.GetLeaderRouterId() != GetLeaderId())
    {
        if (GetDeviceState() != kDeviceStateChild)
        {
            BecomeDetached();
            error = kThreadError_Drop;
        }

        ExitNow();
    }

    VerifyOrExit(IsActiveRouter(sourceAddress.GetRloc16()), ;);
    routerId = GetRouterId(sourceAddress.GetRloc16());
    router = GetRouter(routerId);
    VerifyOrExit(router != NULL, error = kThreadError_Parse);

    if ((mDeviceMode & ModeTlv::kModeFFD) &&
        static_cast<int8_t>(route.GetRouterIdSequence() - mRouterIdSequence) > 0)
    {
        bool processRouteTlv = false;

        switch (mDeviceState)
        {
        case kDeviceStateDisabled:
        case kDeviceStateDetached:
            break;

        case kDeviceStateChild:
            if ((sourceAddress.GetRloc16() == mParent.mValid.mRloc16) ||
                (router->mState == Neighbor::kStateValid))
            {
                processRouteTlv = true;
            }

            break;

        case kDeviceStateRouter:
        case kDeviceStateLeader:
            processRouteTlv = true;
            break;
        }

        if (processRouteTlv)
        {
            SuccessOrExit(error = ProcessRouteTlv(route));
        }
    }

    switch (GetDeviceState())
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        ExitNow();

    case kDeviceStateChild:
        if ((sourceAddress.GetRloc16() == mParent.mValid.mRloc16 || router->mState == Neighbor::kStateValid) &&
            (mDeviceMode & ModeTlv::kModeFFD) &&
            (mRouterSelectionJitterTimeout == 0) &&
            (GetActiveRouterCount() < mRouterUpgradeThreshold))
        {
            mRouterSelectionJitterTimeout = (otPlatRandomGet() % mRouterSelectionJitter) + 1;
            ExitNow();
        }

        if (memcmp(&mParent.mMacAddr, &macAddr, sizeof(mParent.mMacAddr)) == 0)
        {
            router = &mParent;

            if (mParent.mValid.mRloc16 != sourceAddress.GetRloc16())
            {
                SetStateDetached();
                ExitNow(error = kThreadError_NoRoute);
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
                        mRouters[GetLeaderId()].mNextHop = routerId;
                    }
                    else
                    {
                        mRouters[GetLeaderId()].mNextHop = kInvalidRouterId;
                    }

                    break;
                }
            }
        }
        else if ((mDeviceMode & ModeTlv::kModeFFD) && (router->mState != Neighbor::kStateValid))
        {
            memcpy(&router->mMacAddr, &macAddr, sizeof(router->mMacAddr));
            router->mLinkInfo.Clear();
            router->mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
            router->mLinkFailures = 0;
            router->mState = Neighbor::kStateLinkRequest;
            SendLinkRequest(router);
            ExitNow(error = kThreadError_NoRoute);
        }

        router->mLastHeard = Timer::GetNow();

        ExitNow();

    case kDeviceStateRouter:
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

    case kDeviceStateLeader:

        // router is not in list, reject
        if (!router->mAllocated)
        {
            ExitNow(error = kThreadError_NoRoute);
        }

        // Send link request if no link to router
        if (router->mState != Neighbor::kStateValid)
        {
            memcpy(&router->mMacAddr, &macAddr, sizeof(router->mMacAddr));
            router->mLinkInfo.Clear();
            router->mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
            router->mLinkFailures = 0;
            router->mState = Neighbor::kStateLinkRequest;
            router->mDataRequest = false;
            SendLinkRequest(router);
            ExitNow(error = kThreadError_NoRoute);
        }

        router->mLastHeard = Timer::GetNow();
        break;
    }

    UpdateRoutes(route, routerId);

    mNetif.GetNetworkDataLocal().SendServerDataNotification();

exit:
    return error;
}

void MleRouter::UpdateRoutes(const RouteTlv &aRoute, uint8_t aRouterId)
{
    uint8_t curCost;
    uint8_t newCost;
    uint8_t oldNextHop;
    uint8_t cost;
    uint8_t lqi;
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

            if (mRouters[i].mAllocated == false)
            {
                routeCount++;
                continue;
            }

            if (i == mRouterId)
            {
                lqi = aRoute.GetLinkQualityIn(routeCount);

                if (mRouters[aRouterId].mLinkQualityOut != lqi)
                {
                    mRouters[aRouterId].mLinkQualityOut = lqi;
                    update = true;
                }
            }
            else
            {
                oldNextHop = mRouters[i].mNextHop;

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

                if (!IsRouterIdValid(mRouters[i].mNextHop) || mRouters[i].mNextHop == aRouterId)
                {
                    // route has no nexthop or nexthop is neighbor
                    newCost = cost + GetLinkCost(aRouterId);

                    if (i == aRouterId)
                    {
                        if (!IsRouterIdValid(mRouters[i].mNextHop))
                        {
                            ResetAdvertiseInterval();
                        }

                        mRouters[i].mNextHop = aRouterId;
                        mRouters[i].mCost = 0;
                    }
                    else if (newCost <= kMaxRouteCost)
                    {
                        if (!IsRouterIdValid(mRouters[i].mNextHop))
                        {
                            ResetAdvertiseInterval();
                        }

                        mRouters[i].mNextHop = aRouterId;
                        mRouters[i].mCost = cost;
                    }
                    else if (IsRouterIdValid(mRouters[i].mNextHop))
                    {
                        ResetAdvertiseInterval();
                        mRouters[i].mNextHop = kInvalidRouterId;
                        mRouters[i].mCost = 0;
                        mRouters[i].mLastHeard = Timer::GetNow();
                    }
                }
                else
                {
                    curCost = mRouters[i].mCost + GetLinkCost(mRouters[i].mNextHop);
                    newCost = cost + GetLinkCost(aRouterId);

                    if (newCost < curCost || (newCost == curCost && i == aRouterId))
                    {
                        mRouters[i].mNextHop = aRouterId;
                        mRouters[i].mCost = cost;
                    }
                }

                update |= mRouters[i].mNextHop != oldNextHop;
            }

            routeCount++;
        }
    }
    while (update);

#if 1

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mAllocated == false || !IsRouterIdValid(mRouters[i].mNextHop))
        {
            continue;
        }

        otLogDebgMle("%x: %x %d %d %d %d", GetRloc16(i), GetRloc16(mRouters[i].mNextHop),
                     mRouters[i].mCost, GetLinkCost(i), mRouters[i].mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor()),
                     mRouters[i].mLinkQualityOut);
    }

#endif
}

ThreadError MleRouter::HandleParentRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    Mac::ExtAddress macAddr;
    VersionTlv version;
    ScanMaskTlv scanMask;
    ChallengeTlv challenge;
    Child *child;

    otLogInfoMle("Received parent request");

    // A Router MUST NOT send an MLE Parent Response if:

    // 1. It has no available Child capacity (if Max Child Count minus
    // Child Count would be equal to zero)
    // ==> verified below when allocating a child entry

    // 2. It is disconnected from its Partition (that is, it has not
    // received an updated ID sequence number within LEADER_TIMEOUT
    // seconds
    VerifyOrExit(GetLeaderAge() < mNetworkIdTimeout, error = kThreadError_Drop);

    // 3. Its current routing path cost to the Leader is infinite.
    VerifyOrExit(IsRouterIdValid(mRouters[GetLeaderId()].mNextHop), error = kThreadError_Drop);

    macAddr.Set(aMessageInfo.GetPeerAddr());

    // Version
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kVersion, sizeof(version), version));
    VerifyOrExit(version.IsValid() && version.GetVersion() == kVersion, error = kThreadError_Parse);

    // Scan Mask
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kScanMask, sizeof(scanMask), scanMask));
    VerifyOrExit(scanMask.IsValid(), error = kThreadError_Parse);

    switch (GetDeviceState())
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        ExitNow();

    case kDeviceStateChild:
        VerifyOrExit(scanMask.IsEndDeviceFlagSet(), ;);
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        VerifyOrExit(scanMask.IsRouterFlagSet(), ;);
        break;
    }

    if ((child = FindChild(macAddr)) != NULL)
    {
        RemoveNeighbor(*child);
    }
    else
    {
        VerifyOrExit((child = NewChild()) != NULL, ;);
    }

    memset(child, 0, sizeof(*child));

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = kThreadError_Parse);

    // MAC Address
    memcpy(&child->mMacAddr, &macAddr, sizeof(child->mMacAddr));
    child->mLinkInfo.Clear();
    child->mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
    child->mLinkFailures = 0;
    child->mState = Neighbor::kStateParentRequest;
    child->mDataRequest = false;

    child->mLastHeard = Timer::GetNow();
    child->mTimeout = Timer::MsecToSec(kMaxChildIdRequestTimeout);
    SuccessOrExit(error = SendParentResponse(child, challenge, !scanMask.IsEndDeviceFlagSet()));

exit:
    return error;
}

void MleRouter::HandleStateUpdateTimer(void *aContext)
{
    static_cast<MleRouter *>(aContext)->HandleStateUpdateTimer();
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

    switch (GetDeviceState())
    {
    case kDeviceStateDisabled:
        assert(false);
        break;

    case kDeviceStateDetached:
        SetStateDetached();
        BecomeChild(kMleAttachAnyPartition);
        ExitNow();

    case kDeviceStateChild:
        if (routerStateUpdate && GetActiveRouterCount() < mRouterUpgradeThreshold)
        {
            // upgrade to Router
            BecomeRouter(ThreadStatusTlv::kTooFewRouters);
            ExitNow();
        }

    // fall through

    case kDeviceStateRouter:
        // verify path to leader
        otLogDebgMle("network id timeout = %d", GetLeaderAge());

        if (GetLeaderAge() >= mNetworkIdTimeout)
        {
            BecomeChild(kMleAttachSamePartition);
        }

        if (routerStateUpdate && GetActiveRouterCount() > mRouterDowngradeThreshold)
        {
            // downgrade to REED
            BecomeChild(kMleAttachSamePartition);
        }

        break;

    case kDeviceStateLeader:

        // update router id sequence
        if (GetLeaderAge() >= kRouterIdSequencePeriod)
        {
            mRouterIdSequence++;
            mRouterIdSequenceLastUpdated = Timer::GetNow();
        }

        break;
    }

    // update children state
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        uint32_t timeout = 0;

        switch (mChildren[i].mState)
        {
        case Neighbor::kStateInvalid:
        case Neighbor::kStateChildIdRequest:
            continue;

        case Neighbor::kStateParentRequest:
        case Neighbor::kStateValid:
        case Neighbor::kStateRestored:
        case Neighbor::kStateChildUpdateRequest:
            timeout = Timer::SecToMsec(mChildren[i].mTimeout);
            break;

        case Neighbor::kStateLinkRequest:
            assert(false);
            break;
        }

        if ((Timer::GetNow() - mChildren[i].mLastHeard) >= timeout)
        {
            RemoveNeighbor(mChildren[i]);
        }
    }

    // update router state
    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mState == Neighbor::kStateValid)
        {
            if ((Timer::GetNow() - mRouters[i].mLastHeard) >= Timer::SecToMsec(kMaxNeighborAge))
            {
                mRouters[i].mState = Neighbor::kStateInvalid;
                mRouters[i].mLinkInfo.Clear();
                mRouters[i].mNextHop = kInvalidRouterId;
                mRouters[i].mLinkQualityOut = 0;
                mRouters[i].mLastHeard = Timer::GetNow();
            }
        }

        if (GetDeviceState() == kDeviceStateLeader)
        {
            if (mRouters[i].mAllocated)
            {
                if (!IsRouterIdValid(mRouters[i].mNextHop) &&
                    (Timer::GetNow() - mRouters[i].mLastHeard) >= Timer::SecToMsec(kMaxLeaderToRouterTimeout))
                {
                    ReleaseRouterId(i);
                }
            }
            else if (mRouters[i].mReclaimDelay)
            {
                if ((Timer::GetNow() - mRouters[i].mLastHeard) >=
                    Timer::SecToMsec((kMaxLeaderToRouterTimeout + kRouterIdReuseDelay)))
                {
                    mRouters[i].mReclaimDelay = false;
                }
            }
        }
    }

    mStateUpdateTimer.Start(kStateUpdatePeriod);

exit:
    {}
}

void MleRouter::HandleDelayedResponseTimer(void *aContext)
{
    static_cast<MleRouter *>(aContext)->HandleDelayedResponseTimer();
}

void MleRouter::HandleDelayedResponseTimer(void)
{
    DelayedResponseHeader delayedResponse;
    uint32_t now = otPlatAlarmGetNow();
    uint32_t nextDelay = 0xffffffff;
    Message *message = mDelayedResponses.GetHead();
    Message *nextMessage = NULL;

    while (message != NULL)
    {
        nextMessage = message->GetNext();
        delayedResponse.ReadFrom(*message);

        if (delayedResponse.IsLater(now))
        {
            // Calculate the next delay and choose the lowest.
            if (delayedResponse.GetSendTime() - now < nextDelay)
            {
                nextDelay = delayedResponse.GetSendTime() - now;
            }
        }
        else
        {
            mDelayedResponses.Dequeue(*message);

            // Remove the DelayedResponseHeader from the message.
            DelayedResponseHeader::RemoveFrom(*message);

            // Send the message.
            if (SendMessage(*message, delayedResponse.GetDestination()) == kThreadError_None)
            {
                otLogInfoMle("Sent delayed response");
            }
            else
            {
                message->Free();
            }
        }

        message = nextMessage;
    }

    if (nextDelay != 0xffffffff)
    {
        mDelayedResponseTimer.Start(nextDelay);
    }
}

void MleRouter::HandleChildUpdateRequestTimer(void *aContext)
{
    static_cast<MleRouter *>(aContext)->HandleChildUpdateRequestTimer();
}

void MleRouter::HandleChildUpdateRequestTimer(void)
{
    VerifyOrExit(GetDeviceState() == kDeviceStateRouter || GetDeviceState() == kDeviceStateLeader, ;);

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].mState == Neighbor::kStateRestored)
        {
            SendChildUpdateRequest(&mChildren[i]);
            mChildren[i].mState = Neighbor::kStateChildUpdateRequest;

            if ((mChildren[i].mMode & ModeTlv::kModeRxOnWhenIdle) != 0)
            {
                mChildren[i].mTimeout = Timer::MsecToSec(kMaxChildUpdateResponseTimeout);
            }

            mChildUpdateRequestTimer.Start(kChildUpdateRequestPeriod);
            break;
        }
    }

exit:
    {}
}

ThreadError MleRouter::AddDelayedResponse(Message &aMessage, const Ip6::Address &aDestination, uint16_t aDelay)
{
    ThreadError error = kThreadError_None;
    uint32_t alarmFireTime;
    uint32_t sendTime = otPlatAlarmGetNow() + aDelay;

    // Append the message with DelayedRespnoseHeader and add to the list.
    DelayedResponseHeader delayedResponse(sendTime, aDestination);
    SuccessOrExit(error = delayedResponse.AppendTo(aMessage));
    mDelayedResponses.Enqueue(aMessage);

    if (mDelayedResponseTimer.IsRunning())
    {
        // If timer is already running, check if it should be restarted with earlier fire time.
        alarmFireTime = mDelayedResponseTimer.Gett0() + mDelayedResponseTimer.Getdt();

        if (delayedResponse.IsEarlier(alarmFireTime))
        {
            mDelayedResponseTimer.Start(aDelay);
        }
    }
    else
    {
        // Otherwise just set the timer.
        mDelayedResponseTimer.Start(aDelay);
    }

exit:
    return error;
}

ThreadError MleRouter::SendParentResponse(Child *aChild, const ChallengeTlv &challenge, bool aRoutersOnlyRequest)
{
    ThreadError error = kThreadError_None;
    Ip6::Address destination;
    Message *message;
    uint16_t delay;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandParentResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));
    SuccessOrExit(error = AppendResponse(*message, challenge.GetChallenge(), challenge.GetLength()));

    for (uint8_t i = 0; i < sizeof(aChild->mPending.mChallenge); i++)
    {
        aChild->mPending.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    SuccessOrExit(error = AppendChallenge(*message, aChild->mPending.mChallenge, sizeof(aChild->mPending.mChallenge)));

    if (isAssignLinkQuality &&
        (memcmp(mAddr64.m8, aChild->mMacAddr.m8, OT_EXT_ADDRESS_SIZE) == 0))
    {
        // use assigned one to ensure the link quality
        SuccessOrExit(error = AppendLinkMargin(*message, mAssignLinkMargin));
    }
    else
    {
        SuccessOrExit(error = AppendLinkMargin(*message, aChild->mLinkInfo.GetLinkMargin(mMac.GetNoiseFloor())));
    }

    SuccessOrExit(error = AppendConnectivity(*message));
    SuccessOrExit(error = AppendVersion(*message));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild->mMacAddr);

    if (aRoutersOnlyRequest)
    {
        delay = (otPlatRandomGet() % kParentResponseMaxDelayRouters) + 1;
    }
    else
    {
        delay = (otPlatRandomGet() % kParentResponseMaxDelayAll) + 1;
    }

    SuccessOrExit(error = AddDelayedResponse(*message, destination, delay));

    otLogInfoMle("Delayed Parent Response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return kThreadError_None;
}

ThreadError MleRouter::UpdateChildAddresses(const AddressRegistrationTlv &aTlv, Child &aChild)
{
    const AddressRegistrationEntry *entry;
    Lowpan::Context context;

    memset(aChild.mIp6Address, 0, sizeof(aChild.mIp6Address));

    for (uint8_t count = 0; count < sizeof(aChild.mIp6Address) / sizeof(aChild.mIp6Address[0]); count++)
    {
        if ((entry = aTlv.GetAddressEntry(count)) == NULL)
        {
            break;
        }

        if (entry->IsCompressed())
        {
            // xxx check if context id exists
            mNetworkData.GetContext(entry->GetContextId(), context);
            memcpy(&aChild.mIp6Address[count], context.mPrefix, BitVectorBytes(context.mPrefixLength));
            aChild.mIp6Address[count].SetIid(entry->GetIid());
        }
        else
        {
            memcpy(&aChild.mIp6Address[count], entry->GetIp6Address(), sizeof(aChild.mIp6Address[count]));
        }
    }

    return kThreadError_None;
}

ThreadError MleRouter::HandleChildIdRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                            uint32_t aKeySequence)
{
    ThreadError error = kThreadError_None;
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

    otLogInfoMle("Received Child ID Request");

    // Find Child
    macAddr.Set(aMessageInfo.GetPeerAddr());

    VerifyOrExit((child = FindChild(macAddr)) != NULL, ;);

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid() &&
                 memcmp(response.GetResponse(), child->mPending.mChallenge, sizeof(child->mPending.mChallenge)) == 0, ;);

    // Link-Layer Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter),
                                      linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = kThreadError_Parse);

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) ==
        kThreadError_None)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), error = kThreadError_Parse);
    }
    else
    {
        mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    // Mode
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMode, sizeof(mode), mode));
    VerifyOrExit(mode.IsValid(), error = kThreadError_Parse);

    // Timeout
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout));
    VerifyOrExit(timeout.IsValid(), error = kThreadError_Parse);

    // Ip6 Address
    address.SetLength(0);

    if ((mode.GetMode() & ModeTlv::kModeFFD) == 0)
    {
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kAddressRegistration, sizeof(address), address));
        VerifyOrExit(address.IsValid(), error = kThreadError_Parse);
    }

    // TLV Request
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest));
    VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= sizeof(child->mRequestTlvs),
                 error = kThreadError_Parse);

    // Active Timestamp
    activeTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == kThreadError_None)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = kThreadError_Parse);
    }

    // Pending Timestamp
    pendingTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == kThreadError_None)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = kThreadError_Parse);
    }

    // Remove from router table
    for (int i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mState != Neighbor::kStateInvalid &&
            memcmp(&mRouters[i].mMacAddr, &macAddr, sizeof(mRouters[i].mMacAddr)) == 0)
        {
            RemoveNeighbor(mRouters[i]);
            break;
        }
    }

    if (child->mState != Neighbor::kStateValid)
    {
        child->mState = Neighbor::kStateChildIdRequest;
    }

    child->mLastHeard = Timer::GetNow();
    child->mValid.mLinkFrameCounter = linkFrameCounter.GetFrameCounter();
    child->mValid.mMleFrameCounter = mleFrameCounter.GetFrameCounter();
    child->mKeySequence = aKeySequence;
    child->mMode = mode.GetMode();
    child->mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
    child->mTimeout = timeout.GetTimeout();

    if (mode.GetMode() & ModeTlv::kModeFullNetworkData)
    {
        child->mNetworkDataVersion = mLeaderData.GetDataVersion();
    }
    else
    {
        child->mNetworkDataVersion = mLeaderData.GetStableDataVersion();
    }

    UpdateChildAddresses(address, *child);

    memset(child->mRequestTlvs, Tlv::kInvalid, sizeof(child->mRequestTlvs));
    memcpy(child->mRequestTlvs, tlvRequest.GetTlvs(), tlvRequest.GetLength());
    numTlvs = tlvRequest.GetLength();

    if (activeTimestamp.GetLength() == 0 ||
        mNetif.GetActiveDataset().GetNetwork().GetTimestamp() == NULL ||
        mNetif.GetActiveDataset().GetNetwork().GetTimestamp()->Compare(activeTimestamp) != 0)
    {
        child->mRequestTlvs[numTlvs++] = Tlv::kActiveDataset;
    }

    if (pendingTimestamp.GetLength() == 0 ||
        mNetif.GetPendingDataset().GetNetwork().GetTimestamp() == NULL ||
        mNetif.GetPendingDataset().GetNetwork().GetTimestamp()->Compare(pendingTimestamp) != 0)
    {
        child->mRequestTlvs[numTlvs++] = Tlv::kPendingDataset;
    }

    switch (GetDeviceState())
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        assert(false);
        break;

    case kDeviceStateChild:
        child->mState = Neighbor::kStateChildIdRequest;
        BecomeRouter(ThreadStatusTlv::kHaveChildIdRequest);
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        SuccessOrExit(error = SendChildIdResponse(child));
        break;
    }

exit:
    return error;
}

ThreadError MleRouter::HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static const uint8_t kMaxResponseTlvs = 8;

    ThreadError error = kThreadError_None;
    Mac::ExtAddress macAddr;
    ModeTlv mode;
    ChallengeTlv challenge;
    AddressRegistrationTlv address;
    LeaderDataTlv leaderData;
    TimeoutTlv timeout;
    Child *child;
    uint8_t tlvs[kMaxResponseTlvs];
    uint8_t tlvslength = 0;

    otLogInfoMle("Received Child Update Request from child");

    // Find Child
    macAddr.Set(aMessageInfo.GetPeerAddr());

    child = FindChild(macAddr);

    if (child == NULL)
    {
        tlvs[tlvslength++] = Tlv::kStatus;
        SendChildUpdateResponse(NULL, aMessageInfo, tlvs, tlvslength, NULL);
        ExitNow();
    }

    tlvs[tlvslength++] = Tlv::kSourceAddress;
    tlvs[tlvslength++] = Tlv::kLeaderData;

    // Mode
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMode, sizeof(mode), mode));
    VerifyOrExit(mode.IsValid(), error = kThreadError_Parse);
    child->mMode = mode.GetMode();
    tlvs[tlvslength++] = Tlv::kMode;

    // Challenge
    if (Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge) == kThreadError_None)
    {
        VerifyOrExit(challenge.IsValid(), error = kThreadError_Parse);
        tlvs[tlvslength++] = Tlv::kResponse;
    }

    // Ip6 Address TLV
    if (Tlv::GetTlv(aMessage, Tlv::kAddressRegistration, sizeof(address), address) == kThreadError_None)
    {
        VerifyOrExit(address.IsValid(), error = kThreadError_Parse);
        UpdateChildAddresses(address, *child);
        tlvs[tlvslength++] = Tlv::kAddressRegistration;
    }

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == kThreadError_None)
    {
        VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

        if (child->mMode & ModeTlv::kModeFullNetworkData)
        {
            // full network data
            child->mNetworkDataVersion = leaderData.GetDataVersion();

            if (leaderData.GetDataVersion() != mNetworkData.GetVersion())
            {
                tlvs[tlvslength++] = Tlv::kNetworkData;
            }
        }
        else
        {
            // stable network data
            child->mNetworkDataVersion = leaderData.GetStableDataVersion();

            if (leaderData.GetStableDataVersion() != mNetworkData.GetStableVersion())
            {
                tlvs[tlvslength++] = Tlv::kNetworkData;
            }
        }
    }
    else
    {
        tlvs[tlvslength++] = Tlv::kNetworkData;
    }

    // Timeout
    if (Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout) == kThreadError_None)
    {
        VerifyOrExit(timeout.IsValid(), error = kThreadError_Parse);
        child->mTimeout = timeout.GetTimeout();
        tlvs[tlvslength++] = Tlv::kTimeout;
    }

    child->mLastHeard = Timer::GetNow();
    child->mAddSrcMatchEntryShort = true;
    child->mState = Neighbor::kStateValid;

    SendChildUpdateResponse(child, aMessageInfo, tlvs, tlvslength, &challenge);

exit:
    return error;
}

ThreadError MleRouter::HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                                 uint32_t aKeySequence)
{
    ThreadError error = kThreadError_None;
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

    otLogInfoMle("Received Child Update Response from child");

    // Find Child
    macAddr.Set(aMessageInfo.GetPeerAddr());

    VerifyOrExit((child = FindChild(macAddr)) != NULL, error = kThreadError_NotFound);

    // Source Address
    if (Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress) == kThreadError_None)
    {
        VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);
        VerifyOrExit(child->mValid.mRloc16 == sourceAddress.GetRloc16(), error = kThreadError_Parse);
    }

    // Response
    if (Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response) == kThreadError_None)
    {
        VerifyOrExit(response.IsValid() &&
                     memcmp(response.GetResponse(), child->mPending.mChallenge, sizeof(child->mPending.mChallenge)) == 0, ;);
    }

    // Link-Layer Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter), linkFrameCounter) == kThreadError_None)
    {
        VerifyOrExit(linkFrameCounter.IsValid(), error = kThreadError_Parse);
        child->mValid.mLinkFrameCounter = linkFrameCounter.GetFrameCounter();
    }

    // MLE Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) == kThreadError_None)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), error = kThreadError_Parse);
        child->mValid.mMleFrameCounter = mleFrameCounter.GetFrameCounter();
    }

    // Timeout
    if (Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout) == kThreadError_None)
    {
        VerifyOrExit(timeout.IsValid(), error = kThreadError_Parse);
        child->mTimeout = timeout.GetTimeout();
    }

    // Ip6 Address
    if (Tlv::GetTlv(aMessage, Tlv::kAddressRegistration, sizeof(address), address) == kThreadError_None)
    {
        VerifyOrExit(address.IsValid(), error = kThreadError_Parse);
        UpdateChildAddresses(address, *child);
    }

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == kThreadError_None)
    {
        VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

        if (child->mMode & ModeTlv::kModeFullNetworkData)
        {
            child->mNetworkDataVersion = leaderData.GetDataVersion();
        }
        else
        {
            child->mNetworkDataVersion = leaderData.GetStableDataVersion();
        }
    }

    child->mLastHeard = Timer::GetNow();
    child->mKeySequence = aKeySequence;
    child->mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
    child->mAddSrcMatchEntryShort = true;
    child->mState = Neighbor::kStateValid;

exit:
    return error;
}

ThreadError MleRouter::HandleDataRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    TlvRequestTlv tlvRequest;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    uint8_t tlvs[4];
    uint8_t numTlvs;

    otLogInfoMle("Received Data Request");

    // TLV Request
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest));
    VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= sizeof(tlvs), error = kThreadError_Parse);

    // Active Timestamp
    activeTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == kThreadError_None)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = kThreadError_Parse);
    }

    // Pending Timestamp
    pendingTimestamp.SetLength(0);

    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == kThreadError_None)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = kThreadError_Parse);
    }

    memset(tlvs, Tlv::kInvalid, sizeof(tlvs));
    memcpy(tlvs, tlvRequest.GetTlvs(), tlvRequest.GetLength());
    numTlvs = tlvRequest.GetLength();

    if (activeTimestamp.GetLength() == 0 ||
        mNetif.GetActiveDataset().GetNetwork().GetTimestamp() == NULL ||
        mNetif.GetActiveDataset().GetNetwork().GetTimestamp()->Compare(activeTimestamp) != 0)
    {
        tlvs[numTlvs++] = Tlv::kActiveDataset;
    }

    if (pendingTimestamp.GetLength() == 0 ||
        mNetif.GetPendingDataset().GetNetwork().GetTimestamp() == NULL ||
        mNetif.GetPendingDataset().GetNetwork().GetTimestamp()->Compare(pendingTimestamp) != 0)
    {
        tlvs[numTlvs++] = Tlv::kPendingDataset;
    }

    SendDataResponse(aMessageInfo.GetPeerAddr(), tlvs, numTlvs);

exit:
    return error;
}

ThreadError MleRouter::HandleNetworkDataUpdateRouter(void)
{
    static const uint8_t tlvs[] = {Tlv::kNetworkData};
    Ip6::Address destination;

    VerifyOrExit(mDeviceState == kDeviceStateRouter || mDeviceState == kDeviceStateLeader, ;);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);

    SendDataResponse(destination, tlvs, sizeof(tlvs));

    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        Child *child = &mChildren[i];

        if (child->mState != Neighbor::kStateValid || (child->mMode & ModeTlv::kModeRxOnWhenIdle) != 0)
        {
            continue;
        }

        memset(&destination, 0, sizeof(destination));
        destination.mFields.m16[0] = HostSwap16(0xfe80);
        destination.SetIid(child->mMacAddr);

        if (child->mMode & ModeTlv::kModeFullNetworkData)
        {
            if (child->mNetworkDataVersion != mNetworkData.GetVersion())
            {
                SendDataResponse(destination, tlvs, sizeof(tlvs));
            }
        }
        else
        {
            if (child->mNetworkDataVersion != mNetworkData.GetStableVersion())
            {
                static const uint8_t responseTlvs[] = {Tlv::kNetworkData, Tlv::kActiveDataset, Tlv::kPendingDataset};

                SendDataResponse(destination, responseTlvs, sizeof(responseTlvs));
            }
        }
    }

exit:
    return kThreadError_None;
}

ThreadError MleRouter::HandleDiscoveryRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Tlv tlv;
    MeshCoP::Tlv meshcopTlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    MeshCoP::ExtendedPanIdTlv extPanId;
    uint16_t offset;
    uint16_t end;

    otLogInfoMle("Received discovery request");

    // only Routers and REEDs respond
    VerifyOrExit((mDeviceMode & ModeTlv::kModeFFD) != 0, ;);

    offset = aMessage.GetOffset();
    end = aMessage.GetLength();

    // find MLE Discovery TLV
    while (offset < end)
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);

        if (tlv.GetType() == Tlv::kDiscovery)
        {
            break;
        }

        offset += sizeof(tlv) + tlv.GetLength();
    }

    VerifyOrExit(offset < end, error = kThreadError_Parse);

    offset += sizeof(tlv);
    end = offset + sizeof(tlv) + tlv.GetLength();

    while (offset < end)
    {
        aMessage.Read(offset, sizeof(meshcopTlv), &meshcopTlv);

        switch (meshcopTlv.GetType())
        {
        case MeshCoP::Tlv::kDiscoveryRequest:
            aMessage.Read(offset, sizeof(discoveryRequest), &discoveryRequest);
            VerifyOrExit(discoveryRequest.IsValid(), error = kThreadError_Parse);
            break;

        case MeshCoP::Tlv::kExtendedPanId:
            aMessage.Read(offset, sizeof(extPanId), &extPanId);
            VerifyOrExit(extPanId.IsValid(), error = kThreadError_Parse);
            VerifyOrExit(memcmp(mMac.GetExtendedPanId(), extPanId.GetExtendedPanId(), OT_EXT_PAN_ID_SIZE),
                         error = kThreadError_Drop);
            break;

        default:
            break;
        }

        offset += sizeof(meshcopTlv) + meshcopTlv.GetLength();
    }

    error = SendDiscoveryResponse(aMessageInfo.GetPeerAddr(), aMessage.GetPanId());

exit:

    if (error != kThreadError_None)
    {
        otLogWarnMleErr(error, "Failed to process Discovery Request");
    }

    return error;
}

ThreadError MleRouter::SendDiscoveryResponse(const Ip6::Address &aDestination, uint16_t aPanId)
{
    ThreadError error = kThreadError_None;
    Message *message;
    uint16_t startOffset;
    Tlv tlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::ExtendedPanIdTlv extPanId;
    MeshCoP::NetworkNameTlv networkName;
    MeshCoP::JoinerUdpPortTlv joinerUdpPort;
    MeshCoP::Tlv *steeringData;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
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

    if (mNetif.GetKeyManager().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_NATIVE_COMMISSIONING)
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
    extPanId.SetExtendedPanId(mMac.GetExtendedPanId());
    SuccessOrExit(error = message->Append(&extPanId, sizeof(extPanId)));

    // Network Name TLV
    networkName.Init();
    networkName.SetNetworkName(mMac.GetNetworkName());
    SuccessOrExit(error = message->Append(&networkName, sizeof(tlv) + networkName.GetLength()));

    // Steering Data TLV
    steeringData = mNetif.GetNetworkDataLeader().GetCommissioningDataSubTlv(MeshCoP::Tlv::kSteeringData);

    if (steeringData != NULL)
    {
        SuccessOrExit(message->Append(steeringData, sizeof(*steeringData) + steeringData->GetLength()));
    }

    // Joiner UDP Port TLV
    joinerUdpPort.Init();
    joinerUdpPort.SetUdpPort(mJoinerRouter.GetJoinerUdpPort());
    SuccessOrExit(error = message->Append(&joinerUdpPort, sizeof(tlv) + joinerUdpPort.GetLength()));

    tlv.SetLength(static_cast<uint8_t>(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle("Sent discovery response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError MleRouter::SendChildIdResponse(Child *aChild)
{
    ThreadError error = kThreadError_None;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildIdResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendActiveTimestamp(*message, false));
    SuccessOrExit(error = AppendPendingTimestamp(*message, false));

    if (aChild->mState != Neighbor::kStateValid)
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
        aChild->mValid.mRloc16 = mMac.GetShortAddress() | mNextChildId;
    }

    SuccessOrExit(error = AppendAddress16(*message, aChild->mValid.mRloc16));

    for (uint8_t i = 0; i < sizeof(aChild->mRequestTlvs); i++)
    {
        switch (aChild->mRequestTlvs[i])
        {
        case Tlv::kNetworkData:
            SuccessOrExit(error = AppendNetworkData(*message, (aChild->mMode & ModeTlv::kModeFullNetworkData) == 0));
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

    if ((aChild->mMode & ModeTlv::kModeFFD) == 0)
    {
        SuccessOrExit(error = AppendChildAddresses(*message, *aChild));
    }

    aChild->mState = Neighbor::kStateValid;
    mNetif.SetStateChangedFlags(OT_THREAD_CHILD_ADDED);
    StoreChild(aChild->mValid.mRloc16);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild->mMacAddr);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("Sent Child ID Response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return kThreadError_None;
}

ThreadError MleRouter::SendChildUpdateRequest(Child *aChild)
{
    static const uint8_t tlvs[] = {Tlv::kTimeout, Tlv::kAddressRegistration};
    ThreadError error = kThreadError_None;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateRequest));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendNetworkData(*message, (aChild->mMode & ModeTlv::kModeFullNetworkData) == 0));
    SuccessOrExit(error = AppendTlvRequest(*message, tlvs, sizeof(tlvs)));

    for (uint8_t i = 0; i < sizeof(aChild->mPending.mChallenge); i++)
    {
        aChild->mPending.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    SuccessOrExit(error = AppendChallenge(*message, aChild->mPending.mChallenge,
                                          sizeof(aChild->mPending.mChallenge)));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(aChild->mMacAddr);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("Sent Child Update Request to child");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError MleRouter::SendChildUpdateResponse(Child *aChild, const Ip6::MessageInfo &aMessageInfo,
                                               const uint8_t *aTlvs, uint8_t aTlvslength,
                                               const ChallengeTlv *aChallenge)
{
    ThreadError error = kThreadError_None;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
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
            SuccessOrExit(error = AppendMode(*message, aChild->mMode));
            break;

        case Tlv::kNetworkData:
            SuccessOrExit(error = AppendNetworkData(*message, (aChild->mMode & ModeTlv::kModeFullNetworkData) == 0));
            break;

        case Tlv::kResponse:
            SuccessOrExit(error = AppendResponse(*message, aChallenge->GetChallenge(), aChallenge->GetLength()));
            break;

        case Tlv::kSourceAddress:
            SuccessOrExit(error = AppendSourceAddress(*message));
            break;

        case Tlv::kTimeout:
            SuccessOrExit(error = AppendTimeout(*message, aChild->mTimeout));
            break;
        }
    }

    SuccessOrExit(error = SendMessage(*message, aMessageInfo.GetPeerAddr()));

    otLogInfoMle("Sent Child Update Response to child");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return kThreadError_None;
}

ThreadError MleRouter::SendDataResponse(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    ThreadError error = kThreadError_None;
    Message *message;
    Neighbor *neighbor;
    bool stableOnly;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDataResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));
    SuccessOrExit(error = AppendActiveTimestamp(*message, false));
    SuccessOrExit(error = AppendPendingTimestamp(*message, false));

    for (int i = 0; i < aTlvsLength; i++)
    {
        switch (aTlvs[i])
        {
        case Tlv::kNetworkData:
            neighbor = mMleRouter.GetNeighbor(aDestination);
            stableOnly = neighbor != NULL ? (neighbor->mMode & ModeTlv::kModeFullNetworkData) == 0 : false;
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

    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle("Sent Data Response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

Child *MleRouter::GetChild(uint16_t aAddress)
{
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].mState == Neighbor::kStateValid && mChildren[i].mValid.mRloc16 == aAddress)
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
        if (mChildren[i].mState == Neighbor::kStateValid &&
            memcmp(&mChildren[i].mMacAddr, &aAddress, sizeof(mChildren[i].mMacAddr)) == 0)
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

ThreadError MleRouter::SetMaxAllowedChildren(uint8_t aMaxChildren)
{
    ThreadError error = kThreadError_None;

    // Ensure the value is between 1 and kMaxChildren
    VerifyOrExit(aMaxChildren > 0 && aMaxChildren <= kMaxChildren, error = kThreadError_InvalidArgs);

    // Do not allow setting max children if MLE is running
    VerifyOrExit(GetDeviceState() == kDeviceStateDisabled, error = kThreadError_InvalidState);

    // Save the value
    mMaxChildrenAllowed = aMaxChildren;

exit:
    return error;
}

ThreadError MleRouter::RemoveNeighbor(const Mac::Address &aAddress)
{
    ThreadError error = kThreadError_None;
    Neighbor *neighbor;

    VerifyOrExit((neighbor = GetNeighbor(aAddress)) != NULL, error = kThreadError_NotFound);
    SuccessOrExit(error = RemoveNeighbor(*neighbor));

exit:
    return error;
}

ThreadError MleRouter::RemoveNeighbor(Neighbor &aNeighbor)
{
    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        break;

    case kDeviceStateChild:
        if (&aNeighbor == &mParent)
        {
            BecomeDetached();
        }

        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        if (aNeighbor.mState == Neighbor::kStateValid && !IsActiveRouter(aNeighbor.mValid.mRloc16))
        {
            aNeighbor.mState = Neighbor::kStateInvalid;
            mMesh.UpdateIndirectMessages();
            mNetif.SetStateChangedFlags(OT_THREAD_CHILD_REMOVED);
            mNetworkData.SendServerDataNotification(aNeighbor.mValid.mRloc16);
            RemoveStoredChild(aNeighbor.mValid.mRloc16);
        }

        break;
    }

    aNeighbor.mState = Neighbor::kStateInvalid;

    return kThreadError_None;
}

Neighbor *MleRouter::GetNeighbor(uint16_t aAddress)
{
    Neighbor *rval = NULL;

    if (aAddress == Mac::kShortAddrBroadcast || aAddress == Mac::kShortAddrInvalid)
    {
        ExitNow();
    }

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
        break;

    case kDeviceStateDetached:
    case kDeviceStateChild:
        rval = Mle::GetNeighbor(aAddress);
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].mState == Neighbor::kStateValid && mChildren[i].mValid.mRloc16 == aAddress)
            {
                ExitNow(rval = &mChildren[i]);
            }
        }

        for (int i = 0; i <= kMaxRouterId; i++)
        {
            if (mRouters[i].mState == Neighbor::kStateValid && mRouters[i].mValid.mRloc16 == aAddress)
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

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
        break;

    case kDeviceStateDetached:
    case kDeviceStateChild:
        rval = Mle::GetNeighbor(aAddress);
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        for (int i = 0; i < mMaxChildrenAllowed; i++)
        {
            if (mChildren[i].mState == Neighbor::kStateValid &&
                memcmp(&mChildren[i].mMacAddr, &aAddress, sizeof(mChildren[i].mMacAddr)) == 0)
            {
                ExitNow(rval = &mChildren[i]);
            }
        }

        for (int i = 0; i <= kMaxRouterId; i++)
        {
            if (mRouters[i].mState == Neighbor::kStateValid &&
                memcmp(&mRouters[i].mMacAddr, &aAddress, sizeof(mRouters[i].mMacAddr)) == 0)
            {
                ExitNow(rval = &mRouters[i]);
            }
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

    if (mNetworkData.GetContext(aAddress, context) != kThreadError_None)
    {
        context.mContextId = 0xff;
    }

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        child = &mChildren[i];

        if (child->mState != Neighbor::kStateValid)
        {
            continue;
        }

        if (context.mContextId == 0 &&
            aAddress.mFields.m16[4] == HostSwap16(0x0000) &&
            aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
            aAddress.mFields.m16[6] == HostSwap16(0xfe00) &&
            aAddress.mFields.m16[7] == HostSwap16(child->mValid.mRloc16))
        {
            ExitNow(rval = child);
        }

        for (int j = 0; j < Child::kMaxIp6AddressPerChild; j++)
        {
            if (memcmp(&child->mIp6Address[j], aAddress.mFields.m8, sizeof(child->mIp6Address[j])) == 0)
            {
                ExitNow(rval = child);
            }
        }
    }

    VerifyOrExit(context.mContextId == 0, rval = NULL);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        router = &mRouters[i];

        if (router->mState != Neighbor::kStateValid)
        {
            continue;
        }

        if (aAddress.mFields.m16[4] == HostSwap16(0x0000) &&
            aAddress.mFields.m16[5] == HostSwap16(0x00ff) &&
            aAddress.mFields.m16[6] == HostSwap16(0xfe00) &&
            aAddress.mFields.m16[7] == HostSwap16(router->mValid.mRloc16))
        {
            ExitNow(rval = router);
        }
    }

exit:
    return rval;
}

uint16_t MleRouter::GetNextHop(uint16_t aDestination) const
{
    uint16_t rval = Mac::kShortAddrInvalid;
    const Router *router;

    if (mDeviceState == kDeviceStateChild)
    {
        ExitNow(rval = Mle::GetNextHop(aDestination));
    }

    router = GetRouter(GetRouterId(aDestination));
    VerifyOrExit(router != NULL,);

    router = GetRouter(router->mNextHop);
    VerifyOrExit(router != NULL && router->mState != Neighbor::kStateInvalid,);

    rval = GetRloc16(router->mNextHop);

exit:
    return rval;
}

uint8_t MleRouter::GetRouteCost(uint16_t aRloc16) const
{
    uint8_t rval = kMaxRouteCost;
    const Router *router;

    router = GetRouter(GetRouterId(aRloc16));
    VerifyOrExit(router != NULL && GetRouter(router->mNextHop) != NULL,);

    rval = router->mCost;

exit:
    return rval;
}

uint8_t MleRouter::GetRouterIdSequence(void) const
{
    return mRouterIdSequence;
}

uint8_t MleRouter::GetLeaderWeight(void) const
{
    return mLeaderWeight;
}

void MleRouter::SetLeaderWeight(uint8_t aWeight)
{
    mLeaderWeight = aWeight;
}

uint32_t MleRouter::GetLeaderPartitionId(void) const
{
    return mFixedLeaderPartitionId;
}

void MleRouter::SetLeaderPartitionId(uint32_t aPartitionId)
{
    mFixedLeaderPartitionId = aPartitionId;
}

ThreadError MleRouter::SetPreferredRouterId(uint8_t aRouterId)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(
        (mDeviceState == kDeviceStateDetached) || (mDeviceState == kDeviceStateDisabled),
        error = kThreadError_InvalidState
    );

    mPreviousRouterId = aRouterId;

exit:
    return error;
}

void MleRouter::SetPreviousPartitionId(uint32_t aPartitionId)
{
    mPreviousPartitionId = aPartitionId;
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

    VerifyOrExit(aRouterId <= kMaxRouterId,);

    rval = &mRouters[aRouterId];

exit:
    return rval;
}

const Router *MleRouter::GetRouter(uint8_t aRouterId) const
{
    const Router *rval = NULL;

    VerifyOrExit(aRouterId <= kMaxRouterId,);

    rval = &mRouters[aRouterId];

exit:
    return rval;
}

ThreadError MleRouter::GetChildInfoById(uint16_t aChildId, otChildInfo &aChildInfo)
{
    ThreadError error = kThreadError_None;
    Child *child;

    if ((aChildId & ~kMaxChildId) != 0)
    {
        aChildId = GetChildId(aChildId);
    }

    VerifyOrExit((child = FindChild(aChildId)) != NULL, error = kThreadError_NotFound);
    GetChildInfo(*child, aChildInfo);

exit:
    return error;
}

ThreadError MleRouter::GetChildInfoByIndex(uint8_t aChildIndex, otChildInfo &aChildInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aChildIndex < mMaxChildrenAllowed, error = kThreadError_InvalidArgs);
    GetChildInfo(mChildren[aChildIndex], aChildInfo);

exit:
    return error;
}

ThreadError MleRouter::RestoreChildren(void)
{
    ThreadError error = kThreadError_None;

    for (uint8_t i = 0; i < kMaxChildren; i++)
    {
        Child *child;
        otChildInfo childInfo;
        uint16_t length;

        length = sizeof(childInfo);
        SuccessOrExit(otPlatSettingsGet(mNetif.GetInstance(), kKeyChildInfo, i,
                                        reinterpret_cast<uint8_t *>(&childInfo), &length));
        VerifyOrExit(length == sizeof(childInfo), ;);

        VerifyOrExit((child = NewChild()) != NULL, error = kThreadError_NoBufs);
        memset(child, 0, sizeof(*child));

        memcpy(&child->mMacAddr, &childInfo.mExtAddress, sizeof(child->mMacAddr));
        child->mValid.mRloc16 = childInfo.mRloc16;
        child->mTimeout = childInfo.mTimeout;
        child->mMode = (childInfo.mRxOnWhenIdle ? ModeTlv::kModeRxOnWhenIdle : 0) |
                       (childInfo.mSecureDataRequest ? ModeTlv::kModeSecureDataRequest : 0) |
                       (childInfo.mFullFunction ? ModeTlv::kModeFFD : 0) |
                       (childInfo.mFullNetworkData ? ModeTlv::kModeFullNetworkData : 0);
        child->mState = Neighbor::kStateRestored;
        child->mLastHeard = Timer::GetNow();
    }

exit:
    return error;
}

ThreadError MleRouter::RemoveStoredChild(uint16_t aChildRloc16)
{
    ThreadError error = kThreadError_NotFound;

    for (uint8_t i = 0; i < kMaxChildren; i++)
    {
        otChildInfo childInfo;
        uint16_t length = sizeof(childInfo);

        SuccessOrExit(otPlatSettingsGet(mNetif.GetInstance(), kKeyChildInfo, i,
                                        reinterpret_cast<uint8_t *>(&childInfo), &length));
        VerifyOrExit(length == sizeof(childInfo), ;);

        if (childInfo.mRloc16 == aChildRloc16)
        {
            error = otPlatSettingsDelete(mNetif.GetInstance(), kKeyChildInfo, i);
            ExitNow();
        }
    }

exit:
    return error;
}

ThreadError MleRouter::StoreChild(uint16_t aChildRloc16)
{
    ThreadError error = kThreadError_None;
    otChildInfo childInfo;

    SuccessOrExit(error = mMleRouter.GetChildInfoById(GetChildId(aChildRloc16), childInfo));

    error = otPlatSettingsAdd(mNetif.GetInstance(), kKeyChildInfo, reinterpret_cast<uint8_t *>(&childInfo),
                              sizeof(childInfo));

exit:
    return error;
}

void MleRouter::GetChildInfo(Child &aChild, otChildInfo &aChildInfo)
{
    memset(&aChildInfo, 0, sizeof(aChildInfo));

    if (aChild.mState == Neighbor::kStateValid)
    {
        memcpy(&aChildInfo.mExtAddress, &aChild.mMacAddr, sizeof(aChildInfo.mExtAddress));
        aChildInfo.mTimeout = aChild.mTimeout;
        aChildInfo.mRloc16 = aChild.mValid.mRloc16;
        aChildInfo.mChildId = GetChildId(aChild.mValid.mRloc16);
        aChildInfo.mNetworkDataVersion = aChild.mNetworkDataVersion;
        aChildInfo.mAge = Timer::MsecToSec(Timer::GetNow() - aChild.mLastHeard);
        aChildInfo.mLinkQualityIn = aChild.mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor());
        aChildInfo.mAverageRssi = aChild.mLinkInfo.GetAverageRss();

        aChildInfo.mRxOnWhenIdle = (aChild.mMode & ModeTlv::kModeRxOnWhenIdle) != 0;
        aChildInfo.mSecureDataRequest = (aChild.mMode & ModeTlv::kModeSecureDataRequest) != 0;
        aChildInfo.mFullFunction = (aChild.mMode & ModeTlv::kModeFFD) != 0;
        aChildInfo.mFullNetworkData = (aChild.mMode & ModeTlv::kModeFullNetworkData) != 0;
    }
}

ThreadError MleRouter::GetRouterInfo(uint16_t aRouterId, otRouterInfo &aRouterInfo)
{
    ThreadError error = kThreadError_None;
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
    VerifyOrExit(router != NULL, error = kThreadError_InvalidArgs);

    memcpy(&aRouterInfo.mExtAddress, &router->mMacAddr, sizeof(aRouterInfo.mExtAddress));
    aRouterInfo.mAllocated = router->mAllocated;
    aRouterInfo.mRouterId = routerId;
    aRouterInfo.mRloc16 = GetRloc16(routerId);
    aRouterInfo.mNextHop = router->mNextHop;
    aRouterInfo.mLinkEstablished = router->mState == Neighbor::kStateValid;
    aRouterInfo.mPathCost = router->mCost;
    aRouterInfo.mLinkQualityIn = router->mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor());
    aRouterInfo.mLinkQualityOut = router->mLinkQualityOut;
    aRouterInfo.mAge = static_cast<uint8_t>(Timer::MsecToSec(Timer::GetNow() - router->mLastHeard));

exit:
    return error;
}

ThreadError MleRouter::GetNextNeighborInfo(otNeighborInfoIterator &aIterator, otNeighborInfo &aNeighInfo)
{
    ThreadError error = kThreadError_None;
    Neighbor *neighbor = NULL;
    int16_t index;

    memset(&aNeighInfo, 0, sizeof(aNeighInfo));

    // Non-negative iterator value gives the current index into mChildren array

    if (aIterator >= 0)
    {
        for (index = aIterator; index < mMaxChildrenAllowed; index++)
        {
            if (mChildren[index].mState == Neighbor::kStateValid)
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
        if (mRouters[index].mState == Neighbor::kStateValid)
        {
            neighbor = &mRouters[index];
            aNeighInfo.mIsChild = false;
            index++;
            aIterator = -index;
            ExitNow();
        }
    }

    aIterator = -index;
    error = kThreadError_NotFound;

exit:

    if (neighbor != NULL)
    {
        memcpy(&aNeighInfo.mExtAddress, &neighbor->mMacAddr, sizeof(aNeighInfo.mExtAddress));
        aNeighInfo.mAge = Timer::MsecToSec(Timer::GetNow() - neighbor->mLastHeard);
        aNeighInfo.mRloc16 = neighbor->mValid.mRloc16;
        aNeighInfo.mLinkFrameCounter = neighbor->mValid.mLinkFrameCounter;
        aNeighInfo.mMleFrameCounter = neighbor->mValid.mMleFrameCounter;
        aNeighInfo.mLinkQualityIn = neighbor->mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor());
        aNeighInfo.mAverageRssi = neighbor->mLinkInfo.GetAverageRss();
        aNeighInfo.mRxOnWhenIdle = (neighbor->mMode & ModeTlv::kModeRxOnWhenIdle) != 0;
        aNeighInfo.mSecureDataRequest = (neighbor->mMode & ModeTlv::kModeSecureDataRequest) != 0;
        aNeighInfo.mFullFunction = (neighbor->mMode & ModeTlv::kModeFFD) != 0;
        aNeighInfo.mFullNetworkData = (neighbor->mMode & ModeTlv::kModeFullNetworkData) != 0;
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
        router->mNextHop = kInvalidRouterId;
        ResetAdvertiseInterval();
    }
}

ThreadError MleRouter::CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    Ip6::Address destination;

    if (mDeviceState == kDeviceStateChild)
    {
        return Mle::CheckReachability(aMeshSource, aMeshDest, aIp6Header);
    }

    if (aMeshDest == mMac.GetShortAddress())
    {
        // mesh destination is this device
        if (mNetif.IsUnicastAddress(aIp6Header.GetDestination()))
        {
            // IPv6 destination is this device
            return kThreadError_None;
        }
        else if (GetNeighbor(aIp6Header.GetDestination()) != NULL)
        {
            // IPv6 destination is an RFD child
            return kThreadError_None;
        }
    }
    else if (GetRouterId(aMeshDest) == mRouterId)
    {
        // mesh destination is a child of this device
        if (GetChild(aMeshDest))
        {
            return kThreadError_None;
        }
    }
    else if (GetNextHop(aMeshDest) != Mac::kShortAddrInvalid)
    {
        // forwarding to another router and route is known
        return kThreadError_None;
    }

    destination = GetMeshLocal16();
    destination.mFields.m16[7] = HostSwap16(aMeshSource);
    mNetif.GetIp6().mIcmp.SendError(destination, Ip6::IcmpHeader::kTypeDstUnreach,
                                    Ip6::IcmpHeader::kCodeDstUnreachNoRoute, aIp6Header);

    return kThreadError_Drop;
}

ThreadError MleRouter::SendAddressSolicit(ThreadStatusTlv::Status aStatus)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    ThreadExtMacAddressTlv macAddr64Tlv;
    ThreadRloc16Tlv rlocTlv;
    ThreadStatusTlv statusTlv;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_ADDRESS_SOLICIT);
    header.SetPayloadMarker();

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

    macAddr64Tlv.Init();
    macAddr64Tlv.SetMacAddr(*mMac.GetExtAddress());
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

    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo,
                                                  &MleRouter::HandleAddressSolicitResponse, this));

    otLogInfoMle("Sent address solicit to %04x", HostSwap16(messageInfo.GetPeerAddr().mFields.m16[7]));

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError MleRouter::SendAddressRelease(void)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    ThreadRloc16Tlv rlocTlv;
    ThreadExtMacAddressTlv macAddr64Tlv;
    Ip6::MessageInfo messageInfo;
    Message *message;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_ADDRESS_RELEASE);
    header.SetPayloadMarker();

    VerifyOrExit((message = mCoapClient.NewMessage(header)) != NULL, error = kThreadError_NoBufs);

    rlocTlv.Init();
    rlocTlv.SetRloc16(GetRloc16(mRouterId));
    SuccessOrExit(error = message->Append(&rlocTlv, sizeof(rlocTlv)));

    macAddr64Tlv.Init();
    macAddr64Tlv.SetMacAddr(*mMac.GetExtAddress());
    SuccessOrExit(error = message->Append(&macAddr64Tlv, sizeof(macAddr64Tlv)));

    SuccessOrExit(error = GetLeaderAddress(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = mCoapClient.SendMessage(*message, messageInfo));

    otLogInfoMle("Sent address release");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void MleRouter::HandleAddressSolicitResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                             const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicitResponse(static_cast<Coap::Header *>(aHeader),
                                                                     static_cast<Message *>(aMessage),
                                                                     static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                     aResult);
}

void MleRouter::HandleAddressSolicitResponse(Coap::Header *aHeader, Message *aMessage,
                                             const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    (void)aResult;
    (void)aMessageInfo;

    ThreadStatusTlv statusTlv;
    ThreadRloc16Tlv rlocTlv;
    ThreadRouterMaskTlv routerMaskTlv;
    uint8_t routerId;
    Router *router;
    bool old;

    VerifyOrExit(aResult == kThreadError_None && aHeader != NULL && aMessage != NULL, ;);

    VerifyOrExit(aHeader->GetCode() == kCoapResponseChanged, ;);

    otLogInfoMle("Received address reply");

    SuccessOrExit(ThreadTlv::GetTlv(*aMessage, ThreadTlv::kStatus, sizeof(statusTlv), statusTlv));
    VerifyOrExit(statusTlv.IsValid(), ;);

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
    VerifyOrExit(rlocTlv.IsValid(), ;);
    routerId = GetRouterId(rlocTlv.GetRloc16());
    router = GetRouter(routerId);
    VerifyOrExit(router != NULL,);

    SuccessOrExit(ThreadTlv::GetTlv(*aMessage, ThreadTlv::kRouterMask, sizeof(routerMaskTlv), routerMaskTlv));
    VerifyOrExit(routerMaskTlv.IsValid(), ;);

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

    router->mCost = 0;

    // copy router id information
    mRouterIdSequence = routerMaskTlv.GetIdSequence();
    mRouterIdSequenceLastUpdated = Timer::GetNow();

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        old = mRouters[i].mAllocated;
        mRouters[i].mAllocated = routerMaskTlv.IsAssignedRouterIdSet(i);

        if (old && !mRouters[i].mAllocated)
        {
            mAddressResolver.Remove(i);
        }
    }

    // send link request
    SendLinkRequest(NULL);
    ResetAdvertiseInterval();

    // send child id responses
    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        switch (mChildren[i].mState)
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
    {}
}

void MleRouter::HandleAddressSolicit(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                     const otMessageInfo *aMessageInfo)
{
    static_cast<MleRouter *>(aContext)->HandleAddressSolicit(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void MleRouter::HandleAddressSolicit(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    ThreadExtMacAddressTlv macAddr64Tlv;
    ThreadRloc16Tlv rlocTlv;
    ThreadStatusTlv statusTlv;
    uint8_t routerId = kInvalidRouterId;
    Router *router = NULL;

    VerifyOrExit(aHeader.GetType() == kCoapTypeConfirmable && aHeader.GetCode() == kCoapRequestPost,
                 error = kThreadError_Parse);

    otLogInfoMle("Received address solicit");

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kExtMacAddress, sizeof(macAddr64Tlv), macAddr64Tlv));
    VerifyOrExit(macAddr64Tlv.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = ThreadTlv::GetTlv(aMessage, ThreadTlv::kStatus, sizeof(statusTlv), statusTlv));
    VerifyOrExit(statusTlv.IsValid(), error = kThreadError_Parse);

    // see if allocation already exists
    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mAllocated &&
            memcmp(&mRouters[i].mMacAddr, macAddr64Tlv.GetMacAddr(), sizeof(mRouters[i].mMacAddr)) == 0)
        {
            ExitNow(routerId = i);
        }
    }

    // check the request reason
    switch (statusTlv.GetStatus())
    {
    case ThreadStatusTlv::kTooFewRouters:
        VerifyOrExit(GetActiveRouterCount() < mRouterUpgradeThreshold, ;);
        break;

    case ThreadStatusTlv::kHaveChildIdRequest:
    case ThreadStatusTlv::kParentPartitionChange:
        break;

    default:
        ExitNow(error = kThreadError_Parse);
        break;
    }

    if (ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rlocTlv), rlocTlv) == kThreadError_None)
    {
        // specific Router ID requested
        VerifyOrExit(rlocTlv.IsValid(), error = kThreadError_Parse);
        routerId = GetRouterId(rlocTlv.GetRloc16());
        router = GetRouter(routerId);

        if (router != NULL)
        {
            if (router->mAllocated && memcmp(&router->mMacAddr, macAddr64Tlv.GetMacAddr(), sizeof(router->mMacAddr)))
            {
                // requested Router ID is allocated to another device
                routerId = kInvalidRouterId;
            }
            else if (!router->mAllocated && router->mReclaimDelay)
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
        otLogInfoMle("router id requested and provided!");
    }

    router = GetRouter(routerId);

    if (router != NULL)
    {
        memcpy(&router->mMacAddr, macAddr64Tlv.GetMacAddr(), sizeof(router->mMacAddr));
    }
    else
    {
        otLogInfoMle("router address unavailable!");
    }

exit:

    if (error == kThreadError_None)
    {
        SendAddressSolicitResponse(aHeader, routerId, aMessageInfo);
    }
}

void MleRouter::SendAddressSolicitResponse(const Coap::Header &aRequestHeader, uint8_t aRouterId,
                                           const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    ThreadStatusTlv statusTlv;
    ThreadRouterMaskTlv routerMaskTlv;
    ThreadRloc16Tlv rlocTlv;
    Message *message;

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

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
            if (mRouters[i].mAllocated)
            {
                routerMaskTlv.SetAssignedRouterId(i);
            }
        }

        SuccessOrExit(error = message->Append(&routerMaskTlv, sizeof(routerMaskTlv)));
    }

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMle("Sent address reply");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

void MleRouter::HandleAddressRelease(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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

    VerifyOrExit(aHeader.GetType() == kCoapTypeConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost, ;);

    otLogInfoMle("Received address release");

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rlocTlv), rlocTlv));
    VerifyOrExit(rlocTlv.IsValid(),);

    SuccessOrExit(ThreadTlv::GetTlv(aMessage, ThreadTlv::kExtMacAddress, sizeof(macAddr64Tlv), macAddr64Tlv));
    VerifyOrExit(macAddr64Tlv.IsValid(),);

    routerId = GetRouterId(rlocTlv.GetRloc16());
    router = GetRouter(routerId);

    VerifyOrExit(router != NULL &&
                 memcmp(&router->mMacAddr, macAddr64Tlv.GetMacAddr(), sizeof(router->mMacAddr)) == 0,);

    ReleaseRouterId(routerId);
    SendAddressReleaseResponse(aHeader, aMessageInfo);

exit:
    {}
}

void MleRouter::SendAddressReleaseResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Message *message;

    VerifyOrExit((message = mCoapServer.NewMessage(0)) != NULL, error = kThreadError_NoBufs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);

    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoMle("Sent address release response");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

void MleRouter::FillConnectivityTlv(ConnectivityTlv &aTlv)
{
    ConnectivityTlv &tlv = aTlv;
    uint8_t cost;
    uint8_t lqi;
    uint8_t numChildren = 0;

    for (int i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].mState == Neighbor::kStateValid)
        {
            numChildren++;
        }
    }

    if ((mMaxChildrenAllowed - numChildren) < (mMaxChildrenAllowed / 3))
    {
        tlv.SetParentPriority(-1);
    }
    else
    {
        tlv.SetParentPriority(0);
    }

    // compute leader cost and link qualities
    tlv.SetLinkQuality1(0);
    tlv.SetLinkQuality2(0);
    tlv.SetLinkQuality3(0);

    cost = mRouters[GetLeaderId()].mCost;

    switch (GetDeviceState())
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        assert(false);
        break;

    case kDeviceStateChild:
        switch (mParent.mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor()))
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

        cost += LqiToCost(mParent.mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor()));
        break;

    case kDeviceStateRouter:
        cost += GetLinkCost(mRouters[GetLeaderId()].mNextHop);
        break;

    case kDeviceStateLeader:
        cost = 0;
        break;
    }

    tlv.SetActiveRouters(0);

    for (int i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mAllocated)
        {
            tlv.SetActiveRouters(tlv.GetActiveRouters() + 1);
        }

        if (mRouters[i].mState != Neighbor::kStateValid || i == mRouterId)
        {
            continue;
        }

        lqi = mRouters[i].mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor());

        if (lqi > mRouters[i].mLinkQualityOut)
        {
            lqi = mRouters[i].mLinkQualityOut;
        }

        switch (lqi)
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

ThreadError MleRouter::AppendConnectivity(Message &aMessage)
{
    ThreadError error;
    ConnectivityTlv tlv;

    tlv.Init();
    FillConnectivityTlv(tlv);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

exit:
    return error;
}

ThreadError MleRouter::AppendChildAddresses(Message &aMessage, Child &aChild)
{
    ThreadError error;
    Tlv tlv;
    AddressRegistrationEntry entry;
    Lowpan::Context context;
    uint8_t length = 0;
    uint8_t startOffset = static_cast<uint8_t>(aMessage.GetLength());

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    for (size_t i = 0; i < sizeof(aChild.mIp6Address) / sizeof(aChild.mIp6Address[0]); i++)
    {
        if (aChild.mIp6Address[i].IsUnspecified())
        {
            break;
        }

        if (mNetworkData.GetContext(aChild.mIp6Address[i], context) == kThreadError_None)
        {
            // compressed entry
            entry.SetContextId(context.mContextId);
            entry.SetIid(aChild.mIp6Address[i].GetIid());
        }
        else
        {
            // uncompressed entry
            entry.SetUncompressed();
            entry.SetIp6Address(aChild.mIp6Address[i]);
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
    uint8_t cost;

    tlv.SetRouterIdSequence(mRouterIdSequence);
    tlv.ClearRouterIdMask();

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mAllocated == false)
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
            if (!IsRouterIdValid(mRouters[i].mNextHop))
            {
                cost = 0;
            }
            else
            {
                cost = mRouters[i].mCost + GetLinkCost(mRouters[i].mNextHop);

                if (cost >= kMaxRouteCost)
                {
                    cost = 0;
                }
            }

            tlv.SetRouteCost(routeCount, cost);
            tlv.SetLinkQualityOut(routeCount, mRouters[i].mLinkQualityOut);

            if (isAssignLinkQuality &&
                (memcmp(mRouters[i].mMacAddr.m8, mAddr64.m8, OT_EXT_ADDRESS_SIZE) == 0))
            {
                tlv.SetLinkQualityIn(routeCount, mAssignLinkQuality);
            }
            else
            {
                tlv.SetLinkQualityIn(routeCount, mRouters[i].mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor()));
            }
        }

        routeCount++;
    }

    tlv.SetRouteDataLength(routeCount);
}

ThreadError MleRouter::AppendRoute(Message &aMessage)
{
    ThreadError error;
    RouteTlv tlv;
    tlv.Init();
    FillRouteTlv(tlv);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(Tlv) + tlv.GetLength()));
exit:
    return error;
}

ThreadError MleRouter::AppendActiveDataset(Message &aMessage)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mNetif.GetActiveDataset().GetNetwork().GetSize() > 0,);

    SuccessOrExit(error = mNetif.GetActiveDataset().GetNetwork().AppendMleDatasetTlv(aMessage));

exit:
    return error;
}

ThreadError MleRouter::AppendPendingDataset(Message &aMessage)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mNetif.GetPendingDataset().GetNetwork().GetSize() > 0,);

    mNetif.GetPendingDataset().UpdateDelayTimer();
    SuccessOrExit(error = mNetif.GetPendingDataset().GetNetwork().AppendMleDatasetTlv(aMessage));

exit:
    return error;
}

bool MleRouter::HasMinDowngradeNeighborRouters(void)
{
    return GetMinDowngradeNeighborRouters() >= kMinDowngradeNeighbors;
}

bool MleRouter::HasOneNeighborwithComparableConnectivity(const RouteTlv &aRoute, uint8_t aRouterId)
{
    bool rval = true;
    uint8_t localLqi = 0;
    uint8_t peerLqi = 0;
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
        if (mRouters[i].mState == Neighbor::kStateValid)
        {
            // if neighbor is just peer
            if (i == aRouterId)
            {
                routerCount++;
                continue;
            }

            localLqi = mRouters[i].mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor());

            if (localLqi > mRouters[i].mLinkQualityOut)
            {
                localLqi = mRouters[i].mLinkQualityOut;
            }

            if (localLqi >= 2)
            {
                // check if this neighbor router is in peer Route64 TLV
                if (aRoute.IsRouterIdSet(i) == false)
                {
                    ExitNow(rval = false);
                }

                // get the peer's two-way lqi to this router
                peerLqi = aRoute.GetLinkQualityIn(routerCount);

                if (peerLqi > aRoute.GetLinkQualityOut(routerCount))
                {
                    peerLqi = aRoute.GetLinkQualityOut(routerCount);
                }

                // compare local lqi to this router with peer's
                if (peerLqi >= localLqi)
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

bool MleRouter::HasChildren(void)
{
    bool hasChildren = false;

    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].mState == Neighbor::kStateRestored || mChildren[i].mState >= Neighbor::kStateChildIdRequest)
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
        if (mChildren[i].mState == Neighbor::kStateRestored ||
            mChildren[i].mState == Neighbor::kStateChildUpdateRequest ||
            mChildren[i].mState == Neighbor::kStateValid)
        {
            RemoveStoredChild(mChildren[i].mValid.mRloc16);
        }

        mChildren[i].mState = Neighbor::kStateInvalid;
    }
}

bool MleRouter::HasSmallNumberOfChildren(void)
{
    uint8_t numChildren = 0;
    uint8_t routerCount = GetActiveRouterCount();

    VerifyOrExit(routerCount > mRouterDowngradeThreshold, ;);

    for (uint8_t i = 0; i < mMaxChildrenAllowed; i++)
    {
        if (mChildren[i].mState == Neighbor::kStateValid)
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
    uint8_t lqi;
    uint8_t routerCount = 0;

    for (uint8_t i = 0; i <= kMaxRouterId; i++)
    {
        if (mRouters[i].mState != Neighbor::kStateValid)
        {
            continue;
        }

        lqi = mRouters[i].mLinkInfo.GetLinkQuality(mMac.GetNoiseFloor());

        if (lqi > mRouters[i].mLinkQualityOut)
        {
            lqi = mRouters[i].mLinkQualityOut;
        }

        if (lqi >= 2)
        {
            routerCount++;
        }
    }

    return routerCount;
}

}  // namespace Mle
}  // namespace Thread
