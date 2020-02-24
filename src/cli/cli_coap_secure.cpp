/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements a simple CLI for the CoAP Secure service.
 */

#include "cli_coap_secure.hpp"

#if OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE

#include <mbedtls/debug.h>
#include <openthread/ip6.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
// header for place your x509 certificate and private key
#include "x509_cert_key.hpp"

namespace ot {
namespace Cli {

const struct CoapSecure::Command CoapSecure::sCommands[] = {
    {"help", &CoapSecure::ProcessHelp},      {"connect", &CoapSecure::ProcessConnect},
    {"delete", &CoapSecure::ProcessRequest}, {"disconnect", &CoapSecure::ProcessDisconnect},
    {"get", &CoapSecure::ProcessRequest},    {"post", &CoapSecure::ProcessRequest},
    {"put", &CoapSecure::ProcessRequest},    {"resource", &CoapSecure::ProcessResource},
    {"start", &CoapSecure::ProcessStart},    {"stop", &CoapSecure::ProcessStop},
#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
    {"psk", &CoapSecure::ProcessPsk},
#endif
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    {"x509", &CoapSecure::ProcessX509},
#endif
};

CoapSecure::CoapSecure(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
    , mShutdownFlag(false)
    , mUseCertificate(false)
    , mPskLength(0)
    , mPskIdLength(0)
{
    memset(&mResource, 0, sizeof(mResource));
    memset(&mPsk, 0, sizeof(mPsk));
    memset(&mPskId, 0, sizeof(mPskId));
}

void CoapSecure::PrintPayload(otMessage *aMessage) const
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

otError CoapSecure::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError CoapSecure::ProcessResource(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    if (argc > 1)
    {
        VerifyOrExit(strlen(argv[1]) < kMaxUriLength, error = OT_ERROR_INVALID_ARGS);

        mResource.mUriPath = mUriPath;
        mResource.mContext = this;
        mResource.mHandler = &CoapSecure::HandleRequest;

        strncpy(mUriPath, argv[1], sizeof(mUriPath) - 1);
        SuccessOrExit(error = otCoapSecureAddResource(mInterpreter.mInstance, &mResource));
    }
    else
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", mResource.mUriPath);
    }

exit:
    return OT_ERROR_NONE;
}

otError CoapSecure::ProcessStart(int argc, char *argv[])
{
    otError error;
    bool    verifyPeerCert = true;

    if (argc > 1)
    {
        if (strcmp(argv[1], "false") == 0)
        {
            verifyPeerCert = false;
        }
        else if (strcmp(argv[1], "true") != 0)
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    otCoapSecureSetSslAuthMode(mInterpreter.mInstance, verifyPeerCert);
    otCoapSecureSetClientConnectedCallback(mInterpreter.mInstance, &CoapSecure::HandleConnected, this);

#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
    otCoapSecureSetDefaultHandler(mInterpreter.mInstance, &CoapSecure::DefaultHandler, this);
#endif

    SuccessOrExit(error = otCoapSecureStart(mInterpreter.mInstance, OT_DEFAULT_COAP_SECURE_PORT));

exit:
    return error;
}

otError CoapSecure::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otCoapRemoveResource(mInterpreter.mInstance, &mResource);

    if (otCoapSecureIsConnectionActive(mInterpreter.mInstance))
    {
        otCoapSecureDisconnect(mInterpreter.mInstance);
        mShutdownFlag = true;
    }
    else
    {
        Stop();
    }

    return OT_ERROR_NONE;
}

otError CoapSecure::ProcessRequest(int argc, char *argv[])
{
    otError       error   = OT_ERROR_NONE;
    otMessage *   message = NULL;
    otMessageInfo messageInfo;
    uint16_t      payloadLength = 0;
    uint8_t       indexShifter  = 0;

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
        ExitNow(error = OT_ERROR_PARSE);
    }

    // Destination IPv6 address
    if (argc > 1)
    {
        error = otIp6AddressFromString(argv[1], &coapDestinationIp);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // Destination IPv6 address not need as client
    // if no IPv6 is entered, so ignore it and go away
    if (error == OT_ERROR_NONE)
    {
        indexShifter = 0;
    }
    else
    {
        indexShifter = 1;
    }

    // CoAP-URI
    if (argc > (2 - indexShifter))
    {
        strncpy(coapUri, argv[2 - indexShifter], sizeof(coapUri) - 1);
    }

    // CoAP-Type
    if (argc > (3 - indexShifter))
    {
        if (strcmp(argv[3 - indexShifter], "con") == 0)
        {
            coapType = OT_COAP_TYPE_CONFIRMABLE;
        }
    }

    message = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, coapType, coapCode);
    otCoapMessageGenerateToken(message, ot::Coap::Message::kDefaultTokenLength);
    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, coapUri));

    if (argc > (4 - indexShifter))
    {
        payloadLength = static_cast<uint16_t>(strlen(argv[4 - indexShifter]));

        if (payloadLength > 0)
        {
            SuccessOrExit(error = otCoapMessageSetPayloadMarker(message));
        }
    }

    // add payload
    if (payloadLength > 0)
    {
        SuccessOrExit(error = otMessageAppend(message, argv[4 - indexShifter], payloadLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = coapDestinationIp;
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

    if ((coapType == OT_COAP_TYPE_CONFIRMABLE) || (coapCode == OT_COAP_CODE_GET))
    {
        error = otCoapSecureSendRequest(mInterpreter.mInstance, message, &CoapSecure::HandleResponse, this);
    }
    else
    {
        error = otCoapSecureSendRequest(mInterpreter.mInstance, message, NULL, NULL);
    }

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return error;
}

otError CoapSecure::ProcessConnect(int argc, char *argv[])
{
    otError    error;
    otSockAddr sockaddr;

    VerifyOrExit(argc > 1, error = OT_ERROR_INVALID_ARGS);

    // Destination IPv6 address
    memset(&sockaddr, 0, sizeof(sockaddr));
    SuccessOrExit(error = otIp6AddressFromString(argv[1], &sockaddr.mAddress));
    sockaddr.mPort = OT_DEFAULT_COAP_SECURE_PORT;

    // check for port specification
    if (argc > 2)
    {
        long value;

        error = Interpreter::ParseLong(argv[2], value);
        SuccessOrExit(error);
        sockaddr.mPort = static_cast<uint16_t>(value);
    }

    SuccessOrExit(error = otCoapSecureConnect(mInterpreter.mInstance, &sockaddr, &CoapSecure::HandleConnected, this));

exit:
    return error;
}

otError CoapSecure::ProcessDisconnect(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otCoapSecureDisconnect(mInterpreter.mInstance);

    return OT_ERROR_NONE;
}

#ifdef MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
otError CoapSecure::ProcessPsk(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    size_t  length;

    VerifyOrExit(argc > 2, error = OT_ERROR_INVALID_ARGS);

    length = strlen(argv[1]);
    VerifyOrExit(length <= sizeof(mPsk), error = OT_ERROR_INVALID_ARGS);
    mPskLength = static_cast<uint8_t>(length);
    memcpy(mPsk, argv[1], mPskLength);

    length = strlen(argv[2]);
    VerifyOrExit(length <= sizeof(mPskId), error = OT_ERROR_INVALID_ARGS);
    mPskIdLength = static_cast<uint8_t>(length);
    memcpy(mPskId, argv[2], mPskIdLength);

    otCoapSecureSetPsk(mInterpreter.mInstance, mPsk, mPskLength, mPskId, mPskIdLength);
    mUseCertificate = false;

exit:
    return error;
}
#endif // MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
otError CoapSecure::ProcessX509(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otCoapSecureSetCertificate(mInterpreter.mInstance, (const uint8_t *)OT_CLI_COAPS_X509_CERT,
                               sizeof(OT_CLI_COAPS_X509_CERT), (const uint8_t *)OT_CLI_COAPS_PRIV_KEY,
                               sizeof(OT_CLI_COAPS_PRIV_KEY));

    otCoapSecureSetCaCertificateChain(mInterpreter.mInstance, (const uint8_t *)OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE,
                                      sizeof(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE));
    mUseCertificate = true;

    return OT_ERROR_NONE;
}
#endif

otError CoapSecure::Process(int argc, char *argv[])
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

void CoapSecure::Stop(void)
{
    otCoapRemoveResource(mInterpreter.mInstance, &mResource);
    otCoapSecureStop(mInterpreter.mInstance);
}

void CoapSecure::HandleConnected(bool aConnected, void *aContext)
{
    static_cast<CoapSecure *>(aContext)->HandleConnected(aConnected);
}

void CoapSecure::HandleConnected(bool aConnected)
{
    if (aConnected)
    {
        mInterpreter.mServer->OutputFormat("coaps connected\r\n");
    }
    else
    {
        mInterpreter.mServer->OutputFormat("coaps disconnected\r\n");

        if (mShutdownFlag)
        {
            Stop();
            mShutdownFlag = false;
        }
    }
}

void CoapSecure::HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<CoapSecure *>(aContext)->HandleRequest(aMessage, aMessageInfo);
}

void CoapSecure::HandleRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error             = OT_ERROR_NONE;
    otMessage *responseMessage   = NULL;
    otCoapCode responseCode      = OT_COAP_CODE_EMPTY;
    char       responseContent[] = "helloWorld";

    mInterpreter.mServer->OutputFormat("coaps request from ");
    mInterpreter.OutputIp6Address(aMessageInfo->mPeerAddr);
    mInterpreter.mServer->OutputFormat(" ");

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
        return;
    }

    PrintPayload(aMessage);

    if ((otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE) ||
        (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET))
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

        SuccessOrExit(
            error = otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode));

        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            SuccessOrExit(error = otCoapMessageSetPayloadMarker(responseMessage));
        }

        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            SuccessOrExit(error = otMessageAppend(responseMessage, &responseContent, sizeof(responseContent)));
        }

        SuccessOrExit(error = otCoapSecureSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        if (responseMessage != NULL)
        {
            mInterpreter.mServer->OutputFormat("coaps send response error %d: %s\r\n", error,
                                               otThreadErrorToString(error));
            otMessageFree(responseMessage);
        }
    }
    else if (responseCode >= OT_COAP_CODE_RESPONSE_MIN)
    {
        mInterpreter.mServer->OutputFormat("coaps response sent\r\n");
    }
}

void CoapSecure::HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    static_cast<CoapSecure *>(aContext)->HandleResponse(aMessage, aMessageInfo, aError);
}

void CoapSecure::HandleResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    if (aError != OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("coaps receive response error %d: %s\r\n", aError,
                                           otThreadErrorToString(aError));
    }
    else
    {
        mInterpreter.mServer->OutputFormat("coaps response from ");
        mInterpreter.OutputIp6Address(aMessageInfo->mPeerAddr);

        PrintPayload(aMessage);
    }
}

#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
void CoapSecure::DefaultHandler(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<CoapSecure *>(aContext)->DefaultHandler(aMessage, aMessageInfo);
}

void CoapSecure::DefaultHandler(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error           = OT_ERROR_NONE;
    otMessage *responseMessage = NULL;

    if ((otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE) ||
        (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET))
    {
        responseMessage = otCoapNewMessage(mInterpreter.mInstance, NULL);
        VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_NON_CONFIRMABLE,
                                                        OT_COAP_CODE_NOT_FOUND));

        SuccessOrExit(error = otCoapSecureSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));
    }

exit:
    if (error != OT_ERROR_NONE && responseMessage != NULL)
    {
        otMessageFree(responseMessage);
    }
}
#endif // CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
