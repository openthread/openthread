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
 *   This file includes definitions for OpenThread radio types.
 */

#ifndef OT_CORE_RADIO_RADIO_TYPES_HPP_
#define OT_CORE_RADIO_RADIO_TYPES_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>

#include "common/clearable.hpp"
#include "common/string.hpp"
#include "common/time.hpp"

namespace ot {
namespace Radio {

#ifdef OT_CONFIG_RADIO_TIME_ENABLE
#error "OT_CONFIG_RADIO_TIME_ENABLE MUST NOT be defined directly. It is derived from other configs"
#endif

#define OT_CONFIG_RADIO_TIME_ENABLE                                                               \
    (OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE || \
     OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE || OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || \
     OPENTHREAD_CONFIG_TIME_SYNC_ENABLE)

class Radio;

/**
 * Represents a 64-bit radio time in microseconds referenced to a continuous monotonic local radio clock.
 */
typedef otRadioTime64 Time64;

/**
 * Represents a 32-bit radio time in microseconds (holds the lower 32 bits of a `Radio::Time64`).
 */
typedef otRadioTime32 Time32;

/**
 * Converts a 64-bit radio time to a 32-bit radio time.
 *
 * @param[in] aTime64  The 64-bit radio time to convert.
 *
 * @returns The converted 32-bit radio time (lower 32 bits of @p aTime64).
 */
inline Time32 ConvertTime64To32(Time64 aTime64) { return static_cast<Time32>(aTime64); }

/**
 * Indicates whether a given 32-bit radio time is strictly before another 32-bit radio time.
 *
 * This function correctly accounts for 32-bit microsecond counter roll-over.
 *
 * @param[in] aFirstTime   The first 32-bit radio time to compare.
 * @param[in] aSecondTime  The second 32-bit radio time to compare.
 *
 * @retval TRUE   @p aFirstTime is strictly before @p aSecondTime.
 * @retval FALSE  @p aFirstTime is not strictly before @p aSecondTime.
 */
bool IsTimeStrictlyBefore(Time32 aFirstTime, Time32 aSecondTime);

#if OT_CONFIG_RADIO_TIME_ENABLE && OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

/**
 * Represents synchronized radio time and local `TimeMicro`.
 */
class SyncedTime : public Clearable<SyncedTime>
{
public:
    /**
     * Sets the synchronized radio and local time values to the current time.
     *
     * @param[in] aRadio  The `Radio` to use for radio time.
     */
    void SetToNow(Radio &aRadio);

    /**
     * Gets the local time as `TimeMicro`
     *
     * @returns The local microsecond time.
     */
    TimeMicro GetAsLocalTimeMicro(void) const { return mLocalTime; }

    /**
     * Gets the 64-bit radio time.
     *
     * @returns The 64-bit radio time.
     */
    Time64 GetAsTime64(void) const { return mRadioTime; }

    /**
     * Gets the 32-bit radio time.
     *
     * @returns The 32-bit radio time.
     */
    Time32 GetAsTime32(void) const { return ConvertTime64To32(mRadioTime); }

    /**
     * Advances the synchronized radio and local time values by a given duration.
     *
     * @param[in] aDuration  The duration in microseconds to add.
     */
    void operator+=(uint32_t aDuration)
    {
        mRadioTime += aDuration;
        mLocalTime += aDuration;
    }

    /**
     * Rewinds the synchronized radio and local time values by a given duration.
     *
     * @param[in] aDuration  The duration in microseconds to subtract.
     */
    void operator-=(uint32_t aDuration)
    {
        mRadioTime -= aDuration;
        mLocalTime -= aDuration;
    }

private:
    Time64    mRadioTime;
    TimeMicro mLocalTime;
};

#endif // OT_CONFIG_RADIO_TIME_ENABLE && OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE

#if OPENTHREAD_CONFIG_MULTI_RADIO

/**
 * Defines the radio link types.
 */
enum Type : uint8_t
{
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    kTypeIeee802154, ///< IEEE 802.15.4 (2.4GHz) link type.
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    kTypeTrel, ///< Thread Radio Encapsulation link type.
#endif
};

/**
 * This constant specifies the number of supported radio link types.
 */
constexpr uint8_t kNumTypes = (((OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE) ? 1 : 0) +
                               ((OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE) ? 1 : 0));

/**
 * Represents a set of radio links.
 */
class Types
{
public:
    static constexpr uint16_t kInfoStringSize = 32; ///< Max chars for the info string (`ToString()`).

    /**
     * Defines the fixed-length `String` object returned from `ToString()`.
     */
    typedef String<kInfoStringSize> InfoString;

    /**
     * This static class variable defines an array containing all supported radio link types.
     */
    static const Type kAllTypes[kNumTypes];

    /**
     * Initializes a `Types` object as empty set
     */
    Types(void)
        : mBitMask(0)
    {
    }

    /**
     * Initializes a `Types` object with a given bit-mask.
     *
     * @param[in] aMask   A bit-mask representing the radio types (the first bit corresponds to radio type 0, and so on)
     */
    explicit Types(uint8_t aMask)
        : mBitMask(aMask)
    {
    }

    /**
     * Clears the set.
     */
    void Clear(void) { mBitMask = 0; }

    /**
     * Indicates whether the set is empty or not
     *
     * @returns TRUE if the set is empty, FALSE otherwise.
     */
    bool IsEmpty(void) const { return (mBitMask == 0); }

    /**
     *  This method indicates whether the set contains only a single radio type.
     *
     * @returns TRUE if the set contains a single radio type, FALSE otherwise.
     */
    bool ContainsSingleRadio(void) const { return !IsEmpty() && ((mBitMask & (mBitMask - 1)) == 0); }

    /**
     * Indicates whether or not the set contains a given radio type.
     *
     * @param[in] aType  A radio link type.
     *
     * @returns TRUE if the set contains @p aType, FALSE otherwise.
     */
    bool Contains(Type aType) const { return ((mBitMask & BitFlag(aType)) != 0); }

    /**
     * Adds a radio type to the set.
     *
     * @param[in] aType  A radio link type.
     */
    void Add(Type aType) { mBitMask |= BitFlag(aType); }

    /**
     * Adds another radio types set to the current one.
     *
     * @param[in] aTypes   A radio link type set to add.
     */
    void Add(Types aTypes) { mBitMask |= aTypes.mBitMask; }

    /**
     * Adds all radio types supported by device to the set.
     */
    void AddAll(void);

    /**
     * Removes a given radio type from the set.
     *
     * @param[in] aType  A radio link type.
     */
    void Remove(Type aType) { mBitMask &= ~BitFlag(aType); }

    /**
     * Gets the radio type set as a bitmask.
     *
     * The first bit in the mask corresponds to first radio type (radio type with value zero), and so on.
     *
     * @returns A bitmask representing the set of radio types.
     */
    uint8_t GetAsBitMask(void) const { return mBitMask; }

    /**
     * Overloads operator `-` to return a new set which is the set difference between current set and
     * a given set.
     *
     * @param[in] aOther  Another radio type set.
     *
     * @returns A new set which is set difference between current one and @p aOther.
     */
    Types operator-(const Types &aOther) const { return Types(mBitMask & ~aOther.mBitMask); }

    /**
     * Converts the radio set to human-readable string.
     *
     * @return A string representation of the set of radio types.
     */
    InfoString ToString(void) const;

private:
    static uint8_t BitFlag(Type aType) { return static_cast<uint8_t>(1U << static_cast<uint8_t>(aType)); }

    uint8_t mBitMask;
};

/**
 * Converts a link type to a string
 *
 * @param[in] aType  A link type value.
 *
 * @returns A string representation of the link type.
 */
const char *TypeToString(Type aType);

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

} // namespace Radio
} // namespace ot

#endif // OT_CORE_RADIO_RADIO_TYPES_HPP_
