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

static bool     sRequestHandlerCalled = false;
static bool     sNotificationReceived = false;
static uint32_t sObserveValue         = 0;
static char     sReceivedPayload[32];
static bool     sSubscriptionActive = false;

static Coap::Token  sSubscriberToken;
static otIp6Address sSubscriberAddr;
static uint16_t     sSubscriberPort;
static bool         sSubscriberPresent = false;

static void HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Instance      &instance = *static_cast<Instance *>(aContext);
    Coap::Message &message  = AsCoapMessage(aMessage);
    Coap::Code     code     = static_cast<Coap::Code>(message.ReadCode());
    Coap::Message *response = nullptr;

    Log("HandleRequest called");

    sRequestHandlerCalled = true;

    if (code == Coap::kCodeGet)
    {
        response = instance.Get<Coap::ApplicationCoap>().NewMessage();
        VerifyOrQuit(response != nullptr);

        SuccessOrQuit(response->InitAsResponse(Coap::kTypeAck, Coap::kCodeContent, message));

        Coap::Option::Iterator iterator;
        SuccessOrQuit(iterator.Init(message, Coap::kOptionObserve));
        if (iterator.GetOption() != nullptr)
        {
            uint64_t observe = 0;
            SuccessOrQuit(iterator.ReadOptionValue(observe));

            if (observe == 0)
            {
                // New subscriber
                sSubscriberAddr = aMessageInfo->mPeerAddr;
                sSubscriberPort = aMessageInfo->mPeerPort;
                SuccessOrQuit(message.ReadToken(sSubscriberToken));
                sSubscriberPresent = true;

                // Append Observe option to response
                SuccessOrQuit(response->AppendObserveOption(0));
            }
            else if (observe == 1)
            {
                // Cancel subscription
                sSubscriberPresent = false;
            }
        }

        SuccessOrQuit(response->AppendPayloadMarker());
        SuccessOrQuit(response->AppendBytes("Test123", 7));

        SuccessOrQuit(instance.Get<Coap::ApplicationCoap>().SendMessage(*response, AsCoreType(aMessageInfo)));
    }
}

static void HandleNotification(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessageInfo);

    Log("HandleNotification called with result %d", aResult);

    if (!sSubscriptionActive)
    {
        Log("Subscription inactive, ignoring message");
        return;
    }

    if (aResult != OT_ERROR_NONE)
    {
        return;
    }

    sNotificationReceived = true;

    uint16_t length = AsCoapMessage(aMessage).GetLength() - AsCoapMessage(aMessage).GetOffset();
    Log("Message length: %u, offset: %u", AsCoapMessage(aMessage).GetLength(), AsCoapMessage(aMessage).GetOffset());

    if (length > 0)
    {
        length = Min(length, static_cast<uint16_t>(sizeof(sReceivedPayload) - 1));
        SuccessOrQuit(AsCoapMessage(aMessage).Read(AsCoapMessage(aMessage).GetOffset(), sReceivedPayload, length));
        sReceivedPayload[length] = '\0';
    }

    // Only check options if it's a notification (payload starts with "msg")
    if (strncmp(sReceivedPayload, "msg", 3) == 0)
    {
        Coap::Option::Iterator iterator;
        SuccessOrQuit(iterator.Init(AsCoapMessage(aMessage), Coap::kOptionObserve));
        if (iterator.GetOption() != nullptr)
        {
            uint64_t observe = 0;
            SuccessOrQuit(iterator.ReadOptionValue(observe));
            sObserveValue = static_cast<uint32_t>(observe);
        }
    }
}

void TestCoapObserve(void)
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

    Coap::Resource resource("test", &HandleRequest, &leader.GetInstance());
    leader.Get<Coap::ApplicationCoap>().AddResource(resource);

    // Start CoAP on Router
    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().Start(OT_DEFAULT_COAP_PORT));

    // Router sends Observe request
    Coap::Message *message = router.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet));
    SuccessOrQuit(message->AppendObserveOption(0));
    SuccessOrQuit(message->AppendUriPathOptions("test"));

    Coap::Token token;
    SuccessOrQuit(token.SetToken(reinterpret_cast<const uint8_t *>("12345678"), 8));
    IgnoreError(message->WriteToken(token));

    Ip6::MessageInfo messageInfo;
    messageInfo.SetPeerAddr(leader.Get<Mle::Mle>().GetMeshLocalEid());
    messageInfo.SetPeerPort(OT_DEFAULT_COAP_PORT);

    sRequestHandlerCalled = false;
    sSubscriberPresent    = false;
    sSubscriptionActive   = true;

    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *message, messageInfo, nullptr, &HandleNotification, nullptr, nullptr, nullptr));

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sRequestHandlerCalled);
    VerifyOrQuit(sSubscriberPresent);
    VerifyOrQuit(sNotificationReceived); // Initial response acts as first notification
    VerifyOrQuit(strcmp(sReceivedPayload, "Test123") == 0);

    // Now Leader sends a notification
    sNotificationReceived = false;

    Coap::Message *notification = leader.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(notification != nullptr);
    SuccessOrQuit(notification->Init(Coap::kTypeNonConfirmable, Coap::kCodeContent));
    SuccessOrQuit(notification->WriteToken(sSubscriberToken));
    SuccessOrQuit(notification->AppendObserveOption(1));
    SuccessOrQuit(notification->AppendPayloadMarker());
    SuccessOrQuit(notification->AppendBytes("msg0", 4));

    messageInfo.SetPeerAddr(AsCoreType(&sSubscriberAddr));
    messageInfo.SetPeerPort(sSubscriberPort);

    SuccessOrQuit(leader.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *notification, messageInfo, nullptr, nullptr, nullptr, nullptr, nullptr));

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sNotificationReceived);
    VerifyOrQuit(sObserveValue == 1);
    VerifyOrQuit(strcmp(sReceivedPayload, "msg0") == 0);

    // Router cancels subscription
    message = router.Get<Coap::ApplicationCoap>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet));
    SuccessOrQuit(message->WriteToken(sSubscriberToken)); // Use same token
    SuccessOrQuit(message->AppendObserveOption(1));       // Observe=1 means cancel
    SuccessOrQuit(message->AppendUriPathOptions("test"));

    sRequestHandlerCalled = false;
    sSubscriptionActive   = false;

    messageInfo.SetPeerAddr(leader.Get<Mle::Mle>().GetMeshLocalEid());
    messageInfo.SetPeerPort(OT_DEFAULT_COAP_PORT);

    SuccessOrQuit(router.Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        *message, messageInfo, nullptr, &HandleNotification, nullptr, nullptr, nullptr));

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sRequestHandlerCalled);
    VerifyOrQuit(!sSubscriberPresent);

    leader.Get<Coap::ApplicationCoap>().RemoveResource(resource);
    IgnoreError(leader.Get<Coap::ApplicationCoap>().Stop());
    IgnoreError(router.Get<Coap::ApplicationCoap>().Stop());
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCoapObserve();
    printf("All tests passed\n");
    return 0;
}
