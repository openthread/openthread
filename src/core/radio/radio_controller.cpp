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
 *   This file implements the radio controller.
 */

#include "radio_controller.hpp"

#if OPENTHREAD_CONFIG_RADIO_CONTROLLER_ENABLE
#include "instance/instance.hpp"

namespace ot {
RegisterLogModule("RadioCtl");

RadioController::RadioController(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCallbacks(aInstance)
{
}

Error RadioController::Enable(void)
{
    Error error;

    SuccessOrExit(error = Get<Radio>().Enable());

    for (auto &radio : mRadios)
    {
        radio.SetStateAndPriority(kStateEnabled, kPriorityMin);
    }

exit:
    return error;
}

Error RadioController::Disable(void)
{
    Error error;

    SuccessOrExit(error = Get<Radio>().Disable());

    for (auto &radio : mRadios)
    {
        radio.SetStateAndPriority(kStateDisabled, kPriorityMax);
    }

exit:
    return error;
}

Error RadioController::Sleep(Requester aRequester, bool aShouldHandleSleep)
{
    OT_ASSERT(aRequester < kNumRequesters);

    mRadios[aRequester].SetStateAndPriority(kStateSleep, kPrioritySleep);
    mRadios[aRequester].mShouldHandleSleep = aShouldHandleSleep;
    ReceiveOrSleep();

    return kErrorNone;
}

Error RadioController::Receive(uint8_t aChannel, Requester aRequester)
{
    static constexpr Priority kRxPriorities[] = {
        kPriorityReceiveMac,
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        kPriorityReceiveCsl,
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
        kPriorityReceiveWed,
#endif
    };

    OT_ASSERT(aRequester < kNumRequesters);

    mRadios[aRequester].SetStateAndPriority(kStateReceive, kRxPriorities[aRequester]);
    mRadios[aRequester].mChannel = aChannel;

    ReceiveOrSleep();

    return kErrorNone;
}

Error RadioController::ReceiveAt(uint8_t aChannel, uint32_t aStart, uint32_t aDuration)
{
    return Get<Radio>().ReceiveAt(aChannel, aStart, aDuration);
}

void RadioController::ReceiveOrSleep(void)
{
    Error   error       = kErrorNone;
    uint8_t index       = kNumRequesters;
    uint8_t maxPriority = kPriorityMin;

    // Find the radio in the 'sleep' or 'receive' state with the highest priority.
    for (uint8_t i = 0; i < kNumRequesters; i++)
    {
        if (mRadios[i].mPriority > maxPriority)
        {
            index       = i;
            maxPriority = mRadios[index].mPriority;
        }
    }

    VerifyOrExit(index < kNumRequesters);
    VerifyOrExit((mRadios[index].mState == kStateSleep) || (mRadios[index].mState == kStateReceive));

    if (mRadios[index].mState == kStateSleep)
    {
        if (mRadios[index].mShouldHandleSleep)
        {
            error = Get<Radio>().Sleep();
        }
    }
    else if (mRadios[index].mState == kStateReceive)
    {
        error = Get<Radio>().Receive(mRadios[index].mChannel);
    }
    else
    {
        OT_ASSERT(false);
    }

exit:
    if (error != kErrorNone)
    {
        LogWarn("%s() failed, error: %s", (mRadios[index].mState == kStateSleep) ? "Sleep" : "Receive",
                ErrorToString(error));
    }
}

Error RadioController::Transmit(Mac::TxFrame &aFrame)
{
    Error error;

    SuccessOrExit(error = Get<Radio>().Transmit(aFrame));
    mRadios[kRequesterMac].SetStateAndPriority(kStateTransmit, kPriorityTransmit);

exit:
    return error;
}

Error RadioController::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    Error error;

    SuccessOrExit(error = Get<Radio>().EnergyScan(aScanChannel, aScanDuration));
    mRadios[kRequesterMac].SetStateAndPriority(kStateEnergyScan, kPriorityEnergyScan);

exit:
    return error;
}

void RadioController::EnergyScanDone(void)
{
    mRadios[kRequesterMac].SetStateAndPriority(kStateEnabled, kPriorityMin);
    ReceiveOrSleep();
}

void RadioController::TransmitDone(void)
{
    mRadios[kRequesterMac].SetStateAndPriority(kStateEnabled, kPriorityMin);
    ReceiveOrSleep();
}

void RadioController::Callbacks::HandleEnergyScanDone(int8_t aMaxRssi)
{
    Get<RadioController>().EnergyScanDone();
    Get<Mac::SubMac>().HandleEnergyScanDone(aMaxRssi);
}

void RadioController::Callbacks::HandleTransmitDone(Mac::TxFrame &aFrame, Mac::RxFrame *aAckFrame, Error aError)
{
    Get<RadioController>().TransmitDone();
    Get<Mac::SubMac>().HandleTransmitDone(aFrame, aAckFrame, aError);
}
} // namespace ot
#endif // OPENTHREAD_CONFIG_RADIO_CONTROLLER_ENABLE
