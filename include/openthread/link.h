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
 */
#define OT_US_PER_TEN_SYMBOLS OT_RADIO_TEN_SYMBOLS_TIME ///< Time for 10 symbols in units of microseconds

/**
 * Used to indicate no fixed received signal strength was set
 */
#define OT_MAC_FILTER_FIXED_RSS_DISABLED 127

#define OT_MAC_FILTER_ITERATOR_INIT 0 ///< Initializer for otMacFilterIterator.

typedef uint8_t otMacFilterIterator; ///< Used to iterate through mac filter entries.

/**
 * Defines address mode of the mac filter.
 */
typedef enum otMacFilterAddressMode
{
    OT_MAC_FILTER_ADDRESS_MODE_DISABLED,  ///< Address filter is disabled.
    OT_MAC_FILTER_ADDRESS_MODE_ALLOWLIST, ///< Allowlist address filter mode is enabled.
    OT_MAC_FILTER_ADDRESS_MODE_DENYLIST,  ///< Denylist address filter mode is enabled.
} otMacFilterAddressMode;

/**
 * Represents a Mac Filter entry.
 */
typedef struct otMacFilterEntry
{
    otExtAddress mExtAddress; ///< IEEE 802.15.4 Extended Address
    int8_t       mRssIn;      ///< Received signal strength
} otMacFilterEntry;

/**
 * Represents the MAC layer counters.
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
     */
    uint32_t mTxTotal;

    /**
     * The total number of unique unicast MAC frame transmission requests.
     */
    uint32_t mTxUnicast;

    /**
     * The total number of unique broadcast MAC frame transmission requests.
     */
    uint32_t mTxBroadcast;

    /**
     * The total number of unique MAC frame transmission requests with requested acknowledgment.
     */
    uint32_t mTxAckRequested;

    /**
     * The total number of unique MAC frame transmission requests that were acked.
     */
    uint32_t mTxAcked;

    /**
     * The total number of unique MAC frame transmission requests without requested acknowledgment.
     */
    uint32_t mTxNoAckRequested;

    /**
     * The total number of unique MAC Data frame transmission requests.
     */
    uint32_t mTxData;

    /**
     * The total number of unique MAC Data Poll frame transmission requests.
     */
    uint32_t mTxDataPoll;

    /**
     * The total number of unique MAC Beacon frame transmission requests.
     */
    uint32_t mTxBeacon;

    /**
     * The total number of unique MAC Beacon Request frame transmission requests.
     */
    uint32_t mTxBeaconRequest;

    /**
     * The total number of unique other MAC frame transmission requests.
     *
     * This counter is currently used for counting out-of-band frames.
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
     */
    uint32_t mTxRetry;

    /**
     * The total number of unique MAC transmission packets that meet maximal retry limit for direct packets.
     */
    uint32_t mTxDirectMaxRetryExpiry;

    /**
     * The total number of unique MAC transmission packets that meet maximal retry limit for indirect packets.
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
     */
    uint32_t mTxErrCca;

    /**
     * The total number of unique MAC transmission request failures cause by an abort error.
     */
    uint32_t mTxErrAbort;

    /**
     * The total number of unique MAC transmission requests failures caused by a busy channel (a CSMA/CA fail).
     */
    uint32_t mTxErrBusyChannel;

    /**
     * The total number of received frames.
     *
     * This counter counts all frames reported by the platform's radio driver, including frames
     * that were dropped, for example because of an FCS error.
     */
    uint32_t mRxTotal;

    /**
     * The total number of unicast frames received.
     */
    uint32_t mRxUnicast;

    /**
     * The total number of broadcast frames received.
     */
    uint32_t mRxBroadcast;

    /**
     * The total number of MAC Data frames received.
     */
    uint32_t mRxData;

    /**
     * The total number of MAC Data Poll frames received.
     */
    uint32_t mRxDataPoll;

    /**
     * The total number of MAC Beacon frames received.
     */
    uint32_t mRxBeacon;

    /**
     * The total number of MAC Beacon Request frames received.
     */
    uint32_t mRxBeaconRequest;

    /**
     * The total number of other types of frames received.
     */
    uint32_t mRxOther;

    /**
     * The total number of frames dropped by MAC Filter module, for example received from denylisted node.
     */
    uint32_t mRxAddressFiltered;

    /**
     * The total number of frames dropped by destination address check, for example received frame for other node.
     */
    uint32_t mRxDestAddrFiltered;

    /**
     * The total number of frames dropped due to duplication, that is when the frame has been already received.
     *
     * This counter may be incremented, for example when ACK frame generated by the receiver hasn't reached
     * transmitter node which performed retransmission.
     */
    uint32_t mRxDuplicated;

    /**
     * The total number of frames dropped because of missing or malformed content.
     */
    uint32_t mRxErrNoFrame;

    /**
     * The total number of frames dropped due to unknown neighbor.
     */
    uint32_t mRxErrUnknownNeighbor;

    /**
     * The total number of frames dropped due to invalid source address.
     */
    uint32_t mRxErrInvalidSrcAddr;

    /**
     * The total number of frames dropped due to security error.
     *
     * This counter may be incremented, for example when lower than expected Frame Counter is used
     * to encrypt the frame.
     */
    uint32_t mRxErrSec;

    /**
     * The total number of frames dropped due to invalid FCS.
     */
    uint32_t mRxErrFcs;

    /**
     * The total number of frames dropped due to other error.
     */
    uint32_t mRxErrOther;
} otMacCounters;

/**
 * Represents a received IEEE 802.15.4 Beacon.
 */
typedef struct otActiveScanResult
{
    otExtAddress    mExtAddress;    ///< IEEE 802.15.4 Extended Address
    otNetworkName   mNetworkName;   ///< Thread Network Name
    otExtendedPanId mExtendedPanId; ///< Thread Extended PAN ID
    otSteeringData  mSteeringData;  ///< Steering Data
    uint16_t        mPanId;         ///< IEEE 802.15.4 PAN ID
    uint16_t        mJoinerUdpPort; ///< Joiner UDP Port
    uint8_t         mChannel;       ///< IEEE 802.15.4 Channel
    int8_t          mRssi;          ///< RSSI (dBm)
    uint8_t         mLqi;           ///< LQI
    unsigned int    mVersion : 4;   ///< Version
    bool            mIsNative : 1;  ///< Native Commissioner flag
    bool            mDiscover : 1;  ///< Result from MLE Discovery

    // Applicable/Required only when beacon payload parsing feature
    // (`OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE`) is enabled.
    bool mIsJoinable : 1; ///< Joining Permitted flag
} otActiveScanResult;

/**
 * Represents an energy scan result.
 */
typedef struct otEnergyScanResult
{
    uint8_t mChannel; ///< IEEE 802.15.4 Channel
    int8_t  mMaxRssi; ///< The max RSSI (dBm)
} otEnergyScanResult;

/**
 * Pointer is called during an IEEE 802.15.4 Active Scan when an IEEE 802.15.4 Beacon is received or
 * the scan completes.
 *
 * @param[in]  aResult   A valid pointer to the beacon information or NULL when the active scan completes.
 * @param[in]  aContext  A pointer to application-specific context.
 */
typedef void (*otHandleActiveScanResult)(otActiveScanResult *aResult, void *aContext);

/**
 * Starts an IEEE 802.15.4 Active Scan
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aScanChannels     A bit vector indicating which channels to scan (e.g. OT_CHANNEL_11_MASK).
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aCallback         A pointer to a function called on receiving a beacon or scan completes.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE  Accepted the Active Scan request.
 * @retval OT_ERROR_BUSY  Already performing an Active Scan.
 */
otError otLinkActiveScan(otInstance              *aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aScanDuration,
                         otHandleActiveScanResult aCallback,
                         void                    *aCallbackContext);

/**
 * Indicates whether or not an IEEE 802.15.4 Active Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Active Scan is in progress, false otherwise.
 */
bool otLinkIsActiveScanInProgress(otInstance *aInstance);

/**
 * Pointer is called during an IEEE 802.15.4 Energy Scan when the result for a channel is ready or the
 * scan completes.
 *
 * @param[in]  aResult   A valid pointer to the energy scan result information or NULL when the energy scan completes.
 * @param[in]  aContext  A pointer to application-specific context.
 */
typedef void (*otHandleEnergyScanResult)(otEnergyScanResult *aResult, void *aContext);

/**
 * Starts an IEEE 802.15.4 Energy Scan
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aScanChannels     A bit vector indicating on which channels to perform energy scan.
 * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
 * @param[in]  aCallback         A pointer to a function called to pass on scan result on indicate scan completion.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE  Accepted the Energy Scan request.
 * @retval OT_ERROR_BUSY  Could not start the energy scan.
 */
otError otLinkEnergyScan(otInstance              *aInstance,
                         uint32_t                 aScanChannels,
                         uint16_t                 aScanDuration,
                         otHandleEnergyScanResult aCallback,
                         void                    *aCallbackContext);

/**
 * Indicates whether or not an IEEE 802.15.4 Energy Scan is currently in progress.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 Energy Scan is in progress, false otherwise.
 */
bool otLinkIsEnergyScanInProgress(otInstance *aInstance);

/**
 * Enqueues an IEEE 802.15.4 Data Request message for transmission.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully enqueued an IEEE 802.15.4 Data Request message.
 * @retval OT_ERROR_INVALID_STATE  Device is not in rx-off-when-idle mode.
 * @retval OT_ERROR_NO_BUFS        Insufficient message buffers available.
 */
otError otLinkSendDataRequest(otInstance *aInstance);

/**
 * Indicates whether or not an IEEE 802.15.4 MAC is in the transmit state.
 *
 * MAC module is in the transmit state during CSMA/CA procedure, CCA, Data, Beacon or Data Request frame transmission
 * and receiving an ACK of a transmitted frame. MAC module is not in the transmit state during transmission of an ACK
 * frame or a Beacon Request frame.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns true if an IEEE 802.15.4 MAC is in the transmit state, false otherwise.
 */
bool otLinkIsInTransmitState(otInstance *aInstance);

/**
 * Get the IEEE 802.15.4 channel.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 channel.
 *
 * @sa otLinkSetChannel
 */
uint8_t otLinkGetChannel(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 channel
 *
 * Succeeds only when Thread protocols are disabled.  A successful call to this function invalidates the
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
 */
otError otLinkSetChannel(otInstance *aInstance, uint8_t aChannel);

/**
 * Get the supported channel mask of MAC layer.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @returns The supported channel mask as `uint32_t` with bit 0 (lsb) mapping to channel 0, bit 1 to channel 1, so on.
 */
uint32_t otLinkGetSupportedChannelMask(otInstance *aInstance);

/**
 * Set the supported channel mask of MAC layer.
 *
 * Succeeds only when Thread protocols are disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aChannelMask  The supported channel mask (bit 0 or lsb mapping to channel 0, and so on).
 *
 * @retval  OT_ERROR_NONE           Successfully set the supported channel mask.
 * @retval  OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 */
otError otLinkSetSupportedChannelMask(otInstance *aInstance, uint32_t aChannelMask);

/**
 * Gets the IEEE 802.15.4 Extended Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the IEEE 802.15.4 Extended Address.
 */
const otExtAddress *otLinkGetExtendedAddress(otInstance *aInstance);

/**
 * Sets the IEEE 802.15.4 Extended Address.
 *
 * @note Only succeeds when Thread protocols are disabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval OT_ERROR_NONE           Successfully set the IEEE 802.15.4 Extended Address.
 * @retval OT_ERROR_INVALID_ARGS   @p aExtAddress was NULL.
 * @retval OT_ERROR_INVALID_STATE  Thread protocols are enabled.
 */
otError otLinkSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Get the factory-assigned IEEE EUI-64.
 *
 * @param[in]   aInstance  A pointer to the OpenThread instance.
 * @param[out]  aEui64     A pointer to where the factory-assigned IEEE EUI-64 is placed.
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
 */
otPanId otLinkGetPanId(otInstance *aInstance);

/**
 * Set the IEEE 802.15.4 PAN ID.
 *
 * Succeeds only when Thread protocols are disabled.  A successful call to this function also invalidates
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
 */
otError otLinkSetPollPeriod(otInstance *aInstance, uint32_t aPollPeriod);

/**
 * Get the IEEE 802.15.4 Short Address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The IEEE 802.15.4 Short Address.
 */
otShortAddress otLinkGetShortAddress(otInstance *aInstance);

/**
 * Get the IEEE 802.15.4 alternate short address.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The alternate short address, or `OT_RADIO_INVALID_SHORT_ADDR` (0xfffe) if there is no alternate address.
 */
otShortAddress otLinkGetAlternateShortAddress(otInstance *aInstance);

/**
 * Returns the maximum number of frame retries during direct transmission.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The maximum number of retries during direct transmission.
 */
uint8_t otLinkGetMaxFrameRetriesDirect(otInstance *aInstance);

/**
 * Sets the maximum number of frame retries during direct transmission.
 *
 * @param[in]  aInstance               A pointer to an OpenThread instance.
 * @param[in]  aMaxFrameRetriesDirect  The maximum number of retries during direct transmission.
 */
void otLinkSetMaxFrameRetriesDirect(otInstance *aInstance, uint8_t aMaxFrameRetriesDirect);

/**
 * Returns the maximum number of frame retries during indirect transmission.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The maximum number of retries during indirect transmission.
 */
uint8_t otLinkGetMaxFrameRetriesIndirect(otInstance *aInstance);

/**
 * Sets the maximum number of frame retries during indirect transmission.
 *
 * @param[in]  aInstance                 A pointer to an OpenThread instance.
 * @param[in]  aMaxFrameRetriesIndirect  The maximum number of retries during indirect transmission.
 */
void otLinkSetMaxFrameRetriesIndirect(otInstance *aInstance, uint8_t aMaxFrameRetriesIndirect);

/**
 * Gets the current MAC frame counter value.
 *
 * @param[in] aInstance    A pointer to the OpenThread instance.
 *
 * @returns The current MAC frame counter value.
 */
uint32_t otLinkGetFrameCounter(otInstance *aInstance);

/**
 * Gets the address mode of MAC filter.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns  the address mode.
 */
otMacFilterAddressMode otLinkFilterGetAddressMode(otInstance *aInstance);

/**
 * Sets the address mode of MAC filter.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aMode      The address mode to set.
 */
void otLinkFilterSetAddressMode(otInstance *aInstance, otMacFilterAddressMode aMode);

/**
 * Adds an Extended Address to MAC filter.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the Extended Address (MUST NOT be NULL).
 *
 * @retval OT_ERROR_NONE           Successfully added @p aExtAddress to MAC filter.
 * @retval OT_ERROR_NO_BUFS        No available entry exists.
 */
otError otLinkFilterAddAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Removes an Extended Address from MAC filter.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * No action is performed if there is no existing entry in Filter matching the given Extended Address.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the Extended Address (MUST NOT be NULL).
 */
void otLinkFilterRemoveAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Clears all the Extended Addresses from MAC filter.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
void otLinkFilterClearAddresses(otInstance *aInstance);

/**
 * Gets an in-use address filter entry.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]      aInstance  A pointer to an OpenThread instance.
 * @param[in,out]  aIterator  A pointer to the MAC filter iterator context. To get the first in-use address filter
 *                            entry, it should be set to OT_MAC_FILTER_ITERATOR_INIT. MUST NOT be NULL.
 * @param[out]     aEntry     A pointer to where the information is placed. MUST NOT be NULL.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved an in-use address filter entry.
 * @retval OT_ERROR_NOT_FOUND     No subsequent entry exists.
 */
otError otLinkFilterGetNextAddress(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry);

/**
 * Adds the specified Extended Address to the `RssIn` list (or modifies an existing
 * address in the `RssIn` list) and sets the received signal strength (in dBm) entry
 * for messages from that address. The Extended Address does not necessarily have
 * to be in the `address allowlist/denylist` filter to set the `rss`.
 * @note The `RssIn` list contains Extended Addresses whose `rss` or link quality indicator (`lqi`)
 * values have been set to be different from the defaults.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address. MUST NOT be NULL.
 * @param[in]  aRss         A received signal strength (in dBm).
 *
 * @retval OT_ERROR_NONE           Successfully added an entry for @p aExtAddress and @p aRss.
 * @retval OT_ERROR_NO_BUFS        No available entry exists.
 */
otError otLinkFilterAddRssIn(otInstance *aInstance, const otExtAddress *aExtAddress, int8_t aRss);

/**
 * Removes the specified Extended Address from the `RssIn` list. Once removed
 * from the `RssIn` list, this MAC address will instead use the default `rss`
 * and `lqi` settings, assuming defaults have been set.
 * (If no defaults have been set, the over-air signal is used.)
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * No action is performed if there is no existing entry in the `RssIn` list matching the specified Extended Address.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aExtAddress  A pointer to the IEEE 802.15.4 Extended Address. MUST NOT be NULL.
 */
void otLinkFilterRemoveRssIn(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Sets the default received signal strength (in dBm) on MAC Filter.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * The default RSS value is used for all received frames from addresses for which there is no explicit RSS-IN entry
 * in the Filter list (added using `otLinkFilterAddRssIn()`).
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aRss         The default received signal strength (in dBm) to set.
 */
void otLinkFilterSetDefaultRssIn(otInstance *aInstance, int8_t aRss);

/**
 * Clears any previously set default received signal strength (in dBm) on MAC Filter.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 */
void otLinkFilterClearDefaultRssIn(otInstance *aInstance);

/**
 * Clears all the received signal strength (`rss`) and link quality
 * indicator (`lqi`) entries (including defaults) from the `RssIn` list.
 * Performing this action means that all Extended Addresses will use the on-air signal.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 */
void otLinkFilterClearAllRssIn(otInstance *aInstance);

/**
 * Gets an in-use RssIn filter entry.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @param[in]      aInstance  A pointer to an OpenThread instance.
 * @param[in,out]  aIterator  A pointer to the MAC filter iterator context. MUST NOT be NULL.
 *                            To get the first entry, it should be set to OT_MAC_FILTER_ITERATOR_INIT.
 * @param[out]     aEntry     A pointer to where the information is placed. The last entry would have the extended
 *                            address as all 0xff to indicate the default received signal strength if it was set.
                              @p aEntry MUST NOT be NULL.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the next entry.
 * @retval OT_ERROR_NOT_FOUND     No subsequent entry exists.
 */
otError otLinkFilterGetNextRssIn(otInstance *aInstance, otMacFilterIterator *aIterator, otMacFilterEntry *aEntry);

/**
 * Enables/disables IEEE 802.15.4 radio filter mode.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * The radio filter is mainly intended for testing. It can be used to temporarily block all tx/rx on the 802.15.4 radio.
 * When radio filter is enabled, radio is put to sleep instead of receive (to ensure device does not receive any frame
 * and/or potentially send ack). Also the frame transmission requests return immediately without sending the frame over
 * the air (return "no ack" error if ack is requested, otherwise return success).
 *
 * @param[in] aInstance         A pointer to an OpenThread instance.
 * @param[in] aFilterEnabled    TRUE to enable radio filter, FALSE to disable
 */
void otLinkSetRadioFilterEnabled(otInstance *aInstance, bool aFilterEnabled);

/**
 * Indicates whether the IEEE 802.15.4 radio filter is enabled or not.
 *
 * Is available when `OPENTHREAD_CONFIG_MAC_FILTER_ENABLE` configuration is enabled.
 *
 * @retval TRUE   If the radio filter is enabled.
 * @retval FALSE  If the radio filter is disabled.
 */
bool otLinkIsRadioFilterEnabled(otInstance *aInstance);

/**
 * Converts received signal strength to link quality.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aRss       The received signal strength value to be converted.
 *
 * @return Link quality value mapping to @p aRss.
 */
uint8_t otLinkConvertRssToLinkQuality(otInstance *aInstance, int8_t aRss);

/**
 * Converts link quality to typical received signal strength.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aLinkQuality  LinkQuality value, should be in range [0,3].
 *
 * @return Typical platform received signal strength mapping to @p aLinkQuality.
 */
int8_t otLinkConvertLinkQualityToRss(otInstance *aInstance, uint8_t aLinkQuality);

/**
 * Gets histogram of retries for a single direct packet until success.
 *
 * Is valid when OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE configuration is enabled.
 *
 * @param[in]   aInstance          A pointer to an OpenThread instance.
 * @param[out]  aNumberOfEntries   A pointer to where the size of returned histogram array is placed.
 *
 * @returns     A pointer to the histogram of retries (in a form of an array).
 *              The n-th element indicates that the packet has been sent with n-th retry.
 */
const uint32_t *otLinkGetTxDirectRetrySuccessHistogram(otInstance *aInstance, uint8_t *aNumberOfEntries);

/**
 * Gets histogram of retries for a single indirect packet until success.
 *
 * Is valid when OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE configuration is enabled.
 *
 * @param[in]   aInstance          A pointer to an OpenThread instance.
 * @param[out]  aNumberOfEntries   A pointer to where the size of returned histogram array is placed.
 *
 * @returns     A pointer to the histogram of retries (in a form of an array).
 *              The n-th element indicates that the packet has been sent with n-th retry.
 */
const uint32_t *otLinkGetTxIndirectRetrySuccessHistogram(otInstance *aInstance, uint8_t *aNumberOfEntries);

/**
 * Clears histogram statistics for direct and indirect transmissions.
 *
 * Is valid when OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE configuration is enabled.
 *
 * @param[in]   aInstance          A pointer to an OpenThread instance.
 */
void otLinkResetTxRetrySuccessHistogram(otInstance *aInstance);

/**
 * Get the MAC layer counters.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns A pointer to the MAC layer counters.
 */
const otMacCounters *otLinkGetCounters(otInstance *aInstance);

/**
 * Resets the MAC layer counters.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 */
void otLinkResetCounters(otInstance *aInstance);

/**
 * Pointer is called when an IEEE 802.15.4 frame is received.
 *
 * @note This callback is called after FCS processing and @p aFrame may not contain the actual FCS that was received.
 *
 * @note This callback is called before IEEE 802.15.4 security processing.
 *
 * @param[in]  aFrame    A pointer to the received IEEE 802.15.4 frame.
 * @param[in]  aIsTx     Whether this frame is transmitted, not received.
 * @param[in]  aContext  A pointer to application-specific context.
 */
typedef void (*otLinkPcapCallback)(const otRadioFrame *aFrame, bool aIsTx, void *aContext);

/**
 * Registers a callback to provide received raw IEEE 802.15.4 frames.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 */
void otLinkSetPcapCallback(otInstance *aInstance, otLinkPcapCallback aPcapCallback, void *aCallbackContext);

/**
 * Indicates whether or not promiscuous mode is enabled at the link layer.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   Promiscuous mode is enabled.
 * @retval FALSE  Promiscuous mode is not enabled.
 */
bool otLinkIsPromiscuous(otInstance *aInstance);

/**
 * Enables or disables the link layer promiscuous mode.
 *
 * @note Promiscuous mode may only be enabled when the Thread interface is disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPromiscuous  true to enable promiscuous mode, or false otherwise.
 *
 * @retval OT_ERROR_NONE           Successfully enabled promiscuous mode.
 * @retval OT_ERROR_INVALID_STATE  Could not enable promiscuous mode because
 *                                 the Thread interface is enabled.
 */
otError otLinkSetPromiscuous(otInstance *aInstance, bool aPromiscuous);

/**
 * Gets the CSL channel.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 *
 * @returns The CSL channel.
 */
uint8_t otLinkGetCslChannel(otInstance *aInstance);

/**
 * Sets the CSL channel.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aChannel       The CSL sample channel. Channel value should be `0` (Set CSL Channel unspecified) or
 *                            within the range [1, 10] (if 915-MHz supported) and [11, 26] (if 2.4 GHz supported).
 *
 * @retval OT_ERROR_NONE           Successfully set the CSL parameters.
 * @retval OT_ERROR_INVALID_ARGS   Invalid @p aChannel.
 */
otError otLinkSetCslChannel(otInstance *aInstance, uint8_t aChannel);

/**
 * Represents CSL period ten symbols unit in microseconds.
 *
 * The CSL period (in micro seconds) MUST be a multiple of this value.
 */
#define OT_LINK_CSL_PERIOD_TEN_SYMBOLS_UNIT_IN_USEC (160)

/**
 * Gets the CSL period in microseconds
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 *
 * @returns The CSL period in microseconds.
 */
uint32_t otLinkGetCslPeriod(otInstance *aInstance);

/**
 * Sets the CSL period in microseconds. Disable CSL by setting this parameter to `0`.
 *
 * The CSL period MUST be a multiple of `OT_LINK_CSL_PERIOD_TEN_SYMBOLS_UNIT_IN_USEC`, otherwise `OT_ERROR_INVALID_ARGS`
 * is returned.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aPeriod        The CSL period in microseconds.
 *
 * @retval OT_ERROR_NONE           Successfully set the CSL period.
 * @retval OT_ERROR_INVALID_ARGS   Invalid CSL period
 */
otError otLinkSetCslPeriod(otInstance *aInstance, uint32_t aPeriod);

/**
 * Gets the CSL timeout.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 *
 * @returns The CSL timeout in seconds.
 */
uint32_t otLinkGetCslTimeout(otInstance *aInstance);

/**
 * Sets the CSL timeout in seconds.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aTimeout       The CSL timeout in seconds.
 *
 * @retval OT_ERROR_NONE           Successfully set the CSL timeout.
 * @retval OT_ERROR_INVALID_ARGS   Invalid CSL timeout.
 */
otError otLinkSetCslTimeout(otInstance *aInstance, uint32_t aTimeout);

/**
 * Returns the current CCA (Clear Channel Assessment) failure rate.
 *
 * The rate is maintained over a window of (roughly) last `OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW`
 * frame transmissions.
 *
 * @returns The CCA failure rate with maximum value `0xffff` corresponding to 100% failure rate.
 */
uint16_t otLinkGetCcaFailureRate(otInstance *aInstance);

/**
 * Enables or disables the link layer.
 *
 * @note The link layer may only be enabled / disabled when the Thread Interface is disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aEnable       true to enable the link layer, or false otherwise.
 *
 * @retval OT_ERROR_NONE          Successfully enabled / disabled the link layer.
 * @retval OT_ERROR_INVALID_STATE Could not disable the link layer because
 *                                the Thread interface is enabled.
 */
otError otLinkSetEnabled(otInstance *aInstance, bool aEnable);

/**
 * Indicates whether or not the link layer is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   Link layer is enabled.
 * @retval FALSE  Link layer is not enabled.
 */
bool otLinkIsEnabled(otInstance *aInstance);

/**
 * Indicates whether or not CSL is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   Link layer is CSL enabled.
 * @retval FALSE  Link layer is not CSL enabled.
 */
bool otLinkIsCslEnabled(otInstance *aInstance);

/**
 * Indicates whether the device is connected to a parent which supports CSL.
 *
 * @retval TRUE   If parent supports CSL.
 * @retval FALSE  If parent does not support CSL.
 */
bool otLinkIsCslSupported(otInstance *aInstance);

/**
 * Instructs the device to send an empty IEEE 802.15.4 data frame.
 *
 * Is only supported on an Rx-Off-When-Idle device to send an empty data frame to its parent.
 * Note: available only when `OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE` is enabled.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           Successfully enqueued an empty message.
 * @retval OT_ERROR_INVALID_STATE  Device is not in Rx-Off-When-Idle mode.
 * @retval OT_ERROR_NO_BUFS        Insufficient message buffers available.
 */
otError otLinkSendEmptyData(otInstance *aInstance);

/**
 * Sets the region code.
 *
 * The radio region format is the 2-bytes ascii representation of the ISO 3166 alpha-2 code.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aRegionCode  The radio region code. The `aRegionCode >> 8` is first ascii char
 *                          and the `aRegionCode & 0xff` is the second ascii char.
 *
 * @retval  OT_ERROR_FAILED           Other platform specific errors.
 * @retval  OT_ERROR_NONE             Successfully set region code.
 * @retval  OT_ERROR_NOT_IMPLEMENTED  The feature is not implemented.
 */
otError otLinkSetRegion(otInstance *aInstance, uint16_t aRegionCode);

/**
 * Get the region code.
 *
 * The radio region format is the 2-bytes ascii representation of the ISO 3166 alpha-2 code.

 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[out] aRegionCode  The radio region code. The `aRegionCode >> 8` is first ascii char
 *                          and the `aRegionCode & 0xff` is the second ascii char.
 *
 * @retval  OT_ERROR_INVALID_ARGS     @p aRegionCode is NULL.
 * @retval  OT_ERROR_FAILED           Other platform specific errors.
 * @retval  OT_ERROR_NONE             Successfully got region code.
 * @retval  OT_ERROR_NOT_IMPLEMENTED  The feature is not implemented.
 */
otError otLinkGetRegion(otInstance *aInstance, uint16_t *aRegionCode);

/**
 * Gets the Wake-up channel.
 *
 * Requires `OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE` or `OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE`.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The Wake-up channel.
 */
uint8_t otLinkGetWakeupChannel(otInstance *aInstance);

/**
 * Sets the Wake-up channel.
 *
 * Requires `OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE` or `OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE`.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aChannel   The Wake-up sample channel. Channel value should be `0` (Set Wake-up Channel unspecified,
 *                        which means the device will use the PAN channel) or within the range [1, 10] (if 915-MHz
 *                        supported) and [11, 26] (if 2.4 GHz supported).
 *
 * @retval OT_ERROR_NONE           Successfully set the Wake-up channel.
 * @retval OT_ERROR_INVALID_ARGS   Invalid @p aChannel.
 */
otError otLinkSetWakeupChannel(otInstance *aInstance, uint8_t aChannel);

/**
 * Enables or disables listening for wake-up frames.
 *
 * Requires `OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE`.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aEnable       true to enable listening for wake-up frames, or false otherwise.
 *
 * @retval OT_ERROR_NONE          Successfully enabled / disabled the listening for wake-up frames.
 * @retval OT_ERROR_INVALID_ARGS  The listen duration is greater than the listen interval.
 * @retval OT_ERROR_INVALID_STATE Could not enable listening for wake-up frames due to bad configuration.
 */
otError otLinkSetWakeUpListenEnabled(otInstance *aInstance, bool aEnable);

/**
 * Returns whether listening for wake-up frames is enabled.
 *
 * Requires `OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE`.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 *
 * @retval TRUE   If listening for wake-up frames is enabled.
 * @retval FALSE  If listening for wake-up frames is not enabled.
 */
bool otLinkIsWakeupListenEnabled(otInstance *aInstance);

/**
 * Get the wake-up listen parameters.
 *
 * Requires `OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE`.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[out] aInterval   A pointer to return the wake-up listen interval in microseconds.
 * @param[out] aDuration   A pointer to return the wake-up listen duration in microseconds.
 */
void otLinkGetWakeupListenParameters(otInstance *aInstance, uint32_t *aInterval, uint32_t *aDuration);

/**
 * Set the wake-up listen parameters.
 *
 * The listen interval must be greater than the listen duration.
 * The listen duration must be greater or equal than the minimum supported.
 *
 * Requires `OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE`.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aInterval   The wake-up listen interval in microseconds.
 * @param[in]  aDuration   The wake-up listen duration in microseconds.
 *
 * @retval OT_ERROR_NONE           Successfully set the wake-up listen parameters.
 * @retval OT_ERROR_INVALID_ARGS   Invalid wake-up listen parameters.
 */
otError otLinkSetWakeupListenParameters(otInstance *aInstance, uint32_t aInterval, uint32_t aDuration);

/**
 * Sets the rx-on-when-idle state.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[in]  aRxOnWhenIdle  TRUE to keep radio in Receive state, FALSE to put to Sleep state during idle periods.
 *
 * @retval OT_ERROR_NONE             If successful.
 * @retval OT_ERROR_INVALID_STATE    If the raw link-layer isn't enabled.
 */
otError otLinkSetRxOnWhenIdle(otInstance *aInstance, bool aRxOnWhenIdle);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_LINK_H_
