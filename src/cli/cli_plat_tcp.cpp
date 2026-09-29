/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file implements a TCP CLI tool.
 */

#include "openthread-core-config.h"

#include "cli_config.h"

#if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_PLAT_TCP_ENABLE

#include "cli_plat_tcp.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace Cli {

//---------------------------------------------------------------------------------------------------------------------
// PlatTcp::Listener

PlatTcp::Listener::Listener(Instance &aInstance, PlatTcp &aOwner)
    : Ip6::PlatTcp::Listener(aInstance, Accept, nullptr)
    , mOwner(aOwner)
{
}

Ip6::PlatTcp::Connection *PlatTcp::Listener::Accept(Ip6::PlatTcp::Listener &aListener, const SockAddr &aPeerSockAddr)
{
    return static_cast<Listener &>(aListener).mOwner.Accept(aPeerSockAddr);
}

//---------------------------------------------------------------------------------------------------------------------
// PlatTcp::Connection

PlatTcp::Connection::Connection(Instance &aInstance, PlatTcp &aOwner)
    : Ip6::PlatTcp::Connection(aInstance, HandleEvent, nullptr)
    , mOwner(aOwner)
{
}

void PlatTcp::Connection::HandleEvent(Ip6::PlatTcp::Connection &aConnection, const Event aEvent)
{
    return static_cast<Connection &>(aConnection).mOwner.HandleEvent(aEvent);
}

//---------------------------------------------------------------------------------------------------------------------
// PlatTcp

PlatTcp::PlatTcp(otInstance *aInstance, OutputImplementer &aOutputImplementer)
    : Utils(aInstance, aOutputImplementer)
    , mListener(AsCoreType(aInstance), *this)
    , mConnection(AsCoreType(aInstance), *this)
    , mIsBound(false)
{
}

PlatTcp::Connection *PlatTcp::Accept(const SockAddr &aPeerSockAddr)
{
    OutputLine("PlatTcp: Accept %s", aPeerSockAddr.ToString().AsCString());

    return &mConnection;
}

void PlatTcp::HandleEvent(Connection::Event aEvent)
{
    OutputLine("PlatTcp:: HandleEvent %s", Connection::EventToString(aEvent));

    if (aEvent == Connection::kEventDisconnected)
    {
        OutputLine("PlatTcp:: DisconnectReason: %s",
                   Connection::DisconnectReasonToString(mConnection.GetDisconnectReason()));
    }

    if (aEvent == Connection::kEventReceive)
    {
        const Message *rxMessage = mConnection.GetRxMessage();

        VerifyOrExit(rxMessage != nullptr);
        OutputLine("PlatTcp: Received %u bytes", rxMessage->GetLength());

        for (uint16_t offset = 0; offset < rxMessage->GetLength();)
        {
            uint8_t  buffer[32];
            uint16_t readLength = rxMessage->ReadBytes(offset, buffer, sizeof(buffer));

            OutputSpaces(4);
            OutputBytesLine(buffer, readLength);

            offset += readLength;
        }

        mConnection.FreeRxMessage();
    }

exit:
    return;
}

otError PlatTcp::GetSockAddr(Arg aArgs[], SockAddr &aSockAddr)
{
    otError  error = OT_ERROR_NONE;
    uint16_t port;
    uint32_t ifIndex;

    aSockAddr.Clear();

    VerifyOrExit(!aArgs[0].IsEmpty());
    SuccessOrExit(error = aArgs[0].ParseAsIp6Address(aSockAddr.GetAddress()));

    VerifyOrExit(!aArgs[1].IsEmpty());
    SuccessOrExit(error = aArgs[1].ParseAsUint16(port));
    aSockAddr.SetPort(port);

    VerifyOrExit(!aArgs[2].IsEmpty());
    SuccessOrExit(error = aArgs[2].ParseAsUint32(ifIndex));
    aSockAddr.SetIfIndex(ifIndex);

exit:
    return error;
}

template <> otError PlatTcp::Process<Cmd("listener")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        OutputEnabledDisabledStatus(mListener.GetState() == Listener::kStateEnabled);
    }
    else if (aArgs[0] == "enable")
    {
        SockAddr sockAddr;

        SuccessOrExit(error = GetSockAddr(&aArgs[1], sockAddr));
        OutputLine("Enabling listener on %s", sockAddr.ToString().AsCString());
        error = mListener.Enable(sockAddr);
    }
    else if (aArgs[0] == "disable")
    {
        VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
        mListener.Disable();
    }
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
    }

exit:
    return error;
}

template <> otError PlatTcp::Process<Cmd("bind")>(Arg aArgs[])
{
    otError  error = OT_ERROR_NONE;
    SockAddr newSockAddr;

    VerifyOrExit(mConnection.GetState() == Connection::kStateUnused, error = OT_ERROR_INVALID_STATE);

    if (aArgs[0].IsEmpty())
    {
        mIsBound = false;
        mLocalSockAddr.Clear();
        ExitNow();
    }

    SuccessOrExit(error = GetSockAddr(aArgs, newSockAddr));
    mLocalSockAddr = newSockAddr;
    mIsBound       = true;
    OutputLine("LocalSockAddr: %s", mLocalSockAddr.ToString().AsCString());

exit:
    return error;
}

template <> otError PlatTcp::Process<Cmd("connect")>(Arg aArgs[])
{
    otError  error = OT_ERROR_NONE;
    SockAddr peerSockAddr;

    SuccessOrExit(error = GetSockAddr(aArgs, peerSockAddr));

    if (mIsBound)
    {
        OutputLine("Binding to %s, connecting to %s", mLocalSockAddr.ToString().AsCString(),
                   peerSockAddr.ToString().AsCString());
        error = mConnection.BindAndConnect(mLocalSockAddr, peerSockAddr);
    }
    else
    {
        OutputLine("Connecting to %s", peerSockAddr.ToString().AsCString());
        error = mConnection.Connect(peerSockAddr);
    }

exit:
    return error;
}

template <> otError PlatTcp::Process<Cmd("send")>(Arg aArgs[])
{
    otError           error = OT_ERROR_NONE;
    OwnedPtr<Message> message;
    uint16_t          length;

    SuccessOrExit(error = aArgs[0].ParseAsUint16(length));
    VerifyOrExit(aArgs[1].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    message.Reset(AsCoreType(GetInstancePtr()).Get<MessagePool>().Allocate(Message::kTypeOther));
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    for (uint16_t i = 0; i < length; i++)
    {
        SuccessOrExit(error = message->Append<uint8_t>(static_cast<uint8_t>(i & 0xff) ^ (i >> 8)));
    }

    error = mConnection.Send(message.PassOwnership());

exit:
    return error;
}

template <> otError PlatTcp::Process<Cmd("close")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    mConnection.Close();

exit:
    return error;
}

template <> otError PlatTcp::Process<Cmd("abort")>(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);
    mConnection.Abort();

exit:
    return error;
}

otError PlatTcp::Process(Arg aArgs[])
{
#define CmdEntry(aCommandString) {aCommandString, &PlatTcp::Process<Cmd(aCommandString)>}

    static constexpr Command kCommands[] = {CmdEntry("abort"),   CmdEntry("bind"),     CmdEntry("close"),
                                            CmdEntry("connect"), CmdEntry("listener"), CmdEntry("send")};

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

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE && OPENTHREAD_CONFIG_CLI_PLAT_TCP_ENABLE
