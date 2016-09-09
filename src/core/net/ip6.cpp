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
#include <net/ip6_routes.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <thread/mle.hpp>
#include <openthreadinstance.h>

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
    mNetifListHead(NULL),
    mNextInterfaceId(1)
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
    checksum = Ip6::UpdateChecksum(checksum, proto);
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

ThreadError Ip6::AddMplOption(Message &message, Header &header, IpProto nextHeader, uint16_t payloadLength)
{
    ThreadError error = kThreadError_None;
    HopByHopHeader hbhHeader;
    OptionMpl mplOption;

    hbhHeader.SetNextHeader(nextHeader);
    hbhHeader.SetLength(0);
    mMpl.InitOption(mplOption, HostSwap16(header.GetSource().mFields.m16[7]));
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
        VerifyOrExit((source = SelectSourceAddress(messageInfo)) != NULL, error = kThreadError_Error);
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
        message.SetInterfaceId(messageInfo.mInterfaceId);
        mSendQueue.Enqueue(message);
        mSendQueueTask.Post();
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

ThreadError Ip6::HandleOptions(Message &message)
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
            SuccessOrExit(error = mMpl.ProcessOption(message));
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

ThreadError Ip6::HandleExtensionHeaders(Message &message, uint8_t &nextHeader, bool receive)
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

void Ip6::ProcessReceiveCallback(const Message &aMessage, const MessageInfo &messageInfo, uint8_t aIpProto)
{
    ThreadError error = kThreadError_None;
    Message *messageCopy = NULL;

    VerifyOrExit(mReceiveIp6DatagramCallback != NULL, ;);

    if (mIsReceiveIp6FilterEnabled)
    {
        // do not pass messages sent to/from an RLOC
        VerifyOrExit(!messageInfo.GetSockAddr().IsRoutingLocator() &&
                     !messageInfo.GetPeerAddr().IsRoutingLocator(), ;);

        switch (aIpProto)
        {
        case kProtoIcmp6:
            if (mIcmp.IsEchoEnabled())
            {
                IcmpHeader icmp;
                aMessage.Read(aMessage.GetOffset(), sizeof(icmp), &icmp);

                // do not pass ICMP Echo Request messages
                VerifyOrExit(icmp.GetType() != IcmpHeader::kTypeEchoRequest, ;);
            }

            break;

        case kProtoUdp:
            if (messageInfo.GetSockAddr().IsLinkLocal())
            {
                UdpHeader udp;
                aMessage.Read(aMessage.GetOffset(), sizeof(udp), &udp);

                // do not pass MLE messages
                VerifyOrExit(udp.GetDestinationPort() != Mle::kUdpPort, ;);
            }

            break;

        default:
            break;
        }
    }

    // make a copy of the datagram to pass to host
    VerifyOrExit((messageCopy = NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = messageCopy->SetLength(aMessage.GetLength()));
    aMessage.CopyTo(0, 0, aMessage.GetLength(), *messageCopy);
    messageCopy->SetLinkSecurityEnabled(aMessage.IsLinkSecurityEnabled());

    mReceiveIp6DatagramCallback(messageCopy, mReceiveIp6DatagramCallbackContext);

exit:

    if (error != kThreadError_None && messageCopy != NULL)
    {
        messageCopy->Free();
    }
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
    uint8_t nextHeader;
    uint8_t hopLimit;

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

    if (!mForwardingEnabled && netif != NULL)
    {
        forward = false;
    }

    message.SetOffset(sizeof(header));

    // process IPv6 Extension Headers
    nextHeader = header.GetNextHeader();
    SuccessOrExit(error = HandleExtensionHeaders(message, nextHeader, receive));

    // process IPv6 Payload
    if (receive)
    {
        if (fromLocalHost == false)
        {
            ProcessReceiveCallback(message, messageInfo, nextHeader);
        }

        SuccessOrExit(error = HandlePayload(message, messageInfo, nextHeader));
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
            SuccessOrExit(error = ForwardMessage(message, messageInfo));
        }
    }

exit:

    if (error != kThreadError_None || !forward)
    {
        message.Free();
    }

    return error;
}

ThreadError Ip6::ForwardMessage(Message &message, MessageInfo &messageInfo)
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
        otDumpDebgIp6("no route", &messageInfo.GetSockAddr(), 16);
        ExitNow(error = kThreadError_NoRoute);
    }

    // submit message to interface
    VerifyOrExit((netif = GetNetifById(interfaceId)) != NULL, error = kThreadError_NoRoute);
    SuccessOrExit(error = netif->SendMessage(message));

exit:
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
            if (netif == &aNetif)
            {
                ExitNow(error = kThreadError_Busy);
            }
        }
        while (netif->mNext);

        netif->mNext = &aNetif;
    }

    aNetif.mNext = NULL;

    if (aNetif.mInterfaceId < 0)
    {
        aNetif.mInterfaceId = mNextInterfaceId++;
    }

exit:
    return error;
}

ThreadError Ip6::RemoveNetif(Netif &aNetif)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mNetifListHead != NULL, error = kThreadError_Busy);

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

Netif *Ip6::GetNetifByName(char *aName)
{
    Netif *netif;

    for (netif = mNetifListHead; netif; netif = netif->mNext)
    {
        if (strcmp(netif->GetName(), aName) == 0)
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
