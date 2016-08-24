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
 *   This file implements the CLI server on a UDP socket.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <cli/cli.hpp>
#include <cli/cli_udp.hpp>
#include <common/code_utils.hpp>

namespace Thread {
namespace Cli {

Udp::Udp(otInstance *aInstance, Interpreter *aInterpreter):
    mInstance(aInstance),
    mInterpreter(aInterpreter)
{
}

ThreadError Udp::Start(void)
{
    ThreadError error;

    otSockAddr sockaddr;
    memset(&sockaddr, 0, sizeof(otSockAddr));
    sockaddr.mPort = 7335;

    SuccessOrExit(error = otOpenUdpSocket(mInstance, &mSocket, &Udp::HandleUdpReceive, this));
    SuccessOrExit(error = otBindUdpSocket(&mSocket, &sockaddr));

exit:
    return error;
}

void Udp::HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    Udp *obj = reinterpret_cast<Udp *>(aContext);
    obj->HandleUdpReceive(aMessage, aMessageInfo);
}

void Udp::HandleUdpReceive(otMessage aMessage, const otMessageInfo *aMessageInfo)
{
    uint16_t payloadLength = otGetMessageLength(aMessage) - otGetMessageOffset(aMessage);
    char buf[512];

    VerifyOrExit(payloadLength <= sizeof(buf), ;);
    otReadMessage(aMessage, otGetMessageOffset(aMessage), buf, payloadLength);

    if (buf[payloadLength - 1] == '\n')
    {
        buf[--payloadLength] = '\0';
    }

    if (buf[payloadLength - 1] == '\r')
    {
        buf[--payloadLength] = '\0';
    }

    mPeer = *aMessageInfo;

    mInterpreter->ProcessLine(buf, payloadLength, *this);

exit:
    {}
}

int Udp::Output(const char *aBuf, uint16_t aBufLength)
{
    ThreadError error = kThreadError_None;
    otMessage message;

    VerifyOrExit((message = otNewUdpMessage(mInstance)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = otSetMessageLength(message, aBufLength));
    otWriteMessage(message, 0, aBuf, aBufLength);
    SuccessOrExit(error = otSendUdp(&mSocket, message, &mPeer));

exit:

    if (error != kThreadError_None && message != NULL)
    {
        otFreeMessage(message);
        aBufLength = 0;
    }

    return aBufLength;
}

int Udp::OutputFormat(const char *fmt, ...)
{
    char buf[kMaxLineLength];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return Output(buf, static_cast<uint16_t>(strlen(buf)));
}

}  // namespace Cli
}  // namespace Thread
