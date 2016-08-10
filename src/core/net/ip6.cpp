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
 *   This file implements IPv6 networking.
 */

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <common/new.hpp>
#include <net/icmp6.hpp>
#include <net/ip6.hpp>
#include <net/ip6_address.hpp>
#include <net/ip6_mpl.hpp>
#include <net/ip6_routes.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <openthreadcontext.h>

namespace Thread {
namespace Ip6 {

static ThreadError ForwardMessage(Message &message, MessageInfo &messageInfo);

Message *Ip6::NewMessage(otContext *aContext, uint16_t reserved)
{
    return Message::New(aContext, Message::kTypeIp6,
                        sizeof(Header) + sizeof(HopByHopHeader) + sizeof(OptionMpl) + reserved);
}

uint16_t Ip6::UpdateChecksum(uint16_t checksum, uint16_t val)
{
    uint16_t result = checksum + val;
    return result + (result < checksum);
}

uint16_t Ip6::UpdateChecksum(uint16_t checksum, const void *buf, uint16_t len)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(buf);

    for (int i = 0; i < len; i++)
    {
        checksum = Ip6::UpdateChecksum(checksum, (i & 1) ? bytes[i] : static_cast<uint16_t>(bytes[i] << 8));
    }

    return checksum;
}

uint16_t Ip6::UpdateChecksum(uint16_t checksum, const Address &address)
{
    return Ip6::UpdateChecksum(checksum, address.mFields.m8, sizeof(address));
}

uint16_t Ip6::ComputePseudoheaderChecksum(const Address &src, const Address &dst, uint16_t length, IpProto proto)
{
    uint16_t checksum;

    checksum = Ip6::UpdateChecksum(0, length);
    checksum = Ip6::UpdateChecksum(checksum, proto);
    checksum = UpdateChecksum(checksum, src);
    checksum = UpdateChecksum(checksum, dst);

    return checksum;
}

void Ip6::SetReceiveDatagramCallback(otContext *aContext, otReceiveIp6DatagramCallback aCallback)
{
    aContext->mReceiveIp6DatagramCallback = aCallback;
}

ThreadError AddMplOption(Message &message, Header &header, IpProto nextHeader, uint16_t payloadLength)
{
    ThreadError error = kThreadError_None;
    HopByHopHeader hbhHeader;
    OptionMpl mplOption;

    hbhHeader.SetNextHeader(nextHeader);
    hbhHeader.SetLength(0);
    message.GetOpenThreadContext()->mMpl.InitOption(mplOption, HostSwap16(header.GetSource().mFields.m16[7]));
    SuccessOrExit(error = message.Prepend(&mplOption, sizeof(mplOption)));
    SuccessOrExit(error = message.Prepend(&hbhHeader, sizeof(hbhHeader)));
    header.SetPayloadLength(sizeof(hbhHeader) + sizeof(mplOption) + payloadLength);
    header.SetNextHeader(kProtoHopOpts);
exit:
    return error;
}

ThreadError Ip6::SendDatagram(Message &message, MessageInfo &messageInfo, IpProto ipproto)
{
    ThreadError error = kThreadError_None;
    Header header;
    uint16_t payloadLength = message.GetLength();
    uint16_t checksum;
    const NetifUnicastAddress *source;

    header.Init();
    header.SetPayloadLength(payloadLength);
    header.SetNextHeader(ipproto);
    header.SetHopLimit(messageInfo.mHopLimit ? messageInfo.mHopLimit : static_cast<uint8_t>(kDefaultHopLimit));

    if (messageInfo.GetSockAddr().IsUnspecified())
    {
        VerifyOrExit((source = Netif::SelectSourceAddress(message.GetOpenThreadContext(), messageInfo)) != NULL,
                     error = kThreadError_Error);
        header.SetSource(source->GetAddress());
    }
    else
    {
        header.SetSource(messageInfo.GetSockAddr());
    }

    header.SetDestination(messageInfo.GetPeerAddr());

    if (header.GetDestination().IsLinkLocal() || header.GetDestination().IsLinkLocalMulticast())
    {
        VerifyOrExit(messageInfo.mInterfaceId != 0, error = kThreadError_Drop);
    }

    if (messageInfo.GetPeerAddr().IsRealmLocalMulticast())
    {
        SuccessOrExit(error = AddMplOption(message, header, ipproto, payloadLength));
    }

    SuccessOrExit(error = message.Prepend(&header, sizeof(header)));

    // compute checksum
    checksum = ComputePseudoheaderChecksum(header.GetSource(), header.GetDestination(),
                                           payloadLength, ipproto);

    switch (ipproto)
    {
    case kProtoUdp:
        SuccessOrExit(error = Udp::UpdateChecksum(message, checksum));
        break;

    case kProtoIcmp6:
        SuccessOrExit(error = Icmp::UpdateChecksum(message, checksum));
        break;

    default:
        break;
    }

exit:

    if (error == kThreadError_None)
    {
        error = HandleDatagram(message, NULL, messageInfo.mInterfaceId, NULL, false);
    }

    return error;
}

ThreadError HandleOptions(Message &message)
{
    ThreadError error = kThreadError_None;
    HopByHopHeader hbhHeader;
    OptionHeader optionHeader;
    uint16_t endOffset;

    message.Read(message.GetOffset(), sizeof(hbhHeader), &hbhHeader);
    endOffset = message.GetOffset() + (hbhHeader.GetLength() + 1) * 8;

    message.MoveOffset(sizeof(optionHeader));

    while (message.GetOffset() < endOffset)
    {
        message.Read(message.GetOffset(), sizeof(optionHeader), &optionHeader);

        switch (optionHeader.GetType())
        {
        case OptionMpl::kType:
            SuccessOrExit(error = message.GetOpenThreadContext()->mMpl.ProcessOption(message));
            break;

        default:
            switch (optionHeader.GetAction())
            {
            case OptionHeader::kActionSkip:
                break;

            case OptionHeader::kActionDiscard:
                ExitNow(error = kThreadError_Drop);

            case OptionHeader::kActionForceIcmp:
                // TODO: send icmp error
                ExitNow(error = kThreadError_Drop);

            case OptionHeader::kActionIcmp:
                // TODO: send icmp error
                ExitNow(error = kThreadError_Drop);

            }

            break;
        }

        message.MoveOffset(sizeof(optionHeader) + optionHeader.GetLength());
    }

exit:
    return error;
}

ThreadError HandleFragment(Message &message)
{
    ThreadError error = kThreadError_None;
    FragmentHeader fragmentHeader;

    message.Read(message.GetOffset(), sizeof(fragmentHeader), &fragmentHeader);

    VerifyOrExit(fragmentHeader.GetOffset() == 0 && fragmentHeader.IsMoreFlagSet() == false,
                 error = kThreadError_Drop);

    message.MoveOffset(sizeof(fragmentHeader));

exit:
    return error;
}

ThreadError HandleExtensionHeaders(Message &message, uint8_t &nextHeader, bool receive)
{
    ThreadError error = kThreadError_None;
    ExtensionHeader extensionHeader;

    while (receive == true || nextHeader == kProtoHopOpts)
    {
        VerifyOrExit(message.GetOffset() <= message.GetLength(), error = kThreadError_Drop);

        message.Read(message.GetOffset(), sizeof(extensionHeader), &extensionHeader);

        switch (nextHeader)
        {
        case kProtoHopOpts:
            SuccessOrExit(error = HandleOptions(message));
            break;

        case kProtoFragment:
            SuccessOrExit(error = HandleFragment(message));
            break;

        case kProtoDstOpts:
            SuccessOrExit(error = HandleOptions(message));
            break;

        case kProtoIp6:
        case kProtoRouting:
        case kProtoNone:
            ExitNow(error = kThreadError_Drop);

        default:
            ExitNow();
        }

        nextHeader = extensionHeader.GetNextHeader();
    }

exit:
    return error;
}

ThreadError HandlePayload(Message &message, MessageInfo &messageInfo, uint8_t ipproto)
{
    ThreadError error = kThreadError_None;

    switch (ipproto)
    {
    case kProtoUdp:
        ExitNow(error = Udp::HandleMessage(message, messageInfo));

    case kProtoIcmp6:
        ExitNow(error = Icmp::HandleMessage(message, messageInfo));
    }

exit:
    return error;
}

void Ip6::ProcessReceiveCallback(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    Message *messageCopy = NULL;

    VerifyOrExit(aMessage.GetOpenThreadContext()->mReceiveIp6DatagramCallback != NULL, ;);

    // make a copy of the datagram to pass to host
    VerifyOrExit((messageCopy = NewMessage(aMessage.GetOpenThreadContext(), 0)) != NULL, ;);
    SuccessOrExit(error = messageCopy->SetLength(aMessage.GetLength()));
    VerifyOrExit(aMessage.CopyTo(0, 0, aMessage.GetLength(), *messageCopy) == aMessage.GetLength(),
                 error = kThreadError_Drop);

    aMessage.GetOpenThreadContext()->mReceiveIp6DatagramCallback(messageCopy);

exit:

    if (error != kThreadError_None && messageCopy != NULL)
    {
        Message::Free(*messageCopy);
    }
}

ThreadError Ip6::HandleDatagram(Message &message, Netif *netif, int8_t interfaceId, const void *linkMessageInfo,
                                bool fromLocalHost)
{
    ThreadError error = kThreadError_Drop;
    MessageInfo messageInfo;
    Header header;
    uint16_t payloadLength;
    bool receive = false;
    bool forward = false;
    uint8_t nextHeader;
    uint8_t hopLimit;

#if 0
    uint8_t buf[1024];
    message.Read(0, sizeof(buf), buf);
    dump("handle datagram", buf, message.GetLength());
#endif

    // check message length
    VerifyOrExit(message.GetLength() >= sizeof(header), ;);
    message.Read(0, sizeof(header), &header);
    payloadLength = header.GetPayloadLength();

    // check Version
    VerifyOrExit(header.IsVersion6(), ;);

    // check Payload Length
    VerifyOrExit(sizeof(header) + payloadLength == message.GetLength() &&
                 sizeof(header) + payloadLength <= Ip6::kMaxDatagramLength, ;);

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = header.GetSource();
    messageInfo.GetSockAddr() = header.GetDestination();
    messageInfo.mInterfaceId = interfaceId;
    messageInfo.mHopLimit = header.GetHopLimit();
    messageInfo.mLinkInfo = linkMessageInfo;

    // determine destination of packet
    if (header.GetDestination().IsMulticast())
    {
        if (netif != NULL && netif->IsMulticastSubscribed(header.GetDestination()))
        {
            receive = true;
        }

        if (header.GetDestination().GetScope() > Address::kLinkLocalScope)
        {
            forward = true;
        }
        else if (netif == NULL)
        {
            forward = true;
        }
    }
    else
    {
        if (Netif::IsUnicastAddress(message.GetOpenThreadContext(), header.GetDestination()))
        {
            receive = true;
        }
        else if (!header.GetDestination().IsLinkLocal())
        {
            forward = true;
        }
        else if (netif == NULL)
        {
            forward = true;
        }
    }

    message.SetOffset(sizeof(header));

    // process IPv6 Extension Headers
    nextHeader = header.GetNextHeader();
    SuccessOrExit(HandleExtensionHeaders(message, nextHeader, receive));

    // process IPv6 Payload
    if (receive)
    {
        if (fromLocalHost == false)
        {
            ProcessReceiveCallback(message);
        }

        SuccessOrExit(HandlePayload(message, messageInfo, nextHeader));
    }

    if (forward)
    {
        if (netif != NULL)
        {
            header.SetHopLimit(header.GetHopLimit() - 1);
        }

        if (header.GetHopLimit() == 0)
        {
            // send time exceeded
        }
        else
        {
            hopLimit = header.GetHopLimit();
            message.Write(Header::GetHopLimitOffset(), Header::GetHopLimitSize(), &hopLimit);
            SuccessOrExit(ForwardMessage(message, messageInfo));
            ExitNow(error = kThreadError_None);
        }
    }

exit:

    if (error == kThreadError_Drop)
    {
        Message::Free(message);
    }

    return kThreadError_None;
}

ThreadError ForwardMessage(Message &message, MessageInfo &messageInfo)
{
    ThreadError error = kThreadError_None;
    int8_t interfaceId;
    Netif *netif;

    if (messageInfo.GetSockAddr().IsMulticast())
    {
        // multicast
        interfaceId = messageInfo.mInterfaceId;
    }
    else if (messageInfo.GetSockAddr().IsLinkLocal())
    {
        // on-link link-local address
        interfaceId = messageInfo.mInterfaceId;
    }
    else if ((interfaceId = Netif::GetOnLinkNetif(message.GetOpenThreadContext(), messageInfo.GetSockAddr())) > 0)
    {
        // on-link global address
        ;
    }
    else if ((interfaceId = Routes::Lookup(message.GetOpenThreadContext(), messageInfo.GetPeerAddr(),
                                           messageInfo.GetSockAddr())) > 0)
    {
        // route
        ;
    }
    else
    {
        otDumpDebgIp6("no route", &messageInfo.GetSockAddr(), 16);
        ExitNow(error = kThreadError_NoRoute);
    }

    // submit message to interface
    VerifyOrExit((netif = Netif::GetNetifById(message.GetOpenThreadContext(), interfaceId)) != NULL,
                 error = kThreadError_NoRoute);
    SuccessOrExit(error = netif->SendMessage(message));

exit:
    return error;
}

}  // namespace Ip6
}  // namespace Thread
