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
 *   This file contains definitions for a simple CLI EST over CoAP Secure client.
 */

#ifndef CLI_EST_CLIENT_HPP_
#define CLI_EST_CLIENT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_EST_CLIENT_ENABLE

#include "est/est_client.hpp"
#include "openthread/crypto.h"

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the CLI EST over CoAP Secure client.
 *
 */
class EstClient
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    explicit EstClient(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    otError Process(int argc, char *argv[]);

private:
    struct Command
    {
        const char *mName;
        otError (EstClient::*mCommand)(int argc, char *argv[]);
    };

    otError ProcessHelp(int argc, char *argv[]);
    otError ProcessStart(int argc, char *argv[]);
    otError ProcessStop(int argc, char *argv[]);
    otError ProcessConnect(int argc, char *argv[]);
    otError ProcessDisconnect(int argc, char *argv[]);
    otError ProcessGetCaCertificate(int argc, char *argv[]);
    otError ProcessGetCsrAttributes(int argc, char *argv[]);
    otError ProcessSimpleEnroll(int argc, char *argv[]);
    otError ProcessSimpleReEnroll(int argc, char *argv[]);

    static void HandleConnected(bool aConnected, void *aContext);
    void        HandleConnected(bool aConnected);
    static void HandleResponse(otError   aError,
                               otEstType aType,
                               uint8_t * aPayload,
                               uint32_t  aPayloadLenth,
                               void *    aContext);
    void        HandleResponse(otError aError, otEstType aType, uint8_t *aPayload, uint32_t aPayloadLenth);
    void        CleanUpTemporaryBuffer(void);
    otError     PrintoutCsrAttributes(uint8_t *aData, const uint8_t *aDataEnd);

    static const Command sCommands[];
    Interpreter &        mInterpreter;
    bool                 mStopFlag;
    uint32_t             mCaCertificateLength;
    uint32_t             mPrivateKeyTempLength;
    uint32_t             mPublicKeyTempLength;
    uint32_t             mPrivateKeyLength;
    uint32_t             mPublicKeyLength;
    uint32_t             mOpCertificateLength;
    uint8_t              mCaCertificate[1024];
    uint8_t              mPrivateKeyTemp[512];
    uint8_t              mPublicKeyTemp[512];
    uint8_t              mPrivateKey[512];
    uint8_t              mPublicKey[512];
    uint8_t              mOpCertificate[1024];
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_EST_CLIENT_ENABLE

#endif // CLI_EST_CLIENT_HPP_
