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
#include <getopt.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>

uint64_t gNodeId = 0;

static void PrintUsage(const char *aProgramName, FILE *aStream, int aExitCode)
{
    fprintf(aStream,
            "Syntax:\n"
            "    %s [Options] NodeId|Device|Command [DeviceConfig|CommandArgs]\n"
            "Options:\n"
            "    -n  --dry-run               Just verify if arguments is valid and radio spinel is compatible.\n"
            "        --radio-version         Print radio firmware version\n"
            "    -s  --time-speed factor     Time speed up factor.\n"
            "    -h  --help                  Display this usage information.\n",
            aProgramName);
    exit(aExitCode);
}

otInstance *otSysInit(int aArgCount, char *aArgVector[])
{
    const char *radioFile         = NULL;
    const char *radioConfig       = "";
    otInstance *instance          = NULL;
    uint32_t    speedUpFactor     = 1;
    bool        isDryRun          = false;
    bool        printRadioVersion = false;

    while (true)
    {
        int                 index = 0;
        int                 option;
        const struct option options[] = {{"dry-run", no_argument, NULL, 'n'},
                                         {"radio-version", no_argument, NULL, 0},
                                         {"help", no_argument, NULL, 'h'},
                                         {"time-speed", required_argument, NULL, 's'},
                                         {0, 0, 0, 0}};

        option = getopt_long(aArgCount, aArgVector, "hns:", options, &index);

        if (option == -1)
        {
            break;
        }

        switch (option)
        {
        case 'h':
            PrintUsage(aArgVector[0], stdout, OT_EXIT_SUCCESS);
            break;
        case 'n':
            isDryRun = true;
            break;
        case 's':
        {
            char *endptr  = NULL;
            speedUpFactor = (uint32_t)strtol(optarg, &endptr, 0);

            if (*endptr != '\0' || speedUpFactor == 0)
            {
                fprintf(stderr, "Invalid value for TimerSpeedUpFactor: %s\n", optarg);
                exit(OT_EXIT_INVALID_ARGUMENTS);
            }
            break;
        }
        case 0:
            if (!strcmp(options[index].name, "radio-version"))
            {
                printRadioVersion = true;
            }
            break;
        case '?':
            PrintUsage(aArgVector[0], stderr, OT_EXIT_INVALID_ARGUMENTS);
            break;
        default:
            assert(false);
            break;
        }
    }

    if (optind >= aArgCount)
    {
        PrintUsage(aArgVector[0], stderr, OT_EXIT_INVALID_ARGUMENTS);
    }

    radioFile = aArgVector[optind];
    if (optind + 1 < aArgCount)
    {
        radioConfig = aArgVector[optind + 1];
    }

    platformLoggingInit(basename(aArgVector[0]));

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    otSimInit();
#endif
    platformAlarmInit(speedUpFactor);
    platformRadioInit(radioFile, radioConfig);
    platformRandomInit();
#if OPENTHREAD_ENABLE_PLATFORM_UDP
    platformUdpInit();
#endif

    instance = otInstanceInitSingle();
    assert(instance);

    if (printRadioVersion)
    {
        printf("%s\n", otPlatRadioGetVersionString(instance));
    }

    if (isDryRun)
    {
        exit(OT_EXIT_SUCCESS);
    }

    return instance;
}

void otSysDeinit(void)
{
#if OPENTHREAD_POSIX_VIRTUAL_TIME
    otSimDeinit();
#endif
    platformRadioDeinit();
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME
/**
 * This function try selecting the given file descriptors in nonblocking mode.
 *
 * @param[inout]    aReadFdSet   A pointer to the read file descriptors.
 * @param[inout]    aWriteFdSet  A pointer to the write file descriptors.
 * @param[inout]    aErrorFdSet  A pointer to the error file descriptors.
 * @param[in]       aMaxFd       The max file descriptor.
 *
 * @returns The value returned from select().
 *
 */
static int trySelect(fd_set *aReadFdSet, fd_set *aWriteFdSet, fd_set *aErrorFdSet, int aMaxFd)
{
    struct timeval timeout          = {0, 0};
    fd_set         originReadFdSet  = *aReadFdSet;
    fd_set         originWriteFdSet = *aWriteFdSet;
    fd_set         originErrorFdSet = *aErrorFdSet;
    int            rval;

    rval = select(aMaxFd + 1, aReadFdSet, aWriteFdSet, aErrorFdSet, &timeout);

    if (rval == 0)
    {
        *aReadFdSet  = originReadFdSet;
        *aWriteFdSet = originWriteFdSet;
        *aErrorFdSet = originErrorFdSet;
    }

    return rval;
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

void otSysProcessDrivers(otInstance *aInstance)
{
    fd_set         readFdSet;
    fd_set         writeFdSet;
    fd_set         errorFdSet;
    struct timeval timeout;
    int            maxFd = -1;
    int            rval;

    FD_ZERO(&readFdSet);
    FD_ZERO(&writeFdSet);
    FD_ZERO(&errorFdSet);

    platformAlarmUpdateTimeout(&timeout);
    platformUartUpdateFdSet(&readFdSet, &writeFdSet, &errorFdSet, &maxFd);
#if OPENTHREAD_ENABLE_PLATFORM_UDP
    platformUdpUpdateFdSet(aInstance, &readFdSet, &maxFd);
#endif
#if OPENTHREAD_POSIX_VIRTUAL_TIME
    otSimUpdateFdSet(&readFdSet, &writeFdSet, &errorFdSet, &maxFd, &timeout);
#else
    platformRadioUpdateFdSet(&readFdSet, &writeFdSet, &maxFd, &timeout);
#endif

    if (otTaskletsArePending(aInstance))
    {
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;
    }

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    if (timerisset(&timeout))
    {
        // Make sure there are no data ready in UART
        rval = trySelect(&readFdSet, &writeFdSet, &errorFdSet, maxFd);

        if (rval == 0)
        {
            bool noWrite = true;

            // If there are write requests, the device is supposed to wake soon
            for (int i = 0; i < maxFd + 1; ++i)
            {
                if (FD_ISSET(i, &writeFdSet))
                {
                    noWrite = false;
                    break;
                }
            }

            if (noWrite)
            {
                otSimSendSleepEvent(&timeout);
            }

            rval = select(maxFd + 1, &readFdSet, &writeFdSet, &errorFdSet, NULL);
            assert(rval > 0);
        }
    }
    else
#endif
    {
        rval = select(maxFd + 1, &readFdSet, &writeFdSet, &errorFdSet, &timeout);
    }

    if ((rval < 0) && (errno != EINTR))
    {
        perror("select");
        exit(OT_EXIT_FAILURE);
    }

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    otSimProcess(aInstance, &readFdSet, &writeFdSet, &errorFdSet);
#else
    platformRadioProcess(aInstance, &readFdSet, &writeFdSet);
#endif
    platformUartProcess(&readFdSet, &writeFdSet, &errorFdSet);
    platformAlarmProcess(aInstance);
#if OPENTHREAD_ENABLE_PLATFORM_UDP
    platformUdpProcess(aInstance, &readFdSet);
#endif
}
