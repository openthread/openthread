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

    otError ConsumeRemainingBytes(Message &aMessage)
    {
        otError error;

        SuccessOrExit(error = aMessage.AppendBytes(mData, static_cast<uint16_t>(mSize)));
        mSize = 0;

    exit:
        return error;
    }

    bool ConsumeBool(void)
    {
        assert(mSize > 0);

        uint8_t result;

        ConsumeData(&result, sizeof(result));

        return result & 1;
    }

private:
    const uint8_t *mData;
    size_t         mSize;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const uint16_t kMaxMessageSize = 2048;

    FuzzDataProvider fdp(data, size);

    unsigned int          seed;
    Message              *message;
    bool                  isUnicast;
    otPlatMdnsAddressInfo addressInfo;

    if (size < sizeof(seed) + sizeof(isUnicast) + sizeof(addressInfo))
    {
        return 0;
    }

    if (size > sizeof(seed) + sizeof(isUnicast) + sizeof(addressInfo) + kMaxMessageSize)
    {
        return 0;
    }

    fdp.ConsumeData(&seed, sizeof(seed));
    srand(seed);

    Core nexus;

    Node &node = nexus.CreateNode();

    node.GetInstance().SetLogLevel(kLogLevelInfo);

    node.GetInstance().Get<BorderRouter::RoutingManager>().Init(/* aInfraIfIndex */ 1, /* aInfraIfIsRunning */ true);
    node.GetInstance().Get<BorderRouter::RoutingManager>().SetEnabled(true);
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

    isUnicast = fdp.ConsumeBool();
    fdp.ConsumeData(&addressInfo, sizeof(addressInfo));

    message = node.GetInstance().Get<MessagePool>().Allocate(Message::kTypeOther);
    VerifyOrQuit(message != nullptr);

    SuccessOrQuit(fdp.ConsumeRemainingBytes(*message));

    otPlatMdnsHandleReceive(&node.GetInstance(), message, isUnicast, &addressInfo);

    nexus.AdvanceTime(10 * 1000);

exit:
    return 0;
}

} // namespace Nexus
} // namespace ot
