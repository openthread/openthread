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

#include "platform-posix.h"

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

uint64_t NODE_ID = 0;

extern bool gPlatformPseudoResetWasRequested;

int    gArgumentsCount = 0;
char **gArguments      = NULL;

void PrintUsage(const char *aArg0)
{
    fprintf(stderr, "Syntax:\n    %s [-s TimeSpeedUpFactor] {NodeId|Device DeviceConfig|Command CommandArgs}\n", aArg0);
    exit(EXIT_FAILURE);
}

void PlatformInit(int aArgCount, char *aArgVector[])
{
    int         i;
    uint32_t    speedUpFactor = 1;
    char *      endptr;
    const char *radioFile   = NULL;
    const char *radioConfig = "";

    if (gPlatformPseudoResetWasRequested)
    {
        gPlatformPseudoResetWasRequested = false;
        return;
    }

    if (aArgCount < 2)
    {
        PrintUsage(aArgVector[0]);
    }

    i = 1;
    if (!strcmp(aArgVector[i], "-s"))
    {
        ++i;
        speedUpFactor = (uint32_t)strtol(aArgVector[i], &endptr, 0);

        if (*endptr != '\0' || speedUpFactor == 0)
        {
            fprintf(stderr, "Invalid value for TimerSpeedUpFactor: %s\n", aArgVector[i]);
            exit(EXIT_FAILURE);
        }
        ++i;
    }

    if (i >= aArgCount)
    {
        PrintUsage(aArgVector[0]);
    }

    radioFile = aArgVector[i];
    if (i + 1 < aArgCount)
    {
        radioConfig = aArgVector[i + 1];
    }

    openlog(basename(aArgVector[0]), LOG_PID, LOG_USER);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_NOTICE));

    gArgumentsCount = aArgCount;
    gArguments      = aArgVector;

    platformAlarmInit(speedUpFactor);
    platformRadioInit(radioFile, radioConfig);
    platformRandomInit();
}

bool PlatformPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void PlatformDeinit(void)
{
    platformRadioDeinit();
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    fd_set         read_fds;
    fd_set         write_fds;
    fd_set         error_fds;
    struct timeval timeout;
    int            max_fd = -1;
    int            rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);

    platformAlarmUpdateTimeout(&timeout);
    platformUartUpdateFdSet(&read_fds, &write_fds, &error_fds, &max_fd);
    platformRadioUpdateFdSet(&read_fds, &write_fds, &max_fd, &timeout);

    if (otTaskletsArePending(aInstance))
    {
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;
    }

    rval = select(max_fd + 1, &read_fds, &write_fds, &error_fds, &timeout);

    if ((rval < 0) && (errno != EINTR))
    {
        perror("select");
        exit(EXIT_FAILURE);
    }

    platformUartProcess(&read_fds, &write_fds, &error_fds);
    platformRadioProcess(aInstance, &read_fds, &write_fds);
    platformAlarmProcess(aInstance);
}
