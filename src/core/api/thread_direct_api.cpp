/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the Thread Direct public API.
 */

#include "openthread-core-config.h"

#include "openthread/thread_direct.h"

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "mac/direct_handler.hpp"
#include "mac/mac.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_types.hpp"
#include "mac/sub_mac.hpp"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE
#include "mac/wakeup_tx_scheduler.hpp"
#include "meshcop/dataset_manager.hpp"
#endif

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

using namespace ot;

void otThreadDirectSetEventCallback(otInstance *aInstance, otThreadDirectEventCallback aCallback, void *aContext)
{
    AsCoreType(aInstance).Get<Mac::Mac>().SetDirectEventCallback(aCallback, aContext);
}

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

otError otThreadDirectWakeup(otInstance            *aInstance,
                             const otExtAddress    *aExtAddress,
                             otThreadDirectWakeType aWakeType,
                             uint16_t               aIntervalUs,
                             uint16_t               aDurationMs,
                             uint8_t                aKeyIndex)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aExtAddress != nullptr, error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(
        aKeyIndex == 0 || aKeyIndex == OT_MAC_FRAME_WAKE_KEY_INDEX ||
            (aKeyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN && aKeyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX),
        error = OT_ERROR_INVALID_ARGS);

    {
        uint8_t  effectiveKeyIndex = (aKeyIndex == 0) ? OT_MAC_FRAME_WAKE_KEY_INDEX : aKeyIndex;
        uint16_t intervalUs = (aIntervalUs != 0) ? aIntervalUs : OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INTERVAL_US;
        uint16_t durationMs = (aDurationMs != 0) ? aDurationMs : OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_DURATION_MS;

        // Ensure the Network Key is loaded from the active dataset so GetDefaultWakeKey()
        // can derive the wake key even when Thread has not been started (detached WI).
        if (effectiveKeyIndex == OT_MAC_FRAME_WAKE_KEY_INDEX)
        {
            IgnoreError(AsCoreType(aInstance).Get<MeshCoP::ActiveDatasetManager>().ApplyConfiguration());
        }

        if (effectiveKeyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN &&
            effectiveKeyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX)
        {
            VerifyOrExit(AsCoreType(aInstance).Get<ot::Mac::SubMac>().IsGuestWakeKeyRegistered(effectiveKeyIndex),
                         error = OT_ERROR_INVALID_STATE);
        }

        error = AsCoreType(aInstance).Get<WakeupTxScheduler>().StartWakeup(
            AsCoreType(aExtAddress), static_cast<Mac::Frame::WakeFrameType>(aWakeType), intervalUs, durationMs,
            effectiveKeyIndex);
    }

exit:
    return error;
}

bool otThreadDirectIsWakeBurstActive(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<WakeupTxScheduler>().IsRunning();
}

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE

otError otThreadDirectUnlink(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);

    return OT_ERROR_NOT_IMPLEMENTED;
}

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

otError otThreadDirectWakeListenerEnable(otInstance *aInstance, bool aEnable)
{
    return AsCoreType(aInstance).Get<Mac::Mac>().SetWakeupListenEnabled(aEnable);
}

bool otThreadDirectIsWakeListenerEnabled(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<Mac::Mac>().IsWakeupListenEnabled();
}

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

otError otThreadDirectSetSlwSchedule(otInstance *aInstance, uint16_t aSlwPeriodSlots)
{
    return AsCoreType(aInstance).Get<DirectHandler>().SetSlwSchedule(aSlwPeriodSlots);
}

otError otThreadDirectSetRamOverride(otInstance *aInstance, const otThreadDirectRamParams *aParams)
{
    otError        error = OT_ERROR_NONE;
    Mac::ScaParams params;

    VerifyOrExit(aParams != nullptr, error = OT_ERROR_INVALID_ARGS);
    // mDuration = 0 is a wire-encoding sentinel ("no change to prior RAM"); reject it as
    // a stored state since it has no meaning outside of a per-frame codec context.
    VerifyOrExit(aParams->mDuration >= 1 && aParams->mDuration <= Mac::ScaParams::kRamDurationMax,
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aParams->mOffsetUs >= Mac::ScaParams::kRamOffsetUsMin &&
                     aParams->mOffsetUs <= Mac::ScaParams::kRamOffsetUsMax,
                 error = OT_ERROR_INVALID_ARGS);

    params.mRamDuration = aParams->mDuration;
    params.mRamOffsetUs = aParams->mOffsetUs;
    memcpy(params.mRamBits, aParams->mBits, sizeof(params.mRamBits));

    error = AsCoreType(aInstance).Get<DirectHandler>().SetRamMask(params);

exit:
    return error;
}

uint32_t otThreadDirectGetSlwTimeout(otInstance *aInstance)
{
    return AsCoreType(aInstance).Get<DirectHandler>().GetSlwTimeout();
}

otError otThreadDirectSetSlwTimeout(otInstance *aInstance, uint32_t aTimeout)
{
    return AsCoreType(aInstance).Get<DirectHandler>().SetSlwTimeout(aTimeout);
}

otError otThreadDirectGetLocalSca(otInstance *aInstance, otThreadDirectLocalSca *aLocalSca)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aLocalSca != nullptr, error = OT_ERROR_INVALID_ARGS);

    {
        const Mac::ScaParams &params = AsCoreType(aInstance).Get<DirectHandler>().GetLocalSca();

        aLocalSca->mSlwPeriodSlots = params.mSlwPeriodSlots;
        aLocalSca->mRam.mDuration  = params.mRamDuration;
        aLocalSca->mRam.mOffsetUs  = params.mRamOffsetUs;
        memcpy(aLocalSca->mRam.mBits, params.mRamBits, sizeof(aLocalSca->mRam.mBits));
    }

exit:
    return error;
}

otError otThreadDirectGetPeerInfo(otInstance             *aInstance,
                                  const otExtAddress     *aExtAddress,
                                  otThreadDirectPeerInfo *aPeerInfo)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aExtAddress);
    OT_UNUSED_VARIABLE(aPeerInfo);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otThreadDirectSetGuestWakeKey(otInstance *aInstance, uint8_t aKeyIndex, const otThreadDirectWakeKey *aKey)
{
    otError              error = OT_ERROR_NONE;
    ot::Mac::KeyMaterial material;

    VerifyOrExit(aKey != nullptr, error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(aKeyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN &&
                     aKeyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX,
                 error = OT_ERROR_INVALID_ARGS);

    // Import the raw 16-byte key into a PSA-aware KeyMaterial. kExportable ensures
    // the platform security layer can export the key bytes for hardware AES-CCM use.
    material.SetFrom(*reinterpret_cast<const ot::Mac::Key *>(aKey),
                     OPENTHREAD_CONFIG_PLATFORM_MAC_KEYS_EXPORTABLE_ENABLE);

    error = AsCoreType(aInstance).Get<ot::Mac::SubMac>().SetWakeKey(aKeyIndex, &material);

exit:
    return error;
}

otError otThreadDirectRemoveGuestWakeKey(otInstance *aInstance, uint8_t aKeyIndex)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aKeyIndex >= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MIN &&
                     aKeyIndex <= OT_MAC_FRAME_GUEST_WAKE_KEY_INDEX_MAX,
                 error = OT_ERROR_INVALID_ARGS);

    IgnoreError(AsCoreType(aInstance).Get<ot::Mac::SubMac>().SetWakeKey(aKeyIndex, nullptr));

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
