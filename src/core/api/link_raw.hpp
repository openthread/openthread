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
 *   This file includes definitions for the Raw Link-Layer class.
 */

#ifndef LINK_RAW_HPP_
#define LINK_RAW_HPP_

#include <openthread/link_raw.h>

#include "openthread-core-config.h"
#include "common/timer.hpp"

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT || OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT || OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
#define OPENTHREAD_LINKRAW_TIMER_REQUIRED 1
#else
#define OPENTHREAD_LINKRAW_TIMER_REQUIRED 0
#endif

namespace ot {

class LinkRaw
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    LinkRaw(otInstance &aInstance);

    /**
     * This method returns true if the raw link-layer is enabled.
     *
     */
    bool IsEnabled() { return mEnabled; }

    /**
     * This method enables/disables the raw link-layer.
     *
     */
    void SetEnabled(bool aEnabled) { mEnabled = aEnabled; }

    /**
     * This method returns the capabilities of the raw link-layer.
     *
     */
    otRadioCaps GetCaps();

    /**
     * This method starts a (recurring) Receive on the link-layer.
     *
     */
    otError Receive(uint8_t aChannel, otLinkRawReceiveDone aCallback);

    /**
     * This method invokes the mReceiveDoneCallback, if set.
     *
     */
    void InvokeReceiveDone(otRadioFrame *aFrame, otError aError);

    /**
     * This method starts a (single) Transmit on the link-layer.
     *
     */
    otError Transmit(otRadioFrame *aFrame, otLinkRawTransmitDone aCallback);

    /**
     * This method invokes the mTransmitDoneCallback, if set.
     *
     */
    void InvokeTransmitDone(otRadioFrame *aFrame, bool aFramePending, otError aError);

    /**
     * This method starts a (single) Enery Scan on the link-layer.
     *
     */
    otError EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback);

    /**
     * This method invokes the mEnergyScanDoneCallback, if set.
     *
     */
    void InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi);

private:

    otInstance             &mInstance;
    bool                    mEnabled;
    uint8_t                 mReceiveChannel;
    otLinkRawReceiveDone    mReceiveDoneCallback;
    otLinkRawTransmitDone   mTransmitDoneCallback;
    otLinkRawEnergyScanDone mEnergyScanDoneCallback;

    otError DoTransmit(otRadioFrame *aFrame);

#if OPENTHREAD_LINKRAW_TIMER_REQUIRED

    enum TimerReason
    {
        kTimerReasonNone,
        kTimerReasonAckTimeout,
        kTimerReasonRetransmitTimeout,
        kTimerReasonEnergyScanComplete,
    };

    Timer                   mTimer;
    TimerReason             mTimerReason;

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    uint8_t                 mTransmitAttempts;
    uint8_t                 mCsmaAttempts;

    void StartCsmaBackoff(void);

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    enum
    {
        kInvalidRssiValue = 127
    };

    Tasklet                 mEnergyScanTask;
    int8_t                  mEnergyScanRssi;

    static void HandleEnergyScanTask(void *aContext);
    void HandleEnergyScanTask(void);

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
};

}  // namespace ot

#endif  // LINK_RAW_HPP_
