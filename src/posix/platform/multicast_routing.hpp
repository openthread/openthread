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

#ifndef OT_POSIX_PLATFORM_MULTICAST_ROUTING_HPP_
#define OT_POSIX_PLATFORM_MULTICAST_ROUTING_HPP_

#include "openthread-posix-config.h"

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#include <stdio.h>
#include <stdlib.h>

#include <openthread/backbone_router_ftd.h>
#include <openthread/openthread-system.h>

#include "platform-posix.h"
#include "core/common/non_copyable.hpp"
#include "core/net/ip6_address.hpp"
#include "lib/url/url.hpp"

namespace ot {
namespace Posix {

/**
 * This class implements Multicast Routing management.
 *
 */
class MulticastRoutingManager : private NonCopyable
{
public:
    /**
     * This constructor initializes a Multicast Routing manager instance.
     *
     */
    explicit MulticastRoutingManager()
        : mLastExpireTime(0)
        , mMulticastRouterSock(-1)
        , mInstance(nullptr)
    {
    }

    /**
     * This method initializes the Multicast Routing manager.
     *
     * @param[in]  aInstance  A pointer to an OpenThread instance.
     *
     */
    void Init(otInstance *aInstance);

    /**
     * This method updates the fd_set and timeout for mainloop.
     *
     * @param[inout]    aReadFdSet      A reference to fd_set for polling read.
     * @param[inout]    aMaxFd          A reference to the current max fd in fd_sets.
     *
     */
    void UpdateFdSet(fd_set &aReadFdSet, int &aMaxFd) const;

    /**
     * This method performs Multicast Routing processing.
     *
     * @param[in]   aReadFdSet   A reference to read file descriptors.
     *
     */
    void Process(const fd_set &aReadFdSet);

    /**
     * This method handles Thread state changes.
     *
     * @param[in] aInstance  A pointer to an OpenThread instance.
     * @param[in] aFlags     Flags that denote the state change events.
     *
     */
    void HandleStateChange(otInstance *aInstance, otChangedFlags aFlags);

private:
    enum
    {
        kMulticastForwardingCacheExpireTimeout    = 300, //< Expire timeout of Multicast Forwarding Cache (in seconds)
        kMulticastForwardingCacheExpiringInterval = 60,  //< Expire interval of Multicast Forwarding Cache (in seconds)
        kMulitcastForwardingCacheTableSize =
            OPENTHREAD_POSIX_CONFIG_MAX_MULTICAST_FORWARDING_CACHE_TABLE, //< The max size of MFC table.
    };

    enum MifIndex : uint8_t
    {
        kMifIndexNone     = 0xff,
        kMifIndexThread   = 0,
        kMifIndexBackbone = 1,
    };

    class MulticastForwardingCache
    {
        friend class MulticastRoutingManager;

    private:
        MulticastForwardingCache()
            : mIif(kMifIndexNone)
        {
        }

        bool IsValid() const { return mIif != kMifIndexNone; }
        void Set(MifIndex aIif, MifIndex aOif);
        void Set(const Ip6::Address &aSrcAddr, const Ip6::Address &aGroupAddr, MifIndex aIif, MifIndex aOif);
        void Erase(void) { mIif = kMifIndexNone; }
        void SetValidPktCnt(unsigned long aValidPktCnt);

        Ip6::Address  mSrcAddr;
        Ip6::Address  mGroupAddr;
        uint64_t      mLastUseTime;
        unsigned long mValidPktCnt;
        MifIndex      mIif;
        MifIndex      mOif;
    };

    void    Enable(void);
    void    Disable(void);
    void    Add(const Ip6::Address &aAddress);
    void    Remove(const Ip6::Address &aAddress);
    bool    HasMulticastListener(const Ip6::Address &aAddress) const;
    bool    IsEnabled(void) const { return mMulticastRouterSock >= 0; }
    void    InitMulticastRouterSock(void);
    void    FinalizeMulticastRouterSock(void);
    void    ProcessMulticastRouterMessages(void);
    otError AddMulticastForwardingCache(const Ip6::Address &aSrcAddr, const Ip6::Address &aGroupAddr, MifIndex aIif);
    void    SaveMulticastForwardingCache(const Ip6::Address &aSrcAddr,
                                         const Ip6::Address &aGroupAddr,
                                         MifIndex            aIif,
                                         MifIndex            aOif);
    void    UnblockInboundMulticastForwardingCache(const Ip6::Address &aGroupAddr);
    void    RemoveInboundMulticastForwardingCache(const Ip6::Address &aGroupAddr);
    void    ExpireMulticastForwardingCache(void);
    bool    UpdateMulticastRouteInfo(MulticastForwardingCache &aMfc) const;
    void    RemoveMulticastForwardingCache(MulticastForwardingCache &aMfc) const;
    static const char *MifIndexToString(MifIndex aMif);
    void               DumpMulticastForwardingCache(void) const;
    static void        HandleBackboneMulticastListenerEvent(void *                                 aContext,
                                                            otBackboneRouterMulticastListenerEvent aEvent,
                                                            const otIp6Address *                   aAddress);
    void               HandleBackboneMulticastListenerEvent(otBackboneRouterMulticastListenerEvent aEvent,
                                                            const Ip6::Address &                   aAddress);

    MulticastForwardingCache mMulticastForwardingCacheTable[kMulitcastForwardingCacheTableSize];
    uint64_t                 mLastExpireTime;
    int                      mMulticastRouterSock;
    otInstance *             mInstance;
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#endif // OT_POSIX_PLATFORM_MULTICAST_ROUTING_HPP_
