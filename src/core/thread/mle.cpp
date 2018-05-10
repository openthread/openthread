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

#include "mle.hpp"

#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "crypto/aes_ccm.hpp"
#include "mac/mac_frame.hpp"
#include "meshcop/meshcop.hpp"
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

Mle::Mle(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRetrieveNewNetworkData(false)
    , mRole(OT_DEVICE_ROLE_DISABLED)
    , mDeviceMode(ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeSecureDataRequest)
    , mAttachState(kAttachStateIdle)
    , mReattachState(kReattachStop)
    , mAttachTimer(aInstance, &Mle::HandleAttachTimer, this)
    , mDelayedResponseTimer(aInstance, &Mle::HandleDelayedResponseTimer, this)
    , mChildUpdateRequestTimer(aInstance, &Mle::HandleChildUpdateRequestTimer, this)
    , mParentLeaderCost(0)
    , mParentRequestMode(kAttachAny)
    , mParentPriority(0)
    , mParentLinkQuality3(0)
    , mParentLinkQuality2(0)
    , mParentLinkQuality1(0)
    , mChildUpdateAttempts(0)
    , mParentLinkMargin(0)
    , mParentIsSingleton(false)
    , mReceivedResponseFromParent(false)
    , mSocket(aInstance.GetThreadNetif().GetIp6().GetUdp())
    , mTimeout(kMleEndDeviceTimeout)
    , mSendChildUpdateRequest(aInstance, &Mle::HandleSendChildUpdateRequest, this)
    , mDiscoverHandler(NULL)
    , mDiscoverContext(NULL)
    , mIsDiscoverInProgress(false)
    , mEnableEui64Filtering(false)
#if OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
    , mPreviousParentRloc(Mac::kShortAddrInvalid)
#endif
#if OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
    , mParentSearchIsInBackoff(false)
    , mParentSearchBackoffWasCanceled(false)
    , mParentSearchRecentlyDetached(false)
    , mParentSearchBackoffCancelTime(0)
    , mParentSearchTimer(aInstance, &Mle::HandleParentSearchTimer, this)
#endif
    , mAnnounceChannel(OT_RADIO_CHANNEL_MIN)
    , mPreviousChannel(0)
    , mPreviousPanId(Mac::kPanIdBroadcast)
    , mNotifierCallback(&Mle::HandleStateChanged, this)
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
    mLinkLocal64.GetAddress().SetIid(GetNetif().GetMac().GetExtAddress());
    mLinkLocal64.mPrefixLength = 64;
    mLinkLocal64.mPreferred    = true;
    mLinkLocal64.mValid        = true;
    GetNetif().AddUnicastAddress(mLinkLocal64);

    // Leader Aloc
    mLeaderAloc.mPrefixLength       = 128;
    mLeaderAloc.mPreferred          = true;
    mLeaderAloc.mValid              = true;
    mLeaderAloc.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mLeaderAloc.mScopeOverrideValid = true;

#if OPENTHREAD_ENABLE_SERVICE

    // Service Alocs
    for (size_t i = 0; i < sizeof(mServiceAlocs) / sizeof(mServiceAlocs[0]); i++)
    {
        memset(&mServiceAlocs[i], 0, sizeof(mServiceAlocs[i]));

        mServiceAlocs[i].mPrefixLength               = 128;
        mServiceAlocs[i].mPreferred                  = true;
        mServiceAlocs[i].mValid                      = true;
        mServiceAlocs[i].mScopeOverride              = Ip6::Address::kRealmLocalScope;
        mServiceAlocs[i].mScopeOverrideValid         = true;
        mServiceAlocs[i].GetAddress().mFields.m16[7] = HostSwap16(Mac::kShortAddrInvalid);
    }

#endif

    // initialize Mesh Local Prefix
    meshLocalPrefix[0] = 0xfd;
    memcpy(meshLocalPrefix + 1, GetNetif().GetMac().GetExtendedPanId(), 5);
    meshLocalPrefix[6] = 0x00;
    meshLocalPrefix[7] = 0x00;

    // mesh-local 64
    Random::FillBuffer(mMeshLocal64.GetAddress().mFields.m8 + OT_IP6_PREFIX_SIZE,
                       OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);

    mMeshLocal64.mPrefixLength       = 64;
    mMeshLocal64.mPreferred          = true;
    mMeshLocal64.mValid              = true;
    mMeshLocal64.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mMeshLocal64.mScopeOverrideValid = true;

    // mesh-local 16
    mMeshLocal16.GetAddress().mFields.m16[4] = HostSwap16(0x0000);
    mMeshLocal16.GetAddress().mFields.m16[5] = HostSwap16(0x00ff);
    mMeshLocal16.GetAddress().mFields.m16[6] = HostSwap16(0xfe00);
    mMeshLocal16.mPrefixLength               = 64;
    mMeshLocal16.mPreferred                  = true;
    mMeshLocal16.mValid                      = true;
    mMeshLocal16.mScopeOverride              = Ip6::Address::kRealmLocalScope;
    mMeshLocal16.mScopeOverrideValid         = true;
    mMeshLocal16.mRloc                       = true;

    // Store RLOC address reference in MPL module.
    GetNetif().GetIp6().GetMpl().SetMatchingAddress(mMeshLocal16.GetAddress());

    // link-local all thread nodes
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff32);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[6] = HostSwap16(0x0000);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);

    // realm-local all thread nodes
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff33);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[6] = HostSwap16(0x0000);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);

    SetMeshLocalPrefix(meshLocalPrefix);

    // `SetMeshLocalPrefix()` also adds the Mesh-Local EID and subscribes
    // to the Link- and Realm-Local All Thread Nodes multicast addresses.

    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);

#if OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
    StartParentSearchTimer();
#endif
}

otError Mle::Enable(void)
{
    otError       error = OT_ERROR_NONE;
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
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

    // cannot bring up the interface if IEEE 802.15.4 promiscuous mode is enabled
    VerifyOrExit(otPlatRadioGetPromiscuous(&netif.GetInstance()) == false, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(netif.IsUp(), error = OT_ERROR_INVALID_STATE);

    SetStateDetached();

    netif.GetKeyManager().Start();

    if (aEnableReattach)
    {
        mReattachState = kReattachStart;
    }

    if (aAnnounceAttach || (GetRloc16() == Mac::kShortAddrInvalid))
    {
        BecomeChild(kAttachAny);
    }
    else if (IsActiveRouter(GetRloc16()))
    {
        if (netif.GetMle().BecomeRouter(ThreadStatusTlv::kTooFewRouters) != OT_ERROR_NONE)
        {
            BecomeChild(kAttachAny);
        }
    }
    else
    {
        mAttachState         = kAttachStateSynchronize;
        mChildUpdateAttempts = 0;
        SendChildUpdateRequest();
    }

exit:
    return error;
}

otError Mle::Stop(bool aClearNetworkDatasets)
{
    ThreadNetif &netif = GetNetif();

    if (aClearNetworkDatasets)
    {
        netif.GetActiveDataset().HandleDetach();
        netif.GetPendingDataset().HandleDetach();
    }

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED);

    netif.GetKeyManager().Stop();
    SetStateDetached();
    netif.RemoveUnicastAddress(mMeshLocal16);

    SetRole(OT_DEVICE_ROLE_DISABLED);

exit:
    return OT_ERROR_NONE;
}

void Mle::SetRole(otDeviceRole aRole)
{
    VerifyOrExit(aRole != mRole);

    otLogInfoMle(GetInstance(), "Role %s -> %s", RoleToString(mRole), RoleToString(aRole));

    mRole = aRole;
    GetNotifier().SetFlags(OT_CHANGED_THREAD_ROLE);

exit:
    return;
}

otError Mle::Restore(void)
{
    ThreadNetif &         netif    = GetNetif();
    Settings &            settings = GetInstance().GetSettings();
    otError               error    = OT_ERROR_NONE;
    Settings::NetworkInfo networkInfo;
    Settings::ParentInfo  parentInfo;

    netif.GetActiveDataset().Restore();
    netif.GetPendingDataset().Restore();

    SuccessOrExit(error = settings.ReadNetworkInfo(networkInfo));

    netif.GetKeyManager().SetCurrentKeySequence(networkInfo.mKeySequence);
    netif.GetKeyManager().SetMleFrameCounter(networkInfo.mMleFrameCounter);
    netif.GetKeyManager().SetMacFrameCounter(networkInfo.mMacFrameCounter);

    VerifyOrExit(networkInfo.mRole >= OT_DEVICE_ROLE_CHILD);

    mDeviceMode = networkInfo.mDeviceMode;
    SetRloc16(networkInfo.mRloc16);
    netif.GetMac().SetExtAddress(networkInfo.mExtAddress);
    UpdateLinkLocalAddress();

    memcpy(&mMeshLocal64.GetAddress().mFields.m8[OT_IP6_PREFIX_SIZE], networkInfo.mMlIid,
           OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);

    if (networkInfo.mRloc16 == Mac::kShortAddrInvalid)
    {
        ExitNow();
    }

    if (!IsActiveRouter(networkInfo.mRloc16))
    {
        error = settings.ReadParentInfo(parentInfo);

        if (error != OT_ERROR_NONE)
        {
            // If the restored RLOC16 corresponds to an end-device, it
            // is expected that the `ParentInfo` settings to be valid
            // as well. The device can still recover from such an invalid
            // setting by skipping the re-attach ("Child Update Request"
            // exchange) and going through the full attach process.

            otLogWarnMle(GetInstance(), "Invalid settings - no saved parent info with valid end-device RLOC16 0x%04x",
                         networkInfo.mRloc16);
            ExitNow();
        }

        memset(&mParent, 0, sizeof(mParent));
        mParent.SetExtAddress(*static_cast<Mac::ExtAddress *>(&parentInfo.mExtAddress));
        mParent.SetDeviceMode(ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData |
                              ModeTlv::kModeSecureDataRequest);
        mParent.SetRloc16(GetRloc16(GetRouterId(networkInfo.mRloc16)));
        mParent.SetState(Neighbor::kStateRestored);

#if OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
        mPreviousParentRloc = mParent.GetRloc16();
#endif
    }
    else
    {
        netif.GetMle().SetRouterId(GetRouterId(GetRloc16()));
        netif.GetMle().SetPreviousPartitionId(networkInfo.mPreviousPartitionId);
        netif.GetMle().RestoreChildren();
    }

exit:
    return error;
}

otError Mle::Store(void)
{
    ThreadNetif &         netif    = GetNetif();
    Settings &            settings = GetInstance().GetSettings();
    otError               error    = OT_ERROR_NONE;
    Settings::NetworkInfo networkInfo;

    memset(&networkInfo, 0, sizeof(networkInfo));

    if (IsAttached())
    {
        // only update network information while we are attached to avoid losing information when a reboot occurs after
        // a message is sent but before attaching
        networkInfo.mDeviceMode          = mDeviceMode;
        networkInfo.mRole                = mRole;
        networkInfo.mRloc16              = GetRloc16();
        networkInfo.mPreviousPartitionId = mLeaderData.GetPartitionId();
        networkInfo.mExtAddress          = netif.GetMac().GetExtAddress();
        memcpy(networkInfo.mMlIid, &mMeshLocal64.GetAddress().mFields.m8[OT_IP6_PREFIX_SIZE], OT_IP6_IID_SIZE);

        if (mRole == OT_DEVICE_ROLE_CHILD)
        {
            Settings::ParentInfo parentInfo;

            memset(&parentInfo, 0, sizeof(parentInfo));
            parentInfo.mExtAddress = mParent.GetExtAddress();

            SuccessOrExit(error = settings.SaveParentInfo(parentInfo));
        }
    }
    else
    {
        // when not attached, read out any existing values so that we do not change them
        IgnoreReturnValue(settings.ReadNetworkInfo(networkInfo));
    }

    // update MAC and MLE Frame Counters even when we are not attached MLE messages are sent before a device attached
    networkInfo.mKeySequence = netif.GetKeyManager().GetCurrentKeySequence();
    networkInfo.mMleFrameCounter =
        netif.GetKeyManager().GetMleFrameCounter() + OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;
    networkInfo.mMacFrameCounter =
        netif.GetKeyManager().GetMacFrameCounter() + OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD;

    SuccessOrExit(error = settings.SaveNetworkInfo(networkInfo));

    netif.GetKeyManager().SetStoredMleFrameCounter(networkInfo.mMleFrameCounter);
    netif.GetKeyManager().SetStoredMacFrameCounter(networkInfo.mMacFrameCounter);

    otLogDebgMle(GetInstance(), "Store Network Information");

exit:
    return error;
}

otError Mle::Discover(uint32_t        aScanChannels,
                      uint16_t        aPanId,
                      bool            aJoiner,
                      bool            aEnableEui64Filtering,
                      DiscoverHandler aCallback,
                      void *          aContext)
{
    otError                      error   = OT_ERROR_NONE;
    Message *                    message = NULL;
    Ip6::Address                 destination;
    Tlv                          tlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    uint16_t                     startOffset;

    VerifyOrExit(!mIsDiscoverInProgress, error = OT_ERROR_BUSY);

    mEnableEui64Filtering = aEnableEui64Filtering;

    mDiscoverHandler = aCallback;
    mDiscoverContext = aContext;
    GetNetif().GetMeshForwarder().SetDiscoverParameters(aScanChannels);

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

    LogMleMessage("Send Discovery Request", destination);

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
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    // not in reattach stage after reset
    if (mReattachState == kReattachStop)
    {
        netif.GetPendingDataset().HandleDetach();
    }

#if OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
    mParentSearchRecentlyDetached = true;
#endif

    SetStateDetached();
    mParent.SetState(Neighbor::kStateInvalid);
    SetRloc16(Mac::kShortAddrInvalid);
    BecomeChild(kAttachAny);

exit:
    return error;
}

otError Mle::BecomeChild(AttachMode aMode)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mAttachState == kAttachStateIdle, error = OT_ERROR_BUSY);

    otLogInfoMle(GetInstance(), "Attempt to attach");

    if (mReattachState == kReattachStart)
    {
        if (netif.GetActiveDataset().Restore() == OT_ERROR_NONE)
        {
            mReattachState = kReattachActive;
        }
        else
        {
            mReattachState = kReattachStop;
        }
    }

    ResetParentCandidate();
    mAttachState       = kAttachStateStart;
    mParentRequestMode = aMode;

    if (aMode != kAttachBetter)
    {
        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            netif.GetMle().StopAdvertiseTimer();
        }
    }

    netif.GetMeshForwarder().SetRxOnWhenIdle(true);

    mAttachTimer.Start(1 + Random::GetUint32InRange(0, kParentRequestRouterTimeout));

exit:
    return error;
}

bool Mle::IsAttached(void) const
{
    return (mRole == OT_DEVICE_ROLE_CHILD || mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER);
}

otError Mle::SetStateDetached(void)
{
    ThreadNetif &netif = GetNetif();

    if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        netif.RemoveUnicastAddress(mLeaderAloc);
    }

    SetRole(OT_DEVICE_ROLE_DETACHED);
    mAttachState = kAttachStateIdle;
    mAttachTimer.Stop();
    mChildUpdateRequestTimer.Stop();
    netif.GetMeshForwarder().SetRxOnWhenIdle(true);
    netif.GetMac().SetBeaconEnabled(false);
    netif.GetMle().HandleDetachStart();
    netif.GetIp6().SetForwardingEnabled(false);
    netif.GetIp6().GetMpl().SetTimerExpirations(0);

    return OT_ERROR_NONE;
}

otError Mle::SetStateChild(uint16_t aRloc16)
{
    ThreadNetif &netif = GetNetif();

    if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        netif.RemoveUnicastAddress(mLeaderAloc);
    }

    SetRloc16(aRloc16);
    SetRole(OT_DEVICE_ROLE_CHILD);
    mAttachState         = kAttachStateIdle;
    mReattachState       = kReattachStop;
    mChildUpdateAttempts = 0;
    netif.GetMac().SetBeaconEnabled(false);

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0)
    {
        mChildUpdateRequestTimer.Start(TimerMilli::SecToMsec(mTimeout) -
                                       static_cast<uint32_t>(kUnicastRetransmissionDelay) * kMaxChildKeepAliveAttempts);
    }

    if ((mDeviceMode & ModeTlv::kModeFFD) != 0)
    {
        netif.GetMle().HandleChildStart(mParentRequestMode);
    }

#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
    netif.GetNetworkDataLocal().ClearResubmitDelayTimer();
#endif
    netif.GetIp6().SetForwardingEnabled(false);
    netif.GetIp6().GetMpl().SetTimerExpirations(kMplChildDataMessageTimerExpirations);

    // send announce after attached if needed
    InformPreviousChannel();

#if OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
    UpdateParentSearchState();
#endif

#if OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
    InformPreviousParent();
    mPreviousParentRloc = mParent.GetRloc16();
#endif

    return OT_ERROR_NONE;
}

void Mle::InformPreviousChannel(void)
{
    VerifyOrExit(mPreviousPanId != Mac::kPanIdBroadcast);
    VerifyOrExit(mRole == OT_DEVICE_ROLE_CHILD || mRole == OT_DEVICE_ROLE_ROUTER);

    if ((mDeviceMode & ModeTlv::kModeFFD) == 0 || mRole == OT_DEVICE_ROLE_ROUTER ||
        GetNetif().GetMle().GetRouterSelectionJitterTimeout() == 0)
    {
        mPreviousPanId = Mac::kPanIdBroadcast;
        GetNetif().GetAnnounceBeginServer().SendAnnounce(1 << mPreviousChannel);
    }

exit:
    return;
}

otError Mle::SetTimeout(uint32_t aTimeout)
{
    VerifyOrExit(mTimeout != aTimeout);

    if (aTimeout < kMinTimeout)
    {
        aTimeout = kMinTimeout;
    }

    mTimeout = aTimeout;

    GetNetif().GetMeshForwarder().GetDataPollManager().RecalculatePollPeriod();

    if (mRole == OT_DEVICE_ROLE_CHILD)
    {
        SendChildUpdateRequest();
    }

exit:
    return OT_ERROR_NONE;
}

otError Mle::SetDeviceMode(uint8_t aDeviceMode)
{
    otError error   = OT_ERROR_NONE;
    uint8_t oldMode = mDeviceMode;

    VerifyOrExit((aDeviceMode & ModeTlv::kModeFFD) == 0 || (aDeviceMode & ModeTlv::kModeRxOnWhenIdle) != 0,
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mDeviceMode != aDeviceMode);

    mDeviceMode = aDeviceMode;

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        break;

    case OT_DEVICE_ROLE_CHILD:
        SetStateChild(GetRloc16());
        SendChildUpdateRequest();
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if ((oldMode & ModeTlv::kModeFFD) != 0 && (aDeviceMode & ModeTlv::kModeFFD) == 0)
        {
            BecomeDetached();
        }

        break;
    }

exit:
    return error;
}

const Ip6::Address &Mle::GetLinkLocalAddress(void) const
{
    return mLinkLocal64.GetAddress();
}

otError Mle::UpdateLinkLocalAddress(void)
{
    ThreadNetif &netif = GetNetif();

    netif.RemoveUnicastAddress(mLinkLocal64);
    mLinkLocal64.GetAddress().SetIid(netif.GetMac().GetExtAddress());
    netif.AddUnicastAddress(mLinkLocal64);

    GetNotifier().SetFlags(OT_CHANGED_THREAD_LL_ADDR);

    return OT_ERROR_NONE;
}

const uint8_t *Mle::GetMeshLocalPrefix(void) const
{
    return mMeshLocal16.GetAddress().mFields.m8;
}

otError Mle::SetMeshLocalPrefix(const uint8_t *aMeshLocalPrefix)
{
    ThreadNetif &netif = GetNetif();

    if (memcmp(mMeshLocal64.GetAddress().mFields.m8, aMeshLocalPrefix, 8) == 0)
    {
        ExitNow();
    }

    // We must remove the old addresses before adding the new ones.
    netif.RemoveUnicastAddress(mMeshLocal64);
    netif.RemoveUnicastAddress(mMeshLocal16);
    netif.UnsubscribeMulticast(mLinkLocalAllThreadNodes);
    netif.UnsubscribeMulticast(mRealmLocalAllThreadNodes);

    memcpy(mMeshLocal64.GetAddress().mFields.m8, aMeshLocalPrefix, 8);
    memcpy(mMeshLocal16.GetAddress().mFields.m8, mMeshLocal64.GetAddress().mFields.m8, 8);

#if OPENTHREAD_ENABLE_SERVICE

    for (uint8_t i = 0; i < sizeof(mServiceAlocs) / sizeof(mServiceAlocs[0]); i++)
    {
        if (HostSwap16(mServiceAlocs[i].GetAddress().mFields.m16[7]) != Mac::kShortAddrInvalid)
        {
            netif.RemoveUnicastAddress(mServiceAlocs[i]);
            memcpy(mServiceAlocs[i].GetAddress().mFields.m8, mMeshLocal64.GetAddress().mFields.m8, 8);
            netif.AddUnicastAddress(mServiceAlocs[i]);
        }
    }

#endif

    mLinkLocalAllThreadNodes.GetAddress().mFields.m8[3] = 64;
    memcpy(mLinkLocalAllThreadNodes.GetAddress().mFields.m8 + 4, mMeshLocal64.GetAddress().mFields.m8, 8);

    mRealmLocalAllThreadNodes.GetAddress().mFields.m8[3] = 64;
    memcpy(mRealmLocalAllThreadNodes.GetAddress().mFields.m8 + 4, mMeshLocal64.GetAddress().mFields.m8, 8);

    // Add the addresses back into the table.
    netif.AddUnicastAddress(mMeshLocal64);
    netif.SubscribeMulticast(mLinkLocalAllThreadNodes);
    netif.SubscribeMulticast(mRealmLocalAllThreadNodes);

    if (mRole >= OT_DEVICE_ROLE_CHILD)
    {
        netif.AddUnicastAddress(mMeshLocal16);
    }

    // update Leader ALOC
    if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        netif.RemoveUnicastAddress(mLeaderAloc);
        memcpy(mLeaderAloc.GetAddress().mFields.m8, mMeshLocal64.GetAddress().mFields.m8, 8);
        netif.AddUnicastAddress(mLeaderAloc);
    }

    // Changing the prefix also causes the mesh local address to be different.
    GetNotifier().SetFlags(OT_CHANGED_THREAD_ML_ADDR);

exit:
    return OT_ERROR_NONE;
}

const Ip6::Address &Mle::GetLinkLocalAllThreadNodesAddress(void) const
{
    return mLinkLocalAllThreadNodes.GetAddress();
}

const Ip6::Address &Mle::GetRealmLocalAllThreadNodesAddress(void) const
{
    return mRealmLocalAllThreadNodes.GetAddress();
}

uint16_t Mle::GetRloc16(void) const
{
    return GetNetif().GetMac().GetShortAddress();
}

otError Mle::SetRloc16(uint16_t aRloc16)
{
    ThreadNetif &netif = GetNetif();

    netif.RemoveUnicastAddress(mMeshLocal16);

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        // mesh-local 16
        mMeshLocal16.GetAddress().mFields.m16[7] = HostSwap16(aRloc16);
        netif.AddUnicastAddress(mMeshLocal16);
    }

    netif.GetMac().SetShortAddress(aRloc16);
    netif.GetIp6().GetMpl().SetSeedId(aRloc16);

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
        GetNetif().GetMle().HandlePartitionChange();
        GetNotifier().SetFlags(OT_CHANGED_THREAD_PARTITION_ID);
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

#if OPENTHREAD_ENABLE_SERVICE
otError Mle::GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 8);
    aAddress.mFields.m16[4] = HostSwap16(0x0000);
    aAddress.mFields.m16[5] = HostSwap16(0x00ff);
    aAddress.mFields.m16[6] = HostSwap16(0xfe00);
    aAddress.mFields.m16[7] = HostSwap16(GetServiceAlocFromId(aServiceId));

exit:
    return error;
}
#endif

otError Mle::AddLeaderAloc(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = GetLeaderAloc(mLeaderAloc.GetAddress()));

    error = GetNetif().AddUnicastAddress(mLeaderAloc);

exit:
    return error;
}

const LeaderDataTlv &Mle::GetLeaderDataTlv(void)
{
    ThreadNetif &netif = GetNetif();

    mLeaderData.SetDataVersion(netif.GetNetworkDataLeader().GetVersion());
    mLeaderData.SetStableDataVersion(netif.GetNetworkDataLeader().GetStableVersion());
    return mLeaderData;
}

otError Mle::GetLeaderData(otLeaderData &aLeaderData)
{
    const LeaderDataTlv &leaderData(GetLeaderDataTlv());
    otError              error = OT_ERROR_NONE;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED && mRole != OT_DEVICE_ROLE_DETACHED, error = OT_ERROR_DETACHED);

    aLeaderData.mPartitionId       = leaderData.GetPartitionId();
    aLeaderData.mWeighting         = leaderData.GetWeighting();
    aLeaderData.mDataVersion       = leaderData.GetDataVersion();
    aLeaderData.mStableDataVersion = leaderData.GetStableDataVersion();
    aLeaderData.mLeaderRouterId    = leaderData.GetLeaderRouterId();

exit:
    return error;
}

void Mle::GenerateNonce(const Mac::ExtAddress &aMacAddr,
                        uint32_t               aFrameCounter,
                        uint8_t                aSecurityLevel,
                        uint8_t *              aNonce)
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
    Header  header;

    header.Init();

    if (aCommand == Header::kCommandDiscoveryRequest || aCommand == Header::kCommandDiscoveryResponse)
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
    Tlv     tlv;

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
    Tlv     tlv;

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
    tlv.SetFrameCounter(GetNetif().GetKeyManager().GetMacFrameCounter());

    return aMessage.Append(&tlv, sizeof(tlv));
}

otError Mle::AppendMleFrameCounter(Message &aMessage)
{
    MleFrameCounterTlv tlv;

    tlv.Init();
    tlv.SetFrameCounter(GetNetif().GetKeyManager().GetMleFrameCounter());

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
    mLeaderData.SetDataVersion(GetNetif().GetNetworkDataLeader().GetVersion());
    mLeaderData.SetStableDataVersion(GetNetif().GetNetworkDataLeader().GetStableVersion());

    return aMessage.Append(&mLeaderData, sizeof(mLeaderData));
}

void Mle::FillNetworkDataTlv(NetworkDataTlv &aTlv, bool aStableOnly)
{
    uint8_t length = sizeof(NetworkDataTlv) - sizeof(Tlv); // sizeof( NetworkDataTlv::mNetworkData )

    // Ignore result code, provided buffer must be enough
    GetNetif().GetNetworkDataLeader().GetNetworkData(aStableOnly, aTlv.GetNetworkData(), length);
    aTlv.SetLength(length);
}

otError Mle::AppendNetworkData(Message &aMessage, bool aStableOnly)
{
    otError        error = OT_ERROR_NONE;
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
    Tlv     tlv;

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
    ThreadNetif &            netif = GetNetif();
    otError                  error = OT_ERROR_NONE;
    Tlv                      tlv;
    AddressRegistrationEntry entry;
    Lowpan::Context          context;
    uint8_t                  length      = 0;
    uint8_t                  counter     = 0;
    uint16_t                 startOffset = aMessage.GetLength();

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    // write entries to message
    for (const Ip6::NetifUnicastAddress *addr = netif.GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        if (addr->GetAddress().IsLinkLocal() || IsRoutingLocator(addr->GetAddress()) ||
            IsAnycastLocator(addr->GetAddress()))
        {
            continue;
        }

        if (netif.GetNetworkDataLeader().GetContext(addr->GetAddress(), context) == OT_ERROR_NONE)
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
        counter++;
        // only continue to append if there is available entry.
        VerifyOrExit(counter < OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER);
    }

    // For sleepy end device, register external multicast addresses to the parent for indirect transmission
    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        uint8_t      iterator = 0;
        Ip6::Address address;

        // append external multicast address
        while (netif.GetNextExternalMulticast(iterator, address) == OT_ERROR_NONE)
        {
            entry.SetUncompressed();
            entry.SetIp6Address(address);
            SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
            length += entry.GetLength();

            counter++;
            // only continue to append if there is available entry.
            VerifyOrExit(counter < OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER);
        }
    }

exit:

    if (error == OT_ERROR_NONE && length > 0)
    {
        tlv.SetLength(length);
        aMessage.Write(startOffset, sizeof(tlv), &tlv);
    }

    return error;
}

otError Mle::AppendActiveTimestamp(Message &aMessage)
{
    ThreadNetif &             netif = GetNetif();
    otError                   error;
    ActiveTimestampTlv        timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    timestamp = netif.GetActiveDataset().GetTimestamp();
    VerifyOrExit(timestamp, error = OT_ERROR_NONE);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = *timestamp;
    error                                             = aMessage.Append(&timestampTlv, sizeof(timestampTlv));

exit:
    return error;
}

otError Mle::AppendPendingTimestamp(Message &aMessage)
{
    otError                   error;
    PendingTimestampTlv       timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    timestamp = GetNetif().GetPendingDataset().GetTimestamp();
    VerifyOrExit(timestamp && timestamp->GetSeconds() != 0, error = OT_ERROR_NONE);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = *timestamp;
    error                                             = aMessage.Append(&timestampTlv, sizeof(timestampTlv));

exit:
    return error;
}

void Mle::HandleStateChanged(Notifier::Callback &aCallback, uint32_t aFlags)
{
    aCallback.GetOwner<Mle>().HandleStateChanged(aFlags);
}

void Mle::HandleStateChanged(uint32_t aFlags)
{
    ThreadNetif &netif = GetNetif();
    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED);

    if ((aFlags & (OT_CHANGED_IP6_ADDRESS_ADDED | OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0)
    {
        if (!netif.IsUnicastAddress(mMeshLocal64.GetAddress()))
        {
            // Mesh Local EID was removed, choose a new one and add it back
            Random::FillBuffer(mMeshLocal64.GetAddress().mFields.m8 + OT_IP6_PREFIX_SIZE,
                               OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);

            netif.AddUnicastAddress(mMeshLocal64);
            GetNotifier().SetFlags(OT_CHANGED_THREAD_ML_ADDR);
        }

        if (mRole == OT_DEVICE_ROLE_CHILD && (mDeviceMode & ModeTlv::kModeFFD) == 0)
        {
            mSendChildUpdateRequest.Post();
        }
    }

    if ((aFlags & (OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED | OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED)) != 0)
    {
        if (mRole == OT_DEVICE_ROLE_CHILD && (mDeviceMode & ModeTlv::kModeFFD) == 0 &&
            (mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            mSendChildUpdateRequest.Post();
        }
    }

    if ((aFlags & OT_CHANGED_THREAD_NETDATA) != 0)
    {
        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            netif.GetMle().HandleNetworkDataUpdateRouter();
        }
        else if ((aFlags & OT_CHANGED_THREAD_ROLE) == 0)
        {
            mSendChildUpdateRequest.Post();
        }

#if OPENTHREAD_ENABLE_BORDER_ROUTER || OPENTHREAD_ENABLE_SERVICE
        netif.GetNetworkDataLocal().SendServerDataNotification();
#if OPENTHREAD_ENABLE_SERVICE
        this->UpdateServiceAlocs();
#endif
#endif
    }

    if (aFlags & (OT_CHANGED_THREAD_ROLE | OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER))
    {
        Store();
    }

exit:
    return;
}

#if OPENTHREAD_ENABLE_SERVICE
void Mle::UpdateServiceAlocs(void)
{
    ThreadNetif &         netif              = GetNetif();
    uint16_t              rloc               = GetRloc16();
    uint16_t              serviceAloc        = 0;
    uint8_t               serviceId          = 0;
    int                   i                  = 0;
    NetworkData::Leader & leaderData         = netif.GetNetworkDataLeader();
    otNetworkDataIterator serviceIterator    = OT_NETWORK_DATA_ITERATOR_INIT;
    int                   serviceAlocsLength = sizeof(mServiceAlocs) / sizeof(mServiceAlocs[0]);

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED);

    // First remove all alocs which are no longer necessary, to free up space in mServiceAlocs
    for (i = 0; i < serviceAlocsLength; i++)
    {
        serviceAloc = HostSwap16(mServiceAlocs[i].GetAddress().mFields.m16[7]);

        if ((serviceAloc != Mac::kShortAddrInvalid) &&
            (!leaderData.ContainsService(Mle::GetServiceIdFromAloc(serviceAloc), rloc)))
        {
            netif.RemoveUnicastAddress(mServiceAlocs[i]);
            mServiceAlocs[i].GetAddress().mFields.m16[7] = HostSwap16(Mac::kShortAddrInvalid);
        }
    }

    // Now add any missing service alocs which should be there, if there is enough space in mServiceAlocs
    while (leaderData.GetNextServiceId(&serviceIterator, rloc, &serviceId) == OT_ERROR_NONE)
    {
        for (i = 0; i < serviceAlocsLength; i++)
        {
            serviceAloc = HostSwap16(mServiceAlocs[i].GetAddress().mFields.m16[7]);

            if ((serviceAloc != Mac::kShortAddrInvalid) && (Mle::GetServiceIdFromAloc(serviceAloc) == serviceId))
            {
                break;
            }
        }

        if (i >= serviceAlocsLength)
        {
            // Service Aloc is not there, but it should be. Lets add it into first empty space
            for (i = 0; i < serviceAlocsLength; i++)
            {
                serviceAloc = HostSwap16(mServiceAlocs[i].GetAddress().mFields.m16[7]);

                if (serviceAloc == Mac::kShortAddrInvalid)
                {
                    SuccessOrExit(GetServiceAloc(serviceId, mServiceAlocs[i].GetAddress()));
                    netif.AddUnicastAddress(mServiceAlocs[i]);
                    break;
                }
            }
        }
    }

exit:
    return;
}
#endif

void Mle::HandleAttachTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mle>().HandleAttachTimer();
}

void Mle::HandleAttachTimer(void)
{
    uint32_t delay = 0;

    switch (mAttachState)
    {
    case kAttachStateIdle:
        assert(false);
        break;

    case kAttachStateSynchronize:
        SendChildUpdateRequest();
        break;

    case kAttachStateStart:
        mAttachState = kAttachStateParentRequestRouter;
        mParentCandidate.SetState(Neighbor::kStateInvalid);
        mReceivedResponseFromParent = false;

        if (mParentRequestMode == kAttachSame1 || mParentRequestMode == kAttachSame2)
        {
            SendParentRequest(kParentRequestTypeRoutersAndReeds);
            delay = kParentRequestReedTimeout;
        }
        else
        {
            SendParentRequest(kParentRequestTypeRouters);
            delay = kParentRequestRouterTimeout;
        }

        break;

    case kAttachStateParentRequestRouter:
        mAttachState = kAttachStateParentRequestReed;

        if (mParentCandidate.GetState() != Neighbor::kStateParentResponse)
        {
            SendParentRequest(kParentRequestTypeRoutersAndReeds);
            delay = kParentRequestReedTimeout;
            break;
        }

        // fall through

    case kAttachStateParentRequestReed:
        if (mParentCandidate.GetState() == Neighbor::kStateParentResponse &&
            (mRole != OT_DEVICE_ROLE_CHILD || mReceivedResponseFromParent || mParentRequestMode == kAttachBetter) &&
            SendChildIdRequest() == OT_ERROR_NONE)
        {
            mAttachState = kAttachStateChildIdRequest;
            delay        = kParentRequestReedTimeout;
            break;
        }

        // fall through

    case kAttachStateChildIdRequest:
        mAttachState = kAttachStateIdle;
        ResetParentCandidate();
        delay = Reattach();
        break;
    }

    if (delay != 0)
    {
        mAttachTimer.Start(delay);
    }
}

uint32_t Mle::Reattach(void)
{
    ThreadNetif &netif = GetNetif();
    uint32_t     delay = 0;

    if (mReattachState == kReattachActive)
    {
        if (netif.GetPendingDataset().Restore() == OT_ERROR_NONE)
        {
            netif.GetPendingDataset().ApplyConfiguration();
            mReattachState = kReattachPending;
            mAttachState   = kAttachStateStart;
            delay          = kParentRequestRouterTimeout;
        }
        else
        {
            mReattachState = kReattachStop;
        }
    }
    else if (mReattachState == kReattachPending)
    {
        mReattachState = kReattachStop;
        netif.GetActiveDataset().Restore();
    }

    VerifyOrExit(mReattachState == kReattachStop);

    switch (mParentRequestMode)
    {
    case kAttachAny:
        if (mRole != OT_DEVICE_ROLE_CHILD)
        {
            if (mPreviousPanId != Mac::kPanIdBroadcast)
            {
                netif.GetMac().SetPanChannel(mPreviousChannel);
                netif.GetMac().SetPanId(mPreviousPanId);
                mPreviousPanId = Mac::kPanIdBroadcast;
                BecomeDetached();
            }
            else if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
            {
                SendOrphanAnnounce();
                BecomeDetached();
            }
            else if (netif.GetMle().BecomeLeader() != OT_ERROR_NONE)
            {
                BecomeDetached();
            }
        }
        else if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            // return to sleepy operation
            netif.GetMeshForwarder().GetDataPollManager().SetAttachMode(false);
            netif.GetMeshForwarder().SetRxOnWhenIdle(false);
        }

        break;

    case kAttachSame1:
        BecomeChild(kAttachSame2);
        break;

    case kAttachSame2:
        BecomeChild(kAttachAny);
        break;

    case kAttachBetter:
        break;
    }

exit:
    return delay;
}

void Mle::HandleDelayedResponseTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mle>().HandleDelayedResponseTimer();
}

void Mle::HandleDelayedResponseTimer(void)
{
    DelayedResponseHeader delayedResponse;
    uint32_t              now         = TimerMilli::GetNow();
    uint32_t              nextDelay   = 0xffffffff;
    Message *             message     = mDelayedResponses.GetHead();
    Message *             nextMessage = NULL;

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
                LogMleMessage("Send delayed message", delayedResponse.GetDestination());
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

void Mle::RemoveDelayedDataResponseMessage(void)
{
    Message *             message = mDelayedResponses.GetHead();
    DelayedResponseHeader delayedResponse;

    while (message != NULL)
    {
        delayedResponse.ReadFrom(*message);

        if (message->GetSubType() == Message::kSubTypeMleDataResponse)
        {
            mDelayedResponses.Dequeue(*message);
            message->Free();
            LogMleMessage("Remove Delayed Data Response", delayedResponse.GetDestination());

            // no more than one multicast MLE Data Response in Delayed Message Queue.
            break;
        }

        message = message->GetNext();
    }
}

otError Mle::SendParentRequest(ParentRequestType aType)
{
    otError      error = OT_ERROR_NONE;
    Message *    message;
    uint8_t      scanMask = 0;
    Ip6::Address destination;

    Random::FillBuffer(mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge));

    switch (aType)
    {
    case kParentRequestTypeRouters:
        scanMask = ScanMaskTlv::kRouterFlag;
        break;

    case kParentRequestTypeRoutersAndReeds:
        scanMask = ScanMaskTlv::kRouterFlag | ScanMaskTlv::kEndDeviceFlag;
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

    switch (aType)
    {
    case kParentRequestTypeRouters:
        LogMleMessage("Send Parent Request to routers", destination);
        break;

    case kParentRequestTypeRoutersAndReeds:
        LogMleMessage("Send Parent Request to routers and REEDs", destination);
        break;
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Mle::SendChildIdRequest(void)
{
    otError      error   = OT_ERROR_NONE;
    uint8_t      tlvs[]  = {Tlv::kAddress16, Tlv::kNetworkData, Tlv::kRoute};
    uint8_t      tlvsLen = sizeof(tlvs);
    Message *    message = NULL;
    Ip6::Address destination;

    if (mParent.GetExtAddress() == mParentCandidate.GetExtAddress())
    {
        if (mRole == OT_DEVICE_ROLE_CHILD)
        {
            otLogInfoMle(GetInstance(), "Already attached to candidate parent");
            ExitNow(error = OT_ERROR_ALREADY);
        }
        else
        {
            // Invalidate stale parent state.
            //
            // Parent state is not normally invalidated after becoming a Router/Leader (see #1875).  When trying to
            // attach to a better partition, invalidating old parent state (especially when in kStateRestored) ensures
            // that GetNeighbor() returns mParentCandidate when processing the Child ID Response.
            mParent.SetState(Neighbor::kStateInvalid);
        }
    }

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

        // no need to request the last Route64 TLV for MTD
        tlvsLen -= 1;
    }

    SuccessOrExit(error = AppendTlvRequest(*message, tlvs, tlvsLen));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    mParentCandidate.SetState(Neighbor::kStateValid);

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParentCandidate.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));
    LogMleMessage("Send Child ID Request", destination);
    ;

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        GetNetif().GetMeshForwarder().GetDataPollManager().SetAttachMode(true);
        GetNetif().GetMeshForwarder().SetRxOnWhenIdle(false);
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Mle::SendDataRequest(const Ip6::Address &aDestination,
                             const uint8_t *     aTlvs,
                             uint8_t             aTlvsLength,
                             uint16_t            aDelay)
{
    otError  error = OT_ERROR_NONE;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandDataRequest));
    SuccessOrExit(error = AppendTlvRequest(*message, aTlvs, aTlvsLength));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    if (aDelay)
    {
        SuccessOrExit(error = AddDelayedResponse(*message, aDestination, aDelay));
        LogMleMessage("Delay Data Request", aDestination);
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aDestination));
        LogMleMessage("Send Data Request", aDestination);

        if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            GetNetif().GetMeshForwarder().GetDataPollManager().SendFastPolls(DataPollManager::kDefaultFastPolls);
        }
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Mle::HandleSendChildUpdateRequest(Tasklet &aTasklet)
{
    aTasklet.GetOwner<Mle>().HandleSendChildUpdateRequest();
}

void Mle::HandleSendChildUpdateRequest(void)
{
    // a Network Data update can cause a change to the IPv6 address configuration
    // only send a Child Update Request after we know there are no more pending changes
    if (GetNotifier().IsPending())
    {
        mSendChildUpdateRequest.Post();
    }
    else
    {
        SendChildUpdateRequest();
    }
}

void Mle::HandleChildUpdateRequestTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mle>().HandleChildUpdateRequestTimer();
}

void Mle::HandleChildUpdateRequestTimer(void)
{
    SendChildUpdateRequest();
}

otError Mle::SendChildUpdateRequest(void)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message = NULL;

    if (mChildUpdateAttempts >= kMaxChildKeepAliveAttempts)
    {
        mChildUpdateAttempts = 0;
        BecomeDetached();
        ExitNow();
    }

    if (!mParent.IsStateValidOrRestoring())
    {
        otLogWarnMle(GetInstance(), "No valid parent when sending Child Update Request");
        BecomeDetached();
        ExitNow();
    }

    mChildUpdateRequestTimer.Start(kUnicastRetransmissionDelay);
    mChildUpdateAttempts++;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleChildUpdateRequest);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateRequest));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));

    if ((mDeviceMode & ModeTlv::kModeFFD) == 0)
    {
        SuccessOrExit(error = AppendAddressRegistration(*message));
    }

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DETACHED:
        Random::FillBuffer(mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge));
        SuccessOrExit(error = AppendChallenge(*message, mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge)));
        break;

    case OT_DEVICE_ROLE_CHILD:
        SuccessOrExit(error = AppendSourceAddress(*message));
        SuccessOrExit(error = AppendLeaderData(*message));
        SuccessOrExit(error = AppendTimeout(*message, mTimeout));
        break;

    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        assert(false);
        break;
    }

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParent.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    LogMleMessage("Send Child Update Request to parent", destination);

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        netif.GetMeshForwarder().GetDataPollManager().SetAttachMode(true);
        netif.GetMeshForwarder().SetRxOnWhenIdle(false);
    }
    else
    {
        netif.GetMeshForwarder().SetRxOnWhenIdle(true);
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
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message;

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

    LogMleMessage("Send Child Update Response to parent", destination);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Mle::SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce)
{
    Ip6::Address destination;

    memset(&destination, 0, sizeof(destination));
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);

    return SendAnnounce(aChannel, aOrphanAnnounce, destination);
}

otError Mle::SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce, const Ip6::Address &aDestination)
{
    ThreadNetif &      netif = GetNetif();
    otError            error = OT_ERROR_NONE;
    ChannelTlv         channel;
    PanIdTlv           panid;
    ActiveTimestampTlv activeTimestamp;
    Message *          message;

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetLinkSecurityEnabled(true);
    message->SetSubType(Message::kSubTypeMleAnnounce);
    message->SetChannel(aChannel);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandAnnounce));

    channel.Init();
    channel.SetChannelPage(0);
    channel.SetChannel(netif.GetMac().GetPanChannel());
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
        SuccessOrExit(error = AppendActiveTimestamp(*message));
    }

    panid.Init();
    panid.SetPanId(netif.GetMac().GetPanId());
    SuccessOrExit(error = message->Append(&panid, sizeof(panid)));
    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle(GetInstance(), "Send Announce on channel %d", aChannel);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Mle::SendOrphanAnnounce(void)
{
    const MeshCoP::ChannelMask0Tlv *channelMask;
    uint8_t                         channel;
    MeshCoP::Dataset                dataset(MeshCoP::Tlv::kActiveTimestamp);

    GetNetif().GetActiveDataset().Get(dataset);
    channelMask = static_cast<const MeshCoP::ChannelMask0Tlv *>(dataset.Get(MeshCoP::Tlv::kChannelMask));

    VerifyOrExit(channelMask != NULL);

    // find next channel in the Active Operational Dataset Channel Mask
    channel = mAnnounceChannel;

    while (!channelMask->IsChannelSet(channel))
    {
        channel++;

        if (channel > OT_RADIO_CHANNEL_MAX)
        {
            channel = OT_RADIO_CHANNEL_MIN;
        }

        VerifyOrExit(channel != mAnnounceChannel);
    }

    // Send Announce message
    SendAnnounce(channel, true);

    // Move to next channel
    mAnnounceChannel = channel + 1;

    if (mAnnounceChannel > OT_RADIO_CHANNEL_MAX)
    {
        mAnnounceChannel = OT_RADIO_CHANNEL_MIN;
    }

exit:
    return;
}

otError Mle::SendMessage(Message &aMessage, const Ip6::Address &aDestination)
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Header           header;
    uint32_t         keySequence;
    uint8_t          nonce[13];
    uint8_t          tag[4];
    uint8_t          tagLength;
    Crypto::AesCcm   aesCcm;
    uint8_t          buf[64];
    uint16_t         length;
    Ip6::MessageInfo messageInfo;

    aMessage.Read(0, sizeof(header), &header);

    if (header.GetSecuritySuite() == Header::k154Security)
    {
        header.SetFrameCounter(netif.GetKeyManager().GetMleFrameCounter());

        keySequence = netif.GetKeyManager().GetCurrentKeySequence();
        header.SetKeyId(keySequence);

        aMessage.Write(0, header.GetLength(), &header);

        GenerateNonce(netif.GetMac().GetExtAddress(), netif.GetKeyManager().GetMleFrameCounter(),
                      Mac::Frame::kSecEncMic32, nonce);

        aesCcm.SetKey(netif.GetKeyManager().GetCurrentMleKey(), 16);
        error = aesCcm.Init(16 + 16 + header.GetHeaderLength(), aMessage.GetLength() - (header.GetLength() - 1),
                            sizeof(tag), nonce, sizeof(nonce));
        assert(error == OT_ERROR_NONE);

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

        netif.GetKeyManager().IncrementMleFrameCounter();
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(mLinkLocal64.GetAddress());
    messageInfo.SetPeerPort(kUdpPort);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());
    messageInfo.SetHopLimit(kMleHopLimit);

    SuccessOrExit(error = mSocket.SendTo(aMessage, messageInfo));

exit:
    return error;
}

otError Mle::AddDelayedResponse(Message &aMessage, const Ip6::Address &aDestination, uint16_t aDelay)
{
    otError  error = OT_ERROR_NONE;
    uint32_t alarmFireTime;
    uint32_t sendTime = TimerMilli::GetNow() + aDelay;

    // Append the message with DelayedRespnoseHeader and add to the list.
    DelayedResponseHeader delayedResponse(sendTime, aDestination);
    SuccessOrExit(error = delayedResponse.AppendTo(aMessage));
    mDelayedResponses.Enqueue(aMessage);

    if (mDelayedResponseTimer.IsRunning())
    {
        // If timer is already running, check if it should be restarted with earlier fire time.
        alarmFireTime = mDelayedResponseTimer.GetFireTime();

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
    ThreadNetif &   netif = GetNetif();
    MleRouter &     mle   = netif.GetMle();
    Header          header;
    uint32_t        keySequence;
    const uint8_t * mleKey;
    uint32_t        frameCounter;
    uint8_t         messageTag[4];
    uint8_t         nonce[13];
    Mac::ExtAddress macAddr;
    Crypto::AesCcm  aesCcm;
    uint16_t        mleOffset;
    uint8_t         buf[64];
    uint16_t        length;
    uint8_t         tag[4];
    uint8_t         tagLength;
    uint8_t         command;
    Neighbor *      neighbor;

    VerifyOrExit(aMessageInfo.GetLinkInfo() != NULL);
    VerifyOrExit(aMessageInfo.GetHopLimit() == kMleHopLimit);

    length = aMessage.Read(aMessage.GetOffset(), sizeof(header), &header);
    VerifyOrExit(header.IsValid() && header.GetLength() <= length);

    if (header.GetSecuritySuite() == Header::kNoSecurity)
    {
        aMessage.MoveOffset(header.GetLength());

        switch (header.GetCommand())
        {
        case Header::kCommandDiscoveryRequest:
            mle.HandleDiscoveryRequest(aMessage, aMessageInfo);
            break;

        case Header::kCommandDiscoveryResponse:
            HandleDiscoveryResponse(aMessage, aMessageInfo);
            break;

        default:
            break;
        }

        ExitNow();
    }

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED && header.GetSecuritySuite() == Header::k154Security);

    keySequence = header.GetKeyId();

    if (keySequence == netif.GetKeyManager().GetCurrentKeySequence())
    {
        mleKey = netif.GetKeyManager().GetCurrentMleKey();
    }
    else
    {
        mleKey = netif.GetKeyManager().GetTemporaryMleKey(keySequence);
    }

    VerifyOrExit(aMessage.GetOffset() + header.GetLength() + sizeof(messageTag) <= aMessage.GetLength());
    aMessage.MoveOffset(header.GetLength() - 1);

    aMessage.Read(aMessage.GetLength() - sizeof(messageTag), sizeof(messageTag), messageTag);
    SuccessOrExit(aMessage.SetLength(aMessage.GetLength() - sizeof(messageTag)));

    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);
    frameCounter = header.GetFrameCounter();
    GenerateNonce(macAddr, frameCounter, Mac::Frame::kSecEncMic32, nonce);

    aesCcm.SetKey(mleKey, 16);
    SuccessOrExit(
        aesCcm.Init(sizeof(aMessageInfo.GetPeerAddr()) + sizeof(aMessageInfo.GetSockAddr()) + header.GetHeaderLength(),
                    aMessage.GetLength() - aMessage.GetOffset(), sizeof(messageTag), nonce, sizeof(nonce)));

    aesCcm.Header(&aMessageInfo.GetPeerAddr(), sizeof(aMessageInfo.GetPeerAddr()));
    aesCcm.Header(&aMessageInfo.GetSockAddr(), sizeof(aMessageInfo.GetSockAddr()));
    aesCcm.Header(header.GetBytes() + 1, header.GetHeaderLength());

    mleOffset = aMessage.GetOffset();

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        length = aMessage.Read(aMessage.GetOffset(), sizeof(buf), buf);
        aesCcm.Payload(buf, buf, length, false);
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        aMessage.Write(aMessage.GetOffset(), length, buf);
#endif
        aMessage.MoveOffset(length);
    }

    tagLength = sizeof(tag);
    aesCcm.Finalize(tag, &tagLength);
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    VerifyOrExit(memcmp(messageTag, tag, sizeof(tag)) == 0);
#endif

    if (keySequence > netif.GetKeyManager().GetCurrentKeySequence())
    {
        netif.GetKeyManager().SetCurrentKeySequence(keySequence);
    }

    aMessage.SetOffset(mleOffset);

    aMessage.Read(aMessage.GetOffset(), sizeof(command), &command);
    aMessage.MoveOffset(sizeof(command));

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DETACHED:
    case OT_DEVICE_ROLE_CHILD:
        neighbor = GetNeighbor(macAddr);
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if (command == Header::kCommandChildIdResponse)
        {
            neighbor = GetNeighbor(macAddr);
        }
        else
        {
            neighbor = mle.GetNeighbor(macAddr);
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
        if (!(command == Header::kCommandLinkRequest || command == Header::kCommandLinkAccept ||
              command == Header::kCommandLinkAcceptAndRequest || command == Header::kCommandAdvertisement ||
              command == Header::kCommandParentRequest || command == Header::kCommandParentResponse ||
              command == Header::kCommandChildIdRequest || command == Header::kCommandChildUpdateRequest ||
              command == Header::kCommandChildUpdateResponse || command == Header::kCommandAnnounce))
        {
            otLogDebgMle(GetInstance(), "mle sequence unknown! %d", command);
            ExitNow();
        }
    }

    switch (command)
    {
    case Header::kCommandLinkRequest:
        mle.HandleLinkRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandLinkAccept:
        mle.HandleLinkAccept(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandLinkAcceptAndRequest:
        mle.HandleLinkAcceptAndRequest(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandAdvertisement:
        HandleAdvertisement(aMessage, aMessageInfo);
        break;

    case Header::kCommandDataRequest:
        mle.HandleDataRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandDataResponse:
        HandleDataResponse(aMessage, aMessageInfo);
        break;

    case Header::kCommandParentRequest:
        mle.HandleParentRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandParentResponse:
        HandleParentResponse(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdRequest:
        mle.HandleChildIdRequest(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdResponse:
        HandleChildIdResponse(aMessage, aMessageInfo);
        break;

    case Header::kCommandChildUpdateRequest:
        if (mRole == OT_DEVICE_ROLE_LEADER || mRole == OT_DEVICE_ROLE_ROUTER)
        {
            mle.HandleChildUpdateRequest(aMessage, aMessageInfo, keySequence);
        }
        else
        {
            HandleChildUpdateRequest(aMessage, aMessageInfo);
        }

        break;

    case Header::kCommandChildUpdateResponse:
        if (mRole == OT_DEVICE_ROLE_LEADER || mRole == OT_DEVICE_ROLE_ROUTER)
        {
            mle.HandleChildUpdateResponse(aMessage, aMessageInfo, keySequence);
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
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_NONE;
    Mac::ExtAddress  macAddr;
    bool             isNeighbor;
    Neighbor *       neighbor;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv    leaderData;
    RouteTlv         route;
    uint8_t          tlvs[] = {Tlv::kNetworkData};
    uint16_t         delay;

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    LogMleMessage("Receive Advertisement", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);

    if (mRole != OT_DEVICE_ROLE_DETACHED)
    {
        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            SuccessOrExit(error = netif.GetMle().HandleAdvertisement(aMessage, aMessageInfo));
        }
        else
        {
            // Remove stale parent.
            if (GetNeighbor(macAddr) == static_cast<Neighbor *>(&mParent) &&
                mParent.GetRloc16() != sourceAddress.GetRloc16())
            {
                BecomeDetached();
            }
        }
    }

    isNeighbor = false;

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        break;

    case OT_DEVICE_ROLE_CHILD:
        if (mParent.GetExtAddress() != macAddr)
        {
            break;
        }

        if ((mParent.GetRloc16() == sourceAddress.GetRloc16()) &&
            (leaderData.GetPartitionId() != mLeaderData.GetPartitionId() ||
             leaderData.GetLeaderRouterId() != GetLeaderId()))
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

            if ((mDeviceMode & ModeTlv::kModeFFD) &&
                (Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE) && route.IsValid())
            {
                // Overwrite Route Data
                netif.GetMle().ProcessRouteTlv(route);
            }

            mRetrieveNewNetworkData = true;
        }

        isNeighbor = true;
        mParent.SetLastHeard(TimerMilli::GetNow());
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if ((neighbor = netif.GetMle().GetNeighbor(macAddr)) != NULL && neighbor->GetState() == Neighbor::kStateValid)
        {
            isNeighbor = true;
        }

        break;
    }

    if (isNeighbor)
    {
        if (mRetrieveNewNetworkData ||
            (static_cast<int8_t>(leaderData.GetDataVersion() - netif.GetNetworkDataLeader().GetVersion()) > 0))
        {
            delay = Random::GetUint16InRange(0, kMleMaxResponseDelay);
            SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay);
        }
    }

exit:
    return error;
}

otError Mle::HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error;

    LogMleMessage("Receive Data Response", aMessageInfo.GetPeerAddr());

    error = HandleLeaderData(aMessage, aMessageInfo);

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Data Response");
    }

    return error;
}

otError Mle::HandleLeaderData(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &       netif = GetNetif();
    otError             error = OT_ERROR_NONE;
    LeaderDataTlv       leaderData;
    ActiveTimestampTlv  activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    uint16_t            networkDataOffset    = 0;
    uint16_t            activeDatasetOffset  = 0;
    uint16_t            pendingDatasetOffset = 0;
    bool                dataRequest          = false;
    Tlv                 tlv;

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    if ((leaderData.GetPartitionId() != mLeaderData.GetPartitionId()) ||
        (leaderData.GetWeighting() != mLeaderData.GetWeighting()) || (leaderData.GetLeaderRouterId() != GetLeaderId()))
    {
        if (mRole == OT_DEVICE_ROLE_CHILD)
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
            diff = static_cast<int8_t>(leaderData.GetDataVersion() - netif.GetNetworkDataLeader().GetVersion());
        }
        else
        {
            diff = static_cast<int8_t>(leaderData.GetStableDataVersion() -
                                       netif.GetNetworkDataLeader().GetStableVersion());
        }

        VerifyOrExit(diff > 0);
    }

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);
        timestamp = netif.GetActiveDataset().GetTimestamp();

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
        timestamp = netif.GetPendingDataset().GetTimestamp();

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

    if (Tlv::GetOffset(aMessage, Tlv::kNetworkData, networkDataOffset) == OT_ERROR_NONE)
    {
        error = netif.GetNetworkDataLeader().SetNetworkData(
            leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
            (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0, aMessage, networkDataOffset);
        SuccessOrExit(error);
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
            netif.GetActiveDataset().Set(activeTimestamp, aMessage, activeDatasetOffset + sizeof(tlv), tlv.GetLength());
        }
    }

    // Pending Dataset
    if (pendingTimestamp.GetLength() > 0)
    {
        if (pendingDatasetOffset > 0)
        {
            aMessage.Read(pendingDatasetOffset, sizeof(tlv), &tlv);
            netif.GetPendingDataset().Set(pendingTimestamp, aMessage, pendingDatasetOffset + sizeof(tlv),
                                          tlv.GetLength());
        }
    }

    mRetrieveNewNetworkData = false;

exit:

    if (dataRequest)
    {
        static const uint8_t tlvs[] = {Tlv::kNetworkData};
        uint16_t             delay;

        if (aMessageInfo.GetSockAddr().IsMulticast())
        {
            delay = Random::GetUint16InRange(0, kMleMaxResponseDelay);
        }
        else
        {
            // This method may have been called from an MLE request
            // handler.  We add a minimum delay here so that the MLE
            // response is enqueued before the MLE Data Request.
            delay = 10;
        }

        SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay);
    }

    return error;
}

bool Mle::IsBetterParent(uint16_t aRloc16, uint8_t aLinkQuality, uint8_t aLinkMargin, ConnectivityTlv &aConnectivityTlv)
{
    bool rval = false;

    uint8_t candidateLinkQualityIn     = mParentCandidate.GetLinkInfo().GetLinkQuality();
    uint8_t candidateTwoWayLinkQuality = (candidateLinkQualityIn < mParentCandidate.GetLinkQualityOut())
                                             ? candidateLinkQualityIn
                                             : mParentCandidate.GetLinkQualityOut();

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

    rval = (aLinkMargin > mParentLinkMargin);

exit:
    return rval;
}

void Mle::ResetParentCandidate(void)
{
    memset(&mParentCandidate, 0, sizeof(mParentCandidate));
    mParentCandidate.SetState(Neighbor::kStateInvalid);
}

otError Mle::HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint32_t aKeySequence)
{
    ThreadNetif &           netif    = GetNetif();
    otError                 error    = OT_ERROR_NONE;
    const otThreadLinkInfo *linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    ResponseTlv             response;
    SourceAddressTlv        sourceAddress;
    LeaderDataTlv           leaderData;
    LinkMarginTlv           linkMarginTlv;
    uint8_t                 linkMargin;
    uint8_t                 linkQuality;
    ConnectivityTlv         connectivity;
    LinkFrameCounterTlv     linkFrameCounter;
    MleFrameCounterTlv      mleFrameCounter;
    ChallengeTlv            challenge;
    Mac::ExtAddress         extAddress;

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    LogMleMessage("Receive Parent Response", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid() &&
                     memcmp(response.GetResponse(), mParentRequest.mChallenge, response.GetLength()) == 0,
                 error = OT_ERROR_PARSE);

    aMessageInfo.GetPeerAddr().ToExtAddress(extAddress);

    if (mRole == OT_DEVICE_ROLE_CHILD && mParent.GetExtAddress() == extAddress)
    {
        mReceivedResponseFromParent = true;
    }

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    // Link Quality
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLinkMargin, sizeof(linkMarginTlv), linkMarginTlv));
    VerifyOrExit(linkMarginTlv.IsValid(), error = OT_ERROR_PARSE);

    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(netif.GetMac().GetNoiseFloor(), linkInfo->mRss);

    if (linkMargin > linkMarginTlv.GetLinkMargin())
    {
        linkMargin = linkMarginTlv.GetLinkMargin();
    }

    linkQuality = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin);

    VerifyOrExit(mAttachState != kAttachStateParentRequestRouter || linkQuality == 3);

    // Connectivity
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kConnectivity, sizeof(connectivity), connectivity));
    VerifyOrExit(connectivity.IsValid(), error = OT_ERROR_PARSE);

#if OPENTHREAD_FTD
    if ((mDeviceMode & ModeTlv::kModeFFD) && (mRole != OT_DEVICE_ROLE_DETACHED))
    {
        int8_t diff =
            static_cast<int8_t>(connectivity.GetIdSequence() - netif.GetMle().GetRouterTable().GetRouterIdSequence());

        switch (mParentRequestMode)
        {
        case kAttachAny:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId() || diff > 0);
            break;

        case kAttachSame1:
        case kAttachSame2:
            VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());
            VerifyOrExit(diff > 0 || (diff == 0 && netif.GetMle().GetRouterTable().GetLeaderAge() <
                                                       netif.GetMle().GetNetworkIdTimeout()));
            break;

        case kAttachBetter:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId());
            VerifyOrExit(netif.GetMle().ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                          netif.GetMle().IsSingleton(), mLeaderData) > 0);
            break;
        }
    }
#endif

    // if already have a candidate parent, only seek a better parent
    if (mParentCandidate.GetState() == Neighbor::kStateParentResponse)
    {
        int compare = 0;

        if (mDeviceMode & ModeTlv::kModeFFD)
        {
            compare = netif.GetMle().ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                       mParentIsSingleton, mParentLeaderData);
        }

        // only consider partitions that are the same or better
        VerifyOrExit(compare >= 0);

        // only consider better parents if the partitions are the same
        VerifyOrExit(compare != 0 || IsBetterParent(sourceAddress.GetRloc16(), linkQuality, linkMargin, connectivity));
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

    mParentCandidate.SetExtAddress(extAddress);
    mParentCandidate.SetRloc16(sourceAddress.GetRloc16());
    mParentCandidate.SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    mParentCandidate.SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    mParentCandidate.SetDeviceMode(ModeTlv::kModeFFD | ModeTlv::kModeRxOnWhenIdle | ModeTlv::kModeFullNetworkData |
                                   ModeTlv::kModeSecureDataRequest);
    mParentCandidate.GetLinkInfo().Clear();
    mParentCandidate.GetLinkInfo().AddRss(netif.GetMac().GetNoiseFloor(), linkInfo->mRss);
    mParentCandidate.ResetLinkFailures();
    mParentCandidate.SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMarginTlv.GetLinkMargin()));
    mParentCandidate.SetState(Neighbor::kStateParentResponse);
    mParentCandidate.SetKeySequence(aKeySequence);

    mParentPriority     = connectivity.GetParentPriority();
    mParentLinkQuality3 = connectivity.GetLinkQuality3();
    mParentLinkQuality2 = connectivity.GetLinkQuality2();
    mParentLinkQuality1 = connectivity.GetLinkQuality1();
    mParentLeaderCost   = connectivity.GetLeaderCost();
    mParentLeaderData   = leaderData;
    mParentIsSingleton  = connectivity.GetActiveRouters() <= 1;
    mParentLinkMargin   = linkMargin;

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Parent Response");
    }

    return error;
}

otError Mle::HandleChildIdResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &       netif = GetNetif();
    otError             error = OT_ERROR_NONE;
    LeaderDataTlv       leaderData;
    SourceAddressTlv    sourceAddress;
    Address16Tlv        shortAddress;
    RouteTlv            route;
    ActiveTimestampTlv  activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    Tlv                 tlv;
    uint16_t            networkDataOffset;
    uint16_t            offset;

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    LogMleMessage("Receive Child ID Response", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());

    VerifyOrExit(mAttachState == kAttachStateChildIdRequest);

    // Leader Data
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kLeaderData, sizeof(leaderData), leaderData));
    VerifyOrExit(leaderData.IsValid(), error = OT_ERROR_PARSE);

    // ShortAddress
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kAddress16, sizeof(shortAddress), shortAddress));
    VerifyOrExit(shortAddress.IsValid(), error = OT_ERROR_PARSE);

    // Network Data
    error = Tlv::GetOffset(aMessage, Tlv::kNetworkData, networkDataOffset);
    SuccessOrExit(error);

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);

        // Active Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kActiveDataset, offset) == OT_ERROR_NONE)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            netif.GetActiveDataset().Set(activeTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength());
        }
    }

    // clear Pending Dataset if device succeed to reattach using stored Pending Dataset
    if (mReattachState == kReattachPending)
    {
        netif.GetPendingDataset().Clear();
    }

    // Pending Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);

        // Pending Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, offset) == OT_ERROR_NONE)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            netif.GetPendingDataset().Set(pendingTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength());
        }
    }
    else
    {
        netif.GetPendingDataset().ClearNetwork();
    }

    // Parent Attach Success
    mAttachTimer.Stop();
    SetStateDetached();

    SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

    if ((mDeviceMode & ModeTlv::kModeRxOnWhenIdle) == 0)
    {
        netif.GetMeshForwarder().GetDataPollManager().SetAttachMode(false);
        netif.GetMeshForwarder().SetRxOnWhenIdle(false);
    }
    else
    {
        netif.GetMeshForwarder().SetRxOnWhenIdle(true);
    }

    // Route
    if ((Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE) &&
        (mDeviceMode & ModeTlv::kModeFFD))
    {
        SuccessOrExit(error = netif.GetMle().ProcessRouteTlv(route));
    }

    mParent = mParentCandidate;
    ResetParentCandidate();

    mParent.SetRloc16(sourceAddress.GetRloc16());

    netif.GetNetworkDataLeader().SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                                (mDeviceMode & ModeTlv::kModeFullNetworkData) == 0, aMessage,
                                                networkDataOffset);

    netif.GetActiveDataset().ApplyConfiguration();

    SuccessOrExit(error = SetStateChild(shortAddress.GetRloc16()));

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMleErr(GetInstance(), error, "Failed to process Child ID Response");
    }

    OT_UNUSED_VARIABLE(aMessageInfo);
    return error;
}

otError Mle::HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    static const uint8_t kMaxResponseTlvs = 5;

    otError          error = OT_ERROR_NONE;
    Mac::ExtAddress  srcAddr;
    SourceAddressTlv sourceAddress;
    LeaderDataTlv    leaderData;
    ChallengeTlv     challenge;
    StatusTlv        status;
    TlvRequestTlv    tlvRequest;
    uint8_t          tlvs[kMaxResponseTlvs] = {};
    uint8_t          numTlvs                = 0;

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    LogMleMessage("Receive Child Update Request from parent", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());

    VerifyOrExit(mParent.GetRloc16() == sourceAddress.GetRloc16(), error = OT_ERROR_DROP);

    // Leader Data, Network Data, Active Timestamp, Pending Timestamp
    SuccessOrExit(error = HandleLeaderData(aMessage, aMessageInfo));

    // Status
    if (Tlv::GetTlv(aMessage, Tlv::kStatus, sizeof(status), status) == OT_ERROR_NONE)
    {
        VerifyOrExit(status.IsValid(), error = OT_ERROR_PARSE);

        aMessageInfo.GetPeerAddr().ToExtAddress(srcAddr);
        VerifyOrExit(mParent.GetExtAddress() == srcAddr, error = OT_ERROR_DROP);

        if (status.GetStatus() == StatusTlv::kError)
        {
            BecomeDetached();
            ExitNow();
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

exit:
    return error;
}

otError Mle::HandleChildUpdateResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetif &       netif = GetNetif();
    otError             error = OT_ERROR_NONE;
    StatusTlv           status;
    ModeTlv             mode;
    ResponseTlv         response;
    LinkFrameCounterTlv linkFrameCounter;
    MleFrameCounterTlv  mleFrameCounter;
    SourceAddressTlv    sourceAddress;
    TimeoutTlv          timeout;

    LogMleMessage("Receive Child Update Response from parent", aMessageInfo.GetPeerAddr());

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

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DETACHED:
        // Response
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
        VerifyOrExit(response.IsValid(), error = OT_ERROR_PARSE);
        VerifyOrExit(memcmp(response.GetResponse(), mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge)) == 0,
                     error = OT_ERROR_DROP);

        SuccessOrExit(error =
                          Tlv::GetTlv(aMessage, Tlv::kLinkFrameCounter, sizeof(linkFrameCounter), linkFrameCounter));
        VerifyOrExit(linkFrameCounter.IsValid(), error = OT_ERROR_PARSE);

        if (Tlv::GetTlv(aMessage, Tlv::kMleFrameCounter, sizeof(mleFrameCounter), mleFrameCounter) == OT_ERROR_NONE)
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

    case OT_DEVICE_ROLE_CHILD:
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
            netif.GetMeshForwarder().GetDataPollManager().SetAttachMode(false);
            netif.GetMeshForwarder().SetRxOnWhenIdle(false);
            mChildUpdateRequestTimer.Stop();
        }
        else
        {
            mChildUpdateRequestTimer.Start(TimerMilli::SecToMsec(mTimeout) -
                                           static_cast<uint32_t>(kUnicastRetransmissionDelay) *
                                               kMaxChildKeepAliveAttempts);
            netif.GetMeshForwarder().SetRxOnWhenIdle(true);
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
    ThreadNetif &             netif = GetNetif();
    otError                   error = OT_ERROR_NONE;
    ChannelTlv                channelTlv;
    ActiveTimestampTlv        timestamp;
    const MeshCoP::Timestamp *localTimestamp;
    PanIdTlv                  panIdTlv;
    uint8_t                   channel;
    uint16_t                  panId;

    LogMleMessage("Receive Announce", aMessageInfo.GetPeerAddr());

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChannel, sizeof(channelTlv), channelTlv));
    VerifyOrExit(channelTlv.IsValid(), error = OT_ERROR_PARSE);
    channel = static_cast<uint8_t>(channelTlv.GetChannel());

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(timestamp), timestamp));
    VerifyOrExit(timestamp.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kPanId, sizeof(panIdTlv), panIdTlv));
    VerifyOrExit(panIdTlv.IsValid(), error = OT_ERROR_PARSE);
    panId = panIdTlv.GetPanId();

    localTimestamp = netif.GetActiveDataset().GetTimestamp();

    if (localTimestamp == NULL || localTimestamp->Compare(timestamp) > 0)
    {
        uint8_t  curChannel = netif.GetMac().GetPanChannel();
        uint16_t curPanId   = netif.GetMac().GetPanId();

        // No action is required if device is detached, and current
        // channel and pan-id match the values from the received MLE
        // Announce message.

        VerifyOrExit((mRole != OT_DEVICE_ROLE_DETACHED) || (curChannel != channel) || (curPanId != panId));

        Stop(false);
        mPreviousChannel = curChannel;
        mPreviousPanId   = curPanId;
        netif.GetMac().SetPanChannel(channel);
        netif.GetMac().SetPanId(panId);
        Start(false, true);
    }
    else if (localTimestamp->Compare(timestamp) < 0)
    {
        SendAnnounce(channel, false);

#if OPENTHREAD_CONFIG_SEND_UNICAST_ANNOUNCE_RESPONSE
        SendAnnounce(channel, false, aMessageInfo.GetPeerAddr());
#endif
    }
    else
    {
        // do nothing
        // timestamps are equal: no behaviour specified by the Thread spec.
        // If SendAnnounce is executed at this point, there exists a scenario where
        // multiple devices keep sending MLE Announce messages to one another indefinitely.
    }

exit:
    OT_UNUSED_VARIABLE(aMessageInfo);
    return error;
}

otError Mle::HandleDiscoveryResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                       error    = OT_ERROR_NONE;
    const otThreadLinkInfo *      linkInfo = static_cast<const otThreadLinkInfo *>(aMessageInfo.GetLinkInfo());
    Tlv                           tlv;
    MeshCoP::Tlv                  meshcopTlv;
    MeshCoP::DiscoveryResponseTlv discoveryResponse;
    MeshCoP::ExtendedPanIdTlv     extPanId;
    MeshCoP::NetworkNameTlv       networkName;
    MeshCoP::SteeringDataTlv      steeringData;
    MeshCoP::JoinerUdpPortTlv     JoinerUdpPort;
    otActiveScanResult            result;
    uint16_t                      offset;
    uint16_t                      end;

    LogMleMessage("Receive Discovery Response", aMessageInfo.GetPeerAddr());

    VerifyOrExit(mIsDiscoverInProgress, error = OT_ERROR_DROP);

    // find MLE Discovery TLV
    VerifyOrExit(Tlv::GetOffset(aMessage, Tlv::kDiscovery, offset) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
    aMessage.Read(offset, sizeof(tlv), &tlv);

    offset += sizeof(tlv);
    end = offset + tlv.GetLength();

    memset(&result, 0, sizeof(result));
    result.mPanId   = linkInfo->mPanId;
    result.mChannel = linkInfo->mChannel;
    result.mRssi    = linkInfo->mRss;
    result.mLqi     = linkInfo->mLqi;
    aMessageInfo.GetPeerAddr().ToExtAddress(*static_cast<Mac::ExtAddress *>(&result.mExtAddress));

    // process MeshCoP TLVs
    while (offset < end)
    {
        aMessage.Read(offset, sizeof(meshcopTlv), &meshcopTlv);

        switch (meshcopTlv.GetType())
        {
        case MeshCoP::Tlv::kDiscoveryResponse:
            aMessage.Read(offset, sizeof(discoveryResponse), &discoveryResponse);
            VerifyOrExit(discoveryResponse.IsValid(), error = OT_ERROR_PARSE);
            result.mVersion  = discoveryResponse.GetVersion();
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
            result.mNetworkName.m8[networkName.GetLength()] = '\0';
            break;

        case MeshCoP::Tlv::kSteeringData:
            aMessage.Read(offset, sizeof(steeringData), &steeringData);
            VerifyOrExit(steeringData.IsValid(), error = OT_ERROR_PARSE);

            // Pass up MLE discovery responses only if the steering data is set to all 0xFFs,
            // or if it matches the factory set EUI64.
            if (mEnableEui64Filtering)
            {
                Mac::ExtAddress extaddr;
                Crc16           ccitt(Crc16::kCcitt);
                Crc16           ansi(Crc16::kAnsi);

                otPlatRadioGetIeeeEui64(&GetInstance(), extaddr.m8);

                MeshCoP::ComputeJoinerId(extaddr, extaddr);

                // Compute bloom filter
                for (size_t i = 0; i < sizeof(extaddr.m8); i++)
                {
                    ccitt.Update(extaddr.m8[i]);
                    ansi.Update(extaddr.m8[i]);
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
    if ((mParent.IsStateValidOrRestoring()) && (mParent.GetExtAddress() == aAddress))
    {
        return &mParent;
    }

    if ((mParentCandidate.GetState() == Neighbor::kStateValid) && (mParentCandidate.GetExtAddress() == aAddress))
    {
        return &mParentCandidate;
    }

    return NULL;
}

Neighbor *Mle::GetNeighbor(const Mac::Address &aAddress)
{
    Neighbor *neighbor = NULL;

    switch (aAddress.GetType())
    {
    case Mac::Address::kTypeShort:
        neighbor = GetNeighbor(aAddress.GetShort());
        break;

    case Mac::Address::kTypeExtended:
        neighbor = GetNeighbor(aAddress.GetExtended());
        break;

    default:
        break;
    }

    return neighbor;
}

uint16_t Mle::GetNextHop(uint16_t aDestination) const
{
    OT_UNUSED_VARIABLE(aDestination);
    return (mParent.GetState() == Neighbor::kStateValid) ? mParent.GetRloc16()
                                                         : static_cast<uint16_t>(Mac::kShortAddrInvalid);
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
    return &mParent;
}

Router *Mle::GetParentCandidate(void)
{
    Router *rval;

    if (mParentCandidate.GetState() == Neighbor::kStateValid)
    {
        rval = &mParentCandidate;
    }
    else
    {
        rval = &mParent;
    }

    return rval;
}

otError Mle::CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    ThreadNetif &    netif = GetNetif();
    otError          error = OT_ERROR_DROP;
    Ip6::MessageInfo messageInfo;

    if (aMeshDest != GetRloc16())
    {
        ExitNow(error = OT_ERROR_NONE);
    }

    if (netif.IsUnicastAddress(aIp6Header.GetDestination()))
    {
        ExitNow(error = OT_ERROR_NONE);
    }

    messageInfo.GetPeerAddr()                = GetMeshLocal16();
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(aMeshSource);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());

    netif.GetIp6().GetIcmp().SendError(Ip6::IcmpHeader::kTypeDstUnreach, Ip6::IcmpHeader::kCodeDstUnreachNoRoute,
                                       messageInfo, aIp6Header);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
otError Mle::InformPreviousParent(void)
{
    ThreadNetif &    netif   = GetNetif();
    otError          error   = OT_ERROR_NONE;
    Message *        message = NULL;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((mPreviousParentRloc != Mac::kShortAddrInvalid) && (mPreviousParentRloc != mParent.GetRloc16()));

    VerifyOrExit((message = netif.GetIp6().NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->SetLength(0));

    messageInfo.SetSockAddr(GetMeshLocal64());
    messageInfo.SetPeerAddr(GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(mPreviousParentRloc);
    messageInfo.SetInterfaceId(netif.GetInterfaceId());

    SuccessOrExit(error = netif.GetIp6().SendDatagram(*message, messageInfo, Ip6::kProtoNone));

    otLogInfoMle(GetInstance(), "Sending message to inform previous parent 0x%04x", mPreviousParentRloc);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle(GetInstance(), "Failed to inform previous parent, error:%s", otThreadErrorToString(error));

        if (message != NULL)
        {
            message->Free();
        }
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH

#if OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
void Mle::HandleParentSearchTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mle>().HandleParentSearchTimer();
}

void Mle::HandleParentSearchTimer(void)
{
    int8_t parentRss;

    otLogInfoMle(GetInstance(), "PeriodicParentSearch: %s interval passed",
                 mParentSearchIsInBackoff ? "Backoff" : "Check");

    if (mParentSearchBackoffWasCanceled)
    {
        // Backoff can be canceled if the device switches to a new parent.
        // from `UpdateParentSearchState()`. We want to limit this to happen
        // only once within a backoff interval.

        if (TimerMilli::GetNow() - mParentSearchBackoffCancelTime >= kParentSearchBackoffInterval)
        {
            mParentSearchBackoffWasCanceled = false;
            otLogInfoMle(GetInstance(), "PeriodicParentSearch: Backoff cancellation is allowed on parent switch");
        }
    }

    mParentSearchIsInBackoff = false;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_CHILD);

    parentRss = GetParent()->GetLinkInfo().GetAverageRss();
    otLogInfoMle(GetInstance(), "PeriodicParentSearch: Parent RSS %d", parentRss);
    VerifyOrExit(parentRss != OT_RADIO_RSSI_INVALID);

    if (parentRss < kParentSearchRssThreadhold)
    {
        otLogInfoMle(GetInstance(), "PeriodicParentSearch: Parent RSS less than %d, searching for new parents",
                     kParentSearchRssThreadhold);
        mParentSearchIsInBackoff = true;
        BecomeChild(kAttachAny);
    }

exit:
    StartParentSearchTimer();
}

void Mle::StartParentSearchTimer(void)
{
    uint32_t interval;

    interval = Random::GetUint32InRange(0, kParentSearchJitterInterval);

    if (mParentSearchIsInBackoff)
    {
        interval += kParentSearchBackoffInterval;
    }
    else
    {
        interval += kParentSearchCheckInterval;
    }

    mParentSearchTimer.Start(interval);

    otLogInfoMle(GetInstance(), "PeriodicParentSearch: (Re)starting timer for %s interval",
                 mParentSearchIsInBackoff ? "backoff" : "check");
}

void Mle::UpdateParentSearchState(void)
{
#if OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH

    // If we are in middle of backoff and backoff was not canceled
    // recently and we recently detached from a previous parent,
    // then we check if the new parent is different from the previous
    // one, and if so, we cancel the backoff mode and also remember
    // the backoff cancel time. This way the canceling of backoff
    // is allowed only once within a backoff window.
    //
    // The reason behind the canceling of the backoff is to handle
    // the scenario where a previous parent is not available for a
    // short duration (e.g., it is going through a software update)
    // and the child switches to a less desirable parent. With this
    // model the child will check for other parents sooner and have
    // the chance to switch back to the original (and possibly
    // preferred) parent more quickly.

    if (mParentSearchIsInBackoff && !mParentSearchBackoffWasCanceled && mParentSearchRecentlyDetached)
    {
        if ((mPreviousParentRloc != Mac::kShortAddrInvalid) && (mPreviousParentRloc != mParent.GetRloc16()))
        {
            mParentSearchIsInBackoff        = false;
            mParentSearchBackoffWasCanceled = true;
            mParentSearchBackoffCancelTime  = TimerMilli::GetNow();
            otLogInfoMle(GetInstance(), "PeriodicParentSearch: Canceling backoff on switching to a new parent");
        }
    }

#endif // OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH

    mParentSearchRecentlyDetached = false;

    if (!mParentSearchIsInBackoff)
    {
        StartParentSearchTimer();
    }
}
#endif // OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH

void Mle::LogMleMessage(const char *aLogString, const Ip6::Address &aAddress) const
{
#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MLE == 1)
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    otLogInfoMle(GetInstance(), "%s (%s)", aLogString, aAddress.ToString(stringBuffer, sizeof(stringBuffer)));
#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MLE == 1)

    OT_UNUSED_VARIABLE(aLogString);
    OT_UNUSED_VARIABLE(aAddress);
}

void Mle::LogMleMessage(const char *aLogString, const Ip6::Address &aAddress, uint16_t aRloc) const
{
#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MLE == 1)
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    otLogInfoMle(GetInstance(), "%s (%s,0x%04x)", aLogString, aAddress.ToString(stringBuffer, sizeof(stringBuffer)),
                 aRloc);
#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MLE == 1)

    OT_UNUSED_VARIABLE(aLogString);
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aRloc);
}

const char *Mle::RoleToString(otDeviceRole aRole)
{
    const char *roleString = "Unknown";

    switch (aRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        roleString = "Disabled";
        break;

    case OT_DEVICE_ROLE_DETACHED:
        roleString = "Detached";
        break;

    case OT_DEVICE_ROLE_CHILD:
        roleString = "Child";
        break;

    case OT_DEVICE_ROLE_ROUTER:
        roleString = "Router";
        break;

    case OT_DEVICE_ROLE_LEADER:
        roleString = "Leader";
        break;
    }

    return roleString;
}

} // namespace Mle
} // namespace ot
