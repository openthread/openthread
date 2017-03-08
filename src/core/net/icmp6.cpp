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
 *   This file implements ICMPv6.
 */

#define WPP_NAME "icmp6.tmh"

#include <string.h>

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <net/icmp6.hpp>
#include <net/ip6.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace Ip6 {

Icmp::Icmp(Ip6 &aIp6):
    mHandlers(NULL),
    mEchoSequence(1),
    mIsEchoEnabled(true),
    mIp6(aIp6)
{
}

Message *Icmp::NewMessage(uint16_t aReserved)
{
    return mIp6.NewMessage(sizeof(IcmpHeader) + aReserved);
}

ThreadError Icmp::RegisterHandler(IcmpHandler &aHandler)
{
    ThreadError error = kThreadError_None;

    for (IcmpHandler *cur = mHandlers; cur; cur = cur->GetNext())
    {
        if (cur == &aHandler)
        {
            ExitNow(error = kThreadError_Already);
        }
    }

    aHandler.mNext = mHandlers;
    mHandlers = &aHandler;

exit:
    return error;
}

ThreadError Icmp::SendEchoRequest(Message &aMessage, const MessageInfo &aMessageInfo,
                                  uint16_t aIdentifier)
{
    ThreadError error = kThreadError_None;
    MessageInfo messageInfoLocal;
    IcmpHeader icmpHeader;

    messageInfoLocal = aMessageInfo;

    icmpHeader.Init();
    icmpHeader.SetType(kIcmp6TypeEchoRequest);
    icmpHeader.SetId(aIdentifier);
    icmpHeader.SetSequence(mEchoSequence++);

    SuccessOrExit(error = aMessage.Prepend(&icmpHeader, sizeof(icmpHeader)));
    aMessage.SetOffset(0);
    SuccessOrExit(error = mIp6.SendDatagram(aMessage, messageInfoLocal, kProtoIcmp6));

    otLogInfoIcmp("Sent echo request: (seq = %d)", icmpHeader.GetSequence());

exit:
    return error;
}

ThreadError Icmp::SendError(IcmpHeader::Type aType, IcmpHeader::Code aCode, const MessageInfo &aMessageInfo,
                            const Header &aHeader)
{
    ThreadError error = kThreadError_None;
    MessageInfo messageInfoLocal;
    Message *message = NULL;
    IcmpHeader icmp6Header;

    messageInfoLocal = aMessageInfo;

    VerifyOrExit((message = mIp6.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = message->SetLength(sizeof(icmp6Header) + sizeof(aHeader)));

    message->Write(sizeof(icmp6Header), sizeof(aHeader), &aHeader);

    icmp6Header.Init();
    icmp6Header.SetType(aType);
    icmp6Header.SetCode(aCode);
    message->Write(0, sizeof(icmp6Header), &icmp6Header);

    SuccessOrExit(error = mIp6.SendDatagram(*message, messageInfoLocal, kProtoIcmp6));

    otLogInfoIcmp("Sent ICMPv6 Error");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

ThreadError Icmp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    uint16_t payloadLength;
    IcmpHeader icmp6Header;
    uint16_t checksum;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(icmp6Header), &icmp6Header) == sizeof(icmp6Header),);
    payloadLength = aMessage.GetLength() - aMessage.GetOffset();

    // verify checksum
    checksum = Ip6::ComputePseudoheaderChecksum(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(),
                                                payloadLength, kProtoIcmp6);
    checksum = aMessage.UpdateChecksum(checksum, aMessage.GetOffset(), payloadLength);
    VerifyOrExit(checksum == 0xffff, ;);

    if (mIsEchoEnabled && (icmp6Header.GetType() == kIcmp6TypeEchoRequest))
    {
        HandleEchoRequest(aMessage, aMessageInfo);
    }

    aMessage.MoveOffset(sizeof(icmp6Header));

    for (IcmpHandler *handler = mHandlers; handler; handler = handler->GetNext())
    {
        handler->HandleReceiveMessage(aMessage, aMessageInfo, icmp6Header);
    }

exit:
    return error;
}

ThreadError Icmp::HandleEchoRequest(Message &aRequestMessage, const MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    IcmpHeader icmp6Header;
    Message *replyMessage = NULL;
    MessageInfo replyMessageInfo;
    uint16_t payloadLength;

    otLogInfoIcmp("Received Echo Request");

    icmp6Header.Init();
    icmp6Header.SetType(kIcmp6TypeEchoReply);

    if ((replyMessage = mIp6.NewMessage(0)) == NULL)
    {
        otLogDebgIcmp("Failed to allocate a new message");
        ExitNow();
    }

    payloadLength = aRequestMessage.GetLength() - aRequestMessage.GetOffset() - IcmpHeader::GetDataOffset();
    SuccessOrExit(error = replyMessage->SetLength(IcmpHeader::GetDataOffset() + payloadLength));

    replyMessage->Write(0, IcmpHeader::GetDataOffset(), &icmp6Header);
    aRequestMessage.CopyTo(aRequestMessage.GetOffset() + IcmpHeader::GetDataOffset(),
                           IcmpHeader::GetDataOffset(), payloadLength, *replyMessage);

    replyMessageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());

    if (!aMessageInfo.GetSockAddr().IsMulticast())
    {
        replyMessageInfo.SetSockAddr(aMessageInfo.GetSockAddr());
    }

    replyMessageInfo.SetInterfaceId(aMessageInfo.mInterfaceId);

    SuccessOrExit(error = mIp6.SendDatagram(*replyMessage, replyMessageInfo, kProtoIcmp6));

    replyMessage->Read(replyMessage->GetOffset(), sizeof(icmp6Header), &icmp6Header);
    otLogInfoIcmp("Sent Echo Reply (seq = %d)", icmp6Header.GetSequence());

exit:

    if (error != kThreadError_None && replyMessage != NULL)
    {
        replyMessage->Free();
    }

    return error;
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
    return mIsEchoEnabled;
}

void Icmp::SetEchoEnabled(bool aEnabled)
{
    mIsEchoEnabled = aEnabled;
}

}  // namespace Ip6
}  // namespace Thread
