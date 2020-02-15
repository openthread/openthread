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

#include "mle.hpp"

#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "crypto/aes_ccm.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/address_resolver.hpp"
#include "thread/key_manager.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/time_sync_service.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Mle {

Mle::Mle(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRetrieveNewNetworkData(false)
    , mRole(OT_DEVICE_ROLE_DISABLED)
    , mDeviceMode(DeviceMode::kModeRxOnWhenIdle | DeviceMode::kModeSecureDataRequest)
    , mAttachState(kAttachStateIdle)
    , mReattachState(kReattachStop)
    , mAttachCounter(0)
    , mAnnounceDelay(kAnnounceTimeout)
    , mAttachTimer(aInstance, &Mle::HandleAttachTimer, this)
    , mDelayedResponseTimer(aInstance, &Mle::HandleDelayedResponseTimer, this)
    , mMessageTransmissionTimer(aInstance, &Mle::HandleMessageTransmissionTimer, this)
    , mParentLeaderCost(0)
    , mParentRequestMode(kAttachAny)
    , mParentPriority(0)
    , mParentLinkQuality3(0)
    , mParentLinkQuality2(0)
    , mParentLinkQuality1(0)
    , mChildUpdateAttempts(0)
    , mChildUpdateRequestState(kChildUpdateRequestNone)
    , mDataRequestAttempts(0)
    , mDataRequestState(kDataRequestNone)
    , mAddressRegistrationMode(kAppendAllAddresses)
    , mParentLinkMargin(0)
    , mParentIsSingleton(false)
    , mReceivedResponseFromParent(false)
    , mSocket(aInstance.Get<Ip6::Udp>())
    , mTimeout(kMleEndDeviceTimeout)
    , mDiscoverHandler(NULL)
    , mDiscoverContext(NULL)
    , mDiscoverCcittIndex(0)
    , mDiscoverAnsiIndex(0)
    , mDiscoverInProgress(false)
    , mDiscoverEnableFiltering(false)
#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
    , mPreviousParentRloc(Mac::kShortAddrInvalid)
#endif
#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    , mParentSearchIsInBackoff(false)
    , mParentSearchBackoffWasCanceled(false)
    , mParentSearchRecentlyDetached(false)
    , mParentSearchBackoffCancelTime(0)
    , mParentSearchTimer(aInstance, &Mle::HandleParentSearchTimer, this)
#endif
    , mAnnounceChannel(0)
    , mAlternateChannel(0)
    , mAlternatePanId(Mac::kPanIdBroadcast)
    , mAlternateTimestamp(0)
    , mNotifierCallback(aInstance, &Mle::HandleStateChanged, this)
    , mParentResponseCb(NULL)
    , mParentResponseCbContext(NULL)
{
    otMeshLocalPrefix meshLocalPrefix;

    mLeaderData.Clear();
    mParentLeaderData.Clear();
    mParent.Clear();
    memset(&mChildIdRequest, 0, sizeof(mChildIdRequest));
    mParentCandidate.Clear();
    ResetCounters();

    // link-local 64
    mLinkLocal64.Clear();
    mLinkLocal64.GetAddress().mFields.m16[0] = HostSwap16(0xfe80);
    mLinkLocal64.GetAddress().SetIid(Get<Mac::Mac>().GetExtAddress());
    mLinkLocal64.mPrefixLength = 64;
    mLinkLocal64.mPreferred    = true;
    mLinkLocal64.mValid        = true;

    // Leader Aloc
    mLeaderAloc.Clear();
    mLeaderAloc.mPrefixLength       = 64;
    mLeaderAloc.mPreferred          = true;
    mLeaderAloc.mValid              = true;
    mLeaderAloc.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mLeaderAloc.mScopeOverrideValid = true;

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

    // Service Alocs
    for (size_t i = 0; i < OT_ARRAY_LENGTH(mServiceAlocs); i++)
    {
        mServiceAlocs[i].Clear();
        mServiceAlocs[i].mPrefixLength               = 64;
        mServiceAlocs[i].mPreferred                  = true;
        mServiceAlocs[i].mValid                      = true;
        mServiceAlocs[i].mScopeOverride              = Ip6::Address::kRealmLocalScope;
        mServiceAlocs[i].mScopeOverrideValid         = true;
        mServiceAlocs[i].GetAddress().mFields.m16[7] = HostSwap16(Mac::kShortAddrInvalid);
    }

#endif

    // initialize Mesh Local Prefix
    meshLocalPrefix.m8[0] = 0xfd;
    memcpy(&meshLocalPrefix.m8[1], Get<Mac::Mac>().GetExtendedPanId().m8, 5);
    meshLocalPrefix.m8[6] = 0x00;
    meshLocalPrefix.m8[7] = 0x00;

    // mesh-local 64
    mMeshLocal64.Clear();
    Random::NonCrypto::FillBuffer(mMeshLocal64.GetAddress().mFields.m8 + OT_IP6_PREFIX_SIZE,
                                  OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);

    mMeshLocal64.mPrefixLength       = 64;
    mMeshLocal64.mPreferred          = true;
    mMeshLocal64.mValid              = true;
    mMeshLocal64.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mMeshLocal64.mScopeOverrideValid = true;

    // mesh-local 16
    mMeshLocal16.Clear();
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
    Get<Ip6::Mpl>().SetMatchingAddress(mMeshLocal16.GetAddress());

    // link-local all thread nodes
    mLinkLocalAllThreadNodes.Clear();
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff32);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[6] = HostSwap16(0x0000);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);

    // realm-local all thread nodes
    mRealmLocalAllThreadNodes.Clear();
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff33);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[6] = HostSwap16(0x0000);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);

    SetMeshLocalPrefix(meshLocalPrefix);

    // `SetMeshLocalPrefix()` also adds the Mesh-Local EID and subscribes
    // to the Link- and Realm-Local All Thread Nodes multicast addresses.
}

otError Mle::Enable(void)
{
    otError       error = OT_ERROR_NONE;
    Ip6::SockAddr sockaddr;

    UpdateLinkLocalAddress();
    sockaddr.mPort = kUdpPort;
    SuccessOrExit(error = mSocket.Open(&Mle::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(sockaddr));

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    StartParentSearchTimer();
#endif
exit:
    return error;
}

otError Mle::Disable(void)
{
    otError error = OT_ERROR_NONE;

    Stop(false);
    SuccessOrExit(error = mSocket.Close());
    SuccessOrExit(error = Get<ThreadNetif>().RemoveUnicastAddress(mLinkLocal64));

exit:
    return error;
}

otError Mle::Start(bool aAnnounceAttach)
{
    otError error = OT_ERROR_NONE;

    // cannot bring up the interface if IEEE 802.15.4 promiscuous mode is enabled
    VerifyOrExit(!Get<Radio>().GetPromiscuous(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(Get<ThreadNetif>().IsUp(), error = OT_ERROR_INVALID_STATE);

    if (Get<Mac::Mac>().GetPanId() == Mac::kPanIdBroadcast)
    {
        // if PAN ID is not configured, pick a random one to start

        uint16_t panid;

        do
        {
            panid = Random::NonCrypto::GetUint16();
        } while (panid == Mac::kPanIdBroadcast);

        Get<Mac::Mac>().SetPanId(panid);
    }

    SetStateDetached();

    ApplyMeshLocalPrefix();
    SetRloc16(GetRloc16());

    mAttachCounter = 0;

    Get<KeyManager>().Start();

    if (!aAnnounceAttach)
    {
        mReattachState = kReattachStart;
    }

    if (aAnnounceAttach || (GetRloc16() == Mac::kShortAddrInvalid))
    {
        BecomeChild(kAttachAny);
    }
    else if (IsActiveRouter(GetRloc16()))
    {
        if (Get<MleRouter>().BecomeRouter(ThreadStatusTlv::kTooFewRouters) != OT_ERROR_NONE)
        {
            BecomeChild(kAttachAny);
        }
    }
    else
    {
        mChildUpdateAttempts = 0;
        SendChildUpdateRequest();
    }

exit:
    return error;
}

void Mle::Stop(bool aClearNetworkDatasets)
{
    if (aClearNetworkDatasets)
    {
        Get<MeshCoP::ActiveDataset>().HandleDetach();
        Get<MeshCoP::PendingDataset>().HandleDetach();
    }

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED);

    Get<KeyManager>().Stop();
    SetStateDetached();
    Get<ThreadNetif>().UnsubscribeMulticast(mRealmLocalAllThreadNodes);
    Get<ThreadNetif>().UnsubscribeMulticast(mLinkLocalAllThreadNodes);
    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal16);
    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal64);

    SetRole(OT_DEVICE_ROLE_DISABLED);

exit:
    return;
}

void Mle::SetRole(otDeviceRole aRole)
{
    otDeviceRole oldRole = mRole;

    SuccessOrExit(Get<Notifier>().Update(mRole, aRole, OT_CHANGED_THREAD_ROLE));

    otLogNoteMle("Role %s -> %s", RoleToString(oldRole), RoleToString(mRole));

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        mCounters.mDisabledRole++;
        break;
    case OT_DEVICE_ROLE_DETACHED:
        mCounters.mDetachedRole++;
        break;
    case OT_DEVICE_ROLE_CHILD:
        mCounters.mChildRole++;
        break;
    case OT_DEVICE_ROLE_ROUTER:
        mCounters.mRouterRole++;
        break;
    case OT_DEVICE_ROLE_LEADER:
        mCounters.mLeaderRole++;
        break;
    }

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    if (IsAttached())
    {
        SuccessOrExit(Get<MeshCoP::BorderAgent>().Start());
    }
    else
    {
        SuccessOrExit(Get<MeshCoP::BorderAgent>().Stop());
    }
#endif

exit:
    OT_UNUSED_VARIABLE(oldRole);
}

void Mle::SetAttachState(AttachState aState)
{
    VerifyOrExit(aState != mAttachState);
    otLogInfoMle("AttachState %s -> %s", AttachStateToString(mAttachState), AttachStateToString(aState));
    mAttachState = aState;

exit:
    return;
}

otError Mle::Restore(void)
{
    otError               error = OT_ERROR_NONE;
    Settings::NetworkInfo networkInfo;
    Settings::ParentInfo  parentInfo;

    Get<MeshCoP::ActiveDataset>().Restore();
    Get<MeshCoP::PendingDataset>().Restore();

    SuccessOrExit(error = Get<Settings>().ReadNetworkInfo(networkInfo));

    Get<KeyManager>().SetCurrentKeySequence(networkInfo.GetKeySequence());
    Get<KeyManager>().SetMleFrameCounter(networkInfo.GetMleFrameCounter());
    Get<KeyManager>().SetMacFrameCounter(networkInfo.GetMacFrameCounter());
    mDeviceMode.Set(networkInfo.GetDeviceMode());

    switch (networkInfo.GetRole())
    {
    case OT_DEVICE_ROLE_CHILD:
    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        break;

    default:
        ExitNow();
    }

    Get<Mac::Mac>().SetShortAddress(networkInfo.GetRloc16());
    Get<Mac::Mac>().SetExtAddress(networkInfo.GetExtAddress());

    memcpy(&mMeshLocal64.GetAddress().mFields.m8[OT_IP6_PREFIX_SIZE], networkInfo.GetMeshLocalIid(),
           OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);

    if (networkInfo.GetRloc16() == Mac::kShortAddrInvalid)
    {
        ExitNow();
    }

    if (!IsActiveRouter(networkInfo.GetRloc16()))
    {
        error = Get<Settings>().ReadParentInfo(parentInfo);

        if (error != OT_ERROR_NONE)
        {
            // If the restored RLOC16 corresponds to an end-device, it
            // is expected that the `ParentInfo` settings to be valid
            // as well. The device can still recover from such an invalid
            // setting by skipping the re-attach ("Child Update Request"
            // exchange) and going through the full attach process.

            otLogWarnMle("Invalid settings - no saved parent info with valid end-device RLOC16 0x%04x",
                         networkInfo.GetRloc16());
            ExitNow();
        }

        mParent.Clear();
        mParent.SetExtAddress(parentInfo.GetExtAddress());
        mParent.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                         DeviceMode::kModeFullNetworkData | DeviceMode::kModeSecureDataRequest));
        mParent.SetRloc16(Rloc16FromRouterId(RouterIdFromRloc16(networkInfo.GetRloc16())));
        mParent.SetState(Neighbor::kStateRestored);

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
        mPreviousParentRloc = mParent.GetRloc16();
#endif
    }
    else
    {
        Get<MleRouter>().SetRouterId(RouterIdFromRloc16(GetRloc16()));
        Get<MleRouter>().SetPreviousPartitionId(networkInfo.GetPreviousPartitionId());
        Get<MleRouter>().RestoreChildren();
    }

exit:
    return error;
}

otError Mle::Store(void)
{
    otError               error = OT_ERROR_NONE;
    Settings::NetworkInfo networkInfo;

    networkInfo.Init();

    if (IsAttached())
    {
        // Only update network information while we are attached to
        // avoid losing/overwriting previous information when a reboot
        // occurs after a message is sent but before attaching.

        networkInfo.SetRole(mRole);
        networkInfo.SetRloc16(GetRloc16());
        networkInfo.SetPreviousPartitionId(mLeaderData.GetPartitionId());
        networkInfo.SetExtAddress(Get<Mac::Mac>().GetExtAddress());
        networkInfo.SetMeshLocalIid(&mMeshLocal64.GetAddress().mFields.m8[OT_IP6_PREFIX_SIZE]);

        if (mRole == OT_DEVICE_ROLE_CHILD)
        {
            Settings::ParentInfo parentInfo;

            parentInfo.Init();
            parentInfo.SetExtAddress(mParent.GetExtAddress());

            SuccessOrExit(error = Get<Settings>().SaveParentInfo(parentInfo));
        }
    }
    else
    {
        // When not attached, read out any previous saved `NetworkInfo`.
        // If there is none, it indicates that device was never attached
        // before. In that case, no need to save any info (note that on
        // a device reset the MLE/MAC frame counters would reset but
        // device also starts with a new randomly generated extended
        // address. If there is a previously saved `NetworkInfo`, we
        // just update the key sequence and MAC and MLE frame counters.

        SuccessOrExit(Get<Settings>().ReadNetworkInfo(networkInfo));
    }

    networkInfo.SetKeySequence(Get<KeyManager>().GetCurrentKeySequence());
    networkInfo.SetMleFrameCounter(Get<KeyManager>().GetMleFrameCounter() +
                                   OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD);
    networkInfo.SetMacFrameCounter(Get<KeyManager>().GetMacFrameCounter() +
                                   OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD);
    networkInfo.SetDeviceMode(mDeviceMode.Get());

    SuccessOrExit(error = Get<Settings>().SaveNetworkInfo(networkInfo));

    Get<KeyManager>().SetStoredMleFrameCounter(networkInfo.GetMleFrameCounter());
    Get<KeyManager>().SetStoredMacFrameCounter(networkInfo.GetMacFrameCounter());

    otLogDebgMle("Store Network Information");

exit:
    return error;
}

otError Mle::Discover(const Mac::ChannelMask &aScanChannels,
                      uint16_t                aPanId,
                      bool                    aJoiner,
                      bool                    aEnableFiltering,
                      DiscoverHandler         aCallback,
                      void *                  aContext)
{
    otError                      error   = OT_ERROR_NONE;
    Message *                    message = NULL;
    Ip6::Address                 destination;
    Tlv                          tlv;
    MeshCoP::DiscoveryRequestTlv discoveryRequest;
    uint16_t                     startOffset;

    VerifyOrExit(!mDiscoverInProgress, error = OT_ERROR_BUSY);

    mDiscoverEnableFiltering = aEnableFiltering;

    if (mDiscoverEnableFiltering)
    {
        Mac::ExtAddress extAddress;
        Crc16           ccitt(Crc16::kCcitt);
        Crc16           ansi(Crc16::kAnsi);

        Get<Radio>().GetIeeeEui64(extAddress);
        MeshCoP::ComputeJoinerId(extAddress, extAddress);

        // Compute bloom filter (for steering data)
        for (size_t i = 0; i < sizeof(extAddress.m8); i++)
        {
            ccitt.Update(extAddress.m8[i]);
            ansi.Update(extAddress.m8[i]);
        }

        mDiscoverCcittIndex = ccitt.Get();
        mDiscoverAnsiIndex  = ansi.Get();
    }

    mDiscoverHandler = aCallback;
    mDiscoverContext = aContext;
    Get<MeshForwarder>().SetDiscoverParameters(aScanChannels);

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
    discoveryRequest.SetVersion(kThreadVersion);
    discoveryRequest.SetJoiner(aJoiner);
    SuccessOrExit(error = discoveryRequest.AppendTo(*message));

    tlv.SetLength(static_cast<uint8_t>(message->GetLength() - startOffset));
    message->Write(startOffset - sizeof(tlv), sizeof(tlv), &tlv);

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0002);
    SuccessOrExit(error = SendMessage(*message, destination));

    mDiscoverInProgress = true;

    LogMleMessage("Send Discovery Request", destination);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Mle::HandleDiscoverComplete(void)
{
    mDiscoverInProgress      = false;
    mDiscoverEnableFiltering = false;
    mDiscoverHandler(NULL, mDiscoverContext);
}

otError Mle::BecomeDetached(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);

    // In case role is already detached and attach state is `kAttachStateStart`
    // (i.e., waiting to start an attach attempt), there is no need to make any
    // changes.

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DETACHED || mAttachState != kAttachStateStart);

    // not in reattach stage after reset
    if (mReattachState == kReattachStop)
    {
        Get<MeshCoP::PendingDataset>().HandleDetach();
    }

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
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
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(!IsAttaching(), error = OT_ERROR_BUSY);

    if (mReattachState == kReattachStart)
    {
        if (Get<MeshCoP::ActiveDataset>().Restore() == OT_ERROR_NONE)
        {
            mReattachState = kReattachActive;
        }
        else
        {
            mReattachState = kReattachStop;
        }
    }

    mParentCandidate.Clear();
    SetAttachState(kAttachStateStart);
    mParentRequestMode = aMode;

    if (aMode != kAttachBetter)
    {
        if (IsFullThreadDevice())
        {
            Get<MleRouter>().StopAdvertiseTimer();
        }
    }
    else
    {
        mCounters.mBetterPartitionAttachAttempts++;
    }

    mAttachTimer.Start(GetAttachStartDelay());

    if (mRole == OT_DEVICE_ROLE_DETACHED)
    {
        mAttachCounter++;

        if (mAttachCounter == 0)
        {
            mAttachCounter--;
        }

        mCounters.mAttachAttempts++;

        if (!IsRxOnWhenIdle())
        {
            Get<Mac::Mac>().SetRxOnWhenIdle(false);
        }
    }

exit:
    return error;
}

uint32_t Mle::GetAttachStartDelay(void) const
{
    uint32_t delay = 1;
    uint32_t jitter;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_DETACHED);

    if (mAttachCounter == 0)
    {
        delay = 1 + Random::NonCrypto::GetUint32InRange(0, kParentRequestRouterTimeout);
        ExitNow();
    }
#if OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_ENABLE
    else
    {
        uint16_t       counter = mAttachCounter - 1;
        const uint32_t ratio   = kAttachBackoffMaxInterval / kAttachBackoffMinInterval;

        if ((counter < sizeof(ratio) * CHAR_BIT) && ((1UL << counter) <= ratio))
        {
            delay = kAttachBackoffMinInterval;
            delay <<= counter;
        }
        else
        {
            delay = Random::NonCrypto::AddJitter(kAttachBackoffMaxInterval, kAttachBackoffJitter);
        }
    }
#endif // OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_ENABLE

    jitter = Random::NonCrypto::GetUint32InRange(0, kAttachStartJitter);

    if (jitter + delay > delay) // check for overflow
    {
        delay += jitter;
    }

    otLogNoteMle("Attach attempt %d unsuccessful, will try again in %u.%03u seconds", mAttachCounter, delay / 1000,
                 delay % 1000);

exit:
    return delay;
}

bool Mle::IsAttached(void) const
{
    return (mRole == OT_DEVICE_ROLE_CHILD || mRole == OT_DEVICE_ROLE_ROUTER || mRole == OT_DEVICE_ROLE_LEADER);
}

void Mle::SetStateDetached(void)
{
    if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mLeaderAloc);
    }

    SetRole(OT_DEVICE_ROLE_DETACHED);
    SetAttachState(kAttachStateIdle);
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    mChildUpdateRequestState = kChildUpdateRequestNone;
    mChildUpdateAttempts     = 0;
    mDataRequestState        = kDataRequestNone;
    mDataRequestAttempts     = 0;
    Get<MeshForwarder>().SetRxOnWhenIdle(true);
    Get<Mac::Mac>().SetBeaconEnabled(false);
    Get<MleRouter>().HandleDetachStart();
    Get<Ip6::Ip6>().SetForwardingEnabled(false);
    Get<Ip6::Mpl>().SetTimerExpirations(0);
}

void Mle::SetStateChild(uint16_t aRloc16)
{
    if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mLeaderAloc);
    }

    SetRloc16(aRloc16);
    SetRole(OT_DEVICE_ROLE_CHILD);
    SetAttachState(kAttachStateIdle);
    mAttachTimer.Stop();
    mAttachCounter       = 0;
    mReattachState       = kReattachStop;
    mChildUpdateAttempts = 0;
    mDataRequestAttempts = 0;
    Get<Mac::Mac>().SetBeaconEnabled(false);
    ScheduleMessageTransmissionTimer();

    if (IsFullThreadDevice())
    {
        Get<MleRouter>().HandleChildStart(mParentRequestMode);
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    Get<NetworkData::Local>().ClearResubmitDelayTimer();
#endif
    Get<Ip6::Ip6>().SetForwardingEnabled(false);
    Get<Ip6::Mpl>().SetTimerExpirations(kMplChildDataMessageTimerExpirations);

    // send announce after attached if needed
    InformPreviousChannel();

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    UpdateParentSearchState();
#endif

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
    InformPreviousParent();
    mPreviousParentRloc = mParent.GetRloc16();
#endif
}

void Mle::InformPreviousChannel(void)
{
    VerifyOrExit(mAlternatePanId != Mac::kPanIdBroadcast);
    VerifyOrExit(mRole == OT_DEVICE_ROLE_CHILD || mRole == OT_DEVICE_ROLE_ROUTER);

    if (!IsFullThreadDevice() || mRole == OT_DEVICE_ROLE_ROUTER ||
        Get<MleRouter>().GetRouterSelectionJitterTimeout() == 0)
    {
        mAlternatePanId = Mac::kPanIdBroadcast;
        Get<AnnounceBeginServer>().SendAnnounce(1 << mAlternateChannel);
    }

exit:
    return;
}

void Mle::SetTimeout(uint32_t aTimeout)
{
    VerifyOrExit(mTimeout != aTimeout);

    if (aTimeout < kMinTimeout)
    {
        aTimeout = kMinTimeout;
    }

    mTimeout = aTimeout;

    Get<DataPollSender>().RecalculatePollPeriod();

    if (mRole == OT_DEVICE_ROLE_CHILD)
    {
        SendChildUpdateRequest();
    }

exit:
    return;
}

otError Mle::SetDeviceMode(DeviceMode aDeviceMode)
{
    otError    error   = OT_ERROR_NONE;
    DeviceMode oldMode = mDeviceMode;

    VerifyOrExit(aDeviceMode.IsValid(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mDeviceMode != aDeviceMode);
    mDeviceMode = aDeviceMode;

    otLogNoteMle("Mode 0x%02x -> 0x%02x [%s]", oldMode.Get(), mDeviceMode.Get(), mDeviceMode.ToString().AsCString());

    Store();

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
        break;

    case OT_DEVICE_ROLE_DETACHED:
        mAttachCounter = 0;
        SetStateDetached();
        BecomeChild(kAttachAny);
        break;

    case OT_DEVICE_ROLE_CHILD:
        SetStateChild(GetRloc16());
        SendChildUpdateRequest();
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        if (oldMode.IsFullThreadDevice() && !mDeviceMode.IsFullThreadDevice())
        {
            BecomeDetached();
        }

        break;
    }

exit:
    return error;
}

void Mle::UpdateLinkLocalAddress(void)
{
    Get<ThreadNetif>().RemoveUnicastAddress(mLinkLocal64);
    mLinkLocal64.GetAddress().SetIid(Get<Mac::Mac>().GetExtAddress());
    Get<ThreadNetif>().AddUnicastAddress(mLinkLocal64);

    Get<Notifier>().Signal(OT_CHANGED_THREAD_LL_ADDR);
}

void Mle::SetMeshLocalPrefix(const otMeshLocalPrefix &aMeshLocalPrefix)
{
    if (memcmp(mMeshLocal64.GetAddress().mFields.m8, aMeshLocalPrefix.m8, sizeof(aMeshLocalPrefix)) == 0)
    {
        Get<Notifier>().SignalIfFirst(OT_CHANGED_THREAD_ML_ADDR);
        ExitNow();
    }

    if (Get<ThreadNetif>().IsUp())
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mLeaderAloc);
        // We must remove the old addresses before adding the new ones.
        Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal64);
        Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal16);
        Get<ThreadNetif>().UnsubscribeMulticast(mLinkLocalAllThreadNodes);
        Get<ThreadNetif>().UnsubscribeMulticast(mRealmLocalAllThreadNodes);
    }

    memcpy(mMeshLocal64.GetAddress().mFields.m8, aMeshLocalPrefix.m8, sizeof(aMeshLocalPrefix));
    memcpy(mMeshLocal16.GetAddress().mFields.m8, aMeshLocalPrefix.m8, sizeof(aMeshLocalPrefix));
    memcpy(mLeaderAloc.GetAddress().mFields.m8, aMeshLocalPrefix.m8, sizeof(aMeshLocalPrefix));

    // Just keep mesh local prefix if network interface is down
    VerifyOrExit(Get<ThreadNetif>().IsUp());

    ApplyMeshLocalPrefix();

exit:
    return;
}

void Mle::ApplyMeshLocalPrefix(void)
{
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

    for (uint8_t i = 0; i < OT_ARRAY_LENGTH(mServiceAlocs); i++)
    {
        if (HostSwap16(mServiceAlocs[i].GetAddress().mFields.m16[7]) != Mac::kShortAddrInvalid)
        {
            Get<ThreadNetif>().RemoveUnicastAddress(mServiceAlocs[i]);
            memcpy(mServiceAlocs[i].GetAddress().mFields.m8, mMeshLocal64.GetAddress().mFields.m8, 8);
            Get<ThreadNetif>().AddUnicastAddress(mServiceAlocs[i]);
        }
    }

#endif

    mLinkLocalAllThreadNodes.GetAddress().mFields.m8[3] = 64;
    memcpy(mLinkLocalAllThreadNodes.GetAddress().mFields.m8 + 4, mMeshLocal64.GetAddress().mFields.m8, 8);

    mRealmLocalAllThreadNodes.GetAddress().mFields.m8[3] = 64;
    memcpy(mRealmLocalAllThreadNodes.GetAddress().mFields.m8 + 4, mMeshLocal64.GetAddress().mFields.m8, 8);

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED);

    // Add the addresses back into the table.
    Get<ThreadNetif>().AddUnicastAddress(mMeshLocal64);
    Get<ThreadNetif>().SubscribeMulticast(mLinkLocalAllThreadNodes);
    Get<ThreadNetif>().SubscribeMulticast(mRealmLocalAllThreadNodes);

    if (IsAttached())
    {
        Get<ThreadNetif>().AddUnicastAddress(mMeshLocal16);
    }

    // update Leader ALOC
    if (mRole == OT_DEVICE_ROLE_LEADER)
    {
        Get<ThreadNetif>().AddUnicastAddress(mLeaderAloc);
    }

exit:
    // Changing the prefix also causes the mesh local address to be different.
    Get<Notifier>().Signal(OT_CHANGED_THREAD_ML_ADDR);
}

uint16_t Mle::GetRloc16(void) const
{
    return Get<Mac::Mac>().GetShortAddress();
}

void Mle::SetRloc16(uint16_t aRloc16)
{
    uint16_t oldRloc16 = GetRloc16();

    if (aRloc16 != oldRloc16)
    {
        otLogNoteMle("RLOC16 %04x -> %04x", oldRloc16, aRloc16);
    }

    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal16);

    Get<Mac::Mac>().SetShortAddress(aRloc16);
    Get<Ip6::Mpl>().SetSeedId(aRloc16);

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        // mesh-local 16
        mMeshLocal16.GetAddress().mFields.m16[7] = HostSwap16(aRloc16);
        Get<ThreadNetif>().AddUnicastAddress(mMeshLocal16);
#if OPENTHREAD_FTD
        Get<AddressResolver>().RestartAddressQueries();
#endif
    }
}

void Mle::SetLeaderData(uint32_t aPartitionId, uint8_t aWeighting, uint8_t aLeaderRouterId)
{
    if (mLeaderData.GetPartitionId() != aPartitionId)
    {
        Get<MleRouter>().HandlePartitionChange();
        Get<Notifier>().Signal(OT_CHANGED_THREAD_PARTITION_ID);
        mCounters.mPartitionIdChanges++;
    }
    else
    {
        Get<Notifier>().SignalIfFirst(OT_CHANGED_THREAD_PARTITION_ID);
    }

    mLeaderData.SetPartitionId(aPartitionId);
    mLeaderData.SetWeighting(aWeighting);
    mLeaderData.SetLeaderRouterId(aLeaderRouterId);
}

otError Mle::GetLeaderAddress(Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 8);
    aAddress.mFields.m16[4] = HostSwap16(0x0000);
    aAddress.mFields.m16[5] = HostSwap16(0x00ff);
    aAddress.mFields.m16[6] = HostSwap16(0xfe00);
    aAddress.mFields.m16[7] = HostSwap16(Rloc16FromRouterId(mLeaderData.GetLeaderRouterId()));

exit:
    return error;
}

otError Mle::GetAlocAddress(Ip6::Address &aAddress, uint16_t aAloc16) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 14);
    aAddress.mFields.m16[7] = HostSwap16(aAloc16);

exit:
    return error;
}

otError Mle::GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 8);
    aAddress.mFields.m16[4] = HostSwap16(0x0000);
    aAddress.mFields.m16[5] = HostSwap16(0x00ff);
    aAddress.mFields.m16[6] = HostSwap16(0xfe00);
    aAddress.mFields.m16[7] = HostSwap16(ServiceAlocFromId(aServiceId));

exit:
    return error;
}

otError Mle::AddLeaderAloc(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_LEADER, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = GetLeaderAloc(mLeaderAloc.GetAddress()));

    error = Get<ThreadNetif>().AddUnicastAddress(mLeaderAloc);

exit:
    return error;
}

const LeaderDataTlv &Mle::GetLeaderDataTlv(void)
{
    mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion());
    mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetStableVersion());

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

Message *Mle::NewMleMessage(void)
{
    Message *         message;
    otMessageSettings settings = {false, static_cast<otMessagePriority>(kMleMessagePriority)};

    message = mSocket.NewMessage(0, &settings);
    VerifyOrExit(message != NULL);

    message->SetSubType(Message::kSubTypeMleGeneral);

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

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendStatus(Message &aMessage, StatusTlv::Status aStatus)
{
    StatusTlv tlv;

    tlv.Init();
    tlv.SetStatus(aStatus);

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendMode(Message &aMessage, DeviceMode aMode)
{
    ModeTlv tlv;

    tlv.Init();
    tlv.SetMode(aMode);

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendTimeout(Message &aMessage, uint32_t aTimeout)
{
    TimeoutTlv tlv;

    tlv.Init();
    tlv.SetTimeout(aTimeout);

    return tlv.AppendTo(aMessage);
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
    tlv.SetFrameCounter(Get<KeyManager>().GetMacFrameCounter());

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendMleFrameCounter(Message &aMessage)
{
    MleFrameCounterTlv tlv;

    tlv.Init();
    tlv.SetFrameCounter(Get<KeyManager>().GetMleFrameCounter());

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendAddress16(Message &aMessage, uint16_t aRloc16)
{
    Address16Tlv tlv;

    tlv.Init();
    tlv.SetRloc16(aRloc16);

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendLeaderData(Message &aMessage)
{
    mLeaderData.Init();
    mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion());
    mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetStableVersion());

    return mLeaderData.AppendTo(aMessage);
}

void Mle::FillNetworkDataTlv(NetworkDataTlv &aTlv, bool aStableOnly)
{
    uint8_t length = sizeof(NetworkDataTlv) - sizeof(Tlv); // sizeof( NetworkDataTlv::mNetworkData )

    // Ignore result code, provided buffer must be enough
    Get<NetworkData::Leader>().GetNetworkData(aStableOnly, aTlv.GetNetworkData(), length);
    aTlv.SetLength(length);
}

otError Mle::AppendNetworkData(Message &aMessage, bool aStableOnly)
{
    otError        error = OT_ERROR_NONE;
    NetworkDataTlv tlv;

    VerifyOrExit(!mRetrieveNewNetworkData, error = OT_ERROR_INVALID_STATE);

    tlv.Init();
    FillNetworkDataTlv(tlv, aStableOnly);

    error = tlv.AppendTo(aMessage);

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

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendLinkMargin(Message &aMessage, uint8_t aLinkMargin)
{
    LinkMarginTlv tlv;

    tlv.Init();
    tlv.SetLinkMargin(aLinkMargin);

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendVersion(Message &aMessage)
{
    VersionTlv tlv;

    tlv.Init();
    tlv.SetVersion(kThreadVersion);

    return tlv.AppendTo(aMessage);
}

bool Mle::HasUnregisteredAddress(void)
{
    bool retval = false;

    // Checks whether there are any addresses in addition to the mesh-local
    // address that need to be registered.

    for (const Ip6::NetifUnicastAddress *addr = Get<ThreadNetif>().GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        if (!addr->GetAddress().IsLinkLocal() && !IsRoutingLocator(addr->GetAddress()) &&
            !IsAnycastLocator(addr->GetAddress()) && addr->GetAddress() != GetMeshLocal64())
        {
            ExitNow(retval = true);
        }
    }

    if (!IsRxOnWhenIdle())
    {
        uint8_t      iterator = 0;
        Ip6::Address address;

        // For sleepy end-device, we register any external multicast
        // addresses.

        retval = (Get<ThreadNetif>().GetNextExternalMulticast(iterator, address) == OT_ERROR_NONE);
    }

exit:
    return retval;
}

otError Mle::AppendAddressRegistration(Message &aMessage, AddressRegistrationMode aMode)
{
    otError                  error = OT_ERROR_NONE;
    Tlv                      tlv;
    AddressRegistrationEntry entry;
    Lowpan::Context          context;
    uint8_t                  length      = 0;
    uint8_t                  counter     = 0;
    uint16_t                 startOffset = aMessage.GetLength();

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    // Prioritize ML-EID
    entry.SetContextId(kMeshLocalPrefixContextId);
    entry.SetIid(GetMeshLocal64().GetIid());
    SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
    length += entry.GetLength();

    // Continue to append the other addresses if not `kAppendMeshLocalOnly` mode
    VerifyOrExit(aMode != kAppendMeshLocalOnly);
    counter++;

    for (const Ip6::NetifUnicastAddress *addr = Get<ThreadNetif>().GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        if (addr->GetAddress().IsLinkLocal() || IsRoutingLocator(addr->GetAddress()) ||
            IsAnycastLocator(addr->GetAddress()) || addr->GetAddress() == GetMeshLocal64())
        {
            continue;
        }

        if (Get<NetworkData::Leader>().GetContext(addr->GetAddress(), context) == OT_ERROR_NONE)
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
        VerifyOrExit(counter < OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER);
    }

    // For sleepy end device, register external multicast addresses to the parent for indirect transmission
    if (!IsRxOnWhenIdle())
    {
        uint8_t      iterator = 0;
        Ip6::Address address;

        // append external multicast address
        while (Get<ThreadNetif>().GetNextExternalMulticast(iterator, address) == OT_ERROR_NONE)
        {
            entry.SetUncompressed();
            entry.SetIp6Address(address);
            SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
            length += entry.GetLength();

            counter++;
            // only continue to append if there is available entry.
            VerifyOrExit(counter < OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER);
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

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
otError Mle::AppendTimeRequest(Message &aMessage)
{
    TimeRequestTlv tlv;

    tlv.Init();

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendTimeParameter(Message &aMessage)
{
    TimeParameterTlv tlv;

    tlv.Init();
    tlv.SetTimeSyncPeriod(Get<TimeSync>().GetTimeSyncPeriod());
    tlv.SetXtalThreshold(Get<TimeSync>().GetXtalThreshold());

    return tlv.AppendTo(aMessage);
}

otError Mle::AppendXtalAccuracy(Message &aMessage)
{
    XtalAccuracyTlv tlv;

    tlv.Init();
    tlv.SetXtalAccuracy(otPlatTimeGetXtalAccuracy());

    return tlv.AppendTo(aMessage);
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

otError Mle::AppendActiveTimestamp(Message &aMessage)
{
    otError                   error;
    ActiveTimestampTlv        timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    timestamp = Get<MeshCoP::ActiveDataset>().GetTimestamp();
    VerifyOrExit(timestamp, error = OT_ERROR_NONE);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = *timestamp;
    error                                             = timestampTlv.AppendTo(aMessage);

exit:
    return error;
}

otError Mle::AppendPendingTimestamp(Message &aMessage)
{
    otError                   error;
    PendingTimestampTlv       timestampTlv;
    const MeshCoP::Timestamp *timestamp;

    timestamp = Get<MeshCoP::PendingDataset>().GetTimestamp();
    VerifyOrExit(timestamp && timestamp->GetSeconds() != 0, error = OT_ERROR_NONE);

    timestampTlv.Init();
    *static_cast<MeshCoP::Timestamp *>(&timestampTlv) = *timestamp;
    error                                             = timestampTlv.AppendTo(aMessage);

exit:
    return error;
}

void Mle::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<Mle>().HandleStateChanged(aFlags);
}

void Mle::HandleStateChanged(otChangedFlags aFlags)
{
    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED);

    if (aFlags & OT_CHANGED_THREAD_ROLE)
    {
        if (mRole == OT_DEVICE_ROLE_CHILD && !IsFullThreadDevice() && mAddressRegistrationMode == kAppendMeshLocalOnly)
        {
            // If only mesh-local address was registered in the "Child
            // ID Request" message, after device is attached, trigger a
            // "Child Update Request" to register the remaining
            // addresses.

            mAddressRegistrationMode = kAppendAllAddresses;
            mChildUpdateRequestState = kChildUpdateRequestPending;
            ScheduleMessageTransmissionTimer();
        }
    }

    if ((aFlags & (OT_CHANGED_IP6_ADDRESS_ADDED | OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0)
    {
        if (!Get<ThreadNetif>().IsUnicastAddress(mMeshLocal64.GetAddress()))
        {
            // Mesh Local EID was removed, choose a new one and add it back
            Random::NonCrypto::FillBuffer(mMeshLocal64.GetAddress().mFields.m8 + OT_IP6_PREFIX_SIZE,
                                          OT_IP6_ADDRESS_SIZE - OT_IP6_PREFIX_SIZE);

            Get<ThreadNetif>().AddUnicastAddress(mMeshLocal64);
            Get<Notifier>().Signal(OT_CHANGED_THREAD_ML_ADDR);
        }

        if (mRole == OT_DEVICE_ROLE_CHILD && !IsFullThreadDevice())
        {
            mChildUpdateRequestState = kChildUpdateRequestPending;
            ScheduleMessageTransmissionTimer();
        }
    }

    if ((aFlags & (OT_CHANGED_IP6_MULTICAST_SUBSCRIBED | OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED)) != 0)
    {
        if (mRole == OT_DEVICE_ROLE_CHILD && !IsFullThreadDevice() && !IsRxOnWhenIdle())
        {
            mChildUpdateRequestState = kChildUpdateRequestPending;
            ScheduleMessageTransmissionTimer();
        }
    }

    if ((aFlags & OT_CHANGED_THREAD_NETDATA) != 0)
    {
        if (IsFullThreadDevice())
        {
            Get<MleRouter>().HandleNetworkDataUpdateRouter();
        }
        else if ((aFlags & OT_CHANGED_THREAD_ROLE) == 0)
        {
            mChildUpdateRequestState = kChildUpdateRequestPending;
            ScheduleMessageTransmissionTimer();
        }

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
        Get<NetworkData::Local>().SendServerDataNotification();
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
        this->UpdateServiceAlocs();
#endif
#endif

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
        Get<Dhcp6::Dhcp6Server>().UpdateService();
#endif // OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
        Get<Dhcp6::Dhcp6Client>().UpdateAddresses();
#endif // OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
    }

    if (aFlags & (OT_CHANGED_THREAD_ROLE | OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER))
    {
        // Store the settings on a key seq change, or when role changes and device
        // is attached (i.e., skip `Store()` on role change to detached).

        if ((aFlags & OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER) || IsAttached())
        {
            Store();
        }
    }

    if (aFlags & OT_CHANGED_SECURITY_POLICY)
    {
        Get<Ip6::Filter>().AllowNativeCommissioner(
            (Get<KeyManager>().GetSecurityPolicyFlags() & OT_SECURITY_POLICY_NATIVE_COMMISSIONING) != 0);
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
void Mle::UpdateServiceAlocs(void)
{
    uint16_t              rloc               = GetRloc16();
    uint16_t              serviceAloc        = 0;
    uint8_t               serviceId          = 0;
    int                   i                  = 0;
    NetworkData::Iterator serviceIterator    = NetworkData::kIteratorInit;
    int                   serviceAlocsLength = OT_ARRAY_LENGTH(mServiceAlocs);

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED);

    // First remove all alocs which are no longer necessary, to free up space in mServiceAlocs
    for (i = 0; i < serviceAlocsLength; i++)
    {
        serviceAloc = HostSwap16(mServiceAlocs[i].GetAddress().mFields.m16[7]);

        if ((serviceAloc != Mac::kShortAddrInvalid) &&
            (!Get<NetworkData::Leader>().ContainsService(Mle::ServiceIdFromAloc(serviceAloc), rloc)))
        {
            Get<ThreadNetif>().RemoveUnicastAddress(mServiceAlocs[i]);
            mServiceAlocs[i].GetAddress().mFields.m16[7] = HostSwap16(Mac::kShortAddrInvalid);
        }
    }

    // Now add any missing service alocs which should be there, if there is enough space in mServiceAlocs
    while (Get<NetworkData::Leader>().GetNextServiceId(serviceIterator, rloc, serviceId) == OT_ERROR_NONE)
    {
        for (i = 0; i < serviceAlocsLength; i++)
        {
            serviceAloc = HostSwap16(mServiceAlocs[i].GetAddress().mFields.m16[7]);

            if ((serviceAloc != Mac::kShortAddrInvalid) && (Mle::ServiceIdFromAloc(serviceAloc) == serviceId))
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
                    Get<ThreadNetif>().AddUnicastAddress(mServiceAlocs[i]);
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
    uint32_t delay          = 0;
    bool     shouldAnnounce = true;

    if (mAttachState == kAttachStateParentRequestRouter || mAttachState == kAttachStateParentRequestReed ||
        mAttachState == kAttachStateAnnounce)
    {
        uint8_t linkQuality;

        linkQuality = mParentCandidate.GetLinkInfo().GetLinkQuality();

        if (linkQuality > mParentCandidate.GetLinkQualityOut())
        {
            linkQuality = mParentCandidate.GetLinkQualityOut();
        }

        // If already attached, accept the parent candidate if
        // we are trying to attach to a better partition or if a
        // Parent Response was also received from the current parent
        // to which the device is attached. This ensures that the
        // new parent candidate is compared with the current parent
        // and that it is indeed preferred over the current one.
        // If we are in kAttachStateParentRequestRouter and cannot
        // find a parent with best link quality(3), we will keep
        // the candidate and forward to REED stage to find a better
        // parent.

        if ((linkQuality == 3 || mAttachState != kAttachStateParentRequestRouter) &&
            mParentCandidate.IsStateParentResponse() &&
            (mRole != OT_DEVICE_ROLE_CHILD || mReceivedResponseFromParent || mParentRequestMode == kAttachBetter) &&
            SendChildIdRequest() == OT_ERROR_NONE)
        {
            SetAttachState(kAttachStateChildIdRequest);
            delay = kParentRequestReedTimeout;
            ExitNow();
        }
    }

    switch (mAttachState)
    {
    case kAttachStateIdle:
        assert(false);
        break;

    case kAttachStateProcessAnnounce:
        ProcessAnnounce();
        break;

    case kAttachStateStart:
        if (mAttachCounter > 0)
        {
            otLogNoteMle("Attempt to attach - attempt %d, %s %s", mAttachCounter,
                         AttachModeToString(mParentRequestMode), ReattachStateToString(mReattachState));
        }
        else
        {
            otLogNoteMle("Attempt to attach - %s %s", AttachModeToString(mParentRequestMode),
                         ReattachStateToString(mReattachState));
        }

        SetAttachState(kAttachStateParentRequestRouter);
        mParentCandidate.SetState(Neighbor::kStateInvalid);
        mReceivedResponseFromParent = false;
        Get<MeshForwarder>().SetRxOnWhenIdle(true);

        // initial MLE Parent Request has both E and R flags set in Scan Mask TLV
        // during reattach when losing connectivity.
        if (mParentRequestMode == kAttachSame1 || mParentRequestMode == kAttachSame2)
        {
            SendParentRequest(kParentRequestTypeRoutersAndReeds);
            delay = kParentRequestReedTimeout;
        }
        // initial MLE Parent Request has only R flag set in Scan Mask TLV for
        // during initial attach or downgrade process
        else
        {
            SendParentRequest(kParentRequestTypeRouters);
            delay = kParentRequestRouterTimeout;
        }

        break;

    case kAttachStateParentRequestRouter:
        SetAttachState(kAttachStateParentRequestReed);
        SendParentRequest(kParentRequestTypeRoutersAndReeds);
        delay = kParentRequestReedTimeout;
        break;

    case kAttachStateParentRequestReed:
        shouldAnnounce = PrepareAnnounceState();

        if (shouldAnnounce)
        {
            SetAttachState(kAttachStateAnnounce);
            SendParentRequest(kParentRequestTypeRoutersAndReeds);
            mAnnounceChannel = Mac::ChannelMask::kChannelIteratorFirst;
            delay            = mAnnounceDelay;
            break;
        }

        // fall through

    case kAttachStateAnnounce:
        if (shouldAnnounce)
        {
            if (SendOrphanAnnounce() == OT_ERROR_NONE)
            {
                delay = mAnnounceDelay;
                break;
            }
        }

        // fall through

    case kAttachStateChildIdRequest:
        SetAttachState(kAttachStateIdle);
        mParentCandidate.Clear();
        delay = Reattach();
        break;
    }

exit:

    if (delay != 0)
    {
        mAttachTimer.Start(delay);
    }
}

bool Mle::PrepareAnnounceState(void)
{
    bool             shouldAnnounce = false;
    Mac::ChannelMask channelMask;

    VerifyOrExit((mRole != OT_DEVICE_ROLE_CHILD) && (mReattachState == kReattachStop) &&
                 (Get<MeshCoP::ActiveDataset>().IsPartiallyComplete() || !IsFullThreadDevice()));

    if (Get<MeshCoP::ActiveDataset>().GetChannelMask(channelMask) != OT_ERROR_NONE)
    {
        channelMask = Get<Mac::Mac>().GetSupportedChannelMask();
    }

    mAnnounceDelay = kAnnounceTimeout / (channelMask.GetNumberOfChannels() + 1);

    if (mAnnounceDelay < kMinAnnounceDelay)
    {
        mAnnounceDelay = kMinAnnounceDelay;
    }

    shouldAnnounce = true;

exit:
    return shouldAnnounce;
}

uint32_t Mle::Reattach(void)
{
    uint32_t delay = 0;

    if (mReattachState == kReattachActive)
    {
        if (Get<MeshCoP::PendingDataset>().Restore() == OT_ERROR_NONE)
        {
            Get<MeshCoP::PendingDataset>().ApplyConfiguration();
            mReattachState = kReattachPending;
            SetAttachState(kAttachStateStart);
            delay = 1 + Random::NonCrypto::GetUint32InRange(0, kAttachStartJitter);
        }
        else
        {
            mReattachState = kReattachStop;
        }
    }
    else if (mReattachState == kReattachPending)
    {
        mReattachState = kReattachStop;
        Get<MeshCoP::ActiveDataset>().Restore();
    }

    VerifyOrExit(mReattachState == kReattachStop);

    switch (mParentRequestMode)
    {
    case kAttachAny:
        if (mRole != OT_DEVICE_ROLE_CHILD)
        {
            if (mAlternatePanId != Mac::kPanIdBroadcast)
            {
                Get<Mac::Mac>().SetPanChannel(mAlternateChannel);
                Get<Mac::Mac>().SetPanId(mAlternatePanId);
                mAlternatePanId = Mac::kPanIdBroadcast;
                BecomeDetached();
            }
            else if (!IsFullThreadDevice())
            {
                BecomeDetached();
            }
            else if (Get<MleRouter>().BecomeLeader() != OT_ERROR_NONE)
            {
                BecomeDetached();
            }
        }
        else if (!IsRxOnWhenIdle())
        {
            // return to sleepy operation
            Get<DataPollSender>().SetAttachMode(false);
            Get<MeshForwarder>().SetRxOnWhenIdle(false);
        }

        break;

    case kAttachSame1:
        BecomeChild(kAttachSame2);
        break;

    case kAttachSame2:
    case kAttachSameDowngrade:
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
    TimeMilli             now          = TimerMilli::GetNow();
    TimeMilli             nextSendTime = now.GetDistantFuture();
    Message *             message;
    Message *             nextMessage;

    for (message = mDelayedResponses.GetHead(); message != NULL; message = nextMessage)
    {
        nextMessage = message->GetNext();

        delayedResponse.ReadFrom(*message);

        if (now < delayedResponse.GetSendTime())
        {
            if (nextSendTime > delayedResponse.GetSendTime())
            {
                nextSendTime = delayedResponse.GetSendTime();
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

                // Here enters fast poll mode, as for Rx-Off-when-idle device, the enqueued msg should
                // be Mle Data Request.
                // Note: Finer-grade check (e.g. message subtype) might be required when deciding whether
                // or not enters fast poll mode fast poll mode if there are other type of delayed message
                // for Rx-Off-when-idle device.
                if (!IsRxOnWhenIdle())
                {
                    Get<DataPollSender>().SendFastPolls(DataPollSender::kDefaultFastPolls);
                }
            }
            else
            {
                message->Free();
            }
        }
    }

    if (nextSendTime < now.GetDistantFuture())
    {
        mDelayedResponseTimer.FireAt(nextSendTime);
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

    Random::Crypto::FillBuffer(mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge));

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
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = AppendTimeRequest(*message));
#endif

    destination.Clear();
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

void Mle::RequestShorterChildIdRequest(void)
{
    if (mAttachState == kAttachStateChildIdRequest)
    {
        mAddressRegistrationMode = kAppendMeshLocalOnly;
        SendChildIdRequest();
    }
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
            otLogInfoMle("Already attached to candidate parent");
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
    message->SetSubType(Message::kSubTypeMleChildIdRequest);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildIdRequest));
    SuccessOrExit(error = AppendResponse(*message, mChildIdRequest.mChallenge, mChildIdRequest.mChallengeLength));
    SuccessOrExit(error = AppendLinkFrameCounter(*message));
    SuccessOrExit(error = AppendMleFrameCounter(*message));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));
    SuccessOrExit(error = AppendTimeout(*message, mTimeout));
    SuccessOrExit(error = AppendVersion(*message));

    if (!IsFullThreadDevice())
    {
        SuccessOrExit(error = AppendAddressRegistration(*message, mAddressRegistrationMode));

        // no need to request the last Route64 TLV for MTD
        tlvsLen -= 1;
    }

    SuccessOrExit(error = AppendTlvRequest(*message, tlvs, tlvsLen));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    mParentCandidate.SetState(Neighbor::kStateValid);

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParentCandidate.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    if (mAddressRegistrationMode == kAppendMeshLocalOnly)
    {
        LogMleMessage("Send Child ID Request - short", destination);
    }
    else
    {
        LogMleMessage("Send Child ID Request", destination);
    }

    if (!IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SetAttachMode(true);
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
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

        if (!IsRxOnWhenIdle())
        {
            Get<DataPollSender>().SendFastPolls(DataPollSender::kDefaultFastPolls);
        }
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    if ((mRole == OT_DEVICE_ROLE_CHILD) && !IsRxOnWhenIdle())
    {
        mDataRequestState = kDataRequestActive;

        if (mChildUpdateRequestState == kChildUpdateRequestNone)
        {
            ScheduleMessageTransmissionTimer();
        }
    }

    return error;
}

void Mle::ScheduleMessageTransmissionTimer(void)
{
    uint32_t interval = 0;

    switch (mChildUpdateRequestState)
    {
    case kChildUpdateRequestNone:
        break;

    case kChildUpdateRequestPending:
        ExitNow(interval = kChildUpdateRequestPendingDelay);

    case kChildUpdateRequestActive:
        ExitNow(interval = kUnicastRetransmissionDelay);
    }

    switch (mDataRequestState)
    {
    case kDataRequestNone:
        break;

    case kDataRequestActive:
        ExitNow(interval = kUnicastRetransmissionDelay);
    }

    if ((mRole == OT_DEVICE_ROLE_CHILD) && IsRxOnWhenIdle())
    {
        interval =
            Time::SecToMsec(mTimeout) - static_cast<uint32_t>(kUnicastRetransmissionDelay) * kMaxChildKeepAliveAttempts;
    }

exit:
    if (interval != 0)
    {
        mMessageTransmissionTimer.Start(interval);
    }
    else
    {
        mMessageTransmissionTimer.Stop();
    }
}

void Mle::HandleMessageTransmissionTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mle>().HandleMessageTransmissionTimer();
}

void Mle::HandleMessageTransmissionTimer(void)
{
    // The `mMessageTransmissionTimer` is used for:
    //
    //  - Delaying OT_CHANGED notification triggered "Child Update Request" transmission (to allow aggregation),
    //  - Retransmission of "Child Update Request",
    //  - Retransmission of "Data Request" on a child,
    //  - Sending periodic keep-alive "Child Update Request" messages on a non-sleepy (rx-on) child.

    switch (mChildUpdateRequestState)
    {
    case kChildUpdateRequestNone:
        if (mDataRequestState == kDataRequestActive)
        {
            static const uint8_t tlvs[] = {Tlv::kNetworkData};
            Ip6::Address         destination;

            VerifyOrExit(mDataRequestAttempts < kMaxChildKeepAliveAttempts, BecomeDetached());

            destination.Clear();
            destination.mFields.m16[0] = HostSwap16(0xfe80);
            destination.SetIid(mParent.GetExtAddress());

            if (SendDataRequest(destination, tlvs, sizeof(tlvs), 0) == OT_ERROR_NONE)
            {
                mDataRequestAttempts++;
            }

            ExitNow();
        }

        // Keep-alive "Child Update Request" only on a non-sleepy child
        VerifyOrExit((mRole == OT_DEVICE_ROLE_CHILD) && IsRxOnWhenIdle());
        break;

    case kChildUpdateRequestPending:
        if (Get<Notifier>().IsPending())
        {
            // Here intentionally delay another kChildUpdateRequestPendingDelay
            // cycle to ensure we only send a Child Update Request after we
            // know there are no more pending changes.
            ScheduleMessageTransmissionTimer();
            ExitNow();
        }

        mChildUpdateAttempts = 0;
        break;

    case kChildUpdateRequestActive:
        break;
    }

    VerifyOrExit(mChildUpdateAttempts < kMaxChildKeepAliveAttempts, BecomeDetached());

    if (SendChildUpdateRequest() == OT_ERROR_NONE)
    {
        mChildUpdateAttempts++;
    }

exit:
    return;
}

otError Mle::SendChildUpdateRequest(void)
{
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message = NULL;

    if (!mParent.IsStateValidOrRestoring())
    {
        otLogWarnMle("No valid parent when sending Child Update Request");
        BecomeDetached();
        ExitNow();
    }

    mChildUpdateRequestState = kChildUpdateRequestActive;
    ScheduleMessageTransmissionTimer();

    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleChildUpdateRequest);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandChildUpdateRequest));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));

    if (!IsFullThreadDevice())
    {
        SuccessOrExit(error = AppendAddressRegistration(*message));
    }

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DETACHED:
        Random::Crypto::FillBuffer(mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge));
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

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParent.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    LogMleMessage("Send Child Update Request to parent", destination);

    if (!IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SetAttachMode(true);
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
    }
    else
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(true);
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
    bool         checkAddress = false;

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

        case Tlv::kStatus:
            SuccessOrExit(error = AppendStatus(*message, StatusTlv::kError));
            break;

        case Tlv::kAddressRegistration:
            if (!IsFullThreadDevice())
            {
                // We only register the mesh-local address in the "Child
                // Update Response" message and if there are additional
                // addresses to register we follow up with a "Child Update
                // Request".

                SuccessOrExit(error = AppendAddressRegistration(*message, kAppendMeshLocalOnly));
                checkAddress = true;
            }

            break;

        case Tlv::kResponse:
            SuccessOrExit(error = AppendResponse(*message, aChallenge.GetChallenge(), aChallenge.GetChallengeLength()));
            break;

        case Tlv::kLinkFrameCounter:
            SuccessOrExit(error = AppendLinkFrameCounter(*message));
            break;

        case Tlv::kMleFrameCounter:
            SuccessOrExit(error = AppendMleFrameCounter(*message));
            break;
        }
    }

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xfe80);
    destination.SetIid(mParent.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    LogMleMessage("Send Child Update Response to parent", destination);

    if (checkAddress && HasUnregisteredAddress())
    {
        SendChildUpdateRequest();
    }

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

    destination.Clear();
    destination.mFields.m16[0] = HostSwap16(0xff02);
    destination.mFields.m16[7] = HostSwap16(0x0001);

    return SendAnnounce(aChannel, aOrphanAnnounce, destination);
}

otError Mle::SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce, const Ip6::Address &aDestination)
{
    otError            error = OT_ERROR_NONE;
    ChannelTlv         channel;
    PanIdTlv           panid;
    ActiveTimestampTlv activeTimestamp;
    Message *          message = NULL;

    VerifyOrExit(Get<Mac::Mac>().GetSupportedChannelMask().ContainsChannel(aChannel), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((message = NewMleMessage()) != NULL, error = OT_ERROR_NO_BUFS);
    message->SetLinkSecurityEnabled(true);
    message->SetSubType(Message::kSubTypeMleAnnounce);
    message->SetChannel(aChannel);
    SuccessOrExit(error = AppendHeader(*message, Header::kCommandAnnounce));

    channel.Init();
    channel.SetChannel(Get<Mac::Mac>().GetPanChannel());
    SuccessOrExit(error = channel.AppendTo(*message));

    if (aOrphanAnnounce)
    {
        activeTimestamp.Init();
        activeTimestamp.SetSeconds(0);
        activeTimestamp.SetTicks(0);
        activeTimestamp.SetAuthoritative(true);

        SuccessOrExit(error = activeTimestamp.AppendTo(*message));
    }
    else
    {
        SuccessOrExit(error = AppendActiveTimestamp(*message));
    }

    panid.Init();
    panid.SetPanId(Get<Mac::Mac>().GetPanId());
    SuccessOrExit(error = panid.AppendTo(*message));
    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle("Send Announce on channel %d", aChannel);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Mle::SendOrphanAnnounce(void)
{
    otError          error;
    Mac::ChannelMask channelMask;

    if (Get<MeshCoP::ActiveDataset>().GetChannelMask(channelMask) != OT_ERROR_NONE)
    {
        channelMask = Get<Mac::Mac>().GetSupportedChannelMask();
    }

    SuccessOrExit(error = channelMask.GetNextChannel(mAnnounceChannel));

    SendAnnounce(mAnnounceChannel, true);

exit:
    return error;
}

otError Mle::SendMessage(Message &aMessage, const Ip6::Address &aDestination)
{
    otError          error = OT_ERROR_NONE;
    Header           header;
    uint32_t         keySequence;
    uint8_t          nonce[KeyManager::kNonceSize];
    uint8_t          tag[4];
    uint8_t          tagLength;
    Crypto::AesCcm   aesCcm;
    uint8_t          buf[64];
    uint16_t         length;
    Ip6::MessageInfo messageInfo;

    aMessage.Read(0, sizeof(header), &header);

    if (header.GetSecuritySuite() == Header::k154Security)
    {
        header.SetFrameCounter(Get<KeyManager>().GetMleFrameCounter());

        keySequence = Get<KeyManager>().GetCurrentKeySequence();
        header.SetKeyId(keySequence);

        aMessage.Write(0, header.GetLength(), &header);

        KeyManager::GenerateNonce(Get<Mac::Mac>().GetExtAddress(), Get<KeyManager>().GetMleFrameCounter(),
                                  Mac::Frame::kSecEncMic32, nonce);

        aesCcm.SetKey(Get<KeyManager>().GetCurrentMleKey(), 16);
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

        Get<KeyManager>().IncrementMleFrameCounter();
    }

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(mLinkLocal64.GetAddress());
    messageInfo.SetPeerPort(kUdpPort);
    messageInfo.SetHopLimit(kMleHopLimit);

    SuccessOrExit(error = mSocket.SendTo(aMessage, messageInfo));

exit:
    return error;
}

otError Mle::AddDelayedResponse(Message &aMessage, const Ip6::Address &aDestination, uint16_t aDelay)
{
    otError               error = OT_ERROR_NONE;
    DelayedResponseHeader delayedResponse(TimerMilli::GetNow() + aDelay, aDestination);

    SuccessOrExit(error = delayedResponse.AppendTo(aMessage));
    mDelayedResponses.Enqueue(aMessage);

    mDelayedResponseTimer.FireAtIfEarlier(delayedResponse.GetSendTime());

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
    otError         error = OT_ERROR_NONE;
    Header          header;
    uint32_t        keySequence;
    const uint8_t * mleKey;
    uint32_t        frameCounter;
    uint8_t         messageTag[4];
    uint8_t         nonce[KeyManager::kNonceSize];
    Mac::ExtAddress macAddr;
    Crypto::AesCcm  aesCcm;
    uint16_t        mleOffset;
    uint8_t         buf[64];
    uint16_t        length;
    uint8_t         tag[4];
    uint8_t         tagLength;
    uint8_t         command;
    Neighbor *      neighbor;

    otLogDebgMle("Receive UDP message");

    VerifyOrExit(aMessageInfo.GetLinkInfo() != NULL);
    VerifyOrExit(aMessageInfo.GetHopLimit() == kMleHopLimit, error = OT_ERROR_PARSE);

    length = aMessage.Read(aMessage.GetOffset(), sizeof(header), &header);
    VerifyOrExit(header.IsValid() && header.GetLength() <= length, error = OT_ERROR_PARSE);

    if (header.GetSecuritySuite() == Header::kNoSecurity)
    {
        aMessage.MoveOffset(header.GetLength());

        switch (header.GetCommand())
        {
        case Header::kCommandDiscoveryRequest:
            Get<MleRouter>().HandleDiscoveryRequest(aMessage, aMessageInfo);
            break;

        case Header::kCommandDiscoveryResponse:
            HandleDiscoveryResponse(aMessage, aMessageInfo);
            break;

        default:
            break;
        }

        ExitNow();
    }

    VerifyOrExit(mRole != OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(header.GetSecuritySuite() == Header::k154Security, error = OT_ERROR_PARSE);

    keySequence = header.GetKeyId();

    if (keySequence == Get<KeyManager>().GetCurrentKeySequence())
    {
        mleKey = Get<KeyManager>().GetCurrentMleKey();
    }
    else
    {
        mleKey = Get<KeyManager>().GetTemporaryMleKey(keySequence);
    }

    VerifyOrExit(aMessage.GetOffset() + header.GetLength() + sizeof(messageTag) <= aMessage.GetLength(),
                 error = OT_ERROR_PARSE);
    aMessage.MoveOffset(header.GetLength() - 1);

    aMessage.Read(aMessage.GetLength() - sizeof(messageTag), sizeof(messageTag), messageTag);
    SuccessOrExit(error = aMessage.SetLength(aMessage.GetLength() - sizeof(messageTag)));

    aMessageInfo.GetPeerAddr().ToExtAddress(macAddr);
    frameCounter = header.GetFrameCounter();
    KeyManager::GenerateNonce(macAddr, frameCounter, Mac::Frame::kSecEncMic32, nonce);

    aesCcm.SetKey(mleKey, 16);
    SuccessOrExit(error = aesCcm.Init(sizeof(aMessageInfo.GetPeerAddr()) + sizeof(aMessageInfo.GetSockAddr()) +
                                          header.GetHeaderLength(),
                                      aMessage.GetLength() - aMessage.GetOffset(), sizeof(messageTag), nonce,
                                      sizeof(nonce)));

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
    VerifyOrExit(memcmp(messageTag, tag, sizeof(tag)) == 0, error = OT_ERROR_SECURITY);
#endif

    if (keySequence > Get<KeyManager>().GetCurrentKeySequence())
    {
        Get<KeyManager>().SetCurrentKeySequence(keySequence);
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
            neighbor = Get<MleRouter>().GetNeighbor(macAddr);
        }

        break;

    default:
        neighbor = NULL;
        break;
    }

    if (neighbor != NULL && neighbor->IsStateValid())
    {
        if (keySequence == neighbor->GetKeySequence())
        {
            VerifyOrExit(frameCounter >= neighbor->GetMleFrameCounter(), error = OT_ERROR_DUPLICATED);
        }
        else
        {
            VerifyOrExit(keySequence > neighbor->GetKeySequence(), error = OT_ERROR_DUPLICATED);
            neighbor->SetKeySequence(keySequence);
            neighbor->SetLinkFrameCounter(0);
        }

        neighbor->SetMleFrameCounter(frameCounter + 1);
    }

    switch (command)
    {
    case Header::kCommandLinkRequest:
        Get<MleRouter>().HandleLinkRequest(aMessage, aMessageInfo, neighbor);
        break;

    case Header::kCommandLinkAccept:
        Get<MleRouter>().HandleLinkAccept(aMessage, aMessageInfo, keySequence, neighbor);
        break;

    case Header::kCommandLinkAcceptAndRequest:
        Get<MleRouter>().HandleLinkAcceptAndRequest(aMessage, aMessageInfo, keySequence, neighbor);
        break;

    case Header::kCommandAdvertisement:
        HandleAdvertisement(aMessage, aMessageInfo, neighbor);
        break;

    case Header::kCommandDataRequest:
        Get<MleRouter>().HandleDataRequest(aMessage, aMessageInfo, neighbor);
        break;

    case Header::kCommandDataResponse:
        HandleDataResponse(aMessage, aMessageInfo, neighbor);
        break;

    case Header::kCommandParentRequest:
        Get<MleRouter>().HandleParentRequest(aMessage, aMessageInfo);
        break;

    case Header::kCommandParentResponse:
        HandleParentResponse(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdRequest:
        Get<MleRouter>().HandleChildIdRequest(aMessage, aMessageInfo, keySequence);
        break;

    case Header::kCommandChildIdResponse:
        HandleChildIdResponse(aMessage, aMessageInfo, neighbor);
        break;

    case Header::kCommandChildUpdateRequest:
        if (mRole == OT_DEVICE_ROLE_LEADER || mRole == OT_DEVICE_ROLE_ROUTER)
        {
            Get<MleRouter>().HandleChildUpdateRequest(aMessage, aMessageInfo, keySequence);
        }
        else
        {
            HandleChildUpdateRequest(aMessage, aMessageInfo, neighbor);
        }

        break;

    case Header::kCommandChildUpdateResponse:
        if (mRole == OT_DEVICE_ROLE_LEADER || mRole == OT_DEVICE_ROLE_ROUTER)
        {
            Get<MleRouter>().HandleChildUpdateResponse(aMessage, aMessageInfo, keySequence, neighbor);
        }
        else
        {
            HandleChildUpdateResponse(aMessage, aMessageInfo, neighbor);
        }

        break;

    case Header::kCommandAnnounce:
        HandleAnnounce(aMessage, aMessageInfo);
        break;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    case Header::kCommandTimeSync:
        Get<MleRouter>().HandleTimeSync(aMessage, aMessageInfo, neighbor);
        break;
#endif
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogNoteMle("Failed to process UDP: %s", otThreadErrorToString(error));
    }

    return;
}

otError Mle::HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor)
{
    otError          error = OT_ERROR_NONE;
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

    if (mRole != OT_DEVICE_ROLE_DETACHED)
    {
        if (IsFullThreadDevice())
        {
            SuccessOrExit(error = Get<MleRouter>().HandleAdvertisement(aMessage, aMessageInfo, aNeighbor));
        }
        else if ((aNeighbor == &mParent) && (mParent.GetRloc16() != sourceAddress.GetRloc16()))
        {
            // Remove stale parent.
            BecomeDetached();
        }
    }

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        ExitNow();

    case OT_DEVICE_ROLE_CHILD:
        VerifyOrExit(aNeighbor == &mParent);

        if ((mParent.GetRloc16() == sourceAddress.GetRloc16()) &&
            (leaderData.GetPartitionId() != mLeaderData.GetPartitionId() ||
             leaderData.GetLeaderRouterId() != GetLeaderId()))
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

            if (IsFullThreadDevice() && (Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE) &&
                route.IsValid())
            {
                // Overwrite Route Data
                Get<MleRouter>().ProcessRouteTlv(route);
            }

            mRetrieveNewNetworkData = true;
        }

        mParent.SetLastHeard(TimerMilli::GetNow());
        break;

    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        VerifyOrExit(aNeighbor && aNeighbor->IsStateValid());
        break;
    }

    if (mRetrieveNewNetworkData || IsNetworkDataNewer(leaderData))
    {
        delay = Random::NonCrypto::GetUint16InRange(0, kMleMaxResponseDelay);
        SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay);
    }
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    else
    {
        Get<NetworkData::Local>().SendServerDataNotification();
    }
#endif

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Advertisement: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Mle::HandleDataResponse(const Message &         aMessage,
                                const Ip6::MessageInfo &aMessageInfo,
                                const Neighbor *        aNeighbor)
{
    otError error;

    LogMleMessage("Receive Data Response", aMessageInfo.GetPeerAddr());

    VerifyOrExit(aNeighbor && aNeighbor->IsStateValid(), error = OT_ERROR_SECURITY);

    error = HandleLeaderData(aMessage, aMessageInfo);

    if (mDataRequestState == kDataRequestNone && !IsRxOnWhenIdle())
    {
        // Here simply stops fast data poll request by Mle Data Request.
        // Note that in some cases fast data poll may continue after below stop operation until
        // running out the specified number. E.g. other component also trigger fast poll, and
        // is waiting for response; or the corner case where multiple Mle Data Request attempts
        // happened due to the retransmission mechanism.
        IgnoreReturnValue(Get<DataPollSender>().StopFastPolls());
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Data Response: %s", otThreadErrorToString(error));
    }

    return error;
}

bool Mle::IsNetworkDataNewer(const LeaderDataTlv &aLeaderData)
{
    int8_t diff;

    if (IsFullNetworkData())
    {
        diff = static_cast<int8_t>(aLeaderData.GetDataVersion() - Get<NetworkData::Leader>().GetVersion());
    }
    else
    {
        diff = static_cast<int8_t>(aLeaderData.GetStableDataVersion() - Get<NetworkData::Leader>().GetStableVersion());
    }

    return (diff > 0);
}

otError Mle::HandleLeaderData(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
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
        VerifyOrExit(IsNetworkDataNewer(leaderData));
    }

    // Active Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);
        timestamp = Get<MeshCoP::ActiveDataset>().GetTimestamp();

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
        timestamp = Get<MeshCoP::PendingDataset>().GetTimestamp();

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
        error =
            Get<NetworkData::Leader>().SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                                      !IsFullNetworkData(), aMessage, networkDataOffset);
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
            Get<MeshCoP::ActiveDataset>().Save(activeTimestamp, aMessage, activeDatasetOffset + sizeof(tlv),
                                               tlv.GetLength());
        }
    }

    // Pending Dataset
    if (pendingTimestamp.GetLength() > 0)
    {
        if (pendingDatasetOffset > 0)
        {
            aMessage.Read(pendingDatasetOffset, sizeof(tlv), &tlv);
            Get<MeshCoP::PendingDataset>().Save(pendingTimestamp, aMessage, pendingDatasetOffset + sizeof(tlv),
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
            delay = Random::NonCrypto::GetUint16InRange(0, kMleMaxResponseDelay);
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
    else if (error == OT_ERROR_NONE)
    {
        mDataRequestAttempts = 0;
        mDataRequestState    = kDataRequestNone;

        // Here the `mMessageTransmissionTimer` is intentionally not canceled
        // so that when it fires from its callback a "Child Update" is sent
        // if the device is a rx-on child. This way, even when the timer is
        // reused for retransmission of "Data Request" messages, it is ensured
        // that keep-alive "Child Update Request" messages are send within the
        // child's timeout.
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

otError Mle::HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint32_t aKeySequence)
{
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
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    TimeParameterTlv timeParameter;
#endif

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    LogMleMessage("Receive Parent Response", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());

    // Response
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response));
    VerifyOrExit(response.IsValid() &&
                     memcmp(response.GetResponse(), mParentRequest.mChallenge, response.GetResponseLength()) == 0,
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

    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);

    if (linkMargin > linkMarginTlv.GetLinkMargin())
    {
        linkMargin = linkMarginTlv.GetLinkMargin();
    }

    linkQuality = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin);

    // Connectivity
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kConnectivity, sizeof(connectivity), connectivity));
    VerifyOrExit(connectivity.IsValid(), error = OT_ERROR_PARSE);

    // Share data with application, if requested.
    if (mParentResponseCb)
    {
        otThreadParentResponseInfo parentinfo;

        parentinfo.mExtAddr      = extAddress;
        parentinfo.mRloc16       = sourceAddress.GetRloc16();
        parentinfo.mRssi         = linkInfo->mRss;
        parentinfo.mPriority     = connectivity.GetParentPriority();
        parentinfo.mLinkQuality3 = connectivity.GetLinkQuality3();
        parentinfo.mLinkQuality2 = connectivity.GetLinkQuality2();
        parentinfo.mLinkQuality1 = connectivity.GetLinkQuality1();
        parentinfo.mIsAttached   = IsAttached();

        mParentResponseCb(&parentinfo, mParentResponseCbContext);
    }

#if OPENTHREAD_FTD
    if (IsFullThreadDevice() && (mRole != OT_DEVICE_ROLE_DETACHED))
    {
        int8_t diff = static_cast<int8_t>(connectivity.GetIdSequence() - Get<RouterTable>().GetRouterIdSequence());

        switch (mParentRequestMode)
        {
        case kAttachAny:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId() || diff > 0);
            break;

        case kAttachSame1:
        case kAttachSame2:
            VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());
            VerifyOrExit(diff > 0);
            break;

        case kAttachSameDowngrade:
            VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId());
            VerifyOrExit(diff >= 0);
            break;

        case kAttachBetter:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId());

            VerifyOrExit(MleRouter::ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                      Get<MleRouter>().IsSingleton(), mLeaderData) > 0);
            break;
        }
    }
#endif

    // Continue to process the "ParentResponse" if it is from current
    // parent candidate to update the challenge and frame counters.

    if (mParentCandidate.IsStateParentResponse() && (mParentCandidate.GetExtAddress() != extAddress))
    {
        // if already have a candidate parent, only seek a better parent

        int compare = 0;

        if (IsFullThreadDevice())
        {
            compare = MleRouter::ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData, mParentIsSingleton,
                                                   mParentLeaderData);
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

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    // Time Parameter
    if (Tlv::GetTlv(aMessage, Tlv::kTimeParameter, sizeof(timeParameter), timeParameter) == OT_ERROR_NONE)
    {
        VerifyOrExit(timeParameter.IsValid());

        Get<TimeSync>().SetTimeSyncPeriod(timeParameter.GetTimeSyncPeriod());
        Get<TimeSync>().SetXtalThreshold(timeParameter.GetXtalThreshold());
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED
    else
    {
        // If the time sync feature is required, don't choose the parent which doesn't support it.
        ExitNow();
    }

#endif // OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    // Challenge
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge));
    VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);
    mChildIdRequest.mChallengeLength = challenge.GetChallengeLength();
    memcpy(mChildIdRequest.mChallenge, challenge.GetChallenge(), mChildIdRequest.mChallengeLength);

    mParentCandidate.SetExtAddress(extAddress);
    mParentCandidate.SetRloc16(sourceAddress.GetRloc16());
    mParentCandidate.SetLinkFrameCounter(linkFrameCounter.GetFrameCounter());
    mParentCandidate.SetMleFrameCounter(mleFrameCounter.GetFrameCounter());
    mParentCandidate.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                              DeviceMode::kModeFullNetworkData | DeviceMode::kModeSecureDataRequest));
    mParentCandidate.GetLinkInfo().Clear();
    mParentCandidate.GetLinkInfo().AddRss(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->mRss);
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
        otLogWarnMle("Failed to process Parent Response: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Mle::HandleChildIdResponse(const Message &         aMessage,
                                   const Ip6::MessageInfo &aMessageInfo,
                                   const Neighbor *        aNeighbor)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

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

    VerifyOrExit(aNeighbor && aNeighbor->IsStateValid(), error = OT_ERROR_SECURITY);

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
            Get<MeshCoP::ActiveDataset>().Save(activeTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength());
        }
    }

    // clear Pending Dataset if device succeed to reattach using stored Pending Dataset
    if (mReattachState == kReattachPending)
    {
        Get<MeshCoP::PendingDataset>().Clear();
    }

    // Pending Timestamp
    if (Tlv::GetTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);

        // Pending Dataset
        if (Tlv::GetOffset(aMessage, Tlv::kPendingDataset, offset) == OT_ERROR_NONE)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            Get<MeshCoP::PendingDataset>().Save(pendingTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength());
        }
    }
    else
    {
        Get<MeshCoP::PendingDataset>().ClearNetwork();
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Sync to Thread network time
    if (aMessage.GetTimeSyncSeq() != OT_TIME_SYNC_INVALID_SEQ)
    {
        Get<TimeSync>().HandleTimeSyncMessage(aMessage);
    }
#endif

    // Parent Attach Success

    SetStateDetached();

    SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

    if (!IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SetAttachMode(false);
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
    }
    else
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(true);
    }

    // Route
    if ((Tlv::GetTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE) && IsFullThreadDevice())
    {
        SuccessOrExit(error = Get<MleRouter>().ProcessRouteTlv(route));
    }

    mParent = mParentCandidate;
    mParentCandidate.Clear();

    mParent.SetRloc16(sourceAddress.GetRloc16());

    Get<NetworkData::Leader>().SetNetworkData(leaderData.GetDataVersion(), leaderData.GetStableDataVersion(),
                                              !IsFullNetworkData(), aMessage, networkDataOffset);

    SetStateChild(shortAddress.GetRloc16());

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Child ID Response: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Mle::HandleChildUpdateRequest(const Message &         aMessage,
                                      const Ip6::MessageInfo &aMessageInfo,
                                      Neighbor *              aNeighbor)
{
    static const uint8_t kMaxResponseTlvs = 6;

    otError          error = OT_ERROR_NONE;
    SourceAddressTlv sourceAddress;
    ChallengeTlv     challenge;
    TlvRequestTlv    tlvRequest;
    uint8_t          tlvs[kMaxResponseTlvs] = {};
    uint8_t          numTlvs                = 0;

    // Source Address
    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
    VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

    LogMleMessage("Receive Child Update Request from parent", aMessageInfo.GetPeerAddr(), sourceAddress.GetRloc16());

    // Challenge
    if (Tlv::GetTlv(aMessage, Tlv::kChallenge, sizeof(challenge), challenge) == OT_ERROR_NONE)
    {
        VerifyOrExit(challenge.IsValid(), error = OT_ERROR_PARSE);
        tlvs[numTlvs++] = Tlv::kResponse;
        tlvs[numTlvs++] = Tlv::kMleFrameCounter;
        tlvs[numTlvs++] = Tlv::kLinkFrameCounter;
    }

    if (aNeighbor == &mParent)
    {
        StatusTlv status;

        if (Tlv::GetTlv(aMessage, Tlv::kStatus, sizeof(status), status) == OT_ERROR_NONE)
        {
            VerifyOrExit(status.IsValid(), error = OT_ERROR_PARSE);

            if (status.GetStatus() == StatusTlv::kError)
            {
                BecomeDetached();
                ExitNow();
            }
        }

        if (mParent.GetRloc16() != sourceAddress.GetRloc16())
        {
            BecomeDetached();
            ExitNow();
        }

        // Leader Data, Network Data, Active Timestamp, Pending Timestamp
        SuccessOrExit(error = HandleLeaderData(aMessage, aMessageInfo));
    }
    else
    {
        // this device is not a child of the Child Update Request source
        tlvs[numTlvs++] = Tlv::kStatus;
    }

    // TLV Request
    if (Tlv::GetTlv(aMessage, Tlv::kTlvRequest, sizeof(tlvRequest), tlvRequest) == OT_ERROR_NONE)
    {
        VerifyOrExit(tlvRequest.IsValid(), error = OT_ERROR_PARSE);

        for (uint8_t i = 0; i < tlvRequest.GetLength(); i++)
        {
            if (numTlvs >= sizeof(tlvs))
            {
                otLogWarnMle("Failed to respond with TLVs: %d of %d", i, tlvRequest.GetLength());
                break;
            }

            tlvs[numTlvs++] = tlvRequest.GetTlvs()[i];
        }
    }

    SuccessOrExit(error = SendChildUpdateResponse(tlvs, numTlvs, challenge));

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Child Update Request from parent: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Mle::HandleChildUpdateResponse(const Message &         aMessage,
                                       const Ip6::MessageInfo &aMessageInfo,
                                       const Neighbor *        aNeighbor)
{
    otError             error = OT_ERROR_NONE;
    StatusTlv           status;
    ModeTlv             mode;
    ResponseTlv         response;
    LinkFrameCounterTlv linkFrameCounter;
    MleFrameCounterTlv  mleFrameCounter;
    SourceAddressTlv    sourceAddress;
    TimeoutTlv          timeout;

    LogMleMessage("Receive Child Update Response from parent", aMessageInfo.GetPeerAddr());

    switch (mRole)
    {
    case OT_DEVICE_ROLE_DETACHED:
        VerifyOrExit(
            (Tlv::GetTlv(aMessage, Tlv::kResponse, sizeof(response), response) == OT_ERROR_NONE) &&
                (response.IsValid()) &&
                (!memcmp(response.GetResponse(), mParentRequest.mChallenge, sizeof(mParentRequest.mChallenge))),
            error = OT_ERROR_SECURITY);
        break;

    case OT_DEVICE_ROLE_CHILD:
        VerifyOrExit((aNeighbor == &mParent) && mParent.IsStateValid(), error = OT_ERROR_SECURITY);
        break;

    default:
        assert(false);
        break;
    }

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

        mRetrieveNewNetworkData = true;

        // fall through

    case OT_DEVICE_ROLE_CHILD:
        // Source Address
        SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kSourceAddress, sizeof(sourceAddress), sourceAddress));
        VerifyOrExit(sourceAddress.IsValid(), error = OT_ERROR_PARSE);

        if (RouterIdFromRloc16(sourceAddress.GetRloc16()) != RouterIdFromRloc16(GetRloc16()))
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

        if (!IsRxOnWhenIdle())
        {
            Get<DataPollSender>().SetAttachMode(false);
            Get<MeshForwarder>().SetRxOnWhenIdle(false);
        }
        else
        {
            Get<MeshForwarder>().SetRxOnWhenIdle(true);
        }

        break;

    default:
        assert(false);
        break;
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        if (mChildUpdateRequestState == kChildUpdateRequestActive)
        {
            mChildUpdateAttempts     = 0;
            mChildUpdateRequestState = kChildUpdateRequestNone;
            ScheduleMessageTransmissionTimer();
        }
    }
    else
    {
        otLogWarnMle("Failed to process Child Update Response: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Mle::HandleAnnounce(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

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

    localTimestamp = Get<MeshCoP::ActiveDataset>().GetTimestamp();

    if (localTimestamp == NULL || localTimestamp->Compare(timestamp) > 0)
    {
        // No action is required if device is detached, and current
        // channel and pan-id match the values from the received MLE
        // Announce message.

        VerifyOrExit((mRole != OT_DEVICE_ROLE_DETACHED) || (Get<Mac::Mac>().GetPanChannel() != channel) ||
                     (Get<Mac::Mac>().GetPanId() != panId));

        if (mAttachState == kAttachStateProcessAnnounce)
        {
            VerifyOrExit(mAlternateTimestamp < timestamp.GetSeconds());
        }

        mAlternateTimestamp = timestamp.GetSeconds();
        mAlternateChannel   = channel;
        mAlternatePanId     = panId;
        SetAttachState(kAttachStateProcessAnnounce);
        mAttachTimer.Start(kAnnounceProcessTimeout);

        otLogNoteMle("Delay processing Announce - channel %d, panid 0x%02x", channel, panId);
    }
    else if (localTimestamp->Compare(timestamp) < 0)
    {
        SendAnnounce(channel, false);

#if OPENTHREAD_CONFIG_MLE_SEND_UNICAST_ANNOUNCE_RESPONSE
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

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Announce: %s", otThreadErrorToString(error));
    }

    return error;
}

void Mle::ProcessAnnounce(void)
{
    uint8_t  newChannel = mAlternateChannel;
    uint16_t newPanId   = mAlternatePanId;

    assert(mAttachState == kAttachStateProcessAnnounce);

    otLogNoteMle("Processing Announce - channel %d, panid 0x%02x", newChannel, newPanId);

    Stop(/* aClearNetworkDatasets */ false);

    // Save the current/previous channel and pan-id
    mAlternateChannel   = Get<Mac::Mac>().GetPanChannel();
    mAlternatePanId     = Get<Mac::Mac>().GetPanId();
    mAlternateTimestamp = 0;

    Get<Mac::Mac>().SetPanChannel(newChannel);
    Get<Mac::Mac>().SetPanId(newPanId);

    Start(/* aAnnounceAttach */ true);
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
    bool                          didCheckSteeringData = false;

    LogMleMessage("Receive Discovery Response", aMessageInfo.GetPeerAddr());

    VerifyOrExit(mDiscoverInProgress, error = OT_ERROR_DROP);

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
            result.mExtendedPanId = extPanId.GetExtendedPanId();
            break;

        case MeshCoP::Tlv::kNetworkName:
            aMessage.Read(offset, sizeof(networkName), &networkName);
            static_cast<Mac::NetworkName &>(result.mNetworkName).Set(networkName.GetNetworkName());
            break;

        case MeshCoP::Tlv::kSteeringData:
            aMessage.Read(offset, sizeof(steeringData), &steeringData);
            VerifyOrExit(steeringData.IsValid(), error = OT_ERROR_PARSE);

            if (mDiscoverEnableFiltering)
            {
                VerifyOrExit((steeringData.GetBit(mDiscoverCcittIndex % steeringData.GetNumBits()) &&
                              steeringData.GetBit(mDiscoverAnsiIndex % steeringData.GetNumBits())));
            }

            didCheckSteeringData         = true;
            result.mSteeringData.mLength = steeringData.GetSteeringDataLength();
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

    VerifyOrExit(!mDiscoverEnableFiltering || didCheckSteeringData);

    mDiscoverHandler(&result, mDiscoverContext);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to process Discovery Response: %s", otThreadErrorToString(error));
    }

    return error;
}

Neighbor *Mle::GetNeighbor(uint16_t aAddress)
{
    Neighbor *rval = NULL;

    if (mParent.IsStateValidOrRestoring() && (mParent.GetRloc16() == aAddress))
    {
        rval = &mParent;
    }
    else if (mParentCandidate.IsStateValid() && (mParentCandidate.GetRloc16() == aAddress))
    {
        rval = &mParentCandidate;
    }

    return rval;
}

Neighbor *Mle::GetNeighbor(const Mac::ExtAddress &aAddress)
{
    Neighbor *rval = NULL;

    if (mParent.IsStateValidOrRestoring() && (mParent.GetExtAddress() == aAddress))
    {
        rval = &mParent;
    }
    else if (mParentCandidate.IsStateValid() && (mParentCandidate.GetExtAddress() == aAddress))
    {
        rval = &mParentCandidate;
    }

    return rval;
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
    return (mParent.IsStateValid()) ? mParent.GetRloc16() : static_cast<uint16_t>(Mac::kShortAddrInvalid);
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

otError Mle::CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    otError          error = OT_ERROR_DROP;
    Ip6::MessageInfo messageInfo;

    if (aMeshDest != GetRloc16())
    {
        ExitNow(error = OT_ERROR_NONE);
    }

    if (Get<ThreadNetif>().IsUnicastAddress(aIp6Header.GetDestination()))
    {
        ExitNow(error = OT_ERROR_NONE);
    }

    messageInfo.GetPeerAddr()                = GetMeshLocal16();
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(aMeshSource);

    Get<Ip6::Icmp>().SendError(Ip6::IcmpHeader::kTypeDstUnreach, Ip6::IcmpHeader::kCodeDstUnreachNoRoute, messageInfo,
                               aIp6Header);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
otError Mle::InformPreviousParent(void)
{
    otError          error   = OT_ERROR_NONE;
    Message *        message = NULL;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((mPreviousParentRloc != Mac::kShortAddrInvalid) && (mPreviousParentRloc != mParent.GetRloc16()));

    mCounters.mParentChanges++;

    VerifyOrExit((message = Get<Ip6::Ip6>().NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->SetLength(0));

    messageInfo.SetSockAddr(GetMeshLocal64());
    messageInfo.SetPeerAddr(GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(mPreviousParentRloc);

    SuccessOrExit(error = Get<Ip6::Ip6>().SendDatagram(*message, messageInfo, Ip6::kProtoNone));

    otLogNoteMle("Sending message to inform previous parent 0x%04x", mPreviousParentRloc);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to inform previous parent: %s", otThreadErrorToString(error));

        if (message != NULL)
        {
            message->Free();
        }
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
void Mle::HandleParentSearchTimer(Timer &aTimer)
{
    aTimer.GetOwner<Mle>().HandleParentSearchTimer();
}

void Mle::HandleParentSearchTimer(void)
{
    int8_t parentRss;

    otLogInfoMle("PeriodicParentSearch: %s interval passed", mParentSearchIsInBackoff ? "Backoff" : "Check");

    if (mParentSearchBackoffWasCanceled)
    {
        // Backoff can be canceled if the device switches to a new parent.
        // from `UpdateParentSearchState()`. We want to limit this to happen
        // only once within a backoff interval.

        if (TimerMilli::GetNow() - mParentSearchBackoffCancelTime >= kParentSearchBackoffInterval)
        {
            mParentSearchBackoffWasCanceled = false;
            otLogInfoMle("PeriodicParentSearch: Backoff cancellation is allowed on parent switch");
        }
    }

    mParentSearchIsInBackoff = false;

    VerifyOrExit(mRole == OT_DEVICE_ROLE_CHILD);

    parentRss = GetParent().GetLinkInfo().GetAverageRss();
    otLogInfoMle("PeriodicParentSearch: Parent RSS %d", parentRss);
    VerifyOrExit(parentRss != OT_RADIO_RSSI_INVALID);

    if (parentRss < kParentSearchRssThreadhold)
    {
        otLogInfoMle("PeriodicParentSearch: Parent RSS less than %d, searching for new parents",
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

    interval = Random::NonCrypto::GetUint32InRange(0, kParentSearchJitterInterval);

    if (mParentSearchIsInBackoff)
    {
        interval += kParentSearchBackoffInterval;
    }
    else
    {
        interval += kParentSearchCheckInterval;
    }

    mParentSearchTimer.Start(interval);

    otLogInfoMle("PeriodicParentSearch: (Re)starting timer for %s interval",
                 mParentSearchIsInBackoff ? "backoff" : "check");
}

void Mle::UpdateParentSearchState(void)
{
#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH

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
            otLogInfoMle("PeriodicParentSearch: Canceling backoff on switching to a new parent");
        }
    }

#endif // OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH

    mParentSearchRecentlyDetached = false;

    if (!mParentSearchIsInBackoff)
    {
        StartParentSearchTimer();
    }
}
#endif // OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE

void Mle::LogMleMessage(const char *aLogString, const Ip6::Address &aAddress) const
{
    OT_UNUSED_VARIABLE(aLogString);
    OT_UNUSED_VARIABLE(aAddress);

    otLogInfoMle("%s (%s)", aLogString, aAddress.ToString().AsCString());
}

void Mle::LogMleMessage(const char *aLogString, const Ip6::Address &aAddress, uint16_t aRloc) const
{
    OT_UNUSED_VARIABLE(aLogString);
    OT_UNUSED_VARIABLE(aAddress);
    OT_UNUSED_VARIABLE(aRloc);

    otLogInfoMle("%s (%s,0x%04x)", aLogString, aAddress.ToString().AsCString(), aRloc);
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

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MLE == 1)

const char *Mle::AttachModeToString(AttachMode aMode)
{
    const char *str = "unknown";

    switch (aMode)
    {
    case kAttachAny:
        str = "any-partition";
        break;

    case kAttachSame1:
        str = "same-partition-try-1";
        break;

    case kAttachSame2:
        str = "same-partition-try-2";
        break;

    case kAttachBetter:
        str = "better-partition";
        break;

    case kAttachSameDowngrade:
        str = "same-partition-downgrade";
        break;
    }

    return str;
}

const char *Mle::AttachStateToString(AttachState aState)
{
    const char *str = "Unknown";

    switch (aState)
    {
    case kAttachStateIdle:
        str = "Idle";
        break;

    case kAttachStateProcessAnnounce:
        str = "ProcessAnnounce";
        break;

    case kAttachStateStart:
        str = "Start";
        break;

    case kAttachStateParentRequestRouter:
        str = "ParentReqRouters";
        break;

    case kAttachStateParentRequestReed:
        str = "ParentReqReeds";
        break;

    case kAttachStateAnnounce:
        str = "Announce";
        break;

    case kAttachStateChildIdRequest:
        str = "ChildIdReq";
        break;
    };

    return str;
}

const char *Mle::ReattachStateToString(ReattachState aState)
{
    const char *str = "unknown";

    switch (aState)
    {
    case kReattachStop:
        str = "";
        break;

    case kReattachStart:
        str = "reattaching";
        break;

    case kReattachActive:
        str = "reattaching with Active Dataset";
        break;

    case kReattachPending:
        str = "reattaching with Pending Dataset";
        break;
    }

    return str;
}

#endif // (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MLE == 1)

// LCOV_EXCL_STOP

void Mle::RegisterParentResponseStatsCallback(otThreadParentResponseCallback aCallback, void *aContext)
{
    mParentResponseCb        = aCallback;
    mParentResponseCbContext = aContext;
}

} // namespace Mle
} // namespace ot
