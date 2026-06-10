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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

#include "common/message.hpp"
#include "net/ip6.hpp"

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
        if (aLength > mSize)
        {
            aLength = mSize;
        }

        memcpy(aBuf, mData, aLength);
        mData += aLength;
        mSize -= aLength;
    }

    uint8_t ConsumeUint8(void)
    {
        uint8_t val = 0;
        ConsumeData(&val, sizeof(val));
        return val;
    }

    uint16_t ConsumeUint16(void)
    {
        uint16_t val = 0;
        ConsumeData(&val, sizeof(val));
        return val;
    }

    uint32_t ConsumeUint32(void)
    {
        uint32_t val = 0;
        ConsumeData(&val, sizeof(val));
        return val;
    }

    size_t RemainingBytes(void) { return mSize; }

    // aSize is passed by reference so the caller receives the actual number of
    // bytes consumed, which may be less than requested when the buffer runs out.
    const uint8_t *ConsumeBytes(size_t &aSize)
    {
        if (aSize > mSize)
        {
            aSize = mSize;
        }

        const uint8_t *data = mData;
        mData += aSize;
        mSize -= aSize;
        return data;
    }

private:
    const uint8_t *mData;
    size_t         mSize;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzDataProvider fdp(data, size);

    if (fdp.RemainingBytes() < 16)
    {
        return 0;
    }

    Core  nexus;
    Node &node = nexus.CreateNode();

#if !OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
#error "OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE must be enabled for this fuzzer"
#endif

    node.GetInstance().SetLogLevel(kLogLevelNone);
    node.Form();
    nexus.AdvanceTime(60 * 1000);

    // Poison the MessagePool to help detect leaks of stale data
    for (int i = 0; i < 20; i++)
    {
        otMessage *msg = otIp6NewMessage(&node.GetInstance(), nullptr);

        if (msg != nullptr)
        {
            uint8_t poison[512];
            memset(poison, 0xA5, sizeof(poison));
            IgnoreError(otMessageAppend(msg, poison, sizeof(poison)));
            otMessageFree(msg);
        }
    }

    // Fuzz multiple fragments to test reassembly logic
    while (fdp.RemainingBytes() > 12)
    {
        uint32_t ident      = fdp.ConsumeUint32();
        uint16_t offset     = fdp.ConsumeUint16() & 0x1fff;
        bool     more       = fdp.ConsumeUint8() & 1;
        uint8_t  payloadLen = fdp.ConsumeUint8() % 128;

        // actualPayloadLen may be smaller than payloadLen if the fuzzer input
        // runs out of data - ConsumeBytes updates it to reflect actual bytes given.
        size_t         actualPayloadLen = static_cast<size_t>(payloadLen);
        const uint8_t *payload          = fdp.ConsumeBytes(actualPayloadLen);

        otMessage *message = otIp6NewMessage(&node.GetInstance(), nullptr);

        if (message == nullptr)
        {
            break;
        }

        Ip6::Header header;
        header.Clear();
        header.InitVersionTrafficClassFlow();
        header.SetPayloadLength(sizeof(Ip6::FragmentHeader) + static_cast<uint16_t>(actualPayloadLen));
        header.SetNextHeader(Ip6::kProtoFragment);
        header.GetSource().SetToLinkLocalAddress(node.Get<Mac::Mac>().GetExtAddress());
        header.GetDestination() = node.Get<Mle::Mle>().GetMeshLocalEid();

        Ip6::FragmentHeader fragHeader;
        fragHeader.Init();
        fragHeader.SetNextHeader(Ip6::kProtoUdp);
        fragHeader.SetOffset(offset);

        if (more)
        {
            fragHeader.SetMoreFlag();
        }
        else
        {
            fragHeader.ClearMoreFlag();
        }

        fragHeader.SetIdentification(ident);

        IgnoreError(otMessageAppend(message, &header, sizeof(header)));
        IgnoreError(otMessageAppend(message, &fragHeader, sizeof(fragHeader)));
        IgnoreError(otMessageAppend(message, payload, static_cast<uint16_t>(actualPayloadLen)));

        IgnoreError(otIp6Send(&node.GetInstance(), message));

        // Occasionally advance time to trigger reassembly timeouts or processing
        if (fdp.ConsumeUint8() < 32)
        {
            nexus.AdvanceTime(100);
        }
    }

    nexus.AdvanceTime(1000);

    return 0;
}

} // namespace Nexus
} // namespace ot
