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

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Ip6 {

Icmp::Icmp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mHandlers()
    , mEchoSequence(1)
    , mEchoMode(OT_ICMP6_ECHO_HANDLER_ALL)
{
}

Message *Icmp::NewMessage(uint16_t aReserved)
{
    return Get<Ip6>().NewMessage(sizeof(Header) + aReserved);
}

otError Icmp::RegisterHandler(Handler &aHandler)
{
    return mHandlers.Add(aHandler);
}

otError Icmp::SendEchoRequest(Message &aMessage, const MessageInfo &aMessageInfo, uint16_t aIdentifier)
{
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfoLocal;
    Header      icmpHeader;

    messageInfoLocal = aMessageInfo;

    icmpHeader.Clear();
    icmpHeader.SetType(Header::kTypeEchoRequest);
    icmpHeader.SetId(aIdentifier);
    icmpHeader.SetSequence(mEchoSequence++);

    SuccessOrExit(error = aMessage.Prepend(&icmpHeader, sizeof(icmpHeader)));
    aMessage.SetOffset(0);
    SuccessOrExit(error = Get<Ip6>().SendDatagram(aMessage, messageInfoLocal, kProtoIcmp6));

    otLogInfoIcmp("Sent echo request: (seq = %d)", icmpHeader.GetSequence());

exit:
    return error;
}

otError Icmp::SendError(Header::Type       aType,
                        Header::Code       aCode,
                        const MessageInfo &aMessageInfo,
                        const Message &    aMessage)
{
    otError           error = OT_ERROR_NONE;
    MessageInfo       messageInfoLocal;
    Message *         message = nullptr;
    Header            icmp6Header;
    ot::Ip6::Header   ip6Header;
    Message::Settings settings(Message::kWithLinkSecurity, Message::kPriorityNet);

    VerifyOrExit(aMessage.GetLength() >= sizeof(ip6Header), error = OT_ERROR_INVALID_ARGS);

    aMessage.Read(0, sizeof(ip6Header), &ip6Header);

    if (ip6Header.GetNextHeader() == kProtoIcmp6)
    {
        VerifyOrExit(aMessage.GetLength() >= (sizeof(ip6Header) + sizeof(icmp6Header)), OT_NOOP);

        aMessage.Read(sizeof(ip6Header), sizeof(icmp6Header), &icmp6Header);
        VerifyOrExit(!icmp6Header.IsError(), OT_NOOP);
    }

    messageInfoLocal = aMessageInfo;

    VerifyOrExit((message = Get<Ip6>().NewMessage(0, settings)) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->SetLength(sizeof(icmp6Header) + sizeof(ip6Header)));

    message->Write(sizeof(icmp6Header), sizeof(ip6Header), &ip6Header);

    icmp6Header.Clear();
    icmp6Header.SetType(aType);
    icmp6Header.SetCode(aCode);
    message->Write(0, sizeof(icmp6Header), &icmp6Header);

    SuccessOrExit(error = Get<Ip6>().SendDatagram(*message, messageInfoLocal, kProtoIcmp6));

    otLogInfoIcmp("Sent ICMPv6 Error");

exit:
    FreeMessageOnError(message, error);
    return error;
}

otError Icmp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Header  icmp6Header;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(icmp6Header), &icmp6Header) == sizeof(icmp6Header),
                 error = OT_ERROR_PARSE);

    SuccessOrExit(error = Checksum::VerifyMessageChecksum(aMessage, aMessageInfo, kProtoIcmp6));

    if (icmp6Header.GetType() == Header::kTypeEchoRequest)
    {
        SuccessOrExit(error = HandleEchoRequest(aMessage, aMessageInfo));
    }

    aMessage.MoveOffset(sizeof(icmp6Header));

    for (Handler *handler = mHandlers.GetHead(); handler; handler = handler->GetNext())
    {
        handler->HandleReceiveMessage(aMessage, aMessageInfo, icmp6Header);
    }

exit:
    return error;
}

bool Icmp::ShouldHandleEchoRequest(const MessageInfo &aMessageInfo)
{
    bool rval = false;

    switch (mEchoMode)
    {
    case OT_ICMP6_ECHO_HANDLER_DISABLED:
        rval = false;
        break;
    case OT_ICMP6_ECHO_HANDLER_UNICAST_ONLY:
        rval = !aMessageInfo.GetSockAddr().IsMulticast();
        break;
    case OT_ICMP6_ECHO_HANDLER_MULTICAST_ONLY:
        rval = aMessageInfo.GetSockAddr().IsMulticast();
        break;
    case OT_ICMP6_ECHO_HANDLER_ALL:
        rval = true;
        break;
    }

    return rval;
}

otError Icmp::HandleEchoRequest(Message &aRequestMessage, const MessageInfo &aMessageInfo)
{
    otError     error = OT_ERROR_NONE;
    Header      icmp6Header;
    Message *   replyMessage = nullptr;
    MessageInfo replyMessageInfo;
    uint16_t    payloadLength;

    // always handle Echo Request destined for RLOC or ALOC
    VerifyOrExit(ShouldHandleEchoRequest(aMessageInfo) || aMessageInfo.GetSockAddr().GetIid().IsLocator(), OT_NOOP);

    otLogInfoIcmp("Received Echo Request");

    icmp6Header.Clear();
    icmp6Header.SetType(Header::kTypeEchoReply);

    if ((replyMessage = Get<Ip6>().NewMessage(0)) == nullptr)
    {
        otLogDebgIcmp("Failed to allocate a new message");
        ExitNow();
    }

    payloadLength = aRequestMessage.GetLength() - aRequestMessage.GetOffset() - Header::kDataFieldOffset;
    SuccessOrExit(error = replyMessage->SetLength(Header::kDataFieldOffset + payloadLength));

    replyMessage->Write(0, Header::kDataFieldOffset, &icmp6Header);
    aRequestMessage.CopyTo(aRequestMessage.GetOffset() + Header::kDataFieldOffset, Header::kDataFieldOffset,
                           payloadLength, *replyMessage);

    replyMessageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());

    if (!aMessageInfo.GetSockAddr().IsMulticast())
    {
        replyMessageInfo.SetSockAddr(aMessageInfo.GetSockAddr());
    }

    SuccessOrExit(error = Get<Ip6>().SendDatagram(*replyMessage, replyMessageInfo, kProtoIcmp6));

    replyMessage->Read(replyMessage->GetOffset(), sizeof(icmp6Header), &icmp6Header);
    otLogInfoIcmp("Sent Echo Reply (seq = %d)", icmp6Header.GetSequence());

exit:
    FreeMessageOnError(replyMessage, error);
    return error;
}

} // namespace Ip6
} // namespace ot
