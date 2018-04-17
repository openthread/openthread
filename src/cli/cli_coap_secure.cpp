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
 *   This file implements a simple CLI for the CoAP Secure service.
 */

#include "cli_coap_secure.hpp"

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#include <ctype.h>

#include "cli/cli.hpp"
#include "coap/coap_header.hpp"
#include "coap/coap_secure.hpp"		// include the core coap secure implementation ToDo: remove this comment later

namespace ot {
namespace Cli {

bool shutdownFlag = false;

CoapSecureCli::CoapSecureCli(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
    memset(&mResource, 0, sizeof(mResource));
}

void CoapSecureCli::PrintPayload(otMessage *aMessage) const
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

otError CoapSecureCli::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;
    otIp6Address coapDestinationIp;

    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "start") == 0)
    {
        SuccessOrExit(error = otCoapSecureStart(mInterpreter.mInstance, OT_DEFAULT_COAP_SECURE_PORT, this));
        mInterpreter.mServer->OutputFormat("Coap Secure service started: ");
    }
    else if (strcmp(argv[0], "setpsk") == 0)
	{
    	if (argc > 2)
		{
    		uint8_t mPsk[32];
    		uint8_t mPskLength = strlen(argv[1]);
    		uint8_t mPskId[32];
    		uint8_t mPskIdLength = strlen(argv[2]);

    		memcpy(mPsk, argv[1], mPskLength);
    		memcpy(mPskId, argv[2], mPskIdLength);

    		SuccessOrExit(error = otCoapSecureSetPSK(mInterpreter.mInstance, mPsk, mPskLength,
    		                                         mPskId, mPskIdLength));
    		mInterpreter.mServer->OutputFormat("Coap Secure set PSK: ");
		}
    	else
		{
			ExitNow(error = OT_ERROR_INVALID_ARGS);
		}
	}
    else if (strcmp(argv[0], "setx509") == 0)
    {
        if (argc > 2)
        {
            uint8_t mX509Cert[255];
            uint32_t mX509Length = strlen(argv[1]);
            uint8_t mPk[255];
            uint32_t mPkLength = strlen(argv[2]);

            memcpy(mX509Cert, argv[1], mX509Length);
            memcpy(mPk, argv[2], mPkLength);

            SuccessOrExit(error = otCoapSecureSetX509Certificate(mInterpreter.mInstance,
                                                                 mX509Cert, mX509Length,
                                                                 mPk, mPkLength));
            mInterpreter.mServer->OutputFormat("Coap Secure set X509 Cert: ");
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
            //parse ipAddr
			SuccessOrExit(error = otIp6AddressFromString(argv[1], &coapDestinationIp));
			otMessageInfo messageInfo;
			memset(&messageInfo, 0, sizeof(messageInfo));
			messageInfo.mPeerAddr    = coapDestinationIp;
			messageInfo.mPeerPort    = OT_DEFAULT_COAP_SECURE_PORT;
			messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;
			SuccessOrExit(error =
					otCoapSecureConnect(mInterpreter.mInstance, &messageInfo, &CoapSecureCli::HandleSecureCoapClientConnect, this));
		}
		else
		{
			ExitNow(error = OT_ERROR_INVALID_ARGS);
		}

    }
    else if (strcmp(argv[0], "disconnect") == 0)
    {
        SuccessOrExit(error = otCoapSecureDisconnect(mInterpreter.mInstance));
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        // check, if the dtls session is not active, else disconnect it.
        if (otCoapSecureIsConnected(mInterpreter.mInstance) ||
                otCoapSecureIsConncetionActive(mInterpreter.mInstance))
        {
            error = otCoapSecureDisconnect(mInterpreter.mInstance);
            shutdownFlag = true;
        }
        else
        {
                                                                                                        otCoapRemoveResource(mInterpreter.mInstance, &mResource);   // ToDo
            SuccessOrExit(error = otCoapSecureStop(mInterpreter.mInstance));
            mInterpreter.mServer->OutputFormat("Coap Secure service stopped: ");
        }
    }
    else if (strcmp(argv[0], "help") == 0)
    {
    	mInterpreter.mServer->OutputFormat("CLI CoAPS help:\r\n\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps start'                                       : start coap secure service\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps setpsk'      args: psk, identity             : set Preshared Key and Client Identity (Ciphresuit PSK_AES128)\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps setx509'                                     : set X509 Cert und Private Key (Ciphresuit ECDHE_ECDSA_AES128)\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps connect'     args: ipV6_addr_srv             : start dtls session with a server\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps get' 'coaps put' 'coaps post' 'coaps delete' : interact with coap source from server, ipv6 is not need as client\r\n");
        mInterpreter.mServer->OutputFormat("    >> args:(ipV6_addr_srv), coap_src, con, payload\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps disconnect'                                  : stop dtls session with a server\r\n");
        mInterpreter.mServer->OutputFormat(">'coaps stop'                                        : stop coap secure service\r\n");
    	mInterpreter.mServer->OutputFormat("\r\n");

    }
    else
    {
        error = ProcessRequest(argc, argv);
    }

exit:
    return error;
}

void OTCALL CoapSecureCli::HandleSecureCoapClientConnect(bool aConnected, void *aContext)
{
    static_cast<CoapSecureCli *>(aContext)->HandleSecureCoapClientConnect(aConnected);
}


void CoapSecureCli::HandleSecureCoapClientConnect(const bool aConnected)
{

	if(aConnected)
	{
	    mInterpreter.mServer->OutputFormat("CoAP Secure connected!\r\n>");
	}
	else
	{
	    if (!shutdownFlag)
	    {
	        mInterpreter.mServer->OutputFormat("CoAP Secure not connected or disconnected.\r\n>");
	    }
	    else
	    {
	        mInterpreter.mServer->OutputFormat("CoAP Secure disconnected before stop.\r\n>");
	        otCoapRemoveResource(mInterpreter.mInstance, &mResource);   // ToDo: rm the right values, not the coap vals
	        otCoapSecureStop(mInterpreter.mInstance);
	        mInterpreter.mServer->OutputFormat("Coap Secure service stopped: ");
	        shutdownFlag = false;
	    }
	}

	OT_UNUSED_VARIABLE(aConnected);
}

void OTCALL CoapSecureCli::HandleServerResponse(void *      aContext,
                                       otCoapHeader *       aHeader,
                                       otMessage *          aMessage,
                                       const otMessageInfo *aMessageInfo)
{
    static_cast<CoapSecureCli *>(aContext)->HandleServerResponse(aHeader,
                                                                 aMessage,
                                                                 aMessageInfo);
}

void CoapSecureCli::HandleServerResponse(otCoapHeader *aHeader, otMessage *aMessage,
                                         const otMessageInfo *aMessageInfo)
{
    otError      error = OT_ERROR_NONE;
    otCoapHeader responseHeader;
    otMessage *  responseMessage;
    otCoapCode   responseCode    = OT_COAP_CODE_EMPTY;
    char         responseContent = '0';

    mInterpreter.mServer->OutputFormat(
        "Received coap secure request from [%x:%x:%x:%x:%x:%x:%x:%x]: ", HostSwap16(aMessageInfo->mSockAddr.mFields.m16[0]),
        HostSwap16(aMessageInfo->mSockAddr.mFields.m16[1]), HostSwap16(aMessageInfo->mSockAddr.mFields.m16[2]),
        HostSwap16(aMessageInfo->mSockAddr.mFields.m16[3]), HostSwap16(aMessageInfo->mSockAddr.mFields.m16[4]),
        HostSwap16(aMessageInfo->mSockAddr.mFields.m16[5]), HostSwap16(aMessageInfo->mSockAddr.mFields.m16[6]),
        HostSwap16(aMessageInfo->mSockAddr.mFields.m16[7]));

    switch (otCoapHeaderGetCode(aHeader))
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

    if ((otCoapHeaderGetType(aHeader) == OT_COAP_TYPE_CONFIRMABLE) || otCoapHeaderGetCode(aHeader) == OT_COAP_CODE_GET)
    {
        if (otCoapHeaderGetCode(aHeader) == OT_COAP_CODE_GET)
        {
            responseCode = OT_COAP_CODE_CONTENT;
        }
        else
        {
            responseCode = OT_COAP_CODE_VALID;
        }

        otCoapHeaderInit(&responseHeader, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode);
        otCoapHeaderSetMessageId(&responseHeader, otCoapHeaderGetMessageId(aHeader));
        otCoapHeaderSetToken(&responseHeader, otCoapHeaderGetToken(aHeader), otCoapHeaderGetTokenLength(aHeader));

        if (otCoapHeaderGetCode(aHeader) == OT_COAP_CODE_GET)
        {
            otCoapHeaderSetPayloadMarker(&responseHeader);
        }

        responseMessage = otCoapNewMessage(mInterpreter.mInstance, &responseHeader);
        VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);

        if (otCoapHeaderGetCode(aHeader) == OT_COAP_CODE_GET)
        {
            SuccessOrExit(error = otMessageAppend(responseMessage, &responseContent, sizeof(responseContent)));
        }

        SuccessOrExit(error = otCoapSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));
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

otError CoapSecureCli::ProcessRequest(int argc, char *argv[])
{
    otError       error   = OT_ERROR_NONE;
    otMessage *   message = NULL;
    otMessageInfo messageInfo;
    otCoapHeader  header;
    uint16_t      payloadLength = 0;
    uint8_t       index_shifter = 0;

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
    if (error == 0)
    {
        index_shifter = 0;
    }
    else
    {
        index_shifter = 1;
    }

    // CoAP-URI
    if (argc > (2-index_shifter))
    {
        strlcpy(coapUri, argv[2-index_shifter], kMaxUriLength);
    }
    else
    {
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // CoAP-Type
    if (argc > (3-index_shifter))
    {
        if (strcmp(argv[3-index_shifter], "con") == 0)
        {
            coapType = OT_COAP_TYPE_CONFIRMABLE;
        }
    }

    otCoapHeaderInit(&header, coapType, coapCode);
    otCoapHeaderGenerateToken(&header, ot::Coap::Header::kDefaultTokenLength);
    SuccessOrExit(error = otCoapHeaderAppendUriPathOptions(&header, coapUri));

    if (argc > (4-index_shifter))
    {
        payloadLength = static_cast<uint16_t>(strlen(argv[4-index_shifter]));

        if (payloadLength > 0)
        {
            otCoapHeaderSetPayloadMarker(&header);
        }
    }

    message = otCoapNewMessage(mInterpreter.mInstance, &header);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    // Embed content into message if given
    if (payloadLength > 0)
    {
        SuccessOrExit(error = otMessageAppend(message, argv[4-index_shifter], payloadLength));
    }

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr    = coapDestinationIp;
    messageInfo.mPeerPort    = OT_DEFAULT_COAP_PORT;
    messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;

    if ((coapType == OT_COAP_TYPE_CONFIRMABLE) || (coapCode == OT_COAP_CODE_GET))
    {
        error = otCoapSecureSendMessage(mInterpreter.mInstance, message, &CoapSecureCli::HandleClientResponse, this);
    }
    else
    {
        error = otCoapSecureSendMessage(mInterpreter.mInstance, message, NULL, NULL);
    }

    mInterpreter.mServer->OutputFormat("Sending coap secure request: ");

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return error;
}

void OTCALL CoapSecureCli::HandleClientResponse(void *      aContext,
                                       otCoapHeader *       aHeader,
                                       otMessage *          aMessage,
                                       const otMessageInfo *aMessageInfo,
                                       otError              aError)
{
    static_cast<CoapSecureCli *>(aContext)->HandleClientResponse(aHeader,
                                                                 aMessage,
                                                                 aMessageInfo,
                                                                 aError);
}

void CoapSecureCli::HandleClientResponse(otCoapHeader * aHeader,
                                otMessage *             aMessage,
                                const otMessageInfo *   aMessageInfo,
                                otError                 aError)
{
    if (aError != OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("Error receiving coap secure response message: Error %d: %s\r\n", aError,
                                           otThreadErrorToString(aError));
    }
    else
    {
        mInterpreter.mServer->OutputFormat("Received coap secure response");
        PrintPayload(aMessage);
    }

    OT_UNUSED_VARIABLE(aHeader);
    OT_UNUSED_VARIABLE(aMessageInfo);
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
