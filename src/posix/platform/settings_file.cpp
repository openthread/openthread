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
 *   This file implements the settings file module for getting, setting and deleting the key-value pairs.
 */

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "posix/platform/settings_file.hpp"

namespace ot {
namespace Posix {

otError SettingsFile::Init(const char *aSettingsFileBaseName)
{
    otError     error     = OT_ERROR_NONE;
    const char *directory = OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH;

    OT_ASSERT((aSettingsFileBaseName != nullptr) && (strlen(aSettingsFileBaseName) < kMaxFileBaseNameSize));
    strncpy(mSettingFileBaseName, aSettingsFileBaseName, sizeof(mSettingFileBaseName) - 1);

    {
        struct stat st;

        if (stat(directory, &st) == -1)
        {
            VerifyOrDie(mkdir(directory, 0755) == 0, OT_EXIT_ERROR_ERRNO);
        }
    }

    {
        char fileName[kMaxFilePathSize];

        GetSettingsFilePath(fileName, false);
        mSettingsFd = open(fileName, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    }

    VerifyOrDie(mSettingsFd != -1, OT_EXIT_ERROR_ERRNO);

    for (off_t size = lseek(mSettingsFd, 0, SEEK_END), offset = lseek(mSettingsFd, 0, SEEK_SET); offset < size;)
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(mSettingsFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_PARSE);

        rval = read(mSettingsFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length), error = OT_ERROR_PARSE);

        offset += sizeof(key) + sizeof(length) + length;
        VerifyOrExit(offset == lseek(mSettingsFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
    }

exit:
    if (error == OT_ERROR_PARSE)
    {
        VerifyOrDie(ftruncate(mSettingsFd, 0) == 0, OT_EXIT_ERROR_ERRNO);
    }

    return error;
}

void SettingsFile::Deinit(void)
{
    VerifyOrExit(mSettingsFd != -1);
    VerifyOrDie(close(mSettingsFd) == 0, OT_EXIT_ERROR_ERRNO);
    mSettingsFd = -1;

exit:
    return;
}

otError SettingsFile::Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    otError error = OT_ERROR_NOT_FOUND;
    off_t   size;
    off_t   offset;

    OT_ASSERT(mSettingsFd >= 0);

    size   = lseek(mSettingsFd, 0, SEEK_END);
    offset = lseek(mSettingsFd, 0, SEEK_SET);
    VerifyOrExit(offset == 0 && size >= 0, error = OT_ERROR_PARSE);

    while (offset < size)
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(mSettingsFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_PARSE);

        rval = read(mSettingsFd, &length, sizeof(length));
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

                        VerifyOrExit(read(mSettingsFd, aValue, readLength) == readLength, error = OT_ERROR_PARSE);
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
        VerifyOrExit(offset == lseek(mSettingsFd, length, SEEK_CUR), error = OT_ERROR_PARSE);
    }

exit:
    return error;
}

void SettingsFile::Set(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    int swapFd = -1;

    OT_ASSERT(mSettingsFd >= 0);

    switch (Delete(aKey, -1, &swapFd))
    {
    case OT_ERROR_NONE:
    case OT_ERROR_NOT_FOUND:
        break;

    default:
        OT_ASSERT(false);
        break;
    }

    VerifyOrDie(write(swapFd, &aKey, sizeof(aKey)) == sizeof(aKey) &&
                    write(swapFd, &aValueLength, sizeof(aValueLength)) == sizeof(aValueLength) &&
                    write(swapFd, aValue, aValueLength) == aValueLength,
                OT_EXIT_FAILURE);

    SwapPersist(swapFd);
}

void SettingsFile::Add(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    off_t size;
    int   swapFd;

    OT_ASSERT(mSettingsFd >= 0);

    size   = lseek(mSettingsFd, 0, SEEK_END);
    swapFd = SwapOpen();

    if (size > 0)
    {
        VerifyOrDie(0 == lseek(mSettingsFd, 0, SEEK_SET), OT_EXIT_ERROR_ERRNO);
        SwapWrite(swapFd, static_cast<uint16_t>(size));
    }

    VerifyOrDie(write(swapFd, &aKey, sizeof(aKey)) == sizeof(aKey) &&
                    write(swapFd, &aValueLength, sizeof(aValueLength)) == sizeof(aValueLength) &&
                    write(swapFd, aValue, aValueLength) == aValueLength,
                OT_EXIT_FAILURE);

    SwapPersist(swapFd);
}

otError SettingsFile::Delete(uint16_t aKey, int aIndex) { return Delete(aKey, aIndex, nullptr); }

otError SettingsFile::Delete(uint16_t aKey, int aIndex, int *aSwapFd)
{
    otError error = OT_ERROR_NOT_FOUND;
    off_t   size;
    off_t   offset;
    int     swapFd;

    OT_ASSERT(mSettingsFd >= 0);

    size   = lseek(mSettingsFd, 0, SEEK_END);
    offset = lseek(mSettingsFd, 0, SEEK_SET);
    swapFd = SwapOpen();

    OT_ASSERT(swapFd != -1);
    OT_ASSERT(offset == 0);
    VerifyOrExit(offset == 0 && size >= 0, error = OT_ERROR_FAILED);

    while (offset < size)
    {
        uint16_t key;
        uint16_t length;
        ssize_t  rval;

        rval = read(mSettingsFd, &key, sizeof(key));
        VerifyOrExit(rval == sizeof(key), error = OT_ERROR_FAILED);

        rval = read(mSettingsFd, &length, sizeof(length));
        VerifyOrExit(rval == sizeof(length), error = OT_ERROR_FAILED);

        offset += sizeof(key) + sizeof(length) + length;

        if (aKey == key)
        {
            if (aIndex == 0)
            {
                VerifyOrExit(offset == lseek(mSettingsFd, length, SEEK_CUR), error = OT_ERROR_FAILED);
                SwapWrite(swapFd, static_cast<uint16_t>(size - offset));
                error = OT_ERROR_NONE;
                break;
            }
            else if (aIndex == -1)
            {
                VerifyOrExit(offset == lseek(mSettingsFd, length, SEEK_CUR), error = OT_ERROR_FAILED);
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

        SwapWrite(swapFd, length);
    }

exit:
    if (aSwapFd != nullptr)
    {
        *aSwapFd = swapFd;
    }
    else if (error == OT_ERROR_NONE)
    {
        SwapPersist(swapFd);
    }
    else if (error == OT_ERROR_NOT_FOUND)
    {
        SwapDiscard(swapFd);
    }
    else if (error == OT_ERROR_FAILED)
    {
        SwapDiscard(swapFd);
        DieNow(error);
    }

    return error;
}

void SettingsFile::Wipe(void) { VerifyOrDie(0 == ftruncate(mSettingsFd, 0), OT_EXIT_ERROR_ERRNO); }

void SettingsFile::GetSettingsFilePath(char aFileName[kMaxFilePathSize], bool aSwap)
{
    snprintf(aFileName, kMaxFilePathSize, OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH "/%s.%s", mSettingFileBaseName,
             (aSwap ? "Swap" : "data"));
}

int SettingsFile::SwapOpen(void)
{
    char fileName[kMaxFilePathSize];
    int  fd;

    GetSettingsFilePath(fileName, true);

    fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    VerifyOrDie(fd != -1, OT_EXIT_ERROR_ERRNO);

    return fd;
}

void SettingsFile::SwapWrite(int aFd, uint16_t aLength)
{
    const size_t kBlockSize = 512;
    uint8_t      buffer[kBlockSize];

    while (aLength > 0)
    {
        uint16_t count = aLength >= sizeof(buffer) ? sizeof(buffer) : aLength;
        ssize_t  rval  = read(mSettingsFd, buffer, count);

        VerifyOrDie(rval > 0, OT_EXIT_FAILURE);
        count = static_cast<uint16_t>(rval);
        rval  = write(aFd, buffer, count);
        OT_ASSERT(rval == count);
        VerifyOrDie(rval == count, OT_EXIT_FAILURE);
        aLength -= count;
    }
}

void SettingsFile::SwapPersist(int aFd)
{
    char swapFile[kMaxFilePathSize];
    char dataFile[kMaxFilePathSize];

    GetSettingsFilePath(swapFile, true);
    GetSettingsFilePath(dataFile, false);

    VerifyOrDie(0 == close(mSettingsFd), OT_EXIT_ERROR_ERRNO);
    VerifyOrDie(0 == fsync(aFd), OT_EXIT_ERROR_ERRNO);
    VerifyOrDie(0 == rename(swapFile, dataFile), OT_EXIT_ERROR_ERRNO);

    mSettingsFd = aFd;
}

void SettingsFile::SwapDiscard(int aFd)
{
    char swapFileName[kMaxFilePathSize];

    VerifyOrDie(0 == close(aFd), OT_EXIT_ERROR_ERRNO);
    GetSettingsFilePath(swapFileName, true);
    VerifyOrDie(0 == unlink(swapFileName), OT_EXIT_ERROR_ERRNO);
}

} // namespace Posix
} // namespace ot
