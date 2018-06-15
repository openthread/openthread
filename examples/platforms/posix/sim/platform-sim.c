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
 *   This file includes the platform-specific initializers.
 */

#include "platform-posix.h"

#if OPENTHREAD_POSIX_VIRTUAL_TIME

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <openthread/openthread.h>
#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>

uint32_t NODE_ID           = 1;
uint32_t WELLKNOWN_NODE_ID = 34;

extern bool gPlatformPseudoResetWasRequested;

int    gArgumentsCount = 0;
char **gArguments      = NULL;

int      sSockFd;
uint16_t sPortOffset;

static void receiveEvent(otInstance *aInstance)
{
    struct Event event;
    ssize_t      rval = recvfrom(sSockFd, (char *)&event, sizeof(event), 0, NULL, NULL);

    if (rval < 0 || (uint16_t)rval < offsetof(struct Event, mData))
    {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    platformAlarmAdvanceNow(event.mDelay);

    switch (event.mEvent)
    {
    case OT_SIM_EVENT_ALARM_FIRED:
        break;

    case OT_SIM_EVENT_RADIO_RECEIVED:
        platformRadioReceive(aInstance, event.mData, event.mDataLength);
        break;
    }
}

static bool processEvent(otInstance *aInstance)
{
    const int     flags  = POLLIN | POLLRDNORM | POLLERR | POLLNVAL | POLLHUP;
    struct pollfd pollfd = {sSockFd, flags, 0};
    bool          rval   = false;

    if (POLL(&pollfd, 1, 0) > 0 && (pollfd.revents & flags) != 0)
    {
        receiveEvent(aInstance);
        rval = true;
    }

    return rval;
}

static void platformSendSleepEvent(void)
{
    struct sockaddr_in sockaddr;
    struct Event       event;
    ssize_t            rval;

    assert(platformAlarmGetNext() > 0);

    event.mDelay      = (uint64_t)platformAlarmGetNext();
    event.mEvent      = OT_SIM_EVENT_ALARM_FIRED;
    event.mDataLength = 0;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);
    sockaddr.sin_port = htons(9000 + sPortOffset);

    rval = sendto(sSockFd, (const char *)&event, offsetof(struct Event, mData), 0, (struct sockaddr *)&sockaddr,
                  sizeof(sockaddr));

    if (rval < 0)
    {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

static void socket_init(void)
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
            exit(EXIT_FAILURE);
        }

        sPortOffset *= WELLKNOWN_NODE_ID;
    }

    sockaddr.sin_port        = htons(9000 + sPortOffset + NODE_ID);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    sSockFd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sSockFd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(sSockFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

void PlatformInit(int argc, char *argv[])
{
    char *endptr;

    if (gPlatformPseudoResetWasRequested)
    {
        gPlatformPseudoResetWasRequested = false;
        return;
    }

    if (argc != 2)
    {
        exit(EXIT_FAILURE);
    }

    openlog(basename(argv[0]), LOG_PID, LOG_USER);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_NOTICE));

    gArgumentsCount = argc;
    gArguments      = argv;

    NODE_ID = (uint32_t)strtol(argv[1], &endptr, 0);

    if (*endptr != '\0' || NODE_ID < 1 || NODE_ID >= WELLKNOWN_NODE_ID)
    {
        fprintf(stderr, "Invalid NODE_ID: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    socket_init();

    platformAlarmInit(1);
    platformRadioInit();
    platformRandomInit();
}

bool PlatformPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void PlatformDeinit(void)
{
    close(sSockFd);
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    fd_set read_fds;
    fd_set write_fds;
    fd_set error_fds;
    int    max_fd = -1;
    int    rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);

    FD_SET(sSockFd, &read_fds);
    max_fd = sSockFd;

    platformUartUpdateFdSet(&read_fds, &write_fds, &error_fds, &max_fd);

    if (!otTaskletsArePending(aInstance) && platformAlarmGetNext() > 0)
    {
        platformSendSleepEvent();

        rval = select(max_fd + 1, &read_fds, &write_fds, &error_fds, NULL);

        if ((rval < 0) && (errno != EINTR))
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        processEvent(aInstance);
    }

    platformAlarmProcess(aInstance);
    platformRadioProcess(aInstance);
    platformUartProcess();
}

#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
