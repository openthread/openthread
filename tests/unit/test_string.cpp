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

#include "test_platform.h"

#include <openthread/config.h>

#include "test_util.h"
#include "common/code_utils.hpp"
#include "common/string.hpp"

namespace ot {

enum
{
    kStringSize = 10,
};

template <uint16_t kSize> void PrintString(const char *aName, const String<kSize> aString)
{
    printf("\t%s = [%zu] \"%s\"\n", aName, strlen(aString.AsCString()), aString.AsCString());
}

void TestStringWriter(void)
{
    String<kStringSize> str;
    const char          kLongString[] = "abcdefghijklmnopqratuvwxyzabcdefghijklmnopqratuvwxyz";

    printf("\nTest 1: StringWriter constructor\n");

    VerifyOrQuit(str.GetSize() == kStringSize);
    VerifyOrQuit(str.GetLength() == 0, "failed for empty string");

    VerifyOrQuit(strcmp(str.AsCString(), "") == 0);

    PrintString("str", str);

    printf(" -- PASS\n");

    printf("\nTest 2: StringWriter::Append() method\n");

    str.Append("Hi");
    VerifyOrQuit(str.GetLength() == 2);
    VerifyOrQuit(strcmp(str.AsCString(), "Hi") == 0);
    PrintString("str", str);

    str.Append("%s%d", "!", 12);
    VerifyOrQuit(str.GetLength() == 5);
    VerifyOrQuit(strcmp(str.AsCString(), "Hi!12") == 0);
    PrintString("str", str);

    str.Append(kLongString);
    VerifyOrQuit(str.IsTruncated() && str.GetLength() == 5 + sizeof(kLongString) - 1,
                 "String::Append() did not handle overflow buffer correctly");
    PrintString("str", str);

    printf("\nTest 3: StringWriter::Clear() method\n");

    str.Clear();
    str.Append("Hello");
    VerifyOrQuit(str.GetLength() == 5);
    VerifyOrQuit(strcmp(str.AsCString(), "Hello") == 0);
    PrintString("str", str);

    str.Clear();
    VerifyOrQuit(str.GetLength() == 0, "failed after Clear()");
    VerifyOrQuit(strcmp(str.AsCString(), "") == 0);

    str.Append("%d", 12);
    VerifyOrQuit(str.GetLength() == 2);
    VerifyOrQuit(strcmp(str.AsCString(), "12") == 0);
    PrintString("str", str);

    str.Clear();
    str.Append(kLongString);
    VerifyOrQuit(str.IsTruncated() && str.GetLength() == sizeof(kLongString) - 1,
                 "String::Clear() + String::Append() did not handle overflow buffer correctly");
    PrintString("str", str);

    printf(" -- PASS\n");
}

void TestStringLength(void)
{
    char string_a[5] = "\0foo";
    char string_b[8] = "foo\0bar";

    printf("\nTest 4: String::StringLength() method\n");

    VerifyOrQuit(StringLength(string_a, 0) == 0);
    VerifyOrQuit(StringLength(string_a, 1) == 0);
    VerifyOrQuit(StringLength(string_a, 2) == 0);

    VerifyOrQuit(StringLength(string_b, 0) == 0);
    VerifyOrQuit(StringLength(string_b, 1) == 1);
    VerifyOrQuit(StringLength(string_b, 2) == 2);
    VerifyOrQuit(StringLength(string_b, 3) == 3);
    VerifyOrQuit(StringLength(string_b, 4) == 3);
    VerifyOrQuit(StringLength(string_b, 5) == 3);
    VerifyOrQuit(StringLength(string_b, 6) == 3);

    printf(" -- PASS\n");
}

void TestUtf8(void)
{
    printf("\nTest 5: IsValidUtf8String() function\n");

    VerifyOrQuit(IsValidUtf8String("An ASCII string"));
    VerifyOrQuit(IsValidUtf8String(u8"Строка UTF-8"));
    VerifyOrQuit(!IsValidUtf8String("\xbf"));
    VerifyOrQuit(!IsValidUtf8String("\xdf"));
    VerifyOrQuit(!IsValidUtf8String("\xef\x80"));
    VerifyOrQuit(!IsValidUtf8String("\xf7\x80\x80"));
    VerifyOrQuit(!IsValidUtf8String("\xff"));
    VerifyOrQuit(!IsValidUtf8String("NUL\x00NUL", 7)); // UTF-8 NUL
    VerifyOrQuit(!IsValidUtf8String("abcde\x11"));     // control character

    printf(" -- PASS\n");
}

void TestStringFind(void)
{
    char emptyString[1] = {'\0'};
    char testString[]   = "Foo.bar.bar\\.";
    char testString2[]  = "abcabcabcdabc";

    printf("\nTest 6: StringFind() function\n");

    VerifyOrQuit(StringFind(testString, 'F') == testString);
    VerifyOrQuit(StringFind(testString, 'o') == &testString[1]);
    VerifyOrQuit(StringFind(testString, '.') == &testString[3]);
    VerifyOrQuit(StringFind(testString, 'r') == &testString[6]);
    VerifyOrQuit(StringFind(testString, '\\') == &testString[11]);
    VerifyOrQuit(StringFind(testString, 'x') == nullptr);
    VerifyOrQuit(StringFind(testString, ',') == nullptr);

    VerifyOrQuit(StringFind(emptyString, 'F') == nullptr);
    VerifyOrQuit(StringFind(emptyString, '.') == nullptr);

    VerifyOrQuit(StringFind(testString, "Foo") == &testString[0]);
    VerifyOrQuit(StringFind(testString, "oo") == &testString[1]);
    VerifyOrQuit(StringFind(testString, "bar") == &testString[4]);
    VerifyOrQuit(StringFind(testString, "bar\\") == &testString[8]);
    VerifyOrQuit(StringFind(testString, "\\.") == &testString[11]);
    VerifyOrQuit(StringFind(testString, testString) == testString);
    VerifyOrQuit(StringFind(testString, "Fooo") == nullptr);
    VerifyOrQuit(StringFind(testString, "Far") == nullptr);
    VerifyOrQuit(StringFind(testString, "FOO") == nullptr);
    VerifyOrQuit(StringFind(testString, "BAR") == nullptr);
    VerifyOrQuit(StringFind(testString, "bar\\..") == nullptr);
    VerifyOrQuit(StringFind(testString, "") == &testString[0]);

    VerifyOrQuit(StringFind(emptyString, "foo") == nullptr);
    VerifyOrQuit(StringFind(emptyString, "bar") == nullptr);
    VerifyOrQuit(StringFind(emptyString, "") == &emptyString[0]);

    // Verify when sub-string has repeated patterns
    VerifyOrQuit(StringFind(testString2, "abcabc") == &testString2[0]);
    VerifyOrQuit(StringFind(testString2, "abcabcd") == &testString2[3]);

    VerifyOrQuit(StringFind(testString, "FOO", kStringCaseInsensitiveMatch) == &testString[0]);
    VerifyOrQuit(StringFind(testString, "OO", kStringCaseInsensitiveMatch) == &testString[1]);
    VerifyOrQuit(StringFind(testString, "BAR", kStringCaseInsensitiveMatch) == &testString[4]);
    VerifyOrQuit(StringFind(testString, "BAR\\", kStringCaseInsensitiveMatch) == &testString[8]);
    VerifyOrQuit(StringFind(testString, "\\.", kStringCaseInsensitiveMatch) == &testString[11]);
    VerifyOrQuit(StringFind(testString, testString) == testString);
    VerifyOrQuit(StringFind(testString, "FOOO", kStringCaseInsensitiveMatch) == nullptr);
    VerifyOrQuit(StringFind(testString, "FAR", kStringCaseInsensitiveMatch) == nullptr);
    VerifyOrQuit(StringFind(testString, "BAR\\..", kStringCaseInsensitiveMatch) == nullptr);
    VerifyOrQuit(StringFind(testString, "", kStringCaseInsensitiveMatch) == &testString[0]);

    VerifyOrQuit(StringFind(emptyString, "FOO", kStringCaseInsensitiveMatch) == nullptr);
    VerifyOrQuit(StringFind(emptyString, "BAR", kStringCaseInsensitiveMatch) == nullptr);
    VerifyOrQuit(StringFind(emptyString, "", kStringCaseInsensitiveMatch) == &emptyString[0]);

    // Verify when sub-string has repeated patterns
    VerifyOrQuit(StringFind(testString2, "ABCABC", kStringCaseInsensitiveMatch) == &testString2[0]);
    VerifyOrQuit(StringFind(testString2, "ABCABCD", kStringCaseInsensitiveMatch) == &testString2[3]);

    printf(" -- PASS\n");
}

void TestStringStartsWith(void)
{
    printf("\nTest 7: StringStartsWith() function\n");

    VerifyOrQuit(StringStartsWith("FooBar", "Foo"));
    VerifyOrQuit(!StringStartsWith("FooBar", "Ba"));
    VerifyOrQuit(StringStartsWith("FooBar", "FooBar"));
    VerifyOrQuit(!StringStartsWith("FooBar", "FooBarr"));
    VerifyOrQuit(!StringStartsWith("FooBar", "foo"));
    VerifyOrQuit(!StringStartsWith("FooBar", "FoO"));

    VerifyOrQuit(!StringStartsWith("", "foo"));

    VerifyOrQuit(StringStartsWith("FooBar", "FOO", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringStartsWith("FooBar", "BA", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringStartsWith("FooBar", "FOOBAR", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringStartsWith("FooBar", "FooBarr", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringStartsWith("FooBar", "foO", kStringCaseInsensitiveMatch));

    VerifyOrQuit(!StringStartsWith("", "foo", kStringCaseInsensitiveMatch));

    printf(" -- PASS\n");
}

void TestStringEndsWith(void)
{
    printf("\nTest 8: StringEndsWith() function\n");

    VerifyOrQuit(StringEndsWith("FooBar", 'r'));
    VerifyOrQuit(!StringEndsWith("FooBar", 'a'));
    VerifyOrQuit(!StringEndsWith("FooBar", '\0'));
    VerifyOrQuit(StringEndsWith("a", 'a'));
    VerifyOrQuit(!StringEndsWith("a", 'b'));

    VerifyOrQuit(StringEndsWith("FooBar", "Bar"));
    VerifyOrQuit(!StringEndsWith("FooBar", "Ba"));
    VerifyOrQuit(StringEndsWith("FooBar", "FooBar"));
    VerifyOrQuit(!StringEndsWith("FooBar", "FooBarr"));

    VerifyOrQuit(!StringEndsWith("", 'a'));
    VerifyOrQuit(!StringEndsWith("", "foo"));

    VerifyOrQuit(StringEndsWith("FooBar", "baR", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringEndsWith("FooBar", "bA", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringEndsWith("FooBar", "fOOBar", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringEndsWith("FooBar", "Foobarr", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringEndsWith("", "Foo", kStringCaseInsensitiveMatch));

    printf(" -- PASS\n");
}

void TestStringMatch(void)
{
    printf("\nTest 9: StringMatch() function\n");

    VerifyOrQuit(StringMatch("", ""));
    VerifyOrQuit(StringMatch("FooBar", "FooBar"));
    VerifyOrQuit(!StringMatch("FooBar", "FooBa"));
    VerifyOrQuit(!StringMatch("FooBa", "FooBar"));
    VerifyOrQuit(!StringMatch("FooBa", "FooBar"));
    VerifyOrQuit(!StringMatch("FooBar", "fooBar"));
    VerifyOrQuit(!StringMatch("FooBaR", "FooBar"));

    VerifyOrQuit(StringMatch("", "", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch("FooBar", "fOObAR", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringMatch("FooBar", "fOObA", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringMatch("FooBa", "FooBar", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringMatch("FooBa", "FooBar", kStringCaseInsensitiveMatch));
    VerifyOrQuit(!StringMatch("Fooba", "fooBar", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch("FooBar", "FOOBAR", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch("FoobaR", "FooBar", kStringCaseInsensitiveMatch));
    VerifyOrQuit(StringMatch("FOOBAR", "foobar", kStringCaseInsensitiveMatch));

    printf(" -- PASS\n");
}

void TestStringToLowercase(void)
{
    const uint16_t kMaxSize = 100;

    const char kTestString[]      = "!@#$%^&*()_+=[].,<>//;:\"'`~ \t\r\n";
    const char kUppercaseString[] = "ABCDEFGHIJKLMNOPQRATUVWXYZABCDEFGHIJKLMNOPQRATUVWXYZ";
    const char kLowercaseString[] = "abcdefghijklmnopqratuvwxyzabcdefghijklmnopqratuvwxyz";

    char string[kMaxSize];

    printf("\nTest 10: StringConvertToLowercase() function\n");

    memcpy(string, kTestString, sizeof(kTestString));
    StringConvertToLowercase(string);
    VerifyOrQuit(memcmp(string, kTestString, sizeof(kTestString)) == 0);
    StringConvertToUppercase(string);
    VerifyOrQuit(memcmp(string, kTestString, sizeof(kTestString)) == 0);

    memcpy(string, kUppercaseString, sizeof(kUppercaseString));
    StringConvertToLowercase(string);
    VerifyOrQuit(memcmp(string, kLowercaseString, sizeof(kLowercaseString)) == 0);
    StringConvertToUppercase(string);
    VerifyOrQuit(memcmp(string, kUppercaseString, sizeof(kUppercaseString)) == 0);

    printf(" -- PASS\n");
}

void TestStringParseUint8(void)
{
    struct TestCase
    {
        const char *mString;
        Error       mError;
        uint8_t     mExpectedValue;
        uint16_t    mParsedLength;
    };

    static const TestCase kTestCases[] = {
        {"0", kErrorNone, 0, 1},
        {"1", kErrorNone, 1, 1},
        {"12", kErrorNone, 12, 2},
        {"91", kErrorNone, 91, 2},
        {"200", kErrorNone, 200, 3},
        {"00000", kErrorNone, 0, 5},
        {"00000255", kErrorNone, 255, 8},
        {"2 00", kErrorNone, 2, 1},
        {"77a12", kErrorNone, 77, 2},
        {"", kErrorParse},     // Does not start with digit char ['0'-'9']
        {"a12", kErrorParse},  // Does not start with digit char ['0'-'9']
        {" 12", kErrorParse},  // Does not start with digit char ['0'-'9']
        {"256", kErrorParse},  // Larger than max `uint8_t`
        {"1000", kErrorParse}, // Larger than max `uint8_t`
        {"0256", kErrorParse}, // Larger than max `uint8_t`
    };

    uint8_t digit;

    printf("\nTest 11: TestStringParseUint8() function\n");

    for (const TestCase &testCase : kTestCases)
    {
        const char *string = testCase.mString;
        Error       error;
        uint8_t     u8;

        error = StringParseUint8(string, u8);

        VerifyOrQuit(error == testCase.mError);

        if (testCase.mError == kErrorNone)
        {
            printf("\n%-10s -> %-3u (expect: %-3u), len:%u (expect:%u)", testCase.mString, u8, testCase.mExpectedValue,
                   static_cast<uint8_t>(string - testCase.mString), testCase.mParsedLength);

            VerifyOrQuit(u8 == testCase.mExpectedValue);
            VerifyOrQuit(string - testCase.mString == testCase.mParsedLength);
        }
        else
        {
            printf("\n%-10s -> kErrorParse", testCase.mString);
        }
    }

    for (char c = '0'; c <= '9'; c++)
    {
        VerifyOrQuit(IsDigit(c));
        VerifyOrQuit(!IsUppercase(c));
        VerifyOrQuit(!IsLowercase(c));

        SuccessOrQuit(ParseDigit(c, digit));
        VerifyOrQuit(digit == (c - '0'));

        SuccessOrQuit(ParseHexDigit(c, digit));
        VerifyOrQuit(digit == (c - '0'));
    }

    for (char c = 'A'; c <= 'Z'; c++)
    {
        VerifyOrQuit(!IsDigit(c));
        VerifyOrQuit(IsUppercase(c));
        VerifyOrQuit(!IsLowercase(c));
        VerifyOrQuit(ParseDigit(c, digit) != kErrorNone);

        if (c <= 'F')
        {
            SuccessOrQuit(ParseHexDigit(c, digit));
            VerifyOrQuit(digit == (c - 'A' + 10));
        }
        else
        {
            VerifyOrQuit(ParseHexDigit(c, digit) != kErrorNone);
        }
    }

    for (char c = 'a'; c <= 'z'; c++)
    {
        VerifyOrQuit(!IsDigit(c));
        VerifyOrQuit(!IsUppercase(c));
        VerifyOrQuit(IsLowercase(c));
        VerifyOrQuit(ParseDigit(c, digit) != kErrorNone);

        if (c <= 'f')
        {
            SuccessOrQuit(ParseHexDigit(c, digit));
            VerifyOrQuit(digit == (c - 'a' + 10));
        }
        else
        {
            VerifyOrQuit(ParseHexDigit(c, digit) != kErrorNone);
        }
    }

    VerifyOrQuit(!IsDigit(static_cast<char>('0' - 1)));
    VerifyOrQuit(!IsDigit(static_cast<char>('9' + 1)));

    VerifyOrQuit(!IsUppercase(static_cast<char>('A' - 1)));
    VerifyOrQuit(!IsUppercase(static_cast<char>('Z' + 1)));

    VerifyOrQuit(!IsLowercase(static_cast<char>('a' - 1)));
    VerifyOrQuit(!IsLowercase(static_cast<char>('z' + 1)));

    printf("\n\n -- PASS\n");
}

void TestStringCopy(void)
{
    char buffer[10];
    char smallBuffer[1];

    printf("\nTest 11: StringCopy() function\n");

    SuccessOrQuit(StringCopy(buffer, "foo", kStringCheckUtf8Encoding));
    VerifyOrQuit(StringMatch(buffer, "foo"));

    SuccessOrQuit(StringCopy(buffer, nullptr, kStringCheckUtf8Encoding));
    VerifyOrQuit(StringMatch(buffer, ""));

    SuccessOrQuit(StringCopy(buffer, "", kStringCheckUtf8Encoding));
    VerifyOrQuit(StringMatch(buffer, ""));

    SuccessOrQuit(StringCopy(buffer, "123456789", kStringCheckUtf8Encoding));
    VerifyOrQuit(StringMatch(buffer, "123456789"));

    VerifyOrQuit(StringCopy(buffer, "1234567890") == kErrorInvalidArgs);
    VerifyOrQuit(StringCopy(buffer, "1234567890abcdef") == kErrorInvalidArgs);

    SuccessOrQuit(StringCopy(smallBuffer, "", kStringCheckUtf8Encoding));
    VerifyOrQuit(StringMatch(smallBuffer, ""));

    VerifyOrQuit(StringCopy(smallBuffer, "a") == kErrorInvalidArgs);

    printf(" -- PASS\n");
}

// gcc-4 does not support constexpr function
#if __GNUC__ > 4
static_assert(ot::AreStringsInOrder("a", "b"), "AreStringsInOrder() failed");
static_assert(ot::AreStringsInOrder("aa", "aaa"), "AreStringsInOrder() failed");
static_assert(ot::AreStringsInOrder("", "a"), "AreStringsInOrder() failed");
static_assert(!ot::AreStringsInOrder("cd", "cd"), "AreStringsInOrder() failed");
static_assert(!ot::AreStringsInOrder("z", "abcd"), "AreStringsInOrder() failed");
static_assert(!ot::AreStringsInOrder("0", ""), "AreStringsInOrder() failed");
#endif

} // namespace ot

int main(void)
{
    ot::TestStringWriter();
    ot::TestStringLength();
    ot::TestUtf8();
    ot::TestStringFind();
    ot::TestStringStartsWith();
    ot::TestStringEndsWith();
    ot::TestStringMatch();
    ot::TestStringToLowercase();
    ot::TestStringParseUint8();
    ot::TestStringCopy();
    printf("\nAll tests passed.\n");
    return 0;
}
