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
 *   This file implements the OpenThread Instance API.
 */

#define WPP_NAME "instance_api.tmh"

#include "openthread-core-config.h"

#include <openthread/instance.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/settings.h>

#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"

using namespace ot;

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
otInstance *otInstanceInit(void *aInstanceBuffer, size_t *aInstanceBufferSize)
{
    Instance *instance;

    instance = Instance::Init(aInstanceBuffer, aInstanceBufferSize);
    otLogInfoApi(*instance, "otInstance Initialized");

    return instance;
}
#else
otInstance *otInstanceInitSingle(void)
{
    return &Instance::InitSingle();
}
#endif // #if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES


bool otInstanceIsInitialized(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.IsInitialized();
}

void otInstanceFinalize(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Finalize();
}

otError otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetNotifier().RegisterCallback(aCallback, aContext);
}

void otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.GetNotifier().RemoveCallback(aCallback, aContext);
}

void otInstanceReset(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Reset();
}

void otInstanceFactoryReset(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.FactoryReset();
}

otError otInstanceErasePersistentInfo(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.ErasePersistentInfo();
}

otLogLevel otGetDynamicLogLevel(otInstance *aInstance)
{
    otLogLevel logLevel;

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    Instance &instance = *static_cast<Instance *>(aInstance);

    logLevel = instance.GetDynamicLogLevel();
#else
    logLevel = static_cast<otLogLevel>(OPENTHREAD_CONFIG_LOG_LEVEL);
    OT_UNUSED_VARIABLE(aInstance);
#endif

    return logLevel;
}

otError otSetDynamicLogLevel(otInstance *aInstance, otLogLevel aLogLevel)
{
    otError error = OT_ERROR_NONE;

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.SetDynamicLogLevel(aLogLevel);
#else
    error = OT_ERROR_DISABLED_FEATURE;
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLogLevel);
#endif

    return error;
}
