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
 *   This file includes definitions for OpenThread radio abstraction.
 */

#ifndef RADIO_HPP_
#define RADIO_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_frame.hpp"

namespace ot {

enum
{
    kUsPerTenSymbols = OT_US_PER_TEN_SYMBOLS, ///< The microseconds per 10 symbols.
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    kMinCslPeriod = OPENTHREAD_CONFIG_MAC_CSL_MIN_PERIOD * 1000 /
                    kUsPerTenSymbols, ///< Minimum CSL period supported in units of 10 symbols.
    kMaxCslTimeout = OPENTHREAD_CONFIG_MAC_CSL_MAX_TIMEOUT
#endif
};

/**
 * @addtogroup core-radio
 *
 * @brief
 *   This module includes definitions for OpenThread radio abstraction.
 *
 * @{
 *
 */

/**
 * This class represents an OpenThread radio abstraction.
 *
 */
class Radio : public InstanceLocator, private NonCopyable
{
    friend class Instance;

public:
    /**
     * This enumeration defines the IEEE 802.15.4 channel related parameters.
     *
     */
    enum
    {
#if (OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT && OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT)
        kNumChannelPages       = 2,
        kSupportedChannels     = OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK | OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK,
        kChannelMin            = OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN,
        kChannelMax            = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX,
        kSupportedChannelPages = OT_RADIO_CHANNEL_PAGE_0_MASK | OT_RADIO_CHANNEL_PAGE_2_MASK,
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
        kNumChannelPages       = 1,
        kSupportedChannels     = OT_RADIO_915MHZ_OQPSK_CHANNEL_MASK,
        kChannelMin            = OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN,
        kChannelMax            = OT_RADIO_915MHZ_OQPSK_CHANNEL_MAX,
        kSupportedChannelPages = OT_RADIO_CHANNEL_PAGE_2_MASK,
#elif OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
        kNumChannelPages       = 1,
        kSupportedChannels     = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK,
        kChannelMin            = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN,
        kChannelMax            = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX,
        kSupportedChannelPages = OT_RADIO_CHANNEL_PAGE_0_MASK,
#endif
    };

    static_assert((OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT || OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT),
                  "OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT or OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT "
                  "must be set to 1 to specify the radio mode");

    /**
     * This class defines the callbacks from `Radio`.
     *
     */
    class Callbacks : public InstanceLocator
    {
        friend class Radio;

    public:
        /**
         * This callback method handles a "Receive Done" event from radio platform.
         *
         * @param[in]  aFrame    A pointer to the received frame or nullptr if the receive operation failed.
         * @param[in]  aError    OT_ERROR_NONE when successfully received a frame,
         *                       OT_ERROR_ABORT when reception was aborted and a frame was not received,
         *                       OT_ERROR_NO_BUFS when a frame could not be received due to lack of rx buffer space.
         *
         */
        void HandleReceiveDone(Mac::RxFrame *aFrame, otError aError);

        /**
         * This callback method handles a "Transmit Started" event from radio platform.
         *
         * @param[in]  aFrame     The frame that is being transmitted.
         *
         */
        void HandleTransmitStarted(Mac::TxFrame &aFrame);

        /**
         * This callback method handles a "Transmit Done" event from radio platform.
         *
         * @param[in]  aFrame     The frame that was transmitted.
         * @param[in]  aAckFrame  A pointer to the ACK frame, nullptr if no ACK was received.
         * @param[in]  aError     OT_ERROR_NONE when the frame was transmitted,
         *                        OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
         *                        OT_ERROR_CHANNEL_ACCESS_FAILURE tx could not take place due to activity on the
         *                        channel, OT_ERROR_ABORT when transmission was aborted for other reasons.
         *
         */
        void HandleTransmitDone(Mac::TxFrame &aFrame, Mac::RxFrame *aAckFrame, otError aError);

        /**
         * This callback method handles "Energy Scan Done" event from radio platform.
         *
         * This method is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability. It is called from
         * `otPlatRadioEnergyScanDone()`.
         *
         * @param[in]  aInstance           The OpenThread instance structure.
         * @param[in]  aEnergyScanMaxRssi  The maximum RSSI encountered on the scanned channel.
         *
         */
        void HandleEnergyScanDone(int8_t aMaxRssi);

#if OPENTHREAD_CONFIG_DIAG_ENABLE
        /**
         * This callback method handles a "Receive Done" event from radio platform when diagnostics mode is enabled.
         *
         * @param[in]  aFrame    A pointer to the received frame or nullptr if the receive operation failed.
         * @param[in]  aError    OT_ERROR_NONE when successfully received a frame,
         *                       OT_ERROR_ABORT when reception was aborted and a frame was not received,
         *                       OT_ERROR_NO_BUFS when a frame could not be received due to lack of rx buffer space.
         *
         */
        void HandleDiagsReceiveDone(Mac::RxFrame *aFrame, otError aError);

        /**
         * This callback method handles a "Transmit Done" event from radio platform when diagnostics mode is enabled.
         *
         * @param[in]  aFrame     The frame that was transmitted.
         * @param[in]  aError     OT_ERROR_NONE when the frame was transmitted,
         *                        OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
         *                        OT_ERROR_CHANNEL_ACCESS_FAILURE tx could not take place due to activity on the
         *                        channel, OT_ERROR_ABORT when transmission was aborted for other reasons.
         *
         */
        void HandleDiagsTransmitDone(Mac::TxFrame &aFrame, otError aError);
#endif

    private:
        explicit Callbacks(Instance &aInstance)
            : InstanceLocator(aInstance)
        {
        }
    };

    /**
     * This constructor initializes the `Radio` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    Radio(Instance &aInstance)
        : InstanceLocator(aInstance)
        , mCallbacks(aInstance)
    {
    }

    /**
     * This method gets the radio capabilities.
     *
     * @returns The radio capability bit vector (see `OT_RADIO_CAP_*` definitions).
     *
     */
    otRadioCaps GetCaps(void) { return otPlatRadioGetCaps(GetInstance()); }

    /**
     * This method gets the radio version string.
     *
     * @returns A pointer to the OpenThread radio version.
     *
     */
    const char *GetVersionString(void) { return otPlatRadioGetVersionString(GetInstance()); }

    /**
     * This method gets the radio receive sensitivity value.
     *
     * @returns The radio receive sensitivity value in dBm.
     *
     */
    int8_t GetReceiveSensitivity(void) { return otPlatRadioGetReceiveSensitivity(GetInstance()); }

    /**
     * This method gets the factory-assigned IEEE EUI-64 for the device.
     *
     * @param[out] aIeeeEui64  A reference to `Mac::ExtAddress` to place the factory-assigned IEEE EUI-64.
     *
     */
    void GetIeeeEui64(Mac::ExtAddress &aIeeeEui64) { otPlatRadioGetIeeeEui64(GetInstance(), aIeeeEui64.m8); }

    /**
     * This method sets the PAN ID for address filtering.
     *
     * @param[in] aPanId     The IEEE 802.15.4 PAN ID.
     *
     */
    void SetPanId(Mac::PanId aPanId) { otPlatRadioSetPanId(GetInstance(), aPanId); }

    /**
     * This method sets the Extended Address for address filtering.
     *
     * @param[in] aExtAddress  The IEEE 802.15.4 Extended Address stored in little-endian byte order.
     *
     */
    void SetExtendedAddress(const Mac::ExtAddress &aExtAddress);

    /**
     * This method sets the Short Address for address filtering.
     *
     * @param[in] aShortAddress  The IEEE 802.15.4 Short Address.
     *
     */
    void SetShortAddress(Mac::ShortAddress aShortAddress);

    /**
     * This method sets MAC key and key ID.
     *
     * @param[in] aKeyIdMode  MAC key ID mode.
     * @param[in] aKeyId      Current MAC key index.
     * @param[in] aPrevKey    The previous MAC key.
     * @param[in] aCurrKey    The current MAC key.
     * @param[in] aNextKey    The next MAC key.
     *
     */
    void SetMacKey(uint8_t         aKeyIdMode,
                   uint8_t         aKeyId,
                   const Mac::Key &aPrevKey,
                   const Mac::Key &aCurrKey,
                   const Mac::Key &aNextKey)
    {
        otPlatRadioSetMacKey(GetInstance(), aKeyIdMode, aKeyId, &aPrevKey, &aCurrKey, &aNextKey);
    }

    /**
     * This method sets the current MAC Frame Counter value.
     *
     * @param[in] aMacFrameCounter  The MAC Frame Counter value.
     *
     */
    void SetMacFrameCounter(uint32_t aMacFrameCounter)
    {
        otPlatRadioSetMacFrameCounter(GetInstance(), aMacFrameCounter);
    }

    /**
     * This method gets the radio's transmit power in dBm.
     *
     * @param[out] aPower    A reference to output the transmit power in dBm.
     *
     * @retval OT_ERROR_NONE             Successfully retrieved the transmit power.
     * @retval OT_ERROR_NOT_IMPLEMENTED  Transmit power configuration via dBm is not implemented.
     *
     */
    otError GetTransmitPower(int8_t &aPower) { return otPlatRadioGetTransmitPower(GetInstance(), &aPower); }

    /**
     * This method sets the radio's transmit power in dBm.
     *
     * @param[in] aPower     The transmit power in dBm.
     *
     * @retval OT_ERROR_NONE             Successfully set the transmit power.
     * @retval OT_ERROR_NOT_IMPLEMENTED  Transmit power configuration via dBm is not implemented.
     *
     */
    otError SetTransmitPower(int8_t aPower) { return otPlatRadioSetTransmitPower(GetInstance(), aPower); }

    /**
     * This method gets the radio's CCA ED threshold in dBm.
     *
     * @param[in] aThreshold    The CCA ED threshold in dBm.
     *
     * @retval OT_ERROR_NONE             A reference to output the CCA ED threshold in dBm.
     * @retval OT_ERROR_NOT_IMPLEMENTED  CCA ED threshold configuration via dBm is not implemented.
     *
     */
    otError GetCcaEnergyDetectThreshold(int8_t &aThreshold)
    {
        return otPlatRadioGetCcaEnergyDetectThreshold(GetInstance(), &aThreshold);
    }

    /**
     * This method sets the radio's CCA ED threshold in dBm.
     *
     * @param[in] aThreshold    The CCA ED threshold in dBm.
     *
     * @retval OT_ERROR_NONE             Successfully set the CCA ED threshold.
     * @retval OT_ERROR_NOT_IMPLEMENTED  CCA ED threshold configuration via dBm is not implemented.
     *
     */
    otError SetCcaEnergyDetectThreshold(int8_t aThreshold)
    {
        return otPlatRadioSetCcaEnergyDetectThreshold(GetInstance(), aThreshold);
    }

    /**
     * This method gets the status of promiscuous mode.
     *
     * @retval TRUE   Promiscuous mode is enabled.
     * @retval FALSE  Promiscuous mode is disabled.
     *
     */
    bool GetPromiscuous(void) { return otPlatRadioGetPromiscuous(GetInstance()); }

    /**
     * This method enables or disables promiscuous mode.
     *
     * @param[in]  aEnable   TRUE to enable or FALSE to disable promiscuous mode.
     *
     */
    void SetPromiscuous(bool aEnable) { otPlatRadioSetPromiscuous(GetInstance(), aEnable); }

    /**
     * This method returns the current state of the radio.
     *
     * This function is not required by OpenThread. It may be used for debugging and/or application-specific purposes.
     *
     * @note This function may be not implemented. In this case it always returns OT_RADIO_STATE_INVALID state.
     *
     * @return  Current state of the radio.
     *
     */
    otRadioState GetState(void) { return otPlatRadioGetState(GetInstance()); }

    /**
     * This method enables the radio.
     *
     * @retval OT_ERROR_NONE     Successfully enabled.
     * @retval OT_ERROR_FAILED   The radio could not be enabled.
     *
     */
    otError Enable(void) { return otPlatRadioEnable(GetInstance()); }

    /**
     * This method disables the radio.
     *
     * @retval OT_ERROR_NONE            Successfully transitioned to Disabled.
     * @retval OT_ERROR_INVALID_STATE   The radio was not in sleep state.
     *
     */
    otError Disable(void) { return otPlatRadioDisable(GetInstance()); }

    /**
     * This method indicates whether radio is enabled or not.
     *
     * @returns TRUE if the radio is enabled, FALSE otherwise.
     *
     */
    bool IsEnabled(void) { return otPlatRadioIsEnabled(GetInstance()); }

    /**
     * This method transitions the radio from Receive to Sleep (turn off the radio).
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Sleep.
     * @retval OT_ERROR_BUSY          The radio was transmitting.
     * @retval OT_ERROR_INVALID_STATE The radio was disabled.
     *
     */
    otError Sleep(void) { return otPlatRadioSleep(GetInstance()); }

    /**
     * This method transitions the radio from Sleep to Receive (turn on the radio).
     *
     * @param[in]  aChannel   The channel to use for receiving.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Receive.
     * @retval OT_ERROR_INVALID_STATE The radio was disabled or transmitting.
     *
     */
    otError Receive(uint8_t aChannel) { return otPlatRadioReceive(GetInstance(), aChannel); }

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * This method updates the CSL sample time in radio.
     *
     * @param[in]  aCslSampleTime  The CSL sample time.
     *
     */
    void UpdateCslSampleTime(uint32_t aCslSampleTime) { otPlatRadioUpdateCslSampleTime(GetInstance(), aCslSampleTime); }

    /** This method enables CSL sampling in radio.
     *
     * @param[in]  aCslPeriod    CSL period, 0 for disabling CSL.
     * @param[in]  aExtAddr      The extended source address of CSL receiver's parent device (when the platforms
     * generate enhanced ack, platforms may need to know acks to which address should include CSL IE).
     *
     * @retval  OT_ERROR_NOT_SUPPORTED  Radio driver doesn't support CSL.
     * @retval  OT_ERROR_FAILED         Other platform specific errors.
     * @retval  OT_ERROR_NONE           Successfully enabled or disabled CSL.
     *
     */
    otError EnableCsl(uint32_t aCslPeriod, const otExtAddress *aExtAddr)
    {
        return otPlatRadioEnableCsl(GetInstance(), aCslPeriod, aExtAddr);
    }
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

    /**
     * This method gets the radio transmit frame buffer.
     *
     * OpenThread forms the IEEE 802.15.4 frame in this buffer then calls `Transmit()` to request transmission.
     *
     * @returns A reference to the transmit frame buffer.
     *
     */
    Mac::TxFrame &GetTransmitBuffer(void)
    {
        return *static_cast<Mac::TxFrame *>(otPlatRadioGetTransmitBuffer(GetInstance()));
    }

    /**
     * This method starts the transmit sequence on the radio.
     *
     * The caller must form the IEEE 802.15.4 frame in the buffer provided by `GetTransmitBuffer()` before
     * requesting transmission.  The channel and transmit power are also included in the frame.
     *
     * @param[in] aFrame     A reference to the frame to be transmitted.
     *
     * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
     * @retval OT_ERROR_INVALID_STATE The radio was not in the Receive state.
     *
     */
    otError Transmit(Mac::TxFrame &aFrame);

    /**
     * This method gets the most recent RSSI measurement.
     *
     * @returns The RSSI in dBm when it is valid.  127 when RSSI is invalid.
     *
     */
    int8_t GetRssi(void) { return otPlatRadioGetRssi(GetInstance()); }

    /**
     * This method begins the energy scan sequence on the radio.
     *
     * This function is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability.
     *
     * @param[in] aScanChannel   The channel to perform the energy scan on.
     * @param[in] aScanDuration  The duration, in milliseconds, for the channel to be scanned.
     *
     * @retval OT_ERROR_NONE             Successfully started scanning the channel.
     * @retval OT_ERROR_NOT_IMPLEMENTED  The radio doesn't support energy scanning.
     *
     */
    otError EnergyScan(uint8_t aScanChannel, uint16_t aScanDuration)
    {
        return otPlatRadioEnergyScan(GetInstance(), aScanChannel, aScanDuration);
    }

    /**
     * This method enables/disables source address match feature.
     *
     * The source address match feature controls how the radio layer decides the "frame pending" bit for acks sent in
     * response to data request commands from children.
     *
     * If disabled, the radio layer must set the "frame pending" on all acks to data request commands.
     *
     * If enabled, the radio layer uses the source address match table to determine whether to set or clear the "frame
     * pending" bit in an ack to a data request command.
     *
     * The source address match table provides the list of children for which there is a pending frame. Either a short
     * address or an extended/long address can be added to the source address match table.
     *
     * @param[in]  aEnable     Enable/disable source address match feature.
     *
     */
    void EnableSrcMatch(bool aEnable) { return otPlatRadioEnableSrcMatch(GetInstance(), aEnable); }

    /**
     * This method adds a short address to the source address match table.
     *
     * @param[in]  aShortAddress  The short address to be added.
     *
     * @retval OT_ERROR_NONE      Successfully added short address to the source match table.
     * @retval OT_ERROR_NO_BUFS   No available entry in the source match table.
     *
     */
    otError AddSrcMatchShortEntry(Mac::ShortAddress aShortAddress)
    {
        return otPlatRadioAddSrcMatchShortEntry(GetInstance(), aShortAddress);
    }

    /**
     * This method adds an extended address to the source address match table.
     *
     * @param[in]  aExtAddress  The extended address to be added stored in little-endian byte order.
     *
     * @retval OT_ERROR_NONE      Successfully added extended address to the source match table.
     * @retval OT_ERROR_NO_BUFS   No available entry in the source match table.
     *
     */
    otError AddSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress)
    {
        return otPlatRadioAddSrcMatchExtEntry(GetInstance(), &aExtAddress);
    }

    /**
     * This method removes a short address from the source address match table.
     *
     * @param[in]  aShortAddress  The short address to be removed.
     *
     * @retval OT_ERROR_NONE        Successfully removed short address from the source match table.
     * @retval OT_ERROR_NO_ADDRESS  The short address is not in source address match table.
     *
     */
    otError ClearSrcMatchShortEntry(Mac::ShortAddress aShortAddress)
    {
        return otPlatRadioClearSrcMatchShortEntry(GetInstance(), aShortAddress);
    }

    /**
     * This method removes an extended address from the source address match table.
     *
     * @param[in]  aExtAddress  The extended address to be removed stored in little-endian byte order.
     *
     * @retval OT_ERROR_NONE        Successfully removed the extended address from the source match table.
     * @retval OT_ERROR_NO_ADDRESS  The extended address is not in source address match table.
     *
     */
    otError ClearSrcMatchExtEntry(const Mac::ExtAddress &aExtAddress)
    {
        return otPlatRadioClearSrcMatchExtEntry(GetInstance(), &aExtAddress);
    }

    /**
     * This method clears all short addresses from the source address match table.
     *
     */
    void ClearSrcMatchShortEntries(void) { otPlatRadioClearSrcMatchShortEntries(GetInstance()); }

    /**
     * This method clears all the extended/long addresses from source address match table.
     *
     */
    void ClearSrcMatchExtEntries(void) { otPlatRadioClearSrcMatchExtEntries(GetInstance()); }

    /**
     * This method gets the radio supported channel mask that the device is allowed to be on.
     *
     * @returns The radio supported channel mask.
     *
     */
    uint32_t GetSupportedChannelMask(void) { return otPlatRadioGetSupportedChannelMask(GetInstance()); }

    /**
     * This method gets the radio preferred channel mask that the device prefers to form on.
     *
     * @returns The radio preferred channel mask.
     *
     */
    uint32_t GetPreferredChannelMask(void) { return otPlatRadioGetPreferredChannelMask(GetInstance()); }

    /**
     * This method checks if a given channel is valid as a CSL channel.
     *
     * @retval true   The channel is valid.
     * @retval false  The channel is invalid.
     *
     */
    static bool IsCslChannelValid(uint8_t aCslChannel)
    {
        return (aCslChannel == 0) || ((kChannelMin <= aCslChannel) && (aCslChannel <= kChannelMax));
    }

private:
    otInstance *GetInstance(void) { return reinterpret_cast<otInstance *>(&InstanceLocator::GetInstance()); }

    Callbacks mCallbacks;
};

} // namespace ot

#endif // RADIO_HPP_
