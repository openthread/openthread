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
 *   This file includes definitions for MAC radio links.
 */

#ifndef MAC_LINKS_HPP_
#define MAC_LINKS_HPP_

#include "openthread-core-config.h"

#include "common/debug.hpp"
#include "common/locator.hpp"
#include "mac/mac_frame.hpp"
#include "mac/mac_types.hpp"
#include "mac/sub_mac.hpp"
#include "radio/radio.hpp"
#include "radio/trel_link.hpp"

namespace ot {
namespace Mac {

/**
 * @addtogroup core-mac
 *
 * @brief
 *   This module includes definitions for MAC radio links (multi radio).
 *
 * @{
 */

/**
 * Represents tx frames for different radio link types.
 */
class TxFrames : InstanceLocator
{
    friend class Links;

public:
#if OPENTHREAD_CONFIG_MULTI_RADIO
    /**
     * Gets the `TxFrame` for a given radio link type.
     *
     * Also updates the selected radio types (from `GetSelectedRadioTypes()`) to include the @p aRadioType.
     *
     * @param[in] aRadioType   A radio link type.
     *
     * @returns A reference to the `TxFrame` for the given radio link type.
     */
    TxFrame &GetTxFrame(RadioType aRadioType);

    /**
     * Gets the `TxFrame` with the smallest MTU size among a given set of radio types.
     *
     * Also updates the selected radio types (from `GetSelectedRadioTypes()`) to include the set
     * @p aRadioTypes.
     *
     * @param[in] aRadioTypes   A set of radio link types.
     *
     * @returns A reference to the `TxFrame` with the smallest MTU size among the set of @p aRadioTypes.
     */
    TxFrame &GetTxFrame(RadioTypes aRadioTypes);

    /**
     * Gets the `TxFrame` for sending a broadcast frame.
     *
     * Also updates the selected radio type (from `GetSelectedRadioTypes()`) to include all radio types
     * (supported by device).
     *
     * The broadcast frame is the `TxFrame` with the smallest MTU size among all radio types.
     *
     * @returns A reference to a `TxFrame` for broadcast.
     */
    TxFrame &GetBroadcastTxFrame(void);

    /**
     * Gets the selected radio types.
     *
     * This set specifies the radio links the frame should be sent over (in parallel). The set starts a empty after
     * method `Clear()` is called. It gets updated through calls to methods `GetTxFrame(aType)`,
     * `GetTxFrame(aRadioTypes)`, or `GetBroadcastTxFrame()`.
     *
     * @returns The selected radio types.
     */
    RadioTypes GetSelectedRadioTypes(void) const { return mSelectedRadioTypes; }

    /**
     * Gets the required radio types.
     *
     * This set specifies the radio links for which we expect the frame tx to be successful to consider the overall tx
     * successful. If the set is empty, successful tx over any radio link is sufficient for overall tx to be considered
     * successful. The required radio type set is expected to be a subset of selected radio types.
     *
     * The set starts as empty after `Clear()` call. It can be updated through `SetRequiredRadioTypes()` method
     *
     * @returns The required radio types.
     */
    RadioTypes GetRequiredRadioTypes(void) const { return mRequiredRadioTypes; }

    /**
     * Sets the required types.
     *
     * Please see `GetRequiredRadioTypes()` for more details on how this set is used during tx.
     *
     * @param[in] aRadioTypes   A set of radio link types.
     */
    void SetRequiredRadioTypes(RadioTypes aRadioTypes) { mRequiredRadioTypes = aRadioTypes; }

#else // #if OPENTHREAD_CONFIG_MULTI_RADIO

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    /**
     * Gets the tx frame.
     *
     * @returns A reference to `TxFrame`.
     */
    TxFrame &GetTxFrame(void) { return mTxFrame802154; }
#elif OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * Gets the tx frame.
     *
     * @returns A reference to `TxFrame`.
     */
    TxFrame &GetTxFrame(void) { return mTxFrameTrel; }
#endif
    /**
     * Gets a tx frame for sending a broadcast frame.
     *
     * @returns A reference to a `TxFrame` for broadcast.
     */
    TxFrame &GetBroadcastTxFrame(void) { return GetTxFrame(); }

#endif // #if OPENTHREAD_CONFIG_MULTI_RADIO

    /**
     * Clears all supported radio tx frames (sets the PSDU length to zero and clears flags).
     */
    void Clear(void)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mTxFrame802154.SetLength(0);
        mTxFrame802154.SetIsARetransmission(false);
        mTxFrame802154.SetIsSecurityProcessed(false);
        mTxFrame802154.SetCsmaCaEnabled(true); // Set to true by default, only set to `false` for CSL transmission
        mTxFrame802154.SetIsHeaderUpdated(false);
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        mTxFrame802154.SetTxDelay(0);
        mTxFrame802154.SetTxDelayBaseTime(0);
#endif
        mTxFrame802154.SetTxPower(kRadioPowerInvalid);
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        mTxFrame802154.SetCslIePresent(false);
#endif
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTxFrameTrel.SetLength(0);
        mTxFrameTrel.SetIsARetransmission(false);
        mTxFrameTrel.SetIsSecurityProcessed(false);
        mTxFrameTrel.SetCsmaCaEnabled(true);
        mTxFrameTrel.SetIsHeaderUpdated(false);
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
        mSelectedRadioTypes.Clear();
        mRequiredRadioTypes.Clear();
#endif
    }

    /**
     * Sets the channel on all supported radio tx frames.
     *
     * @param[in] aChannel  A channel.
     */
    void SetChannel(uint8_t aChannel)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mTxFrame802154.SetChannel(aChannel);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTxFrameTrel.SetChannel(aChannel);
#endif
    }

    /**
     * Sets the Sequence Number value on all supported radio tx frames.
     *
     * @param[in]  aSequence  The Sequence Number value.
     */
    void SetSequence(uint8_t aSequence)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mTxFrame802154.SetSequence(aSequence);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTxFrameTrel.SetSequence(aSequence);
#endif
    }

    /**
     * Sets the maximum number of the CSMA-CA backoffs on all supported radio tx
     * frames.
     *
     * @param[in]  aMaxCsmaBackoffs  The maximum number of CSMA-CA backoffs.
     */
    void SetMaxCsmaBackoffs(uint8_t aMaxCsmaBackoffs)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mTxFrame802154.SetMaxCsmaBackoffs(aMaxCsmaBackoffs);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTxFrameTrel.SetMaxCsmaBackoffs(aMaxCsmaBackoffs);
#endif
    }

    /**
     * Sets the maximum number of retries allowed after a transmission failure on all supported radio tx
     * frames.
     *
     * @param[in]  aMaxFrameRetries  The maximum number of retries allowed after a transmission failure.
     */
    void SetMaxFrameRetries(uint8_t aMaxFrameRetries)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mTxFrame802154.SetMaxFrameRetries(aMaxFrameRetries);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTxFrameTrel.SetMaxFrameRetries(aMaxFrameRetries);
#endif
    }

private:
    explicit TxFrames(Instance &aInstance);

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    TxFrame &mTxFrame802154;
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    TxFrame &mTxFrameTrel;
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    RadioTypes mSelectedRadioTypes;
    RadioTypes mRequiredRadioTypes;
#endif
};

/**
 * Represents MAC radio links (multi radio).
 */
class Links : public InstanceLocator
{
    friend class ot::Instance;

public:
    /**
     * Initializes the `Links` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit Links(Instance &aInstance);

    /**
     * Sets the PAN ID.
     *
     * @param[in] aPanId  The PAN ID.
     */
    void SetPanId(PanId aPanId)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mSubMac.SetPanId(aPanId);
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.SetPanId(aPanId);
#endif
    }

    /**
     * Gets the MAC Short Address.
     *
     * @returns The MAC Short Address.
     */
    ShortAddress GetShortAddress(void) const
    {
        return
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
            mSubMac.GetShortAddress();
#else
            mShortAddress;
#endif
    }

    /**
     * Sets the MAC Short Address.
     *
     * @param[in] aShortAddress   A MAC Short Address.
     */
    void SetShortAddress(ShortAddress aShortAddress)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mSubMac.SetShortAddress(aShortAddress);
#else
        mShortAddress = aShortAddress;
#endif
    }

    /**
     * Gets the MAC Extended Address.
     *
     * @returns The MAC Extended Address.
     */
    const ExtAddress &GetExtAddress(void) const
    {
        return
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
            mSubMac.GetExtAddress();
#else
            mExtAddress;
#endif
    }

    /**
     * Sets the MAC Extended Address.
     *
     * @param[in] aExtAddress  A MAC Extended Address.
     */
    void SetExtAddress(const ExtAddress &aExtAddress)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mSubMac.SetExtAddress(aExtAddress);
#else
        mExtAddress = aExtAddress;
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.HandleExtAddressChange();
#endif
    }

    /**
     * Registers a callback to provide received packet capture for IEEE 802.15.4 frames.
     *
     * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame
     *                               or nullptr to disable the callback.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     */
    void SetPcapCallback(otLinkPcapCallback aPcapCallback, void *aCallbackContext)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mSubMac.SetPcapCallback(aPcapCallback, aCallbackContext);
#endif
        OT_UNUSED_VARIABLE(aPcapCallback);
        OT_UNUSED_VARIABLE(aCallbackContext);
    }

    /**
     * Indicates whether radio should stay in Receive or Sleep during idle periods.
     *
     * @param[in]  aRxOnWhenIdle  TRUE to keep radio in Receive, FALSE to put to Sleep during idle periods.
     */
    void SetRxOnWhenIdle(bool aRxOnWhenIdle)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mSubMac.SetRxOnWhenIdle(aRxOnWhenIdle);
#endif
        OT_UNUSED_VARIABLE(aRxOnWhenIdle);
    }

    /**
     * Enables all radio links.
     */
    void Enable(void)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        IgnoreError(mSubMac.Enable());
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.Enable();
#endif
    }

    /**
     * Disables all radio links.
     */
    void Disable(void)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        IgnoreError(mSubMac.Disable());
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.Disable();
#endif
    }

    /**
     * Transitions all radio links to Sleep.
     */
    void Sleep(void)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        IgnoreError(mSubMac.Sleep());
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.Sleep();
#endif
    }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Configures CSL parameters in all radios.
     *
     * @param[in]  aPeriod    The CSL period.
     * @param[in]  aChannel   The CSL channel.
     * @param[in]  aShortAddr The short source address of CSL receiver's peer.
     * @param[in]  aExtAddr   The extended source address of CSL receiver's peer.
     *
     * @retval  TRUE if CSL Period or CSL Channel changed.
     * @retval  FALSE if CSL Period and CSL Channel did not change.
     */
    bool UpdateCsl(uint16_t aPeriod, uint8_t aChannel, otShortAddress aShortAddr, const otExtAddress *aExtAddr)
    {
        bool retval = false;

        OT_UNUSED_VARIABLE(aPeriod);
        OT_UNUSED_VARIABLE(aChannel);
        OT_UNUSED_VARIABLE(aShortAddr);
        OT_UNUSED_VARIABLE(aExtAddr);
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        retval = mSubMac.UpdateCsl(aPeriod, aChannel, aShortAddr, aExtAddr);
#endif
        return retval;
    }

    /**
     * Transitions all radios link to CSL sample state, given that a non-zero CSL period is configured.
     *
     * CSL sample state is only applicable and used for 15.4 radio link. Other link are transitioned to sleep state
     * when CSL period is non-zero.
     */
    void CslSample(void)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mSubMac.CslSample();
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.Sleep();
#endif
    }
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    /**
     * Configures wake-up listening parameters in all radios.
     *
     * @param[in]  aEnable    Whether to enable or disable wake-up listening.
     * @param[in]  aInterval  The wake-up listen interval in microseconds.
     * @param[in]  aDuration  The wake-up listen duration in microseconds.
     * @param[in]  aChannel   The wake-up channel.
     */
    void UpdateWakeupListening(bool aEnable, uint32_t aInterval, uint32_t aDuration, uint8_t aChannel)
    {
        OT_UNUSED_VARIABLE(aEnable);
        OT_UNUSED_VARIABLE(aInterval);
        OT_UNUSED_VARIABLE(aDuration);
        OT_UNUSED_VARIABLE(aChannel);
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        mSubMac.UpdateWakeupListening(aEnable, aInterval, aDuration, aChannel);
#endif
    }
#endif

    /**
     * Transitions all radio links to Receive.
     *
     * @param[in]  aChannel   The channel to use for receiving.
     */
    void Receive(uint8_t aChannel)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        IgnoreError(mSubMac.Receive(aChannel));
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.Receive(aChannel);
#endif
    }

    /**
     * Gets the radio transmit frames.
     *
     * @returns The transmit frames.
     */
    TxFrames &GetTxFrames(void) { return mTxFrames; }

#if !OPENTHREAD_CONFIG_MULTI_RADIO

    /**
     * Sends a prepared frame.
     *
     * The prepared frame is from `GetTxFrames()`. This method is available only in single radio link mode.
     */
    void Send(void)
    {
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
        SuccessOrAssert(mSubMac.Send());
#endif
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
        mTrel.Send();
#endif
    }

#else // #if !OPENTHREAD_CONFIG_MULTI_RADIO

    /**
     * Sends prepared frames over a given set of radio links.
     *
     * The prepared frame must be from `GetTxFrames()`. This method is available only in multi radio link mode.
     *
     * @param[in] aFrame       A reference to a prepared frame.
     * @param[in] aRadioTypes  A set of radio types to send on.
     */
    void Send(TxFrame &aFrame, RadioTypes aRadioTypes);

#endif // !OPENTHREAD_CONFIG_MULTI_RADIO

    /**
     * Gets the number of transmit retries for the last transmitted frame.
     *
     * @returns Number of transmit retries.
     */
    uint8_t GetTransmitRetries(void) const
    {
        return
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
            mSubMac.GetTransmitRetries();
#else
            0;
#endif
    }

    /**
     * Gets the most recent RSSI measurement from radio link.
     *
     * @returns The RSSI in dBm when it is valid. `Radio::kInvalidRssi` when RSSI is invalid.
     */
    int8_t GetRssi(void) const
    {
        return
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
            mSubMac.GetRssi();
#else
            Radio::kInvalidRssi;
#endif
    }

    /**
     * Begins energy scan.
     *
     * @param[in] aScanChannel   The channel to perform the energy scan on.
     * @param[in] aScanDuration  The duration, in milliseconds, for the channel to be scanned.
     *
     * @retval kErrorNone            Successfully started scanning the channel.
     * @retval kErrorBusy            The radio is performing energy scanning.
     * @retval kErrorInvalidState    The radio was disabled or transmitting.
     * @retval kErrorNotImplemented  Energy scan is not supported by radio link.
     */
    Error EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
    {
        OT_UNUSED_VARIABLE(aScanChannel);
        OT_UNUSED_VARIABLE(aScanDuration);

        return
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
            mSubMac.EnergyScan(aScanChannel, aScanDuration);
#else
            kErrorNotImplemented;
#endif
    }

    /**
     * Returns the noise floor value (currently use the radio receive sensitivity value).
     *
     * @returns The noise floor value in dBm.
     */
    int8_t GetNoiseFloor(void) const
    {
        return
#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
            mSubMac.GetNoiseFloor();
#else
            kDefaultNoiseFloor;
#endif
    }

    /**
     * Gets a reference to the `SubMac` instance.
     *
     * @returns A reference to the `SubMac` instance.
     */
    SubMac &GetSubMac(void) { return mSubMac; }

    /**
     * Gets a reference to the `SubMac` instance.
     *
     * @returns A reference to the `SubMac` instance.
     */
    const SubMac &GetSubMac(void) const { return mSubMac; }

    /**
     * Returns a reference to the current MAC key (for Key Mode 1) for a given Frame.
     *
     * @param[in] aFrame    The frame for which to get the MAC key.
     *
     * @returns A reference to the current MAC key.
     */
    const KeyMaterial *GetCurrentMacKey(const Frame &aFrame) const;

    /**
     * Returns a reference to the temporary MAC key (for Key Mode 1) for a given Frame based on a given
     * Key Sequence.
     *
     * @param[in] aFrame        The frame for which to get the MAC key.
     * @param[in] aKeySequence  The Key Sequence number (MUST be one off (+1 or -1) from current key sequence number).
     *
     * @returns A reference to the temporary MAC key.
     */
    const KeyMaterial *GetTemporaryMacKey(const Frame &aFrame, uint32_t aKeySequence) const;

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    /**
     * Sets the current MAC frame counter value from the value from a `TxFrame`.
     *
     * @param[in] TxFrame  The `TxFrame` from which to get the counter value.
     *
     * @retval kErrorNone            If successful.
     * @retval kErrorInvalidState    If the raw link-layer isn't enabled.
     */
    void SetMacFrameCounter(TxFrame &aFrame);
#endif

private:
    static constexpr int8_t kDefaultNoiseFloor = Radio::kDefaultReceiveSensitivity;

    SubMac mSubMac;
#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    Trel::Link mTrel;
#endif

    // `TxFrames` member definition should be after `mSubMac`, `mTrel`
    // definitions to allow it to use their methods from its
    // constructor.
    TxFrames mTxFrames;

#if !OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    ShortAddress mShortAddress;
    ExtAddress   mExtAddress;
#endif
};

/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // MAC_LINKS_HPP_
