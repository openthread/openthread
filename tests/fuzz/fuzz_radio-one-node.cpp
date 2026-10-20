/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

class FuzzDataProvider
{
public:
    FuzzDataProvider(const uint8_t *aData, size_t aSize)
        : mData(aData)
        , mSize(aSize)
    {
    }

    void ConsumeData(void *aBuf, size_t aLength)
    {
        assert(aLength <= mSize);
        memcpy(aBuf, mData, aLength);
        mData += aLength;
        mSize -= aLength;
    }

    uint8_t *ConsumeBytes(size_t aLength)
    {
        assert(aLength <= mSize);

        uint8_t *buf = nullptr;

        if (aLength > 0)
        {
            buf = static_cast<uint8_t *>(malloc(aLength));
            memcpy(buf, mData, aLength);
            mData += aLength;
            mSize -= aLength;
        }

        return buf;
    }

    template <typename IntType> IntType ConsumeIntegralInRange(IntType aMin, IntType aMax)
    {
        assert(aMin <= aMax);

        uint64_t range = static_cast<uint64_t>(aMax) - aMin;
        IntType  result;

        ConsumeData(&result, sizeof(result));

        return (range == 0) ? aMin : static_cast<IntType>(aMin + (result % (range + 1)));
    }

    size_t RemainingBytes(void) { return mSize; }

private:
    const uint8_t *mData;
    size_t         mSize;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxIterations  = 8;
    const uint32_t kMaxAdvanceTime = 10000;

    FuzzDataProvider fdp(data, size);

    unsigned int seed;
    otRadioFrame frame;
    otError      error;
    uint32_t     advanceTime;

    if (size < sizeof(seed) + sizeof(error) + sizeof(frame) + sizeof(advanceTime) + sizeof(frame.mLength))
    {
        return 0;
    }

    if (size > sizeof(seed) + kMaxIterations * (sizeof(error) + sizeof(frame) + sizeof(advanceTime) +
                                                sizeof(frame.mLength) + OT_RADIO_FRAME_MAX_SIZE))
    {
        return 0;
    }

    fdp.ConsumeData(&seed, sizeof(seed));
    srand(seed);

    Core nexus;

    Node &node = nexus.CreateNode();

    SuccessOrQuit(node.GetInstance().SetLogLevel(kLogLevelInfo));

    node.GetInstance().Get<BorderRouter::InfraIf>().Init(/* aInfraIfIndex */ 1, /* aInfraIfIsRunning */ true);
    SuccessOrQuit(node.GetInstance().Get<BorderRouter::RoutingManager>().SetEnabled(true));
    node.GetInstance().Get<Srp::Server>().SetAutoEnableMode(true);
    node.GetInstance().Get<BorderRouter::RoutingManager>().SetDhcp6PdEnabled(true);
    node.GetInstance().Get<BorderRouter::RoutingManager>().SetNat64PrefixManagerEnabled(true);
    node.GetInstance().Get<Nat64::Translator>().SetEnabled(true);

    Log("---------------------------------------------------------------------------------------");
    Log("Form network");

    node.Form();
    nexus.AdvanceTime(60 * 1000);
    VerifyOrQuit(node.Get<Mle::Mle>().IsLeader());
    VerifyOrQuit(node.Get<Srp::Server>().GetState() == Srp::Server::kStateRunning);

    Log("---------------------------------------------------------------------------------------");
    Log("Fuzz");

    for (uint16_t i = 0; i < kMaxIterations; i++)
    {
        if (fdp.RemainingBytes() < sizeof(error) + sizeof(frame) + sizeof(advanceTime) + sizeof(frame.mLength))
        {
            break;
        }

        error = static_cast<otError>(fdp.ConsumeIntegralInRange<uint8_t>(OT_ERROR_NONE, OT_NUM_ERRORS - 1));
        fdp.ConsumeData(&frame, sizeof(frame));
        advanceTime   = fdp.ConsumeIntegralInRange(0u, kMaxAdvanceTime);
        frame.mLength = fdp.ConsumeIntegralInRange<uint16_t>(
            0, Min<size_t>(fdp.RemainingBytes() - sizeof(frame.mLength), OT_RADIO_FRAME_MAX_SIZE));
        frame.mPsdu = fdp.ConsumeBytes(frame.mLength);

        otPlatRadioReceiveDone(&node.GetInstance(), &frame, error);

        nexus.AdvanceTime(advanceTime);

        free(frame.mPsdu);
    }

    nexus.AdvanceTime(10 * 1000);

    return 0;
}

} // namespace Nexus
} // namespace ot
