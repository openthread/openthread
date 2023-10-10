/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include "lib/url/url.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/common/code_utils.hpp"

namespace ot {
namespace Url {

Url::Url(void)
{
    mProtocol = nullptr;
    mPath     = nullptr;
    mQuery    = nullptr;
    mEnd      = nullptr;
}

otError Url::Init(char *aUrl)
{
    otError error = OT_ERROR_NONE;
    char   *url   = aUrl;

    mEnd      = aUrl + strlen(aUrl);
    mProtocol = aUrl;

    url = strstr(aUrl, "://");
    VerifyOrExit(url != nullptr, error = OT_ERROR_PARSE);
    *url = '\0';
    url += sizeof("://") - 1;
    mPath = url;

    url = strstr(url, "?");

    if (url != nullptr)
    {
        mQuery = ++url;

        for (char *cur = strtok(url, "&"); cur != nullptr; cur = strtok(nullptr, "&"))
        {
            cur[-1] = '\0';
        }
    }
    else
    {
        mQuery = mEnd;
    }

exit:
    return error;
}

const char *Url::GetValue(const char *aName, const char *aLastValue) const
{
    const char  *rval = nullptr;
    const size_t len  = strlen(aName);
    const char  *start;

    if (aLastValue == nullptr)
    {
        start = mQuery;
    }
    else
    {
        VerifyOrExit(aLastValue > mQuery && aLastValue < mEnd);
        start = aLastValue + strlen(aLastValue) + 1;
    }

    while (start < mEnd)
    {
        const char *last = nullptr;

        if (!strncmp(aName, start, len))
        {
            if (start[len] == '=')
            {
                ExitNow(rval = &start[len + 1]);
            }
            else if (start[len] == '\0')
            {
                ExitNow(rval = &start[len]);
            }
        }
        last  = start;
        start = last + strlen(last) + 1;
    }

exit:
    return rval;
}

otError Url::ParseUint32(const char *aName, uint32_t &aValue) const
{
    otError     error = OT_ERROR_NONE;
    const char *str;
    long long   value;

    VerifyOrExit((str = GetValue(aName)) != nullptr, error = OT_ERROR_NOT_FOUND);

    value = strtoll(str, nullptr, 0);
    VerifyOrExit(0 <= value && value <= UINT32_MAX, error = OT_ERROR_INVALID_ARGS);
    aValue = static_cast<uint32_t>(value);

exit:
    return error;
}

otError Url::ParseUint16(const char *aName, uint16_t &aValue) const
{
    otError  error = OT_ERROR_NONE;
    uint32_t value;

    SuccessOrExit(error = ParseUint32(aName, value));
    VerifyOrExit(value <= UINT16_MAX, error = OT_ERROR_INVALID_ARGS);
    aValue = static_cast<uint16_t>(value);

exit:
    return error;
}

otError Url::ParseUint8(const char *aName, uint8_t &aValue) const
{
    otError  error = OT_ERROR_NONE;
    uint32_t value;

    SuccessOrExit(error = ParseUint32(aName, value));
    VerifyOrExit(value <= UINT8_MAX, error = OT_ERROR_INVALID_ARGS);
    aValue = static_cast<uint8_t>(value);

exit:
    return error;
}

otError Url::ParseInt32(const char *aName, int32_t &aValue) const
{
    otError     error = OT_ERROR_NONE;
    const char *str;
    long long   value;

    VerifyOrExit((str = GetValue(aName)) != nullptr, error = OT_ERROR_NOT_FOUND);

    value = strtoll(str, nullptr, 0);
    VerifyOrExit(INT32_MIN <= value && value <= INT32_MAX, error = OT_ERROR_INVALID_ARGS);
    aValue = static_cast<int32_t>(value);

exit:
    return error;
}

otError Url::ParseInt16(const char *aName, int16_t &aValue) const
{
    otError error = OT_ERROR_NONE;
    int32_t value;

    SuccessOrExit(error = ParseInt32(aName, value));
    VerifyOrExit(INT16_MIN <= value && value <= INT16_MAX, error = OT_ERROR_INVALID_ARGS);
    aValue = static_cast<int16_t>(value);

exit:
    return error;
}

otError Url::ParseInt8(const char *aName, int8_t &aValue) const
{
    otError error = OT_ERROR_NONE;
    int32_t value;

    SuccessOrExit(error = ParseInt32(aName, value));
    VerifyOrExit(INT8_MIN <= value && value <= INT8_MAX, error = OT_ERROR_INVALID_ARGS);
    aValue = static_cast<int8_t>(value);

exit:
    return error;
}

} // namespace Url
} // namespace ot
