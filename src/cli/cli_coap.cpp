/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file implements a simple CLI for the CoAP service.
 */

#include "cli_coap.hpp"

#if OPENTHREAD_CONFIG_COAP_API_ENABLE

#include <ctype.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
#include "coap/coap_message.hpp"

namespace ot {
namespace Cli {

const struct Coap::Command Coap::sCommands[] = {
    {"help", &Coap::ProcessHelp},
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    {"cancel", &Coap::ProcessCancel},
#endif
    {"delete", &Coap::ProcessRequest},
    {"get", &Coap::ProcessRequest},
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    {"observe", &Coap::ProcessRequest},
#endif
    {"parameters", &Coap::ProcessParameters},
    {"post", &Coap::ProcessRequest},
    {"put", &Coap::ProcessRequest},
    {"resource", &Coap::ProcessResource},
    {"set", &Coap::ProcessSet},
    {"start", &Coap::ProcessStart},
    {"stop", &Coap::ProcessStop},
};

Coap::Coap(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
    , mUseDefaultRequestTxParameters(true)
    , mUseDefaultResponseTxParameters(true)
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    , mObserveSerial(0)
    , mRequestTokenLength(0)
    , mSubscriberTokenLength(0)
    , mSubscriberConfirmableNotifications(false)
#endif
{
    memset(&mResource, 0, sizeof(mResource));
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    memset(&mRequestAddr, 0, sizeof(mRequestAddr));
    memset(&mSubscriberSock, 0, sizeof(mSubscriberSock));
    memset(&mRequestToken, 0, sizeof(mRequestToken));
    memset(&mSubscriberToken, 0, sizeof(mSubscriberToken));
    memset(&mRequestUri, 0, sizeof(mRequestUri));
#endif
    memset(&mUriPath, 0, sizeof(mUriPath));
    strncpy(mResourceContent, "0", sizeof(mResourceContent) - 1);
}

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
otError Coap::CancelResourceSubscription(void)
{
    otError       error   = OT_ERROR_NONE;
    otMessage *   message = NULL;
    otMessageInfo messageInfo;

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = mRequestAddr;
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

    VerifyOrExit(mRequestTokenLength != 0, error = OT_ERROR_INVALID_STATE);

    message = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_GET);

    SuccessOrExit(error = otCoapMessageSetToken(message, mRequestToken, mRequestTokenLength));
    SuccessOrExit(error = otCoapMessageAppendObserveOption(message, 1));
    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, mRequestUri));
    SuccessOrExit(error =
                      otCoapSendRequest(mInterpreter.mInstance, message, &messageInfo, &Coap::HandleResponse, this));

    memset(&mRequestAddr, 0, sizeof(mRequestAddr));
    memset(&mRequestUri, 0, sizeof(mRequestUri));
    mRequestTokenLength = 0;

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return error;
}

void Coap::CancelSubscriber(void)
{
    memset(&mSubscriberSock, 0, sizeof(mSubscriberSock));
    mSubscriberTokenLength = 0;
}
#endif // OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE

void Coap::PrintPayload(otMessage *aMessage) const
{
    uint8_t  buf[kMaxBufferSize];
    uint16_t bytesToPrint;
    uint16_t bytesPrinted = 0;
    uint16_t length       = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    if (length > 0)
    {
        mInterpreter.mServer->OutputFormat(" with payload: ");

        while (length > 0)
        {
            bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
            otMessageRead(aMessage, otMessageGetOffset(aMessage) + bytesPrinted, buf, bytesToPrint);

            mInterpreter.OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

            length -= bytesToPrint;
            bytesPrinted += bytesToPrint;
        }
    }

    mInterpreter.mServer->OutputFormat("\r\n");
}

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
otError Coap::ProcessCancel(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    return CancelResourceSubscription();
}
#endif

otError Coap::ProcessHelp(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError Coap::ProcessResource(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgsLength > 1)
    {
        VerifyOrExit(strlen(aArgs[1]) < kMaxUriLength, error = OT_ERROR_INVALID_ARGS);

        mResource.mUriPath = mUriPath;
        mResource.mContext = this;
        mResource.mHandler = &Coap::HandleRequest;

        strncpy(mUriPath, aArgs[1], sizeof(mUriPath) - 1);
        otCoapAddResource(mInterpreter.mInstance, &mResource);
    }
    else
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", mResource.mUriPath);
    }

exit:
    return error;
}

otError Coap::ProcessSet(uint8_t aArgsLength, char *aArgs[])
{
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    otMessage *   notificationMessage = NULL;
    otMessageInfo messageInfo;
#endif
    otError error = OT_ERROR_NONE;

    if (aArgsLength > 1)
    {
        VerifyOrExit(strlen(aArgs[1]) < (kMaxBufferSize - 1), error = OT_ERROR_INVALID_ARGS);
        strncpy(mResourceContent, aArgs[1], sizeof(mResourceContent) - 1);

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        if (mSubscriberTokenLength > 0)
        {
            // Notify the subscriber
            memset(&messageInfo, 0, sizeof(messageInfo));
            messageInfo.mPeerAddr = mSubscriberSock.mAddress;
            messageInfo.mPeerPort = mSubscriberSock.mPort;

            mInterpreter.mServer->OutputFormat("sending coap notification to ");
            mInterpreter.OutputIp6Address(mSubscriberSock.mAddress);
            mInterpreter.mServer->OutputFormat("\r\n");

            notificationMessage = otCoapNewMessage(mInterpreter.mInstance, NULL);
            VerifyOrExit(notificationMessage != NULL, error = OT_ERROR_NO_BUFS);

            otCoapMessageInit(
                notificationMessage,
                ((mSubscriberConfirmableNotifications) ? OT_COAP_TYPE_CONFIRMABLE : OT_COAP_TYPE_NON_CONFIRMABLE),
                OT_COAP_CODE_CONTENT);

            SuccessOrExit(error = otCoapMessageSetToken(notificationMessage, mSubscriberToken, mSubscriberTokenLength));
            SuccessOrExit(error = otCoapMessageAppendObserveOption(notificationMessage, mObserveSerial++));
            SuccessOrExit(error = otCoapMessageSetPayloadMarker(notificationMessage));
            SuccessOrExit(error = otMessageAppend(notificationMessage, mResourceContent,
                                                  static_cast<uint16_t>(strlen(mResourceContent))));

            SuccessOrExit(error = otCoapSendRequest(mInterpreter.mInstance, notificationMessage, &messageInfo,
                                                    &Coap::HandleNotificationResponse, this));
        }
#endif // OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    }
    else
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", mResourceContent);
    }

exit:

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    if ((error != OT_ERROR_NONE) && (notificationMessage != NULL))
    {
        otMessageFree(notificationMessage);
    }
#endif

    return error;
}

otError Coap::ProcessStart(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    return otCoapStart(mInterpreter.mInstance, OT_DEFAULT_COAP_PORT);
}

otError Coap::ProcessStop(uint8_t aArgsLength, char *aArgs[])
{
    OT_UNUSED_VARIABLE(aArgsLength);
    OT_UNUSED_VARIABLE(aArgs);

    otCoapRemoveResource(mInterpreter.mInstance, &mResource);

    return otCoapStop(mInterpreter.mInstance);
}

otError Coap::ProcessParameters(uint8_t aArgsLength, char *aArgs[])
{
    otError             error = OT_ERROR_NONE;
    bool *              defaultTxParameters;
    otCoapTxParameters *txParameters;

    VerifyOrExit(aArgsLength > 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[1], "request") == 0)
    {
        txParameters        = &mRequestTxParameters;
        defaultTxParameters = &mUseDefaultRequestTxParameters;
    }
    else if (strcmp(aArgs[1], "response") == 0)
    {
        txParameters        = &mResponseTxParameters;
        defaultTxParameters = &mUseDefaultResponseTxParameters;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    if (aArgsLength > 2)
    {
        if (strcmp(aArgs[2], "default") == 0)
        {
            *defaultTxParameters = true;
        }
        else
        {
            unsigned long value;

            VerifyOrExit(aArgsLength >= 6, error = OT_ERROR_INVALID_ARGS);

            SuccessOrExit(error = mInterpreter.ParseUnsignedLong(aArgs[2], value));
            txParameters->mAckTimeout = static_cast<uint32_t>(value);

            SuccessOrExit(error = mInterpreter.ParseUnsignedLong(aArgs[3], value));
            VerifyOrExit(value <= 255, error = OT_ERROR_INVALID_ARGS);
            txParameters->mAckRandomFactorNumerator = static_cast<uint8_t>(value);

            SuccessOrExit(error = mInterpreter.ParseUnsignedLong(aArgs[4], value));
            VerifyOrExit(value <= 255, error = OT_ERROR_INVALID_ARGS);
            txParameters->mAckRandomFactorDenominator = static_cast<uint8_t>(value);

            SuccessOrExit(error = mInterpreter.ParseUnsignedLong(aArgs[5], value));
            VerifyOrExit(value <= 255, error = OT_ERROR_INVALID_ARGS);
            txParameters->mMaxRetransmit = static_cast<uint8_t>(value);

            VerifyOrExit(txParameters->mAckRandomFactorNumerator > txParameters->mAckRandomFactorDenominator,
                         error = OT_ERROR_INVALID_ARGS);

            *defaultTxParameters = false;
        }
    }

    mInterpreter.mServer->OutputFormat("Transmission parameters for %s:\r\n", aArgs[1]);
    if (*defaultTxParameters)
    {
        mInterpreter.mServer->OutputFormat("default\r\n");
    }
    else
    {
        mInterpreter.mServer->OutputFormat("ACK_TIMEOUT=%u ms, ACK_RANDOM_FACTOR=%u/%u, MAX_RETRANSMIT=%u\r\n",
                                           txParameters->mAckTimeout, txParameters->mAckRandomFactorNumerator,
                                           txParameters->mAckRandomFactorDenominator, txParameters->mMaxRetransmit);
    }

exit:
    return error;
}

otError Coap::ProcessRequest(uint8_t aArgsLength, char *aArgs[])
{
    otError       error   = OT_ERROR_NONE;
    otMessage *   message = NULL;
    otMessageInfo messageInfo;
    uint16_t      payloadLength = 0;

    // Default parameters
    char         coapUri[kMaxUriLength] = "test";
    otCoapType   coapType               = OT_COAP_TYPE_NON_CONFIRMABLE;
    otCoapCode   coapCode               = OT_COAP_CODE_GET;
    otIp6Address coapDestinationIp;
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    bool coapObserve = false;
#endif

    VerifyOrExit(aArgsLength > 0, error = OT_ERROR_INVALID_ARGS);

    // CoAP-Code
    if (strcmp(aArgs[0], "get") == 0)
    {
        coapCode = OT_COAP_CODE_GET;
    }
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    else if (strcmp(aArgs[0], "observe") == 0)
    {
        // Observe request.  This is a GET with Observe=0
        coapCode    = OT_COAP_CODE_GET;
        coapObserve = true;
    }
#endif
    else if (strcmp(aArgs[0], "post") == 0)
    {
        coapCode = OT_COAP_CODE_POST;
    }
    else if (strcmp(aArgs[0], "put") == 0)
    {
        coapCode = OT_COAP_CODE_PUT;
    }
    else if (strcmp(aArgs[0], "delete") == 0)
    {
        coapCode = OT_COAP_CODE_DELETE;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // Destination IPv6 address
    if (aArgsLength > 1)
    {
        SuccessOrExit(error = otIp6AddressFromString(aArgs[1], &coapDestinationIp));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // CoAP-URI
    if (aArgsLength > 2)
    {
        VerifyOrExit(strlen(aArgs[2]) < kMaxUriLength, error = OT_ERROR_INVALID_ARGS);
        strncpy(coapUri, aArgs[2], sizeof(coapUri) - 1);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // CoAP-Type
    if (aArgsLength > 3)
    {
        if (strcmp(aArgs[3], "con") == 0)
        {
            coapType = OT_COAP_TYPE_CONFIRMABLE;
        }
    }

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    if (coapObserve && mRequestTokenLength)
    {
        // New observe request, cancel any existing observation
        SuccessOrExit(error = CancelResourceSubscription());
    }
#endif

    message = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, coapType, coapCode);
    otCoapMessageGenerateToken(message, ot::Coap::Message::kDefaultTokenLength);

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    if (coapObserve)
    {
        SuccessOrExit(error = otCoapMessageAppendObserveOption(message, 0));
    }
#endif

    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, coapUri));

    if (aArgsLength > 4)
    {
        payloadLength = static_cast<uint16_t>(strlen(aArgs[4]));

        if (payloadLength > 0)
        {
            SuccessOrExit(error = otCoapMessageSetPayloadMarker(message));
        }
    }

    // Embed content into message if given
    if (payloadLength > 0)
    {
        SuccessOrExit(error = otMessageAppend(message, aArgs[4], payloadLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = coapDestinationIp;
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    if (coapObserve)
    {
        // Make a note of the message details for later so we can cancel it later.
        memcpy(&mRequestAddr, &coapDestinationIp, sizeof(mRequestAddr));
        mRequestTokenLength = otCoapMessageGetTokenLength(message);
        memcpy(mRequestToken, otCoapMessageGetToken(message), mRequestTokenLength);
        strncpy(mRequestUri, coapUri, sizeof(mRequestUri) - 1);
        mRequestUri[sizeof(mRequestUri) - 1] = '\0'; // Fix gcc-9.2 warning
    }
#endif

    if ((coapType == OT_COAP_TYPE_CONFIRMABLE) || (coapCode == OT_COAP_CODE_GET))
    {
        error = otCoapSendRequestWithParameters(mInterpreter.mInstance, message, &messageInfo, &Coap::HandleResponse,
                                                this, GetRequestTxParameters());
    }
    else
    {
        error = otCoapSendRequestWithParameters(mInterpreter.mInstance, message, &messageInfo, NULL, NULL,
                                                GetResponseTxParameters());
    }

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return error;
}

otError Coap::Process(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    if (aArgsLength < 1)
    {
        IgnoreError(ProcessHelp(0, NULL));
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(aArgs[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(aArgsLength, aArgs);
                break;
            }
        }
    }

    return error;
}

void Coap::HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Coap *>(aContext)->HandleRequest(aMessage, aMessageInfo);
}

void Coap::HandleRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error           = OT_ERROR_NONE;
    otMessage *responseMessage = NULL;
    otCoapCode responseCode    = OT_COAP_CODE_EMPTY;
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    uint64_t             observe        = 0;
    bool                 observePresent = false;
    otCoapOptionIterator iterator;
#endif

    mInterpreter.mServer->OutputFormat("coap request from ");
    mInterpreter.OutputIp6Address(aMessageInfo->mPeerAddr);
    mInterpreter.mServer->OutputFormat(" ");

    switch (otCoapMessageGetCode(aMessage))
    {
    case OT_COAP_CODE_GET:
        mInterpreter.mServer->OutputFormat("GET");
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        SuccessOrExit(error = otCoapOptionIteratorInit(&iterator, aMessage));
        if (otCoapOptionIteratorGetFirstOptionMatching(&iterator, OT_COAP_OPTION_OBSERVE) != NULL)
        {
            SuccessOrExit(error = otCoapOptionIteratorGetOptionUintValue(&iterator, &observe));
            observePresent = true;

            mInterpreter.mServer->OutputFormat(" OBS=%lu", static_cast<uint32_t>(observe));
        }
#endif
        break;

    case OT_COAP_CODE_DELETE:
        mInterpreter.mServer->OutputFormat("DELETE");
        break;

    case OT_COAP_CODE_PUT:
        mInterpreter.mServer->OutputFormat("PUT");
        break;

    case OT_COAP_CODE_POST:
        mInterpreter.mServer->OutputFormat("POST");
        break;

    default:
        mInterpreter.mServer->OutputFormat("Undefined\r\n");
        ExitNow(error = OT_ERROR_PARSE);
    }

    PrintPayload(aMessage);

    if (otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE ||
        otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
    {
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        if (observePresent && (mSubscriberTokenLength > 0) && (observe == 0))
        {
            // There is already a subscriber
            responseCode = OT_COAP_CODE_SERVICE_UNAVAILABLE;
        }
        else
#endif
            if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            responseCode = OT_COAP_CODE_CONTENT;
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (observePresent)
            {
                if (observe == 0)
                {
                    // New subscriber
                    mInterpreter.mServer->OutputFormat("Subscribing client\r\n");
                    mSubscriberSock.mAddress = aMessageInfo->mPeerAddr;
                    mSubscriberSock.mPort    = aMessageInfo->mPeerPort;
                    mSubscriberTokenLength   = otCoapMessageGetTokenLength(aMessage);
                    memcpy(mSubscriberToken, otCoapMessageGetToken(aMessage), mSubscriberTokenLength);

                    /*
                     * Implementer note.
                     *
                     * Here, we try to match a confirmable GET request with confirmable
                     * notifications, however this is not a requirement of RFC7641:
                     * the server can send notifications of either type regardless of
                     * what the client used to subscribe initially.
                     */
                    mSubscriberConfirmableNotifications = (otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE);
                }
                else if (observe == 1)
                {
                    // See if it matches our subscriber token
                    if ((otCoapMessageGetTokenLength(aMessage) == mSubscriberTokenLength) &&
                        (memcmp(otCoapMessageGetToken(aMessage), mSubscriberToken, mSubscriberTokenLength) == 0))
                    {
                        // Unsubscribe request
                        CancelSubscriber();
                    }
                }
            }
#endif // OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        }
        else
        {
            responseCode = OT_COAP_CODE_VALID;
        }

        responseMessage = otCoapNewMessage(mInterpreter.mInstance, NULL);
        VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(
            error = otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode));

        if (responseCode == OT_COAP_CODE_CONTENT)
        {
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (observePresent && (observe == 0))
            {
                SuccessOrExit(error = otCoapMessageAppendObserveOption(responseMessage, mObserveSerial++));
            }
#endif
            SuccessOrExit(error = otCoapMessageSetPayloadMarker(responseMessage));
            SuccessOrExit(error = otMessageAppend(responseMessage, mResourceContent,
                                                  static_cast<uint16_t>(strlen(mResourceContent))));
        }

        SuccessOrExit(error = otCoapSendResponseWithParameters(mInterpreter.mInstance, responseMessage, aMessageInfo,
                                                               GetResponseTxParameters()));
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        if (responseMessage != NULL)
        {
            mInterpreter.mServer->OutputFormat("coap send response error %d: %s\r\n", error,
                                               otThreadErrorToString(error));
            otMessageFree(responseMessage);
        }
    }
    else if (responseCode >= OT_COAP_CODE_RESPONSE_MIN)
    {
        mInterpreter.mServer->OutputFormat("coap response sent\r\n");
    }
}

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
void Coap::HandleNotificationResponse(void *               aContext,
                                      otMessage *          aMessage,
                                      const otMessageInfo *aMessageInfo,
                                      otError              aError)
{
    static_cast<Coap *>(aContext)->HandleNotificationResponse(aMessage, aMessageInfo, aError);
}

void Coap::HandleNotificationResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    OT_UNUSED_VARIABLE(aMessage);

    switch (aError)
    {
    case OT_ERROR_NONE:
        if (aMessageInfo != NULL)
        {
            mInterpreter.mServer->OutputFormat("Received ACK in reply to notification from ");
            mInterpreter.OutputIp6Address(aMessageInfo->mPeerAddr);
            mInterpreter.mServer->OutputFormat("\r\n");
        }
        break;

    default:
        mInterpreter.mServer->OutputFormat("coap receive notification response error %d: %s\r\n", aError,
                                           otThreadErrorToString(aError));
        CancelSubscriber();
        break;
    }
}
#endif // OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE

void Coap::HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    static_cast<Coap *>(aContext)->HandleResponse(aMessage, aMessageInfo, aError);
}

void Coap::HandleResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("coap receive response error %d: %s\r\n", aError,
                                           otThreadErrorToString(aError));
    }
    else if ((aMessageInfo != NULL) && (aMessage != NULL))
    {
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        otCoapOptionIterator iterator;
#endif

        mInterpreter.mServer->OutputFormat("coap response from ");
        mInterpreter.OutputIp6Address(aMessageInfo->mPeerAddr);

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        if (otCoapOptionIteratorInit(&iterator, aMessage) == OT_ERROR_NONE)
        {
            const otCoapOption *observeOpt =
                otCoapOptionIteratorGetFirstOptionMatching(&iterator, OT_COAP_OPTION_OBSERVE);

            if (observeOpt != NULL)
            {
                uint64_t observeVal = 0;
                otError  error      = otCoapOptionIteratorGetOptionUintValue(&iterator, &observeVal);

                if (error == OT_ERROR_NONE)
                {
                    mInterpreter.mServer->OutputFormat(" OBS=%u", observeVal);
                }
            }
        }
#endif
        PrintPayload(aMessage);
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_COAP_API_ENABLE
