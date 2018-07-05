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
 *   This file includes definitions for Thread network time synchronization service.
 */

#ifndef TIME_SYNC_HPP_
#define TIME_SYNC_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
#include <openthread/network_time.h>
#include <openthread/types.h>

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"

namespace ot {

/**
 * This class implements OpenThread Time Synchronization Service.
 *
 */
class TimeSync : public InstanceLocator
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    TimeSync(Instance &aInstance);

    /**
     * Get the Thread network time.
     *
     * @param[inout] aNetworkTime  The Thread network time in microseconds.
     *
     * @returns The time synchronization status.
     *
     */
    otNetworkTimeStatus GetTime(uint64_t &aNetworkTime) const;

    /**
     * Handle the message which includes time synchronization information.
     *
     * @param[in] aMessage  The message which includes time synchronization information.
     *
     */
    void HandleTimeSyncMessage(const Message &aMessage);

#if OPENTHREAD_FTD
    /**
     * Send a time synchronization message when it is required.
     *
     * Note: A time synchronization message is required in following cases:
     *       1. Leader send time sync message periodically;
     *       2. Router(except Leader) received a time sync message with newer sequence;
     *       3. Router received a time sync message with older sequence.
     *
     */
    void ProcessTimeSync(void);
#endif

    /**
     * This method gets the time synchronization sequence.
     *
     * @returns The time synchronization sequence.
     *
     */
    uint8_t GetTimeSyncSeq(void) const { return mTimeSyncSeq; };

    /**
     * This method gets the time offset to the Thread network time.
     *
     * @returns The time offset to the Thread network time, in microseconds.
     *
     */
    int64_t GetNetworkTimeOffset(void) const { return mNetworkTimeOffset; };

    /**
     * Set the time synchronization period.
     *
     * @param[in] aTimeSyncPeriod   The time synchronization period, in seconds.
     *
     */
    void SetTimeSyncPeriod(uint16_t aTimeSyncPeriod) { mTimeSyncPeriod = aTimeSyncPeriod; }

    /**
     * Get the time synchronization period.
     *
     * @returns The time synchronization period, in seconds.
     *
     */
    uint16_t GetTimeSyncPeriod(void) const { return mTimeSyncPeriod; }

    /**
     * Set the time synchronization XTAL accuracy threshold for Router.
     *
     * @param[in] aXtalThreshold   The XTAL accuracy threshold for Router, in PPM.
     *
     */
    void SetXtalThreshold(uint16_t aXtalThreshold) { mXtalThreshold = aXtalThreshold; }

    /**
     * Get the time synchronization XTAL accuracy threshold for Router.
     *
     * @returns The XTAL accuracy threshold for Router, in PPM.
     *
     */
    uint16_t GetXtalThreshold(void) const { return mXtalThreshold; }

private:
    /**
     * Increase the time synchronization sequence.
     *
     */
    void IncrementTimeSyncSeq(void);

    bool     mTimeSyncRequired;     ///< Indicate whether or not a time synchronization message is required.
    uint8_t  mTimeSyncSeq;          ///< The time synchronization sequence.
    uint16_t mTimeSyncPeriod;       ///< The time synchronization period.
    uint16_t mXtalThreshold;        ///< The XTAL accuracy threshold for a device to become a Router, in PPM.
    uint32_t mLastTimeSyncSent;     ///< The time when the last time synchronization message was sent.
    uint32_t mLastTimeSyncReceived; ///< The time when the last time synchronization message was received.
    int64_t  mNetworkTimeOffset;    ///< The time offset to the Thread Network time
};

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

#endif // TIME_SYNC_HPP_
