/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#ifndef POSIX_PLATFORM_NETIF_HPP_
#define POSIX_PLATFORM_NETIF_HPP_

#include "openthread-posix-config.h"

#include "core/common/non_copyable.hpp"
#include "posix/platform/mainloop.hpp"

#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/nat64.h>
#include <openthread/openthread-system.h>

#include <net/if.h>
#if defined(__linux__)
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE

namespace ot {
namespace Posix {

class NetIf : public Mainloop::Source, private NonCopyable
{
public:
    /** Returns the singleton object of this class. */
    static NetIf &Get(void);

    void Init(const otPlatformConfig *aPlatformConfig);
    void Deinit(void);

    unsigned int GetIfIndex(void) const { return mIfIndex; }
    const char  *GetIfName(void) const { return mIfName; }

    void SetUp(otInstance *aInstance);
    void TearDown(void);

    void HandleStateChanged(otChangedFlags aFlags);

    // Implements ot::Posix::Mainloop::Source

    void Update(otSysMainloopContext &aContext) override;
    void Process(const otSysMainloopContext &aContext) override;

private:
    static constexpr size_t   kMaxIp6Size            = OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH;
    static constexpr uint32_t kOmrRoutesPriority     = OPENTHREAD_POSIX_CONFIG_OMR_ROUTES_PRIORITY;
    static constexpr uint8_t  kMaxOmrRoutesNum       = OPENTHREAD_POSIX_CONFIG_MAX_OMR_ROUTES_NUM;
    static constexpr uint32_t kExternalRoutePriority = OPENTHREAD_POSIX_CONFIG_EXTERNAL_ROUTE_PRIORITY;
    static constexpr uint8_t  kMaxExternalRoutesNum  = OPENTHREAD_POSIX_CONFIG_MAX_EXTERNAL_ROUTE_NUM;
    static constexpr uint32_t kNat64RoutePriority =
        100; ///< Priority for route to NAT64 CIDR, 100 means a high priority.

    int  CreateTunDevice(const otPlatformConfig *aPlatformConfig, char *aInterfaceName);
    void DestroyTunDevice(void);

    void UpdateUnicast(const otIp6AddressInfo &aAddressInfo, bool aIsAdded);
    void UpdateMulticast(const otIp6Address &aAddress, bool aIsAdded);

    void        ProcessReceive(otMessage *aMessage);
    static void ProcessReceive(otMessage *aMessage, void *aNetIf)
    {
        static_cast<NetIf *>(aNetIf)->ProcessReceive(aMessage);
    }
    void        ProcessTransmit(void);
    void        ProcessNetlinkEvent(void);
    void        ProcessAddressChange(const otIp6AddressInfo *aAddressInfo, bool aIsAdded);
    static void ProcessAddressChange(const otIp6AddressInfo *aAddressInfo, bool aIsAdded, void *aNetIf)
    {
        static_cast<NetIf *>(aNetIf)->ProcessAddressChange(aAddressInfo, aIsAdded);
    }
    void SetLinkState(bool aIsUp);

#if defined(__linux__)
    void    SetAddrGenModeToNone(void);
    otError AddRoute(bool aIsIpv6, const uint8_t *aPrefix, uint8_t aPrefixLen, uint32_t aPriority);
    otError AddRoute(const otIp6Prefix &aPrefix, uint32_t aPriority)
    {
        return AddRoute(/* aIsIpv6 */ true, aPrefix.mPrefix.mFields.m8, aPrefix.mLength, aPriority);
    }
    otError AddIp4Route(const otIp4Cidr &aIp4Cidr, uint32_t aPriority)
    {
        return AddRoute(/* aIsIpv6 */ false, aIp4Cidr.mAddress.mFields.m8, aIp4Cidr.mLength, aPriority);
    }

    otError DeleteRoute(bool aIsIpv6, const uint8_t *aPrefix, uint8_t aPrefixLen);
    otError DeleteRoute(const otIp6Prefix &aPrefix)
    {
        return DeleteRoute(/* aIsIpv6 */ true, aPrefix.mPrefix.mFields.m8, aPrefix.mLength);
    }
    otError DeleteIp4Route(const otIp4Cidr &aIp4Cidr)
    {
        return DeleteRoute(/* aIsIpv6 */ false, aIp4Cidr.mAddress.mFields.m8, aIp4Cidr.mLength);
    }

    bool    HasAddedOmrRoute(const otIp6Prefix &aOmrPrefix);
    otError AddOmrRoute(const otIp6Prefix &aPrefix);
    void    UpdateOmrRoutes(void);

    otError AddExternalRoute(const otIp6Prefix &aPrefix);
    bool    HasExternalRouteInNetData(const otIp6Prefix &aExternalRoute);
    bool    HasAddedExternalRoute(const otIp6Prefix &aExternalRoute);
    void    UpdateExternalRoutes(void);

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    void        HandleNat64StateChanged(otNat64State aNewState);
    static void HandleNat64StateChanged(otNat64State aNewState, void *aNetIf)
    {
        static_cast<NetIf *>(aNetIf)->HandleNat64StateChanged(aNewState);
    }
#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
#endif // defined(__linux__)

    unsigned int mIfIndex = 0;
    char         mIfName[IFNAMSIZ];
    int          mTunFd     = -1;
    int          mIpFd      = -1;
    int          mNetlinkFd = -1;
#if defined(RTM_NEWLINK) && defined(RTM_DELLINK)
    bool mIsSyncingState = false;
#endif
#if defined(__linux__)
    uint32_t mNetlinkSequence = 0;
#if OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE
    uint8_t     mAddedOmrRoutesNum = 0;
    otIp6Prefix mAddedOmrRoutes[kMaxOmrRoutesNum];
#endif
#if OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE
    uint8_t     mAddedExternalRoutesNum = 0;
    otIp6Prefix mAddedExternalRoutes[kMaxExternalRoutesNum];
#endif
#endif
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    otIp4Cidr mActiveNat64Cidr;
#endif
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE

#endif // POSIX_PLATFORM_NETIF_HPP_
