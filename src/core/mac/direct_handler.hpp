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
 *   This file includes definitions for the Thread Direct local SCA state manager.
 */

#ifndef OT_CORE_MAC_DIRECT_HANDLER_HPP_
#define OT_CORE_MAC_DIRECT_HANDLER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_header_ie.hpp"

namespace ot {

/**
 * Manages the local SCA state (SLW schedule and RAM parameters) advertised in
 * outgoing SCA LTVs.
 */
class DirectHandler : public InstanceLocator, private NonCopyable
{
public:
    explicit DirectHandler(Instance &aInstance);

    /**
     * Sets the SLW (Scheduled Listen Window) period advertised by this device.
     *
     * Phase is stack-computed at frame-build time and is not configurable here.
     *
     * @param[in] aPeriodSlots  SLW period in 160 us slots.  Pass 0 to clear the schedule.
     *
     * @retval kErrorNone         Schedule stored.
     * @retval kErrorInvalidArgs  @p aPeriodSlots is non-zero and below the configured minimum.
     */
    Error SetSlwSchedule(uint16_t aPeriodSlots);

    /**
     * Gets the SLW (Scheduled Listen Window) period configured on this device.
     *
     * @param[out] aPeriodSlots  Set to the SLW period in 160 us slots (0 if not configured).
     */
    void GetSlwSchedule(uint16_t &aPeriodSlots) const;

    /**
     * Sets the RAM (Radio Availability Mask) parameters advertised by this device.
     *
     * @param[in] aParams  RAM parameters to store.
     *
     * @retval kErrorNone         Parameters stored.
     * @retval kErrorInvalidArgs  @p aParams.mRamDuration is outside [0, 31] or
     *                            @p aParams.mRamOffsetUs is outside [-1024, 1023].
     */
    Error SetRamMask(const Mac::ScaParams &aParams);

    /**
     * Gets the RAM (Radio Availability Mask) parameters configured on this device.
     *
     * @param[out] aParams  Set to the RAM parameters.
     */
    void GetRamMask(Mac::ScaParams &aParams) const;

    /**
     * Gets the local SCA state (SLW schedule and RAM parameters).
     *
     * @returns A const reference to the local SCA parameters.
     */
    const Mac::ScaParams &GetLocalSca(void) const { return mLocalSca; }

    /**
     * Returns the SLW link inactivity timeout in seconds.
     *
     * @returns Current SLW timeout (seconds).
     */
    uint32_t GetSlwTimeout(void) const { return mSlwTimeout; }

    /**
     * Sets the SLW link inactivity timeout in seconds.
     *
     * @param[in] aTimeout  Timeout in seconds; 0 restores the compile-time default.
     *
     * @retval kErrorNone         Timeout stored.
     * @retval kErrorInvalidArgs  @p aTimeout exceeds kMaxSlwTimeout.
     */
    Error SetSlwTimeout(uint32_t aTimeout);

private:
    static constexpr uint32_t kDefaultSlwTimeout = OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_TIMEOUT;
    static constexpr uint32_t kMaxSlwTimeout     = OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MAX_TIMEOUT;

    Mac::ScaParams mLocalSca;
    uint32_t       mSlwTimeout;
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#endif // OT_CORE_MAC_DIRECT_HANDLER_HPP_
