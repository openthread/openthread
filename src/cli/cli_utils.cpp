/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file contains implementation of the CLI output module.
 */

#include "cli_utils.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if OPENTHREAD_FTD || OPENTHREAD_MTD
#include <openthread/dns.h>
#endif
#include <openthread/logging.h>

#include "cli/cli.hpp"
#include "common/string.hpp"

namespace ot {
namespace Cli {

const char Utils::kUnknownString[] = "unknown";

OutputImplementer::OutputImplementer(otCliOutputCallback aCallback, void *aCallbackContext)
    : mCallback(aCallback)
    , mCallbackContext(aCallbackContext)
#if OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
    , mOutputLength(0)
    , mEmittingCommandOutput(true)
#endif
{
}

void Utils::OutputFormat(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);
}

void Utils::OutputFormat(uint8_t aIndentSize, const char *aFormat, ...)
{
    va_list args;

    OutputSpaces(aIndentSize);

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);
}

void Utils::OutputLine(const char *aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);

    OutputNewLine();
}

void Utils::OutputLine(uint8_t aIndentSize, const char *aFormat, ...)
{
    va_list args;

    OutputSpaces(aIndentSize);

    va_start(args, aFormat);
    OutputFormatV(aFormat, args);
    va_end(args);

    OutputNewLine();
}

void Utils::OutputNewLine(void) { OutputFormat("\r\n"); }

void Utils::OutputSpaces(uint8_t aCount) { OutputFormat("%*s", aCount, ""); }

void Utils::OutputBytes(const uint8_t *aBytes, uint16_t aLength)
{
    for (uint16_t i = 0; i < aLength; i++)
    {
        OutputFormat("%02x", aBytes[i]);
    }
}

void Utils::OutputBytesLine(const uint8_t *aBytes, uint16_t aLength)
{
    OutputBytes(aBytes, aLength);
    OutputNewLine();
}

const char *Utils::Uint64ToString(uint64_t aUint64, Uint64StringBuffer &aBuffer)
{
    char *cur = &aBuffer.mChars[Uint64StringBuffer::kSize - 1];

    *cur = '\0';

    if (aUint64 == 0)
    {
        *(--cur) = '0';
    }
    else
    {
        for (; aUint64 != 0; aUint64 /= 10)
        {
            *(--cur) = static_cast<char>('0' + static_cast<uint8_t>(aUint64 % 10));
        }
    }

    return cur;
}

void Utils::OutputUint64(uint64_t aUint64)
{
    Uint64StringBuffer buffer;

    OutputFormat("%s", Uint64ToString(aUint64, buffer));
}

void Utils::OutputUint64Line(uint64_t aUint64)
{
    OutputUint64(aUint64);
    OutputNewLine();
}

void Utils::OutputEnabledDisabledStatus(bool aEnabled) { OutputLine(aEnabled ? "Enabled" : "Disabled"); }

#if OPENTHREAD_FTD || OPENTHREAD_MTD

void Utils::OutputIp6Address(const otIp6Address &aAddress)
{
    char string[OT_IP6_ADDRESS_STRING_SIZE];

    otIp6AddressToString(&aAddress, string, sizeof(string));

    return OutputFormat("%s", string);
}

void Utils::OutputIp6AddressLine(const otIp6Address &aAddress)
{
    OutputIp6Address(aAddress);
    OutputNewLine();
}

void Utils::OutputIp6Prefix(const otIp6Prefix &aPrefix)
{
    char string[OT_IP6_PREFIX_STRING_SIZE];

    otIp6PrefixToString(&aPrefix, string, sizeof(string));

    OutputFormat("%s", string);
}

void Utils::OutputIp6PrefixLine(const otIp6Prefix &aPrefix)
{
    OutputIp6Prefix(aPrefix);
    OutputNewLine();
}

void Utils::OutputIp6Prefix(const otIp6NetworkPrefix &aPrefix)
{
    OutputFormat("%x:%x:%x:%x::/64", (aPrefix.m8[0] << 8) | aPrefix.m8[1], (aPrefix.m8[2] << 8) | aPrefix.m8[3],
                 (aPrefix.m8[4] << 8) | aPrefix.m8[5], (aPrefix.m8[6] << 8) | aPrefix.m8[7]);
}

void Utils::OutputIp6PrefixLine(const otIp6NetworkPrefix &aPrefix)
{
    OutputIp6Prefix(aPrefix);
    OutputNewLine();
}

void Utils::OutputSockAddr(const otSockAddr &aSockAddr)
{
    char string[OT_IP6_SOCK_ADDR_STRING_SIZE];

    otIp6SockAddrToString(&aSockAddr, string, sizeof(string));

    return OutputFormat("%s", string);
}

void Utils::OutputSockAddrLine(const otSockAddr &aSockAddr)
{
    OutputSockAddr(aSockAddr);
    OutputNewLine();
}

void Utils::OutputDnsTxtData(const uint8_t *aTxtData, uint16_t aTxtDataLength)
{
    otDnsTxtEntry         entry;
    otDnsTxtEntryIterator iterator;
    bool                  isFirst = true;

    otDnsInitTxtEntryIterator(&iterator, aTxtData, aTxtDataLength);

    OutputFormat("[");

    while (otDnsGetNextTxtEntry(&iterator, &entry) == OT_ERROR_NONE)
    {
        if (!isFirst)
        {
            OutputFormat(", ");
        }

        if (entry.mKey == nullptr)
        {
            // A null `mKey` indicates that the key in the entry is
            // longer than the recommended max key length, so the entry
            // could not be parsed. In this case, the whole entry is
            // returned encoded in `mValue`.

            OutputFormat("[");
            OutputBytes(entry.mValue, entry.mValueLength);
            OutputFormat("]");
        }
        else
        {
            OutputFormat("%s", entry.mKey);

            if (entry.mValue != nullptr)
            {
                OutputFormat("=");
                OutputBytes(entry.mValue, entry.mValueLength);
            }
        }

        isFirst = false;
    }

    OutputFormat("]");
}

const char *Utils::PercentageToString(uint16_t aValue, PercentageStringBuffer &aBuffer)
{
    uint32_t     scaledValue = aValue;
    StringWriter writer(aBuffer.mChars, sizeof(aBuffer.mChars));

    scaledValue = (scaledValue * 10000) / 0xffff;
    writer.Append("%u.%02u", static_cast<uint16_t>(scaledValue / 100), static_cast<uint16_t>(scaledValue % 100));

    return aBuffer.mChars;
}

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

void Utils::OutputFormatV(const char *aFormat, va_list aArguments) { mImplementer.OutputV(aFormat, aArguments); }

void OutputImplementer::OutputV(const char *aFormat, va_list aArguments)
{
#if OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
    va_list args;
    int     charsWritten;
    bool    truncated = false;

    va_copy(args, aArguments);
#endif

    mCallback(mCallbackContext, aFormat, aArguments);

#if OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
    VerifyOrExit(mEmittingCommandOutput);

    charsWritten = vsnprintf(&mOutputString[mOutputLength], sizeof(mOutputString) - mOutputLength, aFormat, args);

    VerifyOrExit(charsWritten >= 0, mOutputLength = 0);

    if (static_cast<uint32_t>(charsWritten) >= sizeof(mOutputString) - mOutputLength)
    {
        truncated     = true;
        mOutputLength = sizeof(mOutputString) - 1;
    }
    else
    {
        mOutputLength += charsWritten;
    }

    while (true)
    {
        char *lineEnd = strchr(mOutputString, '\r');

        if (lineEnd == nullptr)
        {
            break;
        }

        *lineEnd = '\0';

        if (lineEnd > mOutputString)
        {
            otLogCli(OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LEVEL, "Output: %s", mOutputString);
        }

        lineEnd++;

        while ((*lineEnd == '\n') || (*lineEnd == '\r'))
        {
            lineEnd++;
        }

        // Example of the pointers and lengths.
        //
        // - mOutputString = "hi\r\nmore"
        // - mOutputLength = 8
        // - lineEnd       = &mOutputString[4]
        //
        //
        //   0    1    2    3    4    5    6    7    8    9
        // +----+----+----+----+----+----+----+----+----+---
        // | h  | i  | \r | \n | m  | o  | r  | e  | \0 |
        // +----+----+----+----+----+----+----+----+----+---
        //                       ^                   ^
        //                       |                   |
        //                    lineEnd    mOutputString[mOutputLength]
        //
        //
        // New length is `&mOutputString[8] - &mOutputString[4] -> 4`.
        //
        // We move (newLen + 1 = 5) chars from `lineEnd` to start of
        // `mOutputString` which will include the `\0` char.
        //
        // If `lineEnd` and `mOutputString[mOutputLength]` are the same
        // the code works correctly as well  (new length set to zero and
        // the `\0` is copied).

        mOutputLength = static_cast<uint16_t>(&mOutputString[mOutputLength] - lineEnd);
        memmove(mOutputString, lineEnd, mOutputLength + 1);
    }

    if (truncated)
    {
        otLogCli(OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LEVEL, "Output: %s ...", mOutputString);
        mOutputLength = 0;
    }

exit:
    va_end(args);
#endif // OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
}

#if OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
void Utils::LogInput(const Arg *aArgs)
{
    String<kInputOutputLogStringSize> inputString;

    for (bool isFirst = true; !aArgs->IsEmpty(); aArgs++, isFirst = false)
    {
        inputString.Append(isFirst ? "%s" : " %s", aArgs->GetCString());
    }

    otLogCli(OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LEVEL, "Input: %s", inputString.AsCString());
}
#endif

void Utils::OutputTableHeader(uint8_t aNumColumns, const char *const aTitles[], const uint8_t aWidths[])
{
    for (uint8_t index = 0; index < aNumColumns; index++)
    {
        const char *title       = aTitles[index];
        uint8_t     width       = aWidths[index];
        size_t      titleLength = strlen(title);

        if (titleLength + 2 <= width)
        {
            // `title` fits in column width so we write it with extra space
            // at beginning and end ("| Title    |").

            OutputFormat("| %*s", -static_cast<int>(width - 1), title);
        }
        else
        {
            // Use narrow style (no space at beginning) and write as many
            // chars from `title` as it can fit in the given column width
            // ("|Title|").

            OutputFormat("|%*.*s", -static_cast<int>(width), width, title);
        }
    }

    OutputLine("|");
    OutputTableSeparator(aNumColumns, aWidths);
}

void Utils::OutputTableSeparator(uint8_t aNumColumns, const uint8_t aWidths[])
{
    for (uint8_t index = 0; index < aNumColumns; index++)
    {
        OutputFormat("+");

        for (uint8_t width = aWidths[index]; width != 0; width--)
        {
            OutputFormat("-");
        }
    }

    OutputLine("+");
}

otError Utils::ParseEnableOrDisable(const Arg &aArg, bool &aEnable)
{
    otError error = OT_ERROR_NONE;

    if (aArg == "enable")
    {
        aEnable = true;
    }
    else if (aArg == "disable")
    {
        aEnable = false;
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

otError Utils::ProcessEnableDisable(Arg aArgs[], SetEnabledHandler aSetEnabledHandler)
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    if (ParseEnableOrDisable(aArgs[0], enable) == OT_ERROR_NONE)
    {
        aSetEnabledHandler(GetInstancePtr(), enable);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

otError Utils::ProcessEnableDisable(Arg aArgs[], SetEnabledHandlerFailable aSetEnabledHandler)
{
    otError error = OT_ERROR_NONE;
    bool    enable;

    if (ParseEnableOrDisable(aArgs[0], enable) == OT_ERROR_NONE)
    {
        error = aSetEnabledHandler(GetInstancePtr(), enable);
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

    return error;
}

otError Utils::ProcessEnableDisable(Arg               aArgs[],
                                    IsEnabledHandler  aIsEnabledHandler,
                                    SetEnabledHandler aSetEnabledHandler)
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(aIsEnabledHandler(GetInstancePtr()));
    }
    else
    {
        error = ProcessEnableDisable(aArgs, aSetEnabledHandler);
    }

    return error;
}

otError Utils::ProcessEnableDisable(Arg                       aArgs[],
                                    IsEnabledHandler          aIsEnabledHandler,
                                    SetEnabledHandlerFailable aSetEnabledHandler)
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(aIsEnabledHandler(GetInstancePtr()));
    }
    else
    {
        error = ProcessEnableDisable(aArgs, aSetEnabledHandler);
    }

    return error;
}

otError Utils::ParseJoinerDiscerner(Arg &aArg, otJoinerDiscerner &aDiscerner)
{
    otError error;
    char   *separator;

    VerifyOrExit(!aArg.IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    separator = strstr(aArg.GetCString(), "/");

    VerifyOrExit(separator != nullptr, error = OT_ERROR_NOT_FOUND);

    SuccessOrExit(error = ot::Utils::CmdLineParser::ParseAsUint8(separator + 1, aDiscerner.mLength));
    VerifyOrExit(aDiscerner.mLength > 0 && aDiscerner.mLength <= 64, error = OT_ERROR_INVALID_ARGS);
    *separator = '\0';
    error      = aArg.ParseAsUint64(aDiscerner.mValue);

exit:
    return error;
}

otError Utils::ParsePreference(const Arg &aArg, otRoutePreference &aPreference)
{
    otError error = OT_ERROR_NONE;

    if (aArg == "high")
    {
        aPreference = OT_ROUTE_PREFERENCE_HIGH;
    }
    else if (aArg == "med")
    {
        aPreference = OT_ROUTE_PREFERENCE_MED;
    }
    else if (aArg == "low")
    {
        aPreference = OT_ROUTE_PREFERENCE_LOW;
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}

const char *Utils::PreferenceToString(signed int aPreference)
{
    const char *str = "";

    switch (aPreference)
    {
    case OT_ROUTE_PREFERENCE_LOW:
        str = "low";
        break;

    case OT_ROUTE_PREFERENCE_MED:
        str = "med";
        break;

    case OT_ROUTE_PREFERENCE_HIGH:
        str = "high";
        break;

    default:
        break;
    }

    return str;
}

#if OPENTHREAD_FTD || OPENTHREAD_MTD
otError Utils::ParseToIp6Address(otInstance *aInstance, const Arg &aArg, otIp6Address &aAddress, bool &aSynthesized)
{
    Error error = OT_ERROR_NONE;

    VerifyOrExit(!aArg.IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    error        = aArg.ParseAsIp6Address(aAddress);
    aSynthesized = false;

    if (error != OT_ERROR_NONE)
    {
        // It might be an IPv4 address, let's have a try.
        otIp4Address ip4Address;

        // Do not touch the error value if we failed to parse it as an IPv4 address.
        SuccessOrExit(aArg.ParseAsIp4Address(ip4Address));
        SuccessOrExit(error = otNat64SynthesizeIp6Address(aInstance, &ip4Address, &aAddress));
        aSynthesized = true;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
otError Utils::ParsePrefix(Arg aArgs[], otBorderRouterConfig &aConfig)
{
    otError error = OT_ERROR_NONE;

    ClearAllBytes(aConfig);

    SuccessOrExit(error = aArgs[0].ParseAsIp6Prefix(aConfig.mPrefix));
    aArgs++;

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        otRoutePreference preference;

        if (ParsePreference(*aArgs, preference) == OT_ERROR_NONE)
        {
            aConfig.mPreference = preference;
        }
        else
        {
            for (char *arg = aArgs->GetCString(); *arg != '\0'; arg++)
            {
                switch (*arg)
                {
                case 'p':
                    aConfig.mPreferred = true;
                    break;

                case 'a':
                    aConfig.mSlaac = true;
                    break;

                case 'd':
                    aConfig.mDhcp = true;
                    break;

                case 'c':
                    aConfig.mConfigure = true;
                    break;

                case 'r':
                    aConfig.mDefaultRoute = true;
                    break;

                case 'o':
                    aConfig.mOnMesh = true;
                    break;

                case 's':
                    aConfig.mStable = true;
                    break;

                case 'n':
                    aConfig.mNdDns = true;
                    break;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
                case 'D':
                    aConfig.mDp = true;
                    break;
#endif
                case '-':
                    break;

                default:
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }
        }
    }

exit:
    return error;
}

otError Utils::ParseRoute(Arg aArgs[], otExternalRouteConfig &aConfig)
{
    otError error = OT_ERROR_NONE;

    ClearAllBytes(aConfig);

    SuccessOrExit(error = aArgs[0].ParseAsIp6Prefix(aConfig.mPrefix));
    aArgs++;

    for (; !aArgs->IsEmpty(); aArgs++)
    {
        otRoutePreference preference;

        if (ParsePreference(*aArgs, preference) == OT_ERROR_NONE)
        {
            aConfig.mPreference = preference;
        }
        else
        {
            for (char *arg = aArgs->GetCString(); *arg != '\0'; arg++)
            {
                switch (*arg)
                {
                case 's':
                    aConfig.mStable = true;
                    break;

                case 'n':
                    aConfig.mNat64 = true;
                    break;

                case 'a':
                    aConfig.mAdvPio = true;
                    break;

                case '-':
                    break;

                default:
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }
        }
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD

const char *Utils::LinkModeToString(const otLinkModeConfig &aLinkMode, char (&aStringBuffer)[kLinkModeStringSize])
{
    char *flagsPtr = &aStringBuffer[0];

    if (aLinkMode.mRxOnWhenIdle)
    {
        *flagsPtr++ = 'r';
    }

    if (aLinkMode.mDeviceType)
    {
        *flagsPtr++ = 'd';
    }

    if (aLinkMode.mNetworkData)
    {
        *flagsPtr++ = 'n';
    }

    if (flagsPtr == &aStringBuffer[0])
    {
        *flagsPtr++ = '-';
    }

    *flagsPtr = '\0';

    return aStringBuffer;
}

const char *Utils::AddressOriginToString(uint8_t aOrigin)
{
    static const char *const kOriginStrings[4] = {
        "thread", // 0, OT_ADDRESS_ORIGIN_THREAD
        "slaac",  // 1, OT_ADDRESS_ORIGIN_SLAAC
        "dhcp6",  // 2, OT_ADDRESS_ORIGIN_DHCPV6
        "manual", // 3, OT_ADDRESS_ORIGIN_MANUAL
    };

    static_assert(0 == OT_ADDRESS_ORIGIN_THREAD, "OT_ADDRESS_ORIGIN_THREAD value is incorrect");
    static_assert(1 == OT_ADDRESS_ORIGIN_SLAAC, "OT_ADDRESS_ORIGIN_SLAAC value is incorrect");
    static_assert(2 == OT_ADDRESS_ORIGIN_DHCPV6, "OT_ADDRESS_ORIGIN_DHCPV6 value is incorrect");
    static_assert(3 == OT_ADDRESS_ORIGIN_MANUAL, "OT_ADDRESS_ORIGIN_MANUAL value is incorrect");

    return Stringify(aOrigin, kOriginStrings);
}

} // namespace Cli
} // namespace ot
