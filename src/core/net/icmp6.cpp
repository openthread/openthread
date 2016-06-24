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
 *   This file implements ICMPv6.
 */

#include <string.h>

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <common/timer.hpp>
#include <net/icmp6.hpp>
#include <net/ip6.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Ip6 {

bool Icmp::sIsEchoEnabled = true;

uint16_t IcmpEcho::sNextId = 1;
IcmpEcho *IcmpEcho::sEchoClients = NULL;
IcmpHandler *IcmpHandler::sHandlers = NULL;

IcmpEcho::IcmpEcho(EchoReplyHandler aHandler, void *aContext)
{
    mHandler = aHandler;
    mContext = aContext;
    mId = sNextId++;
    mSeq = 0;
    mNext = sEchoClients;
    sEchoClients = this;
    memset(&mEchoRequests, 0, sizeof(struct IcmpEchoRequests));
}

ThreadError IcmpEcho::SendEchoRequest(const SockAddr &aDestination,
                                      const void *aPayload, uint16_t aPayloadLength)
{
    ThreadError error = kThreadError_None;
    MessageInfo messageInfo;
    Message *message = NULL;
    IcmpHeader icmp6Header;

    VerifyOrExit((message = Ip6::NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->SetLength(sizeof(icmp6Header) + aPayloadLength));

    message->Write(sizeof(icmp6Header), aPayloadLength, aPayload);

    icmp6Header.Init();
    icmp6Header.SetType(IcmpHeader::kTypeEchoRequest);
    icmp6Header.SetId(mId);
    icmp6Header.SetSequence(mSeq);
    message->Write(0, sizeof(icmp6Header), &icmp6Header);

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.GetPeerAddr() = aDestination.GetAddress();
    messageInfo.mInterfaceId = aDestination.mScopeId;

    SuccessOrExit(error = Ip6::SendDatagram(*message, messageInfo, kProtoIcmp6));

    AddEchoRequest(mSeq++);

    otLogInfoIcmp("Sent echo request\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

void IcmpEcho::AddEchoRequest(uint8_t seq)
{
    uint8_t index;

    if (mEchoRequests.mEchoRequestLength == kIcmp6EchoRequestsSize)
    {
        DeleteEchoRequest(mEchoRequests.mEchoRequestSeq[mEchoRequests.mEchoRequestHead]);
    }

    index = (mEchoRequests.mEchoRequestHead + mEchoRequests.mEchoRequestLength) % kIcmp6EchoRequestsSize;
    mEchoRequests.mEchoRequestTime[index] = Timer::GetNow();
    mEchoRequests.mEchoRequestSeq[index] = seq;
    mEchoRequests.mEchoRequestLength++;
}

void IcmpEcho::DeleteEchoRequest(uint8_t seq)
{
    uint8_t index;
    int length;

    VerifyOrExit((index = GetEchoRequest(seq)) != 0xff, ;);
    index++;

    length = index - mEchoRequests.mEchoRequestHead;

    if (length < 0)
    {
        length += kIcmp6EchoRequestsSize;
    }

    mEchoRequests.mEchoRequestHead = index % kIcmp6EchoRequestsSize;
    mEchoRequests.mEchoRequestLength -= length;

exit:
    {}
}

uint8_t IcmpEcho::GetEchoRequest(uint8_t seq)
{
    uint8_t index = mEchoRequests.mEchoRequestHead;
    uint8_t length = mEchoRequests.mEchoRequestLength;

    while ((mEchoRequests.mEchoRequestSeq[index] != seq) && (length > 0))
    {
        index = (index + 1) % kIcmp6EchoRequestsSize;
        length--;
    }

    if (length == 0)
    {
        index = 0xff;
    }

    return index;
}

ThreadError Icmp::RegisterCallbacks(IcmpHandler &aHandler)
{
    ThreadError error = kThreadError_None;

    for (IcmpHandler *cur = IcmpHandler::sHandlers; cur; cur = cur->mNext)
    {
        if (cur == &aHandler)
        {
            ExitNow(error = kThreadError_Busy);
        }
    }

    aHandler.mNext = IcmpHandler::sHandlers;
    IcmpHandler::sHandlers = &aHandler;

exit:
    return error;
}

ThreadError Icmp::SendError(const Address &aDestination, IcmpHeader::Type aType, IcmpHeader::Code aCode,
                            const Header &aHeader)
{
    ThreadError error = kThreadError_None;
    MessageInfo messageInfo;
    Message *message = NULL;
    IcmpHeader icmp6Header;

    VerifyOrExit((message = Ip6::NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->SetLength(sizeof(icmp6Header) + sizeof(aHeader)));

    message->Write(sizeof(icmp6Header), sizeof(aHeader), &aHeader);

    icmp6Header.Init();
    icmp6Header.SetType(aType);
    icmp6Header.SetCode(aCode);
    message->Write(0, sizeof(icmp6Header), &icmp6Header);

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = aDestination;

    SuccessOrExit(error = Ip6::SendDatagram(*message, messageInfo, kProtoIcmp6));

    otLogInfoIcmp("Sent ICMPv6 Error\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
    }

    return error;
}

ThreadError Icmp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    uint16_t payloadLength;
    IcmpHeader icmp6Header;
    uint16_t checksum;

    payloadLength = aMessage.GetLength() - aMessage.GetOffset();

    // check length
    VerifyOrExit(payloadLength >= IcmpHeader::GetDataOffset(),  error = kThreadError_Drop);
    aMessage.Read(aMessage.GetOffset(), sizeof(icmp6Header), &icmp6Header);

    // verify checksum
    checksum = Ip6::ComputePseudoheaderChecksum(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(),
                                                payloadLength, kProtoIcmp6);
    checksum = aMessage.UpdateChecksum(checksum, aMessage.GetOffset(), payloadLength);
    VerifyOrExit(checksum == 0xffff, ;);

    switch (icmp6Header.GetType())
    {
    case IcmpHeader::kTypeEchoRequest:
        return HandleEchoRequest(aMessage, aMessageInfo);

    case IcmpHeader::kTypeEchoReply:
        return HandleEchoReply(aMessage, aMessageInfo, icmp6Header);

    case IcmpHeader::kTypeDstUnreach:
        return HandleDstUnreach(aMessage, aMessageInfo, icmp6Header);
    }

exit:
    return error;
}

ThreadError Icmp::HandleDstUnreach(Message &aMessage, const MessageInfo &aMessageInfo,
                                   const IcmpHeader &aIcmpheader)
{
    aMessage.MoveOffset(sizeof(aIcmpheader));

    for (IcmpHandler *handler = IcmpHandler::sHandlers; handler; handler = handler->mNext)
    {
        handler->HandleDstUnreach(aMessage, aMessageInfo, aIcmpheader);
    }

    return kThreadError_None;
}

ThreadError Icmp::HandleEchoRequest(Message &aRequestMessage, const MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    IcmpHeader icmp6Header;
    Message *replyMessage = NULL;
    MessageInfo replyMessageInfo;
    uint16_t payloadLength;

    VerifyOrExit(sIsEchoEnabled, ;);

    otLogInfoIcmp("Received Echo Request\n");

    icmp6Header.Init();
    icmp6Header.SetType(IcmpHeader::kTypeEchoReply);

    VerifyOrExit((replyMessage = Ip6::NewMessage(0)) != NULL, otLogDebgIcmp("icmp fail\n"));
    payloadLength = aRequestMessage.GetLength() - aRequestMessage.GetOffset() - IcmpHeader::GetDataOffset();
    SuccessOrExit(replyMessage->SetLength(IcmpHeader::GetDataOffset() + payloadLength));

    replyMessage->Write(0, IcmpHeader::GetDataOffset(), &icmp6Header);
    aRequestMessage.CopyTo(aRequestMessage.GetOffset() + IcmpHeader::GetDataOffset(),
                           IcmpHeader::GetDataOffset(), payloadLength, *replyMessage);

    memset(&replyMessageInfo, 0, sizeof(replyMessageInfo));
    replyMessageInfo.GetPeerAddr() = aMessageInfo.GetPeerAddr();

    if (!aMessageInfo.GetSockAddr().IsMulticast())
    {
        replyMessageInfo.GetSockAddr() = aMessageInfo.GetSockAddr();
    }

    replyMessageInfo.mInterfaceId = aMessageInfo.mInterfaceId;

    SuccessOrExit(error = Ip6::SendDatagram(*replyMessage, replyMessageInfo, kProtoIcmp6));

    otLogInfoIcmp("Sent Echo Reply\n");

exit:

    if (error != kThreadError_None && replyMessage != NULL)
    {
        Message::Free(*replyMessage);
    }

    return error;
}

ThreadError Icmp::HandleEchoReply(Message &aMessage, const MessageInfo &aMessageInfo,
                                  const IcmpHeader &aIcmpHeader)
{
    uint8_t index;

    VerifyOrExit(sIsEchoEnabled, ;);

    for (IcmpEcho *client = IcmpEcho::sEchoClients; client; client = client->mNext)
    {
        if (client->mId == aIcmpHeader.GetId())
        {
            index = client->GetEchoRequest(aIcmpHeader.GetSequence());

            if (index != 0xff)
            {
                client->HandleEchoReply(aMessage, aMessageInfo, client->mEchoRequests.mEchoRequestTime[index]);
                client->DeleteEchoRequest(aIcmpHeader.GetSequence());
            }
        }
    }

exit:
    return kThreadError_None;
}

ThreadError Icmp::UpdateChecksum(Message &aMessage, uint16_t aChecksum)
{
    aChecksum = aMessage.UpdateChecksum(aChecksum, aMessage.GetOffset(),
                                        aMessage.GetLength() - aMessage.GetOffset());

    if (aChecksum != 0xffff)
    {
        aChecksum = ~aChecksum;
    }

    aChecksum = HostSwap16(aChecksum);
    aMessage.Write(aMessage.GetOffset() + IcmpHeader::GetChecksumOffset(), sizeof(aChecksum), &aChecksum);
    return kThreadError_None;
}

bool Icmp::IsEchoEnabled(void)
{
    return sIsEchoEnabled;
}

void Icmp::SetEchoEnabled(bool aEnabled)
{
    sIsEchoEnabled = aEnabled;
}

}  // namespace Ip6
}  // namespace Thread
