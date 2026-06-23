/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
#include "test_util.h"

#include <openthread/config.h>

#include "thread/vendor_info.hpp"

namespace ot {

class UnitTester
{
public:
    template <uint64_t kConfig>
    static void TestParserCase(bool aIsSpecified, bool aIsValid, uint8_t aBitLength, const uint8_t aExpectedBytes[5])
    {
        using Parser = VendorInfo::OuiParser<kConfig>;

        VerifyOrQuit(Parser::IsSpecified() == aIsSpecified, "IsSpecified() failed");
        VerifyOrQuit(Parser::IsValid() == aIsValid, "IsValid() failed");

        if (aIsValid && aIsSpecified)
        {
            VerifyOrQuit(Parser::GetBitLength() == aBitLength, "GetBitLength() failed");

            VerifyOrQuit(Parser::GetByte0() == aExpectedBytes[0]);
            VerifyOrQuit(Parser::GetByte1() == aExpectedBytes[1]);
            VerifyOrQuit(Parser::GetByte2() == aExpectedBytes[2]);
            VerifyOrQuit(Parser::GetByte3() == aExpectedBytes[3]);
            VerifyOrQuit(Parser::GetByte4() == aExpectedBytes[4]);
        }
    }

    static void TestCompileTimeOuiParser(void)
    {
        printf("TestCompileTimeOuiParser\n");

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 1. Unspecified cases
        {
            uint8_t expected[5] = {0};
            TestParserCase<0xffffffffULL>(/* aIsSpecified */ false, /* aIsValid */ true, 0, expected);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 2. Implicit 24-bit (legacy OUI format)
        {
            uint8_t expected[5] = {0x64, 0x16, 0x66, 0, 0};
            TestParserCase<0x641666>(/* aIsSpecified */ true, /* aIsValid */ true, 24, expected);
        }

        {
            uint8_t expected[5] = {0x00, 0x00, 0x01, 0, 0};
            TestParserCase<0x000001>(/* aIsSpecified */ true, /* aIsValid */ true, 24, expected);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 3. Explicit 24-bit OUI (length in bits 40-47, bytes in bits 0-39)
        {
            uint8_t expected[5] = {0x00, 0x1a, 0x2b, 0, 0};
            TestParserCase<0x18001a2b0000ULL>(/* aIsSpecified */ true, /* aIsValid */ true, 24, expected);
        }

        // Invalid: non-zero in byte index 3
        {
            uint8_t expected[5] = {0};
            TestParserCase<0x18001a2b0300ULL>(/* aIsSpecified */ true, /* aIsValid */ false, 24, expected);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 4. Explicit 28-bit OUI
        {
            uint8_t expected[5] = {0x00, 0x1a, 0x2b, 0x30, 0};
            TestParserCase<0x1c001a2b3000ULL>(/* aIsSpecified */ true, /* aIsValid */ true, 28, expected);
        }

        // Invalid: non-zero in byte index 4
        {
            uint8_t expected[5] = {0};
            TestParserCase<0x1c001a2b3005ULL>(/* aIsSpecified */ true, /* aIsValid */ false, 28, expected);
        }

        // Invalid: non-zero in lower nibble of byte index 3
        {
            uint8_t expected[5] = {0};
            TestParserCase<0x1c001a2b3500ULL>(/* aIsSpecified */ true, /* aIsValid */ false, 28, expected);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 5. Explicit 36-bit OUI
        {
            uint8_t expected[5] = {0x00, 0x1a, 0x2b, 0x3c, 0x40};
            TestParserCase<0x24001a2b3c40ULL>(/* aIsSpecified */ true, /* aIsValid */ true, 36, expected);
        }
        // Invalid: non-zero in lower nibble of byte index 4
        {
            uint8_t expected[5] = {0};
            TestParserCase<0x24001a2b3c43ULL>(/* aIsSpecified */ true, /* aIsValid */ false, 36, expected);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 6. Invalid Bit Length (e.g. 32 bits)
        {
            uint8_t expected[5] = {0};
            TestParserCase<0x20001a2b3c40ULL>(/* aIsSpecified */ true, /* aIsValid */ false, 32, expected);
        }

        //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 7. Value out of valid range (exceeds 6 bytes)
        {
            uint8_t expected[5] = {0};
            TestParserCase<0x1c11001a2b3000ULL>(/* aIsSpecified */ true, /* aIsValid */ false, 28, expected);
        }
    }

    static void TestOuiClass(void)
    {
        VendorInfo::Oui oui;
        VendorInfo::Oui oui1;
        VendorInfo::Oui oui2;
        VendorInfo::Oui oui3;
        uint8_t         bytes24[] = {0x64, 0x16, 0x66};
        uint8_t         bytes28[] = {0x00, 0x1a, 0x2b, 0x35};
        uint8_t         bytes36[] = {0x00, 0x1a, 0x2b, 0x3c, 0x42};

        printf("TestOuiClass\n");

        // Test ToString() on unspecified OUI
        oui.Clear();
        VerifyOrQuit(StringMatch(oui.ToString().AsCString(), "unspecified"));

        // Test `SetFrom()` with bytes (24-bit)
        SuccessOrQuit(oui.SetFrom(bytes24, sizeof(bytes24)));
        VerifyOrQuit(oui.IsValid());
        VerifyOrQuit(oui.GetBitLength() == 24);
        VerifyOrQuit(oui.GetSize() == 3);
        VerifyOrQuit(oui.GetAsOui24() == 0x641666);
        VerifyOrQuit(StringMatch(oui.ToString().AsCString(), "64-16-66"));

        // Test `SetFrom()` with non-tidy 28-bit
        SuccessOrQuit(oui.SetFrom(bytes28, sizeof(bytes28)));
        VerifyOrQuit(oui.IsValid());
        VerifyOrQuit(oui.GetBitLength() == 28);
        VerifyOrQuit(oui.GetSize() == 4);
        VerifyOrQuit(oui.GetBytes()[3] == 0x30);
        VerifyOrQuit(StringMatch(oui.ToString().AsCString(), "00-1A-2B-3"));

        // Test `SetFrom()` with non-tidy 36-bit
        SuccessOrQuit(oui.SetFrom(bytes36, sizeof(bytes36)));
        VerifyOrQuit(oui.IsValid());
        VerifyOrQuit(oui.GetBitLength() == 36);
        VerifyOrQuit(oui.GetSize() == 5);
        VerifyOrQuit(oui.GetBytes()[4] == 0x40);
        VerifyOrQuit(StringMatch(oui.ToString().AsCString(), "00-1A-2B-3C-4"));

        // Test `operator==` (with tidy check on right-hand side)
        SuccessOrQuit(oui1.SetFrom(bytes28, sizeof(bytes28))); // will be tidied

        oui2.Clear();
        oui2.mBitLength = 28;
        memcpy(oui2.mBytes, bytes28, sizeof(bytes28)); // manually write untidied bytes

        VerifyOrQuit(oui1 == oui2);

        // Test `SetFrom(const Oui &)` tidying
        oui3.SetFrom(oui2);
        VerifyOrQuit(oui3.GetBytes()[3] == 0x30, "SetFrom(const Oui&) failed to tidy");
    }
};

} // namespace ot

int main(void)
{
    ot::UnitTester::TestCompileTimeOuiParser();
    ot::UnitTester::TestOuiClass();

    printf("All tests passed\n");
    return 0;
}
