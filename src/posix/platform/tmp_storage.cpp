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

/**
 * @file
 *   This file implements the temporary storage module for saving, restoring and deleting the temporary key-value pairs.
 *   All temporary key-value pairs will be deleted after the system reboots.
 */

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#else
#include <sys/sysctl.h>
#endif
#include <time.h>

#include "tmp_storage.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"

#if OPENTHREAD_POSIX_CONFIG_TMP_STORAGE_ENABLE
namespace ot {
namespace Posix {

void TmpStorage::Init(void)
{
    otError  error = OT_ERROR_NONE;
    time_t   storedBootTime;
    time_t   currentBootTime;
    uint16_t valueLength = sizeof(time_t);

    VerifyOrDie(SettingsFileInit() == OT_ERROR_NONE, OT_EXIT_FAILURE);

    currentBootTime = GetBootTime();

    error = mStorageFile.Get(kKeyBootTime, 0, reinterpret_cast<uint8_t *>(&storedBootTime), &valueLength);

    // If failed to get the boot time or the current boot time doesn't match the stored boot time, it means
    // the system has been rebooted.
    if ((error != OT_ERROR_NONE) || (valueLength != sizeof(time_t)) || !BootTimeMatch(storedBootTime, currentBootTime))
    {
        mStorageFile.Wipe();
        mStorageFile.Set(kKeyBootTime, reinterpret_cast<const uint8_t *>(&currentBootTime),
                         static_cast<uint16_t>(sizeof(currentBootTime)));
    }
}

void TmpStorage::Deinit(void) { mStorageFile.Deinit(); }

void TmpStorage::SaveRadioSpinelMetrics(const otRadioSpinelMetrics &aMetrics)
{
    mStorageFile.Set(kKeyRadioSpinelMetrics, reinterpret_cast<const uint8_t *>(&aMetrics),
                     static_cast<uint16_t>(sizeof(aMetrics)));
}

otError TmpStorage::RestoreRadioSpinelMetrics(otRadioSpinelMetrics &aMetrics)
{
    uint16_t valueLength = sizeof(aMetrics);

    return mStorageFile.Get(kKeyRadioSpinelMetrics, 0, reinterpret_cast<uint8_t *>(&aMetrics), &valueLength);
}

otError TmpStorage::SettingsFileInit(void)
{
    static constexpr size_t kMaxFileBaseNameSize = 32;
    char                    fileBaseName[kMaxFileBaseNameSize];
    const char             *offset = getenv("PORT_OFFSET");
    uint64_t                eui64;

    otPlatRadioGetIeeeEui64(gInstance, reinterpret_cast<uint8_t *>(&eui64));
    eui64 = ot::BigEndian::HostSwap64(eui64);

    snprintf(fileBaseName, sizeof(fileBaseName), "%s_%" PRIx64 "-tmp", ((offset == nullptr) ? "0" : offset), eui64);
    VerifyOrDie(strlen(fileBaseName) < kMaxFileBaseNameSize, OT_EXIT_FAILURE);

    return mStorageFile.Init(fileBaseName);
}

time_t TmpStorage::GetBootTime(void)
{
    time_t curTime;
    time_t upTime;

#ifdef __linux__
    struct sysinfo info;
    VerifyOrDie(sysinfo(&info) == 0, OT_EXIT_ERROR_ERRNO);
    upTime = info.uptime;
#else
    {
        int            mib[] = {CTL_KERN, KERN_BOOTTIME};
        struct timeval boottime;
        size_t         size = sizeof(boottime);

        VerifyOrDie(sysctl(mib, OT_ARRAY_LENGTH(mib), &boottime, &size, NULL, 0) == 0, OT_EXIT_ERROR_ERRNO);
        upTime = boottime.tv_sec;
    }
#endif

    VerifyOrDie(time(&curTime) != -1, OT_EXIT_ERROR_ERRNO);

    return (curTime - upTime);
}

bool TmpStorage::BootTimeMatch(time_t aBootTimeA, time_t aBootTimeB)
{
    time_t diff = (aBootTimeA > aBootTimeB) ? (aBootTimeA - aBootTimeB) : (aBootTimeB - aBootTimeA);

    // The uptime and the current time are not strictly synchronized. The calculated boot time has 1 second of jitter.
    return diff < 2;
}

} // namespace Posix
} // namespace ot
#endif // OPENTHREAD_POSIX_CONFIG_TMP_STORAGE_ENABLE
