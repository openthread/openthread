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

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "net/ip6.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Ip6 {

UdpSocket::UdpSocket(Udp &aUdp)
    : InstanceLocator(aUdp.GetInstance())
{
}

Udp &UdpSocket::GetUdp(void)
{
    return GetInstance().GetIp6().GetUdp();
}

Message *UdpSocket::NewMessage(uint16_t aReserved, uint8_t aPriority)
{
    return GetUdp().NewMessage(aReserved, aPriority);
}

otError UdpSocket::Open(otUdpReceive aHandler, void *aContext)
{
    memset(&mSockName, 0, sizeof(mSockName));
    memset(&mPeerName, 0, sizeof(mPeerName));
    mHandler = aHandler;
    mContext = aContext;

    return GetUdp().AddSocket(*this);
}

otError UdpSocket::Bind(const SockAddr &aSockAddr)
{
    mSockName = aSockAddr;

    if (GetSockName().mPort == 0)
    {
        mSockName.mPort = GetUdp().GetEphemeralPort();
    }

    return OT_ERROR_NONE;
}

otError UdpSocket::Connect(const SockAddr &aSockAddr)
{
    mPeerName = aSockAddr;
    return OT_ERROR_NONE;
}

otError UdpSocket::Close(void)
{
    otError error = OT_ERROR_NONE;

    SuccessOrExit(error = GetUdp().RemoveSocket(*this));
    memset(&mSockName, 0, sizeof(mSockName));
    memset(&mPeerName, 0, sizeof(mPeerName));

exit:
    return error;
}

otError UdpSocket::SendTo(Message &aMessage, const MessageInfo &aMessageInfo)
{
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfoLocal;

    messageInfoLocal = aMessageInfo;

    if (messageInfoLocal.GetPeerAddr().IsUnspecified())
    {
        VerifyOrExit(!GetPeerName().GetAddress().IsUnspecified(), error = OT_ERROR_INVALID_ARGS);

        messageInfoLocal.SetPeerAddr(GetPeerName().GetAddress());
    }

    if (messageInfoLocal.mPeerPort == 0)
    {
        VerifyOrExit(GetPeerName().mPort != 0, error = OT_ERROR_INVALID_ARGS);
        messageInfoLocal.mPeerPort = GetPeerName().mPort;
    }

    if (messageInfoLocal.GetSockAddr().IsUnspecified())
    {
        messageInfoLocal.SetSockAddr(GetSockName().GetAddress());
    }

    if (GetSockName().mPort == 0)
    {
        GetSockName().mPort = GetUdp().GetEphemeralPort();
    }
    messageInfoLocal.SetSockPort(GetSockName().mPort);

    SuccessOrExit(error = GetUdp().SendDatagram(aMessage, messageInfoLocal, kProtoUdp));

exit:
    return error;
}

Udp::Udp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEphemeralPort(kDynamicPortMin)
    , mReceivers(NULL)
    , mSockets(NULL)
{
}

otError Udp::AddReceiver(UdpReceiver &aReceiver)
{
    for (UdpReceiver *cur = mReceivers; cur; cur = cur->GetNext())
    {
        if (cur == &aReceiver)
        {
            ExitNow();
        }
    }

    aReceiver.SetNext(mReceivers);
    mReceivers = &aReceiver;

exit:
    return OT_ERROR_NONE;
}

otError Udp::RemoveReceiver(UdpReceiver &aReceiver)
{
    if (mReceivers == &aReceiver)
    {
        mReceivers = mReceivers->GetNext();
    }
    else
    {
        for (UdpReceiver *handler = mReceivers; handler; handler = handler->GetNext())
        {
            if (handler->GetNext() == &aReceiver)
            {
                handler->SetNext(aReceiver.GetNext());
                break;
            }
        }
    }

    aReceiver.SetNext(NULL);

    return OT_ERROR_NONE;
}

otError Udp::AddSocket(UdpSocket &aSocket)
{
    for (UdpSocket *cur = mSockets; cur; cur = cur->GetNext())
    {
        if (cur == &aSocket)
        {
            ExitNow();
        }
    }

    aSocket.SetNext(mSockets);
    mSockets = &aSocket;

exit:
    return OT_ERROR_NONE;
}

otError Udp::RemoveSocket(UdpSocket &aSocket)
{
    if (mSockets == &aSocket)
    {
        mSockets = mSockets->GetNext();
    }
    else
    {
        for (UdpSocket *socket = mSockets; socket; socket = socket->GetNext())
        {
            if (socket->GetNext() == &aSocket)
            {
                socket->SetNext(aSocket.GetNext());
                break;
            }
        }
    }

    aSocket.SetNext(NULL);

    return OT_ERROR_NONE;
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

Message *Udp::NewMessage(uint16_t aReserved, uint8_t aPriority)
{
    return GetIp6().NewMessage(sizeof(UdpHeader) + aReserved, aPriority);
}

otError Udp::SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, IpProto aIpProto)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_ENABLE_UDP_PROXY
    if (aMessageInfo.GetInterfaceId() == OT_NETIF_INTERFACE_ID_HOST)
    {
        VerifyOrExit(mProxySender != NULL, error = OT_ERROR_NO_ROUTE);
        mProxySender(&aMessage, aMessageInfo.mPeerPort, &aMessageInfo.GetPeerAddr(), aMessageInfo.mSockPort,
                     mProxySenderContext);
        // message is consumed by the callback
    }
    else
#endif
    {
        UdpHeader udpHeader;

        udpHeader.SetSourcePort(aMessageInfo.mSockPort);
        udpHeader.SetDestinationPort(aMessageInfo.mPeerPort);
        udpHeader.SetLength(sizeof(udpHeader) + aMessage.GetLength());
        udpHeader.SetChecksum(0);

        SuccessOrExit(error = aMessage.Prepend(&udpHeader, sizeof(udpHeader)));
        aMessage.SetOffset(0);

        error = GetIp6().SendDatagram(aMessage, aMessageInfo, aIpProto);
    }

exit:
    return error;
}

otError Udp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    otError   error = OT_ERROR_NONE;
    UdpHeader udpHeader;
    uint16_t  payloadLength;
    uint16_t  checksum;

    payloadLength = aMessage.GetLength() - aMessage.GetOffset();

    // check length
    VerifyOrExit(payloadLength >= sizeof(UdpHeader), error = OT_ERROR_PARSE);

    // verify checksum
    checksum = Ip6::ComputePseudoheaderChecksum(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(), payloadLength,
                                                kProtoUdp);
    checksum = aMessage.UpdateChecksum(checksum, aMessage.GetOffset(), payloadLength);

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    VerifyOrExit(checksum == 0xffff, error = OT_ERROR_DROP);
#endif

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(udpHeader), &udpHeader) == sizeof(udpHeader),
                 error = OT_ERROR_PARSE);
    aMessage.MoveOffset(sizeof(udpHeader));
    aMessageInfo.mPeerPort = udpHeader.GetSourcePort();
    aMessageInfo.mSockPort = udpHeader.GetDestinationPort();

    for (UdpReceiver *receiver = mReceivers; receiver; receiver = receiver->GetNext())
    {
        VerifyOrExit(!receiver->HandleMessage(aMessage, aMessageInfo));
    }

    HandlePayload(aMessage, aMessageInfo);

exit:
    return error;
}

void Udp::HandlePayload(Message &aMessage, MessageInfo &aMessageInfo)
{
    // find socket
    for (UdpSocket *socket = mSockets; socket; socket = socket->GetNext())
    {
        if (socket->GetSockName().mPort != aMessageInfo.GetSockPort())
        {
            continue;
        }

        if (socket->GetSockName().mScopeId != 0 && socket->GetSockName().mScopeId != aMessageInfo.mInterfaceId)
        {
            continue;
        }

        if (!aMessageInfo.GetSockAddr().IsMulticast() && !socket->GetSockName().GetAddress().IsUnspecified() &&
            socket->GetSockName().GetAddress() != aMessageInfo.GetSockAddr())
        {
            continue;
        }

        // verify source if connected socket
        if (socket->GetPeerName().mPort != 0)
        {
            if (socket->GetPeerName().mPort != aMessageInfo.GetPeerPort())
            {
                continue;
            }

            if (!socket->GetPeerName().GetAddress().IsUnspecified() &&
                socket->GetPeerName().GetAddress() != aMessageInfo.GetPeerAddr())
            {
                continue;
            }
        }

        socket->HandleUdpReceive(aMessage, aMessageInfo);
    }
}

otError Udp::UpdateChecksum(Message &aMessage, uint16_t aChecksum)
{
    aChecksum = aMessage.UpdateChecksum(aChecksum, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

    if (aChecksum != 0xffff)
    {
        aChecksum = ~aChecksum;
    }

    aChecksum = HostSwap16(aChecksum);
    aMessage.Write(aMessage.GetOffset() + UdpHeader::GetChecksumOffset(), sizeof(aChecksum), &aChecksum);
    return OT_ERROR_NONE;
}

} // namespace Ip6
} // namespace ot
