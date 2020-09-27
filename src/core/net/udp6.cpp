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

#include <stdio.h>

#include <openthread/platform/udp.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Ip6 {

bool Udp::SocketHandle::Matches(const MessageInfo &aMessageInfo) const
{
    bool matches = false;

    VerifyOrExit(GetSockName().mPort == aMessageInfo.GetSockPort(), OT_NOOP);

    VerifyOrExit(aMessageInfo.GetSockAddr().IsMulticast() || GetSockName().GetAddress().IsUnspecified() ||
                     GetSockName().GetAddress() == aMessageInfo.GetSockAddr(),
                 OT_NOOP);

    // Verify source if connected socket
    if (GetPeerName().mPort != 0)
    {
        VerifyOrExit(GetPeerName().mPort == aMessageInfo.GetPeerPort(), OT_NOOP);

        VerifyOrExit(GetPeerName().GetAddress().IsUnspecified() ||
                         GetPeerName().GetAddress() == aMessageInfo.GetPeerAddr(),
                     OT_NOOP);
    }

    matches = true;

exit:
    return matches;
}

Udp::Socket::Socket(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    mHandle = nullptr;
}

Message *Udp::Socket::NewMessage(uint16_t aReserved, const Message::Settings &aSettings)
{
    return Get<Udp>().NewMessage(aReserved, aSettings);
}

otError Udp::Socket::Open(otUdpReceive aHandler, void *aContext)
{
    return Get<Udp>().Open(*this, aHandler, aContext);
}

otError Udp::Socket::Bind(const SockAddr &aSockAddr)
{
    return Get<Udp>().Bind(*this, aSockAddr);
}

otError Udp::Socket::Bind(uint16_t aPort)
{
    return Bind(SockAddr(aPort));
}

otError Udp::Socket::BindToNetif(otNetifIdentifier aNetifIdentifier)
{
    OT_UNUSED_VARIABLE(aNetifIdentifier);

    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    SuccessOrExit(error = otPlatUdpBindToNetif(this, aNetifIdentifier));
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<Udp>().BindToNetif(*this, aNetifIdentifier);
#endif

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
exit:
#endif
    return error;
}

otError Udp::Socket::Connect(const SockAddr &aSockAddr)
{
    return Get<Udp>().Connect(*this, aSockAddr);
}

otError Udp::Socket::Connect(uint16_t aPort)
{
    return Connect(SockAddr(aPort));
}

otError Udp::Socket::Close(void)
{
    return Get<Udp>().Close(*this);
}

otError Udp::Socket::SendTo(Message &aMessage, const MessageInfo &aMessageInfo)
{
    return Get<Udp>().SendTo(*this, aMessage, aMessageInfo);
}

Udp::Udp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEphemeralPort(kDynamicPortMin)
    , mReceivers()
    , mSockets()
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    , mPrevBackboneSockets(nullptr)
#endif
#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    , mUdpForwarderContext(nullptr)
    , mUdpForwarder(nullptr)
#endif
{
}

otError Udp::AddReceiver(Receiver &aReceiver)
{
    return mReceivers.Add(aReceiver);
}

otError Udp::RemoveReceiver(Receiver &aReceiver)
{
    otError error;

    SuccessOrExit(error = mReceivers.Remove(aReceiver));
    aReceiver.SetNext(nullptr);

exit:
    return error;
}

otError Udp::Open(SocketHandle &aSocket, otUdpReceive aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    aSocket.GetSockName().Clear();
    aSocket.GetPeerName().Clear();
    aSocket.mHandler = aHandler;
    aSocket.mContext = aContext;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    error = otPlatUdpSocket(&aSocket);
#endif
    SuccessOrExit(error);

    AddSocket(aSocket);

exit:
    return error;
}

otError Udp::Bind(SocketHandle &aSocket, const SockAddr &aSockAddr)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aSockAddr.GetAddress().IsUnspecified() || Get<ThreadNetif>().HasUnicastAddress(aSockAddr.GetAddress()),
                 error = OT_ERROR_INVALID_ARGS);

    aSocket.mSockName = aSockAddr;

    if (!aSocket.IsBound())
    {
        do
        {
            aSocket.mSockName.mPort = GetEphemeralPort();
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
            error = otPlatUdpBind(&aSocket);
#endif
        } while (error != OT_ERROR_NONE);
    }
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    else if (!IsMlePort(aSocket.mSockName.mPort))
    {
        error = otPlatUdpBind(&aSocket);
    }
#endif

exit:
    return error;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
void Udp::BindToNetif(SocketHandle &aSocket, otNetifIdentifier aNetifIdentifier)
{
    if (aNetifIdentifier == OT_NETIF_BACKBONE)
    {
        SetBackboneSocket(aSocket);
    }
}

void Udp::SetBackboneSocket(SocketHandle &aSocket)
{
    RemoveSocket(aSocket);

    if (mPrevBackboneSockets != nullptr)
    {
        mSockets.PushAfter(aSocket, *mPrevBackboneSockets);
    }
    else
    {
        mSockets.Push(aSocket);
    }
}

const Udp::SocketHandle *Udp::GetBackboneSockets(void)
{
    return mPrevBackboneSockets != nullptr ? mPrevBackboneSockets->GetNext() : mSockets.GetHead();
}
#endif

otError Udp::Connect(SocketHandle &aSocket, const SockAddr &aSockAddr)
{
    otError error = OT_ERROR_NONE;

    aSocket.mPeerName = aSockAddr;

    if (!aSocket.IsBound())
    {
        SuccessOrExit(error = Bind(aSocket, aSocket.GetSockName()));
    }

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    if (!IsMlePort(aSocket.mSockName.mPort))
    {
        error = otPlatUdpConnect(&aSocket);
    }
#endif

exit:
    return error;
}

otError Udp::Close(SocketHandle &aSocket)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    error = otPlatUdpClose(&aSocket);
#endif
    SuccessOrExit(error);

    RemoveSocket(aSocket);
    aSocket.GetSockName().Clear();
    aSocket.GetPeerName().Clear();

exit:
    return error;
}

otError Udp::SendTo(SocketHandle &aSocket, Message &aMessage, const MessageInfo &aMessageInfo)
{
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfoLocal;

    VerifyOrExit((aMessageInfo.GetSockPort() == 0) || (aSocket.GetSockName().mPort == aMessageInfo.GetSockPort()),
                 error = OT_ERROR_INVALID_ARGS);

    messageInfoLocal = aMessageInfo;

    if (messageInfoLocal.GetPeerAddr().IsUnspecified())
    {
        VerifyOrExit(!aSocket.GetPeerName().GetAddress().IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

        messageInfoLocal.SetPeerAddr(aSocket.GetPeerName().GetAddress());
    }

    if (messageInfoLocal.mPeerPort == 0)
    {
        VerifyOrExit(aSocket.GetPeerName().mPort != 0, error = OT_ERROR_INVALID_ARGS);
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
    if (!IsMlePort(aSocket.mSockName.mPort) &&
        !(aSocket.mSockName.mPort == ot::Tmf::kUdpPort && aMessage.GetSubType() == Message::kSubTypeJoinerEntrust))
    {
        SuccessOrExit(error = otPlatUdpSend(&aSocket, &aMessage, &messageInfoLocal));
    }
    else
#endif
    {
        SuccessOrExit(error = SendDatagram(aMessage, messageInfoLocal, kProtoUdp));
    }

exit:
    return error;
}

void Udp::AddSocket(SocketHandle &aSocket)
{
    SuccessOrExit(mSockets.Add(aSocket));

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (mPrevBackboneSockets == nullptr)
    {
        mPrevBackboneSockets = &aSocket;
    }
#endif
exit:
    return;
}

void Udp::RemoveSocket(SocketHandle &aSocket)
{
    SocketHandle *prev;

    SuccessOrExit(mSockets.Find(aSocket, prev));

    mSockets.PopAfter(prev);
    aSocket.SetNext(nullptr);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (&aSocket == mPrevBackboneSockets)
    {
        mPrevBackboneSockets = prev;
    }
#endif

exit:
    return;
}

uint16_t Udp::GetEphemeralPort(void)
{
    uint16_t rval = mEphemeralPort;

    if (mEphemeralPort < kDynamicPortMax)
    {
        mEphemeralPort++;
    }
    else
    {
        mEphemeralPort = kDynamicPortMin;
    }

    return rval;
}

Message *Udp::NewMessage(uint16_t aReserved, const Message::Settings &aSettings)
{
    return Get<Ip6>().NewMessage(sizeof(Header) + aReserved, aSettings);
}

otError Udp::SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    if (aMessageInfo.IsHostInterface())
    {
        VerifyOrExit(mUdpForwarder != nullptr, error = OT_ERROR_NO_ROUTE);
        mUdpForwarder(&aMessage, aMessageInfo.mPeerPort, &aMessageInfo.GetPeerAddr(), aMessageInfo.mSockPort,
                      mUdpForwarderContext);
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

        SuccessOrExit(error = aMessage.Prepend(&udpHeader, sizeof(udpHeader)));
        aMessage.SetOffset(0);

        error = Get<Ip6>().SendDatagram(aMessage, aMessageInfo, aIpProto);
    }

exit:
    return error;
}

otError Udp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Header  udpHeader;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(udpHeader), &udpHeader) == sizeof(udpHeader),
                 error = OT_ERROR_PARSE);

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    SuccessOrExit(error = Checksum::VerifyMessageChecksum(aMessage, aMessageInfo, kProtoUdp));
#endif

    aMessage.MoveOffset(sizeof(udpHeader));
    aMessageInfo.mPeerPort = udpHeader.GetSourcePort();
    aMessageInfo.mSockPort = udpHeader.GetDestinationPort();

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    VerifyOrExit(IsMlePort(aMessageInfo.mSockPort), OT_NOOP);
#endif

    for (Receiver *receiver = mReceivers.GetHead(); receiver; receiver = receiver->GetNext())
    {
        VerifyOrExit(!receiver->HandleMessage(aMessage, aMessageInfo), OT_NOOP);
    }

    HandlePayload(aMessage, aMessageInfo);

exit:
    return error;
}

void Udp::HandlePayload(Message &aMessage, MessageInfo &aMessageInfo)
{
    SocketHandle *socket;
    SocketHandle *prev;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    {
        const SocketHandle *socketsBegin, *socketsEnd;

        if (!aMessageInfo.IsHostInterface())
        {
            socketsBegin = mSockets.GetHead();
            socketsEnd   = GetBackboneSockets();
        }
        else
        {
            socketsBegin = GetBackboneSockets();
            socketsEnd   = nullptr;
        }

        socket = mSockets.FindMatching(socketsBegin, socketsEnd, aMessageInfo, prev);
    }
#else
    socket = mSockets.FindMatching(aMessageInfo, prev);
#endif

    VerifyOrExit(socket != nullptr, OT_NOOP);

    aMessage.RemoveHeader(aMessage.GetOffset());
    OT_ASSERT(aMessage.GetOffset() == 0);
    socket->HandleUdpReceive(aMessage, aMessageInfo);

exit:
    return;
}

bool Udp::IsMlePort(uint16_t aPort) const
{
    bool isMlePort = (aPort == Mle::kUdpPort);

#if OPENTHREAD_FTD
    isMlePort = isMlePort || (aPort == Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort());
#endif

    return isMlePort;
}

} // namespace Ip6
} // namespace ot
