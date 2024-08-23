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
 *  This file implements OpenThread String class and functions.
 */

#include "string.hpp"
#include "debug.hpp"

#include <string.h>

namespace ot {

namespace {

// The definitions below are included in an unnamed namespace
// to limit their scope to this translation unit (this file).

enum MatchType : uint8_t
{
    kNoMatch,
    kPrefixMatch,
    kFullMatch,
};

MatchType Match(const char *aString, const char *aPrefixString, StringMatchMode aMode)
{
    // This is file private function that is used by other functions.
    // It matches @p aString with @p aPrefixString using match @ aMode.
    //
    // If @p aString and @p aPrefixString match and have the
    // same length `kFullMatch` is returned. If @p aString starts
    // with @p aPrefixString but contains more characters, then
    // `kPrefixMatch` is returned. Otherwise `kNoMatch` is returned.

    MatchType match = kNoMatch;

    switch (aMode)
    {
    case kStringExactMatch:
        while (*aPrefixString != kNullChar)
        {
            VerifyOrExit(*aString++ == *aPrefixString++);
        }
        break;

    case kStringCaseInsensitiveMatch:
        while (*aPrefixString != kNullChar)
        {
            VerifyOrExit(ToLowercase(*aString++) == ToLowercase(*aPrefixString++));
        }
        break;
    }

    match = (*aString == kNullChar) ? kFullMatch : kPrefixMatch;

exit:
    return match;
}

} // namespace

uint16_t StringLength(const char *aString, uint16_t aMaxLength)
{
    uint16_t ret = 0;

    VerifyOrExit(aString != nullptr);

    for (; (ret < aMaxLength) && (aString[ret] != kNullChar); ret++)
    {
        // Empty loop.
    }

exit:
    return ret;
}

const char *StringFind(const char *aString, char aChar)
{
    const char *ret = nullptr;

    for (; *aString != kNullChar; aString++)
    {
        if (*aString == aChar)
        {
            ret = aString;
            break;
        }
    }

    return ret;
}

const char *StringFind(const char *aString, const char *aSubString, StringMatchMode aMode)
{
    const char *ret    = nullptr;
    size_t      len    = strlen(aString);
    size_t      subLen = strlen(aSubString);

    VerifyOrExit(subLen <= len);

    for (size_t index = 0; index <= static_cast<size_t>(len - subLen); index++)
    {
        if (Match(&aString[index], aSubString, aMode) != kNoMatch)
        {
            ExitNow(ret = &aString[index]);
        }
    }

exit:
    return ret;
}

bool StringStartsWith(const char *aString, const char *aPrefixString, StringMatchMode aMode)
{
    return Match(aString, aPrefixString, aMode) != kNoMatch;
}

bool StringEndsWith(const char *aString, char aChar)
{
    size_t len = strlen(aString);

    return (len > 0) && (aString[len - 1] == aChar);
}

bool StringEndsWith(const char *aString, const char *aSubString, StringMatchMode aMode)
{
    size_t len    = strlen(aString);
    size_t subLen = strlen(aSubString);

    return (subLen > 0) && (len >= subLen) && (Match(&aString[len - subLen], aSubString, aMode) != kNoMatch);
}

bool StringMatch(const char *aFirstString, const char *aSecondString)
{
    return Match(aFirstString, aSecondString, kStringExactMatch) == kFullMatch;
}

bool StringMatch(const char *aFirstString, const char *aSecondString, StringMatchMode aMode)
{
    return Match(aFirstString, aSecondString, aMode) == kFullMatch;
}

Error StringCopy(char *aTargetBuffer, uint16_t aTargetSize, const char *aSource, StringEncodingCheck aEncodingCheck)
{
    Error    error = kErrorNone;
    uint16_t length;

    if (aSource == nullptr)
    {
        aTargetBuffer[0] = kNullChar;
        ExitNow();
    }

    length = StringLength(aSource, aTargetSize);
    VerifyOrExit(length < aTargetSize, error = kErrorInvalidArgs);

    switch (aEncodingCheck)
    {
    case kStringNoEncodingCheck:
        break;
    case kStringCheckUtf8Encoding:
        VerifyOrExit(IsValidUtf8String(aSource), error = kErrorParse);
        break;
    }

    memcpy(aTargetBuffer, aSource, length + 1); // `+ 1` to also copy null char.

exit:
    return error;
}

Error StringParseUint8(const char *&aString, uint8_t &aUint8)
{
    return StringParseUint8(aString, aUint8, NumericLimits<uint8_t>::kMax);
}

Error StringParseUint8(const char *&aString, uint8_t &aUint8, uint8_t aMaxValue)
{
    Error       error = kErrorParse;
    const char *cur   = aString;
    uint16_t    value = 0;
    uint8_t     digit;

    while (ParseDigit(*cur, digit) == kErrorNone)
    {
        value *= 10;
        value += digit;
        VerifyOrExit(value <= aMaxValue, error = kErrorParse);
        error = kErrorNone;
        cur++;
    }

    aString = cur;
    aUint8  = static_cast<uint8_t>(value);

exit:
    return error;
}

void StringConvertToLowercase(char *aString)
{
    for (; *aString != kNullChar; aString++)
    {
        *aString = ToLowercase(*aString);
    }
}

void StringConvertToUppercase(char *aString)
{
    for (; *aString != kNullChar; aString++)
    {
        *aString = ToUppercase(*aString);
    }
}

char ToLowercase(char aChar)
{
    if (IsUppercase(aChar))
    {
        aChar += 'a' - 'A';
    }

    return aChar;
}

char ToUppercase(char aChar)
{
    if (IsLowercase(aChar))
    {
        aChar -= 'a' - 'A';
    }

    return aChar;
}

bool IsDigit(char aChar) { return ('0' <= aChar && aChar <= '9'); }

bool IsUppercase(char aChar) { return ('A' <= aChar && aChar <= 'Z'); }

bool IsLowercase(char aChar) { return ('a' <= aChar && aChar <= 'z'); }

Error ParseDigit(char aDigitChar, uint8_t &aValue)
{
    Error error = kErrorNone;

    VerifyOrExit(IsDigit(aDigitChar), error = kErrorInvalidArgs);
    aValue = static_cast<uint8_t>(aDigitChar - '0');

exit:
    return error;
}

Error ParseHexDigit(char aHexChar, uint8_t &aValue)
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

const char *ToYesNo(bool aBool)
{
    static const char *const kYesNoStrings[] = {"no", "yes"};

    return kYesNoStrings[aBool];
}

StringWriter::StringWriter(char *aBuffer, uint16_t aSize)
    : mBuffer(aBuffer)
    , mLength(0)
    , mSize(aSize)
{
    mBuffer[0] = kNullChar;
}

StringWriter &StringWriter::Clear(void)
{
    mBuffer[0] = kNullChar;
    mLength    = 0;
    return *this;
}

StringWriter &StringWriter::Append(const char *aFormat, ...)
{
    va_list args;
    va_start(args, aFormat);
    AppendVarArgs(aFormat, args);
    va_end(args);

    return *this;
}

StringWriter &StringWriter::AppendVarArgs(const char *aFormat, va_list aArgs)
{
    int len;

    len = vsnprintf(mBuffer + mLength, (mSize > mLength ? (mSize - mLength) : 0), aFormat, aArgs);
    OT_ASSERT(len >= 0);

    mLength += static_cast<uint16_t>(len);

    if (IsTruncated())
    {
        mBuffer[mSize - 1] = kNullChar;
    }

    return *this;
}

StringWriter &StringWriter::AppendHexBytes(const uint8_t *aBytes, uint16_t aLength)
{
    while (aLength--)
    {
        Append("%02x", *aBytes++);
    }

    return *this;
}

StringWriter &StringWriter::AppendCharMultipleTimes(char aChar, uint16_t aCount)
{
    while (aCount--)
    {
        Append("%c", aChar);
    }

    return *this;
}

bool IsValidUtf8String(const char *aString) { return IsValidUtf8String(aString, strlen(aString)); }

bool IsValidUtf8String(const char *aString, size_t aLength)
{
    bool    ret = true;
    uint8_t byte;
    uint8_t continuationBytes = 0;
    size_t  position          = 0;

    while (position < aLength)
    {
        byte = *reinterpret_cast<const uint8_t *>(aString + position);
        ++position;

        if ((byte & 0x80) == 0)
        {
            // We don't allow control characters.
            VerifyOrExit(!iscntrl(byte), ret = false);
            continue;
        }

        // This is a leading byte 1xxx-xxxx.

        if ((byte & 0x40) == 0) // 10xx-xxxx
        {
            // We got a continuation byte pattern without seeing a leading byte earlier.
            ExitNow(ret = false);
        }
        else if ((byte & 0x20) == 0) // 110x-xxxx
        {
            continuationBytes = 1;
        }
        else if ((byte & 0x10) == 0) // 1110-xxxx
        {
            continuationBytes = 2;
        }
        else if ((byte & 0x08) == 0) // 1111-0xxx
        {
            continuationBytes = 3;
        }
        else // 1111-1xxx  (invalid pattern).
        {
            ExitNow(ret = false);
        }

        while (continuationBytes-- != 0)
        {
            VerifyOrExit(position < aLength, ret = false);

            byte = *reinterpret_cast<const uint8_t *>(aString + position);
            ++position;

            // Verify the continuation byte pattern 10xx-xxxx
            VerifyOrExit((byte & 0xc0) == 0x80, ret = false);
        }
    }

exit:
    return ret;
}

} // namespace ot
