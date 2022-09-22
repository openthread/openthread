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
#include <openthread/nat64.h>
#include <openthread/udp.h>

#include "cli/cli.hpp"
#include "common/encoding.hpp"

namespace ot {
namespace Cli {

constexpr UdpExample::Command UdpExample::sCommands[];

UdpExample::UdpExample(Output &aOutput)
    : OutputWrapper(aOutput)
    , mLinkSecurityEnabled(true)
{
    memset(&mSocket, 0, sizeof(mSocket));
}

otError UdpExample::ProcessHelp(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

otError UdpExample::ProcessBind(Arg aArgs[])
{
    otError           error;
    otSockAddr        sockaddr;
    otNetifIdentifier netif = OT_NETIF_THREAD;

    if (aArgs[0] == "-u")
    {
        netif = OT_NETIF_UNSPECIFIED;
        aArgs++;
    }
    else if (aArgs[0] == "-b")
    {
        netif = OT_NETIF_BACKBONE;
        aArgs++;
    }

    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(sockaddr.mAddress));
    SuccessOrExit(error = aArgs[1].ParseAsUint16(sockaddr.mPort));
    VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    error = otUdpBind(GetInstancePtr(), &mSocket, &sockaddr, netif);

exit:
    return error;
}

otError UdpExample::ProcessConnect(Arg aArgs[])
{
    otError    error;
    otSockAddr sockaddr;
    bool       nat64SynthesizedAddress;

    SuccessOrExit(
        error = Interpreter::ParseToIp6Address(GetInstancePtr(), aArgs[0], sockaddr.mAddress, nat64SynthesizedAddress));
    if (nat64SynthesizedAddress)
    {
        OutputFormat("Connecting to synthesized IPv6 address: ");
        OutputIp6AddressLine(sockaddr.mAddress);
    }

    SuccessOrExit(error = aArgs[1].ParseAsUint16(sockaddr.mPort));
    VerifyOrExit(aArgs[2].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    error = otUdpConnect(GetInstancePtr(), &mSocket, &sockaddr);

exit:
    return error;
}

otError UdpExample::ProcessClose(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    return otUdpClose(GetInstancePtr(), &mSocket);
}

otError UdpExample::ProcessOpen(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error;

    VerifyOrExit(!otUdpIsOpen(GetInstancePtr(), &mSocket), error = OT_ERROR_ALREADY);
    error = otUdpOpen(GetInstancePtr(), &mSocket, HandleUdpReceive, this);

exit:
    return error;
}

otError UdpExample::ProcessSend(Arg aArgs[])
{
    otError           error   = OT_ERROR_NONE;
    otMessage *       message = nullptr;
    otMessageInfo     messageInfo;
    otMessageSettings messageSettings = {mLinkSecurityEnabled, OT_MESSAGE_PRIORITY_NORMAL};

    memset(&messageInfo, 0, sizeof(messageInfo));

    // Possible argument formats:
    //
    // send             <text>
    // send             <type> <value>
    // send <ip> <port> <text>
    // send <ip> <port> <type> <value>

    if (!aArgs[2].IsEmpty())
    {
        bool nat64SynthesizedAddress;

        SuccessOrExit(error = Interpreter::ParseToIp6Address(GetInstancePtr(), aArgs[0], messageInfo.mPeerAddr,
                                                             nat64SynthesizedAddress));
        if (nat64SynthesizedAddress)
        {
            OutputFormat("Sending to synthesized IPv6 address: ");
            OutputIp6AddressLine(messageInfo.mPeerAddr);
        }

        SuccessOrExit(error = aArgs[1].ParseAsUint16(messageInfo.mPeerPort));
        aArgs += 2;
    }

    message = otUdpNewMessage(GetInstancePtr(), &messageSettings);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    if (aArgs[0] == "-s")
    {
        // Auto-generated payload with a given length

        uint16_t payloadLength;

        SuccessOrExit(error = aArgs[1].ParseAsUint16(payloadLength));
        SuccessOrExit(error = PrepareAutoGeneratedPayload(*message, payloadLength));
    }
    else if (aArgs[0] == "-x")
    {
        // Binary hex data payload

        VerifyOrExit(!aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = PrepareHexStringPaylod(*message, aArgs[1].GetCString()));
    }
    else
    {
        // Text payload (same as without specifying the type)

        if (aArgs[0] == "-t")
        {
            aArgs++;
        }

        VerifyOrExit(!aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = otMessageAppend(message, aArgs[0].GetCString(), aArgs[0].GetLength()));
    }

    SuccessOrExit(error = otUdpSend(GetInstancePtr(), &mSocket, message, &messageInfo));

    message = nullptr;

exit:
    if (message != nullptr)
    {
        otMessageFree(message);
    }

    return error;
}

otError UdpExample::ProcessLinkSecurity(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(mLinkSecurityEnabled);
    }
    else
    {
        error = Interpreter::ParseEnableOrDisable(aArgs[0], mLinkSecurityEnabled);
    }

    return error;
}

otError UdpExample::PrepareAutoGeneratedPayload(otMessage &aMessage, uint16_t aPayloadLength)
{
    otError error     = OT_ERROR_NONE;
    uint8_t character = '0';

    for (; aPayloadLength != 0; aPayloadLength--)
    {
        SuccessOrExit(error = otMessageAppend(&aMessage, &character, sizeof(character)));

        switch (character)
        {
        case '9':
            character = 'A';
            break;
        case 'Z':
            character = 'a';
            break;
        case 'z':
            character = '0';
            break;
        default:
            character++;
            break;
        }
    }

exit:
    return error;
}

otError UdpExample::PrepareHexStringPaylod(otMessage &aMessage, const char *aHexString)
{
    enum : uint8_t
    {
        kBufferSize = 50,
    };

    otError  error;
    uint8_t  buf[kBufferSize];
    uint16_t length;
    bool     done = false;

    while (!done)
    {
        length = sizeof(buf);
        error  = Utils::CmdLineParser::ParseAsHexStringSegment(aHexString, length, buf);

        VerifyOrExit((error == OT_ERROR_NONE) || (error == OT_ERROR_PENDING));
        done = (error == OT_ERROR_NONE);

        SuccessOrExit(error = otMessageAppend(&aMessage, buf, length));
    }

exit:
    return error;
}

otError UdpExample::Process(Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_ARGS;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        IgnoreError(ProcessHelp(aArgs));
        ExitNow();
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr, error = OT_ERROR_INVALID_COMMAND);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

void UdpExample::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<UdpExample *>(aContext)->HandleUdpReceive(aMessage, aMessageInfo);
}

void UdpExample::HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    char buf[1500];
    int  length;

    OutputFormat("%d bytes from ", otMessageGetLength(aMessage) - otMessageGetOffset(aMessage));
    OutputIp6Address(aMessageInfo->mPeerAddr);
    OutputFormat(" %d ", aMessageInfo->mPeerPort);

    length      = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
    buf[length] = '\0';

    OutputLine("%s", buf);
}

} // namespace Cli
} // namespace ot
