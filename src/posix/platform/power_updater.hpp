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

#ifndef POSIX_PLATFORM_POWER_UPDATER_HPP_
#define POSIX_PLATFORM_POWER_UPDATER_HPP_

#include "openthread-posix-config.h"

#if OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE

#include <stdio.h>
#include <string.h>

#include <openthread/error.h>
#include <openthread/logging.h>
#include <openthread/platform/radio.h>

#include "config_file.hpp"
#include "power.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace Posix {

/**
 * Updates the target power table and calibrated power table to the RCP.
 *
 */
class PowerUpdater
{
public:
    PowerUpdater(void)
        : mFactoryConfigFile(kFactoryConfigFile)
        , mProductConfigFile(kProductConfigFile)
        , mRegionCode(0)
    {
    }

    /**
     * Set the region code.
     *
     * The radio region format is the 2-bytes ascii representation of the
     * ISO 3166 alpha-2 code.
     *
     * @param[in]  aRegionCode  The radio region.
     *
     * @retval  OT_ERROR_NONE             Successfully set region code.
     * @retval  OT_ERROR_FAILED           Failed to set the region code.
     *
     */
    otError SetRegion(uint16_t aRegionCode);

    /**
     * Get the region code.
     *
     * The radio region format is the 2-bytes ascii representation of the
     * ISO 3166 alpha-2 code.
     *
     * @returns  The region code.
     *
     */
    uint16_t GetRegion(void) const { return mRegionCode; }

private:
    const char               *kFactoryConfigFile      = OPENTHREAD_POSIX_CONFIG_FACTORY_CONFIG_FILE;
    const char               *kProductConfigFile      = OPENTHREAD_POSIX_CONFIG_PRODUCT_CONFIG_FILE;
    const char               *kKeyCalibratedPower     = "calibrated_power";
    const char               *kKeyTargetPower         = "target_power";
    const char               *kKeyRegionDomainMapping = "region_domain_mapping";
    const char               *kCommaDelimiter         = ",";
    static constexpr uint16_t kMaxValueSize           = 512;
    static constexpr uint16_t kRegionCodeWorldWide    = 0x5757; // Region Code: "WW"

    uint16_t StringToRegionCode(const char *aString) const
    {
        return static_cast<uint16_t>(((aString[0] & 0xFF) << 8) | ((aString[1] & 0xFF) << 0));
    }
    otError GetDomain(uint16_t aRegionCode, Power::Domain &aDomain);
    otError GetNextTargetPower(const Power::Domain &aDomain, int &aIterator, Power::TargetPower &aTargetPower);
    otError UpdateCalibratedPower(void);

    ConfigFile mFactoryConfigFile;
    ConfigFile mProductConfigFile;
    uint16_t   mRegionCode;
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_POWER_CALIBRATION_ENABLE
#endif // POSIX_PLATFORM_POWER_UPDATER_HPP_
