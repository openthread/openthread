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

/*
 * Multi-frame radio fuzzer.
 *
 * The existing `radio-one-node` fuzzer delivers exactly ONE 802.15.4 frame per
 * test case. Any parser whose state is built across several frames is therefore
 * structurally unreachable by it, including:
 *
 *   - 6LoWPAN fragment reassembly (`MeshForwarder` / `Lowpan` reassembly list),
 *     which by definition needs a FRAG1 frame followed by one or more FRAGN.
 *   - CoAP block-wise transfer reassembly.
 *   - Any MLE / TMF exchange whose second message is parsed only after the
 *     first has installed state.
 *
 * This harness delivers a SEQUENCE of frames, with attacker-chosen inter-frame
 * delays, to a single node that has formed a network and is the Leader.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

// Upper bound on frames per test case; keeps a single execution bounded so the
// fuzzer does not report timeouts for merely-long inputs.
static constexpr uint32_t kMaxFrames = 24;

class FuzzDataProvider
{
public:
    FuzzDataProvider(const uint8_t *aData, size_t aSize)
        : mData(aData)
        , mSize(aSize)
    {
    }

    bool ConsumeData(void *aBuf, size_t aLength)
    {
        if (aLength > mSize)
        {
            return false;
        }

        memcpy(aBuf, mData, aLength);
        mData += aLength;
        mSize -= aLength;

        return true;
    }

    uint8_t ConsumeByte(void)
    {
        uint8_t result = 0;

        IgnoreError(ConsumeData(&result, sizeof(result)) ? kErrorNone : kErrorParse);

        return result;
    }

    const uint8_t *Peek(void) const { return mData; }

    void Skip(size_t aLength)
    {
        aLength = (aLength > mSize) ? mSize : aLength;
        mData += aLength;
        mSize -= aLength;
    }

    size_t RemainingBytes(void) const { return mSize; }

private:
    const uint8_t *mData;
    size_t         mSize;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzDataProvider fdp(data, size);

    unsigned int seed;

    // Header: 4-byte seed. Then a stream of records:
    //     u8 length   (0 .. OT_RADIO_FRAME_MAX_SIZE, values above are clamped)
    //     u8 delayMs  (inter-frame delay, in units of 4 ms)
    //     u8 psdu[length]

    if (size < sizeof(seed) + 2)
    {
        return 0;
    }

    if (size > sizeof(seed) + kMaxFrames * (2 + OT_RADIO_FRAME_MAX_SIZE))
    {
        return 0;
    }

    if (!fdp.ConsumeData(&seed, sizeof(seed)))
    {
        return 0;
    }

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

    for (uint32_t frameNum = 0; (frameNum < kMaxFrames) && (fdp.RemainingBytes() >= 2); frameNum++)
    {
        otRadioFrame frame;
        uint8_t      length;
        uint8_t      delay;
        uint8_t     *psdu = nullptr;

        length = fdp.ConsumeByte();
        delay  = fdp.ConsumeByte();

        if (length > OT_RADIO_FRAME_MAX_SIZE)
        {
            length = static_cast<uint8_t>(length % (OT_RADIO_FRAME_MAX_SIZE + 1));
        }

        if (length > fdp.RemainingBytes())
        {
            length = static_cast<uint8_t>(fdp.RemainingBytes());
        }

        memset(&frame, 0, sizeof(frame));

        if (length > 0)
        {
            psdu = static_cast<uint8_t *>(malloc(length));
            VerifyOrQuit(psdu != nullptr);
            memcpy(psdu, fdp.Peek(), length);
            fdp.Skip(length);
        }

        frame.mPsdu                                = psdu;
        frame.mLength                              = length;
        frame.mChannel                             = 11;
        frame.mInfo.mRxInfo.mRssi                  = -20;
        frame.mInfo.mRxInfo.mLqi                   = OT_RADIO_LQI_NONE;
        frame.mInfo.mRxInfo.mTimestamp             = nexus.GetNowMicro64();
        frame.mInfo.mRxInfo.mAckedWithFramePending = false;

        Log("--- frame %u: len=%u delay=%u ---", frameNum, length, delay);

        otPlatRadioReceiveDone(&node.GetInstance(), &frame, OT_ERROR_NONE);

        // Inter-frame delay. Bounded so a test case cannot advance simulated
        // time far enough to be slow, but large enough to cross reassembly and
        // retransmission timers.
        nexus.AdvanceTime(static_cast<uint32_t>(delay) * 4);

        if (psdu != nullptr)
        {
            free(psdu);
        }
    }

    nexus.AdvanceTime(10 * 1000);

exit:
    return 0;
}

} // namespace Nexus
} // namespace ot
