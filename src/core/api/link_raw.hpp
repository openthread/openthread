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

#include "openthread-core-config.h"

#include <openthread/link_raw.h>

#include "common/timer.hpp"

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT || OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT || \
    OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
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
     * @param[in]   aInstance   A reference to the OpenThread instance.
     *
     */
    explicit LinkRaw(Instance &aInstance);

    /**
     * This method returns true if the raw link-layer is enabled.
     *
     * @returns true if enabled, false otherwise.
     *
     */
    bool IsEnabled(void) const { return mEnabled; }

    /**
     * This method enables/disables the raw link-layer.
     *
     * @param[in]   aEnabled    Whether enable raw link-layer.
     *
     * @retval OT_ERROR_INVALID_STATE   Thread stack is enabled.
     * @retval OT_ERROR_NONE            Successfully enabled raw link.
     *
     */
    otError SetEnabled(bool aEnabled);

    /**
     * This method returns the capabilities of the raw link-layer.
     *
     * @returns The radio capability bit vector. The stack enables or disables some functions based on this value.
     *
     */
    otRadioCaps GetCaps(void) const;

    /**
     * This method starts a (recurring) Receive on the link-layer.
     *
     * @param[in]  aCallback    A pointer to a function called on receipt of a IEEE 802.15.4 frame.
     *
     * @retval OT_ERROR_NONE             Successfully transitioned to Receive.
     * @retval OT_ERROR_INVALID_STATE    The radio was disabled or transmitting.
     *
     */
    otError Receive(otLinkRawReceiveDone aCallback);

    /**
     * This method invokes the mReceiveDoneCallback, if set.
     *
     * @param[in]  aFrame    A pointer to the received frame or NULL if the receive operation failed.
     * @param[in]  aError    OT_ERROR_NONE when successfully received a frame, OT_ERROR_ABORT when reception
     *                       was aborted and a frame was not received, OT_ERROR_NO_BUFS when a frame could not be
     *                       received due to lack of rx buffer space.
     *
     */
    void InvokeReceiveDone(otRadioFrame *aFrame, otError aError);

    /**
     * This method starts a (single) Transmit on the link-layer.
     *
     * @param[in]  aFrame               A pointer to the frame that was transmitted.
     * @param[in]  aCallback            A pointer to a function called on completion of the transmission.
     *
     * @retval OT_ERROR_NONE            Successfully transitioned to Transmit.
     * @retval OT_ERROR_INVALID_STATE   The radio was not in the Receive state.
     *
     */
    otError Transmit(otRadioFrame *aFrame, otLinkRawTransmitDone aCallback);

    /**
     * This method invokes the mTransmitDoneCallback, if set.
     *
     * @param[in]  aFrame     A pointer to the frame that was transmitted.
     * @param[in]  aAckFrame  A pointer to the ACK frame, NULL if no ACK was received.
     * @param[in]  aError     OT_ERROR_NONE when the frame was transmitted, OT_ERROR_NO_ACK when the frame was
     *                        transmitted but no ACK was received, OT_ERROR_CHANNEL_ACCESS_FAILURE when the transmission
     *                        could not take place due to activity on the channel, OT_ERROR_ABORT when transmission was
     *                        aborted for other reasons.
     *
     */
    void InvokeTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);

    /**
     * This method starts a (single) Enery Scan on the link-layer.
     *
     * @param[in]  aScanChannel     The channel to perform the energy scan on.
     * @param[in]  aScanDuration    The duration, in milliseconds, for the channel to be scanned.
     * @param[in]  aCallback        A pointer to a function called on completion of a scanned channel.
     *
     * @retval OT_ERROR_NONE             Successfully started scanning the channel.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The radio doesn't support energy scanning.
     * @retval OT_ERROR_INVALID_STATE    If the raw link-layer isn't enabled.
     *
     */
    otError EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback);

    /**
     * This method invokes the mEnergyScanDoneCallback, if set.
     *
     * @param[in]   aEnergyScanMaxRssi  The max RSSI for energy scan.
     *
     */
    void InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi);

    /**
     * This method is called when the transmission has started.
     *
     * @param[in]  aFrame     A pointer to the frame that is being transmitted.
     *
     */
    void TransmitStarted(otRadioFrame *aFrame);

    /**
     * This function returns the short address.
     *
     * @returns short address.
     *
     */
    uint16_t GetShortAddress(void) const { return mShortAddress; }

    /**
     * This method updates short address.
     *
     * @param[in]   aShortAddress   The short address.
     *
     * @retval OT_ERROR_NONE             If successful.
     * @retval OT_ERROR_INVALID_STATE    If the raw link-layer isn't enabled.
     *
     */
    otError SetShortAddress(uint16_t aShortAddress);

    /**
     * This function returns PANID.
     *
     * @returns PANID.
     *
     */
    uint16_t GetPanId(void) const { return mPanId; }

    /**
     * This method updates PANID.
     *
     * @param[in]   aPanId          The PANID.
     *
     * @retval OT_ERROR_NONE             If successful.
     * @retval OT_ERROR_INVALID_STATE    If the raw link-layer isn't enabled.
     *
     */
    otError SetPanId(uint16_t aPanId);

    /**
     * This method gets the current receiving channel.
     *
     * @returns Current receiving channel.
     *
     */
    uint8_t GetChannel(void) const { return mReceiveChannel; }

    /**
     * This method sets the receiving channel.
     *
     * @param[in]  aChannel     The channel to use for receiving.
     *
     */
    otError SetChannel(uint8_t aChannel);

    /**
     * This function returns the extended address.
     *
     * @returns A reference to the extended address.
     *
     */
    const otExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * This method updates extended address.
     *
     * @param[in]   aExtAddress     The extended address.
     *
     * @retval OT_ERROR_NONE             If successful.
     * @retval OT_ERROR_INVALID_STATE    If the raw link-layer isn't enabled.
     *
     */
    otError SetExtAddress(const otExtAddress &aExtAddress);

private:
    otInstance &mInstance;

#if OPENTHREAD_LINKRAW_TIMER_REQUIRED

    enum TimerReason
    {
        kTimerReasonNone,
        kTimerReasonAckTimeout,
        kTimerReasonRetransmitTimeout,
        kTimerReasonEnergyScanComplete,
    };

    TimerMilli  mTimer;
    TimerReason mTimerReason;
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    TimerMicro mTimerMicro;
#endif

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

#endif // OPENTHREAD_LINKRAW_TIMER_REQUIRED

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

    uint8_t mTransmitAttempts;
    uint8_t mCsmaAttempts;

    void StartCsmaBackoff(void);

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT

#if OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    enum
    {
        kInvalidRssiValue = 127
    };

    Tasklet mEnergyScanTask;
    int8_t  mEnergyScanRssi;

    static void HandleEnergyScanTask(Tasklet &aTasklet);
    void        HandleEnergyScanTask(void);

#endif // OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN

    otExtAddress            mExtAddress;
    uint16_t                mPanId;
    uint16_t                mShortAddress;
    bool                    mEnabled;
    uint8_t                 mReceiveChannel;
    otLinkRawReceiveDone    mReceiveDoneCallback;
    otLinkRawTransmitDone   mTransmitDoneCallback;
    otLinkRawEnergyScanDone mEnergyScanDoneCallback;
};

} // namespace ot

#endif // LINK_RAW_HPP_
