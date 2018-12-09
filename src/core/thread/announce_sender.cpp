/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file implements the AnnounceSender.
 */

#define WPP_NAME "announce_sender.tmh"

#include "announce_sender.hpp"

#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"

namespace ot {

AnnounceSenderBase::AnnounceSenderBase(Instance &aInstance, Timer::Handler aHandler)
    : InstanceLocator(aInstance)
    , mChannelMask()
    , mPeriod(0)
    , mJitter(0)
    , mCount(0)
    , mChannel(0)
    , mTimer(aInstance, aHandler, this)
{
}

otError AnnounceSenderBase::SendAnnounce(Mac::ChannelMask aMask, uint8_t aCount, uint32_t aPeriod, uint16_t aJitter)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aPeriod != 0, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aJitter < aPeriod, error = OT_ERROR_INVALID_ARGS);

    aMask.Intersect(OT_RADIO_SUPPORTED_CHANNELS);
    VerifyOrExit(!aMask.IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    mChannelMask = aMask;
    mCount       = aCount;
    mPeriod      = aPeriod;
    mJitter      = aJitter;
    mChannel     = Mac::ChannelMask::kChannelIteratorFirst;

    mTimer.Start(Random::AddJitter(mPeriod, mJitter));

exit:
    return error;
}

void AnnounceSenderBase::HandleTimer(void)
{
    otError error;

    error = mChannelMask.GetNextChannel(mChannel);

    if (error == OT_ERROR_NOT_FOUND)
    {
        if (mCount != 0)
        {
            mCount--;
            VerifyOrExit(mCount != 0);
        }

        mChannel = Mac::ChannelMask::kChannelIteratorFirst;
        error    = mChannelMask.GetNextChannel(mChannel);
    }

    assert(error == OT_ERROR_NONE);

    GetNetif().GetMle().SendAnnounce(mChannel, false);

    mTimer.Start(Random::AddJitter(mPeriod, mJitter));

exit:
    return;
}

#if OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER

AnnounceSender::AnnounceSender(Instance &aInstance)
    : AnnounceSenderBase(aInstance, &AnnounceSender::HandleTimer)
    , mNotifierCallback(HandleStateChanged, this)
{
    aInstance.GetNotifier().RegisterCallback(mNotifierCallback);
}

void AnnounceSender::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<AnnounceSender>().AnnounceSenderBase::HandleTimer();
}

void AnnounceSender::CheckState(void)
{
    Mle::MleRouter & mle      = GetInstance().Get<Mle::MleRouter>();
    uint32_t         interval = kRouterTxInterval;
    uint32_t         period;
    Mac::ChannelMask channelMask;

    switch (mle.GetRole())
    {
    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        period = kRouterTxInterval;
        break;

    case OT_DEVICE_ROLE_CHILD:
        if (mle.IsRouterRoleEnabled() && (mle.GetDeviceMode() & Mle::ModeTlv::kModeRxOnWhenIdle))
        {
            period = kReedTxInterval;
            break;
        }

        // fall through

    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        Stop();
        ExitNow();
    }

    SuccessOrExit(GetNetif().GetActiveDataset().GetChannelMask(channelMask) == OT_ERROR_NONE, Stop());

    period = interval / channelMask.GetNumberOfChannels();

    if (period < kMinTxPeriod)
    {
        period = kMinTxPeriod;
    }

    VerifyOrExit(!IsRunning() || (period != GetPeriod()) || (GetChannelMask() != channelMask));

    SendAnnounce(channelMask, 0, period, kMaxJitter);

    otLogInfoMle("Starting periodic MLE Announcements tx, period %u, mask %s", period,
                 channelMask.ToString().AsCString());

exit:
    return;
}

void AnnounceSender::Stop(void)
{
    AnnounceSenderBase::Stop();
    otLogInfoMle("Stopping periodic MLE Announcements tx");
}

void AnnounceSender::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<AnnounceSender>().HandleStateChanged(aFlags);
}

void AnnounceSender::HandleStateChanged(otChangedFlags aFlags)
{
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        CheckState();
    }
}

#endif // OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER

} // namespace ot
