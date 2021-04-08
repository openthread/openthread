/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file contains definitions for a simple TCP server and client.
 */

#ifndef CLI_TCP_EXAMPLE_HPP_
#define CLI_TCP_EXAMPLE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include <openthread/tcp.h>

#include "utils/lookup_table.hpp"

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements a CLI-based TCP example.
 *
 */
class TcpExample
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    explicit TcpExample(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aArgsLength  The number of elements in @p aArgs.
     * @param[in]  aArgs        An array of command line arguments.
     *
     */
    otError Process(uint8_t aArgsLength, char *aArgs[]);

private:
    struct Command
    {
        const char *mName;
        otError (TcpExample::*mHandler)(uint8_t aArgsLength, char *aArgs[]);
    };

    enum PayloadType
    {
        kTypeText      = 0,
        kTypeAutoSize  = 1,
        kTypeHexString = 2,
    };

    otError ProcessHelp(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessBind(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessClose(uint8_t aArgsLength, char **aArgs);
    otError ProcessConnect(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessInit(uint8_t aArgsLength, char **aArgs);
    otError ProcessRecv(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessListen(uint8_t aArgsLength, char **aArgs);
    otError ProcessSend(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessState(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessAbort(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessRoundTripTime(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessInfo(uint8_t aArgsLength, char *aArgs[]);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    otError ProcessEcho(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessEmit(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessSwallow(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessCounters(uint8_t aArgsLength, char *aArgs[]);
    otError ProcessResetNextSegment(uint8_t aArgsLength, char **aArgs);
    otError ProcessDrop(uint8_t aArgsLength, char *aArgs[]);
#endif

    static void TcpEventHandler(otTcpSocket *aSocket, otTcpSocketEvent aEvent);
    void        HandleTcpEvent(otTcpSocket &aSocket, otTcpSocketEvent aEvent);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    void     HandleEcho(void);
    uint16_t TryEchoWrite(void);
    void     HandleSwallow(void);
    void     HandleEmit(void);
#endif

    static constexpr Command sCommands[] = {
        {"abort", &TcpExample::ProcessAbort},
        {"bind", &TcpExample::ProcessBind},
        {"close", &TcpExample::ProcessClose},
        {"connect", &TcpExample::ProcessConnect},
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        {"counters", &TcpExample::ProcessCounters},
        {"drop", &TcpExample::ProcessDrop},
        {"echo", &TcpExample::ProcessEcho},
        {"emit", &TcpExample::ProcessEmit},
#endif
        {"help", &TcpExample::ProcessHelp},
        {"info", &TcpExample::ProcessInfo},
        {"init", &TcpExample::ProcessInit},
        {"listen", &TcpExample::ProcessListen},
        {"recv", &TcpExample::ProcessRecv},
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        {"reset-next-seg", &TcpExample::ProcessResetNextSegment},
#endif
        {"rtt", &TcpExample::ProcessRoundTripTime},
        {"send", &TcpExample::ProcessSend},
        {"state", &TcpExample::ProcessState},
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        {"swallow", &TcpExample::ProcessSwallow},
#endif
    };

    static_assert(Utils::LookupTable::IsSorted(sCommands), "Command Table is not sorted");

    Interpreter &mInterpreter;

    otTcpSocket mSocket;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    uint8_t  mEchoBuffer[128];
    uint8_t  mEchoBufferLength;
    bool     mEchoServerEnabled : 1;
    bool     mSwallowEnabled : 1;
    bool     mEmitEnabled : 1;
    uint32_t mEchoBytesSize;
    uint32_t mSwallowBytesSize;
    uint32_t mEmitBytesSize;
#endif
};

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_TCP_ENABLE

#endif // CLI_TCP_EXAMPLE_HPP_
