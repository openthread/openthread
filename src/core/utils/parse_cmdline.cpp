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
#include "common/string.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace Utils {
namespace CmdLineParser {

static bool IsSeparator(char aChar) { return (aChar == ' ') || (aChar == '\t') || (aChar == '\r') || (aChar == '\n'); }

static bool IsEscapable(char aChar) { return IsSeparator(aChar) || (aChar == '\\'); }

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

Error ParseCmd(char *aCommandString, Arg aArgs[], uint8_t aArgsMaxLength)
{
    Error   error = kErrorNone;
    uint8_t index = 0;
    char   *cmd;

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

        if ((*cmd != '\0') && ((index == 0) || (*(cmd - 1) == '\0')))
        {
            if (index == aArgsMaxLength - 1)
            {
                error = kErrorInvalidArgs;
                break;
            }

            aArgs[index++].SetCString(cmd);
        }
    }

    while (index < aArgsMaxLength)
    {
        aArgs[index++].Clear();
    }

    return error;
}

template <typename UintType> Error ParseUint(const char *aString, UintType &aUint)
{
    Error    error;
    uint64_t value;

    SuccessOrExit(error = ParseAsUint64(aString, value));

    VerifyOrExit(value <= NumericLimits<UintType>::kMax, error = kErrorInvalidArgs);
    aUint = static_cast<UintType>(value);

exit:
    return error;
}

Error ParseAsUint8(const char *aString, uint8_t &aUint8) { return ParseUint<uint8_t>(aString, aUint8); }

Error ParseAsUint16(const char *aString, uint16_t &aUint16) { return ParseUint<uint16_t>(aString, aUint16); }

Error ParseAsUint32(const char *aString, uint32_t &aUint32) { return ParseUint<uint32_t>(aString, aUint32); }

Error ParseAsUint64(const char *aString, uint64_t &aUint64)
{
    Error       error = kErrorNone;
    uint64_t    value = 0;
    const char *cur   = aString;
    bool        isHex = false;

    enum : uint64_t
    {
        kMaxHexBeforeOverflow = (0xffffffffffffffffULL / 16),
        kMaxDecBeforeOverflow = (0xffffffffffffffffULL / 10),
    };

    VerifyOrExit(aString != nullptr, error = kErrorInvalidArgs);

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
        VerifyOrExit(value <= (isHex ? kMaxHexBeforeOverflow : kMaxDecBeforeOverflow), error = kErrorInvalidArgs);
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

    VerifyOrExit((NumericLimits<IntType>::kMin <= value) && (value <= NumericLimits<IntType>::kMax),
                 error = kErrorInvalidArgs);
    aInt = static_cast<IntType>(value);

exit:
    return error;
}

Error ParseAsInt8(const char *aString, int8_t &aInt8) { return ParseInt<int8_t>(aString, aInt8); }

Error ParseAsInt16(const char *aString, int16_t &aInt16) { return ParseInt<int16_t>(aString, aInt16); }

Error ParseAsInt32(const char *aString, int32_t &aInt32)
{
    Error    error;
    uint64_t value;
    bool     isNegative = false;

    VerifyOrExit(aString != nullptr, error = kErrorInvalidArgs);

    if (*aString == '-')
    {
        aString++;
        isNegative = true;
    }
    else if (*aString == '+')
    {
        aString++;
    }

    SuccessOrExit(error = ParseAsUint64(aString, value));
    VerifyOrExit(value <= (isNegative ? static_cast<uint64_t>(-static_cast<int64_t>(NumericLimits<int32_t>::kMin))
                                      : static_cast<uint64_t>(NumericLimits<int32_t>::kMax)),
                 error = kErrorInvalidArgs);
    aInt32 = static_cast<int32_t>(isNegative ? -static_cast<int64_t>(value) : static_cast<int64_t>(value));

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

Error ParseAsIp6Address(const char *aString, otIp6Address &aAddress)
{
    return (aString != nullptr) ? otIp6AddressFromString(aString, &aAddress) : kErrorInvalidArgs;
}

Error ParseAsIp4Address(const char *aString, otIp4Address &aAddress)
{
    return (aString != nullptr) ? otIp4AddressFromString(aString, &aAddress) : kErrorInvalidArgs;
}

Error ParseAsIp6Prefix(const char *aString, otIp6Prefix &aPrefix)
{
    return (aString != nullptr) ? otIp6PrefixFromString(aString, &aPrefix) : kErrorInvalidArgs;
}
#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD

enum HexStringParseMode
{
    kModeExactSize,    // Parse hex string expecting an exact size (number of bytes when parsed).
    kModeUpToSize,     // Parse hex string expecting less than or equal a given size.
    kModeAllowPartial, // Allow parsing of partial segments.
};

static Error ParseHexString(const char *&aString, uint16_t &aSize, uint8_t *aBuffer, HexStringParseMode aMode)
{
    Error  error      = kErrorNone;
    size_t parsedSize = 0;
    size_t stringLength;
    size_t expectedSize;
    bool   skipFirstDigit;

    VerifyOrExit(aString != nullptr, error = kErrorInvalidArgs);

    stringLength = strlen(aString);
    expectedSize = (stringLength + 1) / 2;

    switch (aMode)
    {
    case kModeExactSize:
        VerifyOrExit(expectedSize == aSize, error = kErrorInvalidArgs);
        break;
    case kModeUpToSize:
        VerifyOrExit(expectedSize <= aSize, error = kErrorInvalidArgs);
        break;
    case kModeAllowPartial:
        break;
    }

    // If number of chars in hex string is odd, we skip parsing
    // the first digit.

    skipFirstDigit = ((stringLength & 1) != 0);

    while (parsedSize < expectedSize)
    {
        uint8_t digit;

        if ((aMode == kModeAllowPartial) && (parsedSize == aSize))
        {
            // If partial parse mode is allowed, stop once we read the
            // requested size.
            ExitNow(error = kErrorPending);
        }

        if (skipFirstDigit)
        {
            *aBuffer       = 0;
            skipFirstDigit = false;
        }
        else
        {
            SuccessOrExit(error = ParseHexDigit(*aString, digit));
            aString++;
            *aBuffer = static_cast<uint8_t>(digit << 4);
        }

        SuccessOrExit(error = ParseHexDigit(*aString, digit));
        aString++;
        *aBuffer |= digit;

        aBuffer++;
        parsedSize++;
    }

    aSize = static_cast<uint16_t>(parsedSize);

exit:
    return error;
}

Error ParseAsHexString(const char *aString, uint8_t *aBuffer, uint16_t aSize)
{
    return ParseHexString(aString, aSize, aBuffer, kModeExactSize);
}

Error ParseAsHexString(const char *aString, uint16_t &aSize, uint8_t *aBuffer)
{
    return ParseHexString(aString, aSize, aBuffer, kModeUpToSize);
}

Error ParseAsHexStringSegment(const char *&aString, uint16_t &aSize, uint8_t *aBuffer)
{
    return ParseHexString(aString, aSize, aBuffer, kModeAllowPartial);
}

//---------------------------------------------------------------------------------------------------------------------
// Arg class

uint16_t Arg::GetLength(void) const { return IsEmpty() ? 0 : static_cast<uint16_t>(strlen(mString)); }

bool Arg::operator==(const char *aString) const { return !IsEmpty() && (strcmp(mString, aString) == 0); }

void Arg::CopyArgsToStringArray(Arg aArgs[], char *aStrings[])
{
    for (uint8_t i = 0; !aArgs[i].IsEmpty(); i++)
    {
        aStrings[i] = aArgs[i].GetCString();
    }
}

uint8_t Arg::GetArgsLength(Arg aArgs[])
{
    uint8_t length = 0;

    while (!aArgs[length].IsEmpty())
    {
        length++;
    }

    return length;
}

} // namespace CmdLineParser
} // namespace Utils
} // namespace ot
