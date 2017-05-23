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
 *   This file implements a simple CLI coap server and client.
 */

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#if OPENTHREAD_ENABLE_APPLICATION_COAP

#include "cli_coap.hpp"

#include <ctype.h>

#include "cli/cli.hpp"
#include "coap/coap_header.hpp"

namespace ot {
namespace Cli {

const CoapCommand Coap::sCommands[] =
{
    { "client", &ProcessClient },
    { "server", &ProcessServer }
};

Server *Coap::sServer;
otCoapResource Coap::sResource;
otInstance *Coap::sInstance;
char Coap::sUriPath[kMaxUriLength];

void Coap::PrintPayload(otMessage *aMessage)
{
    uint8_t buf[kMaxBufferSize];
    uint16_t bytesToPrint;
    uint16_t bytesPrinted = 0;
    uint16_t length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    if (length > 0)
    {
        sServer->OutputFormat(" with payload: ");

        while (length > 0)
        {
            bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
            otMessageRead(aMessage, otMessageGetOffset(aMessage) + bytesPrinted, buf, bytesToPrint);

            OutputBytes(buf, static_cast<uint8_t>(bytesToPrint));

            length       -= bytesToPrint;
            bytesPrinted += bytesToPrint;
        }
    }

    sServer->OutputFormat("\r\n");
}

void Coap::ConvertToLower(char *aString)
{
    uint8_t i = 0;

    while (aString[i])
    {
        aString[i] = (char)tolower(aString[i]);
        i++;
    }
}

void Coap::OutputBytes(const uint8_t *aBytes, uint8_t aLength)
{
    for (int i = 0; i < aLength; i++)
    {
        sServer->OutputFormat("%02x ", aBytes[i]);
    }
}

otError Coap::Process(otInstance *aInstance, int argc, char *argv[], Server &aServer)
{
    otError error = OT_ERROR_NONE;

    sInstance = aInstance;
    sServer = &aServer;
    sResource.mUriPath = sUriPath;
    sResource.mContext = aInstance;
    sResource.mHandler = (otCoapRequestHandler) &Coap::s_HandleServerResponse;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    for (unsigned int i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            error = sCommands[i].mCommand(argc - 1, argv + 1);
            break;
        }
    }

exit:
    return error;
}

otError Coap::ProcessServer(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    ConvertToLower(argv[0]);

    if (strcmp(argv[0], "start") == 0)
    {
        SuccessOrExit(error = otCoapStart(sInstance, OT_DEFAULT_COAP_PORT));
        SuccessOrExit(error = otCoapAddResource(sInstance, &sResource));
        sServer->OutputFormat("Server started with resource '%s': ", sResource.mUriPath);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        otCoapRemoveResource(sInstance, &sResource);
        SuccessOrExit(error = otCoapStop(sInstance));
        sServer->OutputFormat("Server stopped: ");
    }
    else if (strcmp(argv[0], "name") == 0)
    {
        if (argc > 1)
        {
            strlcpy(sUriPath, argv[1], kMaxUriLength);
            sServer->OutputFormat("Changing resource name to '%s': ", sResource.mUriPath);
        }
        else
        {
            sServer->OutputFormat("Current resource name is '%s': ", sResource.mUriPath);
        }
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

exit:
    return error;
}

void OTCALL Coap::s_HandleServerResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                         otMessageInfo *aMessageInfo)
{
    static_cast<Coap *>(aContext)->HandleServerResponse(aHeader, aMessage, aMessageInfo);
}

void Coap::HandleServerResponse(otCoapHeader *aHeader, otMessage *aMessage, otMessageInfo *aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    otCoapHeader responseHeader;
    otMessage *responseMessage;
    otCoapCode responseCode = kCoapCodeEmpty;
    char responseContent = '0';

    sServer->OutputFormat("Received CoAP request from [%x:%x:%x:%x:%x:%x:%x:%x]: ",
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[0]),
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[1]),
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[2]),
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[3]),
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[4]),
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[5]),
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[6]),
                          HostSwap16(aMessageInfo->mSockAddr.mFields.m16[7]));

    switch (otCoapHeaderGetCode(aHeader))
    {
    case kCoapRequestGet:
        sServer->OutputFormat("GET");
        break;

    case kCoapRequestDelete:
        sServer->OutputFormat("DELETE");
        break;

    case kCoapRequestPut:
        sServer->OutputFormat("PUT");
        break;

    case kCoapRequestPost:
        sServer->OutputFormat("POST");
        break;

    default:
        sServer->OutputFormat("Undefined\r\n");
        return;
    }

    PrintPayload(aMessage);

    if ((otCoapHeaderGetType(aHeader) == kCoapTypeConfirmable) || otCoapHeaderGetCode(aHeader) == kCoapRequestGet)
    {
        if (otCoapHeaderGetCode(aHeader) == kCoapRequestGet)
        {
            responseCode = kCoapResponseContent;
        }
        else
        {
            responseCode = kCoapResponseValid;
        }

        otCoapHeaderInit(&responseHeader, kCoapTypeAcknowledgment, responseCode);
        otCoapHeaderSetMessageId(&responseHeader, otCoapHeaderGetMessageId(aHeader));
        otCoapHeaderSetToken(&responseHeader, otCoapHeaderGetToken(aHeader), otCoapHeaderGetTokenLength(aHeader));

        if (otCoapHeaderGetCode(aHeader) == kCoapRequestGet)
        {
            otCoapHeaderSetPayloadMarker(&responseHeader);
        }

        responseMessage = otCoapNewMessage(sInstance, &responseHeader);
        VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);

        if (otCoapHeaderGetCode(aHeader) == kCoapRequestGet)
        {
            SuccessOrExit(error = otMessageAppend(responseMessage, &responseContent, sizeof(responseContent)));
        }

        SuccessOrExit(error = otCoapSendResponse(sInstance, responseMessage, aMessageInfo));
    }

exit:

    if (error != OT_ERROR_NONE && responseMessage != NULL)
    {
        sServer->OutputFormat("Cannot send CoAP response message: Error %d\r\n", error);
        otMessageFree(responseMessage);
    }
    else
    {
        if (responseCode >= kCoapResponseCodeMin)
        {
            sServer->OutputFormat("CoAP response sent successfully!\r\n");
        }
    }
}

otError Coap::ProcessClient(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otMessage *message = NULL;
    otMessageInfo messageInfo;
    otCoapHeader header;
    uint16_t payloadLength = 0;

    // Default parameters
    char coapUri[kMaxUriLength] = "test";
    otCoapType coapType = kCoapTypeNonConfirmable;
    otCoapCode coapCode = kCoapRequestGet;
    otIp6Address coapDestinationIp;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    // CoAP-Code
    ConvertToLower(argv[0]);

    if (strcmp(argv[0], "get") == 0)
    {
        coapCode = kCoapRequestGet;
    }
    else if (strcmp(argv[0], "post") == 0)
    {
        coapCode = kCoapRequestPost;
    }
    else if (strcmp(argv[0], "put") == 0)
    {
        coapCode = kCoapRequestPut;
    }
    else if (strcmp(argv[0], "delete") == 0)
    {
        coapCode = kCoapRequestDelete;
    }
    else
    {
        ExitNow(error = OT_ERROR_PARSE);
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
        strlcpy(coapUri, argv[2], kMaxUriLength);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // CoAP-Type
    ConvertToLower(argv[3]);

    if ((argc > 3) && (strcmp(argv[3], "con") == 0))
    {
        coapType = kCoapTypeConfirmable;
    }

    otCoapHeaderInit(&header, coapType, coapCode);
    otCoapHeaderGenerateToken(&header, ot::Coap::Header::kDefaultTokenLength);
    SuccessOrExit(error = otCoapHeaderAppendUriPathOptions(&header, coapUri));

    if (argc > 4)
    {
        payloadLength = static_cast<uint16_t>(strlen(argv[4]));

        if (payloadLength > 0)
        {
            otCoapHeaderSetPayloadMarker(&header);
        }
    }

    message = otCoapNewMessage(sInstance, &header);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    // Embed content into message if given
    if (payloadLength > 0)
    {
        SuccessOrExit(error = otMessageAppend(message, argv[4], payloadLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = coapDestinationIp;
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;
    messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

    if ((coapType == kCoapTypeConfirmable) || coapCode == kCoapRequestGet)
    {
        error = otCoapSendRequest(sInstance, message, &messageInfo,
                                  (otCoapResponseHandler) &Coap::s_HandleClientResponse, NULL);
    }
    else
    {
        error = otCoapSendRequest(sInstance, message, &messageInfo, NULL, NULL);
    }

    sServer->OutputFormat("Sending CoAP message: ");

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return error;
}

void OTCALL Coap::s_HandleClientResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                         otMessageInfo *aMessageInfo, otError aResult)
{
    static_cast<Coap *>(aContext)->HandleClientResponse(aHeader, aMessage, aMessageInfo, aResult);
}

void Coap::HandleClientResponse(otCoapHeader *aHeader, otMessage *aMessage, otMessageInfo *aMessageInfo,
                                otError aResult)
{
    if (aResult != OT_ERROR_NONE)
    {
        sServer->OutputFormat("Error receiving CoAP response message: %d", aResult);
    }
    else
    {
        sServer->OutputFormat("Received CoAP response");
        PrintPayload(aMessage);
    }

    (void)aHeader;
    (void)aMessageInfo;
}

}  // namespace Cli
}  // namespace ot

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP
