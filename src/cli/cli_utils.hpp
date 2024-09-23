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
 *   This file contains definitions for the CLI util functions.
 */

#ifndef CLI_UTILS_HPP_
#define CLI_UTILS_HPP_

#include "openthread-core-config.h"

#include <stdarg.h>

#include <openthread/border_router.h>
#include <openthread/border_routing.h>
#include <openthread/cli.h>
#include <openthread/joiner.h>
#include <openthread/thread.h>

#include "cli_config.h"

#include "common/binary_search.hpp"
#include "common/num_utils.hpp"
#include "common/string.hpp"
#include "common/type_traits.hpp"
#include "utils/parse_cmdline.hpp"

namespace ot {
namespace Cli {

/**
 * Represents a ID number value associated with a CLI command string.
 */
typedef uint64_t CommandId;

/**
 * This `constexpr` function converts a CLI command string to its associated `CommandId` value.
 *
 * @param[in] aString   The CLI command string.
 *
 * @returns The associated `CommandId` with @p aString.
 */
constexpr static CommandId Cmd(const char *aString)
{
    return (aString[0] == '\0') ? 0 : (static_cast<uint8_t>(aString[0]) + Cmd(aString + 1) * 255u);
}

class Utils;

/**
 * Implements the basic output functions.
 */
class OutputImplementer
{
    friend class Utils;

public:
    /**
     * Initializes the `OutputImplementer` object.
     *
     * @param[in] aCallback           A pointer to an `otCliOutputCallback` to deliver strings to the CLI console.
     * @param[in] aCallbackContext    An arbitrary context to pass in when invoking @p aCallback.
     */
    OutputImplementer(otCliOutputCallback aCallback, void *aCallbackContext);

#if OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
    void SetEmittingCommandOutput(bool aEmittingOutput) { mEmittingCommandOutput = aEmittingOutput; }
#else
    void SetEmittingCommandOutput(bool) {}
#endif

private:
    static constexpr uint16_t kInputOutputLogStringSize = OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LOG_STRING_SIZE;

    void OutputV(const char *aFormat, va_list aArguments);

    otCliOutputCallback mCallback;
    void               *mCallbackContext;
#if OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
    char     mOutputString[kInputOutputLogStringSize];
    uint16_t mOutputLength;
    bool     mEmittingCommandOutput;
#endif
};

/**
 * Provides CLI helper methods.
 */
class Utils
{
public:
    typedef ot::Utils::CmdLineParser::Arg Arg; ///< An argument

    /**
     * Represent a CLI command table entry, mapping a command with `aName` to a handler method.
     *
     * @tparam Cli    The CLI module type.
     */
    template <typename Cli> struct CommandEntry
    {
        typedef otError (Cli::*Handler)(Arg aArgs[]); ///< The handler method pointer type.

        /**
         * Compares the entry's name with a given name.
         *
         * @param aName    The name string to compare with.
         *
         * @return zero means perfect match, positive (> 0) indicates @p aName is larger than entry's name, and
         *         negative (< 0) indicates @p aName is smaller than entry's name.
         */
        int Compare(const char *aName) const { return strcmp(aName, mName); }

        /**
         * This `constexpr` method compares two entries to check if they are in order.
         *
         * @param[in] aFirst     The first entry.
         * @param[in] aSecond    The second entry.
         *
         * @retval TRUE  if @p aFirst and @p aSecond are in order, i.e. `aFirst < aSecond`.
         * @retval FALSE if @p aFirst and @p aSecond are not in order, i.e. `aFirst >= aSecond`.
         */
        constexpr static bool AreInOrder(const CommandEntry &aFirst, const CommandEntry &aSecond)
        {
            return AreStringsInOrder(aFirst.mName, aSecond.mName);
        }

        const char *mName;    ///< The command name.
        Handler     mHandler; ///< The handler method pointer.
    };

    static const char kUnknownString[]; // Constant string "unknown".

    /**
     * This template static method converts an enumeration value to a string using a table array.
     *
     * @tparam EnumType       The `enum` type.
     * @tparam kLength        The table array length (number of entries in the array).
     *
     * @param[in] aEnum       The enumeration value to convert (MUST be of `EnumType`).
     * @param[in] aTable      A reference to the array of strings of length @p kLength. `aTable[e]` is the string
     *                        representation of enumeration value `e`.
     * @param[in] aNotFound   The string to return if the @p aEnum is not in the @p aTable.
     *
     * @returns The string representation of @p aEnum from @p aTable, or @p aNotFound if it is not in the table.
     */
    template <typename EnumType, uint16_t kLength>
    static const char *Stringify(EnumType aEnum,
                                 const char *const (&aTable)[kLength],
                                 const char *aNotFound = kUnknownString)
    {
        return (static_cast<uint16_t>(aEnum) < kLength) ? aTable[static_cast<uint16_t>(aEnum)] : aNotFound;
    }

    /**
     * Initializes the `Utils` object.
     *
     * @param[in] aInstance           A pointer to OpenThread instance.
     * @param[in] aImplementer        An `OutputImplementer`.
     */
    Utils(otInstance *aInstance, OutputImplementer &aImplementer)
        : mInstance(aInstance)
        , mImplementer(aImplementer)
    {
    }

    /**
     * Returns the pointer to OpenThread instance.
     *
     * @returns The pointer to the OpenThread instance.
     */
    otInstance *GetInstancePtr(void) { return mInstance; }

    /**
     * Represents a buffer which is used when converting a `uint64` value to string in decimal format.
     */
    struct Uint64StringBuffer
    {
        static constexpr uint16_t kSize = 21; ///< Size of a buffer

        char mChars[kSize]; ///< Char array (do not access the array directly).
    };

    /**
     * Converts a `uint64_t` value to a decimal format string.
     *
     * @param[in] aUint64  The `uint64_t` value to convert.
     * @param[in] aBuffer  A buffer to allocate the string from.
     *
     * @returns A pointer to the start of the string (null-terminated) representation of @p aUint64.
     */
    static const char *Uint64ToString(uint64_t aUint64, Uint64StringBuffer &aBuffer);

    /**
     * Delivers a formatted output string to the CLI console.
     *
     * @param[in]  aFormat  A pointer to the format string.
     * @param[in]  ...      A variable list of arguments to format.
     */
    void OutputFormat(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);

    /**
     * Delivers a formatted output string to the CLI console (to which it prepends a given number
     * indentation space chars).
     *
     * @param[in]  aIndentSize   Number of indentation space chars to prepend to the string.
     * @param[in]  aFormat       A pointer to the format string.
     * @param[in]  ...           A variable list of arguments to format.
     */
    void OutputFormat(uint8_t aIndentSize, const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(3, 4);

    /**
     * Delivers a formatted output string to the CLI console (to which it also appends newline "\r\n").
     *
     * @param[in]  aFormat  A pointer to the format string.
     * @param[in]  ...      A variable list of arguments to format.
     */
    void OutputLine(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);

    /**
     * Delivers a formatted output string to the CLI console (to which it prepends a given number
     * indentation space chars and appends newline "\r\n").
     *
     * @param[in]  aIndentSize   Number of indentation space chars to prepend to the string.
     * @param[in]  aFormat       A pointer to the format string.
     * @param[in]  ...           A variable list of arguments to format.
     */
    void OutputLine(uint8_t aIndentSize, const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(3, 4);

    /**
     * Delivered newline "\r\n" to the CLI console.
     */
    void OutputNewLine(void);

    /**
     * Outputs a given number of space chars to the CLI console.
     *
     * @param[in] aCount  Number of space chars to output.
     */
    void OutputSpaces(uint8_t aCount);

    /**
     * Outputs a number of bytes to the CLI console as a hex string.
     *
     * @param[in]  aBytes   A pointer to data which should be printed.
     * @param[in]  aLength  @p aBytes length.
     */
    void OutputBytes(const uint8_t *aBytes, uint16_t aLength);

    /**
     * Outputs a number of bytes to the CLI console as a hex string and at the end it also outputs newline
     * "\r\n".
     *
     * @param[in]  aBytes   A pointer to data which should be printed.
     * @param[in]  aLength  @p aBytes length.
     */
    void OutputBytesLine(const uint8_t *aBytes, uint16_t aLength);

    /**
     * Outputs a number of bytes to the CLI console as a hex string.
     *
     * @tparam kBytesLength   The length of @p aBytes array.
     *
     * @param[in]  aBytes     A array of @p kBytesLength bytes which should be printed.
     */
    template <uint8_t kBytesLength> void OutputBytes(const uint8_t (&aBytes)[kBytesLength])
    {
        OutputBytes(aBytes, kBytesLength);
    }

    /**
     * Outputs a number of bytes to the CLI console as a hex string and at the end it also outputs newline
     * "\r\n".
     *
     * @tparam kBytesLength   The length of @p aBytes array.
     *
     * @param[in]  aBytes     A array of @p kBytesLength bytes which should be printed.
     */
    template <uint8_t kBytesLength> void OutputBytesLine(const uint8_t (&aBytes)[kBytesLength])
    {
        OutputBytesLine(aBytes, kBytesLength);
    }

    /**
     * Outputs an Extended MAC Address to the CLI console.
     *
     * param[in] aExtAddress  The Extended MAC Address to output.
     */
    void OutputExtAddress(const otExtAddress &aExtAddress) { OutputBytes(aExtAddress.m8); }

    /**
     * Outputs an Extended MAC Address to the CLI console and at the end it also outputs newline "\r\n".
     *
     * param[in] aExtAddress  The Extended MAC Address to output.
     */
    void OutputExtAddressLine(const otExtAddress &aExtAddress) { OutputBytesLine(aExtAddress.m8); }

    /**
     * Outputs a `uint64_t` value in decimal format.
     *
     * @param[in] aUint64   The `uint64_t` value to output.
     */
    void OutputUint64(uint64_t aUint64);

    /**
     * Outputs a `uint64_t` value in decimal format and at the end it also outputs newline "\r\n".
     *
     * @param[in] aUint64   The `uint64_t` value to output.
     */
    void OutputUint64Line(uint64_t aUint64);

    /**
     * Outputs "Enabled" or "Disabled" status to the CLI console (it also appends newline "\r\n").
     *
     * @param[in] aEnabled  A boolean indicating the status. TRUE outputs "Enabled", FALSE outputs "Disabled".
     */
    void OutputEnabledDisabledStatus(bool aEnabled);

#if OPENTHREAD_FTD || OPENTHREAD_MTD

    /**
     * Outputs an IPv6 address to the CLI console.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     */
    void OutputIp6Address(const otIp6Address &aAddress);

    /**
     * Outputs an IPv6 address to the CLI console and at the end it also outputs newline "\r\n".
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     */
    void OutputIp6AddressLine(const otIp6Address &aAddress);

    /**
     * Outputs an IPv6 prefix to the CLI console.
     *
     * @param[in]  aPrefix  A reference to the IPv6 prefix.
     */
    void OutputIp6Prefix(const otIp6Prefix &aPrefix);

    /**
     * Outputs an IPv6 prefix to the CLI console and at the end it also outputs newline "\r\n".
     *
     * @param[in]  aPrefix  A reference to the IPv6 prefix.
     */
    void OutputIp6PrefixLine(const otIp6Prefix &aPrefix);

    /**
     * Outputs an IPv6 network prefix to the CLI console.
     *
     * @param[in]  aPrefix  A reference to the IPv6 network prefix.
     */
    void OutputIp6Prefix(const otIp6NetworkPrefix &aPrefix);

    /**
     * Outputs an IPv6 network prefix to the CLI console and at the end it also outputs newline "\r\n".
     *
     * @param[in]  aPrefix  A reference to the IPv6 network prefix.
     */
    void OutputIp6PrefixLine(const otIp6NetworkPrefix &aPrefix);

    /**
     * Outputs an IPv6 socket address to the CLI console.
     *
     * @param[in] aSockAddr   A reference to the IPv6 socket address.
     */
    void OutputSockAddr(const otSockAddr &aSockAddr);

    /**
     * Outputs an IPv6 socket address to the CLI console and at the end it also outputs newline "\r\n".
     *
     * @param[in] aSockAddr   A reference to the IPv6 socket address.
     */
    void OutputSockAddrLine(const otSockAddr &aSockAddr);

    /**
     * Outputs DNS TXT data to the CLI console.
     *
     * @param[in] aTxtData        A pointer to a buffer containing the DNS TXT data.
     * @param[in] aTxtDataLength  The length of @p aTxtData (in bytes).
     */
    void OutputDnsTxtData(const uint8_t *aTxtData, uint16_t aTxtDataLength);

    /**
     * Represents a buffer which is used when converting an encoded rate value to percentage string.
     */
    struct PercentageStringBuffer
    {
        static constexpr uint16_t kSize = 7; ///< Size of a buffer

        char mChars[kSize]; ///< Char array (do not access the array directly).
    };

    /**
     * Converts an encoded value to a percentage representation.
     *
     * The encoded @p aValue is assumed to be linearly scaled such that `0` maps to 0% and `0xffff` maps to 100%.
     *
     * The resulting string provides two decimal accuracy, e.g., "100.00", "0.00", "75.37".
     *
     * @param[in] aValue   The encoded percentage value to convert.
     * @param[in] aBuffer  A buffer to allocate the string from.
     *
     * @returns A pointer to the start of the string (null-terminated) representation of @p aValue.
     */
    static const char *PercentageToString(uint16_t aValue, PercentageStringBuffer &aBuffer);

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

    /**
     * Outputs a table header to the CLI console.
     *
     * An example of the table header format:
     *
     *    | Title1    | Title2 |Title3| Title4               |
     *    +-----------+--------+------+----------------------+
     *
     * The titles are left adjusted (extra white space is added at beginning if the column is width enough). The widths
     * are specified as the number chars between two `|` chars (excluding the char `|` itself).
     *
     * @tparam kTableNumColumns   The number columns in the table.
     *
     * @param[in] aTitles   An array specifying the table column titles.
     * @param[in] aWidths   An array specifying the table column widths (in number of chars).
     */
    template <uint8_t kTableNumColumns>
    void OutputTableHeader(const char *const (&aTitles)[kTableNumColumns], const uint8_t (&aWidths)[kTableNumColumns])
    {
        OutputTableHeader(kTableNumColumns, &aTitles[0], &aWidths[0]);
    }

    /**
     * Outputs a table separator to the CLI console.
     *
     * An example of the table separator:
     *
     *    +-----------+--------+------+----------------------+
     *
     * The widths are specified as number chars between two `+` chars (excluding the char `+` itself).
     *
     * @tparam kTableNumColumns   The number columns in the table.
     *
     * @param[in] aWidths   An array specifying the table column widths (in number of chars).
     */
    template <uint8_t kTableNumColumns> void OutputTableSeparator(const uint8_t (&aWidths)[kTableNumColumns])
    {
        OutputTableSeparator(kTableNumColumns, &aWidths[0]);
    }

    /**
     * Outputs the list of commands from a given command table.
     *
     * @tparam Cli      The CLI module type.
     * @tparam kLength  The length of command table array.
     *
     * @param[in] aCommandTable   The command table array.
     */
    template <typename Cli, uint16_t kLength> void OutputCommandTable(const CommandEntry<Cli> (&aCommandTable)[kLength])
    {
        for (const CommandEntry<Cli> &entry : aCommandTable)
        {
            OutputLine("%s", entry.mName);
        }
    }

    /**
     * Clears (sets to zero) all bytes of a given object.
     *
     * @tparam ObjectType    The object type.
     *
     * @param[in] aObject    A reference to the object of type `ObjectType` to clear all its bytes.
     */
    template <typename ObjectType> static void ClearAllBytes(ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        memset(reinterpret_cast<void *>(&aObject), 0, sizeof(ObjectType));
    }

    // Definitions of handlers to process Get/Set/Enable/Disable.
    template <typename ValueType> using GetHandler         = ValueType (&)(otInstance *);
    template <typename ValueType> using SetHandler         = void (&)(otInstance *, ValueType);
    template <typename ValueType> using SetHandlerFailable = otError (&)(otInstance *, ValueType);
    using IsEnabledHandler                                 = bool (&)(otInstance *);
    using SetEnabledHandler                                = void (&)(otInstance *, bool);
    using SetEnabledHandlerFailable                        = otError (&)(otInstance *, bool);

    // Returns format string to output a `ValueType` (e.g., "%u" for `uint16_t`).
    template <typename ValueType> static constexpr const char *FormatStringFor(void);

    /**
     * Checks a given argument string against "enable" or "disable" commands.
     *
     * @param[in]  aArg     The argument string to parse.
     * @param[out] aEnable  Boolean variable to return outcome on success.
     *                      Set to TRUE for "enable" command, and FALSE for "disable" command.
     *
     * @retval OT_ERROR_NONE             Successfully parsed the @p aString and updated @p aEnable.
     * @retval OT_ERROR_INVALID_COMMAND  The @p aString is not "enable" or "disable" command.
     */
    static otError ParseEnableOrDisable(const Arg &aArg, bool &aEnable);

    // General template implementation.
    // Specializations for `uint32_t` and `int32_t` are added at the end.
    template <typename ValueType> otError ProcessGet(Arg aArgs[], GetHandler<ValueType> aGetHandler)
    {
        static_assert(
            TypeTraits::IsSame<ValueType, uint8_t>::kValue || TypeTraits::IsSame<ValueType, uint16_t>::kValue ||
                TypeTraits::IsSame<ValueType, int8_t>::kValue || TypeTraits::IsSame<ValueType, int16_t>::kValue ||
                TypeTraits::IsSame<ValueType, const char *>::kValue,
            "ValueType must be an  8, 16 `int` or `uint` type, or a `const char *`");

        otError error = OT_ERROR_NONE;

        VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        OutputLine(FormatStringFor<ValueType>(), aGetHandler(GetInstancePtr()));

    exit:
        return error;
    }

    template <typename ValueType> otError ProcessSet(Arg aArgs[], SetHandler<ValueType> aSetHandler)
    {
        otError   error;
        ValueType value;

        SuccessOrExit(error = aArgs[0].ParseAs<ValueType>(value));
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        aSetHandler(GetInstancePtr(), value);

    exit:
        return error;
    }

    template <typename ValueType> otError ProcessSet(Arg aArgs[], SetHandlerFailable<ValueType> aSetHandler)
    {
        otError   error;
        ValueType value;

        SuccessOrExit(error = aArgs[0].ParseAs<ValueType>(value));
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        error = aSetHandler(GetInstancePtr(), value);

    exit:
        return error;
    }

    template <typename ValueType>
    otError ProcessGetSet(Arg aArgs[], GetHandler<ValueType> aGetHandler, SetHandler<ValueType> aSetHandler)
    {
        otError error = ProcessGet(aArgs, aGetHandler);

        VerifyOrExit(error != OT_ERROR_NONE);
        error = ProcessSet(aArgs, aSetHandler);

    exit:
        return error;
    }

    template <typename ValueType>
    otError ProcessGetSet(Arg aArgs[], GetHandler<ValueType> aGetHandler, SetHandlerFailable<ValueType> aSetHandler)
    {
        otError error = ProcessGet(aArgs, aGetHandler);

        VerifyOrExit(error != OT_ERROR_NONE);
        error = ProcessSet(aArgs, aSetHandler);

    exit:
        return error;
    }

    otError ProcessEnableDisable(Arg aArgs[], SetEnabledHandler aSetEnabledHandler);
    otError ProcessEnableDisable(Arg aArgs[], SetEnabledHandlerFailable aSetEnabledHandler);
    otError ProcessEnableDisable(Arg aArgs[], IsEnabledHandler aIsEnabledHandler, SetEnabledHandler aSetEnabledHandler);
    otError ProcessEnableDisable(Arg                       aArgs[],
                                 IsEnabledHandler          aIsEnabledHandler,
                                 SetEnabledHandlerFailable aSetEnabledHandler);

    /**
     * Parses a given argument string as a route preference comparing it against  "high", "med", or
     * "low".
     *
     * @param[in]  aArg          The argument string to parse.
     * @param[out] aPreference   Reference to a `otRoutePreference` to return the parsed preference.
     *
     * @retval OT_ERROR_NONE             Successfully parsed @p aArg and updated @p aPreference.
     * @retval OT_ERROR_INVALID_ARG      @p aArg is not a valid preference string "high", "med", or "low".
     */
    static otError ParsePreference(const Arg &aArg, otRoutePreference &aPreference);

    /**
     * Converts a route preference value to human-readable string.
     *
     * @param[in] aPreference   The preference value to convert (`OT_ROUTE_PREFERENCE_*` values).
     *
     * @returns A string representation @p aPreference.
     */
    static const char *PreferenceToString(signed int aPreference);

    /**
     * Parses the argument as an IP address.
     *
     * If the argument string is an IPv4 address, this method will try to synthesize an IPv6 address using preferred
     * NAT64 prefix in the network data.
     *
     * @param[in]  aInstance       A pointer to OpenThread instance.
     * @param[in]  aArg            The argument string to parse.
     * @param[out] aAddress        A reference to an `otIp6Address` to output the parsed IPv6 address.
     * @param[out] aSynthesized    Whether @p aAddress is synthesized from an IPv4 address.
     *
     * @retval OT_ERROR_NONE           The argument was parsed successfully.
     * @retval OT_ERROR_INVALID_ARGS   The argument is empty or does not contain a valid IP address.
     * @retval OT_ERROR_INVALID_STATE  No valid NAT64 prefix in the network data.
     */
    static otError ParseToIp6Address(otInstance   *aInstance,
                                     const Arg    &aArg,
                                     otIp6Address &aAddress,
                                     bool         &aSynthesized);

    /**
     * Parses the argument as a Joiner Discerner.
     *
     * @param[in]  aArg            The argument string to parse.
     * @param[out] aDiscerner      A reference to an `otJoinerDiscerner` to output the parsed discerner
     *
     * @retval OT_ERROR_NONE           The argument was parsed successfully.
     * @retval OT_ERROR_INVALID_ARGS   The argument is empty or does not contain a valid joiner discerner.
     */
    static otError ParseJoinerDiscerner(Arg &aArg, otJoinerDiscerner &aDiscerner);

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    /**
     * Parses the argument as a Border Router configuration.
     *
     * @param[in]  aArg            The argument string to parse.
     * @param[out] aConfig         A reference to an `otBorderRouterConfig` to output the configuration.
     *
     * @retval OT_ERROR_NONE           The argument was parsed successfully.
     * @retval OT_ERROR_INVALID_ARGS   The argument is empty or does not contain a valid configuration.
     */
    static otError ParsePrefix(Arg aArgs[], otBorderRouterConfig &aConfig);

    /**
     * Parses the argument as a External Route configuration.
     *
     * @param[in]  aArg            The argument string to parse.
     * @param[out] aConfig         A reference to an `otExternalRouteConfig` to output the configuration.
     *
     * @retval OT_ERROR_NONE           The argument was parsed successfully.
     * @retval OT_ERROR_INVALID_ARGS   The argument is empty or does not contain a valid configuration.
     */
    static otError ParseRoute(Arg aArgs[], otExternalRouteConfig &aConfig);
#endif

    static constexpr uint8_t kLinkModeStringSize = sizeof("rdn"); ///< Size of string buffer for a MLE Link Mode.

    /**
     * Converts a given MLE Link Mode to flag string.
     *
     * The characters 'r', 'd', and 'n' are respectively used for `mRxOnWhenIdle`, `mDeviceType` and `mNetworkData`
     * flags. If all flags are `false`, then "-" is returned.
     *
     * @param[in]  aLinkMode       The MLE Link Mode to convert.
     * @param[out] aStringBuffer   A reference to an string array to place the string.
     *
     * @returns A pointer @p aStringBuffer which contains the converted string.
     */
    static const char *LinkModeToString(const otLinkModeConfig &aLinkMode, char (&aStringBuffer)[kLinkModeStringSize]);

    /**
     * Converts an IPv6 address origin `OT_ADDRESS_ORIGIN_*` value to human-readable string.
     *
     * @param[in] aOrigin   The IPv6 address origin to convert.
     *
     * @returns A human-readable string representation of @p aOrigin.
     */
    static const char *AddressOriginToString(uint8_t aOrigin);

protected:
    void OutputFormatV(const char *aFormat, va_list aArguments);

#if OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_ENABLE
    void LogInput(const Arg *aArgs);
#else
    void LogInput(const Arg *) {}
#endif

private:
    static constexpr uint16_t kInputOutputLogStringSize = OPENTHREAD_CONFIG_CLI_LOG_INPUT_OUTPUT_LOG_STRING_SIZE;

    void OutputTableHeader(uint8_t aNumColumns, const char *const aTitles[], const uint8_t aWidths[]);
    void OutputTableSeparator(uint8_t aNumColumns, const uint8_t aWidths[]);

    otInstance        *mInstance;
    OutputImplementer &mImplementer;
};

// Specializations of `FormatStringFor<ValueType>()`

template <> inline constexpr const char *Utils::FormatStringFor<uint8_t>(void) { return "%u"; }

template <> inline constexpr const char *Utils::FormatStringFor<uint16_t>(void) { return "%u"; }

template <> inline constexpr const char *Utils::FormatStringFor<uint32_t>(void) { return "%lu"; }

template <> inline constexpr const char *Utils::FormatStringFor<int8_t>(void) { return "%d"; }

template <> inline constexpr const char *Utils::FormatStringFor<int16_t>(void) { return "%d"; }

template <> inline constexpr const char *Utils::FormatStringFor<int32_t>(void) { return "%ld"; }

template <> inline constexpr const char *Utils::FormatStringFor<const char *>(void) { return "%s"; }

// Specialization of ProcessGet<> for `uint32_t` and `int32_t`

template <> inline otError Utils::ProcessGet<uint32_t>(Arg aArgs[], GetHandler<uint32_t> aGetHandler)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputLine(FormatStringFor<uint32_t>(), ToUlong(aGetHandler(GetInstancePtr())));

exit:
    return error;
}

template <> inline otError Utils::ProcessGet<int32_t>(Arg aArgs[], GetHandler<int32_t> aGetHandler)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    OutputLine(FormatStringFor<int32_t>(), static_cast<long int>(aGetHandler(GetInstancePtr())));

exit:
    return error;
}

} // namespace Cli
} // namespace ot

#endif // CLI_UTILS_HPP_
