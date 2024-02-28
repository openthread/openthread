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
#include <sys/time.h>

#include "utils/code_utils.h"

#define UTILS_SOCKET_LOCAL_HOST_ADDR "127.0.0.1"
#define UTILS_SOCKET_GROUP_ADDR "224.0.0.116"

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

void utilsInitSocket(utilsSocket *aSocket, uint16_t aPortBase)
{
    int                fd;
    int                one = 1;
    int                rval;
    struct sockaddr_in sockaddr;
    struct ip_mreqn    mreq;

    aSocket->mInitialized = false;
    aSocket->mPortBase    = aPortBase;
    aSocket->mPort        = (uint16_t)(aSocket->mPortBase + gNodeId);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare `mTxFd`

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    otEXPECT_ACTION(fd != -1, perror("socket(TxFd)"));

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(aSocket->mPort);
    sockaddr.sin_addr.s_addr = inet_addr(UTILS_SOCKET_LOCAL_HOST_ADDR);

    rval = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &sockaddr.sin_addr, sizeof(sockaddr.sin_addr));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(TxFd, IP_MULTICAST_IF)"));

    rval = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &one, sizeof(one));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(TxFd, IP_MULTICAST_LOOP)"));

    rval = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    otEXPECT_ACTION(rval != -1, perror("bind(TxFd)"));

    aSocket->mTxFd = fd;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Prepare `mRxFd`

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    otEXPECT_ACTION(fd != -1, perror("socket(RxFd)"));

    rval = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, SO_REUSEADDR)"));

    rval = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, SO_REUSEPORT)"));

    memset(&mreq, 0, sizeof(mreq));
    inet_pton(AF_INET, UTILS_SOCKET_GROUP_ADDR, &mreq.imr_multiaddr);

    mreq.imr_address.s_addr = inet_addr(UTILS_SOCKET_LOCAL_HOST_ADDR);

    rval = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_address, sizeof(mreq.imr_address));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, IP_MULTICAST_IF)"));

    rval = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    otEXPECT_ACTION(rval != -1, perror("setsockopt(RxFd, IP_ADD_MEMBERSHIP)"));

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(aSocket->mPortBase);
    sockaddr.sin_addr.s_addr = inet_addr(UTILS_SOCKET_GROUP_ADDR);

    rval = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    otEXPECT_ACTION(rval != -1, perror("bind(RxFd)"));

    aSocket->mRxFd = fd;

    aSocket->mInitialized = true;

exit:
    if (!aSocket->mInitialized)
    {
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
    struct sockaddr_in sockaddr;
    socklen_t          socklen = sizeof(sockaddr);
    ssize_t            rval;
    uint16_t           len = 0;

    memset(&sockaddr, 0, sizeof(sockaddr));

    rval = recvfrom(aSocket->mRxFd, (char *)aBuffer, aBufferSize, 0, (struct sockaddr *)&sockaddr, &socklen);

    if (rval > 0)
    {
        uint16_t senderPort = ntohs(sockaddr.sin_port);

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
    ssize_t            rval;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port   = htons(aSocket->mPortBase);
    inet_pton(AF_INET, UTILS_SOCKET_GROUP_ADDR, &sockaddr.sin_addr);

    rval =
        sendto(aSocket->mTxFd, (const char *)aBuffer, aBufferLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    if (rval < 0)
    {
        perror("sendto(sTxFd)");
        exit(EXIT_FAILURE);
    }
}
