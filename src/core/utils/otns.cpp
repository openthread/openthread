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
 *
 */

#include "otns.hpp"

#if (OPENTHREAD_MTD || OPENTHREAD_FTD) && OPENTHREAD_CONFIG_OTNS_ENABLE

#include "common/debug.hpp"
#include "common/locator-getters.hpp"

namespace ot {
namespace Utils {

const int kMaxStatusStringLength = 128;

void Otns::EmitShortAddress(uint16_t aShortAddress)
{
    EmitStatus("rloc16=%d", aShortAddress);
}

void Otns::EmitExtendedAddress(const Mac::ExtAddress &aExtAddress)
{
    Mac::ExtAddress revExtAddress;
    revExtAddress.Set(aExtAddress.m8, Mac::ExtAddress::kReverseByteOrder);
    EmitStatus("extaddr=%s", revExtAddress.ToString().AsCString());
}

void Otns::EmitPingRequest(const Ip6::Address &aPeerAddress,
                           uint16_t            aPingLength,
                           uint32_t            aTimestamp,
                           uint8_t             aHopLimit)
{
    OT_UNUSED_VARIABLE(aHopLimit);
    EmitStatus("ping_request=%s,%d,%lu", aPeerAddress.ToString().AsCString(), aPingLength, aTimestamp);
}

void Otns::EmitPingReply(const Ip6::Address &aPeerAddress, uint16_t aPingLength, uint32_t aTimestamp, uint8_t aHopLimit)
{
    EmitStatus("ping_reply=%s,%u,%lu,%d", aPeerAddress.ToString().AsCString(), aPingLength, aTimestamp, aHopLimit);
}

void Otns::EmitStatus(const char *aFmt, ...)
{
    char statusStr[kMaxStatusStringLength + 1];
    int  n;

    va_list ap;
    va_start(ap, aFmt);

    n = vsnprintf(statusStr, sizeof(statusStr), aFmt, ap);
    OT_ASSERT(n >= 0);

    va_end(ap);

    otPlatOtnsStatus(statusStr);
}

void Otns::HandleNotifierEvents(Notifier::Callback &aCallback, Events aEvents)
{
    aCallback.GetOwner<Otns>().HandleNotifierEvents(aEvents);
}

void Otns::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        EmitStatus("role=%d", Get<Mle::Mle>().GetRole());
    }

    if (aEvents.Contains(kEventThreadPartitionIdChanged))
    {
        EmitStatus("parid=%x", Get<Mle::Mle>().GetLeaderData().GetPartitionId());
    }

    if (aEvents.Contains(kEventJoinerStateChanged))
    {
        EmitStatus("joiner_state=%d", Get<MeshCoP::Joiner>().GetState());
    }
}

void Otns::EmitNeighborChange(otNeighborTableEvent aEvent, Neighbor &aNeighbor)
{
    switch (aEvent)
    {
    case OT_NEIGHBOR_TABLE_EVENT_ROUTER_ADDED:
        EmitStatus("router_added=%s", aNeighbor.GetExtAddress().ToString().AsCString());
        break;
    case OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED:
        EmitStatus("router_removed=%s", aNeighbor.GetExtAddress().ToString().AsCString());
        break;
    case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:
        EmitStatus("child_added=%s", aNeighbor.GetExtAddress().ToString().AsCString());
        break;
    case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED:
        EmitStatus("child_removed=%s", aNeighbor.GetExtAddress().ToString().AsCString());
        break;
    }
}

} // namespace Utils
} // namespace ot

#endif // (OPENTHREAD_MTD || OPENTHREAD_FTD) && OPENTHREAD_CONFIG_OTNS_ENABLE
