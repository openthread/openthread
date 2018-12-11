/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes definitions for the spinel based radio transceiver.
 */

#ifndef RADIO_SPINEL_HPP_
#define RADIO_SPINEL_HPP_

#include <openthread/platform/radio.h>

#include "hdlc_interface.hpp"
#include "spinel.h"

namespace ot {
namespace PosixApp {

class RadioSpinel : public HdlcInterface::Callbacks
{
public:
    /**
     * This constructor initializes the spinel based OpenThread transceiver.
     *
     */
    RadioSpinel(void);

    /**
     * Initialize this radio transceiver.
     *
     * @param[in]   aRadioFile    The path to either a uart device or an executable.
     * @param[in]   aRadioConfig  Parameters given to the device or executable.
     *
     */
    void Init(const char *aRadioFile, const char *aRadioConfig);

    /**
     * Deinitialize this radio transceiver.
     *
     */
    void Deinit(void);

    /**
     * This method gets the status of promiscuous mode.
     *
     * @retval true   Promiscuous mode is enabled.
     * @retval false  Promiscuous mode is disabled.
     *
     */
    bool IsPromiscuous(void) const { return mIsPromiscuous; }

    /**
     * This method sets the status of promiscuous mode.
     *
     * @param[in]   aEnable     Whether to enable or disable promiscuous mode.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError SetPromiscuous(bool aEnable);

    /**
     * This method sets the Short Address for address filtering.
     *
     * @param[in] aShortAddress  The IEEE 802.15.4 Short Address.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError SetShortAddress(uint16_t aAddress);

    /**
     * This method gets the factory-assigned IEEE EUI-64 for this transceiver.
     *
     * @param[in]  aInstance   The OpenThread instance structure.
     * @param[out] aIeeeEui64  A pointer to the factory-assigned IEEE EUI-64.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError GetIeeeEui64(uint8_t *aIeeeEui64);

    /**
     * This method sets the Extended Address for address filtering.
     *
     * @param[in] aExtAddress  A pointer to the IEEE 802.15.4 Extended Address stored in little-endian byte order.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError SetExtendedAddress(const otExtAddress &aAddress);

    /**
     * This method sets the PAN ID for address filtering.
     *
     * @param[in]   aPanId  The IEEE 802.15.4 PAN ID.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError SetPanId(uint16_t aPanId);

    /**
     * This method gets the radio's transmit power in dBm.
     *
     * @param[out]  aPower    The transmit power in dBm.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError GetTransmitPower(int8_t &aPower);

    /**
     * This method sets the radio's transmit power in dBm.
     *
     * @param[in]   aPower     The transmit power in dBm.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError SetTransmitPower(int8_t aPower);

    /**
     * This method returns the radio sw version string.
     *
     * @returns A pointer to the radio version string.
     *
     */
    const char *GetVersion(void) const { return mVersion; }

    /**
     * This method returns the radio capabilities.
     *
     * @returns The radio capability bit vector.
     *
     */
    otRadioCaps GetRadioCaps(void) const { return mRadioCaps; }

    /**
     * This method gets the most recent RSSI measurement.
     *
     * @returns The RSSI in dBm when it is valid.  127 when RSSI is invalid.
     */
    int8_t GetRssi(void);

    /**
     * This method returns the radio receive sensitivity value.
     *
     * @returns The radio receive sensitivity value in dBm.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    int8_t GetReceiveSensitivity(void) const { return mRxSensitivity; }

    /**
     * This method returns a reference to the transmit buffer.
     *
     * The caller forms the IEEE 802.15.4 frame in this buffer then calls otPlatRadioTransmit() to request transmission.
     *
     * @returns A reference to the transmit buffer.
     *
     */
    otRadioFrame &GetTransmitFrame(void) { return mTxRadioFrame; }

    /**
     * This method enables or disables source address match feature.
     *
     * @param[in]  aEnable     Enable/disable source address match feature.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError EnableSrcMatch(bool aEnable);

    /**
     * This method adds a short address to the source address match table.
     *
     * @param[in]  aInstance      The OpenThread instance structure.
     * @param[in]  aShortAddress  The short address to be added.
     *
     * @retval  OT_ERROR_NONE               Successfully added short address to the source match table.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     * @retval  OT_ERROR_NO_BUFS            No available entry in the source match table.
     */
    otError AddSrcMatchShortEntry(const uint16_t aShortAddress);

    /**
     * This method removes a short address from the source address match table.
     *
     * @param[in]  aInstance      The OpenThread instance structure.
     * @param[in]  aShortAddress  The short address to be removed.
     *
     * @retval  OT_ERROR_NONE               Successfully removed short address from the source match table.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     * @retval  OT_ERROR_NO_ADDRESS         The short address is not in source address match table.
     */
    otError ClearSrcMatchShortEntry(const uint16_t aShortAddress);

    /**
     * Clear all short addresses from the source address match table.
     *
     * @param[in]  aInstance   The OpenThread instance structure.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError ClearSrcMatchShortEntries(void);

    /**
     * Add an extended address to the source address match table.
     *
     * @param[in]  aInstance    The OpenThread instance structure.
     * @param[in]  aExtAddress  The extended address to be added stored in little-endian byte order.
     *
     * @retval  OT_ERROR_NONE               Successfully added extended address to the source match table.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     * @retval  OT_ERROR_NO_BUFS            No available entry in the source match table.
     */
    otError AddSrcMatchExtEntry(const otExtAddress &aExtAddress);

    /**
     * Remove an extended address from the source address match table.
     *
     * @param[in]  aInstance    The OpenThread instance structure.
     * @param[in]  aExtAddress  The extended address to be removed stored in little-endian byte order.
     *
     * @retval  OT_ERROR_NONE               Successfully removed the extended address from the source match table.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     * @retval  OT_ERROR_NO_ADDRESS         The extended address is not in source address match table.
     */
    otError ClearSrcMatchExtEntry(const otExtAddress &aExtAddress);

    /**
     * Clear all the extended/long addresses from source address match table.
     *
     * @param[in]  aInstance   The OpenThread instance structure.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError ClearSrcMatchExtEntries(void);

    /**
     * This method begins the energy scan sequence on the radio.
     *
     * @param[in]  aScanChannel     The channel to perform the energy scan on.
     * @param[in]  aScanDuration    The duration, in milliseconds, for the channel to be scanned.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration);

    /**
     * This method switches the radio state from Receive to Transmit.
     *
     * @param[in] aFrame     A reference to the transmitted frame.
     *
     * @retval  OT_ERROR_NONE               Successfully transitioned to Transmit.
     * @retval  OT_ERROR_BUSY               Failed due to another transmission is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     * @retval OT_ERROR_INVALID_STATE The radio was not in the Receive state.
     */
    otError Transmit(otRadioFrame &aFrame);

    /**
     * This method switches the radio state from Sleep to Receive.
     *
     * @param[in]  aChannel   The channel to use for receiving.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Receive.
     * @retval OT_ERROR_INVALID_STATE The radio was disabled or transmitting.
     *
     */
    otError Receive(uint8_t aChannel);

    /**
     * This method switches the radio state from Receive to Sleep.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Sleep.
     * @retval OT_ERROR_BUSY          The radio was transmitting
     * @retval OT_ERROR_INVALID_STATE The radio was disabled
     *
     */
    otError Sleep(void);

    /**
     * Enable the radio.
     *
     * @param[in]   aInstance   A pointer to the OpenThread instance.
     *
     * @retval OT_ERROR_NONE     Successfully enabled.
     * @retval OT_ERROR_FAILED   The radio could not be enabled.
     *
     */
    otError Enable(otInstance *aInstance);

    /**
     * Disable the radio.
     *
     * @retval  OT_ERROR_NONE               Successfully transitioned to Disabled.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError Disable(void);

    /**
     * This method checks whether radio is enabled or not.
     *
     * @returns TRUE if the radio is enabled, FALSE otherwise.
     *
     */
    bool IsEnabled(void) const { return mState != kStateDisabled; }

    /**
     * This method updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[inout]  aReadFdSet   A reference to the read file descriptors.
     * @param[inout]  aWriteFdSet  A reference to the write file descriptors.
     * @param[inout]  aMaxFd       A reference to the max file descriptor.
     * @param[inout]  aTimeout     A reference to the timeout.
     *
     */
    void UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout);

    /**
     * This method performs radio driver processing.
     *
     * @param[in]   aReadFdSet      A reference to the read file descriptors.
     * @param[in]   aWriteFdSet     A reference to the write file descriptors.
     *
     */
    void Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet);

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    /**
     * This method performs radio spinel processing in simulation mode.
     *
     * @param[in]   aEvent  A reference to the current received simulation event.
     *
     */
    void Process(const struct Event &aEvent);

    /**
     * This method updates the @p aTimeout for processing radio spinel in simulation mode.
     *
     * @param[out]   aTimeout    A reference to the current timeout.
     *
     */
    void Update(struct timeval &aTimeout);
#endif

#if OPENTHREAD_ENABLE_DIAG
    /**
     * This method enables/disables the factory diagnostics mode.
     *
     * @param[in]  aMode  TRUE to enable diagnostics mode, FALSE otherwise.
     *
     */
    void SetDiagEnabled(bool aMode) { mDiagMode = aMode; }

    /**
     * This method indicates whether or not factory diagnostics mode is enabled.
     *
     * @returns TRUE if factory diagnostics mode is enabled, FALSE otherwise.
     *
     */
    bool IsDiagEnabled(void) const { return mDiagMode; }

    /**
     * This method processes platform diagnostics commands.
     *
     * @param[in]   aString         A NULL-terminated input string.
     * @param[out]  aOutput         The diagnostics execution result.
     * @param[in]   aOutputMaxLen   The output buffer size.
     *
     * @retval  OT_ERROR_NONE               Succeeded.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError PlatDiagProcess(const char *aString, char *aOutput, size_t aOutputMaxLen);
#endif

    /**
     *  This method processes a received Spinel frame.
     *
     * @param[in] aFrameBuffer The frame buffer constaining the newly received frame.
     */
    void HandleSpinelFrame(HdlcInterface::RxFrameBuffer &aFrameBuffer);

private:
    enum
    {
        kMaxSpinelFrame    = HdlcInterface::kMaxFrameSize,
        kMaxWaitTime       = 2000, ///< Max time to wait for response in milliseconds.
        kVersionStringSize = 128,  ///< Max size of version string.
        kCapsBufferSize    = 100,  ///< Max buffer size used to store `SPINEL_PROP_CAPS` value.
    };

    enum State
    {
        kStateDisabled,        ///< Radio is disabled.
        kStateSleep,           ///< Radio is sleep.
        kStateReceive,         ///< Radio is in receive mode.
        kStateTransmitPending, ///< Frame transmission requested, waiting to pass frame to radio.
        kStateTransmitting,    ///< Frame passed to radio for transmission, waiting for done event from radio.
        kStateTransmitDone,    ///< Radio indicated frame transmission is done.
    };

    otError CheckSpinelVersion(void);
    otError CheckCapabilities(void);
    otError CheckRadioCapabilities(void);
    void    ProcessFrameQueue(void);

    /**
     * This method tries to retrieve a spinel property from OpenThread transceiver.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to unpack property value.
     * @param[out]  ...         Variable arguments list.
     *
     * @retval  OT_ERROR_NONE               Successfully got the property.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError Get(spinel_prop_key_t aKey, const char *aFormat, ...);

    /**
     * This method tries to update a spinel property of OpenThread transceiver.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to pack property value.
     * @param[in]   ...         Variable arguments list.
     *
     * @retval  OT_ERROR_NONE               Successfully set the property.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError Set(spinel_prop_key_t aKey, const char *aFormat, ...);

    /**
     * This method tries to insert a item into a spinel list property of OpenThread transceiver.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to pack the item.
     * @param[in]   ...         Variable arguments list.
     *
     * @retval  OT_ERROR_NONE               Successfully insert item into the property.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError Insert(spinel_prop_key_t aKey, const char *aFormat, ...);

    /**
     * This method tries to remove a item from a spinel list property of OpenThread transceiver.
     *
     * @param[in]   aKey        Spinel property key.
     * @param[in]   aFormat     Spinel formatter to pack the item.
     * @param[in]   ...         Variable arguments list.
     *
     * @retval  OT_ERROR_NONE               Successfully removed item from the property.
     * @retval  OT_ERROR_BUSY               Failed due to another operation is on going.
     * @retval  OT_ERROR_RESPONSE_TIMEOUT   Failed due to no response received from the transceiver.
     *
     */
    otError Remove(spinel_prop_key_t aKey, const char *aFormat, ...);

    spinel_tid_t GetNextTid(void);
    void         FreeTid(spinel_tid_t tid) { mCmdTidsInUse &= ~(1 << tid); }

    otError RequestV(bool aWait, uint32_t aCommand, spinel_prop_key_t aKey, const char *aFormat, va_list aArgs);
    otError Request(bool aWait, uint32_t aCommand, spinel_prop_key_t aKey, const char *aFormat, ...);
    otError WaitResponse(void);
    otError SendReset(void);
    otError SendCommand(uint32_t          command,
                        spinel_prop_key_t key,
                        spinel_tid_t      tid,
                        const char *      pack_format,
                        va_list           args);
    otError ParseRadioFrame(otRadioFrame &aFrame, const uint8_t *aBuffer, uint16_t aLength);

    /**
     * This method returns if the property changed event is safe to be handled now.
     *
     * If a property handler will go up to core stack, it may cause reentrant issue of `Hdlc::Decode()` and
     * `WaitResponse()`.
     *
     * @param[in] aKey The identifier of the property.
     *
     * @returns Whether this property is safe to be handled now.
     *
     */
    bool IsSafeToHandleNow(spinel_prop_key_t aKey) const
    {
        return !((mHdlcInterface.IsDecoding() || mWaitingKey != SPINEL_PROP_LAST_STATUS) &&
                 (aKey == SPINEL_PROP_STREAM_RAW || aKey == SPINEL_PROP_MAC_ENERGY_SCAN_RESULT));
    }

    void HandleNotification(HdlcInterface::RxFrameBuffer &aFrameBuffer);
    void HandleNotification(const uint8_t *aBuffer, uint16_t aLength);
    void HandleValueIs(spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength);

    void HandleResponse(const uint8_t *aBuffer, uint16_t aLength);
    void HandleTransmitDone(uint32_t aCommand, spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength);
    void HandleWaitingResponse(uint32_t aCommand, spinel_prop_key_t aKey, const uint8_t *aBuffer, uint16_t aLength);

    void RadioReceive(void);
    void RadioTransmit(void);

    otInstance *mInstance;

    HdlcInterface mHdlcInterface;

    uint16_t          mCmdTidsInUse;    ///< Used transaction ids.
    spinel_tid_t      mCmdNextTid;      ///< Next available transaction id.
    spinel_tid_t      mTxRadioTid;      ///< The transaction id used to send a radio frame.
    spinel_tid_t      mWaitingTid;      ///< The transaction id of current transaction.
    spinel_prop_key_t mWaitingKey;      ///< The property key of current transaction.
    const char *      mPropertyFormat;  ///< The spinel property format of current transaction.
    va_list           mPropertyArgs;    ///< The arguments pack or unpack spinel property of current transaction.
    uint32_t          mExpectedCommand; ///< Expected response command of current transaction.
    otError           mError;           ///< The result of current transaction.

    uint8_t       mRxPsdu[OT_RADIO_FRAME_MAX_SIZE];
    uint8_t       mTxPsdu[OT_RADIO_FRAME_MAX_SIZE];
    uint8_t       mAckPsdu[OT_RADIO_FRAME_MAX_SIZE];
    otRadioFrame  mRxRadioFrame;
    otRadioFrame  mTxRadioFrame;
    otRadioFrame  mAckRadioFrame;
    otRadioFrame *mTransmitFrame; ///< Points to the frame to send

    otExtAddress mExtendedAddress;
    uint16_t     mShortAddress;
    uint16_t     mPanId;
    otRadioCaps  mRadioCaps;
    uint8_t      mChannel;
    int8_t       mRxSensitivity;
    otError      mTxError;
    char         mVersion[kVersionStringSize];

    State mState;
    bool  mIsPromiscuous : 1;     ///< Promiscuous mode.
    bool  mIsReady : 1;           ///< NCP ready.
    bool  mSupportsLogStream : 1; ///< RCP supports `LOG_STREAM` property with OpenThread log meta-data format.

#if OPENTHREAD_ENABLE_DIAG
    bool   mDiagMode;
    char * mDiagOutput;
    size_t mDiagOutputMaxLen;
#endif
};

} // namespace PosixApp
} // namespace ot

#endif // RADIO_SPINEL_HPP_
