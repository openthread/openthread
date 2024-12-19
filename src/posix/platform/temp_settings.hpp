/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef OT_POSIX_PLATFORM_STEMP_ETTINGS_HPP_
#define OT_POSIX_PLATFORM_STEMP_ETTINGS_HPP_

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include "settings_file.hpp"

#if OPENTHREAD_POSIX_CONFIG_TEMP_SETTINGS_ENABLE
namespace ot {
namespace Posix {

class TempSettings
{
public:
    TempSettings(void)
        : mSettingsFile()
    {
    }

    /**
     * Performs the initialization for the temporary settings file.
     */
    void Init(void);

    /**
     * Performs the de-initialization for the temporary settings file.
     */
    void Deinit(void);

    /**
     * Writes the radio spinel metrics to the temporary settings file.
     *
     * @param[in]  aMetrics   A reference to the radio spinel metrics.
     */
    void WriteRadioSpinelMetrics(const otRadioSpinelMetrics &aMetrics);

    /**
     * Reads the radio spinel metrics from the temporary settings file.
     *
     * @param[out]  aMetrics   A reference to the radio spinel metrics.
     *
     * @retval OT_ERROR_NONE        The radio spinel metrics was found and fetched successfully.
     * @retval OT_ERROR_NOT_FOUND   The radio spinel metrics was not found in the setting store.
     */
    otError ReadRadioSpinelMetrics(otRadioSpinelMetrics &aMetrics);

private:
    enum
    {
        kKeyBootTime           = 1,
        kKeyRadioSpinelMetrics = 2,
    };

    otError SettingsFileInit(void);
    time_t  GetBootTime(void);
    bool    BootTimeMatch(time_t aBootTimeA, time_t aBootTimeB);

    SettingsFile mSettingsFile;
};

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_TEMP_SETTINGS_ENABLE
#endif // OT_POSIX_PLATFORM_STEMP_ETTINGS_HPP_
