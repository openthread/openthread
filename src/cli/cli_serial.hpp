/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file contains definitions for a CLI server on the serial service.
 */

#ifndef CLI_SERIAL_HPP_
#define CLI_SERIAL_HPP_

#include <openthread-types.h>
#include <cli/cli_server.hpp>
#include <common/tasklet.hpp>

namespace Thread {
namespace Cli {

/**
 * This class implements the CLI server on top of the serial platform abstraction.
 *
 */
class Serial: public Server
{
public:
    Serial(void);

    /**
     * This method starts the CLI server.
     *
     * @retval kThreadError_None  Successfully started the server.
     *
     */
    ThreadError Start(void);

    /**
     * This method delivers output to the client.
     *
     * @param[in]  aBuf        A pointer to a buffer.
     * @param[in]  aBufLength  Number of bytes in the buffer.
     *
     * @retval kThreadError_None  Successfully delivered output the client.
     *
     */
    ThreadError Output(const char *aBuf, uint16_t aBufLength);

    static Tasklet sReceiveTask;

private:
    enum
    {
        kRxBufferSize = 128,
    };

    static void ReceiveTask(void *aContext);

    ThreadError ProcessCommand(void);
    void ReceiveTask(void);

    char mRxBuffer[kRxBufferSize];
    uint16_t mRxLength;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_SERIAL_HPP_
