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
 *   This file implements the command line parser.
 */

#include "parse_cmdline.hpp"

#include <limits>
#include <string.h>

#include "common/code_utils.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace Utils {
namespace CmdLineParser {

static bool IsSeparator(char aChar)
{
    return (aChar == ' ') || (aChar == '\t') || (aChar == '\r') || (aChar == '\n');
}

static bool IsEscapable(char aChar)
{
    return IsSeparator(aChar) || (aChar == '\\');
}

static otError ParseDigit(char aDigitChar, uint8_t &aValue)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(('0' <= aDigitChar) && (aDigitChar <= '9'), error = OT_ERROR_INVALID_ARGS);
    aValue = static_cast<uint8_t>(aDigitChar - '0');

exit:
    return error;
}

static otError ParseHexDigit(char aHexChar, uint8_t &aValue)
{
    otError error = OT_ERROR_NONE;

    if (('A' <= aHexChar) && (aHexChar <= 'F'))
    {
        ExitNow(aValue = static_cast<uint8_t>(aHexChar - 'A' + 10));
    }

    if (('a' <= aHexChar) && (aHexChar <= 'f'))
    {
        ExitNow(aValue = static_cast<uint8_t>(aHexChar - 'a' + 10));
    }

    error = ParseDigit(aHexChar, aValue);

exit:
    return error;
}

otError ParseCmd(char *aString, uint8_t &aArgsLength, char *aArgs[], uint8_t aArgsLengthMax)
{
    otError error = OT_ERROR_NONE;
    char *  cmd;

    aArgsLength = 0;

    for (cmd = aString; *cmd; cmd++)
    {
        if ((*cmd == '\\') && IsEscapable(*(cmd + 1)))
        {
            // include the null terminator: strlen(cmd) = strlen(cmd + 1) + 1
            memmove(cmd, cmd + 1, strlen(cmd));
        }
        else if (IsSeparator(*cmd))
        {
            *cmd = '\0';
        }

        if ((*cmd != '\0') && ((aArgsLength == 0) || (*(cmd - 1) == '\0')))
        {
            VerifyOrExit(aArgsLength < aArgsLengthMax, error = OT_ERROR_INVALID_ARGS);
            aArgs[aArgsLength++] = cmd;
        }
    }

exit:
    return error;
}

template <typename UintType> otError ParseUint(const char *aString, UintType &aUint)
{
    otError  error;
    uint64_t value;

    SuccessOrExit(error = ParseAsUint64(aString, value));

    VerifyOrExit(value <= std::numeric_limits<UintType>::max(), error = OT_ERROR_INVALID_ARGS);
    aUint = static_cast<UintType>(value);

exit:
    return error;
}

otError ParseAsUint8(const char *aString, uint8_t &aUint8)
{
    return ParseUint<uint8_t>(aString, aUint8);
}

otError ParseAsUint16(const char *aString, uint16_t &aUint16)
{
    return ParseUint<uint16_t>(aString, aUint16);
}

otError ParseAsUint32(const char *aString, uint32_t &aUint32)
{
    return ParseUint<uint32_t>(aString, aUint32);
}

otError ParseAsUint64(const char *aString, uint64_t &aUint64)
{
    otError     error = OT_ERROR_NONE;
    uint64_t    value = 0;
    const char *cur   = aString;
    bool        isHex = false;

    enum : uint64_t
    {
        kMaxHexBeforeOveflow = (0xffffffffffffffffULL / 16),
        kMaxDecBeforeOverlow = (0xffffffffffffffffULL / 10),
    };

    if (cur[0] == '0' && (cur[1] == 'x' || cur[1] == 'X'))
    {
        cur += 2;
        isHex = true;
    }

    do
    {
        uint8_t  digit;
        uint64_t newValue;

        SuccessOrExit(error = isHex ? ParseHexDigit(*cur, digit) : ParseDigit(*cur, digit));
        VerifyOrExit(value <= (isHex ? kMaxHexBeforeOveflow : kMaxDecBeforeOverlow), error = OT_ERROR_INVALID_ARGS);
        value    = isHex ? (value << 4) : (value * 10);
        newValue = value + digit;
        VerifyOrExit(newValue >= value, error = OT_ERROR_INVALID_ARGS);
        value = newValue;
        cur++;
    } while (*cur != '\0');

    aUint64 = value;

exit:
    return error;
}

template <typename IntType> otError ParseInt(const char *aString, IntType &aInt)
{
    otError error;
    int32_t value;

    SuccessOrExit(error = ParseAsInt32(aString, value));

    VerifyOrExit((std::numeric_limits<IntType>::min() <= value) && (value <= std::numeric_limits<IntType>::max()),
                 error = OT_ERROR_INVALID_ARGS);
    aInt = static_cast<IntType>(value);

exit:
    return error;
}

otError ParseAsInt8(const char *aString, int8_t &aInt8)
{
    return ParseInt<int8_t>(aString, aInt8);
}

otError ParseAsInt16(const char *aString, int16_t &aInt16)
{
    return ParseInt<int16_t>(aString, aInt16);
}

otError ParseAsInt32(const char *aString, int32_t &aInt32)
{
    otError  error;
    uint64_t value;
    bool     isNegavtive = false;

    if (*aString == '-')
    {
        aString++;
        isNegavtive = true;
    }
    else if (*aString == '+')
    {
        aString++;
    }

    SuccessOrExit(error = ParseAsUint64(aString, value));
    VerifyOrExit(value <= (isNegavtive
                               ? static_cast<uint64_t>(-static_cast<int64_t>(std::numeric_limits<int32_t>::min()))
                               : static_cast<uint64_t>(std::numeric_limits<int32_t>::max())),
                 error = OT_ERROR_INVALID_ARGS);
    aInt32 = isNegavtive ? -static_cast<int32_t>(value) : static_cast<int32_t>(value);

exit:
    return error;
}

otError ParseAsBool(const char *aString, bool &aBool)
{
    otError  error;
    uint32_t value;

    SuccessOrExit(error = ParseAsUint32(aString, value));
    aBool = (value != 0);

exit:
    return error;
}
#if OPENTHREAD_FTD || OPENTHREAD_MTD

otError ParseAsIp6Prefix(const char *aString, otIp6Prefix &aPrefix)
{
    enum : uint8_t
    {
        kMaxIp6AddressStringSize = 45,
    };

    otError     error = OT_ERROR_INVALID_ARGS;
    char        string[kMaxIp6AddressStringSize];
    const char *prefixLengthStr;

    prefixLengthStr = strchr(aString, '/');
    VerifyOrExit(prefixLengthStr != nullptr, OT_NOOP);

    VerifyOrExit(prefixLengthStr - aString < static_cast<int32_t>(sizeof(string)), OT_NOOP);

    memcpy(string, aString, static_cast<uint8_t>(prefixLengthStr - aString));
    string[prefixLengthStr - aString] = '\0';

    SuccessOrExit(static_cast<Ip6::Address &>(aPrefix.mPrefix).FromString(string));
    error = ParseAsUint8(prefixLengthStr + 1, aPrefix.mLength);

exit:
    return error;
}
#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD

otError ParseAsHexString(const char *aString, uint8_t *aBuffer, uint16_t aSize)
{
    otError  error;
    uint16_t readSize = aSize;

    SuccessOrExit(error = ParseAsHexString(aString, readSize, aBuffer, kDisallowTruncate));
    VerifyOrExit(readSize == aSize, error = OT_ERROR_INVALID_ARGS);

exit:
    return error;
}

otError ParseAsHexString(const char *aString, uint16_t &aSize, uint8_t *aBuffer, HexStringParseMode aMode)
{
    otError     error     = OT_ERROR_NONE;
    uint8_t     byte      = 0;
    uint16_t    readBytes = 0;
    const char *hex       = aString;
    size_t      hexLength = strlen(aString);
    uint8_t     numChars;

    if (aMode == kDisallowTruncate)
    {
        VerifyOrExit((hexLength + 1) / 2 <= aSize, error = OT_ERROR_INVALID_ARGS);
    }

    // Handle the case where number of chars in hex string is odd.
    numChars = hexLength & 1;

    while (*hex != '\0')
    {
        uint8_t digit;

        SuccessOrExit(error = ParseHexDigit(*hex, digit));
        byte |= digit;

        hex++;
        numChars++;

        if (numChars >= 2)
        {
            numChars   = 0;
            *aBuffer++ = byte;
            byte       = 0;
            readBytes++;

            if (readBytes == aSize)
            {
                ExitNow();
            }
        }
        else
        {
            byte <<= 4;
        }
    }

    aSize = readBytes;

exit:
    return error;
}

} // namespace CmdLineParser
} // namespace Utils
} // namespace ot
