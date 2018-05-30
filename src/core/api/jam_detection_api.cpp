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
 *   This file implements the OpenThread Jam Detection API.
 */

#include "openthread-core-config.h"

#include <openthread/jam_detection.h>

#include "common/instance.hpp"

using namespace ot;

#if OPENTHREAD_ENABLE_JAM_DETECTION

otError otJamDetectionSetRssiThreshold(otInstance *aInstance, int8_t aRssiThreshold)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().SetRssiThreshold(aRssiThreshold);
}

int8_t otJamDetectionGetRssiThreshold(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().GetRssiThreshold();
}

otError otJamDetectionSetWindow(otInstance *aInstance, uint8_t aWindow)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().SetWindow(aWindow);
}

uint8_t otJamDetectionGetWindow(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().GetWindow();
}

otError otJamDetectionSetBusyPeriod(otInstance *aInstance, uint8_t aBusyPeriod)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().SetBusyPeriod(aBusyPeriod);
}

uint8_t otJamDetectionGetBusyPeriod(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().GetBusyPeriod();
}

otError otJamDetectionStart(otInstance *aInstance, otJamDetectionCallback aCallback, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().Start(aCallback, aContext);
}

otError otJamDetectionStop(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().Stop();
}

bool otJamDetectionIsEnabled(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().IsEnabled();
}

bool otJamDetectionGetState(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().GetState();
}

uint64_t otJamDetectionGetHistoryBitmap(otInstance *aInstance)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    return instance.GetThreadNetif().GetJamDetector().GetHistoryBitmap();
}

#endif // OPENTHREAD_ENABLE_JAM_DETECTION
