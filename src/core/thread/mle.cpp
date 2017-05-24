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
 *   This file implements MLE functionality required for the Thread Child, Router and Leader roles.
 */

#define WPP_NAME "mle.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "mle.hpp"

#include <openthread/platform/radio.h>
#include <openthread/platform/random.h>
#include <openthread/platform/settings.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"
#include "crypto/aes_ccm.hpp"
#include "mac/mac_frame.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/address_resolver.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Mle {

Mle::Mle(ThreadNetif &aThreadNetif) :
    mNetif(aThreadNetif),
    mRetrieveNewNetworkData(false),
    mDeviceState(kDeviceStateDisabled),
    mDeviceMode(ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeSecureDataRequest),
    isAssignLinkQuality(false),
    mAssignLinkQuality(0),
    mAssignLinkMargin(0),

    mParentRequestState(kParentIdle),
    mReattachState(kReattachStop),
    mParentRequestTimer(aThreadNetif.GetIp6().mTimerScheduler, &Mle::HandleParentRequestTimer, this),
    mDelayedResponseTimer(aThreadNetif.GetIp6().mTimerScheduler, &Mle::HandleDelayedResponseTimer, this),
    mLastPartitionRouterIdSequence(0),
    mLastPartitionId(0),
    mParentLeaderCost(0),
    mParentRequestMode(kMleAttachAnyPartition),
    mParentPriority(0),
    mParentLinkQuality3(0),
    mParentLinkQuality2(0),
    mParentLinkQuality1(0),
    mChildUpdateAttempts(0),
    mParentIsSingleton(false),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mTimeout(kMleEndDeviceTimeout),
    mSendChildUpdateRequest(aThreadNetif.GetIp6().mTaskletScheduler, &Mle::HandleSendChildUpdateRequest, this),
    mDiscoverHandler(NULL),
    mDiscoverContext(NULL),
    mIsDiscoverInProgress(false),
    mEnableEui64Filtering(false),
    mAnnounceChannel(kPhyMinChannel),
    mPreviousChannel(0),
    mPreviousPanId(Mac::kPanIdBroadcast)
{
    uint8_t meshLocalPrefix[8];

    memset(&mLeaderData, 0, sizeof(mLeaderData));
    memset(&mParentLeaderData, 0, sizeof(mParentLeaderData));
    memset(&mParent, 0, sizeof(mParent));
    memset(&mChildIdRequest, 0, sizeof(mChildIdRequest));
    memset(&mLinkLocal64, 0, sizeof(mLinkLocal64));
    memset(&mMeshLocal64, 0, sizeof(mMeshLocal64));
    memset(&mMeshLocal16, 0, sizeof(mMeshLocal16));
    memset(&mLinkLocalAllThreadNodes, 0, sizeof(mLinkLocalAllThreadNodes));
    memset(&mRealmLocalAllThreadNodes, 0, sizeof(mRealmLocalAllThreadNodes));
    memset(&mLeaderAloc, 0, sizeof(mLeaderAloc));
    memset(&mParentCandidate, 0, sizeof(mParentCandidate));

    // link-local 64
    mLinkLocal64.GetAddress().mFields.m16[0] = HostSwap16(0xfe80);
    mLinkLocal64.GetAddress().SetIid(*mNetif.GetMac().GetExtAddress());
    mLinkLocal64.mPrefixLength = 64;
    mLinkLocal64.mPreferred = true;
    mLinkLocal64.mValid = true;
    mNetif.AddUnicastAddress(mLinkLocal64);

    // Leader Aloc
    mLeaderAloc.mPrefixLength = 128;
    mLeaderAloc.mPreferred = true;
    mLeaderAloc.mValid = true;
    mLeaderAloc.mScopeOverride = Ip6::Address::kRealmLocalScope;
    mLeaderAloc.mScopeOverrideValid = true;

    // initialize Mesh Local Prefix
    meshLocalPrefix[0] = 0xfd;
    memcpy(meshLocalPrefix + 1, mNetif.GetMac().GetExtendedPanId(), 5);
    meshLocalPrefix[6] = 0x00;
    meshLocalPrefix[7] = 0x00;

    // mesh-local 64
    for (int i = OT_IP6_PREFIX_SIZE; i < OT_IP6_ADDRESS_SIZE; i++)
    {
        mMeshLocal64.GetAddress().mFields.m8[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    mMeshLocal64.mPrefixLength = 64;
    mMeshLocal64.mPreferred = true;
    mMeshLocal64.mValid = true;
    mMeshLocal64.mScopeOverride = Ip6::Address::kRealmLocalScope;
    mMeshLocal64.mScopeOverrideValid = true;
    SetMeshLocalPrefix(meshLocalPrefix); // Also calls AddUnicastAddress

    // mesh-local 16
    mMeshLocal16.GetAddress().mFields.m16[4] = HostSwap16(0x0000);
    mMeshLocal16.GetAddress().mFields.m16[5] = HostSwap16(0x00ff);
    mMeshLocal16.GetAddress().mFields.m16[6] = HostSwap16(0xfe00);
    mMeshLocal16.mPrefixLength = 64;
    mMeshLocal16.mPreferred = true;
    mMeshLocal16.mValid = true;
    mMeshLocal16.mScopeOverride = Ip6::Address::kRealmLocalScope;
    mMeshLocal16.mScopeOverrideValid = true;
    mMeshLocal16.mRloc = true;

    // Store RLOC address reference in MPL module.
    mNetif.GetIp6().mMpl.SetMatchingAddress(mMeshLocal16.GetAddress());

    // link-local all thread nodes
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff32);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[6] = HostSwap16(0x0000);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);
    mNetif.SubscribeMulticast(mLinkLocalAllThreadNodes);

    // realm-local all thread nodes
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff33);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[6] = HostSwap16(0x0000);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);
    mNetif.SubscribeMulticast(mRealmLocalAllThreadNodes);

    mNetifCallback.Set(&Mle::HandleNetifStateChanged, this);
    mNetif.RegisterCallback(mNetifCallback);

    memset(&mAddr64, 0, sizeof(mAddr64));
}

otInstance *Mle::GetInstance(void)
{
    return mNetif.GetInstance();
}

otError Mle::Enable(void)
{
    otError error = OT_ERROR_NONE;
    Ip6::SockAddr sockaddr;

    // memcpy(&sockaddr.mAddr, &mLinkLocal64.GetAddress(), sizeof(sockaddr.mAddr));
    sockaddr.mPort = kUdpPort;
    SuccessOrExit(error = mSocket.Open(&Mle::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(sockaddr));

exit:
    return error;
}

otError Mle::Disable(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = Stop(false));
    SuccessOrExit(error = mSocket.Close());

exit:
    return error;
}

otError Mle::Start(bool aEnableReattach, bool aAnnounceAttach)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntry();

    // cannot bring up the interface if IEEE 802.15.4 promiscuous mode is enabled
    VerifyOrExit(otPlatRadioGetPromiscuous(mNetif.GetInstance()) == false, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mNetif.IsUp(), error = OT_ERROR_INVALID_STATE);

    mDeviceState = kDeviceStateDetached;
    mNetif.SetStateChangedFlags(OT_NET_ROLE);
    SetStateDetached();

    mNetif.GetKeyManager().Start();

    if (aEnableReattach)
    {
        mReattachState = kReattachStart;
    }

    if (aAnnounceAttach || (GetRloc16() == Mac::kShortAddrInvalid))
    {
        BecomeChild(kMleAttachAnyPartition);
    }
    else if (IsActiveRouter(GetRloc16()))
    {
        if (mNetif.GetMle().BecomeRouter(ThreadStatusTlv::kTooFewRouters) != OT_ERROR_NONE)
        {
            BecomeChild(kMleAttachAnyPartition);
        }
    }
    else
    {
        mParentRequestState = kParentSynchronize;
        mChildUpdateAttempts = 0;
        SendChildUpdateRequest();
    }

exit:
    otLogFuncExitErr(error);
    return error;
}

otError Mle::Stop(bool aClearNetworkDatasets)
{
    otLogFuncEntry();
    mNetif.GetKeyManager().Stop();
    SetStateDetached();
    mNetif.RemoveUnicastAddress(mMeshLocal16);
    mNetif.GetNetworkDataLocal().Clear();
    mNetif.GetNetworkDataLeader().Clear();
    memset(&mLeaderData, 0, sizeof(mLeaderData));

    if (aClearNetworkDatasets)
    {
        mNetif.GetActiveDataset().Clear(true);
        mNetif.GetPendingDataset().Clear(true);
    }

    mDeviceState = kDeviceStateDisabled;
    otLogFuncExit();
    return OT_ERROR_NONE;
}

otError Mle::Restore(void)
{
    otError error = OT_ERROR_NONE;
    Settings::NetworkInfo networkInfo;
    Settings::ParentInfo parentInfo;
    uint16_t length;

    mNetif.GetActiveDataset().Restore();
    mNetif.GetPendingDataset().Restore();

    length = sizeof(networkInfo);
    SuccessOrExit(error = otPlatSettingsGet(mNetif.GetInstance(), Settings::kKeyNetworkInfo, 0,
                                            reinterpret_cast<uint8_t *>(&networkInfo), &length));
    VerifyOrExit(length == sizeof(networkInfo), error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(networkInfo.mDeviceState >= kDeviceStateChild, error = OT_ERROR_NOT_FOUND);

    mDeviceMode = networkInfo.mDeviceMode;
    SetRloc16(networkInfo.mRloc16);
    mNetif.GetKeyManager().SetCurrentKeySequence(networkInfo.mKeySequence);
    mNetif.GetKeyManager().SetMleFrameCounter(networkInfo.mMleFrameCounter);
    mNetif.GetKeyManager().SetMacFrameCounter(networkInfo.mMacFrameCounter);
    mNetif.GetMac().SetExtAddress(networkInfo.mExtAddress);
    UpdateLinkLocalAddress();

    memcpy(&mMeshLocal64.GetAddress().mFields.m8[OT_IP6_PREFIX_SIZE],
           networkInfo.mMlIid,
           OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);

    if (networkInfo.mDeviceState == kDeviceStateChild)
    {
        length = sizeof(parentInfo);
        SuccessOrExit(error = otPlatSettingsGet(mNetif.GetInstance(), Settings::kKeyParentInfo, 0,
                                                reinterpret_cast<uint8_t *>(&parentInfo), &length));
        VerifyOrExit(length >= sizeof(parentInfo), error = OT_ERROR_PARSE);

        memset(&mParent, 0, sizeof(mParent));
        mParent.SetExtAddress(*static_cast<Mac::ExtAddress *>(&parentInfo.mExtAddress));
        mParent.SetDeviceMode(ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData |
                              ModeTlv::kModeSecureDataRequest);
        mParent.SetRloc16(GetRloc16(GetRouterId(networkInfo.mRloc16)));
        mParent.SetState(Neighbor::kStateRestored);
    }
    else if (networkInfo.mDeviceState == kDeviceStateRouter || networkInfo.mDeviceState == kDeviceStateLeader)
    {
        mNetif.GetMle().SetRouterId(GetRouterId(GetRloc16()));
        mNetif.GetMle().SetPreviousPartitionId(networkInfo.mPreviousPartitionId);

        switch (mNetif.GetMle().RestoreChildren())
        {
        // If there are more saved children in non-volatile settings
        // than could be restored or the values in the settings are
        // invalid, erase all the children info in the settings and
        // refresh the info to ensure that the non-volatile settings
        // stay in sync with the child table.

        case OT_ERROR_FAILED:
        case OT_ERROR_NO_BUFS:
            mNetif.GetMle().RefreshStoredChildren();
            break;

        default:
            break;
        }
    }

exit:
    return error;
}

otError Mle::Store(void)
{
    otError error = OT_ERROR_NONE;
    Settings::NetworkInfo networkInfo;
    Settings::ParentInfo parentInfo;

    VerifyOrExit(IsAttached(), error = OT_ERROR_INVALID_STATE);

    if (mNetif.GetActiveDataset().GetLocal().GetTimestamp() == NULL)
    {
        mNetif.GetActiveDataset().GenerateLocal();
        mNetif.GetActiveDataset().GetLocal().Store();
    }

    memset(&networkInfo, 0, sizeof(networkInfo));

    networkInfo.mDeviceMode = mDeviceMode;
    networkInfo.mDeviceState = mDeviceState;
    networkInfo.mRloc16 = GetRloc16();
    networkInfo.mKeySequence = mNetif.GetKeyManager().GetCurrentKeySequence();
    networkInfo.mMleFrameCounter = mNetif.GetKeyManager().GetMleFrameCounter() +
                                   OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    networkInfo.mMacFrameCounter = mNetif.GetKeyManager().GetMacFrameCounter() +
                                   OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    networkInfo.mPreviousPartitionId = mLeaderData.GetPartitionId();
    memcpy(networkInfo.mExtAddress.m8, mNetif.GetMac().GetExtAddress(), sizeof(networkInfo.mExtAddress));
    memcpy(networkInfo.mMlIid,
           &mMeshLocal64.GetAddress().mFields.m8[OT_IP6_PREFIX_SIZE],
           OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);


    if (mDeviceState == kDeviceStateChild)
    {
        memset(&parentInfo, 0, sizeof(parentInfo));
        memcpy(&parentInfo.mExtAddress, &mParent.GetExtAddress(), sizeof(parentInfo.mExtAddress));

        SuccessOrExit(error = otPlatSettingsSet(mNetif.GetInstance(), Settings::kKeyParentInfo,
                                                reinterpret_cast<uint8_t *>(&parentInfo), sizeof(parentInfo)));
    }

    SuccessOrExit(error = otPlatSettingsSet(mNetif.GetInstance(), Settings::kKeyNetworkInfo,
                                            reinterpret_cast<uint8_t *>(&networkInfo), sizeof(networkInfo)));

    mNetif.GetKeyManager().SetStoredMleFrameCounter(networkInfo.mMleFrameCounter);
    mNetif.GetKeyManager().SetStoredMacFrameCounter(networkInfo.mMacFrameCounter);

    otLogDebgMle(GetInstance(), "Store Network Information");

exit:
    return error;
}

otError Mle::Discover(uint32_t aScanChannels, uint16_t aPanId, bool aJoiner, bool aEnableEui64Filtering,
                      DiscoverHandler aCallback, void *aContext)
{
    otError error = OT_ERROR_NONE;
    Message *message = NULL;
    Ip6::Address destination;
    Tlv tlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    uint16_t startOffset;

    VerifyOrExit(!mIsDiscoverInProgress, error = OT_ERROR_BUSY);

    mEnableEui64Filtering = aEnableEui64Filtering;

    mDiscoverHandler = aCallback;
    mDiscoverContext = aContext;
    mNetif.GetMeshForwarder().SetDiscoverParameters(aScanChannels);

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleDiscoverRequest);
    message->SetPanId(aPanId);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDiscoveryRequest));

    // Discovery TLV
    tlv.SetType(Tlv::kDiscovery);
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

    startOffset = message->GetLength();

    // Discovery Request TLV
    discoveryRequest.Init();
    discoveryRequest.SetVersion(kVersion);
    discoveryRequest.SetJoiner(aJoiner);
    SuccessOrExit(error = message->Append(&discoveryRequest, sizeof(discoveryRequest)));

    tlv.SetLength(static_cast<uint8_t>(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0002);
    SuccessOrExit(error = SendMessage(*message, destination));

    mIsDiscoverInProgress = true;

    otLogInfoMle(GetInstance(), "Sent discovery request");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

bool Mle::IsDiscoverInProgress(void)
{
    return mIsDiscoverInProgress;
}

void Mle::HandleDiscoverComplete(void)
{
    mIsDiscoverInProgress = false;
    mEnableEui64Filtering = false;
    mDiscoverHandler(NULL, mDiscoverContext);
}

otError Mle::BecomeDetached(void)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntry();

    VerifyOrExit(mDeviceState != kDeviceStateDisabled, error = OT_ERROR_INVALID_STATE);

    if (mReattachState == kReattachStop)
    {
        mNetif.GetPendingDataset().UpdateDelayTimer();
        mNetif.GetPendingDataset().Set(mNetif.GetPendingDataset().GetLocal());
    }

    SetStateDetached();
    SetRloc16(Mac::kShortAddrInvalid);
    BecomeChild(kMleAttachAnyPartition);

exit:
    otLogFuncExitErr(error);
    return error;
}

otError Mle::BecomeChild(otMleAttachFilter aFilter)
{
    otError error = OT_ERROR_NONE;

    otLogFuncEntry();

    VerifyOrExit(mDeviceState != kDeviceStateDisabled, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mParentRequestState == kParentIdle, error = OT_ERROR_BUSY);

    if (mReattachState == kReattachStart)
    {
        if (mNetif.GetActiveDataset().Restore() == OT_ERROR_NONE)
        {
            mReattachState = kReattachActive;
        }
        else
        {
            mReattachState = kReattachStop;
        }
    }

    ResetParentCandidate();
    mParentRequestState = kParentRequestStart;
    mParentRequestMode = aFilter;

    if (aFilter != kMleAttachBetterPartition)
    {
        memset(&mParent, 0, sizeof(mParent));

        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            mNetif.GetMle().StopAdvertiseTimer();
        }
    }

    if (aFilter == kMleAttachAnyPartition)
    {
        mParent.SetState(Neighbor::kStateInvalid);
        mLastPartitionId = mNetif.GetMle().GetPreviousPartitionId();
        mLastPartitionRouterIdSequence = mNetif.GetMle().GetRouterIdSequence();
    }

    mNetif.GetMeshForwarder().SetRxOnWhenIdle(true);

    mParentRequestTimer.Start((otPlatRandomGet() % kParentRequestRouterTimeout) + 1);

exit:
    otLogFuncExitErr(error);
    return error;
}

bool Mle::IsAttached(void) const
{
    return (mDeviceState == kDeviceStateChild ||
            mDeviceState == kDeviceStateRouter ||
            mDeviceState == kDeviceStateLeader);
}

otError Mle::SetStateDetached(void)
{
    if (mDeviceState != kDeviceStateDetached)
    {
        mNetif.SetStateChangedFlags(OT_NET_ROLE);
    }

    if (mDeviceState == kDeviceStateLeader)
    {
        mNetif.RemoveUnicastAddress(mLeaderAloc);
    }

    mDeviceState = kDeviceStateDetached;
    mParentRequestState = kParentIdle;
    mParentRequestTimer.Stop();
    mNetif.GetMeshForwarder().SetRxOff();
    mNetif.GetMac().SetBeaconEnabled(false);
    mNetif.GetMle().HandleDetachStart();
    mNetif.GetIp6().SetForwardingEnabled(false);
    mNetif.GetIp6().mMpl.SetTimerExpirations(0);

    otLogInfoMle(GetInstance(), "Mode -> Detached");
    return OT_ERROR_NONE;
}

otError Mle::SetStateChild(uint16_t aRloc16)
{
    if (mDeviceState != kDeviceStateChild)
    {
        mNetif.SetStateChangedFlags(OT_NET_ROLE);
    }

    if (mDeviceState == kDeviceStateLeader)
    {
        mNetif.RemoveUnicastAddress(mLeaderAloc);
    }

    SetRloc16(aRloc16);
    mDeviceState = kDeviceStateChild;
    mParentRequestState = kParentIdle;
    mChildUpdateAttempts = 0;
    mNetif.GetMac().SetBeaconEnabled(false);

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0)
    {
        mParentRequestTimer.Start(Timer::SecToMsec(mTimeout) - kUnicastRetransmissionDelay * kMaxChildKeepAliveAttempts);
    }

    if ((mDeviceMode & ModeTlv::kModeFFD) != 0)
    {
        mNetif.GetMle().HandleChildStart(mParentRequestMode);
    }

    mNetif.GetNetworkDataLocal().ClearResubmitDelayTimer();
    mNetif.GetIp6().SetForwardingEnabled(false);
    mNetif.GetIp6().mMpl.SetTimerExpirations(kMplChildDataMessageTimerExpirations);

    // Once the Thread device receives the new Active Commissioning Dataset, the device MUST
    // transmit its own Announce messages on the channel it was on prior to the attachment.
    if (mPreviousPanId != Mac::kPanIdBroadcast)
    {
        mPreviousPanId = Mac::kPanIdBroadcast;
        mNetif.GetAnnounceBeginServer().SendAnnounce(1 << mPreviousChannel);
    }

    otLogInfoMle(GetInstance(), "Mode -> Child");
    return OT_ERROR_NONE;
}

otError Mle::SetTimeout(uint32_t aTimeout)
{
    VerifyOrExit(mTimeout != aTimeout);

    if (aTimeout < kMinTimeout)
    {
        aTimeout = kMinTimeout;
    }

    mTimeout = aTimeout;

    mNetif.GetMeshForwarder().GetDataPollManager().RecalculatePollPeriod();

    if (mDeviceState == kDeviceStateChild)
    {
        SendChildUpdateRequest();
    }

exit:
    return OT_ERROR_NONE;
}

otError Mle::SetDeviceMode(uint8_t aDeviceMode)
{
    otError error = OT_ERROR_NONE;
    uint8_t oldMode = mDeviceMode;

    VerifyOrExit((aDeviceMode & ModeTlv::kModeFFD) == 0 || (aDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0,
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mDeviceMode != aDeviceMode);

    mDeviceMode = aDeviceMode;

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        break;

    case kDeviceStateChild:
        SetStateChild(GetRloc16());
        SendChildUpdateRequest();
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        if ((oldMode & ModeTlv::kModeFFD) != 0 && (aDeviceMode & ModeTlv::kModeFFD) == 0)
        {
            BecomeDetached();
        }

        break;
    }

exit:
    return error;
}

otError Mle::UpdateLinkLocalAddress(void)
{
    mNetif.RemoveUnicastAddress(mLinkLocal64);
    mLinkLocal64.GetAddress().SetIid(*mNetif.GetMac().GetExtAddress());
    mNetif.AddUnicastAddress(mLinkLocal64);

    mNetif.SetStateChangedFlags(OT_IP6_LL_ADDR_CHANGED);

    return OT_ERROR_NONE;
}

const uint8_t *Mle::GetMeshLocalPrefix(void) const
{
    return mMeshLocal16.GetAddress().mFields.m8;
}

otError Mle::SetMeshLocalPrefix(const uint8_t *aMeshLocalPrefix)
{
    if (memcmp(mMeshLocal64.GetAddress().mFields.m8, aMeshLocalPrefix, 8) == 0)
    {
        ExitNow();
    }

    // We must remove the old address before adding the new one.
    mNetif.RemoveUnicastAddress(mMeshLocal64);
    mNetif.RemoveUnicastAddress(mMeshLocal16);

    memcpy(mMeshLocal64.GetAddress().mFields.m8, aMeshLocalPrefix, 8);
    memcpy(mMeshLocal16.GetAddress().mFields.m8, mMeshLocal64.GetAddress().mFields.m8, 8);

    mLinkLocalAllThreadNodes.GetAddress().mFields.m8[3] = 64;
    memcpy(mLinkLocalAllThreadNodes.GetAddress().mFields.m8 + 4, mMeshLocal64.GetAddress().mFields.m8, 8);

    mRealmLocalAllThreadNodes.GetAddress().mFields.m8[3] = 64;
    memcpy(mRealmLocalAllThreadNodes.GetAddress().mFields.m8 + 4, mMeshLocal64.GetAddress().mFields.m8, 8);

    // Add the address back into the table.
    mNetif.AddUnicastAddress(mMeshLocal64);

    if (mDeviceState >= kDeviceStateChild)
    {
        mNetif.AddUnicastAddress(mMeshLocal16);
    }

    // update Leader ALOC
    if (mDeviceState == kDeviceStateLeader)
    {
        mNetif.RemoveUnicastAddress(mLeaderAloc);
        AddLeaderAloc();
    }

    // Changing the prefix also causes the mesh local address to be different.
    mNetif.SetStateChangedFlags(OT_IP6_ML_ADDR_CHANGED);

exit:
    return OT_ERROR_NONE;
}

const Ip6::Address *Mle::GetLinkLocalAllThreadNodesAddress(void) const
{
    return &mLinkLocalAllThreadNodes.GetAddress();
}

const Ip6::Address *Mle::GetRealmLocalAllThreadNodesAddress(void) const
{
    return &mRealmLocalAllThreadNodes.GetAddress();
}

uint16_t Mle::GetRloc16(void) const
{
    return mNetif.GetMac().GetShortAddress();
}

otError Mle::SetRloc16(uint16_t aRloc16)
{
    mNetif.RemoveUnicastAddress(mMeshLocal16);

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        // mesh-local 16
        mMeshLocal16.GetAddress().mFields.m16[7] = HostSwap16(aRloc16);
        mNetif.AddUnicastAddress(mMeshLocal16);
    }

    mNetif.GetMac().SetShortAddress(aRloc16);
    mNetif.GetIp6().mMpl.SetSeedId(aRloc16);

    return OT_ERROR_NONE;
}

uint8_t Mle::GetLeaderId(void) const
{
    return mLeaderData.GetLeaderRouterId();
}

void Mle::SetLeaderData(uint32_t aPartitionId, uint8_t aWeighting, uint8_t aLeaderRouterId)
{
    if (mLeaderData.GetPartitionId() != aPartitionId)
    {
        mNetif.SetStateChangedFlags(OT_NET_PARTITION_ID);
    }

    mLeaderData.SetPartitionId(aPartitionId);
    mLeaderData.SetWeighting(aWeighting);
    mLeaderData.SetLeaderRouterId(aLeaderRouterId);
}

const Ip6::Address &Mle::GetMeshLocal16(void) const
{
    return mMeshLocal16.GetAddress();
}

const Ip6::Address &Mle::GetMeshLocal64(void) const
{
    return mMeshLocal64.GetAddress();
}

otError Mle::GetLeaderAddress(Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 8);
    aAddress.mFields.m16[4] = HostSwap16(0x0000);
    aAddress.mFields.m16[5] = HostSwap16(0x00ff);
    aAddress.mFields.m16[6] = HostSwap16(0xfe00);
    aAddress.mFields.m16[7] = HostSwap16(GetRloc16(mLeaderData.GetLeaderRouterId()));

exit:
    return error;
}

otError Mle::GetLeaderAloc(Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 8);
    aAddress.mFields.m16[4] = HostSwap16(0x0000);
    aAddress.mFields.m16[5] = HostSwap16(0x00ff);
    aAddress.mFields.m16[6] = HostSwap16(0xfe00);
    aAddress.mFields.m16[7] = HostSwap16(kAloc16Leader);

exit:
    return error;
}

otError Mle::AddLeaderAloc(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mDeviceState == kDeviceStateLeader, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = GetLeaderAloc(mLeaderAloc.GetAddress()));

    error = mNetif.AddUnicastAddress(mLeaderAloc);

exit:
    return error;
}

const LeaderDataTlv &Mle::GetLeaderDataTlv(void)
{
    mLeaderData.SetDataVersion(mNetif.GetNetworkDataLeader().GetVersion());
    mLeaderData.SetStableDataVersion(mNetif.GetNetworkDataLeader().GetStableVersion());
    return mLeaderData;
}

otError Mle::GetLeaderData(otLeaderData &aLeaderData)
{
    const LeaderDataTlv &leaderData(GetLeaderDataTlv());
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mDeviceState != kDeviceStateDisabled && mDeviceState != kDeviceStateDetached,
                 error = OT_ERROR_DETACHED);

    aLeaderData.mPartitionId = leaderData.GetPartitionId();
    aLeaderData.mWeighting = leaderData.GetWeighting();
    aLeaderData.mDataVersion = leaderData.GetDataVersion();
    aLeaderData.mStableDataVersion = leaderData.GetStableDataVersion();
    aLeaderData.mLeaderRouterId = leaderData.GetLeaderRouterId();

exit:
    return error;
}

otError Mle::GetAssignLinkQuality(const Mac::ExtAddress aMacAddr, uint8_t &aLinkQuality)
{
    otError error;

    VerifyOrExit((memcmp(aMacAddr.m8, mAddr64.m8, OT_EXT_ADDRESS_SIZE)) == 0, error = OT_ERROR_INVALID_ARGS);

    aLinkQuality = mAssignLinkQuality;

    return OT_ERROR_NONE;

exit:
    return error;
}

void Mle::SetAssignLinkQuality(const Mac::ExtAddress aMacAddr, uint8_t aLinkQuality)
{
    isAssignLinkQuality = true;
    mAddr64 = aMacAddr;

    mAssignLinkQuality = aLinkQuality;

    switch (aLinkQuality)
    {
    case 3:
        mAssignLinkMargin = kMinAssignedLinkMargin3;
        break;

    case 2:
        mAssignLinkMargin = kMinAssignedLinkMargin2;
        break;

    case 1:
        mAssignLinkMargin = kMinAssignedLinkMargin1;
        break;

    case 0:
        mAssignLinkMargin = kMinAssignedLinkMargin0;

    default:
        break;
    }
}

void Mle::GenerateNonce(const Mac::ExtAddress &aMacAddr, uint32_t aFrameCounter, uint8_t aSecurityLevel,
                        uint8_t *aNonce)
{
    // source address
    memcpy(aNonce, aMacAddr.m8, sizeof(aMacAddr));
    aNonce += sizeof(aMacAddr);

    // frame counter
    aNonce[0] = (aFrameCounter >> 24) & 0xff;
    aNonce[1] = (aFrameCounter >> 16) & 0xff;
    aNonce[2] = (aFrameCounter >> 8) & 0xff;
    aNonce[3] = aFrameCounter & 0xff;
    aNonce += 4;

    // security level
    aNonce[0] = aSecurityLevel;
}

Message *Mle::NewMleMessage(void)
{
    Message *message;

    message = mSocket.NewMessage(0);
    VerifyOrExit(message != NULL);

    message->SetSubType(Message::kSubTypeMleGeneral);
    message->SetLinkSecurityEnabled(false);
    message->SetPriority(kMleMessagePriority);

exit:
    return message;
}

otError Mle::AppendHeader(Message &aMessage, Header::Command aCommand)
{
    otError error = OT_ERROR_NONE;
    Header header;

    header.Init();

    if (aCommand == Header::kCommandDiscoveryRequest ||
        aCommand == Header::kCommandDiscoveryResponse)
    {
        header.SetSecuritySuite(Header::kNoSecurity);
    }
    else
    {
        header.SetKeyIdMode2();
    }

    header.SetCommand(aCommand);

    SuccessOrExit(error = aMessage.Append(&header, header.GetLength()));

exit:
    return error;
}

otError Mle::AppendSourceAddress(Message &aMessage)
{
    SourceAddressTlv tlv;

    tlv.Init();
    tlv.SetRloc16(GetRloc16());

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendStatus(Message &aMessage, StatusTlv::Status aStatus)
{
    StatusTlv tlv;

    tlv.Init();
    tlv.SetStatus(aStatus);

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendMode(Message &aMessage, uint8_t aMode)
{
    ModeTlv tlv;

    tlv.Init();
    tlv.SetMode(aMode);

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendTimeout(Message &aMessage, uint32_t aTimeout)
{
    TimeoutTlv tlv;

    tlv.Init();
    tlv.SetTimeout(aTimeout);

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendChallenge(Message &aMessage, const uint8_t *aChallenge, uint8_t aChallengeLength)
{
    otError error;
    Tlv tlv;

    tlv.SetType(Tlv::kChallenge);
    tlv.SetLength(aChallengeLength);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));
    SuccessOrExit(error = aMessage.Append(aChallenge, aChallengeLength));
exit:
    return error;
}

otError Mle::AppendResponse(Message &aMessage, const uint8_t *aResponse, uint8_t aResponseLength)
{
    otError error;
    Tlv tlv;

    tlv.SetType(Tlv::kResponse);
    tlv.SetLength(aResponseLength);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));
    SuccessOrExit(error = aMessage.Append(aResponse, aResponseLength));

exit:
    return error;
}

otError Mle::AppendLinkFrameCounter(Message &aMessage)
{
    LinkFrameCounterTlv tlv;

    tlv.Init();
    tlv.SetFrameCounter(mNetif.GetKeyManager().GetMacFrameCounter());

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendMleFrameCounter(Message &aMessage)
{
    MleFrameCounterTlv tlv;

    tlv.Init();
    tlv.SetFrameCounter(mNetif.GetKeyManager().GetMleFrameCounter());

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendAddress16(Message &aMessage, uint16_t aRloc16)
{
    Address16Tlv tlv;

    tlv.Init();
    tlv.SetRloc16(aRloc16);

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendLeaderData(Message &aMessage)
{
    mLeaderData.Init();
    mLeaderData.SetDataVersion(mNetif.GetNetworkDataLeader().GetVersion());
    mLeaderData.SetStableDataVersion(mNetif.GetNetworkDataLeader().GetStableVersion());

    return aMessage.Append(&mLeaderData, sizeof(mLeaderData));
}

void Mle::FillNetworkDataTlv(NetworkDataTlv &aTlv, bool aStableOnly)
{
    uint8_t length;
    mNetif.GetNetworkDataLeader().GetNetworkData(aStableOnly, aTlv.GetNetworkData(), length);
    aTlv.SetLength(length);
}

otError Mle::AppendNetworkData(Message &aMessage, bool aStableOnly)
{
    otError error = OT_ERROR_NONE;
    NetworkDataTlv tlv;

    tlv.Init();
    FillNetworkDataTlv(tlv, aStableOnly);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(Tlv) + tlv.GetLength()));

exit:
    return error;
}

otError Mle::AppendTlvRequest(Message &aMessage, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    otError error;
    Tlv tlv;

    tlv.SetType(Tlv::kTlvRequest);
    tlv.SetLength(aTlvsLength);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));
    SuccessOrExit(error = aMessage.Append(aTlvs, aTlvsLength));

exit:
    return error;
}

otError Mle::AppendScanMask(Message &aMessage, uint8_t aScanMask)
{
    ScanMaskTlv tlv;

    tlv.Init();
    tlv.SetMask(aScanMask);

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendLinkMargin(Message &aMessage, uint8_t aLinkMargin)
{
    LinkMarginTlv tlv;

    tlv.Init();
    tlv.SetLinkMargin(aLinkMargin);

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendVersion(Message &aMessage)
{
    VersionTlv tlv;

    tlv.Init();
    tlv.SetVersion(kVersion);

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendAddressRegistration(Message &aMessage)
{
    otError error;
    Tlv tlv;
    AddressRegistrationEntry entry;
    Lowpan::Context context;
    uint8_t length = 0;
    uint16_t startOffset = aMessage.GetLength();

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    // write entries to message
    for (const Ip6::NetifUnicastAddress *addr = mNetif.GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        if (addr->GetAddress().IsLinkLocal() || addr->GetAddress() == mMeshLocal16.GetAddress())
        {
            continue;
        }

        if (mNetif.GetNetworkDataLeader().GetContext(addr->GetAddress(), context) == OT_ERROR_NONE)
        {
            // compressed entry
            entry.SetContextId(context.mContextId);
            entry.SetIid(addr->GetAddress().GetIid());
        }
        else
        {
            // uncompressed entry
            entry.SetUncompressed();
            entry.SetIp6Address(addr->GetAddress());
        }

        SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
        length += entry.GetLength();
    }

    tlv.SetLength(length);
    aMessage.Write(startOffset, sizeof(tlv), &tlv);

exit:
    return error;
}

otError Mle::AppendActiveTimestamp(Message &aMessage, bool aCouldUseLocal)
{
    otError error;
    ActiveTimestampTlv timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    if ((timestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp()) == NULL && aCouldUseLocal)
    {
        timestamp = mNetif.GetActiveDataset().GetLocal().GetTimestamp();
    }

    VerifyOrExit(timestamp, error = OT_ERROR_NONE);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = *timestamp;
    error = aMessage.Append(&timestampTlv, sizeof(timestampTlv));

exit:
    return error;
}

otError Mle::AppendPendingTimestamp(Message &aMessage)
{
    otError error;
    PendingTimestampTlv timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    timestamp = mNetif.GetPendingDataset().GetNetwork().GetTimestamp();
    VerifyOrExit(timestamp && timestamp->GetSeconds() != 0, error = OT_ERROR_NONE);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = *timestamp;
    error = aMessage.Append(&timestampTlv, sizeof(timestampTlv));

exit:
    return error;
}

void Mle::HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    static_cast<Mle *>(aContext)->HandleNetifStateChanged(aFlags);
}

void Mle::HandleNetifStateChanged(uint32_t aFlags)
{
    VerifyOrExit(mDeviceState != kDeviceStateDisabled);

    if ((aFlags & (OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED)) != 0)
    {
        if (!mNetif.IsUnicastAddress(mMeshLocal64.GetAddress()))
        {
            // Mesh Local EID was removed, choose a new one and add it back
            for (int i = 8; i < 16; i++)
            {
                mMeshLocal64.GetAddress().mFields.m8[i] = static_cast<uint8_t>(otPlatRandomGet());
            }

            mNetif.AddUnicastAddress(mMeshLocal64);
            mNetif.SetStateChangedFlags(OT_IP6_ML_ADDR_CHANGED);
        }

        if (mDeviceState == kDeviceStateChild && (mDeviceMode & ModeTlv::kModeFFD) == 0)
        {
            mSendChildUpdateRequest.Post();
        }
    }

    if ((aFlags & OT_THREAD_NETDATA_UPDATED) != 0)
    {
        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            mNetif.GetMle().HandleNetworkDataUpdateRouter();
        }
        else if ((aFlags & OT_NET_ROLE) == 0)
        {
            mSendChildUpdateRequest.Post();
        }

        mNetif.GetNetworkDataLocal().SendServerDataNotification();
    }

    if (aFlags & (OT_NET_ROLE | OT_NET_KEY_SEQUENCE_COUNTER))
    {
        Store();
    }

exit:
    return;
}

void Mle::HandleParentRequestTimer(void *aContext)
{
    static_cast<Mle *>(aContext)->HandleParentRequestTimer();
}

void Mle::HandleParentRequestTimer(void)
{
    switch (mParentRequestState)
    {
    case kParentIdle:
        if (mParent.GetState() == Neighbor::kStateValid)
        {
            SendChildUpdateRequest();
        }
        else
        {
            BecomeDetached();
        }

        break;

    case kParentSynchronize:
        mParentRequestState = kParentIdle;
        BecomeChild(kMleAttachAnyPartition);
        break;

    case kParentRequestStart:
        mParentRequestState = kParentRequestRouter;
        mParentCandidate.SetState(Neighbor::kStateInvalid);
        SendParentRequest();
        break;

    case kParentRequestRouter:
        mParentRequestState = kParentRequestChild;

        if (mParentCandidate.GetState() == Neighbor::kStateValid)
        {
            SendChildIdRequest();
            mParentRequestState = kChildIdRequest;
            mParentRequestTimer.Start(kParentRequestChildTimeout);
        }
        else
        {
            SendParentRequest();
        }

        break;

    case kParentRequestChild:
        mParentRequestState = kParentRequestChild;

        if (mParentCandidate.GetState() == Neighbor::kStateValid)
        {
            SendChildIdRequest();
            mParentRequestState = kChildIdRequest;
            mParentRequestTimer.Start(kParentRequestChildTimeout);
        }
        else
        {
            ResetParentCandidate();

            if (mReattachState == kReattachActive)
            {
                if (mNetif.GetPendingDataset().Restore() == OT_ERROR_NONE)
                {
                    mNetif.GetPendingDataset().ApplyConfiguration();
                    mReattachState = kReattachPending;
                    mParentRequestState = kParentRequestStart;
                    mParentRequestTimer.Start(kParentRequestRouterTimeout);
                }
                else
                {
                    mReattachState = kReattachStop;
                }
            }
            else if (mReattachState == kReattachPending)
            {
                mReattachState = kReattachStop;
                mNetif.GetActiveDataset().Restore();
                mNetif.GetPendingDataset().Set(mNetif.GetPendingDataset().GetLocal());
            }

            if (mReattachState == kReattachStop)
            {
                switch (mParentRequestMode)
                {
                case kMleAttachAnyPartition:
                    if (mPreviousPanId != Mac::kPanIdBroadcast)
                    {
                        mNetif.GetMac().SetChannel(mPreviousChannel);
                        mNetif.GetMac().SetPanId(mPreviousPanId);
                        mPreviousPanId = Mac::kPanIdBroadcast;
                        BecomeDetached();
                    }
                    else if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
                    {
                        SendOrphanAnnounce();
                        BecomeDetached();
                    }
                    else if (mNetif.GetMle().BecomeLeader() != OT_ERROR_NONE)
                    {
                        mParentRequestState = kParentIdle;
                        BecomeDetached();
                    }

                    break;

                case kMleAttachSamePartition1:
                    mParentRequestState = kParentIdle;
                    BecomeChild(kMleAttachSamePartition2);
                    break;

                case kMleAttachSamePartition2:
                    mParentRequestState = kParentIdle;
                    BecomeChild(kMleAttachAnyPartition);
                    break;

                case kMleAttachBetterPartition:
                    mParentRequestState = kParentIdle;

                    if (mDeviceState == kDeviceStateChild)
                    {
                        // Restart keep-alive timer as it was disturbed by attachment procedure.
                        mParentRequestTimer.Start(0);
                    }

                    break;
                }
            }
        }

        break;

    case kChildIdRequest:
        mParentRequestState = kParentIdle;
        ResetParentCandidate();

        if ((mParentRequestMode == kMleAttachBetterPartition) || (mDeviceState == kDeviceStateRouter) ||
            (mDeviceState == kDeviceStateLeader))
        {
            if (mDeviceState == kDeviceStateChild)
            {
                // Restart keep-alive timer as it was disturbed by attachment procedure.
                mParentRequestTimer.Start(0);
            }
        }
        else
        {
            BecomeDetached();
        }

        break;
    }
}

void Mle::HandleDelayedResponseTimer(void *aContext)
{
    static_cast<Mle *>(aContext)->HandleDelayedResponseTimer();
}

void Mle::HandleDelayedResponseTimer(void)
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
            if (SendMessage(*message, delayedResponse.GetDestination()) == OT_ERROR_NONE)
            {
                otLogInfoMle(GetInstance(), "Sent delayed response");
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

otError Mle::SendParentRequest(void)
{
    otError error = OT_ERROR_NONE;
    Message *message;
    uint8_t scanMask = 0;
    Ip6::Address destination;

    for (uint8_t i = 0; i < sizeof(mParentRequest.mChallenge); i++)
    {
        mParentRequest.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    switch (mParentRequestState)
    {
    case kParentRequestRouter:
        scanMask = ScanMaskTlv::kRouterFlag;

        if (mParentRequestMode == kMleAttachSamePartition1 ||
            mParentRequestMode == kMleAttachSamePartition2)
        {
            scanMask |= ScanMaskTlv::kEndDeviceFlag;
        }

        break;

    case kParentRequestChild:
        scanMask = ScanMaskTlv::kRouterFlag | ScanMaskTlv::kEndDeviceFlag;
        break;

    default:
        assert(false);
        break;
    }

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandParentRequest));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));
    SuccessOrExit(error = AppendChallenge(*message, mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge)));
    SuccessOrExit(error = AppendScanMask(*message, scanMask));
    SuccessOrExit(error = AppendVersion(*message));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0002);
    SuccessOrExit(error = SendMessage(*message, destination));

    if ((scanMask & ScanMaskTlv::kEndDeviceFlag) == 0)
    {
        otLogInfoMle(GetInstance(), "Sent parent request to routers");
    }
    else
    {
        otLogInfoMle(GetInstance(), "Sent parent request to all devices");
    }

exit:

    if ((scanMask & ScanMaskTlv::kEndDeviceFlag) == 0)
    {
        mParentRequestTimer.Start(kParentRequestRouterTimeout);
    }
    else
    {
        mParentRequestTimer.Start(kParentRequestChildTimeout);
    }

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return OT_ERROR_NONE;
}

otError Mle::SendChildIdRequest(void)
{
    otError error = OT_ERROR_NONE;
    uint8_t tlvs[] = {Tlv::kAddress16, Tlv::kNetworkData, Tlv::kRoute};
    Message *message;
    Ip6::Address destination;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildIdRequest));
    SuccessOrExit(error = AppendResponse(*message, mChildIdRequest.mChallenge, mChildIdRequest.mChallengeLength));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));
    SuccessOrExit(error = AppendTimeout(*message, mTimeout));
    SuccessOrExit(error = AppendVersion(*message));

    if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
    {
        SuccessOrExit(error = AppendAddressRegistration(*message));
    }

    SuccessOrExit(error = AppendTlvRequest(*message, tlvs, sizeof(tlvs)));
    SuccessOrExit(error = AppendActiveTimestamp(*message, true));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParentCandidate.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));
    otLogInfoMle(GetInstance(), "Sent Child ID Request");

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mNetif.GetMeshForwarder().GetDataPollManager().SetAttachMode(true);
        mNetif.GetMeshForwarder().SetRxOnWhenIdle(false);
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Mle::SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength,
                             uint16_t aDelay)
{
    otError error = OT_ERROR_NONE;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDataRequest));
    SuccessOrExit(error = AppendTlvRequest(*message, aTlvs, aTlvsLength));
    SuccessOrExit(error = AppendActiveTimestamp(*message, false));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    if (aDelay)
    {
        SuccessOrExit(error = AddDelayedResponse(*message, aDestination, aDelay));
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aDestination));

        if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            mNetif.GetMeshForwarder().GetDataPollManager().SendFastPolls(DataPollManager::kDefaultFastPolls);
        }
    }

    otLogInfoMle(GetInstance(), "Sent Data Request");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Mle::HandleSendChildUpdateRequest(void *aContext)
{
    static_cast<Mle *>(aContext)->HandleSendChildUpdateRequest();
}

void Mle::HandleSendChildUpdateRequest(void)
{
    // a Network Data udpate can cause a change to the IPv6 address configuration
    // only send a Child Update Request after we know there are no more pending changes
    if (mNetif.IsStateChangedCallbackPending())
    {
        mSendChildUpdateRequest.Post();
    }
    else
    {
        SendChildUpdateRequest();
    }
}

otError Mle::SendChildUpdateRequest(void)
{
    otError error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *message = NULL;

    if (mChildUpdateAttempts >= kMaxChildKeepAliveAttempts)
    {
        mChildUpdateAttempts = 0;
        BecomeDetached();
        ExitNow();
    }

    mParentRequestTimer.Start(kUnicastRetransmissionDelay);
    mChildUpdateAttempts++;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleChildUpdateRequest);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateRequest));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));

    if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
    {
        SuccessOrExit(error = AppendAddressRegistration(*message));
    }

    switch (mDeviceState)
    {
    case kDeviceStateDetached:
        for (uint8_t i = 0; i < sizeof(mParentRequest.mChallenge); i++)
        {
            mParentRequest.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
        }

        SuccessOrExit(error = AppendChallenge(*message, mParentRequest.mChallenge,
                                              sizeof(mParentRequest.mChallenge)));
        break;

    case kDeviceStateChild:
        SuccessOrExit(error = AppendSourceAddress(*message));
        SuccessOrExit(error = AppendLeaderData(*message));
        SuccessOrExit(error = AppendTimeout(*message, mTimeout));
        break;

    case kDeviceStateDisabled:
    case kDeviceStateRouter:
    case kDeviceStateLeader:
        assert(false);
        break;
    }

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParent.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle(GetInstance(), "Sent Child Update Request to parent");

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mNetif.GetMeshForwarder().GetDataPollManager().SetAttachMode(true);
        mNetif.GetMeshForwarder().SetRxOnWhenIdle(false);
    }
    else
    {
        mNetif.GetMeshForwarder().SetRxOnWhenIdle(true);
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Mle::SendChildUpdateResponse(const uint8_t *aTlvs, uint8_t aNumTlvs, const ChallengeTlv &aChallenge)
{
    otError error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateResponse));
    SuccessOrExit(error = AppendSourceAddress(*message));
    SuccessOrExit(error = AppendLeaderData(*message));

    for (int i = 0; i < aNumTlvs; i++)
    {
        switch (aTlvs[i])
        {
        case Tlv::kTimeout:
            SuccessOrExit(error = AppendTimeout(*message, mTimeout));
            break;

        case Tlv::kAddressRegistration:
            if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
            {
                SuccessOrExit(error = AppendAddressRegistration(*message));
            }

            break;

        case Tlv::kResponse:
            SuccessOrExit(error = AppendResponse(*message, aChallenge.GetChallenge(), aChallenge.GetLength()));
            break;

        case Tlv::kLinkFrameCounter:
            SuccessOrExit(error = AppendLinkFrameCounter(*message));
            break;

        case Tlv::kMleFrameCounter:
            SuccessOrExit(error = AppendMleFrameCounter(*message));
            break;
        }
    }

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParent.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle(GetInstance(), "Sent Child Update Response to parent");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Mle::SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce)
{
    otError error = OT_ERROR_NONE;
    ChannelTlv channel;
    PanIdTlv panid;
    ActiveTimestampTlv activeTimestamp;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetLinkSecurityEnabled(true);
    message->SetSubType(Message::kSubTypeMleAnnounce);
    message->SetChannel(aChannel);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandAnnounce));

    channel.Init();
    channel.SetChannelPage(0);
    channel.SetChannel(mNetif.GetMac().GetChannel());
    SuccessOrExit(error = message->Append(&channel, sizeof(channel)));

    if (aOrphanAnnounce)
    {
        activeTimestamp.Init();
        activeTimestamp.SetSeconds(0);
        activeTimestamp.SetTicks(0);
        activeTimestamp.SetAuthoritative(true);

        SuccessOrExit(error = message->Append(&activeTimestamp, sizeof(activeTimestamp)));
    }
    else
    {
        SuccessOrExit(error = AppendActiveTimestamp(*message, false));
    }

    panid.Init();
    panid.SetPanId(mNetif.GetMac().GetPanId());
    SuccessOrExit(error = message->Append(&panid, sizeof(panid)));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle(GetInstance(), "sent announce on channel %d", aChannel);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Mle::SendOrphanAnnounce(void)
{
    MeshCoP::ChannelMask0Tlv *channelMask;
    uint8_t channel;

    channelMask = static_cast<MeshCoP::ChannelMask0Tlv *>(mNetif.GetActiveDataset().GetNetwork().Get(
                                                              MeshCoP::Tlv::kChannelMask));

    VerifyOrExit(channelMask != NULL);

    // find next channel in the Active Operational Dataset Channel Mask
    channel = mAnnounceChannel;

    while (!channelMask->IsChannelSet(channel))
    {
        channel++;

        if (channel > kPhyMaxChannel)
        {
            channel = kPhyMinChannel;
        }

        VerifyOrExit(channel != mAnnounceChannel);
    }

    // Send Announce message
    SendAnnounce(channel, true);

    // Move to next channel
    mAnnounceChannel = channel + 1;

    if (mAnnounceChannel > kPhyMaxChannel)
    {
        mAnnounceChannel = kPhyMinChannel;
    }

exit:
    return;
}

otError Mle::SendMessage(Message &aMessage, const Ip6::Address &aDestination)
{
    otError error = OT_ERROR_NONE;
    Header header;
    uint32_t keySequence;
    uint8_t nonce[13];
    uint8_t tag[4];
    uint8_t tagLength;
    Crypto::AesCcm aesCcm;
    uint8_t buf[64];
    uint16_t length;
    Ip6::MessageInfo messageInfo;

    aMessage.Read(0, sizeof(header), &header);

    if (header.GetSecuritySuite() == Header::k154Security)
    {
        header.SetFrameCounter(mNetif.GetKeyManager().GetMleFrameCounter());

        keySequence = mNetif.GetKeyManager().GetCurrentKeySequence();
        header.SetKeyId(keySequence);

        aMessage.Write(0, header.GetLength(), &header);

        GenerateNonce(*mNetif.GetMac().GetExtAddress(),
                      mNetif.GetKeyManager().GetMleFrameCounter(),
                      Mac::Frame::kSecEncMic32,
                      nonce);

        aesCcm.SetKey(mNetif.GetKeyManager().GetCurrentMleKey(), 16);
        aesCcm.Init(16 + 16 + header.GetHeaderLength(), aMessage.GetLength() - (header.GetLength() - 1),
                    sizeof(tag), nonce, sizeof(nonce));

        aesCcm.Header(&mLinkLocal64.GetAddress(), sizeof(mLinkLocal64.GetAddress()));
        aesCcm.Header(&aDestination, sizeof(aDestination));
        aesCcm.Header(header.GetBytes() + 1, header.GetHeaderLength());

        aMessage.SetOffset(header.GetLength() - 1);

        while (aMessage.GetOffset() < aMessage.GetLength())
        {
            length = aMessage.Read(aMessage.GetOffset(), sizeof(buf), buf);
            aesCcm.Payload(buf, buf, length, true);
            aMessage.Write(aMessage.GetOffset(), length, buf);
            aMessage.MoveOffset(length);
        }

        tagLength = sizeof(tag);
        aesCcm.Finalize(tag, &tagLength);
        SuccessOrExit(error = aMessage.Append(tag, tagLength));

        mNetif.GetKeyManager().IncrementMleFrameCounter();
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(mLinkLocal64.GetAddress());
    messageInfo.SetPeerPort(kUdpPort);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());
    messageInfo.SetHopLimit(255);

    SuccessOrExit(error = mSocket.SendTo(aMessage, messageInfo));

exit:
    return error;
}

otError Mle::AddDelayedResponse(Message &aMessage, const Ip6::Address &aDestination, uint16_t aDelay)
{
    otError error = OT_ERROR_NONE;
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

void Mle::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Mle *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                   *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Mle::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Header header;
    uint32_t keySequence;
    const uint8_t *mleKey;
    uint32_t frameCounter;
    uint8_t messageTag[4];
    uint16_t messageTagLength;
    uint8_t nonce[13];
    Mac::ExtAddress macAddr;
    Crypto::AesCcm aesCcm;
    uint16_t mleOffset;
    uint8_t buf[64];
    uint16_t length;
    uint8_t tag[4];
    uint8_t tagLength;
    uint8_t command;
    Neighbor *neighbor;

    aMessage.Read(aMessage.GetOffset(), sizeof(header), &header);
    VerifyOrExit(header.IsValid());

    assert(aMessageInfo.GetLinkInfo() != NULL);

    if (header.GetSecuritySuite() == Header::kNoSecurity)
    {
        aMessage.MoveOffset(header.GetLength());

        switch (header.GetCommand())
        {
        case Header::kCommandDiscoveryRequest:
            mNetif.GetMle().HandleDiscoveryRequest(aMessage, aMessageInfo);
            break;

        case Header::kCommandDiscoveryResponse:
            HandleDiscoveryResponse(aMessage, aMessageInfo);
            break;

        default:
            break;
        }

        ExitNow();
    }

    VerifyOrExit(mDeviceState != kDeviceStateDisabled && header.GetSecuritySuite() == Header::k154Security);

    keySequence = header.GetKeyId();

    if (keySequence == mNetif.GetKeyManager().GetCurrentKeySequence())
    {
        mleKey = mNetif.GetKeyManager().GetCurrentMleKey();
    }
    else
    {
        mleKey = mNetif.GetKeyManager().GetTemporaryMleKey(keySequence);
    }

    aMessage.MoveOffset(header.GetLength() - 1);

    frameCounter = header.GetFrameCounter();

    messageTagLength = aMessage.Read(aMessage.GetLength() - sizeof(messageTag), sizeof(messageTag), messageTag);
    VerifyOrExit(messageTagLength == sizeof(messageTag));
    SuccessOrExit(aMessage.SetLength(aMessage.GetLength() - sizeof(messageTag)));

    macAddr.Set(aMessageInfo.GetPeerAddr());
    GenerateNonce(macAddr, frameCounter, Mac::Frame::kSecEncMic32, nonce);

    aesCcm.SetKey(mleKey, 16);
    aesCcm.Init(sizeof(aMessageInfo.GetPeerAddr()) + sizeof(aMessageInfo.GetSockAddr()) + header.GetHeaderLength(),
                aMessage.GetLength() - aMessage.GetOffset(), sizeof(messageTag), nonce, sizeof(nonce));
    aesCcm.Header(&aMessageInfo.GetPeerAddr(), sizeof(aMessageInfo.GetPeerAddr()));
    aesCcm.Header(&aMessageInfo.GetSockAddr(), sizeof(aMessageInfo.GetSockAddr()));
    aesCcm.Header(header.GetBytes() + 1, header.GetHeaderLength());

    mleOffset = aMessage.GetOffset();

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        length = aMessage.Read(aMessage.GetOffset(), sizeof(buf), buf);
        aesCcm.Payload(buf, buf, length, false);
        aMessage.Write(aMessage.GetOffset(), length, buf);
        aMessage.MoveOffset(length);
    }

    tagLength = sizeof(tag);
    aesCcm.Finalize(tag, &tagLength);
    VerifyOrExit(messageTagLength == tagLength && memcmp(messageTag, tag, tagLength) == 0);

    if (keySequence > mNetif.GetKeyManager().GetCurrentKeySequence())
    {
        mNetif.GetKeyManager().SetCurrentKeySequence(keySequence);
    }

    aMessage.SetOffset(mleOffset);

    aMessage.Read(aMessage.GetOffset(), sizeof(command), &command);
    aMessage.MoveOffset(sizeof(command));

    switch (mDeviceState)
    {
    case kDeviceStateDetached:
    case kDeviceStateChild:
        neighbor = GetNeighbor(macAddr);
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        if (command == Header::kCommandChildIdResponse)
        {
            neighbor = GetNeighbor(macAddr);
        }
        else
        {
            neighbor = mNetif.GetMle().GetNeighbor(macAddr);
        }

        break;

    default:
        neighbor = NULL;
        break;
    }

    if (neighbor != NULL && neighbor->GetState() == Neighbor::kStateValid)
    {
        if (keySequence == neighbor->GetKeySequence())
        {
            if (frameCounter < neighbor->GetMleFrameCounter())
            {
                otLogDebgMle(GetInstance(), "mle frame reject 1");
                ExitNow();
            }
        }
        else
        {
            if (keySequence <= neighbor->GetKeySequence())
            {
                otLogDebgMle(GetInstance(), "mle frame reject 2");
                ExitNow();
            }

            neighbor->SetKeySequence(keySequence);
            neighbor->SetLinkFrameCounter(0);
        }

        neighbor->SetMleFrameCounter(frameCounter + 1);
    }
    else
    {
        if (!(command == Header::kCommandLinkRequest ||
              command == Header::kCommandLinkAccept ||
              command == Header::kCommandLinkAcceptAndRequest ||
              command == Header::kCommandAdvertisement ||
              command == Header::kCommandParentRequest ||
              command == Header::kCommandParentResponse ||
              command == Header::kCommandChildIdRequest ||
              command == Header::kCommandChildUpdateRequest ||
              command == Header::kCommandChildUpdateResponse ||
              command == Header::kCommandAnnounce))
        {
            otLogDebgMle(GetInstance(), "mle sequence unknown! %d", command);
            ExitNow();
        }
    }

    switch (command)
    {
    case Header::kCommandLinkRequest:
        mNetif.GetMle().HandleLinkRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandLinkAccept:
        mNetif.GetMle().HandleLinkAccept(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandLinkAcceptAndRequest:
        mNetif.GetMle().HandleLinkAcceptAndRequest(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandAdvertisement:
        HandleAdvertisement(aMessage, aMessageInfo);
        break;

    case Header::kCommandDataRequest:
        mNetif.GetMle().HandleDataRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandDataResponse:
        HandleDataResponse(aMessage, aMessageInfo);
        break;

    case Header::kCommandParentRequest:
        mNetif.GetMle().HandleParentRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandParentResponse:
        HandleParentResponse(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdRequest:
        mNetif.GetMle().HandleChildIdRequest(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdResponse:
        HandleChildIdResponse(aMessage, aMessageInfo);
        break;

    case Header::kCommandChildUpdateRequest:
        if (mDeviceState == kDeviceStateLeader || mDeviceState == kDeviceStateRouter)
        {
            mNetif.GetMle().HandleChildUpdateRequest(aMessage, aMessageInfo);
        }
        else
        {
            HandleChildUpdateRequest(aMessage, aMessageInfo);
        }

        break;

    case Header::kCommandChildUpdateResponse:
        if (mDeviceState == kDeviceStateLeader || mDeviceState == kDeviceStateRouter)
        {
            mNetif.GetMle().HandleChildUpdateResponse(aMessage, aMessageInfo, keySequence);
        }
        else
        {
            HandleChildUpdateResponse(aMessage, aMessageInfo);
        }

        break;

    case Header::kCommandAnnounce:
        HandleAnnounce(aMessage, aMessageInfo);
        break;
    }

exit:
    return;
}

otError Mle::HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Mac::ExtAddress macAddr;
    bool isNeighbor;
    Neighbor *neighbor;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv leaderData;
    RouteTlv route;
    uint8_t tlvs[] = {Tlv::kNetworkData};
    uint16_t delay;

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    otLogInfoMle(GetInstance(), "Received advertisement from %04x", sourceAddress.GetRloc16());

    if (mDeviceState != kDeviceStateDetached)
    {
        SuccessOrExit(error = mNetif.GetMle().HandleAdvertisement(aMessage, aMessageInfo));
    }

    macAddr.Set(aMessageInfo.GetPeerAddr());

    isNeighbor = false;

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        break;

    case kDeviceStateChild:
        if (memcmp(&mParent.GetExtAddress(), &macAddr, sizeof(macAddr)))
        {
            break;
        }

        if ((mParent.GetRloc16() == sourceAddress.GetRloc16()) &&
            (leaderData.GetPartitionId() != mLeaderData.GetPartitionId() ||
             leaderData.GetLeaderRouterId() != GetLeaderId()))
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

            if ((mDeviceMode & ModeTlv::kModeFFD) &&
                (Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE) &&
                route.IsValid())
            {
                // Overwrite Route Data
                mNetif.GetMle().ProcessRouteTlv(route);
            }

            mRetrieveNewNetworkData = true;
        }

        isNeighbor = true;
        mParent.SetLastHeard(Timer::GetNow());
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        if ((neighbor = mNetif.GetMle().GetNeighbor(macAddr)) != NULL &&
            neighbor->GetState() == Neighbor::kStateValid)
        {
            isNeighbor = true;
        }

        break;
    }

    if (isNeighbor)
    {
        if (mRetrieveNewNetworkData ||
            (static_cast<int8_t>(leaderData.GetDataVersion() - mNetif.GetNetworkDataLeader().GetVersion()) > 0))
        {
            delay = otPlatRandomGet() % kMleMaxResponseDelay;
            SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay);
        }
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Advertisement");
    }

    return error;
}

otError Mle::HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error;

    otLogInfoMle(GetInstance(), "Received Data Response");

    error = HandleLeaderData(aMessage, aMessageInfo);

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Data Response");
    }

    return error;
}

otError Mle::HandleLeaderData(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    LeaderDataTlv leaderData;
    NetworkDataTlv networkData;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    uint16_t activeDatasetOffset = 0;
    uint16_t pendingDatasetOffset = 0;
    bool dataRequest = false;
    Tlv tlv;
    uint16_t delay;

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    if ((leaderData.GetPartitionId() != mLeaderData.GetPartitionId()) ||
        (leaderData.GetWeighting() != mLeaderData.GetWeighting()) ||
        (leaderData.GetLeaderRouterId() != GetLeaderId()))
    {
        if (mDeviceState == kDeviceStateChild)
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());
            mRetrieveNewNetworkData = true;
        }
        else
        {
            ExitNow(error = OT_ERROR_DROP);
        }
    }
    else if (!mRetrieveNewNetworkData)
    {
        int8_t diff;

        if (mDeviceMode & ModeTlv::kModeFullNetworkData)
        {
            diff = static_cast<int8_t>(leaderData.GetDataVersion() - mNetif.GetNetworkDataLeader().GetVersion());
        }
        else
        {
            diff = static_cast<int8_t>(leaderData.GetStableDataVersion() -
                                       mNetif.GetNetworkDataLeader().GetStableVersion());
        }

        VerifyOrExit(diff > 0);
    }

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);
        timestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if ((timestamp == NULL || timestamp->Compare(activeTimestamp) != 0) &&
            (Tlv::GetOffset(aMessage, Tlv::kActiveDataset, activeDatasetOffset) != OT_ERROR_NONE))
        {
            ExitNow(dataRequest = true);
        }
    }
    else
    {
        activeTimestamp.SetLength(0);
    }

    // Pending Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);
        timestamp = mNetif.GetPendingDataset().GetNetwork().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if ((timestamp == NULL || timestamp->Compare(pendingTimestamp) != 0) &&
            (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, pendingDatasetOffset) != OT_ERROR_NONE))
        {
            ExitNow(dataRequest = true);
        }
    }
    else
    {
        pendingTimestamp.SetLength(0);
    }

    if (Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData) == OT_ERROR_NONE)
    {
        VerifyOrExit(networkData.IsValid(), error = OT_ERROR_PARSE);

        mNetif.GetNetworkDataLeader().SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                                     (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                                     networkData.GetNetworkData(), networkData.GetLength());
    }
    else
    {
        ExitNow(dataRequest = true);
    }

    // Active Dataset
    if (activeTimestamp.GetLength() > 0)
    {
        if (activeDatasetOffset > 0)
        {
            aMessage.Read(activeDatasetOffset, sizeof(tlv), &tlv);
            mNetif.GetActiveDataset().Set(activeTimestamp, aMessage, activeDatasetOffset + sizeof(tlv),
                                          tlv.GetLength());
        }
    }
    else
    {
        mNetif.GetActiveDataset().Clear(false);
    }

    // Pending Dataset
    if (pendingTimestamp.GetLength() > 0)
    {
        if (pendingDatasetOffset > 0)
        {
            aMessage.Read(pendingDatasetOffset, sizeof(tlv), &tlv);
            mNetif.GetPendingDataset().Set(pendingTimestamp, aMessage, pendingDatasetOffset + sizeof(tlv),
                                           tlv.GetLength());
        }
    }
    else
    {
        mNetif.GetPendingDataset().Clear(false);
    }

    mRetrieveNewNetworkData = false;

exit:

    (void)aMessageInfo;

    if (dataRequest)
    {
        static const uint8_t tlvs[] = {Tlv::kNetworkData};

        delay = aMessageInfo.GetSockAddr().IsMulticast() ? (otPlatRandomGet() % kMleMaxResponseDelay) : 0;

        SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay);
    }

    return error;
}

bool Mle::IsBetterParent(uint16_t aRloc16, uint8_t aLinkQuality, ConnectivityTlv &aConnectivityTlv)
{
    bool rval = false;

    uint8_t candidateLinkQualityIn = mParentCandidate.GetLinkInfo().GetLinkQuality(mNetif.GetMac().GetNoiseFloor());
    uint8_t candidateTwoWayLinkQuality = (candidateLinkQualityIn < mParentCandidate.GetLinkQualityOut())
                                         ?  candidateLinkQualityIn : mParentCandidate.GetLinkQualityOut();

    if (aLinkQuality != candidateTwoWayLinkQuality)
    {
        ExitNow(rval = (aLinkQuality > candidateTwoWayLinkQuality));
    }

    if (IsActiveRouter(aRloc16) != IsActiveRouter(mParentCandidate.GetRloc16()))
    {
        ExitNow(rval = IsActiveRouter(aRloc16));
    }

    if (aConnectivityTlv.GetParentPriority() != mParentPriority)
    {
        ExitNow(rval = (aConnectivityTlv.GetParentPriority() > mParentPriority));
    }

    if (aConnectivityTlv.GetLinkQuality3() != mParentLinkQuality3)
    {
        ExitNow(rval = (aConnectivityTlv.GetLinkQuality3() > mParentLinkQuality3));
    }

    if (aConnectivityTlv.GetLinkQuality2() != mParentLinkQuality2)
    {
        ExitNow(rval = (aConnectivityTlv.GetLinkQuality2() > mParentLinkQuality2));
    }

    if (aConnectivityTlv.GetLinkQuality1() != mParentLinkQuality1)
    {
        ExitNow(rval = (aConnectivityTlv.GetLinkQuality1() > mParentLinkQuality1));
    }

exit:
    return rval;
}

void Mle::ResetParentCandidate(void)
{
    memset(&mParentCandidate, 0, sizeof(mParentCandidate));
    mParentCandidate.SetState(Neighbor::kStateInvalid);
}

otError Mle::HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                  uint32_t aKeySequence)
{
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    ResponseTlv response;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv leaderData;
    LinkMarginTlv linkMarginTlv;
    uint8_t linkMargin;
    uint8_t linkQuality;
    ConnectivityTlv connectivity;
    LinkFrameCounterTlv linkFrameCounter;
    MleFrameCounterTlv mleFrameCounter;
    ChallengeTlv challenge;
    int8_t diff;

    otLogInfoMle(GetInstance(), "Received Parent Response");

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid() &&
                 memcmp(response.GetResponse(), mParentRequest.mChallenge, response.GetLength()) == 0,
                 error = OT_ERROR_PARSE);

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    // Link Quality
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMarginTlv), linkMarginTlv));
    VerifyOrExit(linkMarginTlv.IsValid(), error = OT_ERROR_PARSE);

    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(mNetif.GetMac().GetNoiseFloor(), threadMessageInfo->mRss);

    if (linkMargin > linkMarginTlv.GetLinkMargin())
    {
        linkMargin = linkMarginTlv.GetLinkMargin();
    }

    linkQuality = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin);

    VerifyOrExit(mParentRequestState != kParentRequestRouter || linkQuality == 3);

    // Connectivity
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kConnectivity, sizeof(connectivity), connectivity));
    VerifyOrExit(connectivity.IsValid(), error = OT_ERROR_PARSE);

    if ((mDeviceMode & ModeTlv::kModeFFD) && (mDeviceState != kDeviceStateDetached))
    {
        diff = static_cast<int8_t>(connectivity.GetIdSequence() - mNetif.GetMle().GetRouterIdSequence());

        switch (mParentRequestMode)
        {
        case kMleAttachAnyPartition:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId() || diff > 0);
            break;

        case kMleAttachSamePartition1:
        case kMleAttachSamePartition2:
            VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());
            VerifyOrExit(diff > 0 ||
                         (diff == 0 && mNetif.GetMle().GetLeaderAge() < mNetif.GetMle().GetNetworkIdTimeout()));
            break;

        case kMleAttachBetterPartition:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId());
            VerifyOrExit(mNetif.GetMle().ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                           mNetif.GetMle().IsSingleton(), mLeaderData) > 0);
            break;
        }
    }

    // if already have a candidate parent, only seek a better parent
    if (mParentCandidate.GetState() == Neighbor::kStateValid)
    {
        int compare = 0;

        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            compare = mNetif.GetMle().ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                        mParentIsSingleton, mParentLeaderData);
        }

        // only consider partitions that are the same or better
        VerifyOrExit(compare >= 0);

        // only consider better parents if the partitions are the same
        VerifyOrExit(compare != 0 || IsBetterParent(sourceAddress.GetRloc16(), linkQuality, connectivity));
    }

    // Link Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter), linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);

    // Mle Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) == OT_ERROR_NONE)
    {
        VerifyOrExit(mleFrameCounter.IsValid());
    }
    else
    {
        mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);
    memcpy(mChildIdRequest.mChallenge, challenge.GetChallenge(), challenge.GetLength());
    mChildIdRequest.mChallengeLength = challenge.GetLength();

    mParentCandidate.GetExtAddress().Set(aMessageInfo.GetPeerAddr());
    mParentCandidate.SetRloc16(sourceAddress.GetRloc16());
    mParentCandidate.SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    mParentCandidate.SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    mParentCandidate.SetDeviceMode(ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData |
                                   ModeTlv::kModeSecureDataRequest);
    mParentCandidate.GetLinkInfo().Clear();
    mParentCandidate.GetLinkInfo().AddRss(mNetif.GetMac().GetNoiseFloor(), threadMessageInfo->mRss);
    mParentCandidate.ResetLinkFailures();
    mParentCandidate.SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMarginTlv.GetLinkMargin()));
    mParentCandidate.SetState(Neighbor::kStateValid);
    mParentCandidate.SetKeySequence(aKeySequence);

    mParentPriority = connectivity.GetParentPriority();
    mParentLinkQuality3 = connectivity.GetLinkQuality3();
    mParentLinkQuality2 = connectivity.GetLinkQuality2();
    mParentLinkQuality1 = connectivity.GetLinkQuality1();
    mParentLeaderCost = connectivity.GetLeaderCost();
    mParentLeaderData = leaderData;
    mParentIsSingleton = connectivity.GetActiveRouters() <= 1;

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Parent Response");
    }

    return error;
}

otError Mle::HandleChildIdResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    LeaderDataTlv leaderData;
    SourceAddressTlv sourceAddress;
    Address16Tlv shortAddress;
    NetworkDataTlv networkData;
    RouteTlv route;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    Tlv tlv;
    uint16_t offset;

    otLogInfoMle(GetInstance(), "Received Child ID Response");

    VerifyOrExit(mParentRequestState == kChildIdRequest);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    // ShortAddress
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kAddress16, sizeof(shortAddress), shortAddress));
    VerifyOrExit(shortAddress.IsValid(), error = OT_ERROR_PARSE);

    // Network Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData));

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);

        // Active Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kActiveDataset, offset) == OT_ERROR_NONE)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            mNetif.GetActiveDataset().Set(activeTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength());
        }
        else if (mNetif.GetActiveDataset().GetNetwork().GetTimestamp() == NULL &&
                 mNetif.GetActiveDataset().GetLocal().GetTimestamp() != NULL)
        {
            mNetif.GetActiveDataset().Set(mNetif.GetActiveDataset().GetLocal());
        }
    }

    // clear local Pending Dataset if device succeed to reattach using stored Pending Dataset
    if (mReattachState == kReattachPending)
    {
        mNetif.GetPendingDataset().GetLocal().Clear(true);
    }

    // Pending Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);

        // Pending Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, offset) == OT_ERROR_NONE)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            mNetif.GetPendingDataset().Set(pendingTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength());
        }
    }
    else
    {
        mNetif.GetPendingDataset().Clear(true);
    }

    // Parent Attach Success
    mParentRequestTimer.Stop();
    mReattachState = kReattachStop;
    SetStateDetached();

    SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mNetif.GetMeshForwarder().GetDataPollManager().SetAttachMode(false);
        mNetif.GetMeshForwarder().SetRxOnWhenIdle(false);
    }
    else
    {
        mNetif.GetMeshForwarder().SetRxOnWhenIdle(true);
    }

    // Route
    if ((Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE) &&
        (mDeviceMode & ModeTlv::kModeFFD))
    {
        SuccessOrExit(error = mNetif.GetMle().ProcessRouteTlv(route));
    }

    mParent = mParentCandidate;
    ResetParentCandidate();

    mParent.SetRloc16(sourceAddress.GetRloc16());

    mNetif.GetNetworkDataLeader().SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                                 (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                                 networkData.GetNetworkData(), networkData.GetLength());

    mNetif.GetActiveDataset().ApplyConfiguration();

    SuccessOrExit(error = SetStateChild(shortAddress.GetRloc16()));

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Child ID Response");
    }

    (void)aMessageInfo;
    return error;
}

otError Mle::HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static const uint8_t kMaxResponseTlvs = 5;

    otError error = OT_ERROR_NONE;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv leaderData;
    NetworkDataTlv networkData;
    ChallengeTlv challenge;
    TlvRequestTlv tlvRequest;
    uint8_t dataRequestTlvs[] = {Tlv::kNetworkData};
    uint8_t tlvs[kMaxResponseTlvs] = {};
    uint8_t numTlvs = 0;

    otLogInfoMle(GetInstance(), "Received Child Update Request from parent");

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);
    VerifyOrExit(mParent.GetRloc16() == sourceAddress.GetRloc16(), error = OT_ERROR_DROP);

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == OT_ERROR_NONE)
    {
        VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);
        SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

        if ((mDeviceMode & ModeTlv::kModeFullNetworkData &&
             leaderData.GetDataVersion() != mNetif.GetNetworkDataLeader().GetVersion()) ||
            ((mDeviceMode & ModeTlv::kModeFullNetworkData) == 0 &&
             leaderData.GetStableDataVersion() != mNetif.GetNetworkDataLeader().GetStableVersion()))
        {
            mRetrieveNewNetworkData = true;
        }

        // Network Data
        if (Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData) == OT_ERROR_NONE)
        {
            VerifyOrExit(networkData.IsValid(), error = OT_ERROR_PARSE);
            mNetif.GetNetworkDataLeader().SetNetworkData(leaderData.GetDataVersion(),
                                                         leaderData.GetStableDataVersion(),
                                                         (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                                         networkData.GetNetworkData(),
                                                         networkData.GetLength());
        }
    }

    // TLV Request
    if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == OT_ERROR_NONE)
    {
        VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= sizeof(tlvs), error = OT_ERROR_PARSE);
        memcpy(tlvs, tlvRequest.GetTlvs(), tlvRequest.GetLength());
        numTlvs += tlvRequest.GetLength();
    }

    // Challenge
    if (Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge) == OT_ERROR_NONE)
    {
        VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(static_cast<size_t>(numTlvs + 3) <= sizeof(tlvs), error = OT_ERROR_NO_BUFS);
        tlvs[numTlvs++] = Tlv::kResponse;
        tlvs[numTlvs++] = Tlv::kMleFrameCounter;
        tlvs[numTlvs++] = Tlv::kLinkFrameCounter;
    }

    SuccessOrExit(error = SendChildUpdateResponse(tlvs, numTlvs, challenge));

    if (mRetrieveNewNetworkData)
    {
        SendDataRequest(aMessageInfo.GetPeerAddr(), dataRequestTlvs, sizeof(dataRequestTlvs), 0);
    }

exit:
    return error;
}

otError Mle::HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    StatusTlv status;
    ModeTlv mode;
    ResponseTlv response;
    LinkFrameCounterTlv linkFrameCounter;
    MleFrameCounterTlv mleFrameCounter;
    SourceAddressTlv sourceAddress;
    TimeoutTlv timeout;

    otLogInfoMle(GetInstance(), "Received Child Update Response from parent");

    // Status
    if (Tlv::GetTlv(aMessage, Tlv::kStatus, sizeof(status), status) == OT_ERROR_NONE)
    {
        BecomeDetached();
        ExitNow();
    }

    // Mode
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMode, sizeof(mode), mode));
    VerifyOrExit(mode.IsValid(), error = OT_ERROR_PARSE);
    VerifyOrExit(mode.GetMode() == mDeviceMode, error = OT_ERROR_DROP);

    switch (mDeviceState)
    {
    case kDeviceStateDetached:
        // Response
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
        VerifyOrExit(response.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(memcmp(response.GetResponse(), mParentRequest.mChallenge,
                            sizeof(mParentRequest.mChallenge)) == 0,
                     error = OT_ERROR_DROP);

        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter),
                                          linkFrameCounter));
        VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);

        if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) ==
            OT_ERROR_NONE)
        {
            VerifyOrExit(mleFrameCounter.IsValid(), error = OT_ERROR_PARSE);
        }
        else
        {
            mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
        }

        mParent.SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
        mParent.SetMleFrameCounter(mleFrameCounter.GetFrameCounter());

        mParent.SetState(Neighbor::kStateValid);
        SetStateChild(GetRloc16());

    // fall through

    case kDeviceStateChild:
        // Source Address
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
        VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

        if (GetRouterId(sourceAddress.GetRloc16()) != GetRouterId(GetRloc16()))
        {
            BecomeDetached();
            ExitNow();
        }

        // Leader Data, Network Data, Active Timestamp, Pending Timestamp
        SuccessOrExit(error = HandleLeaderData(aMessage, aMessageInfo));

        // Timeout optional
        if (Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout) == OT_ERROR_NONE)
        {
            VerifyOrExit(timeout.IsValid(), error = OT_ERROR_PARSE);
            mTimeout = timeout.GetTimeout();
        }

        if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            mNetif.GetMeshForwarder().GetDataPollManager().SetAttachMode(false);
            mNetif.GetMeshForwarder().SetRxOnWhenIdle(false);
            mParentRequestTimer.Stop();
        }
        else
        {
            mParentRequestTimer.Start(Timer::SecToMsec(mTimeout) - kUnicastRetransmissionDelay * kMaxChildKeepAliveAttempts);
            mNetif.GetMeshForwarder().SetRxOnWhenIdle(true);
        }

        mChildUpdateAttempts = 0;

        break;

    default:
        assert(false);
        break;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Child Update Response");
    }

    return error;
}

otError Mle::HandleAnnounce(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    ChannelTlv channel;
    ActiveTimestampTlv timestamp;
    const MeshCoP::Timestamp *localTimestamp;
    PanIdTlv panid;

    otLogInfoMle(GetInstance(), "Received announce");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChannel, sizeof(channel), channel));
    VerifyOrExit(channel.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(timestamp), timestamp));
    VerifyOrExit(timestamp.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kPanId, sizeof(panid), panid));
    VerifyOrExit(panid.IsValid(), error = OT_ERROR_PARSE);

    localTimestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp();

    if (localTimestamp == NULL || localTimestamp->Compare(timestamp) > 0)
    {
        Stop(false);
        mPreviousChannel = mNetif.GetMac().GetChannel();
        mPreviousPanId = mNetif.GetMac().GetPanId();
        mNetif.GetMac().SetChannel(static_cast<uint8_t>(channel.GetChannel()));
        mNetif.GetMac().SetPanId(panid.GetPanId());
        Start(false, true);
    }
    else
    {
        SendAnnounce(static_cast<uint8_t>(channel.GetChannel()), false);
    }

exit:
    (void)aMessageInfo;
    return error;
}

otError Mle::HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    const ThreadMessageInfo *threadMessageInfo = static_cast<const ThreadMessageInfo *>(aMessageInfo.GetLinkInfo());
    Tlv tlv;
    MeshCoP::Tlv meshcopTlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::ExtendedPanIdTlv extPanId;
    MeshCoP::NetworkNameTlv networkName;
    MeshCoP::SteeringDataTlv steeringData;
    MeshCoP::JoinerUdpPortTlv JoinerUdpPort;
    otActiveScanResult result;
    uint16_t offset;
    uint16_t end;

    otLogInfoMle(GetInstance(), "Handle discovery response");

    VerifyOrExit(mIsDiscoverInProgress, error = OT_ERROR_DROP);

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

    VerifyOrExit(offset < end, error = OT_ERROR_PARSE);

    offset += sizeof(tlv);
    end = offset + tlv.GetLength();

    memset(&result, 0, sizeof(result));
    result.mPanId = threadMessageInfo->mPanId;
    result.mChannel = threadMessageInfo->mChannel;
    result.mRssi = threadMessageInfo->mRss;
    result.mLqi = threadMessageInfo->mLqi;
    static_cast<Mac::ExtAddress *>(&result.mExtAddress)->Set(aMessageInfo.GetPeerAddr());

    // process MeshCoP TLVs
    while (offset < end)
    {
        aMessage.Read(offset, sizeof(meshcopTlv), &meshcopTlv);

        switch (meshcopTlv.GetType())
        {
        case MeshCoP::Tlv::kDiscoveryResponse:
            aMessage.Read(offset, sizeof(discoveryResponse), &discoveryResponse);
            VerifyOrExit(discoveryResponse.IsValid(), error = OT_ERROR_PARSE);
            result.mVersion = discoveryResponse.GetVersion();
            result.mIsNative = discoveryResponse.IsNativeCommissioner();
            break;

        case MeshCoP::Tlv::kExtendedPanId:
            aMessage.Read(offset, sizeof(extPanId), &extPanId);
            VerifyOrExit(extPanId.IsValid(), error = OT_ERROR_PARSE);
            memcpy(&result.mExtendedPanId, extPanId.GetExtendedPanId(), sizeof(result.mExtendedPanId));
            break;

        case MeshCoP::Tlv::kNetworkName:
            aMessage.Read(offset, sizeof(networkName), &networkName);
            VerifyOrExit(networkName.IsValid(), error = OT_ERROR_PARSE);
            memcpy(&result.mNetworkName, networkName.GetNetworkName(), networkName.GetLength());
            break;

        case MeshCoP::Tlv::kSteeringData:
            aMessage.Read(offset, sizeof(steeringData), &steeringData);
            VerifyOrExit(steeringData.IsValid(), error = OT_ERROR_PARSE);

            // Pass up MLE discovery responses only if the steering data is set to all 0xFFs,
            // or if it matches the factory set EUI64.
            if (mEnableEui64Filtering)
            {
                otExtAddress mfgEUI64;
                Crc16 ccitt(Crc16::kCcitt);
                Crc16 ansi(Crc16::kAnsi);

                // Get Factory set EUI64
                otPlatRadioGetIeeeEui64(GetInstance(), mfgEUI64.m8);

                // Compute bloom filter
                for (size_t i = 0; i < sizeof(mfgEUI64.m8); i++)
                {
                    ccitt.Update(mfgEUI64.m8[i]);
                    ansi.Update(mfgEUI64.m8[i]);
                }

                // Drop responses that don't match the bloom filter
                if (!steeringData.GetBit(ccitt.Get() % steeringData.GetNumBits()) ||
                    !steeringData.GetBit(ansi.Get() % steeringData.GetNumBits()))
                {
                    ExitNow(error = OT_ERROR_NONE);
                }
            }

            result.mSteeringData.mLength = steeringData.GetLength();
            memcpy(result.mSteeringData.m8, steeringData.GetValue(), result.mSteeringData.mLength);

            break;

        case MeshCoP::Tlv::kJoinerUdpPort:
            aMessage.Read(offset, sizeof(JoinerUdpPort), &JoinerUdpPort);
            VerifyOrExit(JoinerUdpPort.IsValid(), error = OT_ERROR_PARSE);
            result.mJoinerUdpPort = JoinerUdpPort.GetUdpPort();
            break;

        default:
            break;
        }

        offset += sizeof(meshcopTlv) + meshcopTlv.GetLength();
    }

    // signal callback
    mDiscoverHandler(&result, mDiscoverContext);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Discovery Response");
    }

    return error;
}

Neighbor *Mle::GetNeighbor(uint16_t aAddress)
{
    if ((mParent.IsStateValidOrRestoring()) && (mParent.GetRloc16() == aAddress))
    {
        return &mParent;
    }

    if ((mParentCandidate.GetState() == Neighbor::kStateValid) && (mParentCandidate.GetRloc16() == aAddress))
    {
        return &mParentCandidate;
    }

    return NULL;
}

Neighbor *Mle::GetNeighbor(const Mac::ExtAddress &aAddress)
{
    if ((mParent.IsStateValidOrRestoring()) &&
        (memcmp(&mParent.GetExtAddress(), &aAddress, sizeof(aAddress)) == 0))
    {
        return &mParent;
    }

    if ((mParentCandidate.GetState() == Neighbor::kStateValid) &&
        (memcmp(&mParentCandidate.GetExtAddress(), &aAddress, sizeof(aAddress)) == 0))
    {
        return &mParentCandidate;
    }

    return NULL;
}

Neighbor *Mle::GetNeighbor(const Mac::Address &aAddress)
{
    Neighbor *neighbor = NULL;

    switch (aAddress.mLength)
    {
    case 2:
        neighbor = GetNeighbor(aAddress.mShortAddress);
        break;

    case 8:
        neighbor = GetNeighbor(aAddress.mExtAddress);
        break;
    }

    return neighbor;
}

uint16_t Mle::GetNextHop(uint16_t aDestination) const
{
    (void)aDestination;
    return (mParent.GetState() == Neighbor::kStateValid) ? mParent.GetRloc16() : static_cast<uint16_t>
           (Mac::kShortAddrInvalid);
}

bool Mle::IsRoutingLocator(const Ip6::Address &aAddress) const
{
    return memcmp(&mMeshLocal16, &aAddress, kRlocPrefixLength) == 0 &&
           aAddress.mFields.m8[14] < Ip6::Address::kAloc16Mask &&
           (aAddress.mFields.m8[14] & Ip6::Address::kRloc16ReservedBitMask) == 0;
}

bool Mle::IsAnycastLocator(const Ip6::Address &aAddress) const
{
    return memcmp(&mMeshLocal16, &aAddress, kRlocPrefixLength) == 0 &&
           aAddress.mFields.m8[14] == Ip6::Address::kAloc16Mask;
}

bool Mle::IsMeshLocalAddress(const Ip6::Address &aAddress) const
{
    return aAddress.PrefixMatch(GetMeshLocal16()) >= Ip6::Address::kMeshLocalPrefixLength;
}

Router *Mle::GetParent(void)
{
    if ((!mParent.IsStateValidOrRestoring()) && (mParentCandidate.GetState() == Neighbor::kStateValid))
    {
        return &mParentCandidate;
    }

    return &mParent;
}

otError Mle::CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    otError error = OT_ERROR_DROP;
    Ip6::MessageInfo messageInfo;

    if (aMeshDest != GetRloc16())
    {
        ExitNow(error = OT_ERROR_NONE);
    }

    if (mNetif.IsUnicastAddress(aIp6Header.GetDestination()))
    {
        ExitNow(error = OT_ERROR_NONE);
    }

    messageInfo.GetPeerAddr() = GetMeshLocal16();
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(aMeshSource);
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    mNetif.GetIp6().mIcmp.SendError(kIcmp6TypeDstUnreach,
                                    kIcmp6CodeDstUnreachNoRoute,
                                    messageInfo, aIp6Header);

exit:
    return error;
}

}  // namespace Mle
}  // namespace ot
