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
{
    Init();
}

void SubMac::CslReceiver::Init(void)
{
    mPeriod    = 0;
    mChannel   = 0;
    mPeerShort = 0;
    mParentAccuracy.Init();
    mSampleTime.Clear();
    mCurWindow.Clear();
    mNextWindow.Clear();
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    mLastSync.SetValue(0);
#else
    mLastSync = 0;
#endif
}

void SubMac::CslReceiver::ProcessTxDone(const TxFrame::ParseInfo &aFrameInfo, RxFrame *aAckFrame)
{
    // Actual synchronization timestamp should be from the sent frame instead of the current time.
    // Assuming the error here since it is bounded and has very small effect on the final window duration.

    VerifyOrExit(IsEnabled());
    VerifyOrExit(aAckFrame != nullptr);
    VerifyOrExit(aFrameInfo.mParsedFully);
    VerifyOrExit(aFrameInfo.Has<CslIe>());

    SetLastSyncToNow();
    Schedule();

exit:
    return;
}

void SubMac::CslReceiver::ProcessRxFrame(const RxFrame &aFrame)
{
    VerifyOrExit(IsEnabled());

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    LogReceived(aFrame);
#endif

    // Assuming the risk of the parent missing the Enh-ACK in favor of smaller CSL receive window
    if (aFrame.IsAckedWithSecEnhAck())
    {
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        SetLastSyncToNow();
#else
        mLastSync = aFrame.GetTimestamp();
#endif
        Schedule();
    }

exit:
    return;
}

void SubMac::CslReceiver::SetParams(uint16_t          aPeriod,
                                    uint8_t           aChannel,
                                    ShortAddress      aShortAddr,
                                    const ExtAddress &aExtAddr)
{
    if ((aPeriod == mPeriod) && (aShortAddr == mPeerShort))
    {
        // Handle a CSL channel change only
        VerifyOrExit(mChannel != aChannel);
        mChannel = aChannel;
        VerifyOrExit(IsEnabled());
        Schedule();
        ExitNow();
    }

    mPeriod    = aPeriod;
    mChannel   = aChannel;
    mPeerShort = aShortAddr;

    IgnoreError(Get<Radio::Radio>().EnableCsl(aPeriod, aShortAddr, aExtAddr));

    if (IsEnabled())
    {
        mSampleTime.SetToNow(Get<Radio::Radio>());

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        mLastSync = mSampleTime.GetAsLocalTimeMicro();
#else
        mLastSync = mSampleTime.GetAsTime64();
#endif
    }

    Schedule();

exit:
    return;
}

void SubMac::CslReceiver::Schedule(void)
{
    UpdateCurAndNextWindows();
    Get<SubMac>().mListenScheduler.UpdateOnCslChange();
}

void SubMac::CslReceiver::AdvanceToNextWindow(void)
{
    VerifyOrExit(IsEnabled());

    mSampleTime += CslPeriodToUsec(mPeriod);
    UpdateCurAndNextWindows();

exit:
    return;
}

void SubMac::CslReceiver::UpdateCurAndNextWindows(void)
{
    Radio::SyncedTime nextSampleTime;

    if (!IsEnabled())
    {
        mCurWindow.Clear();
        mNextWindow.Clear();
        ExitNow();
    }

    DetermineWindow(mSampleTime, mCurWindow);

    nextSampleTime = mSampleTime;
    nextSampleTime += CslPeriodToUsec(mPeriod);

    Get<Radio::Radio>().UpdateCslSampleTime(nextSampleTime.GetAsTime32());

    DetermineWindow(nextSampleTime, mNextWindow);

    LogDebg("CSL windows (start, dur) - cur (%lu, %lu), next (%lu, %lu)",
            ToUlong(Radio::ConvertTime64To32(mCurWindow.GetStartTime())), ToUlong(mCurWindow.GetDuration()),
            ToUlong(Radio::ConvertTime64To32(mNextWindow.GetStartTime())), ToUlong(mNextWindow.GetDuration()));

exit:
    return;
}

void SubMac::CslReceiver::DetermineWindow(const Radio::SyncedTime &aSampleTime, TimedRx &aTimedRx) const
{
    /*
     * CSL sample timing diagram:
     *
     *   |<------------------------------------ Sample Window ----------------------------------->|
     *   |                                                                                        |
     *   |<--MinAhead-->|<-- Uncert -->|<-- Drift -->|<-- Drift -->|<-- Uncert -->|<--MinAfter-->|
     *   |              |<---------- Guard --------->|<---------- Guard --------->|               |
     * --|--------------|--------------|-------------|-------------|--------------|---------------|---
     *   ^                                           ^                                            ^
     *   mStartTime                               SampleTime                                  WindowEnd
     *   (SampleTime - ahead)                                                             (SampleTime + after)
     */

    uint32_t halfPeriod    = CslPeriodToUsec(mPeriod) / 2;
    uint32_t guardInterval = 0;
    uint16_t uncertainty;
    uint32_t ahead;
    uint32_t after;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    if (aSampleTime.GetAsLocalTimeMicro() > mLastSync)
    {
        guardInterval = DetermineClockDrift(aSampleTime.GetAsLocalTimeMicro() - mLastSync);
    }
#else
    if (aSampleTime.GetAsTime64() > mLastSync)
    {
        guardInterval = DetermineClockDrift(ClampToUint32(aSampleTime.GetAsTime64() - mLastSync));
    }
#endif

    uncertainty = mParentAccuracy.GetUncertainty() + Get<Radio::Radio>().GetCslUncertainty();
    guardInterval += Radio::ConvertUncertaintyToUsec(uncertainty);

    ahead = Min(guardInterval + kMinReceiveOnAhead, halfPeriod);
    after = Min(guardInterval + kMinReceiveOnAfter, halfPeriod);

    aTimedRx.Init(aSampleTime.GetAsTime64() - ahead, ahead + after, mChannel);
}

uint32_t SubMac::CslReceiver::DetermineClockDrift(uint32_t aIntervalUs) const
{
    uint16_t clockAccuracy = Get<Radio::Radio>().GetCslAccuracy() + mParentAccuracy.GetClockAccuracy();

    return Radio::DetermineClockDrift(clockAccuracy, aIntervalUs);
}

void SubMac::CslReceiver::SetLastSyncToNow(void)
{
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
    mLastSync = TimerMicro::GetNow();
#else
    mLastSync = Get<Radio::Radio>().GetNow();
#endif
}

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
void SubMac::CslReceiver::LogReceived(const RxFrame &aFrame)
{
    RxFrame::ParseInfo frameInfo;
    uint32_t           margin;
    Radio::Time64      timestamp;
    uint32_t           deviation;
    char               signChar;
    LogLevel           logLevel;

    SuccessOrExit(frameInfo.ParseFrom(aFrame, Frame::kParseAddrFields));

    VerifyOrExit(Get<SubMac>().HasAddress(frameInfo.mAddrs.mDestination));

    LogDebg("Received frame in state %s, timestamp %lu", StateToString(Get<SubMac>().mState),
            ToUlong(Radio::ConvertTime64To32(aFrame.GetTimestamp())));

    VerifyOrExit((Get<SubMac>().mState == kStateTimedReceive) || (Get<SubMac>().mState == kStateSleep));

    // The `kMinReceiveOnAhead` is not considered for the margin since
    // it has no impact on understanding possible deviation errors
    // between transmitter and receiver.

    margin = mSampleTime.GetAsTime32() - Radio::ConvertTime64To32(mCurWindow.GetStartTime());
    margin -= Min(margin, kMinReceiveOnAhead);

    timestamp = aFrame.GetTimestamp() + Radio::kHeaderPhrDuration;

    if (timestamp >= mSampleTime.GetAsTime64())
    {
        deviation = ClampToUint32(timestamp - mSampleTime.GetAsTime64());
        signChar  = '+';
    }
    else
    {
        deviation = ClampToUint32(mSampleTime.GetAsTime64() - timestamp);
        signChar  = '-';
    }

    // Treat as a warning when the deviation is not within the allowable margin.

    logLevel = (deviation <= margin) ? kLogLevelDebg : kLogLevelWarn;

    // The log includes three values (all in microseconds):
    // - Absolute sample time at which the CSL receiver expected the MHR
    //   of the received frame.
    // - Allowed margin around that time accounting for clock drift and
    //   uncertainty from both devices.
    // - Real deviation on the reception of the MHR with regards to
    //   expected sample time. This can be due to clock drift and/or
    //   CSL Phase rounding error.

    LogAt(logLevel, "Expected sample time %lu, margin ±%lu, deviation %c%lu", ToUlong(mSampleTime.GetAsTime32()),
          ToUlong(margin), signChar, ToUlong(deviation));

exit:
    return;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
