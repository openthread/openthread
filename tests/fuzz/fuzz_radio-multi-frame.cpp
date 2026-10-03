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
 * Delivers a sequence of radio frames to a single node, advancing simulated
 * time between them.
 *
 * The existing `radio-one-node` target calls `otPlatRadioReceiveDone()` exactly
 * once per run and then destroys the `Nexus::Core`, so any receive path that
 * needs more than one frame is out of its reach: 6LoWPAN fragment reassembly
 * (FRAG1 + FRAGN), the MLE parent/child exchanges, and CoAP block-wise
 * transfers all require a second frame to arrive while state from the first is
 * still live. This target feeds several frames in one run so those paths can be
 * exercised.
 *
 * Input layout:
 *
 *     [4]  seed for `srand()`
 *     then, repeated until the input is consumed or `kMaxFrames` is reached:
 *       [1]  PSDU length; a length of 0 or one above `OT_RADIO_FRAME_MAX_SIZE`
 *            ends the sequence
 *       [n]  PSDU bytes
 *
 * Each PSDU is copied into an exact-sized heap allocation so that a read past
 * the end of the frame is caught rather than landing in slack.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

namespace {

constexpr uint8_t  kMaxFrames       = 16;
constexpr uint32_t kInterFrameDelay = 50;    // msec, well under the reassembly timeout
constexpr uint32_t kSettleTime      = 10000; // msec

} // namespace

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

    bool ConsumeUint8(uint8_t &aUint8)
    {
        bool didRead = (mSize >= sizeof(uint8_t));

        if (didRead)
        {
            ConsumeData(&aUint8, sizeof(uint8_t));
        }

        return didRead;
    }

    // Copies `aLength` bytes into an allocation of exactly `aLength` bytes, so
    // that a read past the end of the frame is not absorbed by slack.
    uint8_t *ConsumeExactBytes(uint8_t aLength)
    {
        uint8_t *buf = nullptr;

        VerifyOrExit(aLength > 0);
        VerifyOrExit(mSize >= aLength);

        buf = static_cast<uint8_t *>(malloc(aLength));
        VerifyOrExit(buf != nullptr);

        ConsumeData(buf, aLength);

    exit:
        return buf;
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

    if (size < sizeof(seed))
    {
        return 0;
    }

    if (size > sizeof(seed) + kMaxFrames * (sizeof(uint8_t) + OT_RADIO_FRAME_MAX_SIZE))
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

    for (uint8_t frameIndex = 0; frameIndex < kMaxFrames; frameIndex++)
    {
        otRadioFrame frame;
        uint8_t      psduLength;

        if (!fdp.ConsumeUint8(psduLength))
        {
            break;
        }

        if ((psduLength == 0) || (psduLength > OT_RADIO_FRAME_MAX_SIZE))
        {
            break;
        }

        memset(&frame, 0, sizeof(frame));

        frame.mPsdu = fdp.ConsumeExactBytes(psduLength);

        if (frame.mPsdu == nullptr)
        {
            break;
        }

        frame.mLength                              = psduLength;
        frame.mChannel                             = node.Get<Mac::Mac>().GetPanChannel();
        frame.mInfo.mRxInfo.mRssi                  = -20;
        frame.mInfo.mRxInfo.mLqi                   = OT_RADIO_LQI_NONE;
        frame.mInfo.mRxInfo.mAckedWithFramePending = true;

        Log("Frame %u, %u bytes", frameIndex, psduLength);

        otPlatRadioReceiveDone(&node.GetInstance(), &frame, OT_ERROR_NONE);

        free(frame.mPsdu);

        nexus.AdvanceTime(kInterFrameDelay);
    }

    nexus.AdvanceTime(kSettleTime);

    return 0;
}

} // namespace Nexus
} // namespace ot
