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
 * Verifies how the CoAP server handles received FETCH (RFC 8132) requests.
 *
 * Node A sends unicast FETCH requests to node B:
 *
 * - Plain FETCH (CON): Uri-Path and Content-Format options. Must reach B's resource handler.
 *
 * - OSCORE FETCH (CON): outer FETCH as used by OSCORE for Observe (RFC 8613), with Observe and
 *   OSCORE options but no Content-Format and no Uri-Path (both are encrypted inner options).
 *   Must reach B's default handler.
 *
 * - FETCH (CON) with neither Content-Format nor OSCORE option. Must be rejected with a
 *   piggybacked 4.15 (Unsupported Content-Format) without invoking any handler.
 *
 * - FETCH (NON) with neither Content-Format nor OSCORE option. Must be rejected with a
 *   Non-confirmable 4.15 response (RFC 7252 Section 5.2.3), not silently dropped.
 */

#include <stdio.h>
#include <string.h>

#include <openthread/coap.h>

#include "platform/nexus_core.hpp"
#include "platform/nexus_node.hpp"

namespace ot {
namespace Nexus {

static constexpr uint32_t kFormNetworkTime = 13 * 1000;
static constexpr uint32_t kJoinTime        = 30 * 1000;
static constexpr uint32_t kResponseTime    = 2 * 1000;
static constexpr uint16_t kCoapPort        = OT_DEFAULT_COAP_PORT;

// OSCORE option value from RFC 8613 Section 6.3 (example 1): flags 0x09, Partial IV 0x05, kid 0x25.
static const uint8_t kOscoreOptionValue[] = {0x09, 0x05, 0x25};

enum FetchType : uint8_t
{
    kPlainFetch,
    kOscoreFetch,
    kNoFormatFetch,
};

static Node        *sNodeB;
static otIp6Address sNodeBAddr;
static uint8_t      sResourceRequests;
static uint8_t      sDefaultRequests;
static otCoapCode   sReceivedCode;
static bool         sResponseReceived;
static otError      sResponseError;
static otCoapCode   sResponseCode;
static otCoapType   sResponseType;

static const char *FetchTypeToString(FetchType aType)
{
    static const char *const kStrings[] = {"plain", "OSCORE", "no-format"};

    return kStrings[aType];
}

static void SendContentResponse(otMessage *aRequest, const otMessageInfo *aMessageInfo)
{
    otMessage *rsp = otCoapNewMessage(&sNodeB->GetInstance(), nullptr);

    VerifyOrQuit(rsp != nullptr);
    SuccessOrQuit(otCoapMessageInitResponse(rsp, aRequest, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CONTENT));
    SuccessOrQuit(otCoapMessageSetPayloadMarker(rsp));
    SuccessOrQuit(otMessageAppend(rsp, "x", 1));
    SuccessOrQuit(otCoapSendResponse(&sNodeB->GetInstance(), rsp, aMessageInfo));
}

static void HandleResource(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);

    Log("B: resource handler invoked");
    sResourceRequests++;
    sReceivedCode = otCoapMessageGetCode(aMessage);
    SendContentResponse(aMessage, aMessageInfo);
}

static void HandleDefault(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);

    Log("B: default handler invoked");
    sDefaultRequests++;
    sReceivedCode = otCoapMessageGetCode(aMessage);
    SendContentResponse(aMessage, aMessageInfo);
}

static void HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessageInfo);

    sResponseReceived = true;
    sResponseError    = aError;

    if (aMessage != nullptr)
    {
        sResponseCode = otCoapMessageGetCode(aMessage);
        sResponseType = otCoapMessageGetType(aMessage);
    }

    Log("A: response handler invoked (error=%d, code=%u.%02u, type=%u)", aError, sResponseCode >> 5,
        sResponseCode & 0x1f, sResponseType);
}

static void SendFetch(Node &aNode, FetchType aType, otCoapType aMessageType)
{
    static const char kPayload[] = "{\"q\":1}";

    otMessage    *req = otCoapNewMessage(&aNode.GetInstance(), nullptr);
    otMessageInfo msgInfo;

    Log("A: sending %s %s FETCH to B", (aMessageType == OT_COAP_TYPE_CONFIRMABLE) ? "CON" : "NON",
        FetchTypeToString(aType));

    VerifyOrQuit(req != nullptr);
    SuccessOrQuit(otCoapMessageInit(req, aMessageType, OT_COAP_CODE_FETCH));
    otCoapMessageGenerateToken(req, 8);

    // Options must be appended in increasing Option Number order.
    if (aType == kOscoreFetch)
    {
        SuccessOrQuit(otCoapMessageAppendObserveOption(req, 0));
        SuccessOrQuit(
            otCoapMessageAppendOption(req, OT_COAP_OPTION_OSCORE, sizeof(kOscoreOptionValue), kOscoreOptionValue));
    }
    else
    {
        SuccessOrQuit(otCoapMessageAppendUriPathOptions(req, "query"));
    }

    if (aType == kPlainFetch)
    {
        SuccessOrQuit(otCoapMessageAppendContentFormatOption(req, OT_COAP_OPTION_CONTENT_FORMAT_JSON));
    }

    SuccessOrQuit(otCoapMessageSetPayloadMarker(req));
    SuccessOrQuit(otMessageAppend(req, kPayload, sizeof(kPayload) - 1));

    memset(&msgInfo, 0, sizeof(msgInfo));
    msgInfo.mPeerAddr = sNodeBAddr;
    msgInfo.mPeerPort = kCoapPort;

    sResourceRequests = 0;
    sDefaultRequests  = 0;
    sReceivedCode     = OT_COAP_CODE_EMPTY;
    sResponseReceived = false;
    sResponseError    = OT_ERROR_NONE;
    sResponseCode     = OT_COAP_CODE_EMPTY;
    sResponseType     = OT_COAP_TYPE_RESET;

    SuccessOrQuit(otCoapSendRequest(&aNode.GetInstance(), req, &msgInfo, HandleResponse, nullptr));
}

static otCoapResource sResource;

void TestCoapFetch(void)
{
    Core  nexus;
    Node &nodeA = nexus.CreateNode();
    Node &nodeB = nexus.CreateNode();

    sNodeB = &nodeB;

    Log("TestCoapFetch");

    nodeA.Form();
    nexus.AdvanceTime(kFormNetworkTime);
    VerifyOrQuit(nodeA.Get<Mle::Mle>().IsLeader());

    nodeB.Join(nodeA);
    nexus.AdvanceTime(kJoinTime);
    VerifyOrQuit(nodeB.Get<Mle::Mle>().IsAttached());

    sNodeBAddr = nodeB.Get<Mle::Mle>().GetMeshLocalEid();

    SuccessOrQuit(otCoapStart(&nodeA.GetInstance(), kCoapPort));
    SuccessOrQuit(otCoapStart(&nodeB.GetInstance(), kCoapPort));

    memset(&sResource, 0, sizeof(sResource));
    sResource.mUriPath = "query";
    sResource.mHandler = HandleResource;
    otCoapAddResource(&nodeB.GetInstance(), &sResource);
    otCoapSetDefaultHandler(&nodeB.GetInstance(), HandleDefault, nullptr);

    Log("----------------------------------------------------------------------------------------");
    Log("Plain FETCH (with Content-Format) is delivered to the resource handler");

    SendFetch(nodeA, kPlainFetch, OT_COAP_TYPE_CONFIRMABLE);
    nexus.AdvanceTime(kResponseTime);

    VerifyOrQuit(sResourceRequests == 1);
    VerifyOrQuit(sDefaultRequests == 0);
    VerifyOrQuit(sReceivedCode == OT_COAP_CODE_FETCH);
    VerifyOrQuit(sResponseReceived);
    VerifyOrQuit(sResponseError == OT_ERROR_NONE);
    VerifyOrQuit(sResponseCode == OT_COAP_CODE_CONTENT);
    VerifyOrQuit(sResponseType == OT_COAP_TYPE_ACKNOWLEDGMENT);

    Log("----------------------------------------------------------------------------------------");
    Log("OSCORE FETCH (with OSCORE option, no Content-Format, no Uri-Path) is delivered to the default handler");

    SendFetch(nodeA, kOscoreFetch, OT_COAP_TYPE_CONFIRMABLE);
    nexus.AdvanceTime(kResponseTime);

    VerifyOrQuit(sResourceRequests == 0);
    VerifyOrQuit(sDefaultRequests == 1);
    VerifyOrQuit(sReceivedCode == OT_COAP_CODE_FETCH);
    VerifyOrQuit(sResponseReceived);
    VerifyOrQuit(sResponseError == OT_ERROR_NONE);
    VerifyOrQuit(sResponseCode == OT_COAP_CODE_CONTENT);
    VerifyOrQuit(sResponseType == OT_COAP_TYPE_ACKNOWLEDGMENT);

    Log("----------------------------------------------------------------------------------------");
    Log("CON FETCH without Content-Format and without OSCORE option is rejected with piggybacked 4.15");

    SendFetch(nodeA, kNoFormatFetch, OT_COAP_TYPE_CONFIRMABLE);
    nexus.AdvanceTime(kResponseTime);

    VerifyOrQuit(sResourceRequests == 0);
    VerifyOrQuit(sDefaultRequests == 0);
    VerifyOrQuit(sResponseReceived);
    VerifyOrQuit(sResponseError == OT_ERROR_NONE);
    VerifyOrQuit(sResponseCode == OT_COAP_CODE_UNSUPPORTED_FORMAT);
    VerifyOrQuit(sResponseType == OT_COAP_TYPE_ACKNOWLEDGMENT);

    Log("----------------------------------------------------------------------------------------");
    Log("NON FETCH without Content-Format and without OSCORE option is rejected with NON 4.15");

    SendFetch(nodeA, kNoFormatFetch, OT_COAP_TYPE_NON_CONFIRMABLE);
    nexus.AdvanceTime(kResponseTime);

    VerifyOrQuit(sResourceRequests == 0);
    VerifyOrQuit(sDefaultRequests == 0);
    VerifyOrQuit(sResponseReceived);
    VerifyOrQuit(sResponseError == OT_ERROR_NONE);
    VerifyOrQuit(sResponseCode == OT_COAP_CODE_UNSUPPORTED_FORMAT);
    VerifyOrQuit(sResponseType == OT_COAP_TYPE_NON_CONFIRMABLE);

    Log("TestCoapFetch completed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCoapFetch();
    printf("All tests passed\n");
    return 0;
}