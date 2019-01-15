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
 *   This file contains definitions for a simple CLI CoAP Secure server and client.
 */

#ifndef CLI_COAP_SECURE_HPP_
#define CLI_COAP_SECURE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#include "coap/coap_message.hpp"
#include "coap/coap_secure.hpp"

/**
 * to test the default handler for not handled requests set to 1.
 */
#define CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER 0

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the CLI CoAP Secure server and client.
 *
 */
class CoapSecure
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    CoapSecure(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    otError Process(int argc, char *argv[]);

private:
    enum
    {
        kMaxUriLength   = 32,
        kMaxBufferSize  = 16,
        kPskMaxLength   = 32,
        kPskIdMaxLength = 32
    };

    void PrintHeaderInfos(otMessage *aMessage) const;

    void PrintPayload(otMessage *aMessage) const;

    otError ProcessRequest(int argc, char *argv[]);

    otError Stop(void);

    static void OTCALL HandleServerResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void               HandleServerResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo);

    static void OTCALL HandleClientResponse(void *               aContext,
                                            otMessage *          aMessage,
                                            const otMessageInfo *aMessageInfo,
                                            otError              aError);
    void               HandleClientResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError);

#if CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
    static void OTCALL DefaultHandle(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void               DefaultHandle(otMessage *aMessage, const otMessageInfo *aMessageInfo);
#endif // CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER

    static void OTCALL HandleClientConnect(bool aConnected, void *aContext);
    void               HandleClientConnect(bool aConnected);

    otCoapResource mResource;
    char           mUriPath[kMaxUriLength];

    Interpreter &mInterpreter;

    bool    mShutdownFlag;
    bool    mUseCertificate;
    uint8_t mPsk[kPskMaxLength];
    uint8_t mPskLength;
    uint8_t mPskId[kPskIdMaxLength];
    uint8_t mPskIdLength;
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE

#endif // CLI_COAP_SECURE_HPP_
