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
 *   This file implements the posix simulation.
 */

#include "openthread-core-config.h"
#include "platform-posix.h"

#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#if OPENTHREAD_POSIX_VIRTUAL_TIME

static const int kWellKnownNodeId = 34;      ///< Well-known ID used by a simulated radio supporting promiscuous mode.
static const int kBasePort        = 18000;   ///< This base port for posix app simulation.
static const int kUsPerSecond     = 1000000; ///< Number of microseconds per second.

static uint64_t sNow        = 0;  ///< Time of simulation.
static int      sSockFd     = -1; ///< Socket used to communicating with simulator.
static uint16_t sPortOffset = 0;  ///< Port offset for simulation.
static int      sNodeId     = 0;  ///< Node id of this simulated device.

void otSimInit(void)
{
    struct sockaddr_in sockaddr;
    char *             offset;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;

    offset = getenv("PORT_OFFSET");

    if (offset)
    {
        char *endptr;

        sPortOffset = (uint16_t)strtol(offset, &endptr, 0);

        if (*endptr != '\0')
        {
            fprintf(stderr, "Invalid PORT_OFFSET: %s\n", offset);
            exit(OT_EXIT_INVALID_ARGUMENTS);
        }

        sPortOffset *= kWellKnownNodeId;
    }

    // node id is required for virtual time simulation
    sNodeId = atoi(getenv("NODE_ID"));

    sockaddr.sin_port        = htons(kBasePort + sPortOffset + sNodeId);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    sSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sSockFd == -1)
    {
        perror("socket");
        exit(OT_EXIT_FAILURE);
    }

    if (bind(sSockFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
    {
        perror("bind");
        exit(OT_EXIT_FAILURE);
    }
    // fprintf(stderr, "[%s] sim fd is %d\r\n", getenv("NODE_ID"), sSockFd);
}

void otSimDeinit(void)
{
    if (sSockFd != -1)
    {
        close(sSockFd);
        sSockFd = -1;
    }
}

static void otSimSendEvent(struct Event *aEvent, size_t aLength)
{
    ssize_t            rval;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);
    sockaddr.sin_port = htons(9000 + sPortOffset);

    rval = sendto(sSockFd, aEvent, aLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    if (rval < 0)
    {
        perror("sendto");
        exit(OT_EXIT_FAILURE);
    }
}

void otSimReceiveEvent(struct Event *aEvent)
{
    ssize_t rval = recvfrom(sSockFd, aEvent, sizeof(*aEvent), 0, NULL, NULL);

    if (rval < 0 || (uint16_t)rval < offsetof(struct Event, mData))
    {
        perror("recvfrom");
        exit(OT_EXIT_FAILURE);
    }

    sNow += aEvent->mDelay;

    // fprintf(stderr, "--[%s] now is %lu type is %d\n", getenv("NODE_ID"), sNow, aEvent->mEvent);
}

void otSimSendSleepEvent(const struct timeval *aTimeout)
{
    struct Event event;

    event.mDelay      = (uint64_t)aTimeout->tv_sec * kUsPerSecond + (uint64_t)aTimeout->tv_usec;
    event.mEvent      = OT_SIM_EVENT_ALARM_FIRED;
    event.mDataLength = 0;

    otSimSendEvent(&event, offsetof(struct Event, mData));
}

void otSimSendRadioSpinelWriteEvent(const uint8_t *aData, uint16_t aLength)
{
    struct Event event;

    event.mDelay      = 0;
    event.mEvent      = OT_SIM_EVENT_RADIO_SPINEL_WRITE;
    event.mDataLength = aLength;

    memcpy(event.mData, aData, aLength);

    otSimSendEvent(&event, offsetof(struct Event, mData) + event.mDataLength);
}

void otSimUpdateFdSet(fd_set *        aReadFdSet,
                      fd_set *        aWriteFdSet,
                      fd_set *        aErrorFdSet,
                      int *           aMaxFd,
                      struct timeval *aTimeout)
{
    OT_UNUSED_VARIABLE(aWriteFdSet);
    OT_UNUSED_VARIABLE(aErrorFdSet);

    FD_SET(sSockFd, aReadFdSet);
    if (*aMaxFd < sSockFd)
    {
        *aMaxFd = sSockFd;
    }

    otSimRadioSpinelUpdate(aTimeout);
}

void otSimProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet, const fd_set *aErrorFdSet)
{
    struct Event event = {0};

    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aWriteFdSet);
    OT_UNUSED_VARIABLE(aErrorFdSet);

    if (FD_ISSET(sSockFd, aReadFdSet))
    {
        otSimReceiveEvent(&event);
        // fprintf(stderr, "[%s] receive event\r\n", getenv("NODE_ID"));
    }

    otSimRadioSpinelProcess(aInstance, &event);
}

void otSimGetTime(struct timeval *aTime)
{
    aTime->tv_sec  = (time_t)sNow / kUsPerSecond;
    aTime->tv_usec = sNow % kUsPerSecond;
}

#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
