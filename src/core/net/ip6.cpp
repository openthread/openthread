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

#include "ip6.hpp"

#include "backbone_router/bbr_leader.hpp"
#include "backbone_router/bbr_local.hpp"
#include "backbone_router/ndproxy_table.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "instance/instance.hpp"
#include "net/checksum.hpp"
#include "net/icmp6.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_filter.hpp"
#include "net/nat64_translator.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "openthread/ip6.h"
#include "thread/mle.hpp"

using IcmpType = ot::Ip6::Icmp::Header::Type;

static const IcmpType kForwardIcmpTypes[] = {
    IcmpType::kTypeDstUnreach,       IcmpType::kTypePacketToBig, IcmpType::kTypeTimeExceeded,
    IcmpType::kTypeParameterProblem, IcmpType::kTypeEchoRequest, IcmpType::kTypeEchoReply,
};

namespace ot {
namespace Ip6 {

RegisterLogModule("Ip6");

Ip6::Ip6(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsReceiveIp6FilterEnabled(false)
    , mSendQueueTask(aInstance)
    , mIcmp(aInstance)
    , mUdp(aInstance)
    , mMpl(aInstance)
#if OPENTHREAD_CONFIG_TCP_ENABLE
    , mTcp(aInstance)
#endif
{
#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    ResetBorderRoutingCounters();
#endif
}

Message *Ip6::NewMessage(void) { return NewMessage(0); }

Message *Ip6::NewMessage(uint16_t aReserved) { return NewMessage(aReserved, Message::Settings::GetDefault()); }

Message *Ip6::NewMessage(uint16_t aReserved, const Message::Settings &aSettings)
{
    return Get<MessagePool>().Allocate(
        Message::kTypeIp6, sizeof(Header) + sizeof(HopByHopHeader) + sizeof(MplOption) + aReserved, aSettings);
}

Message *Ip6::NewMessageFromData(const uint8_t *aData, uint16_t aDataLength, const Message::Settings &aSettings)
{
    Message          *message  = nullptr;
    Message::Settings settings = aSettings;
    const Header     *header;

    VerifyOrExit((aData != nullptr) && (aDataLength >= sizeof(Header)));

    // Determine priority from IPv6 header
    header = reinterpret_cast<const Header *>(aData);
    VerifyOrExit(header->IsValid());
    VerifyOrExit(sizeof(Header) + header->GetPayloadLength() == aDataLength);
    settings.mPriority = DscpToPriority(header->GetDscp());

    message = Get<MessagePool>().Allocate(Message::kTypeIp6, /* aReserveHeader */ 0, settings);

    VerifyOrExit(message != nullptr);

    if (message->AppendBytes(aData, aDataLength) != kErrorNone)
    {
        message->Free();
        message = nullptr;
    }

exit:
    return message;
}

Message::Priority Ip6::DscpToPriority(uint8_t aDscp)
{
    Message::Priority priority;
    uint8_t           cs = aDscp & kDscpCsMask;

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

uint8_t Ip6::PriorityToDscp(Message::Priority aPriority)
{
    uint8_t dscp = kDscpCs0;

    switch (aPriority)
    {
    case Message::kPriorityLow:
        dscp = kDscpCs1;
        break;

    case Message::kPriorityNormal:
    case Message::kPriorityNet:
        dscp = kDscpCs0;
        break;

    case Message::kPriorityHigh:
        dscp = kDscpCs4;
        break;
    }

    return dscp;
}

Error Ip6::AddMplOption(Message &aMessage, Header &aHeader)
{
    Error          error = kErrorNone;
    HopByHopHeader hbhHeader;
    MplOption      mplOption;
    PadOption      padOption;

    hbhHeader.SetNextHeader(aHeader.GetNextHeader());
    hbhHeader.SetLength(0);
    mMpl.InitOption(mplOption, aHeader.GetSource());

    // Check if MPL option may require padding
    if (padOption.InitToPadHeaderWithSize(sizeof(HopByHopHeader) + mplOption.GetSize()) == kErrorNone)
    {
        SuccessOrExit(error = aMessage.PrependBytes(&padOption, padOption.GetSize()));
    }

    SuccessOrExit(error = aMessage.PrependBytes(&mplOption, mplOption.GetSize()));
    SuccessOrExit(error = aMessage.Prepend(hbhHeader));
    aHeader.SetPayloadLength(aHeader.GetPayloadLength() + sizeof(hbhHeader) + sizeof(mplOption));
    aHeader.SetNextHeader(kProtoHopOpts);

exit:
    return error;
}

Error Ip6::PrepareMulticastToLargerThanRealmLocal(Message &aMessage, const Header &aHeader)
{
    Error          error = kErrorNone;
    Header         tunnelHeader;
    const Address *source;

#if OPENTHREAD_FTD
    if (aHeader.GetDestination().IsMulticastLargerThanRealmLocal() &&
        Get<ChildTable>().HasSleepyChildWithAddress(aHeader.GetDestination()))
    {
        Message *messageCopy = aMessage.Clone();

        if (messageCopy != nullptr)
        {
            EnqueueDatagram(*messageCopy);
        }
        else
        {
            LogWarn("Failed to clone mcast message for indirect tx to sleepy children");
        }
    }
#endif

    // Use IP-in-IP encapsulation (RFC2473) and ALL_MPL_FORWARDERS address.
    tunnelHeader.InitVersionTrafficClassFlow();
    tunnelHeader.SetHopLimit(static_cast<uint8_t>(kDefaultHopLimit));
    tunnelHeader.SetPayloadLength(aHeader.GetPayloadLength() + sizeof(tunnelHeader));
    tunnelHeader.GetDestination().SetToRealmLocalAllMplForwarders();
    tunnelHeader.SetNextHeader(kProtoIp6);

    source = SelectSourceAddress(tunnelHeader.GetDestination());
    VerifyOrExit(source != nullptr, error = kErrorInvalidSourceAddress);

    tunnelHeader.SetSource(*source);

    SuccessOrExit(error = AddMplOption(aMessage, tunnelHeader));
    SuccessOrExit(error = aMessage.Prepend(tunnelHeader));

exit:
    return error;
}

Error Ip6::InsertMplOption(Message &aMessage, Header &aHeader)
{
    Error error = kErrorNone;

    VerifyOrExit(aHeader.GetDestination().IsMulticast() &&
                 aHeader.GetDestination().GetScope() >= Address::kRealmLocalScope);

    if (aHeader.GetDestination().IsRealmLocalMulticast())
    {
        aMessage.RemoveHeader(sizeof(aHeader));

        if (aHeader.GetNextHeader() == kProtoHopOpts)
        {
            HopByHopHeader hbh;
            uint16_t       hbhSize;
            MplOption      mplOption;
            PadOption      padOption;

            // Read existing hop-by-hop option header
            SuccessOrExit(error = aMessage.Read(0, hbh));
            hbhSize = hbh.GetSize();

            VerifyOrExit(hbhSize <= aHeader.GetPayloadLength(), error = kErrorParse);

            // Increment hop-by-hop option header length by one which
            // increases its total size by 8 bytes.
            hbh.SetLength(hbh.GetLength() + 1);
            aMessage.Write(0, hbh);

            // Make space for MPL Option + padding (8 bytes) at the end
            // of hop-by-hop header
            SuccessOrExit(error = aMessage.InsertHeader(hbhSize, ExtensionHeader::kLengthUnitSize));

            // Insert MPL Option
            mMpl.InitOption(mplOption, aHeader.GetSource());
            aMessage.WriteBytes(hbhSize, &mplOption, mplOption.GetSize());

            // Insert Pad Option (if needed)
            if (padOption.InitToPadHeaderWithSize(mplOption.GetSize()) == kErrorNone)
            {
                aMessage.WriteBytes(hbhSize + mplOption.GetSize(), &padOption, padOption.GetSize());
            }

            // Update IPv6 Payload Length
            aHeader.SetPayloadLength(aHeader.GetPayloadLength() + ExtensionHeader::kLengthUnitSize);
        }
        else
        {
            SuccessOrExit(error = AddMplOption(aMessage, aHeader));
        }

        SuccessOrExit(error = aMessage.Prepend(aHeader));
    }
    else
    {
        SuccessOrExit(error = PrepareMulticastToLargerThanRealmLocal(aMessage, aHeader));
    }

exit:
    return error;
}

Error Ip6::RemoveMplOption(Message &aMessage)
{
    Error          error = kErrorNone;
    Header         ip6Header;
    HopByHopHeader hbh;
    Option         option;
    uint16_t       offset;
    uint16_t       endOffset;
    uint16_t       mplOffset = 0;
    uint8_t        mplLength = 0;
    bool           remove    = false;

    offset = 0;
    IgnoreError(aMessage.Read(offset, ip6Header));
    offset += sizeof(ip6Header);
    VerifyOrExit(ip6Header.GetNextHeader() == kProtoHopOpts);

    IgnoreError(aMessage.Read(offset, hbh));
    endOffset = offset + hbh.GetSize();
    VerifyOrExit(aMessage.GetLength() >= endOffset, error = kErrorParse);

    offset += sizeof(hbh);

    for (; offset < endOffset; offset += option.GetSize())
    {
        IgnoreError(option.ParseFrom(aMessage, offset, endOffset));

        if (option.IsPadding())
        {
            continue;
        }

        if (option.GetType() == MplOption::kType)
        {
            // If multiple MPL options exist, discard packet
            VerifyOrExit(mplOffset == 0, error = kErrorParse);

            mplOffset = offset;
            mplLength = option.GetLength();

            VerifyOrExit(mplLength <= sizeof(MplOption) - sizeof(Option), error = kErrorParse);

            if (mplOffset == sizeof(ip6Header) + sizeof(hbh) && hbh.GetLength() == 0)
            {
                // First and only IPv6 Option, remove IPv6 HBH Option header
                remove = true;
            }
            else if (mplOffset + ExtensionHeader::kLengthUnitSize == endOffset)
            {
                // Last IPv6 Option, remove the last 8 bytes
                remove = true;
            }
        }
        else
        {
            // Encountered another option, now just replace
            // MPL Option with Pad Option
            remove = false;
        }
    }

    // verify that IPv6 Options header is properly formed
    VerifyOrExit(offset == endOffset, error = kErrorParse);

    if (remove)
    {
        // Last IPv6 Option, shrink HBH Option header by
        // 8 bytes (`kLengthUnitSize`)
        aMessage.RemoveHeader(endOffset - ExtensionHeader::kLengthUnitSize, ExtensionHeader::kLengthUnitSize);

        if (mplOffset == sizeof(ip6Header) + sizeof(hbh))
        {
            // Remove entire HBH header
            ip6Header.SetNextHeader(hbh.GetNextHeader());
        }
        else
        {
            // Update HBH header length, decrement by one
            // which decreases its total size by 8 bytes.

            hbh.SetLength(hbh.GetLength() - 1);
            aMessage.Write(sizeof(ip6Header), hbh);
        }

        ip6Header.SetPayloadLength(ip6Header.GetPayloadLength() - ExtensionHeader::kLengthUnitSize);
        aMessage.Write(0, ip6Header);
    }
    else if (mplOffset != 0)
    {
        // Replace MPL Option with Pad Option
        PadOption padOption;

        padOption.InitForPadSize(sizeof(Option) + mplLength);
        aMessage.WriteBytes(mplOffset, &padOption, padOption.GetSize());
    }

exit:
    return error;
}

void Ip6::EnqueueDatagram(Message &aMessage)
{
    mSendQueue.Enqueue(aMessage);
    mSendQueueTask.Post();
}

Error Ip6::SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto)
{
    Error    error = kErrorNone;
    Header   header;
    uint8_t  dscp;
    uint16_t payloadLength = aMessage.GetLength();

    if ((aIpProto == kProtoUdp) &&
        Get<Tmf::Agent>().IsTmfMessage(aMessageInfo.GetSockAddr(), aMessageInfo.GetPeerAddr(),
                                       aMessageInfo.GetPeerPort()))
    {
        dscp = Tmf::Agent::PriorityToDscp(aMessage.GetPriority());
    }
    else
    {
        dscp = PriorityToDscp(aMessage.GetPriority());
    }

    header.InitVersionTrafficClassFlow();
    header.SetDscp(dscp);
    header.SetEcn(aMessageInfo.GetEcn());
    header.SetPayloadLength(payloadLength);
    header.SetNextHeader(aIpProto);

    if (aMessageInfo.GetHopLimit() != 0 || aMessageInfo.ShouldAllowZeroHopLimit())
    {
        header.SetHopLimit(aMessageInfo.GetHopLimit());
    }
    else
    {
        header.SetHopLimit(static_cast<uint8_t>(kDefaultHopLimit));
    }

    if (aMessageInfo.GetSockAddr().IsUnspecified() || aMessageInfo.GetSockAddr().IsMulticast())
    {
        const Address *source = SelectSourceAddress(aMessageInfo.GetPeerAddr());

        VerifyOrExit(source != nullptr, error = kErrorInvalidSourceAddress);
        header.SetSource(*source);
    }
    else
    {
        header.SetSource(aMessageInfo.GetSockAddr());
    }

    header.SetDestination(aMessageInfo.GetPeerAddr());

    if (aMessageInfo.GetPeerAddr().IsRealmLocalMulticast())
    {
        SuccessOrExit(error = AddMplOption(aMessage, header));
    }

    SuccessOrExit(error = aMessage.Prepend(header));

    Checksum::UpdateMessageChecksum(aMessage, header.GetSource(), header.GetDestination(), aIpProto);

    if (aMessageInfo.GetPeerAddr().IsMulticastLargerThanRealmLocal())
    {
        SuccessOrExit(error = PrepareMulticastToLargerThanRealmLocal(aMessage, header));
    }

    aMessage.SetMulticastLoop(aMessageInfo.GetMulticastLoop());

    if (aMessage.GetLength() > kMaxDatagramLength)
    {
        error = FragmentDatagram(aMessage, aIpProto);
    }
    else
    {
        EnqueueDatagram(aMessage);
    }

exit:

    return error;
}

void Ip6::HandleSendQueue(void)
{
    Message *message;

    while ((message = mSendQueue.GetHead()) != nullptr)
    {
        mSendQueue.Dequeue(*message);
        IgnoreError(HandleDatagram(OwnedPtr<Message>(message)));
    }
}

Error Ip6::HandleOptions(Message &aMessage, Header &aHeader, bool &aReceive)
{
    Error          error = kErrorNone;
    HopByHopHeader hbhHeader;
    Option         option;
    uint16_t       offset = aMessage.GetOffset();
    uint16_t       endOffset;

    SuccessOrExit(error = aMessage.Read(offset, hbhHeader));

    endOffset = offset + hbhHeader.GetSize();
    VerifyOrExit(endOffset <= aMessage.GetLength(), error = kErrorParse);

    offset += sizeof(HopByHopHeader);

    for (; offset < endOffset; offset += option.GetSize())
    {
        SuccessOrExit(error = option.ParseFrom(aMessage, offset, endOffset));

        if (option.IsPadding())
        {
            continue;
        }

        if (option.GetType() == MplOption::kType)
        {
            SuccessOrExit(error = mMpl.ProcessOption(aMessage, offset, aHeader.GetSource(), aReceive));
            continue;
        }

        VerifyOrExit(option.GetAction() == Option::kActionSkip, error = kErrorDrop);
    }

    aMessage.SetOffset(offset);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
Error Ip6::FragmentDatagram(Message &aMessage, uint8_t aIpProto)
{
    Error          error = kErrorNone;
    Header         header;
    FragmentHeader fragmentHeader;
    Message       *fragment        = nullptr;
    uint16_t       fragmentCnt     = 0;
    uint16_t       payloadFragment = 0;
    uint16_t       offset          = 0;

    uint16_t maxPayloadFragment =
        FragmentHeader::MakeDivisibleByEight(kMinimalMtu - aMessage.GetOffset() - sizeof(fragmentHeader));
    uint16_t payloadLeft = aMessage.GetLength() - aMessage.GetOffset();

    SuccessOrExit(error = aMessage.Read(0, header));
    header.SetNextHeader(kProtoFragment);

    fragmentHeader.Init();
    fragmentHeader.SetIdentification(Random::NonCrypto::GetUint32());
    fragmentHeader.SetNextHeader(aIpProto);
    fragmentHeader.SetMoreFlag();

    while (payloadLeft != 0)
    {
        if (payloadLeft < maxPayloadFragment)
        {
            fragmentHeader.ClearMoreFlag();

            payloadFragment = payloadLeft;
            payloadLeft     = 0;

            LogDebg("Last Fragment");
        }
        else
        {
            payloadLeft -= maxPayloadFragment;
            payloadFragment = maxPayloadFragment;
        }

        offset = fragmentCnt * FragmentHeader::BytesToFragmentOffset(maxPayloadFragment);
        fragmentHeader.SetOffset(offset);

        VerifyOrExit((fragment = NewMessage()) != nullptr, error = kErrorNoBufs);
        IgnoreError(fragment->SetPriority(aMessage.GetPriority()));
        SuccessOrExit(error = fragment->SetLength(aMessage.GetOffset() + sizeof(fragmentHeader) + payloadFragment));

        header.SetPayloadLength(payloadFragment + sizeof(fragmentHeader));
        fragment->Write(0, header);

        fragment->SetOffset(aMessage.GetOffset());
        fragment->Write(aMessage.GetOffset(), fragmentHeader);

        fragment->WriteBytesFromMessage(
            /* aWriteOffset */ aMessage.GetOffset() + sizeof(fragmentHeader), aMessage,
            /* aReadOffset */ aMessage.GetOffset() + FragmentHeader::FragmentOffsetToBytes(offset),
            /* aLength */ payloadFragment);

        EnqueueDatagram(*fragment);

        fragmentCnt++;
        fragment = nullptr;

        LogInfo("Fragment %d with %d bytes sent", fragmentCnt, payloadFragment);
    }

    aMessage.Free();

exit:

    if (error == kErrorNoBufs)
    {
        LogWarn("No buffer for Ip6 fragmentation");
    }

    FreeMessageOnError(fragment, error);
    return error;
}

Error Ip6::HandleFragment(Message &aMessage, MessageInfo &aMessageInfo)
{
    Error          error = kErrorNone;
    Header         header, headerBuffer;
    FragmentHeader fragmentHeader;
    Message       *message         = nullptr;
    uint16_t       offset          = 0;
    uint16_t       payloadFragment = 0;
    bool           isFragmented    = true;

    SuccessOrExit(error = aMessage.Read(0, header));
    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), fragmentHeader));

    if (fragmentHeader.GetOffset() == 0 && !fragmentHeader.IsMoreFlagSet())
    {
        isFragmented = false;
        aMessage.MoveOffset(sizeof(fragmentHeader));
        ExitNow();
    }

    for (Message &msg : mReassemblyList)
    {
        SuccessOrExit(error = msg.Read(0, headerBuffer));

        if (msg.GetDatagramTag() == fragmentHeader.GetIdentification() &&
            headerBuffer.GetSource() == header.GetSource() && headerBuffer.GetDestination() == header.GetDestination())
        {
            message = &msg;
            break;
        }
    }

    offset          = FragmentHeader::FragmentOffsetToBytes(fragmentHeader.GetOffset());
    payloadFragment = aMessage.GetLength() - aMessage.GetOffset() - sizeof(fragmentHeader);

    LogInfo("Fragment with id %d received > %d bytes, offset %d", fragmentHeader.GetIdentification(), payloadFragment,
            offset);

    if (offset + payloadFragment + aMessage.GetOffset() > kMaxAssembledDatagramLength)
    {
        LogWarn("Packet too large for fragment buffer");
        ExitNow(error = kErrorNoBufs);
    }

    if (message == nullptr)
    {
        LogDebg("start reassembly");
        VerifyOrExit((message = NewMessage()) != nullptr, error = kErrorNoBufs);
        mReassemblyList.Enqueue(*message);

        message->SetTimestampToNow();
        message->SetOffset(0);
        message->SetDatagramTag(fragmentHeader.GetIdentification());

        // copying the non-fragmentable header to the fragmentation buffer
        SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, 0, aMessage.GetOffset()));

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kIp6FragmentReassembler);
    }

    // increase message buffer if necessary
    if (message->GetLength() < offset + payloadFragment + aMessage.GetOffset())
    {
        SuccessOrExit(error = message->SetLength(offset + payloadFragment + aMessage.GetOffset()));
    }

    // copy the fragment payload into the message buffer
    message->WriteBytesFromMessage(
        /* aWriteOffset */ aMessage.GetOffset() + offset, aMessage,
        /* aReadOffset */ aMessage.GetOffset() + sizeof(fragmentHeader), /* aLength */ payloadFragment);

    // check if it is the last frame
    if (!fragmentHeader.IsMoreFlagSet())
    {
        // use the offset value for the whole ip message length
        message->SetOffset(aMessage.GetOffset() + offset + payloadFragment);

        // creates the header for the reassembled ipv6 package
        SuccessOrExit(error = aMessage.Read(0, header));
        header.SetPayloadLength(message->GetLength() - sizeof(header));
        header.SetNextHeader(fragmentHeader.GetNextHeader());
        message->Write(0, header);

        LogDebg("Reassembly complete.");

        mReassemblyList.Dequeue(*message);

        IgnoreError(HandleDatagram(OwnedPtr<Message>(message), aMessageInfo.mLinkInfo,
                                   /* aIsReassembled */ true));
    }

exit:
    if (error != kErrorDrop && error != kErrorNone && isFragmented)
    {
        if (message != nullptr)
        {
            mReassemblyList.DequeueAndFree(*message);
        }

        LogWarn("Reassembly failed: %s", ErrorToString(error));
    }

    if (isFragmented)
    {
        // drop all fragments, the payload is stored in the fragment buffer
        error = kErrorDrop;
    }

    return error;
}

void Ip6::CleanupFragmentationBuffer(void) { mReassemblyList.DequeueAndFreeAll(); }

void Ip6::HandleTimeTick(void)
{
    UpdateReassemblyList();

    if (mReassemblyList.GetHead() == nullptr)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kIp6FragmentReassembler);
    }
}

void Ip6::UpdateReassemblyList(void)
{
    TimeMilli now = TimerMilli::GetNow();

    for (Message &message : mReassemblyList)
    {
        if (now - message.GetTimestamp() >= TimeMilli::SecToMsec(kIp6ReassemblyTimeout))
        {
            LogNote("Reassembly timeout.");
            SendIcmpError(message, Icmp::Header::kTypeTimeExceeded, Icmp::Header::kCodeFragmReasTimeEx);

            mReassemblyList.DequeueAndFree(message);
        }
    }
}

void Ip6::SendIcmpError(Message &aMessage, Icmp::Header::Type aIcmpType, Icmp::Header::Code aIcmpCode)
{
    Error       error = kErrorNone;
    Header      header;
    MessageInfo messageInfo;

    SuccessOrExit(error = aMessage.Read(0, header));

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetLinkInfo(nullptr);

    error = mIcmp.SendError(aIcmpType, aIcmpCode, messageInfo, aMessage);

exit:

    if (error != kErrorNone)
    {
        LogWarn("Failed to send ICMP error: %s", ErrorToString(error));
    }
}

#else
Error Ip6::FragmentDatagram(Message &aMessage, uint8_t aIpProto)
{
    OT_UNUSED_VARIABLE(aIpProto);

    EnqueueDatagram(aMessage);

    return kErrorNone;
}

Error Ip6::HandleFragment(Message &aMessage, MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error          error = kErrorNone;
    FragmentHeader fragmentHeader;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), fragmentHeader));

    VerifyOrExit(fragmentHeader.GetOffset() == 0 && !fragmentHeader.IsMoreFlagSet(), error = kErrorDrop);

    aMessage.MoveOffset(sizeof(fragmentHeader));

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

Error Ip6::HandleExtensionHeaders(OwnedPtr<Message> &aMessagePtr,
                                  MessageInfo       &aMessageInfo,
                                  Header            &aHeader,
                                  uint8_t           &aNextHeader,
                                  bool              &aReceive)
{
    Error error = kErrorNone;

    ExtensionHeader extHeader;

    while (aReceive || aNextHeader == kProtoHopOpts)
    {
        SuccessOrExit(error = aMessagePtr->Read(aMessagePtr->GetOffset(), extHeader));

        switch (aNextHeader)
        {
        case kProtoHopOpts:
            SuccessOrExit(error = HandleOptions(*aMessagePtr, aHeader, aReceive));
            break;

        case kProtoFragment:
            IgnoreError(PassToHost(aMessagePtr, aMessageInfo, aNextHeader,
                                   /* aApplyFilter */ false, aReceive, Message::kCopyToUse));
            SuccessOrExit(error = HandleFragment(*aMessagePtr, aMessageInfo));
            break;

        case kProtoDstOpts:
            SuccessOrExit(error = HandleOptions(*aMessagePtr, aHeader, aReceive));
            break;

        case kProtoIp6:
            ExitNow();

        case kProtoRouting:
        case kProtoNone:
            ExitNow(error = kErrorDrop);

        default:
            ExitNow();
        }

        aNextHeader = static_cast<uint8_t>(extHeader.GetNextHeader());
    }

exit:
    return error;
}

Error Ip6::TakeOrCopyMessagePtr(OwnedPtr<Message> &aTargetPtr,
                                OwnedPtr<Message> &aMessagePtr,
                                Message::Ownership aMessageOwnership)
{
    switch (aMessageOwnership)
    {
    case Message::kTakeCustody:
        aTargetPtr = aMessagePtr.PassOwnership();
        break;

    case Message::kCopyToUse:
        aTargetPtr.Reset(aMessagePtr->Clone());
        break;
    }

    return (aTargetPtr != nullptr) ? kErrorNone : kErrorNoBufs;
}

Error Ip6::HandlePayload(Header            &aIp6Header,
                         OwnedPtr<Message> &aMessagePtr,
                         MessageInfo       &aMessageInfo,
                         uint8_t            aIpProto,
                         Message::Ownership aMessageOwnership)
{
#if !OPENTHREAD_CONFIG_TCP_ENABLE
    OT_UNUSED_VARIABLE(aIp6Header);
#endif

    Error             error = kErrorNone;
    OwnedPtr<Message> messagePtr;

    switch (aIpProto)
    {
    case kProtoUdp:
    case kProtoIcmp6:
        break;
#if OPENTHREAD_CONFIG_TCP_ENABLE
    case kProtoTcp:
        break;
#endif
    default:
        ExitNow();
    }

    SuccessOrExit(error = TakeOrCopyMessagePtr(messagePtr, aMessagePtr, aMessageOwnership));

    switch (aIpProto)
    {
#if OPENTHREAD_CONFIG_TCP_ENABLE
    case kProtoTcp:
        error = mTcp.HandleMessage(aIp6Header, *messagePtr, aMessageInfo);
        break;
#endif
    case kProtoUdp:
        error = mUdp.HandleMessage(*messagePtr, aMessageInfo);
        break;

    case kProtoIcmp6:
        error = mIcmp.HandleMessage(*messagePtr, aMessageInfo);
        break;

    default:
        break;
    }

exit:
    if (error != kErrorNone)
    {
        LogNote("Failed to handle payload: %s", ErrorToString(error));
    }

    return error;
}

Error Ip6::PassToHost(OwnedPtr<Message> &aMessagePtr,
                      const MessageInfo &aMessageInfo,
                      uint8_t            aIpProto,
                      bool               aApplyFilter,
                      bool               aReceive,
                      Message::Ownership aMessageOwnership)
{
    // This method passes the message to host by invoking the
    // registered IPv6 receive callback. When NAT64 is enabled, it
    // may also perform translation and invoke IPv4 receive
    // callback.

    Error             error = kErrorNone;
    OwnedPtr<Message> messagePtr;

    VerifyOrExit(aMessagePtr->IsLoopbackToHostAllowed(), error = kErrorNoRoute);

    VerifyOrExit(mReceiveIp6DatagramCallback.IsSet(), error = kErrorNoRoute);

    // Do not pass IPv6 packets that exceed kMinimalMtu.
    VerifyOrExit(aMessagePtr->GetLength() <= kMinimalMtu, error = kErrorDrop);

    // If the sender used mesh-local address as source, do not pass to
    // host unless this message is intended for this device itself.
    if (Get<Mle::Mle>().IsMeshLocalAddress(aMessageInfo.GetPeerAddr()))
    {
        VerifyOrExit(aReceive, error = kErrorDrop);
    }

    if (mIsReceiveIp6FilterEnabled && aApplyFilter)
    {
#if !OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
        // Do not pass messages sent to an RLOC/ALOC, except
        // Service Locator

        bool isLocator = Get<Mle::Mle>().IsMeshLocalAddress(aMessageInfo.GetSockAddr()) &&
                         aMessageInfo.GetSockAddr().GetIid().IsLocator();

        VerifyOrExit(!isLocator || aMessageInfo.GetSockAddr().GetIid().IsAnycastServiceLocator(),
                     error = kErrorNoRoute);
#endif

        switch (aIpProto)
        {
        case kProtoIcmp6:
            if (mIcmp.ShouldHandleEchoRequest(aMessageInfo))
            {
                Icmp::Header icmp;

                IgnoreError(aMessagePtr->Read(aMessagePtr->GetOffset(), icmp));
                VerifyOrExit(icmp.GetType() != Icmp::Header::kTypeEchoRequest, error = kErrorDrop);
            }

            break;

        case kProtoUdp:
        {
            Udp::Header udp;

            IgnoreError(aMessagePtr->Read(aMessagePtr->GetOffset(), udp));
            VerifyOrExit(Get<Udp>().ShouldUsePlatformUdp(udp.GetDestinationPort()) &&
                             !Get<Udp>().IsPortInUse(udp.GetDestinationPort()),
                         error = kErrorNoRoute);
            break;
        }

#if OPENTHREAD_CONFIG_TCP_ENABLE
        // Do not pass TCP message to avoid dual processing from both
        // OpenThread and POSIX TCP stacks.
        case kProtoTcp:
            error = kErrorNoRoute;
            ExitNow();
#endif

        default:
            break;
        }
    }

    SuccessOrExit(error = TakeOrCopyMessagePtr(messagePtr, aMessagePtr, aMessageOwnership));

    IgnoreError(RemoveMplOption(*messagePtr));

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    switch (Get<Nat64::Translator>().TranslateFromIp6(*messagePtr))
    {
    case Nat64::Translator::kNotTranslated:
        break;

    case Nat64::Translator::kDrop:
        ExitNow(error = kErrorDrop);

    case Nat64::Translator::kForward:
        VerifyOrExit(mReceiveIp4DatagramCallback.IsSet(), error = kErrorNoRoute);
        // Pass message to callback transferring its ownership.
        mReceiveIp4DatagramCallback.Invoke(messagePtr.Release());
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    {
        Header header;

        IgnoreError(header.ParseFrom(*messagePtr));
        UpdateBorderRoutingCounters(header, messagePtr->GetLength(), /* aIsInbound */ false);
    }
#endif

    // Pass message to callback transferring its ownership.
    mReceiveIp6DatagramCallback.Invoke(messagePtr.Release());

exit:
    return error;
}

Error Ip6::SendRaw(OwnedPtr<Message> aMessagePtr)
{
    Error  error = kErrorNone;
    Header header;

    SuccessOrExit(error = header.ParseFrom(*aMessagePtr));
    VerifyOrExit(!header.GetSource().IsMulticast(), error = kErrorInvalidSourceAddress);

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    // The filtering rules don't apply to packets from DUA.
    if (!Get<BackboneRouter::Leader>().IsDomainUnicast(header.GetSource()))
#endif
    {
        // When the packet is forwarded from host to Thread, if its source is on-mesh or its destination is
        // mesh-local, we'll drop the packet unless the packet originates from this device.
        if (Get<NetworkData::Leader>().IsOnMesh(header.GetSource()) ||
            Get<Mle::Mle>().IsMeshLocalAddress(header.GetDestination()))
        {
            VerifyOrExit(Get<ThreadNetif>().HasUnicastAddress(header.GetSource()), error = kErrorDrop);
        }
    }

    if (header.GetDestination().IsMulticast())
    {
        SuccessOrExit(error = InsertMplOption(*aMessagePtr, header));
    }

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    UpdateBorderRoutingCounters(header, aMessagePtr->GetLength(), /* aIsInbound */ true);
#endif

    error = HandleDatagram(aMessagePtr.PassOwnership());

exit:
    return error;
}

Error Ip6::HandleDatagram(OwnedPtr<Message> aMessagePtr, const void *aLinkMessageInfo, bool aIsReassembled)
{
    Error       error;
    MessageInfo messageInfo;
    Header      header;
    bool        receive;
    bool        forwardThread;
    bool        forwardHost;
    uint8_t     nextHeader;

    receive       = false;
    forwardThread = false;
    forwardHost   = false;

    SuccessOrExit(error = header.ParseFrom(*aMessagePtr));

    messageInfo.Clear();
    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetEcn(header.GetEcn());
    messageInfo.SetLinkInfo(aLinkMessageInfo);

    // Determine `forwardThread`, `forwardHost` and `receive`
    // based on the destination address.

    if (header.GetDestination().IsMulticast())
    {
        // Destination is multicast

        forwardThread = !aMessagePtr->IsOriginThreadNetif();

#if OPENTHREAD_FTD
        if (aMessagePtr->IsOriginThreadNetif() && header.GetDestination().IsMulticastLargerThanRealmLocal() &&
            Get<ChildTable>().HasSleepyChildWithAddress(header.GetDestination()))
        {
            forwardThread = true;
        }
#endif

        forwardHost = header.GetDestination().IsMulticastLargerThanRealmLocal();

        if ((aMessagePtr->IsOriginThreadNetif() || aMessagePtr->GetMulticastLoop()) &&
            Get<ThreadNetif>().IsMulticastSubscribed(header.GetDestination()))
        {
            receive = true;
        }
        else if (Get<ThreadNetif>().IsMulticastPromiscuousEnabled())
        {
            forwardHost = true;
        }
    }
    else
    {
        // Destination is unicast

        if (Get<ThreadNetif>().HasUnicastAddress(header.GetDestination()))
        {
            receive = true;
        }
        else if (!aMessagePtr->IsOriginThreadNetif() || !header.GetDestination().IsLinkLocal())
        {
            if (header.GetDestination().IsLinkLocal())
            {
                forwardThread = true;
            }
            else if (IsOnLink(header.GetDestination()))
            {
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
                forwardThread = (!aMessagePtr->IsLoopbackToHostAllowed() ||
                                 !Get<BackboneRouter::Manager>().ShouldForwardDuaToBackbone(header.GetDestination()));
#else
                forwardThread = true;
#endif
            }
            else if (RouteLookup(header.GetSource(), header.GetDestination()) == kErrorNone)
            {
                forwardThread = true;
            }

            forwardHost = !forwardThread;
        }
    }

    aMessagePtr->SetOffset(sizeof(header));

    // Process IPv6 Extension Headers
    nextHeader = static_cast<uint8_t>(header.GetNextHeader());
    SuccessOrExit(error = HandleExtensionHeaders(aMessagePtr, messageInfo, header, nextHeader, receive));

    if (receive && (nextHeader == kProtoIp6))
    {
        // Process the embedded IPv6 message in an IPv6 tunnel message.
        // If we need to `forwardThread` we create a copy by cloning
        // the message, otherwise we take ownership of `aMessage`
        // itself and use it directly. The encapsulating header is
        // then removed before processing the embedded message.

        OwnedPtr<Message> messagePtr;
        bool              multicastLoop = aMessagePtr->GetMulticastLoop();

        SuccessOrExit(error = TakeOrCopyMessagePtr(messagePtr, aMessagePtr,
                                                   forwardThread ? Message::kCopyToUse : Message::kTakeCustody));
        messagePtr->SetMulticastLoop(multicastLoop);
        messagePtr->RemoveHeader(messagePtr->GetOffset());

        Get<MeshForwarder>().LogMessage(MeshForwarder::kMessageReceive, *messagePtr);

        IgnoreError(HandleDatagram(messagePtr.PassOwnership(), aLinkMessageInfo, aIsReassembled));

        receive     = false;
        forwardHost = false;
    }

    if ((forwardHost || receive) && !aIsReassembled)
    {
        error = PassToHost(aMessagePtr, messageInfo, nextHeader,
                           /* aApplyFilter */ !forwardHost, receive,
                           (receive || forwardThread) ? Message::kCopyToUse : Message::kTakeCustody);
    }

    if (receive)
    {
        error = HandlePayload(header, aMessagePtr, messageInfo, nextHeader,
                              forwardThread ? Message::kCopyToUse : Message::kTakeCustody);
    }

    if (forwardThread)
    {
        if (aMessagePtr->IsOriginThreadNetif())
        {
            VerifyOrExit(Get<Mle::Mle>().IsRouterOrLeader());
            header.SetHopLimit(header.GetHopLimit() - 1);
        }

        VerifyOrExit(header.GetHopLimit() > 0, error = kErrorDrop);

        aMessagePtr->Write<uint8_t>(Header::kHopLimitFieldOffset, header.GetHopLimit());

        if (nextHeader == kProtoIcmp6)
        {
            uint8_t icmpType;

            SuccessOrExit(error = aMessagePtr->Read(aMessagePtr->GetOffset(), icmpType));

            error = kErrorDrop;

            for (IcmpType type : kForwardIcmpTypes)
            {
                if (icmpType == type)
                {
                    error = kErrorNone;
                    break;
                }
            }

            SuccessOrExit(error);
        }

        if (aMessagePtr->IsOriginHostUntrusted() && (nextHeader == kProtoUdp))
        {
            uint16_t destPort;

            SuccessOrExit(
                error = aMessagePtr->Read(aMessagePtr->GetOffset() + Udp::Header::kDestPortFieldOffset, destPort));
            destPort = BigEndian::HostSwap16(destPort);

            if (destPort == Tmf::kUdpPort)
            {
                LogNote("Dropping TMF message from untrusted origin");
                ExitNow(error = kErrorDrop);
            }
        }

#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if (aMessagePtr->IsOriginHostTrusted() && !aMessagePtr->IsLoopbackToHostAllowed() && (nextHeader == kProtoUdp))
        {
            uint16_t destPort;

            SuccessOrExit(
                error = aMessagePtr->Read(aMessagePtr->GetOffset() + Udp::Header::kDestPortFieldOffset, destPort));
            destPort = BigEndian::HostSwap16(destPort);

            if (nextHeader == kProtoUdp)
            {
                VerifyOrExit(Get<Udp>().ShouldUsePlatformUdp(destPort), error = kErrorDrop);
            }
        }
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
        // Since the message will be forwarded, we clear the radio
        // type on the message to allow the radio type for tx to be
        // selected later (based on the radios supported by the next
        // hop).
        aMessagePtr->ClearRadioType();
#endif

        Get<MeshForwarder>().SendMessage(aMessagePtr.PassOwnership());
    }

exit:
    return error;
}

Error Ip6::SelectSourceAddress(MessageInfo &aMessageInfo) const
{
    Error          error = kErrorNone;
    const Address *source;

    source = SelectSourceAddress(aMessageInfo.GetPeerAddr());
    VerifyOrExit(source != nullptr, error = kErrorNotFound);
    aMessageInfo.SetSockAddr(*source);

exit:
    return error;
}

const Address *Ip6::SelectSourceAddress(const Address &aDestination) const
{
    uint8_t                      destScope    = aDestination.GetScope();
    bool                         destIsRloc   = Get<Mle::Mle>().IsRoutingLocator(aDestination);
    const Netif::UnicastAddress *bestAddr     = nullptr;
    uint8_t                      bestMatchLen = 0;

    for (const Netif::UnicastAddress &addr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        uint8_t matchLen;
        uint8_t overrideScope;

        if (Get<Mle::Mle>().IsAnycastLocator(addr.GetAddress()))
        {
            // Don't use anycast address as source address.
            continue;
        }

        matchLen = aDestination.PrefixMatch(addr.GetAddress());

        if (matchLen >= addr.mPrefixLength)
        {
            matchLen      = addr.mPrefixLength;
            overrideScope = addr.GetScope();
        }
        else
        {
            overrideScope = destScope;
        }

        if (bestAddr == nullptr)
        {
            // Rule 0: Prefer any address
            bestAddr     = &addr;
            bestMatchLen = matchLen;
        }
        else if (addr.GetAddress() == aDestination)
        {
            // Rule 1: Prefer same address
            bestAddr = &addr;
            ExitNow();
        }
        else if (addr.GetScope() < bestAddr->GetScope())
        {
            // Rule 2: Prefer appropriate scope
            if (addr.GetScope() >= overrideScope)
            {
                bestAddr     = &addr;
                bestMatchLen = matchLen;
            }
            else
            {
                continue;
            }
        }
        else if (addr.GetScope() > bestAddr->GetScope())
        {
            if (bestAddr->GetScope() < overrideScope)
            {
                bestAddr     = &addr;
                bestMatchLen = matchLen;
            }
            else
            {
                continue;
            }
        }
        else if (addr.mPreferred != bestAddr->mPreferred)
        {
            // Rule 3: Avoid deprecated addresses

            if (addr.mPreferred)
            {
                bestAddr     = &addr;
                bestMatchLen = matchLen;
            }
            else
            {
                continue;
            }
        }
        else if (matchLen > bestMatchLen)
        {
            // Rule 6: Prefer matching label
            // Rule 7: Prefer public address
            // Rule 8: Use longest prefix matching
            bestAddr     = &addr;
            bestMatchLen = matchLen;
        }
        else if ((matchLen == bestMatchLen) && (destIsRloc == Get<Mle::Mle>().IsRoutingLocator(addr.GetAddress())))
        {
            // Additional rule: Prefer RLOC source for RLOC destination, EID source for anything else
            bestAddr     = &addr;
            bestMatchLen = matchLen;
        }
        else
        {
            continue;
        }

        // infer destination scope based on prefix match
        if (bestMatchLen >= bestAddr->mPrefixLength)
        {
            destScope = bestAddr->GetScope();
        }
    }

exit:
    return (bestAddr != nullptr) ? &bestAddr->GetAddress() : nullptr;
}

bool Ip6::IsOnLink(const Address &aAddress) const
{
    bool isOnLink = false;

    if (Get<NetworkData::Leader>().IsOnMesh(aAddress))
    {
        ExitNow(isOnLink = true);
    }

    for (const Netif::UnicastAddress &unicastAddr : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (unicastAddr.GetAddress().PrefixMatch(aAddress) >= unicastAddr.mPrefixLength)
        {
            ExitNow(isOnLink = true);
        }
    }

exit:
    return isOnLink;
}

Error Ip6::RouteLookup(const Address &aSource, const Address &aDestination) const
{
    Error    error;
    uint16_t rloc;

    error = Get<NetworkData::Leader>().RouteLookup(aSource, aDestination, rloc);

    if (error == kErrorNone)
    {
        if (rloc == Get<Mle::MleRouter>().GetRloc16())
        {
            error = kErrorNoRoute;
        }
    }
    else
    {
        LogNote("Failed to find valid route for: %s", aDestination.ToString().AsCString());
    }

    return error;
}

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
void Ip6::UpdateBorderRoutingCounters(const Header &aHeader, uint16_t aMessageLength, bool aIsInbound)
{
    static constexpr uint8_t kPrefixLength   = 48;
    otPacketsAndBytes       *counter         = nullptr;
    otPacketsAndBytes       *internetCounter = nullptr;

    VerifyOrExit(!aHeader.GetSource().IsLinkLocal());
    VerifyOrExit(!aHeader.GetDestination().IsLinkLocal());
    VerifyOrExit(aHeader.GetSource().GetPrefix() != Get<Mle::Mle>().GetMeshLocalPrefix());
    VerifyOrExit(aHeader.GetDestination().GetPrefix() != Get<Mle::Mle>().GetMeshLocalPrefix());

    if (aIsInbound)
    {
        VerifyOrExit(!Get<Netif>().HasUnicastAddress(aHeader.GetSource()));
        if (!aHeader.GetSource().MatchesPrefix(aHeader.GetDestination().GetPrefix().m8, kPrefixLength))
        {
            internetCounter = &mBorderRoutingCounters.mInboundInternet;
        }
        if (aHeader.GetDestination().IsMulticast())
        {
            VerifyOrExit(aHeader.GetDestination().IsMulticastLargerThanRealmLocal());
            counter = &mBorderRoutingCounters.mInboundMulticast;
        }
        else
        {
            counter = &mBorderRoutingCounters.mInboundUnicast;
        }
    }
    else
    {
        VerifyOrExit(!Get<Netif>().HasUnicastAddress(aHeader.GetDestination()));
        if (!aHeader.GetSource().MatchesPrefix(aHeader.GetDestination().GetPrefix().m8, kPrefixLength))
        {
            internetCounter = &mBorderRoutingCounters.mOutboundInternet;
        }
        if (aHeader.GetDestination().IsMulticast())
        {
            VerifyOrExit(aHeader.GetDestination().IsMulticastLargerThanRealmLocal());
            counter = &mBorderRoutingCounters.mOutboundMulticast;
        }
        else
        {
            counter = &mBorderRoutingCounters.mOutboundUnicast;
        }
    }

exit:

    if (counter)
    {
        counter->mPackets += 1;
        counter->mBytes += aMessageLength;
    }
    if (internetCounter)
    {
        internetCounter->mPackets += 1;
        internetCounter->mBytes += aMessageLength;
    }
}
#endif

// LCOV_EXCL_START

const char *Ip6::IpProtoToString(uint8_t aIpProto)
{
    static constexpr Stringify::Entry kIpProtoTable[] = {
        {kProtoHopOpts, "HopOpts"}, {kProtoTcp, "TCP"},         {kProtoUdp, "UDP"},
        {kProtoIp6, "IP6"},         {kProtoRouting, "Routing"}, {kProtoFragment, "Frag"},
        {kProtoIcmp6, "ICMP6"},     {kProtoNone, "None"},       {kProtoDstOpts, "DstOpts"},
    };

    static_assert(Stringify::IsSorted(kIpProtoTable), "kIpProtoTable is not sorted");

    return Stringify::Lookup(aIpProto, kIpProtoTable, "Unknown");
}

const char *Ip6::EcnToString(Ecn aEcn)
{
    static const char *const kEcnStrings[] = {
        "no", // (0) kEcnNotCapable
        "e1", // (1) kEcnCapable1  (ECT1)
        "e0", // (2) kEcnCapable0  (ECT0)
        "ce", // (3) kEcnMarked    (Congestion Encountered)
    };

    static_assert(0 == kEcnNotCapable, "kEcnNotCapable value is incorrect");
    static_assert(1 == kEcnCapable1, "kEcnCapable1 value is incorrect");
    static_assert(2 == kEcnCapable0, "kEcnCapable0 value is incorrect");
    static_assert(3 == kEcnMarked, "kEcnMarked value is incorrect");

    return kEcnStrings[aEcn];
}

// LCOV_EXCL_STOP

//---------------------------------------------------------------------------------------------------------------------
// Headers

Error Headers::ParseFrom(const Message &aMessage)
{
    Error error = kErrorParse;

    Clear();

    SuccessOrExit(mIp6Header.ParseFrom(aMessage));

    switch (mIp6Header.GetNextHeader())
    {
    case kProtoUdp:
        SuccessOrExit(aMessage.Read(sizeof(Header), mHeader.mUdp));
        break;
    case kProtoTcp:
        SuccessOrExit(aMessage.Read(sizeof(Header), mHeader.mTcp));
        break;
    case kProtoIcmp6:
        SuccessOrExit(aMessage.Read(sizeof(Header), mHeader.mIcmp));
        break;
    default:
        break;
    }

    error = kErrorNone;

exit:
    return error;
}

Error Headers::DecompressFrom(const Message &aMessage, uint16_t aOffset, const Mac::Addresses &aMacAddrs)
{
    static constexpr uint16_t kReadLength = sizeof(Lowpan::FragmentHeader::NextFrag) + sizeof(Headers);

    uint8_t   frameBuffer[kReadLength];
    uint16_t  frameLength;
    FrameData frameData;

    frameLength = aMessage.ReadBytes(aOffset, frameBuffer, sizeof(frameBuffer));
    frameData.Init(frameBuffer, frameLength);

    return DecompressFrom(frameData, aMacAddrs, aMessage.GetInstance());
}

Error Headers::DecompressFrom(const FrameData &aFrameData, const Mac::Addresses &aMacAddrs, Instance &aInstance)
{
    Error                  error     = kErrorNone;
    FrameData              frameData = aFrameData;
    Lowpan::FragmentHeader fragmentHeader;
    bool                   nextHeaderCompressed;

    if (fragmentHeader.ParseFrom(frameData) == kErrorNone)
    {
        // Only the first fragment header is followed by a LOWPAN_IPHC header
        VerifyOrExit(fragmentHeader.GetDatagramOffset() == 0, error = kErrorNotFound);
    }

    VerifyOrExit(Lowpan::Lowpan::IsLowpanHc(frameData), error = kErrorNotFound);

    SuccessOrExit(error = aInstance.Get<Lowpan::Lowpan>().DecompressBaseHeader(mIp6Header, nextHeaderCompressed,
                                                                               aMacAddrs, frameData));

    switch (mIp6Header.GetNextHeader())
    {
    case kProtoUdp:
        if (nextHeaderCompressed)
        {
            SuccessOrExit(error = aInstance.Get<Lowpan::Lowpan>().DecompressUdpHeader(mHeader.mUdp, frameData));
        }
        else
        {
            SuccessOrExit(error = frameData.Read(mHeader.mUdp));
        }
        break;

    case kProtoTcp:
        SuccessOrExit(error = frameData.Read(mHeader.mTcp));
        break;

    case kProtoIcmp6:
        SuccessOrExit(error = frameData.Read(mHeader.mIcmp));
        break;

    default:
        break;
    }

exit:
    return error;
}

uint16_t Headers::GetSourcePort(void) const
{
    uint16_t port = 0;

    switch (GetIpProto())
    {
    case kProtoUdp:
        port = mHeader.mUdp.GetSourcePort();
        break;

    case kProtoTcp:
        port = mHeader.mTcp.GetSourcePort();
        break;

    default:
        break;
    }

    return port;
}

uint16_t Headers::GetDestinationPort(void) const
{
    uint16_t port = 0;

    switch (GetIpProto())
    {
    case kProtoUdp:
        port = mHeader.mUdp.GetDestinationPort();
        break;

    case kProtoTcp:
        port = mHeader.mTcp.GetDestinationPort();
        break;

    default:
        break;
    }

    return port;
}

uint16_t Headers::GetChecksum(void) const
{
    uint16_t checksum = 0;

    switch (GetIpProto())
    {
    case kProtoUdp:
        checksum = mHeader.mUdp.GetChecksum();
        break;

    case kProtoTcp:
        checksum = mHeader.mTcp.GetChecksum();
        break;

    case kProtoIcmp6:
        checksum = mHeader.mIcmp.GetChecksum();
        break;

    default:
        break;
    }

    return checksum;
}

} // namespace Ip6
} // namespace ot
