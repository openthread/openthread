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
 *   This file implements the OpenThread Link Raw API.
 */

#include "openthread-core-config.h"

#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
#include "mac/mac.hpp"

#if OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API

using namespace ot;

LinkRaw::LinkRaw(Instance &aInstance)
    : mInstance(aInstance)
#if OPENTHREAD_LINKRAW_TIMER_REQUIRED
    , mTimer(aInstance, &LinkRaw::HandleTimer, this)
    , mTimerReason(kTimerReasonNone)
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    , mTimerMicro(aInstance, &LinkRaw::HandleTimer, this)
#endif
#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    , mEnergyScanTask(aInstance, &LinkRaw::HandleEnergyScanTask, this)
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    , mReceiveChannel(OPENTHREAD_CONFIG_DEFAULT_CHANNEL)
    , mReceiveDoneCallback(NULL)
    , mTransmitDoneCallback(NULL)
    , mEnergyScanDoneCallback(NULL)
{
    // Query the capabilities to check asserts
    (void)GetCaps();
}

otError LinkRaw::SetEnabled(bool aEnabled)
{
    otError error = OT_ERROR_NONE;

    otLogInfoPlat(mInstance, "LinkRaw Enabled=%d", aEnabled ? 1 : 0);

#if OPENTHREAD_MTD || OPENTHREAD_FTD
    VerifyOrExit(!static_cast<Instance &>(mInstance).GetThreadNetif().IsUp(), error = OT_ERROR_INVALID_STATE);
#endif

    if (aEnabled)
    {
        otPlatRadioEnable(&mInstance);
    }
    else
    {
        otPlatRadioDisable(&mInstance);
    }

    mEnabled = aEnabled;

#if OPENTHREAD_MTD || OPENTHREAD_FTD
exit:
#endif
    return error;
}

otError LinkRaw::SetPanId(uint16_t aPanId)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioSetPanId(&mInstance, aPanId);
    mPanId = aPanId;

exit:
    return error;
}

otError LinkRaw::SetChannel(uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    mReceiveChannel = aChannel;

exit:
    return error;
}

otError LinkRaw::SetExtAddress(const otExtAddress &aExtAddress)
{
    otExtAddress addr;
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    for (size_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress.m8[7 - i];
    }

    otPlatRadioSetExtendedAddress(&mInstance, &addr);
    mExtAddress = aExtAddress;

exit:
    return error;
}

otError LinkRaw::SetShortAddress(uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioSetShortAddress(&mInstance, aShortAddress);
    mShortAddress = aShortAddress;

exit:
    return error;
}

otRadioCaps LinkRaw::GetCaps(void) const
{
    otRadioCaps RadioCaps = otPlatRadioGetCaps(&mInstance);

    // The radio shouldn't support a capability if it is being compile
    // time included into the raw link-layer code.

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    if ((RadioCaps & OT_RADIO_CAPS_ACK_TIMEOUT) == 0)
    {
        RadioCaps = static_cast<otRadioCaps>(RadioCaps | OT_RADIO_CAPS_ACK_TIMEOUT);
    }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
    if ((RadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) == 0)
    {
        RadioCaps = static_cast<otRadioCaps>(RadioCaps | OT_RADIO_CAPS_TRANSMIT_RETRIES);
    }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
    if ((RadioCaps & OT_RADIO_CAPS_ENERGY_SCAN) == 0)
    {
        RadioCaps = static_cast<otRadioCaps>(RadioCaps | OT_RADIO_CAPS_ENERGY_SCAN);
    }
#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    return RadioCaps;
}

otError LinkRaw::Receive(otLinkRawReceiveDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mReceiveDoneCallback = aCallback;
        error                = otPlatRadioReceive(&mInstance, mReceiveChannel);
    }

    return error;
}

void LinkRaw::InvokeReceiveDone(otRadioFrame *aFrame, otError aError)
{
    if (mReceiveDoneCallback)
    {
        if (aError == OT_ERROR_NONE)
        {
            otLogInfoPlat(&mInstance, "LinkRaw Invoke Receive Done (%d bytes)", aFrame->mLength);
            mReceiveDoneCallback(&mInstance, aFrame, aError);
        }
        else
        {
            otLogWarnPlat(&mInstance, "LinkRaw Invoke Receive Done (err=0x%x)", aError);
        }
    }
}

otError LinkRaw::Transmit(otRadioFrame *aFrame, otLinkRawTransmitDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mTransmitDoneCallback = aCallback;

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
        OT_UNUSED_VARIABLE(aFrame);
        mTransmitAttempts = 0;
        mCsmaAttempts     = 0;

        // Start the transmission backlog logic
        StartCsmaBackoff();
        error = OT_ERROR_NONE;
#else
        // Let the hardware do the transmission logic
        error = otPlatRadioTransmit(&mInstance, aFrame);
#endif
    }

    return error;
}

void LinkRaw::InvokeTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    otLogDebgPlat(&mInstance, "LinkRaw Transmit Done (err=0x%x)", aError);

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
    mTimer.Stop();
#endif

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    if (aError == OT_ERROR_CHANNEL_ACCESS_FAILURE)
    {
        if (mCsmaAttempts < Mac::kMaxCSMABackoffs)
        {
            mCsmaAttempts++;
            StartCsmaBackoff();
            goto exit;
        }
    }
    else
    {
        mCsmaAttempts = 0;
    }

    if (aError == OT_ERROR_NO_ACK)
    {
        if (mTransmitAttempts < aFrame->mMaxTxAttempts)
        {
            mTransmitAttempts++;
            StartCsmaBackoff();
            goto exit;
        }
    }

#endif

    // Transition back to receive state on previous channel
    otPlatRadioReceive(&mInstance, mReceiveChannel);

    if (mTransmitDoneCallback)
    {
        if (aError == OT_ERROR_NONE)
        {
            otLogInfoPlat(&mInstance, "LinkRaw Invoke Transmit Done");
        }
        else
        {
            otLogWarnPlat(&mInstance, "LinkRaw Invoke Transmit Failed (err=0x%x)", aError);
        }

        mTransmitDoneCallback(&mInstance, aFrame, aAckFrame, aError);
        mTransmitDoneCallback = NULL;
    }

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
exit:
    return;
#endif
}

otError LinkRaw::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback)
{
    otError error = OT_ERROR_INVALID_STATE;

    if (mEnabled)
    {
        mEnergyScanDoneCallback = aCallback;

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
        // Start listening on the scan channel
        otPlatRadioReceive(&mInstance, aScanChannel);

        // Reset the RSSI value and start scanning
        mEnergyScanRssi = kInvalidRssiValue;
        mTimerReason    = kTimerReasonEnergyScanComplete;
        mTimer.Start(aScanDuration);
        mEnergyScanTask.Post();
#else
        // Do the HW offloaded energy scan
        error = otPlatRadioEnergyScan(&mInstance, aScanChannel, aScanDuration);
#endif
    }

    return error;
}

void LinkRaw::InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    if (IsEnabled() && mEnergyScanDoneCallback)
    {
        mEnergyScanDoneCallback(&mInstance, aEnergyScanMaxRssi);
        mEnergyScanDoneCallback = NULL;
    }
}

void LinkRaw::TransmitStarted(otRadioFrame *aFrame)
{
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

    // If we are implementing the ACK timeout logic, start a timer here (if ACK request)
    // to fire if we don't get a transmit done callback in time.
    if (static_cast<Mac::Frame *>(aFrame)->GetAckRequest() &&
        !(otPlatRadioGetCaps(&mInstance) & OT_RADIO_CAPS_ACK_TIMEOUT))
    {
        otLogDebgPlat(aInstance, "LinkRaw Starting AckTimeout Timer");
        mTimerReason = kTimerReasonAckTimeout;
        mTimer.Start(Mac::kAckTimeout);
    }

#else
    OT_UNUSED_VARIABLE(aFrame);
#endif
}

#if OPENTHREAD_LINKRAW_TIMER_REQUIRED

void LinkRaw::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<LinkRaw>().HandleTimer();
}

void LinkRaw::HandleTimer(void)
{
    TimerReason timerReason = mTimerReason;
    mTimerReason            = kTimerReasonNone;

    switch (timerReason)
    {
#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

    case kTimerReasonAckTimeout:
    {
        // Transition back to receive state on previous channel
        otPlatRadioReceive(&mInstance, mReceiveChannel);

        // Invoke completion callback for transmit
        InvokeTransmitDone(otPlatRadioGetTransmitBuffer(&mInstance), NULL, OT_ERROR_NO_ACK);
        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    case kTimerReasonRetransmitTimeout:
    {
        otRadioFrame *aFrame = otPlatRadioGetTransmitBuffer(&mInstance);

        // Start the  transmit now
        otError error = otPlatRadioTransmit(&mInstance, aFrame);

        if (error != OT_ERROR_NONE)
        {
            InvokeTransmitDone(aFrame, NULL, error);
        }

        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    case kTimerReasonEnergyScanComplete:
    {
        // Invoke completion callback for the energy scan
        InvokeEnergyScanDone(mEnergyScanRssi);
        break;
    }

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    default:
        assert(false);
    }
}

#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

void LinkRaw::StartCsmaBackoff(void)
{
    uint32_t backoffExponent = Mac::kMinBE + mTransmitAttempts + mCsmaAttempts;
    uint32_t backoff;

    if (backoffExponent > Mac::kMaxBE)
    {
        backoffExponent = Mac::kMaxBE;
    }

    backoff = Random::GetUint32InRange(0, 1U << backoffExponent);
    backoff *= (static_cast<uint32_t>(Mac::kUnitBackoffPeriod) * OT_RADIO_SYMBOL_TIME);

    otLogDebgPlat(&mInstance, "LinkRaw Starting RetransmitTimeout Timer (%d ms)", backoff);
    mTimerReason = kTimerReasonRetransmitTimeout;

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    mTimerMicro.Start(backoff);
#else  // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    mTimer.Start(backoff / 1000UL);
#endif // OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
}

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

void LinkRaw::HandleEnergyScanTask(Tasklet &aTasklet)
{
    aTasklet.GetOwner<LinkRaw>().HandleEnergyScanTask();
}

void LinkRaw::HandleEnergyScanTask(void)
{
    // Only process task if we are still energy scanning
    if (mTimerReason == kTimerReasonEnergyScanComplete)
    {
        int8_t rssi = otPlatRadioGetRssi(&mInstance);

        // Only apply the RSSI if it was a valid value
        if (rssi != kInvalidRssiValue)
        {
            if ((mEnergyScanRssi == kInvalidRssiValue) || (rssi > mEnergyScanRssi))
            {
                mEnergyScanRssi = rssi;
            }
        }

        // Post another instance of tha task, since we are
        // still doing the energy scan.
        mEnergyScanTask.Post();
    }
}

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

otError otLinkRawSetEnable(otInstance *aInstance, bool aEnabled)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetEnabled(aEnabled);
}

bool otLinkRawIsEnabled(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled();
}

otError otLinkSetShortAddress(otInstance *aInstance, uint16_t aShortAddress)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetShortAddress(aShortAddress);
}

bool otLinkRawGetPromiscuous(otInstance *aInstance)
{
    return otPlatRadioGetPromiscuous(aInstance);
}

otError otLinkRawSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat(aInstance, "LinkRaw Promiscuous=%d", aEnable ? 1 : 0);

    otPlatRadioSetPromiscuous(aInstance, aEnable);

exit:
    return error;
}

otError otLinkRawSleep(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otLogInfoPlat(aInstance, "LinkRaw Sleep");

    error = otPlatRadioSleep(aInstance);

exit:
    return error;
}

otError otLinkRawReceive(otInstance *aInstance, otLinkRawReceiveDone aCallback)
{
    otLogInfoPlat(aInstance, "LinkRaw Recv");
    return static_cast<Instance *>(aInstance)->GetLinkRaw().Receive(aCallback);
}

otRadioFrame *otLinkRawGetTransmitBuffer(otInstance *aInstance)
{
    otRadioFrame *buffer = NULL;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled());

    buffer = otPlatRadioGetTransmitBuffer(aInstance);

exit:
    return buffer;
}

otError otLinkRawTransmit(otInstance *aInstance, otRadioFrame *aFrame, otLinkRawTransmitDone aCallback)
{
    otLogInfoPlat(aInstance, "LinkRaw Transmit (%d bytes on channel %d)", aFrame->mLength, aFrame->mChannel);
    return static_cast<Instance *>(aInstance)->GetLinkRaw().Transmit(aFrame, aCallback);
}

int8_t otLinkRawGetRssi(otInstance *aInstance)
{
    return otPlatRadioGetRssi(aInstance);
}

otRadioCaps otLinkRawGetCaps(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetCaps();
}

otError otLinkRawEnergyScan(otInstance *            aInstance,
                            uint8_t                 aScanChannel,
                            uint16_t                aScanDuration,
                            otLinkRawEnergyScanDone aCallback)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().EnergyScan(aScanChannel, aScanDuration, aCallback);
}

otError otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioEnableSrcMatch(aInstance, aEnable);

exit:
    return error;
}

otError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioAddSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otExtAddress addr;
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    for (uint8_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(addr) - 1 - i];
    }

    error = otPlatRadioAddSrcMatchExtEntry(aInstance, &addr);

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    error = otPlatRadioClearSrcMatchShortEntry(aInstance, aShortAddress);

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otExtAddress addr;
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    for (uint8_t i = 0; i < sizeof(addr); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(addr) - 1 - i];
    }

    error = otPlatRadioClearSrcMatchExtEntry(aInstance, &addr);

exit:
    return error;
}

otError otLinkRawSrcMatchClearShortEntries(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioClearSrcMatchShortEntries(aInstance);

exit:
    return error;
}

otError otLinkRawSrcMatchClearExtEntries(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(static_cast<Instance *>(aInstance)->GetLinkRaw().IsEnabled(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioClearSrcMatchExtEntries(aInstance);

exit:
    return error;
}

#if OPENTHREAD_RADIO
void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    static_cast<Instance *>(aInstance)->GetLinkRaw().InvokeReceiveDone(aFrame, aError);
}

void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError)
{
    static_cast<Instance *>(aInstance)->GetLinkRaw().InvokeTransmitDone(aFrame, aAckFrame, aError);
}

void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame)
{
    static_cast<Instance *>(aInstance)->GetLinkRaw().TransmitStarted(aFrame);
}

void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi)
{
    VerifyOrExit(otInstanceIsInitialized(aInstance));

    static_cast<Instance *>(aInstance)->GetLinkRaw().InvokeEnergyScanDone(aEnergyScanMaxRssi);

exit:
    return;
}

otDeviceRole otThreadGetDeviceRole(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_DEVICE_ROLE_DISABLED;
}

uint8_t otLinkGetChannel(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetChannel();
}

otError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetChannel(aChannel);
}

otPanId otLinkGetPanId(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetPanId();
}

otError otLinkSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetPanId(aPanId);
}

const otExtAddress *otLinkGetExtendedAddress(otInstance *aInstance)
{
    return &static_cast<Instance *>(aInstance)->GetLinkRaw().GetExtAddress();
}

otError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().SetExtAddress(*aExtAddress);
}

uint16_t otLinkGetShortAddress(otInstance *aInstance)
{
    return static_cast<Instance *>(aInstance)->GetLinkRaw().GetShortAddress();
}

#endif // OPENTHREAD_RADIO

#endif // OPENTHREAD_RADIO || OPENTHREAD_ENABLE_RAW_LINK_API
