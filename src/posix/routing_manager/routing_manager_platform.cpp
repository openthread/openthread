/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes implementation of routing manager platform APIs.
 *
 */

#include "platform-posix.h"

#if OPENTHREAD_CONFIG_DUCKHORN_BORDER_ROUTER_ENABLE

#include <openthread/platform/alarm-milli.h>

#include "routing_manager.hpp"

#include "timer.hpp"

using namespace ot::Posix;

static RoutingManager *sRouterManager = nullptr;

void platformRoutingManagerInit(otInstance *aInstance, const char *aInfraNetifName)
{
    if (sRouterManager == nullptr)
    {
        sRouterManager = new RoutingManager(aInstance);
        sRouterManager->Init(aInfraNetifName);
    }
}

void platformRoutingManagerDeinit()
{
    if (sRouterManager != nullptr)
    {
        sRouterManager->Deinit();
        delete sRouterManager;
        sRouterManager = nullptr;
    }
}

void platformRoutingManagerUpdate(otSysMainloopContext *aMainloop)
{
    Milliseconds timeout;
    Milliseconds nextFire;

    if (sRouterManager != nullptr)
    {
        sRouterManager->Update(aMainloop);
    }

    timeout  = aMainloop->mTimeout.tv_sec * 1000 + aMainloop->mTimeout.tv_usec / 1000;
    nextFire = TimerScheduler::Get().GetEarliestFireTime() - otPlatAlarmMilliGetNow();
    if (nextFire < timeout)
    {
        timeout = nextFire;
    }

    aMainloop->mTimeout.tv_sec  = timeout / 1000;
    aMainloop->mTimeout.tv_usec = (timeout % 1000) * 1000;
}

void platformRoutingManagerProcess(const otSysMainloopContext *aMainloop)
{
    if (sRouterManager != nullptr)
    {
        sRouterManager->Process(aMainloop);
    }

    TimerScheduler::Get().Process(otPlatAlarmMilliGetNow());
}

#endif // OPENTHREAD_CONFIG_DUCKHORN_BORDER_ROUTER_ENABLE
