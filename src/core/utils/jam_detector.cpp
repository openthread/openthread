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

#include <openthread/config.h>

#include "jam_detector.hpp"

#include <openthread/openthread.h>
#include <openthread/platform/random.h>

#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_JAM_DETECTION

namespace ot {
namespace Utils {

JamDetector::JamDetector(ThreadNetif &aNetif) :
    ThreadNetifLocator(aNetif),
    mHandler(NULL),
    mContext(NULL),
    mRssiThreshold(kDefaultRssiThreshold),
    mTimer(aNetif.GetInstance(), &JamDetector::HandleTimer, this),
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

otError JamDetector::Start(Handler aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mEnabled, error = OT_ERROR_ALREADY);
    VerifyOrExit(aHandler != NULL, error = OT_ERROR_INVALID_ARGS);

    mHandler = aHandler;
    mContext = aContext;

    mEnabled = true;

    mCurSecondStartTime = TimerMilli::GetNow();
    mAlwaysAboveThreshold = true;
    mHistoryBitmap = 0;
    mJamState = false;
    mSampleInterval = kMaxSampleInterval;

    mTimer.Start(kMinSampleInterval);

exit:
    return error;
}

otError JamDetector::Stop(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_ALREADY);

    mEnabled = false;
    mJamState = false;

    mTimer.Stop();

exit:
    return error;
}

otError JamDetector::SetRssiThreshold(int8_t aThreshold)
{
    mRssiThreshold = aThreshold;

    return OT_ERROR_NONE;
}

otError JamDetector::SetWindow(uint8_t aWindow)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aWindow != 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aWindow <= kMaxWindow, error = OT_ERROR_INVALID_ARGS);

    mWindow = aWindow;

exit:
    return error;
}

otError JamDetector::SetBusyPeriod(uint8_t aBusyPeriod)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aBusyPeriod != 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aBusyPeriod <= mWindow, error = OT_ERROR_INVALID_ARGS);

    mBusyPeriod = aBusyPeriod;

exit:
    return error;
}

void JamDetector::HandleTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleTimer();
}

void JamDetector::HandleTimer(void)
{
    int8_t rssi;
    bool didExceedThreshold = true;

    VerifyOrExit(mEnabled);

    rssi = otPlatRadioGetRssi(&GetInstance());

    // If the RSSI is valid, check if it exceeds the threshold
    // and try to update the history bit map
    if (rssi != OT_RADIO_RSSI_INVALID)
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
    uint32_t now = TimerMilli::GetNow();

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

JamDetector &JamDetector::GetOwner(const Context &aContext)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    JamDetector &detector = *static_cast<JamDetector *>(aContext.GetContext());
#else
    JamDetector &detector = otGetThreadNetif().GetJamDetector();
    OT_UNUSED_VARIABLE(aContext);
#endif
    return detector;
}

}  // namespace Utils
}  // namespace ot

#endif // OPENTHREAD_ENABLE_JAM_DETECTION
