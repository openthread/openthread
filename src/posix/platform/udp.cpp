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
 * @brief
 *   This file includes the platform UDP driver.
 */

#ifdef __APPLE__
#define __APPLE_USE_RFC_3542
#endif

#include "openthread-core-config.h"
#include "platform-posix.h"

#include <arpa/inet.h>
#include <assert.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <openthread/udp.h>
#include <openthread/platform/udp.h>

#include "common/code_utils.hpp"

static uint32_t sPlatNetifIndex = 0;

static const size_t kMaxUdpSize = 1280;

static void *FdToHandle(int aFd)
{
    return reinterpret_cast<void *>(aFd);
}

static int FdFromHandle(void *aHandle)
{
    return static_cast<int>(reinterpret_cast<long>(aHandle));
}

static bool IsLinkLocal(const struct in6_addr &aAddress)
{
    return aAddress.s6_addr[0] == 0xfe && aAddress.s6_addr[1] == 0x80;
}

static bool IsMulticast(const struct in6_addr &aAddress)
{
    return aAddress.s6_addr[0] == 0xff;
}

static otError transmitPacket(int aFd, uint8_t *aPayload, uint16_t aLength, const otMessageInfo &aMessageInfo)
{
    struct sockaddr_in6 peerAddr;
    uint8_t             control[CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int))];
    size_t              controlLength = 0;
    struct iovec        iov;
    struct msghdr       msg;
    struct cmsghdr *    cmsg;
    ssize_t             rval;

    memset(&peerAddr, 0, sizeof(peerAddr));
    peerAddr.sin6_port   = htons(aMessageInfo.mPeerPort);
    peerAddr.sin6_family = AF_INET6;
    memcpy(&peerAddr.sin6_addr, &aMessageInfo.mPeerAddr, sizeof(peerAddr.sin6_addr));

    if (IsLinkLocal(peerAddr.sin6_addr) && aMessageInfo.mInterfaceId == OT_NETIF_INTERFACE_ID_THREAD)
    {
        // sin6_scope_id only works for link local destinations
        peerAddr.sin6_scope_id = sPlatNetifIndex;
    }

    memset(control, 0, sizeof(control));

    iov.iov_base = aPayload;
    iov.iov_len  = aLength;

    msg.msg_name       = &peerAddr;
    msg.msg_namelen    = sizeof(peerAddr);
    msg.msg_control    = control;
    msg.msg_controllen = sizeof(control);
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_flags      = 0;

    cmsg = CMSG_FIRSTHDR(&msg);

    {
        cmsg->cmsg_level = IPPROTO_IPV6;
        cmsg->cmsg_type  = IPV6_HOPLIMIT;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(int));

        *reinterpret_cast<int *>(CMSG_DATA(cmsg)) = (aMessageInfo.mHopLimit ? aMessageInfo.mHopLimit : -1);

        cmsg = CMSG_NXTHDR(&msg, cmsg);
        controlLength += CMSG_SPACE(sizeof(int));
    }

    if (!IsMulticast(reinterpret_cast<const struct in6_addr &>(aMessageInfo.mSockAddr)) &&
        memcmp(&aMessageInfo.mSockAddr, &in6addr_any, sizeof(aMessageInfo.mSockAddr)))
    {
        struct in6_pktinfo &pktinfo = *reinterpret_cast<struct in6_pktinfo *>(CMSG_DATA(cmsg));

        cmsg->cmsg_level = IPPROTO_IPV6;
        cmsg->cmsg_type  = IPV6_PKTINFO;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(pktinfo));

        pktinfo.ipi6_ifindex = (aMessageInfo.mInterfaceId == OT_NETIF_INTERFACE_ID_THREAD ? sPlatNetifIndex : 0);

        memcpy(&pktinfo.ipi6_addr, &aMessageInfo.mSockAddr, sizeof(pktinfo.ipi6_addr));

        controlLength += CMSG_SPACE(sizeof(pktinfo));
        cmsg = CMSG_NXTHDR(&msg, cmsg);
    }

#ifdef __APPLE__
    msg.msg_controllen = static_cast<socklen_t>(controlLength);
#else
    msg.msg_controllen = controlLength;
#endif

    rval = sendmsg(aFd, &msg, 0);
    VerifyOrExit(rval > 0, perror("sendmsg"));

exit:
    return rval > 0 ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

static otError receivePacket(int aFd, uint8_t *aPayload, uint16_t &aLength, otMessageInfo &aMessageInfo)
{
    struct sockaddr_in6 peerAddr;
    uint8_t             control[kMaxUdpSize];
    struct iovec        iov;
    struct msghdr       msg;
    ssize_t             rval;

    iov.iov_base = aPayload;
    iov.iov_len  = aLength;

    msg.msg_name       = &peerAddr;
    msg.msg_namelen    = sizeof(peerAddr);
    msg.msg_control    = control;
    msg.msg_controllen = sizeof(control);
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_flags      = 0;

    rval = recvmsg(aFd, &msg, 0);
    VerifyOrExit(rval > 0, perror("recvmsg"));
    aLength = static_cast<uint16_t>(rval);

    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        if (cmsg->cmsg_level == IPPROTO_IPV6)
        {
            if (cmsg->cmsg_type == IPV6_HOPLIMIT)
            {
                int hoplimit           = *reinterpret_cast<int *>(CMSG_DATA(cmsg));
                aMessageInfo.mHopLimit = static_cast<uint8_t>(hoplimit);
            }
            else if (cmsg->cmsg_type == IPV6_PKTINFO)
            {
                struct in6_pktinfo *pktinfo;

                pktinfo = reinterpret_cast<in6_pktinfo *>(CMSG_DATA(cmsg));

                aMessageInfo.mInterfaceId =
                    (pktinfo->ipi6_ifindex == sPlatNetifIndex ? static_cast<int8_t>(OT_NETIF_INTERFACE_ID_THREAD) : 0);
                memcpy(&aMessageInfo.mSockAddr, &pktinfo->ipi6_addr, sizeof(aMessageInfo.mSockAddr));
            }
        }
    }

    aMessageInfo.mPeerPort = ntohs(peerAddr.sin6_port);
    memcpy(&aMessageInfo.mPeerAddr, &peerAddr.sin6_addr, sizeof(aMessageInfo.mPeerAddr));

exit:
    return rval > 0 ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatUdpSocket(otUdpSocket *aUdpSocket)
{
    otError error = OT_ERROR_NONE;
    int     fd;

    assert(aUdpSocket->mHandle == NULL);

    fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    VerifyOrExit(fd > 0, error = OT_ERROR_FAILED);

    aUdpSocket->mHandle = FdToHandle(fd);

exit:
    return error;
}

otError otPlatUdpClose(otUdpSocket *aUdpSocket)
{
    otError error = OT_ERROR_NONE;
    int     fd;

    VerifyOrExit(aUdpSocket->mHandle != NULL, error = OT_ERROR_INVALID_ARGS);
    fd = FdFromHandle(aUdpSocket->mHandle);
    VerifyOrExit(0 == close(fd), error = OT_ERROR_FAILED);

    aUdpSocket->mHandle = NULL;

exit:
    return error;
}

otError otPlatUdpBind(otUdpSocket *aUdpSocket)
{
    otError error = OT_ERROR_NONE;
    int     fd;

    assert(sPlatNetifIndex != 0);
    assert(aUdpSocket->mHandle != NULL);
    VerifyOrExit(sPlatNetifIndex != 0, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aUdpSocket->mHandle != NULL, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aUdpSocket->mSockName.mPort != 0, error = OT_ERROR_INVALID_ARGS);
    fd = FdFromHandle(aUdpSocket->mHandle);

    {
        struct sockaddr_in6 sin6;

        memset(&sin6, 0, sizeof(struct sockaddr_in6));
        sin6.sin6_port   = htons(aUdpSocket->mSockName.mPort);
        sin6.sin6_family = AF_INET6;
        memcpy(&sin6.sin6_addr, &aUdpSocket->mSockName.mAddress, sizeof(sin6.sin6_addr));
        VerifyOrExit(0 == bind(fd, reinterpret_cast<struct sockaddr *>(&sin6), sizeof(sin6)), error = OT_ERROR_FAILED);
    }

    {
        int on = 1;
        VerifyOrExit(0 == setsockopt(fd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on)), error = OT_ERROR_FAILED);
        VerifyOrExit(0 == setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)), error = OT_ERROR_FAILED);
    }

    VerifyOrExit(0 == setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &sPlatNetifIndex, sizeof(sPlatNetifIndex)),
                 error = OT_ERROR_FAILED);

exit:
    if (error == OT_ERROR_FAILED)
    {
        perror("otPlatUdpBind");
    }

    return error;
}

otError otPlatUdpConnect(otUdpSocket *aUdpSocket)
{
    otError             error = OT_ERROR_NONE;
    struct sockaddr_in6 sin6;
    int                 fd;
    bool isDisconnect = memcmp(&aUdpSocket->mPeerName.mAddress, &in6addr_any, sizeof(in6addr_any)) == 0 &&
                        aUdpSocket->mPeerName.mPort == 0;

    VerifyOrExit(aUdpSocket->mHandle != NULL, error = OT_ERROR_INVALID_ARGS);

    fd = FdFromHandle(aUdpSocket->mHandle);

    memset(&sin6, 0, sizeof(struct sockaddr_in6));
    sin6.sin6_port = htons(aUdpSocket->mPeerName.mPort);
    if (!isDisconnect)
    {
        sin6.sin6_family = AF_INET6;
        memcpy(&sin6.sin6_addr, &aUdpSocket->mPeerName.mAddress, sizeof(sin6.sin6_addr));
    }
    else
    {
#ifdef __APPLE__
        sin6.sin6_family = AF_UNSPEC;
#else
        // There is a bug in linux that connecting to AF_UNSPEC does not disconnect.
        // We create new socket to disconnect.
        SuccessOrExit(error = otPlatUdpClose(aUdpSocket));
        SuccessOrExit(error = otPlatUdpSocket(aUdpSocket));
        SuccessOrExit(error = otPlatUdpBind(aUdpSocket));
        ExitNow();
#endif
    }

    switch (connect(fd, reinterpret_cast<struct sockaddr *>(&sin6), sizeof(sin6)))
    {
    case 0:
        break;
#ifdef __APPLE__
    case EAFNOSUPPORT:
        VerifyOrExit(isDisconnect, error = OT_ERROR_FAILED);
        break;
#endif

    default:
        error = OT_ERROR_FAILED;
        break;
    }

exit:
    return error;
}

otError otPlatUdpSend(otUdpSocket *aUdpSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    int     fd;

    VerifyOrExit(aUdpSocket->mHandle != NULL, error = OT_ERROR_INVALID_ARGS);
    fd = FdFromHandle(aUdpSocket->mHandle);

    {
        uint8_t  payload[kMaxUdpSize];
        uint16_t len = otMessageGetLength(aMessage);

        VerifyOrExit(len == otMessageRead(aMessage, 0, payload, len), error = OT_ERROR_INVALID_ARGS);
        SuccessOrExit(error = transmitPacket(fd, payload, len, *aMessageInfo));
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        otMessageFree(aMessage);
    }

    return error;
}

void platformUdpUpdateFdSet(otInstance *aInstance, fd_set *aReadFdSet, int *aMaxFd)
{
    VerifyOrExit(sPlatNetifIndex != 0);

    for (otUdpSocket *socket = otUdpGetSockets(aInstance); socket != NULL; socket = socket->mNext)
    {
        int fd;

        if (socket->mHandle == NULL)
        {
            continue;
        }

        fd = FdFromHandle(socket->mHandle);
        FD_SET(fd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < fd)
        {
            *aMaxFd = fd;
        }
    }

exit:
    return;
}

void platformUdpInit(const char *aIfName)
{
    if (aIfName == NULL)
    {
        exit(OT_EXIT_INVALID_ARGUMENTS);
    }

    sPlatNetifIndex = if_nametoindex(aIfName);

    if (sPlatNetifIndex == 0)
    {
        perror("if_nametoindex");
    }
}

void platformUdpProcess(otInstance *aInstance, const fd_set *aReadFdSet)
{
    otMessageSettings msgSettings = {false, OT_MESSAGE_PRIORITY_NORMAL};

    VerifyOrExit(sPlatNetifIndex != 0);

    for (otUdpSocket *socket = otUdpGetSockets(aInstance); socket != NULL; socket = socket->mNext)
    {
        int fd = FdFromHandle(socket->mHandle);

        if (fd > 0 && FD_ISSET(fd, aReadFdSet))
        {
            otMessageInfo messageInfo;
            otMessage *   message = NULL;
            uint8_t       payload[kMaxUdpSize];
            uint16_t      length = sizeof(payload);

            memset(&messageInfo, 0, sizeof(messageInfo));
            messageInfo.mSockPort = socket->mSockName.mPort;

            if (OT_ERROR_NONE != receivePacket(fd, payload, length, messageInfo))
            {
                continue;
            }

            message = otUdpNewMessage(aInstance, &msgSettings);

            if (message == NULL)
            {
                continue;
            }

            if (otMessageAppend(message, payload, length) != OT_ERROR_NONE)
            {
                otMessageFree(message);
                continue;
            }

            socket->mHandler(socket->mContext, message, &messageInfo);
            otMessageFree(message);
            // only process one socket a time
            break;
        }
    }

exit:
    return;
}
