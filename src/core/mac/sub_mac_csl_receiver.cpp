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

SubMac::CslReceiver::CslReceiver(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
    mParentAccuracy.Init();
    Init();
}

void SubMac::CslReceiver::Init(void)
{
    mPeriod    = 0;
    mChannel   = 0;
    mPeerShort = 0;
    mSampleTime.Clear();
    mTimer.Stop();
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    mLastSync.SetValue(0);
#else
    mLastSync = 0;
#endif
}

void SubMac::CslReceiver::RestartTimerAfterSyncUpdate(void)
{
    if (mTimer.IsRunning())
    {
        uint32_t periodUs = CslPeriodToUsec(mPeriod);

        mTimer.Stop();

        // Rewind sample times by one period. HandleTimer() will add this
        // period back, effectively re-evaluating the current CSL period's
        // schedule using the updated mLastSync.
        mSampleTime -= periodUs;

        HandleTimer();
    }
}

void SubMac::CslReceiver::UpdateLastSyncTimestamp(const TxFrame::ParseInfo &aFrameInfo, RxFrame *aAckFrame)
{
    // Actual synchronization timestamp should be from the sent frame instead of the current time.
    // Assuming the error here since it is bounded and has very small effect on the final window duration.

    VerifyOrExit(aAckFrame != nullptr);
    VerifyOrExit(aFrameInfo.mParsedFully);
    VerifyOrExit(aFrameInfo.Has<CslIe>());

    SetLastSyncToNow();
    RestartTimerAfterSyncUpdate();

exit:
    return;
}

void SubMac::CslReceiver::UpdateLastSyncTimestamp(RxFrame *aFrame, Error aError)
{
    VerifyOrExit(aFrame != nullptr && aError == kErrorNone);

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    LogReceived(aFrame);
#endif

    // Assuming the risk of the parent missing the Enh-ACK in favor of smaller CSL receive window
    if ((mPeriod > 0) && aFrame->IsAckedWithSecEnhAck())
    {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        SetLastSyncToNow();
#else
        mLastSync = aFrame->GetTimestamp();
#endif
        RestartTimerAfterSyncUpdate();
    }

exit:
    return;
}

void SubMac::CslReceiver::SetParams(uint16_t          aPeriod,
                                    uint8_t           aChannel,
                                    ShortAddress      aShortAddr,
                                    const ExtAddress &aExtAddr)
{
    mChannel = aChannel;

    VerifyOrExit((aPeriod != mPeriod) || (aShortAddr != mPeerShort));

    mPeerShort = aShortAddr;
    IgnoreError(Get<Radio::Radio>().EnableCsl(aPeriod, aShortAddr, aExtAddr));

    mPeriod = aPeriod;

    mTimer.Stop();

    if (mPeriod > 0)
    {
        mSampleTime.SetToNow(Get<Radio::Radio>());

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        mLastSync = mSampleTime.GetAsLocalTimeMicro();
#else
        mLastSync = mSampleTime.GetAsTime64();
#endif
        HandleTimer();
    }
    else
    {
        Get<SubMac>().CancelPendingReceiveAt();
    }

exit:
    return;
}

void SubMac::CslReceiver::HandleTimer(void)
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
    uint32_t      periodUs = CslPeriodToUsec(mPeriod);
    uint32_t      timeAhead, timeAfter;
    Radio::Time64 winStart;
    uint32_t      winDuration;

    GetWindowEdges(timeAhead, timeAfter);

    mTimer.FireAt(mSampleTime.GetAsLocalTimeMicro() + periodUs - timeAhead - kReceiveTimeAhead - GetNextCycleDrift());

    winStart    = mSampleTime.GetAsTime64() - timeAhead;
    winDuration = timeAhead + timeAfter;

    mSampleTime += periodUs;

    Get<Radio::Radio>().UpdateCslSampleTime(mSampleTime.GetAsTime32());

    Get<SubMac>().ReceiveAt(winStart, winDuration, mChannel);

    LogWindow(winStart, winDuration);
}

void SubMac::CslReceiver::GetWindowEdges(uint32_t &aAhead, uint32_t &aAfter)
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
    uint32_t semiPeriod = CslPeriodToUsec(mPeriod) / 2;
    uint32_t elapsed    = 0;
    uint32_t semiWindow;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    if (mSampleTime.GetAsLocalTimeMicro() > mLastSync)
    {
        elapsed = mSampleTime.GetAsLocalTimeMicro() - mLastSync;
    }
#else
    if (mSampleTime.GetAsTime64() > mLastSync)
    {
        elapsed = ClampToUint32(mSampleTime.GetAsTime64() - mLastSync);
    }
#endif

    semiWindow = DetermineClockDrift(elapsed);
    semiWindow +=
        Radio::ConvertUncertaintyToUsec(mParentAccuracy.GetUncertainty() + Get<Radio::Radio>().GetCslUncertainty());

    aAhead = Min(semiPeriod, semiWindow + kMinReceiveOnAhead);
    aAfter = Min(semiPeriod, semiWindow + kMinReceiveOnAfter);
}

uint32_t SubMac::CslReceiver::DetermineClockDrift(uint32_t aIntervalUs) const
{
    uint16_t clockAccuracy = Get<Radio::Radio>().GetCslAccuracy() + mParentAccuracy.GetClockAccuracy();

    return Radio::DetermineClockDrift(clockAccuracy, aIntervalUs);
}

uint32_t SubMac::CslReceiver::GetNextCycleDrift(void) const { return DetermineClockDrift(CslPeriodToUsec(mPeriod)); }

void SubMac::CslReceiver::SetLastSyncToNow(void)
{
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    mLastSync = TimerMicro::GetNow();
#else
    mLastSync = Get<Radio::Radio>().GetNow();
#endif
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
void SubMac::CslReceiver::LogWindow(Radio::Time64 aWinStart, uint32_t aWinDuration)
{
    LogDebg("CSL window start %lu, duration %lu", ToUlong(Radio::ConvertTime64To32(aWinStart)), ToUlong(aWinDuration));
}
#else
void SubMac::CslReceiver::LogWindow(Radio::Time64, uint32_t) {}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
void SubMac::CslReceiver::LogReceived(RxFrame *aFrame)
{
    static constexpr uint8_t kLogStringSize = 72;

    String<kLogStringSize>  logString;
    Mac::RxFrame::ParseInfo frameInfo;
    int32_t                 deviation;
    uint32_t                sampleTime, ahead, after;

    IgnoreError(frameInfo.ParseFrom(*aFrame, Mac::Frame::kParseAddrFields));

    VerifyOrExit((frameInfo.mAddrs.mDestination.IsShort() &&
                  frameInfo.mAddrs.mDestination.GetShort() == Get<SubMac>().GetShortAddress()) ||
                 (frameInfo.mAddrs.mDestination.IsExtended() &&
                  frameInfo.mAddrs.mDestination.GetExtended() == Get<SubMac>().GetExtAddress()));

    LogDebg("Received frame in state %s, timestamp %lu", StateToString(Get<SubMac>().mState),
            ToUlong(Radio::ConvertTime64To32(aFrame->GetTimestamp())));

    VerifyOrExit((Get<SubMac>().mState == kStateTimedReceive) || (Get<SubMac>().mState == kStateSleep));

    GetWindowEdges(ahead, after);
    ahead -= kMinReceiveOnAhead;

    sampleTime = mSampleTime.GetAsTime32() - CslPeriodToUsec(mPeriod);
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

    // Treat as a warning when the deviation is not within the margins. Neither kReceiveTimeAhead
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
