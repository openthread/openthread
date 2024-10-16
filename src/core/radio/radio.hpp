/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for OpenThread radio abstraction.
 */

#ifndef RADIO_HPP_
#define RADIO_HPP_

#include "openthread-core-config.h"

#include <openthread/radio_stats.h>
#include <openthread/platform/crypto.h>
#include <openthread/platform/radio.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/numeric_limits.hpp"
#include "common/time.hpp"
#include "mac/mac_frame.hpp"

namespace ot {

static constexpr uint32_t kUsPerTenSymbols = OT_US_PER_TEN_SYMBOLS; ///< Time for 10 symbols in units of microseconds
static constexpr uint32_t kRadioHeaderShrDuration = 160;            ///< Duration of SHR in us
static constexpr uint32_t kRadioHeaderPhrDuration = 32;             ///< Duration of PHR in us
static constexpr uint32_t kOctetDuration          = 32;             ///< Duration of one octet in us

static constexpr int8_t kRadioPowerInvalid = OT_RADIO_POWER_INVALID; ///< Invalid TX power value

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
/**
 * Minimum CSL period supported in units of 10 symbols.
 */
static constexpr uint64_t kMinCslPeriod  = OPENTHREAD_CONFIG_MAC_CSL_MIN_PERIOD * 1000 / kUsPerTenSymbols;
static constexpr uint64_t kMaxCslTimeout = OPENTHREAD_CONFIG_MAC_CSL_MAX_TIMEOUT;
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
/**
 * Minimum wake-up listen duration supported in microseconds.
 */
static constexpr uint32_t kMinWakeupListenDuration = 100;
#endif

/**
 * @addtogroup core-radio
 *
 * @brief
 *   This module includes definitions for OpenThread radio abstraction.
 *
 * @{
 */

/**
 * Implements the radio statistics logic.
 *
 * The radio statistics are the time when the radio in TX/RX/radio state.
 * Since this class collects these statistics from pure software level and no platform API is involved, a simplified
 * model is used to calculate the time of different radio states. The data may not be very accurate, but it's
 * sufficient to provide a general understanding of the proportion of time a device is in different radio states.
 *
 * The simplified model is:
 * - The RadioStats is only aware of 2 states: RX and sleep.
 * - Each time `Radio::Receive` or `Radio::Sleep` is called, it will check the current state and add the time since
 *   last time the methods were called. For example, `Sleep` is first called and `Receive` is called after 1 second,
 *   then 1 second will be added to SleepTime and the current state switches to `Receive`.
 * - The time of TX will be calculated from the callback of TransmitDone. If TX returns OT_ERROR_NONE or
 *   OT_ERROR_NO_ACK, the tx time will be added according to the number of bytes sent. And the SleepTime or RxTime
 *   will be reduced accordingly.
 * - When `GetStats` is called, an operation will be executed to calcute the time for the last state. And the result
 *   will be returned.
 */
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)

#if !OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#error "OPENTHREAD_CONFIG_RADIO_STATS_ENABLE requires OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE".
#endif

class RadioStatistics
{
public:
    enum Status : uint8_t
    {
        kDisabled,
        kSleep,
        kReceive,
    };

    explicit RadioStatistics(void);

    void                    RecordStateChange(Status aStatus);
    void                    HandleReceiveAt(uint32_t aDurationUs);
    void                    RecordTxDone(otError aError, uint16_t aPsduLength);
    void                    RecordRxDone(otError aError);
    const otRadioTimeStats &GetStats(void);
    void                    ResetTime(void);

private:
    void UpdateTime(void);

    Status           mStatus;
    otRadioTimeStats mTimeStats;
    TimeMicro        mLastUpdateTime;
};
#endif // OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)

/**
 * Represents an OpenThread radio abstraction.
 */
class Radio : public InstanceLocator, private NonCopyable
{
    friend class Instance;

public:
    static constexpr uint32_t kSymbolTime      = OT_RADIO_SYMBOL_TIME;
    static constexpr uint8_t  kSymbolsPerOctet = OT_RADIO_SYMBOLS_PER_OCTET;
    static constexpr uint32_t kPhyUsPerByte    = kSymbolsPerOctet * kSymbolTime;
    static constexpr uint8_t  kChannelPage0    = OT_RADIO_CHANNEL_PAGE_0;
    static constexpr uint8_t  kChannelPage2    = OT_RADIO_CHANNEL_PAGE_2;
#if (OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT && OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT)
    static constexpr uint16_t kNumChannelPages = 2;
    static constexpr uint32_t kSupportedChannels =
        OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK | OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK;
    static constexpr uint8_t kChannelMin = OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN;
    static constexpr uint8_t kChannelMax = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX;
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    static constexpr uint16_t kNumChannelPages   = 1;
    static constexpr uint32_t kSupportedChannels = OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK;
    static constexpr uint8_t  kChannelMin        = OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN;
    static constexpr uint8_t  kChannelMax        = OT_RADIO_915MHZ_OQPSK_CHANNEL_MAX;
#elif OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    static constexpr uint16_t kNumChannelPages   = 1;
    static constexpr uint32_t kSupportedChannels = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK;
    static constexpr uint8_t  kChannelMin        = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN;
    static constexpr uint8_t  kChannelMax        = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX;
#elif OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
    static constexpr uint16_t kNumChannelPages   = 1;
    static constexpr uint32_t kSupportedChannels = OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MASK;
    static constexpr uint8_t  kChannelMin        = OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN;
    static constexpr uint8_t  kChannelMax        = OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MAX;
#endif

    static const uint8_t kSupportedChannelPages[kNumChannelPages];

    static constexpr int8_t kInvalidRssi = OT_RADIO_RSSI_INVALID; ///< Invalid RSSI value.

    static constexpr int8_t kDefaultReceiveSensitivity = -110; ///< Default receive sensitivity (in dBm).

    static_assert((OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT || OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT ||
                   OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT),
                  "OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT "
                  "or OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT "
                  "or OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT "
                  "must be set to 1 to specify the radio mode");

    /**
     * Defines the callbacks from `Radio`.
     */
    class Callbacks : public InstanceLocator
    {
        friend class Radio;

    public:
        /**
         * This callback method handles a "Receive Done" event from radio platform.
         *
         * @param[in]  aFrame    A pointer to the received frame or `nullptr` if the receive operation failed.
         * @param[in]  aError    kErrorNone when successfully received a frame,
         *                       kErrorAbort when reception was aborted and a frame was not received,
         *                       kErrorNoBufs when a frame could not be received due to lack of rx buffer space.
         */
        void HandleReceiveDone(Mac::RxFrame *aFrame, Error aError);

        /**
         * This callback method handles a "Transmit Started" event from radio platform.
         *
         * @param[in]  aFrame     The frame that is being transmitted.
         */
        void HandleTransmitStarted(Mac::TxFrame &aFrame);

        /**
         * This callback method handles a "Transmit Done" event from radio platform.
         *
         * @param[in]  aFrame     The frame that was transmitted.
         * @param[in]  aAckFrame  A pointer to the ACK frame, `nullptr` if no ACK was received.
         * @param[in]  aError     kErrorNone when the frame was transmitted,
         *                        kErrorNoAck when the frame was transmitted but no ACK was received,
         *                        kErrorChannelAccessFailure tx could not take place due to activity on the
         *                        channel, kErrorAbort when transmission was aborted for other reasons.
         */
        void HandleTransmitDone(Mac::TxFrame &aFrame, Mac::RxFrame *aAckFrame, Error aError);

        /**
         * This callback method handles "Energy Scan Done" event from radio platform.
         *
         * Is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability. It is called from
         * `otPlatRadioEnergyScanDone()`.
         *
         * @param[in]  aMaxRssi  The maximum RSSI encountered on the scanned channel.
         */
        void HandleEnergyScanDone(int8_t aMaxRssi);

        /**
         * This callback method handles "Bus Latency Changed" event from radio platform.
         */
        void HandleBusLatencyChanged(void);

#if OPENTHREAD_CONFIG_DIAG_ENABLE
        /**
         * This callback method handles a "Receive Done" event from radio platform when diagnostics mode is enabled.
         *
         * @param[in]  aFrame    A pointer to the received frame or `nullptr` if the receive operation failed.
         * @param[in]  aError    kErrorNone when successfully received a frame,
         *                       kErrorAbort when reception was aborted and a frame was not received,
         *                       kErrorNoBufs when a frame could not be received due to lack of rx buffer space.
         */
        void HandleDiagsReceiveDone(Mac::RxFrame *aFrame, Error aError);

        /**
         * This callback method handles a "Transmit Done" event from radio platform when diagnostics mode is enabled.
         *
         * @param[in]  aFrame     The frame that was transmitted.
         * @param[in]  aError     kErrorNone when the frame was transmitted,
         *                        kErrorNoAck when the frame was transmitted but no ACK was received,
         *                        kErrorChannelAccessFailure tx could not take place due to activity on the
         *                        channel, kErrorAbort when transmission was aborted for other reasons.
         */
        void HandleDiagsTransmitDone(Mac::TxFrame &aFrame, Error aError);
#endif

    private:
        explicit Callbacks(Instance &aInstance)
            : InstanceLocator(aInstance)
        {
        }
    };

    /**
     * Initializes the `Radio` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit Radio(Instance &aInstance)
        : InstanceLocator(aInstance)
        , mCallbacks(aInstance)
    {
    }

    /**
     * Gets the radio version string.
     *
     * @returns A pointer to the OpenThread radio version.
     */
    const char *GetVersionString(void);

    /**
     * Gets the factory-assigned IEEE EUI-64 for the device.
     *
     * @param[out] aIeeeEui64  A reference to `Mac::ExtAddress` to place the factory-assigned IEEE EUI-64.
     */
    void GetIeeeEui64(Mac::ExtAddress &aIeeeEui64);

    /**
     * Gets the radio capabilities.
     *
     * @returns The radio capability bit vector (see `OT_RADIO_CAP_*` definitions).
     */
    otRadioCaps GetCaps(void);

    /**
     * Gets the radio receive sensitivity value.
     *
     * @returns The radio receive sensitivity value in dBm.
     */
    int8_t GetReceiveSensitivity(void) const;

#if OPENTHREAD_RADIO
    /**
     * Initializes the states of the Thread radio.
     */
    void Init(void);
#endif

    /**
     * Sets the PAN ID for address filtering.
     *
     * @param[in] aPanId     The IEEE 802.15.4 PAN ID.
     */
    void SetPanId(Mac::PanId aPanId);

    /**
     * Sets the Extended Address for address filtering.
     *
     * @param[in] aExtAddress  The IEEE 802.15.4 Extended Address stored in little-endian byte order.
     */
    void SetExtendedAddress(const Mac::ExtAddress &aExtAddress);

    /**
     * Sets the Short Address for address filtering.
     *
     * @param[in] aShortAddress  The IEEE 802.15.4 Short Address.
     */
    void SetShortAddress(Mac::ShortAddress aShortAddress);

    /**
     * Sets MAC key and key ID.
     *
     * @param[in] aKeyIdMode  MAC key ID mode.
     * @param[in] aKeyId      Current MAC key index.
     * @param[in] aPrevKey    The previous MAC key.
     * @param[in] aCurrKey    The current MAC key.
     * @param[in] aNextKey    The next MAC key.
     */
    void SetMacKey(uint8_t                 aKeyIdMode,
                   uint8_t                 aKeyId,
                   const Mac::KeyMaterial &aPrevKey,
                   const Mac::KeyMaterial &aCurrKey,
                   const Mac::KeyMaterial &aNextKey);

    /**
     * Sets the current MAC Frame Counter value.
     *
     * @param[in] aMacFrameCounter  The MAC Frame Counter value.
     */
    void SetMacFrameCounter(uint32_t aMacFrameCounter)
    {
        otPlatRadioSetMacFrameCounter(GetInstancePtr(), aMacFrameCounter);
    }

    /**
     * Sets the current MAC Frame Counter value only if the new given value is larger than the current
     * value.
     *
     * @param[in] aMacFrameCounter  The MAC Frame Counter value.
     */
    void SetMacFrameCounterIfLarger(uint32_t aMacFrameCounter)
    {
        otPlatRadioSetMacFrameCounterIfLarger(GetInstancePtr(), aMacFrameCounter);
    }

    /**
     * Gets the radio's transmit power in dBm.
     *
     * @param[out] aPower    A reference to output the transmit power in dBm.
     *
     * @retval kErrorNone             Successfully retrieved the transmit power.
     * @retval kErrorNotImplemented   Transmit power configuration via dBm is not implemented.
     */
    Error GetTransmitPower(int8_t &aPower);

    /**
     * Sets the radio's transmit power in dBm.
     *
     * @param[in] aPower     The transmit power in dBm.
     *
     * @retval kErrorNone             Successfully set the transmit power.
     * @retval kErrorNotImplemented   Transmit power configuration via dBm is not implemented.
     */
    Error SetTransmitPower(int8_t aPower);

    /**
     * Gets the radio's CCA ED threshold in dBm.
     *
     * @param[in] aThreshold    The CCA ED threshold in dBm.
     *
     * @retval kErrorNone             A reference to output the CCA ED threshold in dBm.
     * @retval kErrorNotImplemented   CCA ED threshold configuration via dBm is not implemented.
     */
    Error GetCcaEnergyDetectThreshold(int8_t &aThreshold);

    /**
     * Sets the radio's CCA ED threshold in dBm.
     *
     * @param[in] aThreshold    The CCA ED threshold in dBm.
     *
     * @retval kErrorNone             Successfully set the CCA ED threshold.
     * @retval kErrorNotImplemented   CCA ED threshold configuration via dBm is not implemented.
     */
    Error SetCcaEnergyDetectThreshold(int8_t aThreshold);

    /**
     * Gets the status of promiscuous mode.
     *
     * @retval TRUE   Promiscuous mode is enabled.
     * @retval FALSE  Promiscuous mode is disabled.
     */
    bool GetPromiscuous(void);

    /**
     * Enables or disables promiscuous mode.
     *
     * @param[in]  aEnable   TRUE to enable or FALSE to disable promiscuous mode.
     */
    void SetPromiscuous(bool aEnable);

    /**
     * Indicates whether radio should stay in Receive or Sleep during idle periods.
     *
     * @param[in]  aEnable   TRUE to keep radio in Receive, FALSE to put to Sleep during idle periods.
     */
    void SetRxOnWhenIdle(bool aEnable);

    /**
     * Returns the current state of the radio.
     *
     * Is not required by OpenThread. It may be used for debugging and/or application-specific purposes.
     *
     * @note This function may be not implemented. In this case it always returns OT_RADIO_STATE_INVALID state.
     *
     * @return  Current state of the radio.
     */
    otRadioState GetState(void);

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
     * @retval kErrorNone           Successfully transitioned to Disabled.
     * @retval kErrorInvalidState   The radio was not in sleep state.
     */
    Error Disable(void);

    /**
     * Indicates whether radio is enabled or not.
     *
     * @returns TRUE if the radio is enabled, FALSE otherwise.
     */
    bool IsEnabled(void);

    /**
     * Transitions the radio from Receive to Sleep (turn off the radio).
     *
     * @retval kErrorNone          Successfully transitioned to Sleep.
     * @retval kErrorBusy          The radio was transmitting.
     * @retval kErrorInvalidState  The radio was disabled.
     */
    Error Sleep(void);

    /**
     * Transitions the radio from Sleep to Receive (turn on the radio).
     *
     * @param[in]  aChannel   The channel to use for receiving.
     *
     * @retval kErrorNone          Successfully transitioned to Receive.
     * @retval kErrorInvalidState  The radio was disabled or transmitting.
     */
    Error Receive(uint8_t aChannel);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    /**
     * Schedules a radio reception window at a specific time and duration.
     *
     * @param[in]  aChannel   The radio channel on which to receive.
     * @param[in]  aStart     The receive window start time, in microseconds.
     * @param[in]  aDuration  The receive window duration, in microseconds.
     *
     * @retval kErrorNone    Successfully scheduled receive window.
     * @retval kErrorFailed  The receive window could not be scheduled.
     */
    Error ReceiveAt(uint8_t aChannel, uint32_t aStart, uint32_t aDuration);
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Updates the CSL sample time in radio.
     *
     * @param[in]  aCslSampleTime  The CSL sample time.
     */
    void UpdateCslSampleTime(uint32_t aCslSampleTime);

    /**
     * Enables CSL sampling in radio.
     *
     * @param[in]  aCslPeriod    CSL period, 0 for disabling CSL.
     * @param[in]  aShortAddr    The short source address of CSL receiver's peer.
     * @param[in]  aExtAddr      The extended source address of CSL receiver's peer.
     *
     * @note Platforms should use CSL peer addresses to include CSL IE when generating enhanced acks.
     *
     * @retval  kErrorNotImplemented Radio driver doesn't support CSL.
     * @retval  kErrorFailed         Other platform specific errors.
     * @retval  kErrorNone           Successfully enabled or disabled CSL.
     */
    Error EnableCsl(uint32_t aCslPeriod, otShortAddress aShortAddr, const otExtAddress *aExtAddr);

    /**
     * Resets CSL receiver in radio.
     *
     * @retval  kErrorNotImplemented Radio driver doesn't support CSL.
     * @retval  kErrorFailed         Other platform specific errors.
     * @retval  kErrorNone           Successfully disabled CSL.
     */
    Error ResetCsl(void);
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE || \
    OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    /**
     * Get the current radio time in microseconds referenced to a continuous monotonic local radio clock (64 bits
     * width).
     *
     * @returns The current radio clock time.
     */
    uint64_t GetNow(void);

    /**
     * Get the current accuracy, in units of ± ppm, of the clock used for scheduling CSL operations.
     *
     * @note Platforms may optimize this value based on operational conditions (i.e.: temperature).
     *
     * @returns The current CSL rx/tx scheduling drift, in units of ± ppm.
     */
    uint8_t GetCslAccuracy(void);

    /**
     * Get the fixed uncertainty of the Device for scheduling CSL operations in units of 10 microseconds.
     *
     * @returns The CSL Uncertainty in units of 10 us.
     */
    uint8_t GetCslUncertainty(void);
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

    /**
     * Gets the radio transmit frame buffer.
     *
     * OpenThread forms the IEEE 802.15.4 frame in this buffer then calls `Transmit()` to request transmission.
     *
     * @returns A reference to the transmit frame buffer.
     */
    Mac::TxFrame &GetTransmitBuffer(void);

    /**
     * Starts the transmit sequence on the radio.
     *
     * The caller must form the IEEE 802.15.4 frame in the buffer provided by `GetTransmitBuffer()` before
     * requesting transmission.  The channel and transmit power are also included in the frame.
     *
     * @param[in] aFrame     A reference to the frame to be transmitted.
     *
     * @retval kErrorNone          Successfully transitioned to Transmit.
     * @retval kErrorInvalidState  The radio was not in the Receive state.
     */
    Error Transmit(Mac::TxFrame &aFrame);

    /**
     * Gets the most recent RSSI measurement.
     *
     * @returns The RSSI in dBm when it is valid.  127 when RSSI is invalid.
     */
    int8_t GetRssi(void);

    /**
     * Begins the energy scan sequence on the radio.
     *
     * Is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability.
     *
     * @param[in] aScanChannel   The channel to perform the energy scan on.
     * @param[in] aScanDuration  The duration, in milliseconds, for the channel to be scanned.
     *
     * @retval kErrorNone            Successfully started scanning the channel.
     * @retval kErrorBusy            The radio is performing energy scanning.
     * @retval kErrorNotImplemented  The radio doesn't support energy scanning.
     */
    Error EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration);

    /**
     * Enables/disables source address match feature.
     *
     * The source address match feature controls how the radio layer decides the "frame pending" bit for acks sent in
     * response to data request commands from children.
     *
     * If disabled, the radio layer must set the "frame pending" on all acks to data request commands.
     *
     * If enabled, the radio layer uses the source address match table to determine whether to set or clear the "frame
     * pending" bit in an ack to a data request command.
     *
     * The source address match table provides the list of children for which there is a pending frame. Either a short
     * address or an extended/long address can be added to the source address match table.
     *
     * @param[in]  aEnable     Enable/disable source address match feature.
     */
    void EnableSrcMatch(bool aEnable);

    /**
     * Adds a short address to the source address match table.
     *
     * @param[in]  aShortAddress  The short address to be added.
     *
     * @retval kErrorNone     Successfully added short address to the source match table.
     * @retval kErrorNoBufs   No available entry in the source match table.
     */
    Error AddSrcMatchShortEntry(Mac::ShortAddress aShortAddress);

    /**
     * Adds an extended address to the source address match table.
     *
     * @param[in]  aExtAddress  The extended address to be added stored in little-endian byte order.
     *
     * @retval kErrorNone     Successfully added extended address to the source match table.
     * @retval kErrorNoBufs   No available entry in the source match table.
     */
    Error AddSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress);

    /**
     * Removes a short address from the source address match table.
     *
     * @param[in]  aShortAddress  The short address to be removed.
     *
     * @retval kErrorNone       Successfully removed short address from the source match table.
     * @retval kErrorNoAddress  The short address is not in source address match table.
     */
    Error ClearSrcMatchShortEntry(Mac::ShortAddress aShortAddress);

    /**
     * Removes an extended address from the source address match table.
     *
     * @param[in]  aExtAddress  The extended address to be removed stored in little-endian byte order.
     *
     * @retval kErrorNone       Successfully removed the extended address from the source match table.
     * @retval kErrorNoAddress  The extended address is not in source address match table.
     */
    Error ClearSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress);

    /**
     * Clears all short addresses from the source address match table.
     */
    void ClearSrcMatchShortEntries(void);

    /**
     * Clears all the extended/long addresses from source address match table.
     */
    void ClearSrcMatchExtEntries(void);

    /**
     * Gets the radio supported channel mask that the device is allowed to be on.
     *
     * @returns The radio supported channel mask.
     */
    uint32_t GetSupportedChannelMask(void);

    /**
     * Gets the radio preferred channel mask that the device prefers to form on.
     *
     * @returns The radio preferred channel mask.
     */
    uint32_t GetPreferredChannelMask(void);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    /**
     * Enables/disables or updates Enhanced-ACK Based Probing in radio for a specific Initiator.
     *
     * After Enhanced-ACK Based Probing is configured by a specific Probing Initiator, the Enhanced-ACK sent to that
     * node should include Vendor-Specific IE containing Link Metrics data. This method informs the radio to
     * starts/stops to collect Link Metrics data and include Vendor-Specific IE that containing the data
     * in Enhanced-ACK sent to that Probing Initiator.
     *
     * @param[in]  aLinkMetrics  This parameter specifies what metrics to query. Per spec 4.11.3.4.4.6, at most 2
     *                           metrics can be specified. The probing would be disabled if @p `aLinkMetrics` is
     *                           bitwise 0.
     * @param[in]  aShortAddress The short address of the the probing Initiator.
     * @param[in]  aExtAddress   The extended source address of the probing Initiator.
     *
     * @retval kErrorNone            Successfully enable/disable or update Enhanced-ACK Based Probing for a specific
     *                               Initiator.
     * @retval kErrorInvalidArgs     @p aDataLength or @p aExtAddr is not valid.
     * @retval kErrorNotFound        The Initiator indicated by @p aShortAddress is not found when trying to clear.
     * @retval kErrorNoBufs          No more Initiator can be supported.
     * @retval kErrorNotImplemented  Radio driver doesn't support Enhanced-ACK Probing.
     */
    Error ConfigureEnhAckProbing(otLinkMetrics            aLinkMetrics,
                                 const Mac::ShortAddress &aShortAddress,
                                 const Mac::ExtAddress   &aExtAddress)
    {
        return otPlatRadioConfigureEnhAckProbing(GetInstancePtr(), aLinkMetrics, aShortAddress, &aExtAddress);
    }
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

    /**
     * Checks if a given channel is valid as a CSL channel.
     *
     * @retval true   The channel is valid.
     * @retval false  The channel is invalid.
     */
    static bool IsCslChannelValid(uint8_t aCslChannel)
    {
        return ((aCslChannel == 0) ||
                ((kChannelMin == aCslChannel) || ((kChannelMin < aCslChannel) && (aCslChannel <= kChannelMax))));
    }

    /**
     * Sets the region code.
     *
     * The radio region format is the 2-bytes ascii representation of the ISO 3166 alpha-2 code.
     *
     * @param[in]  aRegionCode  The radio region code. The `aRegionCode >> 8` is first ascii char
     *                          and the `aRegionCode & 0xff` is the second ascii char.
     *
     * @retval  kErrorFailed          Other platform specific errors.
     * @retval  kErrorNone            Successfully set region code.
     * @retval  kErrorNotImplemented  The feature is not implemented.
     */
    Error SetRegion(uint16_t aRegionCode) { return otPlatRadioSetRegion(GetInstancePtr(), aRegionCode); }

    /**
     * Get the region code.
     *
     * The radio region format is the 2-bytes ascii representation of the ISO 3166 alpha-2 code.
     *
     * @param[out] aRegionCode  The radio region code. The `aRegionCode >> 8` is first ascii char
     *                          and the `aRegionCode & 0xff` is the second ascii char.
     *
     * @retval  kErrorFailed          Other platform specific errors.
     * @retval  kErrorNone            Successfully set region code.
     * @retval  kErrorNotImplemented  The feature is not implemented.
     */
    Error GetRegion(uint16_t &aRegionCode) const { return otPlatRadioGetRegion(GetInstancePtr(), &aRegionCode); }

    /**
     * Indicates whether a given channel page is supported based on the current configurations.
     *
     * @param[in] aChannelPage The channel page to check.
     *
     * @retval TRUE    The @p aChannelPage is supported by radio.
     * @retval FALASE  The @p aChannelPage is not supported by radio.
     */
    static constexpr bool SupportsChannelPage(uint8_t aChannelPage)
    {
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT && OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
        return (aChannelPage == kChannelPage0) || (aChannelPage == kChannelPage2);
#elif OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
        return (aChannelPage == kChannelPage0);
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
        return (aChannelPage == kChannelPage2);
#elif OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
        return (aChannelPage == OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_PAGE);
#endif
    }

    /**
     * Returns the channel mask for a given channel page if supported by the radio.
     *
     * @param[in] aChannelPage   The channel page.
     *
     * @returns The channel mask for @p aChannelPage if page is supported by the radio, otherwise zero.
     */
    static uint32_t ChannelMaskForPage(uint8_t aChannelPage)
    {
        uint32_t mask = 0;

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
        if (aChannelPage == kChannelPage0)
        {
            mask = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK;
        }
#endif

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
        if (aChannelPage == kChannelPage1)
        {
            mask = OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK;
        }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
        if (aChannelPage == OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_PAGE)
        {
            mask = OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MASK;
        }
#endif
        return mask;
    }

private:
    otInstance *GetInstancePtr(void) const { return reinterpret_cast<otInstance *>(&InstanceLocator::GetInstance()); }

    Callbacks mCallbacks;
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    RadioStatistics mRadioStatistics;
#endif
};

//---------------------------------------------------------------------------------------------------------------------
// Radio APIs that are always mapped to the same `otPlatRadio` function (independent of the link type)

inline const char *Radio::GetVersionString(void) { return otPlatRadioGetVersionString(GetInstancePtr()); }

inline void Radio::GetIeeeEui64(Mac::ExtAddress &aIeeeEui64)
{
    otPlatRadioGetIeeeEui64(GetInstancePtr(), aIeeeEui64.m8);
}

inline uint32_t Radio::GetSupportedChannelMask(void) { return otPlatRadioGetSupportedChannelMask(GetInstancePtr()); }

inline uint32_t Radio::GetPreferredChannelMask(void) { return otPlatRadioGetPreferredChannelMask(GetInstancePtr()); }

//---------------------------------------------------------------------------------------------------------------------
// If IEEE 802.15.4 is among supported radio links, provide inline
// mapping of `Radio` method to related `otPlatRadio` functions.

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

inline otRadioCaps Radio::GetCaps(void) { return otPlatRadioGetCaps(GetInstancePtr()); }

inline int8_t Radio::GetReceiveSensitivity(void) const { return otPlatRadioGetReceiveSensitivity(GetInstancePtr()); }

inline void Radio::SetPanId(Mac::PanId aPanId) { otPlatRadioSetPanId(GetInstancePtr(), aPanId); }

inline void Radio::SetMacKey(uint8_t                 aKeyIdMode,
                             uint8_t                 aKeyId,
                             const Mac::KeyMaterial &aPrevKey,
                             const Mac::KeyMaterial &aCurrKey,
                             const Mac::KeyMaterial &aNextKey)
{
    otRadioKeyType aKeyType;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    aKeyType = OT_KEY_TYPE_KEY_REF;
#else
    aKeyType = OT_KEY_TYPE_LITERAL_KEY;
#endif

    otPlatRadioSetMacKey(GetInstancePtr(), aKeyIdMode, aKeyId, &aPrevKey, &aCurrKey, &aNextKey, aKeyType);
}

inline Error Radio::GetTransmitPower(int8_t &aPower) { return otPlatRadioGetTransmitPower(GetInstancePtr(), &aPower); }

inline Error Radio::SetTransmitPower(int8_t aPower) { return otPlatRadioSetTransmitPower(GetInstancePtr(), aPower); }

inline Error Radio::GetCcaEnergyDetectThreshold(int8_t &aThreshold)
{
    return otPlatRadioGetCcaEnergyDetectThreshold(GetInstancePtr(), &aThreshold);
}

inline Error Radio::SetCcaEnergyDetectThreshold(int8_t aThreshold)
{
    return otPlatRadioSetCcaEnergyDetectThreshold(GetInstancePtr(), aThreshold);
}

inline bool Radio::GetPromiscuous(void) { return otPlatRadioGetPromiscuous(GetInstancePtr()); }

inline void Radio::SetPromiscuous(bool aEnable) { otPlatRadioSetPromiscuous(GetInstancePtr(), aEnable); }

inline void Radio::SetRxOnWhenIdle(bool aEnable) { otPlatRadioSetRxOnWhenIdle(GetInstancePtr(), aEnable); }

inline otRadioState Radio::GetState(void) { return otPlatRadioGetState(GetInstancePtr()); }

inline Error Radio::Enable(void)
{
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    mRadioStatistics.RecordStateChange(RadioStatistics::kSleep);
#endif
    return otPlatRadioEnable(GetInstancePtr());
}

inline Error Radio::Disable(void)
{
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    mRadioStatistics.RecordStateChange(RadioStatistics::kDisabled);
#endif
    return otPlatRadioDisable(GetInstancePtr());
}

inline bool Radio::IsEnabled(void) { return otPlatRadioIsEnabled(GetInstancePtr()); }

inline Error Radio::Sleep(void)
{
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    mRadioStatistics.RecordStateChange(RadioStatistics::kSleep);
#endif
    return otPlatRadioSleep(GetInstancePtr());
}

inline Error Radio::Receive(uint8_t aChannel)
{
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    mRadioStatistics.RecordStateChange(RadioStatistics::kReceive);
#endif
    return otPlatRadioReceive(GetInstancePtr(), aChannel);
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
inline Error Radio::ReceiveAt(uint8_t aChannel, uint32_t aStart, uint32_t aDuration)
{
    Error error = otPlatRadioReceiveAt(GetInstancePtr(), aChannel, aStart, aDuration);
#if OPENTHREAD_CONFIG_RADIO_STATS_ENABLE && (OPENTHREAD_FTD || OPENTHREAD_MTD)
    if (error == kErrorNone)
    {
        mRadioStatistics.HandleReceiveAt(aDuration);
    }
#endif
    return error;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
inline void Radio::UpdateCslSampleTime(uint32_t aCslSampleTime)
{
    otPlatRadioUpdateCslSampleTime(GetInstancePtr(), aCslSampleTime);
}

inline Error Radio::EnableCsl(uint32_t aCslPeriod, otShortAddress aShortAddr, const otExtAddress *aExtAddr)
{
    return otPlatRadioEnableCsl(GetInstancePtr(), aCslPeriod, aShortAddr, aExtAddr);
}

inline Error Radio::ResetCsl(void) { return otPlatRadioResetCsl(GetInstancePtr()); }
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE || \
    OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
inline uint64_t Radio::GetNow(void) { return otPlatRadioGetNow(GetInstancePtr()); }

inline uint8_t Radio::GetCslAccuracy(void) { return otPlatRadioGetCslAccuracy(GetInstancePtr()); }

inline uint8_t Radio::GetCslUncertainty(void) { return otPlatRadioGetCslUncertainty(GetInstancePtr()); }
#endif

inline Mac::TxFrame &Radio::GetTransmitBuffer(void)
{
    return *static_cast<Mac::TxFrame *>(otPlatRadioGetTransmitBuffer(GetInstancePtr()));
}

inline int8_t Radio::GetRssi(void) { return otPlatRadioGetRssi(GetInstancePtr()); }

inline Error Radio::EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
{
    return otPlatRadioEnergyScan(GetInstancePtr(), aScanChannel, aScanDuration);
}

inline void Radio::EnableSrcMatch(bool aEnable) { otPlatRadioEnableSrcMatch(GetInstancePtr(), aEnable); }

inline Error Radio::AddSrcMatchShortEntry(Mac::ShortAddress aShortAddress)
{
    return otPlatRadioAddSrcMatchShortEntry(GetInstancePtr(), aShortAddress);
}

inline Error Radio::AddSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress)
{
    return otPlatRadioAddSrcMatchExtEntry(GetInstancePtr(), &aExtAddress);
}

inline Error Radio::ClearSrcMatchShortEntry(Mac::ShortAddress aShortAddress)
{
    return otPlatRadioClearSrcMatchShortEntry(GetInstancePtr(), aShortAddress);
}

inline Error Radio::ClearSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress)
{
    return otPlatRadioClearSrcMatchExtEntry(GetInstancePtr(), &aExtAddress);
}

inline void Radio::ClearSrcMatchShortEntries(void) { otPlatRadioClearSrcMatchShortEntries(GetInstancePtr()); }

inline void Radio::ClearSrcMatchExtEntries(void) { otPlatRadioClearSrcMatchExtEntries(GetInstancePtr()); }

#else //----------------------------------------------------------------------------------------------------------------

inline otRadioCaps Radio::GetCaps(void)
{
    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF | OT_RADIO_CAPS_TRANSMIT_RETRIES;
}

inline int8_t Radio::GetReceiveSensitivity(void) const { return kDefaultReceiveSensitivity; }

inline void Radio::SetPanId(Mac::PanId) {}

inline void Radio::SetExtendedAddress(const Mac::ExtAddress &) {}

inline void Radio::SetShortAddress(Mac::ShortAddress) {}

inline void Radio::SetMacKey(uint8_t,
                             uint8_t,
                             const Mac::KeyMaterial &,
                             const Mac::KeyMaterial &,
                             const Mac::KeyMaterial &)
{
}

inline Error Radio::GetTransmitPower(int8_t &) { return kErrorNotImplemented; }

inline Error Radio::SetTransmitPower(int8_t) { return kErrorNotImplemented; }

inline Error Radio::GetCcaEnergyDetectThreshold(int8_t &) { return kErrorNotImplemented; }

inline Error Radio::SetCcaEnergyDetectThreshold(int8_t) { return kErrorNotImplemented; }

inline bool Radio::GetPromiscuous(void) { return false; }

inline void Radio::SetPromiscuous(bool) {}

inline void Radio::SetRxOnWhenIdle(bool) {}

inline otRadioState Radio::GetState(void) { return OT_RADIO_STATE_DISABLED; }

inline Error Radio::Enable(void) { return kErrorNone; }

inline Error Radio::Disable(void) { return kErrorInvalidState; }

inline bool Radio::IsEnabled(void) { return true; }

inline Error Radio::Sleep(void) { return kErrorNone; }

inline Error Radio::Receive(uint8_t) { return kErrorNone; }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
inline Error Radio::ReceiveAt(uint8_t, uint32_t, uint32_t) { return kErrorNone; }
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
inline void Radio::UpdateCslSampleTime(uint32_t) {}

inline Error Radio::EnableCsl(uint32_t, otShortAddress aShortAddr, const otExtAddress *)
{
    return kErrorNotImplemented;
}

inline Error Radio::ResetCsl(void) { return kErrorNotImplemented; }
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE || \
    OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
inline uint64_t Radio::GetNow(void) { return NumericLimits<uint64_t>::kMax; }

inline uint8_t Radio::GetCslAccuracy(void) { return NumericLimits<uint8_t>::kMax; }

inline uint8_t Radio::GetCslUncertainty(void) { return NumericLimits<uint8_t>::kMax; }
#endif

inline Mac::TxFrame &Radio::GetTransmitBuffer(void)
{
    return *static_cast<Mac::TxFrame *>(otPlatRadioGetTransmitBuffer(GetInstancePtr()));
}

inline Error Radio::Transmit(Mac::TxFrame &) { return kErrorAbort; }

inline int8_t Radio::GetRssi(void) { return kInvalidRssi; }

inline Error Radio::EnergyScan(uint8_t, uint16_t) { return kErrorNotImplemented; }

inline void Radio::EnableSrcMatch(bool) {}

inline Error Radio::AddSrcMatchShortEntry(Mac::ShortAddress) { return kErrorNone; }

inline Error Radio::AddSrcMatchExtEntry(const Mac::ExtAddress &) { return kErrorNone; }

inline Error Radio::ClearSrcMatchShortEntry(Mac::ShortAddress) { return kErrorNone; }

inline Error Radio::ClearSrcMatchExtEntry(const Mac::ExtAddress &) { return kErrorNone; }

inline void Radio::ClearSrcMatchShortEntries(void) {}

inline void Radio::ClearSrcMatchExtEntries(void) {}

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

} // namespace ot

#endif // RADIO_HPP_
