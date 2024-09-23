/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for time instance.
 */

#ifndef TIME_HPP_
#define TIME_HPP_

#include "openthread-core-config.h"

#include <stddef.h>
#include <stdint.h>

#include "common/equatable.hpp"
#include "common/serial_number.hpp"

namespace ot {

/**
 * @addtogroup core-timer
 *
 * @brief
 *   This module includes definitions for the time instance.
 *
 * @{
 */

/**
 * Represents a time instance.
 */
class Time : public Unequatable<Time>
{
public:
    static constexpr uint32_t kOneSecondInMsec = 1000u;                 ///< One second interval in msec.
    static constexpr uint32_t kOneMinuteInMsec = kOneSecondInMsec * 60; ///< One minute interval in msec.
    static constexpr uint32_t kOneHourInMsec   = kOneMinuteInMsec * 60; ///< One hour interval in msec.
    static constexpr uint32_t kOneDayInMsec    = kOneHourInMsec * 24;   ///< One day interval in msec.
    static constexpr uint32_t kOneMsecInUsec   = 1000u;                 ///< One millisecond in microseconds.

    /**
     * This constant defines a maximum time duration ensured to be longer than any other duration.
     */
    static const uint32_t kMaxDuration = ~static_cast<uint32_t>(0UL);

    /**
     * This is the default constructor for a `Time` object.
     */
    Time(void) = default;

    /**
     * Initializes a `Time` object with a given value.
     *
     * @param[in] aValue   The numeric time value to initialize the `Time` object.
     */
    explicit Time(uint32_t aValue) { SetValue(aValue); }

    /**
     * Gets the numeric time value associated with the `Time` object.
     *
     * @returns The numeric `Time` value.
     */
    uint32_t GetValue(void) const { return mValue; }

    /**
     * Sets the numeric time value.
     *
     * @param[in] aValue   The numeric time value.
     */
    void SetValue(uint32_t aValue) { mValue = aValue; }

    /**
     * Calculates the time duration between two `Time` instances.
     *
     * @note Expression `(t1 - t2)` returns the duration of the interval starting from `t2` and ending at `t1`. When
     * calculating the duration, `t2 is assumed to be in the past relative to `t1`. The duration calculation correctly
     * takes into account the wrapping of numeric value of `Time` instances. The returned value can span the entire
     * range of the `uint32_t` type.
     *
     * @param[in]   aOther  A `Time` instance to subtract from.
     *
     * @returns The duration of interval from @p aOther to this `Time` object.
     */
    uint32_t operator-(const Time &aOther) const { return mValue - aOther.mValue; }

    /**
     * Returns a new `Time` which is ahead of this `Time` object by a given duration.
     *
     * @param[in]   aDuration  A duration.
     *
     * @returns A new `Time` which is ahead of this object by @aDuration.
     */
    Time operator+(uint32_t aDuration) const { return Time(mValue + aDuration); }

    /**
     * Returns a new `Time` which is behind this `Time` object by a given duration.
     *
     * @param[in]   aDuration  A duration.
     *
     * @returns A new `Time` which is behind this object by @aDuration.
     */
    Time operator-(uint32_t aDuration) const { return Time(mValue - aDuration); }

    /**
     * Moves this `Time` object forward by a given duration.
     *
     * @param[in]   aDuration  A duration.
     */
    void operator+=(uint32_t aDuration) { mValue += aDuration; }

    /**
     * Moves this `Time` object backward by a given duration.
     *
     * @param[in]   aDuration  A duration.
     */
    void operator-=(uint32_t aDuration) { mValue -= aDuration; }

    /**
     * Indicates whether two `Time` instances are equal.
     *
     * @param[in]   aOther   A `Time` instance to compare with.
     *
     * @retval TRUE    The two `Time` instances are equal.
     * @retval FALSE   The two `Time` instances are not equal.
     */
    bool operator==(const Time &aOther) const { return mValue == aOther.mValue; }

    /**
     * Indicates whether this `Time` instance is strictly before another one.
     *
     * @note The comparison operators correctly take into account the wrapping of `Time` numeric value. For a given
     * `Time` instance `t0`, any `Time` instance `t` where `(t - t0)` is less than half the range of `uint32_t` type
     * is considered to be after `t0`, otherwise it is considered to be before 't0' (or equal to it). As an example
     * to illustrate this model we can use clock hours: If we are at hour 12, hours 1 to 5 are considered to be
     * after 12, and hours 6 to 11 are considered to be before 12.
     *
     * @param[in]   aOther   A `Time` instance to compare with.
     *
     * @retval TRUE    This `Time` instance is strictly before @p aOther.
     * @retval FALSE   This `Time` instance is not strictly before @p aOther.
     */
    bool operator<(const Time &aOther) const { return SerialNumber::IsLess(mValue, aOther.mValue); }

    /**
     * Indicates whether this `Time` instance is after or equal to another one.
     *
     * @param[in]   aOther   A `Time` instance to compare with.
     *
     * @retval TRUE    This `Time` instance is after or equal to @p aOther.
     * @retval FALSE   This `Time` instance is not after or equal to @p aOther.
     */
    bool operator>=(const Time &aOther) const { return !(*this < aOther); }

    /**
     * Indicates whether this `Time` instance is before or equal to another one.
     *
     * @param[in]   aOther   A `Time` instance to compare with.
     *
     * @retval TRUE    This `Time` instance is before or equal to @p aOther.
     * @retval FALSE   This `Time` instance is not before or equal to @p aOther.
     */
    bool operator<=(const Time &aOther) const { return (aOther >= *this); }

    /**
     * Indicates whether this `Time` instance is strictly after another one.
     *
     * @param[in]   aOther   A `Time` instance to compare with.
     *
     * @retval TRUE    This `Time` instance is strictly after @p aOther.
     * @retval FALSE   This `Time` instance is not strictly after @p aOther.
     */
    bool operator>(const Time &aOther) const { return (aOther < *this); }

    /**
     * Returns a new `Time` instance which is in distant future relative to current `Time` object.
     *
     * The distant future is the largest time that is ahead of `Time`. For any time `t`, if `(*this <= t)`, then
     * `t <= this->GetGetDistantFuture()`, except for the ambiguous `t` value which is half range `(1 << 31)` apart.
     *
     * When comparing `GetDistantFuture()` with a time `t` the caller must ensure that `t` is already ahead of `*this`.
     *
     * @returns A new `Time` in distance future relative to current `Time` object.
     */
    Time GetDistantFuture(void) const { return Time(mValue + kDistantInterval); }

    /**
     * Returns a new `Time` instance which is in distant past relative to current `Time` object.
     *
     * The distant past is the smallest time that is before `Time`. For any time `t`, if `(t <= *this )`, then
     * `this->GetGetDistantPast() <= t`, except for the ambiguous `t` value which is half range `(1 << 31)` apart.
     *
     * When comparing `GetDistantPast()` with a time `t` the caller must ensure that the `t` is already before `*this`.
     *
     * @returns A new `Time` in distance past relative to current `Time` object.
     */
    Time GetDistantPast(void) const { return Time(mValue - kDistantInterval); }

    /**
     * Converts a given number of seconds to milliseconds.
     *
     * @param[in] aSeconds   The seconds value to convert to milliseconds.
     *
     * @returns The number of milliseconds.
     */
    static uint32_t constexpr SecToMsec(uint32_t aSeconds) { return aSeconds * 1000u; }

    /**
     * Converts a given number of milliseconds to seconds.
     *
     * @param[in] aMilliseconds  The milliseconds value to convert to seconds.
     *
     * @returns The number of seconds.
     */
    static uint32_t constexpr MsecToSec(uint32_t aMilliseconds) { return aMilliseconds / 1000u; }

private:
    static constexpr uint32_t kDistantInterval = (1UL << 31) - 1;

    uint32_t mValue;
};

/**
 * Represents a time instance (millisecond time).
 */
typedef Time TimeMilli;

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

/**
 * Represents a time instance (microsecond time).
 */
typedef Time TimeMicro;

#endif // OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

/**
 * @}
 */

} // namespace ot

#endif // TIME_HPP_
