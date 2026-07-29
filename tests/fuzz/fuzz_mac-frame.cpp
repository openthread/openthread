/*
 *  Copyright (c) 2026, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted that the following conditions are met:
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
 *  CONTRACT, STRICT LIABILITY, OR TORT ( INCLUDING NEGLIGENCE OR OTHERWISE )
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/platform/radio.h>

#include "mac_frame.h"
#include "common/code_utils.hpp"
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

    size_t RemainingBytes(void) { return mSize; }

private:
    const uint8_t *mData;
    size_t         mSize;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < sizeof(uint16_t) || size > OT_RADIO_FRAME_MAX_SIZE)
    {
        return 0;
    }

    FuzzDataProvider fdp(data, size);

    Core nexus;

    Node &node = nexus.CreateNode();

    SuccessOrQuit(node.GetInstance().SetLogLevel(kLogLevelNone));

    node.Form();
    nexus.AdvanceTime(60 * 1000);

    uint16_t psduLength = fdp.RemainingBytes();
    uint8_t *psdu       = static_cast<uint8_t *>(malloc(psduLength));

    if (psdu == nullptr)
    {
        return 0;
    }

    fdp.ConsumeData(psdu, psduLength);

    otRadioFrame rxFrame;
    otRadioFrame ackFrame;
    uint8_t      ackPsdu[OT_RADIO_FRAME_MAX_SIZE];

    memset(&rxFrame, 0, sizeof(rxFrame));
    memset(&ackFrame, 0, sizeof(ackFrame));

    rxFrame.mPsdu    = psdu;
    rxFrame.mLength  = static_cast<uint8_t>(psduLength);
    rxFrame.mChannel = node.Get<Mac::Mac>().GetPanChannel();

    ackFrame.mPsdu   = ackPsdu;
    ackFrame.mLength = 0;

    IgnoreError(otMacFrameGenerateEnhAck(&rxFrame, false, nullptr, 0, &ackFrame));

    nexus.AdvanceTime(10 * 1000);

    free(psdu);

    return 0;
}

} // namespace Nexus
} // namespace ot
