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
 *   This file implements the Wake Listener (WL) subset of IEEE 802.15.4 MAC primitives.
 */

#include "sub_mac.hpp"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Mac {

RegisterLogModule("SubMac");

void SubMac::WlInit(void)
{
    mIsWlSampling         = false;
    mIsWlEnabled          = false;
    mWakeupListenInterval = 0;
    mWlTimer.Stop();
}

void SubMac::UpdateWakeupListening(bool aEnable, uint32_t aInterval, uint32_t aDuration, uint8_t aChannel)
{
    mWakeupListenInterval = aInterval;
    mWakeupListenDuration = aDuration;
    mWakeupChannel        = aChannel;
    mIsWlSampling         = false;
    mIsWlEnabled          = aEnable;

    mWlTimer.Stop();

    if (aEnable)
    {
        mWlSampleTime      = TimerMicro::GetNow() + kCslReceiveTimeAhead - mWakeupListenInterval;
        mWlSampleTimeRadio = Get<Radio>().GetNow() + kCslReceiveTimeAhead - mWakeupListenInterval;

        HandleWlTimer();
    }
    else if (!RadioSupportsReceiveTiming())
    {
        UpdateRadioSampleState();
    }
}

void SubMac::HandleWlTimer(Timer &aTimer) { aTimer.Get<SubMac>().HandleWlTimer(); }

void SubMac::HandleWlTimer(void)
{
    if (RadioSupportsReceiveTiming())
    {
        HandleWlReceiveAt();
    }
    else
    {
        HandleWlReceiveOrSleep();
    }
}

void SubMac::HandleWlReceiveAt(void)
{
    mWlSampleTime += mWakeupListenInterval;
    mWlSampleTimeRadio += mWakeupListenInterval;
    mWlTimer.FireAt(mWlSampleTime + mWakeupListenDuration + kWlReceiveTimeAfter);

    if (mState != kStateDisabled)
    {
        IgnoreError(
            Get<Radio>().ReceiveAt(mWakeupChannel, static_cast<uint32_t>(mWlSampleTimeRadio), mWakeupListenDuration));
    }
}

void SubMac::HandleWlReceiveOrSleep(void)
{
    TimeMilli fireTime;

    mIsWlSampling = !mIsWlSampling;

    if (mIsWlSampling)
    {
        fireTime = mWlSampleTime + mWakeupListenDuration + kMinReceiveOnAfter;
    }
    else
    {
        mWlSampleTime += mWakeupListenInterval;
        fireTime = mWlSampleTime - kMinReceiveOnAhead;
    }

    mWlTimer.FireAt(fireTime);

    UpdateRadioSampleState();
}
} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
