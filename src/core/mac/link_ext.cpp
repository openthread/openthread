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
{
}

void LinkExt::Reset(void)
{
    CancelBurst();
    mBurstDoneCallback = nullptr;
    mIsReceiving       = false;
}

void LinkExt::HandleLinkRawEnableChange(void)
{
    if (IsBurstActive())
    {
        FinishBurst();
    }

    mIsReceiving = false;
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

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE
