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

#define OT_POSIX_APP_TYPE_NCP 1
#define OT_POSIX_APP_TYPE_CLI 2

#include <openthread/diag.h>
#include <openthread/logging.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/platform/radio.h>
#if OPENTHREAD_POSIX_APP_TYPE == OT_POSIX_APP_TYPE_NCP
#include <openthread/ncp.h>
#define OPENTHREAD_USE_CONSOLE 0
#elif OPENTHREAD_POSIX_APP_TYPE == OT_POSIX_APP_TYPE_CLI
#include <openthread/cli.h>
#if (HAVE_LIBEDIT || HAVE_LIBREADLINE) && !OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
#define OPENTHREAD_USE_CONSOLE 1
#include "console_cli.h"
#else
#define OPENTHREAD_USE_CONSOLE 0
#endif
#else
#error "Unknown posix app type!"
#endif
#include <lib/platform/exit_code.h>
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
    ARG_SPI_GPIO_INT_DEV    = 1011,
    ARG_SPI_GPIO_INT_LINE   = 1012,
    ARG_SPI_GPIO_RESET_DEV  = 1013,
    ARG_SPI_GPIO_RESET_LINE = 1014,
    ARG_SPI_MODE            = 1015,
    ARG_SPI_SPEED           = 1016,
    ARG_SPI_CS_DELAY        = 1017,
    ARG_SPI_RESET_DELAY     = 1018,
    ARG_SPI_ALIGN_ALLOWANCE = 1019,
    ARG_SPI_SMALL_PACKET    = 1020,
    ARG_MAX_POWER_TABLE     = 1021,

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
#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
                                         {"max-power-table", required_argument, NULL, ARG_MAX_POWER_TABLE},
#endif
#if OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_SPI
                                         {"gpio-int-dev", required_argument, NULL, ARG_SPI_GPIO_INT_DEV},
                                         {"gpio-int-line", required_argument, NULL, ARG_SPI_GPIO_INT_LINE},
                                         {"gpio-reset-dev", required_argument, NULL, ARG_SPI_GPIO_RESET_DEV},
                                         {"gpio-reset-line", required_argument, NULL, ARG_SPI_GPIO_RESET_LINE},
                                         {"spi-mode", required_argument, NULL, ARG_SPI_MODE},
                                         {"spi-speed", required_argument, NULL, ARG_SPI_SPEED},
                                         {"spi-cs-delay", required_argument, NULL, ARG_SPI_CS_DELAY},
                                         {"spi-reset-delay", required_argument, NULL, ARG_SPI_RESET_DELAY},
                                         {"spi-align-allowance", required_argument, NULL, ARG_SPI_ALIGN_ALLOWANCE},
                                         {"spi-small-packet", required_argument, NULL, ARG_SPI_SMALL_PACKET},
#endif // OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_SPI
                                         {0, 0, 0, 0}};

static void PrintUsage(const char *aProgramName, FILE *aStream, int aExitCode)
{
    fprintf(aStream,
            "Syntax:\n"
            "    %s [Options] NodeId|Device|Command [DeviceConfig|CommandArgs]\n"
            "Options:\n"
            "    -d  --debug-level             Debug level of logging.\n"
            "    -h  --help                    Display this usage information.\n"
            "    -I  --interface-name name     Thread network interface name.\n"
            "    -n  --dry-run                 Just verify if arguments is valid and radio spinel is compatible.\n"
            "        --no-reset                Do not send Spinel reset command to RCP on initialization.\n"
            "        --radio-version           Print radio firmware version.\n"
            "        --ncp-dataset             Retrieve and save NCP dataset to file.\n"
            "    -s  --time-speed factor       Time speed up factor.\n"
            "    -v  --verbose                 Also log to stderr.\n",
            aProgramName);
#if OPENTHREAD_POSIX_CONFIG_RCP_BUS == OT_POSIX_RCP_BUS_SPI
    fprintf(aStream,
            "        --gpio-int-dev[=gpio-device-path]\n"
            "                                  Specify a path to the Linux sysfs-exported GPIO device for the\n"
            "                                  `I̅N̅T̅` pin. If not specified, `SPI` interface will fall back to\n"
            "                                  polling, which is inefficient.\n"
            "        --gpio-int-line[=line-offset]\n"
            "                                  The offset index of `I̅N̅T̅` pin for the associated GPIO device.\n"
            "                                  If not specified, `SPI` interface will fall back to polling,\n"
            "                                  which is inefficient.\n"
            "        --gpio-reset-dev[=gpio-device-path]\n"
            "                                  Specify a path to the Linux sysfs-exported GPIO device for the\n"
            "                                  `R̅E̅S̅` pin.\n"
            "        --gpio-reset-line[=line-offset]"
            "                                  The offset index of `R̅E̅S̅` pin for the associated GPIO device.\n"
            "        --spi-mode[=mode]         Specify the SPI mode to use (0-3).\n"
            "        --spi-speed[=hertz]       Specify the SPI speed in hertz.\n"
            "        --spi-cs-delay[=usec]     Specify the delay after C̅S̅ assertion, in µsec.\n"
            "        --spi-reset-delay[=ms]    Specify the delay after R̅E̅S̅E̅T̅ assertion, in milliseconds.\n"
            "        --spi-align-allowance[=n] Specify the maximum number of 0xFF bytes to clip from start of\n"
            "                                  MISO frame. Max value is 16.\n"
            "        --spi-small-packet=[n]    Specify the smallest packet we can receive in a single transaction.\n"
            "                                  (larger packets will require two transactions). Default value is 32.\n");
#endif
#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
    fprintf(aStream,
            "        --max-power-table         Max power for channels in ascending order separated by commas,\n"
            "                                  If the number of values is less than that of supported channels,\n"
            "                                  the last value will be applied to all remaining channels.\n"
            "                                  Special value 0x7f disables a channel.\n");
#endif
    exit(aExitCode);
}

static void ParseArg(int aArgCount, char *aArgVector[], PosixConfig *aConfig)
{
    memset(aConfig, 0, sizeof(*aConfig));

    aConfig->mPlatformConfig.mSpeedUpFactor      = 1;
    aConfig->mPlatformConfig.mResetRadio         = true;
    aConfig->mPlatformConfig.mSpiSpeed           = OT_PLATFORM_CONFIG_SPI_DEFAULT_SPEED_HZ;
    aConfig->mPlatformConfig.mSpiCsDelay         = OT_PLATFORM_CONFIG_SPI_DEFAULT_CS_DELAY_US;
    aConfig->mPlatformConfig.mSpiResetDelay      = OT_PLATFORM_CONFIG_SPI_DEFAULT_RESET_DELAY_MS;
    aConfig->mPlatformConfig.mSpiAlignAllowance  = OT_PLATFORM_CONFIG_SPI_DEFAULT_ALIGN_ALLOWANCE;
    aConfig->mPlatformConfig.mSpiSmallPacketSize = OT_PLATFORM_CONFIG_SPI_DEFAULT_SMALL_PACKET_SIZE;
    aConfig->mPlatformConfig.mSpiMode            = OT_PLATFORM_CONFIG_SPI_DEFAULT_MODE;
    aConfig->mLogLevel                           = OT_LOG_LEVEL_CRIT;

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
        case ARG_SPI_GPIO_INT_DEV:
            aConfig->mPlatformConfig.mSpiGpioIntDevice = optarg;
            break;
        case ARG_SPI_GPIO_INT_LINE:
            aConfig->mPlatformConfig.mSpiGpioIntLine = (uint8_t)atoi(optarg);
            break;
        case ARG_SPI_GPIO_RESET_DEV:
            aConfig->mPlatformConfig.mSpiGpioResetDevice = optarg;
            break;
        case ARG_SPI_GPIO_RESET_LINE:
            aConfig->mPlatformConfig.mSpiGpioResetLine = (uint8_t)atoi(optarg);
            break;
        case ARG_SPI_MODE:
            aConfig->mPlatformConfig.mSpiMode = (uint8_t)atoi(optarg);
            break;
        case ARG_SPI_SPEED:
            aConfig->mPlatformConfig.mSpiSpeed = (uint32_t)atoi(optarg);
            break;
        case ARG_SPI_CS_DELAY:
            aConfig->mPlatformConfig.mSpiCsDelay = (uint16_t)atoi(optarg);
            break;
        case ARG_SPI_RESET_DELAY:
            aConfig->mPlatformConfig.mSpiResetDelay = (uint32_t)atoi(optarg);
            break;
        case ARG_SPI_ALIGN_ALLOWANCE:
            aConfig->mPlatformConfig.mSpiAlignAllowance = (uint8_t)atoi(optarg);
            break;
        case ARG_SPI_SMALL_PACKET:
            aConfig->mPlatformConfig.mSpiSmallPacketSize = (uint8_t)atoi(optarg);
            break;
#if OPENTHREAD_POSIX_CONFIG_MAX_POWER_TABLE_ENABLE
        case ARG_MAX_POWER_TABLE:
            aConfig->mPlatformConfig.mMaxPowerTable = optarg;
            break;
#endif
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

    aConfig->mPlatformConfig.mRadioFile = aArgVector[optind];

    if (optind + 1 < aArgCount)
    {
        aConfig->mPlatformConfig.mRadioConfig = aArgVector[optind + 1];
    }
}

static otInstance *InitInstance(int aArgCount, char *aArgVector[])
{
    PosixConfig config;
    otInstance *instance = NULL;

    ParseArg(aArgCount, aArgVector, &config);

    openlog(aArgVector[0], LOG_PID | (config.mIsVerbose ? LOG_PERROR : 0), LOG_DAEMON);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_DEBUG));
    syslog(LOG_INFO, "Running %s", otGetVersionString());
    syslog(LOG_INFO, "Thread version: %hu", otThreadGetVersion());
    otLoggingSetLevel(config.mLogLevel);

    instance = otSysInit(&config.mPlatformConfig);

    if (config.mPrintRadioVersion)
    {
        printf("%s\n", otPlatRadioGetVersionString(instance));
    }
    else
    {
        syslog(LOG_INFO, "RCP version: %s", otPlatRadioGetVersionString(instance));
    }

    if (config.mIsDryRun)
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

#if OPENTHREAD_POSIX_APP_TYPE == OT_POSIX_APP_TYPE_NCP
    otNcpInit(instance);
#elif OPENTHREAD_POSIX_APP_TYPE == OT_POSIX_APP_TYPE_CLI
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
