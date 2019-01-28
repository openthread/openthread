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
 *   This file defines the radio interface for OpenThread.
 *
 */

#ifndef OPENTHREAD_PLATFORM_RADIO_H_
#define OPENTHREAD_PLATFORM_RADIO_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-radio
 *
 * @brief
 *   This module includes the platform abstraction for radio communication.
 *
 * @{
 *
 */

/**
 * @defgroup radio-types Types
 *
 * @brief
 *   This module includes the platform abstraction for a radio frame.
 *
 * @{
 *
 */

enum
{
    OT_RADIO_FRAME_MAX_SIZE     = 127,                            ///< aMaxPHYPacketSize (IEEE 802.15.4-2006)
    OT_RADIO_CHANNEL_PAGE       = 0,                              ///< 2.4 GHz IEEE 802.15.4-2006
    OT_RADIO_CHANNEL_MIN        = 11,                             ///< 2.4 GHz IEEE 802.15.4-2006
    OT_RADIO_CHANNEL_MAX        = 26,                             ///< 2.4 GHz IEEE 802.15.4-2006
    OT_RADIO_SUPPORTED_CHANNELS = 0xffff << OT_RADIO_CHANNEL_MIN, ///< 2.4 GHz IEEE 802.15.4-2006
    OT_RADIO_SYMBOLS_PER_OCTET  = 2,                              ///< 2.4 GHz IEEE 802.15.4-2006
    OT_RADIO_BIT_RATE           = 250000,                         ///< 2.4 GHz IEEE 802.15.4 (bits per second)

    OT_RADIO_BITS_PER_OCTET = 8, ///< Number of bits per octet
    OT_RADIO_SYMBOL_TIME    = ((OT_RADIO_BITS_PER_OCTET / OT_RADIO_SYMBOLS_PER_OCTET) * 1000000) / OT_RADIO_BIT_RATE,

    OT_RADIO_LQI_NONE     = 0,   ///< LQI measurement not supported
    OT_RADIO_RSSI_INVALID = 127, ///< Invalid or unknown RSSI value
};

/**
 * This type represents radio capabilities.
 *
 * The value is a bit-field indicating the capabilities supported by the radio. See `OT_RADIO_CAPS_*` definitions.
 *
 */
typedef uint8_t otRadioCaps;

/**
 * This enumeration defines constants that are used to indicate different radio capabilities. See `otRadioCaps`.
 *
 */
enum
{
    OT_RADIO_CAPS_NONE             = 0,      ///< Radio supports no capability.
    OT_RADIO_CAPS_ACK_TIMEOUT      = 1 << 0, ///< Radio supports AckTime event.
    OT_RADIO_CAPS_ENERGY_SCAN      = 1 << 1, ///< Radio supports Energy Scans.
    OT_RADIO_CAPS_TRANSMIT_RETRIES = 1 << 2, ///< Radio supports tx retry logic with collision avoidance (CSMA).
    OT_RADIO_CAPS_CSMA_BACKOFF     = 1 << 3, ///< Radio supports CSMA backoff for frame transmission (but no retry).
};

#define OT_PANID_BROADCAST 0xffff ///< IEEE 802.15.4 Broadcast PAN ID

/**
 * This type represents the IEEE 802.15.4 PAN ID.
 *
 */
typedef uint16_t otPanId;

/**
 * This type represents the IEEE 802.15.4 Short Address.
 *
 */
typedef uint16_t otShortAddress;

#define OT_EXT_ADDRESS_SIZE 8 ///< Size of an IEEE 802.15.4 Extended Address (bytes)

/**
 * @struct otExtAddress
 *
 * This structure represents the IEEE 802.15.4 Extended Address.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otExtAddress
{
    uint8_t m8[OT_EXT_ADDRESS_SIZE]; ///< IEEE 802.15.4 Extended Address bytes
} OT_TOOL_PACKED_END;

/**
 * This structure represents the IEEE 802.15.4 Extended Address.
 *
 */
typedef struct otExtAddress otExtAddress;

/**
 * This structure represents the IEEE 802.15.4 Header IE (Information Element) related information of a radio frame.
 */
typedef struct otRadioIeInfo
{
    uint8_t  mTimeIeOffset;      ///< The Time IE offset from the start of PSDU.
    uint8_t  mTimeSyncSeq;       ///< The Time sync sequence.
    uint64_t mTimestamp;         ///< The time in microseconds when the SFD was received.
    int64_t  mNetworkTimeOffset; ///< The time offset to the Thread network time.
} otRadioIeInfo;

/**
 * This structure represents an IEEE 802.15.4 radio frame.
 */
typedef struct otRadioFrame
{
    uint8_t *      mPsdu;    ///< The PSDU.
    uint8_t        mLength;  ///< Length of the PSDU.
    uint8_t        mChannel; ///< Channel used to transmit/receive the frame.
    otRadioIeInfo *mIeInfo;  ///< The pointer to the Header IE(s) related information.

    /**
     * The union of transmit and receive information for a radio frame.
     */
    union
    {
        /**
         * Structure representing radio frame transmit information.
         */
        struct
        {
            uint8_t        mMaxCsmaBackoffs;   ///< Maximum number of backoffs attempts before declaring CCA failure.
            uint8_t        mMaxFrameRetries;   ///< Maximum number of retries allowed after a transmission failure.
            bool           mIsARetx : 1;       ///< True if this frame is a retransmission (ignored by radio driver).
            bool           mCsmaCaEnabled : 1; ///< Set to true to enable CSMA-CA for this packet, false otherwise.
            const uint8_t *mAesKey;            ///< The key used for AES-CCM frame security.
        } mTxInfo;

        /**
         * Structure representing radio frame receive information.
         */
        struct
        {
            int8_t  mRssi; ///< Received signal strength indicator in dBm for received frames.
            uint8_t mLqi;  ///< Link Quality Indicator for received frames.

            /**
             * The timestamp when the frame was received (microseconds, the offset to mMsec).
             * Applicable/Required only when raw-link-api feature (`OPENTHREAD_ENABLE_RAW_LINK_API`) is enabled.
             *
             */
            uint16_t mUsec;

            /**
             * The timestamp when the frame was received (milliseconds).
             * Applicable/Required only when raw-link-api feature (`OPENTHREAD_ENABLE_RAW_LINK_API`) is enabled.
             *
             */
            uint32_t mMsec;
        } mRxInfo;
    } mInfo;
} otRadioFrame;

/**
 * This structure represents the state of a radio.
 * Initially, a radio is in the Disabled state.
 */
typedef enum otRadioState
{
    OT_RADIO_STATE_DISABLED = 0,
    OT_RADIO_STATE_SLEEP    = 1,
    OT_RADIO_STATE_RECEIVE  = 2,
    OT_RADIO_STATE_TRANSMIT = 3,
} otRadioState;

/**
 * The following are valid radio state transitions:
 *
 *                                    (Radio ON)
 *  +----------+  Enable()  +-------+  Receive() +---------+   Transmit()  +----------+
 *  |          |----------->|       |----------->|         |-------------->|          |
 *  | Disabled |            | Sleep |            | Receive |               | Transmit |
 *  |          |<-----------|       |<-----------|         |<--------------|          |
 *  +----------+  Disable() +-------+   Sleep()  +---------+   Receive()   +----------+
 *                                    (Radio OFF)                 or
 *                                                        signal TransmitDone
 */

/**
 * @}
 *
 */

/**
 * @defgroup radio-config Configuration
 *
 * @brief
 *   This module includes the platform abstraction for radio configuration.
 *
 * @{
 *
 */

/**
 * Get the radio capabilities.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The radio capability bit vector (see `OT_RADIO_CAP_*` definitions).
 *
 */
otRadioCaps otPlatRadioGetCaps(otInstance *aInstance);

/**
 * Get the radio version string.
 *
 * This is an optional radio driver platform function. If not provided by platform radio driver, OpenThread uses
 * the OpenThread version instead (@sa otGetVersionString()).
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 *
 * @returns A pointer to the OpenThread radio version.
 *
 */
const char *otPlatRadioGetVersionString(otInstance *aInstance);

/**
 * Get the radio receive sensitivity value.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The radio receive sensitivity value in dBm.
 *
 */
int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance);

/**
 * Get the factory-assigned IEEE EUI-64 for this interface.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[out] aIeeeEui64  A pointer to the factory-assigned IEEE EUI-64.
 *
 */
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64);

/**
 * Set the PAN ID for address filtering.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPanId     The IEEE 802.15.4 PAN ID.
 *
 */
void otPlatRadioSetPanId(otInstance *aInstance, otPanId aPanId);

/**
 * Set the Extended Address for address filtering.
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aExtAddress  A pointer to the IEEE 802.15.4 Extended Address stored in little-endian byte order.
 *
 *
 */
void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Set the Short Address for address filtering.
 *
 * @param[in] aInstance      The OpenThread instance structure.
 * @param[in] aShortAddress  The IEEE 802.15.4 Short Address.
 *
 */
void otPlatRadioSetShortAddress(otInstance *aInstance, otShortAddress aShortAddress);

/**
 * Get the radio's transmit power in dBm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[out] aPower    The transmit power in dBm.
 *
 * @retval OT_ERROR_NONE             Successfully retrieved the transmit power.
 * @retval OT_ERROR_INVALID_ARGS     @p aPower was NULL.
 * @retval OT_ERROR_NOT_IMPLEMENTED  Transmit power configuration via dBm is not implemented.
 *
 */
otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower);

/**
 * Set the radio's transmit power in dBm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPower     The transmit power in dBm.
 *
 * @retval OT_ERROR_NONE             Successfully set the transmit power.
 * @retval OT_ERROR_NOT_IMPLEMENTED  Transmit power configuration via dBm is not implemented.
 *
 */
otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower);

/**
 * Get the status of promiscuous mode.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval TRUE   Promiscuous mode is enabled.
 * @retval FALSE  Promiscuous mode is disabled.
 *
 */
bool otPlatRadioGetPromiscuous(otInstance *aInstance);

/**
 * Enable or disable promiscuous mode.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aEnable   TRUE to enable or FALSE to disable promiscuous mode.
 *
 */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable);

/**
 * @}
 *
 */

/**
 * @defgroup radio-operation Operation
 *
 * @brief
 *   This module includes the platform abstraction for radio operations.
 *
 * @{
 *
 */

/**
 * Get current state of the radio.
 *
 * This function is not required by OpenThread. It may be used for debugging and/or application-specific purposes.
 *
 * @note This function may be not implemented. It does not affect OpenThread.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @return  Current state of the radio.
 *
 */
otRadioState otPlatRadioGetState(otInstance *aInstance);

/**
 * Enable the radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE     Successfully enabled.
 * @retval OT_ERROR_FAILED   The radio could not be enabled.
 *
 */
otError otPlatRadioEnable(otInstance *aInstance);

/**
 * Disable the radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE  Successfully transitioned to Disabled.
 *
 */
otError otPlatRadioDisable(otInstance *aInstance);

/**
 * Check whether radio is enabled or not.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns TRUE if the radio is enabled, FALSE otherwise.
 *
 */
bool otPlatRadioIsEnabled(otInstance *aInstance);

/**
 * Transition the radio from Receive to Sleep (turn off the radio).
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Sleep.
 * @retval OT_ERROR_BUSY          The radio was transmitting.
 * @retval OT_ERROR_INVALID_STATE The radio was disabled.
 *
 */
otError otPlatRadioSleep(otInstance *aInstance);

/**
 * Transition the radio from Sleep to Receive (turn on the radio).
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aChannel   The channel to use for receiving.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Receive.
 * @retval OT_ERROR_INVALID_STATE The radio was disabled or transmitting.
 *
 */
otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel);

/**
 * The radio driver calls this method to notify OpenThread of a received frame.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aFrame    A pointer to the received frame or NULL if the receive operation failed.
 * @param[in]  aError    OT_ERROR_NONE when successfully received a frame,
 *                       OT_ERROR_ABORT when reception was aborted and a frame was not received,
 *                       OT_ERROR_NO_BUFS when a frame could not be received due to lack of rx buffer space.
 *
 */
extern void otPlatRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);

/**
 * The radio driver calls this method to notify OpenThread diagnostics module of a received frame.
 *
 * This function is used when diagnostics is enabled.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aFrame    A pointer to the received frame or NULL if the receive operation failed.
 * @param[in]  aError    OT_ERROR_NONE when successfully received a frame,
 *                       OT_ERROR_ABORT when reception was aborted and a frame was not received,
 *                       OT_ERROR_NO_BUFS when a frame could not be received due to lack of rx buffer space.
 *
 */
extern void otPlatDiagRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);

/**
 * Get the radio transmit frame buffer.
 *
 * OpenThread forms the IEEE 802.15.4 frame in this buffer then calls `otPlatRadioTransmit()` to request transmission.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns A pointer to the transmit frame buffer.
 *
 */
otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance);

/**
 * Begin the transmit sequence on the radio.
 *
 * The caller must form the IEEE 802.15.4 frame in the buffer provided by `otPlatRadioGetTransmitBuffer()` before
 * requesting transmission.  The channel and transmit power are also included in the otRadioFrame structure.
 *
 * The transmit sequence consists of:
 * 1. Transitioning the radio to Transmit from Receive.
 * 2. Transmits the psdu on the given channel and at the given transmit power.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aFrame     A pointer to the frame to be transmitted.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the Receive state.
 *
 */
otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame);

/**
 * The radio driver calls this method to notify OpenThread that the transmission has started.
 *
 * @note  This function should be called by the same thread that executes all of the other OpenThread code. It should
 *        not be called by ISR or any other task.
 *
 * @param[in]  aInstance  A pointer to the OpenThread instance structure.
 * @param[in]  aFrame     A pointer to the frame that is being transmitted.
 *
 */
extern void otPlatRadioTxStarted(otInstance *aInstance, otRadioFrame *aFrame);

/**
 * The radio driver calls this function to notify OpenThread that the transmit operation has completed,
 * providing both the transmitted frame and, if applicable, the received ack frame.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aFrame     A pointer to the frame that was transmitted.
 * @param[in]  aAckFrame  A pointer to the ACK frame, NULL if no ACK was received.
 * @param[in]  aError     OT_ERROR_NONE when the frame was transmitted,
 *                        OT_ERROR_NO_ACK when the frame was transmitted but no ACK was received,
 *                        OT_ERROR_CHANNEL_ACCESS_FAILURE tx could not take place due to activity on the channel,
 *                        OT_ERROR_ABORT when transmission was aborted for other reasons.
 *
 */
extern void otPlatRadioTxDone(otInstance *aInstance, otRadioFrame *aFrame, otRadioFrame *aAckFrame, otError aError);

/**
 * The radio driver calls this method to notify OpenThread diagnostics module that the transmission has completed.
 *
 * This function is used when diagnostics is enabled.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aFrame         A pointer to the frame that was transmitted.
 * @param[in]  aError         OT_ERROR_NONE when the frame was transmitted,
 *                            OT_ERROR_CHANNEL_ACCESS_FAILURE tx could not take place due to activity on the channel,
 *                            OT_ERROR_ABORT when transmission was aborted for other reasons.
 *
 */
extern void otPlatDiagRadioTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);

/**
 * The radio driver calls this method to notify OpenThread to process transmit security for the frame,
 * this happens when the frame includes Header IE(s) that were updated before transmission.
 *
 * This function is used when feature `OPENTHREAD_CONFIG_HEADER_IE_SUPPORT` is enabled.
 *
 * @note This function can be called from interrupt context and it would only read/write data passed in
 *       via @p aFrame, but would not read/write any state within OpenThread.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aFrame      The radio frame which needs to process transmit security.
 *
 */
extern void otPlatRadioFrameUpdated(otInstance *aInstance, otRadioFrame *aFrame);

/**
 * Get the most recent RSSI measurement.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The RSSI in dBm when it is valid.  127 when RSSI is invalid.
 *
 */
int8_t otPlatRadioGetRssi(otInstance *aInstance);

/**
 * Begin the energy scan sequence on the radio.
 *
 * This function is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability.
 *
 * @param[in] aInstance      The OpenThread instance structure.
 * @param[in] aScanChannel   The channel to perform the energy scan on.
 * @param[in] aScanDuration  The duration, in milliseconds, for the channel to be scanned.
 *
 * @retval OT_ERROR_NONE             Successfully started scanning the channel.
 * @retval OT_ERROR_NOT_IMPLEMENTED  The radio doesn't support energy scanning.
 *
 */
otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration);

/**
 * The radio driver calls this method to notify OpenThread that the energy scan is complete.
 *
 * This function is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability.
 *
 * @param[in]  aInstance           The OpenThread instance structure.
 * @param[in]  aEnergyScanMaxRssi  The maximum RSSI encountered on the scanned channel.
 *
 */
extern void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi);

/**
 * Enable/Disable source address match feature.
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
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aEnable     Enable/disable source address match feature.
 *
 */
void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable);

/**
 * Add a short address to the source address match table.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aShortAddress  The short address to be added.
 *
 * @retval OT_ERROR_NONE      Successfully added short address to the source match table.
 * @retval OT_ERROR_NO_BUFS   No available entry in the source match table.
 *
 */
otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress);

/**
 * Add an extended address to the source address match table.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aExtAddress  The extended address to be added stored in little-endian byte order.
 *
 * @retval OT_ERROR_NONE      Successfully added extended address to the source match table.
 * @retval OT_ERROR_NO_BUFS   No available entry in the source match table.
 *
 */
otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Remove a short address from the source address match table.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aShortAddress  The short address to be removed.
 *
 * @retval OT_ERROR_NONE        Successfully removed short address from the source match table.
 * @retval OT_ERROR_NO_ADDRESS  The short address is not in source address match table.
 *
 */
otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, otShortAddress aShortAddress);

/**
 * Remove an extended address from the source address match table.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aExtAddress  The extended address to be removed stored in little-endian byte order.
 *
 * @retval OT_ERROR_NONE        Successfully removed the extended address from the source match table.
 * @retval OT_ERROR_NO_ADDRESS  The extended address is not in source address match table.
 *
 */
otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Clear all short addresses from the source address match table.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 *
 */
void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance);

/**
 * Clear all the extended/long addresses from source address match table.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 *
 */
void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance);

/**
 * @}
 *
 */

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_RADIO_H_
