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
 *   This file includes definitions for command line parser.
 */

#ifndef PARSE_CMD_LINE_HPP_
#define PARSE_CMD_LINE_HPP_

#include <stdint.h>
#include <string.h>

#include <openthread/error.h>
#include <openthread/ip6.h>

namespace ot {
namespace Utils {
namespace CmdLineParser {

/**
 * @addtogroup utils-parse-cmd-line
 *
 * @brief
 *   This module includes definitions for command line parser.
 *
 * @{
 */

/**
 * This enumeration type represents the parse mode value used as a parameter in `ParseAsHexString()`.
 *
 */
enum HexStringParseMode : uint8_t
{
    kDisallowTruncate, // Disallow truncation of hex string.
    kAllowTruncate,    // Allow truncation of hex string.
};

/**
 * This function parses a string as a `uint8_t` value.
 *
 * The number in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aUint8    A reference to an `uint8_t` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number (e.g., value out of range).
 *
 */
otError ParseAsUint8(const char *aString, uint8_t &aUint8);

/**
 * This function parses a string as a `uint16_t` value.
 *
 * The number in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aUint16   A reference to an `uint16_t` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number (e.g., value out of range).
 *
 */
otError ParseAsUint16(const char *aString, uint16_t &aUint16);

/**
 * This function parses a string as a `uint32_t` value.
 *
 * The number in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aUint32   A reference to an `uint32_t` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number (e.g., value out of range).
 *
 */
otError ParseAsUint32(const char *aString, uint32_t &aUint32);

/**
 * This function parses a string as a `uint64_t` value.
 *
 * The number in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aUint64   A reference to an `uint64_t` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number (e.g., value out of range).
 *
 */
otError ParseAsUint64(const char *aString, uint64_t &aUint64);

/**
 * This function parses a string as a `int8_t` value.
 *
 * The number in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string can start with
 * `+`/`-` sign.
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aInt8     A reference to an `int8_t` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number (e.g., value out of range).
 *
 */
otError ParseAsInt8(const char *aString, int8_t &aInt8);

/**
 * This function parses a string as a `int16_t` value.
 *
 * The number in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string can start with
 * `+`/`-` sign.
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aInt16    A reference to an `int16_t` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number (e.g., value out of range).
 *
 */
otError ParseAsInt16(const char *aString, int16_t &aInt16);

/**
 * This function parses a string as a `int32_t` value.
 *
 * The number in string is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string can start with
 * `+`/`-` sign.
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aInt32    A reference to an `int32_t` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number (e.g., value out of range).
 *
 */
otError ParseAsInt32(const char *aString, int32_t &aInt32);

/**
 * This function parses a string as a `bool` value.
 *
 * Zero value is treated as `false`, non-zero value as `true`.
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aBool     A reference to a `bool` variable to output the parsed value.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid number.
 *
 */
otError ParseAsBool(const char *aString, bool &aBool);

#if OPENTHREAD_FTD || OPENTHREAD_MTD
/**
 * This function parses a string as an IPv6 address.
 *
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aAddress  A reference to an `otIp6Address` to output the parsed IPv6 address.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid IPv6 address.
 *
 */
inline otError ParseAsIp6Address(const char *aString, otIp6Address &aAddress)
{
    return otIp6AddressFromString(aString, &aAddress);
}

/**
 * This function parses a string as an IPv6 prefix.
 *
 * The string is parsed as `{IPv6Address}/{PrefixLength}`.
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aPrefix   A reference to an `otIp6Prefix` to output the parsed IPv6 prefix.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain a valid IPv6 prefix.
 *
 */
otError ParseAsIp6Prefix(const char *aString, otIp6Prefix &aPrefix);
#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

/**
 * This function parses a hex string into a byte array of fixed expected size.
 *
 * This function returns `kErrorNone` only when the hex string contains exactly @p aSize bytes (after parsing). If
 * there are fewer or more bytes in hex string that @p aSize, the parsed bytes (up to @p aSize) are copied into the
 * `aBuffer` and `kErrorInvalidArgs` is returned.
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aBuffer   A pointer to a buffer to output the parsed byte sequence.
 * @param[in]  aSize     The expected size of byte sequence (number of bytes after parsing).
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid hex bytes and/or not @p aSize bytes.
 *
 */
otError ParseAsHexString(const char *aString, uint8_t *aBuffer, uint16_t aSize);

/**
 * This template function parses a hex string into a a given fixed size array.
 *
 * This function returns `kErrorNone` only when the hex string contains exactly @p kBufferSize bytes (after parsing).
 * If there are fewer or more bytes in hex string that @p kBufferSize, the parsed bytes (up to @p kBufferSize) are
 * copied into the `aBuffer` and `kErrorInvalidArgs` is returned.
 *
 * @tparam kBufferSize   The byte array size (number of bytes).
 *
 * @param[in]  aString   The string to parse.
 * @param[out] aBuffer   A reference to a byte array to output the parsed byte sequence.
 *
 * @retval kErrorNone         The string was parsed successfully.
 * @retval kErrorInvalidArgs  The string does not contain valid hex bytes and/or not @p aSize bytes.
 *
 */
template <uint16_t kBufferSize> static otError ParseAsHexString(const char *aString, uint8_t (&aBuffer)[kBufferSize])
{
    return ParseAsHexString(aString, aBuffer, kBufferSize);
}

/**
 * This function parses a hex string into a byte array.
 *
 * If @p aMode disallows truncation (`kDisallowTruncate`), this function verifies that parses hex string bytes fit in
 * @p aBuffer with its given size in @aSize. Otherwise when @p aMode allows truncation, extra bytes after @p aSize bytes
 * are ignored.
 *
 * @param[in]     aString   The string to parse.
 * @param[inout]  aSize     On entry indicates the number of bytes in @p aBuffer (max size of @p aBuffer).
 *                          On exit provides number of bytes parsed and copied into @p aBuffer
 * @param[out]    aBuffer   A pointer to a buffer to output the parsed byte sequence.
 * @param[in]     aMode     Indicates parsing mode whether to allow truncation or not.
 *
 * @retval kErrorNone        The string was parsed successfully.
 * @retval kErrorInvalidArgs The string does not contain valid format or too many bytes (if truncation not allowed)
 *
 */
otError ParseAsHexString(const char *       aString,
                         uint16_t &         aSize,
                         uint8_t *          aBuffer,
                         HexStringParseMode aMode = kDisallowTruncate);

/**
 * This class represents a single argument from an argument list.
 *
 */
class Arg
{
public:
    /**
     * This method returns the length (number of characters) in the argument C string.
     *
     * @returns The argument string length.
     *
     */
    uint16_t GetLength(void) const { return static_cast<uint16_t>(strlen(mString)); }

    /**
     * This method gets the argument as a C string.
     *
     * @returns A pointer to the argument as a C string.
     *
     */
    const char *GetCString(void) const { return mString; }

    /**
     * This method gets the argument as C string.
     *
     * @returns A pointer to the argument as a C string.
     *
     */
    char *GetCString(void) { return mString; }

    /**
     * This method sets the argument with a given C string.
     *
     * @param[in] aString    A pointer to the new C string.
     *
     */
    void SetCString(char *aString) { mString = aString; }

    /**
     * This method overload the operator `==` to evaluate whether the argument is equal to a given C string.
     *
     * @param[in] aString    The C string to compare with.
     *
     * @retval TRUE   If the argument is equal to @p aString.
     * @retval FALSE  If the argument is not equal to @p aString.
     *
     */
    bool operator==(const char *aString) const { return (strcmp(mString, aString) == 0); }

    /**
     * This method overload the operator `!=` to evaluate whether the argument is unequal to a given C string.
     *
     * @param[in] aString    The C string to compare with.
     *
     * @retval TRUE   If the argument is not equal to @p aString.
     * @retval FALSE  If the argument is equal to @p aString.
     *
     */
    bool operator!=(const char *aString) const { return !(*this == aString); }

    /**
     * This method parses the argument as a `uint8_t` value.
     *
     * The number is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
     *
     * @param[out] aUint8    A reference to an `uint8_t` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number (e.g., value out of range).
     *
     */
    otError ParseAsUint8(uint8_t &aUint8) const { return CmdLineParser::ParseAsUint8(mString, aUint8); }

    /**
     * This method parses the argument as a `uint16_t` value.
     *
     * The number is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
     *
     * @param[out] aUint16   A reference to an `uint16_t` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number (e.g., value out of range).
     *
     */
    otError ParseAsUint16(uint16_t &aUint16) const { return CmdLineParser::ParseAsUint16(mString, aUint16); }

    /**
     * This method parses the argument as a `uint32_t` value.
     *
     * The number is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
     *
     * @param[out] aUint32   A reference to an `uint32_t` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number (e.g., value out of range).
     *
     */
    otError ParseAsUint32(uint32_t &aUint32) const { return CmdLineParser::ParseAsUint32(mString, aUint32); }

    /**
     * This method parses the argument as a `uint64_t` value.
     *
     * The number is parsed as decimal or hex format (if contains `0x` or `0X` prefix).
     *
     * @param[out] aUint64   A reference to an `uint64_t` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number (e.g., value out of range).
     *
     */
    otError ParseAsUint64(uint64_t &aUint64) const { return CmdLineParser::ParseAsUint64(mString, aUint64); }

    /**
     * This method parses the argument as a `int8_t` value.
     *
     * The number is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string can start with
     * `+`/`-` sign.
     *
     * @param[out] aInt8     A reference to an `int8_t` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number (e.g., value out of range).
     *
     */
    otError ParseAsInt8(int8_t &aInt8) const { return CmdLineParser::ParseAsInt8(mString, aInt8); }

    /**
     * This method parses the argument as a `int16_t` value.
     *
     * The number is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string can start with
     * `+`/`-` sign.
     *
     * @param[out] aInt16    A reference to an `int16_t` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number (e.g., value out of range).
     *
     */
    otError ParseAsInt16(int16_t &aInt16) const { return CmdLineParser::ParseAsInt16(mString, aInt16); }

    /**
     * This method parses the argument as a `int32_t` value.
     *
     * The number is parsed as decimal or hex format (if contains `0x` or `0X` prefix). The string can start with
     * `+`/`-` sign.
     *
     * @param[out] aInt32    A reference to an `int32_t` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number (e.g., value out of range).
     *
     */
    otError ParseAsInt32(int32_t &aInt32) const { return CmdLineParser::ParseAsInt32(mString, aInt32); }

    /**
     * This method parses the argument as a `bool` value.
     *
     * Zero value is treated as `false`, non-zero value as `true`.
     *
     * @param[out] aBool     A reference to a `bool` variable to output the parsed value.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid number.
     *
     */
    otError ParseAsBool(bool &aBool) const { return CmdLineParser::ParseAsBool(mString, aBool); }

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    /**
     * This method parses the argument as an IPv6 address.
     *
     * @param[out] aAddress  A reference to an `otIp6Address` to output the parsed IPv6 address.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid IPv6 address.
     *
     */
    otError ParseAsIp6Address(otIp6Address &aAddress) const
    {
        return CmdLineParser::ParseAsIp6Address(mString, aAddress);
    }

    /**
     * This method parses the argument as an IPv6 prefix.
     *
     * The string is parsed as `{IPv6Address}/{PrefixLength}`.
     *
     * @param[out] aPrefix   A reference to an `otIp6Prefix` to output the parsed IPv6 prefix.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain a valid IPv6 prefix.
     *
     */
    otError ParseAsIp6Prefix(otIp6Prefix &aPrefix) const { return CmdLineParser::ParseAsIp6Prefix(mString, aPrefix); }

#endif // OPENTHREAD_FTD || OPENTHREAD_MTD

    /**
     * This method parses the argument as a hex string into a byte array of fixed expected size.
     *
     * This method returns `kErrorNone` only when the hex string contains exactly @p aSize bytes (after parsing). If
     * there are fewer or more bytes in hex string that @p aSize, the parsed bytes (up to @p aSize) are copied into the
     * `aBuffer` and `kErrorInvalidArgs` is returned.
     *
     * @param[out] aBuffer   A pointer to a buffer to output the parsed byte sequence.
     * @param[in]  aSize     The expected size of byte sequence (number of bytes after parsing).
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid hex bytes and/or not @p aSize bytes.
     *
     */
    otError ParseAsHexString(uint8_t *aBuffer, uint16_t aSize) const
    {
        return CmdLineParser::ParseAsHexString(mString, aBuffer, aSize);
    }

    /**
     * This template method parses the argument as a hex string into a a given fixed size array.
     *
     * This method returns `kErrorNone` only when the hex string contains exactly @p kBufferSize bytes (after parsing).
     * If there are fewer or more bytes in hex string that @p kBufferSize, the parsed bytes (up to @p kBufferSize) are
     * copied into the `aBuffer` and `kErrorInvalidArgs` is returned.
     *
     * @tparam kBufferSize   The byte array size (number of bytes).
     *
     * @param[out] aBuffer   A reference to a byte array to output the parsed byte sequence.
     *
     * @retval kErrorNone         The argument was parsed successfully.
     * @retval kErrorInvalidArgs  The argument does not contain valid hex bytes and/or not @p aSize bytes.
     *
     */
    template <uint16_t kBufferSize> otError ParseAsHexString(uint8_t (&aBuffer)[kBufferSize])
    {
        return ParseAsHexString(aBuffer, kBufferSize);
    }

    /**
     * This method parses the argument as a hex string into a byte array.
     *
     * If @p aMode disallows truncation (`kDisallowTruncate`), this method verifies that parses hex string bytes fit in
     * @p aBuffer with its given size in @aSize. Otherwise when @p aMode allows truncation, extra bytes after @p aSize
     * bytes are ignored.
     *
     * @param[inout]  aSize     On entry indicates the number of bytes in @p aBuffer (max size of @p aBuffer).
     *                          On exit provides number of bytes parsed and copied into @p aBuffer
     * @param[out]    aBuffer   A pointer to a buffer to output the parsed byte sequence.
     * @param[in]     aMode     Indicates parsing mode whether to allow truncation or not.
     *
     * @retval kErrorNone        The argument was parsed successfully.
     * @retval kErrorInvalidArgs The argument does not contain valid format or too many bytes (if truncation not
     *                           allowed)
     *
     */
    otError ParseAsHexString(uint16_t &aSize, uint8_t *aBuffer, HexStringParseMode aMode = kDisallowTruncate)
    {
        return CmdLineParser::ParseAsHexString(mString, aSize, aBuffer, aMode);
    }

    /**
     * This static method copies the argument string pointers from an `Arg` array to a C string array.
     *
     * @note this method only copies the string pointer value (i.e., `GetString()` pointer) from `aArgs` array to the
     * @p aStrings array (the content of strings are not copied).
     *
     * @param[in] aArgs         A pointer to an `Arg` array.
     * @param[in] aArgsLength   Number of entries in the @p aArgs array.
     * @param[out] aStrings     An `char *` array to populate with the argument string pointers. The @p aString array
     *                          MUST contain at least @p aArgsLength entries.
     *
     */
    static void CopyArgsToStringArray(Arg aArgs[], uint8_t aArgsLength, char *aStrings[]);

private:
    char *mString;
};

/**
 * This function parses a given command line string and breaks it into an argument list.
 *
 * Note: this method may change the input @p aCommandString, it will put a '\0' by the end of each argument,
 *       and @p aArgs will point to the arguments in the input @p aCommandString. Backslash ('\') can be used
 *       to escape separators (' ', '\t', '\r', '\n') and the backslash itself.
 *
 * @param[in]   aCommandString  A null-terminated input string.
 * @param[out]  aArgsLength     The argument counter of the command line.
 * @param[out]  aArgs           The argument vector of the command line.
 * @param[in]   aArgsLengthMax  The maximum argument counter.
 *
 */
otError ParseCmd(char *aCommandString, uint8_t &aArgsLength, Arg aArgs[], uint8_t aArgsLengthMax);

/**
 * @}
 */

} // namespace CmdLineParser
} // namespace Utils
} // namespace ot

#endif // PARSE_CMD_LINE_HPP_
