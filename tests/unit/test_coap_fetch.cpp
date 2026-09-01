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

#include "coap/coap.hpp"
#include "instance/instance.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {
#if OPENTHREAD_CONFIG_COAP_API_ENABLE || OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
namespace {

static uint8_t    sTransmitCount;
static uint16_t   sMessageId;
static Coap::Code sTransmitCode;
static Coap::Type sTransmitType;
static uint8_t    sDefaultHandlerCount;

class TestCoap : public Coap::CoapBase
{
public:
    explicit TestCoap(Instance &aInstance)
        : CoapBase(aInstance, Transmit)
    {
    }

    void HandleReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo) { Receive(aMessage, aMessageInfo); }

private:
    static Error Transmit(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
    {
        OT_UNUSED_VARIABLE(aCoapBase);
        OT_UNUSED_VARIABLE(aMessageInfo);

        Coap::Message &message = AsCoapMessage(&aMessage);

        sTransmitCount++;
        sTransmitCode = static_cast<Coap::Code>(message.ReadCode());
        sTransmitType = static_cast<Coap::Type>(message.ReadType());
        aMessage.Free();

        return kErrorNone;
    }
};

static void HandleDefault(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    sDefaultHandlerCount++;
}

static void SendFetch(TestCoap           &aCoap,
                      Instance           &aInstance,
                      Coap::Type          aType,
                      bool                aAddContentFormat,
                      bool                aAddOscore,
                      const Ip6::Address &aDestination)
{
    static const uint8_t kOscoreOptionValue[] = {0x09, 0x05, 0x25};
    static const uint8_t kPayload[]           = {0x01};

    Coap::Message   *message = AsCoapMessagePtr(aInstance.Get<MessagePool>().Allocate(Message::kTypeIp6));
    Ip6::MessageInfo messageInfo;

    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->Init(aType, Coap::kCodeFetch, sMessageId++));

    if (aAddOscore)
    {
        SuccessOrQuit(message->AppendOption(Coap::kOptionOscore, sizeof(kOscoreOptionValue), kOscoreOptionValue));
    }
    else
    {
        SuccessOrQuit(message->AppendUriPathOptions("query"));
    }

    if (aAddContentFormat)
    {
        SuccessOrQuit(message->AppendContentFormatOption(OT_COAP_OPTION_CONTENT_FORMAT_JSON));
    }

    SuccessOrQuit(message->AppendPayloadMarker());
    SuccessOrQuit(message->AppendBytes(kPayload, sizeof(kPayload)));

    messageInfo.SetSockAddr(aDestination);

    aCoap.HandleReceive(*message, messageInfo);
    message->Free();
}

} // namespace
#endif

class UnitTester
{
public:
    static void TestCoapFetchValidation(void)
    {
#if OPENTHREAD_CONFIG_COAP_API_ENABLE || OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
        Instance *instance = static_cast<Instance *>(testInitInstance());

        VerifyOrQuit(instance != nullptr);

        {
            TestCoap coap(*instance);
            coap.SetDefaultHandler(HandleDefault, nullptr);

            Ip6::Address unicastAddress;
            Ip6::Address multicastAddress = Ip6::Address::GetRealmLocalAllNodesMulticast();

            sTransmitCount       = 0;
            sMessageId           = 1;
            sDefaultHandlerCount = 0;
            sTransmitCode        = Coap::kCodeEmpty;
            sTransmitType        = Coap::kTypeReset;

            printf("TestCoapFetchValidation()\n");

            // A unicast FETCH without either required option gets an Unsupported Content-Format response.
            SendFetch(coap, *instance, Coap::kTypeConfirmable, false, false, unicastAddress);
            VerifyOrQuit(sTransmitCount == 1);
            VerifyOrQuit(sTransmitCode == Coap::kCodeUnsupportedFormat);
            VerifyOrQuit(sTransmitType == Coap::kTypeAck);
            VerifyOrQuit(sDefaultHandlerCount == 0);

            // The same invalid NON request gets a NON response.
            SendFetch(coap, *instance, Coap::kTypeNonConfirmable, false, false, unicastAddress);
            VerifyOrQuit(sTransmitCount == 2);
            VerifyOrQuit(sTransmitCode == Coap::kCodeUnsupportedFormat);
            VerifyOrQuit(sTransmitType == Coap::kTypeNonConfirmable);
            VerifyOrQuit(sDefaultHandlerCount == 0);

            // Multicast requests without the required options are silently dropped.
            SendFetch(coap, *instance, Coap::kTypeNonConfirmable, false, false, multicastAddress);
            VerifyOrQuit(sTransmitCount == 2);
            VerifyOrQuit(sDefaultHandlerCount == 0);

            // Either Content-Format or OSCORE is sufficient to continue normal request processing.
            SendFetch(coap, *instance, Coap::kTypeNonConfirmable, true, false, unicastAddress);
            VerifyOrQuit(sTransmitCount == 2);
            VerifyOrQuit(sDefaultHandlerCount == 1);

            SendFetch(coap, *instance, Coap::kTypeNonConfirmable, false, true, unicastAddress);
            VerifyOrQuit(sTransmitCount == 2);
            VerifyOrQuit(sDefaultHandlerCount == 2);

            coap.ClearAllRequestsAndResponses();
        }

        testFreeInstance(instance);
#endif
    }
};

} // namespace ot

int main(void)
{
    ot::UnitTester::TestCoapFetchValidation();
    printf("All tests passed\n");
    return 0;
}
