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
 *   This file implements the Wake-up End Device of the subset of IEEE 802.15.4 MAC primitives.
 */

#include "sub_mac.hpp"

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("SubMac");

void SubMac::WedInit(void)
{
    mIsWedSampling        = false;
    mIsWedEnabled         = false;
    mWakeupListenInterval = 0;
    mWedTimer.Stop();
}

void SubMac::UpdateWakeupListening(bool aEnable, uint32_t aInterval, uint32_t aDuration, uint8_t aChannel)
{
    mWakeupListenInterval = aInterval;
    mWakeupListenDuration = aDuration;
    mWakeupChannel        = aChannel;
    mIsWedSampling        = false;
    mIsWedEnabled         = aEnable;

    mWedTimer.Stop();

    if (aEnable)
    {
        mWedSampleTime.SetToNow(Get<Radio::Radio>());
        mWedSampleTime += kCslReceiveTimeAhead;
        mWedSampleTime -= mWakeupListenInterval;

        HandleWedTimer();
    }
    else if (!RadioSupports(kCapReceiveTiming))
    {
        UpdateRadioSampleState();
    }
}

void SubMac::HandleWedTimer(void)
{
    if (RadioSupports(kCapReceiveTiming))
    {
        HandleWedReceiveAt();
    }
    else
    {
        HandleWedReceiveOrSleep();
    }
}

void SubMac::HandleWedReceiveAt(void)
{
    mWedSampleTime += mWakeupListenInterval;

    mWedTimer.FireAt(mWedSampleTime.GetAsLocalTimeMicro() + mWakeupListenDuration + kWedReceiveTimeAfter);

    if (mState != kStateDisabled)
    {
        IgnoreError(Get<Radio::Radio>().ReceiveAt(mWakeupChannel, mWedSampleTime.GetAsTime32(), mWakeupListenDuration));
    }
}

void SubMac::HandleWedReceiveOrSleep(void)
{
    TimeMilli fireTime;

    mIsWedSampling = !mIsWedSampling;

    if (mIsWedSampling)
    {
        fireTime = mWedSampleTime.GetAsLocalTimeMicro() + mWakeupListenDuration + kMinReceiveOnAfter;
    }
    else
    {
        mWedSampleTime += mWakeupListenInterval;
        fireTime = mWedSampleTime.GetAsLocalTimeMicro() - kMinReceiveOnAhead;
    }

    mWedTimer.FireAt(fireTime);

    UpdateRadioSampleState();
}
} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
