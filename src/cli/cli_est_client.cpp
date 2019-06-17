/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements a simple CLI for the EST Client service.
 */

#include "cli_est_client.hpp"

#if OPENTHREAD_ENABLE_EST_CLIENT

#include <mbedtls/debug.h>
#include <openthread/ip6.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
#include "x509_cert_key.hpp"

namespace ot {
namespace Cli {

const struct EstClient::Command EstClient::sCommands[] = {
    {"help", &EstClient::ProcessHelp},
    {"start", &EstClient::ProcessStart},
    {"stop", &EstClient::ProcessStop},
    {"connect", &EstClient::ProcessConnect},
    {"disconnect", &EstClient::ProcessDisconnect},
    {"enroll", &EstClient::ProcessSimpleEnroll},
    {"reenroll", &EstClient::ProcessSimpleReEnroll},

};

EstClient::EstClient(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
    , mStopFlag(false)
{
}

otError EstClient::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError EstClient::ProcessStart(int argc, char *argv[])
{
    otError error;
    bool    mVerifyPeerCert = false;

    if (argc > 1)
    {
        if (strcmp(argv[1], "true") == 0)
        {
            mVerifyPeerCert = true;
        }
        else if (strcmp(argv[1], "false") != 0)
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    SuccessOrExit(error = otEstClientStart(mInterpreter.mInstance, mVerifyPeerCert));

exit:
    return error;
}

otError EstClient::ProcessStop(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    if (otEstClientIsConnected(mInterpreter.mInstance))
    {
        otEstClientDisconnect(mInterpreter.mInstance);
        mStopFlag = true;
    }
    else
    {
        otEstClientStop(mInterpreter.mInstance);
    }

    return OT_ERROR_NONE;
}

otError EstClient::ProcessConnect(int argc, char *argv[])
{
    otError mError;
    otSockAddr mServerAddress;

    mServerAddress.mScopeId = OT_NETIF_INTERFACE_ID_THREAD;
    memset(&mServerAddress, 0, sizeof(mServerAddress));

    // Destination IPv6 address
    if (argc > 1)
    {
        SuccessOrExit(mError = otIp6AddressFromString(argv[1], &mServerAddress.mAddress));
    }
    else
    {
        SuccessOrExit(mError = otIp6AddressFromString((const char*)OT_EST_COAPS_DEFAULT_EST_SERVER_IP6, &mServerAddress.mAddress));
    }

    // check for port specification
    if (argc > 2)
    {
        long value;

        SuccessOrExit(mError = Interpreter::ParseLong(argv[2], value));
        mServerAddress.mPort = static_cast<uint16_t>(value);
    }
    else
    {
        mServerAddress.mPort    = OT_EST_COAPS_DEFAULT_EST_SERVER_PORT;
    }

    SuccessOrExit(mError = otEstClientSetCaCertificateChain(mInterpreter.mInstance,
                                                            (const uint8_t*) OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE,
                                                            sizeof(OT_CLI_COAPS_TRUSTED_ROOT_CERTIFICATE)));

    SuccessOrExit(mError = otEstClientSetCertificate(mInterpreter.mInstance,
                                                     (const uint8_t *) OT_CLI_COAPS_X509_CERT,
                                                     sizeof(OT_CLI_COAPS_X509_CERT),
                                                     (const uint8_t*) OT_CLI_COAPS_PRIV_KEY,
                                                     sizeof(OT_CLI_COAPS_PRIV_KEY)));

    SuccessOrExit(mError = otEstClientConnect(mInterpreter.mInstance,
                                              &mServerAddress,
                                              &EstClient::HandleConnected,
                                              &EstClient::HandleResponse,
                                              this));

exit:
    return mError;
}

otError EstClient::ProcessDisconnect(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otEstClientDisconnect(mInterpreter.mInstance);

    return OT_ERROR_NONE;
}

otError EstClient::ProcessSimpleEnroll(int argc, char *argv[])
{
    otError mError;
    uint32_t mLength = 0;
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    mError = otEstClientGenerateKeyPair(mInterpreter.mInstance, (const uint8_t*) "hallo", &mLength);
    VerifyOrExit(mError == OT_ERROR_NONE);
    mError = otEstClientSimpleEnroll(mInterpreter.mInstance);
    VerifyOrExit(mError == OT_ERROR_NONE);

exit:

    return mError;
}

otError EstClient::ProcessSimpleReEnroll(int argc, char *argv[])
{
    otError mError;
    uint32_t mLength = 0;
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    mError = otEstClientGenerateKeyPair(mInterpreter.mInstance, (const uint8_t*) "hallo", &mLength);
    VerifyOrExit(mError == OT_ERROR_NONE);
    mError = otEstClientSimpleReEnroll(mInterpreter.mInstance);
    VerifyOrExit(mError == OT_ERROR_NONE);

exit:

    return mError;
}

otError EstClient::Process(int argc, char *argv[])
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

void EstClient::HandleConnected(bool aConnected, void *aContext)
{
    static_cast<EstClient *>(aContext)->HandleConnected(aConnected);
}

void EstClient::HandleConnected(bool aConnected)
{
    if (aConnected)
    {
        mInterpreter.mServer->OutputFormat("connected\r\n");
    }
    else
    {
        mInterpreter.mServer->OutputFormat("disconnected\r\n");

        if (mStopFlag)
        {
            otEstClientStop(mInterpreter.mInstance);
            mStopFlag = false;
        }
    }
}

void EstClient::HandleResponse(otError aError, otEstType aType, uint8_t *aPayload, uint32_t aPayloadLenth, void *aContext)
{
    static_cast<EstClient *>(aContext)->HandleResponse(aError, aType, aPayload, aPayloadLenth);
}

void EstClient::HandleResponse(otError aError, otEstType aType, uint8_t *aPayload, uint32_t aPayloadLength)
{
    OT_UNUSED_VARIABLE(aPayload);
    OT_UNUSED_VARIABLE(aPayloadLength);

    if (aError != OT_ERROR_NONE)
    {
        mInterpreter.mServer->OutputFormat("error");
    }

    switch (aType)
    {
        case OT_EST_TYPE_CA_CERTS:
            break;
        case OT_EST_TYPE_CSR_ATTR:
            break;
        case OT_EST_TYPE_SERVER_SIDE_KEY:
            break;
        case OT_EST_TYPE_SIMPLE_ENROLL:
            break;
        case OT_EST_TYPE_SIMPLE_REENROLL:
            break;
        case OT_EST_TYPE_INVALID_CERT:
        case OT_EST_TYPE_INVALID_KEY:
        default:
            mInterpreter.mServer->OutputFormat("error param");
    }
    mInterpreter.mServer->OutputFormat("response\r\n");
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_EST_CLIENT
