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

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "net/icmp6.hpp"
#include "net/ip6_address.hpp"
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
    , mSendQueue()
    , mSendQueueTask(aInstance, HandleSendQueue, this)
    , mIcmp(aInstance)
    , mUdp(aInstance)
    , mMpl(aInstance)
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    , mTimer(aInstance, &Ip6::HandleTimer, this)
#endif
{
}

Message *Ip6::NewMessage(uint16_t aReserved, const otMessageSettings *aSettings)
{
    return Get<MessagePool>().New(Message::kTypeIp6,
                                  sizeof(Header) + sizeof(HopByHopHeader) + sizeof(OptionMpl) + aReserved, aSettings);
}

Message *Ip6::NewMessage(const uint8_t *aData, uint16_t aDataLength, const otMessageSettings *aSettings)
{
    otMessageSettings settings = {true, OT_MESSAGE_PRIORITY_NORMAL};
    Message *         message  = NULL;
    uint8_t           priority;

    if (aSettings != NULL)
    {
        settings = *aSettings;
    }

    SuccessOrExit(GetDatagramPriority(aData, aDataLength, priority));
    settings.mPriority = static_cast<otMessagePriority>(priority);
    VerifyOrExit((message = Get<MessagePool>().New(Message::kTypeIp6, 0, &settings)) != NULL);

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
                                          uint8_t        aProto)
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
    messageInfo.GetPeerAddr().Clear();
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

otError Ip6::InsertMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;

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
            aMessage.Read(0, sizeof(hbh), &hbh);
            hbhLength = (hbh.GetLength() + 1) * 8;

            VerifyOrExit(hbhLength <= aHeader.GetPayloadLength(), error = OT_ERROR_PARSE);

            // increase existing hop-by-hop option header length by 8 bytes
            hbh.SetLength(hbh.GetLength() + 1);
            aMessage.Write(0, sizeof(hbh), &hbh);

            // make space for MPL Option + padding by shifting hop-by-hop option header
            SuccessOrExit(error = aMessage.Prepend(NULL, 8));
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
        if (aHeader.GetDestination().IsMulticastLargerThanRealmLocal() &&
            Get<Mle::MleRouter>().HasSleepyChildrenSubscribed(aHeader.GetDestination()))
        {
            Message *messageCopy = NULL;

            if ((messageCopy = aMessage.Clone()) != NULL)
            {
                HandleDatagram(*messageCopy, NULL, NULL, true);
                otLogInfoIp6("Message copy for indirect transmission to sleepy children");
            }
            else
            {
                otLogWarnIp6("No enough buffer for message copy for indirect transmission to sleepy children");
            }
        }

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
    otError                    error = OT_ERROR_NONE;
    Header                     header;
    uint16_t                   payloadLength = aMessage.GetLength();
    uint16_t                   checksum;
    const NetifUnicastAddress *source;

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
        VerifyOrExit((source = SelectSourceAddress(aMessageInfo)) != NULL, error = OT_ERROR_INVALID_SOURCE_ADDRESS);
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

    // compute checksum
    checksum = ComputePseudoheaderChecksum(header.GetSource(), header.GetDestination(), payloadLength, aIpProto);

    switch (aIpProto)
    {
    case kProtoUdp:
        mUdp.UpdateChecksum(aMessage, checksum);
        break;

    case kProtoIcmp6:
        mIcmp.UpdateChecksum(aMessage, checksum);
        break;

    default:
        break;
    }

    if (aMessageInfo.GetPeerAddr().IsMulticastLargerThanRealmLocal())
    {
        if (Get<Mle::MleRouter>().HasSleepyChildrenSubscribed(header.GetDestination()))
        {
            Message *messageCopy = NULL;

            if ((messageCopy = aMessage.Clone()) != NULL)
            {
                otLogInfoIp6("Message copy for indirect transmission to sleepy children");
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
        if (aMessage.GetLength() > kMaxDatagramLength)
        {
            error = FragmentDatagram(aMessage, aIpProto);
        }
        else
        {
            EnqueueDatagram(aMessage);
        }
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
        HandleDatagram(*message, NULL, NULL, false);
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
    Message *      fragment        = NULL;
    uint16_t       fragmentCnt     = 0;
    uint16_t       payloadFragment = 0;
    uint16_t       offset          = 0;
    int            assertValue     = 0;

    uint16_t maxPayloadFragment =
        FragmentHeader::MakeDivisibleByEight(kMinimalMtu - aMessage.GetOffset() - sizeof(fragmentHeader));
    uint16_t payloadLeft = aMessage.GetLength() - aMessage.GetOffset();

    VerifyOrExit(aMessage.GetLength() <= kMaxAssembledDatagramLength, error = OT_ERROR_NO_BUFS);

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

        VerifyOrExit((fragment = NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = fragment->SetLength(aMessage.GetOffset() + sizeof(fragmentHeader) + payloadFragment));

        header.SetPayloadLength(payloadFragment + sizeof(fragmentHeader));
        assertValue = fragment->Write(0, sizeof(header), &header);
        assert(assertValue == sizeof(header));

        SuccessOrExit(error = fragment->SetOffset(aMessage.GetOffset()));
        assertValue = fragment->Write(aMessage.GetOffset(), sizeof(fragmentHeader), &fragmentHeader);
        assert(assertValue == sizeof(fragmentHeader));

        VerifyOrExit(aMessage.CopyTo(aMessage.GetOffset() + FragmentHeader::FragmentOffsetToBytes(offset),
                                     aMessage.GetOffset() + sizeof(fragmentHeader), payloadFragment,
                                     *fragment) == static_cast<int>(payloadFragment),
                     error = OT_ERROR_NO_BUFS);

        EnqueueDatagram(*fragment);

        fragmentCnt++;
        fragment = NULL;

        otLogInfoIp6("Fragment %d with %d bytes sent", fragmentCnt, payloadFragment);
    }
    aMessage.Free();

exit:

    if (error == OT_ERROR_NO_BUFS)
    {
        otLogWarnIp6("No buffer for Ip6 fragmentation");
    }

    if (error != OT_ERROR_NONE && fragment != NULL)
    {
        fragment->Free();
    }

    return error;
}

otError Ip6::HandleFragment(Message &aMessage, Netif *aNetif, MessageInfo &aMessageInfo, bool aFromNcpHost)
{
    otError        error = OT_ERROR_NONE;
    Header         header, headerBuffer;
    FragmentHeader fragmentHeader;
    Message *      message         = NULL;
    uint16_t       offset          = 0;
    uint16_t       payloadFragment = 0;
    int            assertValue     = 0;
    bool           isFragmented    = true;

    VerifyOrExit(aMessage.Read(0, sizeof(header), &header) == sizeof(header), error = OT_ERROR_PARSE);

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(fragmentHeader), &fragmentHeader) == sizeof(fragmentHeader),
                 error = OT_ERROR_PARSE);

    if (fragmentHeader.GetOffset() == 0 && !fragmentHeader.IsMoreFlagSet())
    {
        isFragmented = false;
        error        = aMessage.MoveOffset(sizeof(fragmentHeader));

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

    if (message == NULL)
    {
        VerifyOrExit((message = NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = message->SetLength(aMessage.GetOffset()));

        message->SetTimeout(kIp6ReassemblyTimeout);
        SuccessOrExit(error = message->SetOffset(0));
        message->SetDatagramTag(fragmentHeader.GetIdentification());

        // copying the non-fragmentable header to the fragmentation buffer
        assertValue = aMessage.CopyTo(0, 0, aMessage.GetOffset(), *message);
        assert(assertValue == aMessage.GetOffset());

        if (!mTimer.IsRunning())
        {
            mTimer.Start(kStateUpdatePeriod);
        }

        mReassemblyList.Enqueue(*message);

        otLogDebgIp6("start reassembly.");
    }

    otLogInfoIp6("Fragment with id %d received > %d bytes, offset %d", message->GetDatagramTag(), payloadFragment,
                 offset);

    if (offset + payloadFragment + aMessage.GetOffset() > kMaxAssembledDatagramLength)
    {
        otLogWarnIp6("Package too large for fragment buffer");
        ExitNow(error = OT_ERROR_NO_BUFS);
    }

    // increase message buffer if necessary
    if (message->GetLength() < offset + payloadFragment + aMessage.GetOffset())
    {
        SuccessOrExit(error = message->SetLength(offset + payloadFragment + aMessage.GetOffset()));
    }

    // copy the fragment payload into the message buffer
    assertValue = aMessage.CopyTo(aMessage.GetOffset() + sizeof(fragmentHeader), aMessage.GetOffset() + offset,
                                  payloadFragment, *message);
    assert(assertValue == static_cast<int>(payloadFragment));

    // check if it is the last frame
    if (!fragmentHeader.IsMoreFlagSet())
    {
        // use the offset value for the whole ip message length
        SuccessOrExit(error = message->SetOffset(offset + payloadFragment + aMessage.GetOffset()));
    }

    if (message->GetOffset() >= message->GetLength())
    {
        // creates the header for the reassembled ipv6 package
        VerifyOrExit(aMessage.Read(0, sizeof(header), &header) == sizeof(header), error = OT_ERROR_PARSE);
        header.SetPayloadLength(message->GetLength() - sizeof(header));
        header.SetNextHeader(fragmentHeader.GetNextHeader());
        assertValue = message->Write(0, sizeof(header), &header);
        assert(assertValue == sizeof(header));

        otLogDebgIp6("Reassembly complete.");

        mReassemblyList.Dequeue(*message);

        error = HandleDatagram(*message, aNetif, aMessageInfo.mLinkInfo, aFromNcpHost);
    }

exit:
    if (error != OT_ERROR_DROP && error != OT_ERROR_NONE && isFragmented)
    {
        if (message != NULL)
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
    for (Message *message = mReassemblyList.GetHead(); message;)
    {
        Message *next = message->GetNext();
        mReassemblyList.Dequeue(*message);

        message->Free();
        message = next;
    }
}

void Ip6::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<Ip6>().HandleUpdateTimer();
}

void Ip6::HandleUpdateTimer(void)
{
    UpdateReassemblyList();

    if (mReassemblyList.GetHead() != NULL)
    {
        mTimer.Start(kStateUpdatePeriod);
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
            SendIcmpError(*message, IcmpHeader::kTypeTimeExceeded, IcmpHeader::kCodeFragmReasTimeEx);

            mReassemblyList.Dequeue(*message);
            message->Free();
        }
    }
}

otError Ip6::SendIcmpError(Message &aMessage, IcmpHeader::Type aIcmpType, IcmpHeader::Code aIcmpCode)
{
    otError     error = OT_ERROR_NONE;
    Header      header;
    MessageInfo messageInfo;

    VerifyOrExit(aMessage.Read(0, sizeof(header), &header) == sizeof(header), error = OT_ERROR_PARSE);

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());
    messageInfo.SetLinkInfo(NULL);

    SuccessOrExit(error = mIcmp.SendError(aIcmpType, aIcmpCode, messageInfo, header));

exit:
    return error;
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
        ExitNow();

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

    VerifyOrExit(!aFromNcpHost, error = OT_ERROR_NO_ROUTE);
    VerifyOrExit(mReceiveIp6DatagramCallback != NULL, error = OT_ERROR_NO_ROUTE);

    if (mIsReceiveIp6FilterEnabled)
    {
        // do not pass messages sent to an RLOC/ALOC, except Service Locator
        VerifyOrExit(
            (!aMessageInfo.GetSockAddr().IsRoutingLocator() && !aMessageInfo.GetSockAddr().IsAnycastRoutingLocator()) ||
                aMessageInfo.GetSockAddr().IsAnycastServiceLocator(),
            error = OT_ERROR_NO_ROUTE);

        switch (aIpProto)
        {
        case kProtoIcmp6:
            if (mIcmp.ShouldHandleEchoRequest(aMessageInfo))
            {
                IcmpHeader icmp;
                aMessage.Read(aMessage.GetOffset(), sizeof(icmp), &icmp);

                // do not pass ICMP Echo Request messages
                VerifyOrExit(icmp.GetType() != IcmpHeader::kTypeEchoRequest, error = OT_ERROR_DROP);
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

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE == 0
            case kCoapUdpPort:

                // do not pass TMF messages
                if (Get<ThreadNetif>().IsTmfMessage(aMessageInfo))
                {
                    ExitNow(error = OT_ERROR_NO_ROUTE);
                }

                break;
#endif // OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE

            default:
#if OPENTHREAD_FTD
                if (udp.GetDestinationPort() == Get<MeshCoP::JoinerRouter>().GetJoinerUdpPort())
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
    messageInfo.SetLinkInfo(NULL);

    if (header.GetDestination().IsMulticast())
    {
        SuccessOrExit(error = InsertMplOption(aMessage, header, messageInfo));
    }

    error = HandleDatagram(aMessage, NULL, NULL, true);
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
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfo;
    Header      header;
    bool        receive              = false;
    bool        forward              = false;
    bool        tunnel               = false;
    bool        multicastPromiscuous = false;
    uint8_t     nextHeader;
    uint8_t     hopLimit;

    SuccessOrExit(error = header.Init(aMessage));

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
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
                Get<Mle::MleRouter>().HasSleepyChildrenSubscribed(header.GetDestination()))
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
        if (Get<ThreadNetif>().IsUnicastAddress(header.GetDestination()))
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
            // Remove encapsulating header.
            aMessage.RemoveHeader(aMessage.GetOffset());

            HandleDatagram(aMessage, aNetif, aLinkMessageInfo, aFromNcpHost);
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
        if (!ShouldForwardToThread(messageInfo))
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
            SuccessOrExit(error = Get<ThreadNetif>().SendMessage(aMessage));
        }
    }

exit:

    if (!tunnel && (error != OT_ERROR_NONE || !forward))
    {
        aMessage.Free();
    }

    return error;
}

bool Ip6::ShouldForwardToThread(const MessageInfo &aMessageInfo) const
{
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
        ExitNow(rval = true);
    }
    else if (Get<ThreadNetif>().RouteLookup(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(), NULL) ==
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
    const NetifUnicastAddress *rvalAddr          = NULL;
    uint8_t                    rvalPrefixMatched = 0;

    for (const NetifUnicastAddress *addr = Get<ThreadNetif>().GetUnicastAddresses(); addr; addr = addr->GetNext())
    {
        const Address *candidateAddr = &addr->GetAddress();
        uint8_t        candidatePrefixMatched;
        uint8_t        overrideScope;

        if (candidateAddr->IsAnycastRoutingLocator())
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

        if (rvalAddr == NULL)
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
                 (destination->IsRoutingLocator() == candidateAddr->IsRoutingLocator()))
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
