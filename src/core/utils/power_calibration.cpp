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

#include "power_calibration.hpp"

#if OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE && OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
#include <openthread/platform/diag.h>

#include "instance/instance.hpp"

namespace ot {
namespace Utils {

PowerCalibration::PowerCalibration(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mLastChannel(0)
    , mCalibratedPowerIndex(kInvalidIndex)
{
    for (int16_t &targetPower : mTargetPowerTable)
    {
        targetPower = kInvalidPower;
    }
}

void PowerCalibration::CalibratedPowerEntry::Init(int16_t        aActualPower,
                                                  const uint8_t *aRawPowerSetting,
                                                  uint16_t       aRawPowerSettingLength)
{
    AssertPointerIsNotNull(aRawPowerSetting);
    OT_ASSERT(aRawPowerSettingLength <= kMaxRawPowerSettingSize);

    mActualPower = aActualPower;
    mLength      = aRawPowerSettingLength;
    memcpy(mSettings, aRawPowerSetting, aRawPowerSettingLength);
}

Error PowerCalibration::CalibratedPowerEntry::GetRawPowerSetting(uint8_t  *aRawPowerSetting,
                                                                 uint16_t *aRawPowerSettingLength)
{
    Error error = kErrorNone;

    AssertPointerIsNotNull(aRawPowerSetting);
    AssertPointerIsNotNull(aRawPowerSettingLength);
    VerifyOrExit(*aRawPowerSettingLength >= mLength, error = kErrorInvalidArgs);

    memcpy(aRawPowerSetting, mSettings, mLength);
    *aRawPowerSettingLength = mLength;

exit:
    return error;
}

Error PowerCalibration::AddCalibratedPower(uint8_t        aChannel,
                                           int16_t        aActualPower,
                                           const uint8_t *aRawPowerSetting,
                                           uint16_t       aRawPowerSettingLength)
{
    Error                error = kErrorNone;
    CalibratedPowerEntry entry;
    uint8_t              chIndex;

    AssertPointerIsNotNull(aRawPowerSetting);
    VerifyOrExit(IsChannelValid(aChannel) && aRawPowerSettingLength <= CalibratedPowerEntry::kMaxRawPowerSettingSize,
                 error = kErrorInvalidArgs);

    chIndex = aChannel - Radio::kChannelMin;
    VerifyOrExit(!mCalibratedPowerTables[chIndex].ContainsMatching(aActualPower), error = kErrorInvalidArgs);
    VerifyOrExit(!mCalibratedPowerTables[chIndex].IsFull(), error = kErrorNoBufs);

    entry.Init(aActualPower, aRawPowerSetting, aRawPowerSettingLength);
    SuccessOrExit(error = mCalibratedPowerTables[chIndex].PushBack(entry));

    if (aChannel == mLastChannel)
    {
        mCalibratedPowerIndex = kInvalidIndex;
    }

exit:
    return error;
}

void PowerCalibration::ClearCalibratedPowers(void)
{
    for (CalibratedPowerTable &table : mCalibratedPowerTables)
    {
        table.Clear();
    }

    mCalibratedPowerIndex = kInvalidIndex;
}

Error PowerCalibration::SetChannelTargetPower(uint8_t aChannel, int16_t aTargetPower)
{
    Error error = kErrorNone;

    VerifyOrExit(IsChannelValid(aChannel), error = kErrorInvalidArgs);
    mTargetPowerTable[aChannel - Radio::kChannelMin] = aTargetPower;

    if (aChannel == mLastChannel)
    {
        mCalibratedPowerIndex = kInvalidIndex;
    }

exit:
    return error;
}

Error PowerCalibration::GetPowerSettings(uint8_t   aChannel,
                                         int16_t  *aTargetPower,
                                         int16_t  *aActualPower,
                                         uint8_t  *aRawPowerSetting,
                                         uint16_t *aRawPowerSettingLength)
{
    Error   error = kErrorNone;
    uint8_t chIndex;
    uint8_t powerIndex = kInvalidIndex;
    int16_t foundPower = kInvalidPower;
    int16_t targetPower;
    int16_t actualPower;
    int16_t minPower      = NumericLimits<int16_t>::kMax;
    uint8_t minPowerIndex = kInvalidIndex;

    VerifyOrExit(IsChannelValid(aChannel), error = kErrorInvalidArgs);
    VerifyOrExit((mLastChannel != aChannel) || IsPowerUpdated());

    chIndex     = aChannel - Radio::kChannelMin;
    targetPower = mTargetPowerTable[chIndex];
    VerifyOrExit(targetPower != kInvalidPower, error = kErrorNotFound);
    VerifyOrExit(mCalibratedPowerTables[chIndex].GetLength() > 0, error = kErrorNotFound);

    for (uint8_t i = 0; i < mCalibratedPowerTables[chIndex].GetLength(); i++)
    {
        actualPower = mCalibratedPowerTables[chIndex][i].GetActualPower();

        if ((actualPower <= targetPower) && ((foundPower == kInvalidPower) || (foundPower <= actualPower)))
        {
            foundPower = actualPower;
            powerIndex = i;
        }
        else if (actualPower < minPower)
        {
            minPower      = actualPower;
            minPowerIndex = i;
        }
    }

    mCalibratedPowerIndex = (powerIndex != kInvalidIndex) ? powerIndex : minPowerIndex;
    mLastChannel          = aChannel;

exit:
    if (error == kErrorNone)
    {
        chIndex = mLastChannel - Radio::kChannelMin;

        if (aTargetPower != nullptr)
        {
            *aTargetPower = mTargetPowerTable[chIndex];
        }

        if (aActualPower != nullptr)
        {
            *aActualPower = mCalibratedPowerTables[chIndex][mCalibratedPowerIndex].GetActualPower();
        }

        error = mCalibratedPowerTables[chIndex][mCalibratedPowerIndex].GetRawPowerSetting(aRawPowerSetting,
                                                                                          aRawPowerSettingLength);
    }

    return error;
}
} // namespace Utils
} // namespace ot

using namespace ot;

otError otPlatRadioAddCalibratedPower(otInstance    *aInstance,
                                      uint8_t        aChannel,
                                      int16_t        aActualPower,
                                      const uint8_t *aRawPowerSetting,
                                      uint16_t       aRawPowerSettingLength)
{
    return AsCoreType(aInstance).Get<Utils::PowerCalibration>().AddCalibratedPower(
        aChannel, aActualPower, aRawPowerSetting, aRawPowerSettingLength);
}

otError otPlatRadioClearCalibratedPowers(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<Utils::PowerCalibration>().ClearCalibratedPowers();
    return OT_ERROR_NONE;
}

otError otPlatRadioSetChannelTargetPower(otInstance *aInstance, uint8_t aChannel, int16_t aTargetPower)
{
    return AsCoreType(aInstance).Get<Utils::PowerCalibration>().SetChannelTargetPower(aChannel, aTargetPower);
}

otError otPlatRadioGetRawPowerSetting(otInstance *aInstance,
                                      uint8_t     aChannel,
                                      uint8_t    *aRawPowerSetting,
                                      uint16_t   *aRawPowerSettingLength)
{
    AssertPointerIsNotNull(aRawPowerSetting);
    AssertPointerIsNotNull(aRawPowerSettingLength);

    return AsCoreType(aInstance).Get<Utils::PowerCalibration>().GetPowerSettings(
        aChannel, nullptr, nullptr, aRawPowerSetting, aRawPowerSettingLength);
}

otError otPlatDiagRadioGetPowerSettings(otInstance *aInstance,
                                        uint8_t     aChannel,
                                        int16_t    *aTargetPower,
                                        int16_t    *aActualPower,
                                        uint8_t    *aRawPowerSetting,
                                        uint16_t   *aRawPowerSettingLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    AssertPointerIsNotNull(aRawPowerSetting);
    AssertPointerIsNotNull(aRawPowerSettingLength);

    return AsCoreType(aInstance).Get<Utils::PowerCalibration>().GetPowerSettings(
        aChannel, aTargetPower, aActualPower, aRawPowerSetting, aRawPowerSettingLength);
}
#endif // OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE && OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
