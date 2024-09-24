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

#include "icmp6.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace Ip6 {

RegisterLogModule("Icmp6");

Icmp::Icmp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEchoSequence(1)
    , mEchoMode(OT_ICMP6_ECHO_HANDLER_ALL)
{
}

Message *Icmp::NewMessage(void) { return Get<Ip6>().NewMessage(sizeof(Header)); }

Error Icmp::RegisterHandler(Handler &aHandler) { return mHandlers.Add(aHandler); }

Error Icmp::SendEchoRequest(Message &aMessage, const MessageInfo &aMessageInfo, uint16_t aIdentifier)
{
    Error       error = kErrorNone;
    MessageInfo messageInfoLocal;
    Header      icmpHeader;

    messageInfoLocal = aMessageInfo;

    icmpHeader.Clear();
    icmpHeader.SetType(Header::kTypeEchoRequest);
    icmpHeader.SetId(aIdentifier);
    icmpHeader.SetSequence(mEchoSequence++);

    SuccessOrExit(error = aMessage.Prepend(icmpHeader));
    aMessage.SetOffset(0);
    SuccessOrExit(error = Get<Ip6>().SendDatagram(aMessage, messageInfoLocal, kProtoIcmp6));

    LogInfo("Sent echo request: (seq = %d)", icmpHeader.GetSequence());

exit:
    return error;
}

Error Icmp::SendError(Header::Type aType, Header::Code aCode, const MessageInfo &aMessageInfo, const Message &aMessage)
{
    Error   error;
    Headers headers;

    SuccessOrExit(error = headers.ParseFrom(aMessage));
    error = SendError(aType, aCode, aMessageInfo, headers);

exit:
    return error;
}

Error Icmp::SendError(Header::Type aType, Header::Code aCode, const MessageInfo &aMessageInfo, const Headers &aHeaders)
{
    Error             error = kErrorNone;
    MessageInfo       messageInfoLocal;
    Message          *message = nullptr;
    Header            icmp6Header;
    Message::Settings settings(Message::kWithLinkSecurity, Message::kPriorityNet);

    if (aHeaders.GetIpProto() == kProtoIcmp6)
    {
        VerifyOrExit(!aHeaders.GetIcmpHeader().IsError());
    }

    messageInfoLocal = aMessageInfo;

    VerifyOrExit((message = Get<Ip6>().NewMessage(0, settings)) != nullptr, error = kErrorNoBufs);

    // Prepare the ICMPv6 error message. We only include the IPv6 header
    // of the original message causing the error.

    icmp6Header.Clear();
    icmp6Header.SetType(aType);
    icmp6Header.SetCode(aCode);
    SuccessOrExit(error = message->Append(icmp6Header));
    SuccessOrExit(error = message->Append(aHeaders.GetIp6Header()));

    SuccessOrExit(error = Get<Ip6>().SendDatagram(*message, messageInfoLocal, kProtoIcmp6));

    LogInfo("Sent ICMPv6 Error");

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error Icmp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    Error  error = kErrorNone;
    Header icmp6Header;

    SuccessOrExit(error = aMessage.Read(aMessage.GetOffset(), icmp6Header));

    SuccessOrExit(error = Checksum::VerifyMessageChecksum(aMessage, aMessageInfo, kProtoIcmp6));

    if (icmp6Header.GetType() == Header::kTypeEchoRequest)
    {
        SuccessOrExit(error = HandleEchoRequest(aMessage, aMessageInfo));
    }

    aMessage.MoveOffset(sizeof(icmp6Header));

    for (Handler &handler : mHandlers)
    {
        handler.HandleReceiveMessage(aMessage, aMessageInfo, icmp6Header);
    }

exit:
    return error;
}

bool Icmp::ShouldHandleEchoRequest(const Address &aAddress)
{
    bool rval = false;

    switch (mEchoMode)
    {
    case OT_ICMP6_ECHO_HANDLER_DISABLED:
        rval = false;
        break;
    case OT_ICMP6_ECHO_HANDLER_UNICAST_ONLY:
        rval = !aAddress.IsMulticast();
        break;
    case OT_ICMP6_ECHO_HANDLER_MULTICAST_ONLY:
        rval = aAddress.IsMulticast();
        break;
    case OT_ICMP6_ECHO_HANDLER_ALL:
        rval = true;
        break;
    case OT_ICMP6_ECHO_HANDLER_RLOC_ALOC_ONLY:
        rval = aAddress.GetIid().IsLocator();
        break;
    }

    return rval;
}

Error Icmp::HandleEchoRequest(Message &aRequestMessage, const MessageInfo &aMessageInfo)
{
    Error       error = kErrorNone;
    Header      icmp6Header;
    Message    *replyMessage = nullptr;
    MessageInfo replyMessageInfo;
    uint16_t    dataOffset;

    VerifyOrExit(ShouldHandleEchoRequest(aMessageInfo.GetSockAddr()));

    LogInfo("Received Echo Request");

    icmp6Header.Clear();
    icmp6Header.SetType(Header::kTypeEchoReply);

    if ((replyMessage = Get<Ip6>().NewMessage(0)) == nullptr)
    {
        LogDebg("Failed to allocate a new message");
        ExitNow();
    }

    dataOffset = aRequestMessage.GetOffset() + Header::kDataFieldOffset;

    SuccessOrExit(error = replyMessage->AppendBytes(&icmp6Header, Header::kDataFieldOffset));
    SuccessOrExit(error = replyMessage->AppendBytesFromMessage(aRequestMessage, dataOffset,
                                                               aRequestMessage.GetLength() - dataOffset));

    replyMessageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());

    if (!aMessageInfo.GetSockAddr().IsMulticast())
    {
        replyMessageInfo.SetSockAddr(aMessageInfo.GetSockAddr());
    }

    SuccessOrExit(error = Get<Ip6>().SendDatagram(*replyMessage, replyMessageInfo, kProtoIcmp6));

    IgnoreError(replyMessage->Read(replyMessage->GetOffset(), icmp6Header));
    LogInfo("Sent Echo Reply (seq = %d)", icmp6Header.GetSequence());

exit:
    FreeMessageOnError(replyMessage, error);
    return error;
}

} // namespace Ip6
} // namespace ot
