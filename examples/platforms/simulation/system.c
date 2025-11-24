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

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <openthread-select.h>
#include <openthread-system.h>

#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

#include "simul_utils.h"

uint32_t gNodeId = 1;

extern bool        gPlatformPseudoResetWasRequested;
extern otRadioCaps gRadioCaps;

static volatile bool gTerminate = false;

static void handleSignal(int aSignal)
{
    OT_UNUSED_VARIABLE(aSignal);

    gTerminate = true;
}

/**
 * Defines the argument return values.
 */
enum
{
    OT_SIM_OPT_HELP               = 'h',
    OT_SIM_OPT_ENABLE_ENERGY_SCAN = 'E',
    OT_SIM_OPT_LOCAL_INTERFACE    = 'L',
    OT_SIM_OPT_SLEEP_TO_TX        = 't',
    OT_SIM_OPT_TIME_SPEED         = 's',
    OT_SIM_OPT_LOG_FILE           = 'l',
    OT_SIM_OPT_UNKNOWN            = '?',
};

static void PrintUsage(const char *aProgramName, int aExitCode)
{
    fprintf(stderr,
            "Syntax:\n"
            "    %s [Options] NodeId\n"
            "Options:\n"
            "    -h --help                  Display this usage information.\n"
            "    -L --local-interface=val   The address or name of the netif to simulate Thread radio.\n"
            "    -E --enable-energy-scan    Enable energy scan capability.\n"
            "    -t --sleep-to-tx           Let radio support direct transition from sleep to TX with CSMA.\n"
            "    -s --time-speed=val        Speed up the time in simulation.\n"
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
        {"enable-energy-scan", no_argument, 0, OT_SIM_OPT_ENABLE_ENERGY_SCAN},
        {"sleep-to-tx", no_argument, 0, OT_SIM_OPT_SLEEP_TO_TX},
        {"time-speed", required_argument, 0, OT_SIM_OPT_TIME_SPEED},
        {"local-interface", required_argument, 0, OT_SIM_OPT_LOCAL_INTERFACE},
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
        {"log-file", required_argument, 0, OT_SIM_OPT_LOG_FILE},
#endif
        {0, 0, 0, 0},
    };

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
    static const char options[] = "Ehts:L:l:";
#else
    static const char options[] = "Ehts:L:";
#endif

    if (gPlatformPseudoResetWasRequested)
    {
        gPlatformPseudoResetWasRequested = false;
        return;
    }

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
        case OT_SIM_OPT_ENABLE_ENERGY_SCAN:
            gRadioCaps |= OT_RADIO_CAPS_ENERGY_SCAN;
            break;
        case OT_SIM_OPT_SLEEP_TO_TX:
            gRadioCaps |= OT_RADIO_CAPS_SLEEP_TO_TX;
            break;
        case OT_SIM_OPT_LOCAL_INTERFACE:
            gLocalInterface = optarg;
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

    if (optind != aArgCount - 1)
    {
        PrintUsage(aArgVector[0], EXIT_FAILURE);
    }

    gNodeId = (uint32_t)strtol(aArgVector[optind], &endptr, 0);

    if (*endptr != '\0' || gNodeId < 1 || gNodeId > MAX_NETWORK_SIZE)
    {
        fprintf(stderr, "Invalid NodeId: %s\n", aArgVector[optind]);
        exit(EXIT_FAILURE);
    }

    signal(SIGTERM, &handleSignal);
    signal(SIGHUP, &handleSignal);

    platformLoggingInit(basename(aArgVector[0]));
    platformAlarmInit(speedUpFactor);
    platformRadioInit();
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelInit(speedUpFactor);
#endif
#if OPENTHREAD_SIMULATION_IMPLEMENT_INFRA_IF && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    platformInfraIfInit();
#endif
    platformRandomInit();
}

bool otSysPseudoResetWasRequested(void) { return gPlatformPseudoResetWasRequested; }

void otSysDeinit(void)
{
    platformRadioDeinit();
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelDeinit();
#endif
#if OPENTHREAD_SIMULATION_IMPLEMENT_INFRA_IF && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    platformInfraIfDeinit();
#endif
    platformLoggingDeinit();
}

void otSysProcessDrivers(otInstance *aInstance)
{
    fd_set         readFdSet;
    fd_set         writeFdSet;
    fd_set         errorFdSet;
    int            maxFd = -1;
    struct timeval timeout;

    FD_ZERO(&readFdSet);
    FD_ZERO(&writeFdSet);
    FD_ZERO(&errorFdSet);

    otSysUpdateEvents(aInstance, &maxFd, &readFdSet, &writeFdSet, &errorFdSet, &timeout);

    if (select(maxFd + 1, &readFdSet, &writeFdSet, &errorFdSet, &timeout) < 0)
    {
        if (errno != EINTR)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        FD_ZERO(&readFdSet);
        FD_ZERO(&writeFdSet);
        FD_ZERO(&errorFdSet);
    }

    otSysProcessEvents(aInstance, &readFdSet, &writeFdSet, &errorFdSet);
}

void otSysUpdateEvents(otInstance     *aInstance,
                       int            *aMaxFd,
                       fd_set         *aReadFdSet,
                       fd_set         *aWriteFdSet,
                       fd_set         *aErrorFdSet,
                       struct timeval *aTimeout)
{
    OT_UNUSED_VARIABLE(aErrorFdSet);

#if OPENTHREAD_SIMULATION_UART_ENABLE
    platformUartUpdateFdSet(aReadFdSet, aWriteFdSet, aErrorFdSet, aMaxFd);
#endif
    platformAlarmUpdateTimeout(aTimeout);
    platformRadioUpdateFdSet(aReadFdSet, aWriteFdSet, aTimeout, aMaxFd);
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelUpdateFdSet(aReadFdSet, aWriteFdSet, aTimeout, aMaxFd);
#endif
#if OPENTHREAD_SIMULATION_IMPLEMENT_INFRA_IF && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    platformInfraIfUpdateFdSet(aReadFdSet, aWriteFdSet, aMaxFd);
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_SIMULATION_MDNS_SOCKET_IMPLEMENT_POSIX
    platformMdnsSocketUpdateFdSet(aReadFdSet, aMaxFd);
#endif

#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    platformBleUpdateFdSet(aReadFdSet, aWriteFdSet, aTimeout, aMaxFd);
#endif
    if (otTaskletsArePending(aInstance))
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }
}

void otSysProcessEvents(otInstance   *aInstance,
                        const fd_set *aReadFdSet,
                        const fd_set *aWriteFdSet,
                        const fd_set *aErrorFdSet)
{
    OT_UNUSED_VARIABLE(aErrorFdSet);

#if OPENTHREAD_SIMULATION_UART_ENABLE
    platformUartProcess();
#endif
    platformRadioProcess(aInstance, aReadFdSet, aWriteFdSet);
#if OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
    platformBleProcess(aInstance, aReadFdSet, aWriteFdSet);
#endif

    platformAlarmProcess(aInstance);
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelProcess(aInstance, aReadFdSet, aWriteFdSet);
#endif
#if OPENTHREAD_SIMULATION_IMPLEMENT_INFRA_IF && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    platformInfraIfProcess(aInstance, aReadFdSet, aWriteFdSet);
#endif
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_SIMULATION_MDNS_SOCKET_IMPLEMENT_POSIX
    platformMdnsSocketProcess(aInstance, aReadFdSet);
#endif
    if (gTerminate)
    {
        exit(0);
    }
}

#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME == 0
