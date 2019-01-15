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

#include "common/locator.hpp"
#include "common/tasklet.hpp"
#include "common/timer.hpp"
#include "mac/channel_mask.hpp"
#include "mac/mac_filter.hpp"
#include "mac/mac_frame.hpp"
#include "mac/sub_mac.hpp"
#include "thread/key_manager.hpp"
#include "thread/link_quality.hpp"
#include "thread/topology.hpp"

namespace ot {

/**
 * @addtogroup core-mac
 *
 * @brief
 *   This module includes definitions for the IEEE 802.15.4 MAC
 *
 * @{
 *
 */

namespace Mac {

/**
 * Protocol parameters and constants.
 *
 */
enum
{
    kDataPollTimeout = 100, ///< Timeout for receiving Data Frame (milliseconds).
    kSleepDelay      = 300, ///< Max sleep delay when frame is pending (milliseconds).
    kNonceSize       = 13,  ///< Size of IEEE 802.15.4 Nonce (bytes).

    kScanDurationDefault = 300, ///< Default interval between channels (milliseconds).

    kMaxCsmaBackoffsDirect =
        OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT, ///< macMaxCsmaBackoffs for direct transmissions
    kMaxCsmaBackoffsIndirect =
        OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT, ///< macMaxCsmaBackoffs for indirect transmissions

    kMaxFrameRetriesDirect =
        OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_DIRECT, ///< macMaxFrameRetries for direct transmissions
    kMaxFrameRetriesIndirect =
        OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_INDIRECT, ///< macMaxFrameRetries for indirect transmissions

    kTxNumBcast = OPENTHREAD_CONFIG_TX_NUM_BCAST ///< Number of times each broadcast frame is transmitted
};

/**
 * This class implements the IEEE 802.15.4 MAC.
 *
 */
class Mac : public InstanceLocator, public SubMac::Callbacks
{
public:
    /**
     * This constructor initializes the MAC object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit Mac(Instance &aInstance);

    /**
     * This method gets the associated `SubMac` object.
     *
     * @returns A reference to the `SubMac` object.
     *
     */
    SubMac &GetSubMac(void) { return mSubMac; }

    /**
     * This function pointer is called on receiving an IEEE 802.15.4 Beacon during an Active Scan.
     *
     * @param[in]  aContext       A pointer to arbitrary context information.
     * @param[in]  aBeaconFrame   A pointer to the Beacon frame or NULL to indicate end of Active Scan operation.
     *
     */
    typedef void (*ActiveScanHandler)(void *aContext, Frame *aBeaconFrame);

    /**
     * This method starts an IEEE 802.15.4 Active Scan.
     *
     * @param[in]  aScanChannels  A bit vector indicating which channels to scan. Zero is mapped to all channels.
     * @param[in]  aScanDuration  The time in milliseconds to spend scanning each channel. Zero duration maps to
     *                            default value `kScanDurationDefault` = 300 ms.
     * @param[in]  aHandler       A pointer to a function that is called on receiving an IEEE 802.15.4 Beacon.
     * @param[in]  aContext       A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE  Successfully scheduled the Active Scan request.
     * @retval OT_ERROR_BUSY  Could not schedule the scan (a scan is ongoing or scheduled).
     *
     */
    otError ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext);

    /**
     * This method converts a beacon frame to an active scan result of type `otActiveScanResult`.
     *
     * @param[in]  aBeaconFrame             A pointer to a beacon frame.
     * @param[out] aResult                  A reference to `otActiveScanResult` where the result is stored.
     *
     * @retval OT_ERROR_NONE            Successfully converted the beacon into active scan result.
     * @retval OT_ERROR_INVALID_ARGS    The @a aBeaconFrame was NULL.
     * @retval OT_ERROR_PARSE           Failed parsing the beacon frame.
     *
     */
    otError ConvertBeaconToActiveScanResult(Frame *aBeaconFrame, otActiveScanResult &aResult);

    /**
     * This function pointer is called during an Energy Scan when the result for a channel is ready or the scan
     * completes.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     * @param[in]  aResult   A valid pointer to the energy scan result information or NULL when the energy scan
     *                       completes.
     *
     */
    typedef void (*EnergyScanHandler)(void *aContext, otEnergyScanResult *aResult);

    /**
     * This method starts an IEEE 802.15.4 Energy Scan.
     *
     * @param[in]  aScanChannels     A bit vector indicating on which channels to scan. Zero is mapped to all channels.
     * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel. If the duration is set to
     *                               zero, a single RSSI sample will be taken per channel.
     * @param[in]  aHandler          A pointer to a function called to pass on scan result or indicate scan completion.
     * @param[in]  aContext          A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE  Accepted the Energy Scan request.
     * @retval OT_ERROR_BUSY  Could not start the energy scan.
     *
     */
    otError EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext);

    /**
     * This method indicates the energy scan for the current channel is complete.
     *
     * @param[in]  aEnergyScanMaxRssi  The maximum RSSI encountered on the scanned channel.
     *
     */
    void EnergyScanDone(int8_t aEnergyScanMaxRssi);

    /**
     * This method indicates whether or not IEEE 802.15.4 Beacon transmissions are enabled.
     *
     * @retval TRUE if IEEE 802.15.4 Beacon transmissions are enabled, FALSE otherwise.
     *
     */
    bool IsBeaconEnabled(void) const { return mBeaconsEnabled; }

    /**
     * This method enables/disables IEEE 802.15.4 Beacon transmissions.
     *
     * @param[in]  aEnabled  TRUE to enable IEEE 802.15.4 Beacon transmissions, FALSE otherwise.
     *
     */
    void SetBeaconEnabled(bool aEnabled) { mBeaconsEnabled = aEnabled; }

    /**
     * This method indicates whether or not rx-on-when-idle is enabled.
     *
     * @retval TRUE   If rx-on-when-idle is enabled.
     * @retval FALSE  If rx-on-when-idle is not enabled.
     */
    bool GetRxOnWhenIdle(void) const { return mRxOnWhenIdle; }

    /**
     * This method sets the rx-on-when-idle mode.
     *
     * @param[in]  aRxOnWhenIdle  The rx-on-when-idle mode.
     *
     */
    void SetRxOnWhenIdle(bool aRxOnWhenIdle);

    /**
     * This method requests a new MAC frame transmission.
     *
     * @retval OT_ERROR_NONE           Frame transmission request is scheduled successfully.
     * @retval OT_ERROR_ALREADY        MAC is busy sending earlier transmission request.
     * @retval OT_ERROR_INVALID_STATE  The MAC layer is not enabled.
     *
     */
    otError SendFrameRequest(void);

    /**
     * This method requests an Out of Band frame for MAC Transmission.
     *
     * An Out of Band frame is one that was generated outside of OpenThread.
     *
     * @param[in]  aOobFrame  A pointer to the frame.
     *
     * @retval OT_ERROR_NONE           Successfully scheduled the frame transmission.
     * @retval OT_ERROR_ALREADY        MAC layer is busy sending a previously requested frame.
     * @retval OT_ERROR_INVALID_STATE  The MAC layer is not enabled.
     * @retval OT_ERROR_INVALID_ARGS   The argument @p aOobFrame is NULL.
     *
     */
    otError SendOutOfBandFrameRequest(otRadioFrame *aOobFrame);

    /**
     * This method returns a reference to the IEEE 802.15.4 Extended Address.
     *
     * @returns A pointer to the IEEE 802.15.4 Extended Address.
     *
     */
    const ExtAddress &GetExtAddress(void) const { return mSubMac.GetExtAddress(); }

    /**
     * This method sets the IEEE 802.15.4 Extended Address.
     *
     * @param[in]  aExtAddress  A reference to the IEEE 802.15.4 Extended Address.
     *
     */
    void SetExtAddress(const ExtAddress &aExtAddress) { mSubMac.SetExtAddress(aExtAddress); }

    /**
     * This method returns the IEEE 802.15.4 Short Address.
     *
     * @returns The IEEE 802.15.4 Short Address.
     *
     */
    ShortAddress GetShortAddress(void) const { return mSubMac.GetShortAddress(); }

    /**
     * This method sets the IEEE 802.15.4 Short Address.
     *
     * @param[in]  aShortAddress  The IEEE 802.15.4 Short Address.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 Short Address.
     *
     */
    otError SetShortAddress(ShortAddress aShortAddress);

    /**
     * This method returns the IEEE 802.15.4 PAN Channel.
     *
     * @returns The IEEE 802.15.4 PAN Channel.
     *
     */
    uint8_t GetPanChannel(void) const { return mPanChannel; }

    /**
     * This method sets the IEEE 802.15.4 PAN Channel.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 PAN Channel.
     *
     * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 PAN Channel.
     * @retval OT_ERROR_INVALID_ARGS   The @p aChannel is not in the supported channel mask.
     *
     */
    otError SetPanChannel(uint8_t aChannel);

    /**
     * This method returns the IEEE 802.15.4 Radio Channel.
     *
     * @returns The IEEE 802.15.4 Radio Channel.
     *
     */
    uint8_t GetRadioChannel(void) const { return mRadioChannel; }

    /**
     * This method sets the IEEE 802.15.4 Radio Channel. It can only be called after successfully calling
     * `AcquireRadioChannel()`.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 Radio Channel.
     *
     * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Radio Channel.
     * @retval OT_ERROR_INVALID_ARGS   The @p aChannel is not in the supported channel mask.
     * @retval OT_ERROR_INVALID_STATE  The acquisition ID is incorrect.
     *
     */
    otError SetRadioChannel(uint16_t aAcquisitionId, uint8_t aChannel);

    /**
     * This method acquires external ownership of the Radio channel so that future calls to `SetRadioChannel)()` will
     * succeed.
     *
     * @param[out]  aAcquisitionId  The AcquisitionId that the caller should use when calling `SetRadioChannel()`.
     *
     * @retval OT_ERROR_NONE           Successfully acquired permission to Set the Radio Channel.
     * @retval OT_ERROR_INVALID_STATE  Failed to acquire permission as the radio Channel has already been acquired.
     *
     */
    otError AcquireRadioChannel(uint16_t *aAcquisitionId);

    /**
     * This method releases external ownership of the radio Channel that was acquired with `AcquireRadioChannel()`. The
     * channel will re-adopt the PAN Channel when this API is called.
     *
     * @retval OT_ERROR_NONE  Successfully released the IEEE 802.15.4 Radio Channel.
     *
     */
    otError ReleaseRadioChannel(void);

    /**
     * This method returns the supported channel mask.
     *
     * @returns The supported channel mask.
     *
     */
    const ChannelMask &GetSupportedChannelMask(void) const { return mSupportedChannelMask; }

    /**
     * This method sets the supported channel mask
     *
     * @param[in] aMask   The supported channel mask.
     *
     */
    void SetSupportedChannelMask(const ChannelMask &aMask);

    /**
     * This method returns the IEEE 802.15.4 Network Name.
     *
     * @returns A pointer to the IEEE 802.15.4 Network Name.
     *
     */
    const char *GetNetworkName(void) const { return mNetworkName.m8; }

    /**
     * This method sets the IEEE 802.15.4 Network Name.
     *
     * @param[in]  aNetworkName  A pointer to the string. Must be null terminated.
     *
     * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Network Name.
     * @retval OT_ERROR_INVALID_ARGS   Given name is too long.
     *
     */
    otError SetNetworkName(const char *aNetworkName);

    /**
     * This method sets the IEEE 802.15.4 Network Name.
     *
     * @param[in]  aBuffer  A pointer to the char buffer containing the name. Does not need to be null terminated.
     * @param[in]  aLength  Number of chars in the buffer.
     *
     * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Network Name.
     * @retval OT_ERROR_INVALID_ARGS   Given name is too long.
     *
     */
    otError SetNetworkName(const char *aBuffer, uint8_t aLength);

    /**
     * This method returns the IEEE 802.15.4 PAN ID.
     *
     * @returns The IEEE 802.15.4 PAN ID.
     *
     */
    PanId GetPanId(void) const { return mPanId; }

    /**
     * This method sets the IEEE 802.15.4 PAN ID.
     *
     * @param[in]  aPanId  The IEEE 802.15.4 PAN ID.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 PAN ID.
     *
     */
    otError SetPanId(PanId aPanId);

    /**
     * This method returns the IEEE 802.15.4 Extended PAN ID.
     *
     * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
     *
     */
    const otExtendedPanId &GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * This method sets the IEEE 802.15.4 Extended PAN ID.
     *
     * @param[in]  aExtendedPanId  The IEEE 802.15.4 Extended PAN ID.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 Extended PAN ID.
     *
     */
    otError SetExtendedPanId(const otExtendedPanId &aExtendedPanId);

#if OPENTHREAD_ENABLE_MAC_FILTER
    /**
     * This method returns the MAC filter.
     *
     * @returns A reference to the MAC filter.
     *
     */
    Filter &GetFilter(void) { return mFilter; }
#endif // OPENTHREAD_ENABLE_MAC_FILTER

    /**
     * This method is called to handle a received frame.
     *
     * @param[in]  aFrame  A pointer to the received frame, or NULL if the receive operation was aborted.
     * @param[in]  aError  OT_ERROR_NONE when successfully received a frame,
     *                     OT_ERROR_ABORT when reception was aborted and a frame was not received.
     *
     */
    void HandleReceivedFrame(Frame *aFrame, otError aError);

    /**
     * This method records CCA status (success/failure) for a frame transmission attempt.
     *
     * @param[in] aCcaSuccess   TRUE if the CCA succeeded, FALSE otherwise.
     * @param[in] aChannel      The channel on which CCA was performed.
     *
     */
    void RecordCcaStatus(bool aCcaSuccess, uint8_t aChannel);

    /**
     * This method records the status of a frame transmission attempt, updating MAC counters.
     *
     * Unlike `HandleTransmitDone` which is called after all transmission attempts of frame to indicate final status
     * of a frame transmission request, this method is invoked on all frame transmission attempts.
     *
     * @param[in] aFrame      The transmitted frame.
     * @param[in] aAckFrame   A pointer to the ACK frame, or NULL if no ACK was received.
     * @param[in] aError      OT_ERROR_NONE when the frame was transmitted successfully,
     *                        OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
     *                        OT_ERROR_CHANNEL_ACCESS_FAILURE tx failed due to activity on the channel,
     *                        OT_ERROR_ABORT when transmission was aborted for other reasons.
     * @param[in] aRetryCount Indicates number of transmission retries for this frame.
     * @param[in] aWillRetx   Indicates whether frame will be retransmitted or not. This is applicable only
     *                        when there was an error in transmission (i.e., `aError` is not NONE).
     *
     */
    void RecordFrameTransmitStatus(const Frame &aFrame,
                                   const Frame *aAckFrame,
                                   otError      aError,
                                   uint8_t      aRetryCount,
                                   bool         aWillRetx);

    /**
     * This method is called to handle transmit events.
     *
     * @param[in]  aFrame      The frame that was transmitted.
     * @param[in]  aAckFrame   A pointer to the ACK frame, NULL if no ACK was received.
     * @param[in]  aError      OT_ERROR_NONE when the frame was transmitted successfully,
     *                         OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
     *                         OT_ERROR_CHANNEL_ACCESS_FAILURE when the tx failed due to activity on the channel,
     *                         OT_ERROR_ABORT when transmission was aborted for other reasons.
     *
     */
    void HandleTransmitDone(Frame &aFrame, Frame *aAckFrame, otError aError);

    /**
     * This method returns if an active scan is in progress.
     *
     */
    bool IsActiveScanInProgress(void);

    /**
     * This method returns if an energy scan is in progress.
     *
     */
    bool IsEnergyScanInProgress(void);

    /**
     * This method returns if the MAC layer is in transmit state.
     *
     * The MAC layer is in transmit state during CSMA/CA, CCA, transmission of Data, Beacon or Data Request frames and
     * receiving of ACK frames. The MAC layer is not in transmit state during transmission of ACK frames or Beacon
     * Requests.
     *
     */
    bool IsInTransmitState(void);

    /**
     * This method registers a callback to provide received raw IEEE 802.15.4 frames.
     *
     * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame
     * or NULL to disable the callback.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext);

    /**
     * This method indicates whether or not promiscuous mode is enabled at the link layer.
     *
     * @retval true   Promiscuous mode is enabled.
     * @retval false  Promiscuous mode is not enabled.
     *
     */
    bool IsPromiscuous(void) const { return mPromiscuous; }

    /**
     * This method enables or disables the link layer promiscuous mode.
     *
     * Promiscuous mode keeps the receiver enabled, overriding the value of mRxOnWhenIdle.
     *
     * @param[in]  aPromiscuous  true to enable promiscuous mode, or false otherwise.
     *
     */
    void SetPromiscuous(bool aPromiscuous);

    /**
     * This method resets mac counters
     *
     */
    void ResetCounters(void);

    /**
     * This method returns the MAC counter.
     *
     * @returns A reference to the MAC counter.
     *
     */
    otMacCounters &GetCounters(void) { return mCounters; }

    /**
     * This method returns the noise floor value (currently use the radio receive sensitivity value).
     *
     * @returns The noise floor value in dBm.
     *
     */
    int8_t GetNoiseFloor(void);

    /**
     * This method returns the current CCA (Clear Channel Assessment) failure rate.
     *
     * The rate is maintained over a window of (roughly) last `OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW`
     * frame transmissions.
     *
     * @returns The CCA failure rate with maximum value `0xffff` corresponding to 100% failure rate.
     *
     */
    uint16_t GetCcaFailureRate(void) const { return mCcaSuccessRateTracker.GetFailureRate(); }

    /**
     * This method Starts/Stops the Link layer. It may only be used when the Netif Interface is down.
     *
     * @param[in]  aEnable The requested State for the MAC layer. true - Start, false - Stop.
     *
     * @retval OT_ERROR_NONE   The operation succeeded or the new State equals the current State.
     *
     */
    otError SetEnabled(bool aEnable);

    /**
     * This method indicates whether or not the link layer is enabled.
     *
     * @retval true   Link layer is enabled.
     * @retval false  Link layer is not enabled.
     *
     */
    bool IsEnabled(void) { return mEnabled; }

    /**
     * This method performs AES CCM on the frame which is going to be sent.
     *
     * @param[in]  aFrame       A reference to the MAC frame buffer that is going to be sent.
     * @param[in]  aExtAddress  A pointer to the extended address, which will be used to generate nonce
     *                          for AES CCM computation.
     *
     */
    static void ProcessTransmitAesCcm(Frame &aFrame, const ExtAddress *aExtAddress);

private:
    enum
    {
        kInvalidRssiValue  = SubMac::kInvalidRssiValue,
        kMaxCcaSampleCount = OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW,
        kMaxAcquisitionId  = 0xffff,
    };

    enum Operation
    {
        kOperationIdle = 0,
        kOperationActiveScan,
        kOperationEnergyScan,
        kOperationTransmitBeacon,
        kOperationTransmitData,
        kOperationWaitingForData,
        kOperationTransmitOutOfBandFrame,
    };

    /**
     * This method processes transmit security on the frame which is going to be sent.
     *
     * This method prepares the frame, fills Mac auxiliary header, and perform AES CCM immediately in most cases
     * (depends on @p aProcessAesCcm). If aProcessAesCcm is False, it probably means that some content in the frame
     * will be updated just before transmission, so AES CCM will be performed after that (before transmission).
     *
     * @param[in]  aFrame          A reference to the MAC frame buffer which is going to be sent.
     * @param[in]  aProcessAesCcm  TRUE to perform AES CCM immediately, FALSE otherwise.
     *
     */
    void ProcessTransmitSecurity(Frame &aFrame, bool aProcessAesCcm);

    static void GenerateNonce(const ExtAddress &aAddress,
                              uint32_t          aFrameCounter,
                              uint8_t           aSecurityLevel,
                              uint8_t *         aNonce);

    otError ProcessReceiveSecurity(Frame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor);
    void    UpdateIdleMode(void);
    void    StartOperation(Operation aOperation);
    void    FinishOperation(void);
    void    PerformNextOperation(void);
    void    SendBeaconRequest(Frame &aFrame);
    void    SendBeacon(Frame &aFrame);
    bool    ShouldSendBeacon(void) const;
    void    BeginTransmit(void);
    otError HandleMacCommand(Frame &aFrame);
    Frame * GetOperationFrame(void);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);
    static void HandleOperationTask(Tasklet &aTasklet);

    void    Scan(Operation aScanOperation, uint32_t aScanChannels, uint16_t aScanDuration, void *aContext);
    otError UpdateScanChannel(void);
    void    PerformActiveScan(void);
    void    PerformEnergyScan(void);
    void    ReportEnergyScanResult(int8_t aRssi);

    void LogFrameRxFailure(const Frame *aFrame, otError aError) const;
    void LogFrameTxFailure(const Frame &aFrame, otError aError, uint8_t aRetryCount) const;
    void LogBeacon(const char *aActionText, const BeaconPayload &aBeaconPayload) const;

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    void    ProcessTimeIe(Frame &aFrame);
    uint8_t GetTimeIeOffset(Frame &aFrame);
#endif

    static const char *OperationToString(Operation aOperation);

    bool mEnabled : 1;
    bool mPendingActiveScan : 1;
    bool mPendingEnergyScan : 1;
    bool mPendingTransmitBeacon : 1;
    bool mPendingTransmitData : 1;
    bool mPendingTransmitOobFrame : 1;
    bool mPendingWaitingForData : 1;
    bool mRxOnWhenIdle : 1;
    bool mPromiscuous : 1;
    bool mBeaconsEnabled : 1;
#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    bool mShouldDelaySleep : 1;
    bool mDelayingSleep : 1;
#endif

    Operation       mOperation;
    uint8_t         mBeaconSequence;
    uint8_t         mDataSequence;
    uint8_t         mBroadcastTransmitCount;
    PanId           mPanId;
    uint8_t         mPanChannel;
    uint8_t         mRadioChannel;
    uint16_t        mRadioChannelAcquisitionId;
    ChannelMask     mSupportedChannelMask;
    otExtendedPanId mExtendedPanId;
    otNetworkName   mNetworkName;
    uint8_t         mScanChannel;
    uint16_t        mScanDuration;
    ChannelMask     mScanChannelMask;
    void *          mScanContext;
    union
    {
        ActiveScanHandler mActiveScanHandler;
        EnergyScanHandler mEnergyScanHandler;
    };

    SubMac             mSubMac;
    Tasklet            mOperationTask;
    TimerMilli         mTimer;
    Frame *            mOobFrame;
    otMacCounters      mCounters;
    uint32_t           mKeyIdMode2FrameCounter;
    SuccessRateTracker mCcaSuccessRateTracker;
    uint16_t           mCcaSampleCount;

#if OPENTHREAD_ENABLE_MAC_FILTER
    Filter mFilter;
#endif // OPENTHREAD_ENABLE_MAC_FILTER
};

/**
 * @}
 *
 */

} // namespace Mac
} // namespace ot

#endif // MAC_HPP_
