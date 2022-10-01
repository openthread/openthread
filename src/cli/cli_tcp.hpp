/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file contains definitions for a TCP CLI tool.
 */

#ifndef CLI_TCP_EXAMPLE_HPP_
#define CLI_TCP_EXAMPLE_HPP_

#include "openthread-core-config.h"

#include <openthread/tcp.h>
#include <openthread/tcp_ext.h>

#include "cli/cli_config.h"
#include "cli/cli_output.hpp"
#include "common/time.hpp"

namespace ot {
namespace Cli {

/**
 * This class implements a CLI-based TCP example.
 *
 */
class TcpExample : private OutputWrapper
{
public:
    using Arg = Utils::CmdLineParser::Arg;

    /**
     * Constructor
     *
     * @param[in]  aOutput  The CLI console output context.
     *
     */
    explicit TcpExample(Output &aOutput);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  aArgs   An array of command line arguments.
     *
     */
    otError Process(Arg aArgs[]);

private:
    using Command = CommandEntry<TcpExample>;

    template <CommandId kCommandId> otError Process(Arg aArgs[]);

    otError ContinueBenchmarkCircularSend(void);
    void    CompleteBenchmark(void);

    static void HandleTcpEstablishedCallback(otTcpEndpoint *aEndpoint);
    static void HandleTcpSendDoneCallback(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData);
    static void HandleTcpForwardProgressCallback(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog);
    static void HandleTcpReceiveAvailableCallback(otTcpEndpoint *aEndpoint,
                                                  size_t         aBytesAvailable,
                                                  bool           aEndOfStream,
                                                  size_t         aBytesRemaining);
    static void HandleTcpDisconnectedCallback(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason);
    static otTcpIncomingConnectionAction HandleTcpAcceptReadyCallback(otTcpListener *   aListener,
                                                                      const otSockAddr *aPeer,
                                                                      otTcpEndpoint **  aAcceptInto);
    static void                          HandleTcpAcceptDoneCallback(otTcpListener *   aListener,
                                                                     otTcpEndpoint *   aEndpoint,
                                                                     const otSockAddr *aPeer);

    void HandleTcpEstablished(otTcpEndpoint *aEndpoint);
    void HandleTcpSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData);
    void HandleTcpForwardProgress(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog);
    void HandleTcpReceiveAvailable(otTcpEndpoint *aEndpoint,
                                   size_t         aBytesAvailable,
                                   bool           aEndOfStream,
                                   size_t         aBytesRemaining);
    void HandleTcpDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason);
    otTcpIncomingConnectionAction HandleTcpAcceptReady(otTcpListener *   aListener,
                                                       const otSockAddr *aPeer,
                                                       otTcpEndpoint **  aAcceptInto);
    void HandleTcpAcceptDone(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *aPeer);

    otTcpEndpoint mEndpoint;
    otTcpListener mListener;

    bool mInitialized;
    bool mEndpointConnected;
    bool mSendBusy;
    bool mUseCircularSendBuffer;

    otTcpCircularSendBuffer mSendBuffer;
    otLinkedBuffer          mSendLink;
    uint8_t                 mSendBufferBytes[OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE];
    uint8_t                 mReceiveBufferBytes[OPENTHREAD_CONFIG_CLI_TCP_RECEIVE_BUFFER_SIZE];

    otLinkedBuffer
              mBenchmarkLinks[(sizeof(mReceiveBufferBytes) + sizeof(mSendBufferBytes) - 1) / sizeof(mSendBufferBytes)];
    uint32_t  mBenchmarkBytesTotal;
    uint32_t  mBenchmarkBytesUnsent;
    TimeMilli mBenchmarkStart;

    static constexpr const char * sBenchmarkData       = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    static constexpr const size_t sBenchmarkDataLength = 52;
};

} // namespace Cli
} // namespace ot

#endif // CLI_TCP_EXAMPLE_HPP_
