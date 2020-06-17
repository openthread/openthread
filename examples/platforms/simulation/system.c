/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#if OPENTHREAD_SIMULATION_VIRTUAL_TIME == 0

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

uint32_t gNodeId = 1;

extern bool        gPlatformPseudoResetWasRequested;
extern otRadioCaps gRadioCaps;

static volatile bool gTerminate = false;

static void handleSignal(int aSignal)
{
    OT_UNUSED_VARIABLE(aSignal);

    gTerminate = true;
}

void otSysInit(int aArgCount, char *aArgVector[])
{
    char *   endptr;
    uint32_t speedUpFactor = 1;
    int      argIndex      = 0;

    if (gPlatformPseudoResetWasRequested)
    {
        gPlatformPseudoResetWasRequested = false;
        return;
    }

    if (aArgCount < 2)
    {
        fprintf(stderr, "Syntax:\n    %s [--sleep-to-tx] NodeId [TimeSpeedUpFactor]\n", aArgVector[0]);
        exit(EXIT_FAILURE);
    }

    openlog(basename(aArgVector[argIndex++]), LOG_PID, LOG_USER);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_NOTICE));

    signal(SIGTERM, &handleSignal);
    signal(SIGHUP, &handleSignal);

    if (!strcmp(aArgVector[argIndex], "--sleep-to-tx"))
    {
        gRadioCaps |= OT_RADIO_CAPS_SLEEP_TO_TX;
        ++argIndex;
    }

    gNodeId = (uint32_t)strtol(aArgVector[argIndex], &endptr, 0);

    if (*endptr != '\0' || gNodeId < 1 || gNodeId >= WELLKNOWN_NODE_ID)
    {
        fprintf(stderr, "Invalid NodeId: %s\n", aArgVector[argIndex]);
        exit(EXIT_FAILURE);
    }

    ++argIndex;

    if (aArgCount > argIndex)
    {
        speedUpFactor = (uint32_t)strtol(aArgVector[argIndex], &endptr, 0);

        if (*endptr != '\0' || speedUpFactor == 0)
        {
            fprintf(stderr, "Invalid value for TimerSpeedUpFactor: %s\n", aArgVector[argIndex]);
            exit(EXIT_FAILURE);
        }
    }

    platformAlarmInit(speedUpFactor);
    platformRadioInit();
    platformRandomInit();
}

bool otSysPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void otSysDeinit(void)
{
    platformRadioDeinit();
}

void otSysProcessDrivers(otInstance *aInstance)
{
    fd_set         read_fds;
    fd_set         write_fds;
    fd_set         error_fds;
    int            max_fd = -1;
    struct timeval timeout;
    int            rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);

    platformUartUpdateFdSet(&read_fds, &write_fds, &error_fds, &max_fd);
    platformRadioUpdateFdSet(&read_fds, &write_fds, &max_fd);
    platformAlarmUpdateTimeout(&timeout);

    if (otTaskletsArePending(aInstance))
    {
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;
    }

    rval = select(max_fd + 1, &read_fds, &write_fds, &error_fds, &timeout);

    if (rval >= 0)
    {
        platformUartProcess();
        platformRadioProcess(aInstance, &read_fds, &write_fds);
    }
    else if (errno != EINTR)
    {
        perror("select");
        exit(EXIT_FAILURE);
    }

    platformAlarmProcess(aInstance);

    if (gTerminate)
    {
        exit(0);
    }
}

#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME == 0
