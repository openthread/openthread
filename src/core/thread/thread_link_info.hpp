/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions for link-specific information for messages received from the Thread radio.
 */

#ifndef THREAD_LINK_INFO_HPP_
#define THREAD_LINK_INFO_HPP_

#include "openthread-core-config.h"

#include <openthread/message.h>

#include "common/clearable.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_types.hpp"

namespace ot {

/**
 * Represents link-specific information for messages received from the Thread radio.
 */
class ThreadLinkInfo : public otThreadLinkInfo, public Clearable<ThreadLinkInfo>
{
public:
    /**
     * Returns the IEEE 802.15.4 Source PAN ID.
     *
     * @returns The IEEE 802.15.4 Source PAN ID.
     */
    Mac::PanId GetPanId(void) const { return mPanId; }

    /**
     * Returns the IEEE 802.15.4 Channel.
     *
     * @returns The IEEE 802.15.4 Channel.
     */
    uint8_t GetChannel(void) const { return mChannel; }

    /**
     * Returns whether the Destination PAN ID is broadcast.
     *
     * @retval TRUE   If Destination PAN ID is broadcast.
     * @retval FALSE  If Destination PAN ID is not broadcast.
     */
    bool IsDstPanIdBroadcast(void) const { return mIsDstPanIdBroadcast; }

    /**
     * Indicates whether or not link security is enabled.
     *
     * @retval TRUE   If link security is enabled.
     * @retval FALSE  If link security is not enabled.
     */
    bool IsLinkSecurityEnabled(void) const { return mLinkSecurity; }

    /**
     * Returns the Received Signal Strength (RSS) in dBm.
     *
     * @returns The Received Signal Strength (RSS) in dBm.
     */
    int8_t GetRss(void) const { return mRss; }

    /**
     * Returns the frame/radio Link Quality Indicator (LQI) value.
     *
     * @returns The Link Quality Indicator value.
     */
    uint8_t GetLqi(void) const { return mLqi; }

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Returns the Time Sync Sequence.
     *
     * @returns The Time Sync Sequence.
     */
    uint8_t GetTimeSyncSeq(void) const { return mTimeSyncSeq; }

    /**
     * Returns the time offset to the Thread network time (in microseconds).
     *
     * @returns The time offset to the Thread network time (in microseconds).
     */
    int64_t GetNetworkTimeOffset(void) const { return mNetworkTimeOffset; }
#endif

    /**
     * Sets the `ThreadLinkInfo` from a given received frame.
     *
     * @param[in] aFrame  A received frame.
     */
    void SetFrom(const Mac::RxFrame &aFrame);
};

DefineCoreType(otThreadLinkInfo, ThreadLinkInfo);

} // namespace ot

#endif // THREAD_LINK_INFO_HPP_
