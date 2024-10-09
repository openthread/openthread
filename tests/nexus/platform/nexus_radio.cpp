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

#include <openthread/platform/radio.h>

#include "nexus_core.hpp"
#include "nexus_node.hpp"

namespace ot {
namespace Nexus {

//---------------------------------------------------------------------------------------------------------------------
// otPlatRadio APIs

extern "C" {

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_RADIO_CAPS_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return Radio::kRadioSensetivity;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    uint32_t nodeId = AsNode(aInstance).GetInstance().GetId();

    memset(aIeeeEui64, 0, sizeof(Mac::ExtAddress));

    aIeeeEui64[6] = (nodeId >> 8) & 0xff;
    aIeeeEui64[7] = nodeId & 0xff;
}

void otPlatRadioSetPanId(otInstance *aInstance, otPanId aPanId) { AsNode(aInstance).mRadio.mPanId = aPanId; }

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    AsNode(aInstance).mRadio.mExtAddress.Set(aExtAddress->m8, Mac::ExtAddress::kReverseByteOrder);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, otShortAddress aShortAddress)
{
    AsNode(aInstance).mRadio.mShortAddress = aShortAddress;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance) { return AsNode(aInstance).mRadio.mPromiscuous; }

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable) { AsNode(aInstance).mRadio.mPromiscuous = aEnable; }

otRadioState otPlatRadioGetState(otInstance *aInstance) { return AsNode(aInstance).mRadio.mState; }

otError otPlatRadioEnable(otInstance *aInstance)
{
    Error  error = kErrorNone;
    Radio &radio = AsNode(aInstance).mRadio;

    VerifyOrExit(radio.mState == Radio::kStateDisabled, error = kErrorFailed);
    radio.mState = Radio::kStateSleep;

exit:
    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    AsNode(aInstance).mRadio.mState = Radio::kStateDisabled;
    return kErrorNone;
}

bool otPlatRadioIsEnabled(otInstance *aInstance) { return AsNode(aInstance).mRadio.mState != Radio::kStateDisabled; }

otError otPlatRadioSleep(otInstance *aInstance)
{
    Error  error = kErrorNone;
    Radio &radio = AsNode(aInstance).mRadio;

    VerifyOrExit(radio.mState != Radio::kStateDisabled, error = kErrorInvalidState);
    VerifyOrExit(radio.mState != Radio::kStateTransmit, error = kErrorBusy);
    radio.mState = Radio::kStateSleep;

exit:
    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    Error  error = kErrorNone;
    Radio &radio = AsNode(aInstance).mRadio;

    VerifyOrExit(radio.mState != Radio::kStateDisabled, error = kErrorInvalidState);
    radio.mState   = Radio::kStateReceive;
    radio.mChannel = aChannel;

exit:
    return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance) { return &AsNode(aInstance).mRadio.mTxFrame; }

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    Error  error = kErrorNone;
    Radio &radio = AsNode(aInstance).mRadio;

    VerifyOrExit(radio.mState == Radio::kStateReceive, error = kErrorInvalidState);
    OT_ASSERT(aFrame == &AsNode(aInstance).mRadio.mTxFrame);
    radio.mState = Radio::kStateTransmit;

    Core::Get().MarkPendingAction();

exit:
    return error;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return Radio::kRadioSensetivity;
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    AsNode(aInstance).mRadio.mSrcMatchEnabled = aEnable;
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress)
{
    Error  error = kErrorNone;
    Radio &radio = AsNode(aInstance).mRadio;

    VerifyOrExit(!radio.mSrcMatchShortEntries.Contains(aShortAddress));
    error = radio.mSrcMatchShortEntries.PushBack(aShortAddress);

exit:
    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Error           error = kErrorNone;
    Radio          &radio = AsNode(aInstance).mRadio;
    Mac::ExtAddress extAddress;

    extAddress.Set(aExtAddress->m8, Mac::ExtAddress::kReverseByteOrder);

    VerifyOrExit(!radio.mSrcMatchExtEntries.Contains(extAddress));
    error = radio.mSrcMatchExtEntries.PushBack(extAddress);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress)
{
    Error     error = kErrorNone;
    Radio    &radio = AsNode(aInstance).mRadio;
    uint16_t *entry;

    entry = radio.mSrcMatchShortEntries.Find(aShortAddress);
    VerifyOrExit(entry != nullptr, error = kErrorNoAddress);

    radio.mSrcMatchShortEntries.Remove(*entry);

exit:
    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    Error            error = kErrorNone;
    Radio           &radio = AsNode(aInstance).mRadio;
    Mac::ExtAddress  extAddress;
    Mac::ExtAddress *entry;

    extAddress.Set(aExtAddress->m8, Mac::ExtAddress::kReverseByteOrder);

    entry = radio.mSrcMatchExtEntries.Find(extAddress);
    VerifyOrExit(entry != nullptr, error = kErrorNoAddress);

    radio.mSrcMatchExtEntries.Remove(*entry);

exit:
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    AsNode(aInstance).mRadio.mSrcMatchShortEntries.Clear();
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance) { AsNode(aInstance).mRadio.mSrcMatchExtEntries.Clear(); }

// Not supported

otError otPlatRadioEnergyScan(otInstance *, uint8_t, uint16_t) { return kErrorNotImplemented; }

otError otPlatRadioGetTransmitPower(otInstance *, int8_t *) { return kErrorNotImplemented; }
otError otPlatRadioSetTransmitPower(otInstance *, int8_t) { return kErrorNotImplemented; }
otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *, int8_t *) { return kErrorNotImplemented; }
otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *, int8_t) { return kErrorNotImplemented; }
otError otPlatRadioGetFemLnaGain(otInstance *, int8_t *) { return kErrorNotImplemented; }
otError otPlatRadioSetFemLnaGain(otInstance *, int8_t) { return kErrorNotImplemented; }
bool    otPlatRadioIsCoexEnabled(otInstance *) { return false; }
otError otPlatRadioSetCoexEnabled(otInstance *, bool) { return kErrorNotImplemented; }
otError otPlatRadioGetCoexMetrics(otInstance *, otRadioCoexMetrics *) { return kErrorNotImplemented; }
otError otPlatRadioEnableCsl(otInstance *, uint32_t, otShortAddress, const otExtAddress *) { return kErrorNone; }
otError otPlatRadioResetCsl(otInstance *) { return kErrorNotImplemented; }
void    otPlatRadioUpdateCslSampleTime(otInstance *, uint32_t) {}
uint8_t otPlatRadioGetCslAccuracy(otInstance *) { return 0; }
otError otPlatRadioSetChannelTargetPower(otInstance *, uint8_t, int16_t) { return kErrorNotImplemented; }
otError otPlatRadioClearCalibratedPowers(otInstance *) { return kErrorNotImplemented; }
otError otPlatRadioAddCalibratedPower(otInstance *, uint8_t, int16_t, const uint8_t *, uint16_t)
{
    return kErrorNotImplemented;
}

} // extern "C"

//---------------------------------------------------------------------------------------------------------------------
// Radio

bool Radio::CanReceiveOnChannel(uint8_t aChannel) const
{
    bool canRx = false;

    switch (mState)
    {
    case kStateReceive:
    case kStateTransmit:
        break;
    default:
        ExitNow();
    }

    VerifyOrExit(mChannel == aChannel);
    canRx = true;

exit:
    return canRx;
}

bool Radio::Matches(const Mac::Address &aAddress, Mac::PanId aPanId) const
{
    bool matches = false;

    if (aAddress.IsShort())
    {
        VerifyOrExit(aAddress.IsBroadcast() || aAddress.GetShort() == mShortAddress);
    }
    else if (aAddress.IsExtended())
    {
        VerifyOrExit(aAddress.GetExtended() == mExtAddress);
    }

    if ((aPanId != Mac::kPanIdBroadcast) && (mPanId != Mac::kPanIdBroadcast))
    {
        VerifyOrExit(mPanId == aPanId);
    }

    matches = true;

exit:
    return matches;
}

bool Radio::HasFramePendingFor(const Mac::Address &aAddress) const
{
    bool hasPending = false;

    if (!mSrcMatchEnabled)
    {
        // Always mark frame pending when `SrcMatch` is disabled.
        hasPending = true;
        ExitNow();
    }

    if (aAddress.IsShort())
    {
        hasPending = mSrcMatchShortEntries.Contains(aAddress.GetShort());
    }
    else if (aAddress.IsExtended())
    {
        hasPending = mSrcMatchExtEntries.Contains(aAddress.GetExtended());
    }

exit:
    return hasPending;
}

//---------------------------------------------------------------------------------------------------------------------
// Radio::Frame

Radio::Frame::Frame(void)
{
    ClearAllBytes(*this);
    mPsdu = &mPsduBuffer[0];
}

Radio::Frame::Frame(const Frame &aFrame)
{
    ClearAllBytes(*this);
    mPsdu = &mPsduBuffer[0];

    mLength    = aFrame.mLength;
    mChannel   = aFrame.mChannel;
    mRadioType = aFrame.mRadioType;
    memcpy(mPsdu, aFrame.mPsdu, mLength);
}

} // namespace Nexus
} // namespace ot
