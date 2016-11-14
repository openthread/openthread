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

#ifndef RADIO_H_
#define RADIO_H_

#include <stdint.h>

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup radio Radio
 * @ingroup platform
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
 *   This module includes the platform abstraction for a radio packet.
 *
 * @{
 *
 */

enum
{
    kMaxPHYPacketSize        = 127,                       ///< aMaxPHYPacketSize (IEEE 802.15.4-2006)
    kPhyMinChannel           = 11,                        ///< 2.4 GHz IEEE 802.15.4-2006
    kPhyMaxChannel           = 26,                        ///< 2.4 GHz IEEE 802.15.4-2006
    kPhySupportedChannelMask = 0xffff << kPhyMinChannel,  ///< 2.4 GHz IEEE 802.15.4-2006
    kPhySymbolsPerOctet      = 2,                         ///< 2.4 GHz IEEE 802.15.4-2006
    kPhyBitRate              = 250000,                    ///< 2.4 GHz IEEE 802.15.4 (kilobits per second)

    kPhyBitsPerOctet    = 8,
    kPhyUsPerSymbol     = ((kPhyBitsPerOctet / kPhySymbolsPerOctet) * 1000000) / kPhyBitRate,

    kPhyNoLqi           = 0,       ///< LQI measurement not supported
    kPhyInvalidRssi     = 127,     ///< Invalid or unknown RSSI value
};

/**
 *   This enum represents radio capabilities.
 *
 */

typedef enum otRadioCaps
{
    kRadioCapsNone              = 0,  ///< None
    kRadioCapsAckTimeout        = 1,  ///< Radio supports AckTime event
    kRadioCapsEnergyScan        = 2,  ///< Radio supports Enegery Scans
    kRadioCapsTransmitRetries   = 4,  ///< Radio supports transmission retry logic with collission avoidance
} otRadioCaps;

/**
 * This structure represents an IEEE 802.15.4 radio frame.
 */
typedef struct RadioPacket
{
    uint8_t  *mPsdu;           ///< The PSDU.
    uint8_t  mLength;          ///< Length of the PSDU.
    uint8_t  mChannel;         ///< Channel used to transmit/receive the frame.
    int8_t   mPower;           ///< Transmit/receive power in dBm.
    uint8_t  mLqi;             ///< Link Quality Indicator for received frames.
    bool     mSecurityValid: 1; ///< Security Enabled flag is set and frame passes security checks.
    bool     mDidTX: 1;        ///< Set to true if this packet sent from the radio. Ignored by radio driver.
} RadioPacket;

/**
 * This structure represents the state of a radio.
 * Initially, a radio is in the Disabled state.
 */
typedef enum PhyState
{
    kStateDisabled = 0,
    kStateSleep = 1,
    kStateReceive = 2,
    kStateTransmit = 3
} PhyState;

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
 * Get the factory-assigned IEEE EUI-64 for this interface.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[out] aIeeeEui64  A pointer to where the factory-assigned IEEE EUI-64 will be placed.
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
void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId);

/**
 * Set the Extended Address for address filtering.
 *
 * @param[in] aInstance         The OpenThread instance structure.
 * @param[in] aExtendedAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 */
void otPlatRadioSetExtendedAddress(otInstance *aInstance, uint8_t *aExtendedAddress);

/**
 * Set the Short Address for address filtering.
 *
 * @param[in] aInstance      The OpenThread instance structure.
 * @param[in] aShortAddress  The IEEE 802.15.4 Short Address.
 *
 */
void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aShortAddress);

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
 * Enable the radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::kThreadError_None     Successfully enabled.
 * @retval ::kThreadError_Failure  The radio could not be enabled.
 */
ThreadError otPlatRadioEnable(otInstance *aInstance);

/**
 * Disable the radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::kThreadError_None  Successfully transitioned to Disabled.
 */
ThreadError otPlatRadioDisable(otInstance *aInstance);

/**
 * Check whether radio is enabled or not.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::true   radio is enabled.
 * @retval ::false  radio is disabled.
 */
bool otPlatRadioIsEnabled(otInstance *aInstance);

/**
 * Transition the radio from Receive to Sleep.
 * Turn off the radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval ::kThreadError_None         Successfully transitioned to Sleep.
 * @retval ::kThreadError_Busy         The radio was transmitting
 * @retval ::kThreadError_InvalidState The radio was disabled
 */
ThreadError otPlatRadioSleep(otInstance *aInstance);

/**
 * Transitioning the radio from Sleep to Receive.
 * Turn on the radio.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 * @param[in]  aChannel   The channel to use for receiving.
 *
 * @retval ::kThreadError_None         Successfully transitioned to Receive.
 * @retval ::kThreadError_InvalidState The radio was disabled or transmitting.
 */
ThreadError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel);

/**
 * Enable/Disable source match for AutoPend.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aEnable     Enable/disable source match for automatical pending.
 */
void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable);

/**
 * Adding short address to the source match table.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aShortAddress  The short address to be added.
 *
 * @retval ::kThreadError_None     Successfully added short address to the source match table.
 * @retval ::kThreadError_NoBufs   No available entry in the source match table.
 */
ThreadError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress);

/**
 * Adding extended address to the source match table.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aExtAddress  The extended address to be added.
 *
 * @retval ::kThreadError_None     Successfully added extended address to the source match table.
 * @retval ::kThreadError_NoBufs   No available entry in the source match table.
 */
ThreadError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress);

/**
 * Removing short address to the source match table.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aShortAddress  The short address to be removed.
 *
 * @retval ::kThreadError_None        Successfully removed short address from the source match table.
 * @retval ::kThreadError_NoAddress   The short address is not in source match table.
 */
ThreadError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress);

/**
 * Removing extended address to the source match table of the radio.
 *
 * @param[in]  aInstance    The OpenThread instance structure.
 * @param[in]  aExtAddress  The extended address to be removed.
 *
 * @retval ::kThreadError_None        Successfully removed the extended address from the source match table.
 * @retval ::kThreadError_NoAddress   The extended address is not in source match table.
 */
ThreadError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const uint8_t *aExtAddress);

/**
 * Removing all the short addresses from the source match table.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 *
 */
void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance);

/**
 * Removing all the extended addresses from the source match table.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 *
 */
void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance);

/**
 * The radio driver calls this method to notify OpenThread of a received packet.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aPacket   A pointer to the received packet or NULL if the receive operation was aborted.
 * @param[in]  aError    ::kThreadError_None when successfully received a frame, ::kThreadError_Abort when reception
 *                       was aborted and a frame was not received.
 *
 */
extern void otPlatRadioReceiveDone(otInstance *aInstance, RadioPacket *aPacket, ThreadError aError);

/**
 * The radio transitions from Transmit to Receive.
 * This method returns a pointer to the transmit buffer.
 *
 * The caller forms the IEEE 802.15.4 frame in this buffer then calls otPlatRadioTransmit() to request transmission.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns A pointer to the transmit buffer.
 *
 */
RadioPacket *otPlatRadioGetTransmitBuffer(otInstance *aInstance);

/**
 * This method begins the transmit sequence on the radio.
 *
 * The caller must form the IEEE 802.15.4 frame in the buffer provided by otPlatRadioGetTransmitBuffer() before
 * requesting transmission.  The channel and transmit power are also included in the RadioPacket structure.
 *
 * The transmit sequence consists of:
 * 1. Transitioning the radio to Transmit from Receive.
 * 2. Transmits the psdu on the given channel and at the given transmit power.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPacket    A pointer to the packet that will be transmitted.
 *
 * @retval ::kThreadError_None         Successfully transitioned to Transmit.
 * @retval ::kThreadError_InvalidState The radio was not in the Receive state.
 */
ThreadError otPlatRadioTransmit(otInstance *aInstance, RadioPacket *aPacket);

/**
 * The radio driver calls this method to notify OpenThread that the transmission has completed.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aPacket        A pointer to the packet that was transmitted.
 * @param[in]  aFramePending  TRUE if an ACK frame was received and the Frame Pending bit was set.
 * @param[in]  aError  ::kThreadError_None when the frame was transmitted, ::kThreadError_NoAck when the frame was
 *                     transmitted but no ACK was received, ::kThreadError_ChannelAccessFailure when the transmission
 *                     could not take place due to activity on the channel, ::kThreadError_Abort when transmission was
 *                     aborted for other reasons.
 *
 */
extern void otPlatRadioTransmitDone(otInstance *aInstance, RadioPacket *aPacket, bool aFramePending,
                                    ThreadError aError);

/**
 * Get the most recent RSSI measurement.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The RSSI in dBm when it is valid.  127 when RSSI is invalid.
 */
int8_t otPlatRadioGetRssi(otInstance *aInstance);

/**
 * Get the radio capabilities.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The radio capability bit vector. The stack enables or disables some functions based on this value.
 */
otRadioCaps otPlatRadioGetCaps(otInstance *aInstance);

/**
 * Get the status of promiscuous mode.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval true   Promiscuous mode is enabled.
 * @retval false  Promiscuous mode is disabled.
 */
bool otPlatRadioGetPromiscuous(otInstance *aInstance);

/**
 * Enable or disable promiscuous mode.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aEnable   A value to enable or disable promiscuous mode.
 */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable);

/**
 * The radio driver calls this method to notify OpenThread diagnostics module that the transmission has completed.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aPacket        A pointer to the packet that was transmitted.
 * @param[in]  aFramePending  TRUE if an ACK frame was received and the Frame Pending bit was set.
 * @param[in]  aError  ::kThreadError_None when the frame was transmitted, ::kThreadError_NoAck when the frame was
 *                     transmitted but no ACK was received, ::kThreadError_ChannelAccessFailure when the transmission
 *                     could not take place due to activity on the channel, ::kThreadError_Abort when transmission was
 *                     aborted for other reasons.
 *
 */
extern void otPlatDiagRadioTransmitDone(otInstance *aInstance, RadioPacket *aPacket, bool aFramePending,
                                        ThreadError aError);

/**
 * The radio driver calls this method to notify OpenThread diagnostics module of a received packet.
 *
 * @param[in]  aInstance The OpenThread instance structure.
 * @param[in]  aPacket   A pointer to the received packet or NULL if the receive operation was aborted.
 * @param[in]  aError    ::kThreadError_None when successfully received a frame, ::kThreadError_Abort when reception
 *                       was aborted and a frame was not received.
 *
 */
extern void otPlatDiagRadioReceiveDone(otInstance *aInstance, RadioPacket *aPacket, ThreadError aError);

/**
 * This method begins the energy scan sequence on the radio.
 *
 * @param[in] aInstance      The OpenThread instance structure.
 * @param[in] aScanChannel   The channel to perform the energy scan on.
 * @param[in] aScanDuration  The duration, in milliseconds, for the channel to be scanned.
 *
 * @retval ::kThreadError_None            Successfully started scanning the channel.
 * @retval ::kThreadError_NotImplemented  The radio doesn't support energy scanning.
 */
ThreadError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration);

/**
 * The radio driver calls this method to notify OpenThread that the energy scan is complete.
 *
 * @param[in]  aInstance           The OpenThread instance structure.
 * @param[in]  aEnergyScanMaxRssi  The maximum RSSI encountered on the scanned channel.
 *
 */
extern void otPlatRadioEnergyScanDone(otInstance *aInstance, int8_t aEnergyScanMaxRssi);

/**
 * @}
 *
 */

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // RADIO_H_
