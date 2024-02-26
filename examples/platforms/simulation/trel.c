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

#include "simul_utils.h"
#include "utils/code_utils.h"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

// Change DEBUG_LOG to all extra logging
#define DEBUG_LOG 0

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

static utilsSocket sSocket;
static uint16_t    sPortOffset = 0;
static bool        sEnabled    = false;

static bool               sServiceRegistered = false;
static uint16_t           sServicePort;
static uint8_t            sServiceTxtLength;
static char               sServiceTxtData[TREL_MAX_SERVICE_TXT_DATA_LEN];
static otPlatTrelCounters sCounters;

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

static uint16_t getMessageSize(const Message *aMessage)
{
    return (uint16_t)(&aMessage->mData[aMessage->mDataLength] - (const uint8_t *)aMessage);
}

static void sendPendingTxMessages(void)
{
    for (uint8_t i = 0; i < sNumPendingTx; i++)
    {
#if DEBUG_LOG
        fprintf(stderr, "\r\n[trel-sim] Sending message (num:%d, type:%s, port:%u)\r\n", i,
                messageTypeToString(sPendingTx[i].mType), sPendingTx[i].mSockAddr.mPort);
#endif
        utilsSendOverSocket(&sSocket, &sPendingTx[i], getMessageSize(&sPendingTx[i]));
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
        otEXPECT(aMessage->mSockAddr.mPort == sSocket.mPort);
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

    *aUdpPort = sSocket.mPort;

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

void otPlatTrelSend(otInstance       *aInstance,
                    const uint8_t    *aUdpPayload,
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
    ++sCounters.mTxPackets;
    sCounters.mTxBytes += aUdpPayloadLen;
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

    utilsInitSocket(&sSocket, TREL_SIM_PORT + sPortOffset);

    OT_UNUSED_VARIABLE(aSpeedUpFactor);
}

void platformTrelDeinit(void) { utilsDeinitSocket(&sSocket); }

void platformTrelUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, struct timeval *aTimeout, int *aMaxFd)
{
    OT_UNUSED_VARIABLE(aTimeout);

    // Always ready to receive
    utilsAddSocketRxFd(&sSocket, aReadFdSet, aMaxFd);

    if (sNumPendingTx > 0)
    {
        utilsAddSocketTxFd(&sSocket, aWriteFdSet, aMaxFd);
    }
}

void platformTrelProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    if ((sNumPendingTx > 0) && utilsCanSocketSend(&sSocket, aWriteFdSet))
    {
        sendPendingTxMessages();
    }

    if (utilsCanSocketReceive(&sSocket, aReadFdSet))
    {
        Message  message;
        uint16_t len;

        message.mDataLength = 0;

        len = utilsReceiveFromSocket(&sSocket, &message, sizeof(message), NULL);

        if (len > 0)
        {
            processMessage(aInstance, &message, len);
        }
    }
}

const otPlatTrelCounters *otPlatTrelGetCounters(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sCounters;
}

void otPlatTrelResetCounters(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    memset(&sCounters, 0, sizeof(sCounters));
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
