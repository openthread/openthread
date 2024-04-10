/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "platform-simulation.h"

#include <openthread/nat64.h>
#include <openthread/platform/mdns_socket.h>

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

//---------------------------------------------------------------------------------------------------------------------
#if OPENTHREAD_SIMULATION_MDNS_SOCKET_IMPLEMENT_POSIX

// Provide a simplified POSIX based implementation of `otPlatMdns`
// platform APIs. This is intended for testing.

#include <openthread/ip6.h>

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "simul_utils.h"
#include "utils/code_utils.h"

#define MAX_BUFFER_SIZE 1600

#define MDNS_PORT 5353

static bool     sEnabled = false;
static uint32_t sInfraIfIndex;
static int      sMdnsFd4 = -1;
static int      sMdnsFd6 = -1;

/* this is a portability hack */
#ifndef IPV6_ADD_MEMBERSHIP
#ifdef IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif
#endif

#ifndef IPV6_DROP_MEMBERSHIP
#ifdef IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#endif

#define VerifyOrDie(aCondition, aErrMsg)                                        \
    do                                                                          \
    {                                                                           \
        if (!(aCondition))                                                      \
        {                                                                       \
            fprintf(stderr, "\n\r" aErrMsg ". errono:%s\n\r", strerror(errno)); \
            exit(1);                                                            \
        }                                                                       \
    } while (false)

static void SetReuseAddrPort(int aFd)
{
    int ret;
    int yes = 1;

    ret = setsockopt(aFd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));
    VerifyOrDie(ret >= 0, "setsocketopt(SO_REUSEADDR) failed");

    ret = setsockopt(aFd, SOL_SOCKET, SO_REUSEPORT, (char *)&yes, sizeof(yes));
    VerifyOrDie(ret >= 0, "setsocketopt(SO_REUSEPORT) failed");
}

static void OpenIp4Socket(uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    struct sockaddr_in addr;
    int                fd;
    int                ret;
    uint8_t            u8;
    int                value;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    VerifyOrDie(fd >= 0, "socket() failed");

#ifdef __linux__
    {
        char        nameBuffer[IF_NAMESIZE];
        const char *ifname;

        ifname = if_indextoname(aInfraIfIndex, nameBuffer);
        VerifyOrDie(ifname != NULL, "if_indextoname() failed");

        ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname));
        VerifyOrDie(ret >= 0, "setsocketopt(SO_BINDTODEVICE) failed");
    }
#else
    value = aInfraIfIndex;
    ret   = setsockopt(fd, IPPROTO_IP, IP_BOUND_IF, &value, sizeof(value));
#endif

    u8  = 255;
    ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &u8, sizeof(u8));
    VerifyOrDie(ret >= 0, "setsocketopt(IP_MULTICAST_TTL) failed");

    value = 255;
    ret   = setsockopt(fd, IPPROTO_IP, IP_TTL, &value, sizeof(value));
    VerifyOrDie(ret >= 0, "setsocketopt(IP_TTL) failed");

    u8  = 1;
    ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &u8, sizeof(u8));
    VerifyOrDie(ret >= 0, "setsocketopt(IP_MULTICAST_LOOP) failed");

    SetReuseAddrPort(fd);

    {
        struct ip_mreqn mreqn;

        memset(&mreqn, 0, sizeof(mreqn));
        mreqn.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
        mreqn.imr_ifindex          = aInfraIfIndex;

        ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreqn, sizeof(mreqn));
        VerifyOrDie(ret >= 0, "setsocketopt(IP_MULTICAST_IF) failed");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(MDNS_PORT);

    ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    VerifyOrDie(ret >= 0, "bind() failed");

    sMdnsFd4 = fd;
}

static void JoinOrLeaveIp4MulticastGroup(bool aJoin, uint32_t aInfraIfIndex)
{
    struct ip_mreqn mreqn;
    int             ret;

    memset(&mreqn, 0, sizeof(mreqn));
    mreqn.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
    mreqn.imr_ifindex          = aInfraIfIndex;

    if (aJoin)
    {
        // Suggested workaround for netif not dropping
        // a previous multicast membership.
        setsockopt(sMdnsFd4, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreqn, sizeof(mreqn));
    }

    ret = setsockopt(sMdnsFd4, IPPROTO_IP, aJoin ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, &mreqn, sizeof(mreqn));
    VerifyOrDie(ret >= 0, "setsocketopt(IP_ADD/DROP_MEMBERSHIP) failed");
}

static void OpenIp6Socket(uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    struct sockaddr_in6 addr6;
    int                 fd;
    int                 ret;
    int                 value;

    fd = socket(AF_INET6, SOCK_DGRAM, 0);
    VerifyOrDie(fd >= 0, "socket() failed");

#ifdef __linux__
    {
        char        nameBuffer[IF_NAMESIZE];
        const char *ifname;

        ifname = if_indextoname(aInfraIfIndex, nameBuffer);
        VerifyOrDie(ifname != NULL, "if_indextoname() failed");

        ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname));
        VerifyOrDie(ret >= 0, "setsocketopt(SO_BINDTODEVICE) failed");
    }
#else
    value = aInfraIfIndex;
    ret   = setsockopt(fd, IPPROTO_IPV6, IPV6_BOUND_IF, &value, sizeof(value));
#endif

    value = 255;
    ret   = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &value, sizeof(value));
    VerifyOrDie(ret >= 0, "setsocketopt(IPV6_MULTICAST_HOPS) failed");

    value = 255;
    ret   = setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &value, sizeof(value));
    VerifyOrDie(ret >= 0, "setsocketopt(IPV6_UNICAST_HOPS) failed");

    value = 1;
    ret   = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof(value));
    VerifyOrDie(ret >= 0, "setsocketopt(IPV6_V6ONLY) failed");

    value = aInfraIfIndex;
    ret   = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &value, sizeof(value));
    VerifyOrDie(ret >= 0, "setsocketopt(IPV6_MULTICAST_IF) failed");

    value = 1;
    ret   = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &value, sizeof(value));
    VerifyOrDie(ret >= 0, "setsocketopt(IPV6_MULTICAST_LOOP) failed");

    SetReuseAddrPort(fd);

    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port   = htons(MDNS_PORT);

    ret = bind(fd, (struct sockaddr *)&addr6, sizeof(addr6));
    VerifyOrDie(ret >= 0, "bind() failed");

    sMdnsFd6 = fd;
}

static void JoinOrLeaveIp6MulticastGroup(bool aJoin, uint32_t aInfraIfIndex)
{
    struct ipv6_mreq mreq6;
    int              ret;

    memset(&mreq6, 0, sizeof(mreq6));

    inet_pton(AF_INET6, "ff02::fb", &mreq6.ipv6mr_multiaddr);
    mreq6.ipv6mr_interface = (int)aInfraIfIndex;

    if (aJoin)
    {
        // Suggested workaround for netif not dropping
        // a previous multicast membership.
        setsockopt(sMdnsFd6, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq6, sizeof(mreq6));
    }

    ret = setsockopt(sMdnsFd6, IPPROTO_IPV6, aJoin ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP, &mreq6, sizeof(mreq6));
    VerifyOrDie(ret >= 0, "setsocketopt(IP6_ADD/DROP_MEMBERSHIP) failed");
}

otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (aEnable)
    {
        otEXPECT(!sEnabled);

        OpenIp4Socket(aInfraIfIndex);
        JoinOrLeaveIp4MulticastGroup(/* aJoin */ true, aInfraIfIndex);
        OpenIp6Socket(aInfraIfIndex);
        JoinOrLeaveIp6MulticastGroup(/* aJoin */ true, aInfraIfIndex);

        sEnabled      = true;
        sInfraIfIndex = aInfraIfIndex;
    }
    else
    {
        otEXPECT(sEnabled);

        JoinOrLeaveIp4MulticastGroup(/* aJoin */ false, aInfraIfIndex);
        JoinOrLeaveIp6MulticastGroup(/* aJoin */ false, aInfraIfIndex);
        close(sMdnsFd4);
        close(sMdnsFd6);
        sEnabled = false;
    }

exit:
    return OT_ERROR_NONE;
}

void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    uint8_t  buffer[MAX_BUFFER_SIZE];
    uint16_t length;
    int      bytes;

    otEXPECT(sEnabled);

    length = otMessageRead(aMessage, 0, buffer, sizeof(buffer));
    otMessageFree(aMessage);

    {
        struct sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = inet_addr("224.0.0.251");
        addr.sin_port        = htons(MDNS_PORT);

        bytes = sendto(sMdnsFd4, buffer, length, 0, (struct sockaddr *)&addr, sizeof(addr));

        VerifyOrDie((bytes == length), "sendTo(sMdnsFd4) failed");
    }

    {
        struct sockaddr_in6 addr6;

        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port   = htons(MDNS_PORT);
        inet_pton(AF_INET6, "ff02::fb", &addr6.sin6_addr);

        bytes = sendto(sMdnsFd6, buffer, length, 0, (struct sockaddr *)&addr6, sizeof(addr6));

        VerifyOrDie((bytes == length), "sendTo(sMdnsFd6) failed");
    }

exit:
    return;
}

void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otIp4Address ip4Addr;
    uint8_t      buffer[MAX_BUFFER_SIZE];
    uint16_t     length;
    int          bytes;

    otEXPECT(sEnabled);

    length = otMessageRead(aMessage, 0, buffer, sizeof(buffer));
    otMessageFree(aMessage);

    if (otIp4FromIp4MappedIp6Address(&aAddress->mAddress, &ip4Addr) == OT_ERROR_NONE)
    {
        struct sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        memcpy(&addr.sin_addr.s_addr, &ip4Addr, sizeof(otIp4Address));
        addr.sin_port = htons(MDNS_PORT);

        bytes = sendto(sMdnsFd4, buffer, length, 0, (struct sockaddr *)&addr, sizeof(addr));

        VerifyOrDie((bytes == length), "sendTo(sMdnsFd4) failed");
    }
    else
    {
        struct sockaddr_in6 addr6;

        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port   = htons(MDNS_PORT);
        memcpy(&addr6.sin6_addr, &aAddress->mAddress, sizeof(otIp6Address));

        bytes = sendto(sMdnsFd6, buffer, length, 0, (struct sockaddr *)&addr6, sizeof(addr6));

        VerifyOrDie((bytes == length), "sendTo(sMdnsFd6) failed");
    }

exit:
    return;
}

void platformMdnsSocketUpdateFdSet(fd_set *aReadFdSet, int *aMaxFd)
{
    otEXPECT(sEnabled);

    utilsAddFdToFdSet(sMdnsFd4, aReadFdSet, aMaxFd);
    utilsAddFdToFdSet(sMdnsFd6, aReadFdSet, aMaxFd);

exit:
    return;
}

void platformMdnsSocketProcess(otInstance *aInstance, const fd_set *aReadFdSet)
{
    otEXPECT(sEnabled);

    if (FD_ISSET(sMdnsFd4, aReadFdSet))
    {
        uint8_t               buffer[MAX_BUFFER_SIZE];
        struct sockaddr_in    sockaddr;
        otPlatMdnsAddressInfo addrInfo;
        otMessage            *message;
        socklen_t             len = sizeof(sockaddr);
        ssize_t               rval;

        memset(&sockaddr, 0, sizeof(sockaddr));
        rval = recvfrom(sMdnsFd4, (char *)&buffer, sizeof(buffer), 0, (struct sockaddr *)&sockaddr, &len);

        VerifyOrDie(rval >= 0, "recvfrom() failed");

        message = otIp6NewMessage(aInstance, NULL);
        VerifyOrDie(message != NULL, "otIp6NewMessage() failed");

        VerifyOrDie(otMessageAppend(message, buffer, (uint16_t)rval) == OT_ERROR_NONE, "otMessageAppend() failed");

        memset(&addrInfo, 0, sizeof(addrInfo));
        otIp4ToIp4MappedIp6Address((otIp4Address *)(&sockaddr.sin_addr.s_addr), &addrInfo.mAddress);
        addrInfo.mPort         = MDNS_PORT;
        addrInfo.mInfraIfIndex = sInfraIfIndex;

        otPlatMdnsHandleReceive(aInstance, message, /* aInUnicast */ false, &addrInfo);
    }

    if (FD_ISSET(sMdnsFd6, aReadFdSet))
    {
        uint8_t               buffer[MAX_BUFFER_SIZE];
        struct sockaddr_in6   sockaddr6;
        otPlatMdnsAddressInfo addrInfo;
        otMessage            *message;
        socklen_t             len = sizeof(sockaddr6);
        ssize_t               rval;

        memset(&sockaddr6, 0, sizeof(sockaddr6));
        rval = recvfrom(sMdnsFd6, (char *)&buffer, sizeof(buffer), 0, (struct sockaddr *)&sockaddr6, &len);
        VerifyOrDie(rval >= 0, "recvfrom(sMdnsFd6) failed");

        message = otIp6NewMessage(aInstance, NULL);
        VerifyOrDie(message != NULL, "otIp6NewMessage() failed");

        VerifyOrDie(otMessageAppend(message, buffer, (uint16_t)rval) == OT_ERROR_NONE, "otMessageAppend() failed");

        memset(&addrInfo, 0, sizeof(addrInfo));
        memcpy(&addrInfo.mAddress, &sockaddr6.sin6_addr, sizeof(otIp6Address));
        addrInfo.mPort         = MDNS_PORT;
        addrInfo.mInfraIfIndex = sInfraIfIndex;

        otPlatMdnsHandleReceive(aInstance, message, /* aInUnicast */ false, &addrInfo);
    }

exit:
    return;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Add weak implementation of `ot` APIs for RCP build. Note that
// `simulation` platform does not get `OPENTHREAD_RADIO` config)

OT_TOOL_WEAK uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aOffset);
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aLength);

    fprintf(stderr, "\n\rWeak otMessageRead() is incorrectly used\n\r");
    exit(1);

    return 0;
}

OT_TOOL_WEAK void otMessageFree(otMessage *aMessage)
{
    OT_UNUSED_VARIABLE(aMessage);
    fprintf(stderr, "\n\rWeak otMessageFree() is incorrectly used\n\r");
    exit(1);
}

OT_TOOL_WEAK otMessage *otIp6NewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aSettings);

    fprintf(stderr, "\n\rWeak otIp6NewMessage() is incorrectly used\n\r");
    exit(1);

    return NULL;
}

OT_TOOL_WEAK otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aLength);

    fprintf(stderr, "\n\rWeak otMessageFree() is incorrectly used\n\r");
    exit(1);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otIp4ToIp4MappedIp6Address(const otIp4Address *aIp4Address, otIp6Address *aIp6Address)
{
    OT_UNUSED_VARIABLE(aIp4Address);
    OT_UNUSED_VARIABLE(aIp6Address);

    fprintf(stderr, "\n\rWeak otIp4ToIp4MappedIp6Address() is incorrectly used\n\r");
    exit(1);
}

OT_TOOL_WEAK otError otIp4FromIp4MappedIp6Address(const otIp6Address *aIp6Address, otIp4Address *aIp4Address)
{
    OT_UNUSED_VARIABLE(aIp6Address);
    OT_UNUSED_VARIABLE(aIp4Address);

    fprintf(stderr, "\n\rWeak otIp4FromIp4MappedIp6Address() is incorrectly used\n\r");
    exit(1);

    return OT_ERROR_NOT_IMPLEMENTED;
}

OT_TOOL_WEAK void otPlatMdnsHandleReceive(otInstance                  *aInstance,
                                          otMessage                   *aMessage,
                                          bool                         aIsUnicast,
                                          const otPlatMdnsAddressInfo *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aIsUnicast);
    OT_UNUSED_VARIABLE(aAddress);

    fprintf(stderr, "\n\rWeak otPlatMdnsHandleReceive() is incorrectly used\n\r");
    exit(1);
}

//---------------------------------------------------------------------------------------------------------------------
#else // OPENTHREAD_SIMULATION_MDNS_SOCKET_IMPLEMENT_POSIX

otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    return OT_ERROR_NOT_IMPLEMENTED;
}

void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    otMessageFree(aMessage);
}

void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAddress);
    otMessageFree(aMessage);
}

#endif // OPENTHREAD_SIMULATION_MDNS_SOCKET_IMPLEMENT_POSIX

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
