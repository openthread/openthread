/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *  This file implements OpenThread String class.
 */

#include "string.hpp"

namespace ot {

uint16_t StringLength(const char *aString, uint16_t aMaxLength)
{
    uint16_t ret;

    for (ret = 0; (ret < aMaxLength) && (aString[ret] != 0); ret++)
    {
        // Empty loop.
    }

    return ret;
}

otError StringBase::Write(char *aBuffer, uint16_t aSize, uint16_t &aLength, const char *aFormat, va_list aArgs)
{
    otError error = OT_ERROR_NONE;
    int     len;

    len = vsnprintf(aBuffer + aLength, aSize - aLength, aFormat, aArgs);

    if (len < 0)
    {
        aLength    = 0;
        aBuffer[0] = 0;
        error      = OT_ERROR_INVALID_ARGS;
    }
    else if (len >= aSize - aLength)
    {
        aLength = aSize - 1;
        error   = OT_ERROR_NO_BUFS;
    }
    else
    {
        aLength += static_cast<uint16_t>(len);
    }

    return error;
}

} // namespace ot
