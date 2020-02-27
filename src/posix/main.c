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

#include "platform/openthread-posix-config.h"

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

#ifndef HAVE_LIBEDIT
#define HAVE_LIBEDIT 0
#endif

#ifndef HAVE_LIBREADLINE
#define HAVE_LIBREADLINE 0
#endif

#define OPENTHREAD_POSIX_APP_TYPE_NCP 1
#define OPENTHREAD_POSIX_APP_TYPE_CLI 2

#include <openthread/diag.h>
#include <openthread/logging.h>
#include <openthread/tasklet.h>
#include <openthread/platform/radio.h>
#if OPENTHREAD_POSIX_APP_TYPE == OPENTHREAD_POSIX_APP_TYPE_NCP
#include <openthread/ncp.h>
#define OPENTHREAD_USE_CONSOLE 0
#elif OPENTHREAD_POSIX_APP_TYPE == OPENTHREAD_POSIX_APP_TYPE_CLI
#include <openthread/cli.h>
#if (HAVE_LIBEDIT || HAVE_LIBREADLINE) && !OPENTHREAD_ENABLE_POSIX_APP_DAEMON
#define OPENTHREAD_USE_CONSOLE 1
#include "console_cli.h"
#else
#define OPENTHREAD_USE_CONSOLE 0
#endif
#else
#error "Unknown posix app type!"
#endif
#include <openthread/openthread-system.h>

#ifndef OPENTHREAD_ENABLE_COVERAGE
#define OPENTHREAD_ENABLE_COVERAGE 0
#endif

typedef struct PosixConfig
{
    otPlatformConfig mPlatformConfig;    ///< Platform configuration.
    otLogLevel       mLogLevel;          ///< Debug level of logging.
    bool             mIsDryRun;          ///< Dry run.
    bool             mPrintRadioVersion; ///< Whether to print radio firmware version.
    bool             mIsVerbose;         ///< Whether to print log to stderr.
} PosixConfig;

const char *aUrlProtocols[] = {"hdlc", "spi", "spinel", "uart", "forkpty"};
const char *aUrlArgs[] = {"speed",           "cstopb",         "parity",          "flow", "arg",      "gpio-int-dev",
                          "gpio-int-line",   "gpio-reset-dev", "gpio-reset-line", "mode", "cs-delay", "reset-delay",
                          "align-allowance", "small-packet"};

static jmp_buf gResetJump;

void __gcov_flush();

/**
 * This enumeration defines the argument return values.
 *
 */
enum
{
    ARG_PRINT_RADIO_VERSION = 1001,
    ARG_NO_RADIO_RESET      = 1002,
    ARG_RESTORE_NCP_DATASET = 1003,
};

static const struct option kOptions[] = {{"debug-level", required_argument, NULL, 'd'},
                                         {"dry-run", no_argument, NULL, 'n'},
                                         {"help", no_argument, NULL, 'h'},
                                         {"interface-name", required_argument, NULL, 'I'},
                                         {"no-reset", no_argument, NULL, ARG_NO_RADIO_RESET},
                                         {"radio-version", no_argument, NULL, ARG_PRINT_RADIO_VERSION},
                                         {"ncp-dataset", no_argument, NULL, ARG_RESTORE_NCP_DATASET},
                                         {"time-speed", required_argument, NULL, 's'},
                                         {"verbose", no_argument, NULL, 'v'},
                                         {0, 0, 0, 0}};

static void PrintUsage(const char *aProgramName, FILE *aStream, int aExitCode)
{
    fprintf(aStream,
            "Syntax:\n"
            "    %s [Options] RadioURL\n"
            "    RadioURL: URL of the device and method for Thread core and RCP communication.\n"
            "              'protocols://device-file?arg1=value1&arg=value2'\n"
            "              e.g. 'spinel+hdlc+uart:///dev/ttyUSB0?speed=115200&parity=N'\n"
            "Options:\n"
            "    -d  --debug-level             Debug level of logging.\n"
            "    -h  --help                    Display this usage information.\n"
            "    -I  --interface-name name     Thread network interface name.\n"
            "    -n  --dry-run                 Just verify if arguments is valid and radio spinel is compatible.\n"
            "        --no-reset                Do not send Spinel reset command to RCP on initialization.\n"
            "        --radio-version           Print radio firmware version.\n"
            "        --ncp-dataset             Retrieve and save NCP dataset to file.\n"
            "    -s  --time-speed factor       Time speed up factor.\n"
            "    -v  --verbose                 Also log to stderr.\n"
            "    -h  --help                    Display this usage information.\n"
            "URL Arguments:\n"
#if OPENTHREAD_POSIX_RCP_SPI_ENABLE
            "    SPI Required:\n"
            "        gpio-int-dev=gpio-device-path\n"
            "                                  Specify a path to the Linux sysfs-exported GPIO device for the\n"
            "                                  `I̅N̅T̅` pin. If not specified, `SPI` interface will fall back to\n"
            "                                  polling, which is inefficient.\n"
            "        gpio-int-line=line-offset\n"
            "                                  The offset index of `I̅N̅T̅` pin for the associated GPIO device.\n"
            "                                  If not specified, `SPI` interface will fall back to polling,\n"
            "                                  which is inefficient.\n"
            "        gpio-reset-dev=gpio-device-path\n"
            "                                  Specify a path to the Linux sysfs-exported GPIO device for the\n"
            "                                  `R̅E̅S̅` pin.\n"
            "        gpio-reset-line=line-offset\n"
            "                                  The offset index of `R̅E̅S̅` pin for the associated GPIO device.\n"
            "    SPI Optional:\n"
            "        mode=mode             Specify the SPI mode to use (0-3). Default value is 0\n"
            "        speed=hertz           Specify the SPI speed in hertz. Default value is 1000000\n"
            "        cs-delay=usec         Specify the delay after C̅S̅ assertion, in µsec. Default value 20\n"
            "        reset-delay=ms        Specify the delay after R̅E̅S̅E̅T̅ assertion, in milliseconds. Default value "
            "is 0\n"
            "        align-allowance=n     Specify the maximum number of 0xFF bytes to clip from start of\n"
            "                                  MISO frame. Max value is 16. Default value is 16\n"
            "        small-packet=n        Specify the smallest packet we can receive in a single transaction.\n"
            "                                  (larger packets will require two transactions). Default value is 32.\n",
#else
            "    UART Optional:\n"
            "        speed=bps             Specify the UART input and output speed in bps. Default value is 115200\n"
            "        parity=N|E|O          Specify the parity option. Default value is N\n"
            "        cstopb=1|2            Specify the stop bits. Default value is 1\n"
            "        flow=N|H              Specify the flow control in milliseconds. Default value is N\n"
            "    ForkPty:\n"
            "        arg=value             Specify arguments for forkpty. Default value is arg=1\n",
#endif
            aProgramName);
    exit(aExitCode);
}

int ParseUrl(otRadioUrl *aRadioUrl, char *url)
{
    char *deviceInfo    = strstr(url, "://");
    char *pStart        = url;
    char *pEnd          = NULL;
    int   i             = 0;
    int   j             = 0;
    int   cUrlProtocols = sizeof(aUrlProtocols) / sizeof(aUrlProtocols[0]);
    int   cUrlArgs      = sizeof(aUrlArgs) / sizeof(aUrlArgs[0]);
    char  aArgName[OT_PLATFORM_CONFIG_URL_DEVICE_FILE_LEN];

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
        for (i = 0; i < cUrlProtocols; i++)
        {
            if (strncmp(pStart, aUrlProtocols[i], (size_t)strlen(aUrlProtocols[i])) == 0)
            {
                aRadioUrl->mProtocols[j] = aUrlProtocols[i];
                break;
            }
        }
        if (!aRadioUrl->mProtocols[j])
        {
            fprintf(stderr, "Invalid protocol\n\n");
            return -1;
        }
        j++;

        pStart = pEnd + 1;
    }

    if (sscanf(deviceInfo, "://%99[^?]", aRadioUrl->mDevice) != 1)
    {
        fprintf(stderr, "Invalid device format:%s\n\n", deviceInfo);
        return -1;
    }

    deviceInfo += 3 + strlen(aRadioUrl->mDevice);
    if (*deviceInfo != '?')
    {
        return 0;
    }
    deviceInfo += 1;

    for (i = 0; i < OT_PLATFORM_CONFIG_URL_MAX_ARGS; i++)
    {
        memset(aArgName, 0, sizeof(aArgName));
        if (sscanf(deviceInfo, "%99[^=]=%99[^&]", aArgName, aRadioUrl->mArgValue[i]) == 2)
        {
            for (j = 0; j < cUrlArgs; j++)
            {
                if (!strcmp(aArgName, aUrlArgs[j]))
                {
                    aRadioUrl->mArgName[i] = aUrlArgs[j];
                    break;
                }
            }
            if (!aRadioUrl->mArgName[i])
            {
                fprintf(stderr, "Invalid Argument Name: %s\n\n", aArgName);
                return -1;
            }
            deviceInfo += strlen(aArgName) + strlen(aRadioUrl->mArgValue[i]) + 1;
            if (*deviceInfo == '\0')
            {
                break;
            }
            deviceInfo += 1;
        }
        else
        {
            fprintf(stderr, "Invalid Argument Format: %s\n\n", deviceInfo);
            return -1;
        }
    }
    return 0;
}

static void ParseArg(int aArgCount, char *aArgVector[], PosixConfig *aConfig)
{
    memset(aConfig, 0, sizeof(*aConfig));

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
        case 'n':
            aConfig->mIsDryRun = true;
            break;
        case 's':
        {
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
        case ARG_PRINT_RADIO_VERSION:
            aConfig->mPrintRadioVersion = true;
            break;
        case ARG_NO_RADIO_RESET:
            aConfig->mPlatformConfig.mResetRadio = false;
            break;
        case ARG_RESTORE_NCP_DATASET:
            aConfig->mPlatformConfig.mRestoreDatasetFromNcp = true;
            break;
        case '?':
            PrintUsage(aArgVector[0], stderr, OT_EXIT_INVALID_ARGUMENTS);
            break;
        default:
            assert(false);
            break;
        }
    }

    if (ParseUrl(&(aConfig->mPlatformConfig.mRadioUrl), aArgVector[optind]) < 0)
    {
        PrintUsage(aArgVector[0], stderr, OT_EXIT_INVALID_ARGUMENTS);
    }
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

    openlog(aArgVector[0], LOG_PID | (config.mIsVerbose ? LOG_PERROR : 0), LOG_DAEMON);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_DEBUG));
    otLoggingSetLevel(config.mLogLevel);

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
