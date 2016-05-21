/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
    kMaxPHYPacketSize   = 127,     ///< aMaxPHYPacketSize (IEEE 802.15.4-2006)
    kPhyMinChannel      = 11,      ///< 2.4 GHz IEEE 802.15.4-2006
    kPhyMaxChannel      = 26,      ///< 2.4 GHz IEEE 802.15.4-2006
    kPhySymbolsPerOctet = 2,       ///< 2.4 GHz IEEE 802.15.4-2006
    kPhyBitRate         = 250000,  ///< 2.4 GHz IEEE 802.15.4 (kilbits per second)

    kPhyBitsPerOctet    = 8,
    kPhyUsPerSymbol     = ((kPhyBitsPerOctet / kPhySymbolsPerOctet) * 1000000) / kPhyBitRate,
};

/**
 *   This enum represents radio capabilities.
 *
 */

typedef enum otRadioCaps
{
    kRadioCapsNone          = 0,  ///< None
    kRadioCapsAckTimeout    = 1,  ///< Radio supports AckTime event
} otRadioCaps;

/**
 * This structure represents an IEEE 802.15.4 radio frame.
 */
typedef struct RadioPacket
{
    uint8_t mLength;                   ///< Length of the PSDU.
    uint8_t mPsdu[kMaxPHYPacketSize];  ///< The PSDU.
    uint8_t mChannel;                  ///< Channel used to transmit/receive the frame.
    int8_t  mPower;                    ///< Transmit/receive power in dBm.
} RadioPacket;

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
 * Set the PAN ID for address filtering.
 *
 * @param[in] aPanId  The IEEE 802.15.4 PAN ID.
 *
 * @retval ::kThreadError_None  If the PAN ID was set properly.
 * @retval ::kThreadError_Fail  If the PAN ID was not set properly.
 */
ThreadError otPlatRadioSetPanId(uint16_t aPanId);

/**
 * Set the Extended Address for address filtering.
 *
 * @param[in] aExtendedAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 * @retval ::kThreadError_None  If the Extended Address was set properly.
 * @retval ::kThreadError_Fail  If the Extended Address was not set properly.
 */
ThreadError otPlatRadioSetExtendedAddress(uint8_t *aExtendedAddress);

/**
 * Set the Short Address for address filtering.
 *
 * @param[in] aShortAddress  The IEEE 802.15.4 Short Address.
 *
 * @retval ::kThreadError_None  If the Short Address was set properly.
 * @retval ::kThreadError_Fail  If the Short Address was not set properly.
 */
ThreadError otPlatRadioSetShortAddress(uint16_t aShortAddress);

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
 * Intialize the radio.
 */
void otPlatRadioInit();

/**
 * Enable the radio.
 *
 * @retval ::kThreadError_None  Successfully transitioned to Idle.
 * @retval ::kThreadError_Fail  Failed to transition to Idle.
 */
ThreadError otPlatRadioEnable(void);

/**
 * Disable the radio.
 *
 * @retval ::kThreadError_None  Successfully transitioned to Disabled.
 * @retval ::kThreadError_Fail  Failed to transition to Disabled.
 */
ThreadError otPlatRadioDisable(void);

/**
 * Transition the radio to Sleep.
 *
 * @retval ::kThreadError_None  Successfully transitioned to Sleep.
 * @retval ::kThreadError_Fail  Failed to transition to Sleep.
 */
ThreadError otPlatRadioSleep(void);

/**
 * Transition the radio to Idle.
 *
 * @retval ::kThreadError_None  Successfully transitioned to Idle.
 * @retval ::kThreadError_Fail  Failed to transition to Idle.
 */
ThreadError otPlatRadioIdle(void);

/**
 * Begins the receive sequence on the radio.
 *
 * The receive sequence consists of:
 * 1. Transitioning the radio to Receive from Idle.
 * 2. Remain in Receive until a packet is received or reception is aborted.
 * 3. Return to Idle.
 *
 * Upon completion of the receive sequence, otPlatRadioSignalReceiveDone() is called to signal completion to the MAC
 * layer.
 *
 * @param[in]  aPacket  A pointer to a packet buffer.
 *
 * @note The channel is specified in @p aPacket.
 *
 * @retval ::kThreadError_None  Successfully transitioned to Receive.
 * @retval ::kThreadError_Fail  Failed to transition to Receive.
 */
ThreadError otPlatRadioReceive(RadioPacket *aPacket);

/**
 * Signal that a packet has been received.
 *
 * This may be called from interrupt context.  The MAC layer will then schedule a call to otPlatRadioHandleReceive().
 */

extern void otPlatRadioSignalReceiveDone(void);

/**
 * Complete the receive sequence.
 *
 * @retval ::kThreadError_None          Successfully received a frame.
 * @retval ::kThreadError_Abort         Reception was aborted and a frame was not received.
 * @retval ::kThreadError_InvalidState  The radio was not in Receive.
 */
ThreadError otPlatRadioHandleReceiveDone(void);

/**
 * Begins the transmit sequence on the radio.
 *
 * The transmit sequence consists of:
 * 1. Transitioning the radio to Transmit from Idle.
 * 2. Transmits the psdu on the given channel and at the given transmit power.
 * 3. Return to Idle.
 *
 * Upon completion of the transmit sequence, otPlatRadioSignalTransmitDone() is called to signal completion to the MAC
 * layer.
 *
 * @param[in]  aPacket  A pointer to a packet buffer.
 *
 * @note The channel is specified in @p aPacket.
 * @note The transmit power is specified in @p aPacket.
 *
 * @retval ::kThreadError_None         Successfully transitioned to Transmit.
 * @retval ::kThreadError_InvalidArgs  One or more parameters in @p aPacket are invalid.
 * @retval ::kThreadError_Fail         Failed to transition to Transmit.
 */
ThreadError otPlatRadioTransmit(RadioPacket *aPacket);

/**
 * Signal that the requested transmission is complete.
 *
 * This may be called from interrupt context.  OpenThread will then schedule a call to
 * otPlatRadio_handle_transmit_done().
 */
extern void otPlatRadioSignalTransmitDone(void);

/**
 * Complete the transmit sequence on the radio.
 *
 * @param[out]  aFramePending  TRUE if an ACK frame was received and the Frame Pending bit was set.
 *
 * @retval ::kThreadError_None          The frame was transmitted.
 * @retval ::kThreadError_NoAck         The frame was transmitted, but no ACK was received.
 * @retval ::kThreadError_CcaFailed     The transmission was aborted due to CCA failure.
 * @retval ::kThreadError_Abort         The transmission was aborted for other reasons.
 * @retval ::kThreadError_InvalidState  The radio did not transmit a packet.
 */
ThreadError otPlatRadioHandleTransmitDone(bool *aFramePending);

/**
 * Get the most recent RSSI measurement.
 *
 * @returns The noise floor value in dBm when the noise floor value is valid.  127 when noise floor value is invalid.
 */
int8_t otPlatRadioGetNoiseFloor(void);

/**
 * Get the radio capabilities.
 *
 * @returns The radio capability bit vector. The stack enables or disables some functions based on this value.
 */
otRadioCaps otPlatRadioGetCaps(void);

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
