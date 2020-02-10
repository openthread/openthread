/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

/**
 * @file
 *   This file implements the OpenThread CoAP API.
 */

#include "openthread-core-config.h"

#include <openthread/coap.h>

#include "coap/coap_message.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"

#if OPENTHREAD_CONFIG_COAP_API_ENABLE

using namespace ot;

otMessage *otCoapNewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    Message * message;
    Instance &instance = *static_cast<Instance *>(aInstance);

    if (aSettings != NULL)
    {
        VerifyOrExit(aSettings->mPriority <= OT_MESSAGE_PRIORITY_HIGH, message = NULL);
    }

    message = instance.GetApplicationCoap().NewMessage(aSettings);

exit:
    return message;
}

void otCoapMessageInit(otMessage *aMessage, otCoapType aType, otCoapCode aCode)
{
    static_cast<Coap::Message *>(aMessage)->Init(aType, aCode);
}

otError otCoapMessageInitResponse(otMessage *aResponse, const otMessage *aRequest, otCoapType aType, otCoapCode aCode)
{
    Coap::Message &      response = *static_cast<Coap::Message *>(aResponse);
    const Coap::Message &request  = *static_cast<const Coap::Message *>(aRequest);

    response.Init(aType, aCode);
    response.SetMessageId(request.GetMessageId());

    return response.SetToken(request.GetToken(), request.GetTokenLength());
}

otError otCoapMessageSetToken(otMessage *aMessage, const uint8_t *aToken, uint8_t aTokenLength)
{
    return static_cast<Coap::Message *>(aMessage)->SetToken(aToken, aTokenLength);
}

void otCoapMessageGenerateToken(otMessage *aMessage, uint8_t aTokenLength)
{
    static_cast<Coap::Message *>(aMessage)->SetToken(aTokenLength);
}

otError otCoapMessageAppendContentFormatOption(otMessage *aMessage, otCoapOptionContentFormat aContentFormat)
{
    return static_cast<Coap::Message *>(aMessage)->AppendContentFormatOption(aContentFormat);
}

otError otCoapMessageAppendOption(otMessage *aMessage, uint16_t aNumber, uint16_t aLength, const void *aValue)
{
    return static_cast<Coap::Message *>(aMessage)->AppendOption(aNumber, aLength, aValue);
}

otError otCoapMessageAppendUintOption(otMessage *aMessage, uint16_t aNumber, uint32_t aValue)
{
    return static_cast<Coap::Message *>(aMessage)->AppendUintOption(aNumber, aValue);
}

otError otCoapMessageAppendObserveOption(otMessage *aMessage, uint32_t aObserve)
{
    return static_cast<Coap::Message *>(aMessage)->AppendObserveOption(aObserve);
}

otError otCoapMessageAppendUriPathOptions(otMessage *aMessage, const char *aUriPath)
{
    return static_cast<Coap::Message *>(aMessage)->AppendUriPathOptions(aUriPath);
}

uint16_t otCoapBlockSizeFromExponent(otCoapBlockSize aSize)
{
    return static_cast<uint16_t>(1 << (aSize + Coap::Message::kBlockSzxBase));
}

otError otCoapMessageAppendBlock2Option(otMessage *aMessage, uint32_t aNum, bool aMore, otCoapBlockSize aSize)
{
    return static_cast<Coap::Message *>(aMessage)->AppendBlockOption(Coap::Message::kBlockType2, aNum, aMore, aSize);
}

otError otCoapMessageAppendBlock1Option(otMessage *aMessage, uint32_t aNum, bool aMore, otCoapBlockSize aSize)
{
    return static_cast<Coap::Message *>(aMessage)->AppendBlockOption(Coap::Message::kBlockType1, aNum, aMore, aSize);
}

otError otCoapMessageAppendProxyUriOption(otMessage *aMessage, const char *aUriPath)
{
    return static_cast<Coap::Message *>(aMessage)->AppendProxyUriOption(aUriPath);
}

otError otCoapMessageAppendMaxAgeOption(otMessage *aMessage, uint32_t aMaxAge)
{
    return static_cast<Coap::Message *>(aMessage)->AppendMaxAgeOption(aMaxAge);
}

otError otCoapMessageAppendUriQueryOption(otMessage *aMessage, const char *aUriQuery)
{
    return static_cast<Coap::Message *>(aMessage)->AppendUriQueryOption(aUriQuery);
}

otError otCoapMessageSetPayloadMarker(otMessage *aMessage)
{
    return static_cast<Coap::Message *>(aMessage)->SetPayloadMarker();
}

otCoapType otCoapMessageGetType(const otMessage *aMessage)
{
    return static_cast<const Coap::Message *>(aMessage)->GetType();
}

otCoapCode otCoapMessageGetCode(const otMessage *aMessage)
{
    return static_cast<const Coap::Message *>(aMessage)->GetCode();
}

const char *otCoapMessageCodeToString(const otMessage *aMessage)
{
    return static_cast<const Coap::Message *>(aMessage)->CodeToString();
}

uint16_t otCoapMessageGetMessageId(const otMessage *aMessage)
{
    return static_cast<const Coap::Message *>(aMessage)->GetMessageId();
}

uint8_t otCoapMessageGetTokenLength(const otMessage *aMessage)
{
    return static_cast<const Coap::Message *>(aMessage)->GetTokenLength();
}

const uint8_t *otCoapMessageGetToken(const otMessage *aMessage)
{
    return static_cast<const Coap::Message *>(aMessage)->GetToken();
}

otError otCoapOptionIteratorInit(otCoapOptionIterator *aIterator, const otMessage *aMessage)
{
    return static_cast<Coap::OptionIterator *>(aIterator)->Init(static_cast<const Coap::Message *>(aMessage));
}

const otCoapOption *otCoapOptionIteratorGetFirstOption(otCoapOptionIterator *aIterator)
{
    return static_cast<Coap::OptionIterator *>(aIterator)->GetFirstOption();
}

const otCoapOption *otCoapOptionIteratorGetNextOption(otCoapOptionIterator *aIterator)
{
    return static_cast<Coap::OptionIterator *>(aIterator)->GetNextOption();
}

otError otCoapOptionIteratorGetOptionValue(otCoapOptionIterator *aIterator, void *aValue)
{
    return static_cast<Coap::OptionIterator *>(aIterator)->GetOptionValue(aValue);
}

otError otCoapSendRequest(otInstance *          aInstance,
                          otMessage *           aMessage,
                          const otMessageInfo * aMessageInfo,
                          otCoapResponseHandler aHandler,
                          void *                aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoap().SendMessage(*static_cast<Coap::Message *>(aMessage),
                                                     *static_cast<const Ip6::MessageInfo *>(aMessageInfo), aHandler,
                                                     aContext);
}

otError otCoapStart(otInstance *aInstance, uint16_t aPort)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoap().Start(aPort);
}

otError otCoapStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoap().Stop();
}

otError otCoapAddResource(otInstance *aInstance, otCoapResource *aResource)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoap().AddResource(*static_cast<Coap::Resource *>(aResource));
}

void otCoapRemoveResource(otInstance *aInstance, otCoapResource *aResource)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoap().RemoveResource(*static_cast<Coap::Resource *>(aResource));
}

void otCoapSetDefaultHandler(otInstance *aInstance, otCoapRequestHandler aHandler, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetApplicationCoap().SetDefaultHandler(aHandler, aContext);
}

otError otCoapSendResponse(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetApplicationCoap().SendMessage(*static_cast<Coap::Message *>(aMessage),
                                                     *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE
