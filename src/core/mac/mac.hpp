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
 *   This file includes definitions for the IEEE 802.15.4 MAC.
 */

#ifndef MAC_HPP_
#define MAC_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/tasklet.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "mac/mac_filter.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_links.hpp"
#include "mac/mac_types.hpp"
#include "mac/sub_mac.hpp"
#include "radio/trel_link.hpp"
#include "thread/key_manager.hpp"
#include "thread/link_quality.hpp"

namespace ot {

class Neighbor;

/**
 * @addtogroup core-mac
 *
 * @brief
 *   This module includes definitions for the IEEE 802.15.4 MAC
 *
 * @{
 */

namespace Mac {

constexpr uint32_t kDataPollTimeout =
    OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT; ///< Timeout for receiving Data Frame (in msec).
constexpr uint32_t kSleepDelay = 300;        ///< Max sleep delay when frame is pending (in msec).

constexpr uint16_t kScanDurationDefault = OPENTHREAD_CONFIG_MAC_SCAN_DURATION; ///< Duration per channel (in msec).

constexpr uint8_t kMaxCsmaBackoffsDirect   = OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT;
constexpr uint8_t kMaxCsmaBackoffsIndirect = OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT;
constexpr uint8_t kMaxCsmaBackoffsCsl      = 0;

constexpr uint8_t kDefaultMaxFrameRetriesDirect   = OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT;
constexpr uint8_t kDefaultMaxFrameRetriesIndirect = OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT;
constexpr uint8_t kMaxFrameRetriesCsl             = 0;

constexpr uint8_t kTxNumBcast = OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST; ///< Num of times broadcast frame is tx.

constexpr uint16_t kMinCslIePeriod = OPENTHREAD_CONFIG_MAC_CSL_MIN_PERIOD;

constexpr uint32_t kDefaultWedListenInterval = OPENTHREAD_CONFIG_WED_LISTEN_INTERVAL;
constexpr uint32_t kDefaultWedListenDuration = OPENTHREAD_CONFIG_WED_LISTEN_DURATION;

/**
 * Defines the function pointer called on receiving an IEEE 802.15.4 Beacon during an Active Scan.
 */
typedef otHandleActiveScanResult ActiveScanHandler;

/**
 * Defines an Active Scan result.
 */
typedef otActiveScanResult ActiveScanResult;

/**
 * Defines the function pointer which is called during an Energy Scan when the scan result for a channel is
 * ready or when the scan completes.
 */
typedef otHandleEnergyScanResult EnergyScanHandler;

/**
 * Defines an Energy Scan result.
 */
typedef otEnergyScanResult EnergyScanResult;

/**
 * Implements the IEEE 802.15.4 MAC.
 */
class Mac : public InstanceLocator, private NonCopyable
{
    friend class ot::Instance;

public:
    /**
     * Initializes the MAC object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit Mac(Instance &aInstance);

    /**
     * Starts an IEEE 802.15.4 Active Scan.
     *
     * @param[in]  aScanChannels  A bit vector indicating which channels to scan. Zero is mapped to all channels.
     * @param[in]  aScanDuration  The time in milliseconds to spend scanning each channel. Zero duration maps to
     *                            default value `kScanDurationDefault` = 300 ms.
     * @param[in]  aHandler       A pointer to a function that is called on receiving an IEEE 802.15.4 Beacon.
     * @param[in]  aContext       A pointer to an arbitrary context (used when invoking `aHandler` callback).
     *
     * @retval kErrorNone  Successfully scheduled the Active Scan request.
     * @retval kErrorBusy  Could not schedule the scan (a scan is ongoing or scheduled).
     */
    Error ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext);

    /**
     * Starts an IEEE 802.15.4 Energy Scan.
     *
     * @param[in]  aScanChannels     A bit vector indicating on which channels to scan. Zero is mapped to all channels.
     * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel. If the duration is set to
     *                               zero, a single RSSI sample will be taken per channel.
     * @param[in]  aHandler          A pointer to a function called to pass on scan result or indicate scan completion.
     * @param[in]  aContext          A pointer to an arbitrary context (used when invoking @p aHandler callback).
     *
     * @retval kErrorNone  Accepted the Energy Scan request.
     * @retval kErrorBusy  Could not start the energy scan.
     */
    Error EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext);

    /**
     * Indicates the energy scan for the current channel is complete.
     *
     * @param[in]  aEnergyScanMaxRssi  The maximum RSSI encountered on the scanned channel.
     */
    void EnergyScanDone(int8_t aEnergyScanMaxRssi);

    /**
     * Indicates whether or not IEEE 802.15.4 Beacon transmissions are enabled.
     *
     * @retval TRUE   If IEEE 802.15.4 Beacon transmissions are enabled.
     * @retval FALSE  If IEEE 802.15.4 Beacon transmissions are not enabled.
     */
    bool IsBeaconEnabled(void) const { return mBeaconsEnabled; }

    /**
     * Enables/disables IEEE 802.15.4 Beacon transmissions.
     *
     * @param[in]  aEnabled  TRUE to enable IEEE 802.15.4 Beacon transmissions, FALSE otherwise.
     */
    void SetBeaconEnabled(bool aEnabled) { mBeaconsEnabled = aEnabled; }

    /**
     * Indicates whether or not rx-on-when-idle is enabled.
     *
     * @retval TRUE   If rx-on-when-idle is enabled.
     * @retval FALSE  If rx-on-when-idle is not enabled.
     */
    bool GetRxOnWhenIdle(void) const { return mRxOnWhenIdle; }

    /**
     * Sets the rx-on-when-idle mode.
     *
     * @param[in]  aRxOnWhenIdle  The rx-on-when-idle mode.
     */
    void SetRxOnWhenIdle(bool aRxOnWhenIdle);

    /**
     * Requests a direct data frame transmission.
     */
    void RequestDirectFrameTransmission(void);

#if OPENTHREAD_FTD
    /**
     * Requests an indirect data frame transmission.
     */
    void RequestIndirectFrameTransmission(void);

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    /**
     * Requests `Mac` to start a CSL tx operation after a delay of @p aDelay time.
     *
     * @param[in]  aDelay  Delay time for `Mac` to start a CSL tx, in units of milliseconds.
     */
    void RequestCslFrameTransmission(uint32_t aDelay);
#endif

#endif

    /**
     * Requests transmission of a data poll (MAC Data Request) frame.
     *
     * @retval kErrorNone          Data poll transmission request is scheduled successfully.
     * @retval kErrorInvalidState  The MAC layer is not enabled.
     */
    Error RequestDataPollTransmission(void);

    /**
     * Returns a reference to the IEEE 802.15.4 Extended Address.
     *
     * @returns A pointer to the IEEE 802.15.4 Extended Address.
     */
    const ExtAddress &GetExtAddress(void) const { return mLinks.GetExtAddress(); }

    /**
     * Sets the IEEE 802.15.4 Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the IEEE 802.15.4 Extended Address.
     */
    void SetExtAddress(const ExtAddress &aExtAddress) { mLinks.SetExtAddress(aExtAddress); }

    /**
     * Returns the IEEE 802.15.4 Short Address.
     *
     * @returns The IEEE 802.15.4 Short Address.
     */
    ShortAddress GetShortAddress(void) const { return mLinks.GetShortAddress(); }

    /**
     * Sets the IEEE 802.15.4 Short Address.
     *
     * @param[in]  aShortAddress  The IEEE 802.15.4 Short Address.
     */
    void SetShortAddress(ShortAddress aShortAddress) { mLinks.SetShortAddress(aShortAddress); }

    /**
     * Returns the IEEE 802.15.4 PAN Channel.
     *
     * @returns The IEEE 802.15.4 PAN Channel.
     */
    uint8_t GetPanChannel(void) const { return mPanChannel; }

    /**
     * Sets the IEEE 802.15.4 PAN Channel.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 PAN Channel.
     *
     * @retval kErrorNone          Successfully set the IEEE 802.15.4 PAN Channel.
     * @retval kErrorInvalidArgs   The @p aChannel is not in the supported channel mask.
     */
    Error SetPanChannel(uint8_t aChannel);

    /**
     * Sets the temporary IEEE 802.15.4 radio channel.
     *
     * Allows user to temporarily change the radio channel and use a different channel (during receive)
     * instead of the PAN channel (from `SetPanChannel()`). A call to `ClearTemporaryChannel()` would clear the
     * temporary channel and adopt the PAN channel again. The `SetTemporaryChannel()` can be used multiple times in row
     * (before a call to `ClearTemporaryChannel()`) to change the temporary channel.
     *
     * @param[in]  aChannel            A IEEE 802.15.4 channel.
     *
     * @retval kErrorNone          Successfully set the temporary channel
     * @retval kErrorInvalidArgs   The @p aChannel is not in the supported channel mask.
     */
    Error SetTemporaryChannel(uint8_t aChannel);

    /**
     * Clears the use of a previously set temporary channel and adopts the PAN channel.
     */
    void ClearTemporaryChannel(void);

    /**
     * Returns the supported channel mask.
     *
     * @returns The supported channel mask.
     */
    const ChannelMask &GetSupportedChannelMask(void) const { return mSupportedChannelMask; }

    /**
     * Sets the supported channel mask
     *
     * @param[in] aMask   The supported channel mask.
     */
    void SetSupportedChannelMask(const ChannelMask &aMask);

    /**
     * Returns the IEEE 802.15.4 PAN ID.
     *
     * @returns The IEEE 802.15.4 PAN ID.
     */
    PanId GetPanId(void) const { return mPanId; }

    /**
     * Sets the IEEE 802.15.4 PAN ID.
     *
     * @param[in]  aPanId  The IEEE 802.15.4 PAN ID.
     */
    void SetPanId(PanId aPanId);

    /**
     * Returns the maximum number of frame retries during direct transmission.
     *
     * @returns The maximum number of retries during direct transmission.
     */
    uint8_t GetMaxFrameRetriesDirect(void) const { return mMaxFrameRetriesDirect; }

    /**
     * Sets the maximum number of frame retries during direct transmission.
     *
     * @param[in]  aMaxFrameRetriesDirect  The maximum number of retries during direct transmission.
     */
    void SetMaxFrameRetriesDirect(uint8_t aMaxFrameRetriesDirect) { mMaxFrameRetriesDirect = aMaxFrameRetriesDirect; }

#if OPENTHREAD_FTD
    /**
     * Returns the maximum number of frame retries during indirect transmission.
     *
     * @returns The maximum number of retries during indirect transmission.
     */
    uint8_t GetMaxFrameRetriesIndirect(void) const { return mMaxFrameRetriesIndirect; }

    /**
     * Sets the maximum number of frame retries during indirect transmission.
     *
     * @param[in]  aMaxFrameRetriesIndirect  The maximum number of retries during indirect transmission.
     */
    void SetMaxFrameRetriesIndirect(uint8_t aMaxFrameRetriesIndirect)
    {
        mMaxFrameRetriesIndirect = aMaxFrameRetriesIndirect;
    }
#endif

    /**
     * Is called to handle a received frame.
     *
     * @param[in]  aFrame  A pointer to the received frame, or `nullptr` if the receive operation was aborted.
     * @param[in]  aError  kErrorNone when successfully received a frame,
     *                     kErrorAbort when reception was aborted and a frame was not received.
     */
    void HandleReceivedFrame(RxFrame *aFrame, Error aError);

    /**
     * Records CCA status (success/failure) for a frame transmission attempt.
     *
     * @param[in] aCcaSuccess   TRUE if the CCA succeeded, FALSE otherwise.
     * @param[in] aChannel      The channel on which CCA was performed.
     */
    void RecordCcaStatus(bool aCcaSuccess, uint8_t aChannel);

    /**
     * Records the status of a frame transmission attempt, updating MAC counters.
     *
     * Unlike `HandleTransmitDone` which is called after all transmission attempts of frame to indicate final status
     * of a frame transmission request, this method is invoked on all frame transmission attempts.
     *
     * @param[in] aFrame      The transmitted frame.
     * @param[in] aError      kErrorNone when the frame was transmitted successfully,
     *                        kErrorNoAck when the frame was transmitted but no ACK was received,
     *                        kErrorChannelAccessFailure tx failed due to activity on the channel,
     *                        kErrorAbort when transmission was aborted for other reasons.
     * @param[in] aRetryCount Indicates number of transmission retries for this frame.
     * @param[in] aWillRetx   Indicates whether frame will be retransmitted or not. This is applicable only
     *                        when there was an error in transmission (i.e., `aError` is not NONE).
     */
    void RecordFrameTransmitStatus(const TxFrame &aFrame, Error aError, uint8_t aRetryCount, bool aWillRetx);

    /**
     * Is called to handle transmit events.
     *
     * @param[in]  aFrame      The frame that was transmitted.
     * @param[in]  aAckFrame   A pointer to the ACK frame, `nullptr` if no ACK was received.
     * @param[in]  aError      kErrorNone when the frame was transmitted successfully,
     *                         kErrorNoAck when the frame was transmitted but no ACK was received,
     *                         kErrorChannelAccessFailure when the tx failed due to activity on the channel,
     *                         kErrorAbort when transmission was aborted for other reasons.
     */
    void HandleTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError);

    /**
     * Returns if an active scan is in progress.
     */
    bool IsActiveScanInProgress(void) const { return IsActiveOrPending(kOperationActiveScan); }

    /**
     * Returns if an energy scan is in progress.
     */
    bool IsEnergyScanInProgress(void) const { return IsActiveOrPending(kOperationEnergyScan); }

#if OPENTHREAD_FTD
    /**
     * Indicates whether the MAC layer is performing an indirect transmission (in middle of a tx).
     *
     * @returns TRUE if in middle of an indirect transmission, FALSE otherwise.
     */
    bool IsPerformingIndirectTransmit(void) const { return (mOperation == kOperationTransmitDataIndirect); }
#endif

    /**
     * Returns if the MAC layer is in transmit state.
     *
     * The MAC layer is in transmit state during CSMA/CA, CCA, transmission of Data, Beacon or Data Request frames and
     * receiving of ACK frames. The MAC layer is not in transmit state during transmission of ACK frames or Beacon
     * Requests.
     */
    bool IsInTransmitState(void) const;

    /**
     * Registers a callback to provide received raw IEEE 802.15.4 frames.
     *
     * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame
     *                               or `nullptr` to disable the callback.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     */
    void SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext)
    {
        mLinks.SetPcapCallback(aPcapCallback, aCallbackContext);
    }

    /**
     * Indicates whether or not promiscuous mode is enabled at the link layer.
     *
     * @retval true   Promiscuous mode is enabled.
     * @retval false  Promiscuous mode is not enabled.
     */
    bool IsPromiscuous(void) const { return mPromiscuous; }

    /**
     * Enables or disables the link layer promiscuous mode.
     *
     * Promiscuous mode keeps the receiver enabled, overriding the value of mRxOnWhenIdle.
     *
     * @param[in]  aPromiscuous  true to enable promiscuous mode, or false otherwise.
     */
    void SetPromiscuous(bool aPromiscuous);

    /**
     * Resets mac counters
     */
    void ResetCounters(void) { ClearAllBytes(mCounters); }

    /**
     * Returns the MAC counter.
     *
     * @returns A reference to the MAC counter.
     */
    otMacCounters &GetCounters(void) { return mCounters; }

#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
    /**
     * Returns the MAC retry histogram for direct transmission.
     *
     * @param[out]  aNumberOfEntries    A reference to where the size of returned histogram array is placed.
     *
     * @returns     A pointer to the histogram of retries (in a form of an array).
     *              The n-th element indicates that the packet has been sent with n-th retry.
     */
    const uint32_t *GetDirectRetrySuccessHistogram(uint8_t &aNumberOfEntries);

#if OPENTHREAD_FTD
    /**
     * Returns the MAC retry histogram for indirect transmission.
     *
     * @param[out]  aNumberOfEntries    A reference to where the size of returned histogram array is placed.
     *
     * @returns     A pointer to the histogram of retries (in a form of an array).
     *              The n-th element indicates that the packet has been sent with n-th retry.
     */
    const uint32_t *GetIndirectRetrySuccessHistogram(uint8_t &aNumberOfEntries);
#endif

    /**
     * Resets MAC retry histogram.
     */
    void ResetRetrySuccessHistogram(void);
#endif // OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE

    /**
     * Returns the noise floor value (currently use the radio receive sensitivity value).
     *
     * @returns The noise floor value in dBm.
     */
    int8_t GetNoiseFloor(void) const { return mLinks.GetNoiseFloor(); }

    /**
     * Computes the link margin for a given a received signal strength value using noise floor.
     *
     * @param[in] aRss The received signal strength in dBm.
     *
     * @returns The link margin for @p aRss in dB based on noise floor.
     */
    uint8_t ComputeLinkMargin(int8_t aRss) const;

    /**
     * Returns the current CCA (Clear Channel Assessment) failure rate.
     *
     * The rate is maintained over a window of (roughly) last `OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW`
     * frame transmissions.
     *
     * @returns The CCA failure rate with maximum value `0xffff` corresponding to 100% failure rate.
     */
    uint16_t GetCcaFailureRate(void) const { return mCcaSuccessRateTracker.GetFailureRate(); }

    /**
     * Starts/Stops the Link layer. It may only be used when the Netif Interface is down.
     *
     * @param[in]  aEnable The requested State for the MAC layer. true - Start, false - Stop.
     */
    void SetEnabled(bool aEnable);

    /**
     * Indicates whether or not the link layer is enabled.
     *
     * @retval true   Link layer is enabled.
     * @retval false  Link layer is not enabled.
     */
    bool IsEnabled(void) const { return mEnabled; }

    /**
     * Clears the Mode2Key stored in PSA ITS.
     */
    void ClearMode2Key(void) { mMode2KeyMaterial.Clear(); }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Gets the CSL channel.
     *
     * @returns CSL channel.
     */
    uint8_t GetCslChannel(void) const { return mCslChannel; }

    /**
     * Sets the CSL channel.
     *
     * @param[in]  aChannel  The CSL channel.
     */
    void SetCslChannel(uint8_t aChannel);

    /**
     * Centralizes CSL state switching conditions evaluating, configuring SubMac accordingly.
     */
    void UpdateCsl(void);

    /**
     * Gets the CSL period.
     *
     * @returns CSL period in units of 10 symbols.
     */
    uint16_t GetCslPeriod(void) const { return mCslPeriod; }

    /**
     * Gets the CSL period in milliseconds.
     *
     * If the CSL period cannot be represented exactly in milliseconds, return the rounded value to the nearest
     * millisecond.
     *
     * @returns CSL period in milliseconds.
     */
    uint32_t GetCslPeriodInMsec(void) const;

    /**
     * Sets the CSL period.
     *
     * @param[in]  aPeriod  The CSL period in 10 symbols.
     */
    void SetCslPeriod(uint16_t aPeriod);

    /**
     * This method converts a given CSL period in units of 10 symbols to microseconds.
     *
     * @param[in] aPeriodInTenSymbols   The CSL period in unit of 10 symbols.
     *
     * @returns The converted CSL period value in microseconds corresponding to @p aPeriodInTenSymbols.
     */
    static uint32_t CslPeriodToUsec(uint16_t aPeriodInTenSymbols);

    /**
     * Indicates whether CSL is started at the moment.
     *
     * @retval TRUE   If CSL is enabled.
     * @retval FALSE  If CSL is not enabled.
     */
    bool IsCslEnabled(void) const;

    /**
     * Indicates whether Link is capable of starting CSL.
     *
     * @retval TRUE   If Link is capable of starting CSL.
     * @retval FALSE  If link is not capable of starting CSL.
     */
    bool IsCslCapable(void) const;

    /**
     * Indicates whether the device is connected to a parent which supports CSL.
     *
     * @retval TRUE   If parent supports CSL.
     * @retval FALSE  If parent does not support CSL.
     */
    bool IsCslSupported(void) const;

    /**
     * Returns parent CSL accuracy (clock accuracy and uncertainty).
     *
     * @returns The parent CSL accuracy.
     */
    const CslAccuracy &GetCslParentAccuracy(void) const { return mLinks.GetSubMac().GetCslParentAccuracy(); }

    /**
     * Sets parent CSL accuracy.
     *
     * @param[in] aCslAccuracy  The parent CSL accuracy.
     */
    void SetCslParentAccuracy(const CslAccuracy &aCslAccuracy)
    {
        mLinks.GetSubMac().SetCslParentAccuracy(aCslAccuracy);
    }
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE && OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    /**
     * Enables/disables the 802.15.4 radio filter.
     *
     * When radio filter is enabled, radio is put to sleep instead of receive (to ensure device does not receive any
     * frame and/or potentially send ack). Also the frame transmission requests return immediately without sending the
     * frame over the air (return "no ack" error if ack is requested, otherwise return success).
     *
     * @param[in] aFilterEnabled    TRUE to enable radio filter, FALSE to disable.
     */
    void SetRadioFilterEnabled(bool aFilterEnabled);

    /**
     * Indicates whether the 802.15.4 radio filter is enabled or not.
     *
     * @retval TRUE   If the radio filter is enabled.
     * @retval FALSE  If the radio filter is disabled.
     */
    bool IsRadioFilterEnabled(void) const { return mLinks.GetSubMac().IsRadioFilterEnabled(); }
#endif

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
    Error SetRegion(uint16_t aRegionCode);

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
    Error GetRegion(uint16_t &aRegionCode) const;

    /**
     * Gets the wake-up channel.
     *
     * @returns wake-up channel.
     */
    uint8_t GetWakeupChannel(void) const { return mWakeupChannel; }

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    /**
     * Sets the wake-up channel.
     *
     * @param[in]  aChannel  The wake-up channel.
     *
     * @retval kErrorNone          Successfully set the wake-up channel.
     * @retval kErrorInvalidArgs   The @p aChannel is not in the supported channel mask.
     */
    Error SetWakeupChannel(uint8_t aChannel);
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    /**
     * Gets the wake-up listen parameters.
     *
     * @param[out]  aInterval  A reference to return the wake-up listen interval in microseconds.
     * @param[out]  aDuration  A reference to return the wake-up listen duration in microseconds.
     */
    void GetWakeupListenParameters(uint32_t &aInterval, uint32_t &aDuration) const;

    /**
     * Sets the wake-up listen parameters.
     *
     * The listen interval must be greater than the listen duration.
     * The listen duration must be greater or equal than `kMinWakeupListenDuration`.
     *
     * @param[in]  aInterval  The wake-up listen interval in microseconds.
     * @param[in]  aDuration  The wake-up listen duration in microseconds.
     *
     * @retval kErrorNone          Successfully set the wake-up listen parameters.
     * @retval kErrorInvalidArgs   Configured listen interval is not greater than listen duration.
     */
    Error SetWakeupListenParameters(uint32_t aInterval, uint32_t aDuration);

    /**
     * Enables/disables listening for wake-up frames.
     *
     * @param[in]  aEnable  TRUE to enable listening for wake-up frames, FALSE otherwise
     *
     * @retval kErrorNone          Successfully enabled/disabled listening for wake-up frames.
     * @retval kErrorInvalidArgs   Configured listen interval is not greater than listen duration.
     * @retval kErrorInvalidState  Could not enable/disable listening for wake-up frames.
     */
    Error SetWakeupListenEnabled(bool aEnable);

    /**
     * Returns whether listening for wake-up frames is enabled.
     *
     * @retval TRUE   If listening for wake-up frames is enabled.
     * @retval FALSE  If listening for wake-up frames is not enabled.
     */
    bool IsWakeupListenEnabled(void) const { return mWakeupListenEnabled; }
#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

private:
    static constexpr uint16_t kMaxCcaSampleCount = OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW;

    enum Operation : uint8_t
    {
        kOperationIdle = 0,
        kOperationActiveScan,
        kOperationEnergyScan,
        kOperationTransmitBeacon,
        kOperationTransmitDataDirect,
        kOperationTransmitPoll,
        kOperationWaitingForData,
#if OPENTHREAD_FTD
        kOperationTransmitDataIndirect,
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        kOperationTransmitDataCsl,
#endif
#endif
    };

#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
    struct RetryHistogram
    {
        /**
         * Histogram of number of retries for a single direct packet until success
         * [0 retry: packet count, 1 retry: packet count, 2 retry : packet count ...
         *  until max retry limit: packet count]
         *
         *  The size of the array is OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT.
         */
        uint32_t mTxDirectRetrySuccess[OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT];

        /**
         * Histogram of number of retries for a single indirect packet until success
         * [0 retry: packet count, 1 retry: packet count, 2 retry : packet count ...
         *  until max retry limit: packet count]
         *
         *  The size of the array is OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT.
         */
        uint32_t mTxIndirectRetrySuccess[OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT];
    };
#endif // OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE

    Error ProcessReceiveSecurity(RxFrame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor);
    void  ProcessTransmitSecurity(TxFrame &aFrame);
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    Error ProcessEnhAckSecurity(TxFrame &aTxFrame, RxFrame &aAckFrame);
#endif

    void     UpdateIdleMode(void);
    bool     IsPending(Operation aOperation) const { return mPendingOperations & (1U << aOperation); }
    bool     IsActiveOrPending(Operation aOperation) const;
    void     SetPending(Operation aOperation) { mPendingOperations |= (1U << aOperation); }
    void     ClearPending(Operation aOperation) { mPendingOperations &= ~(1U << aOperation); }
    void     StartOperation(Operation aOperation);
    void     FinishOperation(void);
    void     PerformNextOperation(void);
    TxFrame *PrepareBeaconRequest(void);
    TxFrame *PrepareBeacon(void);
    bool     ShouldSendBeacon(void) const;
    bool     IsJoinable(void) const;
    void     BeginTransmit(void);
    void     UpdateNeighborLinkInfo(Neighbor &aNeighbor, const RxFrame &aRxFrame);
    bool     HandleMacCommand(RxFrame &aFrame);
    void     HandleTimer(void);

    void  Scan(Operation aScanOperation, uint32_t aScanChannels, uint16_t aScanDuration);
    Error UpdateScanChannel(void);
    void  PerformActiveScan(void);
    void  ReportActiveScanResult(const RxFrame *aBeaconFrame);
    Error ConvertBeaconToActiveScanResult(const RxFrame *aBeaconFrame, ActiveScanResult &aResult);
    void  PerformEnergyScan(void);
    void  ReportEnergyScanResult(int8_t aRssi);

    void LogFrameRxFailure(const RxFrame *aFrame, Error aError) const;
    void LogFrameTxFailure(const TxFrame &aFrame, Error aError, uint8_t aRetryCount, bool aWillRetx) const;
    void LogBeacon(const char *aActionText) const;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    uint8_t GetTimeIeOffset(const Frame &aFrame);
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    void ProcessCsl(const RxFrame &aFrame, const Address &aSrcAddr);
#endif
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
    void ProcessEnhAckProbing(const RxFrame &aFrame, const Neighbor &aNeighbor);
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    Error HandleWakeupFrame(const RxFrame &aFrame);
    void  UpdateWakeupListening(void);
#endif
    static const char *OperationToString(Operation aOperation);

    using OperationTask = TaskletIn<Mac, &Mac::PerformNextOperation>;
    using MacTimer      = TimerMilliIn<Mac, &Mac::HandleTimer>;

    static const otExtAddress sMode2ExtAddress;

    bool mEnabled : 1;
    bool mShouldTxPollBeforeData : 1;
    bool mRxOnWhenIdle : 1;
    bool mPromiscuous : 1;
    bool mBeaconsEnabled : 1;
    bool mUsingTemporaryChannel : 1;
#if OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
    bool mShouldDelaySleep : 1;
    bool mDelayingSleep : 1;
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    bool mWakeupListenEnabled : 1;
#endif
    Operation   mOperation;
    uint16_t    mPendingOperations;
    uint8_t     mBeaconSequence;
    uint8_t     mDataSequence;
    uint8_t     mBroadcastTransmitCount;
    PanId       mPanId;
    uint8_t     mPanChannel;
    uint8_t     mRadioChannel;
    ChannelMask mSupportedChannelMask;
    uint8_t     mScanChannel;
    uint16_t    mScanDuration;
    ChannelMask mScanChannelMask;
    uint8_t     mMaxFrameRetriesDirect;
#if OPENTHREAD_FTD
    uint8_t mMaxFrameRetriesIndirect;
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    TimeMilli mCslTxFireTime;
#endif
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    // When Mac::mCslChannel is 0, it indicates that CSL channel has not been specified by the upper layer.
    uint8_t  mCslChannel;
    uint16_t mCslPeriod;
#endif
    uint8_t mWakeupChannel;
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    uint32_t mWakeupListenInterval;
    uint32_t mWakeupListenDuration;
#endif
    union
    {
        ActiveScanHandler mActiveScanHandler;
        EnergyScanHandler mEnergyScanHandler;
    };

    void *mScanHandlerContext;

    Links              mLinks;
    OperationTask      mOperationTask;
    MacTimer           mTimer;
    otMacCounters      mCounters;
    uint32_t           mKeyIdMode2FrameCounter;
    SuccessRateTracker mCcaSuccessRateTracker;
    uint16_t           mCcaSampleCount;
#if OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
    RetryHistogram mRetryHistogram;
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    RadioTypes mTxPendingRadioLinks;
    RadioTypes mTxBeaconRadioLinks;
    Error      mTxError;
#endif

#if OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
    Filter mFilter;
#endif

    KeyMaterial mMode2KeyMaterial;
};

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // MAC_HPP_
