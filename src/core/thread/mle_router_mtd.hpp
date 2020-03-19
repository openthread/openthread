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
 *   This file includes definitions for MLE functionality required by the Thread Router and Leader roles.
 */

#ifndef MLE_ROUTER_MTD_HPP_
#define MLE_ROUTER_MTD_HPP_

#include "openthread-core-config.h"

#include "thread/mle.hpp"

namespace ot {
namespace Mle {

class MleRouter : public Mle
{
    friend class Mle;
    friend class ot::Instance;

public:
    explicit MleRouter(Instance &aInstance)
        : Mle(aInstance)
    {
    }

    bool IsSingleton(void) const { return false; }

    uint16_t GetNextHop(uint16_t aDestination) const { return Mle::GetNextHop(aDestination); }

    uint8_t GetCost(uint16_t) { return 0; }

    otError RemoveNeighbor(Neighbor &) { return BecomeDetached(); }

    Neighbor *GetNeighbor(const Mac::ExtAddress &aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetNeighbor(const Mac::Address &aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetRxOnlyNeighborRouter(const Mac::Address &aAddress)
    {
        OT_UNUSED_VARIABLE(aAddress);
        return NULL;
    }

    otError GetNextNeighborInfo(otNeighborInfoIterator &, otNeighborInfo &) { return OT_ERROR_NOT_IMPLEMENTED; }

    static bool IsRouterIdValid(uint8_t aRouterId) { return aRouterId <= kMaxRouterId; }

    void FillConnectivityTlv(ConnectivityTlv &) {}

    otError SendChildUpdateRequest(void) { return Mle::SendChildUpdateRequest(); }

    otError GetMaxChildTimeout(uint32_t &) { return OT_ERROR_NOT_IMPLEMENTED; }
};

} // namespace Mle
} // namespace ot

#endif // MLE_ROUTER_MTD_HPP_
