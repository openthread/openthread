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
 *   This file includes definition for AnnounceSender.
 */

#ifndef ANNOUNCE_SENDER_HPP_
#define ANNOUNCE_SENDER_HPP_

#include "openthread-core-config.h"

#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"

namespace ot {

/**
 * This class implements the base class for an `AnnounceSender` and `AnnounceBeginSever`.
 *
 * This class provides APIs to schedule periodic transmission of MLE Announcement messages for a given number
 * transmissions per channel.
 */
class AnnounceSenderBase : public InstanceLocator
{
protected:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     * @param[in]  aHandler    A timer handler provided by sub-class.
     *
     */
    AnnounceSenderBase(Instance &aInstance, Timer::Handler aHandler);

    /**
     * This method schedules the MLE Announce transmissions.
     *
     * This method schedules `aCount` MLE Announcement transmission cycles. Each cycle covers all the channel in
     * the `aChannelMask`, with `aPeriod` time interval between any two successive MLE Announcement transmissions
     * (possibly) on different channels from the given mask. The `aJitter` can be used to add a random jitter
     * of `[-aJitter, aJitter]` to `aPeriod` interval. A zero value for `aCount` indicates non-stop MLE Announcement
     * transmission cycles.
     *
     * @param[in]  aChannelMask   The channel mask providing the list of channels to use for transmission.
     * @param[in]  aCount         The number of transmissions per channel. Zero indicates non-stop transmissions.
     * @param[in]  aPeriod        The time between two successive MLE Announce transmissions (in milliseconds).
     * @param[in]  aJitter        Maximum random jitter added to @aPeriod per transmission (in milliseconds).
     *
     * @retval OT_ERROR_NONE          Successfully started the transmission process.
     * @retval OT_ERROR_INVALID_ARGS  @p aChanelMask is empty, or @p aPeriod is zero or smaller than @aJitter.
     *
     */
    otError SendAnnounce(Mac::ChannelMask aChannelMask, uint8_t aCount, uint32_t aPeriod, uint16_t aJitter);

    /**
     * This method stops the ongoing MLE Announce transmissions.
     *
     */
    void Stop(void) { mTimer.Stop(); }

    /**
     * This method indicates whether the latest scheduled MLE Announce transmission is currently in progress or is
     * finished.
     *
     * @returns TRUE if the MLE Announce transmission is in progress, FALSE otherwise.
     *
     */
    bool IsRunning(void) const { return mTimer.IsRunning(); }

    /**
     * This method gets the period for the latest scheduled MLE Announce transmission (the one in progress or the last
     * finished one).
     *
     * @returns The period interval (in milliseconds) between two successive MLE Announcement transmissions.
     *
     */
    uint32_t GetPeriod(void) const { return mPeriod; }

    /**
     * This method gets the channel mask for the latest scheduled MLE Announce transmission (the one in progress or the
     * last finished one).
     *
     * @returns A constant reference to channel mask
     *
     */
    const Mac::ChannelMask GetChannelMask(void) const { return mChannelMask; }

    /**
     * This method is the timer handler and must be invoked by sub-class when the timer expires from the `aHandler`
     * callback function provided in the constructor.
     *
     */
    void HandleTimer(void);

private:
    Mac::ChannelMask mChannelMask;
    uint32_t         mPeriod;
    uint16_t         mJitter;
    uint8_t          mCount;
    uint8_t          mChannel;
    TimerMilli       mTimer;
};

#if OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER

/**
 * This class implements an AnnounceSender.
 *
 */
class AnnounceSender : public AnnounceSenderBase
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     *
     */
    AnnounceSender(Instance &aInstance);

private:
    enum
    {
        kRouterTxInterval = OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_ROUTER,
        kReedTxInterval   = OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_REED,
        kMinTxPeriod      = 1000, // in ms
        kMaxJitter        = 500,  // in ms
    };

    void        CheckState(void);
    void        Stop(void);
    static void HandleTimer(Timer &aTimer);
    static void HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags);
    void        HandleStateChanged(otChangedFlags aFlags);

    Notifier::Callback mNotifierCallback;
};

#endif // OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER

/**
 * @}
 */

} // namespace ot

#endif // ANNOUNCE_SENDER_HPP_
