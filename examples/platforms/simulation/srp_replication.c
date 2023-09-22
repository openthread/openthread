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

#include "platform-simulation.h"

#include <openthread/dns.h>
#include <openthread/thread.h>
#include <openthread/platform/srp_replication.h>

#include "utils/code_utils.h"

#if OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE

// Change DEBUG_LOG for enable extra logging
#define DEBUG_LOG 0

// The IPv4 group for receiving
#define SRPL_DNSSD_SIM_GROUP "224.0.0.116"
#define SRPL_DNSSD_SIM_PORT 9800

#define SRPL_DNSSD_MAX_TXT_LEN (OT_DNS_MAX_NAME_SIZE + 100)

#define SRPL_MAX_PENDING_TX 32

#define SRPL_SERVICE_PORT 853

typedef enum MessageType
{
    SRPL_DNSSD_BROWSE,
    SRPL_DNSSD_ADD_SERVICE,
    SRPL_DNSSD_REMOVE_SERVICE,
} MessageType;

typedef struct Message
{
    MessageType mType;
    otSockAddr  mSockAddr;
    uint16_t    mTxtLength;
    uint8_t     mTxtData[SRPL_DNSSD_MAX_TXT_LEN];
} Message;

static uint8_t sNumPendingTx = 0;
static Message sPendingTx[SRPL_MAX_PENDING_TX];

static int      sTxFd       = -1;
static int      sRxFd       = -1;
static uint16_t sPortOffset = 0;

static bool       sBrowseEnabled     = false;
static bool       sServiceRegistered = false;
static otSockAddr sServiceSockAddr;
static uint16_t   sServiceTxtLength;
static char       sServiceTxtData[SRPL_DNSSD_MAX_TXT_LEN];

#if DEBUG_LOG
static void dumpBuffer(const void *aBuffer, uint16_t aLength)
{
    const uint8_t *buffer = (const uint8_t *)aBuffer;
    fprintf(stderr, "[ (len:%d) ", aLength);

    while (aLength--)
    {
        fprintf(stderr, "%02x ", *buffer++);
    }

    fprintf(stderr, "]");
}

static const char *messageTypeToString(MessageType aType)
{
    const char *str = "unknown";

    switch (aType)
    {
    case SRPL_DNSSD_BROWSE:
        str = "browse";
        break;
    case SRPL_DNSSD_ADD_SERVICE:
        str = "add-service";
        break;
    case SRPL_DNSSD_REMOVE_SERVICE:
        str = "remove-service";
        break;
    }

    return str;
}
#endif // DEBUG_LOG

static void initFds(void)
{
    int                fd;
    int                one = 1;
    struct sockaddr_in sockaddr;
    struct ip_mreqn    mreq;

    memset(&sockaddr, 0, sizeof(sockaddr));

    otEXPECT_ACTION((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != -1, perror("socket(sTxFd)"));

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons((uint16_t)(SRPL_DNSSD_SIM_PORT + sPortOffset + gNodeId));
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &sockaddr.sin_addr, sizeof(sockaddr.sin_addr)) != -1,
                    perror("setsockopt(sTxFd, IP_MULTICAST_IF)"));

    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &one, sizeof(one)) != -1,
                    perror("setsockopt(sTxFd, IP_MULTICAST_LOOP)"));

    otEXPECT_ACTION(bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != -1, perror("bind(sTxFd)"));

    // Tx fd is successfully initialized.
    sTxFd = fd;

    otEXPECT_ACTION((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != -1, perror("socket(sRxFd)"));

    otEXPECT_ACTION(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) != -1,
                    perror("setsockopt(sRxFd, SO_REUSEADDR)"));
    otEXPECT_ACTION(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) != -1,
                    perror("setsockopt(sRxFd, SO_REUSEPORT)"));

    memset(&mreq, 0, sizeof(mreq));
    inet_pton(AF_INET, SRPL_DNSSD_SIM_GROUP, &mreq.imr_multiaddr);

    // Always use loopback device to send simulation packets.
    mreq.imr_address.s_addr = inet_addr("127.0.0.1");

    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_address, sizeof(mreq.imr_address)) != -1,
                    perror("setsockopt(sRxFd, IP_MULTICAST_IF)"));
    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1,
                    perror("setsockopt(sRxFd, IP_ADD_MEMBERSHIP)"));

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons((uint16_t)(SRPL_DNSSD_SIM_PORT + sPortOffset));
    sockaddr.sin_addr.s_addr = inet_addr(SRPL_DNSSD_SIM_GROUP);

    otEXPECT_ACTION(bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != -1, perror("bind(sRxFd)"));

    // Rx fd is successfully initialized.
    sRxFd = fd;

exit:
    if (sRxFd == -1 || sTxFd == -1)
    {
        exit(EXIT_FAILURE);
    }
}

static void deinitFds(void)
{
    if (sRxFd != -1)
    {
        close(sRxFd);
    }

    if (sTxFd != -1)
    {
        close(sTxFd);
    }
}

static uint16_t getMessageSize(const Message *aMessage)
{
    return (uint16_t)(&aMessage->mTxtData[aMessage->mTxtLength] - (const uint8_t *)aMessage);
}

static void sendPendingTxMessages(void)
{
    ssize_t            rval;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, SRPL_DNSSD_SIM_GROUP, &sockaddr.sin_addr);

    sockaddr.sin_port = htons((uint16_t)(SRPL_DNSSD_SIM_PORT + sPortOffset));

    for (uint8_t i = 0; i < sNumPendingTx; i++)
    {
        uint16_t size = getMessageSize(&sPendingTx[i]);

#if DEBUG_LOG
        fprintf(stderr, "\r\n[srpl-sim] Sending message, type:%s\r\n", messageTypeToString(sPendingTx[i].mType));
#endif

        rval = sendto(sTxFd, &sPendingTx[i], size, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

        if (rval < 0)
        {
            perror("sendto(sTxFd)");
            exit(EXIT_FAILURE);
        }
    }

    sNumPendingTx = 0;
}

static void sendBrowseMessage(void)
{
    Message *message;

    assert(sNumPendingTx < SRPL_MAX_PENDING_TX);
    message = &sPendingTx[sNumPendingTx++];

    message->mType      = SRPL_DNSSD_BROWSE;
    message->mTxtLength = 0;

#if DEBUG_LOG
    fprintf(stderr, "\r\n[srpl-sim] sendBrowseMessage()\r\n");
#endif
}

static void sendServiceMessage(MessageType aType)
{
    Message *message;

    assert((aType == SRPL_DNSSD_ADD_SERVICE) || (aType == SRPL_DNSSD_REMOVE_SERVICE));

    assert(sNumPendingTx < SRPL_MAX_PENDING_TX);
    message = &sPendingTx[sNumPendingTx++];

    message->mType      = aType;
    message->mSockAddr  = sServiceSockAddr;
    message->mTxtLength = sServiceTxtLength;
    memcpy(message->mTxtData, sServiceTxtData, sServiceTxtLength);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[srpl-sim] sendServiceMessage(%s): txt-len:%u\r\n",
            (aType == SRPL_DNSSD_ADD_SERVICE) ? "add" : "remove", sServiceTxtLength);
#endif
}

static void processMessage(otInstance *aInstance, Message *aMessage, uint16_t aLength)
{
    otPlatSrplPartnerInfo partnerInfo;

#if DEBUG_LOG
    fprintf(stderr, "\r\n[srpl-sim] processMessage, type:%s\r\n", messageTypeToString(aMessage->mType));
#endif

    otEXPECT(aLength > 0);
    otEXPECT(getMessageSize(aMessage) == aLength);

    switch (aMessage->mType)
    {
    case SRPL_DNSSD_BROWSE:
        otEXPECT(sServiceRegistered);
        sendServiceMessage(SRPL_DNSSD_ADD_SERVICE);
        break;

    case SRPL_DNSSD_ADD_SERVICE:
    case SRPL_DNSSD_REMOVE_SERVICE:
        otEXPECT(sBrowseEnabled);
        // We skip our own service entry.
        otEXPECT(!sServiceRegistered || memcmp(&aMessage->mSockAddr, &sServiceSockAddr, sizeof(otSockAddr)) != 0);
        memset(&partnerInfo, 0, sizeof(partnerInfo));
        partnerInfo.mRemoved   = (aMessage->mType == SRPL_DNSSD_REMOVE_SERVICE);
        partnerInfo.mTxtData   = aMessage->mTxtData;
        partnerInfo.mTxtLength = aMessage->mTxtLength;
        partnerInfo.mSockAddr  = aMessage->mSockAddr;
        otPlatSrplHandleDnssdBrowseResult(aInstance, &partnerInfo);
        break;
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// `otPlatSrpl` APIs

void otPlatSrplDnssdBrowse(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[srpl-sim] otPlatSrplDnssdBrowse(aEnable=%d)\r\n", aEnable);
#endif

    otEXPECT(aEnable != sBrowseEnabled);

    sBrowseEnabled = aEnable;

    otEXPECT(sBrowseEnabled);
    sendBrowseMessage();

exit:
    return;
}

void otPlatSrplRegisterDnssdService(otInstance *aInstance, const uint8_t *aTxtData, uint16_t aTxtLength)
{
    assert(aTxtLength <= SRPL_DNSSD_MAX_TXT_LEN);

    if (sServiceRegistered)
    {
        sendServiceMessage(SRPL_DNSSD_REMOVE_SERVICE);
    }

    sServiceRegistered        = true;
    sServiceSockAddr.mAddress = *otThreadGetMeshLocalEid(aInstance);
    sServiceSockAddr.mPort    = SRPL_SERVICE_PORT;
    sServiceTxtLength         = aTxtLength;
    memcpy(sServiceTxtData, aTxtData, aTxtLength);

    sendServiceMessage(SRPL_DNSSD_ADD_SERVICE);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[srpl-sim] otPlatSrplRegisterDnssdService()\r\n TxtData:");
    dumpBuffer(aTxtData, aTxtLength);
    fprintf(stderr, "\r\n");
#endif
}

void otPlatSrplUnregisterDnssdService(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[srpl-sim] otPlatSrplUnregisterDnssdService()\r\n");
#endif

    if (sServiceRegistered)
    {
        sendServiceMessage(SRPL_DNSSD_REMOVE_SERVICE);
        sServiceRegistered = false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// platformSrpl system

void platformSrplInit(uint32_t aSpeedUpFactor)
{
    char *str;

    str = getenv("PORT_OFFSET");

    if (str != NULL)
    {
        char *endptr;

        sPortOffset = (uint16_t)strtol(str, &endptr, 0);

        if (*endptr != '\0')
        {
            fprintf(stderr, "Invalid PORT_OFFSET: %s\r\n", str);
            exit(EXIT_FAILURE);
        }

        sPortOffset *= (MAX_NETWORK_SIZE + 1);
    }

    initFds();

    OT_UNUSED_VARIABLE(aSpeedUpFactor);
}

void platformSrplDeinit(void) { deinitFds(); }

void platformSrplUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, struct timeval *aTimeout, int *aMaxFd)
{
    OT_UNUSED_VARIABLE(aTimeout);

    // Always ready to receive
    if (aReadFdSet != NULL)
    {
        FD_SET(sRxFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sRxFd)
        {
            *aMaxFd = sRxFd;
        }
    }

    if ((aWriteFdSet != NULL) && (sNumPendingTx > 0))
    {
        FD_SET(sTxFd, aWriteFdSet);

        if (aMaxFd != NULL && *aMaxFd < sTxFd)
        {
            *aMaxFd = sTxFd;
        }
    }
}

void platformSrplProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    if (FD_ISSET(sTxFd, aWriteFdSet) && (sNumPendingTx > 0))
    {
        sendPendingTxMessages();
    }

    if (FD_ISSET(sRxFd, aReadFdSet))
    {
        Message message;
        ssize_t rval;

        message.mTxtLength = 0;

        rval = recvfrom(sRxFd, (char *)&message, sizeof(message), 0, NULL, NULL);

        if (rval < 0)
        {
            perror("recvfrom(sRxFd)");
            exit(EXIT_FAILURE);
        }

        processMessage(aInstance, &message, (uint16_t)(rval));
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Weak definitions of APIs and callbacks (For RCP build).

OT_TOOL_WEAK void otPlatSrplHandleDnssdBrowseResult(otInstance *aInstance, const otPlatSrplPartnerInfo *aPartnerInfo)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPartnerInfo);

    assert(false);
}

OT_TOOL_WEAK const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    assert(false);

    return NULL;
}

#endif // OPENTHREAD_CONFIG_SRP_REPLICATION_ENABLE
