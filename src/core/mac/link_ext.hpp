/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for the Link Extension class.
 */

#ifndef OT_CORE_MAC_LINK_EXT_HPP_
#define OT_CORE_MAC_LINK_EXT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE

#include <openthread/link_ext.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "mac/mac_frame.hpp"
#include "mac/sub_mac.hpp"
#include "radio/radio.hpp"

#if !OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#error "OPENTHREAD_CONFIG_LINK_EXT_ENABLE requires OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE"
#endif

#if !OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
#error "OPENTHREAD_CONFIG_LINK_EXT_ENABLE requires target time TX support (e.g. CSL transmitter or software TX timing)"
#endif

namespace ot {
namespace Mac {

class LinkRaw;

/**
 * Implements link-layer extensions built on top of the raw link-layer (`LinkRaw`), such as burst transmission.
 */
class LinkExt : public InstanceLocator, private NonCopyable
{
    friend class LinkRaw;

public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit LinkExt(Instance &aInstance);

    /**
     * Starts a burst transmission of the transmit frame of the raw link-layer.
     *
     * See `otLinkExtTransmitBurst()` for details.
     *
     * @param[in]  aConfig    The burst configuration.
     * @param[in]  aCallback  A pointer to a function called when the burst ends.
     *
     * @retval kErrorNone          Successfully started the burst.
     * @retval kErrorInvalidArgs   The configuration or the transmit frame is invalid for a burst.
     * @retval kErrorInvalidState  The raw link-layer isn't enabled.
     * @retval kErrorBusy          A burst transmission is ongoing.
     */
    Error TransmitBurst(const otLinkExtBurstConfig &aConfig, otLinkExtBurstDone aCallback);

    /**
     * Stops the ongoing burst transmission.
     *
     * See `otLinkExtStopBurst()` for details.
     *
     * @retval kErrorNone          The burst is stopped or will stop when the frame being transmitted is done, or no
     *                             burst is ongoing.
     * @retval kErrorInvalidState  The raw link-layer isn't enabled.
     */
    Error StopBurst(void);

    /**
     * Gets the configuration of the ongoing burst transmission.
     *
     * See `otLinkExtGetBurstConfig()` for details.
     *
     * @param[out] aConfig  A reference to return the configuration.
     */
    void GetBurstConfig(otLinkExtBurstConfig &aConfig) const;

    /**
     * Indicates whether a burst transmission is ongoing.
     *
     * @retval TRUE   A burst transmission is ongoing.
     * @retval FALSE  No burst transmission is ongoing.
     */
    bool IsBurstActive(void) const { return mBurstCount > 0; }

    /**
     * Records the radio state requested by the raw link-layer for the time between burst ticks.
     *
     * Called on a raw link-layer receive or sleep request.
     *
     * @param[in]  aIsReceiving  TRUE for a receive request, FALSE for a sleep request.
     *
     * @retval TRUE   A burst frame is with `SubMac`, which would drop it, so the request is applied once the tick is
     *                done instead.
     * @retval FALSE  The request can be applied right away.
     */
    bool ShouldDeferIdleState(bool aIsReceiving);

private:
    static constexpr uint32_t kBurstTxAheadUs = OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US;

    // Called by `LinkRaw`.
    void Reset(void);
    void HandleLinkRawEnableChange(void);
    void HandleTransmitDone(TxFrame &aFrame);

    void  CancelBurst(void);
    void  FinishBurst(void);
    void  ApplyIdleState(void);
    Error StartBurst(const otLinkExtBurstConfig &aConfig);
    void  SkipMissedBurstSlots(Radio::Time32 aNow);
    void  StartBurstTimer(Radio::Time32 aNow);
    void  ScheduleNextBurstTick(void);
    void  HandleBurstTimer(void);
    void  SendBurstFrame(void);

    using BurstTimer = TimerMicroIn<LinkExt, &LinkExt::HandleBurstTimer>;

    otLinkExtBurstDone mBurstDoneCallback;
    BurstTimer         mBurstTimer;
    ChannelMask        mBurstChannelMask;
    Radio::Time32      mBurstTxTime; // Slot start time (radio clock) of the current tick.
    uint32_t           mBurstPeriodUs;
    uint16_t           mBurstCount; // Remaining ticks, including the current one.
    bool               mIsTimedBurst;
    bool               mIsBurstTxPending;     // A burst frame is with `SubMac`.
    bool               mIsBurstStopRequested; // Stop is requested while a burst frame is with the radio.
    bool               mIsReceiving;          // Idle state between burst ticks: receive (true) or sleep (false).
};

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_RADIO && OPENTHREAD_CONFIG_LINK_EXT_ENABLE

#endif // OT_CORE_MAC_LINK_EXT_HPP_
