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

#include "power_calibration.h"

#include <openthread/platform/radio.h>

#include <string.h>

#include "utils/code_utils.h"

#if OPENTHREAD_PLATFORM_CONFIG_POWER_CALIBRATION_ENABLE
class PowerCalibration
{
public:
    PowerCalibration(void)
        : mPowerUpdated(false)
        , mChannel(0)
        , mRawPowerSetting(nullptr)
    {
        memset(mNumCalibratedPowers, 0, sizeof(mNumCalibratedPowers));
        memset(mTargetPowerTable, INT16_MAX, sizeof(mTargetPowerTable));
    }

    otError AddCalibratedPower(uint8_t aChannel, int16_t aActualPower, const otRawPowerSetting *aRawPowerSetting)
    {
        otError                 error = OT_ERROR_NONE;
        struct CalibratedPower *calibratedPower;
        uint8_t                 chIndex;
        int16_t                 i = 0;

        otEXPECT_ACTION(aChannel >= kMinChannel && aChannel <= kMaxChannel && aRawPowerSetting != nullptr,
                        error = OT_ERROR_INVALID_ARGS);
        chIndex = aChannel - kMinChannel;
        otEXPECT_ACTION(mNumCalibratedPowers[chIndex] < kMaxNumCalibratedPowers, error = OT_ERROR_NO_BUFS);
        calibratedPower = &mCalibrationPowerTable[chIndex][0];

        // Insert the calibrated power in order from small to large.
        for (i = mNumCalibratedPowers[chIndex]; i > 0; i--)
        {
            if (aActualPower < calibratedPower[i - 1].mActualPower)
            {
                calibratedPower[i] = calibratedPower[i - 1];
            }
            else
            {
                break;
            }
        }

        calibratedPower[i].mActualPower     = aActualPower;
        calibratedPower[i].mRawPowerSetting = *aRawPowerSetting;
        mPowerUpdated                       = true;
        mNumCalibratedPowers[chIndex]++;

    exit:
        return error;
    }

    otError ClearCalibratedPowers(void)
    {
        memset(mNumCalibratedPowers, 0, sizeof(mNumCalibratedPowers));
        mPowerUpdated = true;
        return OT_ERROR_NONE;
    }

    otError SetChannelTargetPower(uint8_t aChannel, int16_t aTargetPower)
    {
        otError error = OT_ERROR_NONE;

        otEXPECT_ACTION(aChannel >= kMinChannel && aChannel <= kMaxChannel, error = OT_ERROR_INVALID_ARGS);
        mTargetPowerTable[aChannel - kMinChannel] = aTargetPower;
        mPowerUpdated                             = true;

    exit:
        return error;
    }

    otError GetRawPowerSetting(uint8_t aChannel, otRawPowerSetting *aRawPowerSetting)
    {
        otError                 error = OT_ERROR_NONE;
        uint8_t                 chIndex;
        uint8_t                 i;
        int16_t                 targetPower;
        struct CalibratedPower *calibratedPower;

        otEXPECT_ACTION((aChannel >= kMinChannel) && (aChannel <= kMaxChannel) && (aRawPowerSetting != nullptr),
                        error = OT_ERROR_INVALID_ARGS);
        otEXPECT((mChannel != aChannel) || mPowerUpdated);

        chIndex         = aChannel - kMinChannel;
        targetPower     = mTargetPowerTable[chIndex];
        calibratedPower = &mCalibrationPowerTable[chIndex][0];

        otEXPECT_ACTION(targetPower != INT16_MAX, error = OT_ERROR_NOT_FOUND);
        otEXPECT_ACTION((mNumCalibratedPowers[chIndex] != 0) && (targetPower >= calibratedPower[0].mActualPower),
                        error = OT_ERROR_NOT_FOUND);

        for (i = 0; i < mNumCalibratedPowers[chIndex]; i++)
        {
            if (targetPower >= calibratedPower[i].mActualPower)
            {
                break;
            }
        }

        mRawPowerSetting = &calibratedPower[i].mRawPowerSetting;
        mChannel         = aChannel;
        mPowerUpdated    = false;

    exit:
        if (error == OT_ERROR_NONE)
        {
            *aRawPowerSetting = *mRawPowerSetting;
        }

        return error;
    }

private:
    OT_TOOL_PACKED_BEGIN
    struct CalibratedPower
    {
        int16_t           mActualPower;
        otRawPowerSetting mRawPowerSetting;
    } OT_TOOL_PACKED_END;

    enum
    {
        kMinChannel             = 11,
        kMaxChannel             = 26,
        kNumChannels            = kMaxChannel - kMinChannel + 1,
        kMaxNumCalibratedPowers = PLATFORM_UTILS_CONFIG_NUM_CALIBRATED_POWERS,
    };

    bool                      mPowerUpdated;
    uint8_t                   mChannel;
    struct otRawPowerSetting *mRawPowerSetting;
    int16_t                   mTargetPowerTable[kNumChannels];
    uint8_t                   mNumCalibratedPowers[kNumChannels];
    struct CalibratedPower    mCalibrationPowerTable[kNumChannels][kMaxNumCalibratedPowers];
};

static PowerCalibration sPowerCalibration;

otError otPlatRadioAddCalibratedPower(otInstance *             aInstance,
                                      uint8_t                  aChannel,
                                      int16_t                  aActualPower,
                                      const otRawPowerSetting *aRawPowerSetting)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sPowerCalibration.AddCalibratedPower(aChannel, aActualPower, aRawPowerSetting);
}

otError otPlatRadioClearCalibratedPowers(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sPowerCalibration.ClearCalibratedPowers();
}

otError otPlatRadioSetChannelTargetPower(otInstance *aInstance, uint8_t aChannel, int16_t aTargetPower)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sPowerCalibration.SetChannelTargetPower(aChannel, aTargetPower);
}

otError otUtilsPowerCalibrationGetRawPowerSetting(uint8_t aChannel, otRawPowerSetting *aRawPowerSetting)
{
    return sPowerCalibration.GetRawPowerSetting(aChannel, aRawPowerSetting);
}

#endif // OPENTHREAD_PLATFORM_CONFIG_POWER_CALIBRATION_ENABLE
