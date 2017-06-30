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
 *   This file contains definitions for a CLI server on the UART service.
 */

#ifndef CLI_UART_HPP_
#define CLI_UART_HPP_

#include <openthread/types.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
#include "common/tasklet.hpp"

namespace ot {
namespace Cli {

/**
 * This class implements the CLI server on top of the UART platform abstraction.
 *
 */
class Uart: public Server
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     *
     */
    Uart(otInstance *aInstance);

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

    /**
     * This method delivers formatted output to the client.
     *
     * @param[in]  aFmt  A pointer to the format string.
     * @param[in]  aAp   A variable list of arguments for format.
     *
     * @returns The number of bytes placed in the output queue.
     *
     */
    int OutputFormatV(const char *aFmt, va_list aAp);

    /**
     * This method returns a reference to the interpreter object.
     *
     * @returns A reference to the interpreter object.
     *
     */
    Interpreter &GetInterpreter(void) { return mInterpreter; }

    void ReceiveTask(const uint8_t *aBuf, uint16_t aBufLength);
    void SendDoneTask(void);

    static Uart *sUartServer;

private:
    enum
    {
        kRxBufferSize = 512,
        kTxBufferSize = 1024,
        kMaxLineLength = 128,
    };

    otError ProcessCommand(void);
    void Send(void);

    char mRxBuffer[kRxBufferSize];
    uint16_t mRxLength;

    char mTxBuffer[kTxBufferSize];
    uint16_t mTxHead;
    uint16_t mTxLength;

    uint16_t mSendLength;

    Interpreter mInterpreter;

    friend class Interpreter;
};

}  // namespace Cli
}  // namespace ot

#endif  // CLI_UART_HPP_
