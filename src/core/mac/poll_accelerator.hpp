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
 *   This file includes definitions for data poll accelerator.
 */

#ifndef OT_CORE_MAC_POLL_ACCELERATOR_HPP_
#define OT_CORE_MAC_POLL_ACCELERATOR_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_POLL_ACCELERATOR_ENABLE

#if !(OPENTHREAD_FTD || OPENTHREAD_MTD) || !OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE || \
    OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS || OPENTHREAD_CONFIG_MULTI_RADIO
#error "Poll Accelerator can't be enabled"
#endif

#include <openthread/platform/poll_accelerator.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"

namespace ot {

/**
 * Implements the Poll Accelerator.
 *
 */
class PollAccelerator : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the Poll Accelerator object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit PollAccelerator(Instance &aInstance);

    /**
     * Starts the Poll Accelerator.
     *
     * @param[in] aFrame          A reference to the data poll frame to transmit.
     * @param[in] aPollPeriod     The poll period in milliseconds.
     * @param[in] aMaxIterations  The maximum number of poll iterations.
     * @param[in] aStartTime      The start time of the polling.
     *
     * @retval kErrorNone          Successfully started Poll Accelerator.
     * @retval kErrorInvalidArgs   Invalid arguments provided.
     * @retval kErrorInvalidState  Poll Accelerator is already running.
     *
     */
    Error Start(Mac::TxFrame &aFrame, uint32_t aPollPeriod, uint32_t aMaxIterations, TimeMilli aStartTime);

    /**
     * Stops the Poll Accelerator.
     *
     * @retval kErrorNone          Successfully stopped Poll Accelerator.
     * @retval kErrorInvalidState  Poll Accelerator is not running.
     *
     */
    Error Stop(void);

    /**
     * Checks if Poll Accelerator is running.
     *
     * @retval TRUE   Poll Accelerator is running.
     * @retval FALSE  Poll Accelerator is not running.
     *
     */
    bool IsRunning(void) const { return mRunning; }

    /**
     * Handles Poll Accelerator completion event.
     *
     * This method is called when the Poll Accelerator operation completes,
     * either successfully, due to timeout, error, or interruption.
     *
     * @param[in] aIterationsDone  The number of poll iterations completed.
     * @param[in] aPrevAckFrame    A pointer to the previous ACK frame (from iteration N-1),
     *                             or NULL if this is the first iteration or no previous ACK.
     * @param[in] aTxFrame         A reference to the last transmitted MAC frame (Data Request).
     * @param[in] aAckFrame        A pointer to the last received ACK frame (from iteration N),
     *                             or NULL if no ACK was received.
     * @param[in] aTxError         The transmission error status.
     * @param[in] aRxFrame         A pointer to the received data frame, or NULL if no data received.
     * @param[in] aRxError         The reception error status.
     *
     */
    void HandlePollDone(uint32_t      aIterationsDone,
                        Mac::RxFrame *aPrevAckFrame,
                        Mac::TxFrame &aTxFrame,
                        Mac::RxFrame *aAckFrame,
                        Error         aTxError,
                        Mac::RxFrame *aRxFrame,
                        Error         aRxError);

private:
    static constexpr uint8_t kCsmaMinBe = 3; // macMinBE (IEEE 802.15.4-2006).
    static constexpr uint8_t kCsmaMaxBe = 5; // macMaxBE (IEEE 802.15.4-2006).

    bool mRunning;
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_POLL_ACCELERATOR_ENABLE

#endif // OT_CORE_MAC_POLL_ACCELERATOR_HPP_
