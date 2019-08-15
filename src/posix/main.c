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

#include <openthread/config.h>

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#define OPENTHREAD_POSIX_APP_TYPE_NCP 1
#define OPENTHREAD_POSIX_APP_TYPE_CLI 2

#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/platform/radio.h>
#if OPENTHREAD_POSIX_APP_TYPE == OPENTHREAD_POSIX_APP_TYPE_NCP
#include <openthread/ncp.h>
#elif OPENTHREAD_POSIX_APP_TYPE == OPENTHREAD_POSIX_APP_TYPE_CLI
#include <openthread/cli.h>
#if (HAVE_LIBEDIT || HAVE_LIBREADLINE) && !OPENTHREAD_ENABLE_POSIX_APP_DAEMON
#define OPENTHREAD_USE_CONSOLE 1
#include "console_cli.h"
#endif
#else
#error "Unknown posix app type!"
#endif
#include <openthread-system.h>

static jmp_buf gResetJump;

void __gcov_flush();

static const struct option kOptions[] = {{"dry-run", no_argument, NULL, 'n'},
                                         {"help", no_argument, NULL, 'h'},
                                         {"interface-name", required_argument, NULL, 'I'},
                                         {"no-reset", no_argument, NULL, 0},
                                         {"radio-version", no_argument, NULL, 0},
                                         {"time-speed", required_argument, NULL, 's'},
                                         {"verbose", no_argument, NULL, 'v'},
                                         {0, 0, 0, 0}};

static void PrintUsage(const char *aProgramName, FILE *aStream, int aExitCode)
{
    fprintf(aStream,
            "Syntax:\n"
            "    %s [Options] NodeId|Device|Command [DeviceConfig|CommandArgs]\n"
            "Options:\n"
            "    -I  --interface-name name   Thread network interface name.\n"
            "    -n  --dry-run               Just verify if arguments is valid and radio spinel is compatible.\n"
            "        --no-reset              Do not reset RCP on initialization\n"
            "        --radio-version         Print radio firmware version\n"
            "    -s  --time-speed factor     Time speed up factor.\n"
            "    -v  --verbose               Also log to stderr.\n"
            "    -h  --help                  Display this usage information.\n",
            aProgramName);
    exit(aExitCode);
}

static otInstance *InitInstance(int aArgCount, char *aArgVector[])
{
    otPlatformConfig config;
    otInstance *     instance          = NULL;
    bool             isDryRun          = false;
    bool             printRadioVersion = false;
    bool             isVerbose         = false;

    memset(&config, 0, sizeof(config));

    config.mSpeedUpFactor = 1;
    config.mResetRadio    = true;

    optind = 1;

    while (true)
    {
        int index  = 0;
        int option = getopt_long(aArgCount, aArgVector, "hI:ns:v", kOptions, &index);

        if (option == -1)
        {
            break;
        }

        switch (option)
        {
        case 'h':
            PrintUsage(aArgVector[0], stdout, OT_EXIT_SUCCESS);
            break;
        case 'I':
            config.mInterfaceName = optarg;
            break;
        case 'n':
            isDryRun = true;
            break;
        case 's':
        {
            char *endptr          = NULL;
            config.mSpeedUpFactor = (uint32_t)strtol(optarg, &endptr, 0);

            if (*endptr != '\0' || config.mSpeedUpFactor == 0)
            {
                fprintf(stderr, "Invalid value for TimerSpeedUpFactor: %s\n", optarg);
                exit(OT_EXIT_INVALID_ARGUMENTS);
            }
            break;
        }
        case 'v':
            isVerbose = true;
            break;

        case 0:
            if (!strcmp(kOptions[index].name, "radio-version"))
            {
                printRadioVersion = true;
            }
            else if (!strcmp(kOptions[index].name, "no-reset"))
            {
                config.mResetRadio = false;
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

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
    openlog(aArgVector[0], LOG_PID | (isVerbose ? LOG_PERROR : 0), LOG_DAEMON);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_DEBUG));
#endif

    if (optind >= aArgCount)
    {
        PrintUsage(aArgVector[0], stderr, OT_EXIT_INVALID_ARGUMENTS);
    }

    config.mRadioFile = aArgVector[optind];

    if (optind + 1 < aArgCount)
    {
        config.mRadioConfig = aArgVector[optind + 1];
    }

    instance = otSysInit(&config);

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

void otTaskletsSignalPending(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatReset(otInstance *aInstance)
{
    otInstanceFinalize(aInstance);
    otSysDeinit();

    longjmp(gResetJump, 1);
    assert(false);
}

int main(int argc, char *argv[])
{
    otInstance *instance;

#ifdef __linux__
    // Ensure we terminate this process if the
    // parent process dies.
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif

    if (setjmp(gResetJump))
    {
        alarm(0);
#if OPENTHREAD_ENABLE_COVERAGE
        __gcov_flush();
#endif
        execvp(argv[0], argv);
    }

    instance = InitInstance(argc, argv);

#if OPENTHREAD_POSIX_APP_TYPE == OPENTHREAD_POSIX_APP_TYPE_NCP
    otNcpInit(instance);
#elif OPENTHREAD_POSIX_APP_TYPE == OPENTHREAD_POSIX_APP_TYPE_CLI
#if OPENTHREAD_USE_CONSOLE
    otxConsoleInit(instance);
#else
    otCliUartInit(instance);
#endif
#endif

    while (true)
    {
        otSysMainloopContext mainloop;

        otTaskletsProcess(instance);

        FD_ZERO(&mainloop.mReadFdSet);
        FD_ZERO(&mainloop.mWriteFdSet);
        FD_ZERO(&mainloop.mErrorFdSet);

        mainloop.mMaxFd           = -1;
        mainloop.mTimeout.tv_sec  = 10;
        mainloop.mTimeout.tv_usec = 0;

#if OPENTHREAD_USE_CONSOLE
        otxConsoleUpdate(&mainloop);
#endif

        otSysMainloopUpdate(instance, &mainloop);

        if (otSysMainloopPoll(&mainloop) >= 0)
        {
            otSysMainloopProcess(instance, &mainloop);
#if OPENTHREAD_USE_CONSOLE
            otxConsoleProcess(&mainloop);
#endif
        }
        else if (errno != EINTR)
        {
            perror("select");
            exit(OT_EXIT_FAILURE);
        }
    }

#if OPENTHREAD_USE_CONSOLE
    otxConsoleDeinit();
#endif
    otInstanceFinalize(instance);
    otSysDeinit();

    return 0;
}
