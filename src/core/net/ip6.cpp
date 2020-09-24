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
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "net/checksum.hpp"
#include "net/icmp6.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_filter.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"

namespace ot {
namespace Ip6 {

Ip6::Ip6(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mForwardingEnabled(false)
    , mIsReceiveIp6FilterEnabled(false)
    , mReceiveIp6DatagramCallback(nullptr)
    , mReceiveIp6DatagramCallbackContext(nullptr)
    , mSendQueue()
    , mSendQueueTask(aInstance, Ip6::HandleSendQueue, this)
    , mIcmp(aInstance)
    , mUdp(aInstance)
    , mMpl(aInstance)
{
}

Message *Ip6::NewMessage(uint16_t aReserved, const Message::Settings &aSettings)
{
    return Get<MessagePool>().New(Message::kTypeIp6,
                                  sizeof(Header) + sizeof(HopByHopHeader) + sizeof(OptionMpl) + aReserved, aSettings);
}

Message *Ip6::NewMessage(const uint8_t *aData, uint16_t aDataLength, const Message::Settings &aSettings)
{
    Message *message = Get<MessagePool>().New(Message::kTypeIp6, 0, aSettings);

    VerifyOrExit(message != nullptr, OT_NOOP);

    if (message->Append(aData, aDataLength) != OT_ERROR_NONE)
    {
        message->Free();
        message = nullptr;
    }

exit:
    return message;
}

Message *Ip6::NewMessage(const uint8_t *aData, uint16_t aDataLength)
{
    Message *         message = nullptr;
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

otError Ip6::GetDatagramPriority(const uint8_t *aData, uint16_t aDataLen, Message::Priority &aPriority)
{
    otError       error = OT_ERROR_NONE;
    const Header *header;

    VerifyOrExit((aData != nullptr) && (aDataLen >= sizeof(Header)), error = OT_ERROR_INVALID_ARGS);

    header = reinterpret_cast<const Header *>(aData);
    VerifyOrExit(header->IsValid(), error = OT_ERROR_PARSE);
    VerifyOrExit(sizeof(Header) + header->GetPayloadLength() == aDataLen, error = OT_ERROR_PARSE);

    aPriority = DscpToPriority(header->GetDscp());

exit:
    return error;
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
    messageInfo.GetPeerAddr().SetToRealmLocalAllMplForwarders();

    tunnelHeader.Init();
    tunnelHeader.SetHopLimit(static_cast<uint8_t>(kDefaultHopLimit));
    tunnelHeader.SetPayloadLength(aHeader.GetPayloadLength() + sizeof(tunnelHeader));
    tunnelHeader.SetDestination(messageInfo.GetPeerAddr());
    tunnelHeader.SetNextHeader(kProtoIp6);

    VerifyOrExit((source = SelectSourceAddress(messageInfo)) != nullptr, error = OT_ERROR_INVALID_SOURCE_ADDRESS);

    tunnelHeader.SetSource(source->GetAddress());

    SuccessOrExit(error = AddMplOption(aMessage, tunnelHeader));
    SuccessOrExit(error = aMessage.Prepend(&tunnelHeader, sizeof(tunnelHeader)));

exit:
    return error;
}

otError Ip6::InsertMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aHeader.GetDestination().IsMulticast() &&
                     aHeader.GetDestination().GetScope() >= Address::kRealmLocalScope,
                 OT_NOOP);

    if (aHeader.GetDestination().IsRealmLocalMulticast())
    {
        aMessage.RemoveHeader(sizeof(aHeader));

        if (aHeader.GetNextHeader() == kProtoHopOpts)
        {
            HopByHopHeader hbh;
            uint16_t       hbhLength = 0;
            OptionMpl      mplOption;

            // read existing hop-by-hop option header
            aMessage.Read(0, sizeof(hbh), &hbh);
            hbhLength = (hbh.GetLength() + 1) * 8;

            VerifyOrExit(hbhLength <= aHeader.GetPayloadLength(), error = OT_ERROR_PARSE);

            // increase existing hop-by-hop option header length by 8 bytes
            hbh.SetLength(hbh.GetLength() + 1);
            aMessage.Write(0, sizeof(hbh), &hbh);

            // make space for MPL Option + padding by shifting hop-by-hop option header
            SuccessOrExit(error = aMessage.Prepend(nullptr, 8));
            aMessage.CopyTo(8, 0, hbhLength, aMessage);

            // insert MPL Option
            mMpl.InitOption(mplOption, aHeader.GetSource());
            aMessage.Write(hbhLength, mplOption.GetTotalLength(), &mplOption);

            // insert Pad Option (if needed)
            if (mplOption.GetTotalLength() % 8)
            {
                OptionPadN padOption;
                padOption.Init(8 - (mplOption.GetTotalLength() % 8));
                aMessage.Write(hbhLength + mplOption.GetTotalLength(), padOption.GetTotalLength(), &padOption);
            }

            // increase IPv6 Payload Length
            aHeader.SetPayloadLength(aHeader.GetPayloadLength() + 8);
        }
        else
        {
            SuccessOrExit(error = AddMplOption(aMessage, aHeader));
        }

        SuccessOrExit(error = aMessage.Prepend(&aHeader, sizeof(aHeader)));
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
                IgnoreError(HandleDatagram(*messageCopy, nullptr, nullptr, true));
                otLogInfoIp6("Message copy for indirect transmission to sleepy children");
            }
            else
            {
                otLogWarnIp6("No enough buffer for message copy for indirect transmission to sleepy children");
            }
        }
#endif

        SuccessOrExit(error = AddTunneledMplOption(aMessage, aHeader, aMessageInfo));
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
    VerifyOrExit(ip6Header.GetNextHeader() == kProtoHopOpts, OT_NOOP);

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
            // if multiple MPL options exist, discard packet
            VerifyOrExit(mplOffset == 0, error = OT_ERROR_PARSE);

            mplOffset = offset;
            mplLength = option.GetLength();

            VerifyOrExit(mplLength <= sizeof(OptionMpl) - sizeof(OptionHeader), error = OT_ERROR_PARSE);

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

otError Ip6::SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto)
{
    otError  error = OT_ERROR_NONE;
    Header   header;
    uint16_t payloadLength = aMessage.GetLength();

    header.Init();
    header.SetDscp(PriorityToDscp(aMessage.GetPriority()));
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
        const NetifUnicastAddress *source = SelectSourceAddress(aMessageInfo);

        VerifyOrExit(source != nullptr, error = OT_ERROR_INVALID_SOURCE_ADDRESS);
        header.SetSource(source->GetAddress());
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

    SuccessOrExit(error = aMessage.Prepend(&header, sizeof(header)));

    Checksum::UpdateMessageChecksum(aMessage, header.GetSource(), header.GetDestination(), aIpProto);

    if (aMessageInfo.GetPeerAddr().IsMulticastLargerThanRealmLocal())
    {
#if OPENTHREAD_FTD
        if (Get<ChildTable>().HasSleepyChildWithAddress(header.GetDestination()))
        {
            Message *messageCopy = aMessage.Clone();

            if (messageCopy != nullptr)
            {
                otLogInfoIp6("Message copy for indirect transmission to sleepy children");
                EnqueueDatagram(*messageCopy);
            }
            else
            {
                otLogWarnIp6("No enough buffer for message copy for indirect transmission to sleepy children");
            }
        }
#endif

        SuccessOrExit(error = AddTunneledMplOption(aMessage, header, aMessageInfo));
    }

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

void Ip6::HandleSendQueue(Tasklet &aTasklet)
{
    aTasklet.GetOwner<Ip6>().HandleSendQueue();
}

void Ip6::HandleSendQueue(void)
{
    Message *message;

    while ((message = mSendQueue.GetHead()) != nullptr)
    {
        mSendQueue.Dequeue(*message);
        IgnoreError(HandleDatagram(*message, nullptr, nullptr, false));
    }
}

otError Ip6::HandleOptions(Message &aMessage, Header &aHeader, bool &aForward)
{
    otError        error = OT_ERROR_NONE;
    HopByHopHeader hbhHeader;
    OptionHeader   optionHeader;
    uint16_t       endOffset;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(hbhHeader), &hbhHeader) == sizeof(hbhHeader),
                 error = OT_ERROR_PARSE);
    endOffset = aMessage.GetOffset() + (hbhHeader.GetLength() + 1) * 8;

    VerifyOrExit(endOffset <= aMessage.GetLength(), error = OT_ERROR_PARSE);

    aMessage.MoveOffset(sizeof(optionHeader));

    while (aMessage.GetOffset() < endOffset)
    {
        VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(optionHeader), &optionHeader) == sizeof(optionHeader),
                     error = OT_ERROR_PARSE);

        if (optionHeader.GetType() == OptionPad1::kType)
        {
            aMessage.MoveOffset(sizeof(OptionPad1));
            continue;
        }

        VerifyOrExit(aMessage.GetOffset() + sizeof(optionHeader) + optionHeader.GetLength() <= endOffset,
                     error = OT_ERROR_PARSE);

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

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
otError Ip6::FragmentDatagram(Message &aMessage, uint8_t aIpProto)
{
    otError        error = OT_ERROR_NONE;
    Header         header;
    FragmentHeader fragmentHeader;
    Message *      fragment        = nullptr;
    uint16_t       fragmentCnt     = 0;
    uint16_t       payloadFragment = 0;
    uint16_t       offset          = 0;

    uint16_t maxPayloadFragment =
        FragmentHeader::MakeDivisibleByEight(kMinimalMtu - aMessage.GetOffset() - sizeof(fragmentHeader));
    uint16_t payloadLeft = aMessage.GetLength() - aMessage.GetOffset();

    VerifyOrExit(aMessage.Read(0, sizeof(header), &header) == sizeof(header), error = OT_ERROR_PARSE);
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

            otLogDebgIp6("Last Fragment");
        }
        else
        {
            payloadLeft -= maxPayloadFragment;
            payloadFragment = maxPayloadFragment;
        }

        offset = fragmentCnt * FragmentHeader::BytesToFragmentOffset(maxPayloadFragment);
        fragmentHeader.SetOffset(offset);

        VerifyOrExit((fragment = NewMessage(0)) != nullptr, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = fragment->SetLength(aMessage.GetOffset() + sizeof(fragmentHeader) + payloadFragment));

        header.SetPayloadLength(payloadFragment + sizeof(fragmentHeader));
        fragment->Write(0, sizeof(header), &header);

        fragment->SetOffset(aMessage.GetOffset());
        fragment->Write(aMessage.GetOffset(), sizeof(fragmentHeader), &fragmentHeader);

        VerifyOrExit(aMessage.CopyTo(aMessage.GetOffset() + FragmentHeader::FragmentOffsetToBytes(offset),
                                     aMessage.GetOffset() + sizeof(fragmentHeader), payloadFragment,
                                     *fragment) == static_cast<int>(payloadFragment),
                     error = OT_ERROR_NO_BUFS);

        EnqueueDatagram(*fragment);

        fragmentCnt++;
        fragment = nullptr;

        otLogInfoIp6("Fragment %d with %d bytes sent", fragmentCnt, payloadFragment);
    }

    aMessage.Free();

exit:

    if (error == OT_ERROR_NO_BUFS)
    {
        otLogWarnIp6("No buffer for Ip6 fragmentation");
    }

    FreeMessageOnError(fragment, error);
    return error;
}

otError Ip6::HandleFragment(Message &aMessage, Netif *aNetif, MessageInfo &aMessageInfo, bool aFromNcpHost)
{
    otError        error = OT_ERROR_NONE;
    Header         header, headerBuffer;
    FragmentHeader fragmentHeader;
    Message *      message         = nullptr;
    uint16_t       offset          = 0;
    uint16_t       payloadFragment = 0;
    int            assertValue     = 0;
    bool           isFragmented    = true;

    OT_UNUSED_VARIABLE(assertValue);

    VerifyOrExit(aMessage.Read(0, sizeof(header), &header) == sizeof(header), error = OT_ERROR_PARSE);

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(fragmentHeader), &fragmentHeader) == sizeof(fragmentHeader),
                 error = OT_ERROR_PARSE);

    if (fragmentHeader.GetOffset() == 0 && !fragmentHeader.IsMoreFlagSet())
    {
        isFragmented = false;
        aMessage.MoveOffset(sizeof(fragmentHeader));
        ExitNow();
    }

    for (message = mReassemblyList.GetHead(); message; message = message->GetNext())
    {
        VerifyOrExit(message->Read(0, sizeof(headerBuffer), &headerBuffer) == sizeof(headerBuffer),
                     error = OT_ERROR_PARSE);

        if (message->GetDatagramTag() == fragmentHeader.GetIdentification() &&
            headerBuffer.GetSource() == header.GetSource() && headerBuffer.GetDestination() == header.GetDestination())
        {
            break;
        }
    }

    offset          = FragmentHeader::FragmentOffsetToBytes(fragmentHeader.GetOffset());
    payloadFragment = aMessage.GetLength() - aMessage.GetOffset() - sizeof(fragmentHeader);

    otLogInfoIp6("Fragment with id %d received > %d bytes, offset %d", fragmentHeader.GetIdentification(),
                 payloadFragment, offset);

    if (offset + payloadFragment + aMessage.GetOffset() > kMaxAssembledDatagramLength)
    {
        otLogWarnIp6("Packet too large for fragment buffer");
        ExitNow(error = OT_ERROR_NO_BUFS);
    }

    if (message == nullptr)
    {
        VerifyOrExit((message = NewMessage(0)) != nullptr, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = message->SetLength(aMessage.GetOffset()));

        message->SetTimeout(kIp6ReassemblyTimeout);
        message->SetOffset(0);
        message->SetDatagramTag(fragmentHeader.GetIdentification());

        // copying the non-fragmentable header to the fragmentation buffer
        assertValue = aMessage.CopyTo(0, 0, aMessage.GetOffset(), *message);
        OT_ASSERT(assertValue == aMessage.GetOffset());

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kIp6FragmentReassembler);

        mReassemblyList.Enqueue(*message);

        otLogDebgIp6("start reassembly.");
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
        VerifyOrExit(aMessage.Read(0, sizeof(header), &header) == sizeof(header), error = OT_ERROR_PARSE);
        header.SetPayloadLength(message->GetLength() - sizeof(header));
        header.SetNextHeader(fragmentHeader.GetNextHeader());
        message->Write(0, sizeof(header), &header);

        otLogDebgIp6("Reassembly complete.");

        mReassemblyList.Dequeue(*message);

        IgnoreError(HandleDatagram(*message, aNetif, aMessageInfo.mLinkInfo, aFromNcpHost));
    }

exit:
    if (error != OT_ERROR_DROP && error != OT_ERROR_NONE && isFragmented)
    {
        if (message != nullptr)
        {
            mReassemblyList.Dequeue(*message);
            message->Free();
        }
        otLogWarnIp6("Reassembly failed: %s", otThreadErrorToString(error));
    }

    if (isFragmented)
    {
        // drop all fragments, the payload is stored in the fragment buffer
        error = OT_ERROR_DROP;
    }

    return error;
}

void Ip6::CleanupFragmentationBuffer(void)
{
    Message *message;

    while ((message = mReassemblyList.GetHead()) != nullptr)
    {
        mReassemblyList.Dequeue(*message);
        message->Free();
    }
}

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
    Message *next;

    for (Message *message = mReassemblyList.GetHead(); message; message = next)
    {
        next = message->GetNext();

        if (message->GetTimeout() > 0)
        {
            message->DecrementTimeout();
        }
        else
        {
            otLogNoteIp6("Reassembly timeout.");
            SendIcmpError(*message, Icmp::Header::kTypeTimeExceeded, Icmp::Header::kCodeFragmReasTimeEx);

            mReassemblyList.Dequeue(*message);
            message->Free();
        }
    }
}

void Ip6::SendIcmpError(Message &aMessage, Icmp::Header::Type aIcmpType, Icmp::Header::Code aIcmpCode)
{
    otError     error = OT_ERROR_NONE;
    Header      header;
    MessageInfo messageInfo;

    VerifyOrExit(aMessage.Read(0, sizeof(header), &header) == sizeof(header), error = OT_ERROR_PARSE);

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetLinkInfo(nullptr);

    error = mIcmp.SendError(aIcmpType, aIcmpCode, messageInfo, aMessage);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnIp6("Failed to send ICMP error: %s", otThreadErrorToString(error));
    }
}

#else
otError Ip6::FragmentDatagram(Message &aMessage, uint8_t aIpProto)
{
    OT_UNUSED_VARIABLE(aIpProto);

    EnqueueDatagram(aMessage);

    return OT_ERROR_NONE;
}

otError Ip6::HandleFragment(Message &aMessage, Netif *aNetif, MessageInfo &aMessageInfo, bool aFromNcpHost)
{
    OT_UNUSED_VARIABLE(aNetif);
    OT_UNUSED_VARIABLE(aMessageInfo);
    OT_UNUSED_VARIABLE(aFromNcpHost);

    otError        error = OT_ERROR_NONE;
    FragmentHeader fragmentHeader;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(fragmentHeader), &fragmentHeader) == sizeof(fragmentHeader),
                 error = OT_ERROR_PARSE);

    VerifyOrExit(fragmentHeader.GetOffset() == 0 && !fragmentHeader.IsMoreFlagSet(), error = OT_ERROR_DROP);

    aMessage.MoveOffset(sizeof(fragmentHeader));

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

otError Ip6::HandleExtensionHeaders(Message &    aMessage,
                                    Netif *      aNetif,
                                    MessageInfo &aMessageInfo,
                                    Header &     aHeader,
                                    uint8_t &    aNextHeader,
                                    bool         aForward,
                                    bool         aFromNcpHost,
                                    bool         aReceive)
{
    otError         error = OT_ERROR_NONE;
    ExtensionHeader extHeader;

    while (aReceive || aNextHeader == kProtoHopOpts)
    {
        VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(extHeader), &extHeader) == sizeof(extHeader),
                     error = OT_ERROR_PARSE);

        switch (aNextHeader)
        {
        case kProtoHopOpts:
            SuccessOrExit(error = HandleOptions(aMessage, aHeader, aForward));
            break;

        case kProtoFragment:
            // Always forward IPv6 fragments to the Host.
            IgnoreError(ProcessReceiveCallback(aMessage, aMessageInfo, aNextHeader, aFromNcpHost, Message::kCopyToUse));

            SuccessOrExit(error = HandleFragment(aMessage, aNetif, aMessageInfo, aFromNcpHost));
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
        error = mUdp.HandleMessage(aMessage, aMessageInfo);
        if (error == OT_ERROR_DROP)
        {
            otLogNoteIp6("Error UDP Checksum");
        }
        break;

    case kProtoIcmp6:
        error = mIcmp.HandleMessage(aMessage, aMessageInfo);
        break;

    default:
        break;
    }

    return error;
}

otError Ip6::ProcessReceiveCallback(Message &          aMessage,
                                    const MessageInfo &aMessageInfo,
                                    uint8_t            aIpProto,
                                    bool               aFromNcpHost,
                                    Message::Ownership aMessageOwnership)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = &aMessage;

    VerifyOrExit(!aFromNcpHost, error = OT_ERROR_NO_ROUTE);
    VerifyOrExit(mReceiveIp6DatagramCallback != nullptr, error = OT_ERROR_NO_ROUTE);

    // Do not forward reassembled IPv6 packets.
    VerifyOrExit(aMessage.GetLength() <= kMinimalMtu, error = OT_ERROR_DROP);

    if (mIsReceiveIp6FilterEnabled)
    {
        // do not pass messages sent to an RLOC/ALOC, except Service Locator
        VerifyOrExit(!aMessageInfo.GetSockAddr().GetIid().IsLocator() ||
                         aMessageInfo.GetSockAddr().GetIid().IsAnycastServiceLocator(),
                     error = OT_ERROR_NO_ROUTE);

        switch (aIpProto)
        {
        case kProtoIcmp6:
            if (mIcmp.ShouldHandleEchoRequest(aMessageInfo))
            {
                Icmp::Header icmp;
                aMessage.Read(aMessage.GetOffset(), sizeof(icmp), &icmp);

                // do not pass ICMP Echo Request messages
                VerifyOrExit(icmp.GetType() != Icmp::Header::kTypeEchoRequest, error = OT_ERROR_DROP);
            }

            break;

        case kProtoUdp:
        {
            Udp::Header udp;
            uint16_t    destPort;

            aMessage.Read(aMessage.GetOffset(), sizeof(udp), &udp);

            destPort = udp.GetDestinationPort();

            if ((destPort == Mle::kUdpPort) &&
                (aMessageInfo.GetSockAddr().IsLinkLocal() || aMessageInfo.GetSockAddr().IsLinkLocalMulticast()))
            {
                // do not pass MLE messages
                ExitNow(error = OT_ERROR_NO_ROUTE);
            }
#if !OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
            else if ((destPort == Tmf::kUdpPort) && Get<Tmf::TmfAgent>().IsTmfMessage(aMessageInfo))
            {
                // do not pass TMF messages
                ExitNow(error = OT_ERROR_NO_ROUTE);
            }
#endif

#if OPENTHREAD_FTD
            if (destPort == Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort())
            {
                ExitNow(error = OT_ERROR_NO_ROUTE);
            }
#endif
            break;
        }

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
            otLogWarnIp6("No buff to clone msg (len: %d) to pass to host", aMessage.GetLength());
            ExitNow(error = OT_ERROR_NO_BUFS);
        }

        break;
    }

    IgnoreError(RemoveMplOption(*message));
    mReceiveIp6DatagramCallback(message, mReceiveIp6DatagramCallbackContext);

exit:

    if ((error != OT_ERROR_NONE) && (aMessageOwnership == Message::kTakeCustody))
    {
        aMessage.Free();
    }

    return error;
}

otError Ip6::SendRaw(Message &aMessage)
{
    otError     error = OT_ERROR_NONE;
    Header      header;
    MessageInfo messageInfo;
    bool        freed = false;

    SuccessOrExit(error = header.Init(aMessage));

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());

    if (header.GetDestination().IsMulticast())
    {
        SuccessOrExit(error = InsertMplOption(aMessage, header, messageInfo));
    }

    error = HandleDatagram(aMessage, nullptr, nullptr, true);
    freed = true;

exit:

    if (!freed)
    {
        aMessage.Free();
    }

    return error;
}

otError Ip6::HandleDatagram(Message &aMessage, Netif *aNetif, const void *aLinkMessageInfo, bool aFromNcpHost)
{
    otError     error;
    MessageInfo messageInfo;
    Header      header;
    bool        receive;
    bool        forward;
    bool        multicastPromiscuous;
    bool        shouldFreeMessage;
    uint8_t     nextHeader;

start:
    receive              = false;
    forward              = false;
    multicastPromiscuous = false;
    shouldFreeMessage    = true;

    SuccessOrExit(error = header.Init(aMessage));

    messageInfo.Clear();
    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetLinkInfo(aLinkMessageInfo);

    // determine destination of packet
    if (header.GetDestination().IsMulticast())
    {
        if (aNetif != nullptr)
        {
            if (aNetif->IsMulticastSubscribed(header.GetDestination()))
            {
                receive = true;
            }
            else if (aNetif->IsMulticastPromiscuousEnabled())
            {
                multicastPromiscuous = true;
            }

#if OPENTHREAD_FTD
            if (header.GetDestination().IsMulticastLargerThanRealmLocal() &&
                Get<ChildTable>().HasSleepyChildWithAddress(header.GetDestination()))
            {
                forward = true;
            }
#endif
        }
        else
        {
            forward = true;
        }
    }
    else
    {
        if (Get<ThreadNetif>().HasUnicastAddress(header.GetDestination()))
        {
            receive = true;
        }
        else if (!header.GetDestination().IsLinkLocal())
        {
            forward = true;
        }
        else if (aNetif == nullptr)
        {
            forward = true;
        }
    }

    aMessage.SetOffset(sizeof(header));

    // process IPv6 Extension Headers
    nextHeader = static_cast<uint8_t>(header.GetNextHeader());
    SuccessOrExit(error = HandleExtensionHeaders(aMessage, aNetif, messageInfo, header, nextHeader, forward,
                                                 aFromNcpHost, receive));

    // process IPv6 Payload
    if (receive)
    {
        if (nextHeader == kProtoIp6)
        {
            // Remove encapsulating header and start over.
            aMessage.RemoveHeader(aMessage.GetOffset());
            goto start;
        }

#if OPENTHREAD_CONFIG_UNSECURE_TRAFFIC_MANAGED_BY_STACK_ENABLE
        if (nextHeader == kProtoTcp || nextHeader == kProtoUdp)
        {
            uint16_t dstPort;

            // TCP/UDP shares header uint16_t srcPort, uint16_t dstPort
            VerifyOrExit(aMessage.Read(aMessage.GetOffset() + sizeof(uint16_t), sizeof(dstPort), &dstPort) ==
                             sizeof(dstPort),
                         error = OT_ERROR_PARSE);
            dstPort = HostSwap16(dstPort);
            if (aMessage.IsLinkSecurityEnabled() && Get<ot::Ip6::Filter>().IsUnsecurePort(dstPort))
            {
                Get<ot::Ip6::Filter>().RemoveUnsecurePort(dstPort);
            }
        }
#endif

        IgnoreError(ProcessReceiveCallback(aMessage, messageInfo, nextHeader, aFromNcpHost, Message::kCopyToUse));

        SuccessOrExit(error = HandlePayload(aMessage, messageInfo, nextHeader));
    }
    else if (multicastPromiscuous)
    {
        IgnoreError(ProcessReceiveCallback(aMessage, messageInfo, nextHeader, aFromNcpHost, Message::kCopyToUse));
    }

    if (forward)
    {
        uint8_t hopLimit;

        if (!ShouldForwardToThread(messageInfo, aFromNcpHost))
        {
            // try passing to host
            error = ProcessReceiveCallback(aMessage, messageInfo, nextHeader, aFromNcpHost, Message::kTakeCustody);
            ExitNow(shouldFreeMessage = false);
        }

        if (aNetif != nullptr)
        {
            VerifyOrExit(mForwardingEnabled, OT_NOOP);
            header.SetHopLimit(header.GetHopLimit() - 1);
        }

        VerifyOrExit(header.GetHopLimit() > 0, error = OT_ERROR_DROP);

        hopLimit = header.GetHopLimit();
        aMessage.Write(Header::kHopLimitFieldOffset, sizeof(hopLimit), &hopLimit);

#if OPENTHREAD_CONFIG_UNSECURE_TRAFFIC_MANAGED_BY_STACK_ENABLE
        // check whether source port is an unsecure port
        if (aFromNcpHost && (nextHeader == kProtoTcp || nextHeader == kProtoUdp))
        {
            uint16_t sourcePort;

            VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(sourcePort), &sourcePort) == sizeof(sourcePort),
                         error = OT_ERROR_PARSE);
            sourcePort = HostSwap16(sourcePort);
            if (Get<ot::Ip6::Filter>().IsUnsecurePort(sourcePort))
            {
                aMessage.SetLinkSecurityEnabled(false);
                otLogInfoIp6("Disabled link security for packet to %s", header.GetDestination().ToString().AsCString());
            }
        }
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

bool Ip6::ShouldForwardToThread(const MessageInfo &aMessageInfo, bool aFromNcpHost) const
{
    OT_UNUSED_VARIABLE(aFromNcpHost);

    bool rval = false;

    if (aMessageInfo.GetSockAddr().IsMulticast())
    {
        // multicast
        ExitNow(rval = true);
    }
    else if (aMessageInfo.GetSockAddr().IsLinkLocal())
    {
        // on-link link-local address
        ExitNow(rval = true);
    }
    else if (IsOnLink(aMessageInfo.GetSockAddr()))
    {
        // on-link global address
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        ExitNow(rval = (aFromNcpHost ||
                        !Get<BackboneRouter::Manager>().ShouldForwardDuaToBackbone(aMessageInfo.GetSockAddr())));
#else
        ExitNow(rval = true);
#endif
    }
    else if (Get<ThreadNetif>().RouteLookup(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(), nullptr) ==
             OT_ERROR_NONE)
    {
        // route
        ExitNow(rval = true);
    }
    else
    {
        ExitNow(rval = false);
    }

exit:
    return rval;
}

const NetifUnicastAddress *Ip6::SelectSourceAddress(MessageInfo &aMessageInfo)
{
    Address *                  destination       = &aMessageInfo.GetPeerAddr();
    uint8_t                    destinationScope  = destination->GetScope();
    const NetifUnicastAddress *rvalAddr          = nullptr;
    uint8_t                    rvalPrefixMatched = 0;

    for (const NetifUnicastAddress *addr = Get<ThreadNetif>().GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        const Address *candidateAddr = &addr->GetAddress();
        uint8_t        candidatePrefixMatched;
        uint8_t        overrideScope;

        if (candidateAddr->GetIid().IsAnycastLocator())
        {
            // Don't use anycast address as source address.
            continue;
        }

        candidatePrefixMatched = destination->PrefixMatch(*candidateAddr);

        if (candidatePrefixMatched >= addr->mPrefixLength)
        {
            candidatePrefixMatched = addr->mPrefixLength;
            overrideScope          = addr->GetScope();
        }
        else
        {
            overrideScope = destinationScope;
        }

        if (rvalAddr == nullptr)
        {
            // Rule 0: Prefer any address
            rvalAddr          = addr;
            rvalPrefixMatched = candidatePrefixMatched;
        }
        else if (*candidateAddr == *destination)
        {
            // Rule 1: Prefer same address
            rvalAddr = addr;
            ExitNow();
        }
        else if (addr->GetScope() < rvalAddr->GetScope())
        {
            // Rule 2: Prefer appropriate scope
            if (addr->GetScope() >= overrideScope)
            {
                rvalAddr          = addr;
                rvalPrefixMatched = candidatePrefixMatched;
            }
            else
            {
                continue;
            }
        }
        else if (addr->GetScope() > rvalAddr->GetScope())
        {
            if (rvalAddr->GetScope() < overrideScope)
            {
                rvalAddr          = addr;
                rvalPrefixMatched = candidatePrefixMatched;
            }
            else
            {
                continue;
            }
        }
        else if (addr->mPreferred && !rvalAddr->mPreferred)
        {
            // Rule 3: Avoid deprecated addresses
            rvalAddr          = addr;
            rvalPrefixMatched = candidatePrefixMatched;
        }
        else if (candidatePrefixMatched > rvalPrefixMatched)
        {
            // Rule 6: Prefer matching label
            // Rule 7: Prefer public address
            // Rule 8: Use longest prefix matching
            rvalAddr          = addr;
            rvalPrefixMatched = candidatePrefixMatched;
        }
        else if ((candidatePrefixMatched == rvalPrefixMatched) &&
                 (destination->GetIid().IsRoutingLocator() == candidateAddr->GetIid().IsRoutingLocator()))
        {
            // Additional rule: Prefer RLOC source for RLOC destination, EID source for anything else
            rvalAddr          = addr;
            rvalPrefixMatched = candidatePrefixMatched;
        }
        else
        {
            continue;
        }

        // infer destination scope based on prefix match
        if (rvalPrefixMatched >= rvalAddr->mPrefixLength)
        {
            destinationScope = rvalAddr->GetScope();
        }
    }

exit:
    return rvalAddr;
}

bool Ip6::IsOnLink(const Address &aAddress) const
{
    bool rval = false;

    if (Get<ThreadNetif>().IsOnMesh(aAddress))
    {
        ExitNow(rval = true);
    }

    for (const NetifUnicastAddress *cur = Get<ThreadNetif>().GetUnicastAddresses(); cur; cur = cur->GetNext())
    {
        if (cur->GetAddress().PrefixMatch(aAddress) >= cur->mPrefixLength)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

// LCOV_EXCL_START

const char *Ip6::IpProtoToString(uint8_t aIpProto)
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

// LCOV_EXCL_STOP

} // namespace Ip6
} // namespace ot
