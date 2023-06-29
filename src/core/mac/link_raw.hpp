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
 *   This file includes definitions for the Raw Link-Layer class.
 */

#ifndef LINK_RAW_HPP_
#define LINK_RAW_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

#include <openthread/link_raw.h>

#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_frame.hpp"
#include "mac/sub_mac.hpp"

namespace ot {
namespace Mac {

/**
 * Defines the raw link-layer object.
 *
 */
class LinkRaw : public InstanceLocator, private NonCopyable
{
    friend class ot::Instance;

public:
    /**
     * Initializes the object.
     *
     * @param[in]   aInstance   A reference to the OpenThread instance.
     *
     */
    explicit LinkRaw(Instance &aInstance);

    /**
     * Initializes the states of the raw link-layer.
     *
     */
    void Init(void);

    /**
     * Returns true if the raw link-layer is enabled.
     *
     * @returns true if enabled, false otherwise.
     *
     */
    bool IsEnabled(void) const { return mReceiveDoneCallback != nullptr; }

    /**
     * Enables/disables the raw link-layer.
     *
     * @param[in]  aCallback  A pointer to a function called on receipt of a IEEE 802.15.4 frame, `nullptr` to disable
     *                        raw link-layer.
     *
     *
     * @retval kErrorInvalidState    Thread stack is enabled.
     * @retval kErrorFailed          The radio could not be enabled/disabled.
     * @retval kErrorNone            Successfully enabled/disabled raw link.
     *
     */
    Error SetReceiveDone(otLinkRawReceiveDone aCallback);

    /**
     * Returns the capabilities of the raw link-layer.
     *
     * @returns The radio capability bit vector.
     *
     */
    otRadioCaps GetCaps(void) const { return mSubMac.GetCaps(); }

    /**
     * Starts a (recurring) Receive on the link-layer.
     *
     * @retval kErrorNone            Successfully transitioned to Receive.
     * @retval kErrorInvalidState    The radio was disabled or transmitting.
     *
     */
    Error Receive(void);

    /**
     * Invokes the mReceiveDoneCallback, if set.
     *
     * @param[in]  aFrame    A pointer to the received frame or `nullptr` if the receive operation failed.
     * @param[in]  aError    kErrorNone when successfully received a frame,
     *                       kErrorAbort when reception was aborted and a frame was not received,
     *                       kErrorNoBufs when a frame could not be received due to lack of rx buffer space.
     *
     */
    void InvokeReceiveDone(RxFrame *aFrame, Error aError);

    /**
     * Gets the radio transmit frame.
     *
     * @returns The transmit frame.
     *
     */
    TxFrame &GetTransmitFrame(void) { return mSubMac.GetTransmitFrame(); }

    /**
     * Starts a (single) Transmit on the link-layer.
     *
     * @note The callback @p aCallback will not be called if this call does not return kErrorNone.
     *
     * @param[in]  aCallback            A pointer to a function called on completion of the transmission.
     *
     * @retval kErrorNone           Successfully transitioned to Transmit.
     * @retval kErrorInvalidState   The radio was not in the Receive state.
     *
     */
    Error Transmit(otLinkRawTransmitDone aCallback);

    /**
     * Invokes the mTransmitDoneCallback, if set.
     *
     * @param[in]  aFrame     The transmitted frame.
     * @param[in]  aAckFrame  A pointer to the ACK frame, `nullptr` if no ACK was received.
     * @param[in]  aError     kErrorNone when the frame was transmitted,
     *                        kErrorNoAck when the frame was transmitted but no ACK was received,
     *                        kErrorChannelAccessFailure tx failed due to activity on the channel,
     *                        kErrorAbort when transmission was aborted for other reasons.
     *
     */
    void InvokeTransmitDone(TxFrame &aFrame, RxFrame *aAckFrame, Error aError);

    /**
     * Starts a (single) Energy Scan on the link-layer.
     *
     * @param[in]  aScanChannel     The channel to perform the energy scan on.
     * @param[in]  aScanDuration    The duration, in milliseconds, for the channel to be scanned.
     * @param[in]  aCallback        A pointer to a function called on completion of a scanned channel.
     *
     * @retval kErrorNone            Successfully started scanning the channel.
     * @retval kErrorBusy            The radio is performing energy scanning.
     * @retval kErrorNotImplemented  The radio doesn't support energy scanning.
     * @retval kErrorInvalidState    If the raw link-layer isn't enabled.
     *
     */
    Error EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration, otLinkRawEnergyScanDone aCallback);

    /**
     * Invokes the mEnergyScanDoneCallback, if set.
     *
     * @param[in]   aEnergyScanMaxRssi  The max RSSI for energy scan.
     *
     */
    void InvokeEnergyScanDone(int8_t aEnergyScanMaxRssi);

    /**
     * Returns the short address.
     *
     * @returns short address.
     *
     */
    ShortAddress GetShortAddress(void) const { return mSubMac.GetShortAddress(); }

    /**
     * Updates short address.
     *
     * @param[in]   aShortAddress   The short address.
     *
     * @retval kErrorNone            If successful.
     * @retval kErrorInvalidState    If the raw link-layer isn't enabled.
     *
     */
    Error SetShortAddress(ShortAddress aShortAddress);

    /**
     * Returns PANID.
     *
     * @returns PANID.
     *
     */
    PanId GetPanId(void) const { return mPanId; }

    /**
     * Updates PANID.
     *
     * @param[in]   aPanId          The PANID.
     *
     * @retval kErrorNone            If successful.
     * @retval kErrorInvalidState    If the raw link-layer isn't enabled.
     *
     */
    Error SetPanId(PanId aPanId);

    /**
     * Gets the current receiving channel.
     *
     * @returns Current receiving channel.
     *
     */
    uint8_t GetChannel(void) const { return mReceiveChannel; }

    /**
     * Sets the receiving channel.
     *
     * @param[in]  aChannel     The channel to use for receiving.
     *
     */
    Error SetChannel(uint8_t aChannel);

    /**
     * Returns the extended address.
     *
     * @returns A reference to the extended address.
     *
     */
    const ExtAddress &GetExtAddress(void) const { return mSubMac.GetExtAddress(); }

    /**
     * Updates extended address.
     *
     * @param[in]   aExtAddress     The extended address.
     *
     * @retval kErrorNone            If successful.
     * @retval kErrorInvalidState    If the raw link-layer isn't enabled.
     *
     */
    Error SetExtAddress(const ExtAddress &aExtAddress);

    /**
     * Updates MAC keys and key index.
     *
     * @param[in]   aKeyIdMode        The key ID mode.
     * @param[in]   aKeyId            The key index.
     * @param[in]   aPrevKey          The previous MAC key.
     * @param[in]   aCurrKey          The current MAC key.
     * @param[in]   aNextKey          The next MAC key.
     *
     * @retval kErrorNone            If successful.
     * @retval kErrorFailed          Platform failed to import key.
     * @retval kErrorInvalidState    If the raw link-layer isn't enabled.
     *
     */
    Error SetMacKey(uint8_t aKeyIdMode, uint8_t aKeyId, const Key &aPrevKey, const Key &aCurrKey, const Key &aNextKey);

    /**
     * Sets the current MAC frame counter value.
     *
     * @param[in] aFrameCounter  The MAC frame counter value.
     * @param[in] aSetIfLarger   If `true`, set only if the new value @p aFrameCounter is larger than current value.
     *                           If `false`, set the new value independent of the current value.
     *
     * @retval kErrorNone            If successful.
     * @retval kErrorInvalidState    If the raw link-layer isn't enabled.
     *
     */
    Error SetMacFrameCounter(uint32_t aFrameCounter, bool aSetIfLarger);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    /**
     * Records the status of a frame transmission attempt and is mainly used for logging failures.
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
     *
     */
    void RecordFrameTransmitStatus(const TxFrame &aFrame, Error aError, uint8_t aRetryCount, bool aWillRetx);
#else
    void    RecordFrameTransmitStatus(const TxFrame &, Error, uint8_t, bool) {}
#endif

private:
    uint8_t                 mReceiveChannel;
    PanId                   mPanId;
    otLinkRawReceiveDone    mReceiveDoneCallback;
    otLinkRawTransmitDone   mTransmitDoneCallback;
    otLinkRawEnergyScanDone mEnergyScanDoneCallback;

#if OPENTHREAD_RADIO
    SubMac mSubMac;
#elif OPENTHREAD_CONFIG_LINK_RAW_ENABLE
    SubMac &mSubMac;
#endif
};

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_RADIO || OPENTHREAD_CONFIG_LINK_RAW_ENABLE

#endif // LINK_RAW_HPP_
