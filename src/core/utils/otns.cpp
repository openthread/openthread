/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file implements OTNS utilities.
 */

#include "otns.hpp"

#if OPENTHREAD_CONFIG_OTNS_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Utils {

RegisterLogModule("Otns");

void Otns::EmitShortAddress(uint16_t aShortAddress) const { EmitStatus("rloc16=%d", aShortAddress); }

void Otns::EmitExtendedAddress(const Mac::ExtAddress &aExtAddress) const
{
    Mac::ExtAddress revExtAddress;

    revExtAddress.Set(aExtAddress.m8, Mac::ExtAddress::kReverseByteOrder);
    EmitStatus("extaddr=%s", revExtAddress.ToString().AsCString());
}

void Otns::EmitStatus(const char *aFmt, ...) const
{
    StatusString string;
    va_list      args;

    va_start(args, aFmt);
    string.AppendVarArgs(aFmt, args);
    va_end(args);

    EmitStatus(string);
}

void Otns::EmitStatus(const StatusString &aString) const { otPlatOtnsStatus(aString.AsCString()); }

void Otns::EmitTransmit(const Mac::TxFrame &aFrame) const
{
    StatusString string;
    Mac::Address dst;

    IgnoreError(aFrame.GetDstAddr(dst));

    string.Append("transmit=%d,%04x,%d", aFrame.GetChannel(), aFrame.GetFrameControlField(), aFrame.GetSequence());

    if (dst.IsShort())
    {
        string.Append(",%04x", dst.GetShort());
    }
    else if (dst.IsExtended())
    {
        string.Append(",%s", dst.ToString().AsCString());
    }

    EmitStatus(string);
}

#if OPENTHREAD_MTD || OPENTHREAD_FTD
void Otns::EmitPingRequest(const Ip6::Address &aPeerAddress,
                           uint16_t            aPingLength,
                           uint32_t            aTimestamp,
                           uint8_t             aHopLimit) const
{
    OT_UNUSED_VARIABLE(aHopLimit);
    EmitStatus("ping_request=%s,%d,%lu", aPeerAddress.ToString().AsCString(), aPingLength, ToUlong(aTimestamp));
}

void Otns::EmitPingReply(const Ip6::Address &aPeerAddress,
                         uint16_t            aPingLength,
                         uint32_t            aTimestamp,
                         uint8_t             aHopLimit) const
{
    EmitStatus("ping_reply=%s,%u,%lu,%d", aPeerAddress.ToString().AsCString(), aPingLength, ToUlong(aTimestamp),
               aHopLimit);
}

void Otns::HandleNotifierEvents(Events aEvents) const
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        EmitStatus("role=%d", Get<Mle::Mle>().GetRole());
    }

    if (aEvents.Contains(kEventThreadPartitionIdChanged))
    {
        EmitStatus("parid=%lx", ToUlong(Get<Mle::Mle>().GetLeaderData().GetPartitionId()));
    }

#if OPENTHREAD_CONFIG_JOINER_ENABLE
    if (aEvents.Contains(kEventJoinerStateChanged))
    {
        EmitStatus("joiner_state=%d", Get<MeshCoP::Joiner>().GetState());
    }
#endif
}

void Otns::EmitNeighborChange(NeighborTable::Event aEvent, const Neighbor &aNeighbor) const
{
    StatusString string;

    switch (aEvent)
    {
    case NeighborTable::kRouterAdded:
        string.Append("router_added");
        break;
    case NeighborTable::kRouterRemoved:
        string.Append("router_removed");
        break;
    case NeighborTable::kChildAdded:
        string.Append("child_added");
        break;
    case NeighborTable::kChildRemoved:
        string.Append("child_removed");
        break;
    case NeighborTable::kChildModeChanged:
        ExitNow();
    }

    string.Append("=%s", aNeighbor.GetExtAddress().ToString().AsCString());
    EmitStatus(string);

exit:
    return;
}

void Otns::EmitDeviceMode(Mle::DeviceMode aMode) const
{
    StatusString string;

    string.Append("mode=");

    if (aMode.IsRxOnWhenIdle())
    {
        string.Append("r");
    }

    if (aMode.IsFullThreadDevice())
    {
        string.Append("d");
    }

    if (aMode.GetNetworkDataType() == NetworkData::kFullSet)
    {
        string.Append("m");
    }

    EmitStatus(string);
}

void Otns::EmitCoapSend(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    EmitCoapStatus("send", aMessage, aMessageInfo);
}

void Otns::EmitCoapReceive(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    EmitCoapStatus("recv", aMessage, aMessageInfo);
}

void Otns::EmitCoapSendFailure(Error aError, Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo) const
{
    EmitCoapStatus("send_error", aMessage, aMessageInfo, &aError);
}

void Otns::EmitCoapStatus(const char             *aAction,
                          const Coap::Message    &aMessage,
                          const Ip6::MessageInfo &aMessageInfo,
                          Error                  *aError) const
{
    Error        error;
    char         uriPath[Coap::Message::kMaxReceivedUriPath + 1];
    StatusString string;

    SuccessOrExit(error = aMessage.ReadUriPathOptions(uriPath));

    string.Append("coap=%s,%d,%d,%d,%s,%s,%d", aAction, aMessage.GetMessageId(), aMessage.GetType(), aMessage.GetCode(),
                  uriPath, aMessageInfo.GetPeerAddr().ToString().AsCString(), aMessageInfo.GetPeerPort());

    if (aError != nullptr)
    {
        string.Append(",%s", ErrorToString(*aError));
    }

    EmitStatus(string);

exit:
    LogWarnOnError(error, "EmitCoapStatus");
}
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_OTNS_ENABLE
