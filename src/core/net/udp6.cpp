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
 *   This file implements UDP/IPv6 sockets.
 */

#include "udp6.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace Ip6 {

//---------------------------------------------------------------------------------------------------------------------
// Udp::SocketHandle

bool Udp::SocketHandle::Matches(const MessageInfo &aMessageInfo) const
{
    bool matches = false;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (aMessageInfo.IsHostInterface())
    {
        VerifyOrExit(IsBackbone() || (GetNetifId() == kNetifUnspecified));
    }
    else
    {
        VerifyOrExit(!IsBackbone());
    }
#endif

    VerifyOrExit(GetSockName().mPort == aMessageInfo.GetSockPort());

    VerifyOrExit(aMessageInfo.GetSockAddr().IsMulticast() || GetSockName().GetAddress().IsUnspecified() ||
                 GetSockName().GetAddress() == aMessageInfo.GetSockAddr());

    // Verify source if connected socket
    if (GetPeerName().mPort != 0)
    {
        VerifyOrExit(GetPeerName().mPort == aMessageInfo.GetPeerPort());

        VerifyOrExit(GetPeerName().GetAddress().IsUnspecified() ||
                     GetPeerName().GetAddress() == aMessageInfo.GetPeerAddr());
    }

    matches = true;

exit:
    return matches;
}

//---------------------------------------------------------------------------------------------------------------------
// Udp::Socket

Udp::Socket::Socket(Instance &aInstance, ReceiveHandler aHandler, void *aContext)
    : InstanceLocator(aInstance)
{
    Clear();
    mHandler = aHandler;
    mContext = aContext;
}

Message *Udp::Socket::NewMessage(void) { return NewMessage(0); }

Message *Udp::Socket::NewMessage(uint16_t aReserved) { return NewMessage(aReserved, Message::Settings::GetDefault()); }

Message *Udp::Socket::NewMessage(uint16_t aReserved, const Message::Settings &aSettings)
{
    return Get<Udp>().NewMessage(aReserved, aSettings);
}

Error Udp::Socket::Open(NetifIdentifier aNetifId) { return Get<Udp>().Open(*this, aNetifId, mHandler, mContext); }

bool Udp::Socket::IsOpen(void) const { return Get<Udp>().IsOpen(*this); }

Error Udp::Socket::Bind(const SockAddr &aSockAddr) { return Get<Udp>().Bind(*this, aSockAddr); }

Error Udp::Socket::Bind(uint16_t aPort) { return Bind(SockAddr(aPort)); }

Error Udp::Socket::Connect(const SockAddr &aSockAddr) { return Get<Udp>().Connect(*this, aSockAddr); }

Error Udp::Socket::Connect(uint16_t aPort) { return Connect(SockAddr(aPort)); }

Error Udp::Socket::Close(void) { return Get<Udp>().Close(*this); }

Error Udp::Socket::SendTo(Message &aMessage, const MessageInfo &aMessageInfo)
{
    return Get<Udp>().SendTo(*this, aMessage, aMessageInfo);
}

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
Error Udp::Socket::JoinNetifMulticastGroup(NetifIdentifier aNetifIdentifier, const Address &aAddress)
{
    OT_UNUSED_VARIABLE(aNetifIdentifier);
    OT_UNUSED_VARIABLE(aAddress);

    Error error = kErrorNotImplemented;

    VerifyOrExit(aAddress.IsMulticast(), error = kErrorInvalidArgs);

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    error = Plat::JoinMulticastGroup(*this, aNetifIdentifier, aAddress);
#endif

exit:
    return error;
}

Error Udp::Socket::LeaveNetifMulticastGroup(NetifIdentifier aNetifIdentifier, const Address &aAddress)
{
    OT_UNUSED_VARIABLE(aNetifIdentifier);
    OT_UNUSED_VARIABLE(aAddress);

    Error error = kErrorNotImplemented;

    VerifyOrExit(aAddress.IsMulticast(), error = kErrorInvalidArgs);

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    error = Plat::LeaveMulticastGroup(*this, aNetifIdentifier, aAddress);
#endif

exit:
    return error;
}
#endif

//---------------------------------------------------------------------------------------------------------------------
// Udp::Plat

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE

Error Udp::Plat::Open(SocketHandle &aSocket)
{
    return aSocket.ShouldUsePlatformUdp() ? otPlatUdpSocket(&aSocket) : kErrorNone;
}

Error Udp::Plat::Close(SocketHandle &aSocket)
{
    return aSocket.ShouldUsePlatformUdp() ? otPlatUdpClose(&aSocket) : kErrorNone;
}

Error Udp::Plat::Bind(SocketHandle &aSocket)
{
    return aSocket.ShouldUsePlatformUdp() ? otPlatUdpBind(&aSocket) : kErrorNone;
}

Error Udp::Plat::BindToNetif(SocketHandle &aSocket)
{
    return aSocket.ShouldUsePlatformUdp() ? otPlatUdpBindToNetif(&aSocket, MapEnum(aSocket.GetNetifId())) : kErrorNone;
}

Error Udp::Plat::Connect(SocketHandle &aSocket)
{
    return aSocket.ShouldUsePlatformUdp() ? otPlatUdpConnect(&aSocket) : kErrorNone;
}

Error Udp::Plat::Send(SocketHandle &aSocket, Message &aMessage, const MessageInfo &aMessageInfo)
{
    OT_ASSERT(aSocket.ShouldUsePlatformUdp());

    return otPlatUdpSend(&aSocket, &aMessage, &aMessageInfo);
}

Error Udp::Plat::JoinMulticastGroup(SocketHandle &aSocket, NetifIdentifier aNetifId, const Address &aAddress)
{
    return aSocket.ShouldUsePlatformUdp() ? otPlatUdpJoinMulticastGroup(&aSocket, MapEnum(aNetifId), &aAddress)
                                          : kErrorNone;
}

Error Udp::Plat::LeaveMulticastGroup(SocketHandle &aSocket, NetifIdentifier aNetifId, const Address &aAddress)
{
    return aSocket.ShouldUsePlatformUdp() ? otPlatUdpLeaveMulticastGroup(&aSocket, MapEnum(aNetifId), &aAddress)
                                          : kErrorNone;
}

#endif //  OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Udp

Udp::Udp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEphemeralPort(kDynamicPortMin)
{
}

Error Udp::AddReceiver(Receiver &aReceiver) { return mReceivers.Add(aReceiver); }

Error Udp::RemoveReceiver(Receiver &aReceiver)
{
    Error error;

    SuccessOrExit(error = mReceivers.Remove(aReceiver));
    aReceiver.SetNext(nullptr);

exit:
    return error;
}

Error Udp::Open(SocketHandle &aSocket, NetifIdentifier aNetifId, ReceiveHandler aHandler, void *aContext)
{
    Error error = kErrorNone;

    OT_ASSERT(!IsOpen(aSocket));

    aSocket.Clear();
    aSocket.SetNetifId(aNetifId);
    aSocket.mHandler = aHandler;
    aSocket.mContext = aContext;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    error = Plat::Open(aSocket);
#endif
    SuccessOrExit(error);

    AddSocket(aSocket);

exit:
    return error;
}

Error Udp::Bind(SocketHandle &aSocket, const SockAddr &aSockAddr)
{
    Error error = kErrorNone;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    SuccessOrExit(error = Plat::BindToNetif(aSocket));
#endif

    VerifyOrExit(aSockAddr.GetAddress().IsUnspecified() || Get<ThreadNetif>().HasUnicastAddress(aSockAddr.GetAddress()),
                 error = kErrorInvalidArgs);

    aSocket.mSockName = aSockAddr;

    if (!aSocket.IsBound())
    {
        do
        {
            aSocket.mSockName.mPort = GetEphemeralPort();
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
            error = Plat::Bind(aSocket);
#endif
        } while (error != kErrorNone);
    }
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    else
    {
        error = Plat::Bind(aSocket);
    }
#endif

exit:
    return error;
}

Error Udp::Connect(SocketHandle &aSocket, const SockAddr &aSockAddr)
{
    Error error = kErrorNone;

    aSocket.mPeerName = aSockAddr;

    if (!aSocket.IsBound())
    {
        SuccessOrExit(error = Bind(aSocket, aSocket.GetSockName()));
    }

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    error = Plat::Connect(aSocket);
#endif

exit:
    return error;
}

Error Udp::Close(SocketHandle &aSocket)
{
    Error error = kErrorNone;

    VerifyOrExit(IsOpen(aSocket));

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    SuccessOrExit(error = Plat::Close(aSocket));
#endif

    RemoveSocket(aSocket);
    aSocket.GetSockName().Clear();
    aSocket.GetPeerName().Clear();

exit:
    return error;
}

Error Udp::SendTo(SocketHandle &aSocket, Message &aMessage, const MessageInfo &aMessageInfo)
{
    Error       error = kErrorNone;
    MessageInfo messageInfoLocal;

    VerifyOrExit((aMessageInfo.GetSockPort() == 0) || (aSocket.GetSockName().mPort == aMessageInfo.GetSockPort()),
                 error = kErrorInvalidArgs);

    messageInfoLocal = aMessageInfo;

    if (messageInfoLocal.GetPeerAddr().IsUnspecified())
    {
        VerifyOrExit(!aSocket.GetPeerName().GetAddress().IsUnspecified(), error = kErrorInvalidArgs);

        messageInfoLocal.SetPeerAddr(aSocket.GetPeerName().GetAddress());
    }

    if (messageInfoLocal.mPeerPort == 0)
    {
        VerifyOrExit(aSocket.GetPeerName().mPort != 0, error = kErrorInvalidArgs);
        messageInfoLocal.mPeerPort = aSocket.GetPeerName().mPort;
    }

    if (messageInfoLocal.GetSockAddr().IsUnspecified())
    {
        messageInfoLocal.SetSockAddr(aSocket.GetSockName().GetAddress());
    }

    if (!aSocket.IsBound())
    {
        SuccessOrExit(error = Bind(aSocket, aSocket.GetSockName()));
    }

    messageInfoLocal.SetSockPort(aSocket.GetSockName().mPort);

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    if (aSocket.ShouldUsePlatformUdp())
    {
        SuccessOrExit(error = Plat::Send(aSocket, aMessage, messageInfoLocal));
    }
    else
#endif
    {
        SuccessOrExit(error = SendDatagram(aMessage, messageInfoLocal));
    }

exit:
    return error;
}

bool Udp::IsPortReserved(uint16_t aPort)
{
    return aPort == Tmf::kUdpPort || (kSrpServerPortMin <= aPort && aPort <= kSrpServerPortMax);
}

void Udp::AddSocket(SocketHandle &aSocket) { IgnoreError(mSockets.Add(aSocket)); }

void Udp::RemoveSocket(SocketHandle &aSocket)
{
    SocketHandle *prev;

    SuccessOrExit(mSockets.Find(aSocket, prev));

    mSockets.PopAfter(prev);
    aSocket.SetNext(nullptr);

exit:
    return;
}

uint16_t Udp::GetEphemeralPort(void)
{
    do
    {
        if (mEphemeralPort < kDynamicPortMax)
        {
            mEphemeralPort++;
        }
        else
        {
            mEphemeralPort = kDynamicPortMin;
        }
    } while (IsPortReserved(mEphemeralPort));

    return mEphemeralPort;
}

Message *Udp::NewMessage(void) { return NewMessage(0); }

Message *Udp::NewMessage(uint16_t aReserved) { return NewMessage(aReserved, Message::Settings::GetDefault()); }

Message *Udp::NewMessage(uint16_t aReserved, const Message::Settings &aSettings)
{
    return Get<Ip6>().NewMessage(sizeof(Header) + aReserved, aSettings);
}

Error Udp::SendDatagram(Message &aMessage, MessageInfo &aMessageInfo)
{
    Error error = kErrorNone;

#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    if (aMessageInfo.IsHostInterface())
    {
        VerifyOrExit(mUdpForwarder.IsSet(), error = kErrorNoRoute);
        mUdpForwarder.Invoke(&aMessage, aMessageInfo.mPeerPort, &aMessageInfo.GetPeerAddr(), aMessageInfo.mSockPort);
        // message is consumed by the callback
    }
    else
#endif
    {
        Header udpHeader;

        udpHeader.SetSourcePort(aMessageInfo.mSockPort);
        udpHeader.SetDestinationPort(aMessageInfo.mPeerPort);
        udpHeader.SetLength(sizeof(udpHeader) + aMessage.GetLength());
        udpHeader.SetChecksum(0);

        SuccessOrExit(error = aMessage.Prepend(udpHeader));
        aMessage.SetOffset(0);

        error = Get<Ip6>().SendDatagram(aMessage, aMessageInfo, kProtoUdp);
    }

exit:
    return error;
}

Error Udp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    Error  error = kErrorNone;
    Header udpHeader;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), udpHeader));

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    SuccessOrExit(error = Checksum::VerifyMessageChecksum(aMessage, aMessageInfo, kProtoUdp));
#endif

    aMessage.MoveOffset(sizeof(udpHeader));
    aMessageInfo.mPeerPort = udpHeader.GetSourcePort();
    aMessageInfo.mSockPort = udpHeader.GetDestinationPort();

    for (Receiver &receiver : mReceivers)
    {
        VerifyOrExit(!receiver.HandleMessage(aMessage, aMessageInfo));
    }

    HandlePayload(aMessage, aMessageInfo);

exit:
    return error;
}

void Udp::HandlePayload(Message &aMessage, MessageInfo &aMessageInfo)
{
    SocketHandle *socket;

    socket = mSockets.FindMatching(aMessageInfo);
    VerifyOrExit(socket != nullptr);

    aMessage.RemoveHeader(aMessage.GetOffset());
    OT_ASSERT(aMessage.GetOffset() == 0);
    socket->HandleUdpReceive(aMessage, aMessageInfo);

exit:
    return;
}

bool Udp::IsPortInUse(uint16_t aPort) const
{
    bool found = false;

    for (const SocketHandle &socket : mSockets)
    {
        if (socket.GetSockName().GetPort() == aPort)
        {
            found = true;
            break;
        }
    }

    return found;
}

} // namespace Ip6
} // namespace ot
