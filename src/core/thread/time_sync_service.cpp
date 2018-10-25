/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements OpenThread Time Synchronization Service.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

#include "time_sync_service.hpp"

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/time.h>

#include "common/instance.hpp"
#include "common/logging.hpp"

namespace ot {

TimeSync::TimeSync(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimeSyncRequired(false)
    , mTimeSyncSeq(OT_TIME_SYNC_INVALID_SEQ)
    , mTimeSyncPeriod(OPENTHREAD_CONFIG_TIME_SYNC_PERIOD)
    , mXtalThreshold(OPENTHREAD_CONFIG_TIME_SYNC_XTAL_THRESHOLD)
#if OPENTHREAD_FTD
    , mLastTimeSyncSent(0)
#endif
    , mLastTimeSyncReceived(0)
    , mNetworkTimeOffset(0)
{
}

otNetworkTimeStatus TimeSync::GetTime(uint64_t &aNetworkTime) const
{
    otNetworkTimeStatus networkTimeStatus = OT_NETWORK_TIME_SYNCHRONIZED;
    otDeviceRole        role              = GetInstance().GetThreadNetif().GetMle().GetRole();

    switch (role)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        networkTimeStatus = OT_NETWORK_TIME_UNSYNCHRONIZED;
        break;

    case OT_DEVICE_ROLE_CHILD:
    case OT_DEVICE_ROLE_ROUTER:
        if ((TimerMilli::GetNow() - mLastTimeSyncReceived) > 2 * TimerMilli::SecToMsec(mTimeSyncPeriod))
        {
            // The device hasn’t received time sync for more than two periods time.
            networkTimeStatus = OT_NETWORK_TIME_RESYNC_NEEDED;
        }
        break;

    case OT_DEVICE_ROLE_LEADER:
        break;
    }

    aNetworkTime = static_cast<uint64_t>(static_cast<int64_t>(otPlatTimeGet()) + mNetworkTimeOffset);

    return networkTimeStatus;
}

void TimeSync::HandleTimeSyncMessage(const Message &aMessage)
{
    VerifyOrExit(aMessage.GetTimeSyncSeq() != OT_TIME_SYNC_INVALID_SEQ);

    if (mTimeSyncSeq != OT_TIME_SYNC_INVALID_SEQ && (int8_t)(aMessage.GetTimeSyncSeq() - mTimeSyncSeq) < 0)
    {
        // receive older time sync sequence.
        mTimeSyncRequired = true;
    }
    else if (GetInstance().GetThreadNetif().GetMle().GetRole() != OT_DEVICE_ROLE_LEADER)
    {
        // Update network time in following three cases:
        //  1. during first attach;
        //  2. already attached, receive newer time sync sequence;
        //  3. during reattach or migration process.
        if (mTimeSyncSeq == OT_TIME_SYNC_INVALID_SEQ || (int8_t)(aMessage.GetTimeSyncSeq() - mTimeSyncSeq) > 0 ||
            GetInstance().GetThreadNetif().GetMle().GetRole() == OT_DEVICE_ROLE_DETACHED)
        {
            // update network time and forward it.
            mLastTimeSyncReceived = TimerMilli::GetNow();
            mTimeSyncSeq          = aMessage.GetTimeSyncSeq();
            mNetworkTimeOffset    = aMessage.GetNetworkTimeOffset();
            mTimeSyncRequired     = true;
        }
    }

exit:
    return;
}

void TimeSync::IncrementTimeSyncSeq(void)
{
    if (++mTimeSyncSeq == OT_TIME_SYNC_INVALID_SEQ)
    {
        ++mTimeSyncSeq;
    }
}

#if OPENTHREAD_FTD
void TimeSync::ProcessTimeSync(void)
{
    if (GetInstance().GetThreadNetif().GetMle().GetRole() == OT_DEVICE_ROLE_LEADER &&
        TimerMilli::GetNow() - mLastTimeSyncSent > TimerMilli::SecToMsec(mTimeSyncPeriod))
    {
        IncrementTimeSyncSeq();
        mTimeSyncRequired = true;
    }

    if (mTimeSyncRequired)
    {
        VerifyOrExit(GetInstance().GetThreadNetif().GetMle().SendTimeSync() == OT_ERROR_NONE);

        mLastTimeSyncSent = TimerMilli::GetNow();
        mTimeSyncRequired = false;
    }

exit:
    return;
}
#endif // OPENTHREAD_FTD

} // namespace ot

#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
