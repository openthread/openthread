/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#include "platform.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

static volatile bool gTerminate = false;

static void HandleSignal(int aSignal)
{
    OT_UNUSED_VARIABLE(aSignal);
    gTerminate = true;
}

enum
{
    OT_SIM_OPT_HELP       = 'h',
    OT_SIM_OPT_TIME_SPEED = 's',
    OT_SIM_OPT_LOG_FILE   = 'l',
    OT_SIM_OPT_UNKNOWN    = '?',
};

static void PrintUsage(const char *aProgramName, int aExitCode)
{
    fprintf(stderr,
            "Syntax:\n"
            "    %s [Options]\n"
            "Options:\n"
            "    -h --help                  Display this usage information.\n"
            "    -s --time-speed=val        Speed up the time.\n"
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
            "    -l --log-file=name         File name to write logs.\n"
#endif
            ,
            aProgramName);

    exit(aExitCode);
}

void otSysInit(int aArgCount, char *aArgVector[])
{
    char    *endptr;
    uint32_t speedUpFactor = 1;

    static const struct option long_options[] = {
        {"help", no_argument, 0, OT_SIM_OPT_HELP},
        {"time-speed", required_argument, 0, OT_SIM_OPT_TIME_SPEED},
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
        {"log-file", required_argument, 0, OT_SIM_OPT_LOG_FILE},
#endif
        {0, 0, 0, 0},
    };

    static const char options[] = "hs:"
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
                                  "l:"
#endif
        ;

    optind = 1;

    while (true)
    {
        int c = getopt_long(aArgCount, aArgVector, options, long_options, NULL);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case OT_SIM_OPT_UNKNOWN:
            PrintUsage(aArgVector[0], EXIT_FAILURE);
            break;
        case OT_SIM_OPT_HELP:
            PrintUsage(aArgVector[0], EXIT_SUCCESS);
            break;
        case OT_SIM_OPT_TIME_SPEED:
            speedUpFactor = (uint32_t)strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || speedUpFactor == 0)
            {
                fprintf(stderr, "Invalid value for TimerSpeedUpFactor: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
        case OT_SIM_OPT_LOG_FILE:
            platformLoggingSetFileName(optarg);
            break;
#endif
        default:
            break;
        }
    }

    if (optind != aArgCount)
    {
        PrintUsage(aArgVector[0], EXIT_FAILURE);
    }

    signal(SIGTERM, &HandleSignal);
    signal(SIGHUP, &HandleSignal);

    platformLoggingInit(basename(aArgVector[0]));
    platformAlarmInit(speedUpFactor);
}

bool otSysPseudoResetWasRequested(void) { return false; }

void otSysDeinit(void) { platformLoggingDeinit(); }

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

    platformAlarmUpdateTimeout(&timeout);
    platformMdnsSocketUpdateFdSet(&read_fds, &max_fd);
    platformUartUpdateFdSet(&read_fds, &write_fds, &error_fds, &max_fd);

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

    platformUartProcess();
    platformAlarmProcess(aInstance);
    platformMdnsSocketProcess(aInstance, &read_fds);

    if (gTerminate)
    {
        exit(0);
    }
}
