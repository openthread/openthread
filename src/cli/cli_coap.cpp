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

#if OPENTHREAD_ENABLE_APPLICATION_COAP

#include <ctype.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
#include "coap/coap_message.hpp"

namespace ot {
namespace Cli {

const struct Coap::Command Coap::sCommands[] = {
    {"help", &Coap::ProcessHelp},    {"delete", &Coap::ProcessRequest}, {"get", &Coap::ProcessRequest},
    {"post", &Coap::ProcessRequest}, {"put", &Coap::ProcessRequest},    {"resource", &Coap::ProcessResource},
    {"start", &Coap::ProcessStart},  {"stop", &Coap::ProcessStop},
};

Coap::Coap(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
    memset(&mResource, 0, sizeof(mResource));
}

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

otError Coap::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError Coap::ProcessResource(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc > 1)
    {
        VerifyOrExit(strlen(argv[1]) < kMaxUriLength, error = OT_ERROR_INVALID_ARGS);

        mResource.mUriPath = mUriPath;
        mResource.mContext = this;
        mResource.mHandler = &Coap::HandleRequest;

        strlcpy(mUriPath, argv[1], kMaxUriLength);
        SuccessOrExit(error = otCoapAddResource(mInterpreter.mInstance, &mResource));
    }
    else
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", mResource.mUriPath);
    }

exit:
    return OT_ERROR_NONE;
}

otError Coap::ProcessStart(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otCoapStart(mInterpreter.mInstance, OT_DEFAULT_COAP_PORT);
}

otError Coap::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otCoapRemoveResource(mInterpreter.mInstance, &mResource);

    return otCoapStop(mInterpreter.mInstance);
}

otError Coap::ProcessRequest(int argc, char *argv[])
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

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    // CoAP-Code
    if (strcmp(argv[0], "get") == 0)
    {
        coapCode = OT_COAP_CODE_GET;
    }
    else if (strcmp(argv[0], "post") == 0)
    {
        coapCode = OT_COAP_CODE_POST;
    }
    else if (strcmp(argv[0], "put") == 0)
    {
        coapCode = OT_COAP_CODE_PUT;
    }
    else if (strcmp(argv[0], "delete") == 0)
    {
        coapCode = OT_COAP_CODE_DELETE;
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // Destination IPv6 address
    if (argc > 1)
    {
        SuccessOrExit(error = otIp6AddressFromString(argv[1], &coapDestinationIp));
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // CoAP-URI
    if (argc > 2)
    {
        VerifyOrExit(strlen(argv[2]) < kMaxUriLength, error = OT_ERROR_INVALID_ARGS);
        strlcpy(coapUri, argv[2], kMaxUriLength);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // CoAP-Type
    if (argc > 3)
    {
        if (strcmp(argv[3], "con") == 0)
        {
            coapType = OT_COAP_TYPE_CONFIRMABLE;
        }
    }

    message = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, coapType, coapCode);
    otCoapMessageGenerateToken(message, ot::Coap::Message::kDefaultTokenLength);
    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, coapUri));

    if (argc > 4)
    {
        payloadLength = static_cast<uint16_t>(strlen(argv[4]));

        if (payloadLength > 0)
        {
            otCoapMessageSetPayloadMarker(message);
        }
    }

    // Embed content into message if given
    if (payloadLength > 0)
    {
        SuccessOrExit(error = otMessageAppend(message, argv[4], payloadLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr    = coapDestinationIp;
    messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
    messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

    if ((coapType == OT_COAP_TYPE_CONFIRMABLE) || (coapCode == OT_COAP_CODE_GET))
    {
        error = otCoapSendRequest(mInterpreter.mInstance, message, &messageInfo, &Coap::HandleResponse, this);
    }
    else
    {
        error = otCoapSendRequest(mInterpreter.mInstance, message, &messageInfo, NULL, NULL);
    }

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return error;
}

otError Coap::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(argc, argv);
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
    char       responseContent = '0';

    mInterpreter.mServer->OutputFormat(
        "coap request from [%x:%x:%x:%x:%x:%x:%x:%x] ", HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[0]),
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[1]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[2]),
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[3]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[4]),
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[5]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[6]),
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[7]));

    switch (otCoapMessageGetCode(aMessage))
    {
    case OT_COAP_CODE_GET:
        mInterpreter.mServer->OutputFormat("GET");
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
        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            responseCode = OT_COAP_CODE_CONTENT;
        }
        else
        {
            responseCode = OT_COAP_CODE_VALID;
        }

        responseMessage = otCoapNewMessage(mInterpreter.mInstance, NULL);
        VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);

        otCoapMessageInit(responseMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode);
        otCoapMessageSetMessageId(responseMessage, otCoapMessageGetMessageId(aMessage));
        otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            otCoapMessageSetPayloadMarker(responseMessage);
            SuccessOrExit(error = otMessageAppend(responseMessage, &responseContent, sizeof(responseContent)));
        }

        SuccessOrExit(error = otCoapSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));
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
    else
    {
        mInterpreter.mServer->OutputFormat(
            "coap response from [%x:%x:%x:%x:%x:%x:%x:%x]", HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[0]),
            HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[1]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[2]),
            HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[3]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[4]),
            HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[5]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[6]),
            HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[7]));

        PrintPayload(aMessage);
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP
