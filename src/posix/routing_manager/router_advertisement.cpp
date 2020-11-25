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
 *   This file includes implementations for IPv6 Router Advertisement.
 *
 */

#define __APPLE_USE_RFC_3542

#include "router_advertisement.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <errno.h>
#include <netinet/icmp6.h>
#include <unistd.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"
#include "platform/address_utils.hpp"

namespace ot {

namespace Posix {

namespace RouterAdvertisement {

const PrefixInfoOption *RouterAdvMessage::GetNextPrefixInfo(const PrefixInfoOption *aCurPrefixInfo) const
{
    const Option *nextOption = aCurPrefixInfo;

    while ((nextOption = GetNextOption(nextOption)) != nullptr)
    {
        if (nextOption->GetType() == ND_OPT_PREFIX_INFORMATION)
        {
            break;
        }
    }

    return reinterpret_cast<const PrefixInfoOption *>(nextOption);
}

const Option *RouterAdvMessage::GetNextOption(const Option *aCurOption) const
{
    const Option *nextOption = nullptr;

    if (aCurOption == nullptr)
    {
        nextOption = reinterpret_cast<const Option *>(mOptions);
    }
    else
    {
        nextOption = aCurOption->GetNextOption();
    }

    VerifyOrExit(reinterpret_cast<const uint8_t *>(nextOption) >= mOptions, nextOption = nullptr);
    VerifyOrExit(reinterpret_cast<const uint8_t *>(nextOption) + sizeof(nextOption) <=
                     reinterpret_cast<const uint8_t *>(this) + GetTotalLength(),
                 nextOption = nullptr);
    VerifyOrExit(reinterpret_cast<const uint8_t *>(nextOption->GetNextOption()) <=
                     reinterpret_cast<const uint8_t *>(this) + GetTotalLength(),
                 nextOption = nullptr);

exit:
    return nextOption;
}

RouterAdvertiser::RouterAdvertiser(RouterSolicitHandler       aRouterSolicitHandler,
                                   void *                     aRouterSolicitHandlerContext,
                                   RouterAdvertisementHandler aRouterAdvertisementHandler,
                                   void *                     aRouterAdvertisementHandlerContext)
    : mSocketFd(-1)
    , mRouterSolicitHandler(aRouterSolicitHandler)
    , mRouterSolicitHandlerContext(aRouterSolicitHandlerContext)
    , mRouterAdvertisementHandler(aRouterAdvertisementHandler)
    , mRouterAdvertisementHandlerContext(aRouterAdvertisementHandlerContext)
{
}

void RouterAdvertiser::Init()
{
    otError             error = OT_ERROR_FAILED;
    int                 sock;
    struct icmp6_filter filter;
    ssize_t             rval;
    static const int    kTrue  = 1;
    static const int    kFalse = 0;

    sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (sock < 0)
    {
        otLogWarnPlat("failed to open ICMPv6 socket: %s", strerror(errno));
        ExitNow();
    }

    // Only accept router advertisements and router solicits.
    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filter);

    rval = setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof(filter));
    if (rval < 0)
    {
        otLogWarnPlat("Can't set ICMP6_FILTER: %s", strerror(errno));
        ExitNow();
    }

    // We want a source address and interface index.
    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &kTrue, sizeof(kTrue));
    if (rval < 0)
    {
        otLogWarnPlat("Can't set IPV6_RECVPKTINFO: %s", strerror(errno));
        ExitNow();
    }

    // We need to be able to reject RAs arriving from off-link.
    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &kTrue, sizeof(kTrue));
    if (rval < 0)
    {
        otLogWarnPlat("Can't set IPV6_RECVHOPLIMIT: %s", strerror(errno));
        ExitNow();
    }

    // Prevent our router advertisements from updating our routing table.
    rval = setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &kFalse, sizeof(kFalse));
    if (rval < 0)
    {
        otLogWarnPlat("Can't set IPV6_MULTICAST_LOOP: %s", strerror(errno));
        ExitNow();
    }

    mSocketFd = sock;
    error     = OT_ERROR_NONE;

exit:
    if (error != OT_ERROR_NONE)
    {
        if (sock >= 0)
        {
            close(sock);
        }
    }
    return;
}

void RouterAdvertiser::Deinit()
{
    if (mSocketFd >= 0)
    {
        close(mSocketFd);
        mSocketFd = -1;
    }
}

void RouterAdvertiser::Update(otSysMainloopContext *aMainloop)
{
    VerifyOrExit(mSocketFd >= 0);

    FD_SET(mSocketFd, &aMainloop->mReadFdSet);
    FD_SET(mSocketFd, &aMainloop->mErrorFdSet);

    if (mSocketFd > aMainloop->mMaxFd)
    {
        aMainloop->mMaxFd = mSocketFd;
    }

exit:
    return;
}

void RouterAdvertiser::Process(const otSysMainloopContext *aMainloop)
{
    VerifyOrExit(mSocketFd >= 0);

    if (FD_ISSET(mSocketFd, &aMainloop->mReadFdSet))
    {
        Recv();
    }

    if (FD_ISSET(mSocketFd, &aMainloop->mErrorFdSet))
    {
        otLogWarnPlat("ICMP socket errored");
    }

exit:
    return;
}

void RouterAdvertiser::SendRouterAdvertisement(const otIp6Prefix *    aOmrPrefix,
                                               const otIp6Prefix *    aOnLinkPrefix,
                                               const InfraNetif &     aInfraNetif,
                                               const struct in6_addr &aDest)
{
    MessageBuffer    messageBuffer;
    RouterAdvMessage advMessage;

    messageBuffer.Append(advMessage);

    if (aOnLinkPrefix)
    {
        PrefixInfoOption pio;

        otLogInfoPlat("send Router Advertisement with PIO %s", Ip6PrefixString(*aOnLinkPrefix).AsCString());

        pio.SetOnLink(true);
        pio.SetAutoAddrConfig(true);
        pio.SetValidLifetime(1800);     // 30 Minutes.
        pio.SetPreferredLifetime(1800); // 30 Minutes.
        pio.SetPrefix(*aOnLinkPrefix);

        messageBuffer.Append(pio);

        OT_ASSERT(pio.GetLength() == sizeof(pio));
    }

    if (aOmrPrefix)
    {
        RouteInfoOption rio;

        otLogInfoPlat("send Router Advertisement with RIO %s", Ip6PrefixString(*aOmrPrefix).AsCString());

        rio.SetRouteLifetime(1800); // 30 Minutes.
        rio.SetPrefix(*aOmrPrefix);

        messageBuffer.Append(rio);
    }

    if (Send(messageBuffer, aInfraNetif, aDest) != OT_ERROR_NONE)
    {
        otLogWarnPlat("failed to send Router Advertisement on interface %s", aInfraNetif.GetName());
    }
}

void RouterAdvertiser::SendRouterSolicit(const InfraNetif &aInfraNetif, const struct in6_addr &aDest)
{
    MessageBuffer messageBuffer;
    otIcmp6Header header;

    memset(&header, 0, sizeof(header));
    header.mType = ND_ROUTER_SOLICIT;

    messageBuffer.Append(&header, sizeof(header));

    if (Send(messageBuffer, aInfraNetif, aDest) != OT_ERROR_NONE)
    {
        otLogWarnPlat("failed to send Router Solicitation on interface %s", aInfraNetif.GetName());
    }
}

otError RouterAdvertiser::Send(MessageBuffer &        aMessageBuffer,
                               const InfraNetif &     aInfraNetif,
                               const struct in6_addr &aDest)
{
    otError error = OT_ERROR_FAILED;

    struct iovec        iov;
    struct in6_pktinfo *packetInfo;
    uint8_t             cmsgBuffer[CMSG_SPACE(sizeof(*packetInfo)) + CMSG_SPACE(sizeof(int))];
    struct msghdr       msgHeader;
    struct cmsghdr *    cmsgPointer;
    static const int    kHopLimit = 255;
    ssize_t             rval;
    struct sockaddr_in6 dest;

    VerifyOrExit(mSocketFd >= 0);

    // This is critical to get CMSG_NXTHDR work.
    memset(cmsgBuffer, 0, sizeof(cmsgBuffer));

    // Send the message
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family   = AF_INET6;
    dest.sin6_scope_id = aInfraNetif.GetIndex();
    dest.sin6_addr     = aDest;

    iov.iov_base = aMessageBuffer.GetBegin();
    iov.iov_len  = aMessageBuffer.GetLength();

    msgHeader.msg_namelen    = sizeof(dest);
    msgHeader.msg_name       = &dest;
    msgHeader.msg_iov        = &iov;
    msgHeader.msg_iovlen     = 1;
    msgHeader.msg_control    = cmsgBuffer;
    msgHeader.msg_controllen = sizeof(cmsgBuffer);

    // Specify the interface
    cmsgPointer             = CMSG_FIRSTHDR(&msgHeader);
    cmsgPointer->cmsg_level = IPPROTO_IPV6;
    cmsgPointer->cmsg_type  = IPV6_PKTINFO;
    cmsgPointer->cmsg_len   = CMSG_LEN(sizeof(*packetInfo));
    packetInfo              = (struct in6_pktinfo *)CMSG_DATA(cmsgPointer);
    memset(packetInfo, 0, sizeof(*packetInfo));
    packetInfo->ipi6_ifindex = aInfraNetif.GetIndex();

    // Router advertisements and solicitations have a hop limit of 255
    cmsgPointer             = CMSG_NXTHDR(&msgHeader, cmsgPointer);
    cmsgPointer->cmsg_level = IPPROTO_IPV6;
    cmsgPointer->cmsg_type  = IPV6_HOPLIMIT;
    cmsgPointer->cmsg_len   = CMSG_LEN(sizeof(kHopLimit));
    // FIXME(wgtdkp): byte order?
    memcpy(CMSG_DATA(cmsgPointer), &kHopLimit, sizeof(kHopLimit));

    rval = sendmsg(mSocketFd, &msgHeader, 0);
    if (rval < 0)
    {
        otLogWarnPlat("send ICMP message failed: %s", strerror(errno));
        ExitNow();
    }

    if (static_cast<size_t>(rval) != iov.iov_len)
    {
        otLogWarnPlat("send ICMP message failed: partially sent");
        ExitNow();
    }

    error = OT_ERROR_NONE;

exit:
    return error;
}

void RouterAdvertiser::Recv()
{
    MessageBuffer buffer;

    ssize_t         rval;
    struct msghdr   msg;
    struct iovec    bufp;
    char            cmsgbuf[128];
    struct cmsghdr *cmh;
    unsigned int    ifIndex  = 0;
    int             hopLimit = -1;

    struct sockaddr_in6 srcAddr;
    struct in6_addr     dstAddr;

    memset(&srcAddr, 0, sizeof(srcAddr));
    memset(&dstAddr, 0, sizeof(dstAddr));

    bufp.iov_base      = buffer.GetBegin();
    bufp.iov_len       = buffer.kMaxLength;
    msg.msg_iov        = &bufp;
    msg.msg_iovlen     = 1;
    msg.msg_name       = &srcAddr;
    msg.msg_namelen    = sizeof(srcAddr);
    msg.msg_control    = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    VerifyOrExit(mSocketFd >= 0);

    rval = recvmsg(mSocketFd, &msg, 0);
    if (rval < 0)
    {
        otLogWarnPlat("failed to recv ICMP message: %s", strerror(errno));
        ExitNow();
    }

    buffer.SetLength(static_cast<uint16_t>(rval));

    for (cmh = CMSG_FIRSTHDR(&msg); cmh; cmh = CMSG_NXTHDR(&msg, cmh))
    {
        if (cmh->cmsg_level == IPPROTO_IPV6 && cmh->cmsg_type == IPV6_PKTINFO &&
            cmh->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo)))
        {
            struct in6_pktinfo pktinfo;

            memcpy(&pktinfo, CMSG_DATA(cmh), sizeof pktinfo);
            ifIndex = pktinfo.ipi6_ifindex;
            dstAddr = pktinfo.ipi6_addr;
        }
        else if (cmh->cmsg_level == IPPROTO_IPV6 && cmh->cmsg_type == IPV6_HOPLIMIT &&
                 cmh->cmsg_len == CMSG_LEN(sizeof(int)))
        {
            hopLimit = *(int *)CMSG_DATA(cmh);
            OT_UNUSED_VARIABLE(hopLimit);
        }
    }

    otLogInfoPlat("received ICMPv6 message at interface index %u", ifIndex);

    if (buffer.GetLength() < sizeof(otIcmp6Header))
    {
        otLogInfoPlat("drop too short ICMPv6 message (len=%hu)", buffer.GetLength());
        ExitNow();
    }

    switch (reinterpret_cast<otIcmp6Header *>(buffer.GetBegin())->mType) // type
    {
    case ND_ROUTER_SOLICIT:
        HandleRouterSolicit(ifIndex, buffer, srcAddr.sin6_addr, dstAddr);
        break;
    case ND_ROUTER_ADVERT:
        HandleRouterAdvertisement(ifIndex, buffer, srcAddr.sin6_addr, dstAddr);
        break;
    }

exit:
    return;
}

void RouterAdvertiser::HandleRouterSolicit(unsigned int           aIfIndex,
                                           const MessageBuffer &  aBuffer,
                                           const struct in6_addr &aSrcAddr,
                                           const struct in6_addr &aDstAddr)
{
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aSrcAddr);
    OT_UNUSED_VARIABLE(aDstAddr);

    otLogInfoPlat("received Router Solicit message from %s", Ip6AddressString(aSrcAddr).AsCString());

    if (mRouterSolicitHandler)
    {
        mRouterSolicitHandler(aIfIndex, mRouterSolicitHandlerContext);
    }

    return;
}

void RouterAdvertiser::HandleRouterAdvertisement(unsigned int           aIfIndex,
                                                 const MessageBuffer &  aBuffer,
                                                 const struct in6_addr &aSrcAddr,
                                                 const struct in6_addr &aDstAddr)
{
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aSrcAddr);
    OT_UNUSED_VARIABLE(aDstAddr);
    const RouterAdvMessage *routerAdv;

    otLogInfoPlat("received Router Advertisement message from %s", Ip6AddressString(aSrcAddr).AsCString());

    if (aBuffer.GetTotalLength() < sizeof(RouterAdvMessage))
    {
        otLogInfoPlat("drop too short Router Advertisement message (len=%hu)", aBuffer.GetLength());
        ExitNow();
    }

    routerAdv = reinterpret_cast<const RouterAdvMessage *>(&aBuffer);

    if (mRouterAdvertisementHandler)
    {
        mRouterAdvertisementHandler(*routerAdv, aIfIndex, mRouterAdvertisementHandlerContext);
    }

exit:
    return;
}

} // namespace RouterAdvertisement

} // namespace Posix

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
