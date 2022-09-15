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

#include "platform-simulation.h"

#if OPENTHREAD_SIMULATION_VIRTUAL_TIME

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

#include "utils/uart.h"
#include "core/common/debug.hpp"

uint32_t gNodeId        = 1;
uint64_t gLastAlarmEventId = 0;

extern bool          gPlatformPseudoResetWasRequested;
static volatile bool gTerminate = false;

int    gArgumentsCount = 0;
char **gArguments      = NULL;

uint64_t sNow = 0; // microseconds
int      sSockFd;
uint16_t sPortOffset;

static void handleSignal(int aSignal)
{
    OT_UNUSED_VARIABLE(aSignal);

    gTerminate = true;
}

#define VERIFY_EVENT_SIZE(X) OT_ASSERT((uint16_t)rval >= (sizeof(X) + offsetof(struct Event, mData)) && "event payload smaller than: " && sizeof(X) );
static void receiveEvent(otInstance *aInstance)
{
    struct Event event;
    ssize_t      rval = recvfrom(sSockFd, (char *)&event, sizeof(event), 0, NULL, NULL);
    const uint8_t *evData = &event.mData[0];

    if (rval < 0 || (uint16_t)rval < offsetof(struct Event, mData))
    {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    platformAlarmAdvanceNow(event.mDelay);

    switch (event.mEvent)
    {
    case OT_SIM_EVENT_ALARM_FIRED:
        // store the optional msg id from payload
        if ((uint16_t)rval >= sizeof(gLastAlarmEventId) + offsetof(struct Event, mData))
        {
            memcpy(&gLastAlarmEventId, event.mData, sizeof(gLastAlarmEventId));
        }
        break;

    case OT_SIM_EVENT_UART_WRITE:
        otPlatUartReceived(event.mData, event.mDataLength);
        break;

    case OT_SIM_EVENT_RADIO_RECEIVED:
        platformRadioReceive(aInstance, event.mData, event.mDataLength, NULL);
        break;

    case OT_SIM_EVENT_RADIO_RX:
        VERIFY_EVENT_SIZE(struct RxEventData)
        const size_t sz = sizeof(struct RxEventData);
        platformRadioReceive(aInstance, evData + sz,
                       event.mDataLength - sz, (struct RxEventData *)evData);
        break;

    case OT_SIM_EVENT_RADIO_TX_DONE:
        VERIFY_EVENT_SIZE(struct TxDoneEventData)
        platformRadioTransmitDone(aInstance, (struct TxDoneEventData *)evData);
        break;

    default:
        OT_ASSERT(false && "Unrecognized event type received");
    }
}

#if OPENTHREAD_SIMULATION_VIRTUAL_TIME_UART
void platformUartRestore(void)
{
}

otError otPlatUartEnable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aData, uint16_t aLength)
{
    otError      error = OT_ERROR_NONE;
    struct Event event;

    event.mDelay      = 0;
    event.mEvent      = OT_SIM_EVENT_UART_WRITE;
    event.mDataLength = aLength;
    memcpy(event.mData, aData, aLength);

    otSimSendEvent(&event);
    otPlatUartSendDone();

    return error;
}

otError otPlatUartFlush(void)
{
    return OT_ERROR_NONE;
}
#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME_UART

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

        sPortOffset *= (OPENTHREAD_SIMULATION_MAX_NETWORK_SIZE + 1);
    }

    sockaddr.sin_port        = htons((uint16_t)(9000 + sPortOffset + gNodeId));
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    sSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

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

void otSysInit(int argc, char *argv[])
{
    char *endptr;

    if (gPlatformPseudoResetWasRequested)
    {
        gPlatformPseudoResetWasRequested = false;
        return;
    }

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <nodeNumber>\n", basename(argv[0]));
        exit(EXIT_FAILURE);
    }

    openlog(basename(argv[0]), LOG_PID, LOG_USER);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_NOTICE));

    gArgumentsCount = argc;
    gArguments      = argv;

    gNodeId = (uint32_t)strtol(argv[1], &endptr, 0);

    if (*endptr != '\0' || gNodeId < 1 || gNodeId > OPENTHREAD_SIMULATION_MAX_NETWORK_SIZE)
    {
        fprintf(stderr, "Invalid NodeId: %s (must be 1-%i)\n", argv[1], OPENTHREAD_SIMULATION_MAX_NETWORK_SIZE);
        exit(EXIT_FAILURE);
    }

    socket_init();

    platformAlarmInit(1);
    platformRadioInit();
    platformRandomInit();

    signal(SIGTERM, &handleSignal);
    signal(SIGHUP, &handleSignal);
}

bool otSysPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void otSysDeinit(void)
{
    close(sSockFd);
}

void otSysProcessDrivers(otInstance *aInstance)
{
    fd_set read_fds;
    fd_set write_fds;
    fd_set error_fds;
    int    max_fd = -1;
    int    rval;

    if (gTerminate)
    {
        exit(0);
    }

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);

    FD_SET(sSockFd, &read_fds);
    max_fd = sSockFd;

#if OPENTHREAD_SIMULATION_VIRTUAL_TIME_UART == 0
    platformUartUpdateFdSet(&read_fds, &write_fds, &error_fds, &max_fd);
#endif

    if (!otTaskletsArePending(aInstance) && platformAlarmGetNext() > 0 && !platformRadioIsTransmitPending())
    {
        otSimSendSleepEvent();

        rval = select(max_fd + 1, &read_fds, &write_fds, &error_fds, NULL);

        if ((rval < 0) && (errno != EINTR))
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (rval > 0 && FD_ISSET(sSockFd, &read_fds))
        {
            receiveEvent(aInstance);
        }
    }

    platformAlarmProcess(aInstance);
    platformRadioProcess(aInstance, &read_fds, &write_fds);
#if OPENTHREAD_SIMULATION_VIRTUAL_TIME_UART == 0
    platformUartProcess();
#endif
}

#if OPENTHREAD_CONFIG_OTNS_ENABLE

void otPlatOtnsStatus(const char *aStatus)
{
    struct Event event;
    uint16_t     statusLength = (uint16_t)strlen(aStatus);
    if (statusLength > sizeof(event.mData)){
        statusLength = sizeof(event.mData);
        assert(statusLength <= sizeof (event.mData));
    }

    memcpy(event.mData, aStatus, statusLength);
    event.mDataLength = statusLength;
    event.mDelay      = 0;
    event.mEvent      = OT_SIM_EVENT_OTNS_STATUS_PUSH;

    otSimSendEvent(&event);
}

#endif // OPENTHREAD_CONFIG_OTNS_ENABLE

#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME
