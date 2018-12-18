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

#include "ip6.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/owner-locator.hpp"
#include "net/icmp6.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_routes.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Ip6 {

Ip6::Ip6(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mForwardingEnabled(false)
    , mIsReceiveIp6FilterEnabled(false)
    , mReceiveIp6DatagramCallback(NULL)
    , mReceiveIp6DatagramCallbackContext(NULL)
    , mNetifListHead(NULL)
    , mSendQueue()
    , mSendQueueTask(aInstance, HandleSendQueue, this)
    , mRoutes(aInstance)
    , mIcmp(aInstance)
    , mUdp(aInstance)
    , mMpl(aInstance)
{
}

Message *Ip6::NewMessage(uint16_t aReserved, const otMessageSettings *aSettings)
{
    return GetInstance().GetMessagePool().New(
        Message::kTypeIp6, sizeof(Header) + sizeof(HopByHopHeader) + sizeof(OptionMpl) + aReserved, aSettings);
}

Message *Ip6::NewMessage(const uint8_t *aData, uint16_t aDataLength, const otMessageSettings *aSettings)
{
    otMessageSettings settings = {true, OT_MESSAGE_PRIORITY_NORMAL};
    Message *         message  = NULL;

    if (aSettings != NULL)
    {
        settings = *aSettings;
    }

    SuccessOrExit(GetDatagramPriority(aData, aDataLength, *reinterpret_cast<uint8_t *>(&settings.mPriority)));
    VerifyOrExit((message = GetInstance().GetMessagePool().New(Message::kTypeIp6, 0, &settings)) != NULL);

    if (message->Append(aData, aDataLength) != OT_ERROR_NONE)
    {
        message->Free();
        message = NULL;
    }

exit:
    return message;
}

uint8_t Ip6::DscpToPriority(uint8_t aDscp)
{
    uint8_t priority;
    uint8_t cs = aDscp & kDscpCsMask;

    switch (cs)
    {
    case kDscpCs1:
    case kDscpCs2:
        priority = Message::kPriorityLow;
        break;

    case kDscpCs0:
    case kDscpCs3:
        priority = Message::kPriorityNormal;
        break;

    case kDscpCs4:
    case kDscpCs5:
    case kDscpCs6:
    case kDscpCs7:
        priority = Message::kPriorityHigh;
        break;

    default:
        priority = Message::kPriorityNormal;
        break;
    }

    return priority;
}

uint8_t Ip6::PriorityToDscp(uint8_t aPriority)
{
    uint8_t dscp = kDscpCs0;

    switch (aPriority)
    {
    case Message::kPriorityLow:
        dscp = kDscpCs1;
        break;

    case Message::kPriorityNormal:
        dscp = kDscpCs0;
        break;

    case Message::kPriorityHigh:
        dscp = kDscpCs4;
        break;
    }

    return dscp;
}

otError Ip6::GetDatagramPriority(const uint8_t *aData, uint16_t aDataLen, uint8_t &aPriority)
{
    otError       error = OT_ERROR_NONE;
    const Header *header;

    VerifyOrExit((aData != NULL) && (aDataLen >= sizeof(Header)), error = OT_ERROR_INVALID_ARGS);

    header = reinterpret_cast<const Header *>(aData);
    VerifyOrExit(header->IsValid(), error = OT_ERROR_PARSE);
    VerifyOrExit(sizeof(Header) + header->GetPayloadLength() == aDataLen, error = OT_ERROR_PARSE);

    aPriority = DscpToPriority(header->GetDscp());

exit:
    return error;
}

uint16_t Ip6::UpdateChecksum(uint16_t aChecksum, const Address &aAddress)
{
    return Message::UpdateChecksum(aChecksum, aAddress.mFields.m8, sizeof(aAddress));
}

uint16_t Ip6::ComputePseudoheaderChecksum(const Address &aSource,
                                          const Address &aDestination,
                                          uint16_t       aLength,
                                          IpProto        aProto)
{
    uint16_t checksum;

    checksum = Message::UpdateChecksum(0, aLength);
    checksum = Message::UpdateChecksum(checksum, static_cast<uint16_t>(aProto));
    checksum = UpdateChecksum(checksum, aSource);
    checksum = UpdateChecksum(checksum, aDestination);

    return checksum;
}

void Ip6::SetReceiveDatagramCallback(otIp6ReceiveCallback aCallback, void *aCallbackContext)
{
    mReceiveIp6DatagramCallback        = aCallback;
    mReceiveIp6DatagramCallbackContext = aCallbackContext;
}

otError Ip6::AddMplOption(Message &aMessage, Header &aHeader)
{
    otError        error = OT_ERROR_NONE;
    HopByHopHeader hbhHeader;
    OptionMpl      mplOption;
    OptionPadN     padOption;

    hbhHeader.SetNextHeader(aHeader.GetNextHeader());
    hbhHeader.SetLength(0);
    mMpl.InitOption(mplOption, aHeader.GetSource());

    // Mpl option may require two bytes padding.
    if ((mplOption.GetTotalLength() + sizeof(hbhHeader)) % 8)
    {
        padOption.Init(2);
        SuccessOrExit(error = aMessage.Prepend(&padOption, padOption.GetTotalLength()));
    }

    SuccessOrExit(error = aMessage.Prepend(&mplOption, mplOption.GetTotalLength()));
    SuccessOrExit(error = aMessage.Prepend(&hbhHeader, sizeof(hbhHeader)));
    aHeader.SetPayloadLength(aHeader.GetPayloadLength() + sizeof(hbhHeader) + sizeof(mplOption));
    aHeader.SetNextHeader(kProtoHopOpts);
exit:
    return error;
}

otError Ip6::AddTunneledMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo)
{
    otError                    error = OT_ERROR_NONE;
    Header                     tunnelHeader;
    const NetifUnicastAddress *source;
    MessageInfo                messageInfo(aMessageInfo);

    // Use IP-in-IP encapsulation (RFC2473) and ALL_MPL_FORWARDERS address.
    memset(&messageInfo.GetPeerAddr(), 0, sizeof(Address));
    messageInfo.GetPeerAddr().mFields.m16[0] = HostSwap16(0xff03);
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(0x00fc);

    tunnelHeader.Init();
    tunnelHeader.SetHopLimit(static_cast<uint8_t>(kDefaultHopLimit));
    tunnelHeader.SetPayloadLength(aHeader.GetPayloadLength() + sizeof(tunnelHeader));
    tunnelHeader.SetDestination(messageInfo.GetPeerAddr());
    tunnelHeader.SetNextHeader(kProtoIp6);

    VerifyOrExit((source = SelectSourceAddress(messageInfo)) != NULL, error = OT_ERROR_INVALID_SOURCE_ADDRESS);

    tunnelHeader.SetSource(source->GetAddress());

    SuccessOrExit(error = AddMplOption(aMessage, tunnelHeader));
    SuccessOrExit(error = aMessage.Prepend(&tunnelHeader, sizeof(tunnelHeader)));

exit:
    return error;
}

otError Ip6::InsertMplOption(Message &aMessage, Header &aIp6Header, MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aIp6Header.GetDestination().IsMulticast() &&
                 aIp6Header.GetDestination().GetScope() >= Address::kRealmLocalScope);

    if (aIp6Header.GetDestination().IsRealmLocalMulticast())
    {
        aMessage.RemoveHeader(sizeof(aIp6Header));

        if (aIp6Header.GetNextHeader() == kProtoHopOpts)
        {
            HopByHopHeader hbh;
            uint16_t       hbhLength = 0;
            OptionMpl      mplOption;

            // read existing hop-by-hop option header
            aMessage.Read(0, sizeof(hbh), &hbh);
            hbhLength = (hbh.GetLength() + 1) * 8;

            VerifyOrExit(hbhLength <= aIp6Header.GetPayloadLength(), error = OT_ERROR_PARSE);

            // increase existing hop-by-hop option header length by 8 bytes
            hbh.SetLength(hbh.GetLength() + 1);
            aMessage.Write(0, sizeof(hbh), &hbh);

            // make space for MPL Option + padding by shifting hop-by-hop option header
            SuccessOrExit(error = aMessage.Prepend(NULL, 8));
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
        if (aIp6Header.GetDestination().IsMulticastLargerThanRealmLocal() &&
            GetInstance().GetThreadNetif().GetMle().HasSleepyChildrenSubscribed(aIp6Header.GetDestination()))
        {
            Message *messageCopy = NULL;

            if ((messageCopy = aMessage.Clone()) != NULL)
            {
                HandleDatagram(*messageCopy, NULL, aMessageInfo.GetInterfaceId(), NULL, true);
                otLogInfoIp6("Message copy for indirect transmission to sleepy children");
            }
            else
            {
                otLogWarnIp6("No enough buffer for message copy for indirect transmission to sleepy children");
            }
        }

        SuccessOrExit(error = AddTunneledMplOption(aMessage, aIp6Header, aMessageInfo));
    }

exit:
    return error;
}

otError Ip6::RemoveMplOption(Message &aMessage)
{
    otError        error = OT_ERROR_NONE;
    Header         ip6Header;
    HopByHopHeader hbh;
    uint16_t       offset;
    uint16_t       endOffset;
    uint16_t       mplOffset = 0;
    uint8_t        mplLength = 0;
    bool           remove    = false;

    offset = 0;
    aMessage.Read(offset, sizeof(ip6Header), &ip6Header);
    offset += sizeof(ip6Header);
    VerifyOrExit(ip6Header.GetNextHeader() == kProtoHopOpts);

    aMessage.Read(offset, sizeof(hbh), &hbh);
    endOffset = offset + (hbh.GetLength() + 1) * 8;
    VerifyOrExit(aMessage.GetLength() >= endOffset, error = OT_ERROR_PARSE);

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
    VerifyOrExit(offset == endOffset, error = OT_ERROR_PARSE);

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

otError Ip6::SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, IpProto aIpProto)
{
    otError                    error = OT_ERROR_NONE;
    Header                     header;
    uint16_t                   payloadLength = aMessage.GetLength();
    uint16_t                   checksum;
    const NetifUnicastAddress *source;

    header.Init();
    header.SetDscp(PriorityToDscp(aMessage.GetPriority()));
    header.SetPayloadLength(payloadLength);
    header.SetNextHeader(aIpProto);
    header.SetHopLimit(aMessageInfo.mHopLimit ? aMessageInfo.mHopLimit : static_cast<uint8_t>(kDefaultHopLimit));

    if (aMessageInfo.GetSockAddr().IsUnspecified() || aMessageInfo.GetSockAddr().IsMulticast())
    {
        VerifyOrExit((source = SelectSourceAddress(aMessageInfo)) != NULL, error = OT_ERROR_INVALID_SOURCE_ADDRESS);
        header.SetSource(source->GetAddress());
    }
    else
    {
        header.SetSource(aMessageInfo.GetSockAddr());
    }

    header.SetDestination(aMessageInfo.GetPeerAddr());

    if (header.GetDestination().IsLinkLocal() || header.GetDestination().IsLinkLocalMulticast())
    {
        VerifyOrExit(aMessageInfo.GetInterfaceId() != 0, error = OT_ERROR_DROP);
    }

    if (aMessageInfo.GetPeerAddr().IsRealmLocalMulticast())
    {
        SuccessOrExit(error = AddMplOption(aMessage, header));
    }

    SuccessOrExit(error = aMessage.Prepend(&header, sizeof(header)));

    // compute checksum
    checksum = ComputePseudoheaderChecksum(header.GetSource(), header.GetDestination(), payloadLength, aIpProto);

    switch (aIpProto)
    {
    case kProtoUdp:
        SuccessOrExit(error = mUdp.UpdateChecksum(aMessage, checksum));
        break;

    case kProtoIcmp6:
        SuccessOrExit(error = mIcmp.UpdateChecksum(aMessage, checksum));
        break;

    default:
        break;
    }

    if (aMessageInfo.GetPeerAddr().IsMulticastLargerThanRealmLocal())
    {
        if (GetInstance().GetThreadNetif().GetMle().HasSleepyChildrenSubscribed(header.GetDestination()))
        {
            Message *messageCopy = NULL;

            if ((messageCopy = aMessage.Clone()) != NULL)
            {
                otLogInfoIp6("Message copy for indirect transmission to sleepy children");
                messageCopy->SetInterfaceId(aMessageInfo.GetInterfaceId());
                EnqueueDatagram(*messageCopy);
            }
            else
            {
                otLogWarnIp6("No enough buffer for message copy for indirect transmission to sleepy children");
            }
        }

        SuccessOrExit(error = AddTunneledMplOption(aMessage, header, aMessageInfo));
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        aMessage.SetInterfaceId(aMessageInfo.GetInterfaceId());
        EnqueueDatagram(aMessage);
    }

    return error;
}

void Ip6::HandleSendQueue(Tasklet &aTasklet)
{
    aTasklet.GetOwner<Ip6>().HandleSendQueue();
}

void Ip6::HandleSendQueue(void)
{
    Message *message;

    while ((message = mSendQueue.GetHead()) != NULL)
    {
        mSendQueue.Dequeue(*message);
        HandleDatagram(*message, NULL, message->GetInterfaceId(), NULL, false);
    }
}

otError Ip6::HandleOptions(Message &aMessage, Header &aHeader, bool &aForward)
{
    otError        error = OT_ERROR_NONE;
    HopByHopHeader hbhHeader;
    OptionHeader   optionHeader;
    uint16_t       endOffset;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(hbhHeader), &hbhHeader) == sizeof(hbhHeader),
                 error = OT_ERROR_DROP);
    endOffset = aMessage.GetOffset() + (hbhHeader.GetLength() + 1) * 8;

    VerifyOrExit(endOffset <= aMessage.GetLength(), error = OT_ERROR_DROP);

    aMessage.MoveOffset(sizeof(optionHeader));

    while (aMessage.GetOffset() < endOffset)
    {
        VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(optionHeader), &optionHeader) == sizeof(optionHeader),
                     error = OT_ERROR_DROP);

        if (optionHeader.GetType() == OptionPad1::kType)
        {
            aMessage.MoveOffset(sizeof(OptionPad1));
            continue;
        }

        VerifyOrExit(aMessage.GetOffset() + sizeof(optionHeader) + optionHeader.GetLength() <= endOffset,
                     error = OT_ERROR_DROP);

        switch (optionHeader.GetType())
        {
        case OptionMpl::kType:
            SuccessOrExit(error = mMpl.ProcessOption(aMessage, aHeader.GetSource(), aForward));
            break;

        default:
            switch (optionHeader.GetAction())
            {
            case OptionHeader::kActionSkip:
                break;

            case OptionHeader::kActionDiscard:
                ExitNow(error = OT_ERROR_DROP);

            case OptionHeader::kActionForceIcmp:
                // TODO: send icmp error
                ExitNow(error = OT_ERROR_DROP);

            case OptionHeader::kActionIcmp:
                // TODO: send icmp error
                ExitNow(error = OT_ERROR_DROP);
            }

            break;
        }

        aMessage.MoveOffset(sizeof(optionHeader) + optionHeader.GetLength());
    }

exit:
    return error;
}

otError Ip6::HandleFragment(Message &aMessage)
{
    otError        error = OT_ERROR_NONE;
    FragmentHeader fragmentHeader;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(fragmentHeader), &fragmentHeader) == sizeof(fragmentHeader),
                 error = OT_ERROR_DROP);

    VerifyOrExit(fragmentHeader.GetOffset() == 0 && fragmentHeader.IsMoreFlagSet() == false, error = OT_ERROR_DROP);

    aMessage.MoveOffset(sizeof(fragmentHeader));

exit:
    return error;
}

otError Ip6::HandleExtensionHeaders(Message &aMessage,
                                    Header & aHeader,
                                    uint8_t &aNextHeader,
                                    bool     aForward,
                                    bool     aReceive)
{
    otError         error = OT_ERROR_NONE;
    ExtensionHeader extHeader;

    while (aReceive == true || aNextHeader == kProtoHopOpts)
    {
        VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(extHeader), &extHeader) == sizeof(extHeader),
                     error = OT_ERROR_DROP);

        switch (aNextHeader)
        {
        case kProtoHopOpts:
            SuccessOrExit(error = HandleOptions(aMessage, aHeader, aForward));
            break;

        case kProtoFragment:
            SuccessOrExit(error = HandleFragment(aMessage));
            break;

        case kProtoDstOpts:
            SuccessOrExit(error = HandleOptions(aMessage, aHeader, aForward));
            break;

        case kProtoIp6:
            ExitNow();

        case kProtoRouting:
        case kProtoNone:
            ExitNow(error = OT_ERROR_DROP);

        default:
            ExitNow();
        }

        aNextHeader = static_cast<uint8_t>(extHeader.GetNextHeader());
    }

exit:
    return error;
}

otError Ip6::HandlePayload(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto)
{
    otError error = OT_ERROR_NONE;

    switch (aIpProto)
    {
    case kProtoUdp:
        ExitNow(error = mUdp.HandleMessage(aMessage, aMessageInfo));

    case kProtoIcmp6:
        ExitNow(error = mIcmp.HandleMessage(aMessage, aMessageInfo));
    }

exit:
    return error;
}

otError Ip6::ProcessReceiveCallback(const Message &    aMessage,
                                    const MessageInfo &aMessageInfo,
                                    uint8_t            aIpProto,
                                    bool               aFromNcpHost)
{
    otError  error       = OT_ERROR_NONE;
    Message *messageCopy = NULL;

    VerifyOrExit(aFromNcpHost == false, error = OT_ERROR_DROP);
    VerifyOrExit(mReceiveIp6DatagramCallback != NULL, error = OT_ERROR_NO_ROUTE);

    if (mIsReceiveIp6FilterEnabled)
    {
        // do not pass messages sent to an RLOC/ALOC
        VerifyOrExit(!aMessageInfo.GetSockAddr().IsRoutingLocator() &&
                         !aMessageInfo.GetSockAddr().IsAnycastRoutingLocator(),
                     error = OT_ERROR_NO_ROUTE);

        switch (aIpProto)
        {
        case kProtoIcmp6:
            if (mIcmp.ShouldHandleEchoRequest(aMessageInfo))
            {
                IcmpHeader icmp;
                aMessage.Read(aMessage.GetOffset(), sizeof(icmp), &icmp);

                // do not pass ICMP Echo Request messages
                VerifyOrExit(icmp.GetType() != IcmpHeader::kTypeEchoRequest, error = OT_ERROR_NO_ROUTE);
            }

            break;

        case kProtoUdp:
        {
            UdpHeader udp;
            aMessage.Read(aMessage.GetOffset(), sizeof(udp), &udp);

            switch (udp.GetDestinationPort())
            {
            case Mle::kUdpPort:

                // do not pass MLE messages
                if (aMessageInfo.GetSockAddr().IsLinkLocal() || aMessageInfo.GetSockAddr().IsLinkLocalMulticast())
                {
                    ExitNow(error = OT_ERROR_NO_ROUTE);
                }

                break;

#if OPENTHREAD_ENABLE_PLATFORM_UDP == 0
            case kCoapUdpPort:

                // do not pass TMF messages
                if (GetInstance().GetThreadNetif().IsTmfMessage(aMessageInfo))
                {
                    ExitNow(error = OT_ERROR_NO_ROUTE);
                }

                break;
#endif // OPENTHREAD_ENABLE_PLATFORM_UDP

            default:
#if OPENTHREAD_FTD
                if (udp.GetDestinationPort() == GetInstance().Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort())
                {
                    ExitNow(error = OT_ERROR_NO_ROUTE);
                }
#endif
                break;
            }

            break;
        }

        default:
            break;
        }
    }

    // make a copy of the datagram to pass to host
    VerifyOrExit((messageCopy = aMessage.Clone()) != NULL, error = OT_ERROR_NO_BUFS);
    RemoveMplOption(*messageCopy);
    mReceiveIp6DatagramCallback(messageCopy, mReceiveIp6DatagramCallbackContext);

exit:

    switch (error)
    {
    case OT_ERROR_NO_BUFS:
        otLogWarnIp6("Failed to pass up message (len: %d) to host - out of message buffer.", aMessage.GetLength());
        break;

    case OT_ERROR_DROP:
        otLogNoteIp6("Dropping message (len: %d) from local host since next hop is the host.", aMessage.GetLength());
        break;

    default:
        break;
    }

    return error;
}

otError Ip6::SendRaw(Message &aMessage, int8_t aInterfaceId)
{
    otError     error = OT_ERROR_NONE;
    Header      header;
    MessageInfo messageInfo;
    bool        freed = false;

    SuccessOrExit(error = header.Init(aMessage));

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetInterfaceId(aInterfaceId);
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetLinkInfo(NULL);

    if (header.GetDestination().IsMulticast())
    {
        SuccessOrExit(error = InsertMplOption(aMessage, header, messageInfo));
    }

    error = HandleDatagram(aMessage, NULL, aInterfaceId, NULL, true);
    freed = true;

exit:

    if (!freed)
    {
        aMessage.Free();
    }

    return error;
}

otError Ip6::HandleDatagram(Message &   aMessage,
                            Netif *     aNetif,
                            int8_t      aInterfaceId,
                            const void *aLinkMessageInfo,
                            bool        aFromNcpHost)
{
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfo;
    Header      header;
    bool        receive              = false;
    bool        forward              = false;
    bool        tunnel               = false;
    bool        multicastPromiscuous = false;
    uint8_t     nextHeader;
    uint8_t     hopLimit;
    int8_t      forwardInterfaceId;

    SuccessOrExit(error = header.Init(aMessage));

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetInterfaceId(aInterfaceId);
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetLinkInfo(aLinkMessageInfo);

    // determine destination of packet
    if (header.GetDestination().IsMulticast())
    {
        if (aNetif != NULL)
        {
            if (aNetif->IsMulticastSubscribed(header.GetDestination()))
            {
                receive = true;
            }
            else if (aNetif->IsMulticastPromiscuousEnabled())
            {
                multicastPromiscuous = true;
            }

            if (header.GetDestination().IsMulticastLargerThanRealmLocal() &&
                GetInstance().GetThreadNetif().GetMle().HasSleepyChildrenSubscribed(header.GetDestination()))
            {
                forward = true;
            }
        }
        else
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
        else if (aNetif == NULL)
        {
            forward = true;
        }
    }

    aMessage.SetInterfaceId(aInterfaceId);
    aMessage.SetOffset(sizeof(header));

    // process IPv6 Extension Headers
    nextHeader = static_cast<uint8_t>(header.GetNextHeader());
    SuccessOrExit(error = HandleExtensionHeaders(aMessage, header, nextHeader, forward, receive));

    // process IPv6 Payload
    if (receive)
    {
        if (nextHeader == kProtoIp6)
        {
            // Remove encapsulating header.
            aMessage.RemoveHeader(aMessage.GetOffset());

            HandleDatagram(aMessage, aNetif, aInterfaceId, aLinkMessageInfo, aFromNcpHost);
            ExitNow(tunnel = true);
        }

        ProcessReceiveCallback(aMessage, messageInfo, nextHeader, aFromNcpHost);

        SuccessOrExit(error = HandlePayload(aMessage, messageInfo, nextHeader));
    }
    else if (multicastPromiscuous)
    {
        ProcessReceiveCallback(aMessage, messageInfo, nextHeader, aFromNcpHost);
    }

    if (forward)
    {
        forwardInterfaceId = FindForwardInterfaceId(messageInfo);

        if (forwardInterfaceId == 0)
        {
            // try passing to host
            SuccessOrExit(error = ProcessReceiveCallback(aMessage, messageInfo, nextHeader, aFromNcpHost));

            // the caller transfers custody in the success case, so free the aMessage here
            aMessage.Free();

            ExitNow();
        }

        if (aNetif != NULL)
        {
            VerifyOrExit(mForwardingEnabled, forward = false);
            header.SetHopLimit(header.GetHopLimit() - 1);
        }

        if (header.GetHopLimit() == 0)
        {
            // send time exceeded
            ExitNow(error = OT_ERROR_DROP);
        }
        else
        {
            hopLimit = header.GetHopLimit();
            aMessage.Write(Header::GetHopLimitOffset(), Header::GetHopLimitSize(), &hopLimit);

            // submit aMessage to interface
            VerifyOrExit((aNetif = GetNetifById(forwardInterfaceId)) != NULL, error = OT_ERROR_NO_ROUTE);
            SuccessOrExit(error = aNetif->SendMessage(aMessage));
        }
    }

exit:

    if (!tunnel && (error != OT_ERROR_NONE || !forward))
    {
        aMessage.Free();
    }

    return error;
}

int8_t Ip6::FindForwardInterfaceId(const MessageInfo &aMessageInfo)
{
    int8_t interfaceId;

    if (aMessageInfo.GetSockAddr().IsMulticast())
    {
        // multicast
        interfaceId = aMessageInfo.mInterfaceId;
    }
    else if (aMessageInfo.GetSockAddr().IsLinkLocal())
    {
        // on-link link-local address
        interfaceId = aMessageInfo.mInterfaceId;
    }
    else if ((interfaceId = GetOnLinkNetif(aMessageInfo.GetSockAddr())) > 0)
    {
        // on-link global address
        ;
    }
    else if ((interfaceId = mRoutes.Lookup(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr())) > 0)
    {
        // route
        ;
    }
    else
    {
        interfaceId = 0;
    }

    return interfaceId;
}

otError Ip6::AddNetif(Netif &aNetif)
{
    otError error = OT_ERROR_NONE;
    Netif * netif;

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
                ExitNow(error = OT_ERROR_ALREADY);
            }
        } while (netif->mNext);

        netif->mNext = &aNetif;
    }

    aNetif.mNext = NULL;

exit:
    return error;
}

otError Ip6::RemoveNetif(Netif &aNetif)
{
    otError error = OT_ERROR_NOT_FOUND;

    VerifyOrExit(mNetifListHead != NULL, error = OT_ERROR_NOT_FOUND);

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
            error        = OT_ERROR_NONE;
            break;
        }
    }

    aNetif.mNext = NULL;

exit:
    return error;
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
    Address *                  destination = &aMessageInfo.GetPeerAddr();
    int                        interfaceId = aMessageInfo.mInterfaceId;
    const NetifUnicastAddress *rvalAddr    = NULL;
    const Address *            candidateAddr;
    int8_t                     candidateId;
    int8_t                     rvalIface         = 0;
    uint8_t                    rvalPrefixMatched = 0;
    uint8_t                    destinationScope  = destination->GetScope();

    for (Netif *netif = GetNetifList(); netif; netif = netif->mNext)
    {
        candidateId = netif->GetInterfaceId();

        if (destination->IsLinkLocal() || destination->IsMulticast())
        {
            if (interfaceId != candidateId)
            {
                continue;
            }
        }

        for (const NetifUnicastAddress *addr = netif->mUnicastAddresses; addr; addr = addr->GetNext())
        {
            uint8_t overrideScope;
            uint8_t candidatePrefixMatched;

            candidateAddr          = &addr->GetAddress();
            candidatePrefixMatched = destination->PrefixMatch(*candidateAddr);
            overrideScope = (candidatePrefixMatched >= addr->mPrefixLength) ? addr->GetScope() : destinationScope;

            if (candidateAddr->IsAnycastRoutingLocator())
            {
                // Don't use anycast address as source address.
                continue;
            }

            if (rvalAddr == NULL)
            {
                // Rule 0: Prefer any address
                rvalAddr          = addr;
                rvalIface         = candidateId;
                rvalPrefixMatched = candidatePrefixMatched;
            }
            else if (*candidateAddr == *destination)
            {
                // Rule 1: Prefer same address
                rvalAddr  = addr;
                rvalIface = candidateId;
                ExitNow();
            }
            else if (addr->GetScope() < rvalAddr->GetScope())
            {
                // Rule 2: Prefer appropriate scope
                if (addr->GetScope() >= overrideScope)
                {
                    rvalAddr          = addr;
                    rvalIface         = candidateId;
                    rvalPrefixMatched = candidatePrefixMatched;
                }
            }
            else if (addr->GetScope() > rvalAddr->GetScope())
            {
                if (rvalAddr->GetScope() < overrideScope)
                {
                    rvalAddr          = addr;
                    rvalIface         = candidateId;
                    rvalPrefixMatched = candidatePrefixMatched;
                }
            }
            else if ((rvalAddr->GetScope() == Address::kRealmLocalScope) &&
                     (addr->GetScope() == Address::kRealmLocalScope))
            {
                // Additional rule: Prefer EID
                if (rvalAddr->GetAddress().IsRoutingLocator())
                {
                    rvalAddr          = addr;
                    rvalIface         = candidateId;
                    rvalPrefixMatched = candidatePrefixMatched;
                }
            }
            else if (addr->mPreferred && !rvalAddr->mPreferred)
            {
                // Rule 3: Avoid deprecated addresses
                rvalAddr          = addr;
                rvalIface         = candidateId;
                rvalPrefixMatched = candidatePrefixMatched;
            }
            else if (aMessageInfo.mInterfaceId != 0 && aMessageInfo.mInterfaceId == candidateId &&
                     rvalIface != candidateId)
            {
                // Rule 4: Prefer home address
                // Rule 5: Prefer outgoing interface
                rvalAddr          = addr;
                rvalIface         = candidateId;
                rvalPrefixMatched = candidatePrefixMatched;
            }
            else if (candidatePrefixMatched > rvalPrefixMatched)
            {
                // Rule 6: Prefer matching label
                // Rule 7: Prefer public address
                // Rule 8: Use longest prefix matching
                rvalAddr          = addr;
                rvalIface         = candidateId;
                rvalPrefixMatched = candidatePrefixMatched;
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

const char *Ip6::IpProtoToString(IpProto aIpProto)
{
    const char *retval;

    switch (aIpProto)
    {
    case kProtoHopOpts:
        retval = "HopOpts";
        break;

    case kProtoTcp:
        retval = "TCP";
        break;

    case kProtoUdp:
        retval = "UDP";
        break;

    case kProtoIp6:
        retval = "IP6";
        break;

    case kProtoRouting:
        retval = "Routing";
        break;

    case kProtoFragment:
        retval = "Frag";
        break;

    case kProtoIcmp6:
        retval = "ICMP6";
        break;

    case kProtoNone:
        retval = "None";
        break;

    case kProtoDstOpts:
        retval = "DstOpts";
        break;

    default:
        retval = "Unknown";
        break;
    }

    return retval;
}

} // namespace Ip6
} // namespace ot
