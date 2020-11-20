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
    printf("\t%s = [%d] \"%s\"\n", aName, aString.GetLength(), aString.AsCString());
}

void TestString(void)
{
    otError             error;
    String<kStringSize> str1;
    String<kStringSize> str2("abc");
    String<kStringSize> str3("%d", 12);

    printf("\nTest 1: String constructor\n");

    VerifyOrQuit(str1.GetSize() == kStringSize, "GetSize() failed");

    VerifyOrQuit(str1.GetLength() == 0, "GetLength() failed for empty string");
    VerifyOrQuit(str2.GetLength() == 3, "GetLength() failed");
    VerifyOrQuit(str3.GetLength() == 2, "GetLength() failed");

    VerifyOrQuit(strcmp(str1.AsCString(), "") == 0, "String content is incorrect");
    VerifyOrQuit(strcmp(str2.AsCString(), "abc") == 0, "String content is incorrect");
    VerifyOrQuit(strcmp(str3.AsCString(), "12") == 0, "String content is incorrect");

    PrintString("str1", str1);
    PrintString("str2", str2);
    PrintString("str3", str3);

    printf(" -- PASS\n");

    printf("\nTest 2: String::Set() and String::Clear() method\n");

    error = str1.Set("Hello");
    SuccessOrQuit(error, "String::Set() failed unexpectedly");
    VerifyOrQuit(str1.GetLength() == 5, "GetLength() failed for empty string");
    VerifyOrQuit(strcmp(str1.AsCString(), "Hello") == 0, "String content is incorrect");
    PrintString("str1", str1);

    str1.Clear();
    VerifyOrQuit(str1.GetLength() == 0, "GetLength() failed for empty string");
    VerifyOrQuit(strcmp(str1.AsCString(), "") == 0, "String content is incorrect");

    IgnoreError(str1.Set("%d", 12));
    VerifyOrQuit(str1.GetLength() == 2, "GetLength() failed");
    VerifyOrQuit(strcmp(str1.AsCString(), "12") == 0, "String content is incorrect");
    PrintString("str1", str1);

    error = str1.Set("abcdefghijklmnopqratuvwxyzabcdefghijklmnopqratuvwxyz");
    VerifyOrQuit(error == OT_ERROR_NO_BUFS, "String::Set() did not handle overflow buffer correctly");
    PrintString("str1", str1);

    printf("\nTest 3: String::Append() method\n");

    str2.Clear();
    VerifyOrQuit(str2.GetLength() == 0, "GetLength() failed for empty string");
    VerifyOrQuit(strcmp(str2.AsCString(), "") == 0, "String content is incorrect");

    error = str2.Append("Hi");
    SuccessOrQuit(error, "String::Append() failed unexpectedly");
    VerifyOrQuit(str2.GetLength() == 2, "GetLength() failed");
    VerifyOrQuit(strcmp(str2.AsCString(), "Hi") == 0, "String content is incorrect");
    PrintString("str2", str2);

    error = str2.Append("%s%d", "!", 12);
    SuccessOrQuit(error, "String::Append() failed unexpectedly");
    VerifyOrQuit(str2.GetLength() == 5, "GetLength() failed");
    VerifyOrQuit(strcmp(str2.AsCString(), "Hi!12") == 0, "String content is incorrect");
    PrintString("str2", str2);

    error = str2.Append("abcdefghijklmnopqratuvwxyzabcdefghijklmnopqratuvwxyz");
    VerifyOrQuit(error == OT_ERROR_NO_BUFS, "String::Append() did not handle overflow buffer correctly");
    PrintString("str2", str2);

    printf(" -- PASS\n");
}

void TestStringLength(void)
{
    char string_a[5] = "\0foo";
    char string_b[8] = "foo\0bar";

    printf("\nTest 4: String::StringLength() method\n");

    VerifyOrQuit(StringLength(string_a, 0) == 0, "StringLength() 0len 0 fails");
    VerifyOrQuit(StringLength(string_a, 1) == 0, "StringLength() 0len 1 fails");
    VerifyOrQuit(StringLength(string_a, 2) == 0, "StringLength() 0len 2 fails");

    VerifyOrQuit(StringLength(string_b, 0) == 0, "StringLength() 3len 0 fails");
    VerifyOrQuit(StringLength(string_b, 1) == 1, "StringLength() 3len 1 fails");
    VerifyOrQuit(StringLength(string_b, 2) == 2, "StringLength() 3len 2 fails");
    VerifyOrQuit(StringLength(string_b, 3) == 3, "StringLength() 3len 3 fails");
    VerifyOrQuit(StringLength(string_b, 4) == 3, "StringLength() 3len 4 fails");
    VerifyOrQuit(StringLength(string_b, 5) == 3, "StringLength() 3len 5 fails");
    VerifyOrQuit(StringLength(string_b, 6) == 3, "StringLength() 3len 6 fails");

    printf(" -- PASS\n");
}

void TestUtf8(void)
{
    printf("\nTest 5: IsValidUtf8String() function\n");

    VerifyOrQuit(IsValidUtf8String("An ASCII string"), "IsValidUtf8String() ASCII string fails");
    VerifyOrQuit(IsValidUtf8String(u8"Строка UTF-8"), "IsValidUtf8String() UTF-8 string fails");
    VerifyOrQuit(!IsValidUtf8String("\xbf"), "IsValidUtf8String() illegal string fails");
    VerifyOrQuit(!IsValidUtf8String("\xdf"), "IsValidUtf8String() illegal string fails");
    VerifyOrQuit(!IsValidUtf8String("\xef\x80"), "IsValidUtf8String() illegal string fails");
    VerifyOrQuit(!IsValidUtf8String("\xf7\x80\x80"), "IsValidUtf8String() illegal string fails");
    VerifyOrQuit(!IsValidUtf8String("\xff"), "IsValidUtf8String() illegal string fails");

    printf(" -- PASS\n");
}

} // namespace ot

int main(void)
{
    ot::TestString();
    ot::TestStringLength();
    ot::TestUtf8();
    printf("\nAll tests passed.\n");
    return 0;
}
