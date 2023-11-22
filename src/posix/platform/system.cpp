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

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <assert.h>
#include <inttypes.h>

#include <openthread-core-config.h>
#include <openthread/border_router.h>
#include <openthread/heap.h>
#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/infra_if.h>
#include <openthread/platform/otns.h>
#include <openthread/platform/radio.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "posix/platform/daemon.hpp"
#include "posix/platform/firewall.hpp"
#include "posix/platform/infra_if.hpp"
#include "posix/platform/mainloop.hpp"
#include "posix/platform/radio_url.hpp"
#include "posix/platform/udp.hpp"

otInstance *gInstance = nullptr;
bool        gDryRun   = false;

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE || OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
static void processStateChange(otChangedFlags aFlags, void *aContext)
{
    otInstance *instance = static_cast<otInstance *>(aContext);

    OT_UNUSED_VARIABLE(instance);
    OT_UNUSED_VARIABLE(aFlags);

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    platformNetifStateChange(instance, aFlags);
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    ot::Posix::InfraNetif::Get().HandleBackboneStateChange(instance, aFlags);
#endif
}
#endif

static const char *get802154RadioUrl(otPlatformConfig *aPlatformConfig)
{
    const char *radioUrl = nullptr;

    for (uint8_t i = 0; i < aPlatformConfig->mRadioUrlNum; i++)
    {
        ot::Posix::RadioUrl url(aPlatformConfig->mRadioUrls[i]);

        if (strcmp(url.GetProtocol(), "trel") == 0)
        {
            continue;
        }

        radioUrl = aPlatformConfig->mRadioUrls[i];
        break;
    }

    VerifyOrDie(radioUrl != nullptr, OT_EXIT_INVALID_ARGUMENTS);
    return radioUrl;
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
static const char *getTrelRadioUrl(otPlatformConfig *aPlatformConfig)
{
    const char *radioUrl = nullptr;

    for (uint8_t i = 0; i < aPlatformConfig->mRadioUrlNum; i++)
    {
        ot::Posix::RadioUrl url(aPlatformConfig->mRadioUrls[i]);

        if (strcmp(url.GetProtocol(), "trel") == 0)
        {
            radioUrl = aPlatformConfig->mRadioUrls[i];
            break;
        }
    }

    return radioUrl;
}
#endif

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
void otSysSetInfraNetif(const char *aInfraNetifName, int aIcmp6Socket)
{
    ot::Posix::InfraNetif::Get().SetInfraNetif(aInfraNetifName, aIcmp6Socket);
}
#endif

void platformInit(otPlatformConfig *aPlatformConfig)
{
#if OPENTHREAD_POSIX_CONFIG_BACKTRACE_ENABLE
    platformBacktraceInit();
#endif

    platformAlarmInit(aPlatformConfig->mSpeedUpFactor, aPlatformConfig->mRealTimeSignal);
    platformRadioInit(get802154RadioUrl(aPlatformConfig));

    // For Dry-Run option, only init the radio.
    VerifyOrExit(!aPlatformConfig->mDryRun);

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelInit(getTrelRadioUrl(aPlatformConfig));
#endif
    platformRandomInit();

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
    ot::Posix::InfraNetif::Get().Init();

#endif

    gNetifName[0] = '\0';

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    platformNetifInit(aPlatformConfig);
#endif

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    ot::Posix::Udp::Get().Init(otSysGetThreadNetifName());
#else
    ot::Posix::Udp::Get().Init(aPlatformConfig->mInterfaceName);
#endif
#endif

exit:
    return;
}

void platformSetUp(otPlatformConfig *aPlatformConfig)
{
    OT_UNUSED_VARIABLE(aPlatformConfig);

    VerifyOrExit(!gDryRun);

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
    if (aPlatformConfig->mBackboneInterfaceName != nullptr && strlen(aPlatformConfig->mBackboneInterfaceName) > 0)
    {
        int icmp6Sock = -1;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
        icmp6Sock = ot::Posix::InfraNetif::CreateIcmp6Socket(aPlatformConfig->mBackboneInterfaceName);
#endif

        otSysSetInfraNetif(aPlatformConfig->mBackboneInterfaceName, icmp6Sock);
    }
#endif

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
    ot::Posix::InfraNetif::Get().SetUp();
#endif

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    platformNetifSetUp();
#endif

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    ot::Posix::Udp::Get().SetUp();
#endif

#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
    ot::Posix::Daemon::Get().SetUp();
#endif

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE || OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    SuccessOrDie(otSetStateChangedCallback(gInstance, processStateChange, gInstance));
#endif

exit:
    return;
}

otInstance *otSysInit(otPlatformConfig *aPlatformConfig)
{
    OT_ASSERT(gInstance == nullptr);

    platformInit(aPlatformConfig);

    gDryRun   = aPlatformConfig->mDryRun;
    gInstance = otInstanceInitSingle();
    OT_ASSERT(gInstance != nullptr);

    platformSetUp(aPlatformConfig);

    return gInstance;
}

void platformTearDown(void)
{
    VerifyOrExit(!gDryRun);

#if OPENTHREAD_POSIX_CONFIG_DAEMON_ENABLE
    ot::Posix::Daemon::Get().TearDown();
#endif

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    ot::Posix::Udp::Get().TearDown();
#endif

#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    platformNetifTearDown();
#endif

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
    ot::Posix::InfraNetif::Get().TearDown();
#endif

exit:
    return;
}

void platformDeinit(void)
{
#if OPENTHREAD_POSIX_VIRTUAL_TIME
    virtualTimeDeinit();
#endif
    platformRadioDeinit();

    // For Dry-Run option, only the radio is initialized.
    VerifyOrExit(!gDryRun);

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    ot::Posix::Udp::Get().Deinit();
#endif
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    platformNetifDeinit();
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelDeinit();
#endif

#if OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
    ot::Posix::InfraNetif::Get().Deinit();
#endif

exit:
    return;
}

void otSysDeinit(void)
{
    OT_ASSERT(gInstance != nullptr);

    platformTearDown();
    otInstanceFinalize(gInstance);
    gInstance = nullptr;
    platformDeinit();
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME
/**
 * Try selecting the given file descriptors in nonblocking mode.
 *
 * @param[in,out]  aContext  A reference to the mainloop context.
 *
 * @returns The value returned from select().
 *
 */
static int trySelect(otSysMainloopContext &aContext)
{
    struct timeval timeout          = {0, 0};
    fd_set         originReadFdSet  = aContext.mReadFdSet;
    fd_set         originWriteFdSet = aContext.mWriteFdSet;
    fd_set         originErrorFdSet = aContext.mErrorFdSet;
    int            rval;

    rval = select(aContext.mMaxFd + 1, &aContext.mReadFdSet, &aContext.mWriteFdSet, &aContext.mErrorFdSet, &timeout);

    if (rval == 0)
    {
        aContext.mReadFdSet  = originReadFdSet;
        aContext.mWriteFdSet = originWriteFdSet;
        aContext.mErrorFdSet = originErrorFdSet;
    }

    return rval;
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

void otSysMainloopUpdate(otInstance *aInstance, otSysMainloopContext *aMainloop)
{
    ot::Posix::Mainloop::Manager::Get().Update(*aMainloop);

    platformAlarmUpdateTimeout(&aMainloop->mTimeout);
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    platformNetifUpdateFdSet(aMainloop);
#endif
#if OPENTHREAD_POSIX_VIRTUAL_TIME
    virtualTimeUpdateFdSet(aMainloop);
#else
    platformRadioUpdateFdSet(aMainloop);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelUpdateFdSet(aMainloop);
#endif

    if (otTaskletsArePending(aInstance))
    {
        aMainloop->mTimeout.tv_sec  = 0;
        aMainloop->mTimeout.tv_usec = 0;
    }
}

int otSysMainloopPoll(otSysMainloopContext *aMainloop)
{
    int rval;

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    if (timerisset(&aMainloop->mTimeout))
    {
        // Make sure there are no data ready in UART
        rval = trySelect(*aMainloop);

        if (rval == 0)
        {
            bool noWrite = true;

            // If there are write requests, the device is supposed to wake soon
            for (int i = 0; i < aMainloop->mMaxFd + 1; ++i)
            {
                if (FD_ISSET(i, &aMainloop->mWriteFdSet))
                {
                    noWrite = false;
                    break;
                }
            }

            if (noWrite)
            {
                virtualTimeSendSleepEvent(&aMainloop->mTimeout);
            }

            rval = select(aMainloop->mMaxFd + 1, &aMainloop->mReadFdSet, &aMainloop->mWriteFdSet,
                          &aMainloop->mErrorFdSet, nullptr);
        }
    }
    else
#endif
    {
        rval = select(aMainloop->mMaxFd + 1, &aMainloop->mReadFdSet, &aMainloop->mWriteFdSet, &aMainloop->mErrorFdSet,
                      &aMainloop->mTimeout);
    }

    return rval;
}

void otSysMainloopProcess(otInstance *aInstance, const otSysMainloopContext *aMainloop)
{
    ot::Posix::Mainloop::Manager::Get().Process(*aMainloop);

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    virtualTimeProcess(aInstance, aMainloop);
#else
    platformRadioProcess(aInstance, aMainloop);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    platformTrelProcess(aInstance, aMainloop);
#endif
    platformAlarmProcess(aInstance);
#if OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
    platformNetifProcess(aMainloop);
#endif
}

bool IsSystemDryRun(void) { return gDryRun; }
