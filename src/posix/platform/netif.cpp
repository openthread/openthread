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

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <ifaddrs.h>
#ifdef __linux__
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif
#include <net/if.h>
#include <net/if_arp.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <openthread/icmp6.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>

#include "common/code_utils.hpp"
#include "common/logging.hpp"
#include "net/ip6_address.hpp"

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE

#ifndef OPENTHREAD_POSIX_TUN_DEVICE
#define OPENTHREAD_POSIX_TUN_DEVICE "/dev/net/tun"
#endif // OPENTHREAD_TUN_DEVICE

// Some platforms will include linux/ipv6.h in netinet/in.h
#if !defined(_IPV6_H) && !defined(_UAPI_IPV6_H)
// from linux/ipv6.h
struct in6_ifreq
{
    struct in6_addr ifr6_addr;
    __u32           ifr6_prefixlen;
    int             ifr6_ifindex;
};
#endif

static otInstance * sInstance     = NULL;
static int          sTunFd        = -1; ///< Used to exchange IPv6 packets.
static int          sIpFd         = -1; ///< Used to manage IPv6 stack on Thread interface.
static int          sNetlinkFd    = -1; ///< Used to receive netlink events.
static int          sMLDMonitorFd = -1; ///< Used to receive MLD events.
static unsigned int sTunIndex     = 0;
static char         sTunName[IFNAMSIZ];
// ff02::16
static const otIp6Address kMLDv2MulticastAddress = {
    {{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16}}};

OT_TOOL_PACKED_BEGIN
struct MLDv2Header
{
    uint8_t  mType;
    uint8_t  _rsv0;
    uint16_t mChecksum;
    uint16_t _rsv1;
    uint16_t mNumRecords;
} OT_TOOL_PACKED_END;

OT_TOOL_PACKED_BEGIN
struct MLDv2Record
{
    uint8_t         mRecordType;
    uint8_t         mAuxDataLen;
    uint16_t        mNumSources;
    struct in6_addr mMulticastAddress;
    struct in6_addr mSourceAddresses[];
} OT_TOOL_PACKED_END;

enum
{
    kICMPv6MLDv2Type                      = 143,
    kICMPv6MLDv2RecordChangeToExcludeType = 3,
    kICMPv6MLDv2RecordChangeToIncludeType = 4,
};

static const size_t kMaxIp6Size = 1536;

static void UpdateUnicast(otInstance *aInstance, const otIp6Address &aAddress, uint8_t aPrefixLength, bool aIsAdded)
{
    struct in6_ifreq ifr6;
    otError          error = OT_ERROR_NONE;

    assert(sInstance == aInstance);

    VerifyOrExit(sIpFd > 0, error = OT_ERROR_INVALID_STATE);

    memcpy(&ifr6.ifr6_addr, &aAddress, sizeof(ifr6.ifr6_addr));

    ifr6.ifr6_ifindex   = static_cast<int>(sTunIndex);
    ifr6.ifr6_prefixlen = aPrefixLength;

    if (aIsAdded)
    {
        VerifyOrDie(ioctl(sIpFd, SIOCSIFADDR, &ifr6) == 0, OT_EXIT_ERROR_ERRNO);
    }
    else
    {
        VerifyOrExit(ioctl(sIpFd, SIOCDIFADDR, &ifr6) == 0, perror("ioctl"); error = OT_ERROR_FAILED);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
    else
    {
        otLogInfoPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
}

static void UpdateMulticast(otInstance *aInstance, const otIp6Address &aAddress, bool aIsAdded)
{
    struct ipv6_mreq mreq;
    otError          error = OT_ERROR_NONE;

    assert(sInstance == aInstance);

    VerifyOrExit(sIpFd > 0);
    memcpy(&mreq.ipv6mr_multiaddr, &aAddress, sizeof(mreq.ipv6mr_multiaddr));
    mreq.ipv6mr_interface = sTunIndex;

    VerifyOrExit(
        setsockopt(sIpFd, IPPROTO_IPV6, (aIsAdded ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP), &mreq, sizeof(mreq)) == 0,
        perror("setsockopt");
        error = OT_ERROR_FAILED);

exit:
    SuccessOrDie(error);
    otLogInfoPlat("%s: %s", __func__, otThreadErrorToString(error));
}

static void UpdateLink(otInstance *aInstance)
{
    otError      error = OT_ERROR_NONE;
    struct ifreq ifr;

    assert(sInstance == aInstance);

    VerifyOrExit(sIpFd > 0);
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, sTunName, sizeof(ifr.ifr_name));
    VerifyOrExit(ioctl(sIpFd, SIOCGIFFLAGS, &ifr) == 0, perror("ioctl"); error = OT_ERROR_FAILED);

    if (otIp6IsEnabled(aInstance))
    {
        ifr.ifr_flags |= IFF_UP;
    }
    else
    {
        ifr.ifr_flags &= ~IFF_UP;
    }

    VerifyOrExit(ioctl(sIpFd, SIOCSIFFLAGS, &ifr) == 0, perror("ioctl"); error = OT_ERROR_FAILED);

exit:
    if (error == OT_ERROR_NONE)
    {
        otLogInfoPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
    else
    {
        otLogWarnPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
}

static void processAddressChange(const otIp6Address *aAddress, uint8_t aPrefixLength, bool aIsAdded, void *aContext)
{
    if (aAddress->mFields.m8[0] == 0xff)
    {
        UpdateMulticast(static_cast<otInstance *>(aContext), *aAddress, aIsAdded);
    }
    else
    {
        UpdateUnicast(static_cast<otInstance *>(aContext), *aAddress, aPrefixLength, aIsAdded);
    }
}

static void processStateChange(otChangedFlags aFlags, void *aContext)
{
    if (OT_CHANGED_THREAD_NETIF_STATE | aFlags)
    {
        UpdateLink(static_cast<otInstance *>(aContext));
    }
}

static void processReceive(otMessage *aMessage, void *aContext)
{
    char     packet[kMaxIp6Size];
    otError  error  = OT_ERROR_NONE;
    uint16_t length = otMessageGetLength(aMessage);

    assert(sInstance == aContext);

    VerifyOrExit(sTunFd > 0);

    VerifyOrExit(otMessageRead(aMessage, 0, packet, sizeof(packet)) == length, error = OT_ERROR_NO_BUFS);

    VerifyOrExit(write(sTunFd, packet, length) == length, perror("write"); error = OT_ERROR_FAILED);

exit:
    otMessageFree(aMessage);

    if (error == OT_ERROR_NONE)
    {
        otLogInfoPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
    else
    {
        otLogWarnPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
}

static void processTransmit(otInstance *aInstance)
{
    otMessage *message = NULL;
    ssize_t    rval;
    char       packet[kMaxIp6Size];
    otError    error = OT_ERROR_NONE;

    assert(sInstance == aInstance);

    rval = read(sTunFd, packet, sizeof(packet));
    VerifyOrExit(rval > 0, error = OT_ERROR_FAILED);

    message = otIp6NewMessage(aInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = otMessageAppend(message, packet, static_cast<uint16_t>(rval)));

    error   = otIp6Send(aInstance, message);
    message = NULL;

exit:
    if (message != NULL)
    {
        otMessageFree(message);
    }

    if (error == OT_ERROR_NONE)
    {
        otLogInfoPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
    else
    {
        otLogWarnPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
}

static void processNetifAddrEvent(otInstance *aInstance, struct nlmsghdr *aNetlinkMessage)
{
    struct ifaddrmsg *ifaddr = reinterpret_cast<struct ifaddrmsg *>(NLMSG_DATA(aNetlinkMessage));
    size_t            rtaLength;
    otError           error = OT_ERROR_NONE;

    VerifyOrExit(ifaddr->ifa_index == static_cast<unsigned int>(sTunIndex) && ifaddr->ifa_family == AF_INET6);

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
        {
            ot::Ip6::Address addr;
            memcpy(&addr, RTA_DATA(rta), sizeof(addr));

            if (aNetlinkMessage->nlmsg_type == RTM_NEWADDR)
            {
                if (!addr.IsMulticast())
                {
                    otNetifAddress netAddr;

                    netAddr.mAddress      = addr;
                    netAddr.mPrefixLength = ifaddr->ifa_prefixlen;
                    SuccessOrExit(error = otIp6AddUnicastAddress(aInstance, &netAddr));
                }
                else
                {
                    SuccessOrExit(error = otIp6SubscribeMulticastAddress(aInstance, &addr));
                }
            }
            else if (aNetlinkMessage->nlmsg_type == RTM_DELADDR)
            {
                if (!addr.IsMulticast())
                {
                    otIp6RemoveUnicastAddress(aInstance, &addr);
                }
                else
                {
                    otIp6UnsubscribeMulticastAddress(aInstance, &addr);
                }
            }
            else
            {
                continue;
            }
            break;
        }
        default:
            break;
        }
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        otLogInfoPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
    else
    {
        otLogWarnPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
}

static void processNetifLinkEvent(otInstance *aInstance, struct nlmsghdr *aNetlinkMessage)
{
    struct ifinfomsg *ifinfo = reinterpret_cast<struct ifinfomsg *>(NLMSG_DATA(aNetlinkMessage));
    otError           error  = OT_ERROR_NONE;

    VerifyOrExit(ifinfo->ifi_index == static_cast<int>(sTunIndex));
    SuccessOrExit(error = otIp6SetEnabled(aInstance, ifinfo->ifi_flags & IFF_UP));

exit:
    if (error == OT_ERROR_NONE)
    {
        otLogInfoPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
    else
    {
        otLogWarnPlat("%s: %s", __func__, otThreadErrorToString(error));
    }
}

static void processNetifEvent(otInstance *aInstance)
{
    const size_t kMaxNetifEvent = 8192;
    ssize_t      length;
    char         buffer[kMaxNetifEvent];

    length = recv(sNetlinkFd, buffer, sizeof(buffer), 0);

    VerifyOrExit(length > 0);

    for (struct nlmsghdr *msg = reinterpret_cast<struct nlmsghdr *>(buffer); NLMSG_OK(msg, length);
         msg                  = NLMSG_NEXT(msg, length))
    {
        switch (msg->nlmsg_type)
        {
        case RTM_NEWADDR:
        case RTM_DELADDR:
            processNetifAddrEvent(aInstance, msg);
            break;

        case RTM_NEWLINK:
        case RTM_DELLINK:
            processNetifLinkEvent(aInstance, msg);
            break;

        default:
            break;
        }
    }

exit:
    return;
}

void platformNetifDeinit(void)
{
    if (sTunFd != -1)
    {
        close(sTunFd);
        sTunFd = -1;
    }

    if (sIpFd != -1)
    {
        close(sIpFd);
        sIpFd = -1;
    }

    if (sNetlinkFd != -1)
    {
        close(sNetlinkFd);
        sNetlinkFd = -1;
    }

    if (sMLDMonitorFd != -1)
    {
        close(sMLDMonitorFd);
        sMLDMonitorFd = -1;
    }

    sTunIndex = 0;
}

static void mldListenerInit(void)
{
    struct ipv6_mreq mreq6;

    sMLDMonitorFd          = SocketWithCloseExec(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6, /*aNonBlock=*/true);
    mreq6.ipv6mr_interface = sTunIndex;
    memcpy(&mreq6.ipv6mr_multiaddr, kMLDv2MulticastAddress.mFields.m8, sizeof(kMLDv2MulticastAddress.mFields.m8));

    VerifyOrDie(setsockopt(sMLDMonitorFd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) == 0, OT_EXIT_FAILURE);
    VerifyOrDie(setsockopt(sMLDMonitorFd, SOL_SOCKET, SO_BINDTODEVICE, sTunName,
                           static_cast<socklen_t>(strnlen(sTunName, IFNAMSIZ))) == 0,
                OT_EXIT_FAILURE);
}

static void processMLDEvent(otInstance *aInstance)
{
    const size_t        kMaxMLDEvent = 8192;
    uint8_t             buffer[kMaxMLDEvent];
    ssize_t             bufferLen = -1;
    struct sockaddr_in6 srcAddr;
    socklen_t           addrLen  = sizeof(srcAddr);
    bool                fromSelf = false;
    MLDv2Header *       hdr      = reinterpret_cast<MLDv2Header *>(buffer);
    size_t              offset;
    uint8_t             type;
    struct ifaddrs *    ifAddrs = NULL;

    bufferLen = recvfrom(sMLDMonitorFd, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr *>(&srcAddr), &addrLen);
    VerifyOrExit(bufferLen > 0);

    type = buffer[0];
    VerifyOrExit(type == kICMPv6MLDv2Type && bufferLen >= static_cast<ssize_t>(sizeof(MLDv2Header)));

    // Check whether it is sent by self
    VerifyOrExit(getifaddrs(&ifAddrs) == 0);
    for (struct ifaddrs *ifAddr = ifAddrs; ifAddr != NULL; ifAddr = ifAddr->ifa_next)
    {
        if (ifAddr->ifa_addr != NULL && ifAddr->ifa_addr->sa_family == AF_INET6 &&
            strncmp(sTunName, ifAddr->ifa_name, IFNAMSIZ) == 0)
        {
            struct sockaddr_in6 *addr6 = reinterpret_cast<struct sockaddr_in6 *>(ifAddr->ifa_addr);

            if (memcmp(&addr6->sin6_addr, &srcAddr.sin6_addr, sizeof(in6_addr)) == 0)
            {
                fromSelf = true;
                break;
            }
        }
    }
    VerifyOrExit(fromSelf);

    hdr    = reinterpret_cast<MLDv2Header *>(buffer);
    offset = sizeof(MLDv2Header);

    for (size_t i = 0; i < ntohs(hdr->mNumRecords) && offset < static_cast<size_t>(bufferLen); i++)
    {
        if (static_cast<size_t>(bufferLen) >= (sizeof(MLDv2Record) + offset))
        {
            MLDv2Record *record = reinterpret_cast<MLDv2Record *>(&buffer[offset]);

            otError      err;
            otIp6Address address;
            char         addressString[INET6_ADDRSTRLEN + 1];

            memcpy(&address.mFields.m8, &record->mMulticastAddress, sizeof(address.mFields.m8));
            inet_ntop(AF_INET6, &record->mMulticastAddress, addressString, sizeof(addressString));
            if (record->mRecordType == kICMPv6MLDv2RecordChangeToIncludeType)
            {
                err = otIp6SubscribeMulticastAddress(aInstance, &address);
                if (err == OT_ERROR_ALREADY)
                {
                    otLogNotePlat(
                        "Will not subscribe duplicate multicast address %s",
                        inet_ntop(AF_INET6, &record->mMulticastAddress, addressString, sizeof(addressString)));
                }
                else if (err != OT_ERROR_NONE)
                {
                    otLogWarnPlat("Failed to subscribe multicast address %s: %s", addressString,
                                  otThreadErrorToString(err));
                }
                else
                {
                    otLogDebgPlat("Subscribed multicast address %s", addressString);
                }
            }
            else if (record->mRecordType == kICMPv6MLDv2RecordChangeToExcludeType)
            {
                err = otIp6UnsubscribeMulticastAddress(aInstance, &address);
                if (err != OT_ERROR_NONE)
                {
                    otLogWarnPlat("Failed to unsubscribe multicast address %s: %s", addressString,
                                  otThreadErrorToString(err));
                }
                else
                {
                    otLogDebgPlat("Unsubscribed multicast address %s", addressString);
                }
            }

            offset += sizeof(MLDv2Record) + sizeof(in6_addr) * ntohs(record->mNumSources);
        }
    }

exit:
    if (ifAddrs)
    {
        freeifaddrs(ifAddrs);
    }

    return;
}

void platformNetifInit(otInstance *aInstance, const char *aInterfaceName)
{
    struct ifreq ifr;

    sIpFd = SocketWithCloseExec(AF_INET6, SOCK_DGRAM, IPPROTO_IP, /*aNonBlock=*/true);
    VerifyOrDie(sIpFd >= 0, OT_EXIT_ERROR_ERRNO);

    sNetlinkFd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    VerifyOrDie(sNetlinkFd > 0, OT_EXIT_ERROR_ERRNO);

    otIcmp6SetEchoMode(aInstance, OT_ICMP6_ECHO_HANDLER_DISABLED);

    {
        struct sockaddr_nl sa;

        memset(&sa, 0, sizeof(sa));
        sa.nl_family = AF_NETLINK;
        sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV6_IFADDR;
        VerifyOrDie(bind(sNetlinkFd, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) == 0, OT_EXIT_ERROR_ERRNO);
    }

    sTunFd = open(OPENTHREAD_POSIX_TUN_DEVICE, O_RDWR | O_CLOEXEC | O_NONBLOCK);
    VerifyOrDie(sTunFd > 0, OT_EXIT_ERROR_ERRNO);

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if (aInterfaceName)
    {
        VerifyOrDie(strlen(aInterfaceName) < IFNAMSIZ, OT_EXIT_INVALID_ARGUMENTS);

        strncpy(ifr.ifr_name, aInterfaceName, IFNAMSIZ);
    }
    else
    {
        strncpy(ifr.ifr_name, "wpan%d", IFNAMSIZ);
    }

    VerifyOrDie(ioctl(sTunFd, TUNSETIFF, static_cast<void *>(&ifr)) == 0, OT_EXIT_ERROR_ERRNO);
    VerifyOrDie(ioctl(sTunFd, TUNSETLINK, ARPHRD_VOID) == 0, OT_EXIT_ERROR_ERRNO);

    sTunIndex = if_nametoindex(ifr.ifr_name);
    VerifyOrDie(sTunIndex > 0, OT_EXIT_FAILURE);

    strncpy(sTunName, ifr.ifr_name, sizeof(sTunName));
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    platformUdpInit(sTunName);
#endif
    mldListenerInit();

    otIp6SetReceiveCallback(aInstance, processReceive, aInstance);
    otIp6SetAddressCallback(aInstance, processAddressChange, aInstance);
    otSetStateChangedCallback(aInstance, processStateChange, aInstance);
    sInstance = aInstance;
}

void platformNetifUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int *aMaxFd)
{
    OT_UNUSED_VARIABLE(aWriteFdSet);

    VerifyOrExit(sTunIndex > 0);

    assert(sTunFd > 0);
    assert(sNetlinkFd > 0);
    assert(sIpFd > 0);

    FD_SET(sTunFd, aReadFdSet);
    FD_SET(sTunFd, aErrorFdSet);
    FD_SET(sNetlinkFd, aReadFdSet);
    FD_SET(sNetlinkFd, aErrorFdSet);
    FD_SET(sMLDMonitorFd, aReadFdSet);
    FD_SET(sMLDMonitorFd, aErrorFdSet);

    if (sTunFd > *aMaxFd)
    {
        *aMaxFd = sTunFd;
    }

    if (sNetlinkFd > *aMaxFd)
    {
        *aMaxFd = sNetlinkFd;
    }

    if (sMLDMonitorFd > *aMaxFd)
    {
        *aMaxFd = sMLDMonitorFd;
    }
exit:
    return;
}

void platformNetifProcess(const fd_set *aReadFdSet, const fd_set *aWriteFdSet, const fd_set *aErrorFdSet)
{
    OT_UNUSED_VARIABLE(aWriteFdSet);
    VerifyOrExit(sTunIndex > 0);

    if (FD_ISSET(sTunFd, aErrorFdSet))
    {
        close(sTunFd);
        DieNow(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(sNetlinkFd, aErrorFdSet))
    {
        close(sNetlinkFd);
        DieNow(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(sMLDMonitorFd, aErrorFdSet))
    {
        close(sNetlinkFd);
        DieNow(OT_EXIT_FAILURE);
    }

    if (FD_ISSET(sTunFd, aReadFdSet))
    {
        processTransmit(sInstance);
    }

    if (FD_ISSET(sNetlinkFd, aReadFdSet))
    {
        processNetifEvent(sInstance);
    }

    if (FD_ISSET(sMLDMonitorFd, aReadFdSet))
    {
        processMLDEvent(sInstance);
    }

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
