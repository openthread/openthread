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

#include "instance/instance.hpp"

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
    , mReceiveFilterEnabled(false)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mTmfOriginFilterEnabled(true)
#endif
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
    tunnelHeader.SetHopLimit(kDefaultHopLimit);
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

    if (aHeader.GetDestination().IsMulticastLargerThanRealmLocal())
    {
        error = PrepareMulticastToLargerThanRealmLocal(aMessage, aHeader);
        ExitNow();
    }

    VerifyOrExit(aHeader.GetDestination().IsRealmLocalMulticast());

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

exit:
    return error;
}

Error Ip6::RemoveMplOption(Message &aMessage)
{
    enum Action : uint8_t
    {
        kNoMplOption,
        kShrinkHbh,
        kRemoveHbh,
        kReplaceMplWithPad,
    };

    Error          error  = kErrorNone;
    Action         action = kNoMplOption;
    Header         ip6Header;
    HopByHopHeader hbh;
    Option         option;
    OffsetRange    offsetRange;
    OffsetRange    mplOffsetRange;
    PadOption      padOption;

    offsetRange.InitFromMessageFullLength(aMessage);

    IgnoreError(aMessage.Read(offsetRange, ip6Header));
    offsetRange.AdvanceOffset(sizeof(ip6Header));

    VerifyOrExit(ip6Header.GetNextHeader() == kProtoHopOpts);

    SuccessOrExit(error = ReadHopByHopHeader(aMessage, offsetRange, hbh));

    for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(option.GetSize()))
    {
        SuccessOrExit(error = option.ParseFrom(aMessage, offsetRange));

        if (option.IsPadding())
        {
            continue;
        }

        if (option.GetType() == MplOption::kType)
        {
            // If multiple MPL options exist, discard packet
            VerifyOrExit(action == kNoMplOption, error = kErrorParse);

            // `Option::ParseFrom()` already validated that the entire
            // option is present in the `offsetRange`.

            mplOffsetRange = offsetRange;
            mplOffsetRange.ShrinkLength(option.GetSize());

            VerifyOrExit(option.GetSize() <= sizeof(MplOption), error = kErrorParse);

            if (mplOffsetRange.GetOffset() == sizeof(ip6Header) + sizeof(hbh) && hbh.GetLength() == 0)
            {
                // First and only IPv6 Option, remove IPv6 HBH Option header
                action = kRemoveHbh;
            }
            else if (mplOffsetRange.GetOffset() + ExtensionHeader::kLengthUnitSize == offsetRange.GetEndOffset())
            {
                // Last IPv6 Option, shrink the last 8 bytes
                action = kShrinkHbh;
            }
        }
        else if (action != kNoMplOption)
        {
            // Encountered another option, now just replace
            // MPL Option with Pad Option
            action = kReplaceMplWithPad;
        }
    }

    switch (action)
    {
    case kNoMplOption:
        break;

    case kShrinkHbh:
    case kRemoveHbh:
        // Last IPv6 Option, shrink HBH Option header by
        // 8 bytes (`kLengthUnitSize`)
        aMessage.RemoveHeader(offsetRange.GetEndOffset() - ExtensionHeader::kLengthUnitSize,
                              ExtensionHeader::kLengthUnitSize);

        if (action == kRemoveHbh)
        {
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
        break;

    case kReplaceMplWithPad:
        padOption.InitForPadSize(static_cast<uint8_t>(mplOffsetRange.GetLength()));
        aMessage.WriteBytes(mplOffsetRange.GetOffset(), &padOption, padOption.GetSize());
        break;
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
        header.SetHopLimit(kDefaultHopLimit);
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

Error Ip6::ReadHopByHopHeader(const Message &aMessage, OffsetRange &aOffsetRange, HopByHopHeader &aHbhHeader) const
{
    // Reads the HBH header from the message at the given offset range.
    // On success, updates `aOffsetRange` to indicate the location of
    // options within the HBH header.

    Error error;

    SuccessOrExit(error = aMessage.Read(aOffsetRange, aHbhHeader));
    VerifyOrExit(aOffsetRange.Contains(aHbhHeader.GetSize()), error = kErrorParse);
    aOffsetRange.ShrinkLength(aHbhHeader.GetSize());
    aOffsetRange.AdvanceOffset(sizeof(HopByHopHeader));

exit:
    return error;
}

Error Ip6::HandleOptions(Message &aMessage, const Header &aHeader, bool &aReceive)
{
    Error          error = kErrorNone;
    HopByHopHeader hbhHeader;
    Option         option;
    OffsetRange    offsetRange;

    offsetRange.InitFromMessageOffsetToEnd(aMessage);

    SuccessOrExit(error = ReadHopByHopHeader(aMessage, offsetRange, hbhHeader));

    for (; !offsetRange.IsEmpty(); offsetRange.AdvanceOffset(option.GetSize()))
    {
        SuccessOrExit(error = option.ParseFrom(aMessage, offsetRange));

        if (option.IsPadding())
        {
            continue;
        }

        if (option.GetType() == MplOption::kType)
        {
            SuccessOrExit(error = mMpl.ProcessOption(aMessage, offsetRange, aHeader.GetSource(), aReceive));
            continue;
        }

        VerifyOrExit(option.GetAction() == Option::kActionSkip, error = kErrorDrop);
    }

    aMessage.SetOffset(offsetRange.GetEndOffset());

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

Error Ip6::HandleFragment(Message &aMessage)
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

    LogInfo("Fragment with id %lu received > %u bytes, offset %u", ToUlong(fragmentHeader.GetIdentification()),
            payloadFragment, offset);

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

        IgnoreError(HandleDatagram(OwnedPtr<Message>(message), /* aIsReassembled */ true));
    }

exit:
    if (error != kErrorDrop && error != kErrorNone && isFragmented)
    {
        if (message != nullptr)
        {
            mReassemblyList.DequeueAndFree(*message);
        }

        LogWarnOnError(error, "reassemble");
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
        if (now - message.GetTimestamp() >= TimeMilli::SecToMsec(kReassemblyTimeout))
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

    error = mIcmp.SendError(aIcmpType, aIcmpCode, messageInfo, aMessage);

exit:
    LogWarnOnError(error, "send ICMP");
    OT_UNUSED_VARIABLE(error);
}

#else
Error Ip6::FragmentDatagram(Message &aMessage, uint8_t aIpProto)
{
    OT_UNUSED_VARIABLE(aIpProto);

    EnqueueDatagram(aMessage);

    return kErrorNone;
}

Error Ip6::HandleFragment(Message &aMessage)
{
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
                                  const Header      &aHeader,
                                  uint8_t           &aNextHeader,
                                  bool              &aReceive)
{
    Error           error = kErrorNone;
    ExtensionHeader extHeader;

    while (aReceive || aNextHeader == kProtoHopOpts)
    {
        SuccessOrExit(error = aMessagePtr->Read(aMessagePtr->GetOffset(), extHeader));

        switch (aNextHeader)
        {
        case kProtoHopOpts:
        case kProtoDstOpts:
            SuccessOrExit(error = HandleOptions(*aMessagePtr, aHeader, aReceive));
            break;

        case kProtoFragment:
            IgnoreError(PassToHost(aMessagePtr, aHeader, aNextHeader, aReceive, Message::kCopyToUse));
            SuccessOrExit(error = HandleFragment(*aMessagePtr));
            break;

        case kProtoIp6:
            ExitNow();

        case kProtoRouting:
        case kProtoNone:
            ExitNow(error = kErrorDrop);

        default:
            ExitNow();
        }

        aNextHeader = extHeader.GetNextHeader();
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

Error Ip6::Receive(Header            &aIp6Header,
                   OwnedPtr<Message> &aMessagePtr,
                   uint8_t            aIpProto,
                   Message::Ownership aMessageOwnership)
{
    Error             error = kErrorNone;
    OwnedPtr<Message> messagePtr;
    MessageInfo       messageInfo;

    messageInfo.Clear();
    messageInfo.SetPeerAddr(aIp6Header.GetSource());
    messageInfo.SetSockAddr(aIp6Header.GetDestination());
    messageInfo.SetHopLimit(aIp6Header.GetHopLimit());
    messageInfo.SetEcn(aIp6Header.GetEcn());

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
        error = mTcp.HandleMessage(aIp6Header, *messagePtr, messageInfo);
        break;
#endif
    case kProtoUdp:
        error = mUdp.HandleMessage(*messagePtr, messageInfo);
        break;

    case kProtoIcmp6:
        error = mIcmp.HandleMessage(*messagePtr, messageInfo);
        break;

    default:
        break;
    }

exit:
    LogWarnOnError(error, "handle payload");
    return error;
}

Error Ip6::PassToHost(OwnedPtr<Message> &aMessagePtr,
                      const Header      &aHeader,
                      uint8_t            aIpProto,
                      bool               aReceive,
                      Message::Ownership aMessageOwnership)
{
    // This method passes the message to host by invoking the
    // registered IPv6 receive callback. When NAT64 is enabled, it
    // may also perform translation and invoke IPv4 receive
    // callback.

    Error             error = kErrorNone;
    OwnedPtr<Message> messagePtr;

    VerifyOrExit(aMessagePtr->IsLoopbackToHostAllowed());

    VerifyOrExit(mReceiveCallback.IsSet(), error = kErrorNoRoute);

    // Do not pass IPv6 packets that exceed kMinimalMtu.
    VerifyOrExit(aMessagePtr->GetLength() <= kMinimalMtu, error = kErrorDrop);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_REACHABILITY_CHECK_ICMP6_ERROR_ENABLE
    if (!aReceive)
    {
        Get<BorderRouter::RoutingManager>().CheckReachabilityToSendIcmpError(*aMessagePtr, aHeader);
    }
#endif

    // If the sender used mesh-local address as source, do not pass to
    // host unless this message is intended for this device itself.
    if (Get<Mle::Mle>().IsMeshLocalAddress(aHeader.GetSource()))
    {
        VerifyOrExit(aReceive, error = kErrorDrop);
    }

    if (mReceiveFilterEnabled && aReceive)
    {
        switch (aIpProto)
        {
        case kProtoIcmp6:
            if (mIcmp.ShouldHandleEchoRequest(aHeader.GetDestination()))
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
        VerifyOrExit(mIp4ReceiveCallback.IsSet(), error = kErrorNoRoute);
        // Pass message to callback transferring its ownership.
        mIp4ReceiveCallback.Invoke(messagePtr.Release());
        ExitNow();
    }
#endif

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    UpdateBorderRoutingCounters(aHeader, messagePtr->GetLength(), /* aIsInbound */ false);
#endif

#if OPENTHREAD_CONFIG_IP6_RESTRICT_FORWARDING_LARGER_SCOPE_MCAST_WITH_LOCAL_SRC
    // Some platforms (e.g. Android) currently doesn't restrict link-local/mesh-local source
    // addresses when forwarding multicast packets.
    // For a multicast packet sent from link-local/mesh-local address to scope larger
    // than realm-local, set the hop limit to 1 before sending to host, so this packet
    // will not be forwarded by host.
    if (aHeader.GetDestination().IsMulticastLargerThanRealmLocal() &&
        (aHeader.GetSource().IsLinkLocalUnicast() || (Get<Mle::Mle>().IsMeshLocalAddress(aHeader.GetSource()))))
    {
        messagePtr->Write<uint8_t>(Header::kHopLimitFieldOffset, 1);
    }
#endif

    // Pass message to callback transferring its ownership.
    mReceiveCallback.Invoke(messagePtr.Release());

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

Error Ip6::HandleDatagram(OwnedPtr<Message> aMessagePtr, bool aIsReassembled)
{
    Error   error;
    Header  header;
    bool    receive;
    bool    forwardThread;
    bool    forwardHost;
    uint8_t nextHeader;

    receive       = false;
    forwardThread = false;
    forwardHost   = false;

    SuccessOrExit(error = header.ParseFrom(*aMessagePtr));

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

        // Always forward multicast packets to host network stack
        forwardHost = true;

        if ((aMessagePtr->IsOriginThreadNetif() || aMessagePtr->GetMulticastLoop()) &&
            Get<ThreadNetif>().IsMulticastSubscribed(header.GetDestination()))
        {
            receive = true;
        }
    }
    else
    {
        // Destination is unicast

        if (Get<ThreadNetif>().HasUnicastAddress(header.GetDestination()))
        {
            receive = true;
        }
        else if (!aMessagePtr->IsOriginThreadNetif() || !header.GetDestination().IsLinkLocalUnicast())
        {
            if (header.GetDestination().IsLinkLocalUnicast())
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
    nextHeader = header.GetNextHeader();
    SuccessOrExit(error = HandleExtensionHeaders(aMessagePtr, header, nextHeader, receive));

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

        IgnoreError(HandleDatagram(messagePtr.PassOwnership(), aIsReassembled));

        receive     = false;
        forwardHost = false;
    }

    if ((forwardHost || receive) && !aIsReassembled)
    {
        error = PassToHost(aMessagePtr, header, nextHeader, receive,
                           (receive || forwardThread) ? Message::kCopyToUse : Message::kTakeCustody);
    }

    if (receive)
    {
        error = Receive(header, aMessagePtr, nextHeader, forwardThread ? Message::kCopyToUse : Message::kTakeCustody);
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

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if (mTmfOriginFilterEnabled)
#endif
        {
            if (aMessagePtr->IsOriginHostUntrusted() && (nextHeader == kProtoUdp))
            {
                Udp::Header udpHeader;

                SuccessOrExit(error = aMessagePtr->Read(aMessagePtr->GetOffset(), udpHeader));

                if (udpHeader.GetDestinationPort() == Tmf::kUdpPort)
                {
                    LogNote("Dropping TMF message from untrusted origin");
                    ExitNow(error = kErrorDrop);
                }
            }
        }

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
        bool    newAddrIsPreferred = false;
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

        if (addr.GetAddress() == aDestination)
        {
            // Rule 1: Prefer same address
            bestAddr = &addr;
            ExitNow();
        }

        if (bestAddr == nullptr)
        {
            newAddrIsPreferred = true;
        }
        else if (addr.GetScope() < bestAddr->GetScope())
        {
            // Rule 2: Prefer appropriate scope
            newAddrIsPreferred = (addr.GetScope() >= overrideScope);
        }
        else if (addr.GetScope() > bestAddr->GetScope())
        {
            newAddrIsPreferred = (bestAddr->GetScope() < overrideScope);
        }
        else if (addr.mPreferred != bestAddr->mPreferred)
        {
            // Rule 3: Avoid deprecated addresses
            newAddrIsPreferred = addr.mPreferred;
        }
        else if (matchLen > bestMatchLen)
        {
            // Rule 6: Prefer matching label
            // Rule 7: Prefer public address
            // Rule 8: Use longest prefix matching

            newAddrIsPreferred = true;
        }
        else if ((matchLen == bestMatchLen) && (destIsRloc == Get<Mle::Mle>().IsRoutingLocator(addr.GetAddress())))
        {
            // Additional rule: Prefer RLOC source for RLOC destination, EID source for anything else
            newAddrIsPreferred = true;
        }

        if (newAddrIsPreferred)
        {
            bestAddr     = &addr;
            bestMatchLen = matchLen;

            // Infer destination scope based on prefix match
            if (bestMatchLen >= bestAddr->mPrefixLength)
            {
                destScope = bestAddr->GetScope();
            }
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
    static constexpr uint8_t kPrefixLength = 48;

    otPacketsAndBytes *counter         = nullptr;
    otPacketsAndBytes *internetCounter = nullptr;

    VerifyOrExit(!aHeader.GetSource().IsLinkLocalUnicast());
    VerifyOrExit(!aHeader.GetDestination().IsLinkLocalUnicast());
    VerifyOrExit(!Get<Mle::Mle>().IsMeshLocalAddress(aHeader.GetSource()));
    VerifyOrExit(!Get<Mle::Mle>().IsMeshLocalAddress(aHeader.GetDestination()));

    if (aIsInbound)
    {
        VerifyOrExit(!Get<Netif>().HasUnicastAddress(aHeader.GetSource()));

        if (!aHeader.GetSource().MatchesPrefix(aHeader.GetDestination().GetPrefix().m8, kPrefixLength))
        {
            internetCounter = &mBrCounters.mInboundInternet;
        }

        if (aHeader.GetDestination().IsMulticast())
        {
            VerifyOrExit(aHeader.GetDestination().IsMulticastLargerThanRealmLocal());
            counter = &mBrCounters.mInboundMulticast;
        }
        else
        {
            counter = &mBrCounters.mInboundUnicast;
        }
    }
    else
    {
        VerifyOrExit(!Get<Netif>().HasUnicastAddress(aHeader.GetDestination()));

        if (!aHeader.GetSource().MatchesPrefix(aHeader.GetDestination().GetPrefix().m8, kPrefixLength))
        {
            internetCounter = &mBrCounters.mOutboundInternet;
        }

        if (aHeader.GetDestination().IsMulticast())
        {
            VerifyOrExit(aHeader.GetDestination().IsMulticastLargerThanRealmLocal());
            counter = &mBrCounters.mOutboundMulticast;
        }
        else
        {
            counter = &mBrCounters.mOutboundUnicast;
        }
    }

exit:

    if (counter)
    {
        counter->mPackets++;
        counter->mBytes += aMessageLength;
    }
    if (internetCounter)
    {
        internetCounter->mPackets++;
        internetCounter->mBytes += aMessageLength;
    }
}

#endif // OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE

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

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kEcnNotCapable);
        ValidateNextEnum(kEcnCapable1);
        ValidateNextEnum(kEcnCapable0);
        ValidateNextEnum(kEcnMarked);
    };

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
