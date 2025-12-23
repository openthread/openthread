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

void SubMac::UpdateWakeupListening(bool aEnable, uint32_t aInterval, uint32_t aDuration, uint8_t aChannel)
{
    if (aEnable)
    {
        mSampleScheduler.StartWedSample(aChannel, aDuration, aInterval);
    }
    else
    {
        mSampleScheduler.StopWedSample();
    }
}

void SubMac::GetWedWindowEdges(uint32_t &aAhead, uint32_t &aAfter)
{
    aAhead = kMinReceiveOnAhead + kCslReceiveTimeAhead;
    aAfter = kMinReceiveOnAfter;
}

//-------------------------------------------------------------------------

#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
void SubMac::SetECslParams(uint16_t aDuration, uint16_t aPeriod, uint8_t aChannel)
{
    bool diffPeriod  = aPeriod != mSampleScheduler.GetECslScheduler().mPeriod / kECslSlotSize;
    bool diffChannel = aChannel != mSampleScheduler.GetECslScheduler().mChannel;

    LogInfo("SetECslParams() Enabled:%u, period(%lu, %lu), channel(%u,%u)", (aPeriod > 0),
            ToUlong(mSampleScheduler.GetECslScheduler().mPeriod), ToUlong(aPeriod * kECslSlotSize),
            mSampleScheduler.GetECslScheduler().mChannel, aChannel);

    VerifyOrExit(diffPeriod || diffChannel);

    Get<Radio>().SetECslPeriod(aPeriod);

    if (aPeriod > 0)
    {
        mSampleScheduler.StartECslSample(aChannel, aDuration, aPeriod * kECslSlotSize);
    }
    else
    {
        mSampleScheduler.StopECslSample();
    }

exit:
    return;
}

Error SubMac::AddECslPeerAddress(const ExtAddress &aExtAddr) { return Get<Radio>().AddECslPeerAddress(aExtAddr); }

Error SubMac::ClearECslPeerAddress(const ExtAddress &aExtAddr) { return Get<Radio>().ClearECslPeerAddress(aExtAddr); }

void SubMac::ClearECslPeerAddresses(void) { Get<Radio>().ClearECslPeerAddresses(); }

void SubMac::GetECslWindowEdges(uint32_t &aAhead, uint32_t &aAfter)
{
    aAhead = kMinReceiveOnAhead + kCslReceiveTimeAhead;
    aAfter = kMinReceiveOnAfter;
}
#endif

//-------------------------------------------------------------------------

Error SubMac::AddWakeupId(WakeupId aWakeupId)
{
    Error error = kErrorNone;

    VerifyOrExit(!mWakeupIdTable.IsFull(), error = kErrorNoBufs);
    error = mWakeupIdTable.PushBack(aWakeupId);
exit:
    return error;
}

Error SubMac::RemoveWakeupId(WakeupId aWakeupId)
{
    Error     error = kErrorNone;
    WakeupId *wakeupId;

    wakeupId = mWakeupIdTable.Find(aWakeupId);
    VerifyOrExit(wakeupId != nullptr, error = kErrorNotFound);
    mWakeupIdTable.Remove(*wakeupId);

exit:
    return error;
}

void SubMac::ClearWakeupIds(void) { mWakeupIdTable.Clear(); }

bool SubMac::ShouldHandleWakeupFrame(const RxFrame &aFrame)
{
    bool                ret = false;
    Address             dstAddr;
    WakeupId            wakeupId;
    const ConnectionIe *connectionIe;

    VerifyOrExit(mSampleScheduler.GetWedScheduler().mIsEnabled);

    SuccessOrExit(aFrame.GetDstAddr(dstAddr));
    VerifyOrExit(dstAddr.IsBroadcast(), ret = true);

    VerifyOrExit((connectionIe = aFrame.GetConnectionIe()) != nullptr);
    SuccessOrExit(connectionIe->GetWakeupId(wakeupId));
    VerifyOrExit(mWakeupIdTable.Contains(wakeupId));
    ret = true;

exit:
    return ret;
}

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
