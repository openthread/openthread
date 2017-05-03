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
 *   This file implements the jam detector feature.
 */

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "openthread/openthread.h"
#include "openthread/platform/random.h"

#include <thread/thread_netif.hpp>
#include <common/code_utils.hpp>
#include <utils/jam_detector.hpp>

#if OPENTHREAD_ENABLE_JAM_DETECTION

namespace ot {
namespace Utils {

JamDetector::JamDetector(ThreadNetif &aNetif) :
    mNetif(aNetif),
    mHandler(NULL),
    mContext(NULL),
    mRssiThreshold(kDefaultRssiThreshold),
    mTimer(aNetif.GetIp6().mTimerScheduler, &JamDetector::HandleTimer, this),
    mHistoryBitmap(0),
    mCurSecondStartTime(0),
    mSampleInterval(0),
    mWindow(kMaxWindow),
    mBusyPeriod(kMaxWindow),
    mEnabled(false),
    mAlwaysAboveThreshold(false),
    mJamState(false)
{
}

ThreadError JamDetector::Start(Handler aHandler, void *aContext)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!mEnabled, error = kThreadError_Already);
    VerifyOrExit(aHandler != NULL, error = kThreadError_InvalidArgs);

    mHandler = aHandler;
    mContext = aContext;

    mEnabled = true;

    mCurSecondStartTime = Timer::GetNow();
    mAlwaysAboveThreshold = true;
    mHistoryBitmap = 0;
    mJamState = false;
    mSampleInterval = kMaxSampleInterval;

    mTimer.Start(kMinSampleInterval);

exit:
    return error;
}

ThreadError JamDetector::Stop(void)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mEnabled, error = kThreadError_Already);

    mEnabled = false;
    mJamState = false;

    mTimer.Stop();

exit:
    return error;
}

ThreadError JamDetector::SetRssiThreshold(int8_t aThreshold)
{
    mRssiThreshold = aThreshold;

    return kThreadError_None;
}

ThreadError JamDetector::SetWindow(uint8_t aWindow)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aWindow != 0, error = kThreadError_InvalidArgs);
    VerifyOrExit(aWindow <= kMaxWindow, error = kThreadError_InvalidArgs);

    mWindow = aWindow;

exit:
    return error;
}

ThreadError JamDetector::SetBusyPeriod(uint8_t aBusyPeriod)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aBusyPeriod != 0, error = kThreadError_InvalidArgs);
    VerifyOrExit(aBusyPeriod <= mWindow, error = kThreadError_InvalidArgs);

    mBusyPeriod = aBusyPeriod;

exit:
    return error;
}

void JamDetector::HandleTimer(void *aContext)
{
    static_cast<JamDetector *>(aContext)->HandleTimer();
}

void JamDetector::HandleTimer(void)
{
    int8_t rssi;
    bool didExceedThreshold = true;

    VerifyOrExit(mEnabled);

    rssi = otPlatRadioGetRssi(mNetif.GetInstance());

    // If the RSSI is valid, check if it exceeds the threshold
    // and try to update the history bit map
    if (rssi != kPhyInvalidRssi)
    {
        didExceedThreshold = (rssi >= mRssiThreshold);
        UpdateHistory(didExceedThreshold);
    }

    // If the RSSI sample does not exceed the threshold, go back to max sample interval
    // Otherwise, divide the sample interval by half while ensuring it does not go lower
    // than minimum sample interval.

    if (!didExceedThreshold)
    {
        mSampleInterval = kMaxSampleInterval;
    }
    else
    {
        mSampleInterval /= 2;

        if (mSampleInterval < kMinSampleInterval)
        {
            mSampleInterval = kMinSampleInterval;
        }
    }

    mTimer.Start(mSampleInterval + (otPlatRandomGet() % kMaxRandomDelay));

exit:
    return;
}

void JamDetector::UpdateHistory(bool aDidExceedThreshold)
{
    uint32_t now = Timer::GetNow();

    // If the RSSI is ever below the threshold, update mAlwaysAboveThreshold
    // for current second interval.
    if (!aDidExceedThreshold)
    {
        mAlwaysAboveThreshold = false;
    }

    // If we reached end of current one second interval, update the history bitmap
    if (now - mCurSecondStartTime >= kOneSecondInterval)
    {
        mHistoryBitmap <<= 1;

        if (mAlwaysAboveThreshold)
        {
            mHistoryBitmap |= 0x1;
        }

        mAlwaysAboveThreshold = true;

        while (now - mCurSecondStartTime >= kOneSecondInterval)
        {
            mCurSecondStartTime += kOneSecondInterval;
        }

        UpdateJamState();
    }
}

void JamDetector::UpdateJamState(void)
{
    uint8_t numJammedSeconds = 0;
    uint64_t bitmap = mHistoryBitmap;
    bool oldJamState = mJamState;

    // Clear all history bits beyond the current window size
    bitmap &= (static_cast<uint64_t>(1) << mWindow) - 1;

    // Count the number of bits in the bitmap
    while (bitmap != 0)
    {
        numJammedSeconds++;
        bitmap &= (bitmap - 1);
    }

    // Update the Jam state
    mJamState = (numJammedSeconds >= mBusyPeriod);

    // If there is a change, invoke the handler.
    if ((mJamState != oldJamState) || (mJamState == true))
    {
        mHandler(mJamState, mContext);
    }
}

}  // namespace Utils
}  // namespace ot

#endif // OPENTHREAD_ENABLE_JAM_DETECTION
