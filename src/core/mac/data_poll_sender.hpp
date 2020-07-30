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
 *   This file includes definitions for data poll (mac data request command) sender.
 */

#ifndef DATA_POLL_MANAGER_HPP_
#define DATA_POLL_MANAGER_HPP_

#include "openthread-core-config.h"

#include "common/code_utils.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"
#include "thread/topology.hpp"

namespace ot {

/**
 * @addtogroup core-data-poll-sender
 *
 * @brief
 *   This module includes definitions for data poll sender.
 *
 * @{
 */

/**
 * This class implements the data poll (mac data request command) sender.
 *
 */

class DataPollSender : public InstanceLocator
{
public:
    enum
    {
        kDefaultFastPolls  = 8,  ///< Default number of fast poll transmissions (@sa StartFastPolls).
        kMaxFastPolls      = 15, ///< Maximum number of fast poll transmissions allowed.
        kMaxFastPollsUsers = 63, ///< Maximum number of the users of fast poll transmissions allowed.
    };

    /**
     * This constructor initializes the data poll sender object.
     *
     * @param[in]  aInstance   A reference to the OpenThread instance.
     *
     */
    explicit DataPollSender(Instance &aInstance);

    /**
     * This method instructs the data poll sender to start sending periodic data polls.
     *
     */
    void StartPolling(void);

    /**
     * This method instructs the data poll sender to stop sending periodic data polls.
     *
     */
    void StopPolling(void);

    /**
     * This method enqueues a data poll (an IEEE 802.15.4 Data Request) message.
     *
     * @retval OT_ERROR_NONE           Successfully enqueued a data poll message
     * @retval OT_ERROR_ALREADY        A data poll message is already enqueued.
     * @retval OT_ERROR_INVALID_STATE  Device is not in rx-off-when-idle mode.
     * @retval OT_ERROR_NO_BUFS        Insufficient message buffers available.
     *
     */
    otError SendDataPoll(void);

    /**
     * This method sets/clears a user-specified/external data poll period.
     *
     * Value of zero for `aPeriod` clears the user-specified poll period.
     *
     * If the user provides a non-zero poll period, the user value specifies the maximum period between data
     * request transmissions. Note that OpenThread may send data request transmissions more frequently when expecting
     * a control-message from a parent or in case of data poll transmission failures or timeouts, or when the specified
     * value is larger than the child timeout.
     *
     * A non-zero `aPeriod` should be larger than or equal to `OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD` (10ms) or
     * this method returns `OT_ERROR_INVALID_ARGS`. If a non-zero `aPeriod` is larger than maximum value of
     * `0x3FFFFFF ((1 << 26) - 1)`, it would be clipped to this value.
     *
     * @param[in]  aPeriod  The data poll period in milliseconds.
     *
     * @retval OT_ERROR_NONE           Successfully set/cleared user-specified poll period.
     * @retval OT_ERROR_INVALID_ARGS   If aPeriod is invalid.
     *
     */
    otError SetExternalPollPeriod(uint32_t aPeriod);

    /**
     * This method gets the current user-specified/external data poll period.
     *
     * @returns  The data poll period in milliseconds.
     *
     */
    uint32_t GetExternalPollPeriod(void) const { return mExternalPollPeriod; }

    /**
     * This method gets the destination MAC address for a data poll frame.
     *
     * @param[out] aDest       Reference to a `MAC::Address` to output the poll destination address (on success).
     *
     * @retval OT_ERROR_NONE   @p aDest was updated successfully.
     * @retval OT_ERROR_ABORT  Abort the data poll transmission (not currently attached to any parent).
     *
     */
    otError GetPollDestinationAddress(Mac::Address &aDest) const;

    /**
     * This method informs the data poll sender of success/error status of a previously requested poll frame
     * transmission.
     *
     * In case of transmit failure, the data poll sender may choose to send the next data poll more quickly (up to
     * some fixed number of attempts).
     *
     * @param[in] aFrame     The data poll frame.
     * @param[in] aError     Error status of a data poll message transmission.
     *
     */
    void HandlePollSent(Mac::TxFrame &aFrame, otError aError);

    /**
     * This method informs the data poll sender that a data poll timeout happened, i.e., when the ack in response to
     * a data request command indicated that a frame was pending, but no frame was received after timeout interval.
     *
     * Data poll sender may choose to transmit another data poll immediately (up to some fixed number of attempts).
     *
     */
    void HandlePollTimeout(void);

    /**
     * This method informs the data poll sender to process a MAC frame.
     *
     *   1. Data Frame: send an immediate data poll if "frame pending" is set.
     *   2. Ack Frame for a secured data frame in version 1.2 or newer: send an immediate data poll if
     *      "frame pending" is set, otherwise reset the keep-alive timer for sending next poll.
     *
     * @param[in] aFrame     The frame to process.
     *
     */
    void ProcessFrame(const Mac::RxFrame &aFrame);

    /**
     * This method asks the data poll sender to recalculate the poll period.
     *
     * This is mainly used to inform the poll sender that a parameter impacting the poll period (e.g., the child's
     * timeout value which is used to determine the default data poll period) is modified.
     *
     */
    void RecalculatePollPeriod(void);

    /**
     * This method sets/clears the attach mode on data poll sender.
     *
     * When attach mode is enabled, the data poll sender will send data polls at a faster rate determined by
     * poll period configuration option `OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD`.
     *
     * @param[in]  aMode  The mode value.
     *
     */
    void SetAttachMode(bool aMode);

    /**
     * This method asks data poll sender to send the next given number of polls at a faster rate (poll period defined
     * by `kFastPollPeriod`). This is used by OpenThread stack when it expects a response from the parent/sender.
     *
     * If @p aNumFastPolls is zero the default value specified by `kDefaultFastPolls` is used instead. The number of
     * fast polls is clipped by maximum value specified by `kMaxFastPolls`.
     *
     * Note that per `SendFastPolls()` would increase the internal reference count until up to the allowed maximum
     * value. If there are retransmission mechanism in the caller component, it should be responsible to call
     * `StopFastPolls()` the same times as `SendFastPolls()` it triggered to decrease the reference count properly,
     * guaranteeing to exit fast poll mode gracefully. Otherwise, fast poll would continue until all data polls are sent
     * out.
     *
     * @param[in] aNumFastPolls  If non-zero, number of fast polls to send, if zero, default value is used instead.
     *
     */
    void SendFastPolls(uint8_t aNumFastPolls = 0);

    /**
     * This method asks data poll sender to stop fast polls when the expecting response is received.
     *
     */
    void StopFastPolls(void);

    /**
     * This method gets the maximum data polling period in use.
     *
     * The maximum data poll period is determined based as the minimum of the user-specified poll interval and the
     * default poll interval.
     *
     * @returns The maximum data polling period in use.
     *
     */
    uint32_t GetKeepAlivePollPeriod(void) const;

    /**
     * This method resets the timer for sending keep-alive messages.
     *
     */
    void ResetKeepAliveTimer(void);

    /**
     * This method returns the default maximum poll period.
     *
     * The default poll period is determined based on the child timeout interval, ensuing the child would send data poll
     * within the child's timeout.
     *
     * @returns The maximum default data polling interval (in msec).
     *
     */
    uint32_t GetDefaultPollPeriod(void) const;

private:
    enum // Poll period under different conditions (in milliseconds).
    {
        kAttachDataPollPeriod = OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD, ///< Poll period during attach.
        kRetxPollPeriod       = OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD,        ///< Poll retx period due to tx failure.
        kFastPollPeriod       = 188,                                           ///< Period used for fast polls.
        kMinPollPeriod        = OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD,     ///< Minimum allowed poll period.
        kMaxExternalPeriod    = ((1 << 26) - 1), ///< Maximum allowed user-specified period.
                                                 ///< i.e. (0x3FFFFF)ms, about 18.64 hours.
    };

    enum
    {
        kQuickPollsAfterTimeout = 5, ///< Maximum number of quick data poll tx in case of back-to-back poll timeouts.
        kMaxPollRetxAttempts = OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS, ///< Maximum number of retransmit attempts
                                                                             ///< of data poll (mac data request).
    };

    enum PollPeriodSelector
    {
        kUsePreviousPollPeriod,
        kRecalculatePollPeriod,
    };

    void            ScheduleNextPoll(PollPeriodSelector aPollPeriodSelector);
    uint32_t        CalculatePollPeriod(void) const;
    const Neighbor &GetParent(void) const;
    static void     HandlePollTimer(Timer &aTimer);
    static void     UpdateIfLarger(uint32_t &aPeriod, uint32_t aNewPeriod);

    TimeMilli mTimerStartTime;
    uint32_t  mPollPeriod;
    uint32_t  mExternalPollPeriod : 26; //< In milliseconds.
    uint8_t   mFastPollsUsers : 6;      //< Number of callers which request fast polls.

    TimerMilli mTimer;

    bool    mEnabled : 1;              //< Indicates whether data polling is enabled/started.
    bool    mAttachMode : 1;           //< Indicates whether in attach mode (to use attach poll period).
    bool    mRetxMode : 1;             //< Indicates whether last poll tx failed at mac/radio layer (poll retx mode).
    uint8_t mPollTimeoutCounter : 4;   //< Poll timeouts counter (0 to `kQuickPollsAfterTimout`).
    uint8_t mPollTxFailureCounter : 4; //< Poll tx failure counter (0 to `kMaxPollRetxAttempts`).
    uint8_t mRemainingFastPolls : 4;   //< Number of remaining fast polls when in transient fast polling mode.
};

/**
 * @}
 *
 */

} // namespace ot

#endif // DATA_POLL_MANAGER_HPP_
