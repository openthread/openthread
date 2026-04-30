/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements IPv6 Neighbor Discovery Agent.
 */

#include "nd_agent.hpp"

#if OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace NeighborDiscovery {

void Agent::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        UpdateService();
    }
}

void Agent::UpdateService(void)
{
    uint16_t                        rloc16 = Get<Mle::Mle>().GetRloc16();
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    Lowpan::Context                 lowpanContext;

    if (IsAlocInUse())
    {
        uint8_t contextId = Mle::Aloc16::ToNdAgentContextId(mAloc.GetAddress().GetIid().GetLocator());
        bool    found     = false;

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNext(iterator, rloc16, prefixConfig) == kErrorNone)
        {
            if (!prefixConfig.mNdDns)
            {
                continue;
            }

            Get<NetworkData::Leader>().FindContextForAddress(AsCoreType(&prefixConfig.mPrefix.mPrefix), lowpanContext);

            if (lowpanContext.MatchesContextId(contextId))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Get<ThreadNetif>().RemoveUnicastAddress(mAloc);
            FreeAloc();
        }
    }

    VerifyOrExit(!IsAlocInUse());

    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNext(iterator, rloc16, prefixConfig) == kErrorNone)
    {
        if (!prefixConfig.mNdDns)
        {
            continue;
        }

        Get<NetworkData::Leader>().FindContextForAddress(AsCoreType(&prefixConfig.mPrefix.mPrefix), lowpanContext);

        if (lowpanContext.IsValid())
        {
            uint16_t aloc16 = Mle::Aloc16::FromNdAgentContextId(lowpanContext.GetContextId());

            mAloc.InitAsThreadOrigin();
            mAloc.GetAddress().SetToAnycastLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), aloc16);
            mAloc.mMeshLocal = true;
            Get<ThreadNetif>().AddUnicastAddress(mAloc);
            ExitNow();
        }
    }

exit:
    return;
}

} // namespace NeighborDiscovery
} // namespace ot

#endif // OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
