/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements the Link Extension class.
 */

#include "link_ext.hpp"

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE

#include "crypto/aes_ccm.hpp"
#include "instance/instance.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("LinkExt");

LinkExt::LinkExt(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mBurstDoneCallback(nullptr)
    , mBurstTimer(aInstance)
    , mBurstTxTime(0)
    , mBurstPeriodUs(0)
    , mBurstCount(0)
    , mIsTimedBurst(false)
    , mIsBurstTxPending(false)
    , mIsBurstStopRequested(false)
    , mIsReceiving(false)
#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
    , mNumPeriodicRxSchedules(0)
#endif
    , mMacFilterPolicy(0)
#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    , mNumMacKeyTableEntries(0)
    , mNumMacNeighborTableEntries(0)
#endif
{
}

void LinkExt::Reset(void)
{
    CancelBurst();
    mBurstDoneCallback = nullptr;
    mIsReceiving       = false;

#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
    mNumPeriodicRxSchedules = 0;
    mPeriodicRxWindow.Clear();
#endif

    mMacFilterPolicy = 0;
#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    mNumMacKeyTableEntries      = 0;
    mNumMacNeighborTableEntries = 0;
#endif
}

void LinkExt::HandleLinkRawEnableChange(void)
{
    if (IsBurstActive())
    {
        FinishBurst();
    }

    mIsReceiving = false;

#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)
    // Enabling or disabling the radio leaves no timed RX window
    // scheduled on it.
    mPeriodicRxWindow.Clear();
#endif
}

bool LinkExt::ShouldDeferIdleState(bool aIsReceiving)
{
    mIsReceiving = aIsReceiving;

    return mIsBurstTxPending;
}

void LinkExt::CancelBurst(void)
{
    mBurstTimer.Stop();
    mBurstChannelMask.Clear();
    mBurstTxTime          = 0;
    mBurstPeriodUs        = 0;
    mBurstCount           = 0;
    mIsTimedBurst         = false;
    mIsBurstTxPending     = false;
    mIsBurstStopRequested = false;
}

void LinkExt::FinishBurst(void)
{
    otLinkExtBurstDone callback = mBurstDoneCallback;

    CancelBurst();
    mBurstDoneCallback = nullptr;

    if (callback != nullptr)
    {
        callback(&GetInstance());
    }
}

void LinkExt::ApplyIdleState(void)
{
    // Puts the radio in the state last requested by the raw link-layer
    // (see `ShouldDeferIdleState()`). Also drops a burst frame waiting in `SubMac`
    // (e.g., for its target TX time).

    SubMac &subMac = Get<SubMac>();

    mIsBurstTxPending = false;
    IgnoreError(mIsReceiving ? subMac.Receive(Get<LinkRaw>().GetChannel()) : subMac.Sleep());
}

void LinkExt::SkipMissedBurstSlots(Radio::Time32 aNow)
{
    // Skips all periodic slots whose start time `mBurstTxTime` is not
    // strictly after `aNow`. `mBurstCount` is decremented for every
    // skipped slot. MUST be called only when `mBurstPeriodUs` is non-zero.

    uint32_t missed;

    VerifyOrExit(!Radio::IsTimeStrictlyBefore(aNow, mBurstTxTime));

    missed = (aNow - mBurstTxTime) / mBurstPeriodUs + 1;
    missed = Min<uint32_t>(missed, mBurstCount);

    mBurstCount -= static_cast<uint16_t>(missed);
    mBurstTxTime += missed * mBurstPeriodUs;

exit:
    return;
}

void LinkExt::StartBurstTimer(Radio::Time32 aNow)
{
    // Arms `mBurstTimer` to start the next tick. A timed tick is handed
    // to `SubMac` `kBurstTxAheadUs` before its target TX time, so that
    // the radio is not held for the tick earlier than needed.

    uint32_t delay = 0;

    if (mIsTimedBurst)
    {
        if (Radio::IsTimeStrictlyBefore(aNow + kBurstTxAheadUs, mBurstTxTime))
        {
            delay = mBurstTxTime - kBurstTxAheadUs - aNow;
        }
    }
    else if (mBurstPeriodUs != 0)
    {
        delay = mBurstTxTime - aNow;
    }

    mBurstTimer.Start(delay);
}

void LinkExt::ScheduleNextBurstTick(void)
{
    // Called when the current tick is done (all channels sent or the
    // tick is skipped). Determines the next tick on the radio clock and
    // arms `mBurstTimer` to start it.

    Radio::Time32 now = Get<Radio::Radio>().GetNowAsTime32();

    mBurstCount--;

    if (mBurstPeriodUs == 0)
    {
        // Back-to-back ticks: only the first tick may be timed.
        mIsTimedBurst = false;
    }
    else
    {
        mBurstTxTime += mBurstPeriodUs;
        SkipMissedBurstSlots(now);
    }

    VerifyOrExit(mBurstCount > 0, FinishBurst());
    StartBurstTimer(now);

exit:
    return;
}

void LinkExt::HandleBurstTimer(void)
{
    TxFrame &frame   = Get<SubMac>().GetTransmitFrame();
    uint8_t  channel = ChannelMask::kChannelIteratorFirst;

    VerifyOrExit(IsBurstActive());
    VerifyOrExit(Get<LinkRaw>().IsEnabled(), FinishBurst());

    if (mIsTimedBurst)
    {
        Radio::Time32 now = Get<Radio::Radio>().GetNowAsTime32();

        if (!Radio::IsTimeStrictlyBefore(now, mBurstTxTime))
        {
            // The slot was missed, skip this tick.
            ScheduleNextBurstTick();
            ExitNow();
        }

        frame.SetTargetTxTime(mBurstTxTime, now);
    }
    else
    {
        frame.ClearTargetTxTime();
    }

    if (mBurstChannelMask.GetNextChannel(channel) == kErrorNone)
    {
        frame.mChannel = channel;
    }

    SendBurstFrame();

exit:
    return;
}

void LinkExt::SendBurstFrame(void)
{
    // Every burst transmission is requested through this method. A
    // request rejected by `SubMac` is handled as a completed
    // transmission, so that only this transmission is affected and the
    // burst continues.

    TxFrame &frame = Get<SubMac>().GetTransmitFrame();
    Error    error;

    // After each tick, the radio enters the idle state requested by the
    // host (see `ApplyIdleState()`), so `RxChannelAfterTxDone` is set to
    // the receive channel rather than taken from the request.
    frame.SetRxChannelAfterTxDone(Get<LinkRaw>().GetChannel());

    // Set before `Send()`, since the transmission may complete within it.
    mIsBurstTxPending = true;
    error             = Get<SubMac>().Send();
    VerifyOrExit(error != kErrorNone);

    LogWarnOnError(error, "send burst frame");
    HandleTransmitDone(frame);

exit:
    return;
}

void LinkExt::HandleTransmitDone(TxFrame &aFrame)
{
    uint8_t nextChannel = aFrame.GetChannel();

    mIsBurstTxPending = false;

    if (!mIsBurstStopRequested && (mBurstChannelMask.GetNextChannel(nextChannel) == kErrorNone))
    {
        aFrame.ClearTargetTxTime();
        aFrame.mChannel = nextChannel;
        SendBurstFrame();
        ExitNow();
    }

    // The tick is done.
    ApplyIdleState();

    if (mIsBurstStopRequested)
    {
        FinishBurst();
    }
    else
    {
        ScheduleNextBurstTick();
    }

exit:
    return;
}

Error LinkExt::StartBurst(const otLinkExtBurstConfig &aConfig)
{
    // Validates the burst request and starts the burst. The first tick
    // is sent from `HandleBurstTimer()`, like all subsequent ticks.

    Error              error = kErrorNone;
    TxFrame           &frame = Get<SubMac>().GetTransmitFrame();
    TxFrame::ParseInfo frameInfo;
    Radio::Time32      now;

    VerifyOrExit(aConfig.mCount > 0, error = kErrorInvalidArgs);

    // Burst frames are sent repeatedly as-is, so they must not request
    // an ACK and must not require any further processing by the RCP.
    VerifyOrExit(frameInfo.ParseFrom(frame, Frame::kParseAddrFields) == kErrorNone, error = kErrorInvalidArgs);
    VerifyOrExit(!frameInfo.mIsAckRequest, error = kErrorInvalidArgs);
    VerifyOrExit(!frameInfo.mIsSecurityEnabled || frame.IsSecurityProcessed(), error = kErrorInvalidArgs);

    if (aConfig.mChannelMask != 0)
    {
        mBurstChannelMask.SetMask(aConfig.mChannelMask & Radio::kSupportedChannels);
        VerifyOrExit(!mBurstChannelMask.IsEmpty(), error = kErrorInvalidArgs);
    }

    now            = Get<Radio::Radio>().GetNowAsTime32();
    mBurstCount    = aConfig.mCount;
    mBurstPeriodUs = aConfig.mPeriodUs;
    mIsTimedBurst  = frame.IsTargetTxTimeSpecified();
    mBurstTxTime   = mIsTimedBurst ? frame.GetTargetTxTime() : now;

    if (mIsTimedBurst && !Radio::IsTimeStrictlyBefore(now, mBurstTxTime))
    {
        // The first slot was missed.
        if (mBurstPeriodUs == 0)
        {
            mBurstCount--;
            mIsTimedBurst = false;
        }
        else
        {
            SkipMissedBurstSlots(now);
        }

        VerifyOrExit(mBurstCount > 0, error = kErrorInvalidArgs);
    }

    StartBurstTimer(now);

exit:
    return error;
}

Error LinkExt::TransmitBurst(const otLinkExtBurstConfig &aConfig, otLinkExtBurstDone aCallback)
{
    Error error = kErrorNone;

    VerifyOrExit(Get<LinkRaw>().IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(!IsBurstActive(), error = kErrorBusy);

    SuccessOrExit(error = StartBurst(aConfig));
    mBurstDoneCallback = aCallback;

exit:
    if (error == kErrorInvalidArgs)
    {
        CancelBurst();
    }

    return error;
}

Error LinkExt::StopBurst(void)
{
    Error error = kErrorNone;

    VerifyOrExit(Get<LinkRaw>().IsEnabled(), error = kErrorInvalidState);
    VerifyOrExit(IsBurstActive());

    if (mIsBurstTxPending && Get<SubMac>().IsTransmittingOrScanning())
    {
        // The frame is with the radio and cannot be taken back, so the
        // burst ends when the radio is done with it. Only the tick in
        // progress remains.
        mIsBurstStopRequested = true;
        mBurstCount           = 1;
        mBurstTimer.Stop();
        ExitNow();
    }

    if (mIsBurstTxPending)
    {
        ApplyIdleState();
    }

    // The burst ends right away, which the caller learns from the
    // configuration, so the callback is not called.
    CancelBurst();
    mBurstDoneCallback = nullptr;

exit:
    return error;
}

void LinkExt::GetBurstConfig(otLinkExtBurstConfig &aConfig) const
{
    aConfig.mCount       = mBurstCount;
    aConfig.mPeriodUs    = mBurstPeriodUs;
    aConfig.mChannelMask = mBurstChannelMask.GetMask();
}

#if (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)

bool LinkExt::IsValid(const PeriodicRxSchedule &aSchedule)
{
    return ChannelMask(Get<Radio::Radio>().GetSupportedChannelMask()).ContainsChannel(aSchedule.mChannel) &&
           (aSchedule.mPeriodUs != 0) && (aSchedule.mDurationUs != 0) && (aSchedule.mPhaseUs < aSchedule.mPeriodUs) &&
           (aSchedule.mDurationUs <= aSchedule.mPeriodUs);
}

Error LinkExt::SetPeriodicRxSchedules(const PeriodicRxSchedule *aSchedules, uint8_t aNumSchedules)
{
    Error error = kErrorNone;

    VerifyOrExit(aNumSchedules <= kMaxPeriodicRxSchedules, error = kErrorNoBufs);

    for (uint8_t i = 0; i < aNumSchedules; i++)
    {
        VerifyOrExit(IsValid(aSchedules[i]), error = kErrorInvalidArgs);
    }

    for (uint8_t i = 0; i < aNumSchedules; i++)
    {
        mPeriodicRxSchedules[i] = aSchedules[i];
    }

    mNumPeriodicRxSchedules = aNumSchedules;

    LogInfo("Periodic RX schedules updated, count:%u", mNumPeriodicRxSchedules);

    if (mNumPeriodicRxSchedules == 0)
    {
        // A window already scheduled on the radio is not cancelled, since
        // `SubMac::ListenScheduler` does not cancel a window scheduled on
        // the radio when no listen source provides a window.
        mPeriodicRxWindow.Clear();
    }

    Get<SubMac>().UpdateOnPeriodicRxChange();

exit:
    return error;
}

bool LinkExt::DeterminePeriodicRxWindow(const Radio::SyncedTime &aNow, SubMac::TimedRx &aTimedRx)
{
    // Determines the window to provide to `SubMac::ListenScheduler`
    // (see "Periodic RX arbitration" in the header).
    //
    // Design: The radio holds a single timed RX window. A subsequent
    // `otPlatRadioReceiveAt()` replaces a window which has not started
    // yet, while for an active window it only updates the end time
    // (without interrupting reception). Hence windows are provided one at
    // a time, and the window following the active one must not be
    // provided before the active one has started. The listen scheduler
    // evaluates its sources again at the start time of the provided
    // window, which is exactly when the following window can be provided.
    //
    // On each evaluation, the window covering (or following) `now` is
    // determined from the schedules and compared with the last provided
    // window (`last`):
    //
    // - `last` is active and unchanged (same channel and end time): it is
    //   already on the radio, so the window following it is provided
    //   instead. Providing `last` again would make the scheduler evaluate
    //   again immediately (at its start time) without making progress.
    //
    // - `last` is active and changed on the same channel (e.g., its end
    //   moved after the schedules were updated): it is provided again
    //   keeping its start time, so the radio keeps receiving and only
    //   applies the new end time. The scheduler then evaluates again
    //   immediately and finds it unchanged.
    //
    // - Otherwise (no active window, a different channel at `now`, or the
    //   next window starting later): the determined window is provided as
    //   is. It either starts now (e.g., taking over from `last` on another
    //   channel) or replaces a window which has not started yet.

    SubMac::TimedRx &last  = mPeriodicRxWindow;
    bool             found = false;
    SubMac::TimedRx  window;

    VerifyOrExit(mNumPeriodicRxSchedules > 0);
    VerifyOrExit(FindNextPeriodicRxWindow(aNow.GetAsTime64(), window));

    if (last.IsSpecified() && last.HasStarted(aNow) && !last.HasEnded(aNow) &&
        (window.GetStartTime() == aNow.GetAsTime64()) && (window.GetChannel() == last.GetChannel()))
    {
        if (window.GetEndTime() == last.GetEndTime())
        {
            VerifyOrExit(FindNextPeriodicRxWindow(last.GetEndTime(), window));
        }
        else
        {
            window.Init(last.GetStartTime(), ClampToUint32(window.GetEndTime() - last.GetStartTime()),
                        window.GetChannel());
        }
    }

    last     = window;
    aTimedRx = window;
    found    = true;

exit:
    return found;
}

uint8_t LinkExt::GetPeriodicRxOwner(Radio::Time64 aTime) const
{
    // Determines which schedule owns the radio at `aTime` (rules 1 and
    // 4). Returns `kPeriodicRxNoOwner` if no window covers `aTime`.

    uint8_t       owner = kPeriodicRxNoOwner;
    Radio::Time64 end;

    for (uint8_t i = 0; i < mNumPeriodicRxSchedules; i++)
    {
        if (IsCovering(mPeriodicRxSchedules[i], aTime, end))
        {
            owner = i;
            break;
        }
    }

    VerifyOrExit(owner != kPeriodicRxNoOwner);

    for (uint8_t i = 0; i < owner; i++)
    {
        if ((mPeriodicRxSchedules[i].mChannel != mPeriodicRxSchedules[owner].mChannel) &&
            IsInGuard(mPeriodicRxSchedules[i], aTime))
        {
            owner = i;
            break;
        }
    }

exit:
    return owner;
}

Radio::Time64 LinkExt::GetNextPeriodicRxEvent(Radio::Time64 aTime) const
{
    // Returns the earliest time after `aTime` at which the owner may
    // change.

    Radio::Time64 event = Radio::kMaxTime64;

    for (uint8_t i = 0; i < mNumPeriodicRxSchedules; i++)
    {
        event = Min(event, GetNextEvent(mPeriodicRxSchedules[i], aTime));
    }

    return event;
}

bool LinkExt::FindNextPeriodicRxWindow(Radio::Time64 aFrom, SubMac::TimedRx &aWindow) const
{
    // Finds the next window starting at or after `aFrom`: a maximal
    // interval during which the owner channel stays the same (rules 2
    // and 3), dropping short fragments (rule 5).

    bool          found     = false;
    Radio::Time64 time      = aFrom;
    uint32_t      maxPeriod = 0;

    for (uint8_t i = 0; i < mNumPeriodicRxSchedules; i++)
    {
        maxPeriod = Max(maxPeriod, mPeriodicRxSchedules[i].mPeriodUs);
    }

    while (true)
    {
        uint8_t       owner;
        uint8_t       nextOwner;
        uint8_t       channel;
        Radio::Time64 start;
        Radio::Time64 limit;
        uint64_t      length;

        while ((owner = GetPeriodicRxOwner(time)) == kPeriodicRxNoOwner)
        {
            time = GetNextPeriodicRxEvent(time);
            VerifyOrExit(time != Radio::kMaxTime64);
        }

        start   = time;
        channel = mPeriodicRxSchedules[owner].mChannel;
        limit   = start + maxPeriod;

        for (uint16_t numEvents = 0; numEvents < kPeriodicRxMaxMergeEvents; numEvents++)
        {
            time = GetNextPeriodicRxEvent(time);

            if (time >= limit)
            {
                break;
            }

            nextOwner = GetPeriodicRxOwner(time);

            if ((nextOwner == kPeriodicRxNoOwner) || (mPeriodicRxSchedules[nextOwner].mChannel != channel))
            {
                break;
            }
        }

        time   = Min(time, limit);
        length = time - start;

        if ((length >= kPeriodicRxMinFragment) || (length >= mPeriodicRxSchedules[owner].mDurationUs))
        {
            aWindow.Init(start, ClampToUint32(length), channel);
            found = true;
            break;
        }
    }

exit:
    return found;
}

Radio::Time64 LinkExt::GetNextStart(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime)
{
    // Returns the earliest window start time strictly after `aTime`. The
    // modulo is evaluated on the full 64-bit radio time.

    Radio::Time64 next = aTime - (aTime % aSchedule.mPeriodUs) + aSchedule.mPhaseUs;

    if (next <= aTime)
    {
        next += aSchedule.mPeriodUs;
    }

    return next;
}

bool LinkExt::IsCovering(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime, Radio::Time64 &aEnd)
{
    // Indicates whether `aTime` is within a window, and if so, outputs
    // the end time of the window.

    bool          covering = false;
    Radio::Time64 start;

    VerifyOrExit(aTime >= aSchedule.mPhaseUs);

    start = aTime - ((aTime - aSchedule.mPhaseUs) % aSchedule.mPeriodUs);
    aEnd  = start + aSchedule.mDurationUs;

    covering = (aTime < aEnd);

exit:
    return covering;
}

bool LinkExt::IsInGuard(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime)
{
    // Indicates whether `aTime` is within the guard time right before
    // the start of a window.

    return (GetNextStart(aSchedule, aTime) - aTime <= kPeriodicRxGuard);
}

Radio::Time64 LinkExt::GetNextEvent(const PeriodicRxSchedule &aSchedule, Radio::Time64 aTime)
{
    // Returns the earliest time after `aTime` which is a window start,
    // a window end, or a guard start.

    Radio::Time64 next  = GetNextStart(aSchedule, aTime);
    Radio::Time64 event = next;
    Radio::Time64 end;

    if (next > aTime + kPeriodicRxGuard)
    {
        // The guard start of the next window is still ahead.
        event = next - kPeriodicRxGuard;
    }
    else if (next + aSchedule.mPeriodUs > aTime + kPeriodicRxGuard)
    {
        // Already within the guard of the next window, so consider the
        // guard start of the following window. It can only be earlier
        // than `next` if the guard time is longer than the period.
        event = Min(event, next + aSchedule.mPeriodUs - kPeriodicRxGuard);
    }

    if (IsCovering(aSchedule, aTime, end))
    {
        event = Min(event, end);
    }

    return event;
}

#endif // (OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES > 0)

//---------------------------------------------------------------------------------------------------------------------
// MAC frame filter

Error LinkExt::SetMacFilterPolicy(uint8_t aPolicy)
{
    Error error = kErrorNone;

    VerifyOrExit((aPolicy & ~kMacFilterPolicyAll) == 0, error = kErrorInvalidArgs);

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES == 0)
    VerifyOrExit((aPolicy & kMacFilterPolicyKeyTable) == 0, error = kErrorNotImplemented);
#endif

    mMacFilterPolicy = aPolicy;

exit:
    return error;
}

bool LinkExt::FilterReceivedFrame(RxFrame *aFrame, Error aError)
{
    bool               accept = true;
    RxFrame::ParseInfo parseInfo;

    VerifyOrExit(mMacFilterPolicy != 0);
    VerifyOrExit(aFrame != nullptr && aError == kErrorNone, accept = false);

    // 1. Check if frame can be parsed
    if (parseInfo.ParseFrom(*aFrame, Frame::kParseFully) != kErrorNone)
    {
        ExitNow(accept = !(mMacFilterPolicy & OT_LINK_EXT_MAC_FILTER_POLICY_DROP_MALFORMED));
    }

    // 2. Check if frame is unsecured
    if (!parseInfo.mIsSecurityEnabled)
    {
        ExitNow(accept = !(mMacFilterPolicy & OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNSECURED));
    }

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)
    // 3. Check against the key and neighbor tables
    switch (CheckMacKeyTable(parseInfo))
    {
    case kMacKeyTableNoMatch:
        ExitNow(accept = !(mMacFilterPolicy & OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_KEY));

    case kMacKeyTableUnknownNeighbor:
        ExitNow(accept = !(mMacFilterPolicy & OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_NEIGHBOR));

    case kMacKeyTableMicFailed:
    case kMacKeyTableReplayed:
        ExitNow(accept = false);

    case kMacKeyTableAccepted:
        break;
    }
#endif

exit:
    return accept;
}

#if (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)

static_assert(OT_LINK_EXT_MAC_KEY_ID_MAX_SIZE == LinkExt::MacKeyTableEntry::kKeyIdLengthMode2,
              "OT_LINK_EXT_MAC_KEY_ID_MAX_SIZE does not match the Key ID length of Key ID Mode 2");

Error LinkExt::SetMacKeyTable(const otLinkExtMacKeyTableEntry *aEntries, uint8_t aNumEntries)
{
    Error error = kErrorNone;

    VerifyOrExit(aNumEntries <= kMaxMacKeyTableEntries, error = kErrorNoBufs);

    for (uint8_t i = 0; i < aNumEntries; i++)
    {
        VerifyOrExit(static_cast<const MacKeyTableEntry &>(aEntries[i]).IsValid(), error = kErrorInvalidArgs);
    }

    for (uint8_t i = 0; i < aNumEntries; i++)
    {
        static_cast<otLinkExtMacKeyTableEntry &>(mMacKeyTable[i]) = aEntries[i];
    }

    mNumMacKeyTableEntries = aNumEntries;

    // Neighbor entries refer to key table entries by index.
    mNumMacNeighborTableEntries = 0;

exit:
    return error;
}

Error LinkExt::SetMacNeighborTable(const otLinkExtMacNeighborTableEntry *aEntries, uint8_t aNumEntries)
{
    Error                 error = kErrorNone;
    MacNeighborTableEntry table[kMaxMacNeighborTableEntries];

    VerifyOrExit(aNumEntries <= kMaxMacNeighborTableEntries, error = kErrorNoBufs);

    for (uint8_t i = 0; i < aNumEntries; i++)
    {
        MacNeighborTableEntry &entry = table[i];

        static_cast<otLinkExtMacNeighborTableEntry &>(entry) = aEntries[i];

        VerifyOrExit(entry.mKeyTableIndex < mNumMacKeyTableEntries && mMacKeyTable[entry.mKeyTableIndex].mHasKey,
                     error = kErrorInvalidArgs);

        // The extended address and key table index identify a neighbor.
        for (uint8_t j = 0; j < i; j++)
        {
            VerifyOrExit(!entry.IsSameNeighbor(table[j]), error = kErrorInvalidArgs);
        }

        // Never move the frame counter of an existing neighbor backwards.
        for (uint8_t j = 0; j < mNumMacNeighborTableEntries; j++)
        {
            const MacNeighborTableEntry &current = mMacNeighborTable[j];

            if (entry.IsSameNeighbor(current))
            {
                entry.mFrameCounter = Max(entry.mFrameCounter, current.mFrameCounter);
                break;
            }
        }
    }

    for (uint8_t i = 0; i < aNumEntries; i++)
    {
        mMacNeighborTable[i] = table[i];
    }

    mNumMacNeighborTableEntries = aNumEntries;

exit:
    return error;
}

bool LinkExt::IsMode1MacKeyIndex(uint8_t aKeyIndex) const
{
    // Indicates whether `aKeyIndex` is the key index of the previous,
    // current, or next MAC key (`SetMode1MacKeys()`).

    uint8_t keyIndex = Get<SubMac>().GetMode1KeyIndex();

    return IsValueInRange(keyIndex, kMinKeyIndex, kMaxKeyIndex) &&
           ((aKeyIndex == keyIndex) || (aKeyIndex == DeterminePrevKeyIndex(keyIndex)) ||
            (aKeyIndex == DetermineNextKeyIndex(keyIndex)));
}

LinkExt::MacKeyTableResult LinkExt::CheckMacKeyTable(const RxFrame::ParseInfo &aParseInfo)
{
    // A frame matching a key table entry without a key is accepted.
    // For a matching entry with a key, the MIC is verified using the
    // extended address of each matching neighbor for the nonce, and
    // the frame counter is checked against the neighbor that verified
    // it. Entries may share a Key ID, so all matching entries are
    // checked.

    MacKeyTableResult result = kMacKeyTableNoMatch;
    const Address    &src    = aParseInfo.mAddrs.mSource;

    if ((aParseInfo.mKeyIdMode == Frame::kKeyIdMode1) && IsMode1MacKeyIndex(aParseInfo.mKeyIndex))
    {
        ExitNow(result = kMacKeyTableAccepted);
    }

    for (uint8_t i = 0; i < mNumMacKeyTableEntries; i++)
    {
        const MacKeyTableEntry &entry         = mMacKeyTable[i];
        bool                    foundNeighbor = false;

        if (!entry.Matches(aParseInfo))
        {
            continue;
        }

        if (!entry.mHasKey)
        {
            ExitNow(result = kMacKeyTableAccepted);
        }

        for (uint8_t j = 0; j < mNumMacNeighborTableEntries; j++)
        {
            MacNeighborTableEntry &neighbor = mMacNeighborTable[j];

            if (!neighbor.Matches(aParseInfo, i))
            {
                continue;
            }

            foundNeighbor = true;

            if (!entry.VerifyMic(aParseInfo, neighbor.GetExtAddress()))
            {
                if (result == kMacKeyTableNoMatch)
                {
                    result = kMacKeyTableMicFailed;
                }

                continue;
            }

            if (aParseInfo.mFrameCounter < neighbor.mFrameCounter)
            {
                VerifyOrExit(!(mMacFilterPolicy & OT_LINK_EXT_MAC_FILTER_POLICY_DROP_REPLAYED),
                             result = kMacKeyTableReplayed);
            }
            else
            {
                neighbor.mFrameCounter = (aParseInfo.mFrameCounter == NumericLimits<uint32_t>::kMax)
                                             ? aParseInfo.mFrameCounter
                                             : aParseInfo.mFrameCounter + 1;
            }

            ExitNow(result = kMacKeyTableAccepted);
        }

        if (foundNeighbor)
        {
            continue;
        }

        // The sender is not in the neighbor table. A frame from an
        // extended source address can still be verified.

        if (src.IsExtended() && entry.VerifyMic(aParseInfo, src.GetExtended()))
        {
            VerifyOrExit(mMacFilterPolicy & OT_LINK_EXT_MAC_FILTER_POLICY_DROP_UNKNOWN_NEIGHBOR,
                         result = kMacKeyTableAccepted);
            result = kMacKeyTableUnknownNeighbor;
        }
        else if (!src.IsExtended())
        {
            result = kMacKeyTableUnknownNeighbor;
        }
        else if (result == kMacKeyTableNoMatch)
        {
            result = kMacKeyTableMicFailed;
        }
    }

exit:
    return result;
}

Error LinkExt::MacKeyTableEntry::GetKeyIdMode(uint8_t &aKeyIdMode) const
{
    Error error = kErrorNone;

    switch (mKeyIdLength)
    {
    case kKeyIdLengthMode0:
        aKeyIdMode = Frame::kKeyIdMode0;
        break;
    case kKeyIdLengthMode1:
        aKeyIdMode = Frame::kKeyIdMode1;
        break;
    case kKeyIdLengthMode2:
        aKeyIdMode = Frame::kKeyIdMode2;
        break;
    default:
        error = kErrorInvalidArgs;
        break;
    }

    return error;
}

bool LinkExt::MacKeyTableEntry::IsValid(void) const
{
    bool    isValid = false;
    uint8_t keyIdMode;

    SuccessOrExit(GetKeyIdMode(keyIdMode));
    VerifyOrExit(mAllowedSecurityLevels != 0);
    VerifyOrExit((mAllowedSecurityLevels & (1 << Frame::kSecurityNone)) == 0);
    isValid = true;

exit:
    return isValid;
}

bool LinkExt::MacKeyTableEntry::Matches(const RxFrame::ParseInfo &aParseInfo) const
{
    bool    matches = false;
    uint8_t keyIdMode;

    SuccessOrExit(GetKeyIdMode(keyIdMode));
    VerifyOrExit(aParseInfo.mKeyIdMode == keyIdMode);

    switch (keyIdMode)
    {
    case Frame::kKeyIdMode2:
        VerifyOrExit(memcmp(mKeyId, aParseInfo.mKeySource.GetBytes(), Frame::kKeySourceSizeMode2) == 0);
        VerifyOrExit(mKeyId[Frame::kKeySourceSizeMode2] == aParseInfo.mKeyIndex);
        break;

    case Frame::kKeyIdMode1:
        VerifyOrExit(mKeyId[0] == aParseInfo.mKeyIndex);
        break;

    default:
        break;
    }

    VerifyOrExit((mAllowedSecurityLevels & (1 << aParseInfo.mSecurityLevel)) != 0);

    matches = true;

exit:
    return matches;
}

bool LinkExt::MacKeyTableEntry::VerifyMic(const RxFrame::ParseInfo &aParseInfo, const ExtAddress &aNonceExtAddr) const
{
    bool                  verified = false;
    Crypto::AesCcm        aesCcm;
    Crypto::AesCcm::Nonce nonce;
    uint8_t               payload[OT_RADIO_FRAME_MAX_SIZE];
    uint16_t              payloadLength = aParseInfo.mPayload.GetLength();

    VerifyOrExit(aParseInfo.mMicSize > 0);

    // Verify the MIC on a copy of the payload and the MIC (which
    // immediately follows the payload) so that the frame is delivered
    // to the host unmodified.
    VerifyOrExit(payloadLength + aParseInfo.mMicSize <= sizeof(payload));
    memcpy(payload, aParseInfo.mPayload.GetBytes(), payloadLength + aParseInfo.mMicSize);

    nonce.InitFrom(aNonceExtAddr, aParseInfo.mFrameCounter, aParseInfo.mSecurityLevel);

    aesCcm.SetKey(mKey.m8, sizeof(mKey));
    aesCcm.SetNonce(nonce);
    aesCcm.SetAuthData(aParseInfo.mHeader.GetBytes(), aParseInfo.mHeader.GetLength());
    aesCcm.SetTagLength(aParseInfo.mMicSize);

    verified = (aesCcm.Process(Crypto::AesCcm::kDecrypt, payload, payloadLength) == kErrorNone);

exit:
    return verified;
}

bool LinkExt::MacNeighborTableEntry::IsSameNeighbor(const otLinkExtMacNeighborTableEntry &aOther) const
{
    return (mKeyTableIndex == aOther.mKeyTableIndex) && (GetExtAddress() == AsCoreType(&aOther.mExtAddress));
}

bool LinkExt::MacNeighborTableEntry::Matches(const RxFrame::ParseInfo &aParseInfo, uint8_t aKeyTableIndex) const
{
    bool           matches = false;
    const Address &src     = aParseInfo.mAddrs.mSource;

    VerifyOrExit(mKeyTableIndex == aKeyTableIndex);

    if (HasPanId())
    {
        const PanIds &panIds = aParseInfo.mPanIds;

        // The PAN ID is checked only when the frame's source PAN ID is
        // known: the Source PAN ID field, or the Destination PAN ID when
        // the Source PAN ID is elided with PAN ID Compression set and a
        // source address is present.
        if (panIds.IsSourcePresent())
        {
            VerifyOrExit(panIds.GetSource() == mPanId);
        }
        else if (aParseInfo.mIsPanIdCompressed && panIds.IsDestinationPresent() && !src.IsNone())
        {
            VerifyOrExit(panIds.GetDestination() == mPanId);
        }
    }

    if (src.IsExtended())
    {
        matches = (src.GetExtended() == GetExtAddress());
    }
    else if (src.IsShort())
    {
        matches = HasShortAddress() && (src.GetShort() == mShortAddress);
    }
    else
    {
        // A frame without a source address may be from any neighbor.
        matches = true;
    }

exit:
    return matches;
}

#endif // (OPENTHREAD_CONFIG_LINK_EXT_MAC_KEY_TABLE_MAX_ENTRIES > 0)

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE
