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
 * @brief
 *  This file defines the OpenThread IEEE 802.15.4 Link Layer API.
 */

#ifndef OPENTHREAD_LINK_H_
#define OPENTHREAD_LINK_H_

#include <openthread/commissioner.h>
#include <openthread/dataset.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-link-link
 *
 * @brief
 *   This module includes functions that control link-layer configuration.
 *
 * @{
 *
 */

/**
 * This structure represents link-specific information for messages received from the Thread radio.
 *
 */
typedef struct otThreadLinkInfo
{
    uint16_t mPanId;        ///< Source PAN ID
    uint8_t  mChannel;      ///< 802.15.4 Channel
    int8_t   mRss;          ///< Received Signal Strength in dBm.
    uint8_t  mLqi;          ///< Link Quality Indicator for a received message.
    bool     mLinkSecurity; ///< Indicates whether or not link security is enabled.

    // Applicable/Required only when time sync feature (`OPENTHREAD_CONFIG_TIME_SYNC_ENABLE`) is enabled.
    uint8_t mTimeSyncSeq;       ///< The time sync sequence.
    int64_t mNetworkTimeOffset; ///< The time offset to the Thread network time, in microseconds.
} otThreadLinkInfo;

/**
 * Used to indicate no fixed received signal strength was set
 *
 */
#define OT_MAC_FILTER_FIXED_RSS_DISABLED 127

#define OT_MAC_FILTER_ITERATOR_INIT 0 ///< Initializer for otMacFilterIterator.

typedef uint8_t otMacFilterIterator; ///< Used to iterate through mac filter entries.

/**
 * Defines address mode of the mac filter.
 *
 */
typedef enum otMacFilterAddressMode
{
    OT_MAC_FILTER_ADDRESS_MODE_DISABLED,  ///< Address filter is disabled.
    OT_MAC_FILTER_ADDRESS_MODE_WHITELIST, ///< Whitelist address filter mode is enabled.
    OT_MAC_FILTER_ADDRESS_MODE_BLACKLIST, ///< Blacklist address filter mode is enabled.
} otMacFilterAddressMode;

/**
 * This structure represents a Mac Filter entry.
 *
 */
typedef struct otMacFilterEntry
{
    otExtAddress mExtAddress; ///< IEEE 802.15.4 Extended Address
    int8_t       mRssIn;      ///< Received signal strength
} otMacFilterEntry;

/**
 * This structure represents the MAC layer counters.
 *
 */
typedef struct otMacCounters
{
    /**
     * The total number of unique MAC frame transmission requests.
     *
     * Note that this counter is incremented for each MAC transmission request only by one,
     * regardless of the amount of CCA failures, CSMA-CA attempts, or retransmissions.
     *
     * This increment rule applies to the following counters:
     *   - @p mTxUnicast
     *   - @p mTxBroadcast
     *   - @p mTxAckRequested
     *   - @p mTxNoAckRequested
     *   - @p mTxData
     *   - @p mTxDataPoll
     *   - @p mTxBeacon
     *   - @p mTxBeaconRequest
     *   - @p mTxOther
     *   - @p mTxErrAbort
     *   - @p mTxErrBusyChannel
     *
     * The following equations are valid:
     *   - @p mTxTotal = @p mTxUnicast + @p mTxBroadcast
     *   - @p mTxTotal = @p mTxAckRequested + @p mTxNoAckRequested
     *   - @p mTxTotal = @p mTxData + @p mTxDataPoll + @p mTxBeacon + @p mTxBeaconRequest + @p mTxOther
     *
     */
    uint32_t mTxTotal;

    /**
     * The total number of unique unicast MAC frame transmission requests.
     *
     */
    uint32_t mTxUnicast;

    /**
     * The total number of unique broadcast MAC frame transmission requests.
     *
     */
    uint32_t mTxBroadcast;

    /**
     * The total number of unique MAC frame transmission requests with requested acknowledgment.
     *
     */
    uint32_t mTxAckRequested;

    /**
     * The total number of unique MAC frame transmission requests that were acked.
     *
     */
    uint32_t mTxAcked;

    /**
     * The total number of unique MAC frame transmission requests without requested acknowledgment.
     *
     */
    uint32_t mTxNoAckRequested;

    /**
     * The total number of unique MAC Data frame transmission requests.
     *
     */
    uint32_t mTxData;

    /**
     * The total number of unique MAC Data Poll frame transmission requests.
     *
     */
    uint32_t mTxDataPoll;

    /**
     * The total number of unique MAC Beacon frame transmission requests.
     *
     */
    uint32_t mTxBeacon;

    /**
     * The total number of unique MAC Beacon Request frame transmission requests.
     *
     */
    uint32_t mTxBeaconRequest;

    /**
     * The total number of unique other MAC frame transmission requests.
     *
     * This counter is currently used for counting out-of-band frames.
     *
     */
    uint32_t mTxOther;

    /**
     * The total number of MAC retransmission attempts.
     *
     * Note that this counter is incremented by one for each retransmission attempt that may be
     * triggered by lack of acknowledgement, CSMA/CA failure, or other type of transmission error.
     * The @p mTxRetry counter is incremented both for unicast and broadcast MAC frames.
     *
     * Modify the following configuration parameters to control the amount of retransmissions in the system:
     *
     * - OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT
     * - OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
     * - OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST
     * - OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT
     * - OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT
     *
     * Currently, this counter is invalid if the platform's radio driver capability includes
     * @ref OT_RADIO_CAPS_TRANSMIT_RETRIES.
     *
     */
    uint32_t mTxRetry;

    /**
     * The total number of unique MAC transmission packets that meet maximal retry limit for direct packets.
     *
     */
    uint32_t mTxDirectMaxRetryExpiry;

    /**
     * The total number of unique MAC transmission packets that meet maximal retry limit for indirect packets.
     *
     */
    uint32_t mTxIndirectMaxRetryExpiry;

    /**
     * The total number of CCA failures.
     *
     * The meaning of this counter can be different and it depends on the platform's radio driver capabilities.
     *
     * If @ref OT_RADIO_CAPS_CSMA_BACKOFF is enabled, this counter represents the total number of full CSMA/CA
     * failed attempts and it is incremented by one also for each retransmission (in case of a CSMA/CA fail).
     *
     * If @ref OT_RADIO_CAPS_TRANSMIT_RETRIES is enabled, this counter represents the total number of full CSMA/CA
     * failed attempts and it is incremented by one for each individual data frame request (regardless of the
     * amount of retransmissions).
     *
     */
    uint32_t mTxErrCca;

    /**
     * The total number of unique MAC transmission request failures cause by an abort error.
     *
     */
    uint32_t mTxErrAbort;

    /**
     * The total number of unique MAC transmission requests failures caused by a busy channel (a CSMA/CA fail).
     *
     */
    uint32_t mTxErrBusyChannel;

    /**
     * The total number of received frames.
     *
     * This counter counts all frames reported by the platform's radio driver, including frames
     * that were dropped, for example because of an FCS error.
     *
     */
    uint32_t mRxTotal;

    /**
     * The total number of unicast frames received.
     *
     */
    uint32_t mRxUnicast;

    /**
     * The total number of broadcast frames received.
     *
     */
    uint32_t mRxBroadcast;

    /**
     * The total number of MAC Data frames received.
     *
     */
    uint32_t mRxData;

    /**
     * The total number of MAC Data Poll frames received.
     *
     */
    uint32_t mRxDataPoll;

    /**
     * The total number of MAC Beacon frames received.
     *
     */
    uint32_t mRxBeacon;

    /**
     * The total number of MAC Beacon Request frames received.
     *
     */
    uint32_t mRxBeaconRequest;

    /**
     * The total number of other types of frames received.
     *
     */
    uint32_t mRxOther;

    /**
     * The total number of frames dropped by MAC Filter module, for example received from blacklisted node.
     *
     */
    uint32_t mRxAddressFiltered;

    /**
     * The total number of frames dropped by destination address check, for example received frame for other node.
     *
     */
    uint32_t mRxDestAddrFiltered;

    /**
     * The total number of frames dropped due to duplication, that is when the frame has been already received.
     *
     * This counter may be incremented, for example when ACK frame generated by the receiver hasn't reached
     * transmitter node which performed retransmission.
     *
     */
    uint32_t mRxDuplicated;

    /**
     * The total number of frames dropped because of missing or malformed content.
     *
     */
    uint32_t mRxErrNoFrame;

    /**
     * The total number of frames dropped due to unknown neighbor.
     *
     */
    uint32_t mRxErrUnknownNeighbor;

    /**
     * The total number of frames dropped due to invalid source address.
     *
     */
    uint32_t mRxErrInvalidSrcAddr;

    /**
     * The total number of frames dropped due to security error.
     *
     * This counter may be incremented, for example when lower than expected Frame Counter is used
     * to encrypt the frame.
     *
     */
    uint32_t mRxErrSec;

    /**
     * The total number of frames dropped due to invalid FCS.
     *
     */
    uint32_t mRxErrFcs;

    /**
     * The total number of frames dropped due to other error.
     *
     */
    uint32_t mRxErrOther;
} otMacCounters;

/**
 * This structure represents a received IEEE 802.15.4 Beacon.
 *
 */
typedef struct otActiveScanResult
{
    otExtAddress    mExtAddress;     ///< IEEE 802.15.4 Extended Address
    otNetworkName   mNetworkName;    ///< Thread Network Name
    otExtendedPanId mExtendedPanId;  ///< Thread Extended PAN ID
    otSteeringData  mSteeringData;   ///< Steering Data
    uint16_t        mPanId;          ///< IEEE 802.15.4 PAN ID
    uint16_t        mJoinerUdpPort;  ///< Joiner UDP Port
    uint8_t         mChannel;        ///< IEEE 802.15.4 Channel
    int8_t          mRssi;           ///< RSSI (dBm)
    uint8_t         mLqi;            ///< LQI
    unsigned int    mVersion : 4;    ///< Version
    bool            mIsNative : 1;   ///< Native Commissioner flag
    bool            mIsJoinable : 1; ///< Joining Permitted flag
} otActiveScanResult;

/**
 * This structure represents an energy scan result.
 *
 */
typedef struct otEnergyScanResult
{
    uint8_t mChannel; ///< IEEE 802.15.4 Channel
    int8_t  mMaxRssi; ///< The max RSSI (dBm)
} otEnergyScanResult;

/**
 * This function pointer is called during an IEEE 802.15.4 Active Scan when an IEEE 802.15.4 Beacon is received or
 * the scan completes.
 *
 * @param[in]  aResult   A valid pointer to the beacon information or NULL when the active scan completes.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otHandleActiveScanResult)(otActiveScanResult *aResult, void *aContext);

/**
 * This function starts an IEEE 802.15.4 Active Scan
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aScanChannels     A bit vector indicating which channels to scan (e.g. OT_CHANNEL_11_MASK).
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aCallback         A pointer to a function called on receiving a beacon or scan completes.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE  Accepted the Active Scan request.
 * @retval OT_ERROR_BUSY  Already performing an Active Scan.
 *
 */
otError otLinkActiveScan(otInstance *             aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aScanDuration,
                         otHandleActiveScanResult aCallback,
                         void *                   aCallbackContext);

/**
 * This function indicates whether or not an IEEE 802.15.4 Active Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Active Scan is in progress, false otherwise.
 */
bool otLinkIsActiveScanInProgress(otInstance *aInstance);

/**
 * This function pointer is called during an IEEE 802.15.4 Energy Scan when the result for a channel is ready or the
 * scan completes.
 *
 * @param[in]  aResult   A valid pointer to the energy scan result information or NULL when the energy scan completes.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otHandleEnergyScanResult)(otEnergyScanResult *aResult, void *aContext);

/**
 * This function starts an IEEE 802.15.4 Energy Scan
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aScanChannels     A bit vector indicating on which channels to perform energy scan.
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aCallback         A pointer to a function called to pass on scan result on indicate scan completion.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE  Accepted the Energy Scan request.
 * @retval OT_ERROR_BUSY  Could not start the energy scan.
 *
 */
otError otLinkEnergyScan(otInstance *             aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aScanDuration,
                         otHandleEnergyScanResult aCallback,
                         void *                   aCallbackContext);

/**
 * This function indicates whether or not an IEEE 802.15.4 Energy Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Energy Scan is in progress, false otherwise.
 *
 */
bool otLinkIsEnergyScanInProgress(otInstance *aInstance);

/**
 * This function enqueues an IEEE 802.15.4 Data Request message for transmission.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully enqueued an IEEE 802.15.4 Data Request message.
 * @retval OT_ERROR_ALREADY        An IEEE 802.15.4 Data Request message is already enqueued.
 * @retval OT_ERROR_INVALID_STATE  Device is not in rx-off-when-idle mode.
 * @retval OT_ERROR_NO_BUFS        Insufficient message buffers available.
 *
 */
otError otLinkSendDataRequest(otInstance *aInstance);

/**
 * This function indicates whether or not an IEEE 802.15.4 MAC is in the transmit state.
 *
 * MAC module is in the transmit state during CSMA/CA procedure, CCA, Data, Beacon or Data Request frame transmission
 * and receiving an ACK of a transmitted frame. MAC module is not in the transmit state during transmission of an ACK
 * frame or a Beacon Request frame.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 MAC is in the transmit state, false otherwise.
 *
 */
bool otLinkIsInTransmitState(otInstance *aInstance);

/**
 * This function enqueues an IEEE 802.15.4 out of band Frame for transmission.
 *
 * An Out of Band frame is one that was generated outside of OpenThread.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aOobFrame  A pointer to the frame to transmit.
 *
 * @retval OT_ERROR_NONE           Successfully scheduled the frame transmission.
 * @retval OT_ERROR_ALREADY        MAC layer is busy sending a previously requested frame.
 * @retval OT_ERROR_INVALID_STATE  The MAC layer is not enabled.
 * @retval OT_ERROR_INVALID_ARGS   The argument @p aOobFrame is NULL.
 *
 */
otError otLinkOutOfBandTransmitRequest(otInstance *aInstance, otRadioFrame *aOobFrame);

/**
 * Get the IEEE 802.15.4 channel.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 channel.
 *
 * @sa otLinkSetChannel
 *
 */
uint8_t otLinkGetChannel(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 channel
 *
 * This function succeeds only when Thread protocols are disabled.  A successful call to this function invalidates the
 * Active and Pending Operational Datasets in non-volatile memory.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aChannel    The IEEE 802.15.4 channel.
 *
 * @retval  OT_ERROR_NONE           Successfully set the channel.
 * @retval  OT_ERROR_INVALID_ARGS   If @p aChannel is not in the range [11, 26] or is not in the supported channel mask.
 * @retval  OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otLinkGetChannel
 *
 */
otError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel);

/**
 * Get the supported channel mask of MAC layer.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The supported channel mask as `uint32_t` with bit 0 (lsb) mapping to channel 0, bit 1 to channel 1, so on.
 *
 */
uint32_t otLinkGetSupportedChannelMask(otInstance *aInstance);

/**
 * Set the supported channel mask of MAC layer.
 *
 * This function succeeds only when Thread protocols are disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aChannelMask  The supported channel mask (bit 0 or lsb mapping to channel 0, and so on).
 *
 * @retval  OT_ERROR_NONE           Successfully set the supported channel mask.
 * @retval  OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 */
otError otLinkSetSupportedChannelMask(otInstance *aInstance, uint32_t aChannelMask);

/**
 * Get the IEEE 802.15.4 Extended Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended Address.
 *
 */
const otExtAddress *otLinkGetExtendedAddress(otInstance *aInstance);

/**
 * This function sets the IEEE 802.15.4 Extended Address.
 *
 * This function succeeds only when Thread protocols are disabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Extended Address.
 * @retval OT_ERROR_INVALID_ARGS   @p aExtAddress was NULL.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 */
otError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Get the factory-assigned IEEE EUI-64.
 *
 * @param[in]   aInstance  A pointer to the OpenThread instance.
 * @param[out]  aEui64     A pointer to where the factory-assigned IEEE EUI-64 is placed.
 *
 */
void otLinkGetFactoryAssignedIeeeEui64(otInstance *aInstance, otExtAddress *aEui64);

/**
 * Get the IEEE 802.15.4 PAN ID.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 PAN ID.
 *
 * @sa otLinkSetPanId
 *
 */
otPanId otLinkGetPanId(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 PAN ID.
 *
 * This function succeeds only when Thread protocols are disabled.  A successful call to this function also invalidates
 * the Active and Pending Operational Datasets in non-volatile memory.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aPanId       The IEEE 802.15.4 PAN ID.
 *
 * @retval OT_ERROR_NONE           Successfully set the PAN ID.
 * @retval OT_ERROR_INVALID_ARGS   If aPanId is not in the range [0, 65534].
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 *
 * @sa otLinkGetPanId
 *
 */
otError otLinkSetPanId(otInstance *aInstance, otPanId aPanId);

/**
 * Get the data poll period of sleepy end device.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns  The data poll period of sleepy end device in milliseconds.
 *
 * @sa otLinkSetPollPeriod
 *
 */
uint32_t otLinkGetPollPeriod(otInstance *aInstance);

/**
 * Set/clear user-specified/external data poll period for sleepy end device.
 *
 * @note This function updates only poll period of sleepy end device. To update child timeout the function
 *       `otThreadSetChildTimeout()` shall be called.
 *
 * @note Minimal non-zero value should be `OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD` (10ms).
 *       Or zero to clear user-specified poll period.
 *
 * @note User-specified value should be no more than the maximal value 0x3FFFFFF ((1 << 26) - 1) allowed,
 * otherwise it would be clipped by the maximal value.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aPollPeriod  data poll period in milliseconds.
 *
 * @retval OT_ERROR_NONE           Successfully set/cleared user-specified poll period.
 * @retval OT_ERROR_INVALID_ARGS   If aPollPeriod is invalid.
 *
 * @sa otLinkGetPollPeriod
 *
 */
otError otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod);

/**
 * Get the IEEE 802.15.4 Short Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Short Address.
 *
 */
otShortAddress otLinkGetShortAddress(otInstance *aInstance);

/**
 * This method returns the maximum number of frame retries during direct transmission.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The maximum number of retries during direct transmission.
 *
 */
uint8_t otLinkGetMaxFrameRetriesDirect(otInstance *aInstance);

/**
 * This method sets the maximum number of frame retries during direct transmission.
 *
 * @param[in]  aInstance               A pointer to an OpenThread instance.
 * @param[in]  aMaxFrameRetriesDirect  The maximum number of retries during direct transmission.
 *
 */
void otLinkSetMaxFrameRetriesDirect(otInstance *aInstance, uint8_t aMaxFrameRetriesDirect);

/**
 * This method returns the maximum number of frame retries during indirect transmission.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The maximum number of retries during indirect transmission.
 *
 */
uint8_t otLinkGetMaxFrameRetriesIndirect(otInstance *aInstance);

/**
 * This method sets the maximum number of frame retries during indirect transmission.
 *
 * @param[in]  aInstance                 A pointer to an OpenThread instance.
 * @param[in]  aMaxFrameRetriesIndirect  The maximum number of retries during indirect transmission.
 *
 */
void otLinkSetMaxFrameRetriesIndirect(otInstance *aInstance, uint8_t aMaxFrameRetriesIndirect);

/**
 * This function gets the address mode of MAC filter.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns  the address mode.
 *
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
otMacFilterAddressMode otLinkFilterGetAddressMode(otInstance *aInstance);

/**
 * This function sets the address mode of MAC filter.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aMode      The address mode to set.
 *
 * @retval OT_ERROR_NONE           Successfully set the address mode.
 * @retval OT_ERROR_INVALID_ARGS   @p aMode is not valid.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
otError otLinkFilterSetAddressMode(otInstance *aInstance, otMacFilterAddressMode aMode);

/**
 * This method adds an Extended Address to MAC filter.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A reference to the Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully added @p aExtAddress to MAC filter.
 * @retval OT_ERROR_ALREADY        If @p aExtAddress was already in MAC filter.
 * @retval OT_ERROR_INVALID_ARGS   If @p aExtAddress is NULL.
 * @retval OT_ERROR_NO_BUFS        No available entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
otError otLinkFilterAddAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * This method removes an Extended Address from MAC filter.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A reference to the Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully removed @p aExtAddress from MAC filter.
 * @retval OT_ERROR_INVALID_ARGS   If @p aExtAddress is NULL.
 * @retval OT_ERROR_NOT_FOUND      @p aExtAddress is not in MAC filter.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
otError otLinkFilterRemoveAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * This method clears all the Extended Addresses from MAC filter.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
void otLinkFilterClearAddresses(otInstance *aInstance);

/**
 * This method gets an in-use address filter entry.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the MAC filter iterator context. To get the first in-use address filter entry,
 *                           it should be set to OT_MAC_FILTER_ITERATOR_INIT.
 * @param[out]    aEntry     A pointer to where the information is placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved an in-use address filter entry.
 * @retval OT_ERROR_INVALID_ARGS  If @p aIterator or @p aEntry is NULL.
 * @retval OT_ERROR_NOT_FOUND     No subsequent entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
otError otLinkFilterGetNextAddress(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry);

/**
 * This method sets the received signal strength (in dBm) for the messages from the Extended Address.
 * The default received signal strength for all received messages would be set if no Extended Address is specified.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address, or NULL to set the default received signal
 *                          strength.
 * @param[in]  aRss         The received signal strength (in dBm) to set.
 *
 * @retval OT_ERROR_NONE           Successfully set @p aRss for @p aExtAddress or set the default @p aRss for all
 *                                 received messages if @p aExtAddress is NULL.
 * @retval OT_ERROR_NO_BUFS        No available entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
otError otLinkFilterAddRssIn(otInstance *aInstance, const otExtAddress *aExtAddress, int8_t aRss);

/**
 * This method removes the received signal strength setting for the received messages from the Extended Address or
 * removes the default received signal strength setting if no Extended Address is specified.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address, or NULL to reset the default received
 *                          signal strength.
 *
 * @retval OT_ERROR_NONE       Successfully removed received signal strength setting for @p aExtAddress or
 *                             removed the default received signal strength setting if @p aExtAddress is NULL.
 * @retval OT_ERROR_NOT_FOUND  @p aExtAddress is not in MAC filter if it is not NULL.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterClearRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
otError otLinkFilterRemoveRssIn(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * This method clears all the received signal strength settings.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterGetNextRssIn
 *
 */
void otLinkFilterClearRssIn(otInstance *aInstance);

/**
 * This method gets an in-use RssIn filter entry.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A reference to the MAC filter iterator context. To get the first in-use RssIn Filter entry,
 *                           it should be set to OT_MAC_FILTER_ITERATOR_INIT.
 * @param[out]    aEntry     A reference to where the information is placed. The last entry would have the extended
 *                           address as all 0xff to indicate the default received signal strength if it was set.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved an in-use RssIn Filter entry.
 * @retval OT_ERROR_INVALID_ARGS  If @p aIterator or @p aEntry is NULL.
 * @retval OT_ERROR_NOT_FOUND     No subsequent entry exists.
 *
 * @sa otLinkFilterGetAddressMode
 * @sa otLinkFilterSetAddressMode
 * @sa otLinkFilterAddAddress
 * @sa otLinkFilterRemoveAddress
 * @sa otLinkFilterClearAddresses
 * @sa otLinkFilterGetNextAddress
 * @sa otLinkFilterAddRssIn
 * @sa otLinkFilterRemoveRssIn
 * @sa otLinkFilterClearRssIn
 *
 */
otError otLinkFilterGetNextRssIn(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry);

/**
 * This method converts received signal strength to link quality.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aRss       The received signal strength value to be converted.
 *
 * @return Link quality value mapping to @p aRss.
 *
 */
uint8_t otLinkConvertRssToLinkQuality(otInstance *aInstance, int8_t aRss);

/**
 * This method converts link quality to typical received signal strength.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aLinkQuality  LinkQuality value, should be in range [0,3].
 *
 * @return Typical platform received signal strength mapping to @p aLinkQuality.
 *
 */
int8_t otLinkConvertLinkQualityToRss(otInstance *aInstance, uint8_t aLinkQuality);

/**
 * This method gets histogram of retries for a single direct packet until success.
 *
 * This function is valid when OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE configuration is enabled.
 *
 * @param[in]   aInstance          A pointer to an OpenThread instance.
 * @param[out]  aNumberOfEntries   A pointer to where the size of returned histogram array is placed.
 *
 * @returns     A pointer to the histogram of retries (in a form of an array).
 *              The n-th element indicates that the packet has been sent with n-th retry.
 */
const uint32_t *otLinkGetTxDirectRetrySuccessHistogram(otInstance *aInstance, uint8_t *aNumberOfEntries);

/**
 * This method gets histogram of retries for a single indirect packet until success.
 *
 * This function is valid when OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE configuration is enabled.
 *
 * @param[in]   aInstance          A pointer to an OpenThread instance.
 * @param[out]  aNumberOfEntries   A pointer to where the size of returned histogram array is placed.
 *
 * @returns     A pointer to the histogram of retries (in a form of an array).
 *              The n-th element indicates that the packet has been sent with n-th retry.
 *
 */
const uint32_t *otLinkGetTxIndirectRetrySuccessHistogram(otInstance *aInstance, uint8_t *aNumberOfEntries);

/**
 * This method clears histogram statistics for direct and indirect transmissions.
 *
 * This function is valid when OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE configuration is enabled.
 *
 * @param[in]   aInstance          A pointer to an OpenThread instance.
 *
 */
void otLinkResetTxRetrySuccessHistogram(otInstance *aInstance);

/**
 * Get the MAC layer counters.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the MAC layer counters.
 *
 */
const otMacCounters *otLinkGetCounters(otInstance *aInstance);

/**
 * Reset the MAC layer counters.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 */
void otLinkResetCounters(otInstance *aInstance);

/**
 * This function pointer is called when an IEEE 802.15.4 frame is received.
 *
 * @note This callback is called after FCS processing and @p aFrame may not contain the actual FCS that was received.
 *
 * @note This callback is called before IEEE 802.15.4 security processing.
 *
 * @param[in]  aFrame    A pointer to the received IEEE 802.15.4 frame.
 * @param[in]  aIsTx     Whether this frame is transmitted, not received.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otLinkPcapCallback)(const otRadioFrame *aFrame, bool aIsTx, void *aContext);

/**
 * This function registers a callback to provide received raw IEEE 802.15.4 frames.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otLinkSetPcapCallback(otInstance *aInstance, otLinkPcapCallback aPcapCallback, void *aCallbackContext);

/**
 * This function indicates whether or not promiscuous mode is enabled at the link layer.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval true   Promiscuous mode is enabled.
 * @retval false  Promiscuous mode is not enabled.
 *
 */
bool otLinkIsPromiscuous(otInstance *aInstance);

/**
 * This function enables or disables the link layer promiscuous mode.
 *
 * @note Promiscuous mode may only be enabled when the Thread interface is disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPromiscuous  true to enable promiscuous mode, or false otherwise.
 *
 * @retval OT_ERROR_NONE           Successfully enabled promiscuous mode.
 * @retval OT_ERROR_INVALID_STATE  Could not enable promiscuous mode because
 *                                 the Thread interface is enabled.
 *
 */
otError otLinkSetPromiscuous(otInstance *aInstance, bool aPromiscuous);

/**
 * This function returns the current CCA (Clear Channel Assessment) failure rate.
 *
 * The rate is maintained over a window of (roughly) last `OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW`
 * frame transmissions.
 *
 * @returns The CCA failure rate with maximum value `0xffff` corresponding to 100% failure rate.
 *
 */
uint16_t otLinkGetCcaFailureRate(otInstance *aInstance);

/**
 * This function enables or disables the link layer.
 *
 * @note The link layer may only be enabled / disabled when the Thread Interface is disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aEnable       true to enable the link layer, or false otherwise.
 *
 * @retval OT_ERROR_NONE          Successfully enabled / disabled the link layer.
 * @retval OT_ERROR_INVALID_STATE Could not disable the link layer because
 *                                the Thread interface is enabled.
 *
 */
otError otLinkSetEnabled(otInstance *aInstance, bool aEnable);

/**
 * This function indicates whether or not the link layer is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval true   Link layer is enabled.
 * @retval false  Link layer is not enabled.
 *
 */
bool otLinkIsEnabled(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_LINK_H_
