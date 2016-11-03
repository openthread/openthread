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
 *   This file implements IPv6 networking.
 */

#define WPP_NAME "ip6.tmh"

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <net/icmp6.hpp>
#include <net/ip6.hpp>
#include <net/ip6_address.hpp>
#include <net/ip6_routes.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <thread/mle.hpp>
#include <openthread-instance.h>

namespace Thread {
namespace Ip6 {

Ip6::Ip6(void):
    mRoutes(*this),
    mIcmp(*this),
    mUdp(*this),
    mMpl(*this),
    mForwardingEnabled(false),
    mSendQueueTask(mTaskletScheduler, HandleSendQueue, this),
    mReceiveIp6DatagramCallback(NULL),
    mReceiveIp6DatagramCallbackContext(NULL),
    mIsReceiveIp6FilterEnabled(false),
    mNetifListHead(NULL)
{
}

Message *Ip6::NewMessage(uint16_t reserved)
{
    return mMessagePool.New(Message::kTypeIp6, sizeof(Header) + sizeof(HopByHopHeader) + sizeof(OptionMpl) + reserved);
}

void Ip6::SetForwardingEnabled(bool aEnable)
{
    mForwardingEnabled = aEnable;
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
    checksum = Ip6::UpdateChecksum(checksum, static_cast<uint16_t>(proto));
    checksum = UpdateChecksum(checksum, src);
    checksum = UpdateChecksum(checksum, dst);

    return checksum;
}

void Ip6::SetReceiveDatagramCallback(otReceiveIp6DatagramCallback aCallback, void *aCallbackContext)
{
    mReceiveIp6DatagramCallback = aCallback;
    mReceiveIp6DatagramCallbackContext = aCallbackContext;
}

bool Ip6::IsReceiveIp6FilterEnabled(void)
{
    return mIsReceiveIp6FilterEnabled;
}

void Ip6::SetReceiveIp6FilterEnabled(bool aEnabled)
{
    mIsReceiveIp6FilterEnabled = aEnabled;
}

ThreadError Ip6::AddMplOption(Message &message, Header &header)
{
    ThreadError error = kThreadError_None;
    HopByHopHeader hbhHeader;
    OptionMpl mplOption;
    OptionPadN padOption;

    hbhHeader.SetNextHeader(header.GetNextHeader());
    hbhHeader.SetLength(0);
    mMpl.InitOption(mplOption, header.GetSource());

    // Mpl option may require two bytes padding.
    if ((mplOption.GetTotalLength() + sizeof(hbhHeader)) % 8)
    {
        padOption.Init(2);
        SuccessOrExit(error = message.Prepend(&padOption, padOption.GetTotalLength()));
    }

    SuccessOrExit(error = message.Prepend(&mplOption, mplOption.GetTotalLength()));
    SuccessOrExit(error = message.Prepend(&hbhHeader, sizeof(hbhHeader)));
    header.SetPayloadLength(header.GetPayloadLength() + sizeof(hbhHeader) + sizeof(mplOption));
    header.SetNextHeader(kProtoHopOpts);
exit:
    return error;
}

ThreadError Ip6::AddTunneledMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Header tunnelHeader;
    const NetifUnicastAddress *source;
    MessageInfo messageInfo(aMessageInfo);

    // Use IP-in-IP encapsulation (RFC2473) and ALL_MPL_FORWARDERS address.
    memset(&messageInfo.GetPeerAddr(), 0, sizeof(Address));
    messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xff03);
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(0x00fc);

    tunnelHeader.Init();
    tunnelHeader.SetHopLimit(static_cast<uint8_t>(kDefaultHopLimit));
    tunnelHeader.SetPayloadLength(aHeader.GetPayloadLength() + sizeof(tunnelHeader));
    tunnelHeader.SetDestination(messageInfo.GetPeerAddr());
    tunnelHeader.SetNextHeader(kProtoIp6);

    VerifyOrExit((source = SelectSourceAddress(messageInfo)) != NULL,
                 error = kThreadError_Error);

    tunnelHeader.SetSource(source->GetAddress());

    SuccessOrExit(error = AddMplOption(aMessage, tunnelHeader));
    SuccessOrExit(error = aMessage.Prepend(&tunnelHeader, sizeof(tunnelHeader)));

exit:
    return error;
}

ThreadError Ip6::InsertMplOption(Message &aMessage, Header &aIp6Header, MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aIp6Header.GetDestination().IsMulticast() &&
                 aIp6Header.GetDestination().GetScope() >= Address::kRealmLocalScope, ;);

    if (aIp6Header.GetDestination().IsRealmLocalMulticast())
    {
        aMessage.RemoveHeader(sizeof(aIp6Header));

        if (aIp6Header.GetNextHeader() == kProtoHopOpts)
        {
            HopByHopHeader hbh;
            uint8_t hbhLength = 0;
            OptionMpl mplOption;

            // read existing hop-by-hop option header
            aMessage.Read(0, sizeof(hbh), &hbh);
            hbhLength = (hbh.GetLength() + 1) * 8;

            // increase existing hop-by-hop option header length by 8 bytes
            hbh.SetLength(hbh.GetLength() + 1);
            aMessage.Write(0, sizeof(hbh), &hbh);

            // make space for MPL Option + padding by shifting hop-by-hop option header
            aMessage.Prepend(NULL, 8);
            aMessage.CopyTo(8, 0, hbhLength, aMessage);

            // insert MPL Option
            mMpl.InitOption(mplOption, aIp6Header.GetSource());
            aMessage.Write(hbhLength, mplOption.GetTotalLength(), &mplOption);

            // insert Pad Option (if needed)
            if (mplOption.GetTotalLength() % 8)
            {
                OptionPadN padOption;
                padOption.Init(8 - (mplOption.GetTotalLength() % 8));
                aMessage.Write(hbhLength + mplOption.GetTotalLength(), padOption.GetTotalLength(), &padOption);
            }

            // increase IPv6 Payload Length
            aIp6Header.SetPayloadLength(aIp6Header.GetPayloadLength() + 8);
        }
        else
        {
            SuccessOrExit(error = AddMplOption(aMessage, aIp6Header));
        }

        SuccessOrExit(error = aMessage.Prepend(&aIp6Header, sizeof(aIp6Header)));
    }
    else
    {
        SuccessOrExit(error = AddTunneledMplOption(aMessage, aIp6Header, aMessageInfo));
    }

exit:
    return error;
}

ThreadError Ip6::RemoveMplOption(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    Header ip6Header;
    HopByHopHeader hbh;
    uint16_t offset;
    uint16_t endOffset;
    uint16_t mplOffset = 0;
    uint8_t mplLength = 0;
    bool remove = false;

    offset = 0;
    aMessage.Read(offset, sizeof(ip6Header), &ip6Header);
    offset += sizeof(ip6Header);
    VerifyOrExit(ip6Header.GetNextHeader() == kProtoHopOpts,);

    aMessage.Read(offset, sizeof(hbh), &hbh);
    endOffset = offset + (hbh.GetLength() + 1) * 8;
    VerifyOrExit(aMessage.GetLength() >= endOffset,);

    offset += sizeof(hbh);

    while (offset < endOffset)
    {
        OptionHeader option;

        aMessage.Read(offset, sizeof(option), &option);

        switch (option.GetType())
        {
        case OptionMpl::kType:
            mplOffset = offset;
            mplLength = option.GetLength();

            if (mplOffset == sizeof(ip6Header) + sizeof(hbh) && hbh.GetLength() == 0)
            {
                // first and only IPv6 Option, remove IPv6 HBH Option header
                remove = true;
            }
            else if (mplOffset + 8 == endOffset)
            {
                // last IPv6 Option, remove last 8 bytes
                remove = true;
            }

            offset += sizeof(option) + option.GetLength();
            break;

        case OptionPad1::kType:
            offset += sizeof(OptionPad1);
            break;

        case OptionPadN::kType:
            offset += sizeof(option) + option.GetLength();
            break;

        default:
            // encountered another option, now just replace MPL Option with PadN
            remove = false;
            offset += sizeof(option) + option.GetLength();
            break;
        }
    }

    // verify that IPv6 Options header is properly formed
    VerifyOrExit(offset == endOffset,);

    if (remove)
    {
        // last IPv6 Option, shrink HBH Option header
        uint8_t buf[8];

        offset = endOffset - sizeof(buf);

        while (offset >= sizeof(buf))
        {
            aMessage.Read(offset - sizeof(buf), sizeof(buf), buf);
            aMessage.Write(offset, sizeof(buf), buf);
            offset -= sizeof(buf);
        }

        aMessage.RemoveHeader(sizeof(buf));

        if (mplOffset == sizeof(ip6Header) + sizeof(hbh))
        {
            // remove entire HBH header
            ip6Header.SetNextHeader(hbh.GetNextHeader());
        }
        else
        {
            // update HBH header length
            hbh.SetLength(hbh.GetLength() - 1);
            aMessage.Write(sizeof(ip6Header), sizeof(hbh), &hbh);
        }

        ip6Header.SetPayloadLength(ip6Header.GetPayloadLength() - sizeof(buf));
        aMessage.Write(0, sizeof(ip6Header), &ip6Header);
    }
    else if (mplOffset != 0)
    {
        // replace MPL Option with PadN Option
        OptionPadN padOption;

        padOption.Init(sizeof(OptionHeader) + mplLength);
        aMessage.Write(mplOffset, padOption.GetTotalLength(), &padOption);
    }

exit:
    return error;
}

void Ip6::EnqueueDatagram(Message &aMessage)
{
    mSendQueue.Enqueue(aMessage);
    mSendQueueTask.Post();
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
        VerifyOrExit((source = SelectSourceAddress(messageInfo)) != NULL,
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
        VerifyOrExit(messageInfo.GetInterfaceId() != 0, error = kThreadError_Drop);
    }

    if (messageInfo.GetPeerAddr().IsRealmLocalMulticast())
    {
        SuccessOrExit(error = AddMplOption(message, header));
    }

    SuccessOrExit(error = message.Prepend(&header, sizeof(header)));

    if (messageInfo.GetPeerAddr().IsMulticast() &&
        messageInfo.GetPeerAddr().GetScope() > Address::kRealmLocalScope)
    {
        SuccessOrExit(error = AddTunneledMplOption(message, header, messageInfo));
    }

    // compute checksum
    checksum = ComputePseudoheaderChecksum(header.GetSource(), header.GetDestination(),
                                           payloadLength, ipproto);

    switch (ipproto)
    {
    case kProtoUdp:
        SuccessOrExit(error = mUdp.UpdateChecksum(message, checksum));
        break;

    case kProtoIcmp6:
        SuccessOrExit(error = mIcmp.UpdateChecksum(message, checksum));
        break;

    default:
        break;
    }

exit:

    if (error == kThreadError_None)
    {
        message.SetInterfaceId(messageInfo.GetInterfaceId());
        EnqueueDatagram(message);
    }

    return error;
}

void Ip6::HandleSendQueue(void *aContext)
{
    static_cast<Ip6 *>(aContext)->HandleSendQueue();
}

void Ip6::HandleSendQueue(void)
{
    while (mSendQueue.GetHead())
    {
        Message *message = mSendQueue.GetHead();
        mSendQueue.Dequeue(*message);
        HandleDatagram(*message, NULL, message->GetInterfaceId(), NULL, false);
    }
}

ThreadError Ip6::HandleOptions(Message &message, Header &header, bool &forward)
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
            SuccessOrExit(error = mMpl.ProcessOption(message, header.GetSource(), forward));
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

        if (optionHeader.GetType() == OptionPad1::kType)
        {
            message.MoveOffset(sizeof(OptionPad1));
        }
        else
        {
            message.MoveOffset(sizeof(optionHeader) + optionHeader.GetLength());
        }

    }

exit:
    return error;
}

ThreadError Ip6::HandleFragment(Message &message)
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

ThreadError Ip6::HandleExtensionHeaders(Message &message, Header &header, uint8_t &nextHeader, bool forward,
                                        bool receive)
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
            SuccessOrExit(error = HandleOptions(message, header, forward));
            break;

        case kProtoFragment:
            SuccessOrExit(error = HandleFragment(message));
            break;

        case kProtoDstOpts:
            SuccessOrExit(error = HandleOptions(message, header, forward));
            break;

        case kProtoIp6:
            ExitNow();

        case kProtoRouting:
        case kProtoNone:
            ExitNow(error = kThreadError_Drop);

        default:
            ExitNow();
        }

        nextHeader = static_cast<uint8_t>(extensionHeader.GetNextHeader());
    }

exit:
    return error;
}

ThreadError Ip6::HandlePayload(Message &message, MessageInfo &messageInfo, uint8_t ipproto)
{
    ThreadError error = kThreadError_None;

    switch (ipproto)
    {
    case kProtoUdp:
        ExitNow(error = mUdp.HandleMessage(message, messageInfo));

    case kProtoIcmp6:
        ExitNow(error = mIcmp.HandleMessage(message, messageInfo));
    }

exit:
    return error;
}

ThreadError Ip6::ProcessReceiveCallback(const Message &aMessage, const MessageInfo &messageInfo, uint8_t aIpProto)
{
    ThreadError error = kThreadError_None;
    Message *messageCopy = NULL;

    VerifyOrExit(mReceiveIp6DatagramCallback != NULL, error = kThreadError_NoRoute);

    if (mIsReceiveIp6FilterEnabled)
    {
        // do not pass messages sent to/from an RLOC
        VerifyOrExit(!messageInfo.GetSockAddr().IsRoutingLocator() &&
                     !messageInfo.GetPeerAddr().IsRoutingLocator(),
                     error = kThreadError_NoRoute);

        switch (aIpProto)
        {
        case kProtoIcmp6:
            if (mIcmp.IsEchoEnabled())
            {
                IcmpHeader icmp;
                aMessage.Read(aMessage.GetOffset(), sizeof(icmp), &icmp);

                // do not pass ICMP Echo Request messages
                VerifyOrExit(icmp.GetType() != IcmpHeader::kTypeEchoRequest, error = kThreadError_NoRoute);
            }

            break;

        case kProtoUdp:
            if (messageInfo.GetSockAddr().IsLinkLocal())
            {
                UdpHeader udp;
                aMessage.Read(aMessage.GetOffset(), sizeof(udp), &udp);

                // do not pass MLE messages
                VerifyOrExit(udp.GetDestinationPort() != Mle::kUdpPort, error = kThreadError_NoRoute);
            }

            break;

        default:
            break;
        }
    }

    // make a copy of the datagram to pass to host
    VerifyOrExit((messageCopy = aMessage.Clone()) != NULL, error = kThreadError_NoBufs);
    RemoveMplOption(*messageCopy);
    mReceiveIp6DatagramCallback(messageCopy, mReceiveIp6DatagramCallbackContext);

exit:
    return error;
}

ThreadError Ip6::HandleDatagram(Message &message, Netif *netif, int8_t interfaceId, const void *linkMessageInfo,
                                bool fromLocalHost)
{
    ThreadError error = kThreadError_None;
    MessageInfo messageInfo;
    Header header;
    uint16_t payloadLength;
    bool receive = false;
    bool forward = false;
    bool tunnel = false;
    bool multicastPromiscuous = false;
    uint8_t nextHeader;
    uint8_t hopLimit;

    otLogFuncEntry();

#if 0
    uint8_t buf[1024];
    message.Read(0, sizeof(buf), buf);
    dump("handle datagram", buf, message.GetLength());
#endif

    // check message length
    VerifyOrExit(message.GetLength() >= sizeof(header), error = kThreadError_Drop);
    message.Read(0, sizeof(header), &header);
    payloadLength = header.GetPayloadLength();

    // check Version
    VerifyOrExit(header.IsVersion6(), error = kThreadError_Drop);

    // check Payload Length
    VerifyOrExit(sizeof(header) + payloadLength == message.GetLength() &&
                 sizeof(header) + payloadLength <= Ip6::kMaxDatagramLength, error = kThreadError_Drop);

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetInterfaceId(interfaceId);
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetLinkInfo(linkMessageInfo);

    // determine destination of packet
    if (header.GetDestination().IsMulticast())
    {
        if (netif != NULL)
        {
            if (netif->IsMulticastSubscribed(header.GetDestination()))
            {
                receive = true;
            }
            else if (netif->IsMulticastPromiscuousModeEnabled())
            {
                multicastPromiscuous = true;
            }
        }

        if (netif == NULL)
        {
            forward = true;

            if (fromLocalHost)
            {
                SuccessOrExit(error = InsertMplOption(message, header, messageInfo));
            }
        }
    }
    else
    {
        if (IsUnicastAddress(header.GetDestination()))
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

    message.SetInterfaceId(interfaceId);
    message.SetOffset(sizeof(header));

    // process IPv6 Extension Headers
    nextHeader = static_cast<uint8_t>(header.GetNextHeader());
    SuccessOrExit(error = HandleExtensionHeaders(message, header, nextHeader, forward, receive));

    if (!mForwardingEnabled && netif != NULL)
    {
        forward = false;
    }

    // process IPv6 Payload
    if (receive)
    {
        if (nextHeader == kProtoIp6)
        {
            // Remove encapsulating header.
            message.RemoveHeader(message.GetOffset());

            HandleDatagram(message, netif, interfaceId, linkMessageInfo, fromLocalHost);
            ExitNow(tunnel = true);
        }

        if (fromLocalHost == false)
        {
            ProcessReceiveCallback(message, messageInfo, nextHeader);
        }

        SuccessOrExit(error = HandlePayload(message, messageInfo, nextHeader));
    }
    else if (multicastPromiscuous)
    {
        ProcessReceiveCallback(message, messageInfo, nextHeader);
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
            ExitNow(error = kThreadError_Drop);
        }
        else
        {
            hopLimit = header.GetHopLimit();
            message.Write(Header::GetHopLimitOffset(), Header::GetHopLimitSize(), &hopLimit);
            SuccessOrExit(error = ForwardMessage(message, messageInfo, nextHeader));
        }
    }

exit:

    if (!tunnel && (error != kThreadError_None || !forward))
    {
        message.Free();
    }

    otLogFuncExitErr(error);
    return error;
}

ThreadError Ip6::ForwardMessage(Message &message, MessageInfo &messageInfo, uint8_t ipproto)
{
    ThreadError error = kThreadError_None;
    int8_t interfaceId;
    Netif *netif;

    otLogFuncEntry();

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
    else if ((interfaceId = GetOnLinkNetif(messageInfo.GetSockAddr())) > 0)
    {
        // on-link global address
        ;
    }
    else if ((interfaceId = mRoutes.Lookup(messageInfo.GetPeerAddr(), messageInfo.GetSockAddr())) > 0)
    {
        // route
        ;
    }
    else
    {
        // try passing to host
        error = ProcessReceiveCallback(message, messageInfo, ipproto);

        switch (error)
        {
        case kThreadError_None:
            // the caller transfers custody in the success case, so free the message here
            message.Free();
            break;

        case kThreadError_NoRoute:
            otDumpDebgIp6("no route", &messageInfo.GetSockAddr(), 16);
            break;

        default:
            break;
        }

        ExitNow();
    }

    // submit message to interface
    VerifyOrExit((netif = GetNetifById(interfaceId)) != NULL, error = kThreadError_NoRoute);
    SuccessOrExit(error = netif->SendMessage(message));

exit:

    otLogFuncExitErr(error);
    return error;
}

ThreadError Ip6::AddNetif(Netif &aNetif)
{
    ThreadError error = kThreadError_None;
    Netif *netif;

    if (mNetifListHead == NULL)
    {
        mNetifListHead = &aNetif;
    }
    else
    {
        netif = mNetifListHead;

        do
        {
            if (netif == &aNetif || netif->mInterfaceId == aNetif.mInterfaceId)
            {
                ExitNow(error = kThreadError_Already);
            }
        }
        while (netif->mNext);

        netif->mNext = &aNetif;
    }

    aNetif.mNext = NULL;

exit:
    return error;
}

ThreadError Ip6::RemoveNetif(Netif &aNetif)
{
    ThreadError error = kThreadError_NotFound;

    VerifyOrExit(mNetifListHead != NULL, error = kThreadError_NotFound);

    if (mNetifListHead == &aNetif)
    {
        mNetifListHead = aNetif.mNext;
    }
    else
    {
        for (Netif *netif = mNetifListHead; netif->mNext; netif = netif->mNext)
        {
            if (netif->mNext != &aNetif)
            {
                continue;
            }

            netif->mNext = aNetif.mNext;
            error = kThreadError_None;
            break;
        }
    }

    aNetif.mNext = NULL;

exit:
    return error;
}

Netif *Ip6::GetNetifList()
{
    return mNetifListHead;
}

Netif *Ip6::GetNetifById(int8_t aInterfaceId)
{
    Netif *netif;

    for (netif = mNetifListHead; netif; netif = netif->mNext)
    {
        if (netif->GetInterfaceId() == aInterfaceId)
        {
            ExitNow();
        }
    }

exit:
    return netif;
}

bool Ip6::IsUnicastAddress(const Address &aAddress)
{
    bool rval = false;

    for (Netif *netif = mNetifListHead; netif; netif = netif->mNext)
    {
        rval = netif->IsUnicastAddress(aAddress);

        if (rval)
        {
            ExitNow();
        }
    }

exit:
    return rval;
}

const NetifUnicastAddress *Ip6::SelectSourceAddress(MessageInfo &aMessageInfo)
{
    Address *destination = &aMessageInfo.GetPeerAddr();
    int interfaceId = aMessageInfo.mInterfaceId;
    const NetifUnicastAddress *rvalAddr = NULL;
    const Address *candidateAddr;
    int8_t candidateId;
    int8_t rvalIface = 0;

    for (Netif *netif = GetNetifList(); netif; netif = netif->mNext)
    {
        candidateId = netif->GetInterfaceId();

        for (const NetifUnicastAddress *addr = netif->mUnicastAddresses; addr; addr = addr->GetNext())
        {
            candidateAddr = &addr->GetAddress();

            if (destination->IsLinkLocal() || destination->IsMulticast())
            {
                if (interfaceId != candidateId)
                {
                    continue;
                }
            }

            if (rvalAddr == NULL)
            {
                // Rule 0: Prefer any address
                rvalAddr = addr;
                rvalIface = candidateId;
            }
            else if (*candidateAddr == *destination)
            {
                // Rule 1: Prefer same address
                rvalAddr = addr;
                rvalIface = candidateId;
                goto exit;
            }
            else if (candidateAddr->GetScope() < rvalAddr->GetAddress().GetScope())
            {
                // Rule 2: Prefer appropriate scope
                if (candidateAddr->GetScope() >= destination->GetScope())
                {
                    rvalAddr = addr;
                    rvalIface = candidateId;
                }
            }
            else if (candidateAddr->GetScope() > rvalAddr->GetAddress().GetScope())
            {
                if (rvalAddr->GetAddress().GetScope() < destination->GetScope())
                {
                    rvalAddr = addr;
                    rvalIface = candidateId;
                }
            }
            else if (addr->mPreferredLifetime != 0 && rvalAddr->mPreferredLifetime == 0)
            {
                // Rule 3: Avoid deprecated addresses
                rvalAddr = addr;
                rvalIface = candidateId;
            }
            else if (aMessageInfo.mInterfaceId != 0 && aMessageInfo.mInterfaceId == candidateId &&
                     rvalIface != candidateId)
            {
                // Rule 4: Prefer home address
                // Rule 5: Prefer outgoing interface
                rvalAddr = addr;
                rvalIface = candidateId;
            }
            else if (destination->PrefixMatch(*candidateAddr) > destination->PrefixMatch(rvalAddr->GetAddress()))
            {
                // Rule 6: Prefer matching label
                // Rule 7: Prefer public address
                // Rule 8: Use longest prefix matching
                rvalAddr = addr;
                rvalIface = candidateId;
            }
        }
    }

exit:
    aMessageInfo.mInterfaceId = rvalIface;
    return rvalAddr;
}

int8_t Ip6::GetOnLinkNetif(const Address &aAddress)
{
    int8_t rval = -1;

    for (Netif *netif = mNetifListHead; netif; netif = netif->mNext)
    {
        for (const NetifUnicastAddress *cur = netif->mUnicastAddresses; cur; cur = cur->GetNext())
        {
            if (cur->GetAddress().PrefixMatch(aAddress) >= cur->mPrefixLength)
            {
                ExitNow(rval = netif->GetInterfaceId());
            }
        }
    }

exit:
    return rval;
}

otInstance *Ip6::GetInstance()
{
    return otInstanceFromIp6(this);
}

}  // namespace Ip6
}  // namespace Thread
