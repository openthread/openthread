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

#include "power_updater.hpp"

#include "platform-posix.h"
#include <openthread/platform/radio.h>
#include "lib/platform/exit_code.h"

#if OPENTHREAD_PLATFORM_CONFIG_POWER_CALIBRATION_ENABLE

namespace ot {
namespace Posix {

otError PowerUpdater::SetRegion(uint16_t aRegionCode)
{
    otError            error    = OT_ERROR_NONE;
    int                iterator = 0;
    Power::Domain      domain;
    Power::TargetPower targetPower;

    if (GetDomain(aRegionCode, domain) != OT_ERROR_NONE)
    {
        // If failed to find the domain for the region, use the world wide region as the default region.
        VerifyOrExit(GetDomain(kRegionCodeWorldWide, domain) == OT_ERROR_NONE, error = OT_ERROR_FAILED);
    }

    while (GetNextTargetPower(domain, iterator, targetPower) == OT_ERROR_NONE)
    {
        otLogInfoPlat("Update target power: %s\r\n", targetPower.ToString().AsCString());

        for (uint8_t ch = targetPower.GetChannelStart(); ch <= targetPower.GetChannelEnd(); ch++)
        {
            SuccessOrExit(otPlatRadioSetChannelTargetPower(gInstance, ch, targetPower.GetTargetPower()));
        }
    }

    UpdateCalibratedPower();

    mRegionCode = aRegionCode;

exit:
    otLogInfoPlat("Set region \"%c%c\" %s", (aRegionCode >> 8) & 0xff, (aRegionCode & 0xff),
                  (error == OT_ERROR_NONE) ? "success" : "failed");
    return error;
}

otError PowerUpdater::GetRegion(uint16_t *aRegionCode) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aRegionCode != nullptr, error = OT_ERROR_INVALID_ARGS);
    *aRegionCode = mRegionCode;

exit:
    return error;
}

void PowerUpdater::UpdateCalibratedPower(void)
{
    int                    iterator = 0;
    char                   value[kMaxValueSize];
    Power::CalibratedPower calibratedPower;
    ConfigFile *           calibrationFile = &mFactoryConfigFile;

    SuccessOrExit(otPlatRadioClearCalibratedPowers(gInstance));

    // If the distribution of output power is large, the factory needs to measure the power calibration data
    // for each device individually, and the power calibration data will be written to the factory config file.
    // Otherwise, the power calibration data can be pre-configured in the product config file.
    if (calibrationFile->Get(kKeyCalibratedPower, iterator, value, sizeof(value)) != OT_ERROR_NONE)
    {
        calibrationFile = &mProductConfigFile;
    }

    while (calibrationFile->Get(kKeyCalibratedPower, iterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        SuccessOrExit(calibratedPower.FromString(value));
        otLogInfoPlat("Update calibrated power: %s\r\n", calibratedPower.ToString().AsCString());

        for (uint8_t ch = calibratedPower.GetChannelStart(); ch < calibratedPower.GetChannelEnd(); ch++)
        {
            SuccessOrExit(otPlatRadioAddCalibratedPower(gInstance, ch, calibratedPower.GetActualPower(),
                                                        calibratedPower.GetRawPowerSetting().GetData(),
                                                        calibratedPower.GetRawPowerSetting().GetLength()));
        }
    }

exit:
    return;
}

otError PowerUpdater::GetDomain(uint16_t aRegionCode, Power::Domain &aDomain)
{
    otError error    = OT_ERROR_NOT_FOUND;
    int     iterator = 0;
    char    value[kMaxValueSize];
    char *  str;

    while (mProductConfigFile.Get(kKeyRegionDomainMapping, iterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        if ((str = strtok(value, kCommaDelimiter)) == nullptr)
        {
            continue;
        }

        while ((str = strtok(nullptr, kCommaDelimiter)) != nullptr)
        {
            if ((strlen(str) == 2) && (StringToRegionCode(str) == aRegionCode))
            {
                ExitNow(error = aDomain.Set(value));
            }
        }
    }

exit:
    return error;
}

otError PowerUpdater::GetNextTargetPower(const Power::Domain &aDomain, int &aIterator, Power::TargetPower &aTargetPower)
{
    otError error = OT_ERROR_NOT_FOUND;
    char    value[kMaxValueSize];
    char *  domain;
    char *  psave;

    while (mProductConfigFile.Get(kKeyTargetPower, aIterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        if (((domain = strtok_r(value, kCommaDelimiter, &psave)) == nullptr) || (aDomain != domain))
        {
            continue;
        }

        error = aTargetPower.FromString(psave);
        break;
    }

    return error;
}

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_PLATFORM_CONFIG_POWER_CALIBRATION_ENABLE
