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

#include "simul_utils.h"

#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/time.h>

#include "utils/code_utils.h"

#define UTILS_SOCKET_LOCAL_HOST_ADDR "127.0.0.1"
#define UTILS_SOCKET_GROUP_ADDR "224.0.0.116"
#define UTILS_SOCKET_GROUP_ADDR6 "ff02::116"

const char *gLocalInterface = UTILS_SOCKET_LOCAL_HOST_ADDR;

void utilsAddFdToFdSet(int aFd, fd_set *aFdSet, int *aMaxFd)
{
    otEXPECT(aFd >= 0);
    otEXPECT(aFdSet != NULL);

    FD_SET(aFd, aFdSet);

    otEXPECT(aMaxFd != NULL);

    if (*aMaxFd < aFd)
    {
        *aMaxFd = aFd;
    }

exit:
    return;
}

static bool IsAddressLinkLocal(const struct in6_addr *aAddress)
{
    return ((aAddress->s6_addr[0] & 0xff) == 0xfe) && ((aAddress->s6_addr[1] & 0xc0) == 0x80);
}

static void InitRxSocket(utilsSocket *aSocket, const struct in_addr *aIp4Address, unsigned int aIfIndex)
{
    int fd;
    int one = 1;
    int rval;

    fd = socket(aIp4Address ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    otEXPECT_ACTION(fd != -1, perror("socket(RxFd)"));

    rval = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, SO_REUSEADDR)"));

    rval = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, SO_REUSEPORT)"));

    if (aIp4Address)
    {
        struct ip_mreqn     mreq;
        struct sockaddr_in *sockaddr = &aSocket->mGroupAddr.mSockAddr4;

        rval = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, aIp4Address, sizeof(*aIp4Address));
        otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, IP_MULTICAST_IF)"));

        memset(sockaddr, 0, sizeof(*sockaddr));
        sockaddr->sin_family = AF_INET;
        sockaddr->sin_port   = htons(aSocket->mPortBase);
        otEXPECT_ACTION(inet_pton(AF_INET, UTILS_SOCKET_GROUP_ADDR, &sockaddr->sin_addr), perror("inet_pton(AF_INET)"));

        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr = sockaddr->sin_addr;
        mreq.imr_address   = *aIp4Address; // This address is used to identify the network interface

        rval = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, IP_ADD_MEMBERSHIP)"));

        rval = bind(fd, (struct sockaddr *)sockaddr, sizeof(*sockaddr));
        otEXPECT_ACTION(rval != -1, perror("bind(RxFd)"));
    }
    else
    {
        struct ipv6_mreq     mreq;
        struct sockaddr_in6 *sockaddr = &aSocket->mGroupAddr.mSockAddr6;

        rval = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &aIfIndex, sizeof(aIfIndex));
        otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, IPV6_MULTICAST_IF)"));

        memset(sockaddr, 0, sizeof(*sockaddr));
        sockaddr->sin6_family   = AF_INET6;
        sockaddr->sin6_port     = htons(aSocket->mPortBase);
        sockaddr->sin6_scope_id = aIfIndex; // This specifies network interface for link local scope
        otEXPECT_ACTION(inet_pton(AF_INET6, UTILS_SOCKET_GROUP_ADDR6, &sockaddr->sin6_addr),
                        perror("inet_pton(AF_INET6)"));

        memset(&mreq, 0, sizeof(mreq));
        mreq.ipv6mr_multiaddr = sockaddr->sin6_addr;
        mreq.ipv6mr_interface = aIfIndex;

        rval = setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq));
        otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, IPV6_JOIN_GROUP)"));

        rval = bind(fd, (struct sockaddr *)sockaddr, sizeof(*sockaddr));
        otEXPECT_ACTION(rval != -1, perror("bind(RxFd)"));
    }

    aSocket->mRxFd = fd;

exit:
    if (aSocket->mRxFd == -1)
    {
        exit(EXIT_FAILURE);
    }
}

void InitTxSocketIp6(utilsSocket *aSocket, const struct in6_addr *aAddress, unsigned int aIfIndex)
{
    int                 fd;
    int                 one = 1;
    int                 rval;
    struct sockaddr_in6 sockaddr;

    fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    otEXPECT_ACTION(fd != -1, perror("socket(TxFd)"));

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin6_family = AF_INET6;
    sockaddr.sin6_addr   = *aAddress;
    sockaddr.sin6_port   = htons(aSocket->mPort);
    if (IsAddressLinkLocal(aAddress))
    {
        sockaddr.sin6_scope_id = aIfIndex;
    }

    rval = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &aIfIndex, sizeof(aIfIndex));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(TxFd, IPV6_MULTICAST_IF)"));

    rval = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &one, sizeof(one));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(TxFd, IPV6_MULTICAST_LOOP)"));

    rval = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    otEXPECT_ACTION(rval != -1, perror("bind(TxFd)"));

    aSocket->mTxFd = fd;

exit:
    if (aSocket->mTxFd == -1)
    {
        exit(EXIT_FAILURE);
    }
}

static void InitTxSocketIp4(utilsSocket *aSocket, const struct in_addr *aAddress)
{
    int                fd;
    int                one = 1;
    int                rval;
    struct sockaddr_in sockaddr;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare `mTxFd`

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    otEXPECT_ACTION(fd != -1, perror("socket(TxFd)"));

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(aSocket->mPort);
    sockaddr.sin_addr   = *aAddress;

    rval = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &sockaddr.sin_addr, sizeof(sockaddr.sin_addr));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(TxFd, IP_MULTICAST_IF)"));

    rval = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &one, sizeof(one));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(TxFd, IP_MULTICAST_LOOP)"));

    rval = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    otEXPECT_ACTION(rval != -1, perror("bind(TxFd)"));

    aSocket->mTxFd = fd;

exit:
    if (aSocket->mTxFd == -1)
    {
        exit(EXIT_FAILURE);
    }
}

static bool TryInitSocketIfname(utilsSocket *aSocket, const char *aLocalInterface)
{
    const struct in6_addr *addr6   = NULL;
    const struct in6_addr *addr6ll = NULL;
    const struct in_addr  *addr4   = NULL;
    struct ifaddrs        *ifaddr  = NULL;
    unsigned int           ifIndex = 0;

    otEXPECT((ifIndex = if_nametoindex(aLocalInterface)));

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL || strcmp(ifa->ifa_name, aLocalInterface) != 0)
        {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            addr4 = &((const struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            addr6 = &((const struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            if (IsAddressLinkLocal(addr6))
            {
                addr6ll = addr6;
            }
        }
    }

    // Prefer
    //  1. IPv6 link local address
    //  2. IPv4 addresses
    //  3. IPv6 addresses
    if (addr6ll)
    {
        InitTxSocketIp6(aSocket, addr6ll, ifIndex);
        addr6 = addr6ll;
    }
    else if (addr4)
    {
        InitTxSocketIp4(aSocket, addr4);
        addr6 = NULL;
    }
    else if (addr6)
    {
        InitTxSocketIp6(aSocket, addr6, ifIndex);
    }
    else
    {
        fprintf(stderr, "No sock address for TX socket!\n");
        exit(EXIT_FAILURE);
    }

    InitRxSocket(aSocket, (addr6 ? NULL : addr4), ifIndex);
    aSocket->mInitialized = true;
    aSocket->mUseIp6      = (addr6 != NULL);

exit:
    freeifaddrs(ifaddr);
    return aSocket->mInitialized;
}

static bool TryInitSocketIp4(utilsSocket *aSocket, const char *aLocalInterface)
{
    struct in_addr addr4;

    otEXPECT(inet_pton(AF_INET, aLocalInterface, &addr4));

    InitTxSocketIp4(aSocket, &addr4);
    InitRxSocket(aSocket, &addr4, 0);
    aSocket->mInitialized = true;
    aSocket->mUseIp6      = false;

exit:
    return aSocket->mInitialized;
}

static bool TryInitSocketIp6(utilsSocket *aSocket, const char *aLocalInterface)
{
    struct in6_addr addr6;
    struct ifaddrs *ifaddr = NULL;

    otEXPECT(inet_pton(AF_INET6, aLocalInterface, &addr6));

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        const struct sockaddr_in6 *sockaddr6;
        unsigned int               ifIndex;

        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }

        sockaddr6 = (const struct sockaddr_in6 *)ifa->ifa_addr;
        if (memcmp(&sockaddr6->sin6_addr, &addr6, sizeof(addr6)))
        {
            continue;
        }

        ifIndex = if_nametoindex(ifa->ifa_name);
        if (ifIndex == 0)
        {
            perror("if_nametoindex");
            exit(EXIT_FAILURE);
        }

        InitTxSocketIp6(aSocket, &addr6, ifIndex);
        InitRxSocket(aSocket, NULL, ifIndex);
        aSocket->mInitialized = true;
        aSocket->mUseIp6      = true;
        break;
    }

exit:
    freeifaddrs(ifaddr);
    return aSocket->mInitialized;
}

void utilsInitSocket(utilsSocket *aSocket, uint16_t aPortBase)
{
    aSocket->mInitialized = false;
    aSocket->mPortBase    = aPortBase;
    aSocket->mTxFd        = -1;
    aSocket->mRxFd        = -1;
    aSocket->mPort        = (uint16_t)(aSocket->mPortBase + gNodeId);

    if (!TryInitSocketIfname(aSocket, gLocalInterface) && !TryInitSocketIp4(aSocket, gLocalInterface) &&
        !TryInitSocketIp6(aSocket, gLocalInterface))
    {
        fprintf(stderr, "Failed to simulate node %d on %s\n", gNodeId, gLocalInterface);
        exit(EXIT_FAILURE);
    }
}

void utilsDeinitSocket(utilsSocket *aSocket)
{
    if (aSocket->mInitialized)
    {
        close(aSocket->mRxFd);
        close(aSocket->mTxFd);
        aSocket->mInitialized = false;
    }
}

void utilsAddSocketRxFd(const utilsSocket *aSocket, fd_set *aFdSet, int *aMaxFd)
{
    otEXPECT(aSocket->mInitialized);
    utilsAddFdToFdSet(aSocket->mRxFd, aFdSet, aMaxFd);

exit:
    return;
}

void utilsAddSocketTxFd(const utilsSocket *aSocket, fd_set *aFdSet, int *aMaxFd)
{
    otEXPECT(aSocket->mInitialized);
    utilsAddFdToFdSet(aSocket->mTxFd, aFdSet, aMaxFd);

exit:
    return;
}

bool utilsCanSocketReceive(const utilsSocket *aSocket, const fd_set *aReadFdSet)
{
    return aSocket->mInitialized && FD_ISSET(aSocket->mRxFd, aReadFdSet);
}

bool utilsCanSocketSend(const utilsSocket *aSocket, const fd_set *aWriteFdSet)
{
    return aSocket->mInitialized && FD_ISSET(aSocket->mTxFd, aWriteFdSet);
}

uint16_t utilsReceiveFromSocket(const utilsSocket *aSocket,
                                void              *aBuffer,
                                uint16_t           aBufferSize,
                                uint16_t          *aSenderNodeId)
{
    ssize_t  rval;
    uint16_t len = 0;
    union
    {
        struct sockaddr_in  sockaddr4;
        struct sockaddr_in6 sockaddr6;
    } sockaddr;
    socklen_t socklen = aSocket->mUseIp6 ? sizeof(sockaddr.sockaddr6) : sizeof(sockaddr.sockaddr4);

    memset(&sockaddr, 0, sizeof(sockaddr));

    rval = recvfrom(aSocket->mRxFd, (char *)aBuffer, aBufferSize, 0, (struct sockaddr *)&sockaddr, &socklen);

    if (rval > 0)
    {
        uint16_t senderPort = ntohs(aSocket->mUseIp6 ? sockaddr.sockaddr6.sin6_port : sockaddr.sockaddr4.sin_port);

        if (aSenderNodeId != NULL)
        {
            *aSenderNodeId = (uint16_t)(senderPort - aSocket->mPortBase);
        }

        len = (uint16_t)rval;
    }
    else if (rval == 0)
    {
        assert(false);
    }
    else if (errno != EINTR && errno != EAGAIN)
    {
        perror("recvfrom(RxFd)");
        exit(EXIT_FAILURE);
    }

    return len;
}

void utilsSendOverSocket(const utilsSocket *aSocket, const void *aBuffer, uint16_t aBufferLength)
{
    ssize_t rval;

    rval =
        sendto(aSocket->mTxFd, (const char *)aBuffer, aBufferLength, 0, (const struct sockaddr *)&aSocket->mGroupAddr,
               (aSocket->mUseIp6 ? sizeof(aSocket->mGroupAddr.mSockAddr6) : sizeof(aSocket->mGroupAddr.mSockAddr4)));

    if (rval < 0)
    {
        perror("sendto(sTxFd)");
        exit(EXIT_FAILURE);
    }
}
