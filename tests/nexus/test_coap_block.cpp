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
#include <string.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static otError TransmitHook(void *aContext, uint8_t *aBlock, uint32_t aPosition, uint16_t *aBlockLength, bool *aMore);
static otError PublicTransmitHook(void     *aContext,
                                  uint8_t  *aBlock,
                                  uint32_t  aPosition,
                                  uint16_t *aBlockLength,
                                  bool     *aMore);
static otError PrivateTransmitHook(void     *aContext,
                                   uint8_t  *aBlock,
                                   uint32_t  aPosition,
                                   uint16_t *aBlockLength,
                                   bool     *aMore);

static bool       sRequestHandlerCalled         = false;
static bool       sReceiveHookCalled            = false;
static bool       sTransmitHookCalled           = false;
static bool       sReproductionResponseReceived = false;
static otCoapCode sReproductionResponseCode     = OT_COAP_CODE_EMPTY;

static constexpr uint16_t kBypassBlockSize = 16;

static const uint8_t kPublicBlock0[kBypassBlockSize]  = {'p', 'u', 'b', 'l', 'i', 'c', '-', 'b',
                                                         'l', 'o', 'c', 'k', '-', '0', '0', '0'};
static const uint8_t kPrivateBlock0[kBypassBlockSize] = {'p', 'r', 'i', 'v', 'a', 't', 'e', '-',
                                                         'b', 'l', 'o', 'c', 'k', '-', '0', '0'};
static const uint8_t kPrivateBlock1[kBypassBlockSize] = {'s', 'e', 'c', 'r', 'e', 't', '-', 'b',
                                                         'l', 'o', 'c', 'k', '-', '0', '0', '1'};

static bool       sPublicRequestHandlerCalled  = false;
static bool       sPublicTransmitHookCalled    = false;
static bool       sPrivateRequestHandlerCalled = false;
static bool       sPrivateTransmitHookCalled   = false;
static bool       sBypassResponseReceived      = false;
static otCoapCode sBypassResponseCode          = OT_COAP_CODE_EMPTY;
static uint8_t    sBypassResponsePayload[64];
static uint16_t   sBypassResponsePayloadLength = 0;
static uint32_t   sLastTransmitPosition        = 0;

static void HandleReproductionResponse(void                *aContext,
                                       otMessage           *aMessage,
                                       const otMessageInfo *aMessageInfo,
                                       otError              aResult)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessageInfo);
    OT_UNUSED_VARIABLE(aResult);

    sReproductionResponseReceived = true;
    if (aMessage != nullptr)
    {
        sReproductionResponseCode = otCoapMessageGetCode(aMessage);
    }
}

static void ResetBypassResponseState(void)
{
    sBypassResponseReceived      = false;
    sBypassResponseCode          = OT_COAP_CODE_EMPTY;
    sBypassResponsePayloadLength = 0;
    memset(sBypassResponsePayload, 0, sizeof(sBypassResponsePayload));
}

static void HandleBypassResponse(void                *aContext,
                                 otMessage           *aMessage,
                                 const otMessageInfo *aMessageInfo,
                                 otError              aResult)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessageInfo);

    VerifyOrQuit(aResult == OT_ERROR_NONE);

    sBypassResponseReceived = true;

    if (aMessage != nullptr)
    {
        uint16_t offset = otMessageGetOffset(aMessage);
        uint16_t length = otMessageGetLength(aMessage) - offset;

        sBypassResponseCode = otCoapMessageGetCode(aMessage);
        VerifyOrQuit(length <= sizeof(sBypassResponsePayload));
        sBypassResponsePayloadLength = length;
        otMessageRead(aMessage, offset, sBypassResponsePayload, length);
    }
}

static void HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Instance      &instance = *static_cast<Instance *>(aContext);
    Coap::Message &message  = AsCoapMessage(aMessage);
    Coap::Code     code     = static_cast<Coap::Code>(message.ReadCode());
    Coap::Message *response = nullptr;

    sRequestHandlerCalled = true;

    if (code == Coap::kCodeGet)
    {
        response = instance.Get<Coap::ApplicationCoap>().NewMessage();
        VerifyOrQuit(response != nullptr);
        SuccessOrQuit(response->InitAsResponse(Coap::kTypeAck, Coap::kCodeContent, message));

        Coap::Option::Iterator iterator;
        SuccessOrQuit(iterator.Init(message, Coap::kOptionBlock2));
        if (iterator.GetOption() != nullptr)
        {
            uint64_t blockValue = 0;
            SuccessOrQuit(iterator.ReadOptionValue(blockValue));
            uint32_t blockNum = static_cast<uint32_t>(blockValue >> 4);
            bool     more     = (blockNum < 2); // We send 3 blocks in total (0, 1, 2)

            Coap::BlockInfo blockInfo;
            blockInfo.mBlockNumber = blockNum;
            blockInfo.mBlockSzx    = static_cast<Coap::BlockSzx>(blockValue & 0x7);
            blockInfo.mMoreBlocks  = more;

            SuccessOrQuit(response->AppendBlockOption(Coap::kOptionBlock2, blockInfo));
        }

        SuccessOrQuit(instance.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
            *response, AsCoreType(aMessageInfo), nullptr, nullptr, &TransmitHook, nullptr, nullptr));
    }
    else
    {
        response = instance.Get<Coap::ApplicationCoap>().NewMessage();
        VerifyOrQuit(response != nullptr);
        SuccessOrQuit(response->InitAsResponse(Coap::kTypeAck, Coap::kCodeChanged, message));
        SuccessOrQuit(instance.Get<Coap::ApplicationCoap>().SendMessage(*response, AsCoreType(aMessageInfo)));
    }
}

static otError ReceiveHook(void          *aContext,
                           const uint8_t *aBlock,
                           uint32_t       aPosition,
                           uint16_t       aBlockLength,
                           bool           aMore,
                           uint32_t       aTotalLength)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aBlock);
    OT_UNUSED_VARIABLE(aPosition);
    OT_UNUSED_VARIABLE(aBlockLength);
    OT_UNUSED_VARIABLE(aMore);
    OT_UNUSED_VARIABLE(aTotalLength);
    sReceiveHookCalled = true;
    return OT_ERROR_NONE;
}

static otError TransmitHook(void *aContext, uint8_t *aBlock, uint32_t aPosition, uint16_t *aBlockLength, bool *aMore)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aBlock);
    OT_UNUSED_VARIABLE(aPosition);

    // Provide some data
    memset(aBlock, 0xaa, *aBlockLength);

    const uint32_t kTotalLength = 48; // 3 blocks of 16 bytes

    if (aPosition + *aBlockLength >= kTotalLength)
    {
        *aMore        = false;
        *aBlockLength = kTotalLength - aPosition;
    }
    else
    {
        *aMore = true;
    }

    sTransmitHookCalled = true;
    return OT_ERROR_NONE;
}

static void HandlePublicRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Instance       &instance = *static_cast<Instance *>(aContext);
    Coap::Message  &message  = AsCoapMessage(aMessage);
    Coap::Message  *response = nullptr;
    Coap::BlockInfo blockInfo;

    sPublicRequestHandlerCalled = true;

    response = instance.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(response != nullptr);
    SuccessOrQuit(response->InitAsResponse(Coap::kTypeAck, Coap::kCodeContent, message));

    blockInfo.mBlockNumber = 0;
    blockInfo.mBlockSzx    = Coap::kBlockSzx16;
    blockInfo.mMoreBlocks  = true;
    SuccessOrQuit(response->AppendBlockOption(Coap::kOptionBlock2, blockInfo));

    SuccessOrQuit(instance.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *response, AsCoreType(aMessageInfo), nullptr, nullptr, &PublicTransmitHook, nullptr, nullptr));
}

static void HandlePrivateRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Instance      &instance = *static_cast<Instance *>(aContext);
    Coap::Message &message  = AsCoapMessage(aMessage);
    Coap::Message *response = nullptr;

    sPrivateRequestHandlerCalled = true;

    response = instance.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(response != nullptr);
    SuccessOrQuit(response->InitAsResponse(Coap::kTypeAck, Coap::kCodeForbidden, message));
    SuccessOrQuit(instance.Get<Coap::ApplicationCoap>().SendMessage(*response, AsCoreType(aMessageInfo)));
}

static otError PublicTransmitHook(void     *aContext,
                                  uint8_t  *aBlock,
                                  uint32_t  aPosition,
                                  uint16_t *aBlockLength,
                                  bool     *aMore)
{
    OT_UNUSED_VARIABLE(aContext);

    VerifyOrQuit(aPosition == 0);
    VerifyOrQuit(*aBlockLength >= sizeof(kPublicBlock0));

    memcpy(aBlock, kPublicBlock0, sizeof(kPublicBlock0));
    *aBlockLength             = sizeof(kPublicBlock0);
    *aMore                    = true;
    sPublicTransmitHookCalled = true;
    sLastTransmitPosition     = aPosition;

    return OT_ERROR_NONE;
}

static otError PrivateTransmitHook(void     *aContext,
                                   uint8_t  *aBlock,
                                   uint32_t  aPosition,
                                   uint16_t *aBlockLength,
                                   bool     *aMore)
{
    const uint8_t *payload = nullptr;

    OT_UNUSED_VARIABLE(aContext);

    switch (aPosition)
    {
    case 0:
        payload = kPrivateBlock0;
        *aMore  = true;
        break;
    case kBypassBlockSize:
        payload = kPrivateBlock1;
        *aMore  = false;
        break;
    default:
        return OT_ERROR_INVALID_ARGS;
    }

    VerifyOrQuit(*aBlockLength >= kBypassBlockSize);

    memcpy(aBlock, payload, kBypassBlockSize);
    *aBlockLength              = kBypassBlockSize;
    sPrivateTransmitHookCalled = true;
    sLastTransmitPosition      = aPosition;

    return OT_ERROR_NONE;
}

static void SendBlock2Get(Node &aClient, const Ip6::Address &aPeer, const char *aUriPath, uint32_t aBlockNumber)
{
    Coap::Message   *message = aClient.Get<Coap::ApplicationCoap>().NewMessage();
    Ip6::MessageInfo messageInfo;
    Coap::BlockInfo  blockInfo;

    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet));
    SuccessOrQuit(message->AppendUriPathOptions(aUriPath));

    blockInfo.mBlockNumber = aBlockNumber;
    blockInfo.mBlockSzx    = Coap::kBlockSzx16;
    blockInfo.mMoreBlocks  = false;
    SuccessOrQuit(message->AppendBlockOption(Coap::kOptionBlock2, blockInfo));

    messageInfo.SetPeerAddr(aPeer);
    messageInfo.SetPeerPort(OT_DEFAULT_COAP_PORT);

    SuccessOrQuit(aClient.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *message, messageInfo, nullptr, &HandleBypassResponse, nullptr, nullptr, nullptr));
}

void TestCoapBlock(void)
{
    Core nexus;

    Node &leader = nexus.CreateNode();
    Node &router = nexus.CreateNode();

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("Form network");
    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsChild() || router.Get<Mle::Mle>().IsRouter());

    // Start CoAP on Leader
    SuccessOrQuit(leader.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));

    Coap::ResourceBlockWise resource("test", &HandleRequest, &leader.GetInstance(), &ReceiveHook, &TransmitHook);

    leader.Get<Coap::ApplicationCoap>().AddBlockWiseResource(resource);

    // Start CoAP on Router
    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));

    // Router sends GET request
    Coap::Message *message = router.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet));
    SuccessOrQuit(message->AppendUriPathOptions("test"));

    Coap::BlockInfo blockInfo;
    blockInfo.mBlockNumber = 0;
    blockInfo.mBlockSzx    = Coap::kBlockSzx16;
    blockInfo.mMoreBlocks  = false;
    SuccessOrQuit(message->AppendBlockOption(Coap::kOptionBlock2, blockInfo));

    Ip6::MessageInfo messageInfo;
    messageInfo.SetPeerAddr(leader.Get<Mle::Mle>().GetMeshLocalEid());
    messageInfo.SetPeerPort(OT_DEFAULT_COAP_PORT);

    sRequestHandlerCalled = false;
    sTransmitHookCalled   = false;
    sReceiveHookCalled    = false;

    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *message, messageInfo, nullptr, nullptr, &TransmitHook, &ReceiveHook, nullptr));

    nexus.AdvanceTime(5 * 1000);

    // Verify hooks were called
    VerifyOrQuit(sRequestHandlerCalled);
    VerifyOrQuit(sTransmitHookCalled); // Leader should transmit blocks
    VerifyOrQuit(sReceiveHookCalled);  // Router should receive blocks

    // Router sends PUT request
    message = router.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodePut));
    SuccessOrQuit(message->AppendUriPathOptions("test"));

    blockInfo.mBlockNumber = 0;
    blockInfo.mBlockSzx    = Coap::kBlockSzx16;
    blockInfo.mMoreBlocks  = true;
    SuccessOrQuit(message->AppendBlockOption(Coap::kOptionBlock1, blockInfo));

    sRequestHandlerCalled = false;
    sTransmitHookCalled   = false;
    sReceiveHookCalled    = false;

    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *message, messageInfo, nullptr, nullptr, &TransmitHook, &ReceiveHook, nullptr));

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sRequestHandlerCalled);
    VerifyOrQuit(sTransmitHookCalled); // Router should transmit blocks
    VerifyOrQuit(sReceiveHookCalled);  // Leader should receive blocks

    // Reproduction test case:
    // Router sends GET request with Block2 number 1 (NUM > 0) but without active transfer
    message = router.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet));
    SuccessOrQuit(message->AppendUriPathOptions("test"));

    blockInfo.mBlockNumber = 1;
    blockInfo.mBlockSzx    = Coap::kBlockSzx16;
    blockInfo.mMoreBlocks  = false;
    SuccessOrQuit(message->AppendBlockOption(Coap::kOptionBlock2, blockInfo));

    sReproductionResponseReceived = false;
    sReproductionResponseCode     = OT_COAP_CODE_EMPTY;

    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *message, messageInfo, nullptr, &HandleReproductionResponse, nullptr, nullptr, nullptr));

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sReproductionResponseReceived);
    VerifyOrQuit(sReproductionResponseCode == OT_COAP_CODE_REQUEST_INCOMPLETE);

    leader.Get<Coap::ApplicationCoap>().RemoveBlockWiseResource(resource);
    IgnoreError(leader.Get<Coap::ApplicationCoap>().Stop());
    IgnoreError(router.Get<Coap::ApplicationCoap>().Stop());
}

void TestCoapBlockCrossResourceIsolation(void)
{
    Core nexus;

    Node &leader   = nexus.CreateNode();
    Node &attacker = nexus.CreateNode();

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("Form network for cross-resource blockwise test");
    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    attacker.Join(leader);
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(attacker.Get<Mle::Mle>().IsChild() || attacker.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(leader.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));

    Coap::ResourceBlockWise publicResource("public", &HandlePublicRequest, &leader.GetInstance(), &ReceiveHook,
                                           &PublicTransmitHook);
    Coap::ResourceBlockWise privateResource("private", &HandlePrivateRequest, &leader.GetInstance(), &ReceiveHook,
                                            &PrivateTransmitHook);

    leader.Get<Coap::ApplicationCoap>().AddBlockWiseResource(publicResource);
    leader.Get<Coap::ApplicationCoap>().AddBlockWiseResource(privateResource);

    SuccessOrQuit(attacker.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));

    const Ip6::Address &leaderAddress = leader.Get<Mle::Mle>().GetMeshLocalEid();

    // Step 1: attacker primes global blockwise state through a public resource.
    sPublicRequestHandlerCalled = false;
    sPublicTransmitHookCalled   = false;
    sLastTransmitPosition       = 0;
    ResetBypassResponseState();

    SendBlock2Get(attacker, leaderAddress, "public", 0);
    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sBypassResponseReceived);
    VerifyOrQuit(sBypassResponseCode == OT_COAP_CODE_CONTENT);
    VerifyOrQuit(sPublicRequestHandlerCalled);
    VerifyOrQuit(sPublicTransmitHookCalled);
    VerifyOrQuit(sLastTransmitPosition == 0);
    VerifyOrQuit(sBypassResponsePayloadLength == sizeof(kPublicBlock0));
    VerifyOrQuit(memcmp(sBypassResponsePayload, kPublicBlock0, sizeof(kPublicBlock0)) == 0);

    // Step 2: normal access to the protected resource is rejected.
    sPrivateRequestHandlerCalled = false;
    sPrivateTransmitHookCalled   = false;
    sLastTransmitPosition        = 0;
    ResetBypassResponseState();

    SendBlock2Get(attacker, leaderAddress, "private", 0);
    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sBypassResponseReceived);
    VerifyOrQuit(sBypassResponseCode == OT_COAP_CODE_FORBIDDEN);
    VerifyOrQuit(sPrivateRequestHandlerCalled);
    VerifyOrQuit(!sPrivateTransmitHookCalled);

    // Step 3: the same attacker requests Block2 NUM=1 on the protected resource.
    // The continuation must be rejected because the cached public transfer state
    // does not match the protected resource request.
    sPrivateRequestHandlerCalled = false;
    sPrivateTransmitHookCalled   = false;
    sLastTransmitPosition        = 0;
    ResetBypassResponseState();

    SendBlock2Get(attacker, leaderAddress, "private", 1);
    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sBypassResponseReceived);
    VerifyOrQuit(sBypassResponseCode == OT_COAP_CODE_REQUEST_INCOMPLETE);
    VerifyOrQuit(!sPrivateRequestHandlerCalled);
    VerifyOrQuit(!sPrivateTransmitHookCalled);
    VerifyOrQuit(sLastTransmitPosition == 0);
    VerifyOrQuit(sBypassResponsePayloadLength == 0);

    leader.Get<Coap::ApplicationCoap>().RemoveBlockWiseResource(publicResource);
    leader.Get<Coap::ApplicationCoap>().RemoveBlockWiseResource(privateResource);
    IgnoreError(leader.Get<Coap::ApplicationCoap>().Stop());
    IgnoreError(attacker.Get<Coap::ApplicationCoap>().Stop());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCoapBlock();
    ot::Nexus::TestCoapBlockCrossResourceIsolation();
    printf("All tests passed\n");
    return 0;
}
