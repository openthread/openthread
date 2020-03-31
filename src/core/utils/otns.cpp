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

void OtnsStub::EmitShortAddress(uint16_t aShortAddress)
{
    EmitStatus("rloc16=%d", aShortAddress);
}

void OtnsStub::EmitExtendedAddress(const Mac::ExtAddress &aExtAddress)
{
    Mac::ExtAddress revExtAddress;
    revExtAddress.Set(aExtAddress.m8, Mac::ExtAddress::kReverseByteOrder);
    EmitStatus("extaddr=%s", revExtAddress.ToString().AsCString());
}

void OtnsStub::EmitPingRequest(const Ip6::Address &aPeerAddress,
                               uint16_t            aPingLength,
                               uint32_t            aTimestamp,
                               uint8_t             aHopLimit)
{
    OT_UNUSED_VARIABLE(aHopLimit);
    EmitStatus("ping_request=%s,%d,%lu", aPeerAddress.ToString().AsCString(), aPingLength, aTimestamp);
}

void OtnsStub::EmitPingReply(const Ip6::Address &aPeerAddress,
                             uint16_t            aPingLength,
                             uint32_t            aTimestamp,
                             uint8_t             aHopLimit)
{
    EmitStatus("ping_reply=%s,%u,%lu,%d", aPeerAddress.ToString().AsCString(), aPingLength, aTimestamp, aHopLimit);
}

void OtnsStub::EmitStatus(const char *aFmt, ...)
{
    static char statusStr[kMaxStatusStringLength + 1];
    int         n;

    va_list ap;
    va_start(ap, aFmt);

    n = vsnprintf(statusStr, sizeof(statusStr), aFmt, ap);
    OT_ASSERT(n >= 0);

    va_end(ap);

    otPlatOtnsStatus(statusStr);
}

void OtnsStub::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<OtnsStub>().HandleStateChanged(aFlags);
}

void OtnsStub::HandleStateChanged(otChangedFlags aFlags)
{
    if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
    {
        EmitStatus("role=%d", Get<Mle::Mle>().GetRole());
    }

    if ((aFlags & OT_CHANGED_THREAD_PARTITION_ID) != 0)
    {
        EmitStatus("parid=%x", Get<Mle::Mle>().GetLeaderData().GetPartitionId());
    }

    if ((aFlags & OT_CHANGED_JOINER_STATE) != 0)
    {
        EmitStatus("joiner_state=%d", Get<MeshCoP::Joiner>().GetState());
    }
}

void OtnsStub::EmitNeighborChange(otNeighborTableEvent aEvent, Neighbor &aNeighbor)
{
    const char *extAddr = aNeighbor.GetExtAddress().ToString().AsCString();

    switch (aEvent)
    {
    case OT_NEIGHBOR_TABLE_EVENT_ROUTER_ADDED:
        EmitStatus("router_added=%s", extAddr);
        break;
    case OT_NEIGHBOR_TABLE_EVENT_ROUTER_REMOVED:
        EmitStatus("router_removed=%s", extAddr);
        break;
    case OT_NEIGHBOR_TABLE_EVENT_CHILD_ADDED:
        EmitStatus("child_added=%s", extAddr);
        break;
    case OT_NEIGHBOR_TABLE_EVENT_CHILD_REMOVED:
        EmitStatus("child_removed=%s", extAddr);
        break;
    }
}

} // namespace Utils
} // namespace ot

#endif // (OPENTHREAD_MTD || OPENTHREAD_FTD) && OPENTHREAD_CONFIG_OTNS_ENABLE
