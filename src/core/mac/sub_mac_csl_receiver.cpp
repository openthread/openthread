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
    mCslPeriod    = 0;
    mCslChannel   = 0;
    mCslPeerShort = 0;
    mCslSampleTime.Clear();
    mCslTimer.Stop();
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    mCslLastSync.SetValue(0);
#else
    mCslLastSync = 0;
#endif
}

void SubMac::RestartCslTimerAfterSyncUpdate(void)
{
/*
    if (mCslTimer.IsRunning())
    {
        uint32_t periodUs = mCslPeriod * Radio::kUsPerTenSymbols;

        mCslTimer.Stop();

        // Rewind sample times by one period. HandleCslTimer() will add this
        // period back, effectively re-evaluating the current CSL period's
        // schedule using the updated mCslLastSync.
        mCslSampleTime -= periodUs;

        HandleCslTimer();
    }
*/
}

void SubMac::UpdateCslLastSyncTimestamp(TxFrame &aFrame, RxFrame *aAckFrame)
{
    fprintf(stderr, "ABTIN - SubMac::UpdateCslLastSyncTimestamp(TxFrame, ack:%p, hasCslIe:%d)\n",
            (void *)aAckFrame, aFrame.Has<CslIe>());

    // Actual synchronization timestamp should be from the sent frame instead of the current time.
    // Assuming the error here since it is bounded and has very small effect on the final window duration.
    if (aAckFrame != nullptr && aFrame.Has<CslIe>())
    {
        SetCslLastSyncToNow();
        RestartCslTimerAfterSyncUpdate();
    }
}

void SubMac::UpdateCslLastSyncTimestamp(RxFrame *aFrame, Error aError)
{
    fprintf(stderr, "ABTIN - SubMac::UpdateCslLastSyncTimestamp(RxFrame, err:%s, period:%u, secEnhAck:%d)\n",
            ErrorToString(aError), mCslPeriod, (aFrame ? aFrame->IsAckedWithSecEnhAck() : 0));

    VerifyOrExit(aFrame != nullptr && aError == kErrorNone);

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    LogReceived(aFrame);
#endif

    // Assuming the risk of the parent missing the Enh-ACK in favor of smaller CSL receive window
    if ((mCslPeriod > 0) && aFrame->IsAckedWithSecEnhAck())
    {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        SetCslLastSyncToNow();
#else
        mCslLastSync = aFrame->GetTimestamp();
#endif
        RestartCslTimerAfterSyncUpdate();
    }

exit:
    return;
}

void SubMac::SetCslParams(uint16_t aPeriod, uint8_t aChannel, ShortAddress aShortAddr, const ExtAddress &aExtAddr)
{
    bool diffPeriod  = aPeriod != mCslPeriod;
    bool diffChannel = aChannel != mCslChannel;
    bool diffPeer    = aShortAddr != mCslPeerShort;
    bool retval      = diffPeriod || diffChannel || diffPeer;

    fprintf(stderr, "ABTIN - SubMac::SetCslParams(period:%u, channel:%u)\n", aPeriod, aChannel);

    VerifyOrExit(retval);
    mCslChannel = aChannel;

    VerifyOrExit(diffPeriod || diffPeer);
    mCslPeerShort = aShortAddr;
    IgnoreError(Get<Radio::Radio>().EnableCsl(aPeriod, aShortAddr, aExtAddr));

    mCslPeriod = aPeriod;

    mCslTimer.Stop();

    if (mCslPeriod > 0)
    {
        mCslSampleTime.SetToNow(Get<Radio::Radio>());

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        mCslLastSync = mCslSampleTime.GetAsLocalTimeMicro();
#else
        mCslLastSync = mCslSampleTime.GetAsTime64();
#endif
        HandleCslTimer();
    }
    else
    {
        CancelPendingReceiveAt();
    }

exit:
    return;
}

void SubMac::HandleCslTimer(void)
{
    /*
     *   The handler will be called once per CSL period. When the handler is called, it will set the timer to
     *   fire at the next CSL sample time and call `ReceiveAt` to start sampling for the current CSL period.
     *   The timer fires some time before the next sample time.
     *
     *   Timer fires                                         Timer fires
     *       ^                                                    ^
     *       x-|------------|-------------------------------------x-|------------|---------------------------------------|
     *            sample                   sleep                        sample                    sleep
     */

    uint32_t      periodUs = mCslPeriod * Radio::kUsPerTenSymbols;
    uint32_t      timeAhead, timeAfter;
    Radio::Time64 winStart;
    uint32_t      winDuration;

    fprintf(stderr, "ABTIN - SubMac::HandleCslTimer()\n");


    GetCslWindowEdges(timeAhead, timeAfter);

    mCslTimer.FireAt(mCslSampleTime.GetAsLocalTimeMicro() + periodUs - timeAhead - GetNextCycleDrift());

    winStart    = mCslSampleTime.GetAsTime64() - timeAhead;
    winDuration = timeAhead + timeAfter;

    mCslSampleTime += periodUs;

    Get<Radio::Radio>().UpdateCslSampleTime(mCslSampleTime.GetAsTime32());

    ReceiveAt(winStart, winDuration, mCslChannel);

    LogCslWindow(winStart, winDuration);
}

void SubMac::GetCslWindowEdges(uint32_t &aAhead, uint32_t &aAfter)
{
    /*
     * CSL sample timing diagram
     *    |<---------------------------------Sample--------------------------------->|<--------Sleep--------->|
     *    |                                                                          |                        |
     *    |<--Ahead-->|<--UnCert-->|<--Drift-->|<--Drift-->|<--UnCert-->|<--MinWin-->|                        |
     *    |           |            |           |           |            |            |                        |
     * ---|-----------|------------|-----------|-----------|------------|------------|----------//------------|---
     * -timeAhead                           CslPhase                             +timeAfter             -timeAhead
     */
    uint32_t semiPeriod = mCslPeriod * Radio::kUsPerTenSymbols / 2;
    uint32_t elapsed;
    uint32_t semiWindow;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    elapsed = TimerMicro::GetNow() - mCslLastSync;
#else
    elapsed = ClampToUint32(Get<Radio::Radio>().GetNow() - mCslLastSync);
#endif

    semiWindow = DetermineClockDrift(elapsed);
    semiWindow +=
        Radio::ConvertUncertaintyToUsec(mCslParentAccuracy.GetUncertainty() + Get<Radio::Radio>().GetCslUncertainty());

    aAhead = Min(semiPeriod, semiWindow + kMinReceiveOnAhead + kCslReceiveTimeAhead + 3000);
    aAfter = Min(semiPeriod, semiWindow + kMinReceiveOnAfter);
}

uint32_t SubMac::DetermineClockDrift(uint32_t aIntervalUs) const
{
    uint16_t clockAccuracy = Get<Radio::Radio>().GetCslAccuracy() + mCslParentAccuracy.GetClockAccuracy();

    return Radio::DetermineClockDrift(clockAccuracy, aIntervalUs);
}

uint32_t SubMac::GetNextCycleDrift(void) const
{
    return DetermineClockDrift(static_cast<uint32_t>(mCslPeriod) * Radio::kUsPerTenSymbols);
}

void SubMac::SetCslLastSyncToNow(void)
{
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    mCslLastSync = TimerMicro::GetNow();
#else
    mCslLastSync = Get<Radio::Radio>().GetNow();
#endif
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
void SubMac::LogCslWindow(Radio::Time64 aWinStart, uint32_t aWinDuration)
{
    LogDebg("CSL window start %lu, duration %lu", ToUlong(Radio::ConvertTime64To32(aWinStart)), ToUlong(aWinDuration));
}
#else
void SubMac::LogCslWindow(Radio::Time64, uint32_t) {}
#endif

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

    LogDebg("Received frame in state %s, timestamp %lu", StateToString(mState),
            ToUlong(Radio::ConvertTime64To32(aFrame->GetTimestamp())));

    VerifyOrExit((mState == kStateTimedReceive) || (mState == kStateSleep));

    GetCslWindowEdges(ahead, after);
    ahead -= kMinReceiveOnAhead + kCslReceiveTimeAhead;

    sampleTime = mCslSampleTime.GetAsTime32() - mCslPeriod * Radio::kUsPerTenSymbols;
    deviation  = Radio::ConvertTime64To32(aFrame->GetTimestamp()) + Radio::kHeaderPhrDuration - sampleTime;

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
#endif // OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
