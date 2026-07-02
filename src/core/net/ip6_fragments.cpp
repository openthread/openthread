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

/**
 * @file
 *   This file implements IPv6 packet fragmentation and reassembly.
 */

#include "ip6_fragments.hpp"

#include "common/code_utils.hpp"
#include "common/log.hpp"
#include "common/owned_ptr.hpp"
#include "common/random.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "instance/instance.hpp"
#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"
#include "net/ip6_types.hpp"

namespace ot {
namespace Ip6 {

RegisterLogModule("Ip6Frags");

Fragments::Fragments(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

void Fragments::CompleteIp6FragTx(Message &aParent, Error aError)
{
    aParent.ClearIp6FragTxState();
    aParent.InvokeTxCallback(aError);
    aParent.Free();
}

Error Fragments::SendIpFragment(Message &aParent)
{
    Error          error = kErrorNone;
    Header         header;
    FragmentHeader fragmentHeader;
    Message       *fragment        = nullptr;
    uint16_t       payloadFragment = 0;
    uint16_t       fragByteOffset  = 0;
    uint16_t       payloadLeft     = 0;

    const uint16_t maxPayloadFragment =
        FragmentHeader::MakeDivisibleByEight(kMinimalMtu - aParent.GetOffset() - sizeof(fragmentHeader));

    fragByteOffset = aParent.GetIp6FragNextOffset();
    payloadLeft    = aParent.GetLength() - aParent.GetOffset() - fragByteOffset;

    VerifyOrExit(payloadLeft != 0, error = kErrorInvalidState);

    SuccessOrExit(error = aParent.Read(0, header));
    header.SetNextHeader(kProtoFragment);

    fragmentHeader.Init();
    fragmentHeader.SetIdentification(aParent.GetIp6FragIdentification());
    fragmentHeader.SetNextHeader(aParent.GetIp6FragIpProto());
    fragmentHeader.SetOffset(FragmentHeader::BytesToFragmentOffset(fragByteOffset));

    if (payloadLeft < maxPayloadFragment)
    {
        fragmentHeader.ClearMoreFlag();
        payloadFragment = payloadLeft;
    }
    else
    {
        fragmentHeader.SetMoreFlag();
        payloadFragment = maxPayloadFragment;
    }

    VerifyOrExit((fragment = Get<Ip6>().NewMessage()) != nullptr, error = kErrorNoBufs);
    IgnoreError(fragment->SetPriority(aParent.GetPriority()));
    SuccessOrExit(error = fragment->SetLength(aParent.GetOffset() + sizeof(fragmentHeader) + payloadFragment));

    header.SetPayloadLength(payloadFragment + sizeof(fragmentHeader));
    fragment->Write(0, header);

    fragment->SetOffset(aParent.GetOffset());
    fragment->Write(aParent.GetOffset(), fragmentHeader);

    fragment->WriteBytesFromMessage(
        /* aWriteOffset */ aParent.GetOffset() + sizeof(fragmentHeader), aParent,
        /* aReadOffset */ aParent.GetOffset() + fragByteOffset,
        /* aLength */ payloadFragment);

    fragment->SetSubType(Message::kSubTypeIp6Fragment);
    fragment->SetFragmentParent(&aParent);
    fragment->SetDoNotEvict(true);
    fragment->SetLoopbackToHostAllowed(aParent.IsLoopbackToHostAllowed());
    fragment->SetLinkSecurityEnabled(aParent.IsLinkSecurityEnabled());
    fragment->SetOrigin(aParent.GetOrigin());

    Get<Ip6>().EnqueueDatagram(*fragment);

    LogInfo("IPv6 fragment %u bytes at offset %u enqueued", payloadFragment, fragByteOffset);

    fragment = nullptr;

exit:
    FreeMessageOnError(fragment, error);
    return error;
}

Error Fragments::FragmentDatagram(Message &aMessage, uint8_t aIpProto)
{
    Error error = kErrorNone;

    VerifyOrExit(!aMessage.IsIp6FragTxActive(), error = kErrorInvalidState);

    aMessage.SetIp6FragTxActive(true);
    aMessage.SetDoNotEvict(true);
    aMessage.SetIp6FragIdentification(Random::NonCrypto::Generate<uint32_t>());
    aMessage.SetIp6FragIpProto(aIpProto);
    aMessage.SetIp6FragNextOffset(0);

    SuccessOrExit(error = SendIpFragment(aMessage));

exit:
    if (error != kErrorNone)
    {
        if (error == kErrorNoBufs)
        {
            LogWarn("No buffer for Ip6 fragmentation");
        }

        if (aMessage.IsIp6FragTxActive())
        {
            aMessage.InvokeTxCallback(error);
            aMessage.ClearIp6FragTxState();
        }
    }

    return error;
}

void Fragments::HandleIpFragmentTxDone(Message &aFragment, Error aError)
{
    Message *parent = aFragment.GetFragmentParent();

    VerifyOrExit(parent != nullptr);
    VerifyOrExit(parent->IsIp6FragTxActive());

    if (aError == kErrorNone && !aFragment.GetTxSuccess())
    {
        aError = kErrorFailed;
    }

    if (aError != kErrorNone)
    {
        LogInfo("IPv6 fragment tx failed (%s)", ErrorToString(aError));
        CompleteIp6FragTx(*parent, aError);
        ExitNow();
    }

    {
        const uint16_t unfragPartLen    = parent->GetOffset();
        const uint16_t payloadFragment  = aFragment.GetLength() - unfragPartLen - sizeof(FragmentHeader);
        const uint16_t fragStartOffset  = parent->GetIp6FragNextOffset();
        const uint16_t nextOffset       = fragStartOffset + payloadFragment;
        const uint16_t totalFragPayload = parent->GetLength() - unfragPartLen;

        parent->SetIp6FragNextOffset(nextOffset);

        if (nextOffset < totalFragPayload)
        {
            if (SendIpFragment(*parent) != kErrorNone)
            {
                CompleteIp6FragTx(*parent, kErrorNoBufs);
            }

            ExitNow();
        }
    }

    LogDebg("IPv6 fragmentation complete");
    CompleteIp6FragTx(*parent, kErrorNone);

exit:
    return;
}

Error Fragments::HandleFragment(Message &aMessage)
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
        VerifyOrExit((message = Get<Ip6>().NewMessage()) != nullptr, error = kErrorNoBufs);
        mReassemblyList.Enqueue(*message);

        message->SetTimestampToNow();
        message->SetOffset(0);
        message->SetDatagramTag(fragmentHeader.GetIdentification());
        message->SetIp6FragNextOffset(0);

        // copying the non-fragmentable header to the fragmentation buffer
        SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, 0, aMessage.GetOffset()));

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kIp6FragmentReassembler);
    }
    else if (offset < message->GetIp6FragNextOffset())
    {
        ExitNow(error = kErrorDrop);
    }
    else if (offset > message->GetIp6FragNextOffset())
    {
        LogWarn("Gap in IPv6 reassembly at offset %u (expected %u)", offset, message->GetIp6FragNextOffset());
        mReassemblyList.DequeueAndFree(*message);
        message = nullptr;
        ExitNow(error = kErrorFailed);
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

    if (offset + payloadFragment > message->GetIp6FragNextOffset())
    {
        message->SetIp6FragNextOffset(offset + payloadFragment);
    }

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

        IgnoreError(Get<Ip6>().HandleDatagram(OwnedPtr<Message>(message), /* aIsReassembled */ true));
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

void Fragments::CleanupFragmentationBuffer(void) { mReassemblyList.DequeueAndFreeAll(); }

void Fragments::HandleTimeTick(void)
{
    UpdateReassemblyList();

    if (mReassemblyList.GetHead() == nullptr)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kIp6FragmentReassembler);
    }
}

void Fragments::UpdateReassemblyList(void)
{
    TimeMilli now = TimerMilli::GetNow();

    for (Message &message : mReassemblyList)
    {
        if (now - message.GetTimestamp() >= TimeMilli::SecToMsec(kIp6ReassemblyTimeout))
        {
            LogNote("Reassembly timeout.");
            SendIcmpError(message, Icmp6Header::kTypeTimeExceeded, Icmp6Header::kCodeFragmReasTimeEx);

            mReassemblyList.DequeueAndFree(message);
        }
    }
}

void Fragments::SendIcmpError(Message &aMessage, Icmp6Header::Type aIcmpType, Icmp6Header::Code aIcmpCode)
{
    Error       error = kErrorNone;
    Header      header;
    MessageInfo messageInfo;

    SuccessOrExit(error = aMessage.Read(0, header));

    messageInfo.SetPeerAddr(header.GetSource());
    messageInfo.SetSockAddr(header.GetDestination());
    messageInfo.SetHopLimit(header.GetHopLimit());

    error = Get<Icmp>().SendError(aIcmpType, aIcmpCode, messageInfo, aMessage);

exit:
    LogWarnOnError(error, "send ICMP");
    OT_UNUSED_VARIABLE(error);
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
const MessageQueue &Fragments::GetReassemblyQueue(void) const { return mReassemblyList; }
#endif

#else // OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

Error Fragments::FragmentDatagram(Message &aMessage, uint8_t aIpProto)
{
    OT_UNUSED_VARIABLE(aIpProto);

    Get<Ip6>().EnqueueDatagram(aMessage);

    return kErrorNone;
}

void Fragments::HandleIpFragmentTxDone(Message &aFragment, Error aError)
{
    OT_UNUSED_VARIABLE(aFragment);
    OT_UNUSED_VARIABLE(aError);
}

Error Fragments::HandleFragment(Message &aMessage)
{
    Error          error = kErrorNone;
    FragmentHeader fragmentHeader;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), fragmentHeader));

    VerifyOrExit(fragmentHeader.GetOffset() == 0 && !fragmentHeader.IsMoreFlagSet(), error = kErrorDrop);

    aMessage.MoveOffset(sizeof(fragmentHeader));

exit:
    return error;
}

void Fragments::CleanupFragmentationBuffer(void) {}

void Fragments::HandleTimeTick(void) {}

#endif // OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE

} // namespace Ip6
} // namespace ot
