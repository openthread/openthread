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
 *   This file implements UDP/IPv6 sockets.
 */

#include <stdio.h>

#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <net/ip6.hpp>
#include <net/udp6.hpp>
#include <openthreadinstance.h>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Ip6 {

ThreadError UdpSocket::Open(otInstance *aInstance, otUdpReceive aHandler, void *aCallbackContext)
{
    ThreadError error = kThreadError_None;

    for (UdpSocket *cur = aInstance->mUdpSockets; cur; cur = cur->GetNext())
    {
        if (cur == this)
        {
            ExitNow();
        }
    }

    memset(&mSockName, 0, sizeof(mSockName));
    memset(&mPeerName, 0, sizeof(mPeerName));
    mHandler = aHandler;
    mContext = aCallbackContext;

    SetNext(aInstance->mUdpSockets);
    aInstance->mUdpSockets = this;

exit:
    return error;
}

ThreadError UdpSocket::Bind(const SockAddr &aSockAddr)
{
    mSockName = aSockAddr;
    return kThreadError_None;
}

ThreadError UdpSocket::Close(otInstance *aInstance)
{
    if (aInstance->mUdpSockets == this)
    {
        aInstance->mUdpSockets = aInstance->mUdpSockets->GetNext();
    }
    else
    {
        for (UdpSocket *socket = aInstance->mUdpSockets; socket; socket = socket->GetNext())
        {
            if (socket->GetNext() == this)
            {
                socket->SetNext(GetNext());
                break;
            }
        }
    }

    memset(&mSockName, 0, sizeof(mSockName));
    memset(&mPeerName, 0, sizeof(mPeerName));
    SetNext(NULL);

    return kThreadError_None;
}

ThreadError UdpSocket::SendTo(Message &aMessage, const MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    MessageInfo messageInfoLocal;
    UdpHeader udpHeader;

    messageInfoLocal = aMessageInfo;

    if (messageInfoLocal.GetSockAddr().IsUnspecified())
    {
        messageInfoLocal.GetSockAddr() = GetSockName().GetAddress();
    }

    if (GetSockName().mPort == 0)
    {
        GetSockName().mPort = aMessage.GetInstance()->mEphemeralPort;

        if (aMessage.GetInstance()->mEphemeralPort < Udp::kDynamicPortMax)
        {
            aMessage.GetInstance()->mEphemeralPort++;
        }
        else
        {
            aMessage.GetInstance()->mEphemeralPort = Udp::kDynamicPortMin;
        }
    }

    udpHeader.SetSourcePort(GetSockName().mPort);
    udpHeader.SetDestinationPort(messageInfoLocal.mPeerPort);
    udpHeader.SetLength(sizeof(udpHeader) + aMessage.GetLength());
    udpHeader.SetChecksum(0);

    SuccessOrExit(error = aMessage.Prepend(&udpHeader, sizeof(udpHeader)));
    aMessage.SetOffset(0);
    SuccessOrExit(error = Ip6::SendDatagram(aMessage, messageInfoLocal, kProtoUdp));

exit:
    return error;
}

Message *Udp::NewMessage(otInstance *aInstance, uint16_t aReserved)
{
    return Ip6::NewMessage(aInstance, sizeof(UdpHeader) + aReserved);
}

ThreadError Udp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    UdpHeader udpHeader;
    uint16_t payloadLength;
    uint16_t checksum;

    payloadLength = aMessage.GetLength() - aMessage.GetOffset();

    // check length
    VerifyOrExit(payloadLength >= sizeof(UdpHeader), error = kThreadError_Parse);

    // verify checksum
    checksum = Ip6::ComputePseudoheaderChecksum(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(),
                                                payloadLength, kProtoUdp);
    checksum = aMessage.UpdateChecksum(checksum, aMessage.GetOffset(), payloadLength);
    VerifyOrExit(checksum == 0xffff, ;);

    aMessage.Read(aMessage.GetOffset(), sizeof(udpHeader), &udpHeader);
    aMessage.MoveOffset(sizeof(udpHeader));
    aMessageInfo.mPeerPort = udpHeader.GetSourcePort();
    aMessageInfo.mSockPort = udpHeader.GetDestinationPort();

    // find socket
    for (UdpSocket *socket = aMessage.GetInstance()->mUdpSockets; socket; socket = socket->GetNext())
    {
        if (socket->GetSockName().mPort != udpHeader.GetDestinationPort())
        {
            continue;
        }

        if (socket->GetSockName().mScopeId != 0 &&
            socket->GetSockName().mScopeId != aMessageInfo.mInterfaceId)
        {
            continue;
        }

        if (!aMessageInfo.GetSockAddr().IsMulticast() &&
            !socket->GetSockName().GetAddress().IsUnspecified() &&
            socket->GetSockName().GetAddress() != aMessageInfo.GetSockAddr())
        {
            continue;
        }

        // verify source if connected socket
        if (socket->GetPeerName().mPort != 0)
        {
            if (socket->GetPeerName().mPort != udpHeader.GetSourcePort())
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

exit:
    return error;
}

ThreadError Udp::UpdateChecksum(Message &aMessage, uint16_t aChecksum)
{
    aChecksum = aMessage.UpdateChecksum(aChecksum, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

    if (aChecksum != 0xffff)
    {
        aChecksum = ~aChecksum;
    }

    aChecksum = HostSwap16(aChecksum);
    aMessage.Write(aMessage.GetOffset() + UdpHeader::GetChecksumOffset(), sizeof(aChecksum), &aChecksum);
    return kThreadError_None;
}

}  // namespace Ip6
}  // namespace Thread
