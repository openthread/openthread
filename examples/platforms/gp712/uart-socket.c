/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for cli over ip socket communication.
 *
 */

#include "platform_qorvo.h"
#include "alarm_qorvo.h"
#include "uart_qorvo.h"

#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <pthread.h>

#include <openthread/config.h>

#include <common/code_utils.hpp>
#include <openthread/platform/uart.h>

#include "utils/code_utils.h"

#define BUFFER_MAX_SIZE     255
#define SOCKET_PORT 9190
#define SOCKET_WRITE(socketInfo, buf, length)       sendto(socketInfo.socketId, (const char*)buf, length, 0, &socketInfo.addr, sizeof(socketInfo.addr))
#define SOCKET_READ(socketId, buf, length)          recv(socketId, buf, length, 0)

typedef struct
{
    uint32_t socketId;
    bool isValid;
    struct sockaddr addr;
    pthread_t rfReadThread;
} PlatSocket_t;

PlatSocket_t   PlatSocketConnection;
int PlatSocketPipeFd [2];
int PlatServerSocketId;

void PlatSocketRxNewConn(uint8_t id);
void PlatSocketInit(void);
void PlatSocketDeInit(void);
int PlatSocketTxData(uint16_t length, uint8_t *pData, uint32_t socketId);

#define PLAT_UART_MAX_CHAR 1024

uint32_t PlatSocketId = 0;

void PlatSocketSendInput(void *buffer)
{
    uint8_t  len = 0;
    uint8_t *buf = (uint8_t *) buffer;
    len = strlen((char *)buf);
    otPlatUartReceived((uint8_t *) buf, (uint16_t)len);
    free(buf);
    buf = 0;
    len = 0;
}

void PlatSocketRx(uint16_t length, const char *buffer, uint32_t socketId)
{
    uint8_t *buf = 0;
    PlatSocketId = socketId;

    if (length > 0)
    {
        buf = malloc(length + 2);
        memcpy(buf, buffer, length);
        buf[length]     = '\n';
        buf[length + 1] = 0;
        qorvoAlarmScheduleEventArg(0, PlatSocketSendInput, (void *) buf);
    }
}

void PlatSocketClose(uint32_t socketId)
{
    OT_UNUSED_VARIABLE(socketId);
}

otError otPlatUartEnable(void)
{
    PlatSocketInit();
    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    PlatSocketDeInit();
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    otError error = OT_ERROR_NONE;
    char    localbuf[PLAT_UART_MAX_CHAR];

    memcpy(localbuf, aBuf, aBufLength);
    localbuf[aBufLength] = 0;
    printf("%s", localbuf);

    if (PlatSocketId)
    {
        PlatSocketTxData(aBufLength, (uint8_t *) aBuf, PlatSocketId);
    }

    otPlatUartSendDone();
    return error;
}

void platformUartInit(void)
{
}

void platformUartProcess(void)
{
}

int PlatSocketListenForClients()
{
    //Setup server side socket
    int      sockfd;
    struct   sockaddr_in serv_addr;
    uint32_t flag = 1;
    int      ret;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    otEXPECT_ACTION(sockfd >= 0, sockfd = -1);

    // disable Nagle's algorithm to avoid long latency
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    // allow reuse of the same address
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SOCKET_PORT);
    ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    otEXPECT_ACTION(ret >= 0, close(sockfd); sockfd = -1);
    ret = listen(sockfd, 10);
    otEXPECT_ACTION(ret != -1, exit(1));
exit:
    return sockfd;
}

void PlatSocketRxSignaled(uint8_t id)
{
    OT_UNUSED_VARIABLE(id);

    //Dummy callback function to flush pipe
    uint8_t readChar;
    //Remove trigger byte from pipe
    read(PlatSocketPipeFd [0], &readChar, 1);
}

void *PlatSocketReadThread(void *pClientSocket)
{
    char buffer[BUFFER_MAX_SIZE];
    PlatSocket_t *clientSocket = ((PlatSocket_t *)pClientSocket);

    memset(buffer, 0, BUFFER_MAX_SIZE);

    while (1)
    {
        int readLen = SOCKET_READ(clientSocket->socketId, buffer, BUFFER_MAX_SIZE);

        if (readLen < 0)
        {
            perror("Reading socket");
            break;
        }
        else
        {
            if (readLen == 0)
            {
                break;
            }

            {
                uint8_t someByte = 0x12; //No functional use  only using pipe to kick main thread

                PlatSocketRx(readLen, buffer, clientSocket->socketId);

                write(PlatSocketPipeFd [1], &someByte, 1); //[1] = write fd
            }
        }
    }

    clientSocket->isValid = 0;
    qorvoPlatUnRegisterPollFunction(clientSocket->socketId);
    close(clientSocket->socketId);

    PlatSocketClose(clientSocket->socketId);

    return NULL;
}

void PlatSocketRxNewConn(uint8_t id)
{
    //Find first non-valid client in list - add here
    if (PlatSocketConnection.isValid == 0)
    {
        //Add new client to client list
        socklen_t len;
        len = sizeof(PlatSocketConnection.addr);
        int retval = accept(id, (struct sockaddr *)&PlatSocketConnection.addr, (socklen_t *) &len);

        if (retval >= 0)
        {
            int retErr;
            PlatSocketConnection.socketId = retval;
            retErr = pthread_create(&PlatSocketConnection.rfReadThread,
                                    NULL,
                                    PlatSocketReadThread,
                                    &PlatSocketConnection);

            if (retErr)
            {
                close(PlatSocketConnection.socketId);
            }
            else
            {
                PlatSocketConnection.isValid = 1;
            }
        }
    }
    else
    {

        int tempfd;
        tempfd = accept(id, (struct sockaddr *)NULL, NULL);

        if (tempfd >= 0)
        {
            close(tempfd);
        }
    }
}

/*****************************************************************************
 *                    Public Function Definitions
 *****************************************************************************/

void PlatSocketInit(void)
{
    memset(&PlatSocketConnection, 0, sizeof(PlatSocketConnection));

    // in case we are a server, setup listening for client
    PlatServerSocketId = PlatSocketListenForClients();
    qorvoPlatRegisterPollFunction(PlatServerSocketId, PlatSocketRxNewConn);

    // hack
    pipe(PlatSocketPipeFd);
    qorvoPlatRegisterPollFunction(PlatSocketPipeFd [0], PlatSocketRxSignaled);
}

void platformUartRestore(void)
{
    PlatSocketDeInit();
}

void PlatSocketDeInit(void)
{
    qorvoPlatUnRegisterPollFunction(PlatServerSocketId);
    close(PlatServerSocketId);
    qorvoPlatUnRegisterPollFunction(PlatSocketPipeFd[0]);
    close(PlatSocketPipeFd[0]);
    close(PlatSocketPipeFd[1]);
    close(PlatSocketConnection.socketId);
}

int PlatSocketTxData(uint16_t length, uint8_t *pData, uint32_t socketId)
{
    int result = -1;

    //All sockets
    if (PlatSocketConnection.isValid)
    {
        if (PlatSocketConnection.socketId == socketId)
        {
            if (SOCKET_WRITE(PlatSocketConnection, (const char *)pData, length) < 0)
            {
                perror("TxSocket: Error Writing to client");
                close(PlatSocketConnection.socketId);
                PlatSocketConnection.isValid = 0;
            }
            else
            {
                result = 0;
            }
        }
    }

    return result;
}

