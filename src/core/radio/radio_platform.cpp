/*
 *    Copyright (c) 2019, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the radio platform callbacks into OpenThread and default/weak radio platform APIs.
 */

#include <openthread/instance.h>
#include <openthread/platform/time.h>

#include "common/instance.hpp"
#include "radio/radio.hpp"

using namespace ot;

//---------------------------------------------------------------------------------------------------------------------
// otPlatRadio callbacks

extern "C" void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<Radio::Callbacks>().HandleReceiveDone(static_cast<Mac::RxFrame *>(aFrame), aError);
    }
}

extern "C" void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<Radio::Callbacks>().HandleTransmitStarted(*static_cast<Mac::TxFrame *>(aFrame));
    }
}

extern "C" void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<Radio::Callbacks>().HandleTransmitDone(*static_cast<Mac::TxFrame *>(aFrame),
                                                             static_cast<Mac::RxFrame *>(aAckFrame), aError);
    }
}

extern "C" void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    if (instance->IsInitialized())
    {
        instance->Get<Radio::Callbacks>().HandleEnergyScanDone(aEnergyScanMaxRssi);
    }
}

#if OPENTHREAD_CONFIG_DIAG_ENABLE
extern "C" void otPlatDiagRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    instance->Get<Radio::Callbacks>().HandleDiagsReceiveDone(static_cast<Mac::RxFrame *>(aFrame), aError);
}

extern "C" void otPlatDiagRadioTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    Instance *instance = static_cast<Instance *>(aInstance);

    instance->Get<Radio::Callbacks>().HandleDiagsTransmitDone(*static_cast<Mac::TxFrame *>(aFrame), aError);
}
#endif

//---------------------------------------------------------------------------------------------------------------------
// Default/weak implementation of radio platform APIs

OT_TOOL_WEAK uint32_t otPlatRadioGetSupportedChannelMask(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return Radio::kSupportedChannels;
}

OT_TOOL_WEAK uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
    return otPlatRadioGetSupportedChannelMask(aInstance);
}

OT_TOOL_WEAK const char *otPlatRadioGetVersionString(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return otGetVersionString();
}

OT_TOOL_WEAK otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_STATE_INVALID;
}

OT_TOOL_WEAK void otPlatRadioSetMacKey(otInstance *    aInstance,
                                       uint8_t         aKeyIdMode,
                                       uint8_t         aKeyId,
                                       const otMacKey *aPrevKey,
                                       const otMacKey *aCurrKey,
                                       const otMacKey *aNextKey)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKeyIdMode);
    OT_UNUSED_VARIABLE(aKeyId);
    OT_UNUSED_VARIABLE(aPrevKey);
    OT_UNUSED_VARIABLE(aCurrKey);
    OT_UNUSED_VARIABLE(aNextKey);
}

OT_TOOL_WEAK void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aMacFrameCounter);
}

OT_TOOL_WEAK uint64_t otPlatTimeGet(void)
{
    return UINT64_MAX;
}

OT_TOOL_WEAK uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return UINT64_MAX;
}

OT_TOOL_WEAK uint32_t otPlatRadioGetBusSpeed(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}
