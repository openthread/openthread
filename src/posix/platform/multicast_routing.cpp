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

#include "posix/platform/multicast_routing.hpp"

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include <assert.h>
#include <chrono>
#include <net/if.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#if __linux__
#include <linux/mroute6.h>
#else
#error "Multicast Routing feature is not ported to non-Linux platforms yet."
#endif

#include <openthread/backbone_router_ftd.h>

#include "core/common/logging.hpp"

namespace ot {
namespace Posix {

void MulticastRoutingManager::Init(otInstance *aInstance)
{
    otBackboneRouterSetMulticastListenerCallback(aInstance,
                                                 &MulticastRoutingManager::HandleBackboneMulticastListenerEvent, this);
}

void MulticastRoutingManager::HandleBackboneMulticastListenerEvent(void *                                 aContext,
                                                                   otBackboneRouterMulticastListenerEvent aEvent,
                                                                   const otIp6Address *                   aAddress)
{
    static_cast<MulticastRoutingManager *>(aContext)->HandleBackboneMulticastListenerEvent(
        aEvent, static_cast<const Ip6::Address &>(*aAddress));
}

void MulticastRoutingManager::HandleBackboneMulticastListenerEvent(otBackboneRouterMulticastListenerEvent aEvent,
                                                                   const Ip6::Address &                   aAddress)
{
    switch (aEvent)
    {
    case OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ADDED:
        Add(aAddress);
        break;
    case OT_BACKBONE_ROUTER_MULTICAST_LISTENER_REMOVED:
        Remove(aAddress);
        break;
    }
}

void MulticastRoutingManager::Enable(void)
{
    otError error = InitMulticastRouterSock();

    otLogResultPlat(error, "MulticastRoutingManager: %s", __FUNCTION__);
}

void MulticastRoutingManager::Disable(void)
{
    FinalizeMulticastRouterSock();

    otLogResultPlat(OT_ERROR_NONE, "MulticastRoutingManager: %s", __FUNCTION__);
}

void MulticastRoutingManager::Add(const Ip6::Address &aAddress)
{
    assert(mListenerSet.find(aAddress) == mListenerSet.end());
    mListenerSet.insert(aAddress);

    VerifyOrExit(IsEnabled());

    UnblockInboundMulticastForwardingCache(aAddress);

exit:
    otLogResultPlat(OT_ERROR_NONE, "MulticastRoutingManager: %s: %s", __FUNCTION__, aAddress.ToString().AsCString());
}

void MulticastRoutingManager::Remove(const Ip6::Address &aAddress)
{
    otError error = OT_ERROR_NONE;

    assert(mListenerSet.find(aAddress) != mListenerSet.end());
    mListenerSet.erase(aAddress);

    VerifyOrExit(IsEnabled());

    RemoveInboundMulticastForwardingCache(aAddress);

exit:
    otLogResultPlat(error, "MulticastRoutingManager: %s: %s", __FUNCTION__, aAddress.ToString().AsCString());
}

void MulticastRoutingManager::UpdateFdSet(fd_set &aReadFdSet, int &aMaxFd) const
{
    VerifyOrExit(IsEnabled());

    FD_SET(mMulticastRouterSock, &aReadFdSet);
    aMaxFd = std::max(aMaxFd, mMulticastRouterSock);

exit:
    return;
}

void MulticastRoutingManager::Process(const fd_set &aReadFdSet)
{
    VerifyOrExit(IsEnabled());

    ExpireMulticastForwardingCache();

    if (FD_ISSET(mMulticastRouterSock, &aReadFdSet))
    {
        ProcessMulticastRouterMessages();
    }

exit:
    return;
}

otError MulticastRoutingManager::InitMulticastRouterSock(void)
{
    otError             error = OT_ERROR_NONE;
    int                 one   = 1;
    struct icmp6_filter filter;
    struct mif6ctl      mif6ctl;

    VerifyOrExit(!IsEnabled());

    // Create a Multicast Routing socket
    mMulticastRouterSock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    VerifyOrExit(mMulticastRouterSock != -1, error = OT_ERROR_FAILED);

    // Enable Multicast Forwarding in Kernel
    VerifyOrExit(0 == setsockopt(mMulticastRouterSock, IPPROTO_IPV6, MRT6_INIT, &one, sizeof(one)),
                 error = OT_ERROR_FAILED);

    // Filter all ICMPv6 messages
    ICMP6_FILTER_SETBLOCKALL(&filter);
    VerifyOrExit(0 == setsockopt(mMulticastRouterSock, IPPROTO_ICMPV6, ICMP6_FILTER, (void *)&filter, sizeof(filter)),
                 error = OT_ERROR_FAILED);

    memset(&mif6ctl, 0, sizeof(mif6ctl));
    mif6ctl.mif6c_flags     = 0;
    mif6ctl.vifc_threshold  = 1;
    mif6ctl.vifc_rate_limit = 0;

    // Add Thread network interface to MIF
    mif6ctl.mif6c_mifi = kMifIndexThread;
    mif6ctl.mif6c_pifi = if_nametoindex(gNetifName);
    VerifyOrExit(mif6ctl.mif6c_pifi > 0, error = OT_ERROR_FAILED);
    VerifyOrExit(0 == setsockopt(mMulticastRouterSock, IPPROTO_IPV6, MRT6_ADD_MIF, &mif6ctl, sizeof(mif6ctl)),
                 error = OT_ERROR_FAILED);

    // Add Backbone network interface to MIF
    mif6ctl.mif6c_mifi = kMifIndexBackbone;
    mif6ctl.mif6c_pifi = if_nametoindex(gBackboneNetifName);
    VerifyOrExit(mif6ctl.mif6c_pifi > 0, error = OT_ERROR_FAILED);
    VerifyOrExit(0 == setsockopt(mMulticastRouterSock, IPPROTO_IPV6, MRT6_ADD_MIF, &mif6ctl, sizeof(mif6ctl)),
                 error = OT_ERROR_FAILED);

exit:
    if (error != OT_ERROR_NONE)
    {
        FinalizeMulticastRouterSock();
    }

    return error;
}

void MulticastRoutingManager::FinalizeMulticastRouterSock(void)
{
    VerifyOrExit(IsEnabled());

    close(mMulticastRouterSock);
    mMulticastRouterSock = -1;

exit:
    return;
}

void MulticastRoutingManager::ProcessMulticastRouterMessages(void)
{
    otError         error = OT_ERROR_NONE;
    char            buf[128];
    int             nr;
    struct mrt6msg *mrt6msg;
    Ip6::Address    src, dst;

    nr = read(mMulticastRouterSock, buf, sizeof(buf));

    VerifyOrExit(nr > 0, error = OT_ERROR_FAILED);

    mrt6msg = reinterpret_cast<struct mrt6msg *>(buf);

    VerifyOrExit(mrt6msg->im6_mbz == 0);
    VerifyOrExit(mrt6msg->im6_msgtype == MRT6MSG_NOCACHE);

    src.SetBytes(mrt6msg->im6_src.s6_addr);
    dst.SetBytes(mrt6msg->im6_dst.s6_addr);

    error = AddMulticastForwardingCache(src, dst, static_cast<MifIndex>(mrt6msg->im6_mif));

exit:
    otLogResultPlat(error, "MulticastRoutingManager: %s", __FUNCTION__);
}

otError MulticastRoutingManager::AddMulticastForwardingCache(const Ip6::Address &aSrcAddr,
                                                             const Ip6::Address &aGroupAddr,
                                                             MifIndex            aIif)
{
    otError        error = OT_ERROR_NONE;
    struct mf6cctl mf6cctl;
    MifIndex       forwardMif = kMifIndexNone;

    VerifyOrExit(aIif == kMifIndexThread || aIif == kMifIndexBackbone, error = OT_ERROR_INVALID_ARGS);

    ExpireMulticastForwardingCache();

    if (aIif == kMifIndexBackbone)
    {
        // Forward multicast traffic from Backbone to Thread if the group address is subscribed by any Thread device via
        // MLR.
        if (mListenerSet.find(aGroupAddr) != mListenerSet.end())
        {
            forwardMif = kMifIndexThread;
        }
    }
    else
    {
        // Forward multicast traffic from Thread to Backbone if multicast scope > kRealmLocalScope
        // TODO: (MLR) allow scope configuration of outbound multicast routing
        if (aGroupAddr.GetScope() > Ip6::Address::kRealmLocalScope)
        {
            forwardMif = kMifIndexBackbone;
        }
    }

    memset(&mf6cctl, 0, sizeof(mf6cctl));

    memcpy(mf6cctl.mf6cc_origin.sin6_addr.s6_addr, aSrcAddr.GetBytes(), sizeof(mf6cctl.mf6cc_origin.sin6_addr.s6_addr));
    memcpy(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr, aGroupAddr.GetBytes(),
           sizeof(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr));
    mf6cctl.mf6cc_parent = aIif;

    if (forwardMif != kMifIndexNone)
    {
        IF_SET(forwardMif, &mf6cctl.mf6cc_ifset);
    }

    // Note that kernel reports repetitive `MRT6MSG_NOCACHE` upcalls with a rate limit (e.g. once per 10s for Linux).
    // Because of it, we need to add a "blocking" MFC even if there is no forwarding for this group address.
    // When a  Multicast Listener is later added, the "blocking" MFC will be altered to be a "forwarding" MFC so that
    // corresponding multicast traffic can be forwarded instantly.
    VerifyOrExit(0 == setsockopt(mMulticastRouterSock, IPPROTO_IPV6, MRT6_ADD_MFC, &mf6cctl, sizeof(mf6cctl)),
                 error = OT_ERROR_FAILED);

    mMulticastForwardingCache[MulticastRoute(aSrcAddr, aGroupAddr)] = MulticastRouteInfo(aIif, forwardMif);

exit:
    otLogResultPlat(error, "MulticastRoutingManager: %s: add dynamic route: %s %s => %s %s", __FUNCTION__,
                    MifIndexToString(aIif), aSrcAddr.ToString().AsCString(), aGroupAddr.ToString().AsCString(),
                    MifIndexToString(forwardMif));

    return error;
}

void MulticastRoutingManager::UnblockInboundMulticastForwardingCache(const Ip6::Address &aGroupAddr)
{
    struct mf6cctl mf6cctl;

    memset(&mf6cctl, 0, sizeof(mf6cctl));
    memcpy(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr, aGroupAddr.GetBytes(),
           sizeof(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr));
    mf6cctl.mf6cc_parent = kMifIndexBackbone;
    IF_SET(kMifIndexThread, &mf6cctl.mf6cc_ifset);

    for (const std::pair<const MulticastRoute, MulticastRouteInfo> &entry : mMulticastForwardingCache)
    {
        otError                   error;
        const MulticastRoute &    route     = entry.first;
        const MulticastRouteInfo &routeInfo = entry.second;

        if (routeInfo.mIif != kMifIndexBackbone || routeInfo.mOif == kMifIndexThread || route.mGroupAddr != aGroupAddr)
        {
            continue;
        }

        // Unblock this inbound route
        memcpy(mf6cctl.mf6cc_origin.sin6_addr.s6_addr, route.mSrcAddr.GetBytes(),
               sizeof(mf6cctl.mf6cc_origin.sin6_addr.s6_addr));

        error = (0 == setsockopt(mMulticastRouterSock, IPPROTO_IPV6, MRT6_ADD_MFC, &mf6cctl, sizeof(mf6cctl)))
                    ? OT_ERROR_NONE
                    : OT_ERROR_FAILED;

        if (error == OT_ERROR_NONE)
        {
            mMulticastForwardingCache[route] = MulticastRouteInfo(routeInfo.mIif, kMifIndexThread);
        }

        otLogResultPlat(error, "MulticastRoutingManager: %s: %s => %s, MIF=%s, ForwardMif=%s", __FUNCTION__,
                        route.mSrcAddr.ToString().AsCString(), route.mGroupAddr.ToString().AsCString(),
                        MifIndexToString(routeInfo.mIif), MifIndexToString(kMifIndexThread));
    }
}

void MulticastRoutingManager::RemoveInboundMulticastForwardingCache(const Ip6::Address &aGroupAddr)
{
    struct mf6cctl mf6cctl;

    memset(&mf6cctl, 0, sizeof(mf6cctl));
    mf6cctl.mf6cc_parent = kMifIndexBackbone;
    memcpy(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr, aGroupAddr.GetBytes(),
           sizeof(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr));

    for (std::map<MulticastRoute, MulticastRouteInfo>::const_iterator it = mMulticastForwardingCache.begin();
         it != mMulticastForwardingCache.end();)
    {
        otError                   error;
        const MulticastRoute &    route     = it->first;
        const MulticastRouteInfo &routeInfo = it->second;
        bool                      erase     = false;

        if (routeInfo.mIif == kMifIndexBackbone && route.mGroupAddr == aGroupAddr)
        {
            memcpy(mf6cctl.mf6cc_origin.sin6_addr.s6_addr, route.mSrcAddr.GetBytes(),
                   sizeof(mf6cctl.mf6cc_origin.sin6_addr.s6_addr));

            error = (0 == setsockopt(mMulticastRouterSock, IPPROTO_IPV6, MRT6_DEL_MFC, &mf6cctl, sizeof(mf6cctl)))
                        ? OT_ERROR_NONE
                        : OT_ERROR_FAILED;

            if (error == OT_ERROR_NONE || errno == ENOENT)
            {
                erase = true;
            }

            otLogResultPlat(error, "MulticastRoutingManager: %s: %s => %s, MIF=%s, ForwardMIF=%s", __FUNCTION__,
                            route.mSrcAddr.ToString().AsCString(), route.mGroupAddr.ToString().AsCString(),
                            MifIndexToString(routeInfo.mIif), MifIndexToString(kMifIndexNone));
        }

        if (erase)
        {
            mMulticastForwardingCache.erase(it++);
        }
        else
        {
            it++;
        }
    }
}

void MulticastRoutingManager::ExpireMulticastForwardingCache(void)
{
    struct sioc_sg_req6            sioc_sg_req6;
    std::chrono::time_point<Clock> now = Clock::now();
    struct mf6cctl                 mf6cctl;

    VerifyOrExit(now >= mLastExpireTime + std::chrono::seconds(kMulticastForwardingCacheExpiringInterval));

    mLastExpireTime = now;

    memset(&mf6cctl, 0, sizeof(mf6cctl));
    memset(&sioc_sg_req6, 0, sizeof(sioc_sg_req6));

    for (std::map<MulticastRoute, MulticastRouteInfo>::const_iterator it = mMulticastForwardingCache.begin();
         it != mMulticastForwardingCache.end();)
    {
        otError                   error;
        const MulticastRoute &    route     = it->first;
        const MulticastRouteInfo &routeInfo = it->second;
        bool                      erase     = false;

        if ((routeInfo.mLastUseTime + std::chrono::seconds(kMulticastForwardingCacheExpireTimeout) < now))
        {
            if (!UpdateMulticastRouteInfo(route))
            {
                // The multicast route is expired

                memcpy(mf6cctl.mf6cc_origin.sin6_addr.s6_addr, route.mSrcAddr.GetBytes(),
                       sizeof(mf6cctl.mf6cc_origin.sin6_addr.s6_addr));
                memcpy(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr, route.mGroupAddr.GetBytes(),
                       sizeof(mf6cctl.mf6cc_mcastgrp.sin6_addr.s6_addr));

                mf6cctl.mf6cc_parent = routeInfo.mIif;

                error = (0 == setsockopt(mMulticastRouterSock, IPPROTO_IPV6, MRT6_DEL_MFC, &mf6cctl, sizeof(mf6cctl)))
                            ? OT_ERROR_NONE
                            : OT_ERROR_FAILED;

                if (error == OT_ERROR_NONE || errno == ENOENT)
                {
                    erase = true;
                }

                otLogResultPlat(error, "MulticastRoutingManager: %s: %s => %s, MIF=%s, ForwardMIF=%s", __FUNCTION__,
                                route.mSrcAddr.ToString().AsCString(), route.mGroupAddr.ToString().AsCString(),
                                MifIndexToString(routeInfo.mIif), MifIndexToString(routeInfo.mOif));
            }
        }

        if (erase)
        {
            mMulticastForwardingCache.erase(it++);
        }
        else
        {
            it++;
        }
    }

    DumpMulticastForwardingCache();

exit:
    return;
}

bool MulticastRoutingManager::UpdateMulticastRouteInfo(const MulticastRoute &route)
{
    bool                updated = false;
    struct sioc_sg_req6 sioc_sg_req6;
    MulticastRouteInfo &routeInfo = mMulticastForwardingCache[route];

    memset(&sioc_sg_req6, 0, sizeof(sioc_sg_req6));

    memcpy(sioc_sg_req6.src.sin6_addr.s6_addr, route.mSrcAddr.GetBytes(), sizeof(sioc_sg_req6.src.sin6_addr.s6_addr));
    memcpy(sioc_sg_req6.grp.sin6_addr.s6_addr, route.mGroupAddr.GetBytes(), sizeof(sioc_sg_req6.grp.sin6_addr.s6_addr));

    if (ioctl(mMulticastRouterSock, SIOCGETSGCNT_IN6, &sioc_sg_req6) != -1)
    {
        unsigned long validPktCnt;

        otLogDebgPlat("MulticastRoutingManager: %s: SIOCGETSGCNT_IN6 %s => %s: bytecnt=%lu, pktcnt=%lu, wrong_if=%lu",
                      __FUNCTION__, route.mSrcAddr.ToString().AsCString(), route.mGroupAddr.ToString().AsCString(),
                      sioc_sg_req6.bytecnt, sioc_sg_req6.pktcnt, sioc_sg_req6.wrong_if);

        validPktCnt = sioc_sg_req6.pktcnt - sioc_sg_req6.wrong_if;
        if (validPktCnt != routeInfo.mValidPktCnt)
        {
            routeInfo.mValidPktCnt = validPktCnt;
            routeInfo.mLastUseTime = Clock::now();

            updated = true;
        }
    }
    else
    {
        otLogWarnPlat("MulticastRoutingManager: %s: SIOCGETSGCNT_IN6 %s => %s failed: %s", __FUNCTION__,
                      route.mSrcAddr.ToString().AsCString(), route.mGroupAddr.ToString().AsCString(), strerror(errno));
    }

    return updated;
}

const char *MulticastRoutingManager::MifIndexToString(MifIndex aMif)
{
    const char *string = "Unknown";

    switch (aMif)
    {
    case kMifIndexNone:
        string = "None";
        break;
    case kMifIndexThread:
        string = "Thread";
        break;
    case kMifIndexBackbone:
        string = "Backbone";
        break;
    }

    return string;
}

void MulticastRoutingManager::DumpMulticastForwardingCache(void)
{
#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
    otLogDebgPlat("MulticastRoutingManager: ==================== MFC %d entries ====================",
                  mMulticastForwardingCache.size());

    for (const std::pair<const MulticastRoute, MulticastRouteInfo> &entry : mMulticastForwardingCache)
    {
        const MulticastRoute &    route     = entry.first;
        const MulticastRouteInfo &routeInfo = entry.second;

        otLogDebgPlat("MulticastRoutingManager: %s %s => %s %s", MifIndexToString(routeInfo.mIif),
                      route.mSrcAddr.ToString().AsCString(), route.mGroupAddr.ToString().AsCString(),
                      MifIndexToString(routeInfo.mOif));
    }

    otLogDebgPlat("MulticastRoutingManager: ========================================================");
#endif
}

void MulticastRoutingManager::HandleStateChange(otInstance *aInstance, otChangedFlags aFlags)
{
    if (aFlags & OT_CHANGED_THREAD_BACKBONE_ROUTER_STATE)
    {
        otBackboneRouterState state = otBackboneRouterGetState(aInstance);

        switch (state)
        {
        case OT_BACKBONE_ROUTER_STATE_DISABLED:
        case OT_BACKBONE_ROUTER_STATE_SECONDARY:
            Disable();
            break;
        case OT_BACKBONE_ROUTER_STATE_PRIMARY:
            Enable();
            break;
        }
    }
}

bool MulticastRoutingManager::MulticastRoute::operator<(const MulticastRoutingManager::MulticastRoute &aOther) const
{
    bool less = false;

    VerifyOrExit(mGroupAddr == aOther.mGroupAddr, less = mGroupAddr < aOther.mGroupAddr);
    VerifyOrExit(mSrcAddr == aOther.mSrcAddr, less = mSrcAddr < aOther.mSrcAddr);

exit:
    return less;
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
