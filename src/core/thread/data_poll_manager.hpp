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
 *   This file includes definitions for data poll (mac data request command) manager.
 */

#ifndef DATA_POLL_MANAGER_HPP_
#define DATA_POLL_MANAGER_HPP_

#include <openthread-core-config.h>

#include "openthread/types.h"
#include <common/code_utils.hpp>

namespace Thread {

class MeshForwarder;

/**
 * @addtogroup core-data-poll-manager
 *
 * @brief
 *   This module includes definitions for data poll manager.
 *
 * @{
 */

/**
 * This class implements the data poll (mac data request command) manager.
 *
 */

class DataPollManager
{
public:
    enum
    {
        kDefaultFastPolls   = 8,  ///< Default number of fast poll transmissions (@sa StartFastPolls).
        kMaxFastPolls       = 15, ///< Maximum number of fast poll transmissions allowed.
    };

    /**
     * This constructor initializes the data poll manager object.
     *
     * @param[in]  aMeshForwarder  A reference to the Mesh Forwarder.
     *
     */
    explicit DataPollManager(MeshForwarder &aMeshForwarder);

    /**
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure.
     *
     */
    otInstance *GetInstance(void);

    /**
     * This method instructs the data poll manager to start sending periodic data polls.
     *
     * @retval kThreadError_None            Successfully started sending periodic data polls.
     * @retval kThreadError_Already         Periodic data poll transmission is already started/enabled.
     * @retval kThreadError_InvalidState    Device is not in rx-off-when-idle mode.
     *
     */
    ThreadError StartPolling(void);

    /**
     * This method instructs the data poll manager to stop sending periodic data polls.
     *
     */
    void StopPolling(void);

    /**
     * This method enqueues a data poll (an IEEE 802.15.4 Data Request) message.
     *
     * @retval kThreadError_None          Successfully enqueued a data poll message
     * @retval kThreadError_Already       A data poll message is already enqueued.
     * @retval kThreadError_InvalidState  Device is not in rx-off-when-idle mode.
     * @retval kThreadError_NoBufs        Insufficient message buffers available.
     *
     */
    ThreadError SendDataPoll(void);

    /**
     * This method sets a user-specified/external data poll period.
     *
     * If the user provides a non-zero poll period, the user value specifies the maximum period between data
     * request transmissions. Note that OpenThread may send data request transmissions more frequently when expecting
     * a control-message from a parent or in case of data poll transmission failures or timeouts.
     *
     * Default value for the external poll period is zero (i.e., no user-specified poll period).
     *
     * @param[in]  aPeriod  The data poll period in milliseconds, or zero to mean no user-specified poll period.
     *
     */
    void SetExternalPollPeriod(uint32_t aPeriod);

    /**
     * This method gets the current user-specified/external data poll period.
     *
     * @returns  The data poll period in milliseconds.
     *
     */
    uint32_t GetExternalPollPeriod(void) const { return mExternalPollPeriod; }

    /**
     * This method informs the data poll manager of success/error status of a previously requested poll message
     * transmission.
     *
     * In case of transmit failure, the data poll manager may choose to send the next data poll more quickly (up to
     * some fixed number of attempts).
     *
     * @param[in] aError   Error status of a data poll message transmission.
     *
     */
    void HandlePollSent(ThreadError aError);

    /**
     * This method informs the data poll manager that a data poll timeout happened, i.e., when the ack in response to
     * a data request command indicated that a frame was pending, but no frame was received after timeout interval.
     *
     * Data poll manager may choose to transmit another data poll immediately (up to some fixed number of attempts).
     *
     */
    void HandlePollTimeout(void);

    /**
     * This method informs the data poll manager that a mac frame has been received. It checks the "frame pending" in
     * the received frame header and if it is set, data poll manager will send an immediate data poll to retrieve the
     * pending frame.
     *
     */
    void CheckFramePending(Mac::Frame &aFrame);

    /**
     * This method asks the data poll manager to recalculate the poll period.
     *
     * This is mainly used to inform the poll manager that a parameter impacting the poll period (e.g., the child's
     * timeout value which is used to determine the default data poll period) is modified.
     *
     */
    void RecalculatePollPeriod(void);

    /**
     * This method sets/clears the attach mode on data poll manager.
     *
     * When attach mode is enabled, the data poll manager will send data polls at a faster rate determined by
     * poll period configuration option `OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD`.
     *
     * @param[in]  aMode  The mode value.
     *
     */
    void SetAttachMode(bool aMode);

    /**
     * This method asks data poll manager to send the next given number of polls at a faster rate (poll period defined
     * by `kFastPollPeriod`). This is used by OpenThread stack when it expects a response from the parent/sender.
     *
     * If @p aNumFastPolls is zero the default value specified by `kDefaultFastPolls` is used instead. The number of
     * fast polls is clipped by maximum value specified by `kMaxFastPolls`.
     *
     * @param[in] aNumFastPolls  If non-zero, number of fast polls to send, if zero, default value is used instead.
     *
     */
    void SendFastPolls(uint8_t aNumFastPolls);

private:
    enum  // Poll period under different conditions (in milliseconds).
    {
        kAttachDataPollPeriod   = OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD,  ///< Poll period during attach.
        kRetxPollPeriod         = 500,                                        ///< Poll retx period due to tx failure.
        kNoBufferRetxPollPeriod = 200,                                        ///< Poll retx due to no buffer space.
        kFastPollPeriod         = 188,                                        ///< Period used for fast polls.
        kMinPollPeriod          = 10,                                         ///< Minimum allowed poll period.
    };

    enum
    {
        kQuickPollsAfterTimeout = 5,  ///< Maximum number of quick data poll tx in case of back-to-back poll timeouts.
        kMaxPollRetxAttempts    = 5,  ///< Maximum number of retransmit attempts of data poll (mac data request).
    };

    enum PollPeriodSelector
    {
        kUsePreviousPollPeriod,
        kRecalculatePollPeriod,
    };

    void ScheduleNextPoll(PollPeriodSelector aPollPeriodSelector);
    uint32_t CalculatePollPeriod(void) const;
    static void HandlePollTimer(void *aContext);

    MeshForwarder &mMeshForwarder;
    Timer     mTimer;
    uint32_t  mExternalPollPeriod;
    uint32_t  mPollPeriod;

    bool      mEnabled: 1;               //< Indicates whether data polling is enabled/started.
    bool      mAttachMode: 1;            //< Indicates whether in attach mode (to use attach poll period).
    bool      mRetxMode: 1;              //< Indicates whether last poll tx failed at mac/radio layer (poll retx mode).
    bool      mNoBufferRetxMode: 1;      //< Indicates whether last poll tx failed due to insufficient buffer.
    uint8_t   mPollTimeoutCounter: 4;    //< Poll timeouts counter (0 to `kQuickPollsAfterTimout`).
    uint8_t   mPollTxFailureCounter: 4;  //< Poll tx failure counter (0 to `kMaxPollRetxAttempts`).
    uint8_t   mRemainingFastPolls: 4;    //< Number of remaining fast polls when in transient fast polling mode.
};

/**
 * @}
 *
 */

}  // namespace Thread

#endif  // DATA_POLL_MANAGER_HPP_
