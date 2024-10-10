/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file implements MeshCoP-Ext functions for the Commissioner class.
 */

#include "commissioner.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

#include "coap/coap_message.hpp"
#include "common/array.hpp"
#include "common/encoding.hpp"
#include "instance/instance.hpp"
#include "meshcop/joiner.hpp"
#include "net/udp6.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("Commissioner");

void Commissioner::SendToExtCommissioner(MeshCoP::Joiner::Operation aOperation, const Message &aMessage)
{
    OT_UNUSED_VARIABLE(aMessage);

    Error            error   = kErrorNotImplemented;
    Message         *message = nullptr;
    Ip6::MessageInfo msgInfo;
    Ip6::Address     registrarIp6Address;
    uint16_t         len;
    OT_UNUSED_VARIABLE(error);
    OT_UNUSED_VARIABLE(msgInfo);
    OT_UNUSED_VARIABLE(registrarIp6Address);
    OT_UNUSED_VARIABLE(len);

    // FIXME: below code is specific to CCM-BR sending to a known Registrar. In general, every
    // extended commissioner should 'register' itself as an Extended Commissioner destination so
    // that the Commissioner can forward messages without knowing the details of the protocol.
    switch (aOperation)
    {
#if OPENTHREAD_CONFIG_CCM_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    case MeshCoP::Joiner::Operation::kOperationCcmEstCoaps:
        OT_FALL_THROUGH;
    case MeshCoP::Joiner::Operation::kOperationCcmAeCbrski:
        SuccessOrExit(error = Get<BorderRouter::InfraIf>().GetDiscoveredCcmRegistrarAddress(registrarIp6Address));
        msgInfo.SetPeerAddr(registrarIp6Address);
        msgInfo.SetPeerPort(OT_DEFAULT_COAP_SECURE_PORT);
        msgInfo.SetIsHostInterface(true);

        // create copy of message, to send out
        len     = aMessage.GetLength() - aMessage.GetOffset();
        message = Get<Ip6::Udp>().NewMessage(len);
        VerifyOrExit(message != nullptr, error = kErrorNoBufs);
        SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, aMessage.GetOffset(), len));

        // FIXME allow multiple mExtCommSocket for multiple Joiners
        SuccessOrExit(error = Get<Ip6::Udp>().SendTo(mExtCommSocket, *message, msgInfo));
        LogDebg("Sent to Ext Commissioner: %d B srcPort=%d dstPort=%d", len, msgInfo.GetSockPort(),
                msgInfo.GetPeerPort());
        break;
#endif
    default:
        ExitNow(error = kErrorNotImplemented);
    }

exit:
    LogWarnOnError(error, "send msg to Ext Commissioner");
    if (message != nullptr)
    {
        message->Free();
    }
}

void Commissioner::HandleExtCommissionerCallback(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    reinterpret_cast<Commissioner *>(aContext)->HandleExtCommissionerCallback(
        static_cast<const Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleExtCommissionerCallback(const Message *aMessage, const Ip6::MessageInfo *aMessageInfo)
{
    Error    error = kErrorNone;
    Message *txMsg;

    VerifyOrExit(IsActive());

    // create new Relay Tx message to be sent out to JR (FIXME: allow multiple concurrent Joiners/JRs)
    txMsg = Get<Ip6::Udp>().NewMessage(aMessage->GetLength());
    VerifyOrExit(txMsg != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = txMsg->AppendBytesFromMessage(*aMessage, aMessage->GetOffset(), aMessage->GetLength()));
    error = this->SendRelayTransmit(*txMsg, *aMessageInfo);

exit:
    // txMsg is already freed by SendRelayTransmit(); while aMessage MUST NOT be freed.
    LogWarnOnError(error, "Handle Ext Commissioner msg failed");
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
