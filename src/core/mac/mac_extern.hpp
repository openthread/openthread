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

#ifndef MAC_EXTERN_HPP_
#define MAC_EXTERN_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_USE_EXTERNAL_MAC

#include <openthread/platform/radio-mac.h>
#include "common/notifier.hpp"
#include "mac/mac_common.hpp"

namespace ot {

class MeshSender;
namespace Mle {
class MleRouter;
}

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
    kMaxCSMABackoffs = 4, ///< macMaxCSMABackoffs (IEEE 802.15.4-2006)
    kMaxFrameRetries = 3, ///< macMaxFrameRetries (IEEE 802.15.4-2006)

    kBeaconOrderInvalid = 15, ///< Invalid value for beacon order which causes it to be ignored
};

class FullAddr : public otFullAddr
{
public:
    otError GetAddress(Address &aAddress) const;
    otError SetAddress(const Address &aAddress);
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
    typedef void (*ReceiveFrameHandler)(Receiver &aReceiver, otDataIndication &aDataIndication);

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
    void HandleReceivedFrame(otDataIndication &aDataIndication) { mReceiveFrameHandler(*this, aDataIndication); }

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
class Sender
{
    friend class Mac;

public:
    /**
     * This function pointer is called when the MAC is about to transmit the frame.
     *
     * @param[in]  aSender   A reference to the MAC sender client object.
     * @param[in]  aFrame    A reference to the MAC frame buffer.
     *
     */
    typedef otError (*FrameRequestHandler)(Sender &aSender, Frame &aFrame, otDataRequest &aDataReq);

    /**
     * This function pointer is called when the MAC is done sending the frame.
     *
     * @param[in]  aSender   A reference to the MAC sender client object.
     * @param[in]  aFrame    A reference to the MAC frame buffer that was sent.
     * @param[in]  aError    The status of the last MSDU transmission.
     *
     */
    typedef void (*SentFrameHandler)(Sender &aSender, otError aError);

    /**
     * This constructor creates a MAC sender client.
     *
     * @param[in]  aFrameRequestHandler  A pointer to a function that is called when about to send a MAC frame.
     * @param[in]  aSentFrameHandler     A pointer to a function that is called when done sending the frame.
     * @param[in]  aOwner                A pointer to owner of this object.
     *
     */
    Sender(FrameRequestHandler aFrameRequestHandler, SentFrameHandler aSentFrameHandler, MeshSender *aMeshSender)
        : mMsduHandle(0)
        , mMessageOffset(0)
        , mFrameRequestHandler(aFrameRequestHandler)
        , mSentFrameHandler(aSentFrameHandler)
        , mNext(NULL)
        , mMeshSender(aMeshSender)
    {
    }

    /**
     * This constructor creates an empty MAC sender client.
     *
     */
    Sender()
        : mMsduHandle(0)
        , mMessageOffset(0)
        , mFrameRequestHandler(NULL)
        , mSentFrameHandler(NULL)
        , mNext(NULL)
        , mMeshSender(NULL)
    {
    }

    MeshSender *GetMeshSender(void) const { return mMeshSender; }
    void        SetMeshSender(MeshSender *aMeshSender) { mMeshSender = aMeshSender; }
    void        SetMessageEndOffset(uint16_t aMessageOffset) { mMessageOffset = aMessageOffset; }
    uint16_t    GetMessageEndOffset(void) { return mMessageOffset; }
    bool        IsInUse(void) { return mMsduHandle; }

private:
    otError HandleFrameRequest(Frame &frame, otDataRequest &aDataReq)
    {
        return mFrameRequestHandler(*this, frame, aDataReq);
    }
    void HandleSentFrame(otError error)
    {
        mMsduHandle = 0;
        mSentFrameHandler(*this, error);
    }

    uint8_t             mMsduHandle;
    uint16_t            mMessageOffset;
    FrameRequestHandler mFrameRequestHandler;
    SentFrameHandler    mSentFrameHandler;
    Sender *            mNext;
    MeshSender *        mMeshSender;
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
     * @param[in]  aBeaconFrame   A pointer to the Beacon frame.
     *
     */
    typedef void (*ActiveScanHandler)(void *aContext, otBeaconNotify *aResult);

    /**
     * This method starts an IEEE 802.15.4 Active Scan.
     *
     * @param[in]  aScanChannels  A bit vector indicating which channels to scan.
     * @param[in]  aScanDuration  The time in milliseconds to spend scanning each channel.
     * @param[in]  aHandler       A pointer to a function that is called on receiving an IEEE 802.15.4 Beacon.
     * @param[in]  aContext       A pointer to arbitrary context information.
     *
     */
    otError ActiveScan(uint32_t aScanChannels, uint16_t aScanDuration, ActiveScanHandler aHandler, void *aContext);

    /**
     * This method converts a beacon notify indication to an active scan result of type `otActiveScanResult`.
     *
     * @param[in]  aBeaconFrame             A pointer to a beacon notify.
     * @param[out] aResult                  A reference to `otActiveScanResult` where the result is stored.
     *
     * @retval OT_ERROR_NONE            Successfully converted the beacon into active scan result.
     * @retval OT_ERROR_INVALID_ARGS    The @a aBeaconFrame was NULL.
     * @retval OT_ERROR_PARSE           Failed parsing the beacon frame.
     *
     */
    otError ConvertBeaconToActiveScanResult(otBeaconNotify *aBeaconFrame, otActiveScanResult &aResult);

    /**
     * This function pointer is called during an "Energy Scan" when the result for a channel is ready or the scan
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
     * @param[in]  aScanChannels     A bit vector indicating on which channels to perform energy scan.
     * @param[in]  aScanDuration     The time in milliseconds to spend scanning each channel.
     * @param[in]  aHandler          A pointer to a function called to pass on scan result or indicate scan
     * completion.
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
     * This method handles an MLME scan confirm
     *
     * @param[in]  aScanConfirm     A pointer to the otScanConfirm parameter struct
     *
     *
     */
    void HandleScanConfirm(otScanConfirm *aScanConfirm);

    /**
     * This method handles an MLME beacon notification
     *
     * @param[in]  aScanConfirm     A pointer to the otBeaconNotify parameter struct
     *
     *
     */
    void HandleBeaconNotification(otBeaconNotify *aBeaconNotify);

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
    void SetBeaconEnabled(bool aEnabled);

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
     * Request the hardmac to send a poll
     *
     * @param[in]  aPollReq  SAP for a poll request
     *
     * @retval OT_ERROR_NONE     Successfully sent a poll
     * @retval OT_ERROR_NO_ACK   Poll wasn't acked by the destination
     *
     */
    otError SendDataPoll(otPollRequest &aPollReq);

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
     * This method cancels a pending frame request
     *
     * @param[in]  aSender  A reference to the MAC sender client.
     *
     * @retval OT_ERROR_NONE     Successfully purged the sender.
     * @retval OT_ERROR_ALREADY  The sender was not active
     *
     */
    otError PurgeFrameRequest(Sender &aSender);

    /**
     * This method registers a Out of Band frame for MAC Transmission.
     * An Out of Band frame is one that was generated outside of OpenThread.
     *
     * @param[in]  aOobFrame  A pointer to the frame.
     *
     * @retval OT_ERROR_NOT_IMPLEMENTED     Not implemented
     *
     */
    otError SendOutOfBandFrameRequest(otRadioFrame *aOobFrame)
    {
        (void)aOobFrame;
        return OT_ERROR_NOT_IMPLEMENTED;
    }

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
     * This method returns the IEEE 802.15.4 Channel.
     *
     * @returns The IEEE 802.15.4 Channel.
     *
     */
    uint8_t GetPanChannel(void) const { return mChannel; }

    /**
     * This method sets the IEEE 802.15.4 Channel.
     *
     * @param[in]  aChannel  The IEEE 802.15.4 Channel.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 Channel.
     *
     */
    otError SetPanChannel(uint8_t aChannel);

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
     * @param[in]  aNetworkName  A pointer to the IEEE 802.15.4 Network Name.
     *
     * @retval OT_ERROR_NONE  Successfully set the IEEE 802.15.4 Network Name.
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
    const otExtendedPanId &GetExtendedPanId(void) const { return mExtendedPanId; }

    /**
     * This method sets the IEEE 802.15.4 Extended PAN ID.
     *
     * @param[in]  aExtPanId  The IEEE 802.15.4 Extended PAN ID.
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
     * This method is called to handle received data packets
     *
     * @param[in]  aFrame  A pointer to the otDataIndication primitive
     *
     */
    void ProcessDataIndication(otDataIndication *aDataIndication);

    /**
     * This method is called to handle received data packets that failed security
     *
     * @param[in]  aFrame  A pointer to the otCommStatusIndication primitive
     *
     */
    void ProcessCommStatusIndication(otCommStatusIndication *aCommStatusIndication);

    /**
     * This method is called to handle transmission start events.
     *
     * @param[in]  aFrame  A pointer to the frame that is transmitted.
     */
    void TransmitStartedTask(otRadioFrame *aFrame);

    /**
     * This method is called to handle transmit events.
     *
     * @param[in]  aFrame      A pointer to the frame that was transmitted.
     * @param[in]  aAckFrame   A pointer to the ACK frame, NULL if no ACK was received.
     * @param[in]  aError      OT_ERROR_NONE when the frame was transmitted, OT_ERROR_NO_ACK when the frame was
     *                         transmitted but no ACK was received, OT_ERROR_CHANNEL_ACCESS_FAILURE when the
     *                         transmission could not take place due to activity on the channel, OT_ERROR_ABORT when
     *                         transmission was aborted for other reasons.
     *
     */
    void TransmitDoneTask(uint8_t aMsduHandle, int aMacError);

    /**
     * This method returns if a scan is in progress.
     *
     */
    bool IsScanInProgress(void);

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
     * The MAC layer is in transmit state during CSMA/CA, CCA, transmission of Data, Beacon or Data Request frames
     * and receiving of ACK frames. The MAC layer is not in transmit state during transmission of ACK frames or
     * Beacon Requests.
     *
     */
    bool IsInTransmitState(void);

    /**
     * This method registers a callback to provide received raw IEEE 802.15.4 frames.
     *
     * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link
     * frame or NULL to disable the callback.
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
    otMacCounters &GetCounters(void) { return mCounters; }

    /**
     * This method returns the noise floor value (currently use the radio receive sensitivity value).
     *
     * @returns The noise floor value in dBm.
     *
     */
    int8_t GetNoiseFloor(void);

    /**
     * This method configures the external MAC for thread
     *
     * @retval OT_ERROR_NONE  Success.
     *
     */
    otError Start(void);

    /**
     * This method resets the external MAC so that it stops
     *
     * @retval OT_ERROR_NONE  Success.
     *
     */
    otError Stop(void);

    /**
     * This method queries the external mac device table and caches the frame counter for
     * the provided neighbor in its data
     *
     * @param[in]  aNeighbor  The neighbor to cache the frame counter for
     *
     */
    void CacheDevice(Neighbor &aNeighbor);

    /**
     * This method sets the frame counter for a neighbor device in the PIB to
     * match the value stored locally.
     *
     * @param[in]  aNeighbor  The neighbor to update the frame counter for
     *
     */
    otError UpdateDevice(Neighbor &aNeighbor);

    /**
     * This method queries the external mac device table and caches the frame counters in the
     * relevant neighbour data structure.
     *
     */
    void CacheDeviceTable(void);

    /**
     * This method rebuilds the key and device tables for the external mac
     *
     */
    void BuildSecurityTable(void);

    /**
     * This method returns the current frame counter for this device
     *
     * @returns The current frame counter
     *
     */
    uint32_t GetFrameCounter(void);

    /**
     * This method sets the current frame counter in the PIB
     *
     * @param[in]  aFrameCounter  The value of the frame counter to set
     *
     */
    void SetFrameCounter(uint32_t aFrameCounter);

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
    };

    enum Operation
    {
        kOperationIdle = 0,
        kOperationActiveScan,
        kOperationEnergyScan,
        kOperationTransmitData,
    };

    void    ProcessTransmitSecurity(otSecSpec &aSecSpec);
    otError ProcessReceiveSecurity(otSecSpec &aSecSpec, Neighbor *aNeighbor);
    void    StartOperation(Operation aOperation);
    void    FinishOperation(void);
    void    BuildBeacon(void);
    void    HandleBeginTransmit(void);
    otError Scan(Operation aScanOperation, uint32_t aScanChannels, uint16_t aScanDuration, void *aContext);
    void    HandleBeginScan(void);

    otError BuildDeviceDescriptor(const ExtAddress &aExtAddress,
                                  uint32_t          aFrameCounter,
                                  PanId             aPanId,
                                  uint16_t          shortAddr,
                                  uint8_t           aIndex);
    otError BuildDeviceDescriptor(Neighbor &aNeighbor, uint8_t &aIndex);
    otError BuildRouterDeviceDescriptors(uint8_t &aDevIndex, uint8_t &aNumActiveDevices, uint8_t aIgnoreRouterId);
    void    BuildJoinerKeyDescriptor(uint8_t aIndex);
    void    BuildMainKeyDescriptors(uint8_t aDeviceCount, uint8_t &aIndex);
    void    BuildMode2KeyDescriptor(uint8_t aIndex, uint8_t aMode2DevHandle);
    void    HotswapJoinerRouterKeyDescriptor(uint8_t *aDstAddr);

    otError SetTempChannel(uint8_t aChannel, otDataRequest &aDataRequest);
    otError RestoreChannel();

    otError RadioReceive(uint8_t aChannel);

    static void sStateChangedCallback(Notifier::Callback &aCallback, uint32_t aFlags);
    void        stateChangedCallback(uint32_t aFlags);

    uint8_t GetValidMsduHandle(void);
    Sender *PopSendingSender(uint8_t aMsduHandle);

    static const char *OperationToString(Operation aOperation);

    static void CopyReversedExtAddr(const ExtAddress &aExtAddrIn, uint8_t *aExtAddrOut);
    static void CopyReversedExtAddr(const uint8_t *aExtAddrIn, ExtAddress &aExtAddrOut);

    Operation mOperation;

    bool mPendingActiveScan : 1;
    bool mPendingEnergyScan : 1;
    bool mPendingTransmitData : 1;
    bool mRxOnWhenIdle : 1;
    bool mBeaconsEnabled : 1;
    bool mEnabled : 1;
#if OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
    bool mDelaySleep : 1;
#endif

    ExtAddress   mExtAddress;
    ShortAddress mShortAddress;
    PanId        mPanId;
    uint8_t      mChannel;
    uint8_t      mNextMsduHandle;
    uint8_t      mDynamicKeyIndex;
    uint8_t      mMode2DevHandle;
    uint8_t      mJoinerEntrustResponseHandle;
    uint8_t      mTempChannelMessageHandle;
    ChannelMask  mSupportedChannelMask;

    uint8_t mDeviceCurrentKeys[OPENTHREAD_CONFIG_EXTERNAL_MAC_DEVICE_TABLE_SIZE];

    Notifier::Callback mNotifierCallback;

    otNetworkName   mNetworkName;
    otExtendedPanId mExtendedPanId;

    Sender *  mSendHead, *mSendTail;
    Sender *  mSendingHead;
    Receiver *mReceiveHead, *mReceiveTail;

    uint32_t mScanChannels;
    uint16_t mScanDuration;
    void *   mScanContext;
    union
    {
        ActiveScanHandler mActiveScanHandler;
        EnergyScanHandler mEnergyScanHandler;
    };

    union
    {
        otDataRequest  mDataReq;
        otScanRequest  mScanReq;
        otStartRequest mStartReq;
    };

    SuccessRateTracker mCcaSuccessRateTracker;
    uint16_t           mCcaSampleCount;
    otMacCounters      mCounters;
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

#endif // MAC_EXTERN_HPP_
#endif // OPENTHREAD_CONFIG_USE_EXTERNAL_MAC
