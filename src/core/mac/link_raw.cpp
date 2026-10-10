/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file implements the OpenThread Link Raw API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

#include <openthread/diag.h>
#include <openthread/platform/diag.h>

#include "instance/instance.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("LinkRaw");

LinkRaw::LinkRaw(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mPanId(kPanIdBroadcast)
    , mReceiveDoneCallback(nullptr)
    , mTransmitDoneCallback(nullptr)
    , mEnergyScanDoneCallback(nullptr)
#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    , mBurstTimer(aInstance)
    , mBurstChannelMask(0)
    , mBurstTxTime(0)
    , mBurstPeriod(0)
    , mBurstCount(0)
    , mBurstError(kErrorAbort)
    , mIsTimedBurst(false)
#endif
#if OPENTHREAD_RADIO
    , mSubMac(aInstance)
#elif OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    , mSubMac(aInstance.Get<SubMac>())
#endif
{
    Init();
}

void LinkRaw::Init(void)
{
#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    CancelBurst();
#endif

    mEnergyScanDoneCallback = nullptr;
    mTransmitDoneCallback   = nullptr;
    mReceiveDoneCallback    = nullptr;

    mReceiveChannel      = OPENTHREAD_CONFIG_DEFAULT_CHANNEL;
    mPanId               = kPanIdBroadcast;
    mReceiveDoneCallback = nullptr;
#if OPENTHREAD_RADIO
    mSubMac.Init();
#endif
}

Error LinkRaw::SetReceiveDone(otLinkRawReceiveDone aCallback)
{
    Error error  = kErrorNone;
    bool  enable = aCallback != nullptr;

    LogDebg("Enabled(%s)", (enable ? "true" : "false"));

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    CancelBurst();
#endif

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    VerifyOrExit(!Get<ThreadNetif>().IsUp(), error = kErrorInvalidState);

    // In MTD/FTD build, `Mac` has already enabled sub-mac. We ensure to
    // disable/enable MAC layer when link-raw is being enabled/disabled to
    // avoid any conflict in control of radio and sub-mac between `Mac` and
    // `LinkRaw`. in RADIO build, we directly enable/disable sub-mac.

    if (!enable)
    {
        // When disabling link-raw, make sure there is no ongoing
        // transmit or scan operation. Otherwise Mac will attempt to
        // handle an unexpected "done" callback.
        VerifyOrExit(!mSubMac.IsTransmittingOrScanning(), error = kErrorBusy);
    }

    Get<Mac>().SetEnabled(!enable);
#else
    if (enable)
    {
        SuccessOrExit(error = mSubMac.Enable());
    }
    else
    {
        IgnoreError(mSubMac.Disable());
    }
#endif

    mReceiveDoneCallback = aCallback;

exit:
    return error;
}

Error LinkRaw::SetPanId(uint16_t aPanId)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    mSubMac.SetPanId(aPanId);
    mPanId = aPanId;

exit:
    return error;
}

Error LinkRaw::SetChannel(uint8_t aChannel)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    mReceiveChannel = aChannel;

exit:
    return error;
}

Error LinkRaw::SetExtAddress(const ExtAddress &aExtAddress)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    mSubMac.SetExtAddress(aExtAddress);

exit:
    return error;
}

Error LinkRaw::SetShortAddress(ShortAddress aShortAddress)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    mSubMac.SetShortAddress(aShortAddress);

exit:
    return error;
}

Error LinkRaw::SetAlternateShortAddress(ShortAddress aShortAddress)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    mSubMac.SetAlternateShortAddress(aShortAddress);

exit:
    return error;
}

Error LinkRaw::Receive(void)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    AbortBurst();
#endif

    SuccessOrExit(error = mSubMac.Receive(mReceiveChannel));

exit:
    return error;
}

Error LinkRaw::Sleep(void)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    if (IsBurstActive())
    {
        AbortBurst();

        // A burst tick may be pending in `SubMac` (e.g., a timed
        // transmission waiting for its target time). Move `SubMac`
        // to sleep so that the pending transmission is dropped.
        ExitNow(error = mSubMac.Sleep());
    }
#endif

    error = Get<Radio::Radio>().Sleep();

exit:
    return error;
}

void LinkRaw::InvokeReceiveDone(RxFrame *aFrame, Error aError)
{
    LogDebg("ReceiveDone(%d bytes), error:%s", (aFrame != nullptr) ? aFrame->mLength : 0, ErrorToString(aError));

    if (mReceiveDoneCallback && (aError == kErrorNone))
    {
        mReceiveDoneCallback(&GetInstance(), aFrame, aError);
    }
}

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
void LinkRaw::CancelBurst(void)
{
    // Clears the burst state without reporting the completion of the
    // burst request.

    mBurstTimer.Stop();
    mBurstChannelMask = 0;
    mBurstTxTime      = 0;
    mBurstPeriod      = 0;
    mBurstCount       = 0;
    mBurstError       = kErrorAbort;
    mIsTimedBurst     = false;
}

void LinkRaw::FinishBurst(Error aError)
{
    // Ends the burst and reports the completion of the burst request.

    CancelBurst();
    InvokeTransmitDoneCallback(mSubMac.GetTransmitFrame(), nullptr, aError);
}

void LinkRaw::AbortBurst(void)
{
    VerifyOrExit(IsBurstActive());
    FinishBurst(kErrorAbort);

exit:
    return;
}

bool LinkRaw::GetNextBurstChannel(uint8_t &aChannel) const
{
    bool found = false;

    for (aChannel++; aChannel <= Radio::kChannelMax; aChannel++)
    {
        if (GetBit(mBurstChannelMask, aChannel))
        {
            ExitNow(found = true);
        }
    }

exit:
    return found;
}

void LinkRaw::SkipMissedBurstSlots(Radio::Time32 aNow)
{
    // Skips all periodic slots whose start time `mBurstTxTime` is not
    // strictly after `aNow`. `mBurstCount` is decremented for every
    // skipped slot. MUST be called only when `mBurstPeriod` is non-zero.

    uint32_t periodUs = static_cast<uint32_t>(mBurstPeriod) * kBurstSlotTimeUs;
    uint32_t missed;

    VerifyOrExit(!Radio::IsTimeStrictlyBefore(aNow, mBurstTxTime));

    missed = (aNow - mBurstTxTime) / periodUs + 1;
    missed = Min<uint32_t>(missed, mBurstCount);

    mBurstCount -= static_cast<uint16_t>(missed);
    mBurstTxTime += missed * periodUs;

exit:
    return;
}

void LinkRaw::ScheduleNextBurstTick(void)
{
    // Called when the current tick is done (all channels sent or the
    // tick is skipped). Determines the next tick on the radio clock and
    // arms `mBurstTimer` to start it.

    Radio::Time32 now = Get<Radio::Radio>().GetNowAsTime32();

    mBurstCount--;

    if (mBurstPeriod == 0)
    {
        // Back-to-back ticks: only the first tick may be timed.
        mIsTimedBurst = false;
    }
    else
    {
        mBurstTxTime += static_cast<uint32_t>(mBurstPeriod) * kBurstSlotTimeUs;
        SkipMissedBurstSlots(now);
    }

    VerifyOrExit(mBurstCount > 0, FinishBurst(mBurstError));

    if ((mBurstPeriod == 0) || mIsTimedBurst)
    {
        // A timed tick is handed to `SubMac` right away which then
        // handles the transmission at its target time.
        mBurstTimer.Start(0);
    }
    else
    {
        mBurstTimer.Start(mBurstTxTime - now);
    }

exit:
    return;
}

void LinkRaw::HandleBurstTimer(void)
{
    TxFrame &frame = mSubMac.GetTransmitFrame();

    VerifyOrExit(IsBurstActive());
    VerifyOrExit(IsEnabled(), FinishBurst(kErrorAbort));

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

    if (mBurstChannelMask != 0)
    {
        uint8_t firstChannel = Radio::kChannelMin - 1;

        IgnoreReturnValue(GetNextBurstChannel(firstChannel));
        frame.mChannel = firstChannel;
    }

    SendBurstFrame();

exit:
    return;
}

void LinkRaw::SendBurstFrame(void)
{
    // Every burst transmission is requested through this method. A
    // request rejected by `SubMac` is handled as a completed
    // transmission that failed (`kErrorAbort`), so that only this
    // transmission is affected and the burst continues.

    Error error = mSubMac.Send();

    VerifyOrExit(error != kErrorNone);

    LogWarnOnError(error, "send burst frame");
    HandleTransmitDone(mSubMac.GetTransmitFrame(), nullptr, kErrorAbort);

exit:
    return;
}

Error LinkRaw::StartBurst(uint16_t aBurstCount, uint16_t aBurstPeriod, uint32_t aBurstChannelMask)
{
    // Validates the burst request and starts the burst. The first tick
    // is sent from `HandleBurstTimer()`, like all subsequent ticks.

    Error              error = kErrorNone;
    TxFrame           &frame = mSubMac.GetTransmitFrame();
    TxFrame::ParseInfo frameInfo;
    Radio::Time32      now;

    // Burst frames are sent repeatedly as-is, so they must not request
    // an ACK and must not require any further processing by the RCP.
    VerifyOrExit(frameInfo.ParseFrom(frame, Frame::kParseAddrFields) == kErrorNone, error = kErrorInvalidArgs);
    VerifyOrExit(!frameInfo.mIsAckRequest, error = kErrorInvalidArgs);
    VerifyOrExit(!frameInfo.mIsSecurityEnabled || frame.IsSecurityProcessed(), error = kErrorInvalidArgs);

    if (aBurstChannelMask != 0)
    {
        uint8_t channel = Radio::kChannelMin - 1;

        mBurstChannelMask = aBurstChannelMask & Radio::kSupportedChannels;
        VerifyOrExit(GetNextBurstChannel(channel), error = kErrorInvalidArgs);
    }

    now           = Get<Radio::Radio>().GetNowAsTime32();
    mBurstCount   = aBurstCount;
    mBurstPeriod  = aBurstPeriod;
    mBurstError   = kErrorAbort;
    mIsTimedBurst = frame.IsTargetTxTimeSpecified();
    mBurstTxTime  = mIsTimedBurst ? frame.GetTargetTxTime() : now;

    if (mIsTimedBurst && !Radio::IsTimeStrictlyBefore(now, mBurstTxTime))
    {
        // The first slot was missed.
        if (mBurstPeriod == 0)
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

    mBurstTimer.Start(0);

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE

Error LinkRaw::Transmit(otLinkRawTransmitDone aCallback,
                        uint16_t              aBurstCount,
                        uint16_t              aBurstPeriod,
                        uint32_t              aBurstChannelMask)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    AbortBurst();

    if (aBurstCount > 0)
    {
        SuccessOrExit(error = StartBurst(aBurstCount, aBurstPeriod, aBurstChannelMask));
        mTransmitDoneCallback = aCallback;
        ExitNow();
    }
#else
    OT_UNUSED_VARIABLE(aBurstPeriod);
    OT_UNUSED_VARIABLE(aBurstChannelMask);
    VerifyOrExit(aBurstCount == 0, error = kErrorNotImplemented);
#endif

    SuccessOrExit(error = mSubMac.Send());
    mTransmitDoneCallback = aCallback;

exit:
#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    if (error != kErrorNone)
    {
        CancelBurst();
    }
#endif

    return error;
}

void LinkRaw::HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError)
{
    LogDebg("TransmitDone(%u bytes), error:%s", aFrame.GetLength(), ErrorToString(aError));

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    if (IsBurstActive())
    {
        uint8_t nextChannel = aFrame.GetChannel();

        // The burst request succeeds if any of its transmissions
        // succeeds. Otherwise the error of the last transmission is
        // reported.
        if (mBurstError != kErrorNone)
        {
            mBurstError = aError;
        }

        if ((mBurstChannelMask != 0) && GetNextBurstChannel(nextChannel))
        {
            aFrame.ClearTargetTxTime();
            aFrame.mChannel = nextChannel;
            SendBurstFrame();
        }
        else
        {
            ScheduleNextBurstTick();
        }

        ExitNow();
    }
#endif

    InvokeTransmitDoneCallback(aFrame, aAckFrame, aError);

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
exit:
    return;
#endif
}

void LinkRaw::InvokeTransmitDoneCallback(TxFrame &aFrame, RxFrame *aAckFrame, Error aError)
{
    otLinkRawTransmitDone callback = mTransmitDoneCallback;

    VerifyOrExit(callback != nullptr);

    mTransmitDoneCallback = nullptr;
    callback(&GetInstance(), &aFrame, aAckFrame, aError);

exit:
    return;
}

Error LinkRaw::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_LINK_RAW_BURST_ENABLE
    AbortBurst();
#endif

    SuccessOrExit(error = mSubMac.EnergyScan(aScanChannel, aScanDuration));
    mEnergyScanDoneCallback = aCallback;

exit:
    return error;
}

void LinkRaw::InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (IsEnabled() && mEnergyScanDoneCallback != nullptr)
    {
        mEnergyScanDoneCallback(&GetInstance(), aEnergyScanMaxRssi);
        mEnergyScanDoneCallback = nullptr;
    }
}

Error LinkRaw::SetMode1MacKeys(uint8_t aKeyIndex, const Key &aPrevKey, const Key &aCurKey, const Key &aNextKey)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);

    mSubMac.SetMode1MacKeys(aKeyIndex, aPrevKey, aCurKey, aNextKey);

exit:
    return error;
}

Error LinkRaw::SetMacFrameCounter(uint32_t aFrameCounter, bool aSetIfLarger)
{
    Error error = kErrorNone;

    VerifyOrExit(IsEnabled(), error = kErrorInvalidState);
    mSubMac.SetFrameCounter(aFrameCounter, aSetIfLarger);

exit:
    return error;
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void LinkRaw::RecordFrameTransmitStatus(const TxFrame::ParseInfo &aFrameInfo,
                                        Error                     aError,
                                        uint8_t                   aRetryCount,
                                        bool                      aWillRetx)
{
    OT_UNUSED_VARIABLE(aWillRetx);

    if (aError != kErrorNone)
    {
        LogInfo("Frame tx failed, error:%s, retries:%d/%d, %s", ErrorToString(aError), aRetryCount,
                aFrameInfo.GetMaxFrameRetries(), aFrameInfo.ToInfoString().AsCString());
    }
}

#endif

// LCOV_EXCL_STOP

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
