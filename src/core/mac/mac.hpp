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

#include "common/locator.hpp"
#include "common/tasklet.hpp"
#include "common/timer.hpp"
#include "mac/mac_filter.hpp"
#include "mac/mac_frame.hpp"
#include "thread/key_manager.hpp"
#include "thread/link_quality.hpp"
#include "thread/network_diagnostic_tlvs.hpp"
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
    kMinBE             = 3,  ///< macMinBE (IEEE 802.15.4-2006)
    kMaxBE             = 5,  ///< macMaxBE (IEEE 802.15.4-2006)
    kMaxCSMABackoffs   = 4,  ///< macMaxCSMABackoffs (IEEE 802.15.4-2006)
    kUnitBackoffPeriod = 20, ///< Number of symbols (IEEE 802.15.4-2006)

    kMinBackoff = 1, ///< Minimum backoff (milliseconds).

    kAckTimeout      = 16,  ///< Timeout for waiting on an ACK (milliseconds).
    kDataPollTimeout = 100, ///< Timeout for receiving Data Frame (milliseconds).
    kSleepDelay      = 300, ///< Max sleep delay when frame is pending (milliseconds).
    kNonceSize       = 13,  ///< Size of IEEE 802.15.4 Nonce (bytes).

    kScanChannelsAll     = OT_CHANNEL_ALL, ///< All channels.
    kScanDurationDefault = 300,            ///< Default interval between channels (milliseconds).

    /**
     * Maximum number of MAC layer tx attempts for an outbound direct frame.
     *
     */
    kDirectFrameMacTxAttempts = OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT,

    /**
     * Maximum number of MAC layer tx attempts for an outbound indirect frame (for a sleepy child) after receiving
     * a data request command (data poll) from the child.
     *
     */
    kIndirectFrameMacTxAttempts = OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_PER_POLL,
};

/**
 * This class defines a channel mask.
 *
 * It is a wrapper class around a `uint32_t` bit vector representing a set of channels.
 *
 */
class ChannelMask
{
public:
    enum
    {
        kChannelIteratorFirst = 0xff, ///< Value to pass in `GetNextChannel()` to get the first channel in the mask.
        kInfoStringSize       = 45,   ///< Recommended buffer size to use with `ToString()`.
    };

    /**
     * This constructor initializes a `ChannelMask` instance.
     *
     */
    ChannelMask(void)
        : mMask(0)
    {
    }

    /**
     * This constructor initializes a `ChannelMask` instance with a given mask.
     *
     * @param[in]  aMask   A channel mask (as a `uint32_t` bit-vector mask with bit 0 (lsb) -> channel 0, and so on).
     *
     */
    ChannelMask(uint32_t aMask)
        : mMask(aMask)
    {
    }

    /**
     * This method clears the channel mask.
     *
     */
    void Clear(void) { mMask = 0; }

    /**
     * This method gets the channel mask (as a `uint32_t` bit-vector mask with bit 0 (lsb) -> channel 0, and so on).
     *
     * @returns The channel mask.
     *
     */
    uint32_t GetMask(void) const { return mMask; }

    /**
     * This method sets the channel mask.
     *
     * @param[in]  aMask   A channel mask (as a `uint32_t` bit-vector mask with bit 0 (lsb) -> channel 0, and so on).
     *
     */
    void SetMask(uint32_t aMask) { mMask = aMask; }

    /**
     * This method indicates if the mask is empty.
     *
     * @returns TRUE if the mask is empty, FALSE otherwise.
     *
     */
    bool IsEmpty(void) const { return (mMask == 0); }

    /**
     * This method indicates if the mask contains only a single channel.
     *
     * @returns TRUE if channel mask contains a single channel, FALSE otherwise
     *
     */
    bool IsSingleChannel(void) const { return ((mMask != 0) && ((mMask & (mMask - 1)) == 0)); }

    /**
     * This method indicates if the mask contains a given channel.
     *
     * @param[in]  aChannel  A channel.
     *
     * @returns TRUE if the channel @p aChannel is included in the mask, FALSE otherwise.
     *
     */
    bool ContainsChannel(uint8_t aChannel) const { return ((1U << aChannel) & mMask) != 0; }

    /**
     * This method adds a channel to the channel mask.
     *
     * @param[in]  aChannel  A channel
     *
     */
    void AddChannel(uint8_t aChannel) { mMask |= (1U << aChannel); }

    /**
     * This method removes a channel from the channel mask.
     *
     * @param[in]  aChannel  A channel
     *
     */
    void RemoveChannel(uint8_t aChannel) { mMask &= ~(1U << aChannel); }

    /**
     * This method updates the channel mask by intersecting it with another mask.
     *
     * @param[in]  aOtherMask  Another channel mask.
     *
     */
    void Intersect(const ChannelMask &aOtherMask) { mMask &= aOtherMask.mMask; }

    /**
     * This method gets the next channel in the channel mask.
     *
     * This method can be used to iterate over all channels in the channel mask. To get the first channel (channel with
     * lowest number) in the mask the @p aChannel should be set to `kChannelIteratorFirst`.
     *
     * @param[inout] aChannel        A reference to a `uint8_t`.
     *                               On entry it should contain the previous channel or `kChannelIteratorFirst`.
     *                               On exit it contains the next channel.
     *
     * @retval  OT_ERROR_NONE        Got the next channel, @p aChannel updated successfully.
     * @retval  OT_ERROR_NOT_FOUND   No next channel in the channel mask (note: @p aChannel may be changed).
     *
     */
    otError GetNextChannel(uint8_t &aChannel) const;

    /**
     * This method converts the channel mask into a human-readable NULL-terminated string.
     *
     * Examples of possible output:
     *  -  empty mask      ->  "{ }"
     *  -  all channels    ->  "{ 11-26 }"
     *  -  single channel  ->  "{ 20 }"
     *  -  multiple ranges ->  "{ 11, 14-17, 20-22, 24, 25 }"
     *  -  no range        ->  "{ 14, 21, 26 }"
     *
     * @param[out] aBuffer  A pointer to a char buffer to output the string.
     * @param[in]  aSize    Size of the buffer (number of bytes).
     *
     * @returns  A pointer to the @p aBuffer.
     *
     */
    const char *ToString(char *aBuffer, uint16_t aSize) const;

private:
#if (OT_RADIO_CHANNEL_MIN >= 32) || (OT_RADIO_CHANNEL_MAX >= 32)
#error `OT_RADIO_CHANNEL_MAX` or `OT_RADIO_CHANNEL_MIN` are larger than 32. `ChannelMask` uses 32 bit mask.
#endif

    uint32_t mMask;
};

/**
 * This class implements a MAC receiver client.
 *
 */
class Receiver : public OwnerLocator
{
    friend class Mac;

public:
    /**
     * This function pointer is called when a MAC frame is received.
     *
     * @param[in]  aReceiver A reference to the MAC receiver client object.
     * @param[in]  aFrame    A reference to the MAC frame.
     *
     */
    typedef void (*ReceiveFrameHandler)(Receiver &aReceiver, Frame &aFrame);

    /**
     * This function pointer is called on a data request command (data poll) timeout, i.e., when the ack in response to
     * a data request command indicated a frame is pending, but no frame was received after `kDataPollTimeout` interval.
     *
     * @param[in]  aReceiver  A reference to the MAC receiver client object.
     *
     */
    typedef void (*DataPollTimeoutHandler)(Receiver &aReceiver);

    /**
     * This constructor creates a MAC receiver client.
     *
     * @param[in]  aReceiveFrameHandler  A pointer to a function that is called on MAC frame reception.
     * @param[in]  aPollTimeoutHandler   A pointer to a function called on data poll timeout (may be set to NULL).
     * @param[in]  aOwner                A pointer to owner of this object.
     *
     */
    Receiver(ReceiveFrameHandler aReceiveFrameHandler, DataPollTimeoutHandler aPollTimeoutHandler, void *aOwner)
        : OwnerLocator(aOwner)
        , mReceiveFrameHandler(aReceiveFrameHandler)
        , mPollTimeoutHandler(aPollTimeoutHandler)
        , mNext(NULL)
    {
    }

private:
    void HandleReceivedFrame(Frame &aFrame) { mReceiveFrameHandler(*this, aFrame); }

    void HandleDataPollTimeout(void)
    {
        if (mPollTimeoutHandler != NULL)
        {
            mPollTimeoutHandler(*this);
        }
    }

    ReceiveFrameHandler    mReceiveFrameHandler;
    DataPollTimeoutHandler mPollTimeoutHandler;
    Receiver *             mNext;
};

/**
 * This class implements a MAC sender client.
 *
 */
class Sender : public OwnerLocator
{
    friend class Mac;

public:
    /**
     * This function pointer is called when the MAC is about to transmit the frame asking MAC sender client to provide
     * the frame.
     *
     * @param[in]  aSender   A reference to the MAC sender client object.
     * @param[in]  aFrame    A reference to the MAC frame buffer.
     *
     */
    typedef otError (*FrameRequestHandler)(Sender &aSender, Frame &aFrame);

    /**
     * This function pointer is called when the MAC is done sending the frame.
     *
     * @param[in]  aSender   A reference to the MAC sender client object.
     * @param[in]  aFrame    A reference to the MAC frame buffer that was sent.
     * @param[in]  aError    The status of the last MSDU transmission.
     *
     */
    typedef void (*SentFrameHandler)(Sender &aSender, Frame &aFrame, otError aError);

    /**
     * This constructor creates a MAC sender client.
     *
     * @param[in]  aFrameRequestHandler  A pointer to a function that is called when about to send a MAC frame.
     * @param[in]  aSentFrameHandler     A pointer to a function that is called when done sending the frame.
     * @param[in]  aOwner                A pointer to owner of this object.
     *
     */
    Sender(FrameRequestHandler aFrameRequestHandler, SentFrameHandler aSentFrameHandler, void *aOwner)
        : OwnerLocator(aOwner)
        , mFrameRequestHandler(aFrameRequestHandler)
        , mSentFrameHandler(aSentFrameHandler)
        , mNext(NULL)
    {
    }

private:
    otError HandleFrameRequest(Frame &aFrame) { return mFrameRequestHandler(*this, aFrame); }
    void    HandleSentFrame(Frame &aFrame, otError aError) { mSentFrameHandler(*this, aFrame, aError); }

    FrameRequestHandler mFrameRequestHandler;
    SentFrameHandler    mSentFrameHandler;
    Sender *            mNext;
};

/**
 * This class implements the IEEE 802.15.4 MAC.
 *
 */
class Mac : public InstanceLocator
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
     * This method registers a new MAC receiver client.
     *
     * @param[in]  aReceiver  A reference to the MAC receiver client.
     *
     * @retval OT_ERROR_NONE     Successfully registered the receiver.
     * @retval OT_ERROR_ALREADY  The receiver was already registered.
     *
     */
    otError RegisterReceiver(Receiver &aReceiver);

    /**
     * This method registers a new MAC sender client.
     *
     * @param[in]  aSender  A reference to the MAC sender client.
     *
     * @retval OT_ERROR_NONE     Successfully registered the sender.
     * @retval OT_ERROR_ALREADY  The sender was already registered.
     *
     */
    otError SendFrameRequest(Sender &aSender);

    /**
     * This method registers a Out of Band frame for MAC Transmission.
     * An Out of Band frame is one that was generated outside of OpenThread.
     *
     * @param[in]  aOobFrame  A pointer to the frame.
     *
     * @retval OT_ERROR_NONE     Successfully registered the frame.
     * @retval OT_ERROR_ALREADY  MAC layer is busy sending a previously registered frame.
     *
     */
    otError SendOutOfBandFrameRequest(otRadioFrame *aOobFrame);

    /**
     * This method generates a random IEEE 802.15.4 Extended Address.
     *
     * @param[out]  aExtAddress  A pointer to where the generated Extended Address is placed.
     *
     */
    void GenerateExtAddress(ExtAddress *aExtAddress);

    /**
     * This method returns a reference to the IEEE 802.15.4 Extended Address.
     *
     * @returns A pointer to the IEEE 802.15.4 Extended Address.
     *
     */
    const ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * This method sets the IEEE 802.15.4 Extended Address
     *
     * @param[in]  aExtAddress  A reference to the IEEE 802.15.4 Extended Address.
     *
     */
    void SetExtAddress(const ExtAddress &aExtAddress);

    /**
     * This method returns the IEEE 802.15.4 Short Address.
     *
     * @returns The IEEE 802.15.4 Short Address.
     *
     */
    ShortAddress GetShortAddress(void) const { return mShortAddress; }

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
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 PAN Channel.
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
     * This method sets the IEEE 802.15.4 Radio Channel. It can only be called
     * after successfully calling AcquireRadioChannel().
     *
     * @param[in]  aChannel  The IEEE 802.15.4 Radio Channel.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 Radio Channel.
     *
     */
    otError SetRadioChannel(uint16_t aAcquisitionId, uint8_t aChannel);

    /**
     * This method acquires external ownership of the Radio channel so that future calls to
     * SetRadioChannel will succeed.
     *
     * @param[out]  aAcquisitionId  The AcquisitionId that the caller should use when
     *                              calling SetRadioChannel().
     *
     * @retval OT_ERROR_NONE  Successfully acquired permission to Set the Radio Channel.
     * @retval OT_ERROR_INVALID_STATE  Failed to acquire permission as the radio Channel
     *                                 has already been acquired.
     *
     */
    otError AcquireRadioChannel(uint16_t *aAcquisitionId);

    /**
     * This method releases external ownership of the radio Channel
     * that was acquired with AcquireRadioChannel().  The channel
     * will re-adopt the PAN Channel when this API is called.
     *
     * @retval OT_ERROR_NONE  Successfully released the IEEE 802.15.4 Radio Channel.
     *
     */
    otError ReleaseRadioChannel(void);

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
     * @param[in]  aNetworkName  A pointer to the IEEE 802.15.4 Network Name.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 Network Name.
     *
     */
    otError SetNetworkName(const char *aNetworkName);

    /**
     * This method returns the IEEE 802.15.4 PAN ID.
     *
     * @returns The IEEE 802.15.4 PAN ID.
     *
     */
    uint16_t GetPanId(void) const { return mPanId; }

    /**
     * This method sets the IEEE 802.15.4 PAN ID.
     *
     * @param[in]  aPanId  The IEEE 802.15.4 PAN ID.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 PAN ID.
     *
     */
    otError SetPanId(uint16_t aPanId);

    /**
     * This method returns the IEEE 802.15.4 Extended PAN ID.
     *
     * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
     *
     */
    const uint8_t *GetExtendedPanId(void) const { return mExtendedPanId.m8; }

    /**
     * This method sets the IEEE 802.15.4 Extended PAN ID.
     *
     * @param[in]  aExtPanId  The IEEE 802.15.4 Extended PAN ID.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 Extended PAN ID.
     *
     */
    otError SetExtendedPanId(const uint8_t *aExtPanId);

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
     * This method is called to handle transmission start events.
     *
     * @param[in]  aFrame  A pointer to the frame that is transmitted.
     */
    void HandleTransmitStarted(otRadioFrame *aFrame);

    /**
     * This method is called to handle transmit events.
     *
     * @param[in]  aFrame      A pointer to the frame that was transmitted.
     * @param[in]  aAckFrame   A pointer to the ACK frame, NULL if no ACK was received.
     * @param[in]  aError      OT_ERROR_NONE when the frame was transmitted,
     *                         OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
     *                         OT_ERROR_CHANNEL_ACCESS_FAILURE when the tx failed due to activity on the channel,
     *                         OT_ERROR_ABORT when transmission was aborted for other reasons.
     *
     */
    void HandleTransmitDone(otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);

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
    bool IsPromiscuous(void);

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
     * This method fills network diagnostic MacCounterTlv.
     *
     * @param[in]  aMacCountersTlv The reference to the network diagnostic MacCounterTlv.
     *
     */
    void FillMacCountersTlv(NetworkDiagnostic::MacCountersTlv &aMacCounters) const;

    /**
     * This method resets mac counters
     *
     */
    void ResetCounters(void);

#if OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_WHEN_JOINABLE
    /**
     * This method indicates if the device is in joinable state or not.
     *
     * @retval true   Device is joinable.
     * @retval false  Device is non-joinable.
     *
     */
    bool IsBeaconJoinable(void);
#endif // OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_WHEN_JOINABLE

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
     * This method indicates whether or not CSMA backoff is supported by the radio layer.
     *
     * @retval true   CSMA backoff is supported by the radio.
     * @retval false  CSMA backoff is not supported by the radio.
     *
     */
    bool RadioSupportsCsmaBackoff(void);

    /**
     * This method indicates whether or not transmit retries is supported by the radio layer.
     *
     * @retval true   Retries (and CSMA) are supported by the radio.
     * @retval false  Retries (and CSMA) are not supported by the radio.
     *
     */
    bool RadioSupportsRetries(void);

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
     * This method Starts/Stops the Link layer. It may only be used when the Netif Interface is down
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

private:
    enum
    {
        kInvalidRssiValue  = 127,
        kMaxCcaSampleCount = OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW,
        kMaxAcquisitionId  = 0xffff,

    /**
     * Interval between RSSI samples when performing Energy Scan.
     *
     * `mBackoffTimer` is used for adding delay between RSSI samples. If microsecond timer is supported, 128 usec
     * time between samples is used, otherwise with a millisecond timer the minimum value of 1 msec is used.
     *
     */
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
        kEnergyScanRssiSampleInterval = 128,
#else
        kEnergyScanRssiSampleInterval = 1,
#endif
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

    void    GenerateNonce(const ExtAddress &aAddress, uint32_t aFrameCounter, uint8_t aSecurityLevel, uint8_t *aNonce);
    void    ProcessTransmitSecurity(Frame &aFrame);
    otError ProcessReceiveSecurity(Frame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor);
    void    UpdateIdleMode(void);
    void    StartOperation(Operation aOperation);
    void    FinishOperation(void);
    void    SendBeaconRequest(Frame &aFrame);
    void    SendBeacon(Frame &aFrame);
    void    StartBackoff(void);
    void    BeginTransmit(void);
    otError HandleMacCommand(Frame &aFrame);
    Frame * GetOperationFrame(void);

    static void HandleMacTimer(Timer &aTimer);
    void        HandleMacTimer(void);
    static void HandleBackoffTimer(Timer &aTimer);
    void        HandleBackoffTimer(void);
    static void HandleReceiveTimer(Timer &aTimer);
    void        HandleReceiveTimer(void);
    static void PerformOperation(Tasklet &aTasklet);
    void        PerformOperation(void);

    void StartCsmaBackoff(void);

    void    Scan(Operation aScanOperation, uint32_t aScanChannels, uint16_t aScanDuration, void *aContext);
    otError UpdateScanChannel(void);
    void    PerformActiveScan(void);
    void    PerformEnergyScan(void);
    void    ReportEnergyScanResult(int8_t aRssi);
    void    SampleRssi(void);

    otError RadioTransmit(Frame *aSendFrame);
    otError RadioReceive(uint8_t aChannel);
    otError RadioSleep(void);

    void LogFrameRxFailure(const Frame *aFrame, otError aError) const;
    void LogFrameTxFailure(const Frame &aFrame, otError aError) const;
    void LogBeacon(const char *aActionText, const BeaconPayload &aBeaconPayload) const;

    static const char *OperationToString(Operation aOperation);

    Operation mOperation;

    bool mPendingActiveScan : 1;
    bool mPendingEnergyScan : 1;
    bool mPendingTransmitBeacon : 1;
    bool mPendingTransmitData : 1;
    bool mPendingTransmitOobFrame : 1;
    bool mPendingWaitingForData : 1;
    bool mRxOnWhenIdle : 1;
    bool mBeaconsEnabled : 1;
#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    bool mDelaySleep : 1;
#endif

    Tasklet mOperationTask;

    TimerMilli mMacTimer;
#if OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
    TimerMicro mBackoffTimer;
#else
    TimerMilli mBackoffTimer;
#endif
    TimerMilli mReceiveTimer;

    ExtAddress   mExtAddress;
    ShortAddress mShortAddress;
    PanId        mPanId;
    uint8_t      mPanChannel;
    uint8_t      mRadioChannel;
    uint16_t     mRadioChannelAcquisitionId;

    otNetworkName   mNetworkName;
    otExtendedPanId mExtendedPanId;

    Sender *  mSendHead, *mSendTail;
    Receiver *mReceiveHead, *mReceiveTail;

    uint8_t mBeaconSequence;
    uint8_t mDataSequence;
    uint8_t mCsmaAttempts;
    uint8_t mTransmitAttempts;

    ChannelMask mScanChannelMask;
    uint16_t    mScanDuration;
    uint8_t     mScanChannel;
    int8_t      mEnergyScanCurrentMaxRssi;
    void *      mScanContext;
    union
    {
        ActiveScanHandler mActiveScanHandler;
        EnergyScanHandler mEnergyScanHandler;
    };

    otLinkPcapCallback mPcapCallback;
    void *             mPcapCallbackContext;

#if OPENTHREAD_ENABLE_MAC_FILTER
    Filter mFilter;
#endif // OPENTHREAD_ENABLE_MAC_FILTER

    Frame *mTxFrame;
    Frame *mOobFrame;

    otMacCounters mCounters;
    uint32_t      mKeyIdMode2FrameCounter;

    SuccessRateTracker mCcaSuccessRateTracker;
    uint16_t           mCcaSampleCount;
    bool               mEnabled;
};

/**
 * @}
 *
 */

} // namespace Mac
} // namespace ot

#endif // MAC_HPP_
