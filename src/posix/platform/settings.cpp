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

#include "openthread-core-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openthread/platform/misc.h>
#include <openthread/platform/settings.h>

#include "common/code_utils.hpp"

#define VerifyOrDie(aCondition)    \
    do                             \
    {                              \
        if (!(aCondition))         \
        {                          \
            perror(__func__);      \
            exit(OT_EXIT_FAILURE); \
        }                          \
    } while (false)

static const size_t kMaxFileNameSize = sizeof(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH) + 32;

static int sSettingsFd = -1;

static void getSettingsFileName(char aFileName[kMaxFileNameSize], bool aSwap)
{
    const char *offset = getenv("PORT_OFFSET");

    snprintf(aFileName, kMaxFileNameSize, OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH "/%s_%" PRIx64 ".%s",
             offset == NULL ? "0" : offset, gNodeId, (aSwap ? "swap" : "data"));
}

static int swapOpen(void)
{
    char fileName[kMaxFileNameSize];
    int  fd;

    getSettingsFileName(fileName, true);
    fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0600);
    VerifyOrDie(fd != -1);

    return fd;
}

/**
 * This function reads @p aLength bytes from the data file and appends to the swap file.
 *
 * @param[in]   aFd     The file descriptor of the current swap file.
 * @param[in]   aLength Number of bytes to copy.
 *
 */
static void swapWrite(int aFd, uint16_t aLength)
{
    const size_t kBlockSize = 512;
    uint8_t      buffer[kBlockSize];

    while (aLength > 0)
    {
        uint16_t count = aLength >= sizeof(buffer) ? sizeof(buffer) : aLength;
        ssize_t  rval  = read(sSettingsFd, buffer, count);

        VerifyOrDie(rval > 0);
        count = static_cast<uint16_t>(rval);
        rval  = write(aFd, buffer, count);
        assert(rval == count);
        VerifyOrDie(rval == count);
        aLength -= count;
    }
}

static void swapPersist(int aFd)
{
    char swapFile[kMaxFileNameSize];
    char dataFile[kMaxFileNameSize];

    getSettingsFileName(swapFile, true);
    getSettingsFileName(dataFile, false);

    VerifyOrDie(0 == close(sSettingsFd));
    VerifyOrDie(0 == rename(swapFile, dataFile));
    VerifyOrDie(0 == fsync(aFd));

    sSettingsFd = aFd;
}

static void swapDiscard(int aFd)
{
    char swapFileName[kMaxFileNameSize];

    VerifyOrDie(0 == close(aFd));
    getSettingsFileName(swapFileName, true);
    VerifyOrDie(0 == unlink(swapFileName));
}

void otPlatSettingsInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    {
        struct stat st;

        if (stat(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH, &st) == -1)
        {
            mkdir(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH, 0755);
        }
    }

    {
        char fileName[kMaxFileNameSize];

        getSettingsFileName(fileName, false);
        sSettingsFd = open(fileName, O_RDWR | O_CREAT, 0600);
    }

    VerifyOrDie(sSettingsFd != -1);

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

exit:
    if (error == OT_ERROR_PARSE)
    {
        VerifyOrDie(ftruncate(sSettingsFd, 0) == 0);
    }
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
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
    VerifyOrDie(error != OT_ERROR_PARSE);
    return error;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    otPlatSettingsDelete(aInstance, aKey, -1);
    return otPlatSettingsAdd(aInstance, aKey, aValue, aValueLength);
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    off_t size   = lseek(sSettingsFd, 0, SEEK_END);
    int   swapFd = swapOpen();

    if (size > 0)
    {
        VerifyOrDie(0 == lseek(sSettingsFd, 0, SEEK_SET));
        swapWrite(swapFd, static_cast<uint16_t>(size));
    }

    VerifyOrDie(write(swapFd, &aKey, sizeof(aKey)) == sizeof(aKey) &&
                write(swapFd, &aValueLength, sizeof(aValueLength)) == sizeof(aValueLength) &&
                write(swapFd, aValue, aValueLength) == aValueLength);

    swapPersist(swapFd);

    return OT_ERROR_NONE;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error  = OT_ERROR_NOT_FOUND;
    off_t   size   = lseek(sSettingsFd, 0, SEEK_END);
    off_t   offset = lseek(sSettingsFd, 0, SEEK_SET);
    int     swapFd = swapOpen();

    assert(swapFd != -1);
    assert(offset == 0);
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

        offset += sizeof(key) + sizeof(length) + length;

        if (aKey == key)
        {
            if (aIndex == 0)
            {
                VerifyOrExit(offset == lseek(sSettingsFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
                swapWrite(swapFd, static_cast<uint16_t>(size - offset));
                error = OT_ERROR_NONE;
                break;
            }
            else if (aIndex == -1)
            {
                VerifyOrExit(offset == lseek(sSettingsFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
                error = OT_ERROR_NONE;
                continue;
            }
            else
            {
                --aIndex;
            }
        }

        rval = write(swapFd, &key, sizeof(key));
        assert(rval == sizeof(key));
        VerifyOrDie(rval == sizeof(key));

        rval = write(swapFd, &length, sizeof(length));
        assert(rval == sizeof(length));
        VerifyOrDie(rval == sizeof(length));

        swapWrite(swapFd, length);
    }

exit:
    VerifyOrDie(error != OT_ERROR_PARSE);

    if (error == OT_ERROR_NONE)
    {
        swapPersist(swapFd);
    }
    else if (error == OT_ERROR_NOT_FOUND)
    {
        swapDiscard(swapFd);
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    VerifyOrDie(0 == ftruncate(sSettingsFd, 0));
}

#if SELF_TEST

uint64_t gNodeId = 1;

int main()
{
    otInstance *instance = NULL;
    uint8_t     data[60];

    for (size_t i = 0; i < sizeof(data); ++i)
    {
        data[i] = i;
    }

    otPlatSettingsInit(instance);

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

        assert(otPlatSettingsGet(instance, 0, 0, NULL, NULL) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, NULL, &length) == OT_ERROR_NONE);
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
        assert(otPlatSettingsGet(instance, 0, 1, NULL, NULL) == OT_ERROR_NOT_FOUND);
        // wrong key
        assert(otPlatSettingsGet(instance, 1, 0, NULL, NULL) == OT_ERROR_NOT_FOUND);
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
        assert(otPlatSettingsGet(instance, 0, 0, NULL, NULL) == OT_ERROR_NOT_FOUND);
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
        assert(otPlatSettingsGet(instance, 0, 0, NULL, NULL) == OT_ERROR_NOT_FOUND);
    }
    otPlatSettingsWipe(instance);

    return 0;
}
#endif
