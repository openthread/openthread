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

#ifndef OT_CORE_MAC_SUB_MAC_HPP_
#define OT_CORE_MAC_SUB_MAC_HPP_

#include "openthread-core-config.h"

#include <openthread/link.h>

#include <openthread/platform/crypto.h>

#include "common/callback.hpp"
#include "common/clearable.hpp"
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

//----------------------------------------------------------------------------------------------------------------------
// Derived configs

#ifdef OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
#error "OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE MUST NOT be defined directly. It is derived from other configs"
#endif

#define OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE                                                        \
    (OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE || OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE || \
     ((OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE) && OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE))

#ifdef OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
#error "OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE MUST NOT be defined directly. It is derived from other configs"
#endif

#define OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE                                                    \
    (OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE || \
     ((OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE) && OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE))

//----------------------------------------------------------------------------------------------------------------------
// Config validity checks

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE && (OPENTHREAD_CONFIG_THREAD_VERSION < OT_THREAD_VERSION_1_2)
#error "Thread 1.2 or higher version is required for OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE."
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE && (OPENTHREAD_CONFIG_THREAD_VERSION < OT_THREAD_VERSION_1_2)
#error "Thread 1.2 or higher version is required for OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE."
#endif

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE && !OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#error "Microsecond timer OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE is required for "\
       "TARGET TIME RX feature (CSL, Thread direct wakeup listener, etc)."
#endif

//----------------------------------------------------------------------------------------------------------------------

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
    using Capability   = Radio::Capability;   ///< A radio capability.
    using Capabilities = Radio::Capabilities; ///< A bit-vector of radio capabilities.

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
         * @param[in] aFrameInfo  The transmitted frame information.
         * @param[in] aError      kErrorNone when the frame was transmitted successfully,
         *                        kErrorNoAck when the frame was transmitted but no ACK was received,
         *                        kErrorChannelAccessFailure tx failed due to activity on the channel,
         *                        kErrorAbort when transmission was aborted for other reasons.
         * @param[in] aRetryCount Current retry count. This is valid only when sub-mac handles frame re-transmissions.
         * @param[in] aWillRetx   Indicates whether frame will be retransmitted or not. This is applicable only
         *                        when there was an error in current transmission attempt.
         */
        void RecordFrameTransmitStatus(const TxFrame::ParseInfo &aFrameInfo,
                                       Error                     aError,
                                       uint8_t                   aRetryCount,
                                       bool                      aWillRetx);

        /**
         * The method notifies user of `SubMac` that the transmit operation has completed, providing, if applicable,
         * the received ACK frame.
         *
         * @param[in]  aFrameInfo The transmitted frame information.
         * @param[in]  aAckFrame  A pointer to the ACK frame, `nullptr` if no ACK was received.
         * @param[in]  aError     kErrorNone when the frame was transmitted,
         *                        kErrorNoAck when the frame was transmitted but no ACK was received,
         *                        kErrorChannelAccessFailure tx failed due to activity on the channel,
         *                        kErrorAbort when transmission was aborted for other reasons.
         */
        void TransmitDone(TxFrame::ParseInfo &aFrameInfo, RxFrame *aAckFrame, Error aError);

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
     * @returns The capability bit vector (see `Radio::Capability` definitions).
     */
    Capabilities GetRadioCaps(void) const { return mRadioCaps; }

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    /**
     * Gets the capabilities provided by `SubMac` layer.
     *
     * @returns The capability bit vector (see `Radio::Capability` definitions).
     */
    Capabilities GetCaps(void) const;
#elif OPENTHREAD_RADIO
    Capabilities GetCaps(void) const { return mRadioCaps | kSwEnabledCapabilities; }
#endif

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
     * Gets the alternate short address.
     *
     * @returns The alternate short address, or `kShortAddrInvalid` if there is no alternate address.
     */
    ShortAddress GetAlternateShortAddress(void) const { return mAlternateShortAddress; }

    /**
     * Sets the alternate short address.
     *
     * @param[in] aShortAddress   The short address. Use `kShortAddrInvalid` to clear it.
     */
    void SetAlternateShortAddress(ShortAddress aShortAddress);

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
     * @param[in]  aCallback   The packet capture callback, or `nullptr` to disable packet capture.
     * @param[in]  aContext    A pointer to application-specific context.
     */
    void SetPcapCallback(PcapCallback aCallback, void *aContext) { mPcapCallback.Set(aCallback, aContext); }

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
     * Request radio to transition to sleep state.
     *
     * The `SubMac` layer may enter `Receive()` state when the CSL receiver is enabled.
     *
     * @retval kErrorNone          Successfully transitioned to Sleep or the radio is handled by the CSL receiver.
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

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    /**
     * Schedules a radio reception window at a specific time and duration.
     *
     * `SubMac` supports one active and one pending reception window. If an unstarted receive window is already
     * pending when this method is called, the new window will replace the existing pending window.
     *
     * @param[in] aStartTime  The start time in microseconds.
     * @param[in] aDuration   The duration of the receive window in microseconds.
     * @param[in] aChannel    The channel to use for receiving.
     */
    void ReceiveAt(Radio::Time64 aStartTime, uint32_t aDuration, uint8_t aChannel);

    /**
     * Cancels any pending scheduled receive window (`ReceiveAt()`).
     *
     * If a scheduled reception window is currently pending (waiting to start), it is cancelled. If a timed reception
     * window is already active (radio is currently receiving), this method does not interrupt the ongoing reception.
     */
    void CancelPendingReceiveAt(void);
#endif

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
     * Sets CSL parameters in 'SubMac'.
     *
     * @param[in]  aPeriod    The CSL period (in unit of 10 symbols), 0 for disabling CSL receiver.
     * @param[in]  aChannel   The CSL channel.
     * @param[in]  aShortAddr The short source address of CSL receiver's peer.
     * @param[in]  aExtAddr   The extended source address of CSL receiver's peer.
     */
    void SetCslParams(uint16_t aPeriod, uint8_t aChannel, ShortAddress aShortAddr, const ExtAddress &aExtAddr)
    {
        mCslReceiver.SetParams(aPeriod, aChannel, aShortAddr, aExtAddr);
    }

    /**
     * Returns parent CSL accuracy (clock accuracy and uncertainty).
     *
     * @returns The parent CSL accuracy.
     */
    const CslAccuracy &GetCslParentAccuracy(void) const { return mCslReceiver.GetParentAccuracy(); }

    /**
     * Sets parent CSL accuracy.
     *
     * @param[in] aCslAccuracy  The parent CSL accuracy.
     */
    void SetCslParentAccuracy(const CslAccuracy &aCslAccuracy) { mCslReceiver.SetParentAccuracy(aCslAccuracy); }

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

    /**
     * Gets a MAC key of a given type from `SubMac`.
     *
     * @param[in] aType  The key type (`KeyTrio::kPrev`, `KeyTrio::kCur`, or `KeyTrio::kNext`).
     *
     * @returns A reference to the requested MAC key.
     */
    const KeyMaterial &GetMacKey(KeyTrio::Type aType) const { return mKeyTrio.GetKey(aType); }

    /**
     * Sets MAC keys and key index for Key ID Mode 1.
     *
     * @param[in] aKeyIndex   The key index.
     * @param[in] aPrevKey    The previous MAC key.
     * @param[in] aCurKey     The current MAC key.
     * @param[in] aNextKey    The next MAC key.
     */
    void SetMode1MacKeys(uint8_t aKeyIndex, const Key &aPrevKey, const Key &aCurKey, const Key &aNextKey);

    /**
     * Clears the stored MAC keys.
     */
    void ClearMacKeys(void) { mKeyTrio.Clear(); }

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

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
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

    static constexpr Capability kCapAckTimeout         = Radio::kCapAckTimeout;
    static constexpr Capability kCapEnergyScan         = Radio::kCapEnergyScan;
    static constexpr Capability kCapTransmitRetries    = Radio::kCapTransmitRetries;
    static constexpr Capability kCapCsmaBackoff        = Radio::kCapCsmaBackoff;
    static constexpr Capability kCapSleepToTx          = Radio::kCapSleepToTx;
    static constexpr Capability kCapTransmitSec        = Radio::kCapTransmitSec;
    static constexpr Capability kCapTransmitTiming     = Radio::kCapTransmitTiming;
    static constexpr Capability kCapReceiveTiming      = Radio::kCapReceiveTiming;
    static constexpr Capability kCapRxOnWhenIdle       = Radio::kCapRxOnWhenIdle;
    static constexpr Capability kCapTransmitFramePower = Radio::kCapTransmitFramePower;
    static constexpr Capability kCapAltShortAddr       = Radio::kCapAltShortAddr;

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

#define ConditionalCap(kCapability, kEnableConfig) ((kEnableConfig) ? kCapability : 0)

    static constexpr Capabilities kSwEnabledCapabilities =
        ConditionalCap(kCapAckTimeout, OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE) |
        ConditionalCap(kCapEnergyScan, OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE) |
        ConditionalCap(kCapTransmitRetries, OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE) |
        ConditionalCap(kCapCsmaBackoff, OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE) |
        ConditionalCap(kCapTransmitSec, OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE) |
        ConditionalCap(kCapTransmitTiming, OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE) |
        ConditionalCap(kCapSleepToTx, OPENTHREAD_RADIO);

#undef ConditionalCap

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
#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
        kStateTimedTransmit, // Timed TX (e.g., for CSL)
#endif
#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
        kStateTimedReceive, // Timed RX (CSL sampling or wake listening)
#endif
    };

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    // Wake-up listening receivers would wake up `kWedReceiveTimeAhead` earlier
    // than expected sample window. The value is in usec.
    static constexpr uint32_t kWedReceiveTimeAhead = OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD;

    // Margin to be applied after the end of a wake-up listen duration to schedule the next listen interval.
    // The value is in usec.
    static constexpr uint32_t kWedReceiveTimeAfter = 500;
#endif

#if OT_CONFIG_MAC_TARGET_TIME_TX_ENABLE
    // Lead time (in microseconds) to schedule a delayed tx earlier
    // than expected target tx time. Only used when radio does not
    // itself support `kCapTransmitTiming`.
    static constexpr uint32_t kTimedTxLeadTime =
        OPENTHREAD_CONFIG_CSL_TRANSMIT_TIME_AHEAD + kCcaSampleInterval + Radio::kHeaderShrDuration;
#endif

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    class TimedRx : public Clearable<TimedRx>
    {
    public:
        TimedRx(void) { Clear(); }

        void          Init(Radio::Time64 aStartTime, uint32_t aDuration, uint8_t aChannel);
        bool          IsSpecified(void) const { return mIsSpecified; }
        Radio::Time64 GetStartTime(void) const { return mStartTime; }
        Radio::Time64 GetEndTime(void) const { return mStartTime + mDuration; }
        uint8_t       GetChannel(void) const { return mChannel; }
        bool          HasStarted(const Radio::SyncedTime &aNow) const { return mStartTime <= aNow.GetAsTime64(); }
        bool          HasEnded(const Radio::SyncedTime &aNow) const { return GetEndTime() <= aNow.GetAsTime64(); }
        void          ScheduleOnRadio(Radio::Radio &aRadio) const;

    private:
        Radio::Time64 mStartTime;
        uint32_t      mDuration;
        uint8_t       mChannel;
        bool          mIsSpecified;
    };
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    void HandleCslReceiverTimer(void) { mCslReceiver.HandleTimer(); }

    class CslReceiver : public InstanceLocator
    {
    public:
        explicit CslReceiver(Instance &aInstance);

        void Init(void);
        void Stop(void) { mTimer.Stop(); }
        void SetParams(uint16_t aPeriod, uint8_t aChannel, ShortAddress aShortAddr, const ExtAddress &aExtAddr);
        void UpdateLastSyncTimestamp(const TxFrame::ParseInfo &aFrameInfo, RxFrame *aAckFrame);
        void UpdateLastSyncTimestamp(RxFrame *aFrame, Error aError);
        void HandleTimer(void);

        const CslAccuracy &GetParentAccuracy(void) const { return mParentAccuracy; }
        void               SetParentAccuracy(const CslAccuracy &aCslAccuracy) { mParentAccuracy = aCslAccuracy; }

    private:
        static constexpr uint32_t kMinReceiveOnAhead = OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD;
        static constexpr uint32_t kMinReceiveOnAfter = OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER;
        static constexpr uint32_t kReceiveTimeAhead  = OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD;

        void     RestartTimerAfterSyncUpdate(void);
        void     SetLastSyncToNow(void);
        void     GetWindowEdges(uint32_t &aAhead, uint32_t &aAfter);
        uint32_t DetermineClockDrift(uint32_t aIntervalUs) const;
        uint32_t GetNextCycleDrift(void) const;
        bool     IsEnabled(void) const { return mPeriod > 0; }
        void     LogWindow(Radio::Time64 aStart, uint32_t aDuration);
        void     LogReceived(RxFrame *aFrame);

        using CslTimer = TimerMicroIn<SubMac, &SubMac::HandleCslReceiverTimer>;

        uint16_t          mPeriod;
        uint8_t           mChannel;
        uint16_t          mPeerShort;
        Radio::SyncedTime mSampleTime;
        CslAccuracy       mParentAccuracy;
        CslTimer          mTimer;
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
        TimeMicro mLastSync;
#else
        Radio::Time64 mLastSync;
#endif
    };

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

    void Init(void);

    bool RadioSupports(Capability aCapability) const { return (mRadioCaps & aCapability) != 0; }
    bool ShouldHandle(Capability aCapability) const;
    bool ShouldHandleCsmaBackoff(void) const;

    void ProcessTransmitSecurity(TxFrame::ParseInfo &aFrameInfo);
    void ReprocessSecurityForRetx(TxFrame::ParseInfo &aFrameInfo);
    void SignalFrameCounterUsed(uint32_t aFrameCounter, uint8_t aKeyIndex);
    void StartCsmaBackoff(void);
    void StartTimerForBackoff(uint8_t aBackoffExponent);
    void BeginTransmit(void);
    void SampleRssi(void);
    void StartTimer(uint32_t aDelayUs);
    void StartTimerAt(Time aStartTime, uint32_t aDelayUs);

    void HandleReceiveDone(RxFrame *aFrame, Error aError);
    void HandleTransmitStarted(TxFrame &aFrame);
    void HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError);
    void SignalFrameCounterUsedOnTxDone(const TxFrame::ParseInfo &aFrameInfo);
    void HandleEnergyScanDone(int8_t aMaxRssi);
    void HandleTimer(void);

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    void StartPendingTimedRx(void);
    void ProcessTimedRx(void);
#endif

    void               SetState(State aState);
    static const char *StateToString(State aState);

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    void WedInit(void);
    void HandleWedTimer(void);
#endif

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
    using SubMacTimer = TimerMicroIn<SubMac, &SubMac::HandleTimer>;
#else
    using SubMacTimer = TimerMilliIn<SubMac, &SubMac::HandleTimer>;
#endif

    Capabilities mRadioCaps;
    State        mState;
    uint8_t      mCsmaBackoffs;
    uint8_t      mTransmitRetries;
    ShortAddress mShortAddress;
    ShortAddress mAlternateShortAddress;
    ExtAddress   mExtAddress;
    bool         mRxOnWhenIdle : 1;
#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    bool mRadioFilterEnabled : 1;
#endif
    int8_t                 mEnergyScanMaxRssi;
    TimeMilli              mEnergyScanEndTime;
    TxFrame               &mTransmitFrame;
    Callbacks              mCallbacks;
    Callback<PcapCallback> mPcapCallback;
    KeyTrio                mKeyTrio;
    uint32_t               mFrameCounter;
#if OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
    uint8_t mRetxDelayBackoffExponent;
#endif
#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    TimedRx mActiveTimedRx;
    TimedRx mPendingTimedRx;
#endif

    SubMacTimer mTimer;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    CslReceiver mCslReceiver;
#endif

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
    using WedTimer = TimerMicroIn<SubMac, &SubMac::HandleWedTimer>;

    bool              mIsWedEnabled;         // Indicates if the WED is enabled.
    uint32_t          mWakeupListenInterval; // The wake-up listen interval, in microseconds.
    uint32_t          mWakeupListenDuration; // The wake-up listen duration, in microseconds.
    uint8_t           mWakeupChannel;        // The wake-up sample channel.
    Radio::SyncedTime mWedSampleTime;        // The WED sample time of the current interval.
    WedTimer          mWedTimer;
#endif
};

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // OT_CORE_MAC_SUB_MAC_HPP_
