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
c *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
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
#include "common/random.hpp"

namespace ot {

template <typename UintType> void TestRandomNonCryto(const char *aName)
{
    static constexpr uint8_t kMaxIters = 6;

    Instance *instance = static_cast<Instance *>(testInitInstance());
    UintType  value;
    UintType  max = NumericLimits<UintType>::kMax;

    VerifyOrQuit(instance != nullptr);

    printf("--------------------------------------------------------------------\n\r");
    printf("TestRandomNonCryto<%s>()\r\n", aName);

    //--------------------------------------------------------------------------
    // GenerateUpToExcluding

    value = Random::NonCrypto::GenerateUpToExcluding<UintType>(0);
    VerifyOrQuit(value == 0);

    printf("\r\nGenerateUpToExcluding(100): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateUpToExcluding<UintType>(100);
        printf("%lu, ", ToUlong(value));

        VerifyOrQuit(value < 100);
    }

    printf("\r\nGenerateUpToExcluding(kMax): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateUpToExcluding<UintType>(max);
        printf("0x%lx, ", ToUlong(value));

        VerifyOrQuit(value < max);
    }

    //--------------------------------------------------------------------------
    // GenerateUpToExcluding

    value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(12, 12);
    VerifyOrQuit(value == 12);

    value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(12, 11);
    VerifyOrQuit(value == 12);

    value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(12, 0);
    VerifyOrQuit(value == 12);

    value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(max, 0);
    VerifyOrQuit(value == max);

    value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(max, max);
    VerifyOrQuit(value == max);

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(iter, iter + 1);
        VerifyOrQuit(value == iter);
    }

    printf("\r\nGenerateFromMinUpToExcluding(100, 105): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(100, 105);
        printf("%lu, ", ToUlong(value));

        VerifyOrQuit(value >= 100);
        VerifyOrQuit(value < 105);
    }

    printf("\r\nGenerateFromMinUpToExcluding(max - 2, max): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(max - 2, max);
        printf("0x%lx, ", ToUlong(value));

        VerifyOrQuit(value >= max - 2);
        VerifyOrQuit(value < max);
    }

    printf("\r\nGenerateFromMinUpToExcluding(0, max): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateFromMinUpToExcluding<UintType>(0, max);
        printf("0x%lx, ", ToUlong(value));

        VerifyOrQuit(value < max);
    }

    //--------------------------------------------------------------------------
    // GenerateInClosedRange

    value = Random::NonCrypto::GenerateInClosedRange<UintType>(101, 101);
    VerifyOrQuit(value == 101);

    value = Random::NonCrypto::GenerateInClosedRange<UintType>(101, 100);
    VerifyOrQuit(value == 101);

    value = Random::NonCrypto::GenerateInClosedRange<UintType>(101, 0);
    VerifyOrQuit(value == 101);

    value = Random::NonCrypto::GenerateInClosedRange<UintType>(max, max);
    VerifyOrQuit(value == max);

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateInClosedRange<UintType>(iter, iter);
        VerifyOrQuit(value == iter);
    }

    printf("\r\nGenerateInClosedRange(200, 201): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateInClosedRange<UintType>(200, 201);
        printf("%lu, ", ToUlong(value));

        VerifyOrQuit(value >= 200);
        VerifyOrQuit(value <= 201);
    }

    while (true)
    {
        // Make sure upper bound can be returned

        value = Random::NonCrypto::GenerateInClosedRange<UintType>(100, 101);
        VerifyOrQuit(value >= 100);
        VerifyOrQuit(value <= 101);

        if (value == 101)
        {
            break;
        }
    }

    printf("\r\nGenerateInClosedRange(0, max): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateInClosedRange<UintType>(0, max);
        printf("0x%lx, ", ToUlong(value));
    }

    printf("\r\nGenerateInClosedRange(max-1, max): ");

    for (uint8_t iter = 0; iter < kMaxIters; iter++)
    {
        value = Random::NonCrypto::GenerateInClosedRange<UintType>(max - 1, max);
        printf("0x%lx, ", ToUlong(value));

        VerifyOrQuit(value >= max - 1);
    }

    while (true)
    {
        // Make sure upper bound can be returned

        value = Random::NonCrypto::GenerateInClosedRange<UintType>(max - 1, max);
        VerifyOrQuit(value >= max - 1);

        if (value == max)
        {
            break;
        }
    }

    printf("\r\nTest passed\r\n");
}

} // namespace ot

int main(void)
{
    ot::TestRandomNonCryto<uint8_t>("uint8_t");
    ot::TestRandomNonCryto<uint16_t>("uint16_t");
    ot::TestRandomNonCryto<uint32_t>("uint32_t");

    printf("\nAll tests passed.\n");
    return 0;
}
