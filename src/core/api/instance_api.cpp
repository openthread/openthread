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

#include <openthread/config.h>

#include <openthread/instance.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/settings.h>

#include "openthread-instance.h"
#include "openthread-single-instance.h"
#include "common/logging.hpp"
#include "common/new.hpp"

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

static otDEFINE_ALIGNED_VAR(sInstanceRaw, sizeof(otInstance), uint64_t);
otInstance *sInstance = NULL;

otInstance *otGetInstance(void)
{
    return sInstance;
}

ot::ThreadNetif &otGetThreadNetif(void)
{
    return sInstance->mThreadNetif;
}

ot::MeshForwarder &otGetMeshForwarder(void)
{
    return sInstance->mThreadNetif.GetMeshForwarder();
}

ot::TaskletScheduler &otGetTaskletScheduler(void)
{
    return sInstance->mTaskletScheduler;
}

ot::Ip6::Ip6 &otGetIp6(void)
{
    return sInstance->mIp6;
}
#endif // #if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

otInstance::otInstance(void) :
    mReceiveIp6DatagramCallback(NULL),
    mReceiveIp6DatagramCallbackContext(NULL),
    mActiveScanCallback(NULL),
    mActiveScanCallbackContext(NULL),
    mEnergyScanCallback(NULL),
    mEnergyScanCallbackContext(NULL),
    mTimerMilliScheduler(this),
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    mTimerMicroScheduler(this),
#endif
    mThreadNetif(mIp6)
#if OPENTHREAD_ENABLE_RAW_LINK_API
    , mLinkRaw(*this)
#endif // OPENTHREAD_ENABLE_RAW_LINK_API
#if OPENTHREAD_ENABLE_APPLICATION_COAP
    , mApplicationCoap(mThreadNetif)
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP
#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    , mLogLevel(static_cast<otLogLevel>(OPENTHREAD_CONFIG_LOG_LEVEL))
#endif // OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
{
}

using namespace ot;

void otInstancePostConstructor(otInstance *aInstance)
{
    // restore datasets and network information
    otPlatSettingsInit(aInstance);
    aInstance->mThreadNetif.GetMle().Restore();

#if OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT

    // If auto start is configured, do that now
    if (otThreadGetAutoStart(aInstance))
    {
        if (otIp6SetEnabled(aInstance, true) == OT_ERROR_NONE)
        {
            // Only try to start Thread if we could bring up the interface
            if (otThreadSetEnabled(aInstance, true) != OT_ERROR_NONE)
            {
                // Bring the interface down if Thread failed to start
                otIp6SetEnabled(aInstance, false);
            }
        }
    }

#endif
}

#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

otInstance *otInstanceInit(void *aInstanceBuffer, size_t *aInstanceBufferSize)
{
    otInstance *instance = NULL;

    otLogFuncEntry();

    VerifyOrExit(aInstanceBufferSize != NULL);

    // Make sure the input buffer is big enough
    VerifyOrExit(sizeof(otInstance) <= *aInstanceBufferSize, *aInstanceBufferSize = sizeof(otInstance));

    VerifyOrExit(aInstanceBuffer != NULL);

    // Construct the context
    instance = new(aInstanceBuffer)otInstance();

    // Execute post constructor operations
    otInstancePostConstructor(instance);

    otLogInfoApi(instance, "otInstance Initialized");

exit:

    otLogFuncExit();
    return instance;
}

bool otInstanceIsInitialized(otInstance *aInstance)
{
    return (aInstance != NULL);
}

#else // #if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

otInstance *otInstanceInitSingle(void)
{
    otLogFuncEntry();

    VerifyOrExit(sInstance == NULL);

    // We need to ensure `sInstance` pointer is correctly set
    // before any object constructor is called.
    sInstance = reinterpret_cast<otInstance *>(&sInstanceRaw);

    // Construct the context
    sInstance = new(&sInstanceRaw)otInstance();

    // Execute post constructor operations
    otInstancePostConstructor(sInstance);

    otLogInfoApi(sInstance, "otInstance Initialized");

exit:

    otLogFuncExit();
    return sInstance;
}

bool otInstanceIsInitialized(otInstance *aInstance)
{
    return (aInstance != NULL) && (aInstance == sInstance);
}

#endif // #if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES

void otInstanceFinalize(otInstance *aInstance)
{
    otLogFuncEntry();

    // Ensure we are disabled
    (void)otThreadSetEnabled(aInstance, false);
    (void)otIp6SetEnabled(aInstance, false);

#if !OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    sInstance = NULL;
#endif

    otLogFuncExit();
}

otError otSetStateChangedCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aCallbackContext)
{
    otError error = OT_ERROR_NO_BUFS;

    for (size_t i = 0; i < OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS; i++)
    {
        if (aInstance->mNetifCallback[i].IsFree())
        {
            aInstance->mNetifCallback[i].Set(aCallback, aCallbackContext);
            error = aInstance->mThreadNetif.RegisterCallback(aInstance->mNetifCallback[i]);
            break;
        }
    }

    return error;
}

void otRemoveStateChangeCallback(otInstance *aInstance, otStateChangedCallback aCallback, void *aCallbackContext)
{
    for (size_t i = 0; i < OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS; i++)
    {
        if (aInstance->mNetifCallback[i].IsServing(aCallback, aCallbackContext))
        {
            aInstance->mThreadNetif.RemoveCallback(aInstance->mNetifCallback[i]);
            aInstance->mNetifCallback[i].Free();
            break;
        }
    }
}

void otInstanceReset(otInstance *aInstance)
{
    otPlatReset(aInstance);
}

void otInstanceFactoryReset(otInstance *aInstance)
{
    otPlatSettingsWipe(aInstance);
    otPlatReset(aInstance);
}

otError otInstanceErasePersistentInfo(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otThreadGetDeviceRole(aInstance) ==  OT_DEVICE_ROLE_DISABLED, error = OT_ERROR_INVALID_STATE);
    otPlatSettingsWipe(aInstance);

exit:
    return error;
}

otLogLevel otGetDynamicLogLevel(otInstance *aInstance)
{
    otLogLevel logLevel;

#if OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
    logLevel =  aInstance->mLogLevel;
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
    aInstance->mLogLevel = aLogLevel;
#else
    error = OT_ERROR_DISABLED_FEATURE;
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aLogLevel);
#endif

    return error;
}
