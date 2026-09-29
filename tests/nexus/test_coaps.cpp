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

#include "cli/x509_cert_key.hpp"

namespace ot {
namespace Nexus {

static bool sRequestHandlerCalled  = false;
static bool sResponseHandlerCalled = false;

static void HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    Instance      &instance = *static_cast<Instance *>(aContext);
    Coap::Message &message  = AsCoapMessage(aMessage);
    Coap::Message *response = nullptr;

    sRequestHandlerCalled = true;

    Log("Received CoAP request");

    response = instance.Get<Coap::ApplicationCoapSecure>().NewMessage();
    VerifyOrQuit(response != nullptr);
    SuccessOrQuit(response->InitAsResponse(Coap::kTypeAck, Coap::kCodeContent, message));
    SuccessOrQuit(response->AppendPayloadMarker());
    SuccessOrQuit(response->AppendBytes("hello", 5));

    SuccessOrQuit(instance.Get<Coap::ApplicationCoapSecure>().SendMessage(*response));
}

static void HandleResponse(void *aContext, Coap::Msg *aMsg, otError aError)
{
    OT_UNUSED_VARIABLE(aContext);

    sResponseHandlerCalled = true;

    Log("Received CoAP response, error: %d", aError);
    VerifyOrQuit(aError == OT_ERROR_NONE);
    VerifyOrQuit(aMsg != nullptr);

    Coap::Message &message = static_cast<Coap::Message &>(aMsg->mMessage);
    VerifyOrQuit(message.ReadCode() == Coap::kCodeContent);

    uint16_t length = message.GetLength() - message.GetOffset();
    char     payload[10];
    VerifyOrQuit(length < sizeof(payload));
    message.ReadBytes(message.GetOffset(), payload, length);
    payload[length] = '\0';
    Log("Payload: %s", payload);
    VerifyOrQuit(strcmp(payload, "hello") == 0);
}

void TestCoapsPsk(void)
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

    const uint8_t kPsk[]         = {'p', 's', 'k'};
    const char    kPskIdentity[] = "pskIdentity";

    Log("Start CoAPS on Leader (PSK)");
    leader.Get<Coap::ApplicationCoapSecure>().SetPreSharedKey(
        kPsk, sizeof(kPsk), reinterpret_cast<const uint8_t *>(kPskIdentity), strlen(kPskIdentity));
    SuccessOrQuit(leader.Get<Coap::ApplicationCoapSecure>().Open(OT_DEFAULT_COAP_SECURE_PORT));

    Coap::Resource resource("test", &HandleRequest, &leader.GetInstance());
    leader.Get<Coap::ApplicationCoapSecure>().AddResource(resource);

    Log("Start CoAPS on Router (PSK)");
    router.Get<Coap::ApplicationCoapSecure>().SetPreSharedKey(
        kPsk, sizeof(kPsk), reinterpret_cast<const uint8_t *>(kPskIdentity), strlen(kPskIdentity));
    SuccessOrQuit(router.Get<Coap::ApplicationCoapSecure>().Open(0));

    Ip6::SockAddr sockaddr;
    sockaddr.SetAddress(leader.Get<Mle::Mle>().GetMeshLocalEid());
    sockaddr.SetPort(OT_DEFAULT_COAP_SECURE_PORT);

    Log("Connect Router to Leader");
    SuccessOrQuit(router.Get<Coap::ApplicationCoapSecure>().Connect(sockaddr));

    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(router.Get<Coap::ApplicationCoapSecure>().IsConnected());

    Log("Send GET request");
    Coap::Message *message = router.Get<Coap::ApplicationCoapSecure>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet));
    SuccessOrQuit(message->AppendUriPathOptions("test"));

    sRequestHandlerCalled  = false;
    sResponseHandlerCalled = false;

    SuccessOrQuit(router.Get<Coap::ApplicationCoapSecure>().SendMessage(*message, &HandleResponse, nullptr));

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sRequestHandlerCalled);
    VerifyOrQuit(sResponseHandlerCalled);

    Log("Disconnect");
    router.Get<Coap::ApplicationCoapSecure>().Disconnect();
    nexus.AdvanceTime(1 * 1000);
    VerifyOrQuit(!router.Get<Coap::ApplicationCoapSecure>().IsConnected());

    leader.Get<Coap::ApplicationCoapSecure>().RemoveResource(resource);
    leader.Get<Coap::ApplicationCoapSecure>().Close();
    router.Get<Coap::ApplicationCoapSecure>().Close();
}

void TestCoapsX509(void)
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

    Log("Start CoAPS on Leader (X509)");
    leader.Get<Coap::ApplicationCoapSecure>().SetCertificate(
        reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_X509_CERT), sizeof(OT_CLI_COAPS_X509_CERT),
        reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_PRIV_KEY), sizeof(OT_CLI_COAPS_PRIV_KEY));
    leader.Get<Coap::ApplicationCoapSecure>().SetCaCertificateChain(
        reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE),
        sizeof(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE));
    SuccessOrQuit(leader.Get<Coap::ApplicationCoapSecure>().Open(OT_DEFAULT_COAP_SECURE_PORT));

    Coap::Resource resource("test", &HandleRequest, &leader.GetInstance());
    leader.Get<Coap::ApplicationCoapSecure>().AddResource(resource);

    Log("Start CoAPS on Router (X509)");
    router.Get<Coap::ApplicationCoapSecure>().SetCertificate(
        reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_X509_CERT), sizeof(OT_CLI_COAPS_X509_CERT),
        reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_PRIV_KEY), sizeof(OT_CLI_COAPS_PRIV_KEY));
    router.Get<Coap::ApplicationCoapSecure>().SetCaCertificateChain(
        reinterpret_cast<const uint8_t *>(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE),
        sizeof(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE));
    SuccessOrQuit(router.Get<Coap::ApplicationCoapSecure>().Open(0));

    Ip6::SockAddr sockaddr;
    sockaddr.SetAddress(leader.Get<Mle::Mle>().GetMeshLocalEid());
    sockaddr.SetPort(OT_DEFAULT_COAP_SECURE_PORT);

    Log("Connect Router to Leader");
    SuccessOrQuit(router.Get<Coap::ApplicationCoapSecure>().Connect(sockaddr));

    nexus.AdvanceTime(5 * 1000);
    VerifyOrQuit(router.Get<Coap::ApplicationCoapSecure>().IsConnected());

    Log("Send GET request");
    Coap::Message *message = router.Get<Coap::ApplicationCoapSecure>().NewMessage();
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(Coap::kTypeConfirmable, Coap::kCodeGet));
    SuccessOrQuit(message->AppendUriPathOptions("test"));

    sRequestHandlerCalled  = false;
    sResponseHandlerCalled = false;

    SuccessOrQuit(router.Get<Coap::ApplicationCoapSecure>().SendMessage(*message, &HandleResponse, nullptr));

    nexus.AdvanceTime(5 * 1000);

    VerifyOrQuit(sRequestHandlerCalled);
    VerifyOrQuit(sResponseHandlerCalled);

    Log("Disconnect");
    router.Get<Coap::ApplicationCoapSecure>().Disconnect();
    nexus.AdvanceTime(1 * 1000);
    VerifyOrQuit(!router.Get<Coap::ApplicationCoapSecure>().IsConnected());

    leader.Get<Coap::ApplicationCoapSecure>().RemoveResource(resource);
    leader.Get<Coap::ApplicationCoapSecure>().Close();
    router.Get<Coap::ApplicationCoapSecure>().Close();
}

} // namespace Nexus
} // namespace ot

int main(void)
{
    ot::Nexus::TestCoapsPsk();
    ot::Nexus::TestCoapsX509();
    printf("All tests passed\n");
    return 0;
}
