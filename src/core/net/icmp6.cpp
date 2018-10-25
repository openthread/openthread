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

#include "icmp6.hpp"

#include "utils/wrap_string.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Ip6 {

Icmp::Icmp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mHandlers(NULL)
    , mEchoSequence(1)
    , mEchoMode(OT_ICMP6_ECHO_HANDLER_ALL)
{
}

Message *Icmp::NewMessage(uint16_t aReserved)
{
    return GetIp6().NewMessage(sizeof(IcmpHeader) + aReserved);
}

otError Icmp::RegisterHandler(IcmpHandler &aHandler)
{
    otError error = OT_ERROR_NONE;

    for (IcmpHandler *cur = mHandlers; cur; cur = cur->GetNext())
    {
        if (cur == &aHandler)
        {
            ExitNow(error = OT_ERROR_ALREADY);
        }
    }

    aHandler.mNext = mHandlers;
    mHandlers      = &aHandler;

exit:
    return error;
}

otError Icmp::SendEchoRequest(Message &aMessage, const MessageInfo &aMessageInfo, uint16_t aIdentifier)
{
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfoLocal;
    IcmpHeader  icmpHeader;

    messageInfoLocal = aMessageInfo;

    icmpHeader.Init();
    icmpHeader.SetType(IcmpHeader::kTypeEchoRequest);
    icmpHeader.SetId(aIdentifier);
    icmpHeader.SetSequence(mEchoSequence++);

    SuccessOrExit(error = aMessage.Prepend(&icmpHeader, sizeof(icmpHeader)));
    aMessage.SetOffset(0);
    SuccessOrExit(error = GetIp6().SendDatagram(aMessage, messageInfoLocal, kProtoIcmp6));

    otLogInfoIcmp("Sent echo request: (seq = %d)", icmpHeader.GetSequence());

exit:
    return error;
}

otError Icmp::SendError(IcmpHeader::Type   aType,
                        IcmpHeader::Code   aCode,
                        const MessageInfo &aMessageInfo,
                        const Header &     aHeader)
{
    otError     error = OT_ERROR_NONE;
    MessageInfo messageInfoLocal;
    Message *   message = NULL;
    IcmpHeader  icmp6Header;

    messageInfoLocal = aMessageInfo;

    VerifyOrExit((message = GetIp6().NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = message->SetLength(sizeof(icmp6Header) + sizeof(aHeader)));

    message->Write(sizeof(icmp6Header), sizeof(aHeader), &aHeader);

    icmp6Header.Init();
    icmp6Header.SetType(aType);
    icmp6Header.SetCode(aCode);
    message->Write(0, sizeof(icmp6Header), &icmp6Header);

    SuccessOrExit(error = GetIp6().SendDatagram(*message, messageInfoLocal, kProtoIcmp6));

    otLogInfoIcmp("Sent ICMPv6 Error");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Icmp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    otError    error = OT_ERROR_NONE;
    uint16_t   payloadLength;
    IcmpHeader icmp6Header;
    uint16_t   checksum;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(icmp6Header), &icmp6Header) == sizeof(icmp6Header),
                 error = OT_ERROR_PARSE);
    payloadLength = aMessage.GetLength() - aMessage.GetOffset();

    // verify checksum
    checksum = Ip6::ComputePseudoheaderChecksum(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(), payloadLength,
                                                kProtoIcmp6);
    checksum = aMessage.UpdateChecksum(checksum, aMessage.GetOffset(), payloadLength);
    VerifyOrExit(checksum == 0xffff, error = OT_ERROR_PARSE);

    if (icmp6Header.GetType() == IcmpHeader::kTypeEchoRequest)
    {
        SuccessOrExit(error = HandleEchoRequest(aMessage, aMessageInfo));
    }

    aMessage.MoveOffset(sizeof(icmp6Header));

    for (IcmpHandler *handler = mHandlers; handler; handler = handler->GetNext())
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
    IcmpHeader  icmp6Header;
    Message *   replyMessage = NULL;
    MessageInfo replyMessageInfo;
    uint16_t    payloadLength;

    // always handle Echo Request destined for RLOC or ALOC
    VerifyOrExit(ShouldHandleEchoRequest(aMessageInfo) || aMessageInfo.GetSockAddr().IsRoutingLocator() ||
                 aMessageInfo.GetSockAddr().IsAnycastRoutingLocator());

    otLogInfoIcmp("Received Echo Request");

    icmp6Header.Init();
    icmp6Header.SetType(IcmpHeader::kTypeEchoReply);

    if ((replyMessage = GetIp6().NewMessage(0)) == NULL)
    {
        otLogDebgIcmp("Failed to allocate a new message");
        ExitNow();
    }

    payloadLength = aRequestMessage.GetLength() - aRequestMessage.GetOffset() - IcmpHeader::GetDataOffset();
    SuccessOrExit(error = replyMessage->SetLength(IcmpHeader::GetDataOffset() + payloadLength));

    replyMessage->Write(0, IcmpHeader::GetDataOffset(), &icmp6Header);
    aRequestMessage.CopyTo(aRequestMessage.GetOffset() + IcmpHeader::GetDataOffset(), IcmpHeader::GetDataOffset(),
                           payloadLength, *replyMessage);

    replyMessageInfo.SetPeerAddr(aMessageInfo.GetPeerAddr());

    if (!aMessageInfo.GetSockAddr().IsMulticast())
    {
        replyMessageInfo.SetSockAddr(aMessageInfo.GetSockAddr());
    }

    replyMessageInfo.SetInterfaceId(aMessageInfo.mInterfaceId);

    SuccessOrExit(error = GetIp6().SendDatagram(*replyMessage, replyMessageInfo, kProtoIcmp6));

    replyMessage->Read(replyMessage->GetOffset(), sizeof(icmp6Header), &icmp6Header);
    otLogInfoIcmp("Sent Echo Reply (seq = %d)", icmp6Header.GetSequence());

exit:

    if (error != OT_ERROR_NONE && replyMessage != NULL)
    {
        replyMessage->Free();
    }

    return error;
}

otError Icmp::UpdateChecksum(Message &aMessage, uint16_t aChecksum)
{
    aChecksum = aMessage.UpdateChecksum(aChecksum, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());

    if (aChecksum != 0xffff)
    {
        aChecksum = ~aChecksum;
    }

    aChecksum = HostSwap16(aChecksum);
    aMessage.Write(aMessage.GetOffset() + IcmpHeader::GetChecksumOffset(), sizeof(aChecksum), &aChecksum);
    return OT_ERROR_NONE;
}

} // namespace Ip6
} // namespace ot
