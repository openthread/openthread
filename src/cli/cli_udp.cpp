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
 *   This file implements a simple CLI for the CoAP service.
 */

#include "cli_udp.hpp"

#include <openthread/message.h>
#include <openthread/udp.h>

#include "cli/cli.hpp"
#include "cli/cli_server.hpp"
#include "common/encoding.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Cli {

const struct UdpExample::Command UdpExample::sCommands[] = {
    {"help", &UdpExample::ProcessHelp},       {"bind", &UdpExample::ProcessBind}, {"close", &UdpExample::ProcessClose},
    {"connect", &UdpExample::ProcessConnect}, {"open", &UdpExample::ProcessOpen}, {"send", &UdpExample::ProcessSend}};

UdpExample::UdpExample(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
{
    memset(&mSocket, 0, sizeof(mSocket));
}

otError UdpExample::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError UdpExample::ProcessBind(int argc, char *argv[])
{
    otError    error;
    otSockAddr sockaddr;
    long       value;

    VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);

    memset(&sockaddr, 0, sizeof(sockaddr));

    error = otIp6AddressFromString(argv[0], &sockaddr.mAddress);
    SuccessOrExit(error);

    error = Interpreter::ParseLong(argv[1], value);
    SuccessOrExit(error);

    sockaddr.mPort = static_cast<uint16_t>(value);

    error = otUdpBind(&mSocket, &sockaddr);

exit:
    return error;
}

otError UdpExample::ProcessConnect(int argc, char *argv[])
{
    otError    error;
    otSockAddr sockaddr;
    long       value;

    VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);

    memset(&sockaddr, 0, sizeof(sockaddr));

    error = otIp6AddressFromString(argv[0], &sockaddr.mAddress);
    SuccessOrExit(error);

    error = Interpreter::ParseLong(argv[1], value);
    SuccessOrExit(error);

    sockaddr.mPort = static_cast<uint16_t>(value);

    error = otUdpConnect(&mSocket, &sockaddr);

exit:
    return error;
}

otError UdpExample::ProcessClose(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otUdpClose(&mSocket);
}

otError UdpExample::ProcessOpen(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    return otUdpOpen(mInterpreter.mInstance, &mSocket, HandleUdpReceive, this);
}

otError UdpExample::ProcessSend(int argc, char *argv[])
{
    otError       error;
    otMessageInfo messageInfo;
    otMessage *   message              = NULL;
    int           curArg               = 0;
    bool          autoGenPayload       = false;
    unsigned long autoGenPayloadLength = 0;

    memset(&messageInfo, 0, sizeof(messageInfo));

    VerifyOrExit(argc == 1 || argc == 3 || argc == 4, error = OT_ERROR_INVALID_ARGS);

    if (argc == 4)
    {
        if (strcmp(argv[curArg++], "-s") == 0)
        {
            autoGenPayload = true;
            error          = Interpreter::ParseUnsignedLong(argv[curArg++], autoGenPayloadLength);
            SuccessOrExit(error);
        }
        else
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }
    }

    if (argc >= 3)
    {
        long value;

        error = otIp6AddressFromString(argv[curArg++], &messageInfo.mPeerAddr);
        SuccessOrExit(error);

        error = Interpreter::ParseLong(argv[curArg++], value);
        SuccessOrExit(error);

        messageInfo.mPeerPort = static_cast<uint16_t>(value);
    }

    message = otUdpNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    if (autoGenPayload)
    {
        error = WriteCharToBuffer(message, static_cast<uint16_t>(autoGenPayloadLength));
    }
    else
    {
        error = otMessageAppend(message, argv[curArg], static_cast<uint16_t>(strlen(argv[curArg])));
    }
    SuccessOrExit(error);

    error = otUdpSend(&mSocket, message, &messageInfo);

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        otMessageFree(message);
    }

    return error;
}

otError UdpExample::WriteCharToBuffer(otMessage *aMessage, uint16_t aMessageSize)
{
    otError error     = OT_ERROR_NONE;
    uint8_t character = 0x30; // 0

    for (uint16_t index = 0; index < aMessageSize; index++)
    {
        SuccessOrExit(error = otMessageAppend(aMessage, &character, 1));
        character++;

        switch (character)
        {
        case 0x3A:            // 9
            character = 0x41; // A
            break;
        case 0x5B:            // Z
            character = 0x61; // a
            break;
        case 0x7B:            // z
            character = 0x30; // 0
            break;
        }
    }

exit:
    return error;
}

otError UdpExample::Process(int argc, char *argv[])
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

void UdpExample::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<UdpExample *>(aContext)->HandleUdpReceive(aMessage, aMessageInfo);
}

void UdpExample::HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint8_t buf[1500];
    int     length;

    mInterpreter.mServer->OutputFormat("%d bytes from ", otMessageGetLength(aMessage) - otMessageGetOffset(aMessage));
    mInterpreter.OutputIp6Address(aMessageInfo->mPeerAddr);
    mInterpreter.mServer->OutputFormat(" %d ", aMessageInfo->mPeerPort);

    length      = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';

    mInterpreter.mServer->OutputFormat("%s\r\n", buf);
}

} // namespace Cli
} // namespace ot
