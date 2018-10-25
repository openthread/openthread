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

#include "utils/wrap_string.h"

#include "thread/child_table.hpp"
#include "thread/mle.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/router_table.hpp"
#include "thread/thread_tlvs.hpp"

namespace ot {
namespace Mle {

class MleRouter : public Mle
{
    friend class Mle;

public:
    explicit MleRouter(Instance &aInstance)
        : Mle(aInstance)
        , mChildTable(aInstance)
        , mRouterTable(aInstance)
    {
    }

    bool IsRouterRoleEnabled(void) const { return false; }

    bool IsSingleton(void) { return false; }

    otError BecomeRouter(ThreadStatusTlv::Status) { return OT_ERROR_NOT_CAPABLE; }
    otError BecomeLeader(void) { return OT_ERROR_NOT_CAPABLE; }

    uint8_t GetRouterSelectionJitterTimeout(void) { return 0; }

    uint32_t GetPreviousPartitionId(void) const { return 0; }
    void     SetPreviousPartitionId(uint32_t) {}
    void     SetRouterId(uint8_t) {}

    uint16_t GetNextHop(uint16_t aDestination) const { return Mle::GetNextHop(aDestination); }

    uint8_t GetNetworkIdTimeout(void) const { return 0; }

    uint8_t GetRouteCost(uint16_t) const { return 0; }
    uint8_t GetLinkCost(uint16_t) { return 0; }
    uint8_t GetCost(uint16_t) { return 0; }

    otError RemoveNeighbor(const Mac::Address &) { return BecomeDetached(); }
    otError RemoveNeighbor(Neighbor &) { return BecomeDetached(); }

    ChildTable & GetChildTable(void) { return mChildTable; }
    RouterTable &GetRouterTable(void) { return mRouterTable; }

    bool IsMinimalChild(uint16_t) { return false; }

    void    RestoreChildren(void) {}
    otError RemoveStoredChild(uint16_t) { return OT_ERROR_NOT_IMPLEMENTED; }
    otError StoreChild(const Child &) { return OT_ERROR_NOT_IMPLEMENTED; }

    Neighbor *GetNeighbor(uint16_t aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetNeighbor(const Mac::ExtAddress &aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetNeighbor(const Mac::Address &aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetNeighbor(const Ip6::Address &aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetRxOnlyNeighborRouter(const Mac::Address &aAddress)
    {
        OT_UNUSED_VARIABLE(aAddress);
        return NULL;
    }

    otError GetNextNeighborInfo(otNeighborInfoIterator &, otNeighborInfo &) { return OT_ERROR_NOT_IMPLEMENTED; }

    static int ComparePartitions(bool, const LeaderDataTlv &, bool, const LeaderDataTlv &) { return 0; }

    void ResolveRoutingLoops(uint16_t, uint16_t) {}

    otError CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header)
    {
        return Mle::CheckReachability(aMeshSource, aMeshDest, aIp6Header);
    }

    static bool IsRouterIdValid(uint8_t aRouterId) { return aRouterId <= kMaxRouterId; }

    void FillConnectivityTlv(ConnectivityTlv &) {}

    otError SendChildUpdateRequest(void) { return Mle::SendChildUpdateRequest(); }

#if OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
    otError SetSteeringData(const otExtAddress *) { return OT_ERROR_NOT_IMPLEMENTED; }
#endif // OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB

    otError GetMaxChildTimeout(uint32_t &) { return OT_ERROR_NOT_IMPLEMENTED; }

    bool HasSleepyChildrenSubscribed(const Ip6::Address &) { return false; }

    bool IsSleepyChildSubscribed(const Ip6::Address &, Child &) { return false; }

private:
    otError HandleDetachStart(void) { return OT_ERROR_NONE; }
    otError HandleChildStart(AttachMode) { return OT_ERROR_NONE; }
    otError HandleLinkRequest(const Message &, const Ip6::MessageInfo &) { return OT_ERROR_DROP; }
    otError HandleLinkAccept(const Message &, const Ip6::MessageInfo &, uint32_t) { return OT_ERROR_DROP; }
    otError HandleLinkAccept(const Message &, const Ip6::MessageInfo &, uint32_t, bool) { return OT_ERROR_DROP; }
    otError HandleLinkAcceptAndRequest(const Message &, const Ip6::MessageInfo &, uint32_t) { return OT_ERROR_DROP; }
    otError HandleAdvertisement(const Message &, const Ip6::MessageInfo &) { return OT_ERROR_DROP; }
    otError HandleParentRequest(const Message &, const Ip6::MessageInfo &) { return OT_ERROR_DROP; }
    otError HandleChildIdRequest(const Message &, const Ip6::MessageInfo &, uint32_t) { return OT_ERROR_DROP; }
    otError HandleChildUpdateRequest(const Message &, const Ip6::MessageInfo &, uint32_t) { return OT_ERROR_DROP; }
    otError HandleChildUpdateResponse(const Message &, const Ip6::MessageInfo &, uint32_t) { return OT_ERROR_DROP; }
    otError HandleDataRequest(const Message &, const Ip6::MessageInfo &) { return OT_ERROR_DROP; }
    otError HandleNetworkDataUpdateRouter(void) { return OT_ERROR_NONE; }
    otError HandleDiscoveryRequest(const Message &, const Ip6::MessageInfo &) { return OT_ERROR_DROP; }
    void    HandlePartitionChange(void) {}
    void    StopAdvertiseTimer(void) {}
    otError ProcessRouteTlv(const RouteTlv &) { return OT_ERROR_NONE; }
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    otError HandleTimeSync(const Message &, const Ip6::MessageInfo &) { return OT_ERROR_DROP; }
#endif

    ChildTable  mChildTable;
    RouterTable mRouterTable;
};

} // namespace Mle
} // namespace ot

#endif // MLE_ROUTER_MTD_HPP_
