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

namespace Thread {
namespace Cli {

/**
 * This structure represents a CLI command.
 *
 */
struct CoapCommand
{
    const char *mName;                                 ///< A pointer to the command string.
    ThreadError(*mCommand)(int argc, char *argv[]);    ///< A function pointer to process the command.
};

/**
 * This class implements the CLI CoAP server and client.
 *
 */
class Coap
{
public:
    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    static ThreadError Process(otInstance *aInstance, int argc, char *argv[], Server &aServer, otCoapResource &aRessource);

private:
    static void PrintPayload(otMessage *aMessage);

    static ThreadError ProcessClient(int argc, char *argv[]);
    static ThreadError ProcessServer(int argc, char *argv[]);

    static void OTCALL s_HandleServerResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                              otMessageInfo *aMessageInfo);
    static void HandleServerResponse(otCoapHeader *aHeader, otMessage *aMessage, otMessageInfo *aMessageInfo);
    static void OTCALL s_HandleClientResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                              otMessageInfo *aMessageInfo, ThreadError aResult);
    static void HandleClientResponse(otCoapHeader *aHeader, otMessage *aMessage, otMessageInfo *aMessageInfo,
                                     ThreadError aResult);

    static const CoapCommand sCommands[];
    static Server *sServer;
    static otCoapResource *sResource;
    static otInstance *sInstance;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_COAP_HPP_
