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
#include <getopt.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

#include "file_logging.h"

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
    uint32_t speedUpFactor = 1;
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
    char logFileName[128];
    memset(logFileName, 0, sizeof(logFileName));
#endif

    if (gPlatformPseudoResetWasRequested)
    {
        gPlatformPseudoResetWasRequested = false;
        return;
    }

    struct option long_options[] =
    {
        {"sleep-to-tx", no_argument, 0, 's'},
        {"time-speed-up-factor", required_argument, 0, 't'},
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
        {"log-file", required_argument, 0, 'l'},
#endif
    };

    while (true)
    {
        int c = getopt_long(aArgCount, aArgVector, "", long_options, NULL);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 's':
            gRadioCaps |= OT_RADIO_CAPS_SLEEP_TO_TX;
            break;
        case 't':
            speedUpFactor = (uint32_t)strtol(optarg, NULL, 10);
            break;
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
        case 'l':
            if (strlen(optarg) > sizeof(logFileName))
            {
                fprintf(stderr, "Log file name too long\n");
                exit(EXIT_FAILURE);
            }
            sprintf(logFileName, "%s", optarg);
            break;
#endif
        case '?':
            fprintf(stderr, "Syntax:\n    %s [--sleep-to-tx] [--time-speed-up-factor=val] [--log-file=name] NodeId \n",
                    aArgVector[0]);
            exit(EXIT_FAILURE);
            break;
        default:
            break;
        }
    }

    if (optind != aArgCount - 1)
    {
        fprintf(stderr, "Syntax:\n    %s [--sleep-to-tx] [--time-speed-up-factor=val] [--log-file=name] NodeId \n",
                aArgVector[0]);
        exit(EXIT_FAILURE);
    }

    gNodeId = (uint32_t)strtol(aArgVector[optind], NULL, 10);

    if (gNodeId < 1 || gNodeId > MAX_NETWORK_SIZE)
    {
        fprintf(stderr, "Invalid NodeId: %s\n", aArgVector[optind]);
        exit(EXIT_FAILURE);
    }

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
    openlog(basename(aArgVector[0]), LOG_PID, LOG_USER);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_NOTICE));
#elif OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
    if (strlen(logFileName) == 0)
    {
        sprintf(logFileName, "OpenThread-Node-%d.log", gNodeId);
    }

    if (!initLogFile(logFileName))
    {
        fprintf(stderr, "Init log file [%s] failed\n", logFileName);
        exit(EXIT_FAILURE);
    }
#endif

    signal(SIGTERM, &handleSignal);
    signal(SIGHUP, &handleSignal);

    platformAlarmInit(speedUpFactor);
    platformRadioInit();
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelInit(speedUpFactor);
#endif
    platformRandomInit();
}

bool otSysPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void otSysDeinit(void)
{
    platformRadioDeinit();
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelDeinit();
#endif
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
    deinitLogFile();
#endif
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
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelUpdateFdSet(&read_fds, &write_fds, &timeout, &max_fd);
#endif

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
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelProcess(aInstance, &read_fds, &write_fds);
#endif

    if (gTerminate)
    {
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_FILE
        deinitLogFile();
#endif
        exit(0);
    }
}

#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME == 0
