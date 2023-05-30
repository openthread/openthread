/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 * @brief
 *   This file includes definitions for the platform power calibration module.
 *
 */
#ifndef POWER_CALIBRATION_HPP_
#define POWER_CALIBRATION_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE && OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE

#include <openthread/platform/radio.h>

#include "common/array.hpp"
#include "common/numeric_limits.hpp"
#include "radio/radio.hpp"

namespace ot {
namespace Utils {

/**
 * Implements power calibration module.
 *
 * The power calibration module implements the radio platform power calibration APIs. It mainly stores the calibrated
 * power table and the target power table, provides an API for the platform to get the raw power setting of the
 * specified channel.
 *
 */
class PowerCalibration : public InstanceLocator, private NonCopyable
{
public:
    explicit PowerCalibration(Instance &aInstance);

    /**
     * Add a calibrated power of the specified channel to the power calibration table.
     *
     * @param[in] aChannel                The radio channel.
     * @param[in] aActualPower            The actual power in 0.01dBm.
     * @param[in] aRawPowerSetting        A pointer to the raw power setting byte array.
     * @param[in] aRawPowerSettingLength  The length of the @p aRawPowerSetting.
     *
     * @retval kErrorNone         Successfully added the calibrated power to the power calibration table.
     * @retval kErrorNoBufs       No available entry in the power calibration table.
     * @retval kErrorInvalidArgs  The @p aChannel, @p aActualPower or @p aRawPowerSetting is invalid or the
     *                            @ aActualPower already exists in the power calibration table.
     *
     */
    Error AddCalibratedPower(uint8_t        aChannel,
                             int16_t        aActualPower,
                             const uint8_t *aRawPowerSetting,
                             uint16_t       aRawPowerSettingLength);

    /**
     * Clear all calibrated powers from the power calibration table.
     *
     */
    void ClearCalibratedPowers(void);

    /**
     * Set the target power for the given channel.
     *
     * @param[in]  aChannel      The radio channel.
     * @param[in]  aTargetPower  The target power in 0.01dBm. Passing `INT16_MAX` will disable this channel.
     *
     * @retval  kErrorNone         Successfully set the target power.
     * @retval  kErrorInvalidArgs  The @p aChannel or @p aTargetPower is invalid.
     *
     */
    Error SetChannelTargetPower(uint8_t aChannel, int16_t aTargetPower);

    /**
     * Get the power settings for the given channel.
     *
     * Platform radio layer should parse the raw power setting based on the radio layer defined format and set the
     * parameters of each radio hardware module.
     *
     * @param[in]      aChannel                The radio channel.
     * @param[out]     aTargetPower            A pointer to the target power in 0.01 dBm. May be set to nullptr if
     *                                         the caller doesn't want to get the target power.
     * @param[out]     aActualPower            A pointer to the actual power in 0.01 dBm. May be set to nullptr if
     *                                         the caller doesn't want to get the actual power.
     * @param[out]     aRawPowerSetting        A pointer to the raw power setting byte array.
     * @param[in,out]  aRawPowerSettingLength  On input, a pointer to the size of @p aRawPowerSetting.
     *                                         On output, a pointer to the length of the raw power setting data.
     *
     * @retval  kErrorNone         Successfully got the target power.
     * @retval  kErrorInvalidArgs  The @p aChannel is invalid, @p aRawPowerSetting or @p aRawPowerSettingLength is
     *                             nullptr or @aRawPowerSettingLength is too short.
     * @retval  kErrorNotFound     The power settings for the @p aChannel was not found.
     *
     */
    Error GetPowerSettings(uint8_t   aChannel,
                           int16_t  *aTargetPower,
                           int16_t  *aActualPower,
                           uint8_t  *aRawPowerSetting,
                           uint16_t *aRawPowerSettingLength);

private:
    class CalibratedPowerEntry
    {
    public:
        static constexpr uint16_t kMaxRawPowerSettingSize = OPENTHREAD_CONFIG_POWER_CALIBRATION_RAW_POWER_SETTING_SIZE;

        CalibratedPowerEntry(void)
            : mActualPower(kInvalidPower)
            , mLength(0)
        {
        }

        void    Init(int16_t aActualPower, const uint8_t *aRawPowerSetting, uint16_t aRawPowerSettingLength);
        Error   GetRawPowerSetting(uint8_t *aRawPowerSetting, uint16_t *aRawPowerSettingLength);
        int16_t GetActualPower(void) const { return mActualPower; }
        bool    Matches(int16_t aActualPower) const { return aActualPower == mActualPower; }

    private:
        int16_t  mActualPower;
        uint8_t  mSettings[kMaxRawPowerSettingSize];
        uint16_t mLength;
    };

    bool IsPowerUpdated(void) const { return mCalibratedPowerIndex == kInvalidIndex; }
    bool IsChannelValid(uint8_t aChannel) const
    {
        return ((aChannel >= Radio::kChannelMin) && (aChannel <= Radio::kChannelMax));
    }

    static constexpr uint8_t  kInvalidIndex = NumericLimits<uint8_t>::kMax;
    static constexpr uint16_t kInvalidPower = NumericLimits<int16_t>::kMax;
    static constexpr uint16_t kMaxNumCalibratedPowers =
        OPENTHREAD_CONFIG_POWER_CALIBRATION_NUM_CALIBRATED_POWER_ENTRIES;
    static constexpr uint16_t kNumChannels = Radio::kChannelMax - Radio::kChannelMin + 1;

    static_assert(kMaxNumCalibratedPowers < NumericLimits<uint8_t>::kMax,
                  "kMaxNumCalibratedPowers is larger than or equal to max");

    typedef Array<CalibratedPowerEntry, kMaxNumCalibratedPowers> CalibratedPowerTable;

    uint8_t              mLastChannel;
    int16_t              mTargetPowerTable[kNumChannels];
    uint8_t              mCalibratedPowerIndex;
    CalibratedPowerTable mCalibratedPowerTables[kNumChannels];
};
} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE && OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
#endif // POWER_CALIBRATION_HPP_
