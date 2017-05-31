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
 *   This file contains definitions for a simple CLI CoAP server and client.
 */

#ifndef CLI_COAP_HPP_
#define CLI_COAP_HPP_

#include <openthread/types.h>

#include "coap/coap_header.hpp"

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements the CLI CoAP server and client.
 *
 */
class Coap
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    Coap(Interpreter &aInterpreter): mInterpreter(aInterpreter) { }

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
        kMaxUriLength = 32,
        kMaxBufferSize = 16
    };

    void PrintPayload(otMessage *aMessage) const;

    otError ProcessRequest(int argc, char *argv[]);

    static void OTCALL HandleServerResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                            const otMessageInfo *aMessageInfo);
    void HandleServerResponse(otCoapHeader *aHeader, otMessage *aMessage, const otMessageInfo *aMessageInfo);

    static void OTCALL HandleClientResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                            const otMessageInfo *aMessageInfo, otError aError);
    void HandleClientResponse(otCoapHeader *aHeader, otMessage *aMessage, const otMessageInfo *aMessageInfo,
                              otError aError);

    otCoapResource mResource;
    char mUriPath[kMaxUriLength];

    Interpreter &mInterpreter;
};

}  // namespace Cli
}  // namespace ot

#endif  // CLI_COAP_HPP_
