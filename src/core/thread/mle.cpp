/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <thread/mle.hpp>
#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/encoding.hpp>
#include <crypto/aes_ccm.hpp>
#include <mac/mac_frame.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <platform/radio.h>
#include <platform/random.h>
#include <thread/address_resolver.hpp>
#include <thread/meshcop_tlvs.hpp>
#include <thread/key_manager.hpp>
#include <thread/mle_router.hpp>
#include <thread/thread_netif.hpp>
#include <openthreadcontext.h>

#ifdef WINDOWS_LOGGING
#include <mle.tmh>
#endif

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
    mParentRequestTimer(aThreadNetif.GetOpenThreadContext(), &Mle::HandleParentRequestTimer, this)
{
    mDeviceState = kDeviceStateDisabled;
    mDeviceMode = ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeSecureDataRequest | ModeTlv::kModeFFD |
                  ModeTlv::kModeFullNetworkData;
    mParentRequestState = kParentIdle;
    mParentRequestMode = kMleAttachAnyPartition;
    mParentLinkQuality = 0;
    mParentPriority = 0;
    mParentLinkQuality3 = 0;
    mParentLinkQuality2 = 0;
    mParentLinkQuality1 = 0;
    mRetrieveNewNetworkData = false;
    mTimeout = kMaxNeighborAge;

    memset(&mLeaderData, 0, sizeof(mLeaderData));
    memset(&mParent, 0, sizeof(mParent));
    memset(&mChildIdRequest, 0, sizeof(mChildIdRequest));
    memset(&mLinkLocal64, 0, sizeof(mLinkLocal64));
    memset(&mLinkLocal16, 0, sizeof(mLinkLocal16));
    memset(&mMeshLocal64, 0, sizeof(mMeshLocal64));
    memset(&mMeshLocal16, 0, sizeof(mMeshLocal16));
    memset(&mLinkLocalAllThreadNodes, 0, sizeof(mLinkLocalAllThreadNodes));
    memset(&mRealmLocalAllThreadNodes, 0, sizeof(mRealmLocalAllThreadNodes));

    // link-local 64
    memset(&mLinkLocal64, 0, sizeof(mLinkLocal64));
    mLinkLocal64.GetAddress().mFields.m16[0] = HostSwap16(0xfe80);
    mLinkLocal64.GetAddress().SetIid(*mMac.GetExtAddress());
    mLinkLocal64.mPrefixLength = 64;
    mLinkLocal64.mPreferredLifetime = 0xffffffff;
    mLinkLocal64.mValidLifetime = 0xffffffff;
    mNetif.AddUnicastAddress(mLinkLocal64);

    // link-local 16
    memset(&mLinkLocal16, 0, sizeof(mLinkLocal16));
    mLinkLocal16.GetAddress().mFields.m16[0] = HostSwap16(0xfe80);
    mLinkLocal16.GetAddress().mFields.m16[5] = HostSwap16(0x00ff);
    mLinkLocal16.GetAddress().mFields.m16[6] = HostSwap16(0xfe00);
    mLinkLocal16.mPrefixLength = 64;
    mLinkLocal16.mPreferredLifetime = 0xffffffff;
    mLinkLocal16.mValidLifetime = 0xffffffff;

    // initialize Mesh Local Prefix
    mMeshLocal64.GetAddress().mFields.m8[0] = 0xfd;
    memcpy(mMeshLocal64.GetAddress().mFields.m8 + 1, mMac.GetExtendedPanId(), 5);
    mMeshLocal64.GetAddress().mFields.m8[6] = 0x00;
    mMeshLocal64.GetAddress().mFields.m8[7] = 0x00;

    // mesh-local 64
    for (int i = 8; i < 16; i++)
    {
        mMeshLocal64.GetAddress().mFields.m8[i] = (uint8_t)otPlatRandomGet();
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
}

ThreadError Mle::Enable(void)
{
    ThreadError error = kThreadError_None;
    Ip6::SockAddr sockaddr;

    // memcpy(&sockaddr.mAddr, &mLinkLocal64.GetAddress(), sizeof(sockaddr.mAddr));
    sockaddr.mPort = kUdpPort;
    SuccessOrExit(error = mSocket.Open(mNetif.GetOpenThreadContext(), &Mle::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(sockaddr));

exit:
    return error;
}

ThreadError Mle::Disable(void)
{
    ThreadError error = kThreadError_None;

    SuccessOrExit(error = Stop());
    SuccessOrExit(error = mSocket.Close(mNetif.GetOpenThreadContext()));

exit:
    return error;
}

ThreadError Mle::Start(void)
{
    ThreadError error = kThreadError_None;

    // cannot bring up the interface if IEEE 802.15.4 promiscuous mode is enabled
    VerifyOrExit(otPlatRadioGetPromiscuous(mNetif.GetOpenThreadContext()) == false, error = kThreadError_Busy);
    VerifyOrExit(mNetif.IsUp(), error = kThreadError_InvalidState);

    mDeviceState = kDeviceStateDetached;
    SetStateDetached();

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
    return error;
}

ThreadError Mle::Stop(void)
{
    SetStateDetached();
    mNetif.RemoveUnicastAddress(mLinkLocal16);
    mNetif.RemoveUnicastAddress(mMeshLocal16);
    mDeviceState = kDeviceStateDisabled;
    return kThreadError_None;
}

ThreadError Mle::Discover(uint32_t aScanChannels, uint16_t aScanDuration, uint16_t aPanId,
                          DiscoverHandler aCallback, void *aContext)
{
    ThreadError error = kThreadError_None;
    Message *message;
    Ip6::Address destination;
    Tlv tlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    uint16_t startOffset;

    mDiscoverHandler = aCallback;
    mDiscoverContext = aContext;
    mMesh.SetDiscoverParameters(aScanChannels, aScanDuration);

    VerifyOrExit((message = Ip6::Udp::NewMessage(mNetif.GetOpenThreadContext(), 0)) != NULL, ;);
    message->SetLinkSecurityEnabled(false);
    message->SetMleDiscoverRequest(true);
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

    tlv.SetLength((uint8_t)(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0002);
    SuccessOrExit(error = SendMessage(*message, destination));

    otLogInfoMle("Sent discovery request\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

void Mle::HandleDiscoverComplete(void)
{
    mDiscoverHandler(NULL, mDiscoverContext);
}

ThreadError Mle::BecomeDetached(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mDeviceState != kDeviceStateDisabled, error = kThreadError_Busy);

    SetStateDetached();
    SetRloc16(Mac::kShortAddrInvalid);
    BecomeChild(kMleAttachAnyPartition);

exit:
    return error;
}

ThreadError Mle::BecomeChild(otMleAttachFilter aFilter)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mDeviceState != kDeviceStateDisabled &&
                 mParentRequestState == kParentIdle, error = kThreadError_Busy);

    mParentRequestState = kParentRequestStart;
    mParentRequestMode = aFilter;
    memset(&mParent, 0, sizeof(mParent));

    if (aFilter == kMleAttachAnyPartition)
    {
        mParent.mState = Neighbor::kStateInvalid;
    }

    mParentRequestTimer.Start(kParentRequestRouterTimeout);

exit:
    return error;
}

bool Mle::IsAttached(void) const
{ 
    return (mDeviceState == kDeviceStateChild ||
            mDeviceState == kDeviceStateRouter ||
            mDeviceState == kDeviceStateLeader);
}

otContext *Mle::GetOpenThreadContext(void) 
{ 
    return mNetif.GetOpenThreadContext(); 
}

DeviceState Mle::GetDeviceState(void) const
{
    return mDeviceState;
}

ThreadError Mle::SetStateDetached(void)
{
    if (mDeviceState != kDeviceStateDetached)
    {
        mNetif.SetStateChangedFlags(OT_NET_STATE | OT_NET_ROLE);
    }

    mAddressResolver.Clear();
    mDeviceState = kDeviceStateDetached;
    mParentRequestState = kParentIdle;
    mParentRequestTimer.Stop();
    mMesh.SetRxOnWhenIdle(true);
    mMleRouter.HandleDetachStart();
    otLogInfoMle("Mode -> Detached\n");
    return kThreadError_None;
}

ThreadError Mle::SetStateChild(uint16_t aRloc16)
{
    if (mDeviceState == kDeviceStateDetached)
    {
        mNetif.SetStateChangedFlags(OT_NET_STATE);
    }

    if (mDeviceState != kDeviceStateChild)
    {
        mNetif.SetStateChangedFlags(OT_NET_ROLE);
    }

    SetRloc16(aRloc16);
    mDeviceState = kDeviceStateChild;
    mParentRequestState = kParentIdle;

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0)
    {
        mParentRequestTimer.Start(Timer::SecToMsec(mTimeout / 2));
    }

    if ((mDeviceMode & ModeTlv::kModeFFD) != 0)
    {
        mMleRouter.HandleChildStart(mParentRequestMode);
    }

    otLogInfoMle("Mode -> Child\n");
    return kThreadError_None;
}

uint32_t Mle::GetTimeout(void) const
{
    return mTimeout;
}

ThreadError Mle::SetTimeout(uint32_t aTimeout)
{
    if (aTimeout < 2)
    {
        aTimeout = 2;
    }

    mTimeout = aTimeout;

    if (mDeviceState == kDeviceStateChild)
    {
        SendChildUpdateRequest();

        if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0)
        {
            mParentRequestTimer.Start(Timer::SecToMsec(mTimeout / 2));
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
    mNetif.RemoveUnicastAddress(mLinkLocal16);
    mNetif.RemoveUnicastAddress(mMeshLocal16);

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        // link-local 16
        mLinkLocal16.GetAddress().mFields.m16[7] = HostSwap16(aRloc16);
        mNetif.AddUnicastAddress(mLinkLocal16);

        // mesh-local 16
        mMeshLocal16.GetAddress().mFields.m16[7] = HostSwap16(aRloc16);
        mNetif.AddUnicastAddress(mMeshLocal16);
    }

    mMac.SetShortAddress(aRloc16);

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

const Ip6::Address *Mle::GetMeshLocal16(void) const
{
    return &mMeshLocal16.GetAddress();
}

const Ip6::Address *Mle::GetMeshLocal64(void) const
{
    return &mMeshLocal64.GetAddress();
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

void Mle::GenerateNonce(const Mac::ExtAddress &aMacAddr, uint32_t aFrameCounter, uint8_t aSecurityLevel,
                        uint8_t *aNonce)
{
    // source address
    memcpy(aNonce, aMacAddr.m8, sizeof(aMacAddr));
    aNonce += sizeof(aMacAddr);

    // frame counter
    aNonce[0] = (uint8_t)(aFrameCounter >> 24);
    aNonce[1] = (uint8_t)(aFrameCounter >> 16);
    aNonce[2] = (uint8_t)(aFrameCounter >> 8);
    aNonce[3] = (uint8_t)(aFrameCounter >> 0);
    aNonce += 4;

    // security level
    aNonce[0] = aSecurityLevel;
}

ThreadError Mle::AppendHeader(Message &aMessage, Header::Command aCommand)
{
    ThreadError error = kThreadError_None;
    Header header;

    header.Init();
    
    switch (aCommand)
    {
    case Header::kCommandDiscoveryRequest:
    case Header::kCommandDiscoveryResponse:
        header.SetSecuritySuite(255);
        break;

    case Header::kCommandAdvertisement:
    case Header::kCommandChildIdRequest:
    case Header::kCommandLinkReject:
    case Header::kCommandParentRequest:
    case Header::kCommandParentResponse:
        header.SetKeyIdMode2();
        break;

    default:
        header.SetKeyIdMode1();
        break;
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

ThreadError Mle::AppendNetworkData(Message &aMessage, bool aStableOnly)
{
    ThreadError error = kThreadError_None;
    NetworkDataTlv tlv;
    uint8_t length;

    tlv.Init();
    mNetworkData.GetNetworkData(aStableOnly, tlv.GetNetworkData(), length);
    tlv.SetLength(length);

    SuccessOrExit(error = aMessage.Append(&tlv, (uint16_t)(sizeof(Tlv) + tlv.GetLength())));

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
    uint8_t startOffset = (uint8_t)aMessage.GetLength();

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

ThreadError Mle::AppendActiveTimestamp(Message &aMessage)
{
    ThreadError error;
    ActiveTimestampTlv timestampTlv;
    const MeshCoP::Timestamp &timestamp(mNetif.GetActiveDataset().GetNetwork().GetTimestamp());

    VerifyOrExit(timestamp.GetSeconds() != 0, error = kThreadError_None);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = timestamp;
    error = aMessage.Append(&timestampTlv, sizeof(timestampTlv));

exit:
    return error;
}

ThreadError Mle::AppendPendingTimestamp(Message &aMessage)
{
    ThreadError error;
    PendingTimestampTlv timestampTlv;
    const MeshCoP::Timestamp &timestamp(mNetif.GetPendingDataset().GetNetwork().GetTimestamp());

    VerifyOrExit(timestamp.GetSeconds() != 0, error = kThreadError_None);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = timestamp;
    error = aMessage.Append(&timestampTlv, sizeof(timestampTlv));

exit:
    return error;
}

void Mle::HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    Mle *obj = reinterpret_cast<Mle *>(aContext);
    obj->HandleNetifStateChanged(aFlags);
}

void Mle::HandleNetifStateChanged(uint32_t aFlags)
{
    VerifyOrExit((aFlags & (OT_IP6_ADDRESS_ADDED | OT_IP6_ADDRESS_REMOVED)) != 0, ;);

    if (!mNetif.IsUnicastAddress(GetOpenThreadContext(), mMeshLocal64.GetAddress()))
    {
        // Mesh Local EID was removed, choose a new one and add it back
        for (int i = 8; i < 16; i++)
        {
            mMeshLocal64.GetAddress().mFields.m8[i] = (uint8_t)otPlatRandomGet();
        }

        mNetif.AddUnicastAddress(mMeshLocal64);
        mNetif.SetStateChangedFlags(OT_IP6_ML_ADDR_CHANGED);
    }

    switch (mDeviceState)
    {
    case kDeviceStateChild:
        SendChildUpdateRequest();
        break;

    default:
        break;
    }

exit:
    return;
}

void Mle::HandleParentRequestTimer(void *aContext)
{
    Mle *obj = reinterpret_cast<Mle *>(aContext);
    obj->HandleParentRequestTimer();
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
                mParentRequestTimer.Start(Timer::SecToMsec(mTimeout / 2));
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
            switch (mParentRequestMode)
            {
            case kMleAttachAnyPartition:
                if (mDeviceMode & ModeTlv::kModeFFD)
                {
                    mMleRouter.BecomeLeader();
                }
                else
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
        mParentRequest.mChallenge[i] = (uint8_t)otPlatRandomGet();
    }

    VerifyOrExit((message = Ip6::Udp::NewMessage(mNetif.GetOpenThreadContext(), 0)) != NULL, ;);
    message->SetLinkSecurityEnabled(false);
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
        otLogInfoMle("Sent parent request to routers\n");
        break;

    case kParentRequestChild:
        otLogInfoMle("Sent parent request to all devices\n");
        break;

    default:
        assert(false);
        break;
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return kThreadError_None;
}

ThreadError Mle::SendChildIdRequest(void)
{
    ThreadError error = kThreadError_None;
    uint8_t tlvs[] = {Tlv::kAddress16, Tlv::kNetworkData, Tlv::kRoute};
    Message *message;
    Ip6::Address destination;

    VerifyOrExit((message = Ip6::Udp::NewMessage(mNetif.GetOpenThreadContext(), 0)) != NULL, ;);
    message->SetLinkSecurityEnabled(false);
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
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParent.mMacAddr);
    SuccessOrExit(error = SendMessage(*message, destination));
    otLogInfoMle("Sent Child ID Request\n");

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mMesh.SetPollPeriod(kAttachDataPollPeriod);
        mMesh.SetRxOnWhenIdle(false);
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

ThreadError Mle::SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    ThreadError error = kThreadError_None;
    Message *message;

    VerifyOrExit((message = Ip6::Udp::NewMessage(mNetif.GetOpenThreadContext(), 0)) != NULL, ;);
    message->SetLinkSecurityEnabled(false);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDataRequest));
    SuccessOrExit(error = AppendTlvRequest(*message, aTlvs, aTlvsLength));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle("Sent Data Request\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

ThreadError Mle::SendChildUpdateRequest(void)
{
    ThreadError error = kThreadError_None;
    Ip6::Address destination;
    Message *message;

    VerifyOrExit((message = Ip6::Udp::NewMessage(mNetif.GetOpenThreadContext(), 0)) != NULL, ;);
    message->SetLinkSecurityEnabled(false);
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
            mParentRequest.mChallenge[i] = (uint8_t)otPlatRandomGet();
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

    otLogInfoMle("Sent Child Update Request\n");

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mMesh.SetPollPeriod(kAttachDataPollPeriod);
        mMesh.SetRxOnWhenIdle(false);
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

ThreadError Mle::SendMessage(Message &aMessage, const Ip6::Address &aDestination)
{
    ThreadError error = kThreadError_None;
    Header header;
    uint32_t keySequence;
    uint8_t nonce[13];
    uint8_t tag[4];
    uint8_t tagLength;
    Crypto::AesCcm aesCcm(&GetOpenThreadContext()->mCryptoContext);
    uint8_t buf[64];
    int length;
    Ip6::MessageInfo messageInfo;

    aMessage.Read(0, sizeof(header), &header);

    if (header.GetSecuritySuite() == 0)
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
            aMessage.Write(aMessage.GetOffset(), (uint16_t)length, buf);
            aMessage.MoveOffset(length);
        }

        tagLength = sizeof(tag);
        aesCcm.Finalize(tag, &tagLength);
        SuccessOrExit(aMessage.Append(tag, tagLength));
    
        mKeyManager.IncrementMleFrameCounter();
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    memcpy(&messageInfo.GetPeerAddr(), &aDestination, sizeof(messageInfo.GetPeerAddr()));
    memcpy(&messageInfo.GetSockAddr(), &mLinkLocal64.GetAddress(), sizeof(messageInfo.GetSockAddr()));
    messageInfo.mPeerPort = kUdpPort;
    messageInfo.mInterfaceId = (uint8_t)mNetif.GetInterfaceId();
    messageInfo.mHopLimit = 255;

    SuccessOrExit(error = mSocket.SendTo(aMessage, messageInfo));

exit:
    return error;
}

void Mle::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    Mle *obj = reinterpret_cast<Mle *>(aContext);
    obj->HandleUdpReceive(*static_cast<Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Mle::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Header header;
    uint32_t keySequence;
    const uint8_t *mleKey;
    uint8_t keyid;
    uint32_t frameCounter;
    uint8_t messageTag[4];
    uint8_t messageTagLength;
    uint8_t nonce[13];
    Mac::ExtAddress macAddr;
    Crypto::AesCcm aesCcm(&GetOpenThreadContext()->mCryptoContext);
    uint16_t mleOffset;
    uint8_t buf[64];
    int length;
    uint8_t tag[4];
    uint8_t tagLength;
    uint8_t command;
    Neighbor *neighbor;

    otLogFuncEntry();

    aMessage.Read(aMessage.GetOffset(), sizeof(header), &header);
    VerifyOrExit(header.IsValid(),);
    
    if (header.GetSecuritySuite() == 255)
    {
        aMessage.MoveOffset(header.GetLength());

        switch (header.GetCommand())
        {
        case Header::kCommandDiscoveryRequest:
            HandleDiscoveryRequest(aMessage, aMessageInfo);
            break;

        case Header::kCommandDiscoveryResponse:
            HandleDiscoveryResponse(aMessage, aMessageInfo);
            break;

        default:
            break;
        }

        ExitNow();
    }

    VerifyOrExit(mDeviceState != kDeviceStateDisabled && header.GetSecuritySuite() == 0, ;);

    if (header.IsKeyIdMode1())
    {
        keyid = (uint8_t)header.GetKeyId();

        if (keyid == (mKeyManager.GetCurrentKeySequence() & 0x7f))
        {
            keySequence = mKeyManager.GetCurrentKeySequence();
            mleKey = mKeyManager.GetCurrentMleKey();
        }
        else
        {
            keySequence = (mKeyManager.GetCurrentKeySequence() & ~0x7f) | keyid;

            if (keySequence < mKeyManager.GetCurrentKeySequence())
            {
                keySequence += 128;
            }

            mleKey = mKeyManager.GetTemporaryMleKey(keySequence);
        }
    }
    else
    {
        keySequence = header.GetKeyId();

        if (keySequence == mKeyManager.GetCurrentKeySequence())
        {
            mleKey = mKeyManager.GetCurrentMleKey();
        }
        else
        {
            mleKey = mKeyManager.GetTemporaryMleKey(keySequence);
        }
    }

    aMessage.MoveOffset(header.GetLength() - 1);

    frameCounter = header.GetFrameCounter();

    messageTagLength = (uint8_t)aMessage.Read(aMessage.GetLength() - sizeof(messageTag), sizeof(messageTag), messageTag);
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
        aMessage.Write(aMessage.GetOffset(), (uint16_t)length, buf);
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
            if (!(frameCounter >= neighbor->mValid.mMleFrameCounter))
            {
                otLogDebgMle("mle frame reject 1\n");
                goto exit;
            }
        }
        else
        {
            if (!(keySequence > neighbor->mKeySequence))
            {
                otLogDebgMle("mle frame reject 2\n");
                goto exit;
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
              command == Header::kCommandChildUpdateRequest))
        {
            otLogDebgMle("mle sequence unknown! %d\n", command);
            goto exit;
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

    case Header::kCommandLinkReject:
        mMleRouter.HandleLinkReject(aMessage, aMessageInfo);
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
        mMleRouter.HandleChildUpdateRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandChildUpdateResponse:
        HandleChildUpdateResponse(aMessage, aMessageInfo);
        break;
    }

exit:
    {}
    otLogFuncExit();
}

ThreadError Mle::HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Mac::ExtAddress macAddr;
    bool isNeighbor;
    Neighbor *neighbor;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv leaderData;
    uint8_t tlvs[] = {Tlv::kNetworkData};

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = kThreadError_Parse);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = kThreadError_Parse);

    otLogInfoMle("Received advertisement from %04x\n", sourceAddress.GetRloc16());

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
    return error;
}

ThreadError Mle::HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    LeaderDataTlv leaderData;
    NetworkDataTlv networkData;
    ActiveTimestampTlv activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    Tlv tlv;
    uint16_t offset;
    int8_t diff;

    otLogInfoMle("Received Data Response\n");
    
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
    else if (mRetrieveNewNetworkData)
    {
        mRetrieveNewNetworkData = false;
    }
    else
    {
        diff = leaderData.GetDataVersion() - mNetworkData.GetVersion();
        VerifyOrExit(diff > 0, ;);
    }

    // Network Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kNetworkData, sizeof(networkData), networkData));
    VerifyOrExit(networkData.IsValid(), error = kThreadError_Parse);

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == kThreadError_None)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = kThreadError_Parse);

        // Active Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kActiveDataset, offset) == kThreadError_None)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            mNetif.GetActiveDataset().Set(activeTimestamp, aMessage, offset + sizeof(tlv), (uint16_t)tlv.GetLength());
        }
    }

    // Pending Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == kThreadError_None)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = kThreadError_Parse);

        // Pending Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, offset) == kThreadError_None)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            mNetif.GetPendingDataset().Set(activeTimestamp, aMessage, offset + sizeof(tlv), (uint16_t)tlv.GetLength());
        }
    }

    // Network Data
    mNetworkData.SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                networkData.GetNetworkData(), (uint8_t)networkData.GetLength());

exit:
    (void)aMessageInfo;
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
    const ThreadMessageInfo *threadMessageInfo = reinterpret_cast<const ThreadMessageInfo *>(aMessageInfo.mLinkInfo);
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

    otLogInfoMle("Received Parent Response\n");

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

    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(GetOpenThreadContext(), threadMessageInfo->mRss);

    if (linkMargin > linkMarginTlv.GetLinkMargin())
    {
        linkMargin = linkMarginTlv.GetLinkMargin();
    }

    linkQuality = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin);

    VerifyOrExit(mParentRequestState != kParentRequestRouter || linkQuality == 3, ;);

    // Connectivity
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kConnectivity, sizeof(connectivity), connectivity));
    VerifyOrExit(connectivity.IsValid(), error = kThreadError_Parse);

    // if already attached, Router/REED only seeks a better partition
    if ((mDeviceMode & ModeTlv::kModeFFD) &&
        (mDeviceState != kDeviceStateDetached) &&
        (mParentRequestMode != kMleAttachAnyPartition))
    {
        if (leaderData.GetPartitionId() == mLeaderData.GetPartitionId())
        {
            // looking for a larger Sequence ID
            diff = connectivity.GetIdSequence() - mMleRouter.GetRouterIdSequence();
            VerifyOrExit(diff > 0 || (diff == 0 && mMleRouter.GetLeaderAge() < mMleRouter.GetNetworkIdTimeout()), ;);
        }
        else
        {
            // looking for a better partition
            VerifyOrExit(mMleRouter.ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                      mMleRouter.IsSingleton(), mLeaderData) > 0, ;);
        }
    }

    // if already have a candidate parent, only seek a better parent
    if (mParent.mState == Neighbor::kStateValid)
    {
        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            // do not accept worse partitions
            VerifyOrExit(mMleRouter.ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                      mParentIsSingleton, mParentLeaderData) >= 0, ;);
        }

        // looking for a better parent
        VerifyOrExit(IsBetterParent(sourceAddress.GetRloc16(), linkQuality, connectivity), ;);
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
    mChildIdRequest.mChallengeLength = (uint8_t)challenge.GetLength();

    mParent.mMacAddr.Set(aMessageInfo.GetPeerAddr());
    mParent.mValid.mRloc16 = sourceAddress.GetRloc16();
    mParent.mValid.mLinkFrameCounter = linkFrameCounter.GetFrameCounter();
    mParent.mValid.mMleFrameCounter = mleFrameCounter.GetFrameCounter();
    mParent.mMode = ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData;
    mParent.mLinkInfo.Clear();
    mParent.mLinkInfo.AddRss(GetOpenThreadContext(), threadMessageInfo->mRss);
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

    otLogInfoMle("Received Child ID Response\n");

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
            mNetif.GetActiveDataset().Set(activeTimestamp, aMessage, offset + sizeof(tlv), (uint16_t)tlv.GetLength());
        }
    }

    // Pending Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == kThreadError_None)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = kThreadError_Parse);

        // Pending Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, offset) == kThreadError_None)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            mNetif.GetPendingDataset().Set(pendingTimestamp, aMessage, offset + sizeof(tlv), (uint16_t)tlv.GetLength());
        }
    }

    // Parent Attach Success
    mParentRequestTimer.Stop();

    SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        mMesh.SetPollPeriod(Timer::SecToMsec(mTimeout / 2));
        mMesh.SetRxOnWhenIdle(false);
    }
    else
    {
        mMesh.SetRxOnWhenIdle(true);
    }

    mParent.mValid.mRloc16 = sourceAddress.GetRloc16();
    SuccessOrExit(error = SetStateChild(shortAddress.GetRloc16()));

    mNetworkData.SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0,
                                networkData.GetNetworkData(), (uint8_t)networkData.GetLength());

    mNetif.GetActiveDataset().ApplyConfiguration();

    // Route
    if ((Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == kThreadError_None) &&
        (mDeviceMode & ModeTlv::kModeFFD))
    {
        SuccessOrExit(error = mMleRouter.ProcessRouteTlv(route));

        for (int i = 0; i < kMaxRouterId; i++)
        {
            if (route.IsRouterIdSet((uint8_t)i))
            {
                numRouters++;
            }
        }

        if ((mDeviceMode & ModeTlv::kModeFFD) && (numRouters < mMleRouter.GetRouterUpgradeThreshold()))
        {
            mMleRouter.BecomeRouter(ThreadStatusTlv::kTooFewRouters);
        }
    }

exit:
    (void)aMessageInfo;
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
    uint8_t tlvs[] = {Tlv::kNetworkData};

    otLogInfoMle("Received Child Update Response\n");

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

        if (static_cast<int8_t>(leaderData.GetDataVersion() - mNetworkData.GetVersion()) > 0)
        {
            SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs));
        }

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

        if ((mode.GetMode() & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            mMesh.SetPollPeriod((uint8_t)Timer::SecToMsec(mTimeout / 2));
            mMesh.SetRxOnWhenIdle(false);
        }
        else
        {
            mMesh.SetRxOnWhenIdle(true);
        }

        break;

    default:
        assert(false);
        break;
    }

exit:
    return error;
}

ThreadError Mle::HandleDiscoveryRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Tlv tlv;
    MeshCoP::Tlv meshcopTlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    MeshCoP::ExtendedPanIdTlv extPanId;
    uint16_t offset;
    uint16_t end;

    otLogInfoMle("Received discovery request\n");

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

        offset += (uint16_t)(sizeof(tlv) + tlv.GetLength());
    }

    VerifyOrExit(offset < end, error = kThreadError_Parse);

    offset += sizeof(tlv);
    end = offset + sizeof(tlv) + (uint16_t)tlv.GetLength();

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

        offset += sizeof(meshcopTlv) + (uint16_t)meshcopTlv.GetLength();
    }

    error = SendDiscoveryResponse(aMessageInfo.GetPeerAddr(), aMessage.GetPanId());

exit:
    return error;
}

ThreadError Mle::SendDiscoveryResponse(const Ip6::Address &aDestination, uint16_t aPanId)
{
    ThreadError error = kThreadError_None;
    Message *message;
    uint16_t startOffset;
    Tlv tlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::ExtendedPanIdTlv extPanId;
    MeshCoP::NetworkNameTlv networkName;

    VerifyOrExit((message = Ip6::Udp::NewMessage(mNetif.GetOpenThreadContext(), 0)) != NULL, ;);
    message->SetLinkSecurityEnabled(false);
    message->SetMleDiscoverResponse(true);
    message->SetPanId(aPanId);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDiscoveryResponse));

    // Discovery TLV
    tlv.SetType(Tlv::kDiscovery);
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

    startOffset = message->GetLength();

    // Discovery Response TLV
    discoveryResponse.Init();
    discoveryResponse.SetVersion(kVersion);
    SuccessOrExit(error = message->Append(&discoveryResponse, sizeof(discoveryResponse)));

    // Extended PAN ID TLV
    extPanId.Init();
    extPanId.SetExtendedPanId(mMac.GetExtendedPanId());
    SuccessOrExit(error = message->Append(&extPanId, sizeof(extPanId)));

    // Network Name TLV
    networkName.Init();
    networkName.SetNetworkName(mMac.GetNetworkName());
    SuccessOrExit(error = message->Append(&networkName, sizeof(tlv) + networkName.GetLength()));

    tlv.SetLength((uint8_t)(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle("Sent discovery response\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

ThreadError Mle::HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    const ThreadMessageInfo *threadMessageInfo = reinterpret_cast<const ThreadMessageInfo *>(aMessageInfo.mLinkInfo);
    Tlv tlv;
    MeshCoP::Tlv meshcopTlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::ExtendedPanIdTlv extPanId;
    MeshCoP::NetworkNameTlv networkName;
    otActiveScanResult result;
    uint16_t offset;
    uint16_t end;
    char networkNameBuf[OT_NETWORK_NAME_SIZE];

    otLogInfoMle("Handle discovery response\n");

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

        offset += sizeof(tlv) + (uint16_t)tlv.GetLength();
    }

    VerifyOrExit(offset < end, error = kThreadError_Parse);

    offset += sizeof(tlv);
    end = offset + sizeof(tlv) + (uint16_t)tlv.GetLength();

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
            result.mExtPanId = extPanId.GetExtendedPanId();
            break;

        case MeshCoP::Tlv::kNetworkName:
            aMessage.Read(offset, sizeof(networkName), &networkName);
            VerifyOrExit(networkName.IsValid(), error = kThreadError_Parse);
            memcpy(networkNameBuf, networkName.GetNetworkName(), networkName.GetLength());
            memset(networkNameBuf + networkName.GetLength(), 0, sizeof(networkNameBuf) - networkName.GetLength());
            result.mNetworkName = networkNameBuf;
            break;

        default:
            break;
        }

        offset += sizeof(meshcopTlv) + meshcopTlv.GetLength();
    }

    // signal callback
    mDiscoverHandler(&result, mDiscoverContext);

exit:
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
    return memcmp(&mMeshLocal16, &aAddress, kRlocPrefixLength) == 0;
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

    if (mNetif.IsUnicastAddress(GetOpenThreadContext(), aIp6Header.GetDestination()))
    {
        ExitNow(error = kThreadError_None);
    }

    memcpy(&dst, GetMeshLocal16(), kRlocPrefixLength);
    dst.mFields.m16[7] = HostSwap16(aMeshSource);
    Ip6::Icmp::SendError(mNetif.GetOpenThreadContext(), dst, Ip6::IcmpHeader::kTypeDstUnreach, Ip6::IcmpHeader::kCodeDstUnreachNoRoute, aIp6Header);

exit:
    return error;
}

void Mle::HandleNetworkDataUpdate(void)
{
    if (mDeviceMode & ModeTlv::kModeFFD)
    {
        mMleRouter.HandleNetworkDataUpdateRouter();
    }

    switch (mDeviceState)
    {
    case kDeviceStateChild:
        SendChildUpdateRequest();
        break;

    default:
        break;
    }
}

}  // namespace Mle
}  // namespace Thread
