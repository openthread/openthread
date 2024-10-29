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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/tasklet.h>
#include <openthread/tcat.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/ble.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/dso_transport.h>
#include <openthread/platform/entropy.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>
#include <openthread/platform/toolchain.h>
#include <openthread/platform/trel.h>
#include <openthread/platform/udp.h>

using namespace ot;

namespace ot {

FakePlatform *FakePlatform::sPlatform = nullptr;

FakePlatform::FakePlatform()
{
    assert(sPlatform == nullptr);
    sPlatform = this;

    mTransmitFrame.mPsdu = mTransmitBuffer;

#if OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
#if OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE
    mInstance = otInstanceInitMultiple(0);
#else
    {
        size_t instanceBufferLength = 0;
        void  *instanceBuffer       = nullptr;

        otInstanceInit(nullptr, &instanceBufferLength);

        instanceBuffer = malloc(instanceBufferLength);
        assert(instanceBuffer != nullptr);
        memset(instanceBuffer, 0, instanceBufferLength);

        mInstance = otInstanceInit(instanceBuffer, &instanceBufferLength);
    }
#endif
#else
    mInstance = otInstanceInitSingle();
#endif
}

FakePlatform::~FakePlatform()
{
    otInstanceFinalize(mInstance);
    sPlatform = nullptr;
}

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
void FakePlatform::StartMicroAlarm(uint32_t aT0, uint32_t aDt)
{
    uint64_t start = mNow;
    uint32_t now   = mNow;

    if (static_cast<int32_t>(aT0 - now) > 0 || static_cast<int32_t>(aT0 - now) + static_cast<int64_t>(aDt) > 0)
    {
        start += static_cast<uint64_t>(aDt) + static_cast<int32_t>(aT0 - now);
    }

    mMicroAlarmStart = start;
}

void FakePlatform::StopMicroAlarm() { mMicroAlarmStart = kAlarmStop; }
#endif

void FakePlatform::StartMilliAlarm(uint32_t aT0, uint32_t aDt)
{
    uint64_t start = mNow - (mNow % OT_US_PER_MS);
    uint32_t now   = (mNow / OT_US_PER_MS);

    if (static_cast<int32_t>(aT0 - now) > 0 || static_cast<int32_t>(aT0 - now) + static_cast<int64_t>(aDt) > 0)
    {
        start += (static_cast<uint64_t>(aDt) + static_cast<int32_t>(aT0 - now)) * OT_US_PER_MS;
    }

    mMilliAlarmStart = start;
}

void FakePlatform::StopMilliAlarm() { mMilliAlarmStart = kAlarmStop; }

void FakePlatform::ProcessAlarm(uint64_t &aTimeout)
{
    uint64_t end = mNow + aTimeout;

    uint64_t *alarm = &end;
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    if (mMicroAlarmStart < *alarm)
    {
        alarm = &mMicroAlarmStart;
    }
#endif
    if (mMilliAlarmStart < *alarm)
    {
        alarm = &mMilliAlarmStart;
    }

    if (mNow < *alarm)
    {
        aTimeout -= *alarm - mNow;
        mNow = *alarm;
    }
    *alarm = kAlarmStop;
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    if (alarm == &mMicroAlarmStart)
    {
        otPlatAlarmMicroFired(mInstance);
    }
    else
#endif
        if (alarm == &mMilliAlarmStart)
    {
        otPlatAlarmMilliFired(mInstance);
    }
}

uint64_t FakePlatform::Run(uint64_t aTimeoutInUs)
{
    if (otTaskletsArePending(mInstance))
    {
        otTaskletsProcess(mInstance);
    }
    else
    {
        ProcessAlarm(aTimeoutInUs);
    }

    return aTimeoutInUs;
}

void FakePlatform::GoInUs(uint64_t aTimeoutInUs)
{
    while ((aTimeoutInUs = Run(aTimeoutInUs)) > 0)
    {
        // nothing
    }
}

otError FakePlatform::Transmit(otRadioFrame *aFrame)
{
    otPlatRadioTxStarted(mInstance, aFrame);
    return OT_ERROR_NONE;
}

otError FakePlatform::SettingsGet(uint16_t aKey, uint16_t aIndex, uint8_t *aValue, uint16_t *aValueLength) const
{
    auto setting = mSettings.find(aKey);

    if (setting == mSettings.end())
    {
        return OT_ERROR_NOT_FOUND;
    }

    if (aIndex > setting->second.size())
    {
        return OT_ERROR_NOT_FOUND;
    }

    if (aValueLength == nullptr)
    {
        return OT_ERROR_NONE;
    }

    const auto &data = setting->second[aIndex];

    if (aValue == nullptr)
    {
        *aValueLength = data.size();
        return OT_ERROR_NONE;
    }

    if (*aValueLength >= data.size())
    {
        *aValueLength = data.size();
    }

    memcpy(aValue, &data[0], *aValueLength);

    return OT_ERROR_NONE;
}

otError FakePlatform::SettingsSet(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    auto setting = std::vector<uint8_t>(aValue, aValue + aValueLength);

    mSettings[aKey].clear();
    mSettings[aKey].push_back(setting);

    return OT_ERROR_NONE;
}

otError FakePlatform::SettingsAdd(uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    auto setting = std::vector<uint8_t>(aValue, aValue + aValueLength);

    mSettings[aKey].push_back(setting);

    return OT_ERROR_NONE;
}

otError FakePlatform::SettingsDelete(uint16_t aKey, int aIndex)
{
    auto setting = mSettings.find(aKey);
    if (setting == mSettings.end())
    {
        return OT_ERROR_NOT_FOUND;
    }

    if (static_cast<std::size_t>(aIndex) >= setting->second.size())
    {
        return OT_ERROR_NOT_FOUND;
    }
    setting->second.erase(setting->second.begin() + aIndex);
    return OT_ERROR_NONE;
}

void FakePlatform::SettingsWipe() { mSettings.clear(); }

void FakePlatform::FlashInit() { memset(mFlash, 0xff, sizeof(mFlash)); }
void FakePlatform::FlashErase(uint8_t aSwapIndex)
{
    uint32_t address;

    assert(aSwapIndex < kFlashSwapNum);

    address = aSwapIndex ? kFlashSwapSize : 0;

    memset(mFlash + address, 0xff, kFlashSwapSize);
}

void FakePlatform::FlashRead(uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize) const
{
    uint32_t address;

    assert(aSwapIndex < kFlashSwapNum);
    assert(aSize <= kFlashSwapSize);
    assert(aOffset <= (kFlashSwapSize - aSize));

    address = aSwapIndex ? kFlashSwapSize : 0;

    memcpy(aData, mFlash + address + aOffset, aSize);
}

void FakePlatform::FlashWrite(uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    uint32_t address;

    assert(aSwapIndex < kFlashSwapNum);
    assert(aSize <= kFlashSwapSize);
    assert(aOffset <= (kFlashSwapSize - aSize));

    address = aSwapIndex ? kFlashSwapSize : 0;

    for (uint32_t index = 0; index < aSize; index++)
    {
        mFlash[address + aOffset + index] &= static_cast<const uint8_t *>(aData)[index];
    }
}

} // namespace ot

extern "C" {

void otTaskletsSignalPending(otInstance *) {}

void otPlatAlarmMilliStop(otInstance *) { FakePlatform::CurrentPlatform().StopMilliAlarm(); }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    FakePlatform::CurrentPlatform().StartMilliAlarm(aT0, aDt);
}

uint32_t otPlatAlarmMilliGetNow(void) { return FakePlatform::CurrentPlatform().GetNow() / OT_US_PER_MS; }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
void otPlatAlarmMicroStop(otInstance *) { FakePlatform::CurrentPlatform().StopMicroAlarm(); }
void otPlatAlarmMicroStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    FakePlatform::CurrentPlatform().StartMicroAlarm(aT0, aDt);
}
#endif

uint64_t otPlatTimeGet(void) { return FakePlatform::CurrentPlatform().GetNow(); }
uint16_t otPlatTimeGetXtalAccuracy(void) { return 0; }

uint32_t otPlatAlarmMicroGetNow(void) { return otPlatTimeGet(); }

void otPlatRadioGetIeeeEui64(otInstance *, uint8_t *aIeeeEui64)
{
    uint64_t eui64 = FakePlatform::CurrentPlatform().GetEui64();
    memcpy(aIeeeEui64, &eui64, sizeof(eui64));
}

void otPlatRadioSetPanId(otInstance *, uint16_t) {}

void otPlatRadioSetExtendedAddress(otInstance *, const otExtAddress *) {}

void otPlatRadioSetShortAddress(otInstance *, uint16_t) {}

void otPlatRadioSetPromiscuous(otInstance *, bool) {}

void otPlatRadioSetRxOnWhenIdle(otInstance *, bool) {}

bool otPlatRadioIsEnabled(otInstance *) { return true; }

otError otPlatRadioEnable(otInstance *) { return OT_ERROR_NONE; }

otError otPlatRadioDisable(otInstance *) { return OT_ERROR_NONE; }

otError otPlatRadioSleep(otInstance *) { return OT_ERROR_NONE; }

otError otPlatRadioReceive(otInstance *, uint8_t aChannel) { return FakePlatform::CurrentPlatform().Receive(aChannel); }

otError otPlatRadioTransmit(otInstance *, otRadioFrame *aFrame)
{
    return FakePlatform::CurrentPlatform().Transmit(aFrame);
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *) { return FakePlatform::CurrentPlatform().GetTransmitBuffer(); }

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

otError otPlatRadioConfigureEnhAckProbing(otInstance *, otLinkMetrics, otShortAddress, const otExtAddress *)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

// Add WEAK here because in some unit test there is an implementation for `otPlatRadioSetChannelTargetPower`
OT_TOOL_WEAK otError otPlatRadioSetChannelTargetPower(otInstance *, uint8_t, int16_t) { return OT_ERROR_NONE; }

void otPlatReset(otInstance *) {}

otPlatResetReason otPlatGetResetReason(otInstance *) { return OT_PLAT_RESET_REASON_POWER_ON; }

void otPlatWakeHost(void) {}

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error = OT_ERROR_NONE;

    assert(aOutput != nullptr);

    for (uint16_t length = 0; length < aOutputLength; length++)
    {
        aOutput[length] = static_cast<uint8_t>(rand());
    }

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

void *otPlatCAlloc(size_t aNum, size_t aSize) { return calloc(aNum, aSize); }

void otPlatFree(void *aPtr) { free(aPtr); }

bool otPlatInfraIfHasAddress(uint32_t, const otIp6Address *) { return false; }

otError otPlatInfraIfSendIcmp6Nd(uint32_t, const otIp6Address *, const uint8_t *, uint16_t) { return OT_ERROR_FAILED; }

otError otPlatInfraIfDiscoverNat64Prefix(uint32_t) { return OT_ERROR_FAILED; }

void otPlatDsoEnableListening(otInstance *, bool) {}

void otPlatDsoConnect(otPlatDsoConnection *, const otSockAddr *) {}

void otPlatDsoSend(otPlatDsoConnection *, otMessage *) {}

void otPlatDsoDisconnect(otPlatDsoConnection *, otPlatDsoDisconnectMode) {}

otError otPlatBleEnable(otInstance *) { return OT_ERROR_NONE; }

otError otPlatBleDisable(otInstance *) { return OT_ERROR_NONE; }

otError otPlatBleGetAdvertisementBuffer(otInstance *, uint8_t **) { return OT_ERROR_NO_BUFS; }

otError otPlatBleGapAdvStart(otInstance *, uint16_t) { return OT_ERROR_NONE; }

otError otPlatBleGapAdvStop(otInstance *) { return OT_ERROR_NONE; }

otError otPlatBleGapDisconnect(otInstance *) { return OT_ERROR_NONE; }

otError otPlatBleGattMtuGet(otInstance *, uint16_t *) { return OT_ERROR_NONE; }

otError otPlatBleGattServerIndicate(otInstance *, uint16_t, const otBleRadioPacket *) { return OT_ERROR_NONE; }

void otPlatBleGetLinkCapabilities(otInstance *, otBleLinkCapabilities *) {}

bool otPlatBleSupportsMultiRadio(otInstance *) { return false; }

otError otPlatBleGapAdvSetData(otInstance *, uint8_t *, uint16_t) { return OT_ERROR_NONE; }

void otPlatSettingsInit(otInstance *, const uint16_t *, uint16_t) {}

void otPlatSettingsDeinit(otInstance *) {}

otError otPlatSettingsGet(otInstance *, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    return FakePlatform::CurrentPlatform().SettingsGet(aKey, aIndex, aValue, aValueLength);
}

otError otPlatSettingsSet(otInstance *, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return FakePlatform::CurrentPlatform().SettingsSet(aKey, aValue, aValueLength);
}

otError otPlatSettingsAdd(otInstance *, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    return FakePlatform::CurrentPlatform().SettingsAdd(aKey, aValue, aValueLength);
}

otError otPlatSettingsDelete(otInstance *, uint16_t aKey, int aIndex)
{
    return FakePlatform::CurrentPlatform().SettingsDelete(aKey, aIndex);
}

void otPlatSettingsWipe(otInstance *) { FakePlatform::CurrentPlatform().SettingsWipe(); }

void otPlatFlashInit(otInstance *) { return FakePlatform::CurrentPlatform().FlashInit(); }

uint32_t otPlatFlashGetSwapSize(otInstance *) { return FakePlatform::CurrentPlatform().FlashGetSwapSize(); }

void otPlatFlashErase(otInstance *, uint8_t aSwapIndex) { FakePlatform::CurrentPlatform().FlashErase(aSwapIndex); }

void otPlatFlashRead(otInstance *, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    FakePlatform::CurrentPlatform().FlashRead(aSwapIndex, aOffset, aData, aSize);
}

void otPlatFlashWrite(otInstance *, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    FakePlatform::CurrentPlatform().FlashWrite(aSwapIndex, aOffset, aData, aSize);
}

void                      otPlatTrelEnable(otInstance *, uint16_t *) {}
void                      otPlatTrelDisable(otInstance *) {}
void                      otPlatTrelRegisterService(otInstance *, uint16_t, const uint8_t *, uint8_t) {}
void                      otPlatTrelSend(otInstance *, const uint8_t *, uint16_t, const otSockAddr *) {}
const otPlatTrelCounters *otPlatTrelGetCounters(otInstance *) { return nullptr; }
void                      otPlatTrelResetCounters(otInstance *) {}

otError otPlatUdpSocket(otUdpSocket *) { return OT_ERROR_NOT_IMPLEMENTED; }
otError otPlatUdpClose(otUdpSocket *) { return OT_ERROR_NOT_IMPLEMENTED; }
otError otPlatUdpBind(otUdpSocket *) { return OT_ERROR_NOT_IMPLEMENTED; }
otError otPlatUdpBindToNetif(otUdpSocket *, otNetifIdentifier) { return OT_ERROR_NOT_IMPLEMENTED; }
otError otPlatUdpConnect(otUdpSocket *) { return OT_ERROR_NOT_IMPLEMENTED; }
otError otPlatUdpSend(otUdpSocket *, otMessage *, const otMessageInfo *) { return OT_ERROR_NOT_IMPLEMENTED; }
otError otPlatUdpJoinMulticastGroup(otUdpSocket *, otNetifIdentifier, const otIp6Address *)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}
otError otPlatUdpLeaveMulticastGroup(otUdpSocket *, otNetifIdentifier, const otIp6Address *)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}

} // extern "C"
