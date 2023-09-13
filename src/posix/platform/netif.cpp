/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements the platform network on Linux.
 */

#include "posix/platform/netif.hpp"

#include "platform-posix.h"

#if defined(__APPLE__)

//	NOTE: on mac OS, the utun driver is present on the system and "works" --
//	but in a limited way.  In particular, the mac OS "utun" driver is marked IFF_POINTTOPOINT,
//	and you cannot clear that flag with SIOCSIFFLAGS (it's part of the IFF_CANTCHANGE definition
//	in xnu's net/if.h [but removed from the mac OS SDK net/if.h]).  And unfortunately, mac OS's
//	build of mDNSResponder won't allow for mDNS over an interface marked IFF_POINTTOPOINT
//	(see comments near definition of MulticastInterface in mDNSMacOSX.c for the bogus reasoning).
//
//	There is an alternative.  An open-source tuntap kernel extension is available here:
//
//		<http://tuntaposx.sourceforge.net>
//		<https://sourceforge.net/p/tuntaposx/code/ci/master/tree/>
//
//	and can be installed via homebrew here:
//
//		<https://formulae.brew.sh/cask/tuntap>
//
//	Building from source and installing isn't trivial, and it's
//	not getting easier (https://forums.developer.apple.com/thread/79590).
//
//	If you want mDNS support, then you can't use Apple utun.  I use the non-Apple driver
//	pretty much exclusively, because mDNS is a requirement.

#if !(defined(OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION) &&                         \
      ((OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_UTUN) || \
       (OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_TUN)))
#error "Unexpected tunnel driver selection"
#endif

#endif // defined(__APPLE__)

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#ifdef __linux__
#include <linux/if_link.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif // __linux__
#include <math.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
#include <netinet/in.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <net/if_var.h>
#endif // defined(__APPLE__) || defined(__FreeBSD__)
#include <net/route.h>
#include <netinet6/in6_var.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
// the prf_ra structure is defined inside another structure (in6_prflags), and C++
//   treats that as out of scope if another structure tries to use it -- this (slightly gross)
//   workaround makes us dependent on our definition remaining in sync (at least the size of it),
//   so we add a compile-time check that will fail if the SDK ever changes
//
// our definition of the struct:
struct prf_ra
{
    u_char onlink : 1;
    u_char autonomous : 1;
    u_char reserved : 6;
} prf_ra;
// object that contains the SDK's version of the structure:
struct in6_prflags compile_time_check_prflags;
// compile time check to make sure they're the same size:
extern int
    compile_time_check_struct_prf_ra[(sizeof(struct prf_ra) == sizeof(compile_time_check_prflags.prf_ra)) ? 1 : -1];
#endif
#include <net/if_dl.h>    // struct sockaddr_dl
#include <netinet6/nd6.h> // ND6_INFINITE_LIFETIME

#ifdef __APPLE__
#if OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_UTUN
#include <net/if_utun.h>
#endif

#if OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_TUN
#include <sys/ioccom.h>
// FIX ME: include the tun_ioctl.h file (challenging, as it's location depends on where the developer puts it)
#define TUNSIFHEAD _IOW('t', 96, int)
#define TUNGIFHEAD _IOR('t', 97, int)
#endif

#include <sys/kern_control.h>
#endif // defined(__APPLE__)

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <net/if_tun.h>
#endif // defined(__NetBSD__) || defined(__FreeBSD__)

#endif // defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)

#include <openthread/border_router.h>
#include <openthread/icmp6.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/logging.h>
#include <openthread/message.h>
#include <openthread/nat64.h>
#include <openthread/netdata.h>
#include <openthread/platform/border_routing.h>
#include <openthread/platform/misc.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "net/ip6_address.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
#if OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE
#include "firewall.hpp"
#endif
#include "posix/platform/ip6_utils.hpp"
#include "posix/platform/mld_monitor.hpp"
#include "posix/platform/utils.hpp"

using namespace ot::Posix::Ip6Utils;

#ifndef OPENTHREAD_POSIX_TUN_DEVICE

#ifdef __linux__
#define OPENTHREAD_POSIX_TUN_DEVICE "/dev/net/tun"
#elif defined(__NetBSD__) || defined(__FreeBSD__)
#define OPENTHREAD_POSIX_TUN_DEVICE "/dev/tun0"
#elif defined(__APPLE__)
#if OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_UTUN
#define OPENTHREAD_POSIX_TUN_DEVICE // not used - calculated dynamically
#elif OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_TUN
#define OPENTHREAD_POSIX_TUN_DEVICE "/dev/tun0"
#endif
#else
// good luck -- untested platform...
#define OPENTHREAD_POSIX_TUN_DEVICE "/dev/net/tun"
#endif

#endif // OPENTHREAD_TUN_DEVICE

// some platforms (like NetBSD) do not have RTM_NEWMADDR/RTM_DELMADDR messages, and they ALSO lack
// working MLDv2 support.  for those platforms, we must tell the OpenThread interface to
// pass ALL multicast packets up; the kernel's IPv6 will filter and drop those that have no listeners
#define OPENTHREAD_POSIX_MULTICAST_PROMISCUOUS_REQUIRED 0
#if !OPENTHREAD_POSIX_USE_MLD_MONITOR
#if defined(__NetBSD__)
#undef OPENTHREAD_POSIX_MULTICAST_PROMISCUOUS_REQUIRED
#define OPENTHREAD_POSIX_MULTICAST_PROMISCUOUS_REQUIRED 1
#endif
#endif

namespace ot {
namespace Posix {

#define OPENTHREAD_POSIX_LOG_TUN_PACKETS 0

#if !defined(__linux__)
static bool UnicastAddressIsSubscribed(otInstance *aInstance, const otNetifAddress *netAddr)
{
    const otNetifAddress *address = otIp6GetUnicastAddresses(aInstance);

    while (address != nullptr)
    {
        if (memcmp(address->mAddress.mFields.m8, netAddr->mAddress.mFields.m8, sizeof(address->mAddress.mFields.m8)) ==
            0)
        {
            return true;
        }

        address = address->mNext;
    }

    return false;
}
#endif

#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
static const uint8_t allOnes[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void InitNetaskWithPrefixLength(struct in6_addr *address, uint8_t prefixLen)
{
#define MAX_PREFIX_LENGTH (OT_IP6_ADDRESS_SIZE * CHAR_BIT)
    if (prefixLen > MAX_PREFIX_LENGTH)
    {
        prefixLen = MAX_PREFIX_LENGTH;
    }

    ot::Ip6::Address addr;

    addr.Clear();
    addr.SetPrefix(allOnes, prefixLen);
    memcpy(address, addr.mFields.m8, sizeof(addr.mFields.m8));
}

static uint8_t NetmaskToPrefixLength(const struct sockaddr_in6 *netmask)
{
    return otIp6PrefixMatch(reinterpret_cast<const otIp6Address *>(netmask->sin6_addr.s6_addr),
                            reinterpret_cast<const otIp6Address *>(allOnes));
}
#endif

#if defined(__linux__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"

static struct rtattr *AddRtAttr(struct nlmsghdr *aHeader,
                                uint32_t         aMaxLen,
                                uint8_t          aType,
                                const void      *aData,
                                uint8_t          aLen)
{
    uint8_t        len = RTA_LENGTH(aLen);
    struct rtattr *rta;

    assert(NLMSG_ALIGN(aHeader->nlmsg_len) + RTA_ALIGN(len) <= aMaxLen);
    OT_UNUSED_VARIABLE(aMaxLen);

    rta           = (struct rtattr *)((char *)(aHeader) + NLMSG_ALIGN((aHeader)->nlmsg_len));
    rta->rta_type = aType;
    rta->rta_len  = len;
    if (aLen)
    {
        memcpy(RTA_DATA(rta), aData, aLen);
    }
    aHeader->nlmsg_len = NLMSG_ALIGN(aHeader->nlmsg_len) + RTA_ALIGN(len);

    return rta;
}

void AddRtAttrUint32(struct nlmsghdr *aHeader, uint32_t aMaxLen, uint8_t aType, uint32_t aData)
{
    AddRtAttr(aHeader, aMaxLen, aType, &aData, sizeof(aData));
}

#if OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE
static bool IsOmrAddress(otInstance *aInstance, const otIp6AddressInfo &aAddressInfo)
{
    otIp6Prefix addressPrefix{*aAddressInfo.mAddress, aAddressInfo.mPrefixLength};

    return otNetDataContainsOmrPrefix(aInstance, &addressPrefix);
}
#endif

#pragma GCC diagnostic pop
#endif // defined(__linux__)

#if defined(__linux__)
void NetIf::UpdateUnicast(const otIp6AddressInfo &aAddressInfo, bool aIsAdded)
{
    struct
    {
        struct nlmsghdr  nh;
        struct ifaddrmsg ifa;
        char             buf[512];
    } req;

    VerifyOrExit(mIfIndex > 0);

    memset(&req, 0, sizeof(req));

    req.nh.nlmsg_len   = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL;
    req.nh.nlmsg_type  = aIsAdded ? RTM_NEWADDR : RTM_DELADDR;
    req.nh.nlmsg_pid   = 0;
    req.nh.nlmsg_seq   = ++mNetlinkSequence;

    req.ifa.ifa_family    = AF_INET6;
    req.ifa.ifa_prefixlen = aAddressInfo.mPrefixLength;
    req.ifa.ifa_flags     = IFA_F_NODAD;
    req.ifa.ifa_scope     = aAddressInfo.mScope;
    req.ifa.ifa_index     = mIfIndex;

    AddRtAttr(&req.nh, sizeof(req), IFA_LOCAL, aAddressInfo.mAddress, sizeof(*aAddressInfo.mAddress));

    if (!aAddressInfo.mPreferred)
    {
        struct ifa_cacheinfo cacheinfo;

        memset(&cacheinfo, 0, sizeof(cacheinfo));
        cacheinfo.ifa_valid = UINT32_MAX;

        AddRtAttr(&req.nh, sizeof(req), IFA_CACHEINFO, &cacheinfo, sizeof(cacheinfo));
    }

#if OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE
    if (IsOmrAddress(mInstance, aAddressInfo))
    {
        // Remove prefix route for OMR address if `OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE` is enabled to
        // avoid having two routes.
        if (aIsAdded)
        {
            AddRtAttrUint32(&req.nh, sizeof(req), IFA_FLAGS, IFA_F_NOPREFIXROUTE);
        }
    }
    else
#endif
    {
#if OPENTHREAD_POSIX_CONFIG_NETIF_PREFIX_ROUTE_METRIC > 0
        if (aAddressInfo.mScope > ot::Ip6::Address::kLinkLocalScope)
        {
            AddRtAttrUint32(&req.nh, sizeof(req), IFA_RT_PRIORITY, OPENTHREAD_POSIX_CONFIG_NETIF_PREFIX_ROUTE_METRIC);
        }
#endif
    }

    if (send(mNetlinkFd, &req, req.nh.nlmsg_len, 0) != -1)
    {
        otLogInfoPlat("[netif] Sent request#%u to %s %s/%u", mNetlinkSequence, (aIsAdded ? "add" : "remove"),
                      Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength);
    }
    else
    {
        otLogWarnPlat("[netif] Failed to send request#%u to %s %s/%u", mNetlinkSequence, (aIsAdded ? "add" : "remove"),
                      Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength);
    }

exit:
    return;
}

#elif defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)

void NetIf::UpdateUnicast(const otIp6AddressInfo &aAddressInfo, bool aIsAdded)
{
    int                 rval;
    struct in6_aliasreq ifr6;

    assert(mIpFd >= 0);

    memset(&ifr6, 0, sizeof(ifr6));
    strlcpy(ifr6.ifra_name, mIfName, sizeof(ifr6.ifra_name));
    ifr6.ifra_addr.sin6_family = AF_INET6;
    ifr6.ifra_addr.sin6_len    = sizeof(ifr6.ifra_addr);
    memcpy(&ifr6.ifra_addr.sin6_addr, aAddressInfo.mAddress, sizeof(struct in6_addr));
    ifr6.ifra_prefixmask.sin6_family = AF_INET6;
    ifr6.ifra_prefixmask.sin6_len    = sizeof(ifr6.ifra_prefixmask);
    InitNetaskWithPrefixLength(&ifr6.ifra_prefixmask.sin6_addr, aAddressInfo.mPrefixLength);
    ifr6.ifra_lifetime.ia6t_vltime    = ND6_INFINITE_LIFETIME;
    ifr6.ifra_lifetime.ia6t_pltime    = ND6_INFINITE_LIFETIME;

#if defined(__APPLE__)
    ifr6.ifra_lifetime.ia6t_expire    = ND6_INFINITE_LIFETIME;
    ifr6.ifra_lifetime.ia6t_preferred = (aAddressInfo.mPreferred ? ND6_INFINITE_LIFETIME : 0);
#endif

    rval = ioctl(mIpFd, aIsAdded ? SIOCAIFADDR_IN6 : SIOCDIFADDR_IN6, &ifr6);
    if (rval == 0)
    {
        otLogInfoPlat("[netif] %s %s/%u", (aIsAdded ? "Added" : "Removed"),
                      Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength);
    }
    else if (errno != EALREADY)
    {
        otLogWarnPlat("[netif] Failed to %s %s/%u: %s", (aIsAdded ? "add" : "remove"),
                      Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength, strerror(errno));
    }
}

#endif // defined(__linux__)

void NetIf::UpdateMulticast(const otIp6Address &aAddress, bool aIsAdded)
{
    struct ipv6_mreq mreq;
    otError          error = OT_ERROR_NONE;
    int              err;

    VerifyOrExit(mIpFd >= 0);
    memcpy(&mreq.ipv6mr_multiaddr, &aAddress, sizeof(mreq.ipv6mr_multiaddr));
    mreq.ipv6mr_interface = mIfIndex;

    err = setsockopt(mIpFd, IPPROTO_IPV6, (aIsAdded ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP), &mreq, sizeof(mreq));

#if defined(__APPLE__) || defined(__FreeBSD__)
    if ((err != 0) && (errno == EINVAL) && (IN6_IS_ADDR_MC_LINKLOCAL(&mreq.ipv6mr_multiaddr)))
    {
        // FIX ME
        // on mac OS (and FreeBSD), the first time we run (but not subsequently), we get a failure on this
        // particular join. do we need to bring up the interface at least once prior to joining? we need to figure
        // out why so we can get rid of this workaround
        char addressString[INET6_ADDRSTRLEN + 1];

        inet_ntop(AF_INET6, mreq.ipv6mr_multiaddr.s6_addr, addressString, sizeof(addressString));
        otLogWarnPlat("[netif] Ignoring %s failure (EINVAL) for MC LINKLOCAL address (%s)",
                      aIsAdded ? "IPV6_JOIN_GROUP" : "IPV6_LEAVE_GROUP", addressString);
        err = 0;
    }
#endif

    if (err != 0)
    {
        otLogWarnPlat("[netif] %s failure (%d)", aIsAdded ? "IPV6_JOIN_GROUP" : "IPV6_LEAVE_GROUP", errno);
        error = OT_ERROR_FAILED;
        ExitNow();
    }

    otLogInfoPlat("[netif] %s multicast address %s", aIsAdded ? "Added" : "Removed",
                  Ip6AddressString(&aAddress).AsCString());

exit:
    SuccessOrDie(error);
}

void NetIf::SetLinkState(bool aIsUp)
{
    otError      error = OT_ERROR_NONE;
    struct ifreq ifr;
    bool         isUp = false;

    VerifyOrExit(mIpFd >= 0);

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, mIfName, sizeof(ifr.ifr_name));
    VerifyOrExit(ioctl(mIpFd, SIOCGIFFLAGS, &ifr) == 0, perror("ioctl"); error = OT_ERROR_FAILED);

    isUp = ((ifr.ifr_flags & IFF_UP) == IFF_UP) ? true : false;

    if (isUp != aIsUp)
    {
        otLogNotePlat("[netif] Changing interface state to %s", aIsUp ? "up" : "down");

        ifr.ifr_flags = aIsUp ? (ifr.ifr_flags | IFF_UP) : (ifr.ifr_flags & ~IFF_UP);
        VerifyOrExit(ioctl(mIpFd, SIOCSIFFLAGS, &ifr) == 0, perror("ioctl"); error = OT_ERROR_FAILED);
#if defined(RTM_NEWLINK) && defined(RTM_DELLINK)
        // wait for RTM_NEWLINK event before processing notification from kernel to avoid infinite loop
        mIsSyncingState = true;
#endif
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("[netif] Failed to update link state %s", otThreadErrorToString(error));
    }
}

#if defined(__linux__)

otError NetIf::AddRoute(bool aIsIpv6, const uint8_t *aPrefix, uint8_t aPrefixLen, uint32_t aPriority)
{
    constexpr unsigned int kBufSize = 128;
    struct
    {
        struct nlmsghdr header;
        struct rtmsg    msg;
        char            buf[kBufSize];
    } req{};

    otError error = OT_ERROR_NONE;

    VerifyOrExit(mIfIndex > 0, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mNetlinkFd >= 0, error = OT_ERROR_INVALID_STATE);

    req.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL;

    req.header.nlmsg_len  = NLMSG_LENGTH(sizeof(rtmsg));
    req.header.nlmsg_type = RTM_NEWROUTE;
    req.header.nlmsg_pid  = 0;
    req.header.nlmsg_seq  = ++mNetlinkSequence;

    req.msg.rtm_family   = (aIsIpv6 ? AF_INET6 : AF_INET);
    req.msg.rtm_src_len  = 0;
    req.msg.rtm_dst_len  = aPrefixLen;
    req.msg.rtm_tos      = 0;
    req.msg.rtm_scope    = RT_SCOPE_UNIVERSE;
    req.msg.rtm_type     = RTN_UNICAST;
    req.msg.rtm_table    = RT_TABLE_MAIN;
    req.msg.rtm_protocol = RTPROT_BOOT;
    req.msg.rtm_flags    = 0;

    AddRtAttr(reinterpret_cast<nlmsghdr *>(&req), sizeof(req), RTA_DST, aPrefix, aIsIpv6 ? 16 : 4);
    AddRtAttrUint32(&req.header, sizeof(req), RTA_PRIORITY, aPriority);
    AddRtAttrUint32(&req.header, sizeof(req), RTA_OIF, mIfIndex);

    if (send(mNetlinkFd, &req, sizeof(req), 0) < 0)
    {
        VerifyOrExit(errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK, error = OT_ERROR_BUSY);
        DieNow(OT_EXIT_ERROR_ERRNO);
    }
exit:
    return error;
}

otError NetIf::DeleteRoute(bool aIsIpv6, const uint8_t *aPrefix, uint8_t aPrefixLen)
{
    constexpr unsigned int kBufSize = 512;
    struct
    {
        struct nlmsghdr header;
        struct rtmsg    msg;
        char            buf[kBufSize];
    } req{};

    otError error = OT_ERROR_NONE;

    VerifyOrExit(mIfIndex > 0, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mNetlinkFd >= 0, error = OT_ERROR_INVALID_STATE);

    req.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_NONREC;

    req.header.nlmsg_len  = NLMSG_LENGTH(sizeof(rtmsg));
    req.header.nlmsg_type = RTM_DELROUTE;
    req.header.nlmsg_pid  = 0;
    req.header.nlmsg_seq  = ++mNetlinkSequence;

    req.msg.rtm_family   = (aIsIpv6 ? AF_INET6 : AF_INET);
    req.msg.rtm_src_len  = 0;
    req.msg.rtm_dst_len  = aPrefixLen;
    req.msg.rtm_tos      = 0;
    req.msg.rtm_scope    = RT_SCOPE_UNIVERSE;
    req.msg.rtm_type     = RTN_UNICAST;
    req.msg.rtm_table    = RT_TABLE_MAIN;
    req.msg.rtm_protocol = RTPROT_BOOT;
    req.msg.rtm_flags    = 0;

    AddRtAttr(reinterpret_cast<nlmsghdr *>(&req), sizeof(req), RTA_DST, aPrefix, aIsIpv6 ? 16 : 4);
    AddRtAttrUint32(&req.header, sizeof(req), RTA_OIF, mIfIndex);

    if (send(mNetlinkFd, &req, sizeof(req), 0) < 0)
    {
        VerifyOrExit(errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK, error = OT_ERROR_BUSY);
        DieNow(OT_EXIT_ERROR_ERRNO);
    }

exit:
    return error;
}

#if OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE

bool NetIf::HasAddedOmrRoute(const otIp6Prefix &aOmrPrefix)
{
    bool found = false;

    for (uint8_t i = 0; i < mAddedOmrRoutesNum; ++i)
    {
        if (otIp6ArePrefixesEqual(&mAddedOmrRoutes[i], &aOmrPrefix))
        {
            found = true;
            break;
        }
    }

    return found;
}

otError NetIf::AddOmrRoute(const otIp6Prefix &aPrefix)
{
    otError error;

    VerifyOrExit(mAddedOmrRoutesNum < kMaxOmrRoutesNum, error = OT_ERROR_NO_BUFS);

    error = AddRoute(aPrefix, kOmrRoutesPriority);
exit:
    return error;
}

void NetIf::UpdateOmrRoutes(void)
{
    otError               error;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;
    char                  prefixString[OT_IP6_PREFIX_STRING_SIZE];

    // Remove kernel routes if the OMR prefix is removed
    for (int i = 0; i < static_cast<int>(mAddedOmrRoutesNum); ++i)
    {
        if (otNetDataContainsOmrPrefix(mInstance, &mAddedOmrRoutes[i]))
        {
            continue;
        }

        otIp6PrefixToString(&mAddedOmrRoutes[i], prefixString, sizeof(prefixString));
        if ((error = DeleteRoute(mAddedOmrRoutes[i])) != OT_ERROR_NONE)
        {
            otLogWarnPlat("[netif] Failed to delete an OMR route %s in kernel: %s", prefixString,
                          otThreadErrorToString(error));
        }
        else
        {
            mAddedOmrRoutes[i] = mAddedOmrRoutes[mAddedOmrRoutesNum - 1];
            --mAddedOmrRoutesNum;
            --i;
            otLogInfoPlat("[netif] Successfully deleted an OMR route %s in kernel", prefixString);
        }
    }

    // Add kernel routes for OMR prefixes in Network Data
    while (otNetDataGetNextOnMeshPrefix(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        if (HasAddedOmrRoute(config.mPrefix))
        {
            continue;
        }

        otIp6PrefixToString(&config.mPrefix, prefixString, sizeof(prefixString));
        if ((error = AddOmrRoute(config.mPrefix)) != OT_ERROR_NONE)
        {
            otLogWarnPlat("[netif] Failed to add an OMR route %s in kernel: %s", prefixString,
                          otThreadErrorToString(error));
        }
        else
        {
            mAddedOmrRoutes[mAddedOmrRoutesNum++] = config.mPrefix;
            otLogInfoPlat("[netif] Successfully added an OMR route %s in kernel", prefixString);
        }
    }
}

#endif // OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE

#if OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE
otError NetIf::AddExternalRoute(const otIp6Prefix &aPrefix)
{
    otError error;

    VerifyOrExit(mAddedExternalRoutesNum < kMaxExternalRoutesNum, error = OT_ERROR_NO_BUFS);

    error = AddRoute(aPrefix, kExternalRoutePriority);
exit:
    return error;
}

bool NetIf::HasExternalRouteInNetData(const otIp6Prefix &aExternalRoute)
{
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig config;
    bool                  found = false;

    while (otNetDataGetNextRoute(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        if (otIp6ArePrefixesEqual(&config.mPrefix, &aExternalRoute))
        {
            found = true;
            break;
        }
    }
    return found;
}

bool NetIf::HasAddedExternalRoute(const otIp6Prefix &aExternalRoute)
{
    bool found = false;

    for (uint8_t i = 0; i < mAddedExternalRoutesNum; ++i)
    {
        if (otIp6ArePrefixesEqual(&mAddedExternalRoutes[i], &aExternalRoute))
        {
            found = true;
            break;
        }
    }
    return found;
}

void NetIf::UpdateExternalRoutes(void)
{
    otError               error;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otExternalRouteConfig config;
    char                  prefixString[OT_IP6_PREFIX_STRING_SIZE];

    for (int i = 0; i < static_cast<int>(mAddedExternalRoutesNum); ++i)
    {
        if (HasExternalRouteInNetData(mAddedExternalRoutes[i]))
        {
            continue;
        }

        otIp6PrefixToString(&mAddedExternalRoutes[i], prefixString, sizeof(prefixString));
        if ((error = DeleteRoute(mAddedExternalRoutes[i])) != OT_ERROR_NONE)
        {
            otLogWarnPlat("[netif] Failed to delete an external route %s in kernel: %s", prefixString,
                          otThreadErrorToString(error));
        }
        else
        {
            mAddedExternalRoutes[i] = mAddedExternalRoutes[mAddedExternalRoutesNum - 1];
            --mAddedExternalRoutesNum;
            --i;
            otLogWarnPlat("[netif] Successfully deleted an external route %s in kernel", prefixString);
        }
    }

    while (otNetDataGetNextRoute(mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        if (config.mRloc16 == otThreadGetRloc16(mInstance) || HasAddedExternalRoute(config.mPrefix))
        {
            continue;
        }
        VerifyOrExit(mAddedExternalRoutesNum < kMaxExternalRoutesNum,
                     otLogWarnPlat("[netif] No buffer to add more external routes in kernel"));

        otIp6PrefixToString(&config.mPrefix, prefixString, sizeof(prefixString));
        if ((error = AddExternalRoute(config.mPrefix)) != OT_ERROR_NONE)
        {
            otLogWarnPlat("[netif] Failed to add an external route %s in kernel: %s", prefixString,
                          otThreadErrorToString(error));
        }
        else
        {
            mAddedExternalRoutes[mAddedExternalRoutesNum++] = config.mPrefix;
            otLogWarnPlat("[netif] Successfully added an external route %s in kernel", prefixString);
        }
    }
exit:
    return;
}

#endif // OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE

#endif // defined(__linux__)

void NetIf::ProcessAddressChange(const otIp6AddressInfo *aAddressInfo, bool aIsAdded)
{
    if (aAddressInfo->mAddress->mFields.m8[0] == 0xff)
    {
        UpdateMulticast(*aAddressInfo->mAddress, aIsAdded);
    }
    else
    {
        UpdateUnicast(*aAddressInfo, aIsAdded);
    }
}

#if defined(__linux__) && OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
static bool isSameIp4Cidr(const otIp4Cidr &aCidr1, const otIp4Cidr &aCidr2)
{
    bool res = true;

    VerifyOrExit(aCidr1.mLength == aCidr2.mLength, res = false);

    // The higher (32 - length) bits must be the same, host bits are ignored.
    VerifyOrExit(((ntohl(aCidr1.mAddress.mFields.m32) ^ ntohl(aCidr2.mAddress.mFields.m32)) >> (32 - aCidr1.mLength)) ==
                     0,
                 res = false);

exit:
    return res;
}

void NetIf::HandleNat64StateChanged(void)
{
    otIp4Cidr translatorCidr;

    // Skip if NAT64 translator has not been configured with a CIDR.
    SuccessOrExit(otNat64GetCidr(mInstance, &translatorCidr));

    if (!isSameIp4Cidr(translatorCidr, mActiveNat64Cidr)) // Someone sets a new CIDR for NAT64.
    {
        char cidrString[OT_IP4_CIDR_STRING_SIZE];

        if (mActiveNat64Cidr.mLength != 0)
        {
            DeleteIp4Route(mActiveNat64Cidr);
        }
        mActiveNat64Cidr = translatorCidr;

        otIp4CidrToString(&translatorCidr, cidrString, sizeof(cidrString));
        otLogInfoPlat("[netif] NAT64 CIDR updated to %s.", cidrString);
    }

    if (otIp6IsEnabled(mInstance) && otNat64GetTranslatorState(mInstance) == OT_NAT64_STATE_ACTIVE)
    {
        AddIp4Route(mActiveNat64Cidr, kNat64RoutePriority);
        otLogInfoPlat("[netif] Adding route for NAT64");
    }
    else if (mActiveNat64Cidr.mLength > 0) // Translator is not active.
    {
        DeleteIp4Route(mActiveNat64Cidr);
        otLogInfoPlat("[netif] Deleting route for NAT64");
    }

exit:
    return;
}
#endif // defined(__linux__) && OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

void NetIf::HandleStateChanged(otChangedFlags aFlags)
{
    if (OT_CHANGED_THREAD_NETIF_STATE & aFlags)
    {
        SetLinkState(otIp6IsEnabled(mInstance));
    }
    if (OT_CHANGED_THREAD_NETDATA & aFlags)
    {
#if defined(__linux__) && OPENTHREAD_POSIX_CONFIG_INSTALL_OMR_ROUTES_ENABLE
        UpdateOmrRoutes();
#endif
#if defined(__linux__) && OPENTHREAD_POSIX_CONFIG_INSTALL_EXTERNAL_ROUTES_ENABLE
        UpdateExternalRoutes();
#endif
#if OPENTHREAD_POSIX_CONFIG_FIREWALL_ENABLE
        ot::Posix::UpdateIpSets(mInstance);
#endif
    }
#if defined(__linux__) && OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    if ((OT_CHANGED_NAT64_TRANSLATOR_STATE | OT_CHANGED_THREAD_NETIF_STATE) & aFlags)
    {
        HandleNat64StateChanged();
    }
#endif
}

#if defined(__linux__)

static void ProcessNetifAddrEvent(otInstance      *aInstance,
                                  int              aIpFd,
                                  unsigned int     aIfIndex,
                                  const char      *aIfName,
                                  struct nlmsghdr *aNetlinkMessage)
{
    struct ifaddrmsg   *ifaddr = reinterpret_cast<struct ifaddrmsg *>(NLMSG_DATA(aNetlinkMessage));
    size_t              rtaLength;
    otError             error = OT_ERROR_NONE;
    struct sockaddr_in6 addr6;

    OT_UNUSED_VARIABLE(aIpFd);
    OT_UNUSED_VARIABLE(aIfName);

    VerifyOrExit(aInstance != nullptr, error = OT_ERROR_NONE);
    VerifyOrExit(ifaddr->ifa_index == aIfIndex && ifaddr->ifa_family == AF_INET6, error = OT_ERROR_NONE);

    rtaLength = IFA_PAYLOAD(aNetlinkMessage);

    for (struct rtattr *rta = reinterpret_cast<struct rtattr *>(IFA_RTA(ifaddr)); RTA_OK(rta, rtaLength);
         rta                = RTA_NEXT(rta, rtaLength))
    {
        switch (rta->rta_type)
        {
        case IFA_ADDRESS:
        case IFA_LOCAL:
        case IFA_BROADCAST:
        case IFA_ANYCAST:
        case IFA_MULTICAST:
        {
            ot::Ip6::Address addr;
            memcpy(&addr, RTA_DATA(rta), sizeof(addr));

            memset(&addr6, 0, sizeof(addr6));
            addr6.sin6_family = AF_INET6;
            memcpy(&addr6.sin6_addr, RTA_DATA(rta), sizeof(addr6.sin6_addr));

            if (aNetlinkMessage->nlmsg_type == RTM_NEWADDR)
            {
                if (!addr.IsMulticast())
                {
                    otNetifAddress netAddr;

                    netAddr.mAddress      = addr;
                    netAddr.mPrefixLength = ifaddr->ifa_prefixlen;

                    error = otIp6AddUnicastAddress(aInstance, &netAddr);
                }
                else
                {
                    otNetifMulticastAddress netAddr;

                    netAddr.mAddress = addr;

                    error = otIp6SubscribeMulticastAddress(aInstance, &addr);
                }

                logAddrEvent(/* isAdd */ true, addr, error);
                if (error == OT_ERROR_ALREADY || error == OT_ERROR_REJECTED)
                {
                    error = OT_ERROR_NONE;
                }

                SuccessOrExit(error);
            }
            else if (aNetlinkMessage->nlmsg_type == RTM_DELADDR)
            {
                if (!addr.IsMulticast())
                {
                    error = otIp6RemoveUnicastAddress(aInstance, &addr);
                }
                else
                {
                    error = otIp6UnsubscribeMulticastAddress(aInstance, &addr);
                }

                logAddrEvent(/* isAdd */ false, addr, error);
                if (error == OT_ERROR_NOT_FOUND || error == OT_ERROR_REJECTED)
                {
                    error = OT_ERROR_NONE;
                }

                SuccessOrExit(error);
            }
            else
            {
                continue;
            }
            break;
        }

        default:
            otLogDebgPlat("[netif] Unexpected address type (%d).", (int)rta->rta_type);
            break;
        }
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("[netif] Failed to process addr event, error:%s", otThreadErrorToString(error));
    }
}

static void ProcessNetifLinkEvent(otInstance      *aInstance,
                                  bool            &aIsSyncingState,
                                  unsigned int     aIfIndex,
                                  struct nlmsghdr *aNetlinkMessage)
{
    struct ifinfomsg *ifinfo = reinterpret_cast<struct ifinfomsg *>(NLMSG_DATA(aNetlinkMessage));
    otError           error  = OT_ERROR_NONE;
    bool              isUp;

    VerifyOrExit(aInstance != nullptr, error = OT_ERROR_NONE);
    VerifyOrExit(ifinfo->ifi_index == static_cast<int>(aIfIndex) && (ifinfo->ifi_change & IFF_UP),
                 error = OT_ERROR_NONE);

    isUp = ((ifinfo->ifi_flags & IFF_UP) != 0);

    otLogInfoPlat("[netif] Host netif is %s", isUp ? "up" : "down");

#if defined(RTM_NEWLINK) && defined(RTM_DELLINK)
    if (aIsSyncingState)
    {
        VerifyOrExit(isUp == otIp6IsEnabled(aInstance),
                     otLogWarnPlat("[netif] Host netif state notification is unexpected (ignore)"));
        aIsSyncingState = false;
    }
    else
#endif
        if (isUp != otIp6IsEnabled(aInstance))
    {
        SuccessOrExit(error = otIp6SetEnabled(aInstance, isUp));
        otLogInfoPlat("[netif] Succeeded to sync netif state with host");
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("[netif] Failed to sync netif state with host: %s", otThreadErrorToString(error));
    }
}
#endif // defined(__linux__)

#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)

#if defined(__FreeBSD__)
#define ROUNDUP(a) ((a) > 0 ? (1 + (((a)-1) | (sizeof(uint32_t) - 1))) : sizeof(uint32_t))
#endif

#if defined(__APPLE__)
#define ROUNDUP(a) ((a) > 0 ? (1 + (((a)-1) | (sizeof(uint32_t) - 1))) : sizeof(uint32_t))
#define DARWIN_SA_SIZE(sa) ROUNDUP(sa->sa_len)
#define SA_SIZE(sa) DARWIN_SA_SIZE(sa)
#endif

#if defined(__NetBSD__)
#define RT_ROUNDUP2(a, n) ((a) > 0 ? (1 + (((a)-1U) | ((n)-1))) : (n))
#define RT_ROUNDUP(a) RT_ROUNDUP2((a), sizeof(uint64_t))
#define SA_SIZE(sa) RT_ROUNDUP(sa->sa_len)
#endif

static void ProcessNetifAddrEvent(otInstance       *aInstance,
                                  int               aIpFd,
                                  unsigned int      aIfIndex,
                                  const char       *aIfName,
                                  struct rt_msghdr *rtm)
{
    otError            error = OT_ERROR_NONE;
    struct ifa_msghdr *ifam;
#ifdef RTM_NEWMADDR
    struct ifma_msghdr *ifmam;
#endif
    struct sockaddr_in6 addr6;
    struct sockaddr_in6 netmask;
    uint8_t            *addrbuf;
    unsigned int        addrmask = 0;
    unsigned int        i;
    struct sockaddr    *sa;
    bool                is_link_local;

    VerifyOrExit(aInstance != nullptr, error = OT_ERROR_INVALID_STATE);

    addr6.sin6_family   = 0;
    netmask.sin6_family = 0;

    if ((rtm->rtm_type == RTM_NEWADDR) || (rtm->rtm_type == RTM_DELADDR))
    {
        ifam = reinterpret_cast<struct ifa_msghdr *>(rtm);

        VerifyOrExit(ifam->ifam_index == static_cast<unsigned int>(aIfIndex));

        addrbuf  = (uint8_t *)&ifam[1];
        addrmask = (unsigned int)ifam->ifam_addrs;
    }
#ifdef RTM_NEWMADDR
    else if ((rtm->rtm_type == RTM_NEWMADDR) || (rtm->rtm_type == RTM_DELMADDR))
    {
        ifmam = reinterpret_cast<struct ifma_msghdr *>(rtm);

        VerifyOrExit(ifmam->ifmam_index == static_cast<unsigned int>(aIfIndex));

        addrbuf  = (uint8_t *)&ifmam[1];
        addrmask = (unsigned int)ifmam->ifmam_addrs;
    }
#endif

    if (addrmask != 0)
    {
        for (i = 0; i < RTAX_MAX; i++)
        {
            unsigned int mask = (addrmask & (1 << i));
            if (mask)
            {
                sa = (struct sockaddr *)addrbuf;

                if (sa->sa_family == AF_INET6)
                {
                    if (i == RTAX_IFA)
                        memcpy(&addr6, sa, sizeof(sockaddr_in6));
                    if (i == RTAX_NETMASK)
                        memcpy(&netmask, sa, sizeof(sockaddr_in6));
                }
                addrbuf += SA_SIZE(sa);
            }
        }
    }

    if (addr6.sin6_family == AF_INET6)
    {
        is_link_local = false;
        if (IN6_IS_ADDR_LINKLOCAL(&addr6.sin6_addr))
        {
            is_link_local = true;
            // clear the scope -- Mac OS X sends this to us (bozos!)
            addr6.sin6_addr.s6_addr[3] = 0;
        }
        else if (IN6_IS_ADDR_MC_LINKLOCAL(&addr6.sin6_addr))
        {
            addr6.sin6_addr.s6_addr[3] = 0;
        }

        ot::Ip6::Address addr;
        memcpy(&addr, &addr6.sin6_addr, sizeof(addr));

        if (rtm->rtm_type == RTM_NEWADDR
#ifdef RTM_NEWMADDR
            || rtm->rtm_type == RTM_NEWMADDR
#endif
        )
        {
            if (!addr.IsMulticast())
            {
                otNetifAddress netAddr;
                bool           subscribed;

                netAddr.mAddress      = addr;
                netAddr.mPrefixLength = NetmaskToPrefixLength(&netmask);

                subscribed = UnicastAddressIsSubscribed(aInstance, &netAddr);

                if (subscribed)
                {
                    logAddrEvent(/* isAdd */ true, addr, OT_ERROR_ALREADY);
                    error = OT_ERROR_NONE;
                }
                else
                {
                    if (is_link_local)
                    {
                        // remove the stack-added link-local address

                        int                 err;
                        struct in6_aliasreq ifr6;
                        char                addressString[INET6_ADDRSTRLEN + 1];

                        OT_UNUSED_VARIABLE(addressString); // if otLog*Plat is disabled, we'll get a warning

                        memset(&ifr6, 0, sizeof(ifr6));
                        strlcpy(ifr6.ifra_name, aIfName, sizeof(ifr6.ifra_name));
                        ifr6.ifra_addr.sin6_family = AF_INET6;
                        ifr6.ifra_addr.sin6_len    = sizeof(ifr6.ifra_addr);
                        memcpy(&ifr6.ifra_addr.sin6_addr, &addr6.sin6_addr, sizeof(struct in6_addr));
                        ifr6.ifra_prefixmask.sin6_family = AF_INET6;
                        ifr6.ifra_prefixmask.sin6_len    = sizeof(ifr6.ifra_prefixmask);
                        InitNetaskWithPrefixLength(&ifr6.ifra_prefixmask.sin6_addr, netAddr.mPrefixLength);
                        ifr6.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
                        ifr6.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

#if defined(__APPLE__)
                        ifr6.ifra_lifetime.ia6t_expire    = ND6_INFINITE_LIFETIME;
                        ifr6.ifra_lifetime.ia6t_preferred = ND6_INFINITE_LIFETIME;
#endif

                        err = ioctl(aIpFd, SIOCDIFADDR_IN6, &ifr6);
                        if (err != 0)
                        {
                            otLogWarnPlat(
                                "[netif] Error (%d) removing stack-addded link-local address %s", errno,
                                inet_ntop(AF_INET6, addr6.sin6_addr.s6_addr, addressString, sizeof(addressString)));
                            error = OT_ERROR_FAILED;
                        }
                        else
                        {
                            otLogNotePlat(
                                "[netif]        %s (removed stack-added link-local)",
                                inet_ntop(AF_INET6, addr6.sin6_addr.s6_addr, addressString, sizeof(addressString)));
                            error = OT_ERROR_NONE;
                        }
                    }
                    else
                    {
                        error = otIp6AddUnicastAddress(aInstance, &netAddr);
                        logAddrEvent(/* isAdd */ true, addr, error);
                        if (error == OT_ERROR_ALREADY)
                        {
                            error = OT_ERROR_NONE;
                        }
                    }
                }
                SuccessOrExit(error);
            }
            else
            {
                otNetifMulticastAddress netAddr;
                netAddr.mAddress = addr;

                error = otIp6SubscribeMulticastAddress(aInstance, &addr);
                logAddrEvent(/* isAdd */ true, addr, error);
                if (error == OT_ERROR_ALREADY || error == OT_ERROR_REJECTED)
                {
                    error = OT_ERROR_NONE;
                }
                SuccessOrExit(error);
            }
        }
        else if (rtm->rtm_type == RTM_DELADDR
#ifdef RTM_DELMADDR
                 || rtm->rtm_type == RTM_DELMADDR
#endif
        )
        {
            if (!addr.IsMulticast())
            {
                error = otIp6RemoveUnicastAddress(aInstance, &addr);
                logAddrEvent(/* isAdd */ false, addr, error);
                if (error == OT_ERROR_NOT_FOUND)
                {
                    error = OT_ERROR_NONE;
                }
            }
            else
            {
                error = otIp6UnsubscribeMulticastAddress(aInstance, &addr);
                logAddrEvent(/* isAdd */ false, addr, error);
                if (error == OT_ERROR_NOT_FOUND)
                {
                    error = OT_ERROR_NONE;
                }
            }
            SuccessOrExit(error);
        }
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("[netif] Failed to process addr event, error:%s", otThreadErrorToString(error));
    }
}

static void ProcessNetifInfoEvent(otInstance *aInstance, unsigned int aIfIndex, struct rt_msghdr *rtm)
{
    struct if_msghdr *ifm   = reinterpret_cast<struct if_msghdr *>(rtm);
    otError           error = OT_ERROR_NONE;

    VerifyOrExit(aInstance != nullptr, error = OT_ERROR_NONE);
    VerifyOrExit(ifm->ifm_index == static_cast<int>(aIfIndex), error = OT_ERROR_NONE);

    // TODO(wgtdkp):
    // SuccessOrExit(error = otIp6SetEnabled(aInstance, (ifm->ifm_flags & IFF_UP)));

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("[netif] Failed to process info event: %s", otThreadErrorToString(error));
    }
}

#endif

#if defined(__linux__)

#define ERR_RTA(errmsg, requestPayloadLength) \
    ((struct rtattr *)((char *)(errmsg)) + NLMSG_ALIGN(sizeof(struct nlmsgerr)) + NLMSG_ALIGN(requestPayloadLength))

// The format of NLMSG_ERROR is described below:
//
// ----------------------------------------------
// | struct nlmsghdr - response header          |
// ----------------------------------------------------------------
// |    int error                               |                 |
// ---------------------------------------------| struct nlmsgerr |
// | struct nlmsghdr - original request header  |                 |
// ----------------------------------------------------------------
// | ** optionally (1) payload of the request   |
// ----------------------------------------------
// | ** optionally (2) extended ACK attrs       |
// ----------------------------------------------
//
static void HandleNetlinkResponse(struct nlmsghdr *msg)
{
    const struct nlmsgerr *err;
    const char            *errorMsg;
    size_t                 rtaLength;
    size_t                 requestPayloadLength = 0;
    uint32_t               requestSeq           = 0;

    if (msg->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr)))
    {
        otLogWarnPlat("[netif] Truncated netlink reply of request#%u", requestSeq);
        ExitNow();
    }

    err        = reinterpret_cast<const nlmsgerr *>(NLMSG_DATA(msg));
    requestSeq = err->msg.nlmsg_seq;

    if (err->error == 0)
    {
        otLogInfoPlat("[netif] Succeeded to process request#%u", requestSeq);
        ExitNow();
    }

    // For rtnetlink, `abs(err->error)` maps to values of `errno`.
    // But this is not a requirement in RFC 3549.
    errorMsg = strerror(abs(err->error));

    // The payload of the request is omitted if NLM_F_CAPPED is set
    if (!(msg->nlmsg_flags & NLM_F_CAPPED))
    {
        requestPayloadLength = NLMSG_PAYLOAD(&err->msg, 0);
    }

    rtaLength = NLMSG_PAYLOAD(msg, sizeof(struct nlmsgerr)) - requestPayloadLength;

    for (struct rtattr *rta = ERR_RTA(err, requestPayloadLength); RTA_OK(rta, rtaLength);
         rta                = RTA_NEXT(rta, rtaLength))
    {
        if (rta->rta_type == NLMSGERR_ATTR_MSG)
        {
            errorMsg = reinterpret_cast<const char *>(RTA_DATA(rta));
            break;
        }
        else
        {
            otLogDebgPlat("[netif] Ignoring netlink response attribute %d (request#%u)", rta->rta_type, requestSeq);
        }
    }

    otLogWarnPlat("[netif] Failed to process request#%u: %s", requestSeq, errorMsg);

exit:
    return;
}

#endif // defined(__linux__)

void NetIf::ProcessNetlinkEvent(void)
{
    const size_t kMaxNetifEvent = 8192;
    ssize_t      length;

    union
    {
#if defined(__linux__)
        nlmsghdr nlMsg;
#else
        rt_msghdr rtMsg;
#endif
        char buffer[kMaxNetifEvent];
    } msgBuffer;

    length = recv(mNetlinkFd, msgBuffer.buffer, sizeof(msgBuffer.buffer), 0);

    VerifyOrExit(length > 0);

#if defined(__linux__)
    for (struct nlmsghdr *msg = &msgBuffer.nlMsg; NLMSG_OK(msg, static_cast<size_t>(length));
         msg                  = NLMSG_NEXT(msg, length))
    {
#else
    {
        // BSD sends one message per read to routing socket (see route.c, monitor command)
        struct rt_msghdr *msg;

        msg = &msgBuffer.rtMsg;

#define nlmsg_type rtm_type

#endif
        switch (msg->nlmsg_type)
        {
        case RTM_NEWADDR:
        case RTM_DELADDR:
            ProcessNetifAddrEvent(mInstance, mIpFd, mIfIndex, mIfName, msg);
            break;

#if defined(RTM_NEWMADDR) && defined(RTM_DELMADDR)
        case RTM_NEWMADDR:
        case RTM_DELMADDR:
            ProcessNetifAddrEvent(mInstance, mIpFd, mIfIndex, mIfName, msg);
            break;
#endif

#if defined(RTM_NEWLINK) && defined(RTM_DELLINK)
        case RTM_NEWLINK:
        case RTM_DELLINK:
            ProcessNetifLinkEvent(mInstance, mIsSyncingState, mIfIndex, msg);
            break;
#endif

#if !defined(__linux__)
        case RTM_IFINFO:
            ProcessNetifInfoEvent(mInstance, mIfIndex, msg);
            break;

#else
        case NLMSG_ERROR:
            HandleNetlinkResponse(msg);
            break;
#endif

#if defined(ROUTE_FILTER) || defined(RO_MSGFILTER) || defined(__linux__)
        default:
            otLogWarnPlat("[netif] Unhandled/Unexpected netlink/route message (%d).", (int)msg->nlmsg_type);
            break;
#else
            // this platform doesn't support filtering, so we expect messages of other types...we just ignore them
#endif
        }
    }

exit:
    return;
}

#if defined(__linux__)
void NetIf::SetAddrGenModeToNone(void)
{
    struct
    {
        struct nlmsghdr  nh;
        struct ifinfomsg ifi;
        char             buf[512];
    } req;

    const uint8_t mode = IN6_ADDR_GEN_MODE_NONE;

    memset(&req, 0, sizeof(req));

    req.nh.nlmsg_len   = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req.nh.nlmsg_type  = RTM_NEWLINK;
    req.nh.nlmsg_pid   = 0;
    req.nh.nlmsg_seq   = ++mNetlinkSequence;

    req.ifi.ifi_index  = static_cast<int>(mIfIndex);
    req.ifi.ifi_change = 0xffffffff;
    req.ifi.ifi_flags  = IFF_MULTICAST | IFF_NOARP;

    {
        struct rtattr *afSpec  = AddRtAttr(&req.nh, sizeof(req), IFLA_AF_SPEC, 0, 0);
        struct rtattr *afInet6 = AddRtAttr(&req.nh, sizeof(req), AF_INET6, 0, 0);
        struct rtattr *inet6AddrGenMode =
            AddRtAttr(&req.nh, sizeof(req), IFLA_INET6_ADDR_GEN_MODE, &mode, sizeof(mode));

        afInet6->rta_len += inet6AddrGenMode->rta_len;
        afSpec->rta_len += afInet6->rta_len;
    }

    if (send(mNetlinkFd, &req, req.nh.nlmsg_len, 0) != -1)
    {
        otLogInfoPlat("[netif] Sent request#%u to set addr_gen_mode to %d", mNetlinkSequence, mode);
    }
    else
    {
        otLogWarnPlat("[netif] Failed to send request#%u to set addr_gen_mode to %d", mNetlinkSequence, mode);
    }
}

// Creates a new Thread tunnel device/interface with the name specified in `aPlatformConfig`.
// On return, `aInterfaceName` will contain the final tunnel interface name that is created.
int NetIf::CreateTunDevice(const otPlatformConfig *aPlatformConfig, char *aInterfaceName)
{
    int          tunFd;
    struct ifreq ifr;
    const char  *interfaceName;

    tunFd = open(OPENTHREAD_POSIX_TUN_DEVICE, O_RDWR | O_CLOEXEC | O_NONBLOCK);
    VerifyOrDie(tunFd >= 0, OT_EXIT_ERROR_ERRNO);

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (!aPlatformConfig->mPersistentInterface)
    {
        ifr.ifr_flags |= static_cast<short>(IFF_TUN_EXCL);
    }

    interfaceName = aPlatformConfig->mInterfaceName;
    if (interfaceName)
    {
        VerifyOrDie(strlen(interfaceName) < IFNAMSIZ, OT_EXIT_INVALID_ARGUMENTS);

        strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ);
    }
    else
    {
        strncpy(ifr.ifr_name, "wpan%d", IFNAMSIZ);
    }

    VerifyOrDie(ioctl(tunFd, TUNSETIFF, static_cast<void *>(&ifr)) == 0, OT_EXIT_ERROR_ERRNO);

    strncpy(aInterfaceName, ifr.ifr_name, IFNAMSIZ);

    if (aPlatformConfig->mPersistentInterface)
    {
        VerifyOrDie(ioctl(tunFd, TUNSETPERSIST, 1) == 0, OT_EXIT_ERROR_ERRNO);
        // Set link down to reset the tun configuration.
        // This will drop all existing IP addresses on the interface.
        SetLinkState(false);
    }

    VerifyOrDie(ioctl(tunFd, TUNSETLINK, ARPHRD_NONE) == 0, OT_EXIT_ERROR_ERRNO);

    ifr.ifr_mtu = static_cast<int>(kMaxIp6Size);
    VerifyOrDie(ioctl(mIpFd, SIOCSIFMTU, static_cast<void *>(&ifr)) == 0, OT_EXIT_ERROR_ERRNO);

    return tunFd;
}
#endif

#if defined(__APPLE__) && (OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_UTUN)
int NetIf::CreateTunDevice(const otPlatformConfig *aPlatformConfig, char *aInterfaceName)
{
    (void)aPlatformConfig;
    int                 err = 0;
    struct sockaddr_ctl addr;
    struct ctl_info     info;
    socklen_t           devNameLen;

    mTunFd = SocketWithCloseExec(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL, kSocketNonBlock);
    VerifyOrDie(mTunFd >= 0, OT_EXIT_ERROR_ERRNO);

    memset(&info, 0, sizeof(info));
    strncpy(info.ctl_name, UTUN_CONTROL_NAME, strlen(UTUN_CONTROL_NAME));
    err = ioctl(mTunFd, CTLIOCGINFO, &info);
    VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);

    addr.sc_id      = info.ctl_id;
    addr.sc_len     = sizeof(addr);
    addr.sc_family  = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;

    addr.sc_unit = 0;
    err          = connect(mTunFd, (struct sockaddr *)&addr, sizeof(addr));
    VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);

    devNameLen = (socklen_t)IFNAMSIZ;
    err        = getsockopt(mTunFd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, aInterfaceName, &devNameLen);
    VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);

    otLogInfoPlat("[netif] Tunnel device name = '%s'", aInterfaceName);

    return 0;
}
#endif

void NetIf::DestroyTunDevice(void)
{
    otError error = OT_ERROR_NONE;

#if defined(__NetBSD__) || defined(__FreeBSD__)
    struct ifreq ifr;

    VerifyOrExit(mIpFd >= 0, error = OT_ERROR_INVALID_STATE);

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, mIfName, sizeof(ifr.ifr_name));
    VerifyOrExit(ioctl(mIpFd, SIOCIFDESTROY, &ifr) == 0, error = OT_ERROR_FAILED);
exit:

#else
    error     = OT_ERROR_NOT_IMPLEMENTED;
#endif

    if (error != OT_ERROR_NONE)
    {
        const char *errorMsg = (error == OT_ERROR_FAILED) ? strerror(errno) : otThreadErrorToString(error);
        otLogWarnPlat("[netif] Failed to destroy tun device %s: %s", mIfName, errorMsg);
    }
}

#if defined(__NetBSD__) ||                                                                             \
    (defined(__APPLE__) && (OPENTHREAD_POSIX_CONFIG_MACOS_TUN_OPTION == OT_POSIX_CONFIG_MACOS_TUN)) || \
    defined(__FreeBSD__)
int NetIf::CreateTunDevice(const otPlatformConfig *aPlatformConfig, char *aInterfaceName)
{
    int         flags = IFF_BROADCAST | IFF_MULTICAST;
    int         err;
    const char *last_slash;
    const char *path;

    (void)aPlatformConfig;

    path = OPENTHREAD_POSIX_TUN_DEVICE;

    mTunFd = open(path, O_RDWR | O_NONBLOCK);
    VerifyOrDie(mTunFd >= 0, OT_EXIT_ERROR_ERRNO);

#if defined(__NetBSD__) || defined(__FreeBSD__)
    err = ioctl(mTunFd, TUNSIFMODE, &flags);
    VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);
#endif

    flags = 1;
    err   = ioctl(mTunFd, TUNSIFHEAD, &flags);
    VerifyOrDie(err == 0, OT_EXIT_ERROR_ERRNO);

    last_slash = strrchr(OPENTHREAD_POSIX_TUN_DEVICE, '/');
    VerifyOrDie(last_slash != nullptr, OT_EXIT_ERROR_ERRNO);
    last_slash++;

    strncpy(aInterfaceName, last_slash, IFNAMSIZ);
}
#endif

static int InitNetLink(void)
{
    int netlinkFd;

#if defined(__linux__)
    netlinkFd = SocketWithCloseExec(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE, kSocketNonBlock);
#elif defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
    netlinkFd = SocketWithCloseExec(PF_ROUTE, SOCK_RAW, 0, kSocketNonBlock);
#else
#error "!! Unknown platform !!"
#endif
    VerifyOrDie(netlinkFd >= 0, OT_EXIT_ERROR_ERRNO);

#if defined(SOL_NETLINK)
    {
        int enable = 1;

        OT_UNUSED_VARIABLE(enable);

#if defined(NETLINK_EXT_ACK)
        if (setsockopt(netlinkFd, SOL_NETLINK, NETLINK_EXT_ACK, &enable, sizeof(enable)) != 0)
        {
            otLogWarnPlat("[netif] Failed to enable NETLINK_EXT_ACK: %s", strerror(errno));
        }
#endif
#if defined(NETLINK_CAP_ACK)
        if (setsockopt(netlinkFd, SOL_NETLINK, NETLINK_CAP_ACK, &enable, sizeof(enable)) != 0)
        {
            otLogWarnPlat("[netif] Failed to enable NETLINK_CAP_ACK: %s", strerror(errno));
        }
#endif
    }
#endif

#if defined(__linux__)
    {
        struct sockaddr_nl sa;

        memset(&sa, 0, sizeof(sa));
        sa.nl_family = AF_NETLINK;
        sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV6_IFADDR;
        VerifyOrDie(bind(netlinkFd, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) == 0, OT_EXIT_ERROR_ERRNO);
    }
#endif

#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
    {
        int status;
#ifdef ROUTE_FILTER
        unsigned int msgfilter = ROUTE_FILTER(RTM_IFINFO) | ROUTE_FILTER(RTM_NEWADDR) | ROUTE_FILTER(RTM_DELADDR) |
                                 ROUTE_FILTER(RTM_NEWMADDR) | ROUTE_FILTER(RTM_DELMADDR);
#define FILTER_CMD ROUTE_MSGFILTER
#define FILTER_ARG msgfilter
#define FILTER_ARG_SZ sizeof(msgfilter)
#endif
#ifdef RO_MSGFILTER
        uint8_t msgfilter[] = {RTM_IFINFO, RTM_NEWADDR, RTM_DELADDR};
#define FILTER_CMD RO_MSGFILTER
#define FILTER_ARG msgfilter
#define FILTER_ARG_SZ sizeof(msgfilter)
#endif
#if defined(ROUTE_FILTER) || defined(RO_MSGFILTER)
        status = setsockopt(netlinkFd, AF_ROUTE, FILTER_CMD, FILTER_ARG, FILTER_ARG_SZ);
        VerifyOrDie(status == 0, OT_EXIT_ERROR_ERRNO);
#endif
        status = fcntl(netlinkFd, F_SETFL, O_NONBLOCK);
        VerifyOrDie(status == 0, OT_EXIT_ERROR_ERRNO);
    }
#endif // defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)

    return netlinkFd;
}

NetIf &NetIf::Get(void)
{
    static NetIf sNetIf;

    return sNetIf;
}

void NetIf::Deinit(void)
{
    Mainloop::Manager::Get().Remove(*this);

    if (mTunFd != -1)
    {
        close(mTunFd);
        mTunFd = -1;

#if defined(__NetBSD__) || defined(__FreeBSD__)
        DestroyTunDevice();
#endif
    }

    if (mIpFd != -1)
    {
        close(mIpFd);
        mIpFd = -1;
    }

    if (mNetlinkFd != -1)
    {
        close(mNetlinkFd);
        mNetlinkFd = -1;
    }

    mIfIndex   = 0;
    mIfName[0] = '\0';
}

void NetIf::Init(const otPlatformConfig *aPlatformConfig)
{
    mIpFd = SocketWithCloseExec(AF_INET6, SOCK_DGRAM, IPPROTO_IP, kSocketNonBlock);
    VerifyOrDie(mIpFd >= 0, OT_EXIT_ERROR_ERRNO);

    mTunFd   = CreateTunDevice(aPlatformConfig, mIfName);
    mIfIndex = if_nametoindex(mIfName);
    VerifyOrDie(mIfIndex > 0, OT_EXIT_FAILURE);
    otLogInfoPlat("[netif] tun interface index: %d", mIfIndex);

    mNetlinkFd = InitNetLink();

#if __linux__
    SetAddrGenModeToNone();
#endif

    Mainloop::Manager::Get().Add(*this);
}

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
static void Nat64SetUp(otInstance *aInstance)
{
    otIp4Cidr cidr;

    if (otIp4CidrFromString(OPENTHREAD_POSIX_CONFIG_NAT64_CIDR, &cidr) == OT_ERROR_NONE && cidr.mLength != 0)
    {
        otNat64SetIp4Cidr(aInstance, &cidr);
    }
    else
    {
        otLogInfoPlat("[netif] No default NAT64 CIDR provided.");
    }
}
#endif

void NetIf::SetUp(otInstance *aInstance)
{
    OT_ASSERT(aInstance != nullptr);

    mInstance = aInstance;

    otIp6SetReceiveFilterEnabled(mInstance, true);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    otIcmp6SetEchoMode(mInstance, OT_ICMP6_ECHO_HANDLER_ALL);
#else
    otIcmp6SetEchoMode(mInstance, OT_ICMP6_ECHO_HANDLER_DISABLED);
#endif
    otIp6SetReceiveCallback(mInstance, ProcessReceive, this);
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    // We can use the same function for IPv6 and translated IPv4 messages.
    otNat64SetReceiveIp4Callback(mInstance, ProcessReceive, this);
#endif
    otIp6SetAddressCallback(mInstance, ProcessAddressChange, this);
#if OPENTHREAD_POSIX_MULTICAST_PROMISCUOUS_REQUIRED
    otIp6SetMulticastPromiscuousEnabled(aInstance, true);
#endif
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    Nat64SetUp(aInstance);
#endif

#if OPENTHREAD_POSIX_USE_MLD_MONITOR
    ot::Posix::MldMonitor::Get().SetUp(mInstance);
#endif
}

void NetIf::TearDown(void)
{
#if OPENTHREAD_POSIX_USE_MLD_MONITOR
    ot::Posix::MldMonitor::Get().TearDown();
#endif
}

void NetIf::ProcessReceive(otMessage *aMessage)
{
    char     packet[kMaxIp6Size + 4];
    otError  error     = OT_ERROR_NONE;
    uint16_t length    = otMessageGetLength(aMessage);
    size_t   offset    = 0;
    uint16_t maxLength = sizeof(packet) - 4;
#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
    // BSD tunnel drivers use (for legacy reasons) a 4-byte header to determine the address family of the packet
    offset += 4;
#endif

    assert(length <= kMaxIp6Size);

    VerifyOrExit(otMessageRead(aMessage, 0, &packet[offset], maxLength) == length, error = OT_ERROR_NO_BUFS);

#if OPENTHREAD_POSIX_LOG_TUN_PACKETS
    otLogInfoPlat("[netif] Packet from NCP (%u bytes)", static_cast<uint16_t>(length));
    otDumpInfoPlat("", &packet[offset], length);
#endif

#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
    packet[0] = 0;
    packet[1] = 0;
    packet[2] = (PF_INET6 << 8) & 0xFF;
    packet[3] = (PF_INET6 << 0) & 0xFF;
    length += 4;
#endif

    VerifyOrExit(write(mTunFd, packet, length) == length, perror("write"); error = OT_ERROR_FAILED);

exit:
    otMessageFree(aMessage);

    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("[netif] Failed to receive, error:%s", otThreadErrorToString(error));
    }
}

void NetIf::ProcessTransmit(void)
{
    otMessage *message = nullptr;
    ssize_t    rval;
    char       packet[kMaxIp6Size];
    otError    error  = OT_ERROR_NONE;
    size_t     offset = 0;
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    bool isIp4 = false;
#endif

    rval = read(mTunFd, packet, sizeof(packet));
    VerifyOrExit(rval > 0, error = OT_ERROR_FAILED);

    // There may be data in the tunnel interface before this class is set up with OT
    VerifyOrExit(mInstance != nullptr, error = OT_ERROR_DROP);

#if defined(__APPLE__) || defined(__NetBSD__) || defined(__FreeBSD__)
    // BSD tunnel drivers have (for legacy reasons), may have a 4-byte header on them
    if ((rval >= 4) && (packet[0] == 0) && (packet[1] == 0))
    {
        rval -= 4;
        offset = 4;
    }
#endif

    {
        otMessageSettings settings;

        settings.mLinkSecurityEnabled = (otThreadGetDeviceRole(mInstance) != OT_DEVICE_ROLE_DISABLED);
        settings.mPriority            = OT_MESSAGE_PRIORITY_LOW;
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
        isIp4   = (packet[offset] & 0xf0) == 0x40;
        message = isIp4 ? otIp4NewMessage(mInstance, &settings) : otIp6NewMessage(mInstance, &settings);
#else
        message = otIp6NewMessage(mInstance, &settings);
#endif
        VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);
    }

#if OPENTHREAD_POSIX_LOG_TUN_PACKETS
    otLogInfoPlat("[netif] Packet to NCP (%hu bytes)", static_cast<uint16_t>(rval));
    otDumpInfoPlat("", &packet[offset], static_cast<size_t>(rval));
#endif

    SuccessOrExit(error = otMessageAppend(message, &packet[offset], static_cast<uint16_t>(rval)));

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    error = isIp4 ? otNat64Send(mInstance, message) : otIp6Send(mInstance, message);
#else
    error = otIp6Send(mInstance, message);
#endif
    message = nullptr;

exit:
    if (message != nullptr)
    {
        otMessageFree(message);
    }

    if (error != OT_ERROR_NONE)
    {
        if (error == OT_ERROR_DROP)
        {
            otLogInfoPlat("[netif] Message dropped by Thread");
        }
        else
        {
            otLogWarnPlat("[netif] Failed to transmit, error:%s", otThreadErrorToString(error));
        }
    }
}

void NetIf::Update(otSysMainloopContext &aContext)
{
    VerifyOrExit(mIfIndex > 0);

    assert(mTunFd >= 0);
    assert(mNetlinkFd >= 0);
    assert(mIpFd >= 0);

    FD_SET(mTunFd, &aContext.mReadFdSet);
    FD_SET(mTunFd, &aContext.mErrorFdSet);
    FD_SET(mNetlinkFd, &aContext.mReadFdSet);
    FD_SET(mNetlinkFd, &aContext.mErrorFdSet);

    if (mTunFd > aContext.mMaxFd)
    {
        aContext.mMaxFd = mTunFd;
    }

    if (mNetlinkFd > aContext.mMaxFd)
    {
        aContext.mMaxFd = mNetlinkFd;
    }

exit:
    return;
}

void NetIf::Process(const otSysMainloopContext &aContext)
{
    VerifyOrExit(mIfIndex > 0);

    if (FD_ISSET(mTunFd, &aContext.mErrorFdSet))
    {
        close(mTunFd);
        DieNow(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(mNetlinkFd, &aContext.mErrorFdSet))
    {
        close(mNetlinkFd);
        DieNow(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(mTunFd, &aContext.mReadFdSet))
    {
        ProcessTransmit();
    }

    if (FD_ISSET(mNetlinkFd, &aContext.mReadFdSet))
    {
        ProcessNetlinkEvent();
    }

exit:
    return;
}

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
