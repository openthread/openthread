/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for non-volatile storage of settings.
 */

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openthread/logging.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/settings.h>
#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
#include <openthread/platform/secure_settings.h>
#endif

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "posix/platform/settings.hpp"
#include "posix/platform/settings_file.hpp"

#include "system.hpp"

static ot::Posix::SettingsFile sSettingsFile;

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
static const uint16_t *sSensitiveKeys       = nullptr;
static uint16_t        sSensitiveKeysLength = 0;

static bool isSensitiveKey(uint16_t aKey)
{
    bool ret = false;

    VerifyOrExit(sSensitiveKeys != nullptr);

    for (uint16_t i = 0; i < sSensitiveKeysLength; i++)
    {
        VerifyOrExit(aKey != sSensitiveKeys[i], ret = true);
    }

exit:
    return ret;
}
#endif

static otError settingsFileInit(otInstance *aInstance)
{
    static constexpr size_t kMaxFileBaseNameSize = 32;
    char                    fileBaseName[kMaxFileBaseNameSize];
    const char             *offset = getenv("PORT_OFFSET");
    uint64_t                nodeId;

    otPlatRadioGetIeeeEui64(aInstance, reinterpret_cast<uint8_t *>(&nodeId));
    nodeId = ot::BigEndian::HostSwap64(nodeId);

    snprintf(fileBaseName, sizeof(fileBaseName), "%s_%" PRIx64, offset == nullptr ? "0" : offset, nodeId);
    VerifyOrDie(strlen(fileBaseName) < kMaxFileBaseNameSize, OT_EXIT_FAILURE);

    return sSettingsFile.Init(fileBaseName);
}

void otPlatSettingsInit(otInstance *aInstance, const uint16_t *aSensitiveKeys, uint16_t aSensitiveKeysLength)
{
#if !OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    OT_UNUSED_VARIABLE(aSensitiveKeys);
    OT_UNUSED_VARIABLE(aSensitiveKeysLength);
#endif

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    sSensitiveKeys       = aSensitiveKeys;
    sSensitiveKeysLength = aSensitiveKeysLength;
#endif

    // Don't touch the settings file the system runs in dry-run mode.
    VerifyOrExit(!IsSystemDryRun());
    SuccessOrExit(settingsFileInit(aInstance));

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    otPosixSecureSettingsInit(aInstance);
#endif

exit:
    return;
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(!IsSystemDryRun());

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    otPosixSecureSettingsDeinit(aInstance);
#endif

    sSettingsFile.Deinit();

exit:
    return;
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NOT_FOUND;

    VerifyOrExit(!IsSystemDryRun());
#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    if (isSensitiveKey(aKey))
    {
        error = otPosixSecureSettingsGet(aInstance, aKey, aIndex, aValue, aValueLength);
    }
    else
#endif
    {
        error = sSettingsFile.Get(aKey, aIndex, aValue, aValueLength);
    }

exit:
    VerifyOrDie(error != OT_ERROR_PARSE, OT_EXIT_FAILURE);
    return error;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    if (isSensitiveKey(aKey))
    {
        error = otPosixSecureSettingsSet(aInstance, aKey, aValue, aValueLength);
    }
    else
#endif
    {
        sSettingsFile.Set(aKey, aValue, aValueLength);
    }

    return error;
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    if (isSensitiveKey(aKey))
    {
        error = otPosixSecureSettingsAdd(aInstance, aKey, aValue, aValueLength);
    }
    else
#endif
    {
        sSettingsFile.Add(aKey, aValue, aValueLength);
    }

    return error;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error;

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    if (isSensitiveKey(aKey))
    {
        error = otPosixSecureSettingsDelete(aInstance, aKey, aIndex);
    }
    else
#endif
    {
        error = sSettingsFile.Delete(aKey, aIndex);
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    otPosixSecureSettingsWipe(aInstance);
#endif

    sSettingsFile.Wipe();
}

namespace ot {
namespace Posix {
#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
void PlatformSettingsGetSensitiveKeys(otInstance *aInstance, const uint16_t **aKeys, uint16_t *aKeysLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aKeys != nullptr);
    assert(aKeysLength != nullptr);

    *aKeys       = sSensitiveKeys;
    *aKeysLength = sSensitiveKeysLength;
}
#endif

} // namespace Posix
} // namespace ot

#ifndef SELF_TEST
#define SELF_TEST 0
#endif

#if SELF_TEST

void otLogCritPlat(const char *aFormat, ...) { OT_UNUSED_VARIABLE(aFormat); }

const char *otExitCodeToString(uint8_t aExitCode)
{
    OT_UNUSED_VARIABLE(aExitCode);
    return "";
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    memset(aIeeeEui64, 0, sizeof(uint64_t));
}

// Stub implementation for testing
bool IsSystemDryRun(void) { return false; }

int main()
{
    otInstance *instance = nullptr;
    uint8_t     data[60];

    for (uint8_t i = 0; i < sizeof(data); ++i)
    {
        data[i] = i;
    }

    otPlatSettingsInit(instance, nullptr, 0);

    // verify empty situation
    otPlatSettingsWipe(instance);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsDelete(instance, 0, 0) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsDelete(instance, 0, -1) == OT_ERROR_NOT_FOUND);
    }

    // verify write one record
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 0, nullptr, nullptr) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, nullptr, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);

        length = sizeof(value);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        // insufficient buffer
        length -= 1;
        value[length] = 0;
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        // verify length becomes the actual length of the record
        assert(length == sizeof(data) / 2);
        // verify this byte is not changed
        assert(value[length] == 0);

        // wrong index
        assert(otPlatSettingsGet(instance, 0, 1, nullptr, nullptr) == OT_ERROR_NOT_FOUND);
        // wrong key
        assert(otPlatSettingsGet(instance, 1, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND);
    }
    otPlatSettingsWipe(instance);

    // verify write two records
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 1, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        length = sizeof(value);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data));
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify write two records of different keys
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 1, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 1, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        length = sizeof(value);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data));
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify delete record
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 3) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        // wrong key
        assert(otPlatSettingsDelete(instance, 1, 0) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsDelete(instance, 1, -1) == OT_ERROR_NOT_FOUND);

        // wrong index
        assert(otPlatSettingsDelete(instance, 0, 3) == OT_ERROR_NOT_FOUND);

        // delete one record
        assert(otPlatSettingsDelete(instance, 0, 1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 1, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 3);
        assert(0 == memcmp(value, data, length));

        // delete all records
        assert(otPlatSettingsDelete(instance, 0, -1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND);
    }
    otPlatSettingsWipe(instance);

    // verify delete all records of a type
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 1, data, sizeof(data) / 2) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 3) == OT_ERROR_NONE);
    {
        uint8_t  value[sizeof(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsDelete(instance, 0, -1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsGet(instance, 1, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));

        assert(otPlatSettingsDelete(instance, 0, 0) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsGet(instance, 0, 0, nullptr, nullptr) == OT_ERROR_NOT_FOUND);
    }
    otPlatSettingsWipe(instance);
    otPlatSettingsDeinit(instance);

    return 0;
}
#endif
