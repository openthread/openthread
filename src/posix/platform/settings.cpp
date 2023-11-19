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
 *
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

#include "system.hpp"

static const size_t kMaxFileNameSize = sizeof(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH) + 32;

static int sSettingsFd = -1;

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

static void getSettingsFileName(otInstance *aInstance, char aFileName[kMaxFileNameSize], bool aSwap)
{
    const char *offset = getenv("PORT_OFFSET");
    uint64_t    nodeId;

    otPlatRadioGetIeeeEui64(aInstance, reinterpret_cast<uint8_t *>(&nodeId));
    nodeId = ot::BigEndian::HostSwap64(nodeId);
    snprintf(aFileName, kMaxFileNameSize, OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH "/%s_%" PRIx64 ".%s",
             offset == nullptr ? "0" : offset, nodeId, (aSwap ? "swap" : "data"));
}

static int swapOpen(otInstance *aInstance)
{
    char fileName[kMaxFileNameSize];
    int  fd;

    getSettingsFileName(aInstance, fileName, true);

    fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    VerifyOrDie(fd != -1, OT_EXIT_ERROR_ERRNO);

    return fd;
}

/**
 * Reads @p aLength bytes from the data file and appends to the swap file.
 *
 * @param[in]   aFd     The file descriptor of the current swap file.
 * @param[in]   aLength Number of bytes to copy.
 *
 */
static void swapWrite(otInstance *aInstance, int aFd, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    const size_t kBlockSize = 512;
    uint8_t      buffer[kBlockSize];

    while (aLength > 0)
    {
        uint16_t count = aLength >= sizeof(buffer) ? sizeof(buffer) : aLength;
        ssize_t  rval  = read(sSettingsFd, buffer, count);

        VerifyOrDie(rval > 0, OT_EXIT_FAILURE);
        count = static_cast<uint16_t>(rval);
        rval  = write(aFd, buffer, count);
        assert(rval == count);
        VerifyOrDie(rval == count, OT_EXIT_FAILURE);
        aLength -= count;
    }
}

static void swapPersist(otInstance *aInstance, int aFd)
{
    char swapFile[kMaxFileNameSize];
    char dataFile[kMaxFileNameSize];

    getSettingsFileName(aInstance, swapFile, true);
    getSettingsFileName(aInstance, dataFile, false);

    VerifyOrDie(0 == close(sSettingsFd), OT_EXIT_ERROR_ERRNO);
    VerifyOrDie(0 == fsync(aFd), OT_EXIT_ERROR_ERRNO);
    VerifyOrDie(0 == rename(swapFile, dataFile), OT_EXIT_ERROR_ERRNO);

    sSettingsFd = aFd;
}

static void swapDiscard(otInstance *aInstance, int aFd)
{
    char swapFileName[kMaxFileNameSize];

    VerifyOrDie(0 == close(aFd), OT_EXIT_ERROR_ERRNO);
    getSettingsFileName(aInstance, swapFileName, true);
    VerifyOrDie(0 == unlink(swapFileName), OT_EXIT_ERROR_ERRNO);
}

void otPlatSettingsInit(otInstance *aInstance, const uint16_t *aSensitiveKeys, uint16_t aSensitiveKeysLength)
{
#if !OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    OT_UNUSED_VARIABLE(aSensitiveKeys);
    OT_UNUSED_VARIABLE(aSensitiveKeysLength);
#endif

    otError error = OT_ERROR_NONE;

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    sSensitiveKeys       = aSensitiveKeys;
    sSensitiveKeysLength = aSensitiveKeysLength;
#endif

    // Don't touch the settings file the system runs in dry-run mode.
    VerifyOrExit(!IsSystemDryRun());

    {
        struct stat st;

        if (stat(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH, &st) == -1)
        {
            VerifyOrDie(mkdir(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH, 0755) == 0, OT_EXIT_ERROR_ERRNO);
        }
    }

    {
        char fileName[kMaxFileNameSize];

        getSettingsFileName(aInstance, fileName, false);
        sSettingsFd = open(fileName, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    }

    VerifyOrDie(sSettingsFd != -1, OT_EXIT_ERROR_ERRNO);

    for (off_t size = lseek(sSettingsFd, 0, SEEK_END), offset = lseek(sSettingsFd, 0, SEEK_SET); offset < size;)
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(sSettingsFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_PARSE);

        rval = read(sSettingsFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length), error = OT_ERROR_PARSE);

        offset += sizeof(key) + sizeof(length) + length;
        VerifyOrExit(offset == lseek(sSettingsFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
    }

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    otPosixSecureSettingsInit(aInstance);
#endif

exit:
    if (error == OT_ERROR_PARSE)
    {
        VerifyOrDie(ftruncate(sSettingsFd, 0) == 0, OT_EXIT_ERROR_ERRNO);
    }
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(!IsSystemDryRun());

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    otPosixSecureSettingsDeinit(aInstance);
#endif

    VerifyOrExit(sSettingsFd != -1);
    VerifyOrDie(close(sSettingsFd) == 0, OT_EXIT_ERROR_ERRNO);

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
        error = ot::Posix::PlatformSettingsGet(aInstance, aKey, aIndex, aValue, aValueLength);
    }

exit:
    VerifyOrDie(error != OT_ERROR_PARSE, OT_EXIT_FAILURE);
    return error;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    if (isSensitiveKey(aKey))
    {
        error = otPosixSecureSettingsSet(aInstance, aKey, aValue, aValueLength);
    }
    else
#endif
    {
        ot::Posix::PlatformSettingsSet(aInstance, aKey, aValue, aValueLength);
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
        ot::Posix::PlatformSettingsAdd(aInstance, aKey, aValue, aValueLength);
    }

    return error;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    otError error;

#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    if (isSensitiveKey(aKey))
    {
        error = otPosixSecureSettingsDelete(aInstance, aKey, aIndex);
    }
    else
#endif
    {
        error = ot::Posix::PlatformSettingsDelete(aInstance, aKey, aIndex, nullptr);
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
#if OPENTHREAD_POSIX_CONFIG_SECURE_SETTINGS_ENABLE
    otPosixSecureSettingsWipe(aInstance);
#endif

    VerifyOrDie(0 == ftruncate(sSettingsFd, 0), OT_EXIT_ERROR_ERRNO);
}

namespace ot {
namespace Posix {

otError PlatformSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError     error  = OT_ERROR_NOT_FOUND;
    const off_t size   = lseek(sSettingsFd, 0, SEEK_END);
    off_t       offset = lseek(sSettingsFd, 0, SEEK_SET);

    VerifyOrExit(offset == 0 && size >= 0, error = OT_ERROR_PARSE);

    while (offset < size)
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(sSettingsFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_PARSE);

        rval = read(sSettingsFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length), error = OT_ERROR_PARSE);

        if (key == aKey)
        {
            if (aIndex == 0)
            {
                error = OT_ERROR_NONE;

                if (aValueLength)
                {
                    if (aValue)
                    {
                        uint16_t readLength = (length <= *aValueLength ? length : *aValueLength);

                        VerifyOrExit(read(sSettingsFd, aValue, readLength) == readLength, error = OT_ERROR_PARSE);
                    }

                    *aValueLength = length;
                }

                break;
            }
            else
            {
                --aIndex;
            }
        }

        offset += sizeof(key) + sizeof(length) + length;
        VerifyOrExit(offset == lseek(sSettingsFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
    }

exit:
    return error;
}

void PlatformSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    int swapFd = -1;

    switch (PlatformSettingsDelete(aInstance, aKey, -1, &swapFd))
    {
    case OT_ERROR_NONE:
    case OT_ERROR_NOT_FOUND:
        break;

    default:
        assert(false);
        break;
    }

    VerifyOrDie(write(swapFd, &aKey, sizeof(aKey)) == sizeof(aKey) &&
                    write(swapFd, &aValueLength, sizeof(aValueLength)) == sizeof(aValueLength) &&
                    write(swapFd, aValue, aValueLength) == aValueLength,
                OT_EXIT_FAILURE);

    swapPersist(aInstance, swapFd);
}

void PlatformSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    off_t size   = lseek(sSettingsFd, 0, SEEK_END);
    int   swapFd = swapOpen(aInstance);

    if (size > 0)
    {
        VerifyOrDie(0 == lseek(sSettingsFd, 0, SEEK_SET), OT_EXIT_ERROR_ERRNO);
        swapWrite(aInstance, swapFd, static_cast<uint16_t>(size));
    }

    VerifyOrDie(write(swapFd, &aKey, sizeof(aKey)) == sizeof(aKey) &&
                    write(swapFd, &aValueLength, sizeof(aValueLength)) == sizeof(aValueLength) &&
                    write(swapFd, aValue, aValueLength) == aValueLength,
                OT_EXIT_FAILURE);

    swapPersist(aInstance, swapFd);
}

otError PlatformSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex, int *aSwapFd)
{
    otError error  = OT_ERROR_NOT_FOUND;
    off_t   size   = lseek(sSettingsFd, 0, SEEK_END);
    off_t   offset = lseek(sSettingsFd, 0, SEEK_SET);
    int     swapFd = swapOpen(aInstance);

    assert(swapFd != -1);
    assert(offset == 0);
    VerifyOrExit(offset == 0 && size >= 0, error = OT_ERROR_FAILED);

    while (offset < size)
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(sSettingsFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_FAILED);

        rval = read(sSettingsFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length), error = OT_ERROR_FAILED);

        offset += sizeof(key) + sizeof(length) + length;

        if (aKey == key)
        {
            if (aIndex == 0)
            {
                VerifyOrExit(offset == lseek(sSettingsFd, length, SEEK_CUR), error = OT_ERROR_FAILED);
                swapWrite(aInstance, swapFd, static_cast<uint16_t>(size - offset));
                error = OT_ERROR_NONE;
                break;
            }
            else if (aIndex == -1)
            {
                VerifyOrExit(offset == lseek(sSettingsFd, length, SEEK_CUR), error = OT_ERROR_FAILED);
                error = OT_ERROR_NONE;
                continue;
            }
            else
            {
                --aIndex;
            }
        }

        rval = write(swapFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_FAILED);

        rval = write(swapFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length), error = OT_ERROR_FAILED);

        swapWrite(aInstance, swapFd, length);
    }

exit:
    if (aSwapFd != nullptr)
    {
        *aSwapFd = swapFd;
    }
    else if (error == OT_ERROR_NONE)
    {
        swapPersist(aInstance, swapFd);
    }
    else if (error == OT_ERROR_NOT_FOUND)
    {
        swapDiscard(aInstance, swapFd);
    }
    else if (error == OT_ERROR_FAILED)
    {
        swapDiscard(aInstance, swapFd);
        DieNow(error);
    }

    return error;
}

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
