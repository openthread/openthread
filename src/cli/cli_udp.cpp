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

UdpExample::UdpExample(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
    , mLinkSecurityEnabled(true)
{
    ClearAllBytes(mSocket);
}

/**
 * @cli udp bind
 * @code
 * udp bind :: 1234
 * Done
 * @endcode
 * @code
 * udp bind -u :: 1234
 * Done
 * @endcode
 * @code
 * udp bind -b :: 1234
 * Done
 * @endcode
 * @cparam udp bind [@ca{netif}] @ca{ip} @ca{port}
 * - `netif`: The binding network interface, which is determined as follows:
 *   - No value (leaving out this parameter from the command): Thread network interface is used.
 *   - `-u`: Unspecified network interface, which means that the UDP/IPv6 stack determines which
 *   network interface to bind the socket to.
 *   - `-b`: Backbone network interface is used.
 * - `ip`: Unicast IPv6 address to bind to. If you wish to have the UDP/IPv6 stack assign the binding
 *   IPv6 address, or if you wish to bind to multicast IPv6 addresses, then you can use the following
 *   value to use the unspecified IPv6 address: `::`. Each example uses the unspecified IPv6 address.
 * - `port`: UDP port number to bind to. Each of the examples is using port number 1234.
 * @par
 * Assigns an IPv6 address and a port to an open socket, which binds the socket for communication.
 * Assigning the IPv6 address and port is referred to as naming the socket. @moreinfo{@udp}.
 * @sa otUdpBind
 */
template <> otError UdpExample::Process<Cmd("bind")>(Arg aArgs[])
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

/**
 * @cli udp connect
 * @code
 * udp connect fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234
 * Done
 * @endcode
 * @code
 * udp connect 172.17.0.1 1234
 * Connecting to synthesized IPv6 address: fdde:ad00:beef:2:0:0:ac11:1
 * Done
 * @endcode
 * @cparam udp connect @ca{ip} @ca{port}
 * The following parameters are required:
 * - `ip`: IP address of the peer.
 * - `port`: UDP port number of the peer.
 * The address can be an IPv4 address, which gets synthesized to an IPv6 address
 * using the preferred NAT64 prefix from the network data. The command returns
 * `InvalidState` when the preferred NAT64 prefix is unavailable.
 * @par api_copy
 * #otUdpConnect
 * @moreinfo{@udp}.
 */
template <> otError UdpExample::Process<Cmd("connect")>(Arg aArgs[])
{
    otError    error;
    otSockAddr sockaddr;
    bool       nat64Synth;

    SuccessOrExit(error = ParseToIp6Address(GetInstancePtr(), aArgs[0], sockaddr.mAddress, nat64Synth));

    if (nat64Synth)
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

/**
 * @cli udp close
 * @code
 * udp close
 * Done
 * @endcode
 * @par api_copy
 * #otUdpClose
 */
template <> otError UdpExample::Process<Cmd("close")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    return otUdpClose(GetInstancePtr(), &mSocket);
}

/**
 * @cli udp open
 * @code
 * udp open
 * Done
 * @endcode
 * @par api_copy
 * #otUdpOpen
 */
template <> otError UdpExample::Process<Cmd("open")>(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otError error;

    VerifyOrExit(!otUdpIsOpen(GetInstancePtr(), &mSocket), error = OT_ERROR_ALREADY);
    error = otUdpOpen(GetInstancePtr(), &mSocket, HandleUdpReceive, this);

exit:
    return error;
}

/**
 * @cli udp send
 * @code
 * udp send hello
 * Done
 * @endcode
 * @code
 * udp send -t hello
 * Done
 * @endcode
 * @code
 * udp send -x 68656c6c6f
 * Done
 * @endcode
 * @code
 * udp send -s 800
 * Done
 * @endcode
 * @code
 * udp send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 hello
 * Done
 * @endcode
 * @code
 * udp send 172.17.0.1 1234 hello
 * Sending to synthesized IPv6 address: fdde:ad00:beef:2:0:0:ac11:1
 * Done
 * @endcode
 * @code
 * udp send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 -t hello
 * Done
 * @endcode
 * @code
 * udp send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 -x 68656c6c6f
 * Done
 * @endcode
 * @code
 * udp send fdde:ad00:beef:0:bb1:ebd6:ad10:f33 1234 -s 800
 * Done
 * @endcode
 * @cparam udp send [@ca{ip} @ca{port}] [@ca{type}] @ca{value}
 * The `ip` and `port` are optional as a pair, but if you specify one you must
 * specify the other. If `ip` and `port` are not specified, the socket peer address
 * is used from `udp connect`.
 * - `ip`: Destination address. This address can be either an IPv4 or IPv6 address,
 *   An IPv4 address gets synthesized to an IPv6 address with the preferred
 *   NAT64 prefix from the network data. (If the preferred NAT64 prefix
 *   is unavailable, the command returns `InvalidState`).
 * - `port`: UDP destination port.
 * - `type`/`value` combinations:
 *   - `-t`: The payload in the `value` parameter is treated as text. If no `type` value
 *   is entered, the payload in the `value` parameter is also treated as text.
 *   - `-s`: Auto-generated payload with the specified length given in the `value` parameter.
 *   - `-x`: Binary data in hexadecimal representation given in the `value` parameter.
 * @par
 * Sends a UDP message using the socket. @moreinfo{@udp}.
 * @csa{udp open}
 * @csa{udp bind}
 * @csa{udp connect}
 * @sa otUdpSend
 */
template <> otError UdpExample::Process<Cmd("send")>(Arg aArgs[])
{
    otError           error   = OT_ERROR_NONE;
    otMessage        *message = nullptr;
    otMessageInfo     messageInfo;
    otMessageSettings messageSettings = {mLinkSecurityEnabled, OT_MESSAGE_PRIORITY_NORMAL};

    VerifyOrExit(otUdpIsOpen(GetInstancePtr(), &mSocket), error = OT_ERROR_INVALID_STATE);

    ClearAllBytes(messageInfo);

    // Possible argument formats:
    //
    // send             <text>
    // send             <type> <value>
    // send <ip> <port> <text>
    // send <ip> <port> <type> <value>

    if (!aArgs[2].IsEmpty())
    {
        bool nat64Synth;

        SuccessOrExit(error = ParseToIp6Address(GetInstancePtr(), aArgs[0], messageInfo.mPeerAddr, nat64Synth));

        if (nat64Synth)
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
        SuccessOrExit(error = PrepareHexStringPayload(*message, aArgs[1].GetCString()));
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

template <> otError UdpExample::Process<Cmd("linksecurity")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    /**
     * @cli udp linksecurity
     * @code
     * udp linksecurity
     * Enabled
     * Done
     * @endcode
     * @par
     * Indicates whether link security is enabled or disabled.
     */
    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(mLinkSecurityEnabled);
    }
    /**
     * @cli udp linksecurity (enable,disable)
     * @code
     * udp linksecurity enable
     * Done
     * @endcode
     * @code
     * udp linksecurity disable
     * Done
     * @endcode
     * @par
     * Enables or disables link security.
     */
    else
    {
        error = ParseEnableOrDisable(aArgs[0], mLinkSecurityEnabled);
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

otError UdpExample::PrepareHexStringPayload(otMessage &aMessage, const char *aHexString)
{
    static constexpr uint16_t kBufferSize = 50;

    otError  error;
    uint8_t  buf[kBufferSize];
    uint16_t length;
    bool     done = false;

    while (!done)
    {
        length = sizeof(buf);
        error  = ot::Utils::CmdLineParser::ParseAsHexStringSegment(aHexString, length, buf);

        VerifyOrExit((error == OT_ERROR_NONE) || (error == OT_ERROR_PENDING));
        done = (error == OT_ERROR_NONE);

        SuccessOrExit(error = otMessageAppend(&aMessage, buf, length));
    }

exit:
    return error;
}

otError UdpExample::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString)                                  \
    {                                                             \
        aCommandString, &UdpExample::Process<Cmd(aCommandString)> \
    }

    static constexpr Command kCommands[] = {
        CmdEntry("bind"),         CmdEntry("close"), CmdEntry("connect"),
        CmdEntry("linksecurity"), CmdEntry("open"),  CmdEntry("send"),
    };

    static_assert(BinarySearch::IsSorted(kCommands), "kCommands is not sorted");

    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty() || (aArgs[0] == "help"))
    {
        OutputCommandTable(kCommands);
        ExitNow(error = aArgs[0].IsEmpty() ? error : OT_ERROR_NONE);
    }

    command = BinarySearch::Find(aArgs[0].GetCString(), kCommands);
    VerifyOrExit(command != nullptr);

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
