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

#include <openthread-core-config.h>
#include <common/tasklet.hpp>
#include <common/timer.hpp>
#include <mac/mac_frame.hpp>
#include <mac/mac_whitelist.hpp>
#include <mac/mac_blacklist.hpp>
#include <platform/radio.h>
#include <thread/key_manager.hpp>
#include <thread/topology.hpp>
#include <thread/network_diagnostic_tlvs.hpp>

namespace Thread {

namespace Mle { class MleRouter; }

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
    kMinBE                = 3,                     ///< macMinBE (IEEE 802.15.4-2006)
    kMaxBE                = 5,                     ///< macMaxBE (IEEE 802.15.4-2006)
    kMaxCSMABackoffs      = 4,                     ///< macMaxCSMABackoffs (IEEE 802.15.4-2006)
    kMaxFrameRetries      = 3,                     ///< macMaxFrameRetries (IEEE 802.15.4-2006)
    kUnitBackoffPeriod    = 20,                    ///< Number of symbols (IEEE 802.15.4-2006)

    kMinBackoff           = 1,                     ///< Minimum backoff (milliseconds).
    kMaxFrameAttempts     = kMaxFrameRetries + 1,  ///< Number of transmission attempts.

    kAckTimeout           = 16,                    ///< Timeout for waiting on an ACK (milliseconds).
    kDataPollTimeout      = 100,                   ///< Timeout for receiving Data Frame (milliseconds).
    kNonceSize            = 13,                    ///< Size of IEEE 802.15.4 Nonce (bytes).

    kScanChannelsAll      = OT_CHANNEL_ALL,        ///< All channels.
    kScanDurationDefault  = 300,                   ///< Default interval between channels (milliseconds).
};

/**
 * This class implements a MAC receiver client.
 *
 */
class Receiver
{
    friend class Mac;

public:
    /**
     * This function pointer is called when a MAC frame is received.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     * @param[in]  aFrame    A reference to the MAC frame.
     *
     */
    typedef void (*ReceiveFrameHandler)(void *aContext, Frame &aFrame);

    /**
     * This constructor creates a MAC receiver client.
     *
     * @param[in]  aReceiveFrameHandler  A pointer to a function that is called on MAC frame reception.
     * @param[in]  aContext              A pointer to arbitrary context information.
     *
     */
    Receiver(ReceiveFrameHandler aReceiveFrameHandler, void *aContext) {
        mReceiveFrameHandler = aReceiveFrameHandler;
        mContext = aContext;
        mNext = NULL;
    }

private:
    void HandleReceivedFrame(Frame &frame) { mReceiveFrameHandler(mContext, frame); }

    ReceiveFrameHandler mReceiveFrameHandler;
    void *mContext;
    Receiver *mNext;
};

/**
 * This class implements a MAC sender client.
 *
 */
class Sender
{
    friend class Mac;

public:
    /**
     * This function pointer is called when the MAC is about to transmit the frame.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     * @param[in]  aFrame    A reference to the MAC frame buffer.
     *
     */
    typedef ThreadError(*FrameRequestHandler)(void *aContext, Frame &aFrame);

    /**
     * This function pointer is called when the MAC is done sending the frame.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     * @param[in]  aFrame    A reference to the MAC frame buffer that was sent.
     * @param[in]  aError    The status of the last MSDU transmission.
     *
     */
    typedef void (*SentFrameHandler)(void *aContext, Frame &aFrame, ThreadError aError);

    /**
     * This constructor creates a MAC sender client.
     *
     * @param[in]  aFrameRequestHandler  A pointer to a function that is called when about to send a MAC frame.
     * @param[in]  aSentFrameHandler     A pointer to a function that is called when done sending the frame.
     * @param[in]  aContext              A pointer to arbitrary context information.
     *
     */
    Sender(FrameRequestHandler aFrameRequestHandler, SentFrameHandler aSentFrameHandler, void *aContext) {
        mFrameRequestHandler = aFrameRequestHandler;
        mSentFrameHandler = aSentFrameHandler;
        mContext = aContext;
        mNext = NULL;
    }

private:
    ThreadError HandleFrameRequest(Frame &frame) { return mFrameRequestHandler(mContext, frame); }
    void HandleSentFrame(Frame &frame, ThreadError error) { mSentFrameHandler(mContext, frame, error); }

    FrameRequestHandler mFrameRequestHandler;
    SentFrameHandler mSentFrameHandler;
    void *mContext;
    Sender *mNext;
};

/**
 * This class implements the IEEE 802.15.4 MAC.
 *
 */
class Mac
{
public:
    /**
     * This constructor initializes the MAC object.
     *
     * @param[in]  aThreadNetif  A reference to the network interface using this MAC.
     *
     */
    explicit Mac(ThreadNetif &aThreadNetif);

    /**
     * This function pointer is called on receiving an IEEE 802.15.4 Beacon during an Active Scan.
     *
     * @param[in]  aContext       A pointer to arbitrary context information.
     * @param[in]  aBeaconFrame   A pointer to the Beacon frame.
     *
     */
    typedef void (*ActiveScanHandler)(void *aContext, Frame *aBeaconFrame);

    /**
     * This method starts an IEEE 802.15.4 Active Scan.
     *
     * @param[in]  aScanChannels  A bit vector indicating which channels to scan.
     * @param[in]  aScanDuration  The time in milliseconds to spend scanning each channel.
     * @param[in]  aHandler       A pointer to a function that is called on receiving an IEEE 802.15.4 Beacon.
     * @param[in]  aContext       A pointer to arbitrary context information.
     *
     */
    ThreadError ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext);

    /**
     * This function pointer is called during an "Energy Scan" when the result for a channel is ready or the scan
     * completes.
     *
     * @param[in]  aResult   A valid pointer to the energy scan result information or NULL when the energy scan completes.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    typedef void (*EnergyScanHandler)(void *aContext, otEnergyScanResult *aResult);

    /**
     * This method starts an IEEE 802.15.4 Energy Scan.
     *
     * @param[in]  aScanChannels     A bit vector indicating on which channels to perform energy scan.
     * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
     * @param[in]  aHandler          A pointer to a function called to pass on scan result or indicate scan completion.
     * @param[in]  aContext          A pointer to arbitrary context information.
     *
     * @retval kThreadError_None  Accepted the Energy Scan request.
     * @retval kThreadError_Busy  Could not start the energy scan.
     *
     */
    ThreadError EnergyScan(uint32_t aScanChannels, uint16_t aScanDuration, EnergyScanHandler aHandler, void *aContext);

    /**
     * This method indicates the energy scan for the current channel is complete.
     *
     * @param[in]  aEnergyScanMaxRssi  The maximum RSSI encountered on the scanned channel.
     *
     */
    void EnergyScanDone(int8_t aEnergyScanMaxRssi);

    /**
     * This method indicates whether or not rx-on-when-idle is enabled.
     *
     * @retval TRUE   If rx-on-when-idle is enabled.
     * @retval FALSE  If rx-on-when-idle is not enabled.
     */
    bool GetRxOnWhenIdle(void) const;

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
     * @retval kThreadError_None  Successfully registered the receiver.
     * @retval kThreadError_Already  The receiver was already registered.
     *
     */
    ThreadError RegisterReceiver(Receiver &aReceiver);

    /**
     * This method registers a new MAC sender client.
     *
     * @param[in]  aSender  A reference to the MAC sender client.
     *
     * @retval kThreadError_None  Successfully registered the sender.
     * @retval kThreadError_Already  The sender was already registered.
     *
     */
    ThreadError SendFrameRequest(Sender &aSender);

    /**
     * This method returns a pointer to the IEEE 802.15.4 Extended Address.
     *
     * @returns A pointer to the IEEE 802.15.4 Extended Address.
     *
     */
    const ExtAddress *GetExtAddress(void) const;

    /**
     * This method sets the IEEE 802.15.4 Extended Address
     *
     * @param[in]  aExtAddress  A reference to the IEEE 802.15.4 Extended Address.
     *
     */
    void SetExtAddress(const ExtAddress &aExtAddress);

    /**
     * This method gets the Hash Mac Address.
     *
     * Hash Mac Address is the first 64 bits of the result of computing SHA-256 over factory-assigned
     * IEEE EUI-64, which is used as IEEE 802.15.4 Extended Address during commissioning process.
     *
     * @param[out]  aHashMacAddress    A pointer to where the Hash Mac Address is placed.
     *
     */
    void GetHashMacAddress(ExtAddress *aHashMacAddress);

    /**
     * This method returns the IEEE 802.15.4 Short Address.
     *
     * @returns The IEEE 802.15.4 Short Address.
     *
     */
    ShortAddress GetShortAddress(void) const;

    /**
     * This method sets the IEEE 802.15.4 Short Address.
     *
     * @param[in]  aShortAddress  The IEEE 802.15.4 Short Address.
     *
     * @retval kThreadError_None  Successfully set the IEEE 802.15.4 Short Address.
     *
     */
    ThreadError SetShortAddress(ShortAddress aShortAddress);

    /**
     * This method returns the IEEE 802.15.4 Channel.
     *
     * @returns The IEEE 802.15.4 Channel.
     *
     */
    uint8_t GetChannel(void) const;

    /**
     * This method sets the IEEE 802.15.4 Channel.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 Channel.
     *
     * @retval kThreadError_None  Successfully set the IEEE 802.15.4 Channel.
     *
     */
    ThreadError SetChannel(uint8_t aChannel);

    /**
     * This method returns the maximum transmit power in dBm.
     *
     * @returns  The maximum transmit power in dBm.
     *
     */
    int8_t GetMaxTransmitPower(void) const;

    /**
     * This method sets the maximum transmit power in dBm.
     *
     * @param[in]  aPower  The maximum transmit power in dBm.
     *
     */
    void SetMaxTransmitPower(int8_t aPower);

    /**
     * This method returns the IEEE 802.15.4 Network Name.
     *
     * @returns A pointer to the IEEE 802.15.4 Network Name.
     *
     */
    const char *GetNetworkName(void) const;

    /**
     * This method sets the IEEE 802.15.4 Network Name.
     *
     * @param[in]  aNetworkName  A pointer to the IEEE 802.15.4 Network Name.
     *
     * @retval kThreadError_None  Successfully set the IEEE 802.15.4 Network Name.
     *
     */
    ThreadError SetNetworkName(const char *aNetworkName);

    /**
     * This method returns the IEEE 802.15.4 PAN ID.
     *
     * @returns The IEEE 802.15.4 PAN ID.
     *
     */
    uint16_t GetPanId(void) const;

    /**
     * This method sets the IEEE 802.15.4 PAN ID.
     *
     * @param[in]  aPanId  The IEEE 802.15.4 PAN ID.
     *
     * @retval kThreadError_None  Successfully set the IEEE 802.15.4 PAN ID.
     *
     */
    ThreadError SetPanId(uint16_t aPanId);

    /**
     * This method returns the IEEE 802.15.4 Extended PAN ID.
     *
     * @returns A pointer to the IEEE 802.15.4 Extended PAN ID.
     *
     */
    const uint8_t *GetExtendedPanId(void) const;

    /**
     * This method sets the IEEE 802.15.4 Extended PAN ID.
     *
     * @param[in]  aExtPanId  The IEEE 802.15.4 Extended PAN ID.
     *
     * @retval kThreadError_None  Successfully set the IEEE 802.15.4 Extended PAN ID.
     *
     */
    ThreadError SetExtendedPanId(const uint8_t *aExtPanId);

    /**
     * This method returns the MAC whitelist filter.
     *
     * @returns A reference to the MAC whitelist filter.
     *
     */
    Whitelist &GetWhitelist(void);

    /**
     * This method returns the MAC blacklist filter.
     *
     * @returns A reference to the MAC blacklist filter.
     *
     */
    Blacklist &GetBlacklist(void);

    /**
     * This method is called to handle receive events.
     *
     * @param[in]  aFrame  A pointer to the received frame, or NULL if the receive operation aborted.
     * @param[in]  aError  ::kThreadError_None when successfully received a frame, ::kThreadError_Abort when reception
     *                     was aborted and a frame was not received.
     *
     */
    void ReceiveDoneTask(Frame *aFrame, ThreadError aError);

    /**
     * This method is called to handle transmit events.
     *
     * @param[in]  aFramePending  TRUE if an ACK frame was received and the Frame Pending bit was set.
     * @param[in]  aError  ::kThreadError_None when the frame was transmitted, ::kThreadError_NoAck when the frame was
     *                     transmitted but no ACK was received, ::kThreadError_ChannelAccessFailure when the transmission
     *                     could not take place due to activity on the channel, ::kThreadError_Abort when transmission
     *                     was aborted for other reasons.
     *
     */
    void TransmitDoneTask(RadioPacket *aPacket, bool aRxPending, ThreadError aError);

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
     * This method registers a callback to provide received raw IEEE 802.15.4 frames.
     *
     * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame or
     *                               NULL to disable the callback.
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

    /**
     * This method returns the MAC counter.
     *
     * @returns A reference to the MAC counter.
     *
     */
    otMacCounters &GetCounters(void);

    /**
     * This method returns the noise floor state.
     *
     * @returns A reference to the noise floor state.
     *
     */
    LinkQualityInfo &GetNoiseFloor(void) { return mNoiseFloor; }

    /**
     * This method enables/disables source match feature.
     *
     * @param[in]  aEnable  Enable/disable source match for automatical pending.
     *
     */
    void EnableSrcMatch(bool aEnable);

    /**
     * This method adds the address into the source match table.
     *
     * @param[in]  aAddr  The address to be added into the source match table.
     *
     * @retval ::kThreadError_None  Successfully added the address into the source match table.
     * @retval ::kThreadError_NoBufs No available entry in the source match table
     *
     */
    ThreadError AddSrcMatchEntry(Address &aAddr);

    /**
     * This method removes the address from the source match table.
     *
     * @param[in]  aAddr  The address to be removed from the source match table.
     *
     * @retval ::kThreadError_None  Successfully removed the address from the source match table.
     * @retval ::kThreadError_NoAddress  The address is not in the source match table.
     *
     */
    ThreadError ClearSrcMatchEntry(Address &aAddr);

    /**
     * This method clears the source match table.
     *
     */
    void ClearSrcMatchEntries(void);

    /**
     * This method indicates whether or not transmit retries and CSMA backoff logic is supported by the radio layer.
     *
     * @retval true   Retries and CSMA are supported by the radio.
     * @retval false  Retries and CSMA are not supported by the radio.
     *
     */
    bool RadioSupportsRetriesAndCsmaBackoff(void);

private:
    enum ScanType
    {
        kScanTypeNone = 0,
        kScanTypeActive,
        kScanTypeEnergy,
    };

    enum
    {
        kInvalidRssiValue = 127
    };

    void GenerateNonce(const ExtAddress &aAddress, uint32_t aFrameCounter, uint8_t aSecurityLevel, uint8_t *aNonce);
    void NextOperation(void);
    void ProcessTransmitSecurity(Frame &aFrame);
    ThreadError ProcessReceiveSecurity(Frame &aFrame, const Address &aSrcAddr, Neighbor *aNeighbor);
    void ScheduleNextTransmission(void);
    void SentFrame(ThreadError aError);
    void SendBeaconRequest(Frame &aFrame);
    void SendBeacon(Frame &aFrame);
    void StartBackoff(void);
    void StartEnergyScan(void);
    ThreadError HandleMacCommand(Frame &aFrame);

    static void HandleMacTimer(void *aContext);
    void HandleMacTimer(void);
    static void HandleBeginTransmit(void *aContext);
    void HandleBeginTransmit(void);
    static void HandleReceiveTimer(void *aContext);
    void HandleReceiveTimer(void);
    static void HandleEnergyScanSampleRssi(void *aContext);
    void HandleEnergyScanSampleRssi(void);

    void StartCsmaBackoff(void);
    ThreadError Scan(ScanType aType, uint32_t aScanChannels, uint16_t aScanDuration, void *aContext);

    Timer mMacTimer;
    Timer mBackoffTimer;
    Timer mReceiveTimer;

    ThreadNetif &mNetif;

    ExtAddress mExtAddress;
    ShortAddress mShortAddress;
    PanId mPanId;
    uint8_t mChannel;
    int8_t mMaxTransmitPower;

    otNetworkName mNetworkName;
    otExtendedPanId mExtendedPanId;

    Sender *mSendHead, *mSendTail;
    Receiver *mReceiveHead, *mReceiveTail;

    enum
    {
        kStateIdle = 0,
        kStateActiveScan,
        kStateEnergyScan,
        kStateTransmitBeacon,
        kStateTransmitData,
    };
    uint8_t mState;

    uint8_t mBeaconSequence;
    uint8_t mDataSequence;
    bool mRxOnWhenIdle;
    uint8_t mCsmaAttempts;
    uint8_t mTransmitAttempts;
    bool mTransmitBeacon;

    ScanType mPendingScanRequest;
    uint8_t mScanChannel;
    uint32_t mScanChannels;
    uint16_t mScanDuration;
    void *mScanContext;
    union
    {
        ActiveScanHandler mActiveScanHandler;
        EnergyScanHandler mEnergyScanHandler;
    };
    int8_t mEnergyScanCurrentMaxRssi;
    Tasklet mEnergyScanSampleRssiTask;

    LinkQualityInfo mNoiseFloor;

    otLinkPcapCallback mPcapCallback;
    void *mPcapCallbackContext;

    Whitelist mWhitelist;
    Blacklist mBlacklist;

    Frame *mTxFrame;

    otMacCounters mCounters;
    uint32_t mKeyIdMode2FrameCounter;
};

/**
 * @}
 *
 */

}  // namespace Mac
}  // namespace Thread

#endif  // MAC_HPP_
