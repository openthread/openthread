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
#include "posix/platform/netif_manager.hpp"

#if defined(__linux__)

#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <string.h>

#include "posix/platform/ip6_utils.hpp"
#include "posix/platform/netlink_manager.hpp"

namespace ot {
namespace Posix {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"

namespace {
uint32_t sNetlinkSequence = 0; ///< Netlink message sequence.
}

void NetifManager::UpdateUnicast(unsigned int aNetifIndex, const otIp6AddressInfo &aAddressInfo, bool aToAdd)
{
    struct rtattr *rta;

    struct
    {
        struct nlmsghdr  nh;
        struct ifaddrmsg ifa;
        char             buf[512];
    } req;

    memset(&req, 0, sizeof(req));

    req.nh.nlmsg_len   = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL;
    req.nh.nlmsg_type  = aToAdd ? RTM_NEWADDR : RTM_DELADDR;
    req.nh.nlmsg_pid   = 0;
    req.nh.nlmsg_seq   = ++sNetlinkSequence;

    req.ifa.ifa_family    = AF_INET6;
    req.ifa.ifa_prefixlen = aAddressInfo.mPrefixLength;
    req.ifa.ifa_flags     = IFA_F_NODAD;
    req.ifa.ifa_scope     = aAddressInfo.mScope;
    req.ifa.ifa_index     = aNetifIndex;

    rta           = reinterpret_cast<struct rtattr *>((reinterpret_cast<char *>(&req)) + NLMSG_ALIGN(req.nh.nlmsg_len));
    rta->rta_type = IFA_LOCAL;
    rta->rta_len  = RTA_LENGTH(sizeof(*aAddressInfo.mAddress));

    memcpy(RTA_DATA(rta), aAddressInfo.mAddress, sizeof(*aAddressInfo.mAddress));

    req.nh.nlmsg_len = NLMSG_ALIGN(req.nh.nlmsg_len) + rta->rta_len;

    if (!aAddressInfo.mPreferred)
    {
        struct ifa_cacheinfo cacheinfo;

        rta           = reinterpret_cast<struct rtattr *>((reinterpret_cast<char *>(rta)) + rta->rta_len);
        rta->rta_type = IFA_CACHEINFO;
        rta->rta_len  = RTA_LENGTH(sizeof(cacheinfo));

        memset(&cacheinfo, 0, sizeof(cacheinfo));
        cacheinfo.ifa_valid = UINT32_MAX;

        memcpy(RTA_DATA(rta), &cacheinfo, sizeof(cacheinfo));

        req.nh.nlmsg_len += rta->rta_len;
    }

    if (send(NetlinkManager::Get().GetFd(), &req, req.nh.nlmsg_len, 0) != -1)
    {
        otLogInfoPlat("Sent request#%u to %s %s/%u", sNetlinkSequence, (aToAdd ? "add" : "remove"),
                      Ip6Utils::Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength);
    }
    else
    {
        otLogInfoPlat("Failed to send request#%u to %s %s/%u", sNetlinkSequence, (aToAdd ? "add" : "remove"),
                      Ip6Utils::Ip6AddressString(aAddressInfo.mAddress).AsCString(), aAddressInfo.mPrefixLength);
    }
}

#pragma GCC diagnostic pop

} // namespace Posix
} // namespace ot

#endif // defined(__linux__)
