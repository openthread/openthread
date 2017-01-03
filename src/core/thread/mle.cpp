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

#include <thread/mle.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/encoding.hpp>
#include <common/settings.hpp>
#include <crypto/aes_ccm.hpp>
#include <mac/mac_frame.hpp>
#include <meshcop/tlvs.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <platform/radio.h>
#include <platform/random.h>
#include <platform/settings.h>
#include <thread/address_resolver.hpp>
#include <thread/key_manager.hpp>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Mle {

Mle::Mle(ThreadNetif &aThreadNetif) :
    mNetif(aThreadNetif),
    mAddressResolver(aThreadNetif.GetAddressResolver()),
    mKeyManager(aThreadNetif.GetKeyManager()),
    mMac(aThreadNetif.GetMac()),
    mMesh(aThreadNetif.GetMeshForwarder()),
    mMleRouter(aThreadNetif.GetMle()),
    mNetworkData(aThreadNetif.GetNetworkDataLeader()),
    mJoinerRouter(aThreadNetif.GetJoinerRouter()),
    mRetrieveNewNetworkData(false),
    mDeviceState(kDeviceStateDisabled),
    mDeviceMode(ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeSecureDataRequest | ModeTlv::kModeFFD |
                ModeTlv::kModeFullNetworkData),
    isAssignLinkQuality(false),
    mAssignLinkQuality(0),
    mAssignLinkMargin(0),

    mParentRequestState(kParentIdle),
    mReattachState(kReattachStop),
    mParentRequestTimer(aThreadNetif.GetIp6().mTimerScheduler, &Mle::HandleParentRequestTimer, this),
    mRouterSelectionJitter(kRouterSelectionJitter),
    mRouterSelectionJitterTimeout(0),
    mParentRequestMode(kMleAttachAnyPartition),
    mParentLinkQuality(0),
    mParentPriority(0),
    mParentLinkQuality3(0),
    mParentLinkQuality2(0),
    mParentLinkQuality1(0),
    mParentIsSingleton(false),
    mSocket(aThreadNetif.GetIp6().mUdp),
    mTimeout(kMleEndDeviceTimeout),
    mSendChildUpdateRequest(aThreadNetif.GetIp6().mTaskletScheduler, &Mle::HandleSendChildUpdateRequest, this),
    mDiscoverHandler(NULL),
    mDiscoverContext(NULL),
    mIsDiscoverInProgress(false),
    mAnnounceChannel(kPhyMinChannel),
    mPreviousChannel(0),
    mPreviousPanId(Mac::kPanIdBroadcast)
{
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

    // link-local 64
    mLinkLocal64.GetAddress().mFields.m16[0] = HostSwap16(0xfe80);
    mLinkLocal64.GetAddress().SetIid(*mMac.GetExtAddress());
    mLinkLocal64.mPrefixLength = 64;
    mLinkLocal64.mPreferredLifetime = 0xffffffff;
    mLinkLocal64.mValidLifetime = 0xffffffff;
    mNetif.AddUnicastAddress(mLinkLocal64);

    // Leader Aloc
    mLeaderAloc.mPrefixLength = 128;
    mLeaderAloc.mPreferredLifetime = 0xffffffff;
    mLeaderAloc.mValidLifetime = 0xffffffff;
    mLeaderAloc.mScopeOverride = Ip6::Address::kRealmLocalScope;
    mLeaderAloc.mScopeOverrideValid = true;

    // initialize Mesh Local Prefix
    mMeshLocal64.GetAddress().mFields.m8[0] = 0xfd;
    memcpy(mMeshLocal64.GetAddress().mFields.m8 + 1, mMac.GetExtendedPanId(), 5);
    mMeshLocal64.GetAddress().mFields.m8[6] = 0x00;
    mMeshLocal64.GetAddress().mFields.m8[7] = 0x00;
    mMeshLocal64.mScopeOverride = Ip6::Address::kRealmLocalScope;
    mMeshLocal64.mScopeOverrideValid = true;

    // mesh-local 64
    for (int i = 8; i < 16; i++)
    {
        mMeshLocal64.GetAddress().mFields.m8[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    mMeshLocal64.mPrefixLength = 64;
    mMeshLocal64.mPreferredLifetime = 0xffffffff;
    mMeshLocal64.mValidLifetime = 0xffffffff;
    SetMeshLocalPrefix(mMeshLocal64.GetAddress().mFields.m8); // Also calls AddUnicastAddress

    // mesh-local 16
    mMeshLocal16.GetAddress().mFields.m16[4] = HostSwap16(0x0000);
    mMeshLocal16.GetAddress().mFields.m16[5] = HostSwap16(0x00ff);
    mMeshLocal16.GetAddress().mFields.m16[6] = HostSwap16(0xfe00);
    mMeshLocal16.mPrefixLength = 64;
    mMeshLocal16.mPreferredLifetime = 0xffffffff;
    mMeshLocal16.mValidLifetime = 0xffffffff;
    mMeshLocal16.mScopeOverride = Ip6::Address::kRealmLocalScope;
    mMeshLocal16.mScopeOverrideValid = true;

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

ThreadError Mle::Enable(void)
{
    ThreadError error = kThreadError_None;
    Ip6::SockAddr sockaddr;

    // memcpy(&sockaddr.mAddr, &mLinkLocal64.GetAddress(), sizeof(sockaddr.mAddr));
    sockaddr.mPort = kUdpPort;
    SuccessOrExit(error = mSocket.Open(&Mle::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(sockaddr));

exit:
    return error;
}

ThreadError Mle::Disable(void)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = Stop(false));
    SuccessOrExit(error = mSocket.Close());

exit:
    return error;
}

ThreadError Mle::Start(bool aEnableReattach)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    // cannot bring up the interface if IEEE 802.15.4 promiscuous mode is enabled
    VerifyOrExit(otPlatRadioGetPromiscuous(mNetif.GetInstance()) == false, error = kThreadError_InvalidState);
    VerifyOrExit(mNetif.IsUp(), error = kThreadError_InvalidState);

    mDeviceState = kDeviceStateDetached;
    mNetif.SetStateChangedFlags(OT_NET_ROLE);
    SetStateDetached();

    mKeyManager.Start();

    if (aEnableReattach)
    {
        mReattachState = kReattachStart;
    }

    if (GetRloc16() == Mac::kShortAddrInvalid)
    {
        BecomeChild(kMleAttachAnyPartition);
    }
    else if (IsActiveRouter(GetRloc16()))
    {
        mMleRouter.BecomeRouter(ThreadStatusTlv::kTooFewRouters);
    }
    else
    {
        SendChildUpdateRequest();
        mParentRequestState = kParentSynchronize;
        mParentRequestTimer.Start(kParentRequestRouterTimeout);
    }

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError Mle::Stop(bool aClearNetworkDatasets)
{
    otLogFuncEntry();
    mKeyManager.Stop();
    SetStateDetached();
    mNetif.RemoveUnicastAddress(mMeshLocal16);

    if (aClearNetworkDatasets)
    {
        mNetif.GetActiveDataset().Clear(true);
        mNetif.GetPendingDataset().Clear(true);
    }

    mDeviceState = kDeviceStateDisabled;
    otLogFuncExit();
    return kThreadError_None;
}

ThreadError Mle::Restore(void)
{
    ThreadError error = kThreadError_None;
    NetworkInfo networkInfo;
    uint16_t length;

    mNetif.GetActiveDataset().Restore();
    mNetif.GetPendingDataset().Restore();

    SuccessOrExit(error = otPlatSettingsGet(mNetif.GetInstance(), kKeyNetworkInfo, 0,
                                            reinterpret_cast<uint8_t *>(&networkInfo), &length));

    VerifyOrExit(length == sizeof(networkInfo), error = kThreadError_NotFound);
    VerifyOrExit(networkInfo.mDeviceState >= kDeviceStateChild, error = kThreadError_NotFound);

    mDeviceMode = networkInfo.mDeviceMode;
    SetRloc16(networkInfo.mRloc16);
    mKeyManager.SetCurrentKeySequence(networkInfo.mKeySequence);
    mKeyManager.SetMleFrameCounter(networkInfo.mMleFrameCounter);
    mKeyManager.SetMacFrameCounter(networkInfo.mMacFrameCounter);
    mMac.SetExtAddress(networkInfo.mExtAddress);
    UpdateLinkLocalAddress();

    if (networkInfo.mDeviceState == kDeviceStateChild)
    {
        SuccessOrExit(error = otPlatSettingsGet(mNetif.GetInstance(), kKeyParentInfo, 0,
                                                reinterpret_cast<uint8_t *>(&mParent), &length));
    }
    else if (networkInfo.mDeviceState == kDeviceStateRouter || networkInfo.mDeviceState == kDeviceStateLeader)
    {
        mMleRouter.SetRouterId(GetRouterId(GetRloc16()));
        mMleRouter.SetPreviousPartitionId(networkInfo.mPreviousPartitionId);
        mMleRouter.RestoreChildren();
    }

exit:
    return error;
}

ThreadError Mle::Store(void)
{
    ThreadError error = kThreadError_None;
    NetworkInfo networkInfo;

    VerifyOrExit(IsAttached(), error = kThreadError_InvalidState);

    if (mNetif.GetActiveDataset().GetLocal().GetTimestamp() == NULL)
    {
        mNetif.GetActiveDataset().GenerateLocal();
        mNetif.GetActiveDataset().GetLocal().Store();
    }

    memset(&networkInfo, 0, sizeof(networkInfo));

    networkInfo.mDeviceMode = mDeviceMode;
    networkInfo.mDeviceState = mDeviceState;
    networkInfo.mRloc16 = GetRloc16();
    networkInfo.mKeySequence = mKeyManager.GetCurrentKeySequence();
    networkInfo.mMleFrameCounter = mKeyManager.GetMleFrameCounter() + OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    networkInfo.mMacFrameCounter = mKeyManager.GetMacFrameCounter() + OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    networkInfo.mPreviousPartitionId = mLeaderData.GetPartitionId();
    memcpy(networkInfo.mExtAddress.m8, mMac.GetExtAddress(), sizeof(networkInfo.mExtAddress));

    if (mDeviceState == kDeviceStateChild)
    {
        SuccessOrExit(error = otPlatSettingsSet(mNetif.GetInstance(), kKeyParentInfo,
                                                reinterpret_cast<uint8_t *>(&mParent), sizeof(mParent)));
    }

    SuccessOrExit(error = otPlatSettingsSet(mNetif.GetInstance(), kKeyNetworkInfo,
                                            reinterpret_cast<uint8_t *>(&networkInfo), sizeof(networkInfo)));

    mKeyManager.SetStoredMleFrameCounter(networkInfo.mMleFrameCounter);
    mKeyManager.SetStoredMacFrameCounter(networkInfo.mMacFrameCounter);

    otLogDebgMle("Store Network Information");

exit:
    return error;
}

ThreadError Mle::Discover(uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanId,
                          DiscoverHandler aCallback, void *aContext)
{
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    Ip6::Address destination;
    Tlv tlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    uint16_t startOffset;

    VerifyOrExit(!mIsDiscoverInProgress, error = kThreadError_Busy);

    mDiscoverHandler = aCallback;
    mDiscoverContext = aContext;
    mMesh.SetDiscoverParameters(aScanChannels, aScanDuration);

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
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
    SuccessOrExit(error = message->Append(&discoveryRequest, sizeof(discoveryRequest)));

    tlv.SetLength(static_cast<uint8_t>(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0002);
    SuccessOrExit(error = SendMessage(*message, destination));

    mIsDiscoverInProgress = true;

    otLogInfoMle("Sent discovery request");

exit:

    if (error != kThreadError_None && message != NULL)
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
    mDiscoverHandler(NULL, mDiscoverContext);
}

ThreadError Mle::BecomeDetached(void)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(mDeviceState != kDeviceStateDisabled, error = kThreadError_InvalidState);

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

ThreadError Mle::BecomeChild(otMleAttachFilter aFilter)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    VerifyOrExit(mDeviceState != kDeviceStateDisabled, error = kThreadError_InvalidState);
    VerifyOrExit(mParentRequestState == kParentIdle, error = kThreadError_Busy);

    if (mReattachState == kReattachStart)
    {
        if (mNetif.GetActiveDataset().Restore() == kThreadError_None)
        {
            mReattachState = kReattachActive;
        }
        else
        {
            mReattachState = kReattachStop;
        }
    }

    mParentRequestState = kParentRequestStart;
    mParentRequestMode = aFilter;
    memset(&mParent, 0, sizeof(mParent));

    if (aFilter == kMleAttachAnyPartition)
    {
        mParent.mState = Neighbor::kStateInvalid;
    }

    mParentRequestTimer.Start(kParentRequestRouterTimeout);

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

DeviceState Mle::GetDeviceState(void) const
{
    return mDeviceState;
}

ThreadError Mle::SetStateDetached(void)
{
    if (mDeviceState != kDeviceStateDetached)
    {
        mNetif.SetStateChangedFlags(OT_NET_ROLE);
    }

    if (mDeviceState == kDeviceStateLeader)
    {
        mNetif.RemoveUnicastAddress(mLeaderAloc);
    }

    mAddressResolver.Clear();
    mDeviceState = kDeviceStateDetached;
    mParentRequestState = kParentIdle;
    mParentRequestTimer.Stop();
    mMesh.SetRxOnWhenIdle(true);
    mMleRouter.HandleDetachStart();
    mNetif.GetIp6().SetForwardingEnabled(false);
    mNetif.GetIp6().mMpl.SetTimerExpirations(0);

    otLogInfoMle("Mode -> Detached");
    return kThreadError_None;
}

ThreadError Mle::SetStateChild(uint16_t aRloc16)
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

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0)
    {
        mParentRequestTimer.Start(Timer::SecToMsec(mTimeout / kMaxChildKeepAliveAttempts));
    }

    if ((mDeviceMode & ModeTlv::kModeFFD) != 0)
    {
        mMleRouter.HandleChildStart(mParentRequestMode);
    }

    mNetif.GetNetworkDataLocal().ClearResubmitDelayTimer();
    mNetif.GetIp6().SetForwardingEnabled(false);
    mNetif.GetIp6().mMpl.SetTimerExpirations(kMplChildDataMessageTimerExpirations);

    if (mPreviousPanId != Mac::kPanIdBroadcast && (mDeviceMode & ModeTlv::kModeFFD))
    {
        mPreviousPanId = Mac::kPanIdBroadcast;
        mNetif.GetAnnounceBeginServer().SendAnnounce(1 << mPreviousChannel);
    }

    otLogInfoMle("Mode -> Child");
    return kThreadError_None;
}

uint32_t Mle::GetTimeout(void) const
{
    return mTimeout;
}

ThreadError Mle::SetTimeout(uint32_t aTimeout)
{
    if (aTimeout < 4)
    {
        aTimeout = 4;
    }

    mTimeout = aTimeout;

    if (mDeviceState == kDeviceStateChild)
    {
        SendChildUpdateRequest();

        if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0)
        {
            mParentRequestTimer.Start(Timer::SecToMsec(mTimeout / kMaxChildKeepAliveAttempts));
        }
    }

    return kThreadError_None;
}

uint8_t Mle::GetDeviceMode(void) const
{
    return mDeviceMode;
}

ThreadError Mle::SetDeviceMode(uint8_t aDeviceMode)
{
    ThreadError error = kThreadError_None;
    uint8_t oldMode = mDeviceMode;

    VerifyOrExit((aDeviceMode & ModeTlv::kModeFFD) == 0 || (aDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0,
                 error = kThreadError_InvalidArgs);

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

ThreadError Mle::UpdateLinkLocalAddress(void)
{
    mNetif.RemoveUnicastAddress(mLinkLocal64);
    mLinkLocal64.GetAddress().SetIid(*mMac.GetExtAddress());
    mNetif.AddUnicastAddress(mLinkLocal64);

    mNetif.SetStateChangedFlags(OT_IP6_LL_ADDR_CHANGED);

    return kThreadError_None;
}

const uint8_t *Mle::GetMeshLocalPrefix(void) const
{
    return mMeshLocal16.GetAddress().mFields.m8;
}

ThreadError Mle::SetMeshLocalPrefix(const uint8_t *aMeshLocalPrefix)
{
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

    return kThreadError_None;
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
    return mMac.GetShortAddress();
}

ThreadError Mle::SetRloc16(uint16_t aRloc16)
{
    mNetif.RemoveUnicastAddress(mMeshLocal16);

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        // mesh-local 16
        mMeshLocal16.GetAddress().mFields.m16[7] = HostSwap16(aRloc16);
        mNetif.AddUnicastAddress(mMeshLocal16);
    }

    mMac.SetShortAddress(aRloc16);
    mNetif.GetIp6().mMpl.SetSeedId(aRloc16);

    return kThreadError_None;
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

ThreadError Mle::GetLeaderAddress(Ip6::Address &aAddress) const
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = kThreadError_Detached);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 8);
    aAddress.mFields.m16[4] = HostSwap16(0x0000);
    aAddress.mFields.m16[5] = HostSwap16(0x00ff);
    aAddress.mFields.m16[6] = HostSwap16(0xfe00);
    aAddress.mFields.m16[7] = HostSwap16(GetRloc16(mLeaderData.GetLeaderRouterId()));

exit:
    return error;
}

ThreadError Mle::GetLeaderAloc(Ip6::Address &aAddress) const
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = kThreadError_Detached);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 8);
    aAddress.mFields.m16[4] = HostSwap16(0x0000);
    aAddress.mFields.m16[5] = HostSwap16(0x00ff);
    aAddress.mFields.m16[6] = HostSwap16(0xfe00);
    aAddress.mFields.m16[7] = HostSwap16(kAloc16Leader);

exit:
    return error;
}

ThreadError Mle::AddLeaderAloc(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mDeviceState == kDeviceStateLeader, error = kThreadError_InvalidState);

    SuccessOrExit(error = GetLeaderAloc(mLeaderAloc.GetAddress()));

    error = mNetif.AddUnicastAddress(mLeaderAloc);

exit:
    return error;
}

const LeaderDataTlv &Mle::GetLeaderDataTlv(void)
{
    mLeaderData.SetDataVersion(mNetworkData.GetVersion());
    mLeaderData.SetStableDataVersion(mNetworkData.GetStableVersion());
    return mLeaderData;
}

ThreadError Mle::GetLeaderData(otLeaderData &aLeaderData)
{
    const LeaderDataTlv &leaderData(GetLeaderDataTlv());
    ThreadError error = kThreadError_None;

    VerifyOrExit(mDeviceState != kDeviceStateDisabled && mDeviceState != kDeviceStateDetached,
                 error = kThreadError_Detached);

    aLeaderData.mPartitionId = leaderData.GetPartitionId();
    aLeaderData.mWeighting = leaderData.GetWeighting();
    aLeaderData.mDataVersion = leaderData.GetDataVersion();
    aLeaderData.mStableDataVersion = leaderData.GetStableDataVersion();
    aLeaderData.mLeaderRouterId = leaderData.GetLeaderRouterId();

exit:
    return error;
}

ThreadError Mle::GetAssignLinkQuality(const Mac::ExtAddress aMacAddr, uint8_t &aLinkQuality)
{
    ThreadError error;

    VerifyOrExit((memcmp(aMacAddr.m8, mAddr64.m8, OT_EXT_ADDRESS_SIZE)) == 0, error = kThreadError_InvalidArgs);

    aLinkQuality = mAssignLinkQuality;

    return kThreadError_None;

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

uint8_t Mle::GetRouterSelectionJitter(void) const
{
    return mRouterSelectionJitter;
}

void Mle::SetRouterSelectionJitter(uint8_t aRouterJitter)
{
    mRouterSelectionJitter = aRouterJitter;
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
    VerifyOrExit(message != NULL, ;);

    message->SetLinkSecurityEnabled(false);
    message->SetPriority(kMleMessagePriority);

exit:
    return message;
}

ThreadError Mle::AppendHeader(Message &aMessage, Header::Command aCommand)
{
    ThreadError error = kThreadError_None;
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

ThreadError Mle::AppendSourceAddress(Message &aMessage)
{
    SourceAddressTlv tlv;

    tlv.Init();
    tlv.SetRloc16(GetRloc16());

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendStatus(Message &aMessage, StatusTlv::Status aStatus)
{
    StatusTlv tlv;

    tlv.Init();
    tlv.SetStatus(aStatus);

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendMode(Message &aMessage, uint8_t aMode)
{
    ModeTlv tlv;

    tlv.Init();
    tlv.SetMode(aMode);

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendTimeout(Message &aMessage, uint32_t aTimeout)
{
    TimeoutTlv tlv;

    tlv.Init();
    tlv.SetTimeout(aTimeout);

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendChallenge(Message &aMessage, const uint8_t *aChallenge, uint8_t aChallengeLength)
{
    ThreadError error;
    Tlv tlv;

    tlv.SetType(Tlv::kChallenge);
    tlv.SetLength(aChallengeLength);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));
    SuccessOrExit(error = aMessage.Append(aChallenge, aChallengeLength));
exit:
    return error;
}

ThreadError Mle::AppendResponse(Message &aMessage, const uint8_t *aResponse, uint8_t aResponseLength)
{
    ThreadError error;
    Tlv tlv;

    tlv.SetType(Tlv::kResponse);
    tlv.SetLength(aResponseLength);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));
    SuccessOrExit(error = aMessage.Append(aResponse, aResponseLength));

exit:
    return error;
}

ThreadError Mle::AppendLinkFrameCounter(Message &aMessage)
{
    LinkFrameCounterTlv tlv;

    tlv.Init();
    tlv.SetFrameCounter(mKeyManager.GetMacFrameCounter());

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendMleFrameCounter(Message &aMessage)
{
    MleFrameCounterTlv tlv;

    tlv.Init();
    tlv.SetFrameCounter(mKeyManager.GetMleFrameCounter());

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendAddress16(Message &aMessage, uint16_t aRloc16)
{
    Address16Tlv tlv;

    tlv.Init();
    tlv.SetRloc16(aRloc16);

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendLeaderData(Message &aMessage)
{
    mLeaderData.Init();
    mLeaderData.SetDataVersion(mNetworkData.GetVersion());
    mLeaderData.SetStableDataVersion(mNetworkData.GetStableVersion());

    return aMessage.Append(&mLeaderData, sizeof(mLeaderData));
}

void Mle::FillNetworkDataTlv(NetworkDataTlv &aTlv, bool aStableOnly)
{
    uint8_t length;
    mNetworkData.GetNetworkData(aStableOnly, aTlv.GetNetworkData(), length);
    aTlv.SetLength(length);
}

ThreadError Mle::AppendNetworkData(Message &aMessage, bool aStableOnly)
{
    ThreadError error = kThreadError_None;
    NetworkDataTlv tlv;

    tlv.Init();
    FillNetworkDataTlv(tlv, aStableOnly);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(Tlv) + tlv.GetLength()));

exit:
    return error;
}

ThreadError Mle::AppendTlvRequest(Message &aMessage, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    ThreadError error;
    Tlv tlv;

    tlv.SetType(Tlv::kTlvRequest);
    tlv.SetLength(aTlvsLength);

    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));
    SuccessOrExit(error = aMessage.Append(aTlvs, aTlvsLength));

exit:
    return error;
}

ThreadError Mle::AppendScanMask(Message &aMessage, uint8_t aScanMask)
{
    ScanMaskTlv tlv;

    tlv.Init();
    tlv.SetMask(aScanMask);

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendLinkMargin(Message &aMessage, uint8_t aLinkMargin)
{
    LinkMarginTlv tlv;

    tlv.Init();
    tlv.SetLinkMargin(aLinkMargin);

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendVersion(Message &aMessage)
{
    VersionTlv tlv;

    tlv.Init();
    tlv.SetVersion(kVersion);

    return aMessage.Append(&tlv, sizeof(tlv));
}

ThreadError Mle::AppendAddressRegistration(Message &aMessage)
{
    ThreadError error;
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

        if (mNetworkData.GetContext(addr->GetAddress(), context) == kThreadError_None)
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

ThreadError Mle::AppendActiveTimestamp(Message &aMessage, bool aCouldUseLocal)
{
    ThreadError error;
    ActiveTimestampTlv timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    if ((timestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp()) == NULL && aCouldUseLocal)
    {
        timestamp = mNetif.GetActiveDataset().GetLocal().GetTimestamp();
    }

    VerifyOrExit(timestamp, error = kThreadError_None);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = *timestamp;
    error = aMessage.Append(&timestampTlv, sizeof(timestampTlv));

exit:
    return error;
}

ThreadError Mle::AppendPendingTimestamp(Message &aMessage, bool aCouldUseLocal)
{
    ThreadError error;
    PendingTimestampTlv timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    if ((timestamp = mNetif.GetPendingDataset().GetNetwork().GetTimestamp()) == NULL && aCouldUseLocal)
    {
        timestamp = mNetif.GetPendingDataset().GetLocal().GetTimestamp();
    }

    VerifyOrExit(timestamp && timestamp->GetSeconds() != 0, error = kThreadError_None);

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
    VerifyOrExit(mDeviceState != kDeviceStateDisabled, ;);

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
            mMleRouter.HandleNetworkDataUpdateRouter();
        }
        else
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
    {}
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
        if (mParent.mState == Neighbor::kStateValid)
        {
            if (mDeviceMode & ModeTlv::kModeRxOnWhenIdle)
            {
                SendChildUpdateRequest();
                mParentRequestTimer.Start(Timer::SecToMsec(mTimeout / kMaxChildKeepAliveAttempts));
            }
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
        mParent.mState = Neighbor::kStateInvalid;
        SendParentRequest();
        mParentRequestTimer.Start(kParentRequestRouterTimeout);
        break;

    case kParentRequestRouter:
        mParentRequestState = kParentRequestChild;

        if (mParent.mState == Neighbor::kStateValid)
        {
            SendChildIdRequest();
            mParentRequestState = kChildIdRequest;
        }
        else
        {
            SendParentRequest();
        }

        mParentRequestTimer.Start(kParentRequestChildTimeout);
        break;

    case kParentRequestChild:
        mParentRequestState = kParentRequestChild;

        if (mParent.mState == Neighbor::kStateValid)
        {
            SendChildIdRequest();
            mParentRequestState = kChildIdRequest;
            mParentRequestTimer.Start(kParentRequestChildTimeout);
        }
        else
        {
            if (mReattachState == kReattachActive)
            {
                if (mNetif.GetPendingDataset().Restore() == kThreadError_None)
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
                        mMac.SetChannel(mPreviousChannel);
                        mMac.SetPanId(mPreviousPanId);
                        mPreviousPanId = Mac::kPanIdBroadcast;
                        BecomeDetached();
                    }
                    else if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
                    {
                        SendOrphanAnnounce();
                        BecomeDetached();
                    }
                    else if (mMleRouter.BecomeLeader() != kThreadError_None)
                    {
                        mParentRequestState = kParentIdle;
                        BecomeDetached();
                    }

                    break;

                case kMleAttachSamePartition:
                    mParentRequestState = kParentIdle;
                    BecomeChild(kMleAttachAnyPartition);
                    break;

                case kMleAttachBetterPartition:
                    mParentRequestState = kParentIdle;
                    break;
                }
            }
        }

        break;

    case kChildIdRequest:
        mParentRequestState = kParentIdle;

        if (mDeviceState != kDeviceStateRouter && mDeviceState != kDeviceStateLeader)
        {
            BecomeDetached();
        }

        break;
    }
}

ThreadError Mle::SendParentRequest(void)
{
    ThreadError error = kThreadError_None;
    Message *message;
    uint8_t scanMask = 0;
    Ip6::Address destination;

    for (uint8_t i = 0; i < sizeof(mParentRequest.mChallenge); i++)
    {
        mParentRequest.mChallenge[i] = static_cast<uint8_t>(otPlatRandomGet());
    }

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandParentRequest));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));
    SuccessOrExit(error = AppendChallenge(*message, mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge)));

    switch (mParentRequestState)
    {
    case kParentRequestRouter:
        scanMask = ScanMaskTlv::kRouterFlag;

        if (mParentRequestMode == kMleAttachSamePartition)
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

    SuccessOrExit(error = AppendScanMask(*message, scanMask));
    SuccessOrExit(error = AppendVersion(*message));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0002);
    SuccessOrExit(error = SendMessage(*message, destination));

    switch (mParentRequestState)
    {
    case kParentRequestRouter:
        otLogInfoMle("Sent parent request to routers");
        break;

    case kParentRequestChild:
        otLogInfoMle("Sent parent request to all devices");
        break;

    default:
        assert(false);
        break;
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return kThreadError_None;
}

ThreadError Mle::SendChildIdRequest(void)
{
    ThreadError error = kThreadError_None;
    uint8_t tlvs[] = {Tlv::kAddress16, Tlv::kNetworkData, Tlv::kRoute};
    Message *message;
    Ip6::Address destination;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
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
    // SuccessOrExit(error = AppendPendingTimestamp(*message, false));
    // we should not include the Local Pending Timestamp TLV in Child ID Request, this is a workaround for
    // Certification test 9.2.15 and 9.2.16 (https://github.com/openthread/openthread/issues/918)
    SuccessOrExit(error = AppendPendingTimestamp(*message, true));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParent.mMacAddr);
    SuccessOrExit(error = SendMessage(*message, destination));
    otLogInfoMle("Sent Child ID Request");

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mMesh.SetPollPeriod(kAttachDataPollPeriod);
        mMesh.SetRxOnWhenIdle(false);
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError Mle::SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    ThreadError error = kThreadError_None;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDataRequest));
    SuccessOrExit(error = AppendTlvRequest(*message, aTlvs, aTlvsLength));
    SuccessOrExit(error = AppendActiveTimestamp(*message, false));
    SuccessOrExit(error = AppendPendingTimestamp(*message, false));

    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle("Sent Data Request");

exit:

    if (error != kThreadError_None && message != NULL)
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

ThreadError Mle::SendChildUpdateRequest(void)
{
    ThreadError error = kThreadError_None;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
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
    destination.SetIid(mParent.mMacAddr);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("Sent Child Update Request to parent");

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mMesh.SetPollPeriod(kAttachDataPollPeriod);
        mMesh.SetRxOnWhenIdle(false);
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError Mle::SendChildUpdateResponse(const uint8_t *aTlvs, uint8_t aNumTlvs, const ChallengeTlv &aChallenge)
{
    ThreadError error = kThreadError_None;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
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
    destination.SetIid(mParent.mMacAddr);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("Sent Child Update Response to parent");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError Mle::SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce)
{
    ThreadError error = kThreadError_None;
    ChannelTlv channel;
    PanIdTlv panid;
    ActiveTimestampTlv activeTimestamp;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, ;);
    message->SetLinkSecurityEnabled(true);
    message->SetSubType(Message::kSubTypeMleAnnounce);
    message->SetChannel(aChannel);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandAnnounce));

    channel.Init();
    channel.SetChannelPage(0);
    channel.SetChannel(mMac.GetChannel());
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
    panid.SetPanId(mMac.GetPanId());
    SuccessOrExit(error = message->Append(&panid, sizeof(panid)));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("sent announce on channel %d", aChannel);

exit:

    if (error != kThreadError_None && message != NULL)
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

    VerifyOrExit(channelMask != NULL,);

    // find next channel in the Active Operational Dataset Channel Mask
    channel = mAnnounceChannel;

    while (!channelMask->IsChannelSet(channel))
    {
        channel++;

        if (channel > kPhyMaxChannel)
        {
            channel = kPhyMinChannel;
        }

        VerifyOrExit(channel != mAnnounceChannel,);
    }

    // Send Announce message
    SendAnnounce(channel, true);

    // Move to next channel
    mAnnounceChannel++;

    if (mAnnounceChannel > kPhyMaxChannel)
    {
        mAnnounceChannel = kPhyMinChannel;
    }

exit:
    return;
}

ThreadError Mle::SendMessage(Message &aMessage, const Ip6::Address &aDestination)
{
    ThreadError error = kThreadError_None;
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
        header.SetFrameCounter(mKeyManager.GetMleFrameCounter());

        keySequence = mKeyManager.GetCurrentKeySequence();
        header.SetKeyId(keySequence);

        aMessage.Write(0, header.GetLength(), &header);

        GenerateNonce(*mMac.GetExtAddress(), mKeyManager.GetMleFrameCounter(), Mac::Frame::kSecEncMic32, nonce);

        aesCcm.SetKey(mKeyManager.GetCurrentMleKey(), 16);
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

        mKeyManager.IncrementMleFrameCounter();
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

void Mle::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
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
    VerifyOrExit(header.IsValid(),);

    assert(aMessageInfo.GetLinkInfo() != NULL);

    if (header.GetSecuritySuite() == Header::kNoSecurity)
    {
        aMessage.MoveOffset(header.GetLength());

        switch (header.GetCommand())
        {
        case Header::kCommandDiscoveryRequest:
            mMleRouter.HandleDiscoveryRequest(aMessage, aMessageInfo);
            break;

        case Header::kCommandDiscoveryResponse:
            HandleDiscoveryResponse(aMessage, aMessageInfo);
            break;

        default:
            break;
        }

        ExitNow();
    }

    VerifyOrExit(mDeviceState != kDeviceStateDisabled && header.GetSecuritySuite() == Header::k154Security, ;);

    keySequence = header.GetKeyId();

    if (keySequence == mKeyManager.GetCurrentKeySequence())
    {
        mleKey = mKeyManager.GetCurrentMleKey();
    }
    else
    {
        mleKey = mKeyManager.GetTemporaryMleKey(keySequence);
    }

    aMessage.MoveOffset(header.GetLength() - 1);

    frameCounter = header.GetFrameCounter();

    messageTagLength = aMessage.Read(aMessage.GetLength() - sizeof(messageTag), sizeof(messageTag), messageTag);
    VerifyOrExit(messageTagLength == sizeof(messageTag), ;);
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
    VerifyOrExit(messageTagLength == tagLength && memcmp(messageTag, tag, tagLength) == 0, ;);

    if (keySequence > mKeyManager.GetCurrentKeySequence())
    {
        mKeyManager.SetCurrentKeySequence(keySequence);
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
            neighbor = mMleRouter.GetNeighbor(macAddr);
        }

        break;

    default:
        neighbor = NULL;
        break;
    }

    if (neighbor != NULL && neighbor->mState == Neighbor::kStateValid)
    {
        if (keySequence == neighbor->mKeySequence)
        {
            if (frameCounter < neighbor->mValid.mMleFrameCounter)
            {
                otLogDebgMle("mle frame reject 1");
                ExitNow();
            }
        }
        else
        {
            if (keySequence <= neighbor->mKeySequence)
            {
                otLogDebgMle("mle frame reject 2");
                ExitNow();
            }

            neighbor->mKeySequence = keySequence;
            neighbor->mValid.mLinkFrameCounter = 0;
        }

        neighbor->mValid.mMleFrameCounter = frameCounter + 1;
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
            otLogDebgMle("mle sequence unknown! %d", command);
            ExitNow();
        }
    }

    switch (command)
    {
    case Header::kCommandLinkRequest:
        mMleRouter.HandleLinkRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandLinkAccept:
        mMleRouter.HandleLinkAccept(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandLinkAcceptAndRequest:
        mMleRouter.HandleLinkAcceptAndRequest(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandAdvertisement:
        HandleAdvertisement(aMessage, aMessageInfo);
        break;

    case Header::kCommandDataRequest:
        mMleRouter.HandleDataRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandDataResponse:
        HandleDataResponse(aMessage, aMessageInfo);
        break;

    case Header::kCommandParentRequest:
        mMleRouter.HandleParentRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandParentResponse:
        HandleParentResponse(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdRequest:
        mMleRouter.HandleChildIdRequest(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdResponse:
        HandleChildIdResponse(aMessage, aMessageInfo);
        break;

    case Header::kCommandChildUpdateRequest:
        if (mDeviceState == kDeviceStateLeader || mDeviceState == kDeviceStateRouter)
        {
            mMleRouter.HandleChildUpdateRequest(aMessage, aMessageInfo);
        }
        else
        {
            HandleChildUpdateRequest(aMessage, aMessageInfo);
        }

        break;

    case Header::kCommandChildUpdateResponse:
        if (mDeviceState == kDeviceStateLeader || mDeviceState == kDeviceStateRouter)
        {
            mMleRouter.HandleChildUpdateResponse(aMessage, aMessageInfo, keySequence);
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
    {}
}

ThreadError Mle::HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Mac::ExtAddress macAddr;
    bool isNeighbor;
    Neighbor *neighbor;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv leaderData;
    RouteTlv route;
    uint8_t tlvs[] = {Tlv::kNetworkData};

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

    otLogInfoMle("Received advertisement from %04x", sourceAddress.GetRloc16());

    if (mDeviceState != kDeviceStateDetached)
    {
        SuccessOrExit(error = mMleRouter.HandleAdvertisement(aMessage, aMessageInfo));
    }

    macAddr.Set(aMessageInfo.GetPeerAddr());

    isNeighbor = false;

    switch (mDeviceState)
    {
    case kDeviceStateDisabled:
    case kDeviceStateDetached:
        break;

    case kDeviceStateChild:
        if (memcmp(&mParent.mMacAddr, &macAddr, sizeof(mParent.mMacAddr)))
        {
            break;
        }

        if ((mParent.mValid.mRloc16 == sourceAddress.GetRloc16()) &&
            (leaderData.GetPartitionId() != mLeaderData.GetPartitionId() ||
             leaderData.GetLeaderRouterId() != GetLeaderId()))
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

            if ((mDeviceMode & ModeTlv::kModeFFD) &&
                (Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == kThreadError_None) &&
                route.IsValid())
            {
                // Overwrite Route Data
                mMleRouter.ProcessRouteTlv(route);
            }

            mRetrieveNewNetworkData = true;
        }

        isNeighbor = true;
        mParent.mLastHeard = mParentRequestTimer.GetNow();
        break;

    case kDeviceStateRouter:
    case kDeviceStateLeader:
        if ((neighbor = mMleRouter.GetNeighbor(macAddr)) != NULL &&
            neighbor->mState == Neighbor::kStateValid)
        {
            isNeighbor = true;
        }

        break;
    }

    if (isNeighbor)
    {
        if (mRetrieveNewNetworkData ||
            (static_cast<int8_t>(leaderData.GetDataVersion() - mNetworkData.GetVersion()) > 0))
        {
            SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs));
        }
    }

exit:

    if (error != kThreadError_None)
    {
        otLogWarnMleErr(error, "Failed to process Advertisement");
    }

    return error;
}

ThreadError Mle::HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    LeaderDataTlv leaderData;
    NetworkDataTlv networkData;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    uint16_t activeDatasetOffset = 0;
    uint16_t pendingDatasetOffset = 0;
    bool dataRequest = false;
    Tlv tlv;

    otLogInfoMle("Received Data Response");

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

    if ((leaderData.GetPartitionId() != mLeaderData.GetPartitionId()) ||
        (leaderData.GetLeaderRouterId() != GetLeaderId()))
    {
        if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());
        }
        else
        {
            ExitNow(error = kThreadError_Drop);
        }
    }
    else if (!mRetrieveNewNetworkData)
    {
        VerifyOrExit(static_cast<int8_t>(leaderData.GetDataVersion() - mNetworkData.GetVersion()) > 0, ;);
    }

    // Network Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData));
    VerifyOrExit(networkData.IsValid(), error = kThreadError_Parse);

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == kThreadError_None)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(activeTimestamp.IsValid(), error = kThreadError_Parse);
        timestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if ((timestamp == NULL || timestamp->Compare(activeTimestamp) != 0) &&
            (Tlv::GetOffset(aMessage, Tlv::kActiveDataset, activeDatasetOffset) != kThreadError_None))
        {
            ExitNow(dataRequest = true);
        }
    }
    else
    {
        activeTimestamp.SetLength(0);
    }

    // Pending Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == kThreadError_None)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(pendingTimestamp.IsValid(), error = kThreadError_Parse);
        timestamp = mNetif.GetPendingDataset().GetNetwork().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if ((timestamp == NULL || timestamp->Compare(pendingTimestamp) != 0) &&
            (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, pendingDatasetOffset) != kThreadError_None))
        {
            ExitNow(dataRequest = true);
        }
    }
    else
    {
        pendingTimestamp.SetLength(0);
    }

    // Network Data
    mNetworkData.SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                networkData.GetNetworkData(), networkData.GetLength());

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

    if (mPreviousPanId != Mac::kPanIdBroadcast && ((mDeviceMode & ModeTlv::kModeFFD) == 0))
    {
        mPreviousPanId = Mac::kPanIdBroadcast;
        mNetif.GetAnnounceBeginServer().SendAnnounce(1 << mPreviousChannel);
    }

    mRetrieveNewNetworkData = false;

exit:

    if (error != kThreadError_None)
    {
        otLogWarnMleErr(error, "Failed to process Data Response");
    }

    (void)aMessageInfo;

    if (dataRequest)
    {
        static const uint8_t tlvs[] = {Tlv::kNetworkData};

        SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs));
    }

    return error;
}

bool Mle::IsBetterParent(uint16_t aRloc16, uint8_t aLinkQuality, ConnectivityTlv &aConnectivityTlv) const
{
    bool rval = false;

    if (aLinkQuality != mParentLinkQuality)
    {
        ExitNow(rval = (aLinkQuality > mParentLinkQuality));
    }

    if (IsActiveRouter(aRloc16) != IsActiveRouter(mParent.mValid.mRloc16))
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

ThreadError Mle::HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                      uint32_t aKeySequence)
{
    ThreadError error = kThreadError_None;
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

    otLogInfoMle("Received Parent Response");

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid() &&
                 memcmp(response.GetResponse(), mParentRequest.mChallenge, response.GetLength()) == 0,
                 error = kThreadError_Parse);

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

    // Link Quality
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMarginTlv), linkMarginTlv));
    VerifyOrExit(linkMarginTlv.IsValid(), error = kThreadError_Parse);

    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(mMac.GetNoiseFloor(), threadMessageInfo->mRss);

    if (linkMargin > linkMarginTlv.GetLinkMargin())
    {
        linkMargin = linkMarginTlv.GetLinkMargin();
    }

    linkQuality = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin);

    VerifyOrExit(mParentRequestState != kParentRequestRouter || linkQuality == 3, ;);

    // Connectivity
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kConnectivity, sizeof(connectivity), connectivity));
    VerifyOrExit(connectivity.IsValid(), error = kThreadError_Parse);

    if ((mDeviceMode & ModeTlv::kModeFFD) && (mDeviceState != kDeviceStateDetached))
    {
        switch (mParentRequestMode)
        {
        case kMleAttachAnyPartition:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId() ||
                         static_cast<int8_t>(connectivity.GetIdSequence() - mMleRouter.GetRouterIdSequence()) > 0,);
            break;

        case kMleAttachSamePartition:
            VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), ;);
            diff = static_cast<int8_t>(connectivity.GetIdSequence() - mMleRouter.GetRouterIdSequence());
            VerifyOrExit(diff > 0 || (diff == 0 && mMleRouter.GetLeaderAge() < mMleRouter.GetNetworkIdTimeout()), ;);
            break;

        case kMleAttachBetterPartition:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId(), ;);
            VerifyOrExit(mMleRouter.ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                      mMleRouter.IsSingleton(), mLeaderData) > 0, ;);
            break;
        }
    }

    // if already have a candidate parent, only seek a better parent
    if (mParent.mState == Neighbor::kStateValid)
    {
        int compare = 0;

        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            compare = mMleRouter.ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                   mParentIsSingleton, mParentLeaderData);
        }

        // only consider partitions that are the same or better
        VerifyOrExit(compare >= 0, ;);

        // only consider better parents if the partitions are the same
        VerifyOrExit(compare != 0 || IsBetterParent(sourceAddress.GetRloc16(), linkQuality, connectivity), ;);
    }

    // Link Frame Counter
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter), linkFrameCounter));
    VerifyOrExit(linkFrameCounter.IsValid(), error = kThreadError_Parse);

    // Mle Frame Counter
    if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) == kThreadError_None)
    {
        VerifyOrExit(mleFrameCounter.IsValid(), ;);
    }
    else
    {
        mleFrameCounter.SetFrameCounter(linkFrameCounter.GetFrameCounter());
    }

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = kThreadError_Parse);
    memcpy(mChildIdRequest.mChallenge, challenge.GetChallenge(), challenge.GetLength());
    mChildIdRequest.mChallengeLength = challenge.GetLength();

    mParent.mMacAddr.Set(aMessageInfo.GetPeerAddr());
    mParent.mValid.mRloc16 = sourceAddress.GetRloc16();
    mParent.mValid.mLinkFrameCounter = linkFrameCounter.GetFrameCounter();
    mParent.mValid.mMleFrameCounter = mleFrameCounter.GetFrameCounter();
    mParent.mMode = ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData;
    mParent.mLinkInfo.Clear();
    mParent.mLinkInfo.AddRss(mMac.GetNoiseFloor(), threadMessageInfo->mRss);
    mParent.mLinkFailures = 0;
    mParent.mState = Neighbor::kStateValid;
    mParent.mKeySequence = aKeySequence;

    mParentLinkQuality = linkQuality;
    mParentPriority = connectivity.GetParentPriority();
    mParentLinkQuality3 = connectivity.GetLinkQuality3();
    mParentLinkQuality2 = connectivity.GetLinkQuality2();
    mParentLinkQuality1 = connectivity.GetLinkQuality1();
    mParentLeaderData = leaderData;
    mParentIsSingleton = connectivity.GetActiveRouters() <= 1;

exit:

    if (error != kThreadError_None)
    {
        otLogWarnMleErr(error, "Failed to process Parent Response");
    }

    return error;
}

ThreadError Mle::HandleChildIdResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    LeaderDataTlv leaderData;
    SourceAddressTlv sourceAddress;
    Address16Tlv shortAddress;
    NetworkDataTlv networkData;
    RouteTlv route;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    Tlv tlv;
    uint16_t offset;
    uint8_t numRouters = 0;

    otLogInfoMle("Received Child ID Response");

    VerifyOrExit(mParentRequestState == kChildIdRequest, ;);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

    // ShortAddress
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kAddress16, sizeof(shortAddress), shortAddress));
    VerifyOrExit(shortAddress.IsValid(), error = kThreadError_Parse);

    // Network Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData));

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == kThreadError_None)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = kThreadError_Parse);

        // Active Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kActiveDataset, offset) == kThreadError_None)
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
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == kThreadError_None)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = kThreadError_Parse);

        // Pending Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, offset) == kThreadError_None)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            mNetif.GetPendingDataset().Set(pendingTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength());
        }
        // this is a workaround for Certification test 9.2.15 and 9.2.16 (https://github.com/openthread/openthread/issues/918)
        else if (mNetif.GetPendingDataset().GetNetwork().GetTimestamp() == NULL &&
                 mNetif.GetPendingDataset().GetLocal().GetTimestamp() != NULL)
        {
            mNetif.GetPendingDataset().Set(mNetif.GetPendingDataset().GetLocal());
        }
    }
    else
    {
        mNetif.GetPendingDataset().Clear(true);
    }

    // Parent Attach Success
    mParentRequestTimer.Stop();
    mReattachState = kReattachStop;

    SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mMesh.SetPollPeriod(Timer::SecToMsec(mTimeout / kMaxChildKeepAliveAttempts));
        mMesh.SetRxOnWhenIdle(false);
    }
    else
    {
        mMesh.SetRxOnWhenIdle(true);
    }

    // Route
    if ((Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == kThreadError_None) &&
        (mDeviceMode & ModeTlv::kModeFFD))
    {
        SuccessOrExit(error = mMleRouter.ProcessRouteTlv(route));

        for (uint8_t i = 0; i <= kMaxRouterId; i++)
        {
            if (route.IsRouterIdSet(i))
            {
                numRouters++;
            }
        }

        if (mRouterSelectionJitterTimeout == 0 && numRouters < mMleRouter.GetRouterUpgradeThreshold())
        {
            mRouterSelectionJitterTimeout = (otPlatRandomGet() % mRouterSelectionJitter) + 1;
        }
    }

    mParent.mValid.mRloc16 = sourceAddress.GetRloc16();
    SuccessOrExit(error = SetStateChild(shortAddress.GetRloc16()));

    mNetworkData.SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                networkData.GetNetworkData(), networkData.GetLength());

    mNetif.GetActiveDataset().ApplyConfiguration();

exit:

    if (error != kThreadError_None)
    {
        otLogWarnMleErr(error, "Failed to process Child ID Response");
    }

    (void)aMessageInfo;
    return error;
}

ThreadError Mle::HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static const uint8_t kMaxResponseTlvs = 5;

    ThreadError error = kThreadError_None;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv leaderData;
    NetworkDataTlv networkData;
    ChallengeTlv challenge;
    TlvRequestTlv tlvRequest;
    uint8_t dataRequestTlvs[] = {Tlv::kNetworkData};
    uint8_t tlvs[kMaxResponseTlvs] = {};
    uint8_t numTlvs = 0;

    otLogInfoMle("Received Child Update Request from parent");

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);
    VerifyOrExit(mParent.mValid.mRloc16 == sourceAddress.GetRloc16(), error = kThreadError_Drop);

    // Leader Data
    if (Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData) == kThreadError_None)
    {
        VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);
        SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

        if ((mDeviceMode & ModeTlv::kModeFullNetworkData && leaderData.GetDataVersion() != mNetworkData.GetVersion()) ||
            ((mDeviceMode & ModeTlv::kModeFullNetworkData) == 0 &&
             leaderData.GetStableDataVersion() != mNetworkData.GetStableVersion()))
        {
            mRetrieveNewNetworkData = true;
        }

        // Network Data
        if (Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData) == kThreadError_None)
        {
            VerifyOrExit(networkData.IsValid(), error = kThreadError_Parse);
            mNetworkData.SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                        (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                        networkData.GetNetworkData(), networkData.GetLength());
        }
    }

    // TLV Request
    if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == kThreadError_None)
    {
        VerifyOrExit(tlvRequest.IsValid() && tlvRequest.GetLength() <= sizeof(tlvs), error = kThreadError_Parse);
        memcpy(tlvs, tlvRequest.GetTlvs(), tlvRequest.GetLength());
        numTlvs += tlvRequest.GetLength();
    }

    // Challenge
    if (Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge) == kThreadError_None)
    {
        VerifyOrExit(challenge.IsValid(), error = kThreadError_Parse);
        VerifyOrExit(static_cast<size_t>(numTlvs + 3) <= sizeof(tlvs), error = kThreadError_NoBufs);
        tlvs[numTlvs++] = Tlv::kResponse;
        tlvs[numTlvs++] = Tlv::kMleFrameCounter;
        tlvs[numTlvs++] = Tlv::kLinkFrameCounter;
    }

    if (mRetrieveNewNetworkData)
    {
        SendDataRequest(aMessageInfo.GetPeerAddr(), dataRequestTlvs, sizeof(tlvs));
    }

    SuccessOrExit(error = SendChildUpdateResponse(tlvs, numTlvs, challenge));

exit:
    return error;
}

ThreadError Mle::HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    StatusTlv status;
    ModeTlv mode;
    ResponseTlv response;
    LeaderDataTlv leaderData;
    SourceAddressTlv sourceAddress;
    TimeoutTlv timeout;
    NetworkDataTlv networkData;
    uint8_t tlvs[] = {Tlv::kNetworkData};

    otLogInfoMle("Received Child Update Response from parent");

    // Status
    if (Tlv::GetTlv(aMessage, Tlv::kStatus, sizeof(status), status) == kThreadError_None)
    {
        BecomeDetached();
        ExitNow();
    }

    // Mode
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kMode, sizeof(mode), mode));
    VerifyOrExit(mode.IsValid(), error = kThreadError_Parse);
    VerifyOrExit(mode.GetMode() == mDeviceMode, error = kThreadError_Drop);

    switch (mDeviceState)
    {
    case kDeviceStateDetached:
        // Response
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
        VerifyOrExit(response.IsValid(), error = kThreadError_Parse);
        VerifyOrExit(memcmp(response.GetResponse(), mParentRequest.mChallenge,
                            sizeof(mParentRequest.mChallenge)) == 0,
                     error = kThreadError_Drop);

        SetStateChild(GetRloc16());

    // fall through

    case kDeviceStateChild:
        // Leader Data
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
        VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

        // Source Address
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
        VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

        if (GetRouterId(sourceAddress.GetRloc16()) != GetRouterId(GetRloc16()))
        {
            BecomeDetached();
            ExitNow();
        }

        // Timeout optional
        if (Tlv::GetTlv(aMessage, Tlv::kTimeout, sizeof(timeout), timeout) == kThreadError_None)
        {
            VerifyOrExit(timeout.IsValid(), error = kThreadError_Parse);
            mTimeout = timeout.GetTimeout();
        }

        // Network Data optional
        if (Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData) == kThreadError_None)
        {
            VerifyOrExit(networkData.IsValid(), error = kThreadError_Parse);
            mNetworkData.SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                        (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                        networkData.GetNetworkData(), networkData.GetLength());
        }

        if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            mMesh.SetPollPeriod(Timer::SecToMsec(mTimeout / kMaxChildKeepAliveAttempts));
            mMesh.SetRxOnWhenIdle(false);
        }
        else
        {
            mMesh.SetRxOnWhenIdle(true);
        }

        if (mDeviceMode & ModeTlv::kModeFullNetworkData)
        {
            // full network data
            if (leaderData.GetDataVersion() != mNetworkData.GetVersion())
            {
                SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs));
            }
        }
        else
        {
            // stable network data
            if (leaderData.GetStableDataVersion() != mNetworkData.GetStableVersion())
            {
                SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs));
            }
        }

        break;

    default:
        assert(false);
        break;
    }

exit:

    if (error != kThreadError_None)
    {
        otLogWarnMleErr(error, "Failed to process Child Update Response");
    }

    return error;
}

ThreadError Mle::HandleAnnounce(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    ChannelTlv channel;
    ActiveTimestampTlv timestamp;
    const MeshCoP::Timestamp *localTimestamp;
    PanIdTlv panid;

    otLogInfoMle("Received announce");

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kChannel, sizeof(channel), channel));
    VerifyOrExit(channel.IsValid(),);

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(timestamp), timestamp));
    VerifyOrExit(timestamp.IsValid(),);

    SuccessOrExit(Tlv::GetTlv(aMessage, Tlv::kPanId, sizeof(panid), panid));
    VerifyOrExit(panid.IsValid(),);

    localTimestamp = mNetif.GetActiveDataset().GetNetwork().GetTimestamp();

    if (localTimestamp == NULL || localTimestamp->Compare(timestamp) > 0)
    {
        if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
        {
            mRetrieveNewNetworkData = true;
        }

        Stop(false);
        mPreviousChannel = mMac.GetChannel();
        mPreviousPanId = mMac.GetPanId();
        mMac.SetChannel(static_cast<uint8_t>(channel.GetChannel()));
        mMac.SetPanId(panid.GetPanId());
        Start(false);
    }
    else
    {
        SendAnnounce(static_cast<uint8_t>(channel.GetChannel()), false);
    }

exit:
    (void)aMessageInfo;
    return error;
}

ThreadError Mle::HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
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

    otLogInfoMle("Handle discovery response");

    VerifyOrExit(mIsDiscoverInProgress, error = kThreadError_Drop);

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
            VerifyOrExit(discoveryResponse.IsValid(), error = kThreadError_Parse);
            result.mVersion = discoveryResponse.GetVersion();
            result.mIsNative = discoveryResponse.IsNativeCommissioner();
            break;

        case MeshCoP::Tlv::kExtendedPanId:
            aMessage.Read(offset, sizeof(extPanId), &extPanId);
            VerifyOrExit(extPanId.IsValid(), error = kThreadError_Parse);
            memcpy(&result.mExtendedPanId, extPanId.GetExtendedPanId(), sizeof(result.mExtendedPanId));
            break;

        case MeshCoP::Tlv::kNetworkName:
            aMessage.Read(offset, sizeof(networkName), &networkName);
            VerifyOrExit(networkName.IsValid(), error = kThreadError_Parse);
            memcpy(&result.mNetworkName, networkName.GetNetworkName(), networkName.GetLength());
            break;

        case MeshCoP::Tlv::kSteeringData:
            aMessage.Read(offset, sizeof(steeringData), &steeringData);
            VerifyOrExit(steeringData.IsValid(), error = kThreadError_Parse);
            result.mSteeringData.mLength = steeringData.GetLength();
            memcpy(result.mSteeringData.m8, steeringData.GetValue(), result.mSteeringData.mLength);
            break;

        case MeshCoP::Tlv::kJoinerUdpPort:
            aMessage.Read(offset, sizeof(JoinerUdpPort), &JoinerUdpPort);
            VerifyOrExit(JoinerUdpPort.IsValid(), error = kThreadError_Parse);
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

    if (error != kThreadError_None)
    {
        otLogWarnMleErr(error, "Failed to process Discovery Response");
    }

    return error;
}

Neighbor *Mle::GetNeighbor(uint16_t aAddress)
{
    return (mParent.mState == Neighbor::kStateValid && mParent.mValid.mRloc16 == aAddress) ? &mParent : NULL;
}

Neighbor *Mle::GetNeighbor(const Mac::ExtAddress &aAddress)
{
    return (mParent.mState == Neighbor::kStateValid &&
            memcmp(&mParent.mMacAddr, &aAddress, sizeof(mParent.mMacAddr)) == 0) ? &mParent : NULL;
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

Neighbor *Mle::GetNeighbor(const Ip6::Address &aAddress)
{
    (void)aAddress;
    return NULL;
}

uint16_t Mle::GetNextHop(uint16_t aDestination) const
{
    (void)aDestination;
    return (mParent.mState == Neighbor::kStateValid) ? mParent.mValid.mRloc16 : static_cast<uint16_t>
           (Mac::kShortAddrInvalid);
}

bool Mle::IsRoutingLocator(const Ip6::Address &aAddress) const
{
    return memcmp(&mMeshLocal16, &aAddress, kRlocPrefixLength) == 0 && aAddress.mFields.m8[14] != kAloc16Mask;
}

bool Mle::IsAnycastLocator(const Ip6::Address &aAddress) const
{
    return memcmp(&mMeshLocal16, &aAddress, kRlocPrefixLength) == 0 && aAddress.mFields.m8[14] == kAloc16Mask;
}

Router *Mle::GetParent()
{
    return &mParent;
}

ThreadError Mle::CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    ThreadError error = kThreadError_Drop;
    Ip6::Address dst;

    if (aMeshDest != GetRloc16())
    {
        ExitNow(error = kThreadError_None);
    }

    if (mNetif.IsUnicastAddress(aIp6Header.GetDestination()))
    {
        ExitNow(error = kThreadError_None);
    }

    dst = GetMeshLocal16();
    dst.mFields.m16[7] = HostSwap16(aMeshSource);
    mNetif.GetIp6().mIcmp.SendError(dst, Ip6::IcmpHeader::kTypeDstUnreach, Ip6::IcmpHeader::kCodeDstUnreachNoRoute,
                                    aIp6Header);

exit:
    return error;
}

}  // namespace Mle
}  // namespace Thread
