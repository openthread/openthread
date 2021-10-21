/*
 *  Copyright (c) 2019-21, The OpenThread Authors.
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

#include <openthread/random_noncrypto.h>
#include <openthread/platform/trel.h>

#include "utils/code_utils.h"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

// Change DEBUG_LOG to all extra logging
#define DEBUG_LOG 0

// The IPv4 group for receiving
#define TREL_SIM_GROUP "224.0.0.116"
#define TREL_SIM_PORT 9200

#define TREL_MAX_PACKET_SIZE 1800

#define TREL_MAX_PENDING_TX 64

#define TREL_MAX_SERVICE_TXT_DATA_LEN 128

typedef enum MessageType
{
    TREL_DATA_MESSAGE,
    TREL_DNSSD_BROWSE_MESSAGE,
    TREL_DNSSD_ADD_SERVICE_MESSAGE,
    TREL_DNSSD_REMOVE_SERVICE_MESSAGE,
} MessageType;

typedef struct Message
{
    MessageType mType;
    otSockAddr  mSockAddr;                   // Destination (when TREL_DATA_MESSAGE), or peer addr (when DNS-SD service)
    uint16_t    mDataLength;                 // mData length
    uint8_t     mData[TREL_MAX_PACKET_SIZE]; // TREL UDP packet (when TREL_DATA_MESSAGE), or service TXT data.
} Message;

static uint8_t sNumPendingTx = 0;
static Message sPendingTx[TREL_MAX_PENDING_TX];

static int      sTxFd       = -1;
static int      sRxFd       = -1;
static uint16_t sPortOffset = 0;
static bool     sEnabled    = false;
static uint16_t sUdpPort;

static bool     sServiceRegistered = false;
static uint16_t sServicePort;
static uint8_t  sServiceTxtLength;
static char     sServiceTxtData[TREL_MAX_SERVICE_TXT_DATA_LEN];

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
    case TREL_DATA_MESSAGE:
        str = "data";
        break;
    case TREL_DNSSD_BROWSE_MESSAGE:
        str = "browse";
        break;
    case TREL_DNSSD_ADD_SERVICE_MESSAGE:
        str = "add-service";
        break;
    case TREL_DNSSD_REMOVE_SERVICE_MESSAGE:
        str = "remove-service";
        break;
    }

    return str;
}
#endif

static void initFds(void)
{
    int                fd;
    int                one = 1;
    struct sockaddr_in sockaddr;
    struct ip_mreqn    mreq;

    memset(&sockaddr, 0, sizeof(sockaddr));

    otEXPECT_ACTION((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != -1, perror("socket(sTxFd)"));

    sUdpPort                 = (uint16_t)(TREL_SIM_PORT + sPortOffset + gNodeId);
    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(sUdpPort);
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
    inet_pton(AF_INET, TREL_SIM_GROUP, &mreq.imr_multiaddr);

    // Always use loopback device to send simulation packets.
    mreq.imr_address.s_addr = inet_addr("127.0.0.1");

    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_address, sizeof(mreq.imr_address)) != -1,
                    perror("setsockopt(sRxFd, IP_MULTICAST_IF)"));
    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1,
                    perror("setsockopt(sRxFd, IP_ADD_MEMBERSHIP)"));

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons((uint16_t)(TREL_SIM_PORT + sPortOffset));
    sockaddr.sin_addr.s_addr = inet_addr(TREL_SIM_GROUP);

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
    return (uint16_t)(&aMessage->mData[aMessage->mDataLength] - (const uint8_t *)aMessage);
}

static void sendPendingTxMessages(void)
{
    ssize_t            rval;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, TREL_SIM_GROUP, &sockaddr.sin_addr);

    sockaddr.sin_port = htons((uint16_t)(TREL_SIM_PORT + sPortOffset));

    for (uint8_t i = 0; i < sNumPendingTx; i++)
    {
        uint16_t size = getMessageSize(&sPendingTx[i]);

#if DEBUG_LOG
        fprintf(stderr, "\r\n[trel-sim] Sending message (num:%d, type:%s, port:%u)\r\n", i,
                messageTypeToString(sPendingTx[i].mType), sPendingTx[i].mSockAddr.mPort);
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

    assert(sNumPendingTx < TREL_MAX_PENDING_TX);
    message = &sPendingTx[sNumPendingTx++];

    message->mType       = TREL_DNSSD_BROWSE_MESSAGE;
    message->mDataLength = 0;

#if DEBUG_LOG
    fprintf(stderr, "\r\n[trel-sim] sendBrowseMessage()\r\n");
#endif
}

static void sendServiceMessage(MessageType aType)
{
    Message *message;

    assert((aType == TREL_DNSSD_ADD_SERVICE_MESSAGE) || (aType == TREL_DNSSD_REMOVE_SERVICE_MESSAGE));

    assert(sNumPendingTx < TREL_MAX_PENDING_TX);
    message = &sPendingTx[sNumPendingTx++];

    message->mType = aType;
    memset(&message->mSockAddr, 0, sizeof(otSockAddr));
    message->mSockAddr.mPort = sServicePort;
    message->mDataLength     = sServiceTxtLength;
    memcpy(message->mData, sServiceTxtData, sServiceTxtLength);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[trel-sim] sendServiceMessage(%s): service-port:%u, txt-len:%u\r\n",
            aType == TREL_DNSSD_ADD_SERVICE_MESSAGE ? "add" : "remove", sServicePort, sServiceTxtLength);
#endif
}

static void processMessage(otInstance *aInstance, Message *aMessage, uint16_t aLength)
{
    otPlatTrelPeerInfo peerInfo;

#if DEBUG_LOG
    fprintf(stderr, "\r\n[trel-sim] processMessage(len:%u, type:%s, port:%u)\r\n", aLength,
            messageTypeToString(aMessage->mType), aMessage->mSockAddr.mPort);
#endif

    otEXPECT(aLength > 0);
    otEXPECT(getMessageSize(aMessage) == aLength);

    switch (aMessage->mType)
    {
    case TREL_DATA_MESSAGE:
        otEXPECT(aMessage->mSockAddr.mPort == sUdpPort);
        otPlatTrelHandleReceived(aInstance, aMessage->mData, aMessage->mDataLength);
        break;

    case TREL_DNSSD_BROWSE_MESSAGE:
        sendServiceMessage(TREL_DNSSD_ADD_SERVICE_MESSAGE);
        break;

    case TREL_DNSSD_ADD_SERVICE_MESSAGE:
    case TREL_DNSSD_REMOVE_SERVICE_MESSAGE:
        memset(&peerInfo, 0, sizeof(peerInfo));
        peerInfo.mRemoved   = (aMessage->mType == TREL_DNSSD_REMOVE_SERVICE_MESSAGE);
        peerInfo.mTxtData   = aMessage->mData;
        peerInfo.mTxtLength = (uint8_t)(aMessage->mDataLength);
        peerInfo.mSockAddr  = aMessage->mSockAddr;
        otPlatTrelHandleDiscoveredPeerInfo(aInstance, &peerInfo);
        break;
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// otPlatTrel

void otPlatTrelEnable(otInstance *aInstance, uint16_t *aUdpPort)
{
    OT_UNUSED_VARIABLE(aInstance);

    *aUdpPort = sUdpPort;

#if DEBUG_LOG
    fprintf(stderr, "\r\n[trel-sim] otPlatTrelEnable() *aUdpPort=%u\r\n", *aUdpPort);
#endif

    if (!sEnabled)
    {
        sEnabled = true;
        sendBrowseMessage();
    }
}

void otPlatTrelDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[trel-sim] otPlatTrelDisable()\r\n");
#endif

    if (sEnabled)
    {
        sEnabled = false;

        if (sServiceRegistered)
        {
            sendServiceMessage(TREL_DNSSD_REMOVE_SERVICE_MESSAGE);
            sServiceRegistered = false;
        }
    }
}

void otPlatTrelRegisterService(otInstance *aInstance, uint16_t aPort, const uint8_t *aTxtData, uint8_t aTxtLength)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aTxtLength <= TREL_MAX_SERVICE_TXT_DATA_LEN);

    if (sServiceRegistered)
    {
        sendServiceMessage(TREL_DNSSD_REMOVE_SERVICE_MESSAGE);
    }

    sServiceRegistered = true;
    sServicePort       = aPort;
    sServiceTxtLength  = aTxtLength;
    memcpy(sServiceTxtData, aTxtData, aTxtLength);

    sendServiceMessage(TREL_DNSSD_ADD_SERVICE_MESSAGE);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[trel-sim] otPlatTrelRegisterService(aPort:%d, aTxtData:", aPort);
    dumpBuffer(aTxtData, aTxtLength);
    fprintf(stderr, ")\r\n");
#endif
}

void otPlatTrelSend(otInstance *      aInstance,
                    const uint8_t *   aUdpPayload,
                    uint16_t          aUdpPayloadLen,
                    const otSockAddr *aDestSockAddr)
{
    OT_UNUSED_VARIABLE(aInstance);

    Message *message;

    assert(sNumPendingTx < TREL_MAX_PENDING_TX);
    assert(aUdpPayloadLen <= TREL_MAX_PACKET_SIZE);

    message = &sPendingTx[sNumPendingTx++];

    message->mType       = TREL_DATA_MESSAGE;
    message->mSockAddr   = *aDestSockAddr;
    message->mDataLength = aUdpPayloadLen;
    memcpy(message->mData, aUdpPayload, aUdpPayloadLen);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[trel-sim] otPlatTrelSend(len:%u, port:%u)\r\n", aUdpPayloadLen, aDestSockAddr->mPort);
#endif
}

//---------------------------------------------------------------------------------------------------------------------
// platformTrel system

void platformTrelInit(uint32_t aSpeedUpFactor)
{
    char *str;

    str = getenv("PORT_OFFSET");

    if (str != NULL)
    {
        char *endptr;

        sPortOffset = (uint16_t)strtol(str, &endptr, 0);

        if (*endptr != '\0')
        {
            fprintf(stderr, "\r\nInvalid PORT_OFFSET: %s\r\n", str);
            exit(EXIT_FAILURE);
        }

        sPortOffset *= (MAX_NETWORK_SIZE + 1);
    }

    initFds();

    OT_UNUSED_VARIABLE(aSpeedUpFactor);
}

void platformTrelDeinit(void)
{
    deinitFds();
}

void platformTrelUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, struct timeval *aTimeout, int *aMaxFd)
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

void platformTrelProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    if (FD_ISSET(sTxFd, aWriteFdSet) && (sNumPendingTx > 0))
    {
        sendPendingTxMessages();
    }

    if (FD_ISSET(sRxFd, aReadFdSet))
    {
        Message message;
        ssize_t rval;

        message.mDataLength = 0;

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

// This is added for RCP build to be built ok
OT_TOOL_WEAK void otPlatTrelHandleReceived(otInstance *aInstance, uint8_t *aBuffer, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aLength);

    assert(false);
}

OT_TOOL_WEAK void otPlatTrelHandleDiscoveredPeerInfo(otInstance *aInstance, const otPlatTrelPeerInfo *aInfo)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aInfo);

    assert(false);
}

#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
