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
    , mRole(kRoleDisabled)
    , mNeighborTable(aInstance)
    , mDeviceMode(DeviceMode::kModeRxOnWhenIdle | DeviceMode::kModeSecureDataRequest)
    , mAttachState(kAttachStateIdle)
    , mReattachState(kReattachStop)
    , mAttachCounter(0)
    , mAnnounceDelay(kAnnounceTimeout)
    , mAttachTimer(aInstance, Mle::HandleAttachTimer, this)
    , mDelayedResponseTimer(aInstance, Mle::HandleDelayedResponseTimer, this)
    , mMessageTransmissionTimer(aInstance, Mle::HandleMessageTransmissionTimer, this)
    , mParentLeaderCost(0)
    , mParentRequestMode(kAttachAny)
    , mParentPriority(0)
    , mParentLinkQuality3(0)
    , mParentLinkQuality2(0)
    , mParentLinkQuality1(0)
    , mParentSedBufferSize(0)
    , mParentSedDatagramCount(0)
    , mChildUpdateAttempts(0)
    , mChildUpdateRequestState(kChildUpdateRequestNone)
    , mDataRequestAttempts(0)
    , mDataRequestState(kDataRequestNone)
    , mAddressRegistrationMode(kAppendAllAddresses)
    , mHasRestored(false)
    , mParentLinkMargin(0)
    , mParentIsSingleton(false)
    , mReceivedResponseFromParent(false)
    , mSocket(aInstance)
    , mTimeout(kMleEndDeviceTimeout)
#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
    , mPreviousParentRloc(Mac::kShortAddrInvalid)
#endif
#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    , mParentSearchIsInBackoff(false)
    , mParentSearchBackoffWasCanceled(false)
    , mParentSearchRecentlyDetached(false)
    , mParentSearchBackoffCancelTime(0)
    , mParentSearchTimer(aInstance, Mle::HandleParentSearchTimer, this)
#endif
    , mAnnounceChannel(0)
    , mAlternateChannel(0)
    , mAlternatePanId(Mac::kPanIdBroadcast)
    , mAlternateTimestamp(0)
    , mParentResponseCb(nullptr)
    , mParentResponseCbContext(nullptr)
{
    MeshLocalPrefix meshLocalPrefix;

    mParent.Init(aInstance);
    mParentCandidate.Init(aInstance);

    mLeaderData.Clear();
    mParentLeaderData.Clear();
    mParent.Clear();
    mParentCandidate.Clear();
    ResetCounters();

    mLinkLocal64.InitAsThreadOrigin();
    mLinkLocal64.GetAddress().SetToLinkLocalAddress(Get<Mac::Mac>().GetExtAddress());

    mLeaderAloc.InitAsThreadOriginRealmLocalScope();

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    for (Ip6::NetifUnicastAddress &serviceAloc : mServiceAlocs)
    {
        serviceAloc.InitAsThreadOriginRealmLocalScope();
        serviceAloc.GetAddress().GetIid().SetLocator(Mac::kShortAddrInvalid);
    }
#endif

    meshLocalPrefix.SetFromExtendedPanId(Get<Mac::Mac>().GetExtendedPanId());

    mMeshLocal64.InitAsThreadOriginRealmLocalScope();
    mMeshLocal64.GetAddress().GetIid().GenerateRandom();

    mMeshLocal16.InitAsThreadOriginRealmLocalScope();
    mMeshLocal16.GetAddress().GetIid().SetToLocator(0);
    mMeshLocal16.mRloc = true;

    // Store RLOC address reference in MPL module.
    Get<Ip6::Mpl>().SetMatchingAddress(mMeshLocal16.GetAddress());

    mLinkLocalAllThreadNodes.Clear();
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff32);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);

    mRealmLocalAllThreadNodes.Clear();
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[0] = HostSwap16(0xff33);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[7] = HostSwap16(0x0001);

    SetMeshLocalPrefix(meshLocalPrefix);

    // `SetMeshLocalPrefix()` also adds the Mesh-Local EID and subscribes
    // to the Link- and Realm-Local All Thread Nodes multicast addresses.
}

otError Mle::Enable(void)
{
    otError error = OT_ERROR_NONE;

    UpdateLinkLocalAddress();
    SuccessOrExit(error = mSocket.Open(&Mle::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(kUdpPort));

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    StartParentSearchTimer();
#endif
exit:
    return error;
}

void Mle::ScheduleChildUpdateRequest(void)
{
    mChildUpdateRequestState = kChildUpdateRequestPending;
    ScheduleMessageTransmissionTimer();
}

otError Mle::Disable(void)
{
    otError error = OT_ERROR_NONE;

    Stop(false);
    SuccessOrExit(error = mSocket.Close());
    Get<ThreadNetif>().RemoveUnicastAddress(mLinkLocal64);

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
        Get<Mac::Mac>().SetPanId(Mac::GenerateRandomPanId());
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
        IgnoreError(BecomeChild(kAttachAny));
    }
#if OPENTHREAD_FTD
    else if (IsActiveRouter(GetRloc16()))
    {
        if (Get<MleRouter>().BecomeRouter(ThreadStatusTlv::kTooFewRouters) != OT_ERROR_NONE)
        {
            IgnoreError(BecomeChild(kAttachAny));
        }
    }
#endif
    else
    {
        mChildUpdateAttempts = 0;
        IgnoreError(SendChildUpdateRequest());
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

    VerifyOrExit(!IsDisabled(), OT_NOOP);

    Get<KeyManager>().Stop();
    SetStateDetached();
    Get<ThreadNetif>().UnsubscribeMulticast(mRealmLocalAllThreadNodes);
    Get<ThreadNetif>().UnsubscribeMulticast(mLinkLocalAllThreadNodes);
    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal16);
    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal64);

    SetRole(kRoleDisabled);

exit:
    return;
}

void Mle::SetRole(DeviceRole aRole)
{
    DeviceRole oldRole = mRole;

    SuccessOrExit(Get<Notifier>().Update(mRole, aRole, kEventThreadRoleChanged));

    otLogNoteMle("Role %s -> %s", RoleToString(oldRole), RoleToString(mRole));

    switch (mRole)
    {
    case kRoleDisabled:
        mCounters.mDisabledRole++;
        break;
    case kRoleDetached:
        mCounters.mDetachedRole++;
        break;
    case kRoleChild:
        mCounters.mChildRole++;
        break;
    case kRoleRouter:
        mCounters.mRouterRole++;
        break;
    case kRoleLeader:
        mCounters.mLeaderRole++;
        break;
    }

    // If the previous state is disabled, the parent can be in kStateRestored.
    if (!IsChild() && oldRole != kRoleDisabled)
    {
        mParent.SetState(Neighbor::kStateInvalid);
    }

exit:
    OT_UNUSED_VARIABLE(oldRole);
}

void Mle::SetAttachState(AttachState aState)
{
    VerifyOrExit(aState != mAttachState, OT_NOOP);
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

    IgnoreError(Get<MeshCoP::ActiveDataset>().Restore());
    IgnoreError(Get<MeshCoP::PendingDataset>().Restore());

#if OPENTHREAD_CONFIG_DUA_ENABLE
    Get<DuaManager>().Restore();
#endif

    SuccessOrExit(error = Get<Settings>().ReadNetworkInfo(networkInfo));

    Get<KeyManager>().SetCurrentKeySequence(networkInfo.GetKeySequence());
    Get<KeyManager>().SetMleFrameCounter(networkInfo.GetMleFrameCounter());
    Get<KeyManager>().SetMacFrameCounter(networkInfo.GetMacFrameCounter());
    mDeviceMode.Set(networkInfo.GetDeviceMode());

    // force re-attach when version mismatch.
    VerifyOrExit(networkInfo.GetVersion() == kThreadVersion, OT_NOOP);

    switch (networkInfo.GetRole())
    {
    case kRoleChild:
    case kRoleRouter:
    case kRoleLeader:
        break;

    default:
        ExitNow();
    }

    Get<Mac::Mac>().SetShortAddress(networkInfo.GetRloc16());
    Get<Mac::Mac>().SetExtAddress(networkInfo.GetExtAddress());

    mMeshLocal64.GetAddress().SetIid(networkInfo.GetMeshLocalIid());

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
        mParent.SetVersion(static_cast<uint8_t>(parentInfo.GetVersion()));
        mParent.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                         DeviceMode::kModeFullNetworkData | DeviceMode::kModeSecureDataRequest));
        mParent.SetRloc16(Rloc16FromRouterId(RouterIdFromRloc16(networkInfo.GetRloc16())));
        mParent.SetState(Neighbor::kStateRestored);

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
        mPreviousParentRloc = mParent.GetRloc16();
#endif
    }
#if OPENTHREAD_FTD
    else
    {
        Get<MleRouter>().SetRouterId(RouterIdFromRloc16(GetRloc16()));
        Get<MleRouter>().SetPreviousPartitionId(networkInfo.GetPreviousPartitionId());
        Get<ChildTable>().Restore();
    }
#endif

    // Sucessfully restored the network information from non-volatile settings after boot.
    mHasRestored = true;

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
        networkInfo.SetMeshLocalIid(mMeshLocal64.GetAddress().GetIid());
        networkInfo.SetVersion(kThreadVersion);

        if (IsChild())
        {
            Settings::ParentInfo parentInfo;

            parentInfo.Init();
            parentInfo.SetExtAddress(mParent.GetExtAddress());
            parentInfo.SetVersion(mParent.GetVersion());

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

otError Mle::BecomeDetached(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsDisabled(), error = OT_ERROR_INVALID_STATE);

    // In case role is already detached and attach state is `kAttachStateStart`
    // (i.e., waiting to start an attach attempt), there is no need to make any
    // changes.

    VerifyOrExit(!IsDetached() || mAttachState != kAttachStateStart, OT_NOOP);

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
    IgnoreError(BecomeChild(kAttachAny));

exit:
    return error;
}

otError Mle::BecomeChild(AttachMode aMode)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!IsDisabled(), error = OT_ERROR_INVALID_STATE);
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
#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            Get<MleRouter>().StopAdvertiseTimer();
        }
#endif
    }
    else
    {
        mCounters.mBetterPartitionAttachAttempts++;
    }

    mAttachTimer.Start(GetAttachStartDelay());

    if (IsDetached())
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

    VerifyOrExit(IsDetached(), OT_NOOP);

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
    return (IsChild() || IsRouter() || IsLeader());
}

bool Mle::IsRouterOrLeader(void) const
{
    return (IsRouter() || IsLeader());
}

void Mle::SetStateDetached(void)
{
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().Reset();
#endif
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    Get<BackboneRouter::Leader>().Reset();
#endif

    if (IsLeader())
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mLeaderAloc);
    }

    SetRole(kRoleDetached);
    SetAttachState(kAttachStateIdle);
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    mChildUpdateRequestState = kChildUpdateRequestNone;
    mChildUpdateAttempts     = 0;
    mDataRequestState        = kDataRequestNone;
    mDataRequestAttempts     = 0;
    Get<MeshForwarder>().SetRxOnWhenIdle(true);
    Get<Mac::Mac>().SetBeaconEnabled(false);
#if OPENTHREAD_FTD
    Get<MleRouter>().HandleDetachStart();
#endif
    Get<Ip6::Ip6>().SetForwardingEnabled(false);
#if OPENTHREAD_FTD
    Get<Ip6::Mpl>().SetTimerExpirations(0);
#endif
}

void Mle::SetStateChild(uint16_t aRloc16)
{
    if (IsLeader())
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mLeaderAloc);
    }

    SetRloc16(aRloc16);
    SetRole(kRoleChild);
    SetAttachState(kAttachStateIdle);
    mAttachTimer.Stop();
    mAttachCounter       = 0;
    mReattachState       = kReattachStop;
    mChildUpdateAttempts = 0;
    mDataRequestAttempts = 0;
    Get<Mac::Mac>().SetBeaconEnabled(false);
    ScheduleMessageTransmissionTimer();

#if OPENTHREAD_FTD
    if (IsFullThreadDevice())
    {
        Get<MleRouter>().HandleChildStart(mParentRequestMode);
    }
#endif

    Get<Ip6::Ip6>().SetForwardingEnabled(false);
#if OPENTHREAD_FTD
    Get<Ip6::Mpl>().SetTimerExpirations(kMplChildDataMessageTimerExpirations);
#endif

    // send announce after attached if needed
    InformPreviousChannel();

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    UpdateParentSearchState();
#endif

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
    InformPreviousParent();
    mPreviousParentRloc = mParent.GetRloc16();
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (Get<Mac::Mac>().IsCslEnabled())
    {
        IgnoreError(Get<Radio>().EnableCsl(Get<Mac::Mac>().GetCslPeriod(), &GetParent().GetExtAddress()));
        ScheduleChildUpdateRequest();
    }
#endif
}

void Mle::InformPreviousChannel(void)
{
    VerifyOrExit(mAlternatePanId != Mac::kPanIdBroadcast, OT_NOOP);
    VerifyOrExit(IsChild() || IsRouter(), OT_NOOP);

#if OPENTHREAD_FTD
    VerifyOrExit(!IsFullThreadDevice() || IsRouter() || Get<MleRouter>().GetRouterSelectionJitterTimeout() == 0,
                 OT_NOOP);
#endif

    mAlternatePanId = Mac::kPanIdBroadcast;
    Get<AnnounceBeginServer>().SendAnnounce(1 << mAlternateChannel);

exit:
    return;
}

void Mle::SetTimeout(uint32_t aTimeout)
{
    VerifyOrExit(mTimeout != aTimeout, OT_NOOP);

    if (aTimeout < kMinTimeout)
    {
        aTimeout = kMinTimeout;
    }

    mTimeout = aTimeout;

    Get<DataPollSender>().RecalculatePollPeriod();

    if (IsChild())
    {
        IgnoreError(SendChildUpdateRequest());
    }

exit:
    return;
}

otError Mle::SetDeviceMode(DeviceMode aDeviceMode)
{
    otError    error   = OT_ERROR_NONE;
    DeviceMode oldMode = mDeviceMode;

    VerifyOrExit(aDeviceMode.IsValid(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mDeviceMode != aDeviceMode, OT_NOOP);
    mDeviceMode = aDeviceMode;

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitDeviceMode(mDeviceMode);
#endif

    otLogNoteMle("Mode 0x%02x -> 0x%02x [%s]", oldMode.Get(), mDeviceMode.Get(), mDeviceMode.ToString().AsCString());

    IgnoreError(Store());

    switch (mRole)
    {
    case kRoleDisabled:
        break;

    case kRoleDetached:
        mAttachCounter = 0;
        SetStateDetached();
        IgnoreError(BecomeChild(kAttachAny));
        break;

    case kRoleChild:
        SetStateChild(GetRloc16());
        IgnoreError(SendChildUpdateRequest());
        break;

    case kRoleRouter:
    case kRoleLeader:
        if (oldMode.IsFullThreadDevice() && !mDeviceMode.IsFullThreadDevice())
        {
            IgnoreError(BecomeDetached());
        }

        break;
    }

exit:
    return error;
}

void Mle::UpdateLinkLocalAddress(void)
{
    Get<ThreadNetif>().RemoveUnicastAddress(mLinkLocal64);
    mLinkLocal64.GetAddress().GetIid().SetFromExtAddress(Get<Mac::Mac>().GetExtAddress());
    Get<ThreadNetif>().AddUnicastAddress(mLinkLocal64);

    Get<Notifier>().Signal(kEventThreadLinkLocalAddrChanged);
}

void Mle::SetMeshLocalPrefix(const MeshLocalPrefix &aMeshLocalPrefix)
{
    VerifyOrExit(GetMeshLocalPrefix() != aMeshLocalPrefix,
                 Get<Notifier>().SignalIfFirst(kEventThreadMeshLocalAddrChanged));

    if (Get<ThreadNetif>().IsUp())
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mLeaderAloc);

        // We must remove the old addresses before adding the new ones.
        Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal64);
        Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal16);
        Get<ThreadNetif>().UnsubscribeMulticast(mLinkLocalAllThreadNodes);
        Get<ThreadNetif>().UnsubscribeMulticast(mRealmLocalAllThreadNodes);
    }

    mMeshLocal64.GetAddress().SetPrefix(aMeshLocalPrefix);
    mMeshLocal16.GetAddress().SetPrefix(aMeshLocalPrefix);
    mLeaderAloc.GetAddress().SetPrefix(aMeshLocalPrefix);

    // Just keep mesh local prefix if network interface is down
    VerifyOrExit(Get<ThreadNetif>().IsUp(), OT_NOOP);

    ApplyMeshLocalPrefix();

exit:
    return;
}

void Mle::ApplyMeshLocalPrefix(void)
{
    mLinkLocalAllThreadNodes.GetAddress().SetMulticastNetworkPrefix(GetMeshLocalPrefix());
    mRealmLocalAllThreadNodes.GetAddress().SetMulticastNetworkPrefix(GetMeshLocalPrefix());

    VerifyOrExit(!IsDisabled(), OT_NOOP);

    // Add the addresses back into the table.
    Get<ThreadNetif>().AddUnicastAddress(mMeshLocal64);
    Get<ThreadNetif>().SubscribeMulticast(mLinkLocalAllThreadNodes);
    Get<ThreadNetif>().SubscribeMulticast(mRealmLocalAllThreadNodes);

    if (IsAttached())
    {
        Get<ThreadNetif>().AddUnicastAddress(mMeshLocal16);
    }

    // update Leader ALOC
    if (IsLeader())
    {
        Get<ThreadNetif>().AddUnicastAddress(mLeaderAloc);
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    Get<MeshCoP::Commissioner>().ApplyMeshLocalPrefix();
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    Get<MeshCoP::BorderAgent>().ApplyMeshLocalPrefix();
#endif

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
    Get<Dhcp6::Server>().ApplyMeshLocalPrefix();
#endif

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

    for (Ip6::NetifUnicastAddress &serviceAloc : mServiceAlocs)
    {
        if (serviceAloc.GetAddress().GetIid().GetLocator() != Mac::kShortAddrInvalid)
        {
            Get<ThreadNetif>().RemoveUnicastAddress(serviceAloc);
            serviceAloc.GetAddress().SetPrefix(GetMeshLocalPrefix());
            Get<ThreadNetif>().AddUnicastAddress(serviceAloc);
        }
    }

#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().ApplyMeshLocalPrefix();
#endif

exit:
    // Changing the prefix also causes the mesh local address to be different.
    Get<Notifier>().Signal(kEventThreadMeshLocalAddrChanged);
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

        // Clear cached CoAP with old RLOC source
        if (oldRloc16 != Mac::kShortAddrInvalid)
        {
            Get<Tmf::TmfAgent>().ClearRequests(mMeshLocal16.GetAddress());
        }
    }

    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal16);

    Get<Mac::Mac>().SetShortAddress(aRloc16);
    Get<Ip6::Mpl>().SetSeedId(aRloc16);

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        // mesh-local 16
        mMeshLocal16.GetAddress().GetIid().SetLocator(aRloc16);
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
#if OPENTHREAD_FTD
        Get<MleRouter>().HandlePartitionChange();
#endif
        Get<Notifier>().Signal(kEventThreadPartitionIdChanged);
        mCounters.mPartitionIdChanges++;
    }
    else
    {
        Get<Notifier>().SignalIfFirst(kEventThreadPartitionIdChanged);
    }

    mLeaderData.SetPartitionId(aPartitionId);
    mLeaderData.SetWeighting(aWeighting);
    mLeaderData.SetLeaderRouterId(aLeaderRouterId);
}

otError Mle::GetLeaderAddress(Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    aAddress.SetToRoutingLocator(GetMeshLocalPrefix(), Rloc16FromRouterId(mLeaderData.GetLeaderRouterId()));

exit:
    return error;
}

otError Mle::GetLocatorAddress(Ip6::Address &aAddress, uint16_t aLocator) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 14);
    aAddress.GetIid().SetLocator(aLocator);

exit:
    return error;
}

otError Mle::GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = OT_ERROR_DETACHED);
    aAddress.SetToAnycastLocator(GetMeshLocalPrefix(), ServiceAlocFromId(aServiceId));

exit:
    return error;
}

const LeaderData &Mle::GetLeaderData(void)
{
    mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion());
    mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetStableVersion());

    return mLeaderData;
}

Message *Mle::NewMleMessage(void)
{
    Message *         message;
    Message::Settings settings(Message::kNoLinkSecurity, Message::kPriorityNet);

    message = mSocket.NewMessage(0, settings);
    VerifyOrExit(message != nullptr, OT_NOOP);

    message->SetSubType(Message::kSubTypeMleGeneral);

exit:
    return message;
}

otError Mle::AppendHeader(Message &aMessage, Command aCommand)
{
    otError error = OT_ERROR_NONE;
    Header  header;

    header.Init();

    if (aCommand == kCommandDiscoveryRequest || aCommand == kCommandDiscoveryResponse)
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
    return Tlv::AppendUint16Tlv(aMessage, Tlv::kSourceAddress, GetRloc16());
}

otError Mle::AppendStatus(Message &aMessage, StatusTlv::Status aStatus)
{
    return Tlv::AppendUint8Tlv(aMessage, Tlv::kStatus, static_cast<uint8_t>(aStatus));
}

otError Mle::AppendMode(Message &aMessage, DeviceMode aMode)
{
    return Tlv::AppendUint8Tlv(aMessage, Tlv::kMode, aMode.Get());
}

otError Mle::AppendTimeout(Message &aMessage, uint32_t aTimeout)
{
    return Tlv::AppendUint32Tlv(aMessage, Tlv::kTimeout, aTimeout);
}

otError Mle::AppendChallenge(Message &aMessage, const Challenge &aChallenge)
{
    return Tlv::AppendTlv(aMessage, Tlv::kChallenge, aChallenge.mBuffer, aChallenge.mLength);
}

otError Mle::AppendChallenge(Message &aMessage, const uint8_t *aChallenge, uint8_t aChallengeLength)
{
    return Tlv::AppendTlv(aMessage, Tlv::kChallenge, aChallenge, aChallengeLength);
}

otError Mle::AppendResponse(Message &aMessage, const Challenge &aResponse)
{
    return Tlv::AppendTlv(aMessage, Tlv::kResponse, aResponse.mBuffer, aResponse.mLength);
}

otError Mle::ReadChallengeOrResponse(const Message &aMessage, uint8_t aTlvType, Challenge &aBuffer)
{
    otError  error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, aTlvType, offset, length));
    VerifyOrExit(length >= kMinChallengeSize, error = OT_ERROR_PARSE);

    if (length > kMaxChallengeSize)
    {
        length = kMaxChallengeSize;
    }

    aMessage.Read(offset, length, aBuffer.mBuffer);
    aBuffer.mLength = static_cast<uint8_t>(length);

exit:
    return error;
}

otError Mle::ReadChallenge(const Message &aMessage, Challenge &aChallenge)
{
    return ReadChallengeOrResponse(aMessage, Tlv::kChallenge, aChallenge);
}

otError Mle::ReadResponse(const Message &aMessage, Challenge &aResponse)
{
    return ReadChallengeOrResponse(aMessage, Tlv::kResponse, aResponse);
}

otError Mle::AppendLinkFrameCounter(Message &aMessage)
{
    return Tlv::AppendUint32Tlv(aMessage, Tlv::kLinkFrameCounter, Get<KeyManager>().GetMacFrameCounter());
}

otError Mle::AppendMleFrameCounter(Message &aMessage)
{
    return Tlv::AppendUint32Tlv(aMessage, Tlv::kMleFrameCounter, Get<KeyManager>().GetMleFrameCounter());
}

otError Mle::AppendAddress16(Message &aMessage, uint16_t aRloc16)
{
    return Tlv::AppendUint16Tlv(aMessage, Tlv::kAddress16, aRloc16);
}

otError Mle::AppendLeaderData(Message &aMessage)
{
    LeaderDataTlv leaderDataTlv;

    mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion());
    mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetStableVersion());

    leaderDataTlv.Init();
    leaderDataTlv.Set(mLeaderData);

    return leaderDataTlv.AppendTo(aMessage);
}

otError Mle::ReadLeaderData(const Message &aMessage, LeaderData &aLeaderData)
{
    otError       error;
    LeaderDataTlv leaderDataTlv;

    SuccessOrExit(error = Tlv::FindTlv(aMessage, Tlv::kLeaderData, sizeof(leaderDataTlv), leaderDataTlv));
    VerifyOrExit(leaderDataTlv.IsValid(), error = OT_ERROR_PARSE);
    leaderDataTlv.Get(aLeaderData);

exit:
    return error;
}

otError Mle::AppendNetworkData(Message &aMessage, bool aStableOnly)
{
    otError error = OT_ERROR_NONE;
    uint8_t networkData[NetworkData::NetworkData::kMaxSize];
    uint8_t length;

    VerifyOrExit(!mRetrieveNewNetworkData, error = OT_ERROR_INVALID_STATE);

    length = sizeof(networkData);
    IgnoreError(Get<NetworkData::Leader>().GetNetworkData(aStableOnly, networkData, length));

    error = Tlv::AppendTlv(aMessage, Tlv::kNetworkData, networkData, length);

exit:
    return error;
}

otError Mle::AppendTlvRequest(Message &aMessage, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    return Tlv::AppendTlv(aMessage, Tlv::kTlvRequest, aTlvs, aTlvsLength);
}

otError Mle::FindTlvRequest(const Message &aMessage, RequestedTlvs &aRequestedTlvs)
{
    otError  error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Tlv::kTlvRequest, offset, length));

    if (length > sizeof(aRequestedTlvs.mTlvs))
    {
        length = sizeof(aRequestedTlvs.mTlvs);
    }

    aMessage.Read(offset, length, aRequestedTlvs.mTlvs);
    aRequestedTlvs.mNumTlvs = static_cast<uint8_t>(length);

exit:
    return error;
}

otError Mle::AppendScanMask(Message &aMessage, uint8_t aScanMask)
{
    return Tlv::AppendUint8Tlv(aMessage, Tlv::kScanMask, aScanMask);
}

otError Mle::AppendLinkMargin(Message &aMessage, uint8_t aLinkMargin)
{
    return Tlv::AppendUint8Tlv(aMessage, Tlv::kLinkMargin, aLinkMargin);
}

otError Mle::AppendVersion(Message &aMessage)
{
    return Tlv::AppendUint16Tlv(aMessage, Tlv::kVersion, kThreadVersion);
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
        // For sleepy end-device, we register any external multicast
        // addresses.

        retval = Get<ThreadNetif>().HasAnyExternalMulticastAddress();
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
#if OPENTHREAD_CONFIG_DUA_ENABLE
    Ip6::Address domainUnicastAddress;
#endif

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = aMessage.Append(&tlv, sizeof(tlv)));

    // Prioritize ML-EID
    entry.SetContextId(kMeshLocalPrefixContextId);
    entry.SetIid(GetMeshLocal64().GetIid());
    SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
    length += entry.GetLength();

    // Continue to append the other addresses if not `kAppendMeshLocalOnly` mode
    VerifyOrExit(aMode != kAppendMeshLocalOnly, OT_NOOP);
    counter++;

#if OPENTHREAD_CONFIG_DUA_ENABLE
    // Cache Domain Unicast Address.
    domainUnicastAddress = Get<DuaManager>().GetDomainUnicastAddress();

    if (Get<ThreadNetif>().HasUnicastAddress(domainUnicastAddress))
    {
        error = Get<NetworkData::Leader>().GetContext(domainUnicastAddress, context);

        OT_ASSERT(error == OT_ERROR_NONE);

        // Prioritize DUA, compressed entry
        entry.SetContextId(context.mContextId);
        entry.SetIid(domainUnicastAddress.GetIid());
        SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
        length += entry.GetLength();
        counter++;
    }
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

    for (const Ip6::NetifUnicastAddress *addr = Get<ThreadNetif>().GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        if (addr->GetAddress().IsLinkLocal() || IsRoutingLocator(addr->GetAddress()) ||
            IsAnycastLocator(addr->GetAddress()) || addr->GetAddress() == GetMeshLocal64())
        {
            continue;
        }

#if OPENTHREAD_CONFIG_DUA_ENABLE
        // Skip DUA that was already appended above.
        if (addr->GetAddress() == domainUnicastAddress)
        {
            continue;
        }
#endif

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
        VerifyOrExit(counter < OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER, OT_NOOP);
    }

    // Append external multicast addresses.  For sleepy end device,
    // register all external multicast addresses with the parent for
    // indirect transmission. Since Thread 1.2, non-sleepy MED should
    // also register external multicast addresses of scope larger than
    // realm with a 1.2 or higher parent.
    if (!IsRxOnWhenIdle()
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        || !GetParent().IsThreadVersion1p1()
#endif
    )
    {
        for (const Ip6::NetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
        {
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
            // For Thread 1.2 MED, skip multicast address with scope not
            // larger than realm local when registering.
            if (IsRxOnWhenIdle() && !addr.GetAddress().IsMulticastLargerThanRealmLocal())
            {
                continue;
            }
#endif

            entry.SetUncompressed();
            entry.SetIp6Address(addr.GetAddress());
            SuccessOrExit(error = aMessage.Append(&entry, entry.GetLength()));
            length += entry.GetLength();

            counter++;
            // only continue to append if there is available entry.
            VerifyOrExit(counter < OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER, OT_NOOP);
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
    return Tlv::AppendUint16Tlv(aMessage, Tlv::kXtalAccuracy, otPlatTimeGetXtalAccuracy());
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

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError Mle::AppendCslChannel(Message &aMessage)
{
    otError       error = OT_ERROR_NONE;
    CslChannelTlv cslChannel;

    // In current implementation, it's allowed to set CSL Channel unspecified. As `0` is not valid for Channel value
    // in CSL Channel TLV, if CSL channel is not specified, we don't append CSL Channel TLV.
    // And on transmitter side, it would also set CSL Channel for the child to `0` if it doesn't find a CSL Channel
    // TLV.
    VerifyOrExit(Get<Mac::Mac>().IsCslChannelSpecified(), OT_NOOP);

    cslChannel.Init();
    cslChannel.SetChannelPage(0);
    cslChannel.SetChannel(Get<Mac::Mac>().GetCslChannel());

    SuccessOrExit(error = aMessage.Append(&cslChannel, sizeof(CslChannelTlv)));

exit:
    return error;
}

otError Mle::AppendCslTimeout(Message &aMessage)
{
    OT_ASSERT(Get<Mac::Mac>().IsCslEnabled());
    return Tlv::AppendUint32Tlv(aMessage, Tlv::kCslTimeout,
                                Get<Mac::Mac>().GetCslTimeout() == 0 ? mTimeout : Get<Mac::Mac>().GetCslTimeout());
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

void Mle::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(!IsDisabled(), OT_NOOP);

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        if (IsChild() && !IsFullThreadDevice() && mAddressRegistrationMode == kAppendMeshLocalOnly)
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

    if (aEvents.ContainsAny(kEventIp6AddressAdded | kEventIp6AddressRemoved))
    {
        if (!Get<ThreadNetif>().HasUnicastAddress(mMeshLocal64.GetAddress()))
        {
            // Mesh Local EID was removed, choose a new one and add it back
            mMeshLocal64.GetAddress().GetIid().GenerateRandom();

            Get<ThreadNetif>().AddUnicastAddress(mMeshLocal64);
            Get<Notifier>().Signal(kEventThreadMeshLocalAddrChanged);
        }

        if (IsChild() && !IsFullThreadDevice())
        {
            mChildUpdateRequestState = kChildUpdateRequestPending;
            ScheduleMessageTransmissionTimer();
        }
    }

    if (aEvents.ContainsAny(kEventIp6MulticastSubscribed | kEventIp6MulticastUnsubscribed))
    {
        // When multicast subscription changes, SED always notifies its parent as it depends on its
        // parent for indirect transmission. Since Thread 1.2, MED MAY also notify its parent of 1.2
        // or higher version as it could depend on its parent to perform Multicast Listener Report.
        if (IsChild() && !IsFullThreadDevice() &&
            (!IsRxOnWhenIdle()
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
             || !GetParent().IsThreadVersion1p1()
#endif
                 ))

        {
            mChildUpdateRequestState = kChildUpdateRequestPending;
            ScheduleMessageTransmissionTimer();
        }
    }

    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            Get<MleRouter>().HandleNetworkDataUpdateRouter();
        }
        else
#endif
        {
            if (!aEvents.Contains(kEventThreadRoleChanged))
            {
                mChildUpdateRequestState = kChildUpdateRequestPending;
                ScheduleMessageTransmissionTimer();
            }
        }

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        Get<BackboneRouter::Leader>().Update();
#endif
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
        UpdateServiceAlocs();
#endif

#if OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE
        IgnoreError(Get<Dhcp6::Server>().UpdateService());
#endif // OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
        Get<Dhcp6::Client>().UpdateAddresses();
#endif // OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
    }

    if (aEvents.ContainsAny(kEventThreadRoleChanged | kEventThreadKeySeqCounterChanged))
    {
        // Store the settings on a key seq change, or when role changes and device
        // is attached (i.e., skip `Store()` on role change to detached).

        if (aEvents.Contains(kEventThreadKeySeqCounterChanged) || IsAttached())
        {
            IgnoreError(Store());
        }
    }

    if (aEvents.Contains(kEventSecurityPolicyChanged))
    {
        Get<Ip6::Filter>().AllowNativeCommissioner(Get<KeyManager>().IsNativeCommissioningAllowed());
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
    NetworkData::Iterator serviceIterator    = NetworkData::kIteratorInit;
    size_t                serviceAlocsLength = OT_ARRAY_LENGTH(mServiceAlocs);
    size_t                i                  = 0;

    VerifyOrExit(!IsDisabled(), OT_NOOP);

    // First remove all alocs which are no longer necessary, to free up space in mServiceAlocs
    for (i = 0; i < serviceAlocsLength; i++)
    {
        serviceAloc = mServiceAlocs[i].GetAddress().GetIid().GetLocator();

        if ((serviceAloc != Mac::kShortAddrInvalid) &&
            (!Get<NetworkData::Leader>().ContainsService(Mle::ServiceIdFromAloc(serviceAloc), rloc)))
        {
            Get<ThreadNetif>().RemoveUnicastAddress(mServiceAlocs[i]);
            mServiceAlocs[i].GetAddress().GetIid().SetLocator(Mac::kShortAddrInvalid);
        }
    }

    // Now add any missing service alocs which should be there, if there is enough space in mServiceAlocs
    while (Get<NetworkData::Leader>().GetNextServiceId(serviceIterator, rloc, serviceId) == OT_ERROR_NONE)
    {
        for (i = 0; i < serviceAlocsLength; i++)
        {
            serviceAloc = mServiceAlocs[i].GetAddress().GetIid().GetLocator();

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
                serviceAloc = mServiceAlocs[i].GetAddress().GetIid().GetLocator();

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
            (!IsChild() || mReceivedResponseFromParent || mParentRequestMode == kAttachBetter) &&
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
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);

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
            IgnoreError(SendParentRequest(kParentRequestTypeRoutersAndReeds));
            delay = kParentRequestReedTimeout;
        }
        // initial MLE Parent Request has only R flag set in Scan Mask TLV for
        // during initial attach or downgrade process
        else
        {
            IgnoreError(SendParentRequest(kParentRequestTypeRouters));
            delay = kParentRequestRouterTimeout;
        }

        break;

    case kAttachStateParentRequestRouter:
        SetAttachState(kAttachStateParentRequestReed);
        IgnoreError(SendParentRequest(kParentRequestTypeRoutersAndReeds));
        delay = kParentRequestReedTimeout;
        break;

    case kAttachStateParentRequestReed:
        shouldAnnounce = PrepareAnnounceState();

        if (shouldAnnounce)
        {
            SetAttachState(kAttachStateAnnounce);
            IgnoreError(SendParentRequest(kParentRequestTypeRoutersAndReeds));
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

    VerifyOrExit(!IsChild() && (mReattachState == kReattachStop) &&
                     (Get<MeshCoP::ActiveDataset>().IsPartiallyComplete() || !IsFullThreadDevice()),
                 OT_NOOP);

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
            IgnoreError(Get<MeshCoP::PendingDataset>().ApplyConfiguration());
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
        IgnoreError(Get<MeshCoP::ActiveDataset>().Restore());
    }

    VerifyOrExit(mReattachState == kReattachStop, OT_NOOP);

    switch (mParentRequestMode)
    {
    case kAttachAny:
        if (!IsChild())
        {
            if (mAlternatePanId != Mac::kPanIdBroadcast)
            {
                IgnoreError(Get<Mac::Mac>().SetPanChannel(mAlternateChannel));
                Get<Mac::Mac>().SetPanId(mAlternatePanId);
                mAlternatePanId = Mac::kPanIdBroadcast;
                IgnoreError(BecomeDetached());
            }
#if OPENTHREAD_FTD
            else if (IsFullThreadDevice() && Get<MleRouter>().BecomeLeader() == OT_ERROR_NONE)
            {
                // do nothing
            }
#endif
            else
            {
                IgnoreError(BecomeDetached());
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
        IgnoreError(BecomeChild(kAttachSame2));
        break;

    case kAttachSame2:
    case kAttachSameDowngrade:
        IgnoreError(BecomeChild(kAttachAny));
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
    DelayedResponseMetadata metadata;
    TimeMilli               now          = TimerMilli::GetNow();
    TimeMilli               nextSendTime = now.GetDistantFuture();
    Message *               message;
    Message *               nextMessage;

    for (message = mDelayedResponses.GetHead(); message != nullptr; message = nextMessage)
    {
        nextMessage = message->GetNext();

        metadata.ReadFrom(*message);

        if (now < metadata.mSendTime)
        {
            if (nextSendTime > metadata.mSendTime)
            {
                nextSendTime = metadata.mSendTime;
            }
        }
        else
        {
            mDelayedResponses.Dequeue(*message);
            metadata.RemoveFrom(*message);

            if (SendMessage(*message, metadata.mDestination) == OT_ERROR_NONE)
            {
                Log(kMessageSend, kTypeGenericDelayed, metadata.mDestination);

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
    Message *               message = mDelayedResponses.GetHead();
    DelayedResponseMetadata metadata;

    while (message != nullptr)
    {
        metadata.ReadFrom(*message);

        if (message->GetSubType() == Message::kSubTypeMleDataResponse)
        {
            mDelayedResponses.Dequeue(*message);
            message->Free();
            Log(kMessageRemoveDelayed, kTypeDataResponse, metadata.mDestination);

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

    mParentRequestChallenge.GenerateRandom();

    switch (aType)
    {
    case kParentRequestTypeRouters:
        scanMask = ScanMaskTlv::kRouterFlag;
        break;

    case kParentRequestTypeRoutersAndReeds:
        scanMask = ScanMaskTlv::kRouterFlag | ScanMaskTlv::kEndDeviceFlag;
        break;
    }

    VerifyOrExit((message = NewMleMessage()) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, kCommandParentRequest));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));
    SuccessOrExit(error = AppendChallenge(*message, mParentRequestChallenge));
    SuccessOrExit(error = AppendScanMask(*message, scanMask));
    SuccessOrExit(error = AppendVersion(*message));
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = AppendTimeRequest(*message));
#endif

    destination.SetToLinkLocalAllRoutersMulticast();
    SuccessOrExit(error = SendMessage(*message, destination));

    switch (aType)
    {
    case kParentRequestTypeRouters:
        Log(kMessageSend, kTypeParentRequestToRouters, destination);
        break;

    case kParentRequestTypeRoutersAndReeds:
        Log(kMessageSend, kTypeParentRequestToRoutersReeds, destination);
        break;
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Mle::RequestShorterChildIdRequest(void)
{
    if (mAttachState == kAttachStateChildIdRequest)
    {
        mAddressRegistrationMode = kAppendMeshLocalOnly;
        IgnoreError(SendChildIdRequest());
    }
}

otError Mle::SendChildIdRequest(void)
{
    otError      error   = OT_ERROR_NONE;
    uint8_t      tlvs[]  = {Tlv::kAddress16, Tlv::kNetworkData, Tlv::kRoute};
    uint8_t      tlvsLen = sizeof(tlvs);
    Message *    message = nullptr;
    Ip6::Address destination;

    if (mParent.GetExtAddress() == mParentCandidate.GetExtAddress())
    {
        if (IsChild())
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
            // that FindNeighbor() returns mParentCandidate when processing the Child ID Response.
            mParent.SetState(Neighbor::kStateInvalid);
        }
    }

    VerifyOrExit((message = NewMleMessage()) != nullptr, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleChildIdRequest);
    SuccessOrExit(error = AppendHeader(*message, kCommandChildIdRequest));
    SuccessOrExit(error = AppendResponse(*message, mParentCandidateChallenge));
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

    destination.SetToLinkLocalAddress(mParentCandidate.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    Log(kMessageSend,
        (mAddressRegistrationMode == kAppendMeshLocalOnly) ? kTypeChildIdRequestShort : kTypeChildIdRequest,
        destination);

    if (!IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SetAttachMode(true);
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

otError Mle::SendDataRequest(const Ip6::Address &aDestination,
                             const uint8_t *     aTlvs,
                             uint8_t             aTlvsLength,
                             uint16_t            aDelay,
                             const uint8_t *     aExtraTlvs,
                             uint8_t             aExtraTlvsLength)
{
    OT_UNUSED_VARIABLE(aExtraTlvs);
    OT_UNUSED_VARIABLE(aExtraTlvsLength);

    otError  error = OT_ERROR_NONE;
    Message *message;

    VerifyOrExit((message = NewMleMessage()) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, kCommandDataRequest));
    SuccessOrExit(error = AppendTlvRequest(*message, aTlvs, aTlvsLength));
    SuccessOrExit(error = AppendActiveTimestamp(*message));
    SuccessOrExit(error = AppendPendingTimestamp(*message));

    if (aExtraTlvs != nullptr && aExtraTlvsLength > 0)
    {
        SuccessOrExit(error = message->Append(aExtraTlvs, aExtraTlvsLength));
    }

    if (aDelay)
    {
        SuccessOrExit(error = AddDelayedResponse(*message, aDestination, aDelay));
        Log(kMessageDelay, kTypeDataRequest, aDestination);
    }
    else
    {
        SuccessOrExit(error = SendMessage(*message, aDestination));
        Log(kMessageSend, kTypeDataRequest, aDestination);

        if (!IsRxOnWhenIdle())
        {
            Get<DataPollSender>().SendFastPolls(DataPollSender::kDefaultFastPolls);
        }
    }

exit:
    FreeMessageOnError(message, error);

    if (IsChild() && !IsRxOnWhenIdle())
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
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        // CSL transmitter may respond in next CSL cycle.
        if (Get<Mac::Mac>().IsCslEnabled())
        {
            ExitNow(interval = Get<Mac::Mac>().GetCslPeriod() * kUsPerTenSymbols / 1000 +
                               static_cast<uint32_t>(kUnicastRetransmissionDelay));
        }
        else
#endif
        {
            ExitNow(interval = kUnicastRetransmissionDelay);
        }
    }

    switch (mDataRequestState)
    {
    case kDataRequestNone:
        break;

    case kDataRequestActive:
        ExitNow(interval = kUnicastRetransmissionDelay);
    }

    if (IsChild() && IsRxOnWhenIdle())
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
    //  - Delaying kEvent notification triggered "Child Update Request" transmission (to allow aggregation),
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

            VerifyOrExit(mDataRequestAttempts < kMaxChildKeepAliveAttempts, IgnoreError(BecomeDetached()));

            destination.SetToLinkLocalAddress(mParent.GetExtAddress());

            if (SendDataRequest(destination, tlvs, sizeof(tlvs), 0) == OT_ERROR_NONE)
            {
                mDataRequestAttempts++;
            }

            ExitNow();
        }

        // Keep-alive "Child Update Request" only on a non-sleepy child
        VerifyOrExit(IsChild() && IsRxOnWhenIdle(), OT_NOOP);
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

    VerifyOrExit(mChildUpdateAttempts < kMaxChildKeepAliveAttempts, IgnoreError(BecomeDetached()));

    if (SendChildUpdateRequest() == OT_ERROR_NONE)
    {
        mChildUpdateAttempts++;
    }

exit:
    return;
}

otError Mle::SendChildUpdateRequest(void)
{
    otError                 error = OT_ERROR_NONE;
    Ip6::Address            destination;
    Message *               message = nullptr;
    AddressRegistrationMode mode    = kAppendAllAddresses;

    if (!mParent.IsStateValidOrRestoring())
    {
        otLogWarnMle("No valid parent when sending Child Update Request");
        IgnoreError(BecomeDetached());
        ExitNow();
    }

    mChildUpdateRequestState = kChildUpdateRequestActive;
    ScheduleMessageTransmissionTimer();

    VerifyOrExit((message = NewMleMessage()) != nullptr, error = OT_ERROR_NO_BUFS);
    message->SetSubType(Message::kSubTypeMleChildUpdateRequest);
    SuccessOrExit(error = AppendHeader(*message, kCommandChildUpdateRequest));
    SuccessOrExit(error = AppendMode(*message, mDeviceMode));

    switch (mRole)
    {
    case kRoleDetached:
        mParentRequestChallenge.GenerateRandom();
        SuccessOrExit(error = AppendChallenge(*message, mParentRequestChallenge));
        mode = kAppendMeshLocalOnly;
        break;

    case kRoleChild:
        SuccessOrExit(error = AppendSourceAddress(*message));
        SuccessOrExit(error = AppendLeaderData(*message));
        SuccessOrExit(error = AppendTimeout(*message, mTimeout));
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        if (Get<Mac::Mac>().IsCslEnabled())
        {
            SuccessOrExit(error = AppendCslChannel(*message));
            SuccessOrExit(error = AppendCslTimeout(*message));
        }
#endif
        break;

    case kRoleDisabled:
    case kRoleRouter:
    case kRoleLeader:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    if (!IsFullThreadDevice())
    {
        SuccessOrExit(error = AppendAddressRegistration(*message, mode));
    }

    destination.SetToLinkLocalAddress(mParent.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    Log(kMessageSend, kTypeChildUpdateRequestOfParent, destination);

    if (!IsRxOnWhenIdle())
    {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        Get<DataPollSender>().SetAttachMode(!Get<Mac::Mac>().IsCslEnabled());
#else
        Get<DataPollSender>().SetAttachMode(true);
#endif
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
    }
    else
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(true);
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

otError Mle::SendChildUpdateResponse(const uint8_t *aTlvs, uint8_t aNumTlvs, const Challenge &aChallenge)
{
    otError      error = OT_ERROR_NONE;
    Ip6::Address destination;
    Message *    message;
    bool         checkAddress = false;

    VerifyOrExit((message = NewMleMessage()) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = AppendHeader(*message, kCommandChildUpdateResponse));
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
            SuccessOrExit(error = AppendResponse(*message, aChallenge));
            break;

        case Tlv::kLinkFrameCounter:
            SuccessOrExit(error = AppendLinkFrameCounter(*message));
            break;

        case Tlv::kMleFrameCounter:
            SuccessOrExit(error = AppendMleFrameCounter(*message));
            break;
        }
    }

    destination.SetToLinkLocalAddress(mParent.GetExtAddress());
    SuccessOrExit(error = SendMessage(*message, destination));

    Log(kMessageSend, kTypeChildUpdateResponseOfParent, destination);

    if (checkAddress && HasUnregisteredAddress())
    {
        IgnoreError(SendChildUpdateRequest());
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Mle::SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce)
{
    Ip6::Address destination;

    destination.SetToLinkLocalAllNodesMulticast();

    SendAnnounce(aChannel, aOrphanAnnounce, destination);
}

void Mle::SendAnnounce(uint8_t aChannel, bool aOrphanAnnounce, const Ip6::Address &aDestination)
{
    otError            error = OT_ERROR_NONE;
    ChannelTlv         channel;
    ActiveTimestampTlv activeTimestamp;
    Message *          message = nullptr;

    VerifyOrExit(Get<Mac::Mac>().GetSupportedChannelMask().ContainsChannel(aChannel), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((message = NewMleMessage()) != nullptr, error = OT_ERROR_NO_BUFS);
    message->SetLinkSecurityEnabled(true);
    message->SetSubType(Message::kSubTypeMleAnnounce);
    message->SetChannel(aChannel);
    SuccessOrExit(error = AppendHeader(*message, kCommandAnnounce));

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

    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kPanId, Get<Mac::Mac>().GetPanId()));

    SuccessOrExit(error = SendMessage(*message, aDestination));

    otLogInfoMle("Send Announce on channel %d", aChannel);

exit:
    FreeMessageOnError(message, error);
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
    uint8_t          nonce[Crypto::AesCcm::kNonceSize];
    uint8_t          tag[kMleSecurityTagSize];
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

        Crypto::AesCcm::GenerateNonce(Get<Mac::Mac>().GetExtAddress(), Get<KeyManager>().GetMleFrameCounter(),
                                      Mac::Frame::kSecEncMic32, nonce);

        aesCcm.SetKey(Get<KeyManager>().GetCurrentMleKey());
        aesCcm.Init(16 + 16 + header.GetHeaderLength(), aMessage.GetLength() - (header.GetLength() - 1), sizeof(tag),
                    nonce, sizeof(nonce));

        aesCcm.Header(&mLinkLocal64.GetAddress(), sizeof(mLinkLocal64.GetAddress()));
        aesCcm.Header(&aDestination, sizeof(aDestination));
        aesCcm.Header(header.GetBytes() + 1, header.GetHeaderLength());

        aMessage.SetOffset(header.GetLength() - 1);

        while (aMessage.GetOffset() < aMessage.GetLength())
        {
            length = aMessage.Read(aMessage.GetOffset(), sizeof(buf), buf);
            aesCcm.Payload(buf, buf, length, Crypto::AesCcm::kEncrypt);
            aMessage.Write(aMessage.GetOffset(), length, buf);
            aMessage.MoveOffset(length);
        }

        aesCcm.Finalize(tag);
        SuccessOrExit(error = aMessage.Append(tag, sizeof(tag)));

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
    otError                 error = OT_ERROR_NONE;
    DelayedResponseMetadata metadata;

    metadata.mSendTime    = TimerMilli::GetNow() + aDelay;
    metadata.mDestination = aDestination;

    SuccessOrExit(error = metadata.AppendTo(aMessage));
    mDelayedResponses.Enqueue(aMessage);

    mDelayedResponseTimer.FireAtIfEarlier(metadata.mSendTime);

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
    const Key *     mleKey;
    uint32_t        frameCounter;
    uint8_t         messageTag[kMleSecurityTagSize];
    uint8_t         nonce[Crypto::AesCcm::kNonceSize];
    Mac::ExtAddress extAddr;
    Crypto::AesCcm  aesCcm;
    uint16_t        mleOffset;
    uint8_t         buf[64];
    uint16_t        length;
    uint8_t         tag[kMleSecurityTagSize];
    uint8_t         command;
    Neighbor *      neighbor;

    otLogDebgMle("Receive UDP message");

    VerifyOrExit(aMessageInfo.GetLinkInfo() != nullptr, OT_NOOP);
    VerifyOrExit(aMessageInfo.GetHopLimit() == kMleHopLimit, error = OT_ERROR_PARSE);

    length = aMessage.Read(aMessage.GetOffset(), sizeof(header), &header);
    VerifyOrExit(header.IsValid() && header.GetLength() <= length, error = OT_ERROR_PARSE);

    if (header.GetSecuritySuite() == Header::kNoSecurity)
    {
        aMessage.MoveOffset(header.GetLength());

        switch (header.GetCommand())
        {
#if OPENTHREAD_FTD
        case kCommandDiscoveryRequest:
            Get<MleRouter>().HandleDiscoveryRequest(aMessage, aMessageInfo);
            break;
#endif

        case kCommandDiscoveryResponse:
            Get<DiscoverScanner>().HandleDiscoveryResponse(aMessage, aMessageInfo);
            break;

        default:
            break;
        }

        ExitNow();
    }

    VerifyOrExit(!IsDisabled(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(header.GetSecuritySuite() == Header::k154Security, error = OT_ERROR_PARSE);

    keySequence = header.GetKeyId();

    if (keySequence == Get<KeyManager>().GetCurrentKeySequence())
    {
        mleKey = &Get<KeyManager>().GetCurrentMleKey();
    }
    else
    {
        mleKey = &Get<KeyManager>().GetTemporaryMleKey(keySequence);
    }

    VerifyOrExit(aMessage.GetOffset() + header.GetLength() + sizeof(messageTag) <= aMessage.GetLength(),
                 error = OT_ERROR_PARSE);
    aMessage.MoveOffset(header.GetLength() - 1);

    aMessage.Read(aMessage.GetLength() - sizeof(messageTag), sizeof(messageTag), messageTag);
    SuccessOrExit(error = aMessage.SetLength(aMessage.GetLength() - sizeof(messageTag)));

    aMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);

    frameCounter = header.GetFrameCounter();
    Crypto::AesCcm::GenerateNonce(extAddr, frameCounter, Mac::Frame::kSecEncMic32, nonce);

    aesCcm.SetKey(*mleKey);
    aesCcm.Init(sizeof(aMessageInfo.GetPeerAddr()) + sizeof(aMessageInfo.GetSockAddr()) + header.GetHeaderLength(),
                aMessage.GetLength() - aMessage.GetOffset(), sizeof(messageTag), nonce, sizeof(nonce));

    aesCcm.Header(&aMessageInfo.GetPeerAddr(), sizeof(aMessageInfo.GetPeerAddr()));
    aesCcm.Header(&aMessageInfo.GetSockAddr(), sizeof(aMessageInfo.GetSockAddr()));
    aesCcm.Header(header.GetBytes() + 1, header.GetHeaderLength());

    mleOffset = aMessage.GetOffset();

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        length = aMessage.Read(aMessage.GetOffset(), sizeof(buf), buf);
        aesCcm.Payload(buf, buf, length, Crypto::AesCcm::kDecrypt);
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        aMessage.Write(aMessage.GetOffset(), length, buf);
#endif
        aMessage.MoveOffset(length);
    }

    aesCcm.Finalize(tag);
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

    neighbor = (command == kCommandChildIdResponse) ? mNeighborTable.FindParent(extAddr)
                                                    : mNeighborTable.FindNeighbor(extAddr);

    if (neighbor != nullptr && neighbor->IsStateValid())
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
    case kCommandAdvertisement:
        HandleAdvertisement(aMessage, aMessageInfo, neighbor);
        break;

    case kCommandDataResponse:
        HandleDataResponse(aMessage, aMessageInfo, neighbor);
        break;

    case kCommandParentResponse:
        HandleParentResponse(aMessage, aMessageInfo, keySequence);
        break;

    case kCommandChildIdResponse:
        HandleChildIdResponse(aMessage, aMessageInfo, neighbor);
        break;

    case kCommandAnnounce:
        HandleAnnounce(aMessage, aMessageInfo);
        break;

    case kCommandChildUpdateRequest:
#if OPENTHREAD_FTD
        if (IsRouterOrLeader())
        {
            Get<MleRouter>().HandleChildUpdateRequest(aMessage, aMessageInfo, keySequence);
        }
        else
#endif
        {
            HandleChildUpdateRequest(aMessage, aMessageInfo, neighbor);
        }

        break;

    case kCommandChildUpdateResponse:
#if OPENTHREAD_FTD
        if (IsRouterOrLeader())
        {
            Get<MleRouter>().HandleChildUpdateResponse(aMessage, aMessageInfo, keySequence, neighbor);
        }
        else
#endif
        {
            HandleChildUpdateResponse(aMessage, aMessageInfo, neighbor);
        }

        break;

#if OPENTHREAD_FTD
    case kCommandLinkRequest:
        Get<MleRouter>().HandleLinkRequest(aMessage, aMessageInfo, neighbor);
        break;

    case kCommandLinkAccept:
        Get<MleRouter>().HandleLinkAccept(aMessage, aMessageInfo, keySequence, neighbor);
        break;

    case kCommandLinkAcceptAndRequest:
        Get<MleRouter>().HandleLinkAcceptAndRequest(aMessage, aMessageInfo, keySequence, neighbor);
        break;

    case kCommandDataRequest:
        Get<MleRouter>().HandleDataRequest(aMessage, aMessageInfo, neighbor);
        break;

    case kCommandParentRequest:
        Get<MleRouter>().HandleParentRequest(aMessage, aMessageInfo);
        break;

    case kCommandChildIdRequest:
        Get<MleRouter>().HandleChildIdRequest(aMessage, aMessageInfo, keySequence);
        break;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    case kCommandTimeSync:
        Get<MleRouter>().HandleTimeSync(aMessage, aMessageInfo, neighbor);
        break;
#endif
#endif // OPENTHREAD_FTD

    default:
        ExitNow(error = OT_ERROR_DROP);
    }

exit:
    LogProcessError(kTypeGenericUdp, error);
}

void Mle::HandleAdvertisement(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor)
{
    otError    error = OT_ERROR_NONE;
    uint16_t   sourceAddress;
    LeaderData leaderData;
    uint8_t    tlvs[] = {Tlv::kNetworkData};
    uint16_t   delay;

    // Source Address
    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kSourceAddress, sourceAddress));

    Log(kMessageReceive, kTypeAdvertisement, aMessageInfo.GetPeerAddr(), sourceAddress);

    // Leader Data
    SuccessOrExit(error = ReadLeaderData(aMessage, leaderData));

    if (!IsDetached())
    {
#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            SuccessOrExit(error = Get<MleRouter>().HandleAdvertisement(aMessage, aMessageInfo, aNeighbor));
        }
        else
#endif
        {
            if ((aNeighbor == &mParent) && (mParent.GetRloc16() != sourceAddress))
            {
                // Remove stale parent.
                IgnoreError(BecomeDetached());
            }
        }
    }

    switch (mRole)
    {
    case kRoleDisabled:
    case kRoleDetached:
        ExitNow();

    case kRoleChild:
        VerifyOrExit(aNeighbor == &mParent, OT_NOOP);

        if ((mParent.GetRloc16() == sourceAddress) && (leaderData.GetPartitionId() != mLeaderData.GetPartitionId() ||
                                                       leaderData.GetLeaderRouterId() != GetLeaderId()))
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

#if OPENTHREAD_FTD
            if (IsFullThreadDevice())
            {
                RouteTlv route;

                if ((Tlv::FindTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE) && route.IsValid())
                {
                    // Overwrite Route Data
                    IgnoreError(Get<MleRouter>().ProcessRouteTlv(route));
                }
            }
#endif

            mRetrieveNewNetworkData = true;
        }

        mParent.SetLastHeard(TimerMilli::GetNow());
        break;

    case kRoleRouter:
    case kRoleLeader:
        VerifyOrExit(aNeighbor && aNeighbor->IsStateValid(), OT_NOOP);
        break;
    }

    if (mRetrieveNewNetworkData || IsNetworkDataNewer(leaderData))
    {
        delay = Random::NonCrypto::GetUint16InRange(0, kMleMaxResponseDelay);
        IgnoreError(SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay));
    }

exit:
    LogProcessError(kTypeAdvertisement, error);
}

void Mle::HandleDataResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, const Neighbor *aNeighbor)
{
    otError error;
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
    uint16_t metricsReportValueOffset;
    uint16_t length;
#endif

    Log(kMessageReceive, kTypeDataResponse, aMessageInfo.GetPeerAddr());

    VerifyOrExit(aNeighbor && aNeighbor->IsStateValid(), error = OT_ERROR_SECURITY);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
    if (Tlv::FindTlvValueOffset(aMessage, Tlv::kLinkMetricsReport, metricsReportValueOffset, length) == OT_ERROR_NONE)
    {
        Get<LinkMetrics>().HandleLinkMetricsReport(aMessage, metricsReportValueOffset, length,
                                                   aMessageInfo.GetPeerAddr());
    }
#endif

    error = HandleLeaderData(aMessage, aMessageInfo);

    if (mDataRequestState == kDataRequestNone && !IsRxOnWhenIdle())
    {
        // Here simply stops fast data poll request by Mle Data Request.
        // Note that in some cases fast data poll may continue after below stop operation until
        // running out the specified number. E.g. other component also trigger fast poll, and
        // is waiting for response; or the corner case where multiple Mle Data Request attempts
        // happened due to the retransmission mechanism.
        Get<DataPollSender>().StopFastPolls();
    }

exit:
    LogProcessError(kTypeDataResponse, error);
}

bool Mle::IsNetworkDataNewer(const LeaderData &aLeaderData)
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
    LeaderData          leaderData;
    ActiveTimestampTlv  activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    uint16_t            networkDataOffset    = 0;
    uint16_t            activeDatasetOffset  = 0;
    uint16_t            pendingDatasetOffset = 0;
    bool                dataRequest          = false;
    Tlv                 tlv;

    // Leader Data
    SuccessOrExit(error = ReadLeaderData(aMessage, leaderData));

    if ((leaderData.GetPartitionId() != mLeaderData.GetPartitionId()) ||
        (leaderData.GetWeighting() != mLeaderData.GetWeighting()) || (leaderData.GetLeaderRouterId() != GetLeaderId()))
    {
        if (IsChild())
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
        VerifyOrExit(IsNetworkDataNewer(leaderData), OT_NOOP);
    }

    // Active Timestamp
    if (Tlv::FindTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);
        timestamp = Get<MeshCoP::ActiveDataset>().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if (!IsLeader() && ((timestamp == nullptr) || (timestamp->Compare(activeTimestamp) != 0)) &&
            (Tlv::FindTlvOffset(aMessage, Tlv::kActiveDataset, activeDatasetOffset) != OT_ERROR_NONE))
        {
            ExitNow(dataRequest = true);
        }
    }
    else
    {
        activeTimestamp.SetLength(0);
    }

    // Pending Timestamp
    if (Tlv::FindTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        const MeshCoP::Timestamp *timestamp;

        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);
        timestamp = Get<MeshCoP::PendingDataset>().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if (!IsLeader() && ((timestamp == nullptr) || (timestamp->Compare(pendingTimestamp) != 0)) &&
            (Tlv::FindTlvOffset(aMessage, Tlv::kPendingDataset, pendingDatasetOffset) != OT_ERROR_NONE))
        {
            ExitNow(dataRequest = true);
        }
    }
    else
    {
        pendingTimestamp.SetLength(0);
    }

    if (Tlv::FindTlvOffset(aMessage, Tlv::kNetworkData, networkDataOffset) == OT_ERROR_NONE)
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

#if OPENTHREAD_FTD
    if (IsLeader())
    {
        Get<NetworkData::Leader>().IncrementVersionAndStableVersion();
    }
    else
#endif
    {
        // Active Dataset
        if (activeTimestamp.GetLength() > 0)
        {
            if (activeDatasetOffset > 0)
            {
                aMessage.Read(activeDatasetOffset, sizeof(tlv), &tlv);
                IgnoreError(Get<MeshCoP::ActiveDataset>().Save(activeTimestamp, aMessage,
                                                               activeDatasetOffset + sizeof(tlv), tlv.GetLength()));
            }
        }

        // Pending Dataset
        if (pendingTimestamp.GetLength() > 0)
        {
            if (pendingDatasetOffset > 0)
            {
                aMessage.Read(pendingDatasetOffset, sizeof(tlv), &tlv);
                IgnoreError(Get<MeshCoP::PendingDataset>().Save(pendingTimestamp, aMessage,
                                                                pendingDatasetOffset + sizeof(tlv), tlv.GetLength()));
            }
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

        IgnoreError(SendDataRequest(aMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay));
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

bool Mle::IsBetterParent(uint16_t               aRloc16,
                         uint8_t                aLinkQuality,
                         uint8_t                aLinkMargin,
                         const ConnectivityTlv &aConnectivityTlv,
                         uint8_t                aVersion)
{
    bool rval = false;

    uint8_t candidateLinkQualityIn     = mParentCandidate.GetLinkInfo().GetLinkQuality();
    uint8_t candidateTwoWayLinkQuality = (candidateLinkQualityIn < mParentCandidate.GetLinkQualityOut())
                                             ? candidateLinkQualityIn
                                             : mParentCandidate.GetLinkQualityOut();

    // Mesh Impacting Criteria
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

    // Prefer the parent with highest quality links (Link Quality 3 field in Connectivity TLV) to neighbors
    if (aConnectivityTlv.GetLinkQuality3() != mParentLinkQuality3)
    {
        ExitNow(rval = (aConnectivityTlv.GetLinkQuality3() > mParentLinkQuality3));
    }

    // Thread 1.2 Specification 4.5.2.1.2 Child Impacting Criteria
    if (aVersion != mParentCandidate.GetVersion())
    {
        ExitNow(rval = (aVersion > mParentCandidate.GetVersion()));
    }

    if (aConnectivityTlv.GetSedBufferSize() != mParentSedBufferSize)
    {
        ExitNow(rval = (aConnectivityTlv.GetSedBufferSize() > mParentSedBufferSize));
    }

    if (aConnectivityTlv.GetSedDatagramCount() != mParentSedDatagramCount)
    {
        ExitNow(rval = (aConnectivityTlv.GetSedDatagramCount() > mParentSedDatagramCount));
    }

    // Extra rules
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

void Mle::HandleParentResponse(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, uint32_t aKeySequence)
{
    otError               error    = OT_ERROR_NONE;
    const ThreadLinkInfo *linkInfo = aMessageInfo.GetThreadLinkInfo();
    Challenge             response;
    uint16_t              version;
    uint16_t              sourceAddress;
    LeaderData            leaderData;
    uint8_t               linkMarginFromTlv;
    uint8_t               linkMargin;
    uint8_t               linkQuality;
    ConnectivityTlv       connectivity;
    uint32_t              linkFrameCounter;
    uint32_t              mleFrameCounter;
    Mac::ExtAddress       extAddress;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    TimeParameterTlv timeParameter;
#endif

    // Source Address
    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kSourceAddress, sourceAddress));

    Log(kMessageReceive, kTypeParentResponse, aMessageInfo.GetPeerAddr(), sourceAddress);

    // Version
    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kVersion, version));
    VerifyOrExit(version >= OT_THREAD_VERSION_1_1, error = OT_ERROR_PARSE);

    // Response
    SuccessOrExit(error = ReadResponse(aMessage, response));
    VerifyOrExit(response == mParentRequestChallenge, error = OT_ERROR_PARSE);

    aMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddress);

    if (IsChild() && mParent.GetExtAddress() == extAddress)
    {
        mReceivedResponseFromParent = true;
    }

    // Leader Data
    SuccessOrExit(error = ReadLeaderData(aMessage, leaderData));

    // Link Margin
    SuccessOrExit(error = Tlv::FindUint8Tlv(aMessage, Tlv::kLinkMargin, linkMarginFromTlv));

    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->GetRss());

    if (linkMargin > linkMarginFromTlv)
    {
        linkMargin = linkMarginFromTlv;
    }

    linkQuality = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin);

    // Connectivity
    SuccessOrExit(error = Tlv::FindTlv(aMessage, Tlv::kConnectivity, sizeof(connectivity), connectivity));
    VerifyOrExit(connectivity.IsValid(), error = OT_ERROR_PARSE);

    // Share data with application, if requested.
    if (mParentResponseCb)
    {
        otThreadParentResponseInfo parentinfo;

        parentinfo.mExtAddr      = extAddress;
        parentinfo.mRloc16       = sourceAddress;
        parentinfo.mRssi         = linkInfo->GetRss();
        parentinfo.mPriority     = connectivity.GetParentPriority();
        parentinfo.mLinkQuality3 = connectivity.GetLinkQuality3();
        parentinfo.mLinkQuality2 = connectivity.GetLinkQuality2();
        parentinfo.mLinkQuality1 = connectivity.GetLinkQuality1();
        parentinfo.mIsAttached   = IsAttached();

        mParentResponseCb(&parentinfo, mParentResponseCbContext);
    }

#if OPENTHREAD_FTD
    if (IsFullThreadDevice() && !IsDetached())
    {
        int8_t diff = static_cast<int8_t>(connectivity.GetIdSequence() - Get<RouterTable>().GetRouterIdSequence());

        switch (mParentRequestMode)
        {
        case kAttachAny:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId() || diff > 0, OT_NOOP);
            break;

        case kAttachSame1:
        case kAttachSame2:
            VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), OT_NOOP);
            VerifyOrExit(diff > 0, OT_NOOP);
            break;

        case kAttachSameDowngrade:
            VerifyOrExit(leaderData.GetPartitionId() == mLeaderData.GetPartitionId(), OT_NOOP);
            VerifyOrExit(diff >= 0, OT_NOOP);
            break;

        case kAttachBetter:
            VerifyOrExit(leaderData.GetPartitionId() != mLeaderData.GetPartitionId(), OT_NOOP);

            VerifyOrExit(MleRouter::ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData,
                                                      Get<MleRouter>().IsSingleton(), mLeaderData) > 0,
                         OT_NOOP);
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

#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            compare = MleRouter::ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData, mParentIsSingleton,
                                                   mParentLeaderData);
        }

        // only consider partitions that are the same or better
        VerifyOrExit(compare >= 0, OT_NOOP);
#endif

        // only consider better parents if the partitions are the same
        VerifyOrExit(compare != 0 || IsBetterParent(sourceAddress, linkQuality, linkMargin, connectivity,
                                                    static_cast<uint8_t>(version)),
                     OT_NOOP);
    }

    // Link Frame Counter
    SuccessOrExit(error = Tlv::FindUint32Tlv(aMessage, Tlv::kLinkFrameCounter, linkFrameCounter));

    // Mle Frame Counter
    switch (Tlv::FindUint32Tlv(aMessage, Tlv::kMleFrameCounter, mleFrameCounter))
    {
    case OT_ERROR_NONE:
        break;
    case OT_ERROR_NOT_FOUND:
        mleFrameCounter = linkFrameCounter;
        break;
    default:
        ExitNow(error = OT_ERROR_PARSE);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    // Time Parameter
    if (Tlv::FindTlv(aMessage, Tlv::kTimeParameter, sizeof(timeParameter), timeParameter) == OT_ERROR_NONE)
    {
        VerifyOrExit(timeParameter.IsValid(), OT_NOOP);

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
    SuccessOrExit(error = ReadChallenge(aMessage, mParentCandidateChallenge));

    mParentCandidate.SetExtAddress(extAddress);
    mParentCandidate.SetRloc16(sourceAddress);
    mParentCandidate.SetLinkFrameCounter(linkFrameCounter);
    mParentCandidate.SetMleFrameCounter(mleFrameCounter);
    mParentCandidate.SetVersion(static_cast<uint8_t>(version));
    mParentCandidate.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                              DeviceMode::kModeFullNetworkData | DeviceMode::kModeSecureDataRequest));
    mParentCandidate.GetLinkInfo().Clear();
    mParentCandidate.GetLinkInfo().AddRss(linkInfo->GetRss());
    mParentCandidate.ResetLinkFailures();
    mParentCandidate.SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMarginFromTlv));
    mParentCandidate.SetState(Neighbor::kStateParentResponse);
    mParentCandidate.SetKeySequence(aKeySequence);

    mParentPriority         = connectivity.GetParentPriority();
    mParentLinkQuality3     = connectivity.GetLinkQuality3();
    mParentLinkQuality2     = connectivity.GetLinkQuality2();
    mParentLinkQuality1     = connectivity.GetLinkQuality1();
    mParentLeaderCost       = connectivity.GetLeaderCost();
    mParentSedBufferSize    = connectivity.GetSedBufferSize();
    mParentSedDatagramCount = connectivity.GetSedDatagramCount();
    mParentLeaderData       = leaderData;
    mParentIsSingleton      = connectivity.GetActiveRouters() <= 1;
    mParentLinkMargin       = linkMargin;

exit:
    LogProcessError(kTypeParentResponse, error);
}

void Mle::HandleChildIdResponse(const Message &         aMessage,
                                const Ip6::MessageInfo &aMessageInfo,
                                const Neighbor *        aNeighbor)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError             error = OT_ERROR_NONE;
    LeaderData          leaderData;
    uint16_t            sourceAddress;
    uint16_t            shortAddress;
    ActiveTimestampTlv  activeTimestamp;
    PendingTimestampTlv pendingTimestamp;
    Tlv                 tlv;
    uint16_t            networkDataOffset;
    uint16_t            offset;

    // Source Address
    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kSourceAddress, sourceAddress));

    Log(kMessageReceive, kTypeChildIdResponse, aMessageInfo.GetPeerAddr(), sourceAddress);

    VerifyOrExit(aNeighbor && aNeighbor->IsStateValid(), error = OT_ERROR_SECURITY);

    VerifyOrExit(mAttachState == kAttachStateChildIdRequest, OT_NOOP);

    // Leader Data
    SuccessOrExit(error = ReadLeaderData(aMessage, leaderData));

    // ShortAddress
    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kAddress16, shortAddress));

    // Network Data
    error = Tlv::FindTlvOffset(aMessage, Tlv::kNetworkData, networkDataOffset);
    SuccessOrExit(error);

    // Active Timestamp
    if (Tlv::FindTlv(aMessage, Tlv::kActiveTimestamp, sizeof(activeTimestamp), activeTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(activeTimestamp.IsValid(), error = OT_ERROR_PARSE);

        // Active Dataset
        if (Tlv::FindTlvOffset(aMessage, Tlv::kActiveDataset, offset) == OT_ERROR_NONE)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            IgnoreError(
                Get<MeshCoP::ActiveDataset>().Save(activeTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength()));
        }
    }

    // clear Pending Dataset if device succeed to reattach using stored Pending Dataset
    if (mReattachState == kReattachPending)
    {
        Get<MeshCoP::PendingDataset>().Clear();
    }

    // Pending Timestamp
    if (Tlv::FindTlv(aMessage, Tlv::kPendingTimestamp, sizeof(pendingTimestamp), pendingTimestamp) == OT_ERROR_NONE)
    {
        VerifyOrExit(pendingTimestamp.IsValid(), error = OT_ERROR_PARSE);

        // Pending Dataset
        if (Tlv::FindTlvOffset(aMessage, Tlv::kPendingDataset, offset) == OT_ERROR_NONE)
        {
            aMessage.Read(offset, sizeof(tlv), &tlv);
            IgnoreError(
                Get<MeshCoP::PendingDataset>().Save(pendingTimestamp, aMessage, offset + sizeof(tlv), tlv.GetLength()));
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

#if OPENTHREAD_FTD
    if (IsFullThreadDevice())
    {
        RouteTlv route;

        if (Tlv::FindTlv(aMessage, Tlv::kRoute, sizeof(route), route) == OT_ERROR_NONE)
        {
            SuccessOrExit(error = Get<MleRouter>().ProcessRouteTlv(route));
        }
    }
#endif

    mParent = mParentCandidate;
    mParentCandidate.Clear();

    mParent.SetRloc16(sourceAddress);

    IgnoreError(Get<NetworkData::Leader>().SetNetworkData(leaderData.GetDataVersion(),
                                                          leaderData.GetStableDataVersion(), !IsFullNetworkData(),
                                                          aMessage, networkDataOffset));

    SetStateChild(shortAddress);

exit:
    LogProcessError(kTypeChildIdResponse, error);
}

void Mle::HandleChildUpdateRequest(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Neighbor *aNeighbor)
{
    static const uint8_t kMaxResponseTlvs = 6;

    otError       error = OT_ERROR_NONE;
    uint16_t      sourceAddress;
    Challenge     challenge;
    RequestedTlvs requestedTlvs;
    uint8_t       tlvs[kMaxResponseTlvs] = {};
    uint8_t       numTlvs                = 0;

    // Source Address
    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kSourceAddress, sourceAddress));

    Log(kMessageReceive, kTypeChildUpdateRequestOfParent, aMessageInfo.GetPeerAddr(), sourceAddress);

    // Challenge
    switch (ReadChallenge(aMessage, challenge))
    {
    case OT_ERROR_NONE:
        tlvs[numTlvs++] = Tlv::kResponse;
        tlvs[numTlvs++] = Tlv::kMleFrameCounter;
        tlvs[numTlvs++] = Tlv::kLinkFrameCounter;
        break;
    case OT_ERROR_NOT_FOUND:
        break;
    default:
        ExitNow(error = OT_ERROR_PARSE);
    }

    if (aNeighbor == &mParent)
    {
        uint8_t status;

        switch (Tlv::FindUint8Tlv(aMessage, Tlv::kStatus, status))
        {
        case OT_ERROR_NONE:
            VerifyOrExit(status != StatusTlv::kError, IgnoreError(BecomeDetached()));
            break;
        case OT_ERROR_NOT_FOUND:
            break;
        default:
            ExitNow(error = OT_ERROR_PARSE);
        }

        if (mParent.GetRloc16() != sourceAddress)
        {
            IgnoreError(BecomeDetached());
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
    switch (FindTlvRequest(aMessage, requestedTlvs))
    {
    case OT_ERROR_NONE:
        for (uint8_t i = 0; i < requestedTlvs.mNumTlvs; i++)
        {
            if (numTlvs >= sizeof(tlvs))
            {
                otLogWarnMle("Failed to respond with TLVs: %d of %d", i, requestedTlvs.mNumTlvs);
                break;
            }

            tlvs[numTlvs++] = requestedTlvs.mTlvs[i];
        }
        break;
    case OT_ERROR_NOT_FOUND:
        break;
    default:
        ExitNow(error = OT_ERROR_PARSE);
    }

    SuccessOrExit(error = SendChildUpdateResponse(tlvs, numTlvs, challenge));

exit:
    LogProcessError(kTypeChildUpdateRequestOfParent, error);
}

void Mle::HandleChildUpdateResponse(const Message &         aMessage,
                                    const Ip6::MessageInfo &aMessageInfo,
                                    const Neighbor *        aNeighbor)
{
    otError   error = OT_ERROR_NONE;
    uint8_t   status;
    uint8_t   mode;
    Challenge response;
    uint32_t  linkFrameCounter;
    uint32_t  mleFrameCounter;
    uint16_t  sourceAddress;
    uint32_t  timeout;

    Log(kMessageReceive, kTypeChildUpdateResponseOfParent, aMessageInfo.GetPeerAddr());

    switch (mRole)
    {
    case kRoleDetached:
        SuccessOrExit(error = ReadResponse(aMessage, response));
        VerifyOrExit(response == mParentRequestChallenge, error = OT_ERROR_SECURITY);
        break;

    case kRoleChild:
        VerifyOrExit((aNeighbor == &mParent) && mParent.IsStateValid(), error = OT_ERROR_SECURITY);
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    // Status
    if (Tlv::FindUint8Tlv(aMessage, Tlv::kStatus, status) == OT_ERROR_NONE)
    {
        IgnoreError(BecomeDetached());
        ExitNow();
    }

    // Mode
    SuccessOrExit(error = Tlv::FindUint8Tlv(aMessage, Tlv::kMode, mode));
    VerifyOrExit(DeviceMode(mode) == mDeviceMode, error = OT_ERROR_DROP);

    switch (mRole)
    {
    case kRoleDetached:
        SuccessOrExit(error = Tlv::FindUint32Tlv(aMessage, Tlv::kLinkFrameCounter, linkFrameCounter));

        switch (Tlv::FindUint32Tlv(aMessage, Tlv::kMleFrameCounter, mleFrameCounter))
        {
        case OT_ERROR_NONE:
            break;
        case OT_ERROR_NOT_FOUND:
            mleFrameCounter = linkFrameCounter;
            break;
        default:
            ExitNow(error = OT_ERROR_PARSE);
        }

        mParent.SetLinkFrameCounter(linkFrameCounter);
        mParent.SetMleFrameCounter(mleFrameCounter);

        mParent.SetState(Neighbor::kStateValid);
        SetStateChild(GetRloc16());

        mRetrieveNewNetworkData = true;

        // fall through

    case kRoleChild:
        // Source Address
        SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kSourceAddress, sourceAddress));

        if (RouterIdFromRloc16(sourceAddress) != RouterIdFromRloc16(GetRloc16()))
        {
            IgnoreError(BecomeDetached());
            ExitNow();
        }

        // Leader Data, Network Data, Active Timestamp, Pending Timestamp
        SuccessOrExit(error = HandleLeaderData(aMessage, aMessageInfo));

        // Timeout optional
        switch (Tlv::FindUint32Tlv(aMessage, Tlv::kTimeout, timeout))
        {
        case OT_ERROR_NONE:
            mTimeout = timeout;
            break;
        case OT_ERROR_NOT_FOUND:
            break;
        default:
            ExitNow(error = OT_ERROR_PARSE);
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
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
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

    LogProcessError(kTypeChildUpdateResponseOfParent, error);
}

void Mle::HandleAnnounce(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError                   error = OT_ERROR_NONE;
    ChannelTlv                channelTlv;
    ActiveTimestampTlv        timestamp;
    const MeshCoP::Timestamp *localTimestamp;
    uint8_t                   channel;
    uint16_t                  panId;

    Log(kMessageReceive, kTypeAnnounce, aMessageInfo.GetPeerAddr());

    SuccessOrExit(error = Tlv::FindTlv(aMessage, Tlv::kChannel, sizeof(channelTlv), channelTlv));
    VerifyOrExit(channelTlv.IsValid(), error = OT_ERROR_PARSE);

    channel = static_cast<uint8_t>(channelTlv.GetChannel());

    SuccessOrExit(error = Tlv::FindTlv(aMessage, Tlv::kActiveTimestamp, sizeof(timestamp), timestamp));
    VerifyOrExit(timestamp.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kPanId, panId));

    localTimestamp = Get<MeshCoP::ActiveDataset>().GetTimestamp();

    if (localTimestamp == nullptr || localTimestamp->Compare(timestamp) > 0)
    {
        // No action is required if device is detached, and current
        // channel and pan-id match the values from the received MLE
        // Announce message.

        VerifyOrExit(!IsDetached() || (Get<Mac::Mac>().GetPanChannel() != channel) ||
                         (Get<Mac::Mac>().GetPanId() != panId),
                     OT_NOOP);

        if (mAttachState == kAttachStateProcessAnnounce)
        {
            VerifyOrExit(mAlternateTimestamp < timestamp.GetSeconds(), OT_NOOP);
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
    LogProcessError(kTypeAnnounce, error);
}

void Mle::ProcessAnnounce(void)
{
    uint8_t  newChannel = mAlternateChannel;
    uint16_t newPanId   = mAlternatePanId;

    OT_ASSERT(mAttachState == kAttachStateProcessAnnounce);

    otLogNoteMle("Processing Announce - channel %d, panid 0x%02x", newChannel, newPanId);

    Stop(/* aClearNetworkDatasets */ false);

    // Save the current/previous channel and pan-id
    mAlternateChannel   = Get<Mac::Mac>().GetPanChannel();
    mAlternatePanId     = Get<Mac::Mac>().GetPanId();
    mAlternateTimestamp = 0;

    IgnoreError(Get<Mac::Mac>().SetPanChannel(newChannel));
    Get<Mac::Mac>().SetPanId(newPanId);

    IgnoreError(Start(/* aAnnounceAttach */ true));
}

uint16_t Mle::GetNextHop(uint16_t aDestination) const
{
    OT_UNUSED_VARIABLE(aDestination);
    return (mParent.IsStateValid()) ? mParent.GetRloc16() : static_cast<uint16_t>(Mac::kShortAddrInvalid);
}

bool Mle::IsRoutingLocator(const Ip6::Address &aAddress) const
{
    return IsMeshLocalAddress(aAddress) && aAddress.GetIid().IsRoutingLocator();
}

bool Mle::IsAnycastLocator(const Ip6::Address &aAddress) const
{
    return IsMeshLocalAddress(aAddress) && aAddress.GetIid().IsAnycastLocator();
}

bool Mle::IsMeshLocalAddress(const Ip6::Address &aAddress) const
{
    return (aAddress.GetPrefix() == GetMeshLocalPrefix());
}

otError Mle::CheckReachability(uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    otError error;

    if ((aMeshDest != GetRloc16()) || Get<ThreadNetif>().HasUnicastAddress(aIp6Header.GetDestination()))
    {
        error = OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NO_ROUTE;
    }

    return error;
}

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
void Mle::InformPreviousParent(void)
{
    otError          error   = OT_ERROR_NONE;
    Message *        message = nullptr;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((mPreviousParentRloc != Mac::kShortAddrInvalid) && (mPreviousParentRloc != mParent.GetRloc16()),
                 OT_NOOP);

    mCounters.mParentChanges++;

    VerifyOrExit((message = Get<Ip6::Ip6>().NewMessage(0)) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->SetLength(0));

    messageInfo.SetSockAddr(GetMeshLocal64());
    messageInfo.SetPeerAddr(GetMeshLocal16());
    messageInfo.GetPeerAddr().GetIid().SetLocator(mPreviousParentRloc);

    SuccessOrExit(error = Get<Ip6::Ip6>().SendDatagram(*message, messageInfo, Ip6::kProtoNone));

    otLogNoteMle("Sending message to inform previous parent 0x%04x", mPreviousParentRloc);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to inform previous parent: %s", otThreadErrorToString(error));

        FreeMessage(message);
    }
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

    VerifyOrExit(IsChild(), OT_NOOP);

    parentRss = GetParent().GetLinkInfo().GetAverageRss();
    otLogInfoMle("PeriodicParentSearch: Parent RSS %d", parentRss);
    VerifyOrExit(parentRss != OT_RADIO_RSSI_INVALID, OT_NOOP);

    if (parentRss < kParentSearchRssThreadhold)
    {
        otLogInfoMle("PeriodicParentSearch: Parent RSS less than %d, searching for new parents",
                     kParentSearchRssThreadhold);
        mParentSearchIsInBackoff = true;
        IgnoreError(BecomeChild(kAttachAny));
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

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MLE == 1)
void Mle::Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress)
{
    Log(aAction, aType, aAddress, Mac::kShortAddrInvalid);
}

void Mle::Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress, uint16_t aRloc)
{
    enum : uint8_t
    {
        kRlocStringSize = 17,
    };

    String<kRlocStringSize> rlocString;

    if (aRloc != Mac::kShortAddrInvalid)
    {
        IgnoreError(rlocString.Set(",0x%04x", aRloc));
    }

    otLogInfoMle("%s %s%s (%s%s)", MessageActionToString(aAction), MessageTypeToString(aType),
                 MessageTypeActionToSuffixString(aType, aAction), aAddress.ToString().AsCString(),
                 rlocString.AsCString());
}
#endif

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN) && (OPENTHREAD_CONFIG_LOG_MLE == 1)
void Mle::LogProcessError(MessageType aType, otError aError)
{
    LogError(kMessageReceive, aType, aError);
}

void Mle::LogSendError(MessageType aType, otError aError)
{
    LogError(kMessageSend, aType, aError);
}

void Mle::LogError(MessageAction aAction, MessageType aType, otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        otLogWarnMle("Failed to %s %s%s: %s", aAction == kMessageSend ? "send" : "process", MessageTypeToString(aType),
                     MessageTypeActionToSuffixString(aType, aAction), otThreadErrorToString(aError));
    }
}

const char *Mle::MessageActionToString(MessageAction aAction)
{
    const char *str = "Unknown";

    switch (aAction)
    {
    case kMessageSend:
        str = "Send";
        break;
    case kMessageReceive:
        str = "Receive";
        break;
    case kMessageDelay:
        str = "Delay";
        break;
    case kMessageRemoveDelayed:
        str = "Remove Delayed";
        break;
    }

    return str;
}

const char *Mle::MessageTypeToString(MessageType aType)
{
    const char *str = "Unknown";

    switch (aType)
    {
    case kTypeAdvertisement:
        str = "Advertisement";
        break;
    case kTypeAnnounce:
        str = "Announce";
        break;
    case kTypeChildIdRequest:
        str = "Child ID Request";
        break;
    case kTypeChildIdRequestShort:
    case kTypeChildIdResponse:
        str = "Child ID Response";
        break;
    case kTypeChildUpdateRequestOfParent:
#if OPENTHREAD_FTD
    case kTypeChildUpdateRequestOfChild:
#endif
        str = "Child Update Request";
        break;
    case kTypeChildUpdateResponseOfParent:
#if OPENTHREAD_FTD
    case kTypeChildUpdateResponseOfChild:
    case kTypeChildUpdateResponseOfUnknownChild:
#endif
        str = "Child Update Response";
        break;
    case kTypeDataRequest:
        str = "Data Request";
        break;
    case kTypeDataResponse:
        str = "Data Response";
        break;
    case kTypeDiscoveryRequest:
        str = "Discovery Request";
        break;
    case kTypeDiscoveryResponse:
        str = "Discovery Response";
        break;
    case kTypeGenericDelayed:
        str = "delayed message";
        break;
    case kTypeGenericUdp:
        str = "UDP";
        break;
    case kTypeParentRequestToRouters:
    case kTypeParentRequestToRoutersReeds:
    case kTypeParentResponse:
        str = "Parent Response";
        break;
#if OPENTHREAD_FTD
    case kTypeAddressRelease:
        str = "Address Release";
        break;
    case kTypeAddressReleaseReply:
        str = "Address Release Reply";
        break;
    case kTypeAddressReply:
        str = "Address Reply";
        break;
    case kTypeAddressSolicit:
        str = "Address Solicit";
        break;
    case kTypeLinkAccept:
        str = "Link Accept";
        break;
    case kTypeLinkAcceptAndRequest:
        str = "Link Accept and Request";
        break;
    case kTypeLinkReject:
        str = "Link Reject";
        break;
    case kTypeLinkRequest:
        str = "Link Request";
        break;
    case kTypeParentRequest:
        str = "Parent Request";
        break;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    case kTypeTimeSync:
        str = "Time Sync";
        break;
#endif
#endif // OPENTHREAD_FTD
    }

    return str;
}

const char *Mle::MessageTypeActionToSuffixString(MessageType aType, MessageAction aAction)
{
    const char *str = "";

    switch (aType)
    {
    case kTypeChildIdRequestShort:
        str = " - short";
        break;
    case kTypeChildUpdateRequestOfParent:
    case kTypeChildUpdateResponseOfParent:
        str = (aAction == kMessageReceive) ? " from parent" : " to parent";
        break;

    case kTypeParentRequestToRouters:
        str = " to routers";
        break;
    case kTypeParentRequestToRoutersReeds:
        str = " to routers and REEDs";
        break;
#if OPENTHREAD_FTD
    case kTypeChildUpdateRequestOfChild:
    case kTypeChildUpdateResponseOfChild:
        str = (aAction == kMessageReceive) ? " from child" : " to child";
        break;
    case kTypeChildUpdateResponseOfUnknownChild:
        str = (aAction == kMessageReceive) ? " from unknown child" : " to child";
        break;
#endif // OPENTHREAD_FTD
    default:
        break;
    }

    return str;
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_WARN) && (OPENTHREAD_CONFIG_LOG_MLE == 1)

const char *Mle::RoleToString(DeviceRole aRole)
{
    const char *roleString = "Unknown";

    switch (aRole)
    {
    case kRoleDisabled:
        roleString = "Disabled";
        break;

    case kRoleDetached:
        roleString = "Detached";
        break;

    case kRoleChild:
        roleString = "Child";
        break;

    case kRoleRouter:
        roleString = "Router";
        break;

    case kRoleLeader:
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

void Mle::Challenge::GenerateRandom(void)
{
    mLength = kMaxChallengeSize;
    IgnoreError(Random::Crypto::FillBuffer(mBuffer, mLength));
}

bool Mle::Challenge::Matches(const uint8_t *aBuffer, uint8_t aLength) const
{
    return (mLength == aLength) && (memcmp(mBuffer, aBuffer, aLength) == 0);
}

void Mle::DelayedResponseMetadata::ReadFrom(const Message &aMessage)
{
    uint16_t length = aMessage.GetLength();

    OT_ASSERT(length >= sizeof(*this));
    aMessage.Read(length - sizeof(*this), sizeof(*this), this);
}

void Mle::DelayedResponseMetadata::RemoveFrom(Message &aMessage) const
{
    otError error = aMessage.SetLength(aMessage.GetLength() - sizeof(*this));
    OT_ASSERT(error == OT_ERROR_NONE);
    OT_UNUSED_VARIABLE(error);
}

} // namespace Mle
} // namespace ot
