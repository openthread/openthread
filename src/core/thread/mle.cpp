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

#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/random.hpp"
#include "common/serial_number.hpp"
#include "common/settings.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/address_resolver.hpp"
#include "thread/key_manager.hpp"
#include "thread/link_metrics.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/time_sync_service.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Mle {

RegisterLogModule("Mle");

const otMeshLocalPrefix Mle::sMeshLocalPrefixInit = {
    {0xfd, 0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0x00},
};

Mle::Mle(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRetrieveNewNetworkData(false)
    , mRole(kRoleDisabled)
    , mNeighborTable(aInstance)
    , mDeviceMode(DeviceMode::kModeRxOnWhenIdle)
    , mAttachState(kAttachStateIdle)
    , mReattachState(kReattachStop)
    , mAttachCounter(0)
    , mAnnounceDelay(kAnnounceTimeout)
    , mAttachTimer(aInstance, Mle::HandleAttachTimer)
    , mDelayedResponseTimer(aInstance, Mle::HandleDelayedResponseTimer)
    , mMessageTransmissionTimer(aInstance, Mle::HandleMessageTransmissionTimer)
    , mParentLeaderCost(0)
    , mAttachMode(kAnyPartition)
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
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    , mCslTimeout(OPENTHREAD_CONFIG_CSL_TIMEOUT)
#endif
    , mPreviousParentRloc(Mac::kShortAddrInvalid)
#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    , mParentSearchIsInBackoff(false)
    , mParentSearchBackoffWasCanceled(false)
    , mParentSearchRecentlyDetached(false)
    , mParentSearchBackoffCancelTime(0)
    , mParentSearchTimer(aInstance, Mle::HandleParentSearchTimer)
#endif
    , mAnnounceChannel(0)
    , mAlternateChannel(0)
    , mAlternatePanId(Mac::kPanIdBroadcast)
    , mAlternateTimestamp(0)
    , mParentResponseCb(nullptr)
    , mParentResponseCbContext(nullptr)
{
    mParent.Init(aInstance);
    mParentCandidate.Init(aInstance);

    mLeaderData.Clear();
    mParentLeaderData.Clear();
    mParent.Clear();
    mParentCandidate.Clear();
    ResetCounters();

    mLinkLocal64.InitAsThreadOrigin(/* aPreferred */ true);
    mLinkLocal64.GetAddress().SetToLinkLocalAddress(Get<Mac::Mac>().GetExtAddress());

    mLeaderAloc.InitAsThreadOriginRealmLocalScope();

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

    SetMeshLocalPrefix(AsCoreType(&sMeshLocalPrefixInit));

    // `SetMeshLocalPrefix()` also adds the Mesh-Local EID and subscribes
    // to the Link- and Realm-Local All Thread Nodes multicast addresses.
}

Error Mle::Enable(void)
{
    Error error = kErrorNone;

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

Error Mle::Disable(void)
{
    Error error = kErrorNone;

    Stop(kKeepNetworkDatasets);
    SuccessOrExit(error = mSocket.Close());
    Get<ThreadNetif>().RemoveUnicastAddress(mLinkLocal64);

exit:
    return error;
}

Error Mle::Start(StartMode aMode)
{
    Error error = kErrorNone;

    // cannot bring up the interface if IEEE 802.15.4 promiscuous mode is enabled
    VerifyOrExit(!Get<Radio>().GetPromiscuous(), error = kErrorInvalidState);
    VerifyOrExit(Get<ThreadNetif>().IsUp(), error = kErrorInvalidState);

    if (Get<Mac::Mac>().GetPanId() == Mac::kPanIdBroadcast)
    {
        Get<Mac::Mac>().SetPanId(Mac::GenerateRandomPanId());
    }

    SetStateDetached();

    ApplyMeshLocalPrefix();
    SetRloc16(GetRloc16());

    mAttachCounter = 0;

    Get<KeyManager>().Start();

    if (aMode == kNormalAttach)
    {
        mReattachState = kReattachStart;
    }

    if ((aMode == kAnnounceAttach) || (GetRloc16() == Mac::kShortAddrInvalid))
    {
        Attach(kAnyPartition);
    }
#if OPENTHREAD_FTD
    else if (IsActiveRouter(GetRloc16()))
    {
        if (Get<MleRouter>().BecomeRouter(ThreadStatusTlv::kTooFewRouters) != kErrorNone)
        {
            Attach(kAnyPartition);
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

void Mle::Stop(StopMode aMode)
{
    if (aMode == kUpdateNetworkDatasets)
    {
        Get<MeshCoP::ActiveDatasetManager>().HandleDetach();
        Get<MeshCoP::PendingDatasetManager>().HandleDetach();
    }

    VerifyOrExit(!IsDisabled());

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

    LogNote("Role %s -> %s", RoleToString(oldRole), RoleToString(mRole));

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
    return;
}

void Mle::SetAttachState(AttachState aState)
{
    VerifyOrExit(aState != mAttachState);
    LogInfo("AttachState %s -> %s", AttachStateToString(mAttachState), AttachStateToString(aState));
    mAttachState = aState;

exit:
    return;
}

void Mle::Restore(void)
{
    Settings::NetworkInfo networkInfo;
    Settings::ParentInfo  parentInfo;

    IgnoreError(Get<MeshCoP::ActiveDatasetManager>().Restore());
    IgnoreError(Get<MeshCoP::PendingDatasetManager>().Restore());

#if OPENTHREAD_CONFIG_DUA_ENABLE
    Get<DuaManager>().Restore();
#endif

    SuccessOrExit(Get<Settings>().Read(networkInfo));

    Get<KeyManager>().SetCurrentKeySequence(networkInfo.GetKeySequence());
    Get<KeyManager>().SetMleFrameCounter(networkInfo.GetMleFrameCounter());
    Get<KeyManager>().SetAllMacFrameCounters(networkInfo.GetMacFrameCounter());

#if OPENTHREAD_MTD
    mDeviceMode.Set(networkInfo.GetDeviceMode() & ~DeviceMode::kModeFullThreadDevice);
#else
    mDeviceMode.Set(networkInfo.GetDeviceMode());
#endif

    // force re-attach when version mismatch.
    VerifyOrExit(networkInfo.GetVersion() == kThreadVersion);

    switch (networkInfo.GetRole())
    {
    case kRoleChild:
    case kRoleRouter:
    case kRoleLeader:
        break;

    default:
        ExitNow();
    }

#if OPENTHREAD_MTD
    if (!IsActiveRouter(networkInfo.GetRloc16()))
#endif
    {
        Get<Mac::Mac>().SetShortAddress(networkInfo.GetRloc16());
    }
    Get<Mac::Mac>().SetExtAddress(networkInfo.GetExtAddress());

    mMeshLocal64.GetAddress().SetIid(networkInfo.GetMeshLocalIid());

    if (networkInfo.GetRloc16() == Mac::kShortAddrInvalid)
    {
        ExitNow();
    }

    if (!IsActiveRouter(networkInfo.GetRloc16()))
    {
        if (Get<Settings>().Read(parentInfo) != kErrorNone)
        {
            // If the restored RLOC16 corresponds to an end-device, it
            // is expected that the `ParentInfo` settings to be valid
            // as well. The device can still recover from such an invalid
            // setting by skipping the re-attach ("Child Update Request"
            // exchange) and going through the full attach process.

            LogWarn("Invalid settings - no saved parent info with valid end-device RLOC16 0x%04x",
                    networkInfo.GetRloc16());
            ExitNow();
        }

        mParent.Clear();
        mParent.SetExtAddress(parentInfo.GetExtAddress());
        mParent.SetVersion(static_cast<uint8_t>(parentInfo.GetVersion()));
        mParent.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                         DeviceMode::kModeFullNetworkData));
        mParent.SetRloc16(Rloc16FromRouterId(RouterIdFromRloc16(networkInfo.GetRloc16())));
        mParent.SetState(Neighbor::kStateRestored);

        mPreviousParentRloc = mParent.GetRloc16();
    }
#if OPENTHREAD_FTD
    else
    {
        Get<MleRouter>().SetRouterId(RouterIdFromRloc16(GetRloc16()));
        Get<MleRouter>().SetPreviousPartitionId(networkInfo.GetPreviousPartitionId());
        Get<ChildTable>().Restore();
    }
#endif

    // Successfully restored the network information from non-volatile settings after boot.
    mHasRestored = true;

exit:
    return;
}

Error Mle::Store(void)
{
    Error                 error = kErrorNone;
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

            SuccessOrExit(error = Get<Settings>().Save(parentInfo));
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

        SuccessOrExit(Get<Settings>().Read(networkInfo));
    }

    networkInfo.SetKeySequence(Get<KeyManager>().GetCurrentKeySequence());
    networkInfo.SetMleFrameCounter(Get<KeyManager>().GetMleFrameCounter() +
                                   OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD);
    networkInfo.SetMacFrameCounter(Get<KeyManager>().GetMaximumMacFrameCounter() +
                                   OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD);
    networkInfo.SetDeviceMode(mDeviceMode.Get());

    SuccessOrExit(error = Get<Settings>().Save(networkInfo));

    Get<KeyManager>().SetStoredMleFrameCounter(networkInfo.GetMleFrameCounter());
    Get<KeyManager>().SetStoredMacFrameCounter(networkInfo.GetMacFrameCounter());

    LogDebg("Store Network Information");

exit:
    return error;
}

Error Mle::BecomeDetached(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);

    // In case role is already detached and attach state is `kAttachStateStart`
    // (i.e., waiting to start an attach attempt), there is no need to make any
    // changes.

    VerifyOrExit(!IsDetached() || mAttachState != kAttachStateStart);

    // not in reattach stage after reset
    if (mReattachState == kReattachStop)
    {
        Get<MeshCoP::PendingDatasetManager>().HandleDetach();
    }

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    mParentSearchRecentlyDetached = true;
#endif

    SetStateDetached();
    mParent.SetState(Neighbor::kStateInvalid);
    SetRloc16(Mac::kShortAddrInvalid);
    Attach(kAnyPartition);

exit:
    return error;
}

Error Mle::BecomeChild(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsAttaching(), error = kErrorBusy);

    Attach(kAnyPartition);

exit:
    return error;
}

void Mle::Attach(AttachMode aMode)
{
    VerifyOrExit(!IsDisabled() && !IsAttaching());

    if (!IsDetached())
    {
        mAttachCounter = 0;
    }

    if (mReattachState == kReattachStart)
    {
        if (Get<MeshCoP::ActiveDatasetManager>().Restore() == kErrorNone)
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
    mAttachMode = aMode;

    if (aMode != kBetterPartition)
    {
#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            Get<MleRouter>().StopAdvertiseTrickleTimer();
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
    return;
}

uint32_t Mle::GetAttachStartDelay(void) const
{
    uint32_t delay = 1;
    uint32_t jitter;

    VerifyOrExit(IsDetached());

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

    LogNote("Attach attempt %d unsuccessful, will try again in %u.%03u seconds", mAttachCounter, delay / 1000,
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
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (Get<Mac::Mac>().IsCslEnabled())
    {
        IgnoreError(Get<Radio>().EnableCsl(0, GetParent().GetRloc16(), &GetParent().GetExtAddress()));
    }
#endif
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
    mAttachTimer.Start(kAttachBackoffDelayToResetCounter);
    mReattachState       = kReattachStop;
    mChildUpdateAttempts = 0;
    mDataRequestAttempts = 0;
    Get<Mac::Mac>().SetBeaconEnabled(false);
    ScheduleMessageTransmissionTimer();

#if OPENTHREAD_FTD
    if (IsFullThreadDevice())
    {
        Get<MleRouter>().HandleChildStart(mAttachMode);
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

    if ((mPreviousParentRloc != Mac::kShortAddrInvalid) && (mPreviousParentRloc != mParent.GetRloc16()))
    {
        mCounters.mParentChanges++;

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
        InformPreviousParent();
#endif
    }

    mPreviousParentRloc = mParent.GetRloc16();

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (Get<Mac::Mac>().IsCslCapable())
    {
        uint32_t period = IsRxOnWhenIdle() ? 0 : Get<Mac::Mac>().GetCslPeriod();
        IgnoreError(Get<Radio>().EnableCsl(period, GetParent().GetRloc16(), &GetParent().GetExtAddress()));
        ScheduleChildUpdateRequest();
    }
#endif
}

void Mle::InformPreviousChannel(void)
{
    VerifyOrExit(mAlternatePanId != Mac::kPanIdBroadcast);
    VerifyOrExit(IsChild() || IsRouter());

#if OPENTHREAD_FTD
    VerifyOrExit(!IsFullThreadDevice() || IsRouter() || Get<MleRouter>().GetRouterSelectionJitterTimeout() == 0);
#endif

    mAlternatePanId = Mac::kPanIdBroadcast;
    Get<AnnounceBeginServer>().SendAnnounce(1 << mAlternateChannel);

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

    if (IsChild())
    {
        IgnoreError(SendChildUpdateRequest());
    }

exit:
    return;
}

Error Mle::SetDeviceMode(DeviceMode aDeviceMode)
{
    Error      error   = kErrorNone;
    DeviceMode oldMode = mDeviceMode;

#if OPENTHREAD_MTD
    VerifyOrExit(!aDeviceMode.IsFullThreadDevice(), error = kErrorInvalidArgs);
#endif

    VerifyOrExit(aDeviceMode.IsValid(), error = kErrorInvalidArgs);
    VerifyOrExit(mDeviceMode != aDeviceMode);
    mDeviceMode = aDeviceMode;

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    Get<Utils::HistoryTracker>().RecordNetworkInfo();
#endif

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitDeviceMode(mDeviceMode);
#endif

    LogNote("Mode 0x%02x -> 0x%02x [%s]", oldMode.Get(), mDeviceMode.Get(), mDeviceMode.ToString().AsCString());

    IgnoreError(Store());

    switch (mRole)
    {
    case kRoleDisabled:
        break;

    case kRoleDetached:
        mAttachCounter = 0;
        SetStateDetached();
        Attach(kAnyPartition);
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

void Mle::SetMeshLocalPrefix(const Ip6::NetworkPrefix &aMeshLocalPrefix)
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
    VerifyOrExit(Get<ThreadNetif>().IsUp());

    ApplyMeshLocalPrefix();

exit:
    return;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
Error Mle::SetMeshLocalIid(const Ip6::InterfaceIdentifier &aMlIid)
{
    Error error = kErrorNone;

    VerifyOrExit(!Get<ThreadNetif>().HasUnicastAddress(mMeshLocal64), error = kErrorInvalidState);

    mMeshLocal64.GetAddress().SetIid(aMlIid);
exit:
    return error;
}
#endif

void Mle::ApplyMeshLocalPrefix(void)
{
    mLinkLocalAllThreadNodes.GetAddress().SetMulticastNetworkPrefix(GetMeshLocalPrefix());
    mRealmLocalAllThreadNodes.GetAddress().SetMulticastNetworkPrefix(GetMeshLocalPrefix());

    VerifyOrExit(!IsDisabled());

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

#if OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
    Get<NeighborDiscovery::Agent>().ApplyMeshLocalPrefix();
#endif

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    for (ServiceAloc &serviceAloc : mServiceAlocs)
    {
        if (serviceAloc.IsInUse())
        {
            Get<ThreadNetif>().RemoveUnicastAddress(serviceAloc);
        }

        serviceAloc.ApplyMeshLocalPrefix(GetMeshLocalPrefix());

        if (serviceAloc.IsInUse())
        {
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
        LogNote("RLOC16 %04x -> %04x", oldRloc16, aRloc16);
    }

    if (Get<ThreadNetif>().HasUnicastAddress(mMeshLocal16) &&
        (mMeshLocal16.GetAddress().GetIid().GetLocator() != aRloc16))
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocal16);
        Get<Tmf::Agent>().ClearRequests(mMeshLocal16.GetAddress());
    }

    Get<Mac::Mac>().SetShortAddress(aRloc16);
    Get<Ip6::Mpl>().SetSeedId(aRloc16);

    if (aRloc16 != Mac::kShortAddrInvalid)
    {
        // We can always call `AddUnicastAddress(mMeshLocat16)` and if
        // the address is already added, it will perform no action.

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

Error Mle::GetLeaderAddress(Ip6::Address &aAddress) const
{
    Error error = kErrorNone;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = kErrorDetached);

    aAddress.SetToRoutingLocator(GetMeshLocalPrefix(), Rloc16FromRouterId(mLeaderData.GetLeaderRouterId()));

exit:
    return error;
}

Error Mle::GetLocatorAddress(Ip6::Address &aAddress, uint16_t aLocator) const
{
    Error error = kErrorNone;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = kErrorDetached);

    memcpy(&aAddress, &mMeshLocal16.GetAddress(), 14);
    aAddress.GetIid().SetLocator(aLocator);

exit:
    return error;
}

Error Mle::GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const
{
    Error error = kErrorNone;

    VerifyOrExit(GetRloc16() != Mac::kShortAddrInvalid, error = kErrorDetached);
    aAddress.SetToAnycastLocator(GetMeshLocalPrefix(), ServiceAlocFromId(aServiceId));

exit:
    return error;
}

const LeaderData &Mle::GetLeaderData(void)
{
    mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet));
    mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kStableSubset));

    return mLeaderData;
}

Mle::TxMessage *Mle::NewMleMessage(Command aCommand)
{
    Error             error = kErrorNone;
    TxMessage *       message;
    Message::Settings settings(Message::kNoLinkSecurity, Message::kPriorityNet);
    Message::SubType  subType;
    uint8_t           securitySuite;

    message = static_cast<TxMessage *>(mSocket.NewMessage(0, settings));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    securitySuite = k154Security;
    subType       = Message::kSubTypeMleGeneral;

    switch (aCommand)
    {
    case kCommandAnnounce:
        subType = Message::kSubTypeMleAnnounce;
        break;

    case kCommandDiscoveryRequest:
        subType       = Message::kSubTypeMleDiscoverRequest;
        securitySuite = kNoSecurity;
        break;

    case kCommandDiscoveryResponse:
        subType       = Message::kSubTypeMleDiscoverResponse;
        securitySuite = kNoSecurity;
        break;

    case kCommandChildUpdateRequest:
        subType = Message::kSubTypeMleChildUpdateRequest;
        break;

    case kCommandDataResponse:
        subType = Message::kSubTypeMleDataResponse;
        break;

    case kCommandChildIdRequest:
        subType = Message::kSubTypeMleChildIdRequest;
        break;

    case kCommandDataRequest:
        subType = Message::kSubTypeMleDataRequest;
        break;

    default:
        break;
    }

    message->SetSubType(subType);

    SuccessOrExit(error = message->Append(securitySuite));

    if (securitySuite == k154Security)
    {
        SecurityHeader securityHeader;

        // The other fields in security header are updated in the
        // message in `TxMessage::SendTo()` before message is sent.

        securityHeader.InitSecurityControl();
        SuccessOrExit(error = message->Append(securityHeader));
    }

    error = message->Append<uint8_t>(aCommand);

exit:
    FreeAndNullMessageOnError(message, error);
    return message;
}

Error Mle::TxMessage::AppendSourceAddressTlv(void)
{
    return Tlv::Append<SourceAddressTlv>(*this, Get<Mle>().GetRloc16());
}

Error Mle::TxMessage::AppendStatusTlv(StatusTlv::Status aStatus)
{
    return Tlv::Append<StatusTlv>(*this, aStatus);
}

Error Mle::TxMessage::AppendModeTlv(DeviceMode aMode)
{
    return Tlv::Append<ModeTlv>(*this, aMode.Get());
}

Error Mle::TxMessage::AppendTimeoutTlv(uint32_t aTimeout)
{
    return Tlv::Append<TimeoutTlv>(*this, aTimeout);
}

Error Mle::TxMessage::AppendChallengeTlv(const Challenge &aChallenge)
{
    return Tlv::Append<ChallengeTlv>(*this, aChallenge.mBuffer, aChallenge.mLength);
}

Error Mle::TxMessage::AppendChallengeTlv(const uint8_t *aChallenge, uint8_t aChallengeLength)
{
    return Tlv::Append<ChallengeTlv>(*this, aChallenge, aChallengeLength);
}

Error Mle::TxMessage::AppendResponseTlv(const Challenge &aResponse)
{
    return Tlv::Append<ResponseTlv>(*this, aResponse.mBuffer, aResponse.mLength);
}

Error Mle::RxMessage::ReadChallengeOrResponse(uint8_t aTlvType, Challenge &aBuffer) const
{
    Error    error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(*this, aTlvType, offset, length));
    VerifyOrExit(length >= kMinChallengeSize, error = kErrorParse);

    if (length > kMaxChallengeSize)
    {
        length = kMaxChallengeSize;
    }

    ReadBytes(offset, aBuffer.mBuffer, length);
    aBuffer.mLength = static_cast<uint8_t>(length);

exit:
    return error;
}

Error Mle::RxMessage::ReadChallengeTlv(Challenge &aChallenge) const
{
    return ReadChallengeOrResponse(Tlv::kChallenge, aChallenge);
}

Error Mle::RxMessage::ReadResponseTlv(Challenge &aResponse) const
{
    return ReadChallengeOrResponse(Tlv::kResponse, aResponse);
}

Error Mle::TxMessage::AppendLinkFrameCounterTlv(void)
{
    uint32_t counter;

    // When including Link-layer Frame Counter TLV in an MLE message
    // the value is set to the maximum MAC frame counter on all
    // supported radio links. All radio links must also start using
    // the same counter value as the value included in the TLV.

    counter = Get<KeyManager>().GetMaximumMacFrameCounter();

#if OPENTHREAD_CONFIG_MULTI_RADIO
    Get<KeyManager>().SetAllMacFrameCounters(counter);
#endif

    return Tlv::Append<LinkFrameCounterTlv>(*this, counter);
}

Error Mle::TxMessage::AppendMleFrameCounterTlv(void)
{
    return Tlv::Append<MleFrameCounterTlv>(*this, Get<KeyManager>().GetMleFrameCounter());
}

Error Mle::RxMessage::ReadFrameCounterTlvs(uint32_t &aLinkFrameCounter, uint32_t &aMleFrameCounter) const
{
    Error error;

    SuccessOrExit(error = Tlv::Find<LinkFrameCounterTlv>(*this, aLinkFrameCounter));

    switch (Tlv::Find<MleFrameCounterTlv>(*this, aMleFrameCounter))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        aMleFrameCounter = aLinkFrameCounter;
        break;
    default:
        error = kErrorParse;
        break;
    }

exit:
    return error;
}

Error Mle::TxMessage::AppendAddress16Tlv(uint16_t aRloc16)
{
    return Tlv::Append<Address16Tlv>(*this, aRloc16);
}

Error Mle::TxMessage::AppendLeaderDataTlv(void)
{
    LeaderDataTlv leaderDataTlv;

    Get<Mle>().mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet));
    Get<Mle>().mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kStableSubset));

    leaderDataTlv.Init();
    leaderDataTlv.Set(Get<Mle>().mLeaderData);

    return leaderDataTlv.AppendTo(*this);
}

Error Mle::RxMessage::ReadLeaderDataTlv(LeaderData &aLeaderData) const
{
    Error         error;
    LeaderDataTlv leaderDataTlv;

    SuccessOrExit(error = Tlv::FindTlv(*this, leaderDataTlv));
    VerifyOrExit(leaderDataTlv.IsValid(), error = kErrorParse);
    leaderDataTlv.Get(aLeaderData);

exit:
    return error;
}

Error Mle::TxMessage::AppendNetworkDataTlv(NetworkData::Type aType)
{
    Error   error = kErrorNone;
    uint8_t networkData[NetworkData::NetworkData::kMaxSize];
    uint8_t length;

    VerifyOrExit(!Get<Mle>().mRetrieveNewNetworkData, error = kErrorInvalidState);

    length = sizeof(networkData);
    IgnoreError(Get<NetworkData::Leader>().CopyNetworkData(aType, networkData, length));

    error = Tlv::Append<NetworkDataTlv>(*this, networkData, length);

exit:
    return error;
}

Error Mle::TxMessage::AppendTlvRequestTlv(const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    return Tlv::Append<TlvRequestTlv>(*this, aTlvs, aTlvsLength);
}

Error Mle::RxMessage::ReadTlvRequestTlv(RequestedTlvs &aRequestedTlvs) const
{
    Error    error;
    uint16_t offset;
    uint16_t length;

    SuccessOrExit(error = Tlv::FindTlvValueOffset(*this, Tlv::kTlvRequest, offset, length));

    if (length > sizeof(aRequestedTlvs.mTlvs))
    {
        length = sizeof(aRequestedTlvs.mTlvs);
    }

    ReadBytes(offset, aRequestedTlvs.mTlvs, length);
    aRequestedTlvs.mNumTlvs = static_cast<uint8_t>(length);

exit:
    return error;
}

Error Mle::TxMessage::AppendScanMaskTlv(uint8_t aScanMask)
{
    return Tlv::Append<ScanMaskTlv>(*this, aScanMask);
}

Error Mle::TxMessage::AppendLinkMarginTlv(uint8_t aLinkMargin)
{
    return Tlv::Append<LinkMarginTlv>(*this, aLinkMargin);
}

Error Mle::TxMessage::AppendVersionTlv(void)
{
    return Tlv::Append<VersionTlv>(*this, kThreadVersion);
}

bool Mle::HasUnregisteredAddress(void)
{
    bool retval = false;

    // Checks whether there are any addresses in addition to the mesh-local
    // address that need to be registered.

    for (const Ip6::Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (!addr.GetAddress().IsLinkLocal() && !IsRoutingLocator(addr.GetAddress()) &&
            !IsAnycastLocator(addr.GetAddress()) && addr.GetAddress() != GetMeshLocal64())
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

Error Mle::TxMessage::AppendAddressRegistrationTlv(AddressRegistrationMode aMode)
{
    Error                    error = kErrorNone;
    Tlv                      tlv;
    AddressRegistrationEntry entry;
    Lowpan::Context          context;
    uint8_t                  length      = 0;
    uint8_t                  counter     = 0;
    uint16_t                 startOffset = GetLength();
#if OPENTHREAD_CONFIG_DUA_ENABLE
    Ip6::Address domainUnicastAddress;
#endif

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = Append(tlv));

    // Prioritize ML-EID
    entry.SetContextId(kMeshLocalPrefixContextId);
    entry.SetIid(Get<Mle>().GetMeshLocal64().GetIid());
    SuccessOrExit(error = AppendBytes(&entry, entry.GetLength()));
    length += entry.GetLength();

    // Continue to append the other addresses if not `kAppendMeshLocalOnly` mode
    VerifyOrExit(aMode != kAppendMeshLocalOnly);
    counter++;

#if OPENTHREAD_CONFIG_DUA_ENABLE
    // Cache Domain Unicast Address.
    domainUnicastAddress = Get<DuaManager>().GetDomainUnicastAddress();

    if (Get<ThreadNetif>().HasUnicastAddress(domainUnicastAddress))
    {
        SuccessOrAssert(Get<NetworkData::Leader>().GetContext(domainUnicastAddress, context));

        // Prioritize DUA, compressed entry
        entry.SetContextId(context.mContextId);
        entry.SetIid(domainUnicastAddress.GetIid());
        SuccessOrExit(error = AppendBytes(&entry, entry.GetLength()));
        length += entry.GetLength();
        counter++;
    }
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

    for (const Ip6::Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (addr.GetAddress().IsLinkLocal() || Get<Mle>().IsRoutingLocator(addr.GetAddress()) ||
            Get<Mle>().IsAnycastLocator(addr.GetAddress()) || addr.GetAddress() == Get<Mle>().GetMeshLocal64())
        {
            continue;
        }

#if OPENTHREAD_CONFIG_DUA_ENABLE
        // Skip DUA that was already appended above.
        if (addr.GetAddress() == domainUnicastAddress)
        {
            continue;
        }
#endif

        if (Get<NetworkData::Leader>().GetContext(addr.GetAddress(), context) == kErrorNone)
        {
            // compressed entry
            entry.SetContextId(context.mContextId);
            entry.SetIid(addr.GetAddress().GetIid());
        }
        else
        {
            // uncompressed entry
            entry.SetUncompressed();
            entry.SetIp6Address(addr.GetAddress());
        }

        SuccessOrExit(error = AppendBytes(&entry, entry.GetLength()));
        length += entry.GetLength();
        counter++;
        // only continue to append if there is available entry.
        VerifyOrExit(counter < OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER);
    }

    // Append external multicast addresses.  For sleepy end device,
    // register all external multicast addresses with the parent for
    // indirect transmission. Since Thread 1.2, non-sleepy MED should
    // also register external multicast addresses of scope larger than
    // realm with a 1.2 or higher parent.
    if (!Get<Mle>().IsRxOnWhenIdle()
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        || !Get<Mle>().GetParent().IsThreadVersion1p1()
#endif
    )
    {
        for (const Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
        {
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
            // For Thread 1.2 MED, skip multicast address with scope not
            // larger than realm local when registering.
            if (Get<Mle>().IsRxOnWhenIdle() && !addr.GetAddress().IsMulticastLargerThanRealmLocal())
            {
                continue;
            }
#endif

            entry.SetUncompressed();
            entry.SetIp6Address(addr.GetAddress());
            SuccessOrExit(error = AppendBytes(&entry, entry.GetLength()));
            length += entry.GetLength();

            counter++;
            // only continue to append if there is available entry.
            VerifyOrExit(counter < OPENTHREAD_CONFIG_MLE_IP_ADDRS_TO_REGISTER);
        }
    }

exit:

    if (error == kErrorNone && length > 0)
    {
        tlv.SetLength(length);
        Write(startOffset, tlv);
    }

    return error;
}

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
Error Mle::TxMessage::AppendTimeRequestTlv(void)
{
    // `TimeRequestTlv` has no value.
    return Tlv::Append<TimeRequestTlv>(*this, nullptr, 0);
}

Error Mle::TxMessage::AppendTimeParameterTlv(void)
{
    TimeParameterTlv tlv;

    tlv.Init();
    tlv.SetTimeSyncPeriod(Get<TimeSync>().GetTimeSyncPeriod());
    tlv.SetXtalThreshold(Get<TimeSync>().GetXtalThreshold());

    return tlv.AppendTo(*this);
}

Error Mle::TxMessage::AppendXtalAccuracyTlv(void)
{
    return Tlv::Append<XtalAccuracyTlv>(*this, otPlatTimeGetXtalAccuracy());
}
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

Error Mle::TxMessage::AppendActiveTimestampTlv(void)
{
    Error                     error     = kErrorNone;
    const MeshCoP::Timestamp *timestamp = Get<MeshCoP::ActiveDatasetManager>().GetTimestamp();

    VerifyOrExit(timestamp != nullptr);
    error = Tlv::Append<ActiveTimestampTlv>(*this, *timestamp);

exit:
    return error;
}

Error Mle::TxMessage::AppendPendingTimestampTlv(void)
{
    Error                     error     = kErrorNone;
    const MeshCoP::Timestamp *timestamp = Get<MeshCoP::PendingDatasetManager>().GetTimestamp();

    VerifyOrExit(timestamp != nullptr && timestamp->GetSeconds() != 0);
    error = Tlv::Append<PendingTimestampTlv>(*this, *timestamp);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
Error Mle::TxMessage::AppendCslChannelTlv(void)
{
    Error         error = kErrorNone;
    CslChannelTlv cslChannel;

    // In current implementation, it's allowed to set CSL Channel unspecified. As `0` is not valid for Channel value
    // in CSL Channel TLV, if CSL channel is not specified, we don't append CSL Channel TLV.
    // And on transmitter side, it would also set CSL Channel for the child to `0` if it doesn't find a CSL Channel
    // TLV.
    VerifyOrExit(Get<Mac::Mac>().IsCslChannelSpecified());

    cslChannel.Init();
    cslChannel.SetChannelPage(0);
    cslChannel.SetChannel(Get<Mac::Mac>().GetCslChannel());

    SuccessOrExit(error = Append(cslChannel));

exit:
    return error;
}

Error Mle::TxMessage::AppendCslTimeoutTlv(void)
{
    OT_ASSERT(Get<Mac::Mac>().IsCslEnabled());
    return Tlv::Append<CslTimeoutTlv>(*this,
                                      Get<Mle>().mCslTimeout == 0 ? Get<Mle>().mTimeout : Get<Mle>().mCslTimeout);
}

void Mle::SetCslTimeout(uint32_t aTimeout)
{
    VerifyOrExit(mCslTimeout != aTimeout);

    mCslTimeout = aTimeout;

    Get<DataPollSender>().RecalculatePollPeriod();

    if (Get<Mac::Mac>().IsCslEnabled())
    {
        ScheduleChildUpdateRequest();
    }

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
Error Mle::TxMessage::AppendCslClockAccuracyTlv(void)
{
    Error               error = kErrorNone;
    CslClockAccuracyTlv cslClockAccuracy;

    cslClockAccuracy.Init();

    cslClockAccuracy.SetCslClockAccuracy(Get<Radio>().GetCslAccuracy());
    cslClockAccuracy.SetCslUncertainty(Get<Radio>().GetCslUncertainty());

    SuccessOrExit(error = Append(cslClockAccuracy));

exit:
    return error;
}
#endif

void Mle::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(!IsDisabled());

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
#endif

#if OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
        Get<NeighborDiscovery::Agent>().UpdateService();
#endif

#if OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE
        Get<Dhcp6::Client>().UpdateAddresses();
#endif
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

exit:
    return;
}

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

Mle::ServiceAloc::ServiceAloc(void)
{
    InitAsThreadOriginRealmLocalScope();
    GetAddress().GetIid().SetToLocator(kNotInUse);
}

Mle::ServiceAloc *Mle::FindInServiceAlocs(uint16_t aAloc16)
{
    // Search in `mServiceAlocs` for an entry matching `aAloc16`.
    // Can be used with `aAloc16 = ServerAloc::kNotInUse` to find
    // an unused entry in the array.

    ServiceAloc *match = nullptr;

    for (ServiceAloc &serviceAloc : mServiceAlocs)
    {
        if (serviceAloc.GetAloc16() == aAloc16)
        {
            match = &serviceAloc;
            break;
        }
    }

    return match;
}

void Mle::UpdateServiceAlocs(void)
{
    NetworkData::Iterator      iterator;
    NetworkData::ServiceConfig service;

    VerifyOrExit(!IsDisabled());

    // First remove all ALOCs which are no longer in the Network
    // Data to free up space in `mServiceAlocs` array.

    for (ServiceAloc &serviceAloc : mServiceAlocs)
    {
        bool found = false;

        if (!serviceAloc.IsInUse())
        {
            continue;
        }

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextService(iterator, GetRloc16(), service) == kErrorNone)
        {
            if (service.mServiceId == ServiceIdFromAloc(serviceAloc.GetAloc16()))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Get<ThreadNetif>().RemoveUnicastAddress(serviceAloc);
            serviceAloc.MarkAsNotInUse();
        }
    }

    // Now add any new ALOCs if there is space in `mServiceAlocs`.

    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextService(iterator, GetRloc16(), service) == kErrorNone)
    {
        uint16_t aloc16 = ServiceAlocFromId(service.mServiceId);

        if (FindInServiceAlocs(aloc16) == nullptr)
        {
            // No matching ALOC in `mServiceAlocs`, so we try to add it.
            ServiceAloc *newServiceAloc = FindInServiceAlocs(ServiceAloc::kNotInUse);

            VerifyOrExit(newServiceAloc != nullptr);
            newServiceAloc->SetAloc16(aloc16);
            Get<ThreadNetif>().AddUnicastAddress(*newServiceAloc);
        }
    }

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

void Mle::HandleAttachTimer(Timer &aTimer)
{
    aTimer.Get<Mle>().HandleAttachTimer();
}

bool Mle::HasAcceptableParentCandidate(void) const
{
    bool        hasAcceptableParent = false;
    LinkQuality linkQuality;

    VerifyOrExit(mParentCandidate.IsStateParentResponse());

    switch (mAttachState)
    {
    case kAttachStateAnnounce:
        VerifyOrExit(!HasMoreChannelsToAnnouce());
        break;

    case kAttachStateParentRequestRouter:
        // If we cannot find a parent with best link quality (3) when
        // in `kAttachStateParentRequestRouter` state we will keep the
        // candidate and forward to REED stage to potentially find a
        // better parent.
        linkQuality = OT_MIN(mParentCandidate.GetLinkInfo().GetLinkQuality(), mParentCandidate.GetLinkQualityOut());
        VerifyOrExit(linkQuality == kLinkQuality3);
        break;

    case kAttachStateParentRequestReed:
        break;

    default:
        ExitNow();
    }

    if (IsChild())
    {
        // If already attached, accept the parent candidate if
        // we are trying to attach to a better partition or if a
        // Parent Response was also received from the current parent
        // to which the device is attached. This ensures that the
        // new parent candidate is compared with the current parent
        // and that it is indeed preferred over the current one.

        VerifyOrExit(mReceivedResponseFromParent || (mAttachMode == kBetterPartition));
    }

    hasAcceptableParent = true;

exit:
    return hasAcceptableParent;
}

void Mle::HandleAttachTimer(void)
{
    uint32_t delay          = 0;
    bool     shouldAnnounce = true;

    // First, check if we are waiting to receive parent responses and
    // found an acceptable parent candidate.

    if (HasAcceptableParentCandidate() && (SendChildIdRequest() == kErrorNone))
    {
        SetAttachState(kAttachStateChildIdRequest);
        delay = kParentRequestReedTimeout;
        ExitNow();
    }

    switch (mAttachState)
    {
    case kAttachStateIdle:
        mAttachCounter = 0;
        break;

    case kAttachStateProcessAnnounce:
        ProcessAnnounce();
        break;

    case kAttachStateStart:
        if (mAttachCounter > 0)
        {
            LogNote("Attempt to attach - attempt %d, %s %s", mAttachCounter, AttachModeToString(mAttachMode),
                    ReattachStateToString(mReattachState));
        }
        else
        {
            LogNote("Attempt to attach - %s %s", AttachModeToString(mAttachMode),
                    ReattachStateToString(mReattachState));
        }

        SetAttachState(kAttachStateParentRequestRouter);
        mParentCandidate.SetState(Neighbor::kStateInvalid);
        mReceivedResponseFromParent = false;
        Get<MeshForwarder>().SetRxOnWhenIdle(true);

        // initial MLE Parent Request has both E and R flags set in Scan Mask TLV
        // during reattach when losing connectivity.
        if (mAttachMode == kSamePartition || mAttachMode == kSamePartitionRetry)
        {
            SendParentRequest(kToRoutersAndReeds);
            delay = kParentRequestReedTimeout;
        }
        // initial MLE Parent Request has only R flag set in Scan Mask TLV for
        // during initial attach or downgrade process
        else
        {
            SendParentRequest(kToRouters);
            delay = kParentRequestRouterTimeout;
        }

        break;

    case kAttachStateParentRequestRouter:
        SetAttachState(kAttachStateParentRequestReed);
        SendParentRequest(kToRoutersAndReeds);
        delay = kParentRequestReedTimeout;
        break;

    case kAttachStateParentRequestReed:
        shouldAnnounce = PrepareAnnounceState();

        if (shouldAnnounce)
        {
            // We send an extra "Parent Request" as we switch to
            // `kAttachStateAnnounce` and start sending Announce on
            // all channels. This gives an additional chance to find
            // a parent during this phase. Note that we can stay in
            // `kAttachStateAnnounce` for multiple iterations, each
            // time sending an Announce on a different channel
            // (with `mAnnounceDelay` wait between them).

            SetAttachState(kAttachStateAnnounce);
            SendParentRequest(kToRoutersAndReeds);
            mAnnounceChannel = Mac::ChannelMask::kChannelIteratorFirst;
            delay            = mAnnounceDelay;
            break;
        }

        OT_FALL_THROUGH;

    case kAttachStateAnnounce:
        if (shouldAnnounce && (GetNextAnnouceChannel(mAnnounceChannel) == kErrorNone))
        {
            SendAnnounce(mAnnounceChannel, kOrphanAnnounce);
            delay = mAnnounceDelay;
            break;
        }

        OT_FALL_THROUGH;

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
                 (Get<MeshCoP::ActiveDatasetManager>().IsPartiallyComplete() || !IsFullThreadDevice()));

    if (Get<MeshCoP::ActiveDatasetManager>().GetChannelMask(channelMask) != kErrorNone)
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
        if (Get<MeshCoP::PendingDatasetManager>().Restore() == kErrorNone)
        {
            IgnoreError(Get<MeshCoP::PendingDatasetManager>().ApplyConfiguration());
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
        IgnoreError(Get<MeshCoP::ActiveDatasetManager>().Restore());
    }

    VerifyOrExit(mReattachState == kReattachStop);

    switch (mAttachMode)
    {
    case kAnyPartition:
    case kBetterParent:
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
            else if (IsFullThreadDevice() && Get<MleRouter>().BecomeLeader() == kErrorNone)
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

    case kSamePartition:
        Attach(kSamePartitionRetry);
        break;

    case kSamePartitionRetry:
    case kDowngradeToReed:
        Attach(kAnyPartition);
        break;

    case kBetterPartition:
        break;
    }

exit:
    return delay;
}

void Mle::HandleDelayedResponseTimer(Timer &aTimer)
{
    aTimer.Get<Mle>().HandleDelayedResponseTimer();
}

void Mle::HandleDelayedResponseTimer(void)
{
    TimeMilli now          = TimerMilli::GetNow();
    TimeMilli nextSendTime = now.GetDistantFuture();

    for (Message &message : mDelayedResponses)
    {
        DelayedResponseMetadata metadata;

        metadata.ReadFrom(message);

        if (now < metadata.mSendTime)
        {
            if (nextSendTime > metadata.mSendTime)
            {
                nextSendTime = metadata.mSendTime;
            }
        }
        else
        {
            mDelayedResponses.Dequeue(message);
            SendDelayedResponse(static_cast<TxMessage &>(message), metadata);
        }
    }

    if (nextSendTime < now.GetDistantFuture())
    {
        mDelayedResponseTimer.FireAt(nextSendTime);
    }
}

void Mle::SendDelayedResponse(TxMessage &aMessage, const DelayedResponseMetadata &aMetadata)
{
    Error error = kErrorNone;

    aMetadata.RemoveFrom(aMessage);

    if (aMessage.GetSubType() == Message::kSubTypeMleDataRequest)
    {
        SuccessOrExit(error = aMessage.AppendActiveTimestampTlv());
        SuccessOrExit(error = aMessage.AppendPendingTimestampTlv());
    }

    SuccessOrExit(error = aMessage.SendTo(aMetadata.mDestination));

    Log(kMessageSend, kTypeGenericDelayed, aMetadata.mDestination);

    if (!IsRxOnWhenIdle())
    {
        // Start fast poll mode, assuming enqueued msg is MLE Data Request.
        // Note: Finer-grade check may be required when deciding whether or
        // not to enter fast poll mode for other type of delayed message.

        Get<DataPollSender>().SendFastPolls(DataPollSender::kDefaultFastPolls);
    }

exit:
    FreeMessageOnError(&aMessage, error);
}

void Mle::RemoveDelayedDataResponseMessage(void)
{
    RemoveDelayedMessage(Message::kSubTypeMleDataResponse, kTypeDataResponse, nullptr);
}

void Mle::RemoveDelayedDataRequestMessage(const Ip6::Address &aDestination)
{
    RemoveDelayedMessage(Message::kSubTypeMleDataRequest, kTypeDataRequest, &aDestination);
}

void Mle::RemoveDelayedMessage(Message::SubType aSubType, MessageType aMessageType, const Ip6::Address *aDestination)
{
    for (Message &message : mDelayedResponses)
    {
        DelayedResponseMetadata metadata;

        metadata.ReadFrom(message);

        if ((message.GetSubType() == aSubType) &&
            ((aDestination == nullptr) || (metadata.mDestination == *aDestination)))
        {
            mDelayedResponses.DequeueAndFree(message);
            Log(kMessageRemoveDelayed, aMessageType, metadata.mDestination);
        }
    }
}

void Mle::SendParentRequest(ParentRequestType aType)
{
    Error        error = kErrorNone;
    TxMessage *  message;
    uint8_t      scanMask = 0;
    Ip6::Address destination;

    mParentRequestChallenge.GenerateRandom();

    switch (aType)
    {
    case kToRouters:
        scanMask = ScanMaskTlv::kRouterFlag;
        break;

    case kToRoutersAndReeds:
        scanMask = ScanMaskTlv::kRouterFlag | ScanMaskTlv::kEndDeviceFlag;
        break;
    }

    VerifyOrExit((message = NewMleMessage(kCommandParentRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendModeTlv(mDeviceMode));
    SuccessOrExit(error = message->AppendChallengeTlv(mParentRequestChallenge));
    SuccessOrExit(error = message->AppendScanMaskTlv(scanMask));
    SuccessOrExit(error = message->AppendVersionTlv());
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SuccessOrExit(error = message->AppendTimeRequestTlv());
#endif

    destination.SetToLinkLocalAllRoutersMulticast();
    SuccessOrExit(error = message->SendTo(destination));

    switch (aType)
    {
    case kToRouters:
        Log(kMessageSend, kTypeParentRequestToRouters, destination);
        break;

    case kToRoutersAndReeds:
        Log(kMessageSend, kTypeParentRequestToRoutersReeds, destination);
        break;
    }

exit:
    FreeMessageOnError(message, error);
}

void Mle::RequestShorterChildIdRequest(void)
{
    if (mAttachState == kAttachStateChildIdRequest)
    {
        mAddressRegistrationMode = kAppendMeshLocalOnly;
        IgnoreError(SendChildIdRequest());
    }
}

Error Mle::SendChildIdRequest(void)
{
    Error        error   = kErrorNone;
    uint8_t      tlvs[]  = {Tlv::kAddress16, Tlv::kNetworkData, Tlv::kRoute};
    uint8_t      tlvsLen = sizeof(tlvs);
    TxMessage *  message = nullptr;
    Ip6::Address destination;

    if (mParent.GetExtAddress() == mParentCandidate.GetExtAddress())
    {
        if (IsChild())
        {
            LogInfo("Already attached to candidate parent");
            ExitNow(error = kErrorAlready);
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

    VerifyOrExit((message = NewMleMessage(kCommandChildIdRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendResponseTlv(mParentCandidateChallenge));
    SuccessOrExit(error = message->AppendLinkFrameCounterTlv());
    SuccessOrExit(error = message->AppendMleFrameCounterTlv());
    SuccessOrExit(error = message->AppendModeTlv(mDeviceMode));
    SuccessOrExit(error = message->AppendTimeoutTlv(mTimeout));
    SuccessOrExit(error = message->AppendVersionTlv());

    if (!IsFullThreadDevice())
    {
        SuccessOrExit(error = message->AppendAddressRegistrationTlv(mAddressRegistrationMode));

        // no need to request the last Route64 TLV for MTD
        tlvsLen -= 1;
    }

    SuccessOrExit(error = message->AppendTlvRequestTlv(tlvs, tlvsLen));
    SuccessOrExit(error = message->AppendActiveTimestampTlv());
    SuccessOrExit(error = message->AppendPendingTimestampTlv());

    mParentCandidate.SetState(Neighbor::kStateValid);

    destination.SetToLinkLocalAddress(mParentCandidate.GetExtAddress());
    SuccessOrExit(error = message->SendTo(destination));

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

Error Mle::SendDataRequest(const Ip6::Address &aDestination,
                           const uint8_t *     aTlvs,
                           uint8_t             aTlvsLength,
                           uint16_t            aDelay,
                           const uint8_t *     aExtraTlvs,
                           uint8_t             aExtraTlvsLength)
{
    Error      error = kErrorNone;
    TxMessage *message;

    RemoveDelayedDataRequestMessage(aDestination);

    VerifyOrExit((message = NewMleMessage(kCommandDataRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendTlvRequestTlv(aTlvs, aTlvsLength));

    if (aExtraTlvs != nullptr && aExtraTlvsLength > 0)
    {
        SuccessOrExit(error = message->AppendBytes(aExtraTlvs, aExtraTlvsLength));
    }

    if (aDelay)
    {
        SuccessOrExit(error = message->SendAfterDelay(aDestination, aDelay));
        Log(kMessageDelay, kTypeDataRequest, aDestination);
    }
    else
    {
        SuccessOrExit(error = message->AppendActiveTimestampTlv());
        SuccessOrExit(error = message->AppendPendingTimestampTlv());

        SuccessOrExit(error = message->SendTo(aDestination));
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
        // This condition IsCslCapable() && !IsRxOnWhenIdle() is used instead of
        // IsCslEnabled because during transitions SSED -> MED and MED -> SSED
        // there is a delay in synchronisation of IsRxOnWhenIdle residing in MAC
        // and in MLE, which causes below datapoll interval to be calculated incorrectly.
        if (Get<Mac::Mac>().IsCslCapable() && !IsRxOnWhenIdle())
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
    aTimer.Get<Mle>().HandleMessageTransmissionTimer();
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

            if (SendDataRequest(destination, tlvs, sizeof(tlvs), 0) == kErrorNone)
            {
                mDataRequestAttempts++;
            }

            ExitNow();
        }

        // Keep-alive "Child Update Request" only on a non-sleepy child
        VerifyOrExit(IsChild() && IsRxOnWhenIdle());
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

    if (SendChildUpdateRequest() == kErrorNone)
    {
        mChildUpdateAttempts++;
    }

exit:
    return;
}

Error Mle::SendChildUpdateRequest(void)
{
    Error                   error = kErrorNone;
    Ip6::Address            destination;
    TxMessage *             message = nullptr;
    AddressRegistrationMode mode    = kAppendAllAddresses;

    if (!mParent.IsStateValidOrRestoring())
    {
        LogWarn("No valid parent when sending Child Update Request");
        IgnoreError(BecomeDetached());
        ExitNow();
    }

    mChildUpdateRequestState = kChildUpdateRequestActive;
    ScheduleMessageTransmissionTimer();

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendModeTlv(mDeviceMode));

    switch (mRole)
    {
    case kRoleDetached:
        mParentRequestChallenge.GenerateRandom();
        SuccessOrExit(error = message->AppendChallengeTlv(mParentRequestChallenge));
        mode = kAppendMeshLocalOnly;
        break;

    case kRoleChild:
        SuccessOrExit(error = message->AppendSourceAddressTlv());
        SuccessOrExit(error = message->AppendLeaderDataTlv());
        SuccessOrExit(error = message->AppendTimeoutTlv(mTimeout));
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        if (Get<Mac::Mac>().IsCslEnabled())
        {
            SuccessOrExit(error = message->AppendCslChannelTlv());
            SuccessOrExit(error = message->AppendCslTimeoutTlv());
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
        SuccessOrExit(error = message->AppendAddressRegistrationTlv(mode));
    }

    destination.SetToLinkLocalAddress(mParent.GetExtAddress());
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeChildUpdateRequestOfParent, destination);

    if (!IsRxOnWhenIdle())
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        Get<DataPollSender>().SetAttachMode(!Get<Mac::Mac>().IsCslEnabled());
#else
        Get<DataPollSender>().SetAttachMode(true);
#endif
    }
    else
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(true);
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Mle::SendChildUpdateResponse(const uint8_t *aTlvs, uint8_t aNumTlvs, const Challenge &aChallenge)
{
    Error        error = kErrorNone;
    Ip6::Address destination;
    TxMessage *  message;
    bool         checkAddress = false;

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateResponse)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());

    for (int i = 0; i < aNumTlvs; i++)
    {
        switch (aTlvs[i])
        {
        case Tlv::kTimeout:
            SuccessOrExit(error = message->AppendTimeoutTlv(mTimeout));
            break;

        case Tlv::kStatus:
            SuccessOrExit(error = message->AppendStatusTlv(StatusTlv::kError));
            break;

        case Tlv::kAddressRegistration:
            if (!IsFullThreadDevice())
            {
                // We only register the mesh-local address in the "Child
                // Update Response" message and if there are additional
                // addresses to register we follow up with a "Child Update
                // Request".

                SuccessOrExit(error = message->AppendAddressRegistrationTlv(kAppendMeshLocalOnly));
                checkAddress = true;
            }

            break;

        case Tlv::kResponse:
            SuccessOrExit(error = message->AppendResponseTlv(aChallenge));
            break;

        case Tlv::kLinkFrameCounter:
            SuccessOrExit(error = message->AppendLinkFrameCounterTlv());
            break;

        case Tlv::kMleFrameCounter:
            SuccessOrExit(error = message->AppendMleFrameCounterTlv());
            break;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        case Tlv::kCslTimeout:
            if (Get<Mac::Mac>().IsCslEnabled())
            {
                SuccessOrExit(error = message->AppendCslTimeoutTlv());
            }
            break;
#endif
        }
    }

    destination.SetToLinkLocalAddress(mParent.GetExtAddress());
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeChildUpdateResponseOfParent, destination);

    if (checkAddress && HasUnregisteredAddress())
    {
        IgnoreError(SendChildUpdateRequest());
    }

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Mle::SendAnnounce(uint8_t aChannel, AnnounceMode aMode)
{
    Ip6::Address destination;

    destination.SetToLinkLocalAllNodesMulticast();

    SendAnnounce(aChannel, destination, aMode);
}

void Mle::SendAnnounce(uint8_t aChannel, const Ip6::Address &aDestination, AnnounceMode aMode)
{
    Error              error = kErrorNone;
    ChannelTlv         channel;
    MeshCoP::Timestamp activeTimestamp;
    TxMessage *        message = nullptr;

    VerifyOrExit(Get<Mac::Mac>().GetSupportedChannelMask().ContainsChannel(aChannel), error = kErrorInvalidArgs);
    VerifyOrExit((message = NewMleMessage(kCommandAnnounce)) != nullptr, error = kErrorNoBufs);
    message->SetLinkSecurityEnabled(true);
    message->SetChannel(aChannel);

    channel.Init();
    channel.SetChannelPage(0);
    channel.SetChannel(Get<Mac::Mac>().GetPanChannel());
    SuccessOrExit(error = channel.AppendTo(*message));

    switch (aMode)
    {
    case kOrphanAnnounce:
        activeTimestamp.Clear();
        activeTimestamp.SetAuthoritative(true);
        SuccessOrExit(error = Tlv::Append<ActiveTimestampTlv>(*message, activeTimestamp));
        break;

    case kNormalAnnounce:
        SuccessOrExit(error = message->AppendActiveTimestampTlv());
        break;
    }

    SuccessOrExit(error = Tlv::Append<PanIdTlv>(*message, Get<Mac::Mac>().GetPanId()));

    SuccessOrExit(error = message->SendTo(aDestination));

    LogInfo("Send Announce on channel %d", aChannel);

exit:
    FreeMessageOnError(message, error);
}

Error Mle::GetNextAnnouceChannel(uint8_t &aChannel) const
{
    // This method gets the next channel to send announce on after
    // `aChannel`. Returns `kErrorNotFound` if no more channel in the
    // channel mask after `aChannel`.

    Mac::ChannelMask channelMask;

    if (Get<MeshCoP::ActiveDatasetManager>().GetChannelMask(channelMask) != kErrorNone)
    {
        channelMask = Get<Mac::Mac>().GetSupportedChannelMask();
    }

    return channelMask.GetNextChannel(aChannel);
}

bool Mle::HasMoreChannelsToAnnouce(void) const
{
    uint8_t channel = mAnnounceChannel;

    return GetNextAnnouceChannel(channel) == kErrorNone;
}

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
Error Mle::SendLinkMetricsManagementResponse(const Ip6::Address &aDestination, LinkMetrics::Status aStatus)
{
    Error      error = kErrorNone;
    TxMessage *message;
    Tlv        tlv;
    ot::Tlv    statusSubTlv;

    VerifyOrExit((message = NewMleMessage(kCommandLinkMetricsManagementResponse)) != nullptr, error = kErrorNoBufs);

    tlv.SetType(Tlv::kLinkMetricsManagement);
    statusSubTlv.SetType(LinkMetrics::SubTlv::kStatus);
    statusSubTlv.SetLength(sizeof(aStatus));
    tlv.SetLength(statusSubTlv.GetSize());

    SuccessOrExit(error = message->Append(tlv));
    SuccessOrExit(error = message->Append(statusSubTlv));
    SuccessOrExit(error = message->Append(aStatus));

    SuccessOrExit(error = message->SendTo(aDestination));

exit:
    FreeMessageOnError(message, error);
    return error;
}
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
Error Mle::SendLinkProbe(const Ip6::Address &aDestination, uint8_t aSeriesId, uint8_t *aBuf, uint8_t aLength)
{
    Error      error = kErrorNone;
    TxMessage *message;
    Tlv        tlv;

    VerifyOrExit((message = NewMleMessage(kCommandLinkProbe)) != nullptr, error = kErrorNoBufs);

    tlv.SetType(Tlv::kLinkProbe);
    tlv.SetLength(sizeof(aSeriesId) + aLength);

    SuccessOrExit(error = message->Append(tlv));
    SuccessOrExit(error = message->Append(aSeriesId));
    SuccessOrExit(error = message->AppendBytes(aBuf, aLength));

    SuccessOrExit(error = message->SendTo(aDestination));

exit:
    FreeMessageOnError(message, error);
    return error;
}
#endif

Error Mle::ProcessMessageSecurity(Crypto::AesCcm::Mode    aMode,
                                  Message &               aMessage,
                                  const Ip6::MessageInfo &aMessageInfo,
                                  uint16_t                aCmdOffset,
                                  const SecurityHeader &  aHeader)
{
    // This method performs MLE message security. Based on `aMode` it
    // can be used to encrypt and append tag to `aMessage` or to
    // decrypt and validate the tag in a received `aMessage` (which is
    // then removed from `aMessage`).
    //
    // `aCmdOffset` in both cases specifies the offset in `aMessage`
    // to the start of MLE payload (i.e., the command field).
    //
    // When decrypting, possible errors are:
    // `kErrorNone` decrypted and verified tag, tag is also removed.
    // `kErrorParse` message does not contain the tag
    // `kErrorSecurity` message tag is invalid.
    //
    // When encrypting, possible errors are:
    // `kErrorNone` message encrypted and tag appended to message.
    // `kErrorNoBufs` could not grow the message to append the tag.

    Error               error = kErrorNone;
    Crypto::AesCcm      aesCcm;
    uint8_t             nonce[Crypto::AesCcm::kNonceSize];
    uint8_t             tag[kMleSecurityTagSize];
    Mac::ExtAddress     extAddress;
    uint32_t            keySequence;
    uint16_t            payloadLength   = aMessage.GetLength() - aCmdOffset;
    const Ip6::Address *senderAddress   = &aMessageInfo.GetSockAddr();
    const Ip6::Address *receiverAddress = &aMessageInfo.GetPeerAddr();

    switch (aMode)
    {
    case Crypto::AesCcm::kEncrypt:
        // Use the initialized values for `senderAddress`,
        // `receiverAddress` and `payloadLength`
        break;

    case Crypto::AesCcm::kDecrypt:
        senderAddress   = &aMessageInfo.GetPeerAddr();
        receiverAddress = &aMessageInfo.GetSockAddr();
        // Ensure message contains command field (uint8_t) and
        // tag. Then exclude the tag from payload to decrypt.
        VerifyOrExit(aCmdOffset + sizeof(uint8_t) + kMleSecurityTagSize <= aMessage.GetLength(), error = kErrorParse);
        payloadLength -= kMleSecurityTagSize;
        break;
    }

    senderAddress->GetIid().ConvertToExtAddress(extAddress);
    Crypto::AesCcm::GenerateNonce(extAddress, aHeader.GetFrameCounter(), Mac::Frame::kSecEncMic32, nonce);

    keySequence = aHeader.GetKeyId();

    aesCcm.SetKey(keySequence == Get<KeyManager>().GetCurrentKeySequence()
                      ? Get<KeyManager>().GetCurrentMleKey()
                      : Get<KeyManager>().GetTemporaryMleKey(keySequence));

    aesCcm.Init(sizeof(Ip6::Address) + sizeof(Ip6::Address) + sizeof(SecurityHeader), payloadLength,
                kMleSecurityTagSize, nonce, sizeof(nonce));

    aesCcm.Header(*senderAddress);
    aesCcm.Header(*receiverAddress);
    aesCcm.Header(aHeader);

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    if (aMode == Crypto::AesCcm::kDecrypt)
    {
        // Skip decrypting the message under fuzz build mode
        IgnoreError(aMessage.SetLength(aMessage.GetLength() - kMleSecurityTagSize));
        ExitNow();
    }
#endif

    aesCcm.Payload(aMessage, aCmdOffset, payloadLength, aMode);
    aesCcm.Finalize(tag);

    if (aMode == Crypto::AesCcm::kEncrypt)
    {
        SuccessOrExit(error = aMessage.Append(tag));
    }
    else
    {
        VerifyOrExit(aMessage.Compare(aMessage.GetLength() - kMleSecurityTagSize, tag), error = kErrorSecurity);
        IgnoreError(aMessage.SetLength(aMessage.GetLength() - kMleSecurityTagSize));
    }

exit:
    return error;
}

Error Mle::TxMessage::SendTo(const Ip6::Address &aDestination)
{
    Error            error  = kErrorNone;
    uint16_t         offset = 0;
    uint8_t          securitySuite;
    Ip6::MessageInfo messageInfo;

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(Get<Mle>().mLinkLocal64.GetAddress());
    messageInfo.SetPeerPort(kUdpPort);
    messageInfo.SetHopLimit(kMleHopLimit);

    IgnoreError(Read(offset, securitySuite));
    offset += sizeof(securitySuite);

    if (securitySuite == k154Security)
    {
        SecurityHeader header;

        // Update the fields in the security header

        IgnoreError(Read(offset, header));
        header.SetFrameCounter(Get<KeyManager>().GetMleFrameCounter());
        header.SetKeyId(Get<KeyManager>().GetCurrentKeySequence());
        Write(offset, header);
        offset += sizeof(SecurityHeader);

        SuccessOrExit(
            error = Get<Mle>().ProcessMessageSecurity(Crypto::AesCcm::kEncrypt, *this, messageInfo, offset, header));

        Get<KeyManager>().IncrementMleFrameCounter();
    }

    SuccessOrExit(error = Get<Mle>().mSocket.SendTo(*this, messageInfo));

exit:
    return error;
}

Error Mle::TxMessage::SendAfterDelay(const Ip6::Address &aDestination, uint16_t aDelay)
{
    Error                   error = kErrorNone;
    DelayedResponseMetadata metadata;

    metadata.mSendTime    = TimerMilli::GetNow() + aDelay;
    metadata.mDestination = aDestination;

    SuccessOrExit(error = metadata.AppendTo(*this));
    Get<Mle>().mDelayedResponses.Enqueue(*this);

    Get<Mle>().mDelayedResponseTimer.FireAtIfEarlier(metadata.mSendTime);

exit:
    return error;
}

void Mle::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Mle *>(aContext)->HandleUdpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo));
}

void Mle::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error           error = kErrorNone;
    RxInfo          rxInfo(aMessage, aMessageInfo);
    uint8_t         securitySuite;
    SecurityHeader  header;
    uint32_t        keySequence;
    uint32_t        frameCounter;
    Mac::ExtAddress extAddr;
    uint8_t         command;
    Neighbor *      neighbor;

    LogDebg("Receive MLE message");

    VerifyOrExit(aMessageInfo.GetLinkInfo() != nullptr);
    VerifyOrExit(aMessageInfo.GetHopLimit() == kMleHopLimit, error = kErrorParse);

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), securitySuite));
    aMessage.MoveOffset(sizeof(securitySuite));

    if (securitySuite == kNoSecurity)
    {
        SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), command));
        aMessage.MoveOffset(sizeof(command));

        switch (command)
        {
#if OPENTHREAD_FTD
        case kCommandDiscoveryRequest:
            Get<MleRouter>().HandleDiscoveryRequest(rxInfo);
            break;
#endif
        case kCommandDiscoveryResponse:
            Get<DiscoverScanner>().HandleDiscoveryResponse(rxInfo);
            break;

        default:
            break;
        }

        ExitNow();
    }

    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(securitySuite == k154Security, error = kErrorParse);

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), header));
    aMessage.MoveOffset(sizeof(header));

    VerifyOrExit(header.IsSecurityControlValid(), error = kErrorParse);

    keySequence  = header.GetKeyId();
    frameCounter = header.GetFrameCounter();

    SuccessOrExit(
        error = ProcessMessageSecurity(Crypto::AesCcm::kDecrypt, aMessage, aMessageInfo, aMessage.GetOffset(), header));

    if (keySequence > Get<KeyManager>().GetCurrentKeySequence())
    {
        Get<KeyManager>().SetCurrentKeySequence(keySequence);
    }

    IgnoreError(aMessage.Read(aMessage.GetOffset(), command));
    aMessage.MoveOffset(sizeof(command));

    aMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);
    neighbor = (command == kCommandChildIdResponse) ? mNeighborTable.FindParent(extAddr)
                                                    : mNeighborTable.FindNeighbor(extAddr);

    if (neighbor != nullptr && neighbor->IsStateValid())
    {
        if (keySequence == neighbor->GetKeySequence())
        {
#if OPENTHREAD_CONFIG_MULTI_RADIO
            // Only when counter is exactly one off, we allow it to be
            // used for updating radio link info (by `RadioSelector`)
            // before message is dropped as a duplicate. This handles
            // the common case where a broadcast MLE message (such as
            // Link Advertisement) is received over multiple radio
            // links.

            if ((frameCounter + 1) == neighbor->GetMleFrameCounter())
            {
                OT_ASSERT(aMessage.IsRadioTypeSet());
                Get<RadioSelector>().UpdateOnReceive(*neighbor, aMessage.GetRadioType(), /* IsDuplicate */ true);

                // We intentionally exit without setting the error to
                // skip logging "Failed to process UDP" at the exit
                // label. Note that in multi-radio mode, receiving
                // duplicate MLE message (with one-off counter) would
                // be common and ok for broadcast MLE messages (e.g.
                // MLE Link Advertisements).
                ExitNow();
            }
#endif
            VerifyOrExit(frameCounter >= neighbor->GetMleFrameCounter(), error = kErrorDuplicated);
        }
        else
        {
            VerifyOrExit(keySequence > neighbor->GetKeySequence(), error = kErrorDuplicated);
            neighbor->SetKeySequence(keySequence);
            neighbor->GetLinkFrameCounters().Reset();
            neighbor->SetLinkAckFrameCounter(0);
        }

        neighbor->SetMleFrameCounter(frameCounter + 1);
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (neighbor != nullptr)
    {
        OT_ASSERT(aMessage.IsRadioTypeSet());
        Get<RadioSelector>().UpdateOnReceive(*neighbor, aMessage.GetRadioType(), /* IsDuplicate */ false);
    }
#endif

    rxInfo.mKeySequence  = keySequence;
    rxInfo.mFrameCounter = frameCounter;
    rxInfo.mNeighbor     = neighbor;

    switch (command)
    {
    case kCommandAdvertisement:
        HandleAdvertisement(rxInfo);
        break;

    case kCommandDataResponse:
        HandleDataResponse(rxInfo);
        break;

    case kCommandParentResponse:
        HandleParentResponse(rxInfo);
        break;

    case kCommandChildIdResponse:
        HandleChildIdResponse(rxInfo);
        break;

    case kCommandAnnounce:
        HandleAnnounce(rxInfo);
        break;

    case kCommandChildUpdateRequest:
#if OPENTHREAD_FTD
        if (IsRouterOrLeader())
        {
            Get<MleRouter>().HandleChildUpdateRequest(rxInfo);
        }
        else
#endif
        {
            HandleChildUpdateRequest(rxInfo);
        }

        break;

    case kCommandChildUpdateResponse:
#if OPENTHREAD_FTD
        if (IsRouterOrLeader())
        {
            Get<MleRouter>().HandleChildUpdateResponse(rxInfo);
        }
        else
#endif
        {
            HandleChildUpdateResponse(rxInfo);
        }

        break;

#if OPENTHREAD_FTD
    case kCommandLinkRequest:
        Get<MleRouter>().HandleLinkRequest(rxInfo);
        break;

    case kCommandLinkAccept:
        Get<MleRouter>().HandleLinkAccept(rxInfo);
        break;

    case kCommandLinkAcceptAndRequest:
        Get<MleRouter>().HandleLinkAcceptAndRequest(rxInfo);
        break;

    case kCommandDataRequest:
        Get<MleRouter>().HandleDataRequest(rxInfo);
        break;

    case kCommandParentRequest:
        Get<MleRouter>().HandleParentRequest(rxInfo);
        break;

    case kCommandChildIdRequest:
        Get<MleRouter>().HandleChildIdRequest(rxInfo);
        break;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    case kCommandTimeSync:
        Get<MleRouter>().HandleTimeSync(rxInfo);
        break;
#endif
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    case kCommandLinkMetricsManagementRequest:
        HandleLinkMetricsManagementRequest(rxInfo);
        break;
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    case kCommandLinkMetricsManagementResponse:
        HandleLinkMetricsManagementResponse(rxInfo);
        break;
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    case kCommandLinkProbe:
        HandleLinkProbe(rxInfo);
        break;
#endif

    default:
        ExitNow(error = kErrorDrop);
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    // If we could not find a neighbor matching the MAC address of the
    // received MLE messages, or if the neighbor is now invalid, we
    // check again after the message is handled with a relaxed neighbor
    // state filer. The processing of the received MLE message may
    // create a new neighbor or change the neighbor table (e.g.,
    // receiving a "Parent Request" from a new child, or processing a
    // "Link Request" from a previous child which is being promoted to a
    // router).

    if ((neighbor == nullptr) || neighbor->IsStateInvalid())
    {
        neighbor = Get<NeighborTable>().FindNeighbor(extAddr, Neighbor::kInStateAnyExceptInvalid);

        if (neighbor != nullptr)
        {
            Get<RadioSelector>().UpdateOnReceive(*neighbor, aMessage.GetRadioType(), /* aIsDuplicate */ false);
        }
    }
#endif

exit:
    // We skip logging failures for broadcast MLE messages since it
    // can be common to receive such messages from adjacent Thread
    // networks.
    if (!aMessageInfo.GetSockAddr().IsMulticast() || !aMessageInfo.GetThreadLinkInfo()->IsDstPanIdBroadcast())
    {
        LogProcessError(kTypeGenericUdp, error);
    }
}

void Mle::HandleAdvertisement(RxInfo &aRxInfo)
{
    Error      error = kErrorNone;
    uint16_t   sourceAddress;
    LeaderData leaderData;
    uint8_t    tlvs[] = {Tlv::kNetworkData};
    uint16_t   delay;

    // Source Address
    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeAdvertisement, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    // Leader Data
    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

    if (!IsDetached())
    {
#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            SuccessOrExit(error = Get<MleRouter>().HandleAdvertisement(aRxInfo));
        }
        else
#endif
        {
            if ((aRxInfo.mNeighbor == &mParent) && (mParent.GetRloc16() != sourceAddress))
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
        VerifyOrExit(aRxInfo.mNeighbor == &mParent);

        if ((mParent.GetRloc16() == sourceAddress) && (leaderData.GetPartitionId() != mLeaderData.GetPartitionId() ||
                                                       leaderData.GetLeaderRouterId() != GetLeaderId()))
        {
            SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

#if OPENTHREAD_FTD
            if (IsFullThreadDevice())
            {
                switch (Get<MleRouter>().ProcessRouteTlv(aRxInfo))
                {
                case kErrorNone:
                case kErrorNotFound:
                    break;
                default:
                    ExitNow(error = kErrorParse);
                }
            }
#endif

            mRetrieveNewNetworkData = true;
        }

        mParent.SetLastHeard(TimerMilli::GetNow());
        break;

    case kRoleRouter:
    case kRoleLeader:
        VerifyOrExit(aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid());
        break;
    }

    if (mRetrieveNewNetworkData || IsNetworkDataNewer(leaderData))
    {
        delay = Random::NonCrypto::GetUint16InRange(0, kMleMaxResponseDelay);
        IgnoreError(SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay));
    }

exit:
    LogProcessError(kTypeAdvertisement, error);
}

void Mle::HandleDataResponse(RxInfo &aRxInfo)
{
    Error error;
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    uint16_t metricsReportValueOffset;
    uint16_t length;
#endif

    Log(kMessageReceive, kTypeDataResponse, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid(), error = kErrorDrop);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    if (Tlv::FindTlvValueOffset(aRxInfo.mMessage, Tlv::kLinkMetricsReport, metricsReportValueOffset, length) ==
        kErrorNone)
    {
        Get<LinkMetrics::LinkMetrics>().HandleReport(aRxInfo.mMessage, metricsReportValueOffset, length,
                                                     aRxInfo.mMessageInfo.GetPeerAddr());
    }
#endif

    error = HandleLeaderData(aRxInfo);

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
    return SerialNumber::IsGreater(aLeaderData.GetDataVersion(GetNetworkDataType()),
                                   Get<NetworkData::Leader>().GetVersion(GetNetworkDataType()));
}

Error Mle::HandleLeaderData(RxInfo &aRxInfo)
{
    Error                     error = kErrorNone;
    LeaderData                leaderData;
    MeshCoP::Timestamp        activeTimestamp;
    MeshCoP::Timestamp        pendingTimestamp;
    const MeshCoP::Timestamp *timestamp;
    bool                      hasActiveTimestamp   = false;
    bool                      hasPendingTimestamp  = false;
    uint16_t                  networkDataOffset    = 0;
    uint16_t                  activeDatasetOffset  = 0;
    uint16_t                  pendingDatasetOffset = 0;
    bool                      dataRequest          = false;
    Tlv                       tlv;

    // Leader Data
    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

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
            ExitNow(error = kErrorDrop);
        }
    }
    else if (!mRetrieveNewNetworkData)
    {
        VerifyOrExit(IsNetworkDataNewer(leaderData));
    }

    // Active Timestamp
    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, activeTimestamp))
    {
    case kErrorNone:
        hasActiveTimestamp = true;

        timestamp = Get<MeshCoP::ActiveDatasetManager>().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if (!IsLeader() && (MeshCoP::Timestamp::Compare(&activeTimestamp, timestamp) != 0) &&
            (Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kActiveDataset, activeDatasetOffset) != kErrorNone))
        {
            ExitNow(dataRequest = true);
        }

        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // Pending Timestamp
    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, pendingTimestamp))
    {
    case kErrorNone:
        hasPendingTimestamp = true;

        timestamp = Get<MeshCoP::PendingDatasetManager>().GetTimestamp();

        // if received timestamp does not match the local value and message does not contain the dataset,
        // send MLE Data Request
        if (!IsLeader() && (MeshCoP::Timestamp::Compare(&pendingTimestamp, timestamp) != 0) &&
            (Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kPendingDataset, pendingDatasetOffset) != kErrorNone))
        {
            ExitNow(dataRequest = true);
        }

        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    if (Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kNetworkData, networkDataOffset) == kErrorNone)
    {
        error = Get<NetworkData::Leader>().SetNetworkData(leaderData.GetDataVersion(NetworkData::kFullSet),
                                                          leaderData.GetDataVersion(NetworkData::kStableSubset),
                                                          GetNetworkDataType(), aRxInfo.mMessage, networkDataOffset);
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
        if (hasActiveTimestamp)
        {
            if (activeDatasetOffset > 0)
            {
                IgnoreError(aRxInfo.mMessage.Read(activeDatasetOffset, tlv));
                IgnoreError(Get<MeshCoP::ActiveDatasetManager>().Save(
                    activeTimestamp, aRxInfo.mMessage, activeDatasetOffset + sizeof(tlv), tlv.GetLength()));
            }
        }

        // Pending Dataset
        if (hasPendingTimestamp)
        {
            if (pendingDatasetOffset > 0)
            {
                IgnoreError(aRxInfo.mMessage.Read(pendingDatasetOffset, tlv));
                IgnoreError(Get<MeshCoP::PendingDatasetManager>().Save(
                    pendingTimestamp, aRxInfo.mMessage, pendingDatasetOffset + sizeof(tlv), tlv.GetLength()));
            }
        }
    }

    mRetrieveNewNetworkData = false;

exit:

    if (dataRequest)
    {
        static const uint8_t tlvs[] = {Tlv::kNetworkData};
        uint16_t             delay;

        if (aRxInfo.mMessageInfo.GetSockAddr().IsMulticast())
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

        IgnoreError(SendDataRequest(aRxInfo.mMessageInfo.GetPeerAddr(), tlvs, sizeof(tlvs), delay));
    }
    else if (error == kErrorNone)
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
                         LinkQuality            aLinkQuality,
                         uint8_t                aLinkMargin,
                         const ConnectivityTlv &aConnectivityTlv,
                         uint8_t                aVersion,
                         uint8_t                aCslClockAccuracy,
                         uint8_t                aCslUncertainty)
{
    bool rval = false;

    LinkQuality candidateLinkQualityIn     = mParentCandidate.GetLinkInfo().GetLinkQuality();
    LinkQuality candidateTwoWayLinkQuality = OT_MIN(candidateLinkQualityIn, mParentCandidate.GetLinkQualityOut());
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    uint64_t candidateCslMetric = 0;
    uint64_t cslMetric          = 0;
#else
    OT_UNUSED_VARIABLE(aCslClockAccuracy);
    OT_UNUSED_VARIABLE(aCslUncertainty);
#endif

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

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    // CSL metric
    if (!IsRxOnWhenIdle())
    {
        cslMetric = CalcParentCslMetric(aCslClockAccuracy, aCslUncertainty);
        candidateCslMetric =
            CalcParentCslMetric(mParentCandidate.GetCslClockAccuracy(), mParentCandidate.GetCslUncertainty());
        if (candidateCslMetric != cslMetric)
        {
            ExitNow(rval = (cslMetric < candidateCslMetric));
        }
    }
#endif

    rval = (aLinkMargin > mParentLinkMargin);

exit:
    return rval;
}

void Mle::HandleParentResponse(RxInfo &aRxInfo)
{
    Error                 error    = kErrorNone;
    const ThreadLinkInfo *linkInfo = aRxInfo.mMessageInfo.GetThreadLinkInfo();
    Challenge             response;
    uint16_t              version;
    uint16_t              sourceAddress;
    LeaderData            leaderData;
    uint8_t               linkMarginFromTlv;
    uint8_t               linkMargin;
    LinkQuality           linkQuality;
    ConnectivityTlv       connectivity;
    uint32_t              linkFrameCounter;
    uint32_t              mleFrameCounter;
    Mac::ExtAddress       extAddress;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    TimeParameterTlv timeParameter;
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    CslClockAccuracyTlv clockAccuracy;
#endif

    // Source Address
    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeParentResponse, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    // Version
    SuccessOrExit(error = Tlv::Find<VersionTlv>(aRxInfo.mMessage, version));
    VerifyOrExit(version >= OT_THREAD_VERSION_1_1, error = kErrorParse);

    // Response
    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));
    VerifyOrExit(response == mParentRequestChallenge, error = kErrorParse);

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddress);

    if (IsChild() && mParent.GetExtAddress() == extAddress)
    {
        mReceivedResponseFromParent = true;
    }

    // Leader Data
    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

    // Link Margin
    SuccessOrExit(error = Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMarginFromTlv));

    linkMargin = LinkQualityInfo::ConvertRssToLinkMargin(Get<Mac::Mac>().GetNoiseFloor(), linkInfo->GetRss());

    if (linkMargin > linkMarginFromTlv)
    {
        linkMargin = linkMarginFromTlv;
    }

    linkQuality = LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMargin);

    // Connectivity
    SuccessOrExit(error = Tlv::FindTlv(aRxInfo.mMessage, connectivity));
    VerifyOrExit(connectivity.IsValid(), error = kErrorParse);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    // CSL Accuracy
    if (Tlv::FindTlv(aRxInfo.mMessage, clockAccuracy) != kErrorNone)
    {
        clockAccuracy.SetCslClockAccuracy(kCslWorstCrystalPpm);
        clockAccuracy.SetCslUncertainty(kCslWorstUncertainty);
    }
#endif

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
        bool isPartitionIdSame = (leaderData.GetPartitionId() == mLeaderData.GetPartitionId());
        bool isIdSequenceSame  = (connectivity.GetIdSequence() == Get<RouterTable>().GetRouterIdSequence());
        bool isIdSequenceGreater =
            SerialNumber::IsGreater(connectivity.GetIdSequence(), Get<RouterTable>().GetRouterIdSequence());

        switch (mAttachMode)
        {
        case kAnyPartition:
        case kBetterParent:
            VerifyOrExit(!isPartitionIdSame || isIdSequenceGreater);
            break;

        case kSamePartition:
        case kSamePartitionRetry:
            VerifyOrExit(isPartitionIdSame && isIdSequenceGreater);
            break;

        case kDowngradeToReed:
            VerifyOrExit(isPartitionIdSame && (isIdSequenceSame || isIdSequenceGreater));
            break;

        case kBetterPartition:
            VerifyOrExit(!isPartitionIdSame);

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

#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            compare = MleRouter::ComparePartitions(connectivity.GetActiveRouters() <= 1, leaderData, mParentIsSingleton,
                                                   mParentLeaderData);
        }

        // only consider partitions that are the same or better
        VerifyOrExit(compare >= 0);
#endif

        // only consider better parents if the partitions are the same
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        VerifyOrExit(compare != 0 ||
                     IsBetterParent(sourceAddress, linkQuality, linkMargin, connectivity, static_cast<uint8_t>(version),
                                    clockAccuracy.GetCslClockAccuracy(), clockAccuracy.GetCslUncertainty()));
#else
        VerifyOrExit(compare != 0 || IsBetterParent(sourceAddress, linkQuality, linkMargin, connectivity,
                                                    static_cast<uint8_t>(version), 0, 0));
#endif
    }

    // Link/MLE Frame Counters
    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    // Time Parameter
    if (Tlv::FindTlv(aRxInfo.mMessage, timeParameter) == kErrorNone)
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
    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(mParentCandidateChallenge));

    mParentCandidate.SetExtAddress(extAddress);
    mParentCandidate.SetRloc16(sourceAddress);
    mParentCandidate.GetLinkFrameCounters().SetAll(linkFrameCounter);
    mParentCandidate.SetLinkAckFrameCounter(linkFrameCounter);
    mParentCandidate.SetMleFrameCounter(mleFrameCounter);
    mParentCandidate.SetVersion(static_cast<uint8_t>(version));
    mParentCandidate.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                              DeviceMode::kModeFullNetworkData));
    mParentCandidate.GetLinkInfo().Clear();
    mParentCandidate.GetLinkInfo().AddRss(linkInfo->GetRss());
    mParentCandidate.ResetLinkFailures();
    mParentCandidate.SetLinkQualityOut(LinkQualityInfo::ConvertLinkMarginToLinkQuality(linkMarginFromTlv));
    mParentCandidate.SetState(Neighbor::kStateParentResponse);
    mParentCandidate.SetKeySequence(aRxInfo.mKeySequence);
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mParentCandidate.SetCslClockAccuracy(clockAccuracy.GetCslClockAccuracy());
    mParentCandidate.SetCslUncertainty(clockAccuracy.GetCslUncertainty());
#endif

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

void Mle::HandleChildIdResponse(RxInfo &aRxInfo)
{
    Error              error = kErrorNone;
    LeaderData         leaderData;
    uint16_t           sourceAddress;
    uint16_t           shortAddress;
    MeshCoP::Timestamp timestamp;
    Tlv                tlv;
    uint16_t           networkDataOffset;
    uint16_t           offset;

    // Source Address
    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeChildIdResponse, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    VerifyOrExit(aRxInfo.mNeighbor && aRxInfo.mNeighbor->IsStateValid(), error = kErrorSecurity);

    VerifyOrExit(mAttachState == kAttachStateChildIdRequest);

    // ShortAddress
    SuccessOrExit(error = Tlv::Find<Address16Tlv>(aRxInfo.mMessage, shortAddress));
    VerifyOrExit(RouterIdMatch(sourceAddress, shortAddress), error = kErrorRejected);

    // Leader Data
    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

    // Network Data
    error = Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kNetworkData, networkDataOffset);
    SuccessOrExit(error);

    // Active Timestamp
    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        // Active Dataset
        if (Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kActiveDataset, offset) == kErrorNone)
        {
            IgnoreError(aRxInfo.mMessage.Read(offset, tlv));
            SuccessOrExit(error = Get<MeshCoP::ActiveDatasetManager>().Save(timestamp, aRxInfo.mMessage,
                                                                            offset + sizeof(tlv), tlv.GetLength()));
        }
        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // clear Pending Dataset if device succeed to reattach using stored Pending Dataset
    if (mReattachState == kReattachPending)
    {
        Get<MeshCoP::PendingDatasetManager>().Clear();
    }

    // Pending Timestamp
    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        // Pending Dataset
        if (Tlv::FindTlvOffset(aRxInfo.mMessage, Tlv::kPendingDataset, offset) == kErrorNone)
        {
            IgnoreError(aRxInfo.mMessage.Read(offset, tlv));
            IgnoreError(Get<MeshCoP::PendingDatasetManager>().Save(timestamp, aRxInfo.mMessage, offset + sizeof(tlv),
                                                                   tlv.GetLength()));
        }
        break;

    case kErrorNotFound:
        Get<MeshCoP::PendingDatasetManager>().ClearNetwork();
        break;

    default:
        ExitNow(error = kErrorParse);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    // Sync to Thread network time
    if (aRxInfo.mMessage.GetTimeSyncSeq() != OT_TIME_SYNC_INVALID_SEQ)
    {
        Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);
    }
#endif

    // Parent Attach Success

    SetStateDetached();

    SetLeaderData(leaderData.GetPartitionId(), leaderData.GetWeighting(), leaderData.GetLeaderRouterId());

#if OPENTHREAD_FTD
    if (IsFullThreadDevice())
    {
        switch (Get<MleRouter>().ProcessRouteTlv(aRxInfo))
        {
        case kErrorNone:
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }
    }
#endif

    mParent = mParentCandidate;
    mParentCandidate.Clear();

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    Get<Mac::Mac>().SetCslParentUncertainty(mParent.GetCslUncertainty());
    Get<Mac::Mac>().SetCslParentClockAccuracy(mParent.GetCslClockAccuracy());
#endif

    mParent.SetRloc16(sourceAddress);

    IgnoreError(Get<NetworkData::Leader>().SetNetworkData(leaderData.GetDataVersion(NetworkData::kFullSet),
                                                          leaderData.GetDataVersion(NetworkData::kStableSubset),
                                                          GetNetworkDataType(), aRxInfo.mMessage, networkDataOffset));

    SetStateChild(shortAddress);

    if (!IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SetAttachMode(false);
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
    }
    else
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(true);
    }

exit:
    LogProcessError(kTypeChildIdResponse, error);
}

void Mle::HandleChildUpdateRequest(RxInfo &aRxInfo)
{
    static const uint8_t kMaxResponseTlvs = 6;

    Error         error = kErrorNone;
    uint16_t      sourceAddress;
    Challenge     challenge;
    RequestedTlvs requestedTlvs;
    uint8_t       tlvs[kMaxResponseTlvs] = {};
    uint8_t       numTlvs                = 0;

    // Source Address
    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeChildUpdateRequestOfParent, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    // Challenge
    switch (aRxInfo.mMessage.ReadChallengeTlv(challenge))
    {
    case kErrorNone:
        tlvs[numTlvs++] = Tlv::kResponse;
        tlvs[numTlvs++] = Tlv::kMleFrameCounter;
        tlvs[numTlvs++] = Tlv::kLinkFrameCounter;
        break;
    case kErrorNotFound:
        challenge.mLength = 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    if (aRxInfo.mNeighbor == &mParent)
    {
        uint8_t status;

        switch (Tlv::Find<StatusTlv>(aRxInfo.mMessage, status))
        {
        case kErrorNone:
            VerifyOrExit(status != StatusTlv::kError, IgnoreError(BecomeDetached()));
            break;
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }

        if (mParent.GetRloc16() != sourceAddress)
        {
            IgnoreError(BecomeDetached());
            ExitNow();
        }

        // Leader Data, Network Data, Active Timestamp, Pending Timestamp
        SuccessOrExit(error = HandleLeaderData(aRxInfo));

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        CslClockAccuracyTlv cslClockAccuracyTlv;
        if (Tlv::FindTlv(aRxInfo.mMessage, cslClockAccuracyTlv) == kErrorNone)
        {
            // MUST include CSL timeout TLV when request includes CSL accuracy
            tlvs[numTlvs++] = Tlv::kCslTimeout;
        }
#endif
    }
    else
    {
        // this device is not a child of the Child Update Request source
        tlvs[numTlvs++] = Tlv::kStatus;
    }

    // TLV Request
    switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvs))
    {
    case kErrorNone:
        for (uint8_t i = 0; i < requestedTlvs.mNumTlvs; i++)
        {
            if (numTlvs >= sizeof(tlvs))
            {
                LogWarn("Failed to respond with TLVs: %d of %d", i, requestedTlvs.mNumTlvs);
                break;
            }

            tlvs[numTlvs++] = requestedTlvs.mTlvs[i];
        }
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if ((aRxInfo.mNeighbor != nullptr) && (challenge.mLength != 0))
    {
        aRxInfo.mNeighbor->ClearLastRxFragmentTag();
    }
#endif

    SuccessOrExit(error = SendChildUpdateResponse(tlvs, numTlvs, challenge));

exit:
    LogProcessError(kTypeChildUpdateRequestOfParent, error);
}

void Mle::HandleChildUpdateResponse(RxInfo &aRxInfo)
{
    Error     error = kErrorNone;
    uint8_t   status;
    uint8_t   mode;
    Challenge response;
    uint32_t  linkFrameCounter;
    uint32_t  mleFrameCounter;
    uint16_t  sourceAddress;
    uint32_t  timeout;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    CslClockAccuracyTlv clockAccuracy;
#endif

    Log(kMessageReceive, kTypeChildUpdateResponseOfParent, aRxInfo.mMessageInfo.GetPeerAddr());

    switch (mRole)
    {
    case kRoleDetached:
        SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));
        VerifyOrExit(response == mParentRequestChallenge, error = kErrorSecurity);
        break;

    case kRoleChild:
        VerifyOrExit((aRxInfo.mNeighbor == &mParent) && mParent.IsStateValid(), error = kErrorSecurity);
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    // Status
    if (Tlv::Find<StatusTlv>(aRxInfo.mMessage, status) == kErrorNone)
    {
        IgnoreError(BecomeDetached());
        ExitNow();
    }

    // Mode
    SuccessOrExit(error = Tlv::Find<ModeTlv>(aRxInfo.mMessage, mode));
    VerifyOrExit(DeviceMode(mode) == mDeviceMode, error = kErrorDrop);

    switch (mRole)
    {
    case kRoleDetached:
        SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));

        mParent.GetLinkFrameCounters().SetAll(linkFrameCounter);
        mParent.SetLinkAckFrameCounter(linkFrameCounter);
        mParent.SetMleFrameCounter(mleFrameCounter);

        mParent.SetState(Neighbor::kStateValid);
        SetStateChild(GetRloc16());

        mRetrieveNewNetworkData = true;

        OT_FALL_THROUGH;

    case kRoleChild:
        // Source Address
        SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

        if (RouterIdFromRloc16(sourceAddress) != RouterIdFromRloc16(GetRloc16()))
        {
            IgnoreError(BecomeDetached());
            ExitNow();
        }

        // Leader Data, Network Data, Active Timestamp, Pending Timestamp
        SuccessOrExit(error = HandleLeaderData(aRxInfo));

        // Timeout optional
        switch (Tlv::Find<TimeoutTlv>(aRxInfo.mMessage, timeout))
        {
        case kErrorNone:
            mTimeout = timeout;
            break;
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        // CSL Accuracy
        if (Tlv::FindTlv(aRxInfo.mMessage, clockAccuracy) != kErrorNone)
        {
            Get<Mac::Mac>().SetCslParentClockAccuracy(clockAccuracy.GetCslClockAccuracy());
            Get<Mac::Mac>().SetCslParentUncertainty(clockAccuracy.GetCslUncertainty());
        }
#endif

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

    if (error == kErrorNone)
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

void Mle::HandleAnnounce(RxInfo &aRxInfo)
{
    Error                     error = kErrorNone;
    ChannelTlv                channelTlv;
    MeshCoP::Timestamp        timestamp;
    const MeshCoP::Timestamp *localTimestamp;
    uint8_t                   channel;
    uint16_t                  panId;

    Log(kMessageReceive, kTypeAnnounce, aRxInfo.mMessageInfo.GetPeerAddr());

    SuccessOrExit(error = Tlv::FindTlv(aRxInfo.mMessage, channelTlv));
    VerifyOrExit(channelTlv.IsValid(), error = kErrorParse);

    channel = static_cast<uint8_t>(channelTlv.GetChannel());

    SuccessOrExit(error = Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp));
    SuccessOrExit(error = Tlv::Find<PanIdTlv>(aRxInfo.mMessage, panId));

    localTimestamp = Get<MeshCoP::ActiveDatasetManager>().GetTimestamp();

    if (MeshCoP::Timestamp::Compare(&timestamp, localTimestamp) > 0)
    {
        // No action is required if device is detached, and current
        // channel and pan-id match the values from the received MLE
        // Announce message.

        VerifyOrExit(!IsDetached() || (Get<Mac::Mac>().GetPanChannel() != channel) ||
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
        mAttachCounter = 0;

        LogNote("Delay processing Announce - channel %d, panid 0x%02x", channel, panId);
    }
    else if (MeshCoP::Timestamp::Compare(&timestamp, localTimestamp) < 0)
    {
        SendAnnounce(channel);

#if OPENTHREAD_CONFIG_MLE_SEND_UNICAST_ANNOUNCE_RESPONSE
        SendAnnounce(channel, aRxInfo.mMessageInfo.GetPeerAddr());
#endif
    }
    else
    {
        // Timestamps are equal.

#if OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
        // Notify `AnnounceSender` of the received Announce
        // message so it can update its state to determine
        // whether to send Announce or not.
        Get<AnnounceSender>().UpdateOnReceivedAnnounce();
#endif
    }

exit:
    LogProcessError(kTypeAnnounce, error);
}

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
void Mle::HandleLinkMetricsManagementRequest(RxInfo &aRxInfo)
{
    Error               error = kErrorNone;
    LinkMetrics::Status status;

    Log(kMessageReceive, kTypeLinkMetricsManagementRequest, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.mNeighbor != nullptr, error = kErrorInvalidState);

    SuccessOrExit(
        error = Get<LinkMetrics::LinkMetrics>().HandleManagementRequest(aRxInfo.mMessage, *aRxInfo.mNeighbor, status));
    error = SendLinkMetricsManagementResponse(aRxInfo.mMessageInfo.GetPeerAddr(), status);

exit:
    LogProcessError(kTypeLinkMetricsManagementRequest, error);
}

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
void Mle::HandleLinkMetricsManagementResponse(RxInfo &aRxInfo)
{
    Error error = kErrorNone;

    Log(kMessageReceive, kTypeLinkMetricsManagementResponse, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.mNeighbor != nullptr, error = kErrorInvalidState);

    error =
        Get<LinkMetrics::LinkMetrics>().HandleManagementResponse(aRxInfo.mMessage, aRxInfo.mMessageInfo.GetPeerAddr());

exit:
    LogProcessError(kTypeLinkMetricsManagementResponse, error);
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
void Mle::HandleLinkProbe(RxInfo &aRxInfo)
{
    Error   error = kErrorNone;
    uint8_t seriesId;

    Log(kMessageReceive, kTypeLinkProbe, aRxInfo.mMessageInfo.GetPeerAddr());

    SuccessOrExit(error = Get<LinkMetrics::LinkMetrics>().HandleLinkProbe(aRxInfo.mMessage, seriesId));
    aRxInfo.mNeighbor->AggregateLinkMetrics(seriesId, LinkMetrics::SeriesInfo::kSeriesTypeLinkProbe,
                                            aRxInfo.mMessage.GetAverageLqi(), aRxInfo.mMessage.GetAverageRss());

exit:
    LogProcessError(kTypeLinkProbe, error);
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

void Mle::ProcessAnnounce(void)
{
    uint8_t  newChannel = mAlternateChannel;
    uint16_t newPanId   = mAlternatePanId;

    OT_ASSERT(mAttachState == kAttachStateProcessAnnounce);

    LogNote("Processing Announce - channel %d, panid 0x%02x", newChannel, newPanId);

    Stop(kKeepNetworkDatasets);

    // Save the current/previous channel and pan-id
    mAlternateChannel   = Get<Mac::Mac>().GetPanChannel();
    mAlternatePanId     = Get<Mac::Mac>().GetPanId();
    mAlternateTimestamp = 0;

    IgnoreError(Get<Mac::Mac>().SetPanChannel(newChannel));
    Get<Mac::Mac>().SetPanId(newPanId);

    IgnoreError(Start(kAnnounceAttach));
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

Error Mle::CheckReachability(uint16_t aMeshDest, Ip6::Header &aIp6Header)
{
    Error error;

    if ((aMeshDest != GetRloc16()) || Get<ThreadNetif>().HasUnicastAddress(aIp6Header.GetDestination()))
    {
        error = kErrorNone;
    }
    else
    {
        error = kErrorNoRoute;
    }

    return error;
}

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
void Mle::InformPreviousParent(void)
{
    Error            error   = kErrorNone;
    Message *        message = nullptr;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Ip6::Ip6>().NewMessage(0)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->SetLength(0));

    messageInfo.SetSockAddr(GetMeshLocal64());
    messageInfo.SetPeerAddr(GetMeshLocal16());
    messageInfo.GetPeerAddr().GetIid().SetLocator(mPreviousParentRloc);

    SuccessOrExit(error = Get<Ip6::Ip6>().SendDatagram(*message, messageInfo, Ip6::kProtoNone));

    LogNote("Sending message to inform previous parent 0x%04x", mPreviousParentRloc);

exit:

    if (error != kErrorNone)
    {
        LogWarn("Failed to inform previous parent: %s", ErrorToString(error));

        FreeMessage(message);
    }
}
#endif // OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
void Mle::HandleParentSearchTimer(Timer &aTimer)
{
    aTimer.Get<Mle>().HandleParentSearchTimer();
}

void Mle::HandleParentSearchTimer(void)
{
    int8_t parentRss;

    LogInfo("PeriodicParentSearch: %s interval passed", mParentSearchIsInBackoff ? "Backoff" : "Check");

    if (mParentSearchBackoffWasCanceled)
    {
        // Backoff can be canceled if the device switches to a new parent.
        // from `UpdateParentSearchState()`. We want to limit this to happen
        // only once within a backoff interval.

        if (TimerMilli::GetNow() - mParentSearchBackoffCancelTime >= kParentSearchBackoffInterval)
        {
            mParentSearchBackoffWasCanceled = false;
            LogInfo("PeriodicParentSearch: Backoff cancellation is allowed on parent switch");
        }
    }

    mParentSearchIsInBackoff = false;

    VerifyOrExit(IsChild());

    parentRss = GetParent().GetLinkInfo().GetAverageRss();
    LogInfo("PeriodicParentSearch: Parent RSS %d", parentRss);
    VerifyOrExit(parentRss != OT_RADIO_RSSI_INVALID);

    if (parentRss < kParentSearchRssThreadhold)
    {
        LogInfo("PeriodicParentSearch: Parent RSS less than %d, searching for new parents", kParentSearchRssThreadhold);
        mParentSearchIsInBackoff = true;
        Attach(kBetterParent);
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

    LogInfo("PeriodicParentSearch: (Re)starting timer for %s interval", mParentSearchIsInBackoff ? "backoff" : "check");
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
            LogInfo("PeriodicParentSearch: Canceling backoff on switching to a new parent");
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

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
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
        rlocString.Append(",0x%04x", aRloc);
    }

    LogInfo("%s %s%s (%s%s)", MessageActionToString(aAction), MessageTypeToString(aType),
            MessageTypeActionToSuffixString(aType, aAction), aAddress.ToString().AsCString(), rlocString.AsCString());
}
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
void Mle::LogProcessError(MessageType aType, Error aError)
{
    LogError(kMessageReceive, aType, aError);
}

void Mle::LogSendError(MessageType aType, Error aError)
{
    LogError(kMessageSend, aType, aError);
}

void Mle::LogError(MessageAction aAction, MessageType aType, Error aError)
{
    if (aError != kErrorNone)
    {
        if (aAction == kMessageReceive && (aError == kErrorDrop || aError == kErrorNoRoute))
        {
            LogInfo("Failed to %s %s%s: %s", "process", MessageTypeToString(aType),
                    MessageTypeActionToSuffixString(aType, aAction), ErrorToString(aError));
        }
        else
        {
            LogWarn("Failed to %s %s%s: %s", aAction == kMessageSend ? "send" : "process", MessageTypeToString(aType),
                    MessageTypeActionToSuffixString(aType, aAction), ErrorToString(aError));
        }
    }
}

const char *Mle::MessageActionToString(MessageAction aAction)
{
    static const char *const kMessageActionStrings[] = {
        "Send",           // (0) kMessageSend
        "Receive",        // (1) kMessageReceive
        "Delay",          // (2) kMessageDelay
        "Remove Delayed", // (3) kMessageRemoveDelayed
    };

    static_assert(kMessageSend == 0, "kMessageSend value is incorrect");
    static_assert(kMessageReceive == 1, "kMessageReceive value is incorrect");
    static_assert(kMessageDelay == 2, "kMessageDelay value is incorrect");
    static_assert(kMessageRemoveDelayed == 3, "kMessageRemoveDelayed value is incorrect");

    return kMessageActionStrings[aAction];
}

const char *Mle::MessageTypeToString(MessageType aType)
{
    static const char *const kMessageTypeStrings[] = {
        "Advertisement",         // (0)  kTypeAdvertisement
        "Announce",              // (1)  kTypeAnnounce
        "Child ID Request",      // (2)  kTypeChildIdRequest
        "Child ID Request",      // (3)  kTypeChildIdRequestShort
        "Child ID Response",     // (4)  kTypeChildIdResponse
        "Child Update Request",  // (5)  kTypeChildUpdateRequestOfParent
        "Child Update Response", // (6)  kTypeChildUpdateResponseOfParent
        "Data Request",          // (7)  kTypeDataRequest
        "Data Response",         // (8)  kTypeDataResponse
        "Discovery Request",     // (9)  kTypeDiscoveryRequest
        "Discovery Response",    // (10) kTypeDiscoveryResponse
        "delayed message",       // (11) kTypeGenericDelayed
        "UDP",                   // (12) kTypeGenericUdp
        "Parent Request",        // (13) kTypeParentRequestToRouters
        "Parent Request",        // (14) kTypeParentRequestToRoutersReeds
        "Parent Response",       // (15) kTypeParentResponse
#if OPENTHREAD_FTD
        "Address Release",         // (16) kTypeAddressRelease
        "Address Release Reply",   // (17) kTypeAddressReleaseReply
        "Address Reply",           // (18) kTypeAddressReply
        "Address Solicit",         // (19) kTypeAddressSolicit
        "Child Update Request",    // (20) kTypeChildUpdateRequestOfChild
        "Child Update Response",   // (21) kTypeChildUpdateResponseOfChild
        "Child Update Response",   // (22) kTypeChildUpdateResponseOfUnknownChild
        "Link Accept",             // (23) kTypeLinkAccept
        "Link Accept and Request", // (24) kTypeLinkAcceptAndRequest
        "Link Reject",             // (25) kTypeLinkReject
        "Link Request",            // (26) kTypeLinkRequest
        "Parent Request",          // (27) kTypeParentRequest
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        "Time Sync", // (28) kTypeTimeSync
#endif
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        "Link Metrics Management Request",  // (29) kTypeLinkMetricsManagementRequest
        "Link Metrics Management Response", // (30) kTypeLinkMetricsManagementResponse
        "Link Probe",                       // (31) kTypeLinkProbe
#endif
    };

    static_assert(kTypeAdvertisement == 0, "kTypeAdvertisement value is incorrect");
    static_assert(kTypeAnnounce == 1, "kTypeAnnounce value is incorrect");
    static_assert(kTypeChildIdRequest == 2, "kTypeChildIdRequest value is incorrect");
    static_assert(kTypeChildIdRequestShort == 3, "kTypeChildIdRequestShort value is incorrect");
    static_assert(kTypeChildIdResponse == 4, "kTypeChildIdResponse value is incorrect");
    static_assert(kTypeChildUpdateRequestOfParent == 5, "kTypeChildUpdateRequestOfParent value is incorrect");
    static_assert(kTypeChildUpdateResponseOfParent == 6, "kTypeChildUpdateResponseOfParent value is incorrect");
    static_assert(kTypeDataRequest == 7, "kTypeDataRequest value is incorrect");
    static_assert(kTypeDataResponse == 8, "kTypeDataResponse value is incorrect");
    static_assert(kTypeDiscoveryRequest == 9, "kTypeDiscoveryRequest value is incorrect");
    static_assert(kTypeDiscoveryResponse == 10, "kTypeDiscoveryResponse value is incorrect");
    static_assert(kTypeGenericDelayed == 11, "kTypeGenericDelayed value is incorrect");
    static_assert(kTypeGenericUdp == 12, "kTypeGenericUdp value is incorrect");
    static_assert(kTypeParentRequestToRouters == 13, "kTypeParentRequestToRouters value is incorrect");
    static_assert(kTypeParentRequestToRoutersReeds == 14, "kTypeParentRequestToRoutersReeds value is incorrect");
    static_assert(kTypeParentResponse == 15, "kTypeParentResponse value is incorrect");
#if OPENTHREAD_FTD
    static_assert(kTypeAddressRelease == 16, "kTypeAddressRelease value is incorrect");
    static_assert(kTypeAddressReleaseReply == 17, "kTypeAddressReleaseReply value is incorrect");
    static_assert(kTypeAddressReply == 18, "kTypeAddressReply value is incorrect");
    static_assert(kTypeAddressSolicit == 19, "kTypeAddressSolicit value is incorrect");
    static_assert(kTypeChildUpdateRequestOfChild == 20, "kTypeChildUpdateRequestOfChild value is incorrect");
    static_assert(kTypeChildUpdateResponseOfChild == 21, "kTypeChildUpdateResponseOfChild value is incorrect");
    static_assert(kTypeChildUpdateResponseOfUnknownChild == 22, "kTypeChildUpdateResponseOfUnknownChild is incorrect");
    static_assert(kTypeLinkAccept == 23, "kTypeLinkAccept value is incorrect");
    static_assert(kTypeLinkAcceptAndRequest == 24, "kTypeLinkAcceptAndRequest value is incorrect");
    static_assert(kTypeLinkReject == 25, "kTypeLinkReject value is incorrect");
    static_assert(kTypeLinkRequest == 26, "kTypeLinkRequest value is incorrect");
    static_assert(kTypeParentRequest == 27, "kTypeParentRequest value is incorrect");
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    static_assert(kTypeTimeSync == 28, "kTypeTimeSync value is incorrect");
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    static_assert(kTypeLinkMetricsManagementRequest == 29, "kTypeLinkMetricsManagementRequest value is incorrect)");
    static_assert(kTypeLinkMetricsManagementResponse == 30, "kTypeLinkMetricsManagementResponse value is incorrect)");
    static_assert(kTypeLinkProbe == 31, "kTypeLinkProbe value is incorrect)");
#endif
#else // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    static_assert(kTypeLinkMetricsManagementRequest == 28, "kTypeLinkMetricsManagementRequest value is incorrect)");
    static_assert(kTypeLinkMetricsManagementResponse == 29, "kTypeLinkMetricsManagementResponse value is incorrect)");
    static_assert(kTypeLinkProbe == 30, "kTypeLinkProbe value is incorrect)");
#endif
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#else  // OPENTHREAD_FTD
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    static_assert(kTypeLinkMetricsManagementRequest == 16, "kTypeLinkMetricsManagementRequest value is incorrect)");
    static_assert(kTypeLinkMetricsManagementResponse == 17, "kTypeLinkMetricsManagementResponse value is incorrect)");
    static_assert(kTypeLinkProbe == 18, "kTypeLinkProbe value is incorrect)");
#endif
#endif // OPENTHREAD_FTD

    return kMessageTypeStrings[aType];
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

#endif // #if OT_SHOULD_LOG_AT( OT_LOG_LEVEL_WARN)

const char *Mle::RoleToString(DeviceRole aRole)
{
    static const char *const kRoleStrings[] = {
        "disabled", // (0) kRoleDisabled
        "detached", // (1) kRoleDetached
        "child",    // (2) kRoleChild
        "router",   // (3) kRoleRouter
        "leader",   // (4) kRoleLeader
    };

    static_assert(kRoleDisabled == 0, "kRoleDisabled value is incorrect");
    static_assert(kRoleDetached == 1, "kRoleDetached value is incorrect");
    static_assert(kRoleChild == 2, "kRoleChild value is incorrect");
    static_assert(kRoleRouter == 3, "kRoleRouter value is incorrect");
    static_assert(kRoleLeader == 4, "kRoleLeader value is incorrect");

    return (aRole < GetArrayLength(kRoleStrings)) ? kRoleStrings[aRole] : "invalid";
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

const char *Mle::AttachModeToString(AttachMode aMode)
{
    static const char *const kAttachModeStrings[] = {
        "AnyPartition",       // (0) kAnyPartition
        "SamePartition",      // (1) kSamePartition
        "SamePartitionRetry", // (2) kSamePartitionRetry
        "BetterPartition",    // (3) kBetterPartition
        "DowngradeToReed",    // (4) kDowngradeToReed
        "BetterParent",       // (5) kBetterParent
    };

    static_assert(kAnyPartition == 0, "kAnyPartition value is incorrect");
    static_assert(kSamePartition == 1, "kSamePartition value is incorrect");
    static_assert(kSamePartitionRetry == 2, "kSamePartitionRetry value is incorrect");
    static_assert(kBetterPartition == 3, "kBetterPartition value is incorrect");
    static_assert(kDowngradeToReed == 4, "kDowngradeToReed value is incorrect");
    static_assert(kBetterParent == 5, "kBetterParent value is incorrect");

    return kAttachModeStrings[aMode];
}

const char *Mle::AttachStateToString(AttachState aState)
{
    static const char *const kAttachStateStrings[] = {
        "Idle",             // (0) kAttachStateIdle
        "ProcessAnnounce",  // (1) kAttachStateProcessAnnounce
        "Start",            // (2) kAttachStateStart
        "ParentReqRouters", // (3) kAttachStateParentRequestRouter
        "ParentReqReeds",   // (4) kAttachStateParentRequestReed
        "Announce",         // (5) kAttachStateAnnounce
        "ChildIdReq",       // (6) kAttachStateChildIdRequest
    };

    static_assert(kAttachStateIdle == 0, "kAttachStateIdle value is incorrect");
    static_assert(kAttachStateProcessAnnounce == 1, "kAttachStateProcessAnnounce value is incorrect");
    static_assert(kAttachStateStart == 2, "kAttachStateStart value is incorrect");
    static_assert(kAttachStateParentRequestRouter == 3, "kAttachStateParentRequestRouter value is incorrect");
    static_assert(kAttachStateParentRequestReed == 4, "kAttachStateParentRequestReed value is incorrect");
    static_assert(kAttachStateAnnounce == 5, "kAttachStateAnnounce value is incorrect");
    static_assert(kAttachStateChildIdRequest == 6, "kAttachStateChildIdRequest value is incorrect");

    return kAttachStateStrings[aState];
}

const char *Mle::ReattachStateToString(ReattachState aState)
{
    static const char *const kReattachStateStrings[] = {
        "",                                 // (0) kReattachStop
        "reattaching",                      // (1) kReattachStart
        "reattaching with Active Dataset",  // (2) kReattachActive
        "reattaching with Pending Dataset", // (3) kReattachPending
    };

    static_assert(kReattachStop == 0, "kReattachStop value is incorrect");
    static_assert(kReattachStart == 1, "kReattachStart value is incorrect");
    static_assert(kReattachActive == 2, "kReattachActive value is incorrect");
    static_assert(kReattachPending == 3, "kReattachPending value is incorrect");

    return kReattachStateStrings[aState];
}

#endif // OT_SHOULD_LOG_AT( OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
Error Mle::SendLinkMetricsManagementRequest(const Ip6::Address &aDestination, const uint8_t *aSubTlvs, uint8_t aLength)
{
    Error      error = kErrorNone;
    TxMessage *message;
    Tlv        tlv;

    VerifyOrExit((message = NewMleMessage(kCommandLinkMetricsManagementRequest)) != nullptr, error = kErrorNoBufs);

    // Link Metrics Management TLV
    tlv.SetType(Tlv::kLinkMetricsManagement);
    tlv.SetLength(aLength);

    SuccessOrExit(error = message->AppendBytes(&tlv, sizeof(tlv)));
    SuccessOrExit(error = message->AppendBytes(aSubTlvs, aLength));

    SuccessOrExit(error = message->SendTo(aDestination));

exit:
    FreeMessageOnError(message, error);
    return error;
}
#endif

void Mle::RegisterParentResponseStatsCallback(otThreadParentResponseCallback aCallback, void *aContext)
{
    mParentResponseCb        = aCallback;
    mParentResponseCbContext = aContext;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
uint64_t Mle::CalcParentCslMetric(uint8_t aCslClockAccuracy, uint8_t aCslUncertainty)
{
    /*
     * This function calculates the overall time that device will operate on battery
     * by summming sequence of "ON quants" over a period of time.
     */
    const uint64_t usInSecond   = 1000000;
    uint64_t       cslPeriodUs  = kMinCslPeriod * kUsPerTenSymbols;
    uint64_t       cslTimeoutUs = GetCslTimeout() * usInSecond;
    uint64_t       k            = cslTimeoutUs / cslPeriodUs;

    return k * (k + 1) * cslPeriodUs / usInSecond * aCslClockAccuracy + aCslUncertainty * k * kUsPerUncertUnit;
}
#endif

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
    IgnoreError(aMessage.Read(length - sizeof(*this), *this));
}

void Mle::DelayedResponseMetadata::RemoveFrom(Message &aMessage) const
{
    SuccessOrAssert(aMessage.SetLength(aMessage.GetLength() - sizeof(*this)));
}

} // namespace Mle
} // namespace ot
