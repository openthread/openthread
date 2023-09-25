/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements the processing of OpenThread stack in thread safe mode.
 */

#include "openthread-posix-config.h"

#include <openthread/config.h>

#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include <openthread/logging.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>

#include <common/code_utils.hpp>
#include <lib/platform/exit_code.h>
#include <openthread/openthread-system.h>

#if OPENTHREAD_POSIX_CONFIG_MULTITHREAD_SUPPORT_ENABLE

#define MULTIPLE_INSTANCE_MAX 10
#define MAX_TTY_LEN 100
#define MAX_RADIO_URL_LEN 200
#define MAX_INTERFACE_LEN 100

static otInstance     *gInstance = nullptr;
static pthread_t       gThreadId;
static pthread_mutex_t gLock      = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  gCond      = PTHREAD_COND_INITIALIZER;
static bool            gTerminate = 0;

typedef struct OtConfig
{
    char       mComPort[MAX_TTY_LEN];
    otLogLevel mOtLogLevel;
} OtConfig;

static bool getRadioURL(char *aComPort)
{
    struct stat st;
    char        radioDevice[MAX_RADIO_URL_LEN] = {0};
    bool        found                          = false;

    /* For multiple devices and intefaces,
     * we will align the index of them.
     */
    snprintf(radioDevice, sizeof(radioDevice), "/dev/%s", aComPort);
    if (stat(radioDevice, &st) == 0)
    {
        syslog(LOG_INFO, "radio device found [%s]", radioDevice);
        found = true;
    }
    return found;
}

static int getInterface(void)
{
    struct stat st;
    int         idx                      = 0;
    char        iface[MAX_INTERFACE_LEN] = {0};

    for (idx = 0; idx < MULTIPLE_INSTANCE_MAX; idx++)
    {
        snprintf(iface, sizeof(iface), "/sys/class/net/wpan%d", idx);
        if (stat(iface, &st) == 0)
        {
            syslog(LOG_INFO, "Interface is already used [%s]", iface);
            continue;
        }
        else
        {
            syslog(LOG_INFO, "find empty interface [%s]", iface);
            break;
        }
    }

    return idx;
}

void *pThreadOtMainLoop(void *arg)
{
    pthread_mutex_lock(&gLock);
    otPlatformConfig platformConfig;
    int              interfaceIdx                = 0;
    char             radioUrl[MAX_RADIO_URL_LEN] = {0};
    char             iface[MAX_INTERFACE_LEN]    = {0};
    OtConfig        *config                      = (OtConfig *)arg;

    syslog(LOG_INFO, "Inside pThreadOtMainLoop");

    interfaceIdx = getInterface();
    VerifyOrExit(interfaceIdx != MULTIPLE_INSTANCE_MAX,
                 syslog(LOG_CRIT, "Interface reached max count...Not able to create new interface"));
    snprintf(iface, sizeof(iface), "wpan%d", interfaceIdx);
    syslog(LOG_INFO, "interface found [%s]", iface);

    VerifyOrExit(getRadioURL(config->mComPort) != false, syslog(LOG_CRIT, "radio device not found"));
    snprintf(radioUrl, sizeof(radioUrl), "spinel+hdlc+uart:///dev/%s", config->mComPort);
    syslog(LOG_INFO, "radio Url found [%s]", radioUrl);
    syslog(LOG_INFO, "ot log level [%d]", config->mOtLogLevel);

    platformConfig.mInterfaceName = iface;
    VerifyOrDie(platformConfig.mRadioUrlNum < OT_ARRAY_LENGTH(platformConfig.mRadioUrls), OT_EXIT_INVALID_ARGUMENTS);
    platformConfig.mRadioUrls[platformConfig.mRadioUrlNum++] = radioUrl;
#if defined(__linux)
    platformConfig.mRealTimeSignal = 41;
#endif
    platformConfig.mSpeedUpFactor = 1;

    syslog(LOG_INFO, "Running %s", otGetVersionString());
    syslog(LOG_INFO, "Thread version: %hu", otThreadGetVersion());
    IgnoreError(otLoggingSetLevel(config->mOtLogLevel));

    gInstance = otSysInit(&platformConfig);
    if (gInstance == nullptr)
    {
        syslog(LOG_INFO, "otSysInit failed");
        DieNow(OT_EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Thread interface: %s", otSysGetThreadNetifName());
    syslog(LOG_INFO, "RCP version: %s", otPlatRadioGetVersionString(gInstance));

    otLogDebgPlat("ot instance create success!!!");

    pthread_cond_signal(&gCond);
    pthread_mutex_unlock(&gLock);

    while (gTerminate == false)
    {
        otSysMainloopContext mainloop;

        otTaskletsProcess(gInstance);

        FD_ZERO(&mainloop.mReadFdSet);
        FD_ZERO(&mainloop.mWriteFdSet);
        FD_ZERO(&mainloop.mErrorFdSet);

        mainloop.mMaxFd           = -1;
        mainloop.mTimeout.tv_sec  = 10;
        mainloop.mTimeout.tv_usec = 0;

        otSysMainloopUpdate(gInstance, &mainloop);

        if (otSysMainloopPoll(&mainloop) >= 0)
        {
            pthread_mutex_lock(&gLock);

            otSysMainloopProcess(gInstance, &mainloop);

            pthread_mutex_unlock(&gLock);
        }
        else if (errno != EINTR)
        {
            perror("select");
            ExitNow();
        }
    }

exit:
    if (gInstance)
    {
        otSysDeinit();
    }
    gThreadId  = 0;
    gInstance  = nullptr;
    gTerminate = false;
    pthread_cond_signal(&gCond);
    pthread_mutex_unlock(&gLock);
    syslog(LOG_INFO, "terminate thread mainloop : exit");
    return nullptr;
}

void otSysGetInstance(otInstance **aInstance, const char *aComPort, otLogLevel aLogLevel)
{
    OtConfig *config = nullptr;

    syslog(LOG_INFO, "otSysGetInstance");

    if (gInstance)
    {
        syslog(LOG_INFO, "ot instance already initialised!!!");
        *aInstance = gInstance;
        ExitNow();
    }

    config = (OtConfig *)malloc(sizeof(OtConfig));
    VerifyOrExit(config != nullptr, syslog(LOG_INFO, "malloc failed"));
    memset(config, 0, sizeof(OtConfig));
    strncpy(config->mComPort, aComPort, strlen(aComPort));
    config->mOtLogLevel = aLogLevel;

    VerifyOrExit(pthread_mutex_init(&gLock, NULL) == 0, syslog(LOG_INFO, "mutex init has failed"));
    VerifyOrExit(pthread_cond_init(&gCond, NULL) == 0, syslog(LOG_INFO, "cond init has failed"));

    pthread_create(&gThreadId, NULL, pThreadOtMainLoop, config);
    pthread_mutex_lock(&gLock);
    syslog(LOG_INFO, "Wait for signal to initialise openthread stack !!!");
    pthread_cond_wait(&gCond, &gLock);
    *aInstance = gInstance;
    syslog(LOG_INFO, "otThread : gThreadId[%ld]", gThreadId);
    pthread_mutex_unlock(&gLock);

exit:
    free(config);
}

void otSysWait(void)
{
    otLogDebgPlat("otSysWait");
    pthread_join(gThreadId, NULL);
}

void otSysLock(void)
{
    otLogDebgPlat("otSysLock");
    pthread_mutex_lock(&gLock);
}

void otSysUnlock(void)
{
    otLogDebgPlat("otSysUnlock");
    pthread_mutex_unlock(&gLock);
}
void otSysDestroyInstance(void)
{
    otLogDebgPlat("otSysDestroyInstance");
    gTerminate = true;
    pthread_join(gThreadId, NULL);
    pthread_mutex_destroy(&gLock);
}
#endif // OPENTHREAD_POSIX_CONFIG_MULTITHREAD_SUPPORT_ENABLE
