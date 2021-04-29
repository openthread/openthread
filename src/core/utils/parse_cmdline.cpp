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

#include <string.h>

#include "common/code_utils.hpp"
#include "common/numeric_limits.hpp"
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

static Error ParseDigit(char aDigitChar, uint8_t &aValue)
{
    Error error = kErrorNone;

    VerifyOrExit(('0' <= aDigitChar) && (aDigitChar <= '9'), error = kErrorInvalidArgs);
    aValue = static_cast<uint8_t>(aDigitChar - '0');

exit:
    return error;
}

static Error ParseHexDigit(char aHexChar, uint8_t &aValue)
{
    Error error = kErrorNone;

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

Error ParseCmd(char *aCommandString, uint8_t &aArgsLength, Arg *aArgs, uint8_t aArgsLengthMax)
{
    Error error = kErrorNone;
    char *cmd;

    aArgsLength = 0;

    for (cmd = aCommandString; *cmd; cmd++)
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
            VerifyOrExit(aArgsLength < aArgsLengthMax, error = kErrorInvalidArgs);

            aArgs[aArgsLength++].SetCString(cmd);
        }
    }

exit:
    return error;
}

template <typename UintType> Error ParseUint(const char *aString, UintType &aUint)
{
    Error    error;
    uint64_t value;

    SuccessOrExit(error = ParseAsUint64(aString, value));

    VerifyOrExit(value <= NumericLimits<UintType>::Max(), error = kErrorInvalidArgs);
    aUint = static_cast<UintType>(value);

exit:
    return error;
}

Error ParseAsUint8(const char *aString, uint8_t &aUint8)
{
    return ParseUint<uint8_t>(aString, aUint8);
}

Error ParseAsUint16(const char *aString, uint16_t &aUint16)
{
    return ParseUint<uint16_t>(aString, aUint16);
}

Error ParseAsUint32(const char *aString, uint32_t &aUint32)
{
    return ParseUint<uint32_t>(aString, aUint32);
}

Error ParseAsUint64(const char *aString, uint64_t &aUint64)
{
    Error       error = kErrorNone;
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
        VerifyOrExit(value <= (isHex ? kMaxHexBeforeOveflow : kMaxDecBeforeOverlow), error = kErrorInvalidArgs);
        value    = isHex ? (value << 4) : (value * 10);
        newValue = value + digit;
        VerifyOrExit(newValue >= value, error = kErrorInvalidArgs);
        value = newValue;
        cur++;
    } while (*cur != '\0');

    aUint64 = value;

exit:
    return error;
}

template <typename IntType> Error ParseInt(const char *aString, IntType &aInt)
{
    Error   error;
    int32_t value;

    SuccessOrExit(error = ParseAsInt32(aString, value));

    VerifyOrExit((NumericLimits<IntType>::Min() <= value) && (value <= NumericLimits<IntType>::Max()),
                 error = kErrorInvalidArgs);
    aInt = static_cast<IntType>(value);

exit:
    return error;
}

Error ParseAsInt8(const char *aString, int8_t &aInt8)
{
    return ParseInt<int8_t>(aString, aInt8);
}

Error ParseAsInt16(const char *aString, int16_t &aInt16)
{
    return ParseInt<int16_t>(aString, aInt16);
}

Error ParseAsInt32(const char *aString, int32_t &aInt32)
{
    Error    error;
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
    VerifyOrExit(value <= (isNegavtive ? static_cast<uint64_t>(-static_cast<int64_t>(NumericLimits<int32_t>::Min()))
                                       : static_cast<uint64_t>(NumericLimits<int32_t>::Max())),
                 error = kErrorInvalidArgs);
    aInt32 = static_cast<int32_t>(isNegavtive ? -static_cast<int64_t>(value) : static_cast<int64_t>(value));

exit:
    return error;
}

Error ParseAsBool(const char *aString, bool &aBool)
{
    Error    error;
    uint32_t value;

    SuccessOrExit(error = ParseAsUint32(aString, value));
    aBool = (value != 0);

exit:
    return error;
}
#if OPENTHREAD_FTD || OPENTHREAD_MTD

Error ParseAsIp6Prefix(const char *aString, otIp6Prefix &aPrefix)
{
    enum : uint8_t
    {
        kMaxIp6AddressStringSize = 45,
    };

    Error       error = kErrorInvalidArgs;
    char        string[kMaxIp6AddressStringSize];
    const char *prefixLengthStr;

    prefixLengthStr = strchr(aString, '/');
    VerifyOrExit(prefixLengthStr != nullptr);

    VerifyOrExit(prefixLengthStr - aString < static_cast<int32_t>(sizeof(string)));

    memcpy(string, aString, static_cast<uint8_t>(prefixLengthStr - aString));
    string[prefixLengthStr - aString] = '\0';

    SuccessOrExit(static_cast<Ip6::Address &>(aPrefix.mPrefix).FromString(string));
    error = ParseAsUint8(prefixLengthStr + 1, aPrefix.mLength);

exit:
    return error;
}
#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD

Error ParseAsHexString(const char *aString, uint8_t *aBuffer, uint16_t aSize)
{
    Error    error;
    uint16_t readSize = aSize;

    SuccessOrExit(error = ParseAsHexString(aString, readSize, aBuffer, kDisallowTruncate));
    VerifyOrExit(readSize == aSize, error = kErrorInvalidArgs);

exit:
    return error;
}

Error ParseAsHexString(const char *aString, uint16_t &aSize, uint8_t *aBuffer, HexStringParseMode aMode)
{
    Error       error     = kErrorNone;
    uint8_t     byte      = 0;
    uint16_t    readBytes = 0;
    const char *hex       = aString;
    size_t      hexLength = strlen(aString);
    uint8_t     numChars;

    if (aMode == kDisallowTruncate)
    {
        VerifyOrExit((hexLength + 1) / 2 <= aSize, error = kErrorInvalidArgs);
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

void Arg::CopyArgsToStringArray(Arg aArgs[], uint8_t aArgsLength, char *aStrings[])
{
    for (uint8_t i = 0; i < aArgsLength; i++)
    {
        aStrings[i] = aArgs[i].GetCString();
    }
}

} // namespace CmdLineParser
} // namespace Utils
} // namespace ot
