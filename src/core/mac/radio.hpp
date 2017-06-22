/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file defines the wrappers around the platform radio calls.
 */

#ifndef RADIO_HPP_
#define RADIO_HPP_

#include <openthread/platform/radio.h>
#include "common/timer.hpp"

namespace ot {
namespace Mac {

/**
 * The following are valid radio state transitions:
 *
 *                                    (Radio ON)
 *  +----------+  Enable()  +-------+  Receive() +---------+   Transmit()  +----------+
 *  |          |----------->|       |----------->|         |-------------->|          |
 *  | Disabled |            | Sleep |            | Receive |               | Transmit |
 *  |          |<-----------|       |<-----------|         |<--------------|          |
 *  +----------+  Disable() +-------+   Sleep()  +---------+   Receive()   +----------+
 *                                    (Radio OFF)                 or
 *                                                        signal TransmitDone
 */

class Radio
{
public:
    Radio();

    /**
     * Transition the radio from Receive to Sleep.
     * Turn off the radio.
     * Wrapper around otPlatRadioSleep
     *
     * @param[in] aInstance  The OpenThread instance structure.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Sleep.
     * @retval OT_ERROR_BUSY          The radio was transmitting
     * @retval OT_ERROR_INVALID_STATE The radio was disabled
     */
    otError Sleep(otInstance *aInstance);

    /**
     * Transitioning the radio from Sleep to Receive.
     * Turn on the radio.
     * Wrapper around otPlatRadioReceive
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     * @param[in]  aChannel   The channel to use for receiving.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Receive.
     * @retval OT_ERROR_INVALID_STATE The radio was disabled or transmitting.
     */
    otError Receive(otInstance *aInstance, uint8_t aChannel);

    /**
     * This method begins the transmit sequence on the radio.
     *
     * The caller must form the IEEE 802.15.4 frame in the buffer provided by otPlatRadioGetTransmitBuffer() before
     * requesting transmission.  The channel and transmit power are also included in the otRadioFrame structure.
     *
     * The transmit sequence consists of:
     * 1. Transitioning the radio to Transmit from Receive.
     * 2. Transmits the psdu on the given channel and at the given transmit power.
     *
     * Wrapper around otPlatRadioTransmit
     *
     * @param[in] aInstance  The OpenThread instance structure.
     * @param[in] aFrame     A pointer to the frame that will be transmitted.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
     * @retval OT_ERROR_INVALID_STATE The radio was not in the Receive state.
     */
    otError Transmit(otInstance *aInstance, otRadioFrame *aFrame);

    /**
     * This function handles transmit done signals, should be called from Mac::TransmitDoneTask
     *
     * @retval OT_ERROR_NONE  Successfully handled transmit done.
     */
    otError TransmitDone(void);

    /**
     * This method returns the amount of time the radio has spent in Tx mode.
     *
     * @returns  A pointer to the total Tx time.
     *
     */
    const uint32_t *GetTxTotalTime(void) { return &mTxTotal; }

    /**
     * This method returns the amount of time the radio has spent in Rx mode.
     *
     * @returns  A pointer to the total Rx time.
     *
     */
    const uint32_t *GetRxTotalTime(void) { return &mRxTotal; }

private:
    uint32_t mRxTotal;
    uint32_t mTxTotal;
    uint32_t mLastChange;
};

} // namespace Mac
} // namespace ot

#endif // RADIO_HPP_
