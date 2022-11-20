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

#if OPENTHREAD_PLATFORM_CONFIG_POWER_CALIBRATION_ENABLE

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"

namespace ot {
namespace Utils {
PowerCalibration::PowerCalibration(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mPowerUpdated(false)
    , mChannel(0)
    , mCalibratedPower(nullptr)
{
    memset(mNumCalibratedPowers, 0, sizeof(mNumCalibratedPowers));

    for (uint16_t i = 0; i < kNumChannels; i++)
    {
        mTargetPowerTable[i] = CalibratedPower::kInvalidPower;
    }
}

void PowerCalibration::CalibratedPower::Init(int16_t        aActualPower,
                                             const uint8_t *aRawPowerSetting,
                                             uint16_t       aRawPowerSettingLength)
{
    AssertPointerIsNotNull(aRawPowerSetting);
    OT_ASSERT(aRawPowerSettingLength <= kMaxRawPowerSettingSize);

    mActualPower           = aActualPower;
    mRawPowerSettingLength = aRawPowerSettingLength;
    memcpy(mRawPowerSetting, aRawPowerSetting, aRawPowerSettingLength);
}

Error PowerCalibration::CalibratedPower::GetRawPowerSetting(uint8_t *aRawPowerSetting, uint16_t *aRawPowerSettingLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aRawPowerSetting != nullptr && aRawPowerSettingLength != nullptr &&
                     *aRawPowerSettingLength >= mRawPowerSettingLength,
                 error = kErrorInvalidArgs);

    memcpy(aRawPowerSetting, mRawPowerSetting, mRawPowerSettingLength);
    *aRawPowerSettingLength = mRawPowerSettingLength;

exit:
    return error;
}

Error PowerCalibration::AddCalibratedPower(uint8_t        aChannel,
                                           int16_t        aActualPower,
                                           const uint8_t *aRawPowerSetting,
                                           uint16_t       aRawPowerSettingLength)
{
    Error            error = kErrorNone;
    CalibratedPower *calibratedPowers;
    uint8_t          numCalibratedPowers;
    uint8_t          chIndex;
    int16_t          i = 0;

    VerifyOrExit(IsChannelValid(aChannel) && aRawPowerSetting != nullptr &&
                     aRawPowerSettingLength <= CalibratedPower::kMaxRawPowerSettingSize,
                 error = kErrorInvalidArgs);

    chIndex             = aChannel - Radio::kChannelMin;
    calibratedPowers    = &mCalibrationPowerTable[chIndex][0];
    numCalibratedPowers = mNumCalibratedPowers[chIndex];

    VerifyOrExit(numCalibratedPowers < kMaxNumCalibratedPowers, error = kErrorNoBufs);
    VerifyOrExit(!FindCalibratedPower(calibratedPowers, numCalibratedPowers, aActualPower), error = kErrorInvalidArgs);

    // Insert the calibrated power in order from small to large.
    for (i = numCalibratedPowers; (i > 0) && (aActualPower < calibratedPowers[i - 1].GetActualPower()); i--)
    {
        calibratedPowers[i] = calibratedPowers[i - 1];
    }

    calibratedPowers[i].Init(aActualPower, aRawPowerSetting, aRawPowerSettingLength);
    mPowerUpdated = true;
    mNumCalibratedPowers[chIndex]++;

exit:
    return error;
}

void PowerCalibration::ClearCalibratedPowers(void)
{
    memset(mNumCalibratedPowers, 0, sizeof(mNumCalibratedPowers));
    mPowerUpdated = true;
}

Error PowerCalibration::SetChannelTargetPower(uint8_t aChannel, int16_t aTargetPower)
{
    Error error = kErrorNone;

    VerifyOrExit(IsChannelValid(aChannel), error = kErrorInvalidArgs);
    mTargetPowerTable[aChannel - Radio::kChannelMin] = aTargetPower;
    mPowerUpdated                                    = true;

exit:
    return error;
}

Error PowerCalibration::GetRawPowerSetting(uint8_t   aChannel,
                                           uint8_t * aRawPowerSetting,
                                           uint16_t *aRawPowerSettingLength)
{
    Error            error = kErrorNone;
    uint8_t          chIndex;
    uint8_t          numCalibratedPowers;
    int16_t          i;
    int16_t          targetPower;
    CalibratedPower *calibratedPowers;

    VerifyOrExit(IsChannelValid(aChannel) && (aRawPowerSetting != nullptr), error = kErrorInvalidArgs);
    VerifyOrExit((mChannel != aChannel) || mPowerUpdated);

    chIndex             = aChannel - Radio::kChannelMin;
    targetPower         = mTargetPowerTable[chIndex];
    calibratedPowers    = &mCalibrationPowerTable[chIndex][0];
    numCalibratedPowers = mNumCalibratedPowers[chIndex];

    VerifyOrExit(targetPower != CalibratedPower::kInvalidPower, error = kErrorNotFound);
    VerifyOrExit(numCalibratedPowers > 0, error = kErrorNotFound);

    for (i = numCalibratedPowers - 1; (i >= 0) && (targetPower < calibratedPowers[i].GetActualPower()); i--)
        ;

    VerifyOrExit(i >= 0, error = kErrorNotFound);

    mCalibratedPower = &calibratedPowers[i];

    mChannel      = aChannel;
    mPowerUpdated = false;

exit:
    if (error == kErrorNone)
    {
        mCalibratedPower->GetRawPowerSetting(aRawPowerSetting, aRawPowerSettingLength);
    }

    return error;
}

bool PowerCalibration::FindCalibratedPower(CalibratedPower *aCalibratedPowers,
                                           uint8_t          aNumCalibratedPowers,
                                           int16_t          aActualPower)
{
    bool ret = false;

    for (uint8_t i = 0; i < aNumCalibratedPowers; i++)
    {
        VerifyOrExit(aCalibratedPowers[i].GetActualPower() != aActualPower, ret = true);
    }

exit:
    return ret;
}
} // namespace Utils
} // namespace ot

using namespace ot;

OT_TOOL_WEAK otError otPlatRadioAddCalibratedPower(otInstance *   aInstance,
                                                   uint8_t        aChannel,
                                                   int16_t        aActualPower,
                                                   const uint8_t *aRawPowerSetting,
                                                   uint16_t       aRawPowerSettingLength)
{
    return AsCoreType(aInstance).Get<Utils::PowerCalibration>().AddCalibratedPower(
        aChannel, aActualPower, aRawPowerSetting, aRawPowerSettingLength);
}

OT_TOOL_WEAK otError otPlatRadioClearCalibratedPowers(otInstance *aInstance)
{
    AsCoreType(aInstance).Get<Utils::PowerCalibration>().ClearCalibratedPowers();
    return OT_ERROR_NONE;
}

OT_TOOL_WEAK otError otPlatRadioSetChannelTargetPower(otInstance *aInstance, uint8_t aChannel, int16_t aTargetPower)
{
    return AsCoreType(aInstance).Get<Utils::PowerCalibration>().SetChannelTargetPower(aChannel, aTargetPower);
}

otError otPlatRadioGetRawPowerSetting(otInstance *aInstance,
                                      uint8_t     aChannel,
                                      uint8_t *   aRawPowerSetting,
                                      uint16_t *  aRawPowerSettingLength)
{
    return AsCoreType(aInstance).Get<Utils::PowerCalibration>().GetRawPowerSetting(aChannel, aRawPowerSetting,
                                                                                   aRawPowerSettingLength);
}
#endif // OPENTHREAD_PLATFORM_CONFIG_POWER_CALIBRATION_ENABLE
