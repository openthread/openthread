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
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
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

static const IcmpType sForwardICMPTypes[] = {
    IcmpType::kTypeDstUnreach,       IcmpType::kTypePacketToBig, IcmpType::kTypeTimeExceeded,
    IcmpType::kTypeParameterProblem, IcmpType::kTypeEchoRequest, IcmpType::kTypeEchoReply,
};

namespace ot {
namespace Ip6 {

RegisterLogModule("Ip6");

Ip6::Ip6(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mForwardingEnabled(false)
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

Message *Ip6::NewMessage(uint16_t aReserved, const Message::Settings &aSettings)
{
    return Get<MessagePool>().Allocate(
        Message::kTypeIp6, sizeof(Header) + sizeof(HopByHopHeader) + sizeof(OptionMpl) + aReserved, aSettings);
}

Message *Ip6::NewMessage(const uint8_t *aData, uint16_t aDataLength, const Message::Settings &aSettings)
{
    Message *message = Get<MessagePool>().Allocate(Message::kTypeIp6, /* aReserveHeader */ 0, aSettings);

    VerifyOrExit(message != nullptr);

    if (message->AppendBytes(aData, aDataLength) != kErrorNone)
    {
        message->Free();
        message = nullptr;
    }

exit:
    return message;
}

Message *Ip6::NewMessage(const uint8_t *aData, uint16_t aDataLength)
{
    Message          *message = nullptr;
    Message::Priority priority;

    SuccessOrExit(GetDatagramPriority(aData, aDataLength, priority));
    message = NewMessage(aData, aDataLength, Message::Settings(Message::kWithLinkSecurity, priority));

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

Error Ip6::GetDatagramPriority(const uint8_t *aData, uint16_t aDataLen, Message::Priority &aPriority)
{
    Error         error = kErrorNone;
    const Header *header;

    VerifyOrExit((aData != nullptr) && (aDataLen >= sizeof(Header)), error = kErrorInvalidArgs);

    header = reinterpret_cast<const Header *>(aData);
    VerifyOrExit(header->IsValid(), error = kErrorParse);
    VerifyOrExit(sizeof(Header) + header->GetPayloadLength() == aDataLen, error = kErrorParse);

    aPriority = DscpToPriority(header->GetDscp());

exit:
    return error;
}

Error Ip6::AddMplOption(Message &aMessage, Header &aHeader)
{
    Error          error = kErrorNone;
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
        SuccessOrExit(error = aMessage.PrependBytes(&padOption, padOption.GetTotalLength()));
    }

    SuccessOrExit(error = aMessage.PrependBytes(&mplOption, mplOption.GetTotalLength()));
    SuccessOrExit(error = aMessage.Prepend(hbhHeader));
    aHeader.SetPayloadLength(aHeader.GetPayloadLength() + sizeof(hbhHeader) + sizeof(mplOption));
    aHeader.SetNextHeader(kProtoHopOpts);

exit:
    return error;
}

Error Ip6::AddTunneledMplOption(Message &aMessage, Header &aHeader)
{
    Error          error = kErrorNone;
    Header         tunnelHeader;
    const Address *source;

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
            uint16_t       hbhLength = 0;
            OptionMpl      mplOption;

            // read existing hop-by-hop option header
            SuccessOrExit(error = aMessage.Read(0, hbh));
            hbhLength = (hbh.GetLength() + 1) * 8;

            VerifyOrExit(hbhLength <= aHeader.GetPayloadLength(), error = kErrorParse);

            // increase existing hop-by-hop option header length by 8 bytes
            hbh.SetLength(hbh.GetLength() + 1);
            aMessage.Write(0, hbh);

            // make space for MPL Option + padding by shifting hop-by-hop option header
            SuccessOrExit(error = aMessage.PrependBytes(nullptr, 8));
            aMessage.CopyTo(8, 0, hbhLength, aMessage);

            // insert MPL Option
            mMpl.InitOption(mplOption, aHeader.GetSource());
            aMessage.WriteBytes(hbhLength, &mplOption, mplOption.GetTotalLength());

            // insert Pad Option (if needed)
            if (mplOption.GetTotalLength() % 8)
            {
                OptionPadN padOption;
                padOption.Init(8 - (mplOption.GetTotalLength() % 8));
                aMessage.WriteBytes(hbhLength + mplOption.GetTotalLength(), &padOption, padOption.GetTotalLength());
            }

            // increase IPv6 Payload Length
            aHeader.SetPayloadLength(aHeader.GetPayloadLength() + 8);
        }
        else
        {
            SuccessOrExit(error = AddMplOption(aMessage, aHeader));
        }

        SuccessOrExit(error = aMessage.Prepend(aHeader));
    }
    else
    {
#if OPENTHREAD_FTD
        if (aHeader.GetDestination().IsMulticastLargerThanRealmLocal() &&
            Get<ChildTable>().HasSleepyChildWithAddress(aHeader.GetDestination()))
        {
            Message *messageCopy = nullptr;

            if ((messageCopy = aMessage.Clone()) != nullptr)
            {
                IgnoreError(HandleDatagram(*messageCopy, kFromHostDisallowLoopBack));
                LogInfo("Message copy for indirect transmission to sleepy children");
            }
            else
            {
                LogWarn("No enough buffer for message copy for indirect transmission to sleepy children");
            }
        }
#endif

        SuccessOrExit(error = AddTunneledMplOption(aMessage, aHeader));
    }

exit:
    return error;
}

Error Ip6::RemoveMplOption(Message &aMessage)
{
    Error          error = kErrorNone;
    Header         ip6Header;
    HopByHopHeader hbh;
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
    endOffset = offset + (hbh.GetLength() + 1) * 8;
    VerifyOrExit(aMessage.GetLength() >= endOffset, error = kErrorParse);

    offset += sizeof(hbh);

    while (offset < endOffset)
    {
        OptionHeader option;

        IgnoreError(aMessage.Read(offset, option));

        switch (option.GetType())
        {
        case OptionMpl::kType:
            // if multiple MPL options exist, discard packet
            VerifyOrExit(mplOffset == 0, error = kErrorParse);

            mplOffset = offset;
            mplLength = option.GetLength();

            VerifyOrExit(mplLength <= sizeof(OptionMpl) - sizeof(OptionHeader), error = kErrorParse);

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
    VerifyOrExit(offset == endOffset, error = kErrorParse);

    if (remove)
    {
        // last IPv6 Option, shrink HBH Option header
        uint8_t buf[8];

        offset = endOffset - sizeof(buf);

        while (offset >= sizeof(buf))
        {
            IgnoreError(aMessage.Read(offset - sizeof(buf), buf));
            aMessage.Write(offset, buf);
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
            aMessage.Write(sizeof(ip6Header), hbh);
        }

        ip6Header.SetPayloadLength(ip6Header.GetPayloadLength() - sizeof(buf));
        aMessage.Write(0, ip6Header);
    }
    else if (mplOffset != 0)
    {
        // replace MPL Option with PadN Option
        OptionPadN padOption;

        padOption.Init(sizeof(OptionHeader) + mplLength);
        aMessage.WriteBytes(mplOffset, &padOption, padOption.GetTotalLength());
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
    uint16_t payloadLength = aMessage.GetLength();

    header.InitVersionTrafficClassFlow();
    header.SetDscp(PriorityToDscp(aMessage.GetPriority()));
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
#if OPENTHREAD_FTD
        if (Get<ChildTable>().HasSleepyChildWithAddress(header.GetDestination()))
        {
            Message *messageCopy = aMessage.Clone();

            if (messageCopy != nullptr)
            {
                LogInfo("Message copy for indirect transmission to sleepy children");
                EnqueueDatagram(*messageCopy);
            }
            else
            {
                LogWarn("No enough buffer for message copy for indirect transmission to sleepy children");
            }
        }
#endif

        SuccessOrExit(error = AddTunneledMplOption(aMessage, header));
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
        IgnoreError(HandleDatagram(*message, kFromHostAllowLoopBack));
    }
}

Error Ip6::HandleOptions(Message &aMessage, Header &aHeader, bool aIsOutbound, bool &aReceive)
{
    Error          error = kErrorNone;
    HopByHopHeader hbhHeader;
    OptionHeader   optionHeader;
    uint16_t       endOffset;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), hbhHeader));
    endOffset = aMessage.GetOffset() + (hbhHeader.GetLength() + 1) * 8;

    VerifyOrExit(endOffset <= aMessage.GetLength(), error = kErrorParse);

    aMessage.MoveOffset(sizeof(optionHeader));

    while (aMessage.GetOffset() < endOffset)
    {
        SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), optionHeader));

        if (optionHeader.GetType() == OptionPad1::kType)
        {
            aMessage.MoveOffset(sizeof(OptionPad1));
            continue;
        }

        VerifyOrExit(aMessage.GetOffset() + sizeof(optionHeader) + optionHeader.GetLength() <= endOffset,
                     error = kErrorParse);

        switch (optionHeader.GetType())
        {
        case OptionMpl::kType:
            SuccessOrExit(error = mMpl.ProcessOption(aMessage, aHeader.GetSource(), aIsOutbound, aReceive));
            break;

        default:
            switch (optionHeader.GetAction())
            {
            case OptionHeader::kActionSkip:
                break;

            case OptionHeader::kActionDiscard:
                ExitNow(error = kErrorDrop);

            case OptionHeader::kActionForceIcmp:
                // TODO: send icmp error
                ExitNow(error = kErrorDrop);

            case OptionHeader::kActionIcmp:
                // TODO: send icmp error
                ExitNow(error = kErrorDrop);
            }

            break;
        }

        aMessage.MoveOffset(sizeof(optionHeader) + optionHeader.GetLength());
    }

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

        VerifyOrExit((fragment = NewMessage(0)) != nullptr, error = kErrorNoBufs);
        IgnoreError(fragment->SetPriority(aMessage.GetPriority()));
        SuccessOrExit(error = fragment->SetLength(aMessage.GetOffset() + sizeof(fragmentHeader) + payloadFragment));

        header.SetPayloadLength(payloadFragment + sizeof(fragmentHeader));
        fragment->Write(0, header);

        fragment->SetOffset(aMessage.GetOffset());
        fragment->Write(aMessage.GetOffset(), fragmentHeader);

        VerifyOrExit(aMessage.CopyTo(aMessage.GetOffset() + FragmentHeader::FragmentOffsetToBytes(offset),
                                     aMessage.GetOffset() + sizeof(fragmentHeader), payloadFragment,
                                     *fragment) == static_cast<int>(payloadFragment),
                     error = kErrorNoBufs);

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

Error Ip6::HandleFragment(Message &aMessage, MessageOrigin aOrigin, MessageInfo &aMessageInfo)
{
    Error          error = kErrorNone;
    Header         header, headerBuffer;
    FragmentHeader fragmentHeader;
    Message       *message         = nullptr;
    uint16_t       offset          = 0;
    uint16_t       payloadFragment = 0;
    int            assertValue     = 0;
    bool           isFragmented    = true;

    OT_UNUSED_VARIABLE(assertValue);

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
        VerifyOrExit((message = NewMessage(0)) != nullptr, error = kErrorNoBufs);
        mReassemblyList.Enqueue(*message);
        SuccessOrExit(error = message->SetLength(aMessage.GetOffset()));

        message->SetTimestampToNow();
        message->SetOffset(0);
        message->SetDatagramTag(fragmentHeader.GetIdentification());

        // copying the non-fragmentable header to the fragmentation buffer
        assertValue = aMessage.CopyTo(0, 0, aMessage.GetOffset(), *message);
        OT_ASSERT(assertValue == aMessage.GetOffset());

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kIp6FragmentReassembler);
    }

    // increase message buffer if necessary
    if (message->GetLength() < offset + payloadFragment + aMessage.GetOffset())
    {
        SuccessOrExit(error = message->SetLength(offset + payloadFragment + aMessage.GetOffset()));
    }

    // copy the fragment payload into the message buffer
    assertValue = aMessage.CopyTo(aMessage.GetOffset() + sizeof(fragmentHeader), aMessage.GetOffset() + offset,
                                  payloadFragment, *message);
    OT_ASSERT(assertValue == static_cast<int>(payloadFragment));

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

        IgnoreError(HandleDatagram(*message, aOrigin, aMessageInfo.mLinkInfo, /* aIsReassembled */ true));
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

Error Ip6::HandleFragment(Message &aMessage, MessageOrigin aOrigin, MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aOrigin);
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

Error Ip6::HandleExtensionHeaders(Message      &aMessage,
                                  MessageOrigin aOrigin,
                                  MessageInfo  &aMessageInfo,
                                  Header       &aHeader,
                                  uint8_t      &aNextHeader,
                                  bool         &aReceive)
{
    Error           error      = kErrorNone;
    bool            isOutbound = (aOrigin != kFromThreadNetif);
    ExtensionHeader extHeader;

    while (aReceive || aNextHeader == kProtoHopOpts)
    {
        SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), extHeader));

        switch (aNextHeader)
        {
        case kProtoHopOpts:
            SuccessOrExit(error = HandleOptions(aMessage, aHeader, isOutbound, aReceive));
            break;

        case kProtoFragment:
            IgnoreError(ProcessReceiveCallback(aMessage, aOrigin, aMessageInfo, aNextHeader,
                                               /* aAllowReceiveFilter */ false, Message::kCopyToUse));
            SuccessOrExit(error = HandleFragment(aMessage, aOrigin, aMessageInfo));
            break;

        case kProtoDstOpts:
            SuccessOrExit(error = HandleOptions(aMessage, aHeader, isOutbound, aReceive));
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

Error Ip6::HandlePayload(Header            &aIp6Header,
                         Message           &aMessage,
                         MessageInfo       &aMessageInfo,
                         uint8_t            aIpProto,
                         Message::Ownership aMessageOwnership)
{
#if !OPENTHREAD_CONFIG_TCP_ENABLE
    OT_UNUSED_VARIABLE(aIp6Header);
#endif

    Error    error   = kErrorNone;
    Message *message = (aMessageOwnership == Message::kTakeCustody) ? &aMessage : nullptr;

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

    if (aMessageOwnership == Message::kCopyToUse)
    {
        VerifyOrExit((message = aMessage.Clone()) != nullptr, error = kErrorNoBufs);
    }

    switch (aIpProto)
    {
#if OPENTHREAD_CONFIG_TCP_ENABLE
    case kProtoTcp:
        error = mTcp.HandleMessage(aIp6Header, *message, aMessageInfo);
        if (error == kErrorDrop)
        {
            LogNote("Error TCP Checksum");
        }
        break;
#endif
    case kProtoUdp:
        error = mUdp.HandleMessage(*message, aMessageInfo);
        if (error == kErrorDrop)
        {
            LogNote("Error UDP Checksum");
        }
        break;

    case kProtoIcmp6:
        error = mIcmp.HandleMessage(*message, aMessageInfo);
        break;

    default:
        break;
    }

exit:
    if (error != kErrorNone)
    {
        LogNote("Failed to handle payload: %s", ErrorToString(error));
    }

    FreeMessage(message);

    return error;
}

Error Ip6::ProcessReceiveCallback(Message           &aMessage,
                                  MessageOrigin      aOrigin,
                                  const MessageInfo &aMessageInfo,
                                  uint8_t            aIpProto,
                                  bool               aAllowReceiveFilter,
                                  Message::Ownership aMessageOwnership)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;
#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    Header header;

    IgnoreError(header.ParseFrom(aMessage));
#endif

    // `message` points to the `Message` instance we own in this
    // method. If we can take ownership of `aMessage`, we use it as
    // `message`. Otherwise, we may create a clone of it and use as
    // `message`. `message` variable will be set to `nullptr` if the
    // message ownership is transferred to an invoked callback. At
    // the end of this method we free `message` if it is not `nullptr`
    // indicating it was not passed to a callback.

    if (aMessageOwnership == Message::kTakeCustody)
    {
        message = &aMessage;
    }

    VerifyOrExit(aOrigin != kFromHostDisallowLoopBack, error = kErrorNoRoute);

    VerifyOrExit(mReceiveIp6DatagramCallback.IsSet(), error = kErrorNoRoute);

    // Do not forward IPv6 packets that exceed kMinimalMtu.
    VerifyOrExit(aMessage.GetLength() <= kMinimalMtu, error = kErrorDrop);

    if (mIsReceiveIp6FilterEnabled && aAllowReceiveFilter)
    {
#if !OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
        // do not pass messages sent to an RLOC/ALOC, except Service Locator
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

                IgnoreError(aMessage.Read(aMessage.GetOffset(), icmp));

                // do not pass ICMP Echo Request messages
                VerifyOrExit(icmp.GetType() != Icmp::Header::kTypeEchoRequest, error = kErrorDrop);
            }

            break;

        case kProtoUdp:
        {
            Udp::Header udp;

            IgnoreError(aMessage.Read(aMessage.GetOffset(), udp));
            VerifyOrExit(Get<Udp>().ShouldUsePlatformUdp(udp.GetDestinationPort()) &&
                             !Get<Udp>().IsPortInUse(udp.GetDestinationPort()),
                         error = kErrorNoRoute);
            break;
        }

#if OPENTHREAD_CONFIG_TCP_ENABLE
        // Do not pass TCP message to avoid dual processing from both openthread and POSIX tcp stacks
        case kProtoTcp:
        {
            error = kErrorNoRoute;
            goto exit;
        }
#endif

        default:
            break;
        }
    }

    switch (aMessageOwnership)
    {
    case Message::kTakeCustody:
        break;

    case Message::kCopyToUse:
        message = aMessage.Clone();

        if (message == nullptr)
        {
            LogWarn("No buff to clone msg (len: %d) to pass to host", aMessage.GetLength());
            ExitNow(error = kErrorNoBufs);
        }

        break;
    }

    IgnoreError(RemoveMplOption(*message));

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    switch (Get<Nat64::Translator>().TranslateFromIp6(aMessage))
    {
    case Nat64::Translator::kNotTranslated:
        break;
    case Nat64::Translator::kDrop:
        ExitNow(error = kErrorDrop);
    case Nat64::Translator::kForward:
        VerifyOrExit(mReceiveIp4DatagramCallback.IsSet(), error = kErrorNoRoute);
        // Pass message to callback transferring its ownership.
        mReceiveIp4DatagramCallback.Invoke(message);
        message = nullptr;
        ExitNow();
    }
#endif

    // Pass message to callback transferring its ownership.
    mReceiveIp6DatagramCallback.Invoke(message);
    message = nullptr;

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    UpdateBorderRoutingCounters(header, aMessage.GetLength(), /* aIsInbound */ false);
#endif

exit:
    FreeMessage(message);
    return error;
}

Error Ip6::SendRaw(Message &aMessage, bool aAllowLoopBackToHost)
{
    Error  error = kErrorNone;
    Header header;
    bool   freed = false;

    SuccessOrExit(error = header.ParseFrom(aMessage));
    VerifyOrExit(!header.GetSource().IsMulticast(), error = kErrorInvalidSourceAddress);

    if (header.GetDestination().IsMulticast())
    {
        SuccessOrExit(error = InsertMplOption(aMessage, header));
    }

    error = HandleDatagram(aMessage, aAllowLoopBackToHost ? kFromHostAllowLoopBack : kFromHostDisallowLoopBack);
    freed = true;

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    UpdateBorderRoutingCounters(header, aMessage.GetLength(), /* aIsInbound */ true);
#endif

exit:

    if (!freed)
    {
        aMessage.Free();
    }

    return error;
}

Error Ip6::HandleDatagram(Message &aMessage, MessageOrigin aOrigin, const void *aLinkMessageInfo, bool aIsReassembled)
{
    Error       error;
    MessageInfo messageInfo;
    Header      header;
    bool        receive;
    bool        forwardThread;
    bool        forwardHost;
    bool        shouldFreeMessage;
    uint8_t     nextHeader;

start:
    receive           = false;
    forwardThread     = false;
    forwardHost       = false;
    shouldFreeMessage = true;

    SuccessOrExit(error = header.ParseFrom(aMessage));

    messageInfo.Clear();
    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetEcn(header.GetEcn());
    messageInfo.SetLinkInfo(aLinkMessageInfo);

    // determine destination of packet
    if (header.GetDestination().IsMulticast())
    {
        if (aOrigin == kFromThreadNetif)
        {
#if OPENTHREAD_FTD
            if (header.GetDestination().IsMulticastLargerThanRealmLocal() &&
                Get<ChildTable>().HasSleepyChildWithAddress(header.GetDestination()))
            {
                forwardThread = true;
            }
#endif
        }
        else
        {
            forwardThread = true;
        }

        forwardHost = header.GetDestination().IsMulticastLargerThanRealmLocal();

        if (((aOrigin == kFromThreadNetif) || aMessage.GetMulticastLoop()) &&
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
        // unicast
        if (Get<ThreadNetif>().HasUnicastAddress(header.GetDestination()))
        {
            receive = true;
        }
        else if (!header.GetDestination().IsLinkLocal())
        {
            forwardThread = true;
        }
        else if (aOrigin != kFromThreadNetif)
        {
            forwardThread = true;
        }

        if (forwardThread && !ShouldForwardToThread(messageInfo, aOrigin))
        {
            forwardThread = false;
            forwardHost   = true;
        }
    }

    // never forward reassembled frames as they were already delivered as fragments
    if (aIsReassembled)
    {
        forwardHost = false;
    }
    aMessage.SetOffset(sizeof(header));

    // process IPv6 Extension Headers
    nextHeader = static_cast<uint8_t>(header.GetNextHeader());
    SuccessOrExit(error = HandleExtensionHeaders(aMessage, aOrigin, messageInfo, header, nextHeader, receive));

    // process IPv6 Payload
    if (receive)
    {
        if (nextHeader == kProtoIp6)
        {
            // Remove encapsulating header and start over.
            aMessage.RemoveHeader(aMessage.GetOffset());
            Get<MeshForwarder>().LogMessage(MeshForwarder::kMessageReceive, aMessage);
            goto start;
        }
        if (!aIsReassembled)
        {
            error = ProcessReceiveCallback(aMessage, aOrigin, messageInfo, nextHeader,
                                           /* aAllowReceiveFilter */ !forwardHost, Message::kCopyToUse);

            if ((error == kErrorNone || error == kErrorNoRoute) && forwardHost)
            {
                forwardHost = false;
            }
        }
        error             = HandlePayload(header, aMessage, messageInfo, nextHeader,
                                          (forwardThread || forwardHost ? Message::kCopyToUse : Message::kTakeCustody));
        shouldFreeMessage = forwardThread || forwardHost;
    }

    if (forwardHost)
    {
        // try passing to host
        error = ProcessReceiveCallback(aMessage, aOrigin, messageInfo, nextHeader, /* aAllowReceiveFilter */ false,
                                       forwardThread ? Message::kCopyToUse : Message::kTakeCustody);
        shouldFreeMessage = forwardThread;
    }

    if (forwardThread)
    {
        uint8_t hopLimit;

        if (aOrigin == kFromThreadNetif)
        {
            VerifyOrExit(mForwardingEnabled);
            header.SetHopLimit(header.GetHopLimit() - 1);
        }

        VerifyOrExit(header.GetHopLimit() > 0, error = kErrorDrop);

        hopLimit = header.GetHopLimit();
        aMessage.Write(Header::kHopLimitFieldOffset, hopLimit);

        if (nextHeader == kProtoIcmp6)
        {
            uint8_t icmpType;
            bool    isAllowedType = false;

            SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), icmpType));
            for (IcmpType type : sForwardICMPTypes)
            {
                if (icmpType == type)
                {
                    isAllowedType = true;
                    break;
                }
            }
            VerifyOrExit(isAllowedType, error = kErrorDrop);
        }

#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if ((aOrigin == kFromHostDisallowLoopBack) && (nextHeader == kProtoUdp))
        {
            uint16_t destPort;

            SuccessOrExit(error = aMessage.Read(aMessage.GetOffset() + Udp::Header::kDestPortFieldOffset, destPort));
            destPort = HostSwap16(destPort);

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
        aMessage.ClearRadioType();
#endif

        // `SendMessage()` takes custody of message in the success case
        SuccessOrExit(error = Get<ThreadNetif>().SendMessage(aMessage));
        shouldFreeMessage = false;
    }

exit:

    if (shouldFreeMessage)
    {
        aMessage.Free();
    }

    return error;
}

bool Ip6::ShouldForwardToThread(const MessageInfo &aMessageInfo, MessageOrigin aOrigin) const
{
    bool shouldForward = false;

    if (aMessageInfo.GetSockAddr().IsMulticast() || aMessageInfo.GetSockAddr().IsLinkLocal())
    {
        shouldForward = true;
    }
    else if (IsOnLink(aMessageInfo.GetSockAddr()))
    {
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
        shouldForward = ((aOrigin == kFromHostDisallowLoopBack) ||
                         !Get<BackboneRouter::Manager>().ShouldForwardDuaToBackbone(aMessageInfo.GetSockAddr()));
#else
        OT_UNUSED_VARIABLE(aOrigin);
        shouldForward = true;
#endif
    }
    else if (Get<ThreadNetif>().RouteLookup(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr()) == kErrorNone)
    {
        shouldForward = true;
    }

    return shouldForward;
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
        else if (addr.mPreferred && !bestAddr->mPreferred)
        {
            // Rule 3: Avoid deprecated addresses
            bestAddr     = &addr;
            bestMatchLen = matchLen;
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
    bool rval = false;

    if (Get<ThreadNetif>().IsOnMesh(aAddress))
    {
        ExitNow(rval = true);
    }

    for (const Netif::UnicastAddress &cur : Get<ThreadNetif>().GetUnicastAddresses())
    {
        if (cur.GetAddress().PrefixMatch(aAddress) >= cur.mPrefixLength)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
void Ip6::UpdateBorderRoutingCounters(const Header &aHeader, uint16_t aMessageLength, bool aIsInbound)
{
    otPacketsAndBytes *counter = nullptr;

    VerifyOrExit(!aHeader.GetSource().IsLinkLocal());
    VerifyOrExit(!aHeader.GetDestination().IsLinkLocal());
    VerifyOrExit(aHeader.GetSource().GetPrefix() != Get<Mle::Mle>().GetMeshLocalPrefix());
    VerifyOrExit(aHeader.GetDestination().GetPrefix() != Get<Mle::Mle>().GetMeshLocalPrefix());

    if (aIsInbound)
    {
        VerifyOrExit(!Get<Netif>().HasUnicastAddress(aHeader.GetSource()));

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
