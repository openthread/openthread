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
 *   This file implements CCM Commissioner functions for the Commissioner class.
 */

#include "../commissioner.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

#include <stdio.h>

#include "coap/coap_message.hpp"
#include "common/array.hpp"
#include "common/as_core_type.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/string.hpp"
#include "common/as_core_type.hpp"
#include "instance/instance.hpp"
#include "net/udp6.hpp"
#include "meshcop/joiner.hpp"
#include "meshcop/joiner_router.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("CommissionerC");

void Commissioner::SendBrskiRelayTransmit(const Coap::Message &aRlyMessage, const Ip6::MessageInfo &aMessageInfo,
                                          uint16_t dtlsPayloadOffset, uint16_t dtlsLen,
                                          uint16_t joinerPort, const Ip6::InterfaceIdentifier &joinerIid, uint16_t joinerRloc)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error   error = kErrorNone;
    Message *message = nullptr;
    uint8_t buf[1280]; // FIXME could be dynamic buffer
    uint8_t *bufPtr = &buf[0];
    uint16_t bufLen;

    mJoinerPort = joinerPort;
    mJoinerIid = joinerIid;
    mJoinerRloc = joinerRloc; // FIXME assumes a single joiner and stateful cBRSKI relay. Test only.

    // get DTLS payload from relay msg
    VerifyOrExit(aRlyMessage.GetLength() <= sizeof(buf));
    bufLen = aRlyMessage.ReadBytes(dtlsPayloadOffset, buf, dtlsLen);

    message = this->NewJpyMessage( bufPtr, bufLen, joinerPort, joinerIid, joinerRloc);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = ForwardToRegistrar(*message));
    LogDebg("Sent to Registrar as DTLS: %u bytes", dtlsLen);

exit:
    LogWarnOnError(error, "SendBrskiRelayTransmit() send");
    if (message != nullptr) {
        message->Free();
    }
}

// FIXME return error and set msg via params
Message* Commissioner::NewJpyMessage(const uint8_t *aDtlsData,  uint16_t aDtlsLen, uint16_t joinerPort,
                                     const Ip6::InterfaceIdentifier &joinerIid, uint16_t joinerRloc)
{
    JpyHeader hdr;
    Message   *jpyMsg = nullptr;
    uint16_t  jpyPayloadSize = sizeof(struct JpyHeader) + aDtlsLen + 128; // FIXME

    hdr.mPort = joinerPort;
    hdr.mRloc = joinerRloc;
    hdr.mIid = joinerIid;

    jpyMsg = Get<Ip6::Udp>().NewMessage(jpyPayloadSize);
    if (jpyMsg != nullptr) {
        // FIXME copy header
        //  ((JpyHeader) jpyMsg) = hdr;
        SuccessOrExit(jpyMsg->SetLength(aDtlsLen));
        LogDebg("jpyMsg->GetOffset() = %d", jpyMsg->GetOffset());
        LogDebg("aDtlsLen=%d", aDtlsLen);
        LogDebg("jpyMsg->GetLength() = %d", jpyMsg->GetLength());
        jpyMsg->WriteBytes(jpyMsg->GetOffset(), aDtlsData, aDtlsLen);
    }

exit:
    return jpyMsg;
}

Error Commissioner::ForwardToRegistrar(Message &aJpyMessage)
{
    /*
    Error error;
    Ip6::MessageInfo msgInfo = Ip6::MessageInfo();
    Ip6::Address registrarIp6Address = Ip6::Address();

    SuccessOrExit(error = aForwardMessage.AppendBytesFromMessage(aMessage, aMessage.GetOffset(),
                                                                 aMessage.GetLength() - aMessage.GetOffset()));
    SuccessOrExit(error = registrarIp6Address.FromString("910b::1234")); // FIXME hardcoded
    msgInfo.SetPeerAddr(registrarIp6Address);
    msgInfo.SetPeerPort(5683); // FIXME hardcoded
    msgInfo.SetIsHostInterface(true);
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(aForwardMessage, msgInfo));
*/
    Error error;
    Ip6::MessageInfo msgInfo = Ip6::MessageInfo();
    Ip6::Address registrarIp6Address = Ip6::Address();

    SuccessOrExit(error = registrarIp6Address.FromString("910b::1234")); // FIXME hardcoded
    msgInfo.SetSockPort(0); // FIXME test
    msgInfo.SetPeerAddr(registrarIp6Address);
    msgInfo.SetPeerPort(5684); // FIXME hardcoded for DTLS
    msgInfo.SetIsHostInterface(true);

    SuccessOrExit(error = Get<Ip6::Udp>().SendTo(mRelaySocket, aJpyMessage, msgInfo));
    LogInfo("Sent to Registrar successfully");

exit:
    LogWarnOnError(error, "send to Registrar");
    return error;
}

void Commissioner::HandleRelayRegistrarCallback(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo) {
    reinterpret_cast<Commissioner *>(aContext)->HandleRelayRegistrar(static_cast<Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Commissioner::HandleRelayRegistrar(Message *aMessage, const Ip6::MessageInfo *aMessageInfo) {
    // create new Relay Tx message to be sent out
    Message* txMsg = Get<Ip6::Udp>().NewMessage(aMessage->GetLength());
    txMsg->AppendBytesFromMessage(*aMessage, aMessage->GetOffset(), aMessage->GetLength());
    this->SendRelayTransmit(*txMsg, *aMessageInfo);

    // txMsg is already freed by SendRelayTransmit(); while aMessage MUST NOT be freed.
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
