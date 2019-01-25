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

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#include <mbedtls/debug.h>
#include <openthread/ip6.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
// header for place your x509 certificate and private key
#include "x509_cert_key.hpp"

namespace ot {
namespace Cli {

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

void CoapSecure::PrintHeaderInfos(otMessage *aMessage) const
{
    otCoapCode mCoapCode;
    otCoapType mCoapType;

    mCoapCode = otCoapMessageGetCode(aMessage);
    mCoapType = otCoapMessageGetType(aMessage);

    mInterpreter.mServer->OutputFormat("\r\n    CoapSecure RX Header Information:"
                                       "\r\n        Type %d => ",
                                       static_cast<uint16_t>(mCoapType));

    switch (mCoapType)
    {
    case OT_COAP_TYPE_ACKNOWLEDGMENT:
        mInterpreter.mServer->OutputFormat("Ack");
        break;
    case OT_COAP_TYPE_CONFIRMABLE:
        mInterpreter.mServer->OutputFormat("Confirmable");
        break;
    case OT_COAP_TYPE_NON_CONFIRMABLE:
        mInterpreter.mServer->OutputFormat("NonConfirmable");
        break;
    case OT_COAP_TYPE_RESET:
        mInterpreter.mServer->OutputFormat("Reset");
        break;
    default:
        break;
    }
    mInterpreter.mServer->OutputFormat("\r\n        Code %d => %s\r\n", static_cast<uint16_t>(mCoapCode),
                                       static_cast<const char *>(otCoapMessageCodeToString(aMessage)));
}

void CoapSecure::PrintPayload(otMessage *aMessage) const
{
    uint8_t  buf[kMaxBufferSize];
    uint16_t bytesToPrint;
    uint16_t bytesPrinted = 0;
    uint16_t length       = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);

    if (length > 0)
    {
        mInterpreter.mServer->OutputFormat("    With payload [UTF8]:\r\n", aMessage);

        while (length > 0)
        {
            bytesToPrint = (length < sizeof(buf)) ? length : sizeof(buf);
            otMessageRead(aMessage, otMessageGetOffset(aMessage) + bytesPrinted, buf, bytesToPrint);

            for (int i = 0; i < bytesToPrint; i++)
            {
                mInterpreter.mServer->OutputFormat("%c", buf[i]);
            }
            mInterpreter.mServer->OutputFormat("\r\n");

            length -= bytesToPrint;
            bytesPrinted += bytesToPrint;
        }
    }
    else
    {
        mInterpreter.mServer->OutputFormat("    No payload.");
    }

    mInterpreter.mServer->OutputFormat("\r\n> ");
}

otError CoapSecure::Process(int argc, char *argv[])
{
    otError error           = OT_ERROR_NONE;
    bool    mVerifyPeerCert = true;
    long    value;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "start") == 0)
    {
        if (argc > 1)
        {
            if (strcmp(argv[1], "false") == 0)
            {
                mVerifyPeerCert = false;
            }
            else if (strcmp(argv[1], "true") != 0)
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }
        otCoapSecureSetSslAuthMode(mInterpreter.mInstance, mVerifyPeerCert);
        SuccessOrExit(error = otCoapSecureStart(mInterpreter.mInstance, OT_DEFAULT_COAP_SECURE_PORT, this));
        otCoapSecureSetClientConnectedCallback(mInterpreter.mInstance, &CoapSecure::HandleClientConnect, this);
#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
        otCoapSecureSetDefaultHandler(mInterpreter.mInstance, &CoapSecure::DefaultHandle, this);
#endif // CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
        if (mUseCertificate)
        {
            mInterpreter.mServer->OutputFormat("Verify Peer Certificate: %s. Coap Secure service started: ",
                                               mVerifyPeerCert ? "true" : "false");
        }
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
    }
    else if (strcmp(argv[0], "set") == 0)
    {
        if (argc > 1)
        {
            if (strcmp(argv[1], "psk") == 0)
            {
                if (argc > 3)
                {
                    size_t length;

                    length = strlen(argv[2]);
                    VerifyOrExit(length <= sizeof(mPsk), error = OT_ERROR_INVALID_ARGS);
                    mPskLength = static_cast<uint8_t>(length);
                    memcpy(mPsk, argv[2], mPskLength);

                    length = strlen(argv[3]);
                    VerifyOrExit(length <= sizeof(mPskId), error = OT_ERROR_INVALID_ARGS);
                    mPskIdLength = static_cast<uint8_t>(length);
                    memcpy(mPskId, argv[3], mPskIdLength);

                    SuccessOrExit(
                        error = otCoapSecureSetPsk(mInterpreter.mInstance, mPsk, mPskLength, mPskId, mPskIdLength));
                    mUseCertificate = false;
                    mInterpreter.mServer->OutputFormat("Coap Secure set PSK: ");
                }
                else
                {
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }
            else if (strcmp(argv[1], "x509") == 0)
            {
#ifdef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
                SuccessOrExit(error = otCoapSecureSetCertificate(
                                  mInterpreter.mInstance, (const uint8_t *)OT_CLI_COAPS_X509_CERT,
                                  sizeof(OT_CLI_COAPS_X509_CERT), (const uint8_t *)OT_CLI_COAPS_PRIV_KEY,
                                  sizeof(OT_CLI_COAPS_PRIV_KEY)));

                SuccessOrExit(error = otCoapSecureSetCaCertificateChain(
                                  mInterpreter.mInstance, (const uint8_t *)OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE,
                                  sizeof(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE)));
                mUseCertificate = true;

                mInterpreter.mServer->OutputFormat("Coap Secure set own .X509 certificate: ");
#else
                ExitNow(error = OT_ERROR_DISABLED_FEATURE);
#endif // MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
            }
            else
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
    else if (strcmp(argv[0], "connect") == 0)
    {
        // Destination IPv6 address
        if (argc > 1)
        {
            otSockAddr sockaddr;

            memset(&sockaddr, 0, sizeof(sockaddr));
            SuccessOrExit(error = otIp6AddressFromString(argv[1], &sockaddr.mAddress));
            sockaddr.mPort    = OT_DEFAULT_COAP_SECURE_PORT;
            sockaddr.mScopeId = OT_NETIF_INTERFACE_ID_THREAD;

            // check for port specification
            if (argc > 2)
            {
                error = Interpreter::ParseLong(argv[2], value);
                SuccessOrExit(error);
                sockaddr.mPort = static_cast<uint16_t>(value);
            }

            SuccessOrExit(
                error = otCoapSecureConnect(mInterpreter.mInstance, &sockaddr, &CoapSecure::HandleClientConnect, this));
            mInterpreter.mServer->OutputFormat("Coap Secure connect: ");
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }
    else if (strcmp(argv[0], "resource") == 0)
    {
        mResource.mUriPath = mUriPath;
        mResource.mContext = this;
        mResource.mHandler = &CoapSecure::HandleServerResponse;

        if (argc > 1)
        {
            strlcpy(mUriPath, argv[1], kMaxUriLength);
            SuccessOrExit(error = otCoapSecureAddResource(mInterpreter.mInstance, &mResource));
        }

        mInterpreter.mServer->OutputFormat("Resource name is '%s': ", mResource.mUriPath);
    }
    else if (strcmp(argv[0], "disconnect") == 0)
    {
        SuccessOrExit(error = otCoapSecureDisconnect(mInterpreter.mInstance));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        if (otCoapSecureIsConnectionActive(mInterpreter.mInstance))
        {
            error         = otCoapSecureDisconnect(mInterpreter.mInstance);
            mShutdownFlag = true;
        }
        else
        {
            SuccessOrExit(error = Stop());
        }
    }
    else if (strcmp(argv[0], "help") == 0)
    {
        mInterpreter.mServer->OutputFormat("CLI CoAPS help:\r\n\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps start (false)'                               "
                                           ": start coap secure service, false disable peer cert verification\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps set psk <psk> <client id>'                   "
                                           ": set Preshared Key and Client Identity (Ciphersuite PSK_AES128)\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps set x509'                                    "
                                           ": set X509 Cert und Private Key (Ciphersuite ECDHE_ECDSA_AES128)\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps connect <servers ipv6 addr> (port)'          "
                                           ": start dtls session with a server\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps get' 'coaps put' 'coaps post' 'coaps delete' "
                                           ": interact with coap resource from server, ipv6 is not need as client\r\n");
        mInterpreter.mServer->OutputFormat(
            "    >> args:(ipv6_addr_srv) <coap_src> and, if you have payload: <con> <payload>\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps resource <uri>'                              "
                                           ": add a coap server resource with 'helloWorld' as content.\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps disconnect'                                  "
                                           ": stop dtls session with a server\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps stop'                                        "
                                           ": stop coap secure service\r\n");
        mInterpreter.mServer->OutputFormat("\r\n legend: <>: must, (): opt                       "
                                           "\r\n");
        mInterpreter.mServer->OutputFormat("\r\n");
    }
    else
    {
        error = ProcessRequest(argc, argv);
    }

exit:
    return error;
}

otError CoapSecure::Stop(void)
{
    otError error = OT_ERROR_ABORT;
    otCoapRemoveResource(mInterpreter.mInstance, &mResource);
    error = otCoapSecureStop(mInterpreter.mInstance);
    mInterpreter.mServer->OutputFormat("Coap Secure service stopped: ");
    return error;
}

void OTCALL CoapSecure::HandleClientConnect(bool aConnected, void *aContext)
{
    static_cast<CoapSecure *>(aContext)->HandleClientConnect(aConnected);
}

void CoapSecure::HandleClientConnect(bool aConnected)
{
    OT_UNUSED_VARIABLE(aConnected);

    if (aConnected)
    {
        mInterpreter.mServer->OutputFormat("CoAP Secure connected!\r\n> ");
    }
    else
    {
        if (!mShutdownFlag)
        {
            mInterpreter.mServer->OutputFormat("CoAP Secure not connected or disconnected.\r\n> ");
        }
        else
        {
            mInterpreter.mServer->OutputFormat("CoAP Secure disconnected before stop.\r\n> ");
            if (Stop() == OT_ERROR_NONE)
            {
                mInterpreter.mServer->OutputFormat(" Done\r\n> ");
            }
            else
            {
                mInterpreter.mServer->OutputFormat(" With error\r\n> ");
            }
            mShutdownFlag = false;
        }
    }
}

void OTCALL CoapSecure::HandleServerResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<CoapSecure *>(aContext)->HandleServerResponse(aMessage, aMessageInfo);
}

void CoapSecure::HandleServerResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error             = OT_ERROR_NONE;
    otMessage *responseMessage   = NULL;
    otCoapCode responseCode      = OT_COAP_CODE_EMPTY;
    char       responseContent[] = "helloWorld";

    mInterpreter.mServer->OutputFormat(
        "Received coap secure request from [%x:%x:%x:%x:%x:%x:%x:%x]: ",
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[0]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[1]),
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[2]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[3]),
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[4]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[5]),
        HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[6]), HostSwap16(aMessageInfo->mPeerAddr.mFields.m16[7]));

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

        otCoapMessageInit(responseMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode);
        otCoapMessageSetMessageId(responseMessage, otCoapMessageGetMessageId(aMessage));
        otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage), otCoapMessageGetTokenLength(aMessage));

        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            otCoapMessageSetPayloadMarker(responseMessage);
        }

        if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
        {
            SuccessOrExit(error = otMessageAppend(responseMessage, &responseContent, sizeof(responseContent)));
        }

        SuccessOrExit(error = otCoapSecureSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));
    }

exit:

    if (error != OT_ERROR_NONE && responseMessage != NULL)
    {
        mInterpreter.mServer->OutputFormat("Cannot send coap secure response message: Error %d: %s\r\n", error,
                                           otThreadErrorToString(error));
        otMessageFree(responseMessage);
    }
    else if (responseCode >= OT_COAP_CODE_RESPONSE_MIN)
    {
        mInterpreter.mServer->OutputFormat("coap secure response sent successfully!\r\n");
    }
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
        strlcpy(coapUri, argv[2 - indexShifter], kMaxUriLength);
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
            otCoapMessageSetPayloadMarker(message);
        }
    }

    // add payload
    if (payloadLength > 0)
    {
        SuccessOrExit(error = otMessageAppend(message, argv[4 - indexShifter], payloadLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr    = coapDestinationIp;
    messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
    messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

    if ((coapType == OT_COAP_TYPE_CONFIRMABLE) || (coapCode == OT_COAP_CODE_GET))
    {
        error = otCoapSecureSendRequest(mInterpreter.mInstance, message, &CoapSecure::HandleClientResponse, this);
    }
    else
    {
        error = otCoapSecureSendRequest(mInterpreter.mInstance, message, NULL, NULL);
    }

    mInterpreter.mServer->OutputFormat("Sending coap secure request: ");

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return error;
}

void OTCALL CoapSecure::HandleClientResponse(void *               aContext,
                                             otMessage *          aMessage,
                                             const otMessageInfo *aMessageInfo,
                                             otError              aError)
{
    static_cast<CoapSecure *>(aContext)->HandleClientResponse(aMessage, aMessageInfo, aError);
}

void CoapSecure::HandleClientResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    if (aError != OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("Error receiving coap secure response message: Error %d: %s\r\n", aError,
                                           otThreadErrorToString(aError));
    }
    else
    {
        mInterpreter.mServer->OutputFormat("Received coap secure response");
        PrintHeaderInfos(aMessage);
        PrintPayload(aMessage);
    }
}

#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
void OTCALL CoapSecure::DefaultHandle(void *               aContext,
                                      otCoapMessage *      aHeader,
                                      otMessage *          aMessage,
                                      const otMessageInfo *aMessageInfo)
{
    static_cast<CoapSecure *>(aContext)->DefaultHandle(aHeader, aMessage, aMessageInfo);
}

void CoapSecure::DefaultHandle(otCoapMessage *aHeader, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessage);

    otError       error = OT_ERROR_NONE;
    otCoapMessage responseHeader;
    otMessage *   responseMessage;

    if (otCoapMessageGetType(aHeader) == OT_COAP_TYPE_CONFIRMABLE || otCoapMessageGetCode(aHeader) == OT_COAP_CODE_GET)
    {
        otCoapMessageInit(&responseHeader, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_NOT_FOUND);
        otCoapMessageSetMessageId(&responseHeader, otCoapMessageGetMessageId(aHeader));
        otCoapMessageSetToken(&responseHeader, otCoapMessageGetToken(aHeader), otCoapMessageGetTokenLength(aHeader));

        responseMessage = otCoapNewMessage(mInterpreter.mInstance, NULL);
        VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = otCoapSecureSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));
    }

exit:

    mInterpreter.mServer->OutputFormat("Default handler called.\r\n> ");
}
#endif // CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
