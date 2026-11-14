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

static bool       sRequestHandlerCalled         = false;
static bool       sReceiveHookCalled            = false;
static bool       sTransmitHookCalled           = false;
static bool       sReproductionResponseReceived = false;
static otCoapCode sReproductionResponseCode     = OT_COAP_CODE_EMPTY;

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

enum EtagScenario : uint8_t
{
    kEtagMatching4Bytes,        // All blocks carry identical 4-byte ETag.
    kEtagMatching8Bytes,        // All blocks carry identical 8-byte (max length) ETag.
    kEtagMismatchOnMiddleBlock, // Block 0 has ETag A, block 1 (M=1) has ETag B.
    kEtagMismatchOnFinalBlock,  // Blocks 0 & 1 have ETag A, block 2 (M=0) has ETag B.
    kEtagLengthChangeMidway,    // Block 0 has 4-byte ETag, block 1 has 2-byte ETag.
    kEtagOmittedOnLaterBlock,   // Block 0 has ETag A, later blocks omit ETag.
    kEtagAddedOnLaterBlock,     // Block 0 has no ETag, later blocks include ETag A.
    kEtagZeroLength,            // First response carries a 0-byte ETag option.
    kEtagTooLong,               // First response carries a 9-byte (> max) ETag option.
    kEtagDuplicateOptions,      // First response carries duplicate ETag options.
};

struct EtagClientContext
{
    uint16_t mResponseCount;
    otError  mResult;
    uint16_t mBlocksReceived;
    uint32_t mTotalBytesReceived;
    uint32_t mExpectedNextPosition;
};

static EtagScenario sEtagScenario       = kEtagMatching4Bytes;
static uint16_t     sEtagServerRequests = 0;

static void HandleEtagRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static constexpr uint8_t  kEtag4A[4]                      = {0xde, 0xad, 0xbe, 0xef};
    static constexpr uint8_t  kEtag4B[4]                      = {0xca, 0xfe, 0xba, 0xbe};
    static constexpr uint8_t  kEtag8[OT_COAP_MAX_ETAG_LENGTH] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    static constexpr uint8_t  kEtag9[9]                       = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    static constexpr uint16_t kFullBlockLength                = 16;
    static constexpr uint16_t kFinalBlockLength               = 12;

    Instance       &instance = *static_cast<Instance *>(aContext);
    Coap::Message  &request  = AsCoapMessage(aMessage);
    Coap::Message  *response = nullptr;
    Coap::BlockInfo requestBlockInfo;
    Coap::BlockInfo responseBlockInfo;
    uint8_t         payload[kFullBlockLength];
    uint16_t        payloadLength;
    uint32_t        blockNum;
    bool            moreBlocks;

    sEtagServerRequests++;

    VerifyOrQuit(request.ReadCode() == Coap::kCodeGet);
    SuccessOrQuit(request.ReadBlockOptionValues(Coap::kOptionBlock2, requestBlockInfo));
    VerifyOrQuit(requestBlockInfo.mBlockSzx == Coap::kBlockSzx16);

    blockNum      = requestBlockInfo.mBlockNumber;
    moreBlocks    = (blockNum < 2);
    payloadLength = moreBlocks ? kFullBlockLength : kFinalBlockLength;

    response = instance.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(response != nullptr);
    SuccessOrQuit(response->InitAsResponse(Coap::kTypeAck, Coap::kCodeContent, request));

    switch (sEtagScenario)
    {
    case kEtagMatching4Bytes:
        SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        break;

    case kEtagMatching8Bytes:
        SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag8), kEtag8));
        break;

    case kEtagMismatchOnMiddleBlock:
        if (blockNum == 0)
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        }
        else
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4B), kEtag4B));
        }
        break;

    case kEtagMismatchOnFinalBlock:
        if (blockNum < 2)
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        }
        else
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4B), kEtag4B));
        }
        break;

    case kEtagLengthChangeMidway:
        if (blockNum == 0)
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        }
        else
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, 2, kEtag4A));
        }
        break;

    case kEtagOmittedOnLaterBlock:
        if (blockNum == 0)
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        }
        break;

    case kEtagAddedOnLaterBlock:
        if (blockNum > 0)
        {
            SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        }
        break;

    case kEtagZeroLength:
        SuccessOrQuit(response->AppendOption(Coap::kOptionETag, 0, kEtag4A));
        break;

    case kEtagTooLong:
        SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag9), kEtag9));
        break;

    case kEtagDuplicateOptions:
        SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        SuccessOrQuit(response->AppendOption(Coap::kOptionETag, sizeof(kEtag4A), kEtag4A));
        break;
    }

    responseBlockInfo.mBlockNumber = blockNum;
    responseBlockInfo.mBlockSzx    = Coap::kBlockSzx16;
    responseBlockInfo.mMoreBlocks  = moreBlocks;
    SuccessOrQuit(response->AppendBlockOption(Coap::kOptionBlock2, responseBlockInfo));

    memset(payload, static_cast<uint8_t>(0xa0 + blockNum), payloadLength);
    SuccessOrQuit(response->AppendPayloadMarker());
    SuccessOrQuit(response->AppendBytes(payload, payloadLength));

    SuccessOrQuit(instance.Get<Coap::ApplicationCoap>().SendMessage(*response, AsCoreType(aMessageInfo)));
}

static otError HandleEtagBlockReceive(void          *aContext,
                                      const uint8_t *aBlock,
                                      uint32_t       aPosition,
                                      uint16_t       aBlockLength,
                                      bool           aMore,
                                      uint32_t       aTotalLength)
{
    OT_UNUSED_VARIABLE(aMore);
    OT_UNUSED_VARIABLE(aTotalLength);

    EtagClientContext &context     = *static_cast<EtagClientContext *>(aContext);
    uint8_t            expectedVal = static_cast<uint8_t>(0xa0 + (aPosition / 16));

    VerifyOrQuit(aPosition == context.mExpectedNextPosition);

    for (uint16_t i = 0; i < aBlockLength; i++)
    {
        VerifyOrQuit(aBlock[i] == expectedVal);
    }

    context.mBlocksReceived++;
    context.mTotalBytesReceived += aBlockLength;
    context.mExpectedNextPosition += aBlockLength;

    return OT_ERROR_NONE;
}

static void HandleEtagResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    EtagClientContext &context = *static_cast<EtagClientContext *>(aContext);

    context.mResponseCount++;
    context.mResult = aResult;
}

static void RunEtagTransferCase(Core        &aNexus,
                                Node        &aClient,
                                Node        &aServer,
                                EtagScenario aScenario,
                                uint32_t     aStartBlockNum,
                                otError      aExpectedResult,
                                uint16_t     aExpectedServerRequests,
                                uint16_t     aExpectedBlocksReceived,
                                uint32_t     aExpectedTotalBytes)
{
    EtagClientContext  context;
    otMessage         *request = nullptr;
    Ip6::MessageInfo   messageInfo;
    MessageQueue::Info queueInfo;

    ClearAllBytes(context);
    context.mResult               = OT_ERROR_FAILED;
    context.mExpectedNextPosition = aStartBlockNum * 16;

    sEtagScenario       = aScenario;
    sEtagServerRequests = 0;

    request = otCoapNewMessage(&aClient.GetInstance(), nullptr);
    VerifyOrQuit(request != nullptr);
    SuccessOrQuit(otCoapMessageInit(request, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET));
    otCoapMessageGenerateToken(request, OT_COAP_DEFAULT_TOKEN_LENGTH);
    SuccessOrQuit(otCoapMessageAppendUriPathOptions(request, "etag"));
    SuccessOrQuit(otCoapMessageAppendBlock2Option(request, aStartBlockNum, false, OT_COAP_OPTION_BLOCK_SZX_16));

    messageInfo.SetPeerAddr(aServer.Get<Mle::Mle>().GetMeshLocalEid());
    messageInfo.SetPeerPort(OT_DEFAULT_COAP_PORT);

    SuccessOrQuit(otCoapSendRequestBlockWise(&aClient.GetInstance(), request, &messageInfo, &HandleEtagResponse,
                                             &context, nullptr, &HandleEtagBlockReceive));

    aNexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(context.mResponseCount == 1);
    VerifyOrQuit(context.mResult == aExpectedResult);
    VerifyOrQuit(sEtagServerRequests == aExpectedServerRequests);
    VerifyOrQuit(context.mBlocksReceived == aExpectedBlocksReceived);
    VerifyOrQuit(context.mTotalBytesReceived == aExpectedTotalBytes);

    aClient.Get<Coap::ApplicationCoap>().GetRequestAndCachedResponsesQueueInfo(queueInfo);
    VerifyOrQuit(queueInfo.mNumMessages == 0);
}

void TestCoapBlock2Etag(void)
{
    Core           nexus;
    Node          &leader = nexus.CreateNode();
    Node          &router = nexus.CreateNode();
    Coap::Resource etagResource("etag", &HandleEtagRequest, &leader.GetInstance());

    nexus.AdvanceTime(0);

    SuccessOrQuit(Instance::SetGlobalLogLevel(kLogLevelInfo));

    Log("TestCoapBlock2Etag: Form network");
    leader.Form();
    nexus.AdvanceTime(13 * 1000);
    VerifyOrQuit(leader.Get<Mle::Mle>().IsLeader());

    router.Join(leader);
    nexus.AdvanceTime(10 * 1000);
    VerifyOrQuit(router.Get<Mle::Mle>().IsChild() || router.Get<Mle::Mle>().IsRouter());

    SuccessOrQuit(leader.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));
    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));

    leader.Get<Coap::ApplicationCoap>().AddResource(etagResource);

    Log("Case 1a: Matching 4-byte ETag across all blocks (0..2)");
    RunEtagTransferCase(nexus, router, leader, kEtagMatching4Bytes, 0, OT_ERROR_NONE, 3, 3, 44);

    Log("Case 1b: Matching 8-byte max-length ETag across all blocks (0..2)");
    RunEtagTransferCase(nexus, router, leader, kEtagMatching8Bytes, 0, OT_ERROR_NONE, 3, 3, 44);

    Log("Case 1c: Matching ETag when starting Block2 transfer at block 1");
    RunEtagTransferCase(nexus, router, leader, kEtagMatching4Bytes, 1, OT_ERROR_NONE, 2, 2, 28);

    Log("Case 2a: Different ETag on intermediate block (M=1) aborts before receive hook");
    RunEtagTransferCase(nexus, router, leader, kEtagMismatchOnMiddleBlock, 0, OT_ERROR_ABORT, 2, 1, 16);

    Log("Case 2b: Different ETag on final block (M=0) aborts before receive hook");
    RunEtagTransferCase(nexus, router, leader, kEtagMismatchOnFinalBlock, 0, OT_ERROR_ABORT, 3, 2, 32);

    Log("Case 2c: Different ETag when starting at block 1 aborts on block 2");
    RunEtagTransferCase(nexus, router, leader, kEtagMismatchOnFinalBlock, 1, OT_ERROR_ABORT, 2, 1, 16);

    Log("Case 2d: ETag length change mid-transfer aborts");
    RunEtagTransferCase(nexus, router, leader, kEtagLengthChangeMidway, 0, OT_ERROR_ABORT, 2, 1, 16);

    Log("Case 3: ETag present on first block and omitted on subsequent block aborts");
    RunEtagTransferCase(nexus, router, leader, kEtagOmittedOnLaterBlock, 0, OT_ERROR_ABORT, 2, 1, 16);

    Log("Case 4: ETag absent on first block and added on subsequent block aborts");
    RunEtagTransferCase(nexus, router, leader, kEtagAddedOnLaterBlock, 0, OT_ERROR_ABORT, 2, 1, 16);

    Log("Case 5a: Invalid 0-byte ETag option aborts");
    RunEtagTransferCase(nexus, router, leader, kEtagZeroLength, 0, OT_ERROR_ABORT, 1, 0, 0);

    Log("Case 5b: Invalid 9-byte (> OT_COAP_MAX_ETAG_LENGTH) ETag option aborts");
    RunEtagTransferCase(nexus, router, leader, kEtagTooLong, 0, OT_ERROR_ABORT, 1, 0, 0);

    Log("Case 5c: Duplicate ETag options in a response aborts");
    RunEtagTransferCase(nexus, router, leader, kEtagDuplicateOptions, 0, OT_ERROR_ABORT, 1, 0, 0);

    leader.Get<Coap::ApplicationCoap>().RemoveResource(etagResource);
    IgnoreError(leader.Get<Coap::ApplicationCoap>().Stop());
    IgnoreError(router.Get<Coap::ApplicationCoap>().Stop());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCoapBlock();
    ot::Nexus::TestCoapBlock2Etag();
    printf("All tests passed\n");
    return 0;
}
