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

#if OPENTHREAD_CONFIG_COAP_API_ENABLE

#include "instance/instance.hpp"

using namespace ot;

otMessage *otCoapNewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    return AsCoreType(aInstance).Get<Coap::ApplicationCoap>().NewMessage(Message::Settings::From(aSettings));
}

otError otCoapMessageInit(otMessage *aMessage, otCoapType aType, otCoapCode aCode)
{
    return AsCoapMessage(aMessage).Init(MapEnum(aType), MapEnum(aCode));
}

otError otCoapMessageInitResponse(otMessage *aResponse, const otMessage *aRequest, otCoapType aType, otCoapCode aCode)
{
    return AsCoapMessage(aResponse).InitAsResponse(MapEnum(aType), MapEnum(aCode), AsCoapMessage(aRequest));
}

otError otCoapMessageWriteToken(otMessage *aMessage, const otCoapToken *aToken)
{
    return AsCoapMessage(aMessage).WriteToken(AsCoreType(aToken));
}

otError otCoapMessageSetToken(otMessage *aMessage, const uint8_t *aToken, uint8_t aTokenLength)
{
    Error       error;
    Coap::Token token;

    SuccessOrExit(error = token.SetToken(aToken, aTokenLength));
    error = AsCoapMessage(aMessage).WriteToken(token);

exit:
    return error;
}

void otCoapMessageGenerateToken(otMessage *aMessage, uint8_t aTokenLength)
{
    IgnoreError(AsCoapMessage(aMessage).WriteRandomToken(aTokenLength));
}

otError otCoapMessageAppendContentFormatOption(otMessage *aMessage, otCoapOptionContentFormat aContentFormat)
{
    return AsCoapMessage(aMessage).AppendContentFormatOption(aContentFormat);
}

otError otCoapMessageAppendOption(otMessage *aMessage, uint16_t aNumber, uint16_t aLength, const void *aValue)
{
    return AsCoapMessage(aMessage).AppendOption(aNumber, aLength, aValue);
}

otError otCoapMessageAppendUintOption(otMessage *aMessage, uint16_t aNumber, uint32_t aValue)
{
    return AsCoapMessage(aMessage).AppendUintOption(aNumber, aValue);
}

otError otCoapMessageAppendObserveOption(otMessage *aMessage, uint32_t aObserve)
{
    return AsCoapMessage(aMessage).AppendObserveOption(aObserve);
}

otError otCoapMessageAppendUriPathOptions(otMessage *aMessage, const char *aUriPath)
{
    return AsCoapMessage(aMessage).AppendUriPathOptions(aUriPath);
}

otError otCoapMessageAppendUriQueryOptions(otMessage *aMessage, const char *aUriQuery)
{
    return AsCoapMessage(aMessage).AppendUriQueryOptions(aUriQuery);
}

uint16_t otCoapBlockSizeFromExponent(otCoapBlockSzx aSize) { return Coap::BlockSizeFromExponent(MapEnum(aSize)); }

otError otCoapMessageAppendBlock2Option(otMessage *aMessage, uint32_t aNum, bool aMore, otCoapBlockSzx aSize)
{
    Coap::BlockInfo blockInfo;

    blockInfo.mBlockNumber = aNum;
    blockInfo.mBlockSzx    = MapEnum(aSize);
    blockInfo.mMoreBlocks  = aMore;

    return AsCoapMessage(aMessage).AppendBlockOption(Coap::kOptionBlock2, blockInfo);
}

otError otCoapMessageAppendBlock1Option(otMessage *aMessage, uint32_t aNum, bool aMore, otCoapBlockSzx aSize)
{
    Coap::BlockInfo blockInfo;

    blockInfo.mBlockNumber = aNum;
    blockInfo.mBlockSzx    = MapEnum(aSize);
    blockInfo.mMoreBlocks  = aMore;

    return AsCoapMessage(aMessage).AppendBlockOption(Coap::kOptionBlock1, blockInfo);
}

otError otCoapMessageAppendProxyUriOption(otMessage *aMessage, const char *aUriPath)
{
    return AsCoapMessage(aMessage).AppendProxyUriOption(aUriPath);
}

otError otCoapMessageAppendMaxAgeOption(otMessage *aMessage, uint32_t aMaxAge)
{
    return AsCoapMessage(aMessage).AppendMaxAgeOption(aMaxAge);
}

otError otCoapMessageAppendUriQueryOption(otMessage *aMessage, const char *aUriQuery)
{
    return AsCoapMessage(aMessage).AppendUriQueryOption(aUriQuery);
}

otError otCoapMessageSetPayloadMarker(otMessage *aMessage) { return AsCoapMessage(aMessage).AppendPayloadMarker(); }

otCoapType otCoapMessageGetType(const otMessage *aMessage)
{
    return static_cast<otCoapType>(AsCoapMessage(aMessage).ReadType());
}

otCoapCode otCoapMessageGetCode(const otMessage *aMessage)
{
    return static_cast<otCoapCode>(AsCoapMessage(aMessage).ReadCode());
}

void otCoapMessageSetCode(otMessage *aMessage, otCoapCode aCode) { AsCoapMessage(aMessage).WriteCode(MapEnum(aCode)); }

const char *otCoapMessageCodeToString(const otMessage *aMessage) { return AsCoapMessage(aMessage).CodeToString(); }

uint16_t otCoapMessageGetMessageId(const otMessage *aMessage) { return AsCoapMessage(aMessage).ReadMessageId(); }

otError otCoapMessageReadToken(const otMessage *aMessage, otCoapToken *aToken)
{
    return AsCoapMessage(aMessage).ReadToken(AsCoreType(aToken));
}

bool otCoapMessageAreTokensEqual(const otCoapToken *aFirstToken, const otCoapToken *aSecondToken)
{
    return AsCoreType(aFirstToken) == AsCoreType(aSecondToken);
}

uint8_t otCoapMessageGetTokenLength(const otMessage *aMessage)
{
    uint8_t length;

    if (AsCoapMessage(aMessage).ReadTokenLength(length) != kErrorNone)
    {
        length = 0;
    }

    return length;
}

const uint8_t *otCoapMessageGetToken(const otMessage *aMessage)
{
    static Coap::Token token;

    if (AsCoapMessage(aMessage).ReadToken(token) != kErrorNone)
    {
        token.Clear();
    }

    return token.GetBytes();
}

otError otCoapOptionIteratorInit(otCoapOptionIterator *aIterator, const otMessage *aMessage)
{
    return AsCoreType(aIterator).Init(AsCoapMessage(aMessage));
}

const otCoapOption *otCoapOptionIteratorGetFirstOptionMatching(otCoapOptionIterator *aIterator, uint16_t aOption)
{
    Coap::Option::Iterator &iterator = AsCoreType(aIterator);

    IgnoreError(iterator.Init(iterator.GetMessage(), aOption));
    return iterator.GetOption();
}

const otCoapOption *otCoapOptionIteratorGetFirstOption(otCoapOptionIterator *aIterator)
{
    Coap::Option::Iterator &iterator = AsCoreType(aIterator);

    IgnoreError(iterator.Init(iterator.GetMessage()));
    return iterator.GetOption();
}

const otCoapOption *otCoapOptionIteratorGetNextOptionMatching(otCoapOptionIterator *aIterator, uint16_t aOption)
{
    Coap::Option::Iterator &iterator = AsCoreType(aIterator);

    IgnoreError(iterator.Advance(aOption));
    return iterator.GetOption();
}

const otCoapOption *otCoapOptionIteratorGetNextOption(otCoapOptionIterator *aIterator)
{
    Coap::Option::Iterator &iterator = AsCoreType(aIterator);

    IgnoreError(iterator.Advance());
    return iterator.GetOption();
}

otError otCoapOptionIteratorGetOptionUintValue(otCoapOptionIterator *aIterator, uint64_t *aValue)
{
    return AsCoreType(aIterator).ReadOptionValue(*aValue);
}

otError otCoapOptionIteratorGetOptionValue(otCoapOptionIterator *aIterator, void *aValue)
{
    return AsCoreType(aIterator).ReadOptionValue(aValue);
}

otError otCoapSendRequestWithParameters(otInstance               *aInstance,
                                        otMessage                *aMessage,
                                        const otMessageInfo      *aMessageInfo,
                                        otCoapResponseHandler     aHandler,
                                        void                     *aContext,
                                        const otCoapTxParameters *aTxParameters)
{
    Error error;

    VerifyOrExit(!AsCoreType(aMessage).IsOriginThreadNetif(), error = kErrorInvalidArgs);

    error = AsCoreType(aInstance).Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        AsCoapMessage(aMessage), AsCoreType(aMessageInfo), AsCoreTypePtr(aTxParameters), aHandler,
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
        /* aTransmitHook */ nullptr, /* aReceiveHook */ nullptr,
#endif
        aContext);

exit:
    return error;
}

otError otCoapSendRequest(otInstance           *aInstance,
                          otMessage            *aMessage,
                          const otMessageInfo  *aMessageInfo,
                          otCoapResponseHandler aHandler,
                          void                 *aContext)
{
    return otCoapSendRequestWithParameters(aInstance, aMessage, aMessageInfo, aHandler, aContext, nullptr);
}

otError otCoapStart(otInstance *aInstance, uint16_t aPort)
{
    return AsCoreType(aInstance).Get<Coap::ApplicationCoap>().Start(aPort);
}

otError otCoapStop(otInstance *aInstance) { return AsCoreType(aInstance).Get<Coap::ApplicationCoap>().Stop(); }

void otCoapAddResource(otInstance *aInstance, otCoapResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoap>().AddResource(AsCoreType(aResource));
}

void otCoapRemoveResource(otInstance *aInstance, otCoapResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoap>().RemoveResource(AsCoreType(aResource));
}

void otCoapSetDefaultHandler(otInstance *aInstance, otCoapRequestHandler aHandler, void *aContext)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoap>().SetDefaultHandler(aHandler, aContext);
}

void otCoapSetResponseFallback(otInstance *aInstance, otCoapResponseFallback aHandler, void *aContext)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoap>().SetResponseFallback(aHandler, aContext);
}

otError otCoapSendResponseWithParameters(otInstance               *aInstance,
                                         otMessage                *aMessage,
                                         const otMessageInfo      *aMessageInfo,
                                         const otCoapTxParameters *aTxParameters)
{
    otError error;

    VerifyOrExit(!AsCoreType(aMessage).IsOriginThreadNetif(), error = kErrorInvalidArgs);

    error = AsCoreType(aInstance).Get<Coap::ApplicationCoap>().SendMessage(
        AsCoapMessage(aMessage), AsCoreType(aMessageInfo), AsCoreTypePtr(aTxParameters), nullptr, nullptr);

exit:
    return error;
}

otError otCoapSendResponse(otInstance *aInstance, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    return otCoapSendResponseWithParameters(aInstance, aMessage, aMessageInfo, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

void otCoapAddBlockWiseResource(otInstance *aInstance, otCoapBlockwiseResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoap>().AddBlockWiseResource(AsCoreType(aResource));
}

void otCoapRemoveBlockWiseResource(otInstance *aInstance, otCoapBlockwiseResource *aResource)
{
    AsCoreType(aInstance).Get<Coap::ApplicationCoap>().RemoveBlockWiseResource(AsCoreType(aResource));
}

otError otCoapSendRequestBlockWiseWithParameters(otInstance                 *aInstance,
                                                 otMessage                  *aMessage,
                                                 const otMessageInfo        *aMessageInfo,
                                                 otCoapResponseHandler       aHandler,
                                                 void                       *aContext,
                                                 const otCoapTxParameters   *aTxParameters,
                                                 otCoapBlockwiseTransmitHook aTransmitHook,
                                                 otCoapBlockwiseReceiveHook  aReceiveHook)
{
    Error error;

    VerifyOrExit(!AsCoreType(aMessage).IsOriginThreadNetif(), error = kErrorInvalidArgs);

    error = AsCoreType(aInstance).Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        AsCoapMessage(aMessage), AsCoreType(aMessageInfo), AsCoreTypePtr(aTxParameters), aHandler, aTransmitHook,
        aReceiveHook, aContext);

exit:
    return error;
}

otError otCoapSendRequestBlockWise(otInstance                 *aInstance,
                                   otMessage                  *aMessage,
                                   const otMessageInfo        *aMessageInfo,
                                   otCoapResponseHandler       aHandler,
                                   void                       *aContext,
                                   otCoapBlockwiseTransmitHook aTransmitHook,
                                   otCoapBlockwiseReceiveHook  aReceiveHook)
{
    return otCoapSendRequestBlockWiseWithParameters(aInstance, aMessage, aMessageInfo, aHandler, aContext,
                                                    /* aTxParameters */ nullptr, aTransmitHook, aReceiveHook);
}

otError otCoapSendResponseBlockWiseWithParameters(otInstance                 *aInstance,
                                                  otMessage                  *aMessage,
                                                  const otMessageInfo        *aMessageInfo,
                                                  const otCoapTxParameters   *aTxParameters,
                                                  void                       *aContext,
                                                  otCoapBlockwiseTransmitHook aTransmitHook)
{
    otError error;

    VerifyOrExit(!AsCoreType(aMessage).IsOriginThreadNetif(), error = kErrorInvalidArgs);

    error = AsCoreType(aInstance).Get<Coap::ApplicationCoap>().SendMessageWithResponseHandlerSeparateParams(
        AsCoapMessage(aMessage), AsCoreType(aMessageInfo), AsCoreTypePtr(aTxParameters), /* aResponseHandler */ nullptr,
        aTransmitHook, /* aReceiveHook */ nullptr, aContext);
exit:
    return error;
}

otError otCoapSendResponseBlockWise(otInstance                 *aInstance,
                                    otMessage                  *aMessage,
                                    const otMessageInfo        *aMessageInfo,
                                    void                       *aContext,
                                    otCoapBlockwiseTransmitHook aTransmitHook)
{
    return otCoapSendResponseBlockWiseWithParameters(aInstance, aMessage, aMessageInfo, /* aTxParamters */ nullptr,
                                                     aContext, aTransmitHook);
}

#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE
