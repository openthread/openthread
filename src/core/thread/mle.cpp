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

#include "instance/instance.hpp"
#include "utils/static_counter.hpp"

namespace ot {
namespace Mle {

RegisterLogModule("Mle");

const otMeshLocalPrefix Mle::kMeshLocalPrefixInit = {
    {0xfd, 0xde, 0xad, 0x00, 0xbe, 0xef, 0x00, 0x00},
};

Mle::Mle(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRetrieveNewNetworkData(false)
    , mRequestRouteTlv(false)
    , mHasRestored(false)
    , mReceivedResponseFromParent(false)
    , mDetachingGracefully(false)
    , mInitiallyAttachedAsSleepy(false)
    , mRole(kRoleDisabled)
    , mLastSavedRole(kRoleDisabled)
    , mDeviceMode(DeviceMode::kModeRxOnWhenIdle)
    , mAttachState(kAttachStateIdle)
    , mReattachState(kReattachStop)
    , mAttachMode(kAnyPartition)
    , mDataRequestState(kDataRequestNone)
    , mAddressRegistrationMode(kAppendAllAddresses)
    , mChildUpdateRequestState(kChildUpdateRequestNone)
    , mParentRequestCounter(0)
    , mChildUpdateAttempts(0)
    , mDataRequestAttempts(0)
    , mAnnounceChannel(0)
    , mAlternateChannel(0)
    , mRloc16(kInvalidRloc16)
    , mPreviousParentRloc(kInvalidRloc16)
    , mAttachCounter(0)
    , mAnnounceDelay(kAnnounceTimeout)
    , mAlternatePanId(Mac::kPanIdBroadcast)
    , mStoreFrameCounterAhead(kDefaultStoreFrameCounterAhead)
    , mTimeout(kDefaultChildTimeout)
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    , mCslTimeout(kDefaultCslTimeout)
#endif
    , mAlternateTimestamp(0)
    , mNeighborTable(aInstance)
    , mDelayedSender(aInstance)
    , mSocket(aInstance, *this)
#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    , mParentSearch(aInstance)
#endif
    , mAttachTimer(aInstance)
    , mMessageTransmissionTimer(aInstance)
{
    mParent.Init(aInstance);
    mParentCandidate.Init(aInstance);

    mLeaderData.Clear();
    mParent.Clear();
    mParentCandidate.Clear();
    ResetCounters();

    mLinkLocalAddress.InitAsThreadOrigin();
    mLinkLocalAddress.GetAddress().SetToLinkLocalAddress(Get<Mac::Mac>().GetExtAddress());

    mMeshLocalEid.InitAsThreadOriginMeshLocal();
    mMeshLocalEid.GetAddress().GetIid().GenerateRandom();

    mMeshLocalRloc.InitAsThreadOriginMeshLocal();
    mMeshLocalRloc.GetAddress().GetIid().SetToLocator(0);
    mMeshLocalRloc.mRloc = true;

    mLinkLocalAllThreadNodes.Clear();
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[0] = BigEndian::HostSwap16(0xff32);
    mLinkLocalAllThreadNodes.GetAddress().mFields.m16[7] = BigEndian::HostSwap16(0x0001);

    mRealmLocalAllThreadNodes.Clear();
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[0] = BigEndian::HostSwap16(0xff33);
    mRealmLocalAllThreadNodes.GetAddress().mFields.m16[7] = BigEndian::HostSwap16(0x0001);

    mMeshLocalPrefix.Clear();
    SetMeshLocalPrefix(AsCoreType(&kMeshLocalPrefixInit));
}

Error Mle::Enable(void)
{
    Error error = kErrorNone;

    UpdateLinkLocalAddress();
    SuccessOrExit(error = mSocket.Open());
    SuccessOrExit(error = mSocket.Bind(kUdpPort));

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    mParentSearch.StartTimer();
#endif
exit:
    return error;
}

void Mle::ScheduleChildUpdateRequest(void)
{
    if (mChildUpdateRequestState != kChildUpdateRequestPending)
    {
        mChildUpdateRequestState = kChildUpdateRequestPending;
        ScheduleMessageTransmissionTimer();
    }
}

Error Mle::Disable(void)
{
    Error error = kErrorNone;

    Stop(kKeepNetworkDatasets);
    SuccessOrExit(error = mSocket.Close());
    Get<ThreadNetif>().RemoveUnicastAddress(mLinkLocalAddress);

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

    Get<ThreadNetif>().AddUnicastAddress(mMeshLocalEid);

    Get<ThreadNetif>().SubscribeMulticast(mLinkLocalAllThreadNodes);
    Get<ThreadNetif>().SubscribeMulticast(mRealmLocalAllThreadNodes);

    SetRloc16(GetRloc16());

    mAttachCounter = 0;

    Get<KeyManager>().Start();

    if (aMode == kNormalAttach)
    {
        mReattachState = kReattachStart;
    }

    if ((aMode == kAnnounceAttach) || (GetRloc16() == kInvalidRloc16))
    {
        Attach(kAnyPartition);
    }
#if OPENTHREAD_FTD
    else if (IsRouterRloc16(GetRloc16()))
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
        IgnoreError(Get<MeshCoP::ActiveDatasetManager>().Restore());
        IgnoreError(Get<MeshCoP::PendingDatasetManager>().Restore());
    }

    VerifyOrExit(!IsDisabled());

    Get<KeyManager>().Stop();
    SetStateDetached();
    Get<ThreadNetif>().UnsubscribeMulticast(mRealmLocalAllThreadNodes);
    Get<ThreadNetif>().UnsubscribeMulticast(mLinkLocalAllThreadNodes);
    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocalRloc);
    Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocalEid);

#if OPENTHREAD_FTD
    Get<MleRouter>().mRouterRoleRestorer.Stop();
#endif

    SetRole(kRoleDisabled);

exit:
    if (mDetachingGracefully)
    {
        mDetachingGracefully = false;
        mDetachGracefullyCallback.InvokeAndClearIfSet();
    }
}

void Mle::ResetCounters(void)
{
    ClearAllBytes(mCounters);
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    mLastUpdatedTimestamp = Get<Uptime>().GetUptime();
#endif
}

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
void Mle::UpdateRoleTimeCounters(DeviceRole aRole)
{
    uint64_t currentUptimeMsec = Get<Uptime>().GetUptime();
    uint64_t durationMsec      = currentUptimeMsec - mLastUpdatedTimestamp;

    mLastUpdatedTimestamp = currentUptimeMsec;

    mCounters.mTrackedTime += durationMsec;

    switch (aRole)
    {
    case kRoleDisabled:
        mCounters.mDisabledTime += durationMsec;
        break;
    case kRoleDetached:
        mCounters.mDetachedTime += durationMsec;
        break;
    case kRoleChild:
        mCounters.mChildTime += durationMsec;
        break;
    case kRoleRouter:
        mCounters.mRouterTime += durationMsec;
        break;
    case kRoleLeader:
        mCounters.mLeaderTime += durationMsec;
        break;
    }
}
#endif

void Mle::SetRole(DeviceRole aRole)
{
    DeviceRole oldRole = mRole;

    SuccessOrExit(Get<Notifier>().Update(mRole, aRole, kEventThreadRoleChanged));

    LogNote("Role %s -> %s", RoleToString(oldRole), RoleToString(mRole));

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    UpdateRoleTimeCounters(oldRole);
#endif

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

    if ((oldRole == kRoleDetached) && IsChild())
    {
        // On transition from detached to child, we remember whether we
        // attached as sleepy or not. This is then used to determine
        // whether or not we need to re-attach on mode changes between
        // rx-on and sleepy (rx-off). If we initially attach as sleepy,
        // then rx-on/off mode changes are allowed without re-attach.

        mInitiallyAttachedAsSleepy = !GetDeviceMode().IsRxOnWhenIdle();
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

    Get<KeyManager>().SetCurrentKeySequence(networkInfo.GetKeySequence(),
                                            KeyManager::kForceUpdate | KeyManager::kGuardTimerUnchanged);
    Get<KeyManager>().SetMleFrameCounter(networkInfo.GetMleFrameCounter());
    Get<KeyManager>().SetAllMacFrameCounters(networkInfo.GetMacFrameCounter(), /* aSetIfLarger */ false);

#if OPENTHREAD_MTD
    mDeviceMode.Set(networkInfo.GetDeviceMode() & ~DeviceMode::kModeFullThreadDevice);
#else
    mDeviceMode.Set(networkInfo.GetDeviceMode());
#endif

    // force re-attach when version mismatch.
    VerifyOrExit(networkInfo.GetVersion() == kThreadVersion);

    mLastSavedRole = static_cast<DeviceRole>(networkInfo.GetRole());

    switch (mLastSavedRole)
    {
    case kRoleChild:
    case kRoleRouter:
    case kRoleLeader:
        break;

    default:
        ExitNow();
    }

#if OPENTHREAD_MTD
    if (IsChildRloc16(networkInfo.GetRloc16()))
#endif
    {
        Get<Mac::Mac>().SetShortAddress(networkInfo.GetRloc16());
        mRloc16 = networkInfo.GetRloc16();
    }
    Get<Mac::Mac>().SetExtAddress(networkInfo.GetExtAddress());

    mMeshLocalEid.GetAddress().SetIid(networkInfo.GetMeshLocalIid());

    if (networkInfo.GetRloc16() == kInvalidRloc16)
    {
        ExitNow();
    }

    if (IsChildRloc16(networkInfo.GetRloc16()))
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
        mParent.SetVersion(parentInfo.GetVersion());
        mParent.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                         DeviceMode::kModeFullNetworkData));
        mParent.SetRloc16(ParentRloc16ForRloc16(networkInfo.GetRloc16()));
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

    // Successfully restored the network information from
    // non-volatile settings after boot.
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
        networkInfo.SetMeshLocalIid(mMeshLocalEid.GetAddress().GetIid());
        networkInfo.SetVersion(kThreadVersion);
        mLastSavedRole = mRole;

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
    networkInfo.SetMleFrameCounter(Get<KeyManager>().GetMleFrameCounter() + mStoreFrameCounterAhead);
    networkInfo.SetMacFrameCounter(Get<KeyManager>().GetMaximumMacFrameCounter() + mStoreFrameCounterAhead);
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

    if (IsDetached() && (mAttachState == kAttachStateStart))
    {
        // Already detached and waiting to start an attach attempt, so
        // there is not need to make any changes.
        ExitNow();
    }

    // Not in reattach stage after reset
    if (mReattachState == kReattachStop)
    {
        IgnoreError(Get<MeshCoP::PendingDatasetManager>().Restore());
    }

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    mParentSearch.SetRecentlyDetached();
#endif

    SetStateDetached();
    mParent.SetState(Neighbor::kStateInvalid);
    SetRloc16(kInvalidRloc16);
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

Error Mle::SearchForBetterParent(void)
{
    Error error = kErrorNone;

    VerifyOrExit(IsChild(), error = kErrorInvalidState);
    Attach(kBetterParent);

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

        if ((counter < BitSizeOf(ratio)) && ((1UL << counter) <= ratio))
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

    LogNote("Attach attempt %u unsuccessful, will try again in %lu.%03u seconds", mAttachCounter, ToUlong(delay / 1000),
            static_cast<uint16_t>(delay % 1000));

exit:
    return delay;
}

bool Mle::IsAttached(void) const { return (IsChild() || IsRouter() || IsLeader()); }

bool Mle::IsRouterOrLeader(void) const { return (IsRouter() || IsLeader()); }

void Mle::SetStateDetached(void)
{
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().Reset();
#endif
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    Get<BackboneRouter::Leader>().Reset();
#endif

#if OPENTHREAD_FTD
    if (IsLeader())
    {
        Get<ThreadNetif>().RemoveUnicastAddress(Get<MleRouter>().mLeaderAloc);
    }
#endif

    SetRole(kRoleDetached);
    SetAttachState(kAttachStateIdle);
    mAttachTimer.Stop();
    mMessageTransmissionTimer.Stop();
    mChildUpdateRequestState   = kChildUpdateRequestNone;
    mChildUpdateAttempts       = 0;
    mDataRequestState          = kDataRequestNone;
    mDataRequestAttempts       = 0;
    mInitiallyAttachedAsSleepy = false;
    Get<MeshForwarder>().SetRxOnWhenIdle(true);
    Get<Mac::Mac>().SetBeaconEnabled(false);
#if OPENTHREAD_FTD
    Get<MleRouter>().HandleDetachStart();
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    Get<Mac::Mac>().UpdateCsl();
#endif
}

void Mle::SetStateChild(uint16_t aRloc16)
{
#if OPENTHREAD_FTD
    if (IsLeader())
    {
        Get<ThreadNetif>().RemoveUnicastAddress(Get<MleRouter>().mLeaderAloc);
    }
#endif

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

    // send announce after attached if needed
    InformPreviousChannel();

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
    mParentSearch.UpdateState();
#endif

    if ((mPreviousParentRloc != kInvalidRloc16) && (mPreviousParentRloc != mParent.GetRloc16()))
    {
        mCounters.mParentChanges++;

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
        InformPreviousParent();
#endif
    }

    mPreviousParentRloc = mParent.GetRloc16();

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    Get<Mac::Mac>().UpdateCsl();
#endif
}

void Mle::InformPreviousChannel(void)
{
    VerifyOrExit(mAlternatePanId != Mac::kPanIdBroadcast);
    VerifyOrExit(IsChild() || IsRouter());

#if OPENTHREAD_FTD
    VerifyOrExit(!IsFullThreadDevice() || IsRouter() || !Get<MleRouter>().IsRouterRoleTransitionPending());
#endif

    mAlternatePanId = Mac::kPanIdBroadcast;
    Get<AnnounceBeginServer>().SendAnnounce(1 << mAlternateChannel);

exit:
    return;
}

void Mle::SetTimeout(uint32_t aTimeout)
{
    // Determine `kMinTimeout` based on other parameters
    static constexpr uint32_t kMinPollPeriod       = OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD;
    static constexpr uint32_t kRetxPollPeriod      = OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD;
    static constexpr uint32_t kMinTimeoutDataPoll  = kMinPollPeriod + kFailedChildTransmissions * kRetxPollPeriod;
    static constexpr uint32_t kMinTimeoutKeepAlive = (kMaxChildKeepAliveAttempts + 1) * kUnicastRetxDelay;
    static constexpr uint32_t kMinTimeout          = Time::MsecToSec(OT_MAX(kMinTimeoutKeepAlive, kMinTimeoutDataPoll));

    aTimeout = Max(aTimeout, kMinTimeout);

    VerifyOrExit(mTimeout != aTimeout);

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

    if (IsAttached())
    {
        bool shouldReattach = false;

        // We need to re-attach when switching between MTD/FTD modes.

        if (oldMode.IsFullThreadDevice() != mDeviceMode.IsFullThreadDevice())
        {
            shouldReattach = true;
        }

        // If we initially attached as sleepy we allow mode changes
        // between rx-on/off without a re-attach (we send "Child Update
        // Request" to update the parent). But if we initially attached
        // as rx-on, we require a re-attach on switching from rx-on to
        // sleepy (rx-off) mode.

        if (!mInitiallyAttachedAsSleepy && oldMode.IsRxOnWhenIdle() && !mDeviceMode.IsRxOnWhenIdle())
        {
            shouldReattach = true;
        }

        if (shouldReattach)
        {
            mAttachCounter = 0;
            IgnoreError(BecomeDetached());
            ExitNow();
        }
    }

    if (IsDetached())
    {
        mAttachCounter = 0;
        SetStateDetached();
        Attach(kAnyPartition);
    }
    else if (IsChild())
    {
        SetStateChild(GetRloc16());
        IgnoreError(SendChildUpdateRequest());
    }

exit:
    return error;
}

void Mle::UpdateLinkLocalAddress(void)
{
    Get<ThreadNetif>().RemoveUnicastAddress(mLinkLocalAddress);
    mLinkLocalAddress.GetAddress().GetIid().SetFromExtAddress(Get<Mac::Mac>().GetExtAddress());
    Get<ThreadNetif>().AddUnicastAddress(mLinkLocalAddress);

    Get<Notifier>().Signal(kEventThreadLinkLocalAddrChanged);
}

void Mle::SetMeshLocalPrefix(const Ip6::NetworkPrefix &aMeshLocalPrefix)
{
    VerifyOrExit(mMeshLocalPrefix != aMeshLocalPrefix);

    mMeshLocalPrefix = aMeshLocalPrefix;

    // We ask `ThreadNetif` to apply the new mesh-local prefix which
    // will then update all of its assigned unicast addresses that are
    // marked as mesh-local, as well as all of the subscribed mesh-local
    // prefix-based multicast addresses (such as link-local or
    // realm-local All Thread Nodes addresses). It is important to call
    // `ApplyNewMeshLocalPrefix()` first so that `ThreadNetif` can
    // correctly signal the updates. It will first signal the removal
    // of the previous address based on the old prefix, and then the
    // addition of the new address with the new mesh-local prefix.

    Get<ThreadNetif>().ApplyNewMeshLocalPrefix();

    // Some of the addresses may already be updated from the
    // `ApplyNewMeshLocalPrefix()` call, but we apply the new prefix to
    // them in case they are not yet added to the `Netif`. This ensures
    // that addresses are always updated and other modules can retrieve
    // them using methods such as `GetMeshLocalRloc()`, `GetMeshLocalEid()`
    // or `GetLinkLocalAllThreadNodesAddress()`, even if they have not
    // yet been added to the `Netif`.

    mMeshLocalEid.GetAddress().SetPrefix(mMeshLocalPrefix);
    mMeshLocalRloc.GetAddress().SetPrefix(mMeshLocalPrefix);
    mLinkLocalAllThreadNodes.GetAddress().SetMulticastNetworkPrefix(mMeshLocalPrefix);
    mRealmLocalAllThreadNodes.GetAddress().SetMulticastNetworkPrefix(mMeshLocalPrefix);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().ApplyNewMeshLocalPrefix();
#endif

    Get<Notifier>().Signal(kEventThreadMeshLocalAddrChanged);

exit:
    return;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
Error Mle::SetMeshLocalIid(const Ip6::InterfaceIdentifier &aMlIid)
{
    Error error = kErrorNone;

    VerifyOrExit(!Get<ThreadNetif>().HasUnicastAddress(mMeshLocalEid), error = kErrorInvalidState);

    mMeshLocalEid.GetAddress().SetIid(aMlIid);
exit:
    return error;
}
#endif

void Mle::SetRloc16(uint16_t aRloc16)
{
    uint16_t oldRloc16 = GetRloc16();

    if (aRloc16 != oldRloc16)
    {
        LogNote("RLOC16 %04x -> %04x", oldRloc16, aRloc16);
    }

    if (Get<ThreadNetif>().HasUnicastAddress(mMeshLocalRloc) &&
        (mMeshLocalRloc.GetAddress().GetIid().GetLocator() != aRloc16))
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mMeshLocalRloc);
        Get<Tmf::Agent>().ClearRequests(mMeshLocalRloc.GetAddress());
    }

    Get<Mac::Mac>().SetShortAddress(aRloc16);
    mRloc16 = aRloc16;

    if (aRloc16 != kInvalidRloc16)
    {
        // We can always call `AddUnicastAddress(mMeshLocat16)` and if
        // the address is already added, it will perform no action.

        mMeshLocalRloc.GetAddress().GetIid().SetLocator(aRloc16);
        Get<ThreadNetif>().AddUnicastAddress(mMeshLocalRloc);
#if OPENTHREAD_FTD
        Get<AddressResolver>().RestartAddressQueries();
#endif
    }
}

void Mle::SetLeaderData(const LeaderData &aLeaderData)
{
    SetLeaderData(aLeaderData.GetPartitionId(), aLeaderData.GetWeighting(), aLeaderData.GetLeaderRouterId());
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

void Mle::GetLeaderRloc(Ip6::Address &aAddress) const
{
    aAddress.SetToRoutingLocator(mMeshLocalPrefix, GetLeaderRloc16());
}

void Mle::GetLeaderAloc(Ip6::Address &aAddress) const { aAddress.SetToAnycastLocator(mMeshLocalPrefix, kAloc16Leader); }

void Mle::GetCommissionerAloc(uint16_t aSessionId, Ip6::Address &aAddress) const
{
    aAddress.SetToAnycastLocator(mMeshLocalPrefix, CommissionerAloc16FromId(aSessionId));
}

void Mle::GetServiceAloc(uint8_t aServiceId, Ip6::Address &aAddress) const
{
    aAddress.SetToAnycastLocator(mMeshLocalPrefix, ServiceAlocFromId(aServiceId));
}

const LeaderData &Mle::GetLeaderData(void)
{
    mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet));
    mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kStableSubset));

    return mLeaderData;
}

bool Mle::HasUnregisteredAddress(void)
{
    bool retval = false;

    // Checks whether there are any addresses in addition to the mesh-local
    // address that need to be registered.

    for (const Ip6::Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (!addr.GetAddress().IsLinkLocalUnicast() && !IsRoutingLocator(addr.GetAddress()) &&
            !IsAnycastLocator(addr.GetAddress()) && addr.GetAddress() != GetMeshLocalEid())
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

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
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
#endif

void Mle::InitNeighbor(Neighbor &aNeighbor, const RxInfo &aRxInfo)
{
    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(aNeighbor.GetExtAddress());
    aNeighbor.GetLinkInfo().Clear();
    aNeighbor.GetLinkInfo().AddRss(aRxInfo.mMessage.GetAverageRss());
    aNeighbor.ResetLinkFailures();
    aNeighbor.SetLastHeard(TimerMilli::GetNow());
}

void Mle::ScheduleChildUpdateRequestIfMtdChild(void)
{
    if (IsChild() && !IsFullThreadDevice())
    {
        ScheduleChildUpdateRequest();
    }
}

void Mle::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(!IsDisabled());

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        if (mAddressRegistrationMode == kAppendMeshLocalOnly)
        {
            // If only mesh-local address was registered in the "Child
            // ID Request" message, after device is attached, trigger a
            // "Child Update Request" to register the remaining
            // addresses.

            mAddressRegistrationMode = kAppendAllAddresses;
            ScheduleChildUpdateRequestIfMtdChild();
        }
    }

    if (aEvents.ContainsAny(kEventIp6AddressAdded | kEventIp6AddressRemoved))
    {
        if (!Get<ThreadNetif>().HasUnicastAddress(mMeshLocalEid.GetAddress()))
        {
            mMeshLocalEid.GetAddress().GetIid().GenerateRandom();

            Get<ThreadNetif>().AddUnicastAddress(mMeshLocalEid);
            Get<Notifier>().Signal(kEventThreadMeshLocalAddrChanged);
        }

        ScheduleChildUpdateRequestIfMtdChild();
    }

    if (aEvents.ContainsAny(kEventIp6MulticastSubscribed | kEventIp6MulticastUnsubscribed))
    {
        // When multicast subscription changes, SED always notifies
        // its parent as it depends on its parent for indirect
        // transmission. Since Thread 1.2, MED MAY also notify its
        // parent of 1.2 or higher version as it could depend on its
        // parent to perform Multicast Listener Report.

        if (!IsRxOnWhenIdle()
#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
            || !GetParent().IsThreadVersion1p1()
#endif
        )

        {
            ScheduleChildUpdateRequestIfMtdChild();
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
                ScheduleChildUpdateRequest();
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

#if OPENTHREAD_FTD
    if (aEvents.Contains(kEventSecurityPolicyChanged))
    {
        Get<MleRouter>().HandleSecurityPolicyChanged();
    }
#endif

    if (aEvents.Contains(kEventSupportedChannelMaskChanged))
    {
        Mac::ChannelMask channelMask = Get<Mac::Mac>().GetSupportedChannelMask();

        if (!channelMask.ContainsChannel(Get<Mac::Mac>().GetPanChannel()) && (mRole != kRoleDisabled))
        {
            LogWarn("Channel %u is not in the supported channel mask %s, detach the network gracefully!",
                    Get<Mac::Mac>().GetPanChannel(), channelMask.ToString().AsCString());
            IgnoreError(DetachGracefully(nullptr, nullptr));
        }
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

Mle::ServiceAloc::ServiceAloc(void)
{
    InitAsThreadOriginMeshLocal();
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

Error Mle::DetermineParentRequestType(ParentRequestType &aType) const
{
    // This method determines the Parent Request type to use during an
    // attach cycle based on `mAttachMode`, `mAttachCounter` and
    // `mParentRequestCounter`. This method MUST be used while in
    // `kAttachStateParentRequest` state.
    //
    // On success it returns `kErrorNone` and sets `aType`. It returns
    // `kErrorNotFound` to indicate that device can now transition
    // from `kAttachStateParentRequest` state (has already sent the
    // required number of Parent Requests for this attach attempt
    // cycle).

    Error error = kErrorNone;

    OT_ASSERT(mAttachState == kAttachStateParentRequest);

    aType = kToRoutersAndReeds;

    // If device is not yet attached, `mAttachCounter` will track the
    // number of attach attempt cycles so far, starting from one for
    // the first attempt. `mAttachCounter` will be zero if device is
    // already attached. Examples of this situation include a leader or
    // router trying to attach to a better partition, or a child trying
    // to find a better parent.

    if ((mAttachCounter <= 1) && (mAttachMode != kBetterParent))
    {
        VerifyOrExit(mParentRequestCounter <= kFirstAttachCycleTotalParentRequests, error = kErrorNotFound);

        // During reattach to the same partition all the Parent
        // Request are sent to Routers and REEDs.

        if ((mAttachMode != kSamePartition) && (mParentRequestCounter <= kFirstAttachCycleNumParentRequestToRouters))
        {
            aType = kToRouters;
        }
    }
    else
    {
        VerifyOrExit(mParentRequestCounter <= kNextAttachCycleTotalParentRequests, error = kErrorNotFound);

        if (mParentRequestCounter <= kNextAttachCycleNumParentRequestToRouters)
        {
            aType = kToRouters;
        }
    }

exit:
    return error;
}

bool Mle::HasAcceptableParentCandidate(void) const
{
    bool              hasAcceptableParent = false;
    ParentRequestType parentReqType;

    VerifyOrExit(mParentCandidate.IsStateParentResponse());

    switch (mAttachState)
    {
    case kAttachStateAnnounce:
        VerifyOrExit(!HasMoreChannelsToAnnounce());
        break;

    case kAttachStateParentRequest:
        SuccessOrAssert(DetermineParentRequestType(parentReqType));

        if (parentReqType == kToRouters)
        {
            // If we cannot find a parent with best link quality (3) when
            // in Parent Request was sent to routers, we will keep the
            // candidate and forward to REED stage to potentially find a
            // better parent.
            VerifyOrExit(mParentCandidate.GetTwoWayLinkQuality() == kLinkQuality3);
        }

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
    uint32_t          delay          = 0;
    bool              shouldAnnounce = true;
    ParentRequestType type;

    if (mDetachingGracefully)
    {
        Stop();
        ExitNow();
    }

#if OPENTHREAD_FTD
    if (IsDetached() && Get<MleRouter>().mRouterRoleRestorer.IsActive())
    {
        Get<MleRouter>().mRouterRoleRestorer.HandleTimer();
        ExitNow();
    }
#endif

    // First, check if we are waiting to receive parent responses and
    // found an acceptable parent candidate.

    if (HasAcceptableParentCandidate() && (SendChildIdRequest() == kErrorNone))
    {
        SetAttachState(kAttachStateChildIdRequest);
        delay = kChildIdResponseTimeout;
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
        LogNote("Attach attempt %d, %s %s", mAttachCounter, AttachModeToString(mAttachMode),
                ReattachStateToString(mReattachState));

        SetAttachState(kAttachStateParentRequest);
        mParentCandidate.SetState(Neighbor::kStateInvalid);
        mReceivedResponseFromParent = false;
        mParentRequestCounter       = 0;
        Get<MeshForwarder>().SetRxOnWhenIdle(true);

        OT_FALL_THROUGH;

    case kAttachStateParentRequest:
        mParentRequestCounter++;
        if (DetermineParentRequestType(type) == kErrorNone)
        {
            SendParentRequest(type);
            delay = (type == kToRouters) ? kParentRequestRouterTimeout : kParentRequestReedTimeout;
            break;
        }

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
        if (shouldAnnounce && (GetNextAnnounceChannel(mAnnounceChannel) == kErrorNone))
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
    mAnnounceDelay = Max(mAnnounceDelay, kMinAnnounceDelay);
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
            else if (IsFullThreadDevice() && Get<MleRouter>().BecomeLeader(/* aCheckWeight */ false) == kErrorNone)
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
            // Return to sleepy operation
            Get<DataPollSender>().SetAttachMode(false);
            Get<MeshForwarder>().SetRxOnWhenIdle(false);
        }

        break;

    case kSamePartition:
    case kDowngradeToReed:
        Attach(kAnyPartition);
        break;

    case kBetterPartition:
        break;
    }

exit:
    return delay;
}

void Mle::SendParentRequest(ParentRequestType aType)
{
    Error        error = kErrorNone;
    TxMessage   *message;
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

void Mle::HandleChildIdRequestTxDone(Message &aMessage)
{
    if (aMessage.GetTxSuccess() && !IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SetAttachMode(true);
        Get<MeshForwarder>().SetRxOnWhenIdle(false);
    }

    if (aMessage.IsLinkSecurityEnabled())
    {
        // If the Child ID Request requires fragmentation and therefore
        // link layer security, the frame transmission will be aborted.
        // When the message is being freed, we signal to MLE to prepare a
        // shorter Child ID Request message (by only including mesh-local
        // address in the Address Registration TLV).

        LogInfo("Requesting shorter `Child ID Request`");
        RequestShorterChildIdRequest();
    }
}

Error Mle::SendChildIdRequest(void)
{
    static const uint8_t kTlvs[] = {Tlv::kAddress16, Tlv::kNetworkData, Tlv::kRoute};

    Error        error   = kErrorNone;
    uint8_t      tlvsLen = sizeof(kTlvs);
    TxMessage   *message = nullptr;
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
            // Parent state is not normally invalidated after becoming
            // a Router/Leader (see #1875).  When trying to attach to
            // a better partition, invalidating old parent state
            // (especially when in `kStateRestored`) ensures that
            // `FindNeighbor()` returns `mParentCandidate` when
            // processing the Child ID Response.

            mParent.SetState(Neighbor::kStateInvalid);
        }
    }

    VerifyOrExit((message = NewMleMessage(kCommandChildIdRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendResponseTlv(mParentCandidate.mRxChallenge));
    SuccessOrExit(error = message->AppendLinkAndMleFrameCounterTlvs());
    SuccessOrExit(error = message->AppendModeTlv(mDeviceMode));
    SuccessOrExit(error = message->AppendTimeoutTlv(mTimeout));
    SuccessOrExit(error = message->AppendVersionTlv());
    SuccessOrExit(error = message->AppendSupervisionIntervalTlvIfSleepyChild());

    if (!IsFullThreadDevice())
    {
        SuccessOrExit(error = message->AppendAddressRegistrationTlv(mAddressRegistrationMode));

        // No need to request the last Route64 TLV for MTD
        tlvsLen -= 1;
    }

    SuccessOrExit(error = message->AppendTlvRequestTlv(kTlvs, tlvsLen));
    SuccessOrExit(error = message->AppendActiveAndPendingTimestampTlvs());

    mParentCandidate.SetState(Neighbor::kStateValid);

    destination.SetToLinkLocalAddress(mParentCandidate.GetExtAddress());
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend,
        (mAddressRegistrationMode == kAppendMeshLocalOnly) ? kTypeChildIdRequestShort : kTypeChildIdRequest,
        destination);
exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Mle::SendDataRequest(const Ip6::Address &aDestination)
{
    return SendDataRequestAfterDelay(aDestination, /* aDelay */ 0);
}

Error Mle::SendDataRequestAfterDelay(const Ip6::Address &aDestination, uint16_t aDelay)
{
    static const uint8_t kTlvs[] = {Tlv::kNetworkData, Tlv::kRoute};

    Error error;

    mDelayedSender.RemoveDataRequestMessage(aDestination);

    // Based on `mRequestRouteTlv` include both Network Data and Route
    // TLVs or only Network Data TLV.

    error = SendDataRequest(aDestination, kTlvs, mRequestRouteTlv ? 2 : 1, aDelay);

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

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
Error Mle::SendDataRequestForLinkMetricsReport(const Ip6::Address                      &aDestination,
                                               const LinkMetrics::Initiator::QueryInfo &aQueryInfo)
{
    static const uint8_t kTlvs[] = {Tlv::kLinkMetricsReport};

    return SendDataRequest(aDestination, kTlvs, sizeof(kTlvs), /* aDelay */ 0, &aQueryInfo);
}

Error Mle::SendDataRequest(const Ip6::Address                      &aDestination,
                           const uint8_t                           *aTlvs,
                           uint8_t                                  aTlvsLength,
                           uint16_t                                 aDelay,
                           const LinkMetrics::Initiator::QueryInfo *aQueryInfo)
#else
Error Mle::SendDataRequest(const Ip6::Address &aDestination, const uint8_t *aTlvs, uint8_t aTlvsLength, uint16_t aDelay)
#endif
{
    Error      error = kErrorNone;
    TxMessage *message;

    VerifyOrExit((message = NewMleMessage(kCommandDataRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendTlvRequestTlv(aTlvs, aTlvsLength));

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    if (aQueryInfo != nullptr)
    {
        SuccessOrExit(error = Get<LinkMetrics::Initiator>().AppendLinkMetricsQueryTlv(*message, *aQueryInfo));
    }
#endif

    if (aDelay)
    {
        SuccessOrExit(error = message->SendAfterDelay(aDestination, aDelay));
        Log(kMessageDelay, kTypeDataRequest, aDestination);
    }
    else
    {
        SuccessOrExit(error = message->AppendActiveAndPendingTimestampTlvs());

        SuccessOrExit(error = message->SendTo(aDestination));
        Log(kMessageSend, kTypeDataRequest, aDestination);

        if (!IsRxOnWhenIdle())
        {
            Get<DataPollSender>().SendFastPolls(DataPollSender::kDefaultFastPolls);
        }
    }

exit:
    FreeMessageOnError(message, error);
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
        if (Get<Mac::Mac>().IsCslEnabled())
        {
            ExitNow(interval = Get<Mac::Mac>().GetCslPeriodInMsec() + kUnicastRetxDelay);
        }
        else
#endif
        {
            ExitNow(interval = kUnicastRetxDelay);
        }
    }

    switch (mDataRequestState)
    {
    case kDataRequestNone:
        break;

    case kDataRequestActive:
        ExitNow(interval = kUnicastRetxDelay);
    }

    if (IsChild() && IsRxOnWhenIdle())
    {
        interval = Time::SecToMsec(mTimeout) - kUnicastRetxDelay * kMaxChildKeepAliveAttempts;
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
            Ip6::Address destination;

            VerifyOrExit(mDataRequestAttempts < kMaxChildKeepAliveAttempts, IgnoreError(BecomeDetached()));

            destination.SetToLinkLocalAddress(mParent.GetExtAddress());

            if (SendDataRequest(destination) == kErrorNone)
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
            // Add another delay to ensures the Child Update Request is sent
            // only after all pending changes are incorporated.
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

Error Mle::SendChildUpdateRequest(void) { return SendChildUpdateRequest(kNormalChildUpdateRequest); }

Error Mle::SendChildUpdateRequest(ChildUpdateRequestMode aMode)
{
    Error                   error = kErrorNone;
    Ip6::Address            destination;
    TxMessage              *message     = nullptr;
    AddressRegistrationMode addrRegMode = kAppendAllAddresses;

    if (!mParent.IsStateValidOrRestoring())
    {
        LogWarn("No valid parent when sending Child Update Request");
        IgnoreError(BecomeDetached());
        ExitNow();
    }

    if (aMode != kAppendZeroTimeout)
    {
        // Enable MLE retransmissions on all Child Update Request
        // messages, except when actively detaching.
        mChildUpdateRequestState = kChildUpdateRequestActive;
        ScheduleMessageTransmissionTimer();
    }

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendModeTlv(mDeviceMode));

    if ((aMode == kAppendChallengeTlv) || IsDetached())
    {
        mParentRequestChallenge.GenerateRandom();
        SuccessOrExit(error = message->AppendChallengeTlv(mParentRequestChallenge));
    }

    switch (mRole)
    {
    case kRoleDetached:
        addrRegMode = kAppendMeshLocalOnly;
        break;

    case kRoleChild:
        SuccessOrExit(error = message->AppendSourceAddressTlv());
        SuccessOrExit(error = message->AppendLeaderDataTlv());
        SuccessOrExit(error = message->AppendTimeoutTlv((aMode == kAppendZeroTimeout) ? 0 : mTimeout));
        SuccessOrExit(error = message->AppendSupervisionIntervalTlvIfSleepyChild());
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
    }

    if (!IsFullThreadDevice())
    {
        SuccessOrExit(error = message->AppendAddressRegistrationTlv(addrRegMode));
    }

    destination.SetToLinkLocalAddress(mParent.GetExtAddress());
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeChildUpdateRequestAsChild, destination);

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

Error Mle::SendChildUpdateResponse(const TlvList      &aTlvList,
                                   const RxChallenge  &aChallenge,
                                   const Ip6::Address &aDestination)
{
    Error      error = kErrorNone;
    TxMessage *message;
    bool       checkAddress = false;

    VerifyOrExit((message = NewMleMessage(kCommandChildUpdateResponse)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendSourceAddressTlv());
    SuccessOrExit(error = message->AppendLeaderDataTlv());

    for (uint8_t tlvType : aTlvList)
    {
        switch (tlvType)
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

        case Tlv::kSupervisionInterval:
            SuccessOrExit(error = message->AppendSupervisionIntervalTlvIfSleepyChild());
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

    SuccessOrExit(error = message->SendTo(aDestination));

    Log(kMessageSend, kTypeChildUpdateResponseAsChild, aDestination);

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
    MeshCoP::Timestamp activeTimestamp;
    TxMessage         *message = nullptr;

    VerifyOrExit(Get<Mac::Mac>().GetSupportedChannelMask().ContainsChannel(aChannel), error = kErrorInvalidArgs);
    VerifyOrExit((message = NewMleMessage(kCommandAnnounce)) != nullptr, error = kErrorNoBufs);
    message->SetLinkSecurityEnabled(true);
    message->SetChannel(aChannel);

    SuccessOrExit(error = Tlv::Append<ChannelTlv>(*message, ChannelTlvValue(Get<Mac::Mac>().GetPanChannel())));

    switch (aMode)
    {
    case kOrphanAnnounce:
        activeTimestamp.SetToOrphanAnnounce();
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

Error Mle::GetNextAnnounceChannel(uint8_t &aChannel) const
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

bool Mle::HasMoreChannelsToAnnounce(void) const
{
    uint8_t channel = mAnnounceChannel;

    return GetNextAnnounceChannel(channel) == kErrorNone;
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
                                  Message                &aMessage,
                                  const Ip6::MessageInfo &aMessageInfo,
                                  uint16_t                aCmdOffset,
                                  const SecurityHeader   &aHeader)
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
    Crypto::AesCcm::GenerateNonce(extAddress, aHeader.GetFrameCounter(), Mac::Frame::kSecurityEncMic32, nonce);

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
        aMessage.RemoveFooter(kMleSecurityTagSize);
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
        aMessage.RemoveFooter(kMleSecurityTagSize);
    }

exit:
    return error;
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
    Neighbor       *neighbor;
#if OPENTHREAD_FTD
    bool isNeighborRxOnly = false;
#endif

    LogDebg("Receive MLE message");

    VerifyOrExit(aMessage.GetOrigin() == Message::kOriginThreadNetif);
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

    IgnoreError(aMessage.Read(aMessage.GetOffset(), command));
    aMessage.MoveOffset(sizeof(command));

    aMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddr);
    neighbor = (command == kCommandChildIdResponse) ? mNeighborTable.FindParent(extAddr)
                                                    : mNeighborTable.FindNeighbor(extAddr);

#if OPENTHREAD_FTD
    if (neighbor == nullptr)
    {
        // As an FTD child, we may have rx-only neighbors. We find and set
        // `neighbor` to perform security processing (frame counter
        // and key sequence checks) for messages from such neighbors.

        neighbor         = mNeighborTable.FindRxOnlyNeighborRouter(extAddr);
        isNeighborRxOnly = true;
    }
#endif

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

#if OPENTHREAD_FTD
    if (isNeighborRxOnly)
    {
        // Clear the `neighbor` if it is a rx-only one before calling
        // `Handle{Msg}()`, except for a subset of MLE messages such
        // as MLE Advertisement. This ensures that, as an FTD child,
        // we are selective about which messages to process from
        // rx-only neighbors.

        switch (command)
        {
        case kCommandAdvertisement:
        case kCommandLinkRequest:
        case kCommandLinkAccept:
        case kCommandLinkAcceptAndRequest:
            break;

        default:
            neighbor = nullptr;
            break;
        }
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
        HandleChildUpdateRequest(rxInfo);
        break;

    case kCommandChildUpdateResponse:
        HandleChildUpdateResponse(rxInfo);
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
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    case kCommandTimeSync:
        HandleTimeSync(rxInfo);
        break;
#endif

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

    ProcessKeySequence(rxInfo);

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
    if (!aMessageInfo.GetSockAddr().IsMulticast() || !aMessage.IsDstPanIdBroadcast())
    {
        LogProcessError(kTypeGenericUdp, error);
    }
}

void Mle::ProcessKeySequence(RxInfo &aRxInfo)
{
    // In case key sequence is larger, we determine whether to adopt it
    // or not. The `Handle{MleMsg}()` methods set the `rxInfo.mClass`
    // based on the message command type and the included TLVs. If
    // there is any error during parsing of the message the `mClass`
    // remains as its default value of `RxInfo::kUnknown`. Message
    // classes are determined based on this:
    //
    // Authoritative : Larger key seq MUST be adopted.
    // Peer          : If from a known neighbor
    //                    If difference is 1, adopt
    //                    Otherwise don't adopt and try to re-sync with
    //                    neighbor.
    //                 Otherwise larger key seq MUST NOT be adopted.

    bool                          isNextKeySeq;
    KeyManager::KeySeqUpdateFlags flags = 0;

    VerifyOrExit(aRxInfo.mKeySequence > Get<KeyManager>().GetCurrentKeySequence());

    isNextKeySeq = (aRxInfo.mKeySequence - Get<KeyManager>().GetCurrentKeySequence() == 1);

    switch (aRxInfo.mClass)
    {
    case RxInfo::kAuthoritativeMessage:
        flags = KeyManager::kForceUpdate;
        break;

    case RxInfo::kPeerMessage:
        VerifyOrExit(aRxInfo.IsNeighborStateValid());

        if (!isNextKeySeq)
        {
            LogInfo("Large key seq jump in peer class msg from 0x%04x ", aRxInfo.mNeighbor->GetRloc16());
            ReestablishLinkWithNeighbor(*aRxInfo.mNeighbor);
            ExitNow();
        }

        flags = KeyManager::kApplySwitchGuard;
        break;

    case RxInfo::kUnknown:
        ExitNow();
    }

    if (isNextKeySeq)
    {
        flags |= KeyManager::kResetGuardTimer;
    }

    Get<KeyManager>().SetCurrentKeySequence(aRxInfo.mKeySequence, flags);

exit:
    return;
}

void Mle::ReestablishLinkWithNeighbor(Neighbor &aNeighbor)
{
    VerifyOrExit(IsAttached() && aNeighbor.IsStateValid());

    if (IsChild() && (&aNeighbor == &mParent))
    {
        IgnoreError(SendChildUpdateRequest(kAppendChallengeTlv));
        ExitNow();
    }

#if OPENTHREAD_FTD
    VerifyOrExit(IsFullThreadDevice());

    if (IsRouterRloc16(aNeighbor.GetRloc16()))
    {
        IgnoreError(Get<MleRouter>().SendLinkRequest(&aNeighbor));
    }
    else if (Get<ChildTable>().Contains(aNeighbor))
    {
        Child &child = static_cast<Child &>(aNeighbor);

        child.SetState(Child::kStateChildUpdateRequest);
        IgnoreError(Get<MleRouter>().SendChildUpdateRequest(child));
    }
#endif

exit:
    return;
}

void Mle::HandleAdvertisement(RxInfo &aRxInfo)
{
    Error      error = kErrorNone;
    uint16_t   sourceAddress;
    LeaderData leaderData;
    uint16_t   delay;

    VerifyOrExit(IsAttached());

    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeAdvertisement, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

#if OPENTHREAD_FTD
    if (IsFullThreadDevice())
    {
        SuccessOrExit(error = Get<MleRouter>().HandleAdvertisementOnFtd(aRxInfo, sourceAddress, leaderData));
    }
#endif

    if (IsChild())
    {
        VerifyOrExit(aRxInfo.mNeighbor == &mParent);

        if (mParent.GetRloc16() != sourceAddress)
        {
            // Remove stale parent.
            IgnoreError(BecomeDetached());
            ExitNow();
        }

        if ((leaderData.GetPartitionId() != mLeaderData.GetPartitionId()) ||
            (leaderData.GetLeaderRouterId() != GetLeaderId()))
        {
            SetLeaderData(leaderData);

#if OPENTHREAD_FTD
            SuccessOrExit(error = Get<MleRouter>().ReadAndProcessRouteTlvOnFtdChild(aRxInfo, mParent.GetRouterId()));
#endif

            mRetrieveNewNetworkData = true;
        }

        mParent.SetLastHeard(TimerMilli::GetNow());
    }
    else // Device is router or leader
    {
        VerifyOrExit(aRxInfo.IsNeighborStateValid());
    }

    if (mRetrieveNewNetworkData || IsNetworkDataNewer(leaderData))
    {
        delay = Random::NonCrypto::GetUint16InRange(0, kMleMaxResponseDelay);
        IgnoreError(SendDataRequestAfterDelay(aRxInfo.mMessageInfo.GetPeerAddr(), delay));
    }

    aRxInfo.mClass = RxInfo::kPeerMessage;

exit:
    LogProcessError(kTypeAdvertisement, error);
}

void Mle::HandleDataResponse(RxInfo &aRxInfo)
{
    Error error;

    Log(kMessageReceive, kTypeDataResponse, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.IsNeighborStateValid(), error = kErrorDrop);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    {
        OffsetRange offsetRange;

        if (Tlv::FindTlvValueOffsetRange(aRxInfo.mMessage, Tlv::kLinkMetricsReport, offsetRange) == kErrorNone)
        {
            Get<LinkMetrics::Initiator>().HandleReport(aRxInfo.mMessage, offsetRange,
                                                       aRxInfo.mMessageInfo.GetPeerAddr());
        }
    }
#endif

#if OPENTHREAD_FTD
    SuccessOrExit(error = Get<MleRouter>().ReadAndProcessRouteTlvOnFtdChild(aRxInfo, mParent.GetRouterId()));
#endif

    error = HandleLeaderData(aRxInfo);

    if (mDataRequestState == kDataRequestNone && !IsRxOnWhenIdle())
    {
        // Stop fast data poll request by MLE since we received
        // the response.
        Get<DataPollSender>().StopFastPolls();
    }

    SuccessOrExit(error);
    aRxInfo.mClass = RxInfo::kPeerMessage;

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
    Error              error = kErrorNone;
    LeaderData         leaderData;
    MeshCoP::Timestamp activeTimestamp;
    MeshCoP::Timestamp pendingTimestamp;
    bool               saveActiveDataset  = false;
    bool               savePendingDataset = false;
    bool               dataRequest        = false;

    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

    if ((leaderData.GetPartitionId() != mLeaderData.GetPartitionId()) ||
        (leaderData.GetWeighting() != mLeaderData.GetWeighting()) || (leaderData.GetLeaderRouterId() != GetLeaderId()))
    {
        if (IsChild())
        {
            SetLeaderData(leaderData);
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

    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, activeTimestamp))
    {
    case kErrorNone:
#if OPENTHREAD_FTD
        if (IsLeader())
        {
            break;
        }
#endif
        if (activeTimestamp != Get<MeshCoP::ActiveDatasetManager>().GetTimestamp())
        {
            // Send an MLE Data Request if the received timestamp
            // mismatches the local value and the message does not
            // include the dataset.

            VerifyOrExit(aRxInfo.mMessage.ContainsTlv(Tlv::kActiveDataset), dataRequest = true);
            saveActiveDataset = true;
        }

        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, pendingTimestamp))
    {
    case kErrorNone:
#if OPENTHREAD_FTD
        if (IsLeader())
        {
            break;
        }
#endif
        if (pendingTimestamp != Get<MeshCoP::PendingDatasetManager>().GetTimestamp())
        {
            VerifyOrExit(aRxInfo.mMessage.ContainsTlv(Tlv::kPendingDataset), dataRequest = true);
            savePendingDataset = true;
        }

        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    switch (error = aRxInfo.mMessage.ReadAndSetNetworkDataTlv(leaderData))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        dataRequest = true;
        OT_FALL_THROUGH;
    default:
        ExitNow();
    }

#if OPENTHREAD_FTD
    if (IsLeader())
    {
        Get<NetworkData::Leader>().IncrementVersionAndStableVersion();
    }
    else
#endif
    {
        // We previously confirmed the message contains an
        // Active or a Pending Dataset TLV before setting the
        // corresponding `saveDataset` flag.

        if (saveActiveDataset)
        {
            IgnoreError(aRxInfo.mMessage.ReadAndSaveActiveDataset(activeTimestamp));
        }

        if (savePendingDataset)
        {
            IgnoreError(aRxInfo.mMessage.ReadAndSavePendingDataset(pendingTimestamp));
        }
    }

    mRetrieveNewNetworkData = false;

exit:

    if (dataRequest)
    {
        uint16_t delay;

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

        IgnoreError(SendDataRequestAfterDelay(aRxInfo.mMessageInfo.GetPeerAddr(), delay));
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

bool Mle::IsBetterParent(uint16_t                aRloc16,
                         uint8_t                 aTwoWayLinkMargin,
                         const ConnectivityTlv  &aConnectivityTlv,
                         uint16_t                aVersion,
                         const Mac::CslAccuracy &aCslAccuracy)
{
    int rval;

    // Mesh Impacting Criteria
    rval = ThreeWayCompare(LinkQualityForLinkMargin(aTwoWayLinkMargin), mParentCandidate.GetTwoWayLinkQuality());
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(IsRouterRloc16(aRloc16), IsRouterRloc16(mParentCandidate.GetRloc16()));
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aConnectivityTlv.GetParentPriority(), mParentCandidate.mPriority);
    VerifyOrExit(rval == 0);

    // Prefer the parent with highest quality links (Link Quality 3 field in Connectivity TLV) to neighbors
    rval = ThreeWayCompare(aConnectivityTlv.GetLinkQuality3(), mParentCandidate.mLinkQuality3);
    VerifyOrExit(rval == 0);

    // Thread 1.2 Specification 4.5.2.1.2 Child Impacting Criteria

    rval = ThreeWayCompare(aVersion, mParentCandidate.GetVersion());
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aConnectivityTlv.GetSedBufferSize(), mParentCandidate.mSedBufferSize);
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aConnectivityTlv.GetSedDatagramCount(), mParentCandidate.mSedDatagramCount);
    VerifyOrExit(rval == 0);

    // Extra rules
    rval = ThreeWayCompare(aConnectivityTlv.GetLinkQuality2(), mParentCandidate.mLinkQuality2);
    VerifyOrExit(rval == 0);

    rval = ThreeWayCompare(aConnectivityTlv.GetLinkQuality1(), mParentCandidate.mLinkQuality1);
    VerifyOrExit(rval == 0);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    // CSL metric
    if (!IsRxOnWhenIdle())
    {
        uint64_t cslMetric          = CalcParentCslMetric(aCslAccuracy);
        uint64_t candidateCslMetric = CalcParentCslMetric(mParentCandidate.GetCslAccuracy());

        // Smaller metric is better.
        rval = ThreeWayCompare(candidateCslMetric, cslMetric);
        VerifyOrExit(rval == 0);
    }
#else
    OT_UNUSED_VARIABLE(aCslAccuracy);
#endif

    rval = ThreeWayCompare(aTwoWayLinkMargin, mParentCandidate.mLinkMargin);

exit:
    return (rval > 0);
}

void Mle::HandleParentResponse(RxInfo &aRxInfo)
{
    Error            error = kErrorNone;
    int8_t           rss   = aRxInfo.mMessage.GetAverageRss();
    uint16_t         version;
    uint16_t         sourceAddress;
    LeaderData       leaderData;
    uint8_t          linkMarginOut;
    uint8_t          twoWayLinkMargin;
    ConnectivityTlv  connectivityTlv;
    uint32_t         linkFrameCounter;
    uint32_t         mleFrameCounter;
    Mac::ExtAddress  extAddress;
    Mac::CslAccuracy cslAccuracy;
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    TimeParameterTlv timeParameterTlv;
#endif

    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeParentResponse, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

    SuccessOrExit(error = aRxInfo.mMessage.ReadAndMatchResponseTlvWith(mParentRequestChallenge));

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddress);

    if (IsChild() && mParent.GetExtAddress() == extAddress)
    {
        mReceivedResponseFromParent = true;
    }

    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

    SuccessOrExit(error = Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMarginOut));
    twoWayLinkMargin = Min(Get<Mac::Mac>().ComputeLinkMargin(rss), linkMarginOut);

    SuccessOrExit(error = Tlv::FindTlv(aRxInfo.mMessage, connectivityTlv));
    VerifyOrExit(connectivityTlv.IsValid(), error = kErrorParse);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    switch (aRxInfo.mMessage.ReadCslClockAccuracyTlv(cslAccuracy))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        cslAccuracy.Init(); // Use worst-case values if TLV is not found
        break;
    default:
        ExitNow(error = kErrorParse);
    }
#else
    cslAccuracy.Init();
#endif

#if OPENTHREAD_CONFIG_MLE_PARENT_RESPONSE_CALLBACK_API_ENABLE
    if (mParentResponseCallback.IsSet())
    {
        otThreadParentResponseInfo parentinfo;

        parentinfo.mExtAddr      = extAddress;
        parentinfo.mRloc16       = sourceAddress;
        parentinfo.mRssi         = rss;
        parentinfo.mPriority     = connectivityTlv.GetParentPriority();
        parentinfo.mLinkQuality3 = connectivityTlv.GetLinkQuality3();
        parentinfo.mLinkQuality2 = connectivityTlv.GetLinkQuality2();
        parentinfo.mLinkQuality1 = connectivityTlv.GetLinkQuality1();
        parentinfo.mIsAttached   = IsAttached();

        mParentResponseCallback.Invoke(&parentinfo);
    }
#endif

    aRxInfo.mClass = RxInfo::kAuthoritativeMessage;

#if OPENTHREAD_FTD
    if (IsFullThreadDevice() && !IsDetached())
    {
        bool isPartitionIdSame = (leaderData.GetPartitionId() == mLeaderData.GetPartitionId());
        bool isIdSequenceSame  = (connectivityTlv.GetIdSequence() == Get<RouterTable>().GetRouterIdSequence());
        bool isIdSequenceGreater =
            SerialNumber::IsGreater(connectivityTlv.GetIdSequence(), Get<RouterTable>().GetRouterIdSequence());

        switch (mAttachMode)
        {
        case kAnyPartition:
        case kBetterParent:
            VerifyOrExit(!isPartitionIdSame || isIdSequenceGreater);
            break;

        case kSamePartition:
            VerifyOrExit(isPartitionIdSame && isIdSequenceGreater);
            break;

        case kDowngradeToReed:
            VerifyOrExit(isPartitionIdSame && (isIdSequenceSame || isIdSequenceGreater));
            break;

        case kBetterPartition:
            VerifyOrExit(!isPartitionIdSame);

            VerifyOrExit(MleRouter::ComparePartitions(connectivityTlv.IsSingleton(), leaderData,
                                                      Get<MleRouter>().IsSingleton(), mLeaderData) > 0);
            break;
        }
    }
#endif

    // Continue to process the "ParentResponse" if it is from current
    // parent candidate to update the challenge and frame counters.

    if (mParentCandidate.IsStateParentResponse() && (mParentCandidate.GetExtAddress() != extAddress))
    {
        // If already have a candidate parent, only seek a better parent

        int compare = 0;

#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            compare = MleRouter::ComparePartitions(connectivityTlv.IsSingleton(), leaderData,
                                                   mParentCandidate.mIsSingleton, mParentCandidate.mLeaderData);
        }

        // Only consider partitions that are the same or better
        VerifyOrExit(compare >= 0);
#endif

        // Only consider better parents if the partitions are the same
        if (compare == 0)
        {
            VerifyOrExit(IsBetterParent(sourceAddress, twoWayLinkMargin, connectivityTlv, version, cslAccuracy));
        }
    }

    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    if (Tlv::FindTlv(aRxInfo.mMessage, timeParameterTlv) == kErrorNone)
    {
        VerifyOrExit(timeParameterTlv.IsValid());

        Get<TimeSync>().SetTimeSyncPeriod(timeParameterTlv.GetTimeSyncPeriod());
        Get<TimeSync>().SetXtalThreshold(timeParameterTlv.GetXtalThreshold());
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED
    else
    {
        // If the time sync feature is required, don't choose the
        // parent which doesn't support it.
        ExitNow();
    }
#endif
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(mParentCandidate.mRxChallenge));

    InitNeighbor(mParentCandidate, aRxInfo);
    mParentCandidate.SetRloc16(sourceAddress);
    mParentCandidate.GetLinkFrameCounters().SetAll(linkFrameCounter);
    mParentCandidate.SetLinkAckFrameCounter(linkFrameCounter);
    mParentCandidate.SetMleFrameCounter(mleFrameCounter);
    mParentCandidate.SetVersion(version);
    mParentCandidate.SetDeviceMode(DeviceMode(DeviceMode::kModeFullThreadDevice | DeviceMode::kModeRxOnWhenIdle |
                                              DeviceMode::kModeFullNetworkData));
    mParentCandidate.SetLinkQualityOut(LinkQualityForLinkMargin(linkMarginOut));
    mParentCandidate.SetState(Neighbor::kStateParentResponse);
    mParentCandidate.SetKeySequence(aRxInfo.mKeySequence);
    mParentCandidate.SetLeaderCost(connectivityTlv.GetLeaderCost());
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    mParentCandidate.SetCslAccuracy(cslAccuracy);
#endif

    mParentCandidate.mPriority         = connectivityTlv.GetParentPriority();
    mParentCandidate.mLinkQuality3     = connectivityTlv.GetLinkQuality3();
    mParentCandidate.mLinkQuality2     = connectivityTlv.GetLinkQuality2();
    mParentCandidate.mLinkQuality1     = connectivityTlv.GetLinkQuality1();
    mParentCandidate.mSedBufferSize    = connectivityTlv.GetSedBufferSize();
    mParentCandidate.mSedDatagramCount = connectivityTlv.GetSedDatagramCount();
    mParentCandidate.mLeaderData       = leaderData;
    mParentCandidate.mIsSingleton      = connectivityTlv.IsSingleton();
    mParentCandidate.mLinkMargin       = twoWayLinkMargin;

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

    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeChildIdResponse, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    VerifyOrExit(aRxInfo.IsNeighborStateValid(), error = kErrorSecurity);

    VerifyOrExit(mAttachState == kAttachStateChildIdRequest);

    SuccessOrExit(error = Tlv::Find<Address16Tlv>(aRxInfo.mMessage, shortAddress));
    VerifyOrExit(RouterIdMatch(sourceAddress, shortAddress), error = kErrorRejected);

    SuccessOrExit(error = aRxInfo.mMessage.ReadLeaderDataTlv(leaderData));

    VerifyOrExit(aRxInfo.mMessage.ContainsTlv(Tlv::kNetworkData));

    switch (Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        error = aRxInfo.mMessage.ReadAndSaveActiveDataset(timestamp);
        error = (error == kErrorNotFound) ? kErrorNone : error;
        SuccessOrExit(error);
        break;

    case kErrorNotFound:
        break;

    default:
        ExitNow(error = kErrorParse);
    }

    // Clear Pending Dataset if device succeed to reattach using stored Pending Dataset
    if (mReattachState == kReattachPending)
    {
        Get<MeshCoP::PendingDatasetManager>().Clear();
    }

    switch (Tlv::Find<PendingTimestampTlv>(aRxInfo.mMessage, timestamp))
    {
    case kErrorNone:
        IgnoreError(aRxInfo.mMessage.ReadAndSavePendingDataset(timestamp));
        break;

    case kErrorNotFound:
        Get<MeshCoP::PendingDatasetManager>().Clear();
        break;

    default:
        ExitNow(error = kErrorParse);
    }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aRxInfo.mMessage.GetTimeSyncSeq() != OT_TIME_SYNC_INVALID_SEQ)
    {
        Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);
    }
#endif

    // Parent Attach Success

    SetStateDetached();

    SetLeaderData(leaderData);

#if OPENTHREAD_FTD
    SuccessOrExit(error =
                      Get<MleRouter>().ReadAndProcessRouteTlvOnFtdChild(aRxInfo, RouterIdFromRloc16(sourceAddress)));
#endif

    mParentCandidate.CopyTo(mParent);
    mParentCandidate.Clear();

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    Get<Mac::Mac>().SetCslParentAccuracy(mParent.GetCslAccuracy());
#endif

    mParent.SetRloc16(sourceAddress);

    IgnoreError(aRxInfo.mMessage.ReadAndSetNetworkDataTlv(leaderData));

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

    aRxInfo.mClass = RxInfo::kPeerMessage;

exit:
    LogProcessError(kTypeChildIdResponse, error);
}

void Mle::HandleChildUpdateRequest(RxInfo &aRxInfo)
{
#if OPENTHREAD_FTD
    if (IsRouterOrLeader())
    {
        Get<MleRouter>().HandleChildUpdateRequestOnParent(aRxInfo);
    }
    else
#endif
    {
        HandleChildUpdateRequestOnChild(aRxInfo);
    }
}

void Mle::HandleChildUpdateRequestOnChild(RxInfo &aRxInfo)
{
    Error       error = kErrorNone;
    uint16_t    sourceAddress;
    RxChallenge challenge;
    TlvList     requestedTlvList;
    TlvList     tlvList;
    uint8_t     linkMarginOut;

    SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

    Log(kMessageReceive, kTypeChildUpdateRequestAsChild, aRxInfo.mMessageInfo.GetPeerAddr(), sourceAddress);

    switch (aRxInfo.mMessage.ReadChallengeTlv(challenge))
    {
    case kErrorNone:
        tlvList.Add(Tlv::kResponse);
        tlvList.Add(Tlv::kMleFrameCounter);
        tlvList.Add(Tlv::kLinkFrameCounter);
        break;
    case kErrorNotFound:
        challenge.Clear();
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

        SuccessOrExit(error = HandleLeaderData(aRxInfo));

        switch (Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMarginOut))
        {
        case kErrorNone:
            mParent.SetLinkQualityOut(LinkQualityForLinkMargin(linkMarginOut));
            break;
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        {
            Mac::CslAccuracy cslAccuracy;

            if (aRxInfo.mMessage.ReadCslClockAccuracyTlv(cslAccuracy) == kErrorNone)
            {
                // MUST include CSL timeout TLV when request includes
                // CSL accuracy
                tlvList.Add(Tlv::kCslTimeout);
            }
        }
#endif
    }
    else
    {
        // This device is not a child of the Child Update Request source
        tlvList.Add(Tlv::kStatus);
    }

    switch (aRxInfo.mMessage.ReadTlvRequestTlv(requestedTlvList))
    {
    case kErrorNone:
        tlvList.AddElementsFrom(requestedTlvList);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    aRxInfo.mClass = RxInfo::kPeerMessage;
    ProcessKeySequence(aRxInfo);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    if ((aRxInfo.mNeighbor != nullptr) && !challenge.IsEmpty())
    {
        aRxInfo.mNeighbor->ClearLastRxFragmentTag();
    }
#endif

    // Send the response to the requester, regardless if it's this
    // device's parent or not.
    SuccessOrExit(error = SendChildUpdateResponse(tlvList, challenge, aRxInfo.mMessageInfo.GetPeerAddr()));

exit:
    LogProcessError(kTypeChildUpdateRequestAsChild, error);
}

void Mle::HandleChildUpdateResponse(RxInfo &aRxInfo)
{
#if OPENTHREAD_FTD
    if (IsRouterOrLeader())
    {
        Get<MleRouter>().HandleChildUpdateResponseOnParent(aRxInfo);
    }
    else
#endif
    {
        HandleChildUpdateResponseOnChild(aRxInfo);
    }
}

void Mle::HandleChildUpdateResponseOnChild(RxInfo &aRxInfo)
{
    Error       error = kErrorNone;
    uint8_t     status;
    DeviceMode  mode;
    RxChallenge response;
    uint32_t    linkFrameCounter;
    uint32_t    mleFrameCounter;
    uint16_t    sourceAddress;
    uint32_t    timeout;
    uint8_t     linkMarginOut;

    Log(kMessageReceive, kTypeChildUpdateResponseAsChild, aRxInfo.mMessageInfo.GetPeerAddr());

    switch (aRxInfo.mMessage.ReadResponseTlv(response))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        response.Clear();
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    switch (mRole)
    {
    case kRoleDetached:
        VerifyOrExit(response == mParentRequestChallenge, error = kErrorSecurity);
        break;

    case kRoleChild:
        VerifyOrExit((aRxInfo.mNeighbor == &mParent) && mParent.IsStateValid(), error = kErrorSecurity);
        break;

    default:
        OT_ASSERT(false);
    }

    if (Tlv::Find<StatusTlv>(aRxInfo.mMessage, status) == kErrorNone)
    {
        IgnoreError(BecomeDetached());
        ExitNow();
    }

    SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));
    VerifyOrExit(mode == mDeviceMode, error = kErrorDrop);

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

#if OPENTHREAD_FTD
        if (IsFullThreadDevice())
        {
            mRequestRouteTlv = true;
        }
#endif

        OT_FALL_THROUGH;

    case kRoleChild:
        SuccessOrExit(error = Tlv::Find<SourceAddressTlv>(aRxInfo.mMessage, sourceAddress));

        if (!HasMatchingRouterIdWith(sourceAddress))
        {
            IgnoreError(BecomeDetached());
            ExitNow();
        }

        SuccessOrExit(error = HandleLeaderData(aRxInfo));

        switch (Tlv::Find<TimeoutTlv>(aRxInfo.mMessage, timeout))
        {
        case kErrorNone:
            if (timeout == 0 && mDetachingGracefully)
            {
                Stop();
            }
            else
            {
                mTimeout = timeout;
            }
            break;
        case kErrorNotFound:
            break;
        default:
            ExitNow(error = kErrorParse);
        }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        {
            Mac::CslAccuracy cslAccuracy;

            switch (aRxInfo.mMessage.ReadCslClockAccuracyTlv(cslAccuracy))
            {
            case kErrorNone:
                Get<Mac::Mac>().SetCslParentAccuracy(cslAccuracy);
                break;
            case kErrorNotFound:
                break;
            default:
                ExitNow(error = kErrorParse);
            }
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
    }

    switch (Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMarginOut))
    {
    case kErrorNone:
        mParent.SetLinkQualityOut(LinkQualityForLinkMargin(linkMarginOut));
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    aRxInfo.mClass = response.IsEmpty() ? RxInfo::kPeerMessage : RxInfo::kAuthoritativeMessage;

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

    LogProcessError(kTypeChildUpdateResponseAsChild, error);
}

void Mle::HandleAnnounce(RxInfo &aRxInfo)
{
    Error              error = kErrorNone;
    ChannelTlvValue    channelTlvValue;
    MeshCoP::Timestamp timestamp;
    MeshCoP::Timestamp pendingActiveTimestamp;
    uint8_t            channel;
    uint16_t           panId;
    bool               isFromOrphan;
    bool               channelAndPanIdMatch;
    int                timestampCompare;

    Log(kMessageReceive, kTypeAnnounce, aRxInfo.mMessageInfo.GetPeerAddr());

    SuccessOrExit(error = Tlv::Find<ChannelTlv>(aRxInfo.mMessage, channelTlvValue));
    channel = static_cast<uint8_t>(channelTlvValue.GetChannel());

    SuccessOrExit(error = Tlv::Find<ActiveTimestampTlv>(aRxInfo.mMessage, timestamp));
    SuccessOrExit(error = Tlv::Find<PanIdTlv>(aRxInfo.mMessage, panId));

    aRxInfo.mClass = RxInfo::kPeerMessage;

    isFromOrphan         = timestamp.IsOrphanAnnounce();
    timestampCompare     = MeshCoP::Timestamp::Compare(timestamp, Get<MeshCoP::ActiveDatasetManager>().GetTimestamp());
    channelAndPanIdMatch = (channel == Get<Mac::Mac>().GetPanChannel()) && (panId == Get<Mac::Mac>().GetPanId());

    if (isFromOrphan || (timestampCompare < 0))
    {
        if (isFromOrphan)
        {
            VerifyOrExit(!channelAndPanIdMatch);
        }

        SendAnnounce(channel);

#if OPENTHREAD_CONFIG_MLE_SEND_UNICAST_ANNOUNCE_RESPONSE
        SendAnnounce(channel, aRxInfo.mMessageInfo.GetPeerAddr());
#endif
    }
    else if (timestampCompare > 0)
    {
        // No action is required if device is detached, and current
        // channel and pan-id match the values from the received MLE
        // Announce message.

        if (IsDetached())
        {
            VerifyOrExit(!channelAndPanIdMatch);
        }

        if (Get<MeshCoP::PendingDatasetManager>().ReadActiveTimestamp(pendingActiveTimestamp) == kErrorNone)
        {
            // Ignore the Announce and take no action, if a pending
            // dataset exists with an equal or more recent timestamp,
            // and it will be applied soon.

            if (pendingActiveTimestamp >= timestamp)
            {
                uint32_t remainingDelay;

                if ((Get<MeshCoP::PendingDatasetManager>().ReadRemainingDelay(remainingDelay) == kErrorNone) &&
                    (remainingDelay < kAnnounceBackoffForPendingDataset))
                {
                    ExitNow();
                }
            }
        }

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

    VerifyOrExit(aRxInfo.IsNeighborStateValid(), error = kErrorInvalidState);

    SuccessOrExit(
        error = Get<LinkMetrics::Subject>().HandleManagementRequest(aRxInfo.mMessage, *aRxInfo.mNeighbor, status));

    error = SendLinkMetricsManagementResponse(aRxInfo.mMessageInfo.GetPeerAddr(), status);

    aRxInfo.mClass = RxInfo::kPeerMessage;

exit:
    LogProcessError(kTypeLinkMetricsManagementRequest, error);
}
#endif

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
void Mle::HandleTimeSync(RxInfo &aRxInfo)
{
    Log(kMessageReceive, kTypeTimeSync, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.IsNeighborStateValid());

    aRxInfo.mClass = RxInfo::kPeerMessage;

    Get<TimeSync>().HandleTimeSyncMessage(aRxInfo.mMessage);

exit:
    return;
}
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
void Mle::HandleLinkMetricsManagementResponse(RxInfo &aRxInfo)
{
    Error error = kErrorNone;

    Log(kMessageReceive, kTypeLinkMetricsManagementResponse, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.IsNeighborStateValid(), error = kErrorInvalidState);

    error =
        Get<LinkMetrics::Initiator>().HandleManagementResponse(aRxInfo.mMessage, aRxInfo.mMessageInfo.GetPeerAddr());

    aRxInfo.mClass = RxInfo::kPeerMessage;

exit:
    LogProcessError(kTypeLinkMetricsManagementResponse, error);
}
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
void Mle::HandleLinkProbe(RxInfo &aRxInfo)
{
    Error   error = kErrorNone;
    uint8_t seriesId;

    Log(kMessageReceive, kTypeLinkProbe, aRxInfo.mMessageInfo.GetPeerAddr());

    VerifyOrExit(aRxInfo.IsNeighborStateValid(), error = kErrorInvalidState);

    SuccessOrExit(error = Get<LinkMetrics::Subject>().HandleLinkProbe(aRxInfo.mMessage, seriesId));
    aRxInfo.mNeighbor->AggregateLinkMetrics(seriesId, LinkMetrics::SeriesInfo::kSeriesTypeLinkProbe,
                                            aRxInfo.mMessage.GetAverageLqi(), aRxInfo.mMessage.GetAverageRss());

    aRxInfo.mClass = RxInfo::kPeerMessage;

exit:
    LogProcessError(kTypeLinkProbe, error);
}
#endif

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

uint16_t Mle::GetParentRloc16(void) const { return (mParent.IsStateValid() ? mParent.GetRloc16() : kInvalidRloc16); }

Error Mle::GetParentInfo(Router::Info &aParentInfo) const
{
    Error error = kErrorNone;

    // Skip the check for reference device since it needs to get the
    // original parent's info even after role change.

#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    VerifyOrExit(IsChild(), error = kErrorInvalidState);
#endif

    aParentInfo.SetFrom(mParent);
    ExitNow();

exit:
    return error;
}

bool Mle::IsRoutingLocator(const Ip6::Address &aAddress) const
{
    return IsMeshLocalAddress(aAddress) && aAddress.GetIid().IsRoutingLocator();
}

bool Mle::IsAnycastLocator(const Ip6::Address &aAddress) const
{
    return IsMeshLocalAddress(aAddress) && aAddress.GetIid().IsAnycastLocator();
}

bool Mle::IsMeshLocalAddress(const Ip6::Address &aAddress) const { return (aAddress.GetPrefix() == mMeshLocalPrefix); }

#if OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
void Mle::InformPreviousParent(void)
{
    Error            error   = kErrorNone;
    Message         *message = nullptr;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit((message = Get<Ip6::Ip6>().NewMessage(0)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->SetLength(0));

    messageInfo.SetSockAddr(GetMeshLocalEid());
    messageInfo.GetPeerAddr().SetToRoutingLocator(mMeshLocalPrefix, mPreviousParentRloc);

    SuccessOrExit(error = Get<Ip6::Ip6>().SendDatagram(*message, messageInfo, Ip6::kProtoNone));

    LogNote("Sending message to inform previous parent 0x%04x", mPreviousParentRloc);

exit:
    LogWarnOnError(error, "inform previous parent");
    FreeMessageOnError(message, error);
}
#endif // OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH

#if OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
void Mle::ParentSearch::HandleTimer(void)
{
    int8_t parentRss;

    LogInfo("PeriodicParentSearch: %s interval passed", mIsInBackoff ? "Backoff" : "Check");

    if (mBackoffWasCanceled)
    {
        // Backoff can be canceled if the device switches to a new parent.
        // from `UpdateParentSearchState()`. We want to limit this to happen
        // only once within a backoff interval.

        if (TimerMilli::GetNow() - mBackoffCancelTime >= kBackoffInterval)
        {
            mBackoffWasCanceled = false;
            LogInfo("PeriodicParentSearch: Backoff cancellation is allowed on parent switch");
        }
    }

    mIsInBackoff = false;

    VerifyOrExit(Get<Mle>().IsChild());

    parentRss = Get<Mle>().GetParent().GetLinkInfo().GetAverageRss();
    LogInfo("PeriodicParentSearch: Parent RSS %d", parentRss);
    VerifyOrExit(parentRss != Radio::kInvalidRssi);

    if (parentRss < kRssThreshold)
    {
        LogInfo("PeriodicParentSearch: Parent RSS less than %d, searching for new parents", kRssThreshold);
        mIsInBackoff = true;
        Get<Mle>().Attach(kBetterParent);
    }

exit:
    StartTimer();
}

void Mle::ParentSearch::StartTimer(void)
{
    uint32_t interval;

    interval = Random::NonCrypto::GetUint32InRange(0, kJitterInterval);

    if (mIsInBackoff)
    {
        interval += kBackoffInterval;
    }
    else
    {
        interval += kCheckInterval;
    }

    mTimer.Start(interval);

    LogInfo("PeriodicParentSearch: (Re)starting timer for %s interval", mIsInBackoff ? "backoff" : "check");
}

void Mle::ParentSearch::UpdateState(void)
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

    if (mIsInBackoff && !mBackoffWasCanceled && mRecentlyDetached)
    {
        if ((Get<Mle>().mPreviousParentRloc != kInvalidRloc16) &&
            (Get<Mle>().mPreviousParentRloc != Get<Mle>().mParent.GetRloc16()))
        {
            mIsInBackoff        = false;
            mBackoffWasCanceled = true;
            mBackoffCancelTime  = TimerMilli::GetNow();
            LogInfo("PeriodicParentSearch: Canceling backoff on switching to a new parent");
        }
    }

#endif // OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH

    mRecentlyDetached = false;

    if (!mIsInBackoff)
    {
        StartTimer();
    }
}
#endif // OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void Mle::Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress)
{
    Log(aAction, aType, aAddress, kInvalidRloc16);
}

void Mle::Log(MessageAction aAction, MessageType aType, const Ip6::Address &aAddress, uint16_t aRloc)
{
    enum : uint8_t
    {
        kRlocStringSize = 17,
    };

    String<kRlocStringSize> rlocString;

    if (aRloc != kInvalidRloc16)
    {
        rlocString.Append(",0x%04x", aRloc);
    }

    LogInfo("%s %s%s (%s%s)", MessageActionToString(aAction), MessageTypeToString(aType),
            MessageTypeActionToSuffixString(aType, aAction), aAddress.ToString().AsCString(), rlocString.AsCString());
}
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
void Mle::LogProcessError(MessageType aType, Error aError) { LogError(kMessageReceive, aType, aError); }

void Mle::LogSendError(MessageType aType, Error aError) { LogError(kMessageSend, aType, aError); }

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

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kMessageSend);
        ValidateNextEnum(kMessageReceive);
        ValidateNextEnum(kMessageDelay);
        ValidateNextEnum(kMessageRemoveDelayed);
    };

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
        "Child Update Request",  // (5)  kTypeChildUpdateRequestAsChild
        "Child Update Response", // (6)  kTypeChildUpdateResponseAsChild
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
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        "Link Metrics Management Request",  // (28) kTypeLinkMetricsManagementRequest
        "Link Metrics Management Response", // (29) kTypeLinkMetricsManagementResponse
        "Link Probe",                       // (30) kTypeLinkProbe
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        "Time Sync", // (31) kTypeTimeSync
#endif
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kTypeAdvertisement);
        ValidateNextEnum(kTypeAnnounce);
        ValidateNextEnum(kTypeChildIdRequest);
        ValidateNextEnum(kTypeChildIdRequestShort);
        ValidateNextEnum(kTypeChildIdResponse);
        ValidateNextEnum(kTypeChildUpdateRequestAsChild);
        ValidateNextEnum(kTypeChildUpdateResponseAsChild);
        ValidateNextEnum(kTypeDataRequest);
        ValidateNextEnum(kTypeDataResponse);
        ValidateNextEnum(kTypeDiscoveryRequest);
        ValidateNextEnum(kTypeDiscoveryResponse);
        ValidateNextEnum(kTypeGenericDelayed);
        ValidateNextEnum(kTypeGenericUdp);
        ValidateNextEnum(kTypeParentRequestToRouters);
        ValidateNextEnum(kTypeParentRequestToRoutersReeds);
        ValidateNextEnum(kTypeParentResponse);
#if OPENTHREAD_FTD
        ValidateNextEnum(kTypeAddressRelease);
        ValidateNextEnum(kTypeAddressReleaseReply);
        ValidateNextEnum(kTypeAddressReply);
        ValidateNextEnum(kTypeAddressSolicit);
        ValidateNextEnum(kTypeChildUpdateRequestOfChild);
        ValidateNextEnum(kTypeChildUpdateResponseOfChild);
        ValidateNextEnum(kTypeChildUpdateResponseOfUnknownChild);
        ValidateNextEnum(kTypeLinkAccept);
        ValidateNextEnum(kTypeLinkAcceptAndRequest);
        ValidateNextEnum(kTypeLinkReject);
        ValidateNextEnum(kTypeLinkRequest);
        ValidateNextEnum(kTypeParentRequest);
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        ValidateNextEnum(kTypeLinkMetricsManagementRequest);
        ValidateNextEnum(kTypeLinkMetricsManagementResponse);
        ValidateNextEnum(kTypeLinkProbe);
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        ValidateNextEnum(kTypeTimeSync);
#endif
    };

    return kMessageTypeStrings[aType];
}

const char *Mle::MessageTypeActionToSuffixString(MessageType aType, MessageAction aAction)
{
    const char *str = "";

    OT_UNUSED_VARIABLE(aAction); // Not currently used in non-FTD builds

    switch (aType)
    {
    case kTypeChildIdRequestShort:
        str = " - short";
        break;
    case kTypeChildUpdateRequestAsChild:
    case kTypeChildUpdateResponseAsChild:
        str = " as child";
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

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

const char *Mle::AttachModeToString(AttachMode aMode)
{
    static const char *const kAttachModeStrings[] = {
        "AnyPartition",    // (0) kAnyPartition
        "SamePartition",   // (1) kSamePartition
        "BetterPartition", // (2) kBetterPartition
        "DowngradeToReed", // (3) kDowngradeToReed
        "BetterParent",    // (4) kBetterParent
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kAnyPartition);
        ValidateNextEnum(kSamePartition);
        ValidateNextEnum(kBetterPartition);
        ValidateNextEnum(kDowngradeToReed);
        ValidateNextEnum(kBetterParent);
    };

    return kAttachModeStrings[aMode];
}

const char *Mle::AttachStateToString(AttachState aState)
{
    static const char *const kAttachStateStrings[] = {
        "Idle",            // (0) kAttachStateIdle
        "ProcessAnnounce", // (1) kAttachStateProcessAnnounce
        "Start",           // (2) kAttachStateStart
        "ParentReq",       // (3) kAttachStateParent
        "Announce",        // (4) kAttachStateAnnounce
        "ChildIdReq",      // (5) kAttachStateChildIdRequest
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kAttachStateIdle);
        ValidateNextEnum(kAttachStateProcessAnnounce);
        ValidateNextEnum(kAttachStateStart);
        ValidateNextEnum(kAttachStateParentRequest);
        ValidateNextEnum(kAttachStateAnnounce);
        ValidateNextEnum(kAttachStateChildIdRequest);
    };

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

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kReattachStop);
        ValidateNextEnum(kReattachStart);
        ValidateNextEnum(kReattachActive);
        ValidateNextEnum(kReattachPending);
    };

    return kReattachStateStrings[aState];
}

#endif // OT_SHOULD_LOG_AT( OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
Error Mle::SendLinkMetricsManagementRequest(const Ip6::Address &aDestination, const ot::Tlv &aSubTlv)
{
    Error      error   = kErrorNone;
    TxMessage *message = NewMleMessage(kCommandLinkMetricsManagementRequest);
    Tlv        tlv;

    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    tlv.SetType(Tlv::kLinkMetricsManagement);
    tlv.SetLength(static_cast<uint8_t>(aSubTlv.GetSize()));

    SuccessOrExit(error = message->Append(tlv));
    SuccessOrExit(error = aSubTlv.AppendTo(*message));

    error = message->SendTo(aDestination);

exit:
    FreeMessageOnError(message, error);
    return error;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
uint64_t Mle::CalcParentCslMetric(const Mac::CslAccuracy &aCslAccuracy) const
{
    // This function calculates the overall time that device will operate
    // on battery by summing sequence of "ON quants" over a period of time.

    static constexpr uint64_t usInSecond = 1000000;

    uint64_t cslPeriodUs  = kMinCslPeriod * kUsPerTenSymbols;
    uint64_t cslTimeoutUs = GetCslTimeout() * usInSecond;
    uint64_t k            = cslTimeoutUs / cslPeriodUs;

    return k * (k + 1) * cslPeriodUs / usInSecond * aCslAccuracy.GetClockAccuracy() +
           aCslAccuracy.GetUncertaintyInMicrosec() * k;
}
#endif

Error Mle::DetachGracefully(DetachCallback aCallback, void *aContext)
{
    Error    error   = kErrorNone;
    uint32_t timeout = kDetachGracefullyTimeout;

    VerifyOrExit(!mDetachingGracefully, error = kErrorBusy);

    mDetachGracefullyCallback.Set(aCallback, aContext);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    Get<BorderRouter::RoutingManager>().RequestStop();
#endif

    switch (mRole)
    {
    case kRoleLeader:
        break;

    case kRoleRouter:
#if OPENTHREAD_FTD
        Get<MleRouter>().SendAddressRelease();
#endif
        break;

    case kRoleChild:
        IgnoreError(SendChildUpdateRequest(kAppendZeroTimeout));
        break;

    case kRoleDisabled:
    case kRoleDetached:
        // If device is already detached or disabled, we start the timer
        // with zero duration to stop and invoke the callback when the
        // timer fires, so the operation finishes immediately and
        // asynchronously.
        timeout = 0;
        break;
    }

    mDetachingGracefully = true;
    mAttachTimer.Start(timeout);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// TlvList

void Mle::TlvList::Add(uint8_t aTlvType)
{
    VerifyOrExit(!Contains(aTlvType));

    if (PushBack(aTlvType) != kErrorNone)
    {
        LogWarn("Failed to include TLV %d", aTlvType);
    }

exit:
    return;
}

void Mle::TlvList::AddElementsFrom(const TlvList &aTlvList)
{
    for (uint8_t tlvType : aTlvList)
    {
        Add(tlvType);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// DelayedSender

Mle::DelayedSender::DelayedSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
}

Error Mle::DelayedSender::SendMessage(TxMessage &aMessage, const Ip6::Address &aDestination, uint16_t aDelay)
{
    Error    error = kErrorNone;
    Metadata metadata;

    metadata.mSendTime    = TimerMilli::GetNow() + aDelay;
    metadata.mDestination = aDestination;

    SuccessOrExit(error = metadata.AppendTo(aMessage));
    mQueue.Enqueue(aMessage);

    mTimer.FireAtIfEarlier(metadata.mSendTime);

exit:
    return error;
}

void Mle::DelayedSender::HandleTimer(void)
{
    NextFireTime nextSendTime;

    for (Message &message : mQueue)
    {
        Metadata metadata;

        metadata.ReadFrom(message);

        if (nextSendTime.GetNow() < metadata.mSendTime)
        {
            nextSendTime.UpdateIfEarlier(metadata.mSendTime);
        }
        else
        {
            mQueue.Dequeue(message);
            Send(static_cast<TxMessage &>(message), metadata);
        }
    }

    mTimer.FireAt(nextSendTime);
}

void Mle::DelayedSender::Send(TxMessage &aMessage, const Metadata &aMetadata)
{
    Error error = kErrorNone;

    aMetadata.RemoveFrom(aMessage);

    if (aMessage.IsMleCommand(kCommandDataRequest))
    {
        SuccessOrExit(error = aMessage.AppendActiveAndPendingTimestampTlvs());
    }

    SuccessOrExit(error = aMessage.SendTo(aMetadata.mDestination));

    Log(kMessageSend, kTypeGenericDelayed, aMetadata.mDestination);

    if (!Get<Mle>().IsRxOnWhenIdle())
    {
        // Start fast poll mode, assuming enqueued msg is MLE Data Request.
        // Note: Finer-grade check may be required when deciding whether or
        // not to enter fast poll mode for other type of delayed message.

        Get<DataPollSender>().SendFastPolls(DataPollSender::kDefaultFastPolls);
    }

exit:
    if (error != kErrorNone)
    {
        aMessage.Free();
    }
}

void Mle::DelayedSender::RemoveDataResponseMessage(void)
{
    RemoveMessage(kCommandDataResponse, kTypeDataResponse, nullptr);
}

void Mle::DelayedSender::RemoveDataRequestMessage(const Ip6::Address &aDestination)
{
    RemoveMessage(kCommandDataRequest, kTypeDataRequest, &aDestination);
}

void Mle::DelayedSender::RemoveMessage(Command aCommand, MessageType aMessageType, const Ip6::Address *aDestination)
{
    for (Message &message : mQueue)
    {
        Metadata metadata;

        metadata.ReadFrom(message);

        if (message.IsMleCommand(aCommand) && ((aDestination == nullptr) || (metadata.mDestination == *aDestination)))
        {
            mQueue.DequeueAndFree(message);
            Log(kMessageRemoveDelayed, aMessageType, metadata.mDestination);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
// TxMessage

Mle::TxMessage *Mle::NewMleMessage(Command aCommand)
{
    Error             error = kErrorNone;
    TxMessage        *message;
    Message::Settings settings(Message::kNoLinkSecurity, Message::kPriorityNet);
    uint8_t           securitySuite;

    message = static_cast<TxMessage *>(mSocket.NewMessage(0, settings));
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    securitySuite = k154Security;

    if ((aCommand == kCommandDiscoveryRequest) || (aCommand == kCommandDiscoveryResponse))
    {
        securitySuite = kNoSecurity;
    }

    message->SetSubType(Message::kSubTypeMle);
    message->SetMleCommand(aCommand);

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

Error Mle::TxMessage::AppendStatusTlv(StatusTlv::Status aStatus) { return Tlv::Append<StatusTlv>(*this, aStatus); }

Error Mle::TxMessage::AppendModeTlv(DeviceMode aMode) { return Tlv::Append<ModeTlv>(*this, aMode.Get()); }

Error Mle::TxMessage::AppendTimeoutTlv(uint32_t aTimeout) { return Tlv::Append<TimeoutTlv>(*this, aTimeout); }

Error Mle::TxMessage::AppendChallengeTlv(const TxChallenge &aChallenge)
{
    return Tlv::Append<ChallengeTlv>(*this, &aChallenge, sizeof(aChallenge));
}

Error Mle::TxMessage::AppendResponseTlv(const RxChallenge &aResponse)
{
    return Tlv::Append<ResponseTlv>(*this, aResponse.GetBytes(), aResponse.GetLength());
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
    Get<KeyManager>().SetAllMacFrameCounters(counter, /* aSetIfLarger */ true);
#endif

    return Tlv::Append<LinkFrameCounterTlv>(*this, counter);
}

Error Mle::TxMessage::AppendMleFrameCounterTlv(void)
{
    return Tlv::Append<MleFrameCounterTlv>(*this, Get<KeyManager>().GetMleFrameCounter());
}

Error Mle::TxMessage::AppendLinkAndMleFrameCounterTlvs(void)
{
    Error error;

    SuccessOrExit(error = AppendLinkFrameCounterTlv());
    error = AppendMleFrameCounterTlv();

exit:
    return error;
}

Error Mle::TxMessage::AppendAddress16Tlv(uint16_t aRloc16) { return Tlv::Append<Address16Tlv>(*this, aRloc16); }

Error Mle::TxMessage::AppendLeaderDataTlv(void)
{
    LeaderDataTlv leaderDataTlv;

    Get<Mle>().mLeaderData.SetDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kFullSet));
    Get<Mle>().mLeaderData.SetStableDataVersion(Get<NetworkData::Leader>().GetVersion(NetworkData::kStableSubset));

    leaderDataTlv.Init();
    leaderDataTlv.Set(Get<Mle>().mLeaderData);

    return leaderDataTlv.AppendTo(*this);
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

Error Mle::TxMessage::AppendScanMaskTlv(uint8_t aScanMask) { return Tlv::Append<ScanMaskTlv>(*this, aScanMask); }

Error Mle::TxMessage::AppendLinkMarginTlv(uint8_t aLinkMargin)
{
    return Tlv::Append<LinkMarginTlv>(*this, aLinkMargin);
}

Error Mle::TxMessage::AppendVersionTlv(void) { return Tlv::Append<VersionTlv>(*this, kThreadVersion); }

Error Mle::TxMessage::AppendAddressRegistrationTlv(AddressRegistrationMode aMode)
{
    Error           error = kErrorNone;
    Tlv             tlv;
    Lowpan::Context context;
    uint8_t         counter     = 0;
    uint16_t        startOffset = GetLength();

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = Append(tlv));

    // Prioritize ML-EID
    SuccessOrExit(error = AppendCompressedAddressEntry(kMeshLocalPrefixContextId, Get<Mle>().GetMeshLocalEid()));

    // Continue to append the other addresses if not `kAppendMeshLocalOnly` mode
    VerifyOrExit(aMode != kAppendMeshLocalOnly);
    counter++;

#if OPENTHREAD_CONFIG_DUA_ENABLE
    if (Get<ThreadNetif>().HasUnicastAddress(Get<DuaManager>().GetDomainUnicastAddress()) &&
        (Get<NetworkData::Leader>().GetContext(Get<DuaManager>().GetDomainUnicastAddress(), context) == kErrorNone))
    {
        // Prioritize DUA, compressed entry
        SuccessOrExit(
            error = AppendCompressedAddressEntry(context.mContextId, Get<DuaManager>().GetDomainUnicastAddress()));
        counter++;
    }
#endif

    for (const Ip6::Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (addr.GetAddress().IsLinkLocalUnicast() || Get<Mle>().IsRoutingLocator(addr.GetAddress()) ||
            Get<Mle>().IsAnycastLocator(addr.GetAddress()) || addr.GetAddress() == Get<Mle>().GetMeshLocalEid())
        {
            continue;
        }

#if OPENTHREAD_CONFIG_DUA_ENABLE
        if (addr.GetAddress() == Get<DuaManager>().GetDomainUnicastAddress())
        {
            continue;
        }
#endif

        if (Get<NetworkData::Leader>().GetContext(addr.GetAddress(), context) == kErrorNone)
        {
            SuccessOrExit(error = AppendCompressedAddressEntry(context.mContextId, addr.GetAddress()));
        }
        else
        {
            SuccessOrExit(error = AppendAddressEntry(addr.GetAddress()));
        }

        counter++;
        // only continue to append if there is available entry.
        VerifyOrExit(counter < kMaxIpAddressesToRegister);
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

            SuccessOrExit(error = AppendAddressEntry(addr.GetAddress()));
            counter++;
            // only continue to append if there is available entry.
            VerifyOrExit(counter < kMaxIpAddressesToRegister);
        }
    }

exit:

    if (error == kErrorNone)
    {
        tlv.SetLength(static_cast<uint8_t>(GetLength() - startOffset - sizeof(Tlv)));
        Write(startOffset, tlv);
    }

    return error;
}

Error Mle::TxMessage::AppendCompressedAddressEntry(uint8_t aContextId, const Ip6::Address &aAddress)
{
    // Append an IPv6 address entry in an Address Registration TLV
    // using compressed format (context ID with IID).

    Error error;

    SuccessOrExit(error = Append<uint8_t>(AddressRegistrationTlv::ControlByteFor(aContextId)));
    error = Append(aAddress.GetIid());

exit:
    return error;
}

Error Mle::TxMessage::AppendAddressEntry(const Ip6::Address &aAddress)
{
    // Append an IPv6 address entry in an Address Registration TLV
    // using uncompressed format

    Error   error;
    uint8_t controlByte = AddressRegistrationTlv::kControlByteUncompressed;

    SuccessOrExit(error = Append(controlByte));
    error = Append(aAddress);

exit:
    return error;
}

Error Mle::TxMessage::AppendSupervisionIntervalTlvIfSleepyChild(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!Get<Mle>().IsRxOnWhenIdle());
    error = AppendSupervisionIntervalTlv(Get<SupervisionListener>().GetInterval());

exit:
    return error;
}

Error Mle::TxMessage::AppendSupervisionIntervalTlv(uint16_t aInterval)
{
    return Tlv::Append<SupervisionIntervalTlv>(*this, aInterval);
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
    const MeshCoP::Timestamp &timestamp = Get<MeshCoP::ActiveDatasetManager>().GetTimestamp();

    VerifyOrExit(timestamp.IsValid());
    error = Tlv::Append<ActiveTimestampTlv>(*this, timestamp);

exit:
    return error;
}

Error Mle::TxMessage::AppendPendingTimestampTlv(void)
{
    Error                     error     = kErrorNone;
    const MeshCoP::Timestamp &timestamp = Get<MeshCoP::PendingDatasetManager>().GetTimestamp();

    VerifyOrExit(timestamp.IsValid());
    error = Tlv::Append<PendingTimestampTlv>(*this, timestamp);

exit:
    return error;
}

Error Mle::TxMessage::AppendActiveAndPendingTimestampTlvs(void)
{
    Error error;

    SuccessOrExit(error = AppendActiveTimestampTlv());
    error = AppendPendingTimestampTlv();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
Error Mle::TxMessage::AppendCslChannelTlv(void)
{
    // CSL channel value of zero indicates that the CSL channel is not
    // specified. We can use this value in the TLV as well.

    return Tlv::Append<CslChannelTlv>(*this, ChannelTlvValue(Get<Mac::Mac>().GetCslChannel()));
}

Error Mle::TxMessage::AppendCslTimeoutTlv(void)
{
    uint32_t timeout = Get<Mle>().GetCslTimeout();

    if (timeout == 0)
    {
        timeout = Get<Mle>().GetTimeout();
    }

    return Tlv::Append<CslTimeoutTlv>(*this, timeout);
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
Error Mle::TxMessage::AppendCslClockAccuracyTlv(void)
{
    CslClockAccuracyTlv cslClockAccuracyTlv;

    cslClockAccuracyTlv.Init();
    cslClockAccuracyTlv.SetCslClockAccuracy(Get<Radio>().GetCslAccuracy());
    cslClockAccuracyTlv.SetCslUncertainty(Get<Radio>().GetCslUncertainty());

    return Append(cslClockAccuracyTlv);
}
#endif

Error Mle::TxMessage::SendTo(const Ip6::Address &aDestination)
{
    Error            error  = kErrorNone;
    uint16_t         offset = 0;
    uint8_t          securitySuite;
    Ip6::MessageInfo messageInfo;

    messageInfo.SetPeerAddr(aDestination);
    messageInfo.SetSockAddr(Get<Mle>().mLinkLocalAddress.GetAddress());
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
    return Get<Mle>().mDelayedSender.SendMessage(*this, aDestination, aDelay);
}

#if OPENTHREAD_FTD

Error Mle::TxMessage::AppendConnectivityTlv(void)
{
    ConnectivityTlv tlv;

    tlv.Init();
    Get<MleRouter>().FillConnectivityTlv(tlv);

    return tlv.AppendTo(*this);
}

Error Mle::TxMessage::AppendAddressRegistrationTlv(Child &aChild)
{
    Error           error;
    Tlv             tlv;
    Lowpan::Context context;
    uint16_t        startOffset = GetLength();

    tlv.SetType(Tlv::kAddressRegistration);
    SuccessOrExit(error = Append(tlv));

    // The parent must echo back all registered IPv6 addresses except
    // for the ML-EID, which is excluded by `Child::GetIp6Addresses()`.

    for (const Ip6::Address &address : aChild.GetIp6Addresses())
    {
        if (address.IsMulticast() || Get<NetworkData::Leader>().GetContext(address, context) != kErrorNone)
        {
            SuccessOrExit(error = AppendAddressEntry(address));
        }
        else
        {
            SuccessOrExit(error = AppendCompressedAddressEntry(context.mContextId, address));
        }
    }

    tlv.SetLength(static_cast<uint8_t>(GetLength() - startOffset - sizeof(Tlv)));
    Write(startOffset, tlv);

exit:
    return error;
}

Error Mle::TxMessage::AppendRouteTlv(Neighbor *aNeighbor)
{
    RouteTlv tlv;

    tlv.Init();
    Get<RouterTable>().FillRouteTlv(tlv, aNeighbor);

    return tlv.AppendTo(*this);
}

Error Mle::TxMessage::AppendActiveDatasetTlv(void) { return AppendDatasetTlv(MeshCoP::Dataset::kActive); }

Error Mle::TxMessage::AppendPendingDatasetTlv(void) { return AppendDatasetTlv(MeshCoP::Dataset::kPending); }

Error Mle::TxMessage::AppendDatasetTlv(MeshCoP::Dataset::Type aDatasetType)
{
    Error            error = kErrorNotFound;
    Tlv::Type        tlvType;
    MeshCoP::Dataset dataset;

    switch (aDatasetType)
    {
    case MeshCoP::Dataset::kActive:
        error   = Get<MeshCoP::ActiveDatasetManager>().Read(dataset);
        tlvType = Tlv::kActiveDataset;
        break;

    case MeshCoP::Dataset::kPending:
        error   = Get<MeshCoP::PendingDatasetManager>().Read(dataset);
        tlvType = Tlv::kPendingDataset;
        break;
    }

    if (error != kErrorNone)
    {
        // If there's no dataset, no need to append TLV. We'll treat it
        // as success.

        ExitNow(error = kErrorNone);
    }

    // Remove the Timestamp TLV from Dataset before appending to the
    // message. The Timestamp is appended as its own MLE TLV to the
    // message.

    dataset.RemoveTimestamp(aDatasetType);

    error = Tlv::AppendTlv(*this, tlvType, dataset.GetBytes(), dataset.GetLength());

exit:
    return error;
}

Error Mle::TxMessage::AppendSteeringDataTlv(void)
{
    Error                 error = kErrorNone;
    MeshCoP::SteeringData steeringData;

#if OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
    if (!Get<MleRouter>().mSteeringData.IsEmpty())
    {
        steeringData = Get<MleRouter>().mSteeringData;
    }
    else
#endif
    {
        SuccessOrExit(Get<NetworkData::Leader>().FindSteeringData(steeringData));
    }

    error = Tlv::Append<MeshCoP::SteeringDataTlv>(*this, steeringData.GetData(), steeringData.GetLength());

exit:
    return error;
}

#endif // OPENTHREAD_FTD

//---------------------------------------------------------------------------------------------------------------------
// RxMessage

bool Mle::RxMessage::ContainsTlv(Tlv::Type aTlvType) const
{
    OffsetRange offsetRange;

    return Tlv::FindTlvValueOffsetRange(*this, aTlvType, offsetRange) == kErrorNone;
}

Error Mle::RxMessage::ReadModeTlv(DeviceMode &aMode) const
{
    Error   error;
    uint8_t modeBitmask;

    SuccessOrExit(error = Tlv::Find<ModeTlv>(*this, modeBitmask));
    aMode.Set(modeBitmask);

exit:
    return error;
}

Error Mle::RxMessage::ReadVersionTlv(uint16_t &aVersion) const
{
    Error error;

    SuccessOrExit(error = Tlv::Find<VersionTlv>(*this, aVersion));
    VerifyOrExit(aVersion >= kThreadVersion1p1, error = kErrorParse);

exit:
    return error;
}

Error Mle::RxMessage::ReadChallengeOrResponse(uint8_t aTlvType, RxChallenge &aRxChallenge) const
{
    Error       error;
    OffsetRange offsetRange;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(*this, aTlvType, offsetRange));
    error = aRxChallenge.ReadFrom(*this, offsetRange);

exit:
    return error;
}

Error Mle::RxMessage::ReadChallengeTlv(RxChallenge &aChallenge) const
{
    return ReadChallengeOrResponse(Tlv::kChallenge, aChallenge);
}

Error Mle::RxMessage::ReadResponseTlv(RxChallenge &aResponse) const
{
    return ReadChallengeOrResponse(Tlv::kResponse, aResponse);
}

Error Mle::RxMessage::ReadAndMatchResponseTlvWith(const TxChallenge &aChallenge) const
{
    Error       error;
    RxChallenge response;

    SuccessOrExit(error = ReadResponseTlv(response));
    VerifyOrExit(response == aChallenge, error = kErrorSecurity);

exit:
    return error;
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

Error Mle::RxMessage::ReadAndSetNetworkDataTlv(const LeaderData &aLeaderData) const
{
    Error       error;
    OffsetRange offsetRange;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(*this, Tlv::kNetworkData, offsetRange));

    error = Get<NetworkData::Leader>().SetNetworkData(aLeaderData.GetDataVersion(NetworkData::kFullSet),
                                                      aLeaderData.GetDataVersion(NetworkData::kStableSubset),
                                                      Get<Mle>().GetNetworkDataType(), *this, offsetRange);
exit:
    return error;
}

Error Mle::RxMessage::ReadAndSaveActiveDataset(const MeshCoP::Timestamp &aActiveTimestamp) const
{
    return ReadAndSaveDataset(MeshCoP::Dataset::kActive, aActiveTimestamp);
}

Error Mle::RxMessage::ReadAndSavePendingDataset(const MeshCoP::Timestamp &aPendingTimestamp) const
{
    return ReadAndSaveDataset(MeshCoP::Dataset::kPending, aPendingTimestamp);
}

Error Mle::RxMessage::ReadAndSaveDataset(MeshCoP::Dataset::Type    aDatasetType,
                                         const MeshCoP::Timestamp &aTimestamp) const
{
    Error            error   = kErrorNone;
    Tlv::Type        tlvType = (aDatasetType == MeshCoP::Dataset::kActive) ? Tlv::kActiveDataset : Tlv::kPendingDataset;
    MeshCoP::Dataset dataset;
    OffsetRange      offsetRange;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(*this, tlvType, offsetRange));

    SuccessOrExit(error = dataset.SetFrom(*this, offsetRange));
    SuccessOrExit(error = dataset.ValidateTlvs());
    SuccessOrExit(error = dataset.WriteTimestamp(aDatasetType, aTimestamp));

    switch (aDatasetType)
    {
    case MeshCoP::Dataset::kActive:
        error = Get<MeshCoP::ActiveDatasetManager>().Save(dataset);
        break;
    case MeshCoP::Dataset::kPending:
        error = Get<MeshCoP::PendingDatasetManager>().Save(dataset);
        break;
    }

exit:
    return error;
}

Error Mle::RxMessage::ReadTlvRequestTlv(TlvList &aTlvList) const
{
    Error       error;
    OffsetRange offsetRange;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(*this, Tlv::kTlvRequest, offsetRange));

    offsetRange.ShrinkLength(aTlvList.GetMaxSize());

    ReadBytes(offsetRange, aTlvList.GetArrayBuffer());
    aTlvList.SetLength(static_cast<uint8_t>(offsetRange.GetLength()));

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
Error Mle::RxMessage::ReadCslClockAccuracyTlv(Mac::CslAccuracy &aCslAccuracy) const
{
    Error               error;
    CslClockAccuracyTlv clockAccuracyTlv;

    SuccessOrExit(error = Tlv::FindTlv(*this, clockAccuracyTlv));
    VerifyOrExit(clockAccuracyTlv.IsValid(), error = kErrorParse);
    aCslAccuracy.SetClockAccuracy(clockAccuracyTlv.GetCslClockAccuracy());
    aCslAccuracy.SetUncertainty(clockAccuracyTlv.GetCslUncertainty());

exit:
    return error;
}
#endif

#if OPENTHREAD_FTD
Error Mle::RxMessage::ReadRouteTlv(RouteTlv &aRouteTlv) const
{
    Error error;

    SuccessOrExit(error = Tlv::FindTlv(*this, aRouteTlv));
    VerifyOrExit(aRouteTlv.IsValid(), error = kErrorParse);

exit:
    return error;
}
#endif

//---------------------------------------------------------------------------------------------------------------------
// ParentCandidate

void Mle::ParentCandidate::Clear(void)
{
    Instance &instance = GetInstance();

    ClearAllBytes(*this);
    Init(instance);
}

void Mle::ParentCandidate::CopyTo(Parent &aParent) const
{
    // We use an intermediate pointer to copy `ParentCandidate`
    // to silence code checker's warning about object slicing
    // (assigning a sub-class to base class instance).

    const Parent *candidateAsParent = this;

    aParent = *candidateAsParent;
}

} // namespace Mle
} // namespace ot
