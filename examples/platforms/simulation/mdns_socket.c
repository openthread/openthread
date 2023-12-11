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

#include <openthread/platform/mdns_socket.h>

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

//---------------------------------------------------------------------------------------------------------------------
#if OPENTHREAD_SIMULATION_MDNS_SCOKET_IMPLEMENT_POSIX

// Provide a simplified POSIX based implementation of `otPlatMdns`
// platform APIs. This is intended for testing.

#include <openthread/ip6.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "utils/code_utils.h"

#define MAX_BUFFER_SIZE 1600

static bool sEnabled = false;
static int  sMdnsFd4 = -1;

otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    const char    *errMessage = NULL;
    struct ip_mreq mreq;

    if (aEnable)
    {
        struct sockaddr_in addr;
        int                yes = 1;
        int                fd;
        int                ret;

        otEXPECT(!sEnabled);

        fd = socket(AF_INET, SOCK_DGRAM, 0);
        otEXPECT_ACTION(fd >= 0, errMessage = "socket() failed for sMulticastRx");

        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));
        otEXPECT_ACTION(ret >= 0, errMessage = "setsocketopt(SO_REUSEADDR) failed");

        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&yes, sizeof(yes));
        otEXPECT_ACTION(ret >= 0, errMessage = "setsocketopt(SO_REUSEPORT) failed");

        memset(&addr, 0, sizeof(addr));
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port        = htons(5353);
        ret                  = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
        otEXPECT_ACTION(ret >= 0, errMessage = "bind() failed");

        mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        ret                       = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
        otEXPECT_ACTION(ret >= 0, errMessage = "setsocketopt(IP_ADD_MEMBERSHIP) failed");

        sMdnsFd4 = fd;
        sEnabled = true;
    }
    else
    {
        otEXPECT(sEnabled);

        mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(sMdnsFd4, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq, sizeof(mreq));

        close(sMdnsFd4);
        sEnabled = false;
    }

exit:
    if (errMessage != NULL)
    {
        fprintf(stderr, "\n\rotPlatMdnsSetEnabled(): %s\n\r", errMessage);
        exit(1);
    }

    return OT_ERROR_NONE;
}

void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfraIfIndex);

    struct sockaddr_in addr;
    uint8_t            buffer[MAX_BUFFER_SIZE];
    uint16_t           length;
    int                bytes;

    otEXPECT(sEnabled);

    length = otMessageRead(aMessage, 0, buffer, sizeof(buffer));
    otMessageFree(aMessage);

    // fprintf(stderr, "\n\rotPlatMdnsSendMulticast() len:%u", length);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("224.0.0.251");
    addr.sin_port        = htons(5353);

    bytes = sendto(sMdnsFd4, buffer, length, 0, (struct sockaddr *)&addr, sizeof(addr));

    if ((bytes < 0) || (bytes != length))
    {
        fprintf(stderr, "\ntPlatMdnsSendMulticast() failed");
        exit(1);
    }

exit:
    return;
}

void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aAddress);
    otMessageFree(aMessage);
}

void platformMdnsSocketUpdateFdSet(fd_set *aReadFdSet, int *aMaxFd)
{
    otEXPECT(sEnabled);

    FD_SET(sMdnsFd4, aReadFdSet);

    if (*aMaxFd < sMdnsFd4)
    {
        *aMaxFd = sMdnsFd4;
    }

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

        if (rval < 0)
        {
            fprintf(stderr, "\n\rplatformMdnsSocketProcess() - recvfrom() failed\n\r");
            exit(1);
        }

        message = otIp6NewMessage(aInstance, NULL);

        if (message == NULL)
        {
            fprintf(stderr, "\n\rplatformMdnsSocketProcess() - `otIp6NewMessage()` failed\n\r");
            exit(1);
        }

        if (otMessageAppend(message, buffer, (uint16_t)rval) != OT_ERROR_NONE)
        {
            fprintf(stderr, "\n\rplatformMdnsSocketProcess() - `otMessageAppend()` failed\n\r");
            exit(1);
        }

        memset(&addrInfo, 0, sizeof(addrInfo));
        addrInfo.mPort = 5353;

        // fprintf(stderr, "\n\rCalling otPlatMdnsSendMulticast(len:%u)", (uint16_t)rval);

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
#else // OPENTHREAD_SIMULATION_MDNS_SCOKET_IMPLEMENT_POSIX

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

#endif // OPENTHREAD_SIMULATION_MDNS_SCOKET_IMPLEMENT_POSIX

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
