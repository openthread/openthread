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
#include "net/ip6.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Ip6 {

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
static bool IsMle(Instance &aInstance, uint16_t aPort)
{
#if OPENTHREAD_FTD
    return aPort == ot::Mle::kUdpPort || aPort == aInstance.Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort();
#else
    OT_UNUSED_VARIABLE(aInstance);
    return aPort == ot::Mle::kUdpPort;
#endif
}
#endif

UdpSocket::UdpSocket(Udp &aUdp)
    : InstanceLocator(aUdp.GetInstance())
{
    mHandle = NULL;
}

Message *UdpSocket::NewMessage(uint16_t aReserved, const otMessageSettings *aSettings)
{
    return Get<Udp>().NewMessage(aReserved, aSettings);
}

otError UdpSocket::Open(otUdpReceive aHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    GetSockName().Clear();
    GetPeerName().Clear();
    mHandler = aHandler;
    mContext = aContext;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    SuccessOrExit(error = otPlatUdpSocket(this));
#endif

    Get<Udp>().AddSocket(*this);

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
exit:
#endif
    return error;
}

otError UdpSocket::Bind(const SockAddr &aSockAddr)
{
    otError error = OT_ERROR_NONE;

    mSockName = aSockAddr;

    if (!IsBound())
    {
        do
        {
            mSockName.mPort = Get<Udp>().GetEphemeralPort();
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
            error = otPlatUdpBind(this);
#endif
        } while (error != OT_ERROR_NONE);
    }
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    else if (!IsMle(GetInstance(), mSockName.mPort))
    {
        error = otPlatUdpBind(this);
    }
#endif

    return error;
}

otError UdpSocket::Connect(const SockAddr &aSockAddr)
{
    otError error = OT_ERROR_NONE;

    mPeerName = aSockAddr;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    if (!IsMle(GetInstance(), mSockName.mPort))
    {
        error = otPlatUdpConnect(this);
    }
#endif
    return error;
}

otError UdpSocket::Close(void)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    SuccessOrExit(error = otPlatUdpClose(this));
#endif

    Get<Udp>().RemoveSocket(*this);
    GetSockName().Clear();
    GetPeerName().Clear();

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
exit:
#endif
    return error;
}

otError UdpSocket::SendTo(Message &aMessage, const MessageInfo &aMessageInfo)
{
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfoLocal;

    VerifyOrExit((aMessageInfo.GetSockPort() == 0) || (GetSockName().mPort == aMessageInfo.GetSockPort()),
                 error = OT_ERROR_INVALID_ARGS);

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

    if (!IsBound())
    {
        SuccessOrExit(error = Bind(GetSockName()));
    }

    messageInfoLocal.SetSockPort(GetSockName().mPort);

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    if (!IsMle(GetInstance(), mSockName.mPort) &&
        !(mSockName.mPort == ot::kCoapUdpPort && aMessage.GetSubType() == Message::kSubTypeJoinerEntrust))
    {
        SuccessOrExit(error = otPlatUdpSend(this, &aMessage, &messageInfoLocal));
    }
    else
#endif
    {
        SuccessOrExit(error = Get<Udp>().SendDatagram(aMessage, messageInfoLocal, kProtoUdp));
    }

exit:
    return error;
}

Udp::Udp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEphemeralPort(kDynamicPortMin)
    , mReceivers()
    , mSockets()
#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    , mUdpForwarderContext(NULL)
    , mUdpForwarder(NULL)
#endif
{
}

otError Udp::AddReceiver(UdpReceiver &aReceiver)
{
    return mReceivers.Add(aReceiver);
}

otError Udp::RemoveReceiver(UdpReceiver &aReceiver)
{
    otError error;

    SuccessOrExit(error = mReceivers.Remove(aReceiver));
    aReceiver.SetNext(NULL);

exit:
    return error;
}

void Udp::AddSocket(UdpSocket &aSocket)
{
    IgnoreError(mSockets.Add(aSocket));
}

void Udp::RemoveSocket(UdpSocket &aSocket)
{
    SuccessOrExit(mSockets.Remove(aSocket));
    aSocket.SetNext(NULL);

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

Message *Udp::NewMessage(uint16_t aReserved, const otMessageSettings *aSettings)
{
    return Get<Ip6>().NewMessage(sizeof(UdpHeader) + aReserved, aSettings);
}

otError Udp::SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    if (aMessageInfo.IsHostInterface())
    {
        VerifyOrExit(mUdpForwarder != NULL, error = OT_ERROR_NO_ROUTE);
        mUdpForwarder(&aMessage, aMessageInfo.mPeerPort, &aMessageInfo.GetPeerAddr(), aMessageInfo.mSockPort,
                      mUdpForwarderContext);
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

        error = Get<Ip6>().SendDatagram(aMessage, aMessageInfo, aIpProto);
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
    IgnoreError(aMessage.MoveOffset(sizeof(udpHeader)));
    aMessageInfo.mPeerPort = udpHeader.GetSourcePort();
    aMessageInfo.mSockPort = udpHeader.GetDestinationPort();

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    VerifyOrExit(IsMle(GetInstance(), aMessageInfo.mSockPort), OT_NOOP);
#endif

    for (UdpReceiver *receiver = mReceivers.GetHead(); receiver; receiver = receiver->GetNext())
    {
        VerifyOrExit(!receiver->HandleMessage(aMessage, aMessageInfo), OT_NOOP);
    }

    HandlePayload(aMessage, aMessageInfo);

exit:
    return error;
}

void Udp::HandlePayload(Message &aMessage, MessageInfo &aMessageInfo)
{
    // find socket
    for (UdpSocket *socket = mSockets.GetHead(); socket; socket = socket->GetNext())
    {
        if (socket->GetSockName().mPort != aMessageInfo.GetSockPort())
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

        aMessage.RemoveHeader(aMessage.GetOffset());
        OT_ASSERT(aMessage.GetOffset() == 0);
        socket->HandleUdpReceive(aMessage, aMessageInfo);
        break;
    }
}

void Udp::UpdateChecksum(Message &aMessage, uint16_t aChecksum)
{
    aChecksum = aMessage.UpdateChecksum(aChecksum, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

    if (aChecksum != 0xffff)
    {
        aChecksum = ~aChecksum;
    }

    aChecksum = HostSwap16(aChecksum);
    aMessage.Write(aMessage.GetOffset() + UdpHeader::GetChecksumOffset(), sizeof(aChecksum), &aChecksum);
}

} // namespace Ip6
} // namespace ot
