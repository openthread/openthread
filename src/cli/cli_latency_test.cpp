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
 *   This file implements one-way latency test over UDP
 */

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC && OPENTHREAD_CONFIG_ENABLE_PERFORMANCE_TEST

#include "cli_latency_test.hpp"
#include "cli_udp_example.hpp"

#include <openthread/message.h>
#include <openthread/network_time.h>
#include <openthread/udp.h>

#include "cli/cli.hpp"
#include "common/encoding.hpp"

#define ETHERNET_MTU 1500

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Cli {

const struct LatencyTest::Command LatencyTest::sCommands[] = {
    {"help", &LatencyTest::ProcessHelp},   {"bind", &LatencyTest::ProcessBind},
    {"close", &LatencyTest::ProcessClose}, {"hoplimit", &LatencyTest::ProcessHopLimit},
    {"open", &LatencyTest::ProcessOpen},   {"result", &LatencyTest::ProcessResult},
    {"send", &LatencyTest::ProcessSend}};

LatencyTest::LatencyTest(Interpreter &aInterpreter)
    : UdpExample(aInterpreter)
    , mInterpreter(aInterpreter)
    , mHopLimit(0)
    , mLatency(0)
{
    memset(&mSocket, 0, sizeof(mSocket));
    mUdpReceviceCallback = static_cast<UdpExample::UdpReceiveHandler>(HandleUdpReceive);
}

otError LatencyTest::ProcessHelp(int argc, char *argv[])
{
    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return OT_ERROR_NONE;
}

otError LatencyTest::ProcessSend(int argc, char *argv[])
{
    otError       error;
    otMessageInfo messageInfo;
    otMessage *   message = NULL;
    int           curArg  = 0;
    uint64_t      txTimestamp;
    char          payload[ETHERNET_MTU];

    memset(&messageInfo, 0, sizeof(messageInfo));

    VerifyOrExit(argc == 3, error = OT_ERROR_INVALID_ARGS);
    {
        long value;

        error = otIp6AddressFromString(argv[curArg++], &messageInfo.mPeerAddr);
        SuccessOrExit(error);

        error = Interpreter::ParseLong(argv[curArg++], value);
        SuccessOrExit(error);

        messageInfo.mPeerPort    = static_cast<uint16_t>(value);
        messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;
    }

    message = otUdpNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    {
        long                length;
        otNetworkTimeStatus status;

        status = otNetworkTimeGet(mInterpreter.mInstance, txTimestamp);
        if (status != OT_NETWORK_TIME_SYNCHRONIZED)
        {
            txTimestamp = 0;
        }
        error = Interpreter::ParseLong(argv[curArg], length);
        memset(payload, 'T', (size_t)length);
        memcpy(payload, &txTimestamp, sizeof(uint64_t));

        error = otMessageAppend(message, payload, static_cast<uint16_t>(length));
        SuccessOrExit(error);
    }

    error = otUdpSend(&mSocket, message, &messageInfo);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        otMessageFree(message);
    }

    return error;
}

otError LatencyTest::Process(int argc, char *argv[])
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
                error = (this->*sCommands[i].mCommand)(argc - 1, argv + 1);
                break;
            }
        }
    }
    return error;
}

otError LatencyTest::ProcessResult(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);
    mInterpreter.mServer->OutputFormat("%u\r\n", mLatency);
    OT_UNUSED_VARIABLE(argv);

exit:
    return error;
}

otError LatencyTest::ProcessHopLimit(int argc, char *argv[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(argc == 0, error = OT_ERROR_INVALID_ARGS);
    mInterpreter.mServer->OutputFormat("%d\r\n", mHopLimit);
    OT_UNUSED_VARIABLE(argv);
exit:
    return error;
}

void LatencyTest::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<LatencyTest *>(aContext)->HandleUdpReceive(aMessage, aMessageInfo);
}

void LatencyTest::HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint8_t             buf[1500];
    uint64_t            rxTimestamp;
    uint64_t            txTimestamp;
    otNetworkTimeStatus status;

    otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);

    mHopLimit = aMessageInfo->mHopLimit;

    status = otNetworkTimeGet(mInterpreter.mInstance, rxTimestamp);
    if (status != OT_NETWORK_TIME_SYNCHRONIZED)
    {
        mLatency = 0;
        mInterpreter.mServer->OutputFormat("unsynchronized network time\r\n");
        return;
    }
    memcpy(&txTimestamp, buf, sizeof(uint64_t));
    mLatency = (uint32_t)(rxTimestamp - txTimestamp);
}

} // namespace Cli
} // namespace ot
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC && OPENTHREAD_CONFIG_ENABLE_PERFORMANCE_TEST
