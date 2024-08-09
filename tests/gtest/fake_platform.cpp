/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "fake_platform.hpp"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

#include <openthread/instance.h>
#include <openthread/tasklet.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/entropy.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/toolchain.h>

using namespace ot;

#define US_PER_MS 1000

namespace ot {

FakePlatform *sPlatform = nullptr;

FakePlatform::FakePlatform()
{
    assert(sPlatform == nullptr);
    sPlatform = this;

    mTransmitFrame.mPsdu = mTransmitBuffer;

    mInstance = otInstanceInitSingle();
}

FakePlatform::~FakePlatform()
{
    otInstanceFinalize(mInstance);
    sPlatform = nullptr;
}

FakePlatform &FakePlatform::CurrentPlatform() { return *sPlatform; }

void FakePlatform::StartMicroAlarm(uint32_t aT0, uint32_t aDt)
{
    uint32_t now = mNow;

    if (static_cast<int32_t>(aT0 - now) > 0 || static_cast<int32_t>(aT0 - now) + static_cast<int64_t>(aDt) > 0)
    {
        mMicroAlarmStart = mNow + (static_cast<uint64_t>(aDt) + static_cast<int32_t>(aT0 - now));
    }
    else
    {
        mMicroAlarmStart = mNow;
    }
}
void FakePlatform::StartMilliAlarm(uint32_t aT0, uint32_t aDt)
{
    uint32_t now = (mNow / US_PER_MS);

    if (static_cast<int32_t>(aT0 - now) > 0 || static_cast<int32_t>(aT0 - now) + static_cast<int64_t>(aDt) > 0)
    {
        mMilliAlarmStart = mNow + (static_cast<uint64_t>(aDt) + static_cast<int32_t>(aT0 - now)) * US_PER_MS;
    }
    else
    {
        mMilliAlarmStart = mNow;
    }
}

void FakePlatform::StopMicroAlarm() { mMicroAlarmStart = 0; }

void FakePlatform::StopMilliAlarm() { mMilliAlarmStart = 0; }

uint64_t FakePlatform::Run(uint64_t aTimeout)
{
    if (otTaskletsArePending(mInstance))
    {
        otTaskletsProcess(mInstance);
    }
    else
    {
        uint64_t end = mNow + aTimeout;

        if (mMicroAlarmStart && mMicroAlarmStart <= mMilliAlarmStart && mMicroAlarmStart < end)
        {
            aTimeout -= (mMicroAlarmStart - mNow);
            mNow             = mMicroAlarmStart;
            mMicroAlarmStart = 0;
            otPlatAlarmMicroFired(mInstance);
        }
        else if (mMilliAlarmStart && mMilliAlarmStart <= mMicroAlarmStart && mMilliAlarmStart < end)
        {
            aTimeout -= (mMilliAlarmStart - mNow);
            mNow             = mMilliAlarmStart;
            mMilliAlarmStart = 0;
            otPlatAlarmMilliFired(mInstance);
        }
        else
        {
            aTimeout = 0;
            mNow     = end;
        }
    }

    return aTimeout;
}

void FakePlatform::Go(uint64_t aTimeout)
{
    while ((aTimeout = Run(aTimeout)) > 0)
    {
        // nothing
    }
}

} // namespace ot

extern "C" {

void otTaskletsSignalPending(otInstance *) {}

void otPlatAlarmMilliStop(otInstance *) { sPlatform->StopMilliAlarm(); }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt) { sPlatform->StartMilliAlarm(aT0, aDt); }

uint32_t otPlatAlarmMilliGetNow(void) { return sPlatform->GetNow() / US_PER_MS; }
void     otPlatAlarmMicroStop(otInstance *) { sPlatform->StopMicroAlarm(); }

void otPlatAlarmMicroStartAt(otInstance *, uint32_t aT0, uint32_t aDt) { sPlatform->StartMicroAlarm(aT0, aDt); }

uint64_t otPlatTimeGet(void) { return sPlatform->GetNow(); }

uint32_t otPlatAlarmMicroGetNow(void) { return otPlatTimeGet(); }

void otPlatRadioGetIeeeEui64(otInstance *, uint8_t *) {}

void otPlatRadioSetPanId(otInstance *, uint16_t) {}

void otPlatRadioSetExtendedAddress(otInstance *, const otExtAddress *) {}

void otPlatRadioSetShortAddress(otInstance *, uint16_t) {}

void otPlatRadioSetPromiscuous(otInstance *, bool) {}

void otPlatRadioSetRxOnWhenIdle(otInstance *, bool) {}

bool otPlatRadioIsEnabled(otInstance *) { return true; }

otError otPlatRadioEnable(otInstance *) { return OT_ERROR_NONE; }

otError otPlatRadioDisable(otInstance *) { return OT_ERROR_NONE; }

otError otPlatRadioSleep(otInstance *) { return OT_ERROR_NONE; }

otError otPlatRadioReceive(otInstance *, uint8_t) { return OT_ERROR_NONE; }

otError otPlatRadioTransmit(otInstance *, otRadioFrame *aFrame) { return sPlatform->Transmit(aFrame); }

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *) { return &sPlatform->mTransmitFrame; }

int8_t otPlatRadioGetRssi(otInstance *) { return 0; }

otRadioCaps otPlatRadioGetCaps(otInstance *) { return OT_RADIO_CAPS_NONE; }

bool otPlatRadioGetPromiscuous(otInstance *) { return false; }

void otPlatRadioEnableSrcMatch(otInstance *, bool) {}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *, uint16_t) { return OT_ERROR_NONE; }

otError otPlatRadioAddSrcMatchExtEntry(otInstance *, const otExtAddress *) { return OT_ERROR_NONE; }

otError otPlatRadioClearSrcMatchShortEntry(otInstance *, uint16_t) { return OT_ERROR_NONE; }

otError otPlatRadioClearSrcMatchExtEntry(otInstance *, const otExtAddress *) { return OT_ERROR_NONE; }

void otPlatRadioClearSrcMatchShortEntries(otInstance *) {}

void otPlatRadioClearSrcMatchExtEntries(otInstance *) {}

otError otPlatRadioEnergyScan(otInstance *, uint8_t, uint16_t) { return OT_ERROR_NOT_IMPLEMENTED; }

otError otPlatRadioSetTransmitPower(otInstance *, int8_t) { return OT_ERROR_NOT_IMPLEMENTED; }

int8_t otPlatRadioGetReceiveSensitivity(otInstance *) { return -100; }

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *, int8_t) { return OT_ERROR_NONE; }

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *, int8_t *) { return OT_ERROR_NONE; }

otError otPlatRadioGetCoexMetrics(otInstance *, otRadioCoexMetrics *) { return OT_ERROR_NONE; }

otError otPlatRadioGetTransmitPower(otInstance *, int8_t *) { return OT_ERROR_NONE; }

bool otPlatRadioIsCoexEnabled(otInstance *) { return true; }

otError otPlatRadioSetCoexEnabled(otInstance *, bool) { return OT_ERROR_NOT_IMPLEMENTED; }

void otPlatReset(otInstance *) {}

otPlatResetReason otPlatGetResetReason(otInstance *) { return OT_PLAT_RESET_REASON_POWER_ON; }

void otPlatWakeHost(void) {}

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error = OT_ERROR_NONE;

    assert(aOutput != nullptr);

    for (uint16_t length = 0; length < aOutputLength; length++)
    {
        aOutput[length] = (uint8_t)rand();
    }

exit:
    return error;
}

void otPlatDiagSetOutputCallback(otInstance *, otPlatDiagOutputCallback, void *) {}

void otPlatDiagModeSet(bool) {}

bool otPlatDiagModeGet() { return false; }

void otPlatDiagChannelSet(uint8_t) {}

void otPlatDiagTxPowerSet(int8_t) {}

void otPlatDiagRadioReceived(otInstance *, otRadioFrame *, otError) {}

void otPlatDiagAlarmCallback(otInstance *) {}

OT_TOOL_WEAK void otPlatLog(otLogLevel, otLogRegion, const char *, ...) {}

OT_TOOL_WEAK void *otPlatCAlloc(size_t aNum, size_t aSize) { return calloc(aNum, aSize); }

OT_TOOL_WEAK void otPlatFree(void *aPtr) { free(aPtr); }
} // extern "C"
