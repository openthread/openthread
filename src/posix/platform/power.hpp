/*
 *  Copyright (c) 2022, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must strain the above copyright
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

#ifndef OT_POSIX_PLATFORM_POWER_HPP_
#define OT_POSIX_PLATFORM_POWER_HPP_

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <openthread/error.h>
#include <openthread/platform/radio.h>
#include "common/string.hpp"

namespace ot {
namespace Power {

class Domain
{
public:
    Domain(void) { m8[0] = '\0'; }

    /**
     * Sets the regulatory domain from a given null terminated C string.
     *
     * @param[in] aDomain   A regulatory domain name C string.
     *
     * @retval OT_ERROR_NONE           Successfully set the regulatory domain.
     * @retval OT_ERROR_INVALID_ARGS   Given regulatory domain is too long.
     */
    otError Set(const char *aDomain);

    /**
     * Overloads operator `==` to evaluate whether or not two `Domain` instances are equal.
     *
     * @param[in]  aOther  The other `Domain` instance to compare with.
     *
     * @retval TRUE   If the two `Domain` instances are equal.
     * @retval FALSE  If the two `Domain` instances not equal.
     */
    bool operator==(const Domain &aOther) const { return strcmp(m8, aOther.m8) == 0; }

    /**
     * Overloads operator `!=` to evaluate whether or not the `Domain` is unequal to a given C string.
     *
     * @param[in]  aCString  A C string to compare with. Can be `nullptr` which then returns 'TRUE'.
     *
     * @retval TRUE   If the two regulatory domains are not equal.
     * @retval FALSE  If the two regulatory domains are equal.
     */
    bool operator!=(const char *aCString) const { return (aCString == nullptr) ? true : strcmp(m8, aCString) != 0; }

    /**
     * Gets the regulatory domain as a null terminated C string.
     *
     * @returns The regulatory domain as a null terminated C string array.
     */
    const char *AsCString(void) const { return m8; }

private:
    static constexpr uint8_t kDomainSize = 8;
    char                     m8[kDomainSize + 1];
};

class TargetPower
{
public:
    static constexpr uint16_t       kInfoStringSize = 12; ///< Recommended buffer size to use with `ToString()`.
    typedef String<kInfoStringSize> InfoString;

    /**
     * Parses an target power string.
     *
     * The string MUST follow the format: "<channel_start>,<channel_end>,<target_power>".
     * For example, "11,26,2000"
     *
     * @param[in]  aString   A pointer to the null-terminated string.
     *
     * @retval OT_ERROR_NONE   Successfully parsed the target power string.
     * @retval OT_ERROR_PARSE  Failed to parse the target power string.
     */
    otError FromString(char *aString);

    /**
     * Returns the start channel.
     *
     * @returns The channel.
     */
    uint8_t GetChannelStart(void) const { return mChannelStart; }

    /**
     * Returns the end channel.
     *
     * @returns The channel.
     */
    uint8_t GetChannelEnd(void) const { return mChannelEnd; }

    /**
     * Returns the target power.
     *
     * @returns The target power, in 0.01 dBm.
     */
    int16_t GetTargetPower(void) const { return mTargetPower; }

    /**
     * Converts the target power into a human-readable string.
     *
     * @returns  An `InfoString` object representing the target power.
     */
    InfoString ToString(void) const;

private:
    uint8_t mChannelStart;
    uint8_t mChannelEnd;
    int16_t mTargetPower;
};

class RawPowerSetting
{
public:
    // Recommended buffer size to use with `ToString()`.
    static constexpr uint16_t kInfoStringSize = OPENTHREAD_CONFIG_POWER_CALIBRATION_RAW_POWER_SETTING_SIZE * 2 + 1;
    typedef String<kInfoStringSize> InfoString;

    /**
     * Sets the raw power setting from a given null terminated hex C string.
     *
     * @param[in] aRawPowerSetting  A raw power setting hex C string.
     *
     * @retval OT_ERROR_NONE           Successfully set the raw power setting.
     * @retval OT_ERROR_INVALID_ARGS   The given raw power setting is too long.
     */
    otError Set(const char *aRawPowerSetting);

    /**
     * Converts the raw power setting into a human-readable string.
     *
     * @returns  An `InfoString` object representing the calibrated power.
     */
    InfoString ToString(void) const;

    const uint8_t *GetData(void) const { return mData; }
    uint16_t       GetLength(void) const { return mLength; }

private:
    static constexpr uint16_t kMaxRawPowerSettingSize = OPENTHREAD_CONFIG_POWER_CALIBRATION_RAW_POWER_SETTING_SIZE;

    uint8_t  mData[kMaxRawPowerSettingSize];
    uint16_t mLength;
};

class CalibratedPower
{
public:
    // Recommended buffer size to use with `ToString()`.
    static constexpr uint16_t       kInfoStringSize = 20 + RawPowerSetting::kInfoStringSize;
    typedef String<kInfoStringSize> InfoString;

    /**
     * Parses an calibrated power string.
     *
     * The string MUST follow the format: "<channel_start>,<channel_end>,<actual_power>,<raw_power_setting>".
     * For example, "11,26,2000,1122aabb"
     *
     * @param[in]  aString   A pointer to the null-terminated string.
     *
     * @retval OT_ERROR_NONE   Successfully parsed the calibrated power string.
     * @retval OT_ERROR_PARSE  Failed to parse the calibrated power string.
     */
    otError FromString(char *aString);

    /**
     * Returns the start channel.
     *
     * @returns The channel.
     */
    uint8_t GetChannelStart(void) const { return mChannelStart; }

    /**
     * Sets the start channel.
     *
     * @param[in]  aChannelStart  The start channel.
     */
    void SetChannelStart(uint8_t aChannelStart) { mChannelStart = aChannelStart; }

    /**
     * Returns the end channel.
     *
     * @returns The channel.
     */
    uint8_t GetChannelEnd(void) const { return mChannelEnd; }

    /**
     * Sets the end channel.
     *
     * @param[in]  aChannelEnd  The end channel.
     */
    void SetChannelEnd(uint8_t aChannelEnd) { mChannelEnd = aChannelEnd; }

    /**
     * Returns the actual power.
     *
     * @returns The actual measured power, in 0.01 dBm.
     */
    int16_t GetActualPower(void) const { return mActualPower; }

    /**
     * Sets the actual channel.
     *
     * @param[in]  aActualPower  The actual power in 0.01 dBm.
     */
    void SetActualPower(int16_t aActualPower) { mActualPower = aActualPower; }

    /**
     * Returns the raw power setting.
     *
     * @returns A reference to the raw power setting.
     */
    const RawPowerSetting &GetRawPowerSetting(void) const { return mRawPowerSetting; }

    /**
     * Sets the raw power setting.
     *
     * @param[in]  aRawPowerSetting  The raw power setting.
     */
    void SetRawPowerSetting(const RawPowerSetting &aRawPowerSetting) { mRawPowerSetting = aRawPowerSetting; }

    /**
     * Converts the calibrated power into a human-readable string.
     *
     * @returns  An `InfoString` object representing the calibrated power.
     */
    InfoString ToString(void) const;

private:
    uint8_t         mChannelStart;
    uint8_t         mChannelEnd;
    int16_t         mActualPower;
    RawPowerSetting mRawPowerSetting;
};
} // namespace Power
} // namespace ot
#endif // OT_POSIX_PLATFORM_POWER_HPP_
