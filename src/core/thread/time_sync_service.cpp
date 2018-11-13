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
#include "common/owner-locator.hpp"

#define ABS(value) (((value) >= 0) ? (value) : -(value))

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
    , mTimeSyncCallback(NULL)
    , mTimeSyncCallbackContext(NULL)
    , mNotifierCallback(&TimeSync::HandleStateChanged, this)
    , mTimer(aInstance, HandleTimeout, this)
    , mCurrentStatus(OT_NETWORK_TIME_UNSYNCHRONIZED)
{
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);

    CheckAndHandleChanges(false);
}

otNetworkTimeStatus TimeSync::GetTime(uint64_t &aNetworkTime) const
{
    aNetworkTime = static_cast<uint64_t>(static_cast<int64_t>(otPlatTimeGet()) + mNetworkTimeOffset);

    return mCurrentStatus;
}

void TimeSync::HandleTimeSyncMessage(const Message &aMessage)
{
    const int64_t origNetworkTimeOffset = mNetworkTimeOffset;

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

            // Only notify listeners of an update for network time offset jumps of more than
            // OPENTHREAD_CONFIG_TIME_SYNC_JUMP_NOTIF_MIN_US but notify listeners regardless if the status changes.
            CheckAndHandleChanges(ABS(mNetworkTimeOffset - origNetworkTimeOffset) >=
                                  OPENTHREAD_CONFIG_TIME_SYNC_JUMP_NOTIF_MIN_US);
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

void TimeSync::NotifyTimeSyncCallback(void)
{
    if (mTimeSyncCallback != NULL)
    {
        mTimeSyncCallback(mTimeSyncCallbackContext);
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

void TimeSync::HandleStateChanged(otChangedFlags aFlags)
{
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        CheckAndHandleChanges(false);
    }
}

void TimeSync::HandleTimeout(void)
{
    CheckAndHandleChanges(false);
}

void TimeSync::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<TimeSync>().HandleStateChanged(aFlags);
}

void TimeSync::HandleTimeout(Timer &aTimer)
{
    aTimer.GetOwner<TimeSync>().HandleTimeout();
}

void TimeSync::CheckAndHandleChanges(bool aTimeUpdated)
{
    otNetworkTimeStatus networkTimeStatus       = OT_NETWORK_TIME_SYNCHRONIZED;
    const otDeviceRole  role                    = GetInstance().GetThreadNetif().GetMle().GetRole();
    const uint32_t      resyncNeededThresholdMs = 2 * TimerMilli::SecToMsec(mTimeSyncPeriod);
    const uint32_t      timeSyncLastSyncMs      = TimerMilli::GetNow() - mLastTimeSyncReceived;

    mTimer.Stop();

    switch (role)
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        networkTimeStatus = OT_NETWORK_TIME_UNSYNCHRONIZED;
        break;

    case OT_DEVICE_ROLE_CHILD:
    case OT_DEVICE_ROLE_ROUTER:
        if (mLastTimeSyncReceived == 0)
        {
            // Haven't yet received any time sync
            networkTimeStatus = OT_NETWORK_TIME_UNSYNCHRONIZED;
        }
        else if (timeSyncLastSyncMs > resyncNeededThresholdMs)
        {
            // The device hasn’t received time sync for more than two periods time.
            networkTimeStatus = OT_NETWORK_TIME_RESYNC_NEEDED;
        }
        else
        {
            // Schedule a check 1 millisecond after two periods of time
            assert(resyncNeededThresholdMs >= timeSyncLastSyncMs);
            mTimer.Start(resyncNeededThresholdMs - timeSyncLastSyncMs + 1);
        }
        break;

    case OT_DEVICE_ROLE_LEADER:
        break;
    }

    if (networkTimeStatus != mCurrentStatus || aTimeUpdated)
    {
        mCurrentStatus = networkTimeStatus;

        NotifyTimeSyncCallback();
    }
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
