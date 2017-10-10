/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file contains definitions for a CLI server on a UDP socket.
 */

#ifndef CLI_UDP_HPP_
#define CLI_UDP_HPP_

#include "openthread-core-config.h"

#include <openthread/types.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"

namespace ot {
namespace Cli {

/**
 * This class implements the CLI server on top of a UDP socket.
 *
 */
class Udp: public Server
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInstance     The OpenThread instance structure.
     * @param[in]  aInterpreter  The Interpreter structure.
     *
     */
    Udp(otInstance *aInstance, Interpreter *aInterpreter);

    /**
     * This method starts the CLI server.
     *
     * @retval OT_ERROR_NONE  Successfully started the server.
     *
     */
    otError Start(void);

    /**
     * This method delivers raw characters to the client.
     *
     * @param[in]  aBuf        A pointer to a buffer.
     * @param[in]  aBufLength  Number of bytes in the buffer.
     *
     * @returns The number of bytes placed in the output queue.
     *
     */
    virtual int Output(const char *aBuf, uint16_t aBufLength);

    /**
     * This method delivers formatted output to the client.
     *
     * @param[in]  aFmt  A pointer to the format string.
     * @param[in]  ...   A variable list of arguments to format.
     *
     * @returns The number of bytes placed in the output queue.
     *
     */
    virtual int OutputFormat(const char *fmt, ...);

private:
    enum
    {
        kMaxLineLength = 128,
    };

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo);

    otUdpSocket mSocket;
    otMessageInfo mPeer;
    otInstance *mInstance;
    Interpreter *mInterpreter;
};

}  // namespace Cli
}  // namespace ot

#endif  // CLI_UDP_HPP_
