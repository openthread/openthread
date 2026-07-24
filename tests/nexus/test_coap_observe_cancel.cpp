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
 * Verifies in-handler cancellation of a CoAP observation.
 *
 * Node A observes a resource on node B (GET + Observe=0, confirmable). B answers with a
 * piggybacked ACK carrying an Observe option. Inside A's response handler (the documented
 * RFC7641 cancellation pattern) A sends a GET with Observe=1 and the SAME token, which makes
 * the CoAP core finalize and free the tracked observe request message. The request metadata
 * update for the notification must therefore happen before the handler is dispatched; this
 * test exercises that ordering (message lifetime issues surface under sanitizer builds).
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
static constexpr uint16_t kCoapPort        = OT_DEFAULT_COAP_PORT;

static Node *sNodeA;
static Node *sNodeB;
static otIp6Address sNodeBAddr;
static bool  sNotificationSeen;
static bool  sCancelSent;

static void HandleSecondResponse(void *, otMessage *, const otMessageInfo *, otError) {}

static void HandleObserveResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessageInfo);

    Log("A: observe response handler invoked (error=%d, msg=%s)", aError,
        (aMessage != nullptr) ? "yes" : "null");

    // The in-handler cancellation reentrantly finalizes this same request (handler invoked
    // again with a null message); only act on the first, genuine notification.
    if (sCancelSent || (aMessage == nullptr) || (aError != OT_ERROR_NONE))
    {
        return;
    }

    sNotificationSeen = true;

    // RFC7641 cancellation from inside the notification callback: GET with Observe=1 and
    // the SAME token to the same server.
    {
        otMessage    *req = otCoapNewMessage(&sNodeA->GetInstance(), nullptr);
        otMessageInfo msgInfo;

        VerifyOrQuit(req != nullptr);
        otCoapMessageInit(req, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);
        SuccessOrQuit(otCoapMessageSetToken(req, otCoapMessageGetToken(aMessage),
                                            otCoapMessageGetTokenLength(aMessage)));
        SuccessOrQuit(otCoapMessageAppendObserveOption(req, 1));
        SuccessOrQuit(otCoapMessageAppendUriPathOptions(req, "obs"));

        memset(&msgInfo, 0, sizeof(msgInfo));
        msgInfo.mPeerAddr = sNodeBAddr;
        msgInfo.mPeerPort = kCoapPort;

        SuccessOrQuit(otCoapSendRequest(&sNodeA->GetInstance(), req, &msgInfo, HandleSecondResponse, nullptr));
        sCancelSent = true;
        Log("A: cancellation GET (Observe=1, same token) sent from inside the handler");
    }
}

static void HandleResource(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);

    otMessage *rsp = otCoapNewMessage(&sNodeB->GetInstance(), nullptr);

    Log("B: resource request received; sending piggybacked ACK with Observe option");
    VerifyOrQuit(rsp != nullptr);
    SuccessOrQuit(otCoapMessageInitResponse(rsp, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CONTENT));
    SuccessOrQuit(otCoapMessageAppendObserveOption(rsp, 1));
    SuccessOrQuit(otCoapMessageSetPayloadMarker(rsp));
    SuccessOrQuit(otMessageAppend(rsp, "x", 1));
    SuccessOrQuit(otCoapSendResponse(&sNodeB->GetInstance(), rsp, aMessageInfo));
}

static otCoapResource sResource;

void TestCoapObserveCancelInHandler(void)
{
    Core  nexus;
    Node &nodeA = nexus.CreateNode();
    Node &nodeB = nexus.CreateNode();

    sNodeA = &nodeA;
    sNodeB = &nodeB;

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
    sResource.mUriPath = "obs";
    sResource.mHandler = HandleResource;
    otCoapAddResource(&nodeB.GetInstance(), &sResource);

    Log("A: sending observe GET (Observe=0) to B /obs");
    {
        otMessage    *req = otCoapNewMessage(&nodeA.GetInstance(), nullptr);
        otMessageInfo msgInfo;

        VerifyOrQuit(req != nullptr);
        otCoapMessageInit(req, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);
        otCoapMessageGenerateToken(req, 8);
        SuccessOrQuit(otCoapMessageAppendObserveOption(req, 0));
        SuccessOrQuit(otCoapMessageAppendUriPathOptions(req, "obs"));

        memset(&msgInfo, 0, sizeof(msgInfo));
        msgInfo.mPeerAddr = sNodeBAddr;
        msgInfo.mPeerPort = kCoapPort;

        SuccessOrQuit(otCoapSendRequest(&nodeA.GetInstance(), req, &msgInfo, HandleObserveResponse, nullptr));
    }

    nexus.AdvanceTime(10 * 1000);

    Log("notification seen: %s, cancel sent: %s", sNotificationSeen ? "YES" : "NO",
        sCancelSent ? "YES" : "NO");
    VerifyOrQuit(sNotificationSeen && sCancelSent);

    Log("TestCoapObserveCancelInHandler completed");
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCoapObserveCancelInHandler();
    printf("All tests passed\n");
    return 0;
}
