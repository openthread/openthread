/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "common/preference.hpp"
#include "common/serial_number.hpp"

namespace ot {

template <typename UintType> void TestSerialNumber(const char *aName)
{
    static constexpr UintType kMax = NumericLimits<UintType>::kMax;
    static constexpr UintType kMid = kMax / 2;

    static const UintType kNumbers[] = {0, 1, 20, kMid - 1, kMid, kMid + 1, kMax - 20, kMax - 1, kMax};

    for (UintType number : kNumbers)
    {
        VerifyOrQuit(!SerialNumber::IsGreater<UintType>(number, number));
        VerifyOrQuit(!SerialNumber::IsLess<UintType>(number, number));

        VerifyOrQuit(SerialNumber::IsGreater<UintType>(number + 1, number));
        VerifyOrQuit(SerialNumber::IsGreater<UintType>(number + kMid - 1, number));
        VerifyOrQuit(SerialNumber::IsGreater<UintType>(number + kMid, number));
        VerifyOrQuit(!SerialNumber::IsGreater<UintType>(number + kMid + 2, number));
        VerifyOrQuit(!SerialNumber::IsGreater<UintType>(number + kMax - 1, number));

        VerifyOrQuit(SerialNumber::IsLess<UintType>(number - 1, number));
        VerifyOrQuit(SerialNumber::IsLess<UintType>(number - kMid + 1, number));
        VerifyOrQuit(SerialNumber::IsLess<UintType>(number - kMid, number));
        VerifyOrQuit(!SerialNumber::IsLess<UintType>(number - kMid - 2, number));
        VerifyOrQuit(!SerialNumber::IsLess<UintType>(number - kMax + 1, number));
    }

    printf("TestSerialNumber<%s>() passed\n", aName);
}

void TestNumUtils(void)
{
    uint16_t u16;
    uint32_t u32;

    VerifyOrQuit(Min<uint8_t>(1, 2) == 1);
    VerifyOrQuit(Min<uint8_t>(2, 1) == 1);
    VerifyOrQuit(Min<uint8_t>(1, 1) == 1);

    VerifyOrQuit(Max<uint8_t>(1, 2) == 2);
    VerifyOrQuit(Max<uint8_t>(2, 1) == 2);
    VerifyOrQuit(Max<uint8_t>(1, 1) == 1);

    VerifyOrQuit(Clamp<uint8_t>(1, 5, 10) == 5);
    VerifyOrQuit(Clamp<uint8_t>(5, 5, 10) == 5);
    VerifyOrQuit(Clamp<uint8_t>(7, 5, 10) == 7);
    VerifyOrQuit(Clamp<uint8_t>(10, 5, 10) == 10);
    VerifyOrQuit(Clamp<uint8_t>(12, 5, 10) == 10);

    VerifyOrQuit(Clamp<uint8_t>(10, 10, 10) == 10);
    VerifyOrQuit(Clamp<uint8_t>(9, 10, 10) == 10);
    VerifyOrQuit(Clamp<uint8_t>(11, 10, 10) == 10);

    u16 = 100;
    VerifyOrQuit(ClampToUint8(u16) == 100);
    u16 = 255;
    VerifyOrQuit(ClampToUint8(u16) == 255);
    u16 = 256;
    VerifyOrQuit(ClampToUint8(u16) == 255);
    u16 = 400;
    VerifyOrQuit(ClampToUint8(u16) == 255);

    u32 = 100;
    VerifyOrQuit(ClampToUint16(u32) == 100);
    u32 = 256;
    VerifyOrQuit(ClampToUint16(u32) == 256);
    u32 = 0xffff;
    VerifyOrQuit(ClampToUint16(u32) == 0xffff);
    u32 = 0x10000;
    VerifyOrQuit(ClampToUint16(u32) == 0xffff);
    u32 = 0xfff0000;
    VerifyOrQuit(ClampToUint16(u32) == 0xffff);

    VerifyOrQuit(ThreeWayCompare<uint8_t>(2, 2) == 0);
    VerifyOrQuit(ThreeWayCompare<uint8_t>(2, 1) > 0);
    VerifyOrQuit(ThreeWayCompare<uint8_t>(1, 2) < 0);

    VerifyOrQuit(ThreeWayCompare<bool>(false, false) == 0);
    VerifyOrQuit(ThreeWayCompare<bool>(true, true) == 0);
    VerifyOrQuit(ThreeWayCompare<bool>(true, false) > 0);
    VerifyOrQuit(ThreeWayCompare<bool>(false, true) < 0);

    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(2, 1) == 2);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(1, 3) == 0);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(1, 2) == 1);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(2, 3) == 1);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(3, 2) == 2);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(4, 2) == 2);

    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(0, 10) == 0);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(4, 10) == 0);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(5, 10) == 1);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(9, 10) == 1);
    VerifyOrQuit(DivideAndRoundToClosest<uint8_t>(10, 10) == 1);

    VerifyOrQuit(CountBitsInMask<uint8_t>(0) == 0);
    VerifyOrQuit(CountBitsInMask<uint8_t>(1) == 1);
    VerifyOrQuit(CountBitsInMask<uint8_t>(2) == 1);
    VerifyOrQuit(CountBitsInMask<uint8_t>(3) == 2);
    VerifyOrQuit(CountBitsInMask<uint8_t>(4) == 1);
    VerifyOrQuit(CountBitsInMask<uint8_t>(7) == 3);
    VerifyOrQuit(CountBitsInMask<uint8_t>(11) == 3);
    VerifyOrQuit(CountBitsInMask<uint8_t>(15) == 4);
    VerifyOrQuit(CountBitsInMask<uint8_t>(0x11) == 2);
    VerifyOrQuit(CountBitsInMask<uint8_t>(0xef) == 7);
    VerifyOrQuit(CountBitsInMask<uint8_t>(0xff) == 8);

    VerifyOrQuit(CountBitsInMask<uint16_t>(0) == 0);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xff00) == 8);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xff) == 8);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xaa55) == 8);
    VerifyOrQuit(CountBitsInMask<uint16_t>(0xffff) == 16);

    printf("TestNumUtils() passed\n");
}

void TestPreference(void)
{
    VerifyOrQuit(Preference::kHigh == 1);
    VerifyOrQuit(Preference::kMedium == 0);
    VerifyOrQuit(Preference::kLow == -1);

    // To2BitUint()
    VerifyOrQuit(Preference::To2BitUint(Preference::kHigh) == 0x1);
    VerifyOrQuit(Preference::To2BitUint(Preference::kMedium) == 0x0);
    VerifyOrQuit(Preference::To2BitUint(Preference::kLow) == 0x3);
    VerifyOrQuit(Preference::To2BitUint(2) == 0x1);
    VerifyOrQuit(Preference::To2BitUint(-2) == 0x3);
    VerifyOrQuit(Preference::To2BitUint(127) == 0x1);
    VerifyOrQuit(Preference::To2BitUint(-128) == 0x3);

    // From2BitUint()
    VerifyOrQuit(Preference::From2BitUint(0x1) == Preference::kHigh);
    VerifyOrQuit(Preference::From2BitUint(0x0) == Preference::kMedium);
    VerifyOrQuit(Preference::From2BitUint(0x3) == Preference::kLow);
    VerifyOrQuit(Preference::From2BitUint(0x2) == Preference::kMedium);

    VerifyOrQuit(Preference::From2BitUint(0x1 | 4) == Preference::kHigh);
    VerifyOrQuit(Preference::From2BitUint(0x0 | 4) == Preference::kMedium);
    VerifyOrQuit(Preference::From2BitUint(0x3 | 4) == Preference::kLow);
    VerifyOrQuit(Preference::From2BitUint(0x2 | 4) == Preference::kMedium);

    VerifyOrQuit(Preference::From2BitUint(0x1 | 0xfc) == Preference::kHigh);
    VerifyOrQuit(Preference::From2BitUint(0x0 | 0xfc) == Preference::kMedium);
    VerifyOrQuit(Preference::From2BitUint(0x3 | 0xfc) == Preference::kLow);
    VerifyOrQuit(Preference::From2BitUint(0x2 | 0xfc) == Preference::kMedium);

    // IsValid()
    VerifyOrQuit(Preference::IsValid(Preference::kHigh));
    VerifyOrQuit(Preference::IsValid(Preference::kMedium));
    VerifyOrQuit(Preference::IsValid(Preference::kLow));

    VerifyOrQuit(!Preference::IsValid(2));
    VerifyOrQuit(!Preference::IsValid(-2));
    VerifyOrQuit(!Preference::IsValid(127));
    VerifyOrQuit(!Preference::IsValid(-128));

    // Is2BitUintValid
    VerifyOrQuit(Preference::Is2BitUintValid(0x1));
    VerifyOrQuit(Preference::Is2BitUintValid(0x0));
    VerifyOrQuit(Preference::Is2BitUintValid(0x3));
    VerifyOrQuit(!Preference::Is2BitUintValid(0x2));

    VerifyOrQuit(Preference::Is2BitUintValid(0x1 | 4));
    VerifyOrQuit(Preference::Is2BitUintValid(0x0 | 4));
    VerifyOrQuit(Preference::Is2BitUintValid(0x3 | 4));
    VerifyOrQuit(!Preference::Is2BitUintValid(0x2 | 4));

    VerifyOrQuit(Preference::Is2BitUintValid(0x1 | 0xfc));
    VerifyOrQuit(Preference::Is2BitUintValid(0x0 | 0xfc));
    VerifyOrQuit(Preference::Is2BitUintValid(0x3 | 0xfc));
    VerifyOrQuit(!Preference::Is2BitUintValid(0x2 | 0xfc));

    printf("TestPreference() passed\n");
}

} // namespace ot

int main(void)
{
    ot::TestSerialNumber<uint8_t>("uint8_t");
    ot::TestSerialNumber<uint16_t>("uint16_t");
    ot::TestSerialNumber<uint32_t>("uint32_t");
    ot::TestSerialNumber<uint64_t>("uint64_t");
    ot::TestNumUtils();
    ot::TestPreference();
    printf("\nAll tests passed.\n");
    return 0;
}
