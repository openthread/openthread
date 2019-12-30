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
#define MAX_SUPPORT_PROTOCOL 3
#define MAX_SUPPORT_PARAMETER 3
#define DEVICE_FILE_LEN 20

#include <openthread/diag.h>
#include <openthread/logging.h>
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

typedef struct PosixConfig
{
    otPlatformConfig mPlatformConfig;    ///< Platform configuration.
    otLogLevel       mLogLevel;          ///< Debug level of logging.
    bool             mIsDryRun;          ///< Dry run.
    bool             mPrintRadioVersion; ///< Whether to print radio firmware version.
    bool             mIsVerbose;         ///< Whether to print log to stderr.
} PosixConfig;

const char *sProtocol[]    = {"hdlc", "spi", "spinel", "uart", "forkpty"};
const char *sConnectPara[] = {"baudrate", "parity", "flowcontrol", "forkpty-arg"};

typedef struct RadioUrl
{
    const char *sProtocol[MAX_SUPPORT_PROTOCOL];
    char        device[DEVICE_FILE_LEN];
    const char *sParameter[MAX_SUPPORT_PARAMETER];
    char        sValue[DEVICE_FILE_LEN];
} RadioUrl;

static jmp_buf gResetJump;

void __gcov_flush();

static const struct option kOptions[] = {{"debug-level", required_argument, NULL, 'd'},
                                         {"dry-run", no_argument, NULL, 'n'},
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
            "    %s [Options] RadioURL\n"
            "    RadioURL: URL of the device and method for Thread core and RCP communication.\n"
            "              'protocols://device-file?connection-parameter1=value1&connection-parameter2=value2'\n"
            "              e.g. 'spinel+hdlc+uart:///dev/ttyUSB0?baudrate=115200'\n"
            "Options:\n"
            "    -I  --interface-name name   Thread network interface name.\n"
            "    -d  --debug-level           Debug level of logging.\n"
            "    -n  --dry-run               Just verify if arguments is valid and radio spinel is compatible.\n"
            "        --no-reset              Do not reset RCP on initialization\n"
            "        --radio-version         Print radio firmware version\n"
            "    -s  --time-speed factor     Time speed up factor.\n"
            "    -v  --verbose               Also log to stderr.\n"
            "    -h  --help                  Display this usage information.\n",
            aProgramName);
    exit(aExitCode);
}

static int ParseUrl(RadioUrl *info, char *url)
{
    char *deviceInfo = strstr(url, "://");
    char *pStart     = url;
    char *pEnd       = NULL;
    char *pMid       = NULL;
    int   i          = 0;
    int   j          = 0;
    int   cProtocol  = sizeof(sProtocol) / sizeof(sProtocol[0]);
    // int cParameter = sizeof(sConnectPara) / sizeof(sConnectPara[0]);
    memset(info, 0, sizeof(RadioUrl));

    if (!deviceInfo)
    {
        return -1;
    }

    while (pEnd != deviceInfo)
    {
        pEnd = strstr(pStart, "+");
        if (!pEnd)
        {
            pEnd = deviceInfo;
        }
        for (i = 0; i < cProtocol; i++)
        {
            if (strncmp(pStart, sProtocol[i], (int)(pEnd - pStart)) == 0)
            {
                info->sProtocol[j] = sProtocol[i];
                j++;
            }
        }

        pStart = pEnd + 1;
    }

    pStart = deviceInfo + 3;
    pEnd   = strstr(pStart, "?");
    if (pEnd)
    {
        strncpy(info->device, pStart, pEnd - pStart);
    }
    else
    {
        return -1;
    }

    pStart = pEnd + 1;
    while (pStart)
    {
        pEnd = strstr(pStart, "&");
        pMid = strstr(pStart, "=");
        if (!pEnd)
        {
            strcpy(info->sValue, pMid + 1);
            break;
        }
        pStart = pEnd + 1;
    }
    return 0;
}

static void ParseArg(int aArgCount, char *aArgVector[], PosixConfig *aConfig)
{
    memset(aConfig, 0, sizeof(PosixConfig));
    RadioUrl aUrl;

    aConfig->mPlatformConfig.mSpeedUpFactor = 1;
    aConfig->mPlatformConfig.mResetRadio    = true;
    aConfig->mLogLevel                      = OT_LOG_LEVEL_CRIT;

    optind = 1;

    while (true)
    {
        int index  = 0;
        int option = getopt_long(aArgCount, aArgVector, "d:hI:ns:v", kOptions, &index);

        if (option == -1)
        {
            break;
        }

        switch (option)
        {
        case 'd':
            aConfig->mLogLevel = (otLogLevel)atoi(optarg);
            break;
        case 'h':
            PrintUsage(aArgVector[0], stdout, OT_EXIT_SUCCESS);
            break;
        case 'I':
            aConfig->mPlatformConfig.mInterfaceName = optarg;
            break;
        case 'R':
            break;
        case 'n':
            aConfig->mIsDryRun = true;
            break;
        case 's': {
            char *endptr = NULL;

            aConfig->mPlatformConfig.mSpeedUpFactor = (uint32_t)strtol(optarg, &endptr, 0);

            if (*endptr != '\0' || aConfig->mPlatformConfig.mSpeedUpFactor == 0)
            {
                fprintf(stderr, "Invalid value for TimerSpeedUpFactor: %s\n", optarg);
                exit(OT_EXIT_INVALID_ARGUMENTS);
            }
            break;
        }
        case 'v':
            aConfig->mIsVerbose = true;
            break;

        case 0:
            if (!strcmp(kOptions[index].name, "radio-version"))
            {
                aConfig->mPrintRadioVersion = true;
            }
            else if (!strcmp(kOptions[index].name, "no-reset"))
            {
                aConfig->mPlatformConfig.mResetRadio = false;
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

    if (ParseUrl(&aUrl, aArgVector[optind]) < 0)
    {
        PrintUsage(aArgVector[0], stderr, OT_EXIT_INVALID_ARGUMENTS);
    }
    aConfig->mPlatformConfig.mRadioFile   = aUrl.device;
    aConfig->mPlatformConfig.mRadioConfig = aUrl.sValue;
    if (optind >= aArgCount)
    {
        PrintUsage(aArgVector[0], stderr, OT_EXIT_INVALID_ARGUMENTS);
    }
}

static otInstance *InitInstance(int aArgCount, char *aArgVector[])
{
    PosixConfig config;
    otInstance *instance = NULL;

    ParseArg(aArgCount, aArgVector, &config);

#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
    openlog(aArgVector[0], LOG_PID | (config.mIsVerbose ? LOG_PERROR : 0), LOG_DAEMON);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_DEBUG));
#endif

    instance = otSysInit(&config.mPlatformConfig);

    if (config.mPrintRadioVersion)
    {
        printf("%s\n", otPlatRadioGetVersionString(instance));
    }

    if (config.mIsDryRun)
    {
        exit(OT_EXIT_SUCCESS);
    }

    otLoggingSetLevel(config.mLogLevel);
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
