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

#include "nexus_udp.hpp"

#include <openthread/platform/udp.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"
#include "common/logging.hpp"
#include "net/checksum.hpp"
#include "thread/network_data_leader.hpp"

namespace ot {
namespace Nexus {

Udp::Udp(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Error Udp::Socket(Ip6::Udp::SocketHandle &aSocket)
{
    OT_UNUSED_VARIABLE(aSocket);
    return kErrorNone;
}

Error Udp::Close(Ip6::Udp::SocketHandle &aSocket)
{
    OT_UNUSED_VARIABLE(aSocket);
    return kErrorNone;
}

Error Udp::Bind(Ip6::Udp::SocketHandle &aSocket)
{
    OT_UNUSED_VARIABLE(aSocket);
    return kErrorNone;
}

Error Udp::BindToNetif(Ip6::Udp::SocketHandle &aSocket, Ip6::NetifIdentifier aNetifIdentifier)
{
    OT_UNUSED_VARIABLE(aSocket);
    OT_UNUSED_VARIABLE(aNetifIdentifier);
    return kErrorNone;
}

Error Udp::Connect(Ip6::Udp::SocketHandle &aSocket)
{
    OT_UNUSED_VARIABLE(aSocket);
    return kErrorNone;
}

Error Udp::Send(Ip6::Udp::SocketHandle &aSocket, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                error   = kErrorNone;
    Ip6::Address         srcAddr = aMessageInfo.GetSockAddr();
    Ip6::NetifIdentifier netifId = aSocket.GetNetifId();

    if (netifId == Ip6::kNetifUnspecified)
    {
        if (aMessageInfo.IsHostInterface())
        {
            netifId = Ip6::kNetifBackbone;
        }
        else if (aMessageInfo.GetPeerAddr().IsMulticast())
        {
            netifId = Ip6::kNetifThreadHost;
        }
        else if (GetNode().Get<Mle::Mle>().IsAttached() &&
                 GetNode().Get<NetworkData::Leader>().IsOnMesh(aMessageInfo.GetPeerAddr()))
        {
            // If we are attached and the peer is on-mesh, we use the Thread interface.
            netifId = Ip6::kNetifThreadHost;
        }
        else if (Core::Get().FindNodeByInfraIfAddress(aMessageInfo.GetPeerAddr()) != nullptr)
        {
            netifId = Ip6::kNetifBackbone;
        }
        else
        {
            // For all other communication (e.g. unjoined nodes or external commissioners),
            // we prefer using the Backbone interface for better reliability.
            netifId = Ip6::kNetifBackbone;
        }
    }

    if (srcAddr.IsUnspecified() && (netifId == Ip6::kNetifBackbone))
    {
        if (Core::Get().IsThreadAddress(aMessageInfo.GetPeerAddr()))
        {
            srcAddr = GetNode().Get<Mle::Mle>().GetLinkLocalAddress();
        }
        else
        {
            srcAddr = GetNode().mInfraIf.GetLinkLocalAddress();
        }
    }

    if (netifId == Ip6::kNetifBackbone)
    {
        GetNode().mInfraIf.SendUdp(srcAddr, aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockPort(),
                                   aMessageInfo.GetPeerPort(), aMessage);
    }
    else
    {
        Ip6::MessageInfo messageInfo(aMessageInfo);

        error = GetNode().Get<Ip6::Udp>().SendDatagram(aMessage, messageInfo);
    }

    return error;
}

Error Udp::JoinMulticastGroup(Ip6::Udp::SocketHandle &aSocket,
                              Ip6::NetifIdentifier    aNetifIdentifier,
                              const Ip6::Address     &aAddress)
{
    OT_UNUSED_VARIABLE(aSocket);
    OT_UNUSED_VARIABLE(aNetifIdentifier);
    OT_UNUSED_VARIABLE(aAddress);
    return kErrorNone;
}

Error Udp::LeaveMulticastGroup(Ip6::Udp::SocketHandle &aSocket,
                               Ip6::NetifIdentifier    aNetifIdentifier,
                               const Ip6::Address     &aAddress)
{
    OT_UNUSED_VARIABLE(aSocket);
    OT_UNUSED_VARIABLE(aNetifIdentifier);
    OT_UNUSED_VARIABLE(aAddress);
    return kErrorNone;
}

bool Udp::HandleReceive(const Message &aMessage, const Ip6::Headers &aHeaders)
{
    bool handled        = false;
    bool isThreadSource = (Core::Get().IsThreadAddress(aHeaders.GetSourceAddress()));

    if (!GetNode().mInfraIf.HasAddress(aHeaders.GetDestinationAddress()) &&
        !GetNode().Get<ThreadNetif>().HasUnicastAddress(aHeaders.GetDestinationAddress()) &&
        !aHeaders.GetDestinationAddress().IsMulticast())
    {
        ExitNow();
    }

    // If the node is attached and the source is on-mesh, we expect the packet
    // via Radio. We drop it here to prevent duplicate delivery when the
    // packet is forwarded to the Infra interface.
    if (GetNode().Get<Mle::Mle>().IsAttached() &&
        GetNode().Get<NetworkData::Leader>().IsOnMesh(aHeaders.GetSourceAddress()))
    {
        ExitNow();
    }

    for (Ip6::Udp::SocketHandle *socket = GetNode().Get<Ip6::Udp>().GetUdpSockets(); socket != nullptr;
         socket                         = socket->GetNext())
    {
        Ip6::MessageInfo messageInfo;

        if (!socket->ShouldUsePlatformUdp())
        {
            continue;
        }

        if (socket->GetSockName().GetPort() != aHeaders.GetDestinationPort())
        {
            continue;
        }

        if (socket->GetSockName().GetAddress().IsUnspecified() ||
            socket->GetSockName().GetAddress() == aHeaders.GetDestinationAddress())
        {
            // Found a matching socket.
        }
        else
        {
            continue;
        }

        if (socket->GetPeerName().GetPort() != 0)
        {
            if (socket->GetPeerName().GetPort() != aHeaders.GetSourcePort() ||
                socket->GetPeerName().GetAddress() != aHeaders.GetSourceAddress())
            {
                continue;
            }
        }

        if (socket->mHandler == nullptr)
        {
            continue;
        }

        messageInfo.SetPeerAddr(aHeaders.GetSourceAddress());
        messageInfo.SetPeerPort(aHeaders.GetSourcePort());
        messageInfo.SetSockAddr(aHeaders.GetDestinationAddress());
        messageInfo.SetSockPort(aHeaders.GetDestinationPort());
        messageInfo.SetHopLimit(aHeaders.GetIp6Header().GetHopLimit());
        messageInfo.SetIsHostInterface(!isThreadSource);

        {
            Message *payload = aMessage.Clone<kNoReservedHeader>();

            VerifyOrExit(payload != nullptr);
            payload->RemoveHeader(sizeof(Ip6::Header) + sizeof(Ip6::Udp::Header));

            socket->mHandler(socket->mContext, payload, &messageInfo);
            payload->Free();
        }

        handled = true;
        if (!aHeaders.GetDestinationAddress().IsMulticast())
        {
            break;
        }
    }

exit:
    return handled;
}

Node &Udp::GetNode(otUdpSocket *aUdpSocket) { return AsNode(static_cast<otInstance *>(aUdpSocket->mHandle)); }

Node &Udp::GetNode(void) { return static_cast<Node &>(GetInstance()); }

const Node &Udp::GetNode(void) const { return static_cast<const Node &>(GetInstance()); }

} // namespace Nexus
} // namespace ot

extern "C" {

otError otPlatUdpSocket(otUdpSocket *aUdpSocket)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket).mUdp.Socket(ot::AsCoreType(aUdpSocket));
}

otError otPlatUdpClose(otUdpSocket *aUdpSocket)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket).mUdp.Close(ot::AsCoreType(aUdpSocket));
}

otError otPlatUdpBind(otUdpSocket *aUdpSocket)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket).mUdp.Bind(ot::AsCoreType(aUdpSocket));
}

otError otPlatUdpBindToNetif(otUdpSocket *aUdpSocket, otNetifIdentifier aNetifIdentifier)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket)
        .mUdp.BindToNetif(ot::AsCoreType(aUdpSocket), ot::MapEnum(aNetifIdentifier));
}

otError otPlatUdpConnect(otUdpSocket *aUdpSocket)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket).mUdp.Connect(ot::AsCoreType(aUdpSocket));
}

otError otPlatUdpSend(otUdpSocket *aUdpSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket)
        .mUdp.Send(ot::AsCoreType(aUdpSocket), ot::AsCoreType(aMessage), ot::AsCoreType(aMessageInfo));
}

otError otPlatUdpJoinMulticastGroup(otUdpSocket        *aUdpSocket,
                                    otNetifIdentifier   aNetifIdentifier,
                                    const otIp6Address *aAddress)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket)
        .mUdp.JoinMulticastGroup(ot::AsCoreType(aUdpSocket), ot::MapEnum(aNetifIdentifier), ot::AsCoreType(aAddress));
}

otError otPlatUdpLeaveMulticastGroup(otUdpSocket        *aUdpSocket,
                                     otNetifIdentifier   aNetifIdentifier,
                                     const otIp6Address *aAddress)
{
    return ot::Nexus::Udp::GetNode(aUdpSocket)
        .mUdp.LeaveMulticastGroup(ot::AsCoreType(aUdpSocket), ot::MapEnum(aNetifIdentifier), ot::AsCoreType(aAddress));
}

} // extern "C"
