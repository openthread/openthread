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

#include "platform-posix.h"

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <sys/stat.h>

#include <openthread/platform/settings.h>

#include "common/code_utils.hpp"

static int sFd = -1;

static otError offsetWrite(off_t &aOffset, uint16_t aLength)
{
    const size_t kBlockSize = 512;
    uint8_t      buffer[kBlockSize];
    otError      error = OT_ERROR_NONE;

    while (aLength > 0)
    {
        uint16_t count = aLength >= sizeof(buffer) ? sizeof(buffer) : aLength;
        ssize_t  rval  = read(sFd, buffer, count);

        assert(rval == count);
        VerifyOrExit(rval == count, error = OT_ERROR_PARSE);
        rval = pwrite(sFd, buffer, count, aOffset);
        assert(rval == count);
        VerifyOrExit(rval == count, error = OT_ERROR_PARSE);
        aOffset += count;
        aLength -= count;
    }

exit:
    return error;
}

void otPlatSettingsInit(otInstance *aInstance)
{
    const char *path = OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH;
    char        fileName[sizeof(OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH) + 32];
    struct stat st;
    const char *offset = getenv("PORT_OFFSET");

    OT_UNUSED_VARIABLE(aInstance);
    assert(sFd == -1);

    memset(&st, 0, sizeof(st));

    if (stat(path, &st) == -1)
    {
        mkdir(path, 0755);
    }

    if (offset == NULL)
    {
        offset = "0";
    }

    snprintf(fileName, sizeof(fileName), "%s/%s_%" PRIx64 ".flash", path, offset, NODE_ID);

    sFd = open(fileName, O_RDWR | O_CREAT, 0600);

    if (sFd < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    otError error = OT_ERROR_NOT_FOUND;

    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(lseek(sFd, 0, SEEK_SET) == 0);

    do
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(sFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key));

        rval = read(sFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length));

        if (key == aKey)
        {
            if (aIndex == 0)
            {
                error = OT_ERROR_NONE;

                if (aValueLength)
                {
                    if (*aValueLength >= length)
                    {
                        *aValueLength = length;
                    }

                    if (aValue)
                    {
                        VerifyOrExit(read(sFd, aValue, *aValueLength) == *aValueLength);
                    }
                }

                break;
            }
            else
            {
                --aIndex;
            }
        }

        VerifyOrExit(lseek(sFd, length, SEEK_CUR) > 0);
    } while (true);

exit:
    return error;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    otPlatSettingsDelete(aInstance, aKey, -1);
    return otPlatSettingsAdd(aInstance, aKey, aValue, aValueLength);
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    otError error  = OT_ERROR_PARSE;
    off_t   offset = lseek(sFd, 0, SEEK_END);

    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(offset >= 0);
    VerifyOrExit(write(sFd, &aKey, sizeof(aKey)) == sizeof(aKey));
    VerifyOrExit(write(sFd, &aValueLength, sizeof(aValueLength)) == sizeof(aValueLength));
    VerifyOrExit(write(sFd, aValue, aValueLength) == aValueLength);

    error = OT_ERROR_NONE;

exit:
    if (error != OT_ERROR_NONE)
    {
        if (ftruncate(sFd, offset) != 0)
        {
            perror("ftruncate");
        }
    }
    return error;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    otError error  = OT_ERROR_NOT_FOUND;
    off_t   size   = lseek(sFd, 0, SEEK_END);
    off_t   offset = lseek(sFd, 0, SEEK_SET);

    VerifyOrExit(offset == 0);

    for (off_t readOffset = offset; readOffset < size;)
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(sFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_PARSE);

        rval = read(sFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length), error = OT_ERROR_PARSE);

        readOffset += sizeof(key) + sizeof(length) + length;

        if (aKey == key)
        {
            if (aIndex == 0)
            {
                VerifyOrExit(readOffset == lseek(sFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
                error = offsetWrite(offset, size - readOffset);
                break;
            }
            else if (aIndex == -1)
            {
                VerifyOrExit(readOffset == lseek(sFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
                error = OT_ERROR_NONE;
                continue;
            }
            else
            {
                --aIndex;
            }
        }

        if (error == OT_ERROR_NONE)
        {
            rval = pwrite(sFd, &key, sizeof(key), offset);
            assert(rval == sizeof(key));
            VerifyOrExit(rval == sizeof(key), error = OT_ERROR_PARSE);
            offset += rval;

            rval = pwrite(sFd, &length, sizeof(length), offset);
            assert(rval == sizeof(length));
            VerifyOrExit(rval == sizeof(length), error = OT_ERROR_PARSE);
            offset += rval;

            SuccessOrExit(error = offsetWrite(offset, length));
        }
        else
        {
            VerifyOrExit(readOffset == lseek(sFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
            offset = readOffset;
        }
    }

    if (error == OT_ERROR_NONE)
    {
        VerifyOrExit(0 == ftruncate(sFd, offset), error = OT_ERROR_PARSE);
    }

exit:
    if (error == OT_ERROR_PARSE)
    {
        otPlatSettingsWipe(aInstance);
    }

    return error;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    ftruncate(sFd, 0);
}

#if SELF_TEST
uint64_t NODE_ID = 1;

int main()
{
    otInstance *instance = NULL;
    uint8_t     data[60];

    for (size_t i = 0; i < OT_ARRAY_LENGTH(data); ++i)
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
    }

    // verify write one record
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[OT_ARRAY_LENGTH(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 0, NULL, NULL) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, NULL, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify write two records
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[OT_ARRAY_LENGTH(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 0, 1, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify write two records of different keys
    assert(otPlatSettingsSet(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 1, data, sizeof(data) / 2) == OT_ERROR_NONE);
    {
        uint8_t  value[OT_ARRAY_LENGTH(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsGet(instance, 1, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify delete record
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 2) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 3) == OT_ERROR_NONE);
    {
        uint8_t  value[OT_ARRAY_LENGTH(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsDelete(instance, 0, 1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 1, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 3);
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    // verify delete all records of a type
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data)) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 1, data, sizeof(data) / 2) == OT_ERROR_NONE);
    assert(otPlatSettingsAdd(instance, 0, data, sizeof(data) / 3) == OT_ERROR_NONE);
    {
        uint8_t  value[OT_ARRAY_LENGTH(data)];
        uint16_t length = sizeof(value);

        assert(otPlatSettingsDelete(instance, 0, -1) == OT_ERROR_NONE);
        assert(otPlatSettingsGet(instance, 0, 0, value, &length) == OT_ERROR_NOT_FOUND);
        assert(otPlatSettingsGet(instance, 1, 0, value, &length) == OT_ERROR_NONE);
        assert(length == sizeof(data) / 2);
        assert(0 == memcmp(value, data, length));
    }
    otPlatSettingsWipe(instance);

    return 0;
}
#endif
