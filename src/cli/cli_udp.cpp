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
 *   This file implements the CLI server on a UDP socket.
 */

#include  "openthread/openthread_enable_defines.h"

#include "cli_udp.hpp"

#include <stdarg.h>
#include <stdio.h>
#include "utils/wrap_string.h"

#include "cli/cli.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace Cli {

Udp::Udp(otInstance *aInstance, Interpreter *aInterpreter):
    mInstance(aInstance),
    mInterpreter(aInterpreter)
{
    memset(&mSocket, 0, sizeof(mSocket));
    memset(&mPeer, 0, sizeof(mPeer));
}

otError Udp::Start(void)
{
    otError error;

    otSockAddr sockaddr;
    memset(&sockaddr, 0, sizeof(otSockAddr));
    sockaddr.mPort = 7335;

    SuccessOrExit(error = otUdpOpen(mInstance, &mSocket, &Udp::HandleUdpReceive, this));
    SuccessOrExit(error = otUdpBind(&mSocket, &sockaddr));

exit:
    return error;
}

void Udp::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Udp *>(aContext)->HandleUdpReceive(aMessage, aMessageInfo);
}

void Udp::HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint16_t payloadLength = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    char buf[512];

    VerifyOrExit(payloadLength <= sizeof(buf));
    otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, payloadLength);

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
    return;
}

int Udp::Output(const char *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;
    otMessage *message;

    VerifyOrExit((message = otUdpNewMessage(mInstance, true)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = otMessageSetLength(message, aBufLength));
    otMessageWrite(message, 0, aBuf, aBufLength);
    SuccessOrExit(error = otUdpSend(&mSocket, message, &mPeer));

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        otMessageFree(message);
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
}  // namespace ot
