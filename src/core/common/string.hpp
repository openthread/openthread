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
 *  This file defines OpenThread String class.
 */

#ifndef STRING_HPP_
#define STRING_HPP_

#include "openthread-core-config.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "common/binary_search.hpp"
#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/num_utils.hpp"

namespace ot {

/**
 * @addtogroup core-string
 *
 * @brief
 *   This module includes definitions for OpenThread String class.
 *
 * @{
 */

/**
 * Represents comparison mode when matching strings.
 */
enum StringMatchMode : uint8_t
{
    kStringExactMatch,           ///< Exact match of characters.
    kStringCaseInsensitiveMatch, ///< Case insensitive match (uppercase and lowercase characters are treated as equal).
};

/**
 * Represents string encoding check when copying string.
 */
enum StringEncodingCheck : uint8_t
{
    kStringNoEncodingCheck,   ///< Do not check the string encoding.
    kStringCheckUtf8Encoding, ///< Validate that string follows UTF-8 encoding.
};

static constexpr char kNullChar = '\0'; ///< null character.

/**
 * Returns the number of characters that precede the terminating null character.
 *
 * @param[in] aString      A pointer to the string.
 * @param[in] aMaxLength   The maximum length in bytes.
 *
 * @returns The number of characters that precede the terminating null character or @p aMaxLength,
 *          whichever is smaller. `0` if @p aString is `nullptr`.
 */
uint16_t StringLength(const char *aString, uint16_t aMaxLength);

/**
 * Finds the first occurrence of a given character in a null-terminated string.
 *
 * @param[in] aString     A pointer to the string.
 * @param[in] aChar       A char to search for in the string.
 *
 * @returns The pointer to first occurrence of the @p aChar in @p aString, or `nullptr` if cannot be found.
 */
const char *StringFind(const char *aString, char aChar);

/**
 * Finds the first occurrence of a given sub-string in a null-terminated string.
 *
 * @param[in] aString     A pointer to the string.
 * @param[in] aSubString  A sub-string to search for.
 * @param[in] aMode       The string comparison mode, exact match or case insensitive match.
 *
 * @returns The pointer to first match of the @p aSubString in @p aString (using comparison @p aMode), or `nullptr` if
 *          cannot be found.
 */
const char *StringFind(const char *aString, const char *aSubString, StringMatchMode aMode = kStringExactMatch);

/**
 * Checks whether a null-terminated string starts with a given prefix string.
 *
 * @param[in] aString         A pointer to the string.
 * @param[in] aPrefixString   A prefix string.
 * @param[in] aMode           The string comparison mode, exact match or case insensitive match.
 *
 * @retval TRUE   If @p aString starts with @p aPrefixString.
 * @retval FALSE  If @p aString does not start with @p aPrefixString.
 */
bool StringStartsWith(const char *aString, const char *aPrefixString, StringMatchMode aMode = kStringExactMatch);

/**
 * Checks whether a null-terminated string ends with a given character.
 *
 * @param[in] aString  A pointer to the string.
 * @param[in] aChar    A char to check.
 *
 * @retval TRUE   If @p aString ends with character @p aChar.
 * @retval FALSE  If @p aString does not end with character @p aChar.
 */
bool StringEndsWith(const char *aString, char aChar);

/**
 * Checks whether a null-terminated string ends with a given sub-string.
 *
 * @param[in] aString      A pointer to the string.
 * @param[in] aSubString   A sub-string to check against.
 * @param[in] aMode        The string comparison mode, exact match or case insensitive match.
 *
 * @retval TRUE   If @p aString ends with sub-string @p aSubString.
 * @retval FALSE  If @p aString does not end with sub-string @p aSubString.
 */
bool StringEndsWith(const char *aString, const char *aSubString, StringMatchMode aMode = kStringExactMatch);

/**
 * Checks whether or not two null-terminated strings match exactly.
 *
 * @param[in] aFirstString   A pointer to the first string.
 * @param[in] aSecondString  A pointer to the second string.
 *
 * @retval TRUE   If @p aFirstString matches @p aSecondString.
 * @retval FALSE  If @p aFirstString does not match @p aSecondString.
 */
bool StringMatch(const char *aFirstString, const char *aSecondString);

/**
 * Checks whether or not two null-terminated strings match.
 *
 * @param[in] aFirstString   A pointer to the first string.
 * @param[in] aSecondString  A pointer to the second string.
 * @param[in] aMode          The string comparison mode, exact match or case insensitive match.
 *
 * @retval TRUE   If @p aFirstString matches @p aSecondString using match mode @p aMode.
 * @retval FALSE  If @p aFirstString does not match @p aSecondString using match mode @p aMode.
 */
bool StringMatch(const char *aFirstString, const char *aSecondString, StringMatchMode aMode);

/**
 * Copies a string into a given target buffer with a given size if it fits.
 *
 * @param[out] aTargetBuffer  A pointer to the target buffer to copy into.
 * @param[out] aTargetSize    The size (number of characters) in @p aTargetBuffer array.
 * @param[in]  aSource        A pointer to null-terminated string to copy from. Can be `nullptr` which treated as "".
 * @param[in]  aEncodingCheck Specifies the encoding format check (e.g., UTF-8) to perform.
 *
 * @retval kErrorNone         The @p aSource fits in the given buffer. @p aTargetBuffer is updated.
 * @retval kErrorInvalidArgs  The @p aSource does not fit in the given buffer.
 * @retval kErrorParse        The @p aSource does not follow the encoding format specified by @p aEncodingCheck.
 */
Error StringCopy(char *TargetBuffer, uint16_t aTargetSize, const char *aSource, StringEncodingCheck aEncodingCheck);

/**
 * Copies a string into a given target buffer with a given size if it fits.
 *
 * @tparam kSize  The size of buffer.
 *
 * @param[out] aTargetBuffer  A reference to the target buffer array to copy into.
 * @param[in]  aSource        A pointer to null-terminated string to copy from. Can be `nullptr` which treated as "".
 * @param[in]  aEncodingCheck Specifies the encoding format check (e.g., UTF-8) to perform.
 *
 * @retval kErrorNone         The @p aSource fits in the given buffer. @p aTargetBuffer is updated.
 * @retval kErrorInvalidArgs  The @p aSource does not fit in the given buffer.
 * @retval kErrorParse        The @p aSource does not follow the encoding format specified by @p aEncodingCheck.
 */
template <uint16_t kSize>
Error StringCopy(char (&aTargetBuffer)[kSize],
                 const char         *aSource,
                 StringEncodingCheck aEncodingCheck = kStringNoEncodingCheck)
{
    return StringCopy(aTargetBuffer, kSize, aSource, aEncodingCheck);
}

/**
 * Parses a decimal number from a string as `uint8_t` and skips over the parsed characters.
 *
 * If the string does not start with a digit, `kErrorParse` is returned.
 *
 * All the digit characters in the string are parsed until reaching a non-digit character. The pointer `aString` is
 * updated to point to the first non-digit character after the parsed digits.
 *
 * If the parsed number value is larger than @p aMaxValue, `kErrorParse` is returned.
 *
 * @param[in,out] aString    A reference to a pointer to string to parse.
 * @param[out]    aUint8     A reference to return the parsed value.
 * @param[in]     aMaxValue  Maximum allowed value for the parsed number.
 *
 * @retval kErrorNone   Successfully parsed the number from string. @p aString and @p aUint8 are updated.
 * @retval kErrorParse  Failed to parse the number from @p aString, or parsed number is larger than @p aMaxValue.
 */
Error StringParseUint8(const char *&aString, uint8_t &aUint8, uint8_t aMaxValue);

/**
 * Parses a decimal number from a string as `uint8_t` and skips over the parsed characters.
 *
 * If the string does not start with a digit, `kErrorParse` is returned.
 *
 * All the digit characters in the string are parsed until reaching a non-digit character. The pointer `aString` is
 * updated to point to the first non-digit character after the parsed digits.
 *
 * If the parsed number value is larger than maximum `uint8_t` value, `kErrorParse` is returned.
 *
 * @param[in,out] aString    A reference to a pointer to string to parse.
 * @param[out]    aUint8     A reference to return the parsed value.
 *
 * @retval kErrorNone   Successfully parsed the number from string. @p aString and @p aUint8 are updated.
 * @retval kErrorParse  Failed to parse the number from @p aString, or parsed number is out of range.
 */
Error StringParseUint8(const char *&aString, uint8_t &aUint8);

/**
 * Converts all uppercase letter characters in a given string to lowercase.
 *
 * @param[in,out] aString   A pointer to the string to convert.
 */
void StringConvertToLowercase(char *aString);

/**
 * Converts all lowercase letter characters in a given string to uppercase.
 *
 * @param[in,out] aString   A pointer to the string to convert.
 */
void StringConvertToUppercase(char *aString);

/**
 * Converts an uppercase letter character to lowercase.
 *
 * If @p aChar is uppercase letter it is converted lowercase. Otherwise, it remains unchanged.
 *
 * @param[in] aChar   The character to convert
 *
 * @returns The character converted to lowercase.
 */
char ToLowercase(char aChar);

/**
 * Converts a lowercase letter character to uppercase.
 *
 * If @p aChar is lowercase letter it is converted uppercase. Otherwise, it remains unchanged.
 *
 * @param[in] aChar   The character to convert
 *
 * @returns The character converted to uppercase.
 */
char ToUppercase(char aChar);

/**
 * Checks whether a given character is an uppercase letter ('A'-'Z').
 *
 * @param[in] aChar   The character to check.
 *
 * @retval TRUE    @p aChar is an uppercase letter.
 * @retval FALSE   @p aChar is not an uppercase letter.
 */
bool IsUppercase(char aChar);

/**
 * Checks whether a given character is a lowercase letter ('a'-'z').
 *
 * @param[in] aChar   The character to check.
 *
 * @retval TRUE    @p aChar is a lowercase letter.
 * @retval FALSE   @p aChar is not a lowercase letter.
 */
bool IsLowercase(char aChar);

/**
 * Checks whether a given character is a digit character ('0'-'9').
 *
 * @param[in] aChar   The character to check.
 *
 * @retval TRUE    @p aChar is a digit character.
 * @retval FALSE   @p aChar is not a digit character.
 */
bool IsDigit(char aChar);

/**
 * Parse a given digit character to its numeric value.
 *
 * @param[in]  aDigitChar   The digit character to parse.
 * @param[out] aValue       A reference to return the parsed value on success.
 *
 * @retval kErrorNone            Successfully parsed the digit, @p aValue is updated.
 * @retval kErrorInvalidArgs     @p aDigitChar is not a valid digit character.
 */
Error ParseDigit(char aDigitChar, uint8_t &aValue);

/**
 * Parse a given hex digit character ('0'-'9', 'A'-'F', or 'a'-'f') to its numeric value.
 *
 * @param[in]  aHexChar     The hex digit character to parse.
 * @param[out] aValue       A reference to return the parsed value on success.
 *
 * @retval kErrorNone            Successfully parsed the digit, @p aValue is updated.
 * @retval kErrorInvalidArgs     @p aHexChar is not a valid hex digit character.
 */
Error ParseHexDigit(char aHexChar, uint8_t &aValue);

/**
 * Converts a boolean to "yes" or "no" string.
 *
 * @param[in] aBool  A boolean value to convert.
 *
 * @returns The converted string representation of @p aBool ("yes" for TRUE and "no" for FALSE).
 */
const char *ToYesNo(bool aBool);

/**
 * Validates whether a given byte sequence (string) follows UTF-8 encoding.
 * Control characters are not allowed.
 *
 * @param[in]  aString  A null-terminated byte sequence.
 *
 * @retval TRUE   The sequence is a valid UTF-8 string.
 * @retval FALSE  The sequence is not a valid UTF-8 string.
 */
bool IsValidUtf8String(const char *aString);

/**
 * Validates whether a given byte sequence (string) follows UTF-8 encoding.
 * Control characters are not allowed.
 *
 * @param[in]  aString  A byte sequence.
 * @param[in]  aLength  Length of the sequence.
 *
 * @retval TRUE   The sequence is a valid UTF-8 string.
 * @retval FALSE  The sequence is not a valid UTF-8 string.
 */
bool IsValidUtf8String(const char *aString, size_t aLength);

/**
 * This `constexpr` function checks whether two given C strings are in order (alphabetical order).
 *
 * This is intended for use from `static_assert`, e.g., checking if a lookup table entries are sorted. It is not
 * recommended to use this function in other situations as it uses recursion so that it can be `constexpr`.
 *
 * @param[in] aFirst    The first string.
 * @param[in] aSecond   The second string.
 *
 * @retval TRUE  If first string is strictly before second string (alphabetical order).
 * @retval FALSE If first string is not strictly before second string (alphabetical order).
 */
inline constexpr bool AreStringsInOrder(const char *aFirst, const char *aSecond)
{
    return (*aFirst < *aSecond)
               ? true
               : ((*aFirst > *aSecond) || (*aFirst == '\0') ? false : AreStringsInOrder(aFirst + 1, aSecond + 1));
}

/**
 * Implements writing to a string buffer.
 */
class StringWriter
{
public:
    /**
     * Initializes the object as cleared on the provided buffer.
     *
     * @param[in] aBuffer  A pointer to the char buffer to write into.
     * @param[in] aSize    The size of @p aBuffer.
     */
    StringWriter(char *aBuffer, uint16_t aSize);

    /**
     * Clears the string writer.
     *
     * @returns The string writer.
     */
    StringWriter &Clear(void);

    /**
     * Returns whether the output is truncated.
     *
     * @note If the output is truncated, the buffer is still null-terminated.
     *
     * @retval  true    The output is truncated.
     * @retval  false   The output is not truncated.
     */
    bool IsTruncated(void) const { return mLength >= mSize; }

    /**
     * Gets the length of the wanted string.
     *
     * Similar to `strlen()` the length does not include the null character at the end of the string.
     *
     * @returns The string length.
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * Returns the size (number of chars) in the buffer.
     *
     * @returns The size of the buffer.
     */
    uint16_t GetSize(void) const { return mSize; }

    /**
     * Appends `printf()` style formatted data to the buffer.
     *
     * @param[in] aFormat    A pointer to the format string.
     * @param[in] ...        Arguments for the format specification.
     *
     * @returns The string writer.
     */
    StringWriter &Append(const char *aFormat, ...) OT_TOOL_PRINTF_STYLE_FORMAT_ARG_CHECK(2, 3);

    /**
     * Appends `printf()` style formatted data to the buffer.
     *
     * @param[in] aFormat    A pointer to the format string.
     * @param[in] aArgs      Arguments for the format specification (as `va_list`).
     *
     * @returns The string writer.
     */
    StringWriter &AppendVarArgs(const char *aFormat, va_list aArgs);

    /**
     * Appends an array of bytes in hex representation (using "%02x" style) to the buffer.
     *
     * @param[in] aBytes    A pointer to buffer containing the bytes to append.
     * @param[in] aLength   The length of @p aBytes buffer (in bytes).
     *
     * @returns The string writer.
     */
    StringWriter &AppendHexBytes(const uint8_t *aBytes, uint16_t aLength);

    /**
     * Appends a given character a given number of times.
     *
     * @param[in] aChar    The character to append.
     * @param[in] aCount   Number of times to append @p aChar.
     */
    StringWriter &AppendCharMultipleTimes(char aChar, uint16_t aCount);

    /**
     * Converts all uppercase letter characters in the string to lowercase.
     */
    void ConvertToLowercase(void) { StringConvertToLowercase(mBuffer); }

    /**
     * Converts all lowercase letter characters in the string to uppercase.
     */
    void ConvertToUppercase(void) { StringConvertToUppercase(mBuffer); }

private:
    char          *mBuffer;
    uint16_t       mLength;
    const uint16_t mSize;
};

/**
 * Defines a fixed-size string.
 */
template <uint16_t kSize> class String : public StringWriter
{
    static_assert(kSize > 0, "String buffer cannot be empty.");

public:
    /**
     * Initializes the string as empty.
     */
    String(void)
        : StringWriter(mBuffer, sizeof(mBuffer))
    {
    }

    /**
     * Returns the string as a null-terminated C string.
     *
     * @returns The null-terminated C string.
     */
    const char *AsCString(void) const { return mBuffer; }

private:
    char mBuffer[kSize];
};

/**
 * Provides helper methods to convert from a set of `uint16_t` values (e.g., a non-sequential `enum`) to
 * string using binary search in a lookup table.
 */
class Stringify : public BinarySearch
{
public:
    /**
     * Represents a entry in the lookup table.
     */
    class Entry
    {
        friend class BinarySearch;

    public:
        uint16_t    mKey;    ///< The key value.
        const char *mString; ///< The associated string.

    private:
        int Compare(uint16_t aKey) const { return ThreeWayCompare(aKey, mKey); }

        constexpr static bool AreInOrder(const Entry &aFirst, const Entry &aSecond)
        {
            return aFirst.mKey < aSecond.mKey;
        }
    };

    /**
     * Looks up a key in a given sorted table array (using binary search) and return the associated
     * strings with the key.
     *
     * @note This method requires the array to be sorted, otherwise its behavior is undefined.
     *
     * @tparam kLength     The array length (number of entries in the array).
     *
     * @param[in] aKey       The key to search for within the table.
     * @param[in] aTable     A reference to an array of `kLength` entries.
     * @param[in] aNotFound  A C string to return if @p aKey was not found in the table.
     *
     * @returns The associated string with @p aKey in @p aTable if found, or @p aNotFound otherwise.
     */
    template <uint16_t kLength>
    static const char *Lookup(uint16_t aKey, const Entry (&aTable)[kLength], const char *aNotFound = "unknown")
    {
        const Entry *entry = BinarySearch::Find(aKey, aTable);

        return (entry != nullptr) ? entry->mString : aNotFound;
    }

    Stringify(void) = delete;
};

/**
 * @}
 */

} // namespace ot

#endif // STRING_HPP_
