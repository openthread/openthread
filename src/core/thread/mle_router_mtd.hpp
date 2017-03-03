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

#ifndef MLE_ROUTER_HPP_
#define MLE_ROUTER_HPP_

#include <string.h>

#include <thread/mle.hpp>
#include <thread/mle_tlvs.hpp>
#include <thread/thread_tlvs.hpp>

namespace Thread {
namespace Mle {

class MleRouter: public Mle
{
    friend class Mle;

public:
    explicit MleRouter(ThreadNetif &aThreadNetif) : Mle(aThreadNetif) { }

    bool IsRouterRoleEnabled(void) const { return false; }
    void SetRouterRoleEnabled(bool) { }

    bool IsSingleton(void) { return false; }

    ThreadError BecomeRouter(ThreadStatusTlv::Status) { return kThreadError_NotCapable; }
    ThreadError BecomeLeader(void) { return kThreadError_NotCapable; }

    uint8_t GetActiveRouterCount(void) const { return 0; }

    uint32_t GetLeaderAge(void) const { return 0; }

    uint8_t GetLeaderWeight(void) const { return 0; }
    void SetLeaderWeight(uint8_t) { }

    uint32_t GetLeaderPartitionId(void) const { return 0; }
    void SetLeaderPartitionId(uint32_t) { }

    ThreadError SetPreferredRouterId(uint8_t) { return kThreadError_NotImplemented; }
    uint32_t GetPreviousPartitionId(void) const { return 0; }
    void SetPreviousPartitionId(uint32_t) { }
    void SetRouterId(uint8_t) { }

    uint16_t GetNextHop(uint16_t aDestination) const { return Mle::GetNextHop(aDestination); }

    uint8_t GetNetworkIdTimeout(void) const { return 0; }
    void SetNetworkIdTimeout(uint8_t) { }

    uint8_t GetRouteCost(uint16_t) const { return 0; }
    uint8_t GetLinkCost(uint16_t) { return 0; }

    uint8_t GetRouterIdSequence(void) const { return 0; }

    uint8_t GetRouterUpgradeThreshold(void) const { return 0; }
    void SetRouterUpgradeThreshold(uint8_t) { }

    uint8_t GetRouterDowngradeThreshold(void) const { return 0; }
    void SetRouterDowngradeThreshold(uint8_t) { }

    ThreadError ReleaseRouterId(uint8_t) { return kThreadError_NotImplemented; }

    ThreadError RemoveNeighbor(const Mac::Address &) { return BecomeDetached(); }
    ThreadError RemoveNeighbor(Neighbor &) { return BecomeDetached(); }

    Child *GetChild(uint16_t) { return NULL; }
    Child *GetChild(const Mac::ExtAddress &) { return NULL; }
    Child *GetChild(const Mac::Address &) { return NULL; }

    uint8_t GetChildIndex(const Child &) { return 0; }

    Child *GetChildren(uint8_t *aNumChildren) {
        if (aNumChildren != NULL) {
            *aNumChildren = 0;
        }

        return NULL;
    }

    ThreadError SetMaxAllowedChildren(uint8_t) { return kThreadError_NotImplemented; }

    ThreadError RestoreChildren(void) {return kThreadError_NotImplemented; }
    ThreadError RemoveStoredChild(uint16_t) {return kThreadError_NotImplemented; }
    ThreadError StoreChild(uint16_t) {return kThreadError_NotImplemented; }

    Neighbor *GetNeighbor(uint16_t aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetNeighbor(const Mac::ExtAddress &aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetNeighbor(const Mac::Address &aAddress) { return Mle::GetNeighbor(aAddress); }
    Neighbor *GetNeighbor(const Ip6::Address &aAddress) { return Mle::GetNeighbor(aAddress); }

    ThreadError GetChildInfoById(uint16_t, otChildInfo &) { return kThreadError_NotImplemented; }
    ThreadError GetChildInfoByIndex(uint8_t, otChildInfo &) { return kThreadError_NotImplemented; }

    ThreadError GetNextNeighborInfo(otNeighborInfoIterator &, otNeighborInfo &) { return kThreadError_NotImplemented; }

    Router *GetRouters(uint8_t *aNumRouters) {
        if (aNumRouters != NULL) {
            *aNumRouters = 0;
        }

        return NULL;
    }

    ThreadError GetRouterInfo(uint16_t, otRouterInfo &) { return kThreadError_NotImplemented; }

    static int ComparePartitions(bool, const LeaderDataTlv &, bool, const LeaderDataTlv &) { return 0; }

    void ResolveRoutingLoops(uint16_t, uint16_t) { }

    ThreadError CheckReachability(uint16_t aMeshSource, uint16_t aMeshDest, Ip6::Header &aIp6Header) {
        return Mle::CheckReachability(aMeshSource, aMeshDest, aIp6Header);
    }

    static bool IsRouterIdValid(uint8_t aRouterId) { return aRouterId <= kMaxRouterId; }

    void FillConnectivityTlv(ConnectivityTlv &) { }
    void FillRouteTlv(RouteTlv &) { }

private:
    ThreadError HandleDetachStart(void) { return kThreadError_None; }
    ThreadError HandleChildStart(otMleAttachFilter) { return kThreadError_None; }
    ThreadError HandleLinkRequest(const Message &, const Ip6::MessageInfo &) { return kThreadError_Drop; }
    ThreadError HandleLinkAccept(const Message &, const Ip6::MessageInfo &, uint32_t) { return kThreadError_Drop; }
    ThreadError HandleLinkAccept(const Message &, const Ip6::MessageInfo &, uint32_t, bool) { return kThreadError_Drop; }
    ThreadError HandleLinkAcceptAndRequest(const Message &, const Ip6::MessageInfo &, uint32_t) { return kThreadError_Drop; }
    ThreadError HandleAdvertisement(const Message &, const Ip6::MessageInfo &) { return kThreadError_Drop; }
    ThreadError HandleParentRequest(const Message &, const Ip6::MessageInfo &) { return kThreadError_Drop; }
    ThreadError HandleChildIdRequest(const Message &, const Ip6::MessageInfo &, uint32_t) { return kThreadError_Drop; }
    ThreadError HandleChildUpdateRequest(const Message &, const Ip6::MessageInfo &) { return kThreadError_Drop; }
    ThreadError HandleChildUpdateResponse(const Message &, const Ip6::MessageInfo &, uint32_t) { return kThreadError_Drop; }
    ThreadError HandleDataRequest(const Message &, const Ip6::MessageInfo &) { return kThreadError_Drop; }
    ThreadError HandleNetworkDataUpdateRouter(void) { return kThreadError_None; }
    ThreadError HandleDiscoveryRequest(const Message &, const Ip6::MessageInfo &) { return kThreadError_Drop; }

    ThreadError ProcessRouteTlv(const RouteTlv &aRoute) { (void)aRoute; return kThreadError_None; }
};

}  // namespace Mle
}  // namespace Thread

#endif  // MLE_ROUTER_HPP_
