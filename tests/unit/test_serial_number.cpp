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
#include "common/numeric_limits.hpp"
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

} // namespace ot

int main(void)
{
    ot::TestSerialNumber<uint8_t>("uint8_t");
    ot::TestSerialNumber<uint16_t>("uint16_t");
    ot::TestSerialNumber<uint32_t>("uint32_t");
    ot::TestSerialNumber<uint64_t>("uint64_t");
    printf("\nAll tests passed.\n");
    return 0;
}
