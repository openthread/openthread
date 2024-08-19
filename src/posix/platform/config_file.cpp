/*
 *  Copyright (c) 2022, The OpenThread Authors.
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

#include "config_file.hpp"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.hpp"
#include <openthread/logging.h>
#include "common/code_utils.hpp"
#include "lib/platform/exit_code.h"

namespace ot {
namespace Posix {

ConfigFile::ConfigFile(const char *aFilePath)
    : mFilePath(aFilePath)
{
    assert(mFilePath != nullptr);
    VerifyOrDie(strlen(mFilePath) + strlen(kSwapSuffix) < kFilePathMaxSize, OT_EXIT_FAILURE);
}

bool ConfigFile::HasKey(const char *aKey) const
{
    int iterator = 0;

    return (Get(aKey, iterator, nullptr, 0) == OT_ERROR_NONE);
}

bool ConfigFile::DoesExist(void) const { return (access(mFilePath, 0) == 0); }

otError ConfigFile::Get(const char *aKey, int &aIterator, char *aValue, int aValueLength) const
{
    otError  error = OT_ERROR_NONE;
    char     line[kLineMaxSize + 1];
    FILE    *fp = nullptr;
    char    *ret;
    char    *psave;
    long int pos;

    VerifyOrExit(aKey != nullptr, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((fp = fopen(mFilePath, "r")) != nullptr, error = OT_ERROR_NOT_FOUND);
    VerifyOrDie(fseek(fp, aIterator, SEEK_SET) == 0, OT_EXIT_ERROR_ERRNO);

    while ((ret = fgets(line, sizeof(line), fp)) != nullptr)
    {
        char *str;
        char *key;
        char *value;

        // If the string exceeds the `sizeof(line) - 1`, the string will be truncated to `sizeof(line) - 1` bytes string
        // by the function `fgets()`.
        if (strlen(line) + 1 == sizeof(line))
        {
            // The line is too long.
            continue;
        }

        // Remove comments
        strtok_r(line, kCommentDelimiter, &psave);

        if ((str = strstr(line, "=")) == nullptr)
        {
            continue;
        }

        *str = '\0';
        key  = line;

        Strip(key);

        if (strcmp(aKey, key) == 0)
        {
            if (aValue != nullptr)
            {
                value = str + 1;
                Strip(value);
                aValueLength = OT_MIN(static_cast<int>(strlen(value)), (aValueLength - 1));
                memcpy(aValue, value, static_cast<size_t>(aValueLength));
                aValue[aValueLength] = '\0';
            }
            break;
        }
    }

    VerifyOrExit(ret != nullptr, error = OT_ERROR_NOT_FOUND);
    VerifyOrDie((pos = ftell(fp)) >= 0, OT_EXIT_ERROR_ERRNO);
    aIterator = static_cast<int>(pos);

exit:
    if (fp != nullptr)
    {
        fclose(fp);
    }

    return error;
}

otError ConfigFile::Add(const char *aKey, const char *aValue)
{
    otError     error = OT_ERROR_NONE;
    FILE       *fp    = nullptr;
    char       *path  = nullptr;
    char       *dir;
    struct stat st;

    VerifyOrExit((aKey != nullptr) && (aValue != nullptr), error = OT_ERROR_INVALID_ARGS);
    VerifyOrDie((path = strdup(mFilePath)) != nullptr, OT_EXIT_ERROR_ERRNO);
    dir = dirname(path);

    if (stat(dir, &st) == -1)
    {
        VerifyOrDie(mkdir(dir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == 0, OT_EXIT_ERROR_ERRNO);
    }

    VerifyOrDie((fp = fopen(mFilePath, "at")) != NULL, OT_EXIT_ERROR_ERRNO);
    VerifyOrDie(fprintf(fp, "%s=%s\n", aKey, aValue) > 0, OT_EXIT_ERROR_ERRNO);

exit:
    if (fp != nullptr)
    {
        fclose(fp);
    }

    if (path != nullptr)
    {
        free(path);
    }

    return error;
}

otError ConfigFile::Clear(const char *aKey)
{
    otError error = OT_ERROR_NONE;
    char    swapFile[kFilePathMaxSize];
    char    line[kLineMaxSize];
    FILE   *fp     = nullptr;
    FILE   *fpSwap = nullptr;

    VerifyOrExit(aKey != nullptr, error = OT_ERROR_INVALID_ARGS);
    VerifyOrDie((fp = fopen(mFilePath, "r")) != NULL, OT_EXIT_ERROR_ERRNO);
    snprintf(swapFile, sizeof(swapFile), "%s%s", mFilePath, kSwapSuffix);
    VerifyOrDie((fpSwap = fopen(swapFile, "w+")) != NULL, OT_EXIT_ERROR_ERRNO);

    while (fgets(line, sizeof(line), fp) != nullptr)
    {
        bool  containsKey;
        char *str1;
        char *str2;

        str1 = strstr(line, kCommentDelimiter);
        str2 = strstr(line, aKey);

        // If only the comment contains the key string, ignore it.
        containsKey = (str2 != nullptr) && (str1 == nullptr || str2 < str1);

        if (!containsKey)
        {
            fputs(line, fpSwap);
        }
    }

exit:
    if (fp != nullptr)
    {
        fclose(fp);
    }

    if (fpSwap != nullptr)
    {
        fclose(fpSwap);
    }

    if (error == OT_ERROR_NONE)
    {
        VerifyOrDie(rename(swapFile, mFilePath) == 0, OT_EXIT_ERROR_ERRNO);
    }

    return error;
}

void ConfigFile::Strip(char *aString) const
{
    int count = 0;

    for (int i = 0; aString[i]; i++)
    {
        if (aString[i] != ' ' && aString[i] != '\r' && aString[i] != '\n')
        {
            aString[count++] = aString[i];
        }
    }

    aString[count] = '\0';
}
} // namespace Posix
} // namespace ot
