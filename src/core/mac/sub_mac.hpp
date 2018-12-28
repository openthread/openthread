/*
 *  Copyright (c) 2016-2018, The OpenThread Authors.
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
 *   This file includes definitions for the IEEE 802.15.4 MAC layer (sub-MAC).
 */

#ifndef SUB_MAC_HPP_
#define SUB_MAC_HPP_

#include "openthread-core-config.h"

#include <openthread/link.h>
#include <openthread/platform/radio.h>

#include "common/locator.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"

namespace ot {

/**
 * @addtogroup core-mac
 *
 * @brief
 *   This module includes definitions for the IEEE 802.15.4 MAC (sub-MAC).
 *
 * @{
 *
 */

namespace Mac {

/**
 * This class implements the IEEE 802.15.4 MAC (sub-MAC).
 *
 * Sub-MAC layer implements a subset of IEEE802.15.4 MAC primitives which are shared by both MAC layer (in FTD/MTD
 * modes) and Raw Link (Radio only mode).

 * The sub-MAC layer handles the following (if not provided by radio platform):
 *
 *    - Ack timeout for frame transmission,
 *    - CSMA backoff logic,
 *    - Frame re-transmissions,
 *    - Energy scan on a single channel and RSSI sampling.
 *
 * It also act as the interface (to radio platform) for setting/getting radio configurations such as short or extended
 * addresses and PAN Id.
 *
 */
class SubMac : public InstanceLocator
{
public:
    enum
    {
        kInvalidRssiValue = 127, ///< Invalid Received Signal Strength Indicator (RSSI) value.
    };

    /**
     * This class defines the callbacks notifying `SubMac` user of changes and events.
     *
     */
    class Callbacks
    {
        friend class SubMac;

    protected:
        /**
         * This method notifies user of `SubMac` of a received frame.
         *
         * @param[in]  aFrame    A pointer to the received frame or NULL if the receive operation failed.
         * @param[in]  aError    OT_ERROR_NONE when successfully received a frame,
         *                       OT_ERROR_ABORT when reception was aborted and a frame was not received,
         *                       OT_ERROR_NO_BUFS when a frame could not be received due to lack of rx buffer space.
         *
         */
        void ReceiveDone(Frame *aFrame, otError aError);

        /**
         * This method notifies user of `SubMac` of CCA status (success/failure) for a frame transmission attempt.
         *
         * This is intended for updating counters, logging, and/or tracking CCA failure rate statistics.
         *
         * @param[in] aCcaSuccess   TRUE if the CCA succeeded, FALSE otherwise.
         * @param[in] aChannel      The channel on which CCA was performed.
         *
         */
        void RecordCcaStatus(bool aCcaSuccess, uint8_t aChannel);

        /**
         * This method notifies user of `SubMac` of the status of a frame transmission attempt.
         *
         * This is intended for updating counters, logging, and/or collecting statistics.
         *
         * @note Unlike `TransmitDone` which is invoked after all re-transmission attempts to indicate the final status
         * of a frame transmission, this method is invoked on all frame transmission attempts.
         *
         * @param[in] aFrame      The transmitted frame.
         * @param[in] aAckFrame   A pointer to the ACK frame, or NULL if no ACK was received.
         * @param[in] aError      OT_ERROR_NONE when the frame was transmitted successfully,
         *                        OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
         *                        OT_ERROR_CHANNEL_ACCESS_FAILURE tx failed due to activity on the channel,
         *                        OT_ERROR_ABORT when transmission was aborted for other reasons.
         * @param[in] aRetryCount Number of transmission retries for this frame.
         * @param[in] aWillRetx   Indicates whether frame will be retransmitted or not. This is applicable only
         *                        when there was an error in current transmission attempt.
         *
         */
        void RecordFrameTransmitStatus(const Frame &aFrame,
                                       const Frame *aAckFrame,
                                       otError      aError,
                                       uint8_t      aRetryCount,
                                       bool         aWillRetx);

        /**
         * The method notifies user of `SubMac` that the transmit operation has completed, providing, if applicable,
         * the received ACK frame.
         *
         * @param[in]  aFrame     The transmitted frame.
         * @param[in]  aAckFrame  A pointer to the ACK frame, NULL if no ACK was received.
         * @param[in]  aError     OT_ERROR_NONE when the frame was transmitted,
         *                        OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
         *                        OT_ERROR_CHANNEL_ACCESS_FAILURE tx failed due to activity on the channel,
         *                        OT_ERROR_ABORT when transmission was aborted for other reasons.
         *
         */
        void TransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError);

        /**
         * This method notifies user of `SubMac` that energy scan is complete.
         *
         * @param[in]  aMaxRssi  Maximum RSSI seen on the channel, or `SubMac::kInvalidRssiValue` if failed.
         *
         */
        void EnergyScanDone(int8_t aMaxRssi);

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
        /**
         * The method notifies user of `SubMac` to process transmit security for the frame, which  happens when the
         * frame includes Header IE(s) that were updated before transmission.
         *
         * @note This function can be called from interrupt context and it would only read/write data passed in
         *       via @p aFrame, but would not read/write any state within OpenThread.
         *
         * @param[in]  aFrame      The frame which needs to process transmit security.
         *
         */
        void FrameUpdated(Frame &aFrame);
#endif

        /**
         * This constructor initializes the `Callbacks` object.
         *
         */
        Callbacks(void) {}
    };

    /**
     * This constructor initializes the `SubMac` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     * @param[in]  aCallbacks A reference to the `Callbacks` object.
     *
     */
    SubMac(Instance &aInstance, Callbacks &aCallbacks);

    /**
     * This method gets the capabilities provided by platform radio.
     *
     * @returns The capability bit vector (see `OT_RADIO_CAP_*` definitions).
     *
     */
    otRadioCaps GetRadioCaps(void) const { return mRadioCaps; }

    /**
     * This method gets the capabilities provided by `SubMac` layer.
     *
     * @returns The capability bit vector (see `OT_RADIO_CAP_*` definitions).
     *
     */
    otRadioCaps GetCaps(void) const;

    /**
     * This method sets the PAN ID.
     *
     * @param[in] aPanId  The PAN ID.
     *
     */
    void SetPanId(PanId aPanId);

    /**
     * This method gets the short address.
     *
     * @returns The short address.
     *
     */
    ShortAddress GetShortAddress(void) const { return mShortAddress; }

    /**
     * This method sets the short address.
     *
     * @param[in] aShortAddress   The short address.
     *
     */
    void SetShortAddress(ShortAddress aShortAddress);

    /**
     * This function gets the extended address.
     *
     * @returns A reference to the extended address.
     *
     */
    const ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * This method sets extended address.
     *
     * @param[in] aExtAddress  The extended address.
     *
     */
    void SetExtAddress(const ExtAddress &aExtAddress);

    /**
     * This method registers a callback to provide received packet capture for IEEE 802.15.4 frames.
     *
     * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame
     *                                or NULL to disable the callback.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext);

    /**
     * This method indicates whether radio should stay in Receive or Sleep during CSMA backoff.
     *
     * @param[in]  aRxOnWhenBackoff  TRUE to keep radio in Receive, FALSE to put to Sleep during CSMA backoff.
     *
     */
    void SetRxOnWhenBackoff(bool aRxOnWhenBackoff) { mRxOnWhenBackoff = aRxOnWhenBackoff; }

    /**
     * This method enables the radio.
     *
     * @retval OT_ERROR_NONE     Successfully enabled.
     * @retval OT_ERROR_FAILED   The radio could not be enabled.
     *
     */
    otError Enable(void);

    /**
     * This method disables the radio.
     *
     * @retval OT_ERROR_NONE     Successfully disabled the radio.
     *
     */
    otError Disable(void);

    /**
     * This method transitions the radio to Sleep.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Sleep.
     * @retval OT_ERROR_BUSY          The radio was transmitting.
     * @retval OT_ERROR_INVALID_STATE The radio was disabled.
     *
     */
    otError Sleep(void);

    /**
     * This method transitions the radio to Receive.
     *
     * @param[in]  aChannel   The channel to use for receiving.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Receive.
     * @retval OT_ERROR_INVALID_STATE The radio was disabled or transmitting.
     *
     */
    otError Receive(uint8_t aChannel);

    /**
     * This method gets the radio transmit frame.
     *
     * @returns The transmit frame.
     *
     */
    Frame &GetTransmitFrame(void) { return mTransmitFrame; }

    /**
     * This method sends a prepared frame.
     *
     * The frame should be placed in `GetTransmitFrame()` frame.
     *
     * The `SubMac` layer handles Ack timeout, CSMA backoff, and frame retransmission.
     *
     * @retval OT_ERROR_NONE          Successfully started the frame transmission
     * @retval OT_ERROR_INVALID_STATE The radio was disabled or transmitting.
     *
     */
    otError Send(void);

    /**
     * This method gets the most recent RSSI measurement.
     *
     * @returns The RSSI in dBm when it is valid. `kInvalidRssiValue` when RSSI is invalid.
     *
     */
    int8_t GetRssi(void) const;

    /**
     * This method begins energy scan.
     *
     * @param[in] aScanChannel   The channel to perform the energy scan on.
     * @param[in] aScanDuration  The duration, in milliseconds, for the channel to be scanned.
     *
     * @retval OT_ERROR_NONE             Successfully started scanning the channel.
     * @retval OT_ERROR_INVALID_STATE    The radio was disabled or transmitting.
     * @retval OT_ERROR_NOT_IMPLEMENTED  Energy scan is not supported (applicable in link-raw/radio mode only).
     *
     */
    otError EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration);

    /**
     * This method handles a "Receive Done" event from radio platform.
     *
     *
     * @param[in]  aFrame    A pointer to the received frame or NULL if the receive operation failed.
     * @param[in]  aError    OT_ERROR_NONE when successfully received a frame,
     *                       OT_ERROR_ABORT when reception was aborted and a frame was not received,
     *                       OT_ERROR_NO_BUFS when a frame could not be received due to lack of rx buffer space.
     *
     */
    void HandleReceiveDone(Frame *aFrame, otError aError);

    /**
     * This method handles a Transmit Started event from radio platform.
     *
     * @param[in]  aFrame     The frame that is being transmitted.
     *
     */
    void HandleTransmitStarted(Frame &aFrame);

    /**
     * This method handles a "Transmit Done" event from radio platform.
     *
     * @param[in]  aFrame     The frame that was transmitted.
     * @param[in]  aAckFrame  A pointer to the ACK frame, NULL if no ACK was received.
     * @param[in]  aError     OT_ERROR_NONE when the frame was transmitted,
     *                        OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
     *                        OT_ERROR_CHANNEL_ACCESS_FAILURE tx could not take place due to activity on the channel,
     *                        OT_ERROR_ABORT when transmission was aborted for other reasons.
     *
     */
    void HandleTransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError);

    /**
     * This method handles "Energy Scan Done" event from radio platform.
     *
     * This method is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability. It is called from
     * `otPlatRadioEnergyScanDone()`.
     *
     * @param[in]  aInstance           The OpenThread instance structure.
     * @param[in]  aEnergyScanMaxRssi  The maximum RSSI encountered on the scanned channel.
     *
     */
    void HandleEnergyScanDone(int8_t aMaxRssi);

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
    /**
     * This method handles a "Frame Updated" event from radio platform.
     *
     * This is called to notify OpenThread to process transmit security for the frame, this happens when the frame
     * includes Header IE(s) that were updated before transmission. It is called from `otPlatRadioFrameUpdated()`.
     *
     * @note This method can be called from interrupt context and it would only read/write data passed in
     *       via @p aFrame, but would not read/write any state within OpenThread.
     *
     * @param[in]  aFrame      The frame which needs to process transmit security.
     *
     */
    void HandleFrameUpdated(Frame &aFrame);
#endif

private:
    enum
    {
        kMinBE             = 3,  ///< macMinBE (IEEE 802.15.4-2006).
        kMaxBE             = 5,  ///< macMaxBE (IEEE 802.15.4-2006).
        kUnitBackoffPeriod = 20, ///< Number of symbols (IEEE 802.15.4-2006).
        kMinBackoff        = 1,  ///< Minimum backoff (milliseconds).
        kAckTimeout        = 16, ///< Timeout for waiting on an ACK (milliseconds).

#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
        kEnergyScanRssiSampleInterval = 128, ///< RSSI sample interval during energy scan, 128 usec
#else
        kEnergyScanRssiSampleInterval = 1, ///< RSSI sample interval during energy scan, 1 ms
#endif
    };

    enum State
    {
        kStateDisabled,    ///< Radio is disabled.
        kStateSleep,       ///< Radio is in sleep.
        kStateReceive,     ///< Radio in in receive.
        kStateCsmaBackoff, ///< CSMA backoff before transmission.
        kStateTransmit,    ///< Radio is transmitting.
        kStateEnergyScan,  ///< Energy scan.
    };

    bool RadioSupportsCsmaBackoff(void) const
    {
        return ((mRadioCaps & (OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_TRANSMIT_RETRIES)) != 0);
    }

    bool RadioSupportsRetries(void) const { return ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) != 0); }
    bool RadioSupportsAckTimeout(void) const { return ((mRadioCaps & OT_RADIO_CAPS_ACK_TIMEOUT) != 0); }
    bool RadioSupportsEnergyScan(void) const { return ((mRadioCaps & OT_RADIO_CAPS_ENERGY_SCAN) != 0); }

    bool ShouldHandleCsmaBackOff(void) const;
    bool ShouldHandleAckTimeout(void) const;
    bool ShouldHandleRetries(void) const;
    bool ShouldHandleEnergyScan(void) const;

    void StartCsmaBackoff(void);
    void BeginTransmit(void);
    void SampleRssi(void);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    void               SetState(State aState);
    static const char *StateToString(State aState);

    otRadioCaps        mRadioCaps;
    State              mState;
    uint8_t            mCsmaBackoffs;
    uint8_t            mTransmitRetries;
    ShortAddress       mShortAddress;
    ExtAddress         mExtAddress;
    bool               mRxOnWhenBackoff;
    int8_t             mEnergyScanMaxRssi;
    uint32_t           mEnergyScanEndTime;
    Frame &            mTransmitFrame;
    Callbacks &        mCallbacks;
    otLinkPcapCallback mPcapCallback;
    void *             mPcapCallbackContext;
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    TimerMicro mTimer;
#else
    TimerMilli mTimer;
#endif
};

/**
 * @}
 *
 */

} // namespace Mac
} // namespace ot

#endif // SUB_MAC_HPP_
