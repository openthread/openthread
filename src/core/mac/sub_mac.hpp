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

#include <openthread/platform/crypto.h>

#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"
#include "radio/radio.hpp"

namespace ot {

/**
 * @addtogroup core-mac
 *
 * @brief
 *   This module includes definitions for the IEEE 802.15.4 MAC (sub-MAC).
 *
 * @{
 */

namespace Mac {

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE && (OPENTHREAD_CONFIG_THREAD_VERSION < OT_THREAD_VERSION_1_2)
#error "Thread 1.2 or higher version is required for OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE."
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if (OPENTHREAD_CONFIG_THREAD_VERSION < OT_THREAD_VERSION_1_2)
#error "Thread 1.2 or higher version is required for OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE."
#endif

#if !OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#error "Microsecond timer OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE is required for "\
    "OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE"
#endif

#endif

#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE && !OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#error "OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE is required for OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE."
#endif

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE
class LinkRaw;
#endif

/**
 * Implements the IEEE 802.15.4 MAC (sub-MAC).
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
 */
class SubMac : public InstanceLocator, private NonCopyable
{
    friend class Radio::Callbacks;
    friend class LinkRaw;

public:
    /**
     * Defines the callbacks notifying `SubMac` user of changes and events.
     */
    class Callbacks : public InstanceLocator
    {
    public:
        /**
         * Initializes the `Callbacks` object.
         *
         * @param[in]  aInstance  A reference to the OpenThread instance.
         */
        explicit Callbacks(Instance &aInstance);

        /**
         * Notifies user of `SubMac` of a received frame.
         *
         * @param[in]  aFrame    A pointer to the received frame or `nullptr` if the receive operation failed.
         * @param[in]  aError    kErrorNone when successfully received a frame,
         *                       kErrorAbort when reception was aborted and a frame was not received,
         *                       kErrorNoBufs when a frame could not be received due to lack of rx buffer space.
         */
        void ReceiveDone(RxFrame *aFrame, Error aError);

        /**
         * Notifies user of `SubMac` of CCA status (success/failure) for a frame transmission attempt.
         *
         * This is intended for updating counters, logging, and/or tracking CCA failure rate statistics.
         *
         * @param[in] aCcaSuccess   TRUE if the CCA succeeded, FALSE otherwise.
         * @param[in] aChannel      The channel on which CCA was performed.
         */
        void RecordCcaStatus(bool aCcaSuccess, uint8_t aChannel);

        /**
         * Notifies user of `SubMac` of the status of a frame transmission attempt.
         *
         * This is intended for updating counters, logging, and/or collecting statistics.
         *
         * @note Unlike `TransmitDone` which is invoked after all re-transmission attempts to indicate the final status
         * of a frame transmission, this method is invoked on all frame transmission attempts.
         *
         * @param[in] aFrame      The transmitted frame.
         * @param[in] aError      kErrorNone when the frame was transmitted successfully,
         *                        kErrorNoAck when the frame was transmitted but no ACK was received,
         *                        kErrorChannelAccessFailure tx failed due to activity on the channel,
         *                        kErrorAbort when transmission was aborted for other reasons.
         * @param[in] aRetryCount Current retry count. This is valid only when sub-mac handles frame re-transmissions.
         * @param[in] aWillRetx   Indicates whether frame will be retransmitted or not. This is applicable only
         *                        when there was an error in current transmission attempt.
         */
        void RecordFrameTransmitStatus(const TxFrame &aFrame, Error aError, uint8_t aRetryCount, bool aWillRetx);

        /**
         * The method notifies user of `SubMac` that the transmit operation has completed, providing, if applicable,
         * the received ACK frame.
         *
         * @param[in]  aFrame     The transmitted frame.
         * @param[in]  aAckFrame  A pointer to the ACK frame, `nullptr` if no ACK was received.
         * @param[in]  aError     kErrorNone when the frame was transmitted,
         *                        kErrorNoAck when the frame was transmitted but no ACK was received,
         *                        kErrorChannelAccessFailure tx failed due to activity on the channel,
         *                        kErrorAbort when transmission was aborted for other reasons.
         */
        void TransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError);

        /**
         * Notifies user of `SubMac` that energy scan is complete.
         *
         * @param[in]  aMaxRssi  Maximum RSSI seen on the channel, or `Radio::kInvalidRssi` if failed.
         */
        void EnergyScanDone(int8_t aMaxRssi);

        /**
         * Notifies user of `SubMac` that a specific MAC frame counter is used for transmission.
         *
         * It is possible that this callback is invoked out of order in terms of counter values (i.e., called for a
         * smaller counter value after a call for a larger counter value).
         *
         * @param[in]  aFrameCounter  The MAC frame counter value which was used.
         */
        void FrameCounterUsed(uint32_t aFrameCounter);
    };

    /**
     * Initializes the `SubMac` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit SubMac(Instance &aInstance);

    /**
     * Gets the capabilities provided by platform radio.
     *
     * @returns The capability bit vector (see `OT_RADIO_CAP_*` definitions).
     */
    otRadioCaps GetRadioCaps(void) const { return mRadioCaps; }

    /**
     * Gets the capabilities provided by `SubMac` layer.
     *
     * @returns The capability bit vector (see `OT_RADIO_CAP_*` definitions).
     */
    otRadioCaps GetCaps(void) const;

    /**
     * Sets the PAN ID.
     *
     * @param[in] aPanId  The PAN ID.
     */
    void SetPanId(PanId aPanId);

    /**
     * Gets the short address.
     *
     * @returns The short address.
     */
    ShortAddress GetShortAddress(void) const { return mShortAddress; }

    /**
     * Sets the short address.
     *
     * @param[in] aShortAddress   The short address.
     */
    void SetShortAddress(ShortAddress aShortAddress);

    /**
     * Gets the extended address.
     *
     * @returns A reference to the extended address.
     */
    const ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * Sets extended address.
     *
     * @param[in] aExtAddress  The extended address.
     */
    void SetExtAddress(const ExtAddress &aExtAddress);

    /**
     * Registers a callback to provide received packet capture for IEEE 802.15.4 frames.
     *
     * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame
     *                               or `nullptr` to disable the callback.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     */
    void SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext)
    {
        mPcapCallback.Set(aPcapCallback, aCallbackContext);
    }

    /**
     * Indicates whether radio should stay in Receive or Sleep during idle periods.
     *
     * @param[in]  aRxOnWhenIdle  TRUE to keep radio in Receive, FALSE to put to Sleep during idle periods.
     */
    void SetRxOnWhenIdle(bool aRxOnWhenIdle);

    /**
     * Enables the radio.
     *
     * @retval kErrorNone     Successfully enabled.
     * @retval kErrorFailed   The radio could not be enabled.
     */
    Error Enable(void);

    /**
     * Disables the radio.
     *
     * @retval kErrorNone     Successfully disabled the radio.
     */
    Error Disable(void);

    /**
     * Transitions the radio to Sleep.
     *
     * @retval kErrorNone          Successfully transitioned to Sleep.
     * @retval kErrorBusy          The radio was transmitting.
     * @retval kErrorInvalidState  The radio was disabled.
     */
    Error Sleep(void);

    /**
     * Indicates whether the sub-mac is busy transmitting or scanning.
     *
     * @retval TRUE if the sub-mac is busy transmitting or scanning.
     * @retval FALSE if the sub-mac is not busy transmitting or scanning.
     */
    bool IsTransmittingOrScanning(void) const { return (mState == kStateTransmit) || (mState == kStateEnergyScan); }

    /**
     * Transitions the radio to Receive.
     *
     * @param[in]  aChannel   The channel to use for receiving.
     *
     * @retval kErrorNone          Successfully transitioned to Receive.
     * @retval kErrorInvalidState  The radio was disabled or transmitting.
     */
    Error Receive(uint8_t aChannel);

    /**
     * Gets the radio transmit frame.
     *
     * @returns The transmit frame.
     */
    TxFrame &GetTransmitFrame(void) { return mTransmitFrame; }

    /**
     * Sends a prepared frame.
     *
     * The frame should be placed in `GetTransmitFrame()` frame.
     *
     * The `SubMac` layer handles Ack timeout, CSMA backoff, and frame retransmission.
     *
     * @retval kErrorNone          Successfully started the frame transmission
     * @retval kErrorInvalidState  The radio was disabled or transmitting.
     */
    Error Send(void);

    /**
     * Gets the number of transmit retries of last transmitted frame.
     *
     * @returns Number of transmit retries.
     */
    uint8_t GetTransmitRetries(void) const { return mTransmitRetries; }

    /**
     * Gets the most recent RSSI measurement.
     *
     * @returns The RSSI in dBm when it is valid. `Radio::kInvalidRssi` when RSSI is invalid.
     */
    int8_t GetRssi(void) const;

    /**
     * Begins energy scan.
     *
     * @param[in] aScanChannel   The channel to perform the energy scan on.
     * @param[in] aScanDuration  The duration, in milliseconds, for the channel to be scanned.
     *
     * @retval kErrorNone            Successfully started scanning the channel.
     * @retval kErrorBusy            The radio is performing energy scanning.
     * @retval kErrorInvalidState    The radio was disabled or transmitting.
     * @retval kErrorNotImplemented  Energy scan is not supported (applicable in link-raw/radio mode only).
     */
    Error EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration);

    /**
     * Returns the noise floor value (currently use the radio receive sensitivity value).
     *
     * @returns The noise floor value in dBm.
     */
    int8_t GetNoiseFloor(void) const;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Configures CSL parameters in 'SubMac'.
     *
     * @param[in]  aPeriod    The CSL period (in unit of 10 symbols).
     * @param[in]  aChannel   The CSL channel.
     * @param[in]  aShortAddr The short source address of CSL receiver's peer.
     * @param[in]  aExtAddr   The extended source address of CSL receiver's peer.
     *
     * @retval  TRUE if CSL Period or CSL Channel changed.
     * @retval  FALSE if CSL Period and CSL Channel did not change.
     */
    bool UpdateCsl(uint16_t aPeriod, uint8_t aChannel, otShortAddress aShortAddr, const otExtAddress *aExtAddr);

    /**
     * Lets `SubMac` start CSL sample mode given a configured non-zero CSL period.
     *
     * `SubMac` would switch the radio state between `Receive` and `Sleep` according the CSL timer.
     */
    void CslSample(void);

    /**
     * Returns parent CSL accuracy (clock accuracy and uncertainty).
     *
     * @returns The parent CSL accuracy.
     */
    const CslAccuracy &GetCslParentAccuracy(void) const { return mCslParentAccuracy; }

    /**
     * Sets parent CSL accuracy.
     *
     * @param[in] aCslAccuracy  The parent CSL accuracy.
     */
    void SetCslParentAccuracy(const CslAccuracy &aCslAccuracy) { mCslParentAccuracy = aCslAccuracy; }

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

    /**
     * Sets MAC keys and key index.
     *
     * @param[in] aKeyIdMode  MAC key ID mode.
     * @param[in] aKeyId      The key ID.
     * @param[in] aPrevKey    The previous MAC key.
     * @param[in] aCurrKey    The current MAC key.
     * @param[in] aNextKey    The next MAC key.
     */
    void SetMacKey(uint8_t            aKeyIdMode,
                   uint8_t            aKeyId,
                   const KeyMaterial &aPrevKey,
                   const KeyMaterial &aCurrKey,
                   const KeyMaterial &aNextKey);

    /**
     * Returns a reference to the current MAC key.
     *
     * @returns A reference to the current MAC key.
     */
    const KeyMaterial &GetCurrentMacKey(void) const { return mCurrKey; }

    /**
     * Returns a reference to the previous MAC key.
     *
     * @returns A reference to the previous MAC key.
     */
    const KeyMaterial &GetPreviousMacKey(void) const { return mPrevKey; }

    /**
     * Returns a reference to the next MAC key.
     *
     * @returns A reference to the next MAC key.
     */
    const KeyMaterial &GetNextMacKey(void) const { return mNextKey; }

    /**
     * Clears the stored MAC keys.
     */
    void ClearMacKeys(void)
    {
        mPrevKey.Clear();
        mCurrKey.Clear();
        mNextKey.Clear();
    }

    /**
     * Returns the current MAC frame counter value.
     *
     * @returns The current MAC frame counter value.
     */
    uint32_t GetFrameCounter(void) const { return mFrameCounter; }

    /**
     * Sets the current MAC Frame Counter value.
     *
     * @param[in] aFrameCounter  The MAC Frame Counter value.
     * @param[in] aSetIfLarger   If `true`, set only if the new value @p aFrameCounter is larger than the current value.
     *                           If `false`, set the new value independent of the current value.
     */
    void SetFrameCounter(uint32_t aFrameCounter, bool aSetIfLarger);

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    /**
     * Enables/disables the radio filter.
     *
     * When radio filter is enabled, radio is put to sleep instead of receive (to ensure device does not receive any
     * frame and/or potentially send ack). Also the frame transmission requests return immediately without sending the
     * frame over the air (return "no ack" error if ack is requested, otherwise return success).
     *
     * @param[in] aFilterEnabled    TRUE to enable radio filter, FALSE to disable.
     */
    void SetRadioFilterEnabled(bool aFilterEnabled) { mRadioFilterEnabled = aFilterEnabled; }

    /**
     * Indicates whether the radio filter is enabled or not.
     *
     * @retval TRUE   If the radio filter is enabled.
     * @retval FALSE  If the radio filter is disabled.
     */
    bool IsRadioFilterEnabled(void) const { return mRadioFilterEnabled; }
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    /**
     * Configures wake-up listening parameters in all radios.
     *
     * @param[in]  aEnable    Whether to enable or disable wake-up listening.
     * @param[in]  aInterval  The wake-up listen interval in microseconds.
     * @param[in]  aDuration  The wake-up listen duration in microseconds.
     * @param[in]  aChannel   The wake-up channel.
     */
    void UpdateWakeupListening(bool aEnable, uint32_t aInterval, uint32_t aDuration, uint8_t aChannel);
#endif

private:
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    void        CslInit(void);
    void        UpdateCslLastSyncTimestamp(TxFrame &aFrame, RxFrame *aAckFrame);
    void        UpdateCslLastSyncTimestamp(RxFrame *aFrame, Error aError);
    static void HandleCslTimer(Timer &aTimer);
    void        HandleCslTimer(void);
    void        GetCslWindowEdges(uint32_t &aAhead, uint32_t &aAfter);
    uint32_t    GetLocalTime(void);
#if OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
    void LogReceived(RxFrame *aFrame);
#endif
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    void        WedInit(void);
    static void HandleWedTimer(Timer &aTimer);
    void        HandleWedTimer(void);
#endif

    static constexpr uint8_t  kCsmaMinBe         = 3;                  // macMinBE (IEEE 802.15.4-2006).
    static constexpr uint8_t  kCsmaMaxBe         = 5;                  // macMaxBE (IEEE 802.15.4-2006).
    static constexpr uint32_t kUnitBackoffPeriod = 20;                 // Number of symbols (IEEE 802.15.4-2006).
    static constexpr uint32_t kAckTimeout = 16 * Time::kOneMsecInUsec; // Timeout for waiting on an ACK (in usec).
    static constexpr uint32_t kCcaSampleInterval = 128;                // CCA sample interval, 128 usec.

#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    static constexpr uint8_t kRetxDelayMinBackoffExponent = OPENTHREAD_CONFIG_MAC_RETX_DELAY_MIN_BACKOFF_EXPONENT;
    static constexpr uint8_t kRetxDelayMaxBackoffExponent = OPENTHREAD_CONFIG_MAC_RETX_DELAY_MAX_BACKOFF_EXPONENT;
#endif

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    static constexpr uint32_t kEnergyScanRssiSampleInterval = 128; // RSSI sample interval for energy scan, in usec
#else
    static constexpr uint32_t kEnergyScanRssiSampleInterval = 1000; // RSSI sample interval for energy scan, in usec
#endif

    enum State : uint8_t
    {
        kStateDisabled,    // Radio is disabled.
        kStateSleep,       // Radio is in sleep.
        kStateReceive,     // Radio in in receive.
        kStateCsmaBackoff, // CSMA backoff before transmission.
        kStateTransmit,    // Radio is transmitting.
        kStateEnergyScan,  // Energy scan.
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
        kStateDelayBeforeRetx, // Delay before retx
#endif
#if !OPENTHREAD_MTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        kStateCslTransmit, // CSL transmission.
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        kStateCslSample, // CSL receive.
#endif
    };

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    // Radio on times needed before and after MHR time for proper frame detection
    static constexpr uint32_t kMinReceiveOnAhead = OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD;
    static constexpr uint32_t kMinReceiveOnAfter = OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER;

    // CSL/wake-up listening receivers would wake up `kCslReceiveTimeAhead` earlier
    // than expected sample window. The value is in usec.
    static constexpr uint32_t kCslReceiveTimeAhead = OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD;
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    // Margin to be applied after the end of a wake-up listen duration to schedule the next listen interval.
    // The value is in usec.
    static constexpr uint32_t kWedReceiveTimeAfter = OPENTHREAD_CONFIG_WED_RECEIVE_TIME_AFTER;
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    // CSL transmitter would schedule delayed transmission `kCslTransmitTimeAhead` earlier
    // than expected delayed transmit time. The value is in usec.
    // Only for radios not supporting OT_RADIO_CAPS_TRANSMIT_TIMING.
    static constexpr uint32_t kCslTransmitTimeAhead = OPENTHREAD_CONFIG_CSL_TRANSMIT_TIME_AHEAD;
#endif

    /**
     * Initializes the states of the sub-MAC layer.
     */
    void Init(void);

    bool RadioSupportsCsmaBackoff(void) const
    {
        return ((mRadioCaps & (OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_TRANSMIT_RETRIES)) != 0);
    }

    bool RadioSupportsTransmitSecurity(void) const { return ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_SEC) != 0); }
    bool RadioSupportsRetries(void) const { return ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_RETRIES) != 0); }
    bool RadioSupportsAckTimeout(void) const { return ((mRadioCaps & OT_RADIO_CAPS_ACK_TIMEOUT) != 0); }
    bool RadioSupportsEnergyScan(void) const { return ((mRadioCaps & OT_RADIO_CAPS_ENERGY_SCAN) != 0); }
    bool RadioSupportsTransmitTiming(void) const { return ((mRadioCaps & OT_RADIO_CAPS_TRANSMIT_TIMING) != 0); }
    bool RadioSupportsReceiveTiming(void) const { return ((mRadioCaps & OT_RADIO_CAPS_RECEIVE_TIMING) != 0); }
    bool RadioSupportsRxOnWhenIdle(void) const { return ((mRadioCaps & OT_RADIO_CAPS_RX_ON_WHEN_IDLE) != 0); }

    bool ShouldHandleTransmitSecurity(void) const;
    bool ShouldHandleCsmaBackOff(void) const;
    bool ShouldHandleAckTimeout(void) const;
    bool ShouldHandleRetries(void) const;
    bool ShouldHandleEnergyScan(void) const;
    bool ShouldHandleTransmitTargetTime(void) const;
    bool ShouldHandleTransitionToSleep(void) const;

    void ProcessTransmitSecurity(void);
    void SignalFrameCounterUsed(uint32_t aFrameCounter, uint8_t aKeyId);
    void StartCsmaBackoff(void);
    void StartTimerForBackoff(uint8_t aBackoffExponent);
    void BeginTransmit(void);
    void SampleRssi(void);
    void StartTimer(uint32_t aDelayUs);
    void StartTimerAt(Time aStartTime, uint32_t aDelayUs);

    void HandleReceiveDone(RxFrame *aFrame, Error aError);
    void HandleTransmitStarted(TxFrame &aFrame);
    void HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError);
    void SignalFrameCounterUsedOnTxDone(const TxFrame &aFrame);
    void HandleEnergyScanDone(int8_t aMaxRssi);
    void HandleTimer(void);

    void               SetState(State aState);
    static const char *StateToString(State aState);

    using SubMacTimer =
#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
        TimerMicroIn<SubMac, &SubMac::HandleTimer>;
#else
        TimerMilliIn<SubMac, &SubMac::HandleTimer>;
#endif

    otRadioCaps  mRadioCaps;
    State        mState;
    uint8_t      mCsmaBackoffs;
    uint8_t      mTransmitRetries;
    ShortAddress mShortAddress;
    ExtAddress   mExtAddress;
    bool         mRxOnWhenIdle : 1;
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    bool mRadioFilterEnabled : 1;
#endif
    int8_t                       mEnergyScanMaxRssi;
    TimeMilli                    mEnergyScanEndTime;
    TxFrame                     &mTransmitFrame;
    Callbacks                    mCallbacks;
    Callback<otLinkPcapCallback> mPcapCallback;
    KeyMaterial                  mPrevKey;
    KeyMaterial                  mCurrKey;
    KeyMaterial                  mNextKey;
    uint32_t                     mFrameCounter;
    uint8_t                      mKeyId;
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    uint8_t mRetxDelayBackOffExponent;
#endif
    SubMacTimer mTimer;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    uint16_t mCslPeriod;            // The CSL sample period, in units of 10 symbols (160 microseconds).
    uint8_t  mCslChannel : 7;       // The CSL sample channel.
    bool     mIsCslSampling : 1;    // Indicates that the radio is receiving in CSL state for platforms not supporting
                                    // delayed reception.
    uint16_t    mCslPeerShort;      // The CSL peer short address.
    TimeMicro   mCslSampleTime;     // The CSL sample time of the current period relative to the local radio clock.
    TimeMicro   mCslLastSync;       // The timestamp of the last successful CSL synchronization.
    CslAccuracy mCslParentAccuracy; // The parent's CSL accuracy (clock accuracy and uncertainty).
    TimerMicro  mCslTimer;
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    uint32_t   mWakeupListenInterval; // The wake-up listen interval, in microseconds.
    uint32_t   mWakeupListenDuration; // The wake-up listen duration, in microseconds.
    uint8_t    mWakeupChannel;        // The wake-up sample channel.
    TimeMicro  mWedSampleTime;        // The WED sample time of the current interval in local time.
    uint64_t   mWedSampleTimeRadio;   // The WED sample time of the current interval in radio time.
    TimerMicro mWedTimer;
#endif
};

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // SUB_MAC_HPP_
