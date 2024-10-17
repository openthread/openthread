/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements the CSL receiver of the subset of IEEE 802.15.4 MAC primitives.
 */

#include "sub_mac.hpp"

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("SubMac");

void SubMac::CslInit(void)
{
    mCslPeriod     = 0;
    mCslChannel    = 0;
    mCslPeerShort  = 0;
    mIsCslSampling = false;
    mCslSampleTime = TimeMicro{0};
    mCslLastSync   = TimeMicro{0};
    mCslTimer.Stop();
}

void SubMac::UpdateCslLastSyncTimestamp(TxFrame &aFrame, RxFrame *aAckFrame)
{
    // Actual synchronization timestamp should be from the sent frame instead of the current time.
    // Assuming the error here since it is bounded and has very small effect on the final window duration.
    if (aAckFrame != nullptr && aFrame.HasCslIe())
    {
        mCslLastSync = TimeMicro(GetLocalTime());
    }
}

void SubMac::UpdateCslLastSyncTimestamp(RxFrame *aFrame, Error aError)
{
    VerifyOrExit(aFrame != nullptr && aError == kErrorNone);

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    LogReceived(aFrame);
#endif

    // Assuming the risk of the parent missing the Enh-ACK in favor of smaller CSL receive window
    if ((mCslPeriod > 0) && aFrame->mInfo.mRxInfo.mAckedWithSecEnhAck)
    {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        mCslLastSync = TimerMicro::GetNow();
#else
        mCslLastSync = TimeMicro(static_cast<uint32_t>(aFrame->mInfo.mRxInfo.mTimestamp));
#endif
    }

exit:
    return;
}

void SubMac::CslSample(void)
{
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    VerifyOrExit(!mRadioFilterEnabled, IgnoreError(Get<Radio>().Sleep()));
#endif

    SetState(kStateCslSample);

    if (mIsCslSampling && !RadioSupportsReceiveTiming())
    {
        IgnoreError(Get<Radio>().Receive(mCslChannel));
        ExitNow();
    }

#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    IgnoreError(Get<Radio>().Sleep()); // Don't actually sleep for debugging
#endif

exit:
    return;
}

bool SubMac::UpdateCsl(uint16_t aPeriod, uint8_t aChannel, otShortAddress aShortAddr, const otExtAddress *aExtAddr)
{
    bool diffPeriod  = aPeriod != mCslPeriod;
    bool diffChannel = aChannel != mCslChannel;
    bool diffPeer    = aShortAddr != mCslPeerShort;
    bool retval      = diffPeriod || diffChannel || diffPeer;

    VerifyOrExit(retval);
    mCslChannel = aChannel;

    VerifyOrExit(diffPeriod || diffPeer);
    mCslPeriod    = aPeriod;
    mCslPeerShort = aShortAddr;
    IgnoreError(Get<Radio>().EnableCsl(aPeriod, aShortAddr, aExtAddr));

    mCslTimer.Stop();
    if (mCslPeriod > 0)
    {
        mCslSampleTime = TimeMicro(static_cast<uint32_t>(Get<Radio>().GetNow()));
        mIsCslSampling = false;
        HandleCslTimer();
    }

exit:
    return retval;
}

void SubMac::HandleCslTimer(Timer &aTimer) { aTimer.Get<SubMac>().HandleCslTimer(); }

void SubMac::HandleCslTimer(void)
{
    /*
     * CSL sample timing diagram
     *    |<---------------------------------Sample--------------------------------->|<--------Sleep--------->|
     *    |                                                                          |                        |
     *    |<--Ahead-->|<--UnCert-->|<--Drift-->|<--Drift-->|<--UnCert-->|<--MinWin-->|                        |
     *    |           |            |           |           |            |            |                        |
     * ---|-----------|------------|-----------|-----------|------------|------------|----------//------------|---
     * -timeAhead                           CslPhase                             +timeAfter             -timeAhead
     *
     * The handler works in different ways when the radio supports receive-timing and doesn't.
     *
     * When the radio supports receive-timing:
     *   The handler will be called once per CSL period. When the handler is called, it will set the timer to
     *   fire at the next CSL sample time and call `Radio::ReceiveAt` to start sampling for the current CSL period.
     *   The timer fires some time before the actual sample time. After `Radio::ReceiveAt` is called, the radio will
     *   remain in sleep state until the actual sample time.
     *   Note that it never call `Radio::Sleep` explicitly. The radio will fall into sleep after `ReceiveAt` ends. This
     *   will be done by the platform as part of the `otPlatRadioReceiveAt` API.
     *
     *   Timer fires                                         Timer fires
     *       ^                                                    ^
     *       x-|------------|-------------------------------------x-|------------|---------------------------------------|
     *            sample                   sleep                        sample                    sleep
     *
     * When the radio doesn't support receive-timing:
     *   The handler will be called twice per CSL period: at the beginning of sample and sleep. When the handler is
     *   called, it will explicitly change the radio state due to the current state by calling `Radio::Receive` or
     *   `Radio::Sleep`.
     *
     *   Timer fires  Timer fires                            Timer fires  Timer fires
     *       ^            ^                                       ^            ^
     *       |------------|---------------------------------------|------------|---------------------------------------|
     *          sample                   sleep                        sample                    sleep
     */
    uint32_t periodUs = mCslPeriod * kUsPerTenSymbols;
    uint32_t timeAhead, timeAfter, winStart, winDuration;

    GetCslWindowEdges(timeAhead, timeAfter);

    if (mIsCslSampling)
    {
        mIsCslSampling = false;
        mCslTimer.FireAt(mCslSampleTime - timeAhead);
        if (mState == kStateCslSample)
        {
#if !OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
            IgnoreError(Get<Radio>().Sleep()); // Don't actually sleep for debugging
#endif
            LogDebg("CSL sleep %lu", ToUlong(mCslTimer.GetNow().GetValue()));
        }
    }
    else
    {
        if (RadioSupportsReceiveTiming())
        {
            mCslTimer.FireAt(mCslSampleTime - timeAhead + periodUs);
            timeAhead -= kCslReceiveTimeAhead;
            winStart = mCslSampleTime.GetValue() - timeAhead;
        }
        else
        {
            mCslTimer.FireAt(mCslSampleTime + timeAfter);
            mIsCslSampling = true;
            winStart       = ot::TimerMicro::GetNow().GetValue();
        }

        winDuration = timeAhead + timeAfter;
        mCslSampleTime += periodUs;

        Get<Radio>().UpdateCslSampleTime(mCslSampleTime.GetValue());

        // Schedule reception window for any state except RX - so that CSL RX Window has lower priority
        // than scanning or RX after the data poll.
        if (RadioSupportsReceiveTiming() && (mState != kStateDisabled) && (mState != kStateReceive))
        {
            IgnoreError(Get<Radio>().ReceiveAt(mCslChannel, winStart, winDuration));
        }
        else if (mState == kStateCslSample)
        {
            IgnoreError(Get<Radio>().Receive(mCslChannel));
        }

        LogDebg("CSL window start %lu, duration %lu", ToUlong(winStart), ToUlong(winDuration));
    }
}

void SubMac::GetCslWindowEdges(uint32_t &aAhead, uint32_t &aAfter)
{
    uint32_t semiPeriod = mCslPeriod * kUsPerTenSymbols / 2;
    uint32_t curTime, elapsed, semiWindow;

    curTime = GetLocalTime();
    elapsed = curTime - mCslLastSync.GetValue();

    semiWindow =
        static_cast<uint32_t>(static_cast<uint64_t>(elapsed) *
                              (Get<Radio>().GetCslAccuracy() + mCslParentAccuracy.GetClockAccuracy()) / 1000000);
    semiWindow += mCslParentAccuracy.GetUncertaintyInMicrosec() + Get<Radio>().GetCslUncertainty() * 10;

    aAhead = Min(semiPeriod, semiWindow + kMinReceiveOnAhead + kCslReceiveTimeAhead);
    aAfter = Min(semiPeriod, semiWindow + kMinReceiveOnAfter);
}

uint32_t SubMac::GetLocalTime(void)
{
    uint32_t now;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    now = TimerMicro::GetNow().GetValue();
#else
    now = static_cast<uint32_t>(Get<Radio>().GetNow());
#endif

    return now;
}

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
void SubMac::LogReceived(RxFrame *aFrame)
{
    static constexpr uint8_t kLogStringSize = 72;

    String<kLogStringSize> logString;
    Address                dst;
    int32_t                deviation;
    uint32_t               sampleTime, ahead, after;

    IgnoreError(aFrame->GetDstAddr(dst));

    VerifyOrExit((dst.GetType() == Address::kTypeShort && dst.GetShort() == GetShortAddress()) ||
                 (dst.GetType() == Address::kTypeExtended && dst.GetExtended() == GetExtAddress()));

    LogDebg("Received frame in state (SubMac %s, CSL %s), timestamp %lu", StateToString(mState),
            mIsCslSampling ? "CslSample" : "CslSleep",
            ToUlong(static_cast<uint32_t>(aFrame->mInfo.mRxInfo.mTimestamp)));

    VerifyOrExit(mState == kStateCslSample);

    GetCslWindowEdges(ahead, after);
    ahead -= kMinReceiveOnAhead + kCslReceiveTimeAhead;

    sampleTime = mCslSampleTime.GetValue() - mCslPeriod * kUsPerTenSymbols;
    deviation  = aFrame->mInfo.mRxInfo.mTimestamp + kRadioHeaderPhrDuration - sampleTime;

    // This logs three values (all in microseconds):
    // - Absolute sample time in which the CSL receiver expected the MHR of the received frame.
    // - Allowed margin around that time accounting for accuracy and uncertainty from both devices.
    // - Real deviation on the reception of the MHR with regards to expected sample time. This can
    //   be due to clocks drift and/or CSL Phase rounding error.
    // This means that a deviation absolute value greater than the margin would result in the frame
    // not being received out of the debug mode.
    logString.Append("Expected sample time %lu, margin ±%lu, deviation %ld", ToUlong(sampleTime), ToUlong(ahead),
                     static_cast<long>(deviation));

    // Treat as a warning when the deviation is not within the margins. Neither kCslReceiveTimeAhead
    // or kMinReceiveOnAhead/kMinReceiveOnAfter are considered for the margin since they have no
    // impact on understanding possible deviation errors between transmitter and receiver. So in this
    // case only `ahead` is used, as an allowable max deviation in both +/- directions.
    if ((deviation + ahead > 0) && (deviation < static_cast<int32_t>(ahead)))
    {
        LogDebg("%s", logString.AsCString());
    }
    else
    {
        LogWarn("%s", logString.AsCString());
    }

exit:
    return;
}
#endif

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
