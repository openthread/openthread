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

#include "config.hpp"

#include "platform-posix.h"
#include <openthread/platform/radio.h>
#include "lib/platform/exit_code.h"
#include "utils/parse_cmdline.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE && !OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
#error \
    "OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE is required for OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE"
#endif

#if OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE

namespace ot {
namespace Posix {

otError Config::SetRegion(uint16_t aRegionCode)
{
    otError       error = OT_ERROR_NONE;
    Power::Domain domain;

    if (GetDomain(aRegionCode, domain) != OT_ERROR_NONE)
    {
        // If failed to find the domain for the region, use the world wide region as the default region.
        VerifyOrExit(GetDomain(kRegionCodeWorldWide, domain) == OT_ERROR_NONE, error = OT_ERROR_FAILED);
    }

    SuccessOrExit(error = UpdateChannelMasks(domain));
#if OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
    SuccessOrExit(error = UpdateTargetPower(domain));
    SuccessOrExit(error = UpdateCalibratedPower());
#endif

    mRegionCode = aRegionCode;

exit:
    if (error == OT_ERROR_NONE)
    {
        otLogInfoPlat("Set region \"%c%c\" successfully", (aRegionCode >> 8) & 0xff, (aRegionCode & 0xff));
    }
    else
    {
        otLogCritPlat("Set region \"%c%c\" failed, Error: %s", (aRegionCode >> 8) & 0xff, (aRegionCode & 0xff),
                      otThreadErrorToString(error));
    }

    return error;
}

otError Config::GetDomain(uint16_t aRegionCode, Power::Domain &aDomain)
{
    otError error    = OT_ERROR_NOT_FOUND;
    int     iterator = 0;
    char    value[kMaxValueSize];
    char   *str;

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
    if (error != OT_ERROR_NONE)
    {
        otLogCritPlat("Get domain failed, Error: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Config::GetChannelMask(const char *aKey, const Power::Domain &aDomain, uint32_t &aChannelMask)
{
    otError       error    = OT_ERROR_NOT_FOUND;
    int           iterator = 0;
    char          value[kMaxValueSize];
    char         *str;
    Power::Domain domain;
    uint32_t      channelMask;

    while (mProductConfigFile.Get(aKey, iterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        if (((str = strtok(value, kCommaDelimiter)) == nullptr) || (aDomain != str))
        {
            continue;
        }

        if ((str = strtok(nullptr, kCommaDelimiter)) != nullptr)
        {
            SuccessOrExit(error = Utils::CmdLineParser::ParseAsUint32(str, channelMask));
            aChannelMask = channelMask;
            error        = OT_ERROR_NONE;
            break;
        }
    }

exit:
    return error;
}

otError Config::UpdateChannelMasks(const Power::Domain &aDomain)
{
    otError error = OT_ERROR_NONE;

    if (mProductConfigFile.HasKey(kKeySupportedChannelMask))
    {
        SuccessOrExit(error = GetChannelMask(kKeySupportedChannelMask, aDomain, mSupportedChannelMask));
    }

    if (mProductConfigFile.HasKey(kKeySupportedChannelMask))
    {
        SuccessOrExit(error = GetChannelMask(kKeyPreferredChannelMask, aDomain, mPreferredChannelMask));
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogCritPlat("Update channel mask failed, Error: %s", otThreadErrorToString(error));
    }

    return error;
}

#if OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
otError Config::UpdateTargetPower(const Power::Domain &aDomain)
{
    otError            error    = OT_ERROR_NONE;
    int                iterator = 0;
    Power::TargetPower targetPower;

    VerifyOrExit(mProductConfigFile.HasKey(kKeyTargetPower));

    while (GetNextTargetPower(aDomain, iterator, targetPower) == OT_ERROR_NONE)
    {
        otLogInfoPlat("Update target power: %s\r\n", targetPower.ToString().AsCString());

        for (uint8_t ch = targetPower.GetChannelStart(); ch <= targetPower.GetChannelEnd(); ch++)
        {
            SuccessOrExit(error = otPlatRadioSetChannelTargetPower(gInstance, ch, targetPower.GetTargetPower()));
        }
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogCritPlat("Update target power failed, Error: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Config::UpdateCalibratedPower(void)
{
    otError                error    = OT_ERROR_NONE;
    int                    iterator = 0;
    char                   value[kMaxValueSize];
    Power::CalibratedPower calibratedPower;
    ConfigFile            *calibrationFile = &mFactoryConfigFile;

    // If the distribution of output power is large, the factory needs to measure the power calibration data
    // for each device individually, and the power calibration data will be written to the factory config file.
    // Otherwise, the power calibration data can be pre-configured in the product config file.
    if (calibrationFile->Get(kKeyCalibratedPower, iterator, value, sizeof(value)) != OT_ERROR_NONE)
    {
        calibrationFile = &mProductConfigFile;
    }

    VerifyOrExit(calibrationFile->HasKey(kKeyCalibratedPower));
    SuccessOrExit(error = otPlatRadioClearCalibratedPowers(gInstance));

    iterator = 0;
    while (calibrationFile->Get(kKeyCalibratedPower, iterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        SuccessOrExit(error = calibratedPower.FromString(value));
        otLogInfoPlat("Update calibrated power: %s\r\n", calibratedPower.ToString().AsCString());

        for (uint8_t ch = calibratedPower.GetChannelStart(); ch <= calibratedPower.GetChannelEnd(); ch++)
        {
            SuccessOrExit(error = otPlatRadioAddCalibratedPower(gInstance, ch, calibratedPower.GetActualPower(),
                                                                calibratedPower.GetRawPowerSetting().GetData(),
                                                                calibratedPower.GetRawPowerSetting().GetLength()));
        }
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogCritPlat("Update calibrated power table failed, Error: %s", otThreadErrorToString(error));
    }

    return error;
}

otError Config::GetNextTargetPower(const Power::Domain &aDomain, int &aIterator, Power::TargetPower &aTargetPower)
{
    otError error = OT_ERROR_NOT_FOUND;
    char    value[kMaxValueSize];
    char   *domain;
    char   *psave;

    while (mProductConfigFile.Get(kKeyTargetPower, aIterator, value, sizeof(value)) == OT_ERROR_NONE)
    {
        if (((domain = strtok_r(value, kCommaDelimiter, &psave)) == nullptr) || (aDomain != domain))
        {
            continue;
        }

        if ((error = aTargetPower.FromString(psave)) != OT_ERROR_NONE)
        {
            otLogCritPlat("Read target power failed, Error: %s", otThreadErrorToString(error));
        }
        break;
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_CONFIGURATION_FILE_ENABLE
