/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for Thread Direct Local Radio Availability.
 */

#ifndef OT_CORE_THREAD_DIRECT_TD_LOCAL_RADIO_AVAIL_HPP_
#define OT_CORE_THREAD_DIRECT_TD_LOCAL_RADIO_AVAIL_HPP_

#include "openthread-core-config.h"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "radio/radio.hpp"
#include "thread_direct/td_types.hpp"

namespace ot {
namespace ThreadDirect {

/**
 * Tracks and caches the local radio availability and preferred slots.
 */
class LocalRadioAvailability : public InstanceLocator, private NonCopyable
{
    friend class ot::Instance;
    friend class ot::Radio::Callbacks;

public:
    /**
     * Initializes the `LocalRadioAvailability` object.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    explicit LocalRadioAvailability(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
        ClearAllBytes(mPreferredBitmap);
    }

    /**
     * Gets the cached local radio availability schedule.
     *
     * @returns A reference to the cached `RadioAvailability` schedule.
     */
    const RadioAvailability &GetAvailability(void) const { return mAvailability; }

    /**
     * Gets the preferred slots bitmap buffer.
     *
     * @returns A pointer to the preferred slots bitmap byte array.
     */
    const uint8_t *GetPreferredBitmap(void) const { return mPreferredBitmap; }

    /**
     * Indicates whether the radio is in a preferred slot at a given time.
     *
     * @param[in] aTime  The radio time in microseconds to check.
     *
     * @retval TRUE   If the radio is in a preferred slot at @p aTime (or no schedule is specified).
     * @retval FALSE  If the radio is not in a preferred slot at @p aTime.
     */
    bool IsPreferredAt(Radio::Time64 aTime) const;

private:
    void AfterInit(void) { Update(); }
    void HandleRadioAvailabilityUpdated(void) { Update(); }
    void Update(void);
    bool IsPreferred(uint8_t aBitIndex) const;

    RadioAvailability mAvailability;
    uint8_t           mPreferredBitmap[RadioAvailability::kMaxBitmapSize];
};

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE

#endif // OT_CORE_THREAD_DIRECT_TD_LOCAL_RADIO_AVAIL_HPP_
