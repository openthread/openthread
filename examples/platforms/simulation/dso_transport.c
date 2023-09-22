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

#include <openthread/thread.h>
#include <openthread/platform/dso_transport.h>

#include "utils/code_utils.h"

#if OPENTHREAD_CONFIG_DNS_DSO_ENABLE

// Change DEBUG_LOG for enable extra logging
#define DEBUG_LOG 0

// The IPv4 group for receiving
#define DSO_SIM_GROUP "224.0.0.116"
#define DSO_SIM_PORT 9600

#define DSO_MAX_CONNECTIONS 32
#define DSO_MAX_PENDING_TX 32
#define DSO_MAX_DATA_SIZE 1600
#define DSO_MESSAGE_STRING_SIZE 200
#define DSO_SRC_PORT 853

typedef enum MessageType
{
    DSO_MSG_CMD_CONNECT,
    DSO_MSG_CMD_ACCEPT,
    DSO_MSG_CMD_CLOSE,
    DSO_MSG_CMD_ABORT,
    DSO_MSG_DATA,
} MessageType;

typedef struct Message
{
    MessageType mType;
    otSockAddr  mSrcAddr;
    otSockAddr  mDstAddr;
    uint16_t    mDataLength;
    uint8_t     mData[DSO_MAX_DATA_SIZE];
} Message;

typedef struct Connection
{
    otSockAddr           mPeerAddr;
    otPlatDsoConnection *mDsoConnection;
} Connection;

static Connection sConnections[DSO_MAX_CONNECTIONS];

static uint8_t sNumPendingTx = 0;
static Message sPendingTx[DSO_MAX_PENDING_TX];

static int      sTxFd       = -1;
static int      sRxFd       = -1;
static uint16_t sPortOffset = 0;
static uint16_t sUdpPort;

static bool sListeningEnabled = false;

#if DEBUG_LOG
static const char *messageTypeToString(MessageType aType)
{
    const char *str = "unknown";

    switch (aType)
    {
    case DSO_MSG_CMD_CONNECT:
        str = "connect";
        break;
    case DSO_MSG_CMD_ACCEPT:
        str = "accept";
        break;
    case DSO_MSG_CMD_CLOSE:
        str = "close";
        break;
    case DSO_MSG_CMD_ABORT:
        str = "abort";
        break;
    case DSO_MSG_DATA:
        str = "data";
        break;
    }

    return str;
}

static const char *messageToString(const Message *aMessage)
{
    static char string[DSO_MESSAGE_STRING_SIZE];

    string[0] = '\0';

    snprintf(string, sizeof(string), "type:%s, src:", messageTypeToString(aMessage->mType));
    otIp6SockAddrToString(&aMessage->mSrcAddr, &string[strlen(string)], sizeof(string) - strlen(string));
    snprintf(&string[strlen(string)], sizeof(string) - strlen(string), ", dst:");
    otIp6SockAddrToString(&aMessage->mDstAddr, &string[strlen(string)], sizeof(string) - strlen(string));

    if (aMessage->mType == DSO_MSG_DATA)
    {
        snprintf(&string[strlen(string)], sizeof(string) - strlen(string), ", data-len:%u", aMessage->mDataLength);
    }

    return string;
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

    sUdpPort                 = (uint16_t)(DSO_SIM_PORT + sPortOffset + gNodeId);
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
    inet_pton(AF_INET, DSO_SIM_GROUP, &mreq.imr_multiaddr);

    // Always use loopback device to send simulation packets.
    mreq.imr_address.s_addr = inet_addr("127.0.0.1");

    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_address, sizeof(mreq.imr_address)) != -1,
                    perror("setsockopt(sRxFd, IP_MULTICAST_IF)"));
    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1,
                    perror("setsockopt(sRxFd, IP_ADD_MEMBERSHIP)"));

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons((uint16_t)(DSO_SIM_PORT + sPortOffset));
    sockaddr.sin_addr.s_addr = inet_addr(DSO_SIM_GROUP);

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

static Connection *findConnectionByPeerAddr(const otSockAddr *aPeerAddr)
{
    Connection *rval = NULL;
    Connection *conn;

    for (conn = sConnections; conn < &sConnections[DSO_MAX_CONNECTIONS]; conn++)
    {
        if ((conn->mDsoConnection != NULL) && (memcmp(&conn->mPeerAddr, aPeerAddr, sizeof(otSockAddr)) == 0))
        {
            rval = conn;
            break;
        }
    }

    return rval;
}

static Connection *findConnection(const otPlatDsoConnection *aConnection)
{
    Connection *rval = NULL;
    Connection *conn;

    for (conn = sConnections; conn < &sConnections[DSO_MAX_CONNECTIONS]; conn++)
    {
        if (conn->mDsoConnection == aConnection)
        {
            rval = conn;
            break;
        }
    }

    return rval;
}

static Connection *newConnection(void) { return findConnection(NULL); }

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
    inet_pton(AF_INET, DSO_SIM_GROUP, &sockaddr.sin_addr);

    sockaddr.sin_port = htons((uint16_t)(DSO_SIM_PORT + sPortOffset));

    for (uint8_t i = 0; i < sNumPendingTx; i++)
    {
        uint16_t size = getMessageSize(&sPendingTx[i]);

#if DEBUG_LOG
        fprintf(stderr, "\r\n[dso-sim] Sent message, %s\r\n", messageToString(&sPendingTx[i]));
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

static void sendCommand(MessageType aType, const Connection *aConnection)
{
    Message *message;

    assert(sNumPendingTx < DSO_MAX_PENDING_TX);
    message = &sPendingTx[sNumPendingTx++];

    message->mType             = aType;
    message->mSrcAddr.mAddress = *otThreadGetMeshLocalEid(otPlatDsoGetInstance(aConnection->mDsoConnection));
    message->mSrcAddr.mPort    = DSO_SRC_PORT;
    message->mDstAddr          = aConnection->mPeerAddr;
    message->mDataLength       = 0;
}

static void sendData(const otMessage *aData, const Connection *aConnection)
{
    Message *message;

    assert(sNumPendingTx < DSO_MAX_PENDING_TX);
    message = &sPendingTx[sNumPendingTx++];

    message->mType             = DSO_MSG_DATA;
    message->mSrcAddr.mAddress = *otThreadGetMeshLocalEid(otPlatDsoGetInstance(aConnection->mDsoConnection));
    message->mSrcAddr.mPort    = DSO_SRC_PORT;
    message->mDstAddr          = aConnection->mPeerAddr;

    message->mDataLength = otMessageGetLength(aData);
    assert(message->mDataLength <= DSO_MAX_DATA_SIZE);
    otMessageRead(aData, 0, message->mData, message->mDataLength);
}

static void processMessage(otInstance *aInstance, Message *aMessage, uint16_t aLength)
{
    Connection *conn;
    otMessage  *message = NULL;

    otEXPECT(aLength > 0);
    otEXPECT(getMessageSize(aMessage) == aLength);

    otEXPECT(aMessage->mDstAddr.mPort == DSO_SRC_PORT);
    otEXPECT(memcmp(&aMessage->mDstAddr.mAddress, otThreadGetMeshLocalEid(aInstance), sizeof(otIp6Address)) == 0);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[dso-sim] processMessage, %s\r\n", messageToString(aMessage));
#endif

    conn = findConnectionByPeerAddr(&aMessage->mSrcAddr);

    switch (aMessage->mType)
    {
    case DSO_MSG_CMD_CONNECT:
        otEXPECT(sListeningEnabled);
        otEXPECT(conn == NULL);

        conn = newConnection();
        otEXPECT(conn != NULL);

        conn->mDsoConnection = otPlatDsoAccept(aInstance, &aMessage->mSrcAddr);
        conn->mPeerAddr      = aMessage->mSrcAddr;

        if (conn->mDsoConnection == NULL)
        {
            sendCommand(DSO_MSG_CMD_ABORT, conn);
        }
        else
        {
            sendCommand(DSO_MSG_CMD_ACCEPT, conn);
            otPlatDsoHandleConnected(conn->mDsoConnection);
        }
        break;

    case DSO_MSG_CMD_ACCEPT:
        otEXPECT(conn != NULL);
        otPlatDsoHandleConnected(conn->mDsoConnection);
        break;

    case DSO_MSG_CMD_CLOSE:
    case DSO_MSG_CMD_ABORT:
        otEXPECT(conn != NULL);
        otPlatDsoHandleDisconnected(conn->mDsoConnection, (aMessage->mType == DSO_MSG_CMD_CLOSE)
                                                              ? OT_PLAT_DSO_DISCONNECT_MODE_GRACEFULLY_CLOSE
                                                              : OT_PLAT_DSO_DISCONNECT_MODE_FORCIBLY_ABORT);
        conn->mDsoConnection = NULL;
        break;

    case DSO_MSG_DATA:
        otEXPECT(conn != NULL);
        message = otIp6NewMessage(aInstance, NULL);
        otEXPECT(message != NULL);
        otEXPECT(otMessageAppend(message, aMessage->mData, aMessage->mDataLength) == OT_ERROR_NONE);
        otPlatDsoHandleReceive(conn->mDsoConnection, message);
        message = NULL;
        break;
    }

exit:
    if (message != NULL)
    {
        otMessageFree(message);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// `otPlatDso` functions

void otPlatDsoEnableListening(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

#if DEBUG_LOG
    fprintf(stderr, "\r\n[dso-sim] otPlatDsoEnableListening(aEnable:%d)\r\n", aEnable);
#endif

    sListeningEnabled = aEnable;
}

void otPlatDsoConnect(otPlatDsoConnection *aConnection, const otSockAddr *aPeerSockAddr)
{
    Connection *conn;

    assert(findConnectionByPeerAddr(aPeerSockAddr) == NULL);

    conn = newConnection();
    otEXPECT(conn != NULL);

    conn->mDsoConnection = aConnection;
    conn->mPeerAddr      = *aPeerSockAddr;
    sendCommand(DSO_MSG_CMD_CONNECT, conn);

exit:
    return;
}

void otPlatDsoSend(otPlatDsoConnection *aConnection, otMessage *aMessage)
{
    Connection *conn = findConnection(aConnection);

    otEXPECT(conn != NULL);
    sendData(aMessage, conn);

exit:
    otMessageFree(aMessage);
}

void otPlatDsoDisconnect(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode)
{
    Connection *conn = findConnection(aConnection);
    MessageType type;

    otEXPECT(conn != NULL);
    type = (aMode == OT_PLAT_DSO_DISCONNECT_MODE_GRACEFULLY_CLOSE) ? DSO_MSG_CMD_CLOSE : DSO_MSG_CMD_ABORT;
    sendCommand(type, conn);

    conn->mDsoConnection = NULL;

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// platformDso system

void platformDsoInit(uint32_t aSpeedUpFactor)
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

void platformDsoDeinit(void) { deinitFds(); }

void platformDsoUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, struct timeval *aTimeout, int *aMaxFd)
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

void platformDsoProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
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
// Weak definitions of APIs and callbacks (For RCP build).

OT_TOOL_WEAK otInstance *otPlatDsoGetInstance(otPlatDsoConnection *aConnection)
{
    OT_UNUSED_VARIABLE(aConnection);
    assert(false);
    return NULL;
}

OT_TOOL_WEAK otPlatDsoConnection *otPlatDsoAccept(otInstance *aInstance, const otSockAddr *aPeerSockAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPeerSockAddr);
    assert(false);
    return NULL;
}

OT_TOOL_WEAK void otPlatDsoHandleConnected(otPlatDsoConnection *aConnection)
{
    OT_UNUSED_VARIABLE(aConnection);
    assert(false);
}

OT_TOOL_WEAK void otPlatDsoHandleReceive(otPlatDsoConnection *aConnection, otMessage *aMessage)
{
    OT_UNUSED_VARIABLE(aConnection);
    OT_UNUSED_VARIABLE(aMessage);
    assert(false);
}

OT_TOOL_WEAK void otPlatDsoHandleDisconnected(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode)
{
    OT_UNUSED_VARIABLE(aConnection);
    OT_UNUSED_VARIABLE(aMode);
    assert(false);
}

OT_TOOL_WEAK otMessage *otIp6NewMessage(otInstance *aInstance, const otMessageSettings *aSettings)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aSettings);

    assert(false);
    return NULL;
}

OT_TOOL_WEAK void otMessageFree(otMessage *aMessage)
{
    OT_UNUSED_VARIABLE(aMessage);
    assert(false);
}

OT_TOOL_WEAK uint16_t otMessageGetLength(const otMessage *aMessage)
{
    OT_UNUSED_VARIABLE(aMessage);
    assert(false);
    return 0;
}

OT_TOOL_WEAK otError otMessageAppend(otMessage *aMessage, const void *aBuf, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aLength);
    assert(false);
    return OT_ERROR_NO_BUFS;
}

OT_TOOL_WEAK uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aOffset);
    OT_UNUSED_VARIABLE(aBuf);
    OT_UNUSED_VARIABLE(aLength);
    assert(false);
    return 0;
}

OT_TOOL_WEAK const otIp6Address *otThreadGetMeshLocalEid(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    assert(false);
    return NULL;
}

OT_TOOL_WEAK void otIp6SockAddrToString(const otSockAddr *aSockAddr, char *aBuffer, uint16_t aSize)
{
    OT_UNUSED_VARIABLE(aSockAddr);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aSize);
    assert(false);
}

#endif // OPENTHREAD_CONFIG_DNS_DSO_ENABLE
