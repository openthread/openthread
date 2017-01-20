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
 *   This file defines the raw OpenThread IEEE 802.15.4 Link Layer API.
 */

#ifndef LINK_RAW_H_
#define LINK_RAW_H_

#include <openthread-types.h>
//#include "openthread/types.h"
#include "platform/radio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup linkraw  Link Raw
 *
 * @brief
 *   This module includes functions that control the raw link-layer configuration.
 *
 * @{
 *
 */

/**
 * This function enables/disables the raw link-layer.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 * @param[in] aEnabled      TRUE to enable raw link-layer, FALSE otherwise.
 *
 * @retval kThreadError_None            If the enable state was successfully set.
 * @retval kThreadError_InvalidState    If the OpenThread Ip6 interface is already enabled.
 *
 */
ThreadError otLinkRawSetEnable(otInstance *aInstance, bool aEnabled);

/**
 * This function indicates whether or not the raw link-layer is enabled.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 *
 * @retval true     The raw link-layer is enabled.
 * @retval false    The raw link-layer is disabled.
 *
 */
bool otLinkRawIsEnabled(otInstance *aInstance);

/**
 * This function set the IEEE 802.15.4 PAN ID.
 *
 * @param[in] aInstance      A pointer to an OpenThread instance.
 * @param[in] aPanId         The IEEE 802.15.4 PAN ID.
 *
 */
void otLinkRawSetPanId(otInstance *aInstance, uint16_t aPanId);

/**
 * This function sets the IEEE 802.15.4 Extended Address.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aExtendedAddress  A pointer to the IEEE 802.15.4 Extended Address.
 *
 */
void otLinkRawSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtendedAddress);

/**
 * Set the Short Address for address filtering.
 *
 * @param[in] aInstance      A pointer to an OpenThread instance.
 * @param[in] aShortAddress  The IEEE 802.15.4 Short Address.
 *
 */
void otLinkRawSetShortAddress(otInstance *aInstance, uint16_t aShortAddress);

/**
 * This function gets the status of promiscuous mode.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @retval true     Promiscuous mode is enabled.
 * @retval false    Promiscuous mode is disabled.
 *
 */
bool otLinkRawGetPromiscuous(otInstance *aInstance);

/**
 * This function enables or disables promiscuous mode.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aEnable      A value to enable or disable promiscuous mode.
 *
 */
void otLinkRawSetPromiscuous(otInstance *aInstance, bool aEnable);

/**
 * Transition the radio from Receive to Sleep.
 * Turn off the radio.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None            Successfully transitioned to Sleep.
 * @retval kThreadError_Busy            The radio was transmitting
 * @retval kThreadError_InvalidState    The radio was disabled
 *
 */
ThreadError otLinkRawSleep(otInstance *aInstance);

/**
 * This function pointer on receipt of a IEEE 802.15.4 frame.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aPacket      A pointer to the received packet or NULL if the receive operation was aborted.
 * @param[in]  aaError      kThreadError_None when successfully received a frame.
 *                          kThreadError_Abort when reception was aborted and a frame was not received.
 *
 */
typedef void (OTCALL *otLinkRawReceiveDone)(otInstance *aInstance, RadioPacket *aPacket, ThreadError aError);

/**
 * Transitioning the radio from Sleep to Receive.
 * Turn on the radio.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aChannel     The channel to use for receiving.
 * @param[in]  aCallback    A pointer to a function called on receipt of a IEEE 802.15.4 frame.
 *
 * @retval kThreadError_None            Successfully transitioned to Receive.
 * @retval kThreadError_InvalidState    The radio was disabled or transmitting.
 *
 */
ThreadError otLinkRawReceive(otInstance *aInstance, uint8_t aChannel, otLinkRawReceiveDone aCallback);

/**
 * The radio transitions from Transmit to Receive.
 * This method returns a pointer to the transmit buffer.
 *
 * The caller forms the IEEE 802.15.4 frame in this buffer then calls otLinkRawTransmit()
 * to request transmission.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 *
 * @returns A pointer to the transmit buffer.
 *
 */
RadioPacket *otLinkRawGetTransmitBuffer(otInstance *aInstance);

/**
 * This function pointer on receipt of a IEEE 802.15.4 frame.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aPacket          A pointer to the packet that was transmitted.
 * @param[in]  aFramePending    TRUE if an ACK frame was received and the Frame Pending bit was set.
 * @param[in]  aError           kThreadError_None when the frame was transmitted.
 *                              kThreadError_NoAck when the frame was transmitted but no ACK was received
 *                              kThreadError_ChannelAccessFailure when the transmission could not take place
                                    due to activity on the channel.
 *                              kThreadError_Abort when transmission was aborted for other reasons.
 *
 */
typedef void (*otLinkRawTransmitDone)(otInstance *aInstance, RadioPacket *aPacket, bool aFramePending,
                                      ThreadError aError);

/**
 * This method begins the transmit sequence on the radio.
 *
 * The caller must form the IEEE 802.15.4 frame in the buffer provided by otLinkRawGetTransmitBuffer() before
 * requesting transmission.  The channel and transmit power are also included in the RadioPacket structure.
 *
 * The transmit sequence consists of:
 * 1. Transitioning the radio to Transmit from Receive.
 * 2. Transmits the psdu on the given channel and at the given transmit power.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aPacket      A pointer to the packet that was transmitted.
 * @param[in]  aCallback    A pointer to a function called on completion of the transmission.
 *
 * @retval kThreadError_None         Successfully transitioned to Transmit.
 * @retval kThreadError_InvalidState The radio was not in the Receive state.
 *
 */
ThreadError otLinkRawTransmit(otInstance *aInstance, RadioPacket *aPacket, otLinkRawTransmitDone aCallback);

/**
 * Get the most recent RSSI measurement.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 *
 * @returns The RSSI in dBm when it is valid. 127 when RSSI is invalid.
 *
 */
int8_t otLinkRawGetRssi(otInstance *aInstance);

/**
 * Get the radio capabilities.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 *
 * @returns The radio capability bit vector. The stack enables or disables some functions based on this value.
 *
 */
otRadioCaps otLinkRawGetCaps(otInstance *aInstance);

/**
 * This function pointer on receipt of a IEEE 802.15.4 frame.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aEnergyScanMaxRssi   The maximum RSSI encountered on the scanned channel.
 *
 */
typedef void (*otLinkRawEnergyScanDone)(otInstance *aInstance, int8_t aEnergyScanMaxRssi);

/**
 * This method begins the energy scan sequence on the radio.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aScanChannel     The channel to perform the energy scan on.
 * @param[in]  aScanDuration    The duration, in milliseconds, for the channel to be scanned.
 * @param[in]  aCallback        A pointer to a function called on completion of a scanned channel.
 *
 * @retval kThreadError_None            Successfully started scanning the channel.
 * @retval kThreadError_NotImplemented  The radio doesn't support energy scanning.
 *
 */
ThreadError otLinkRawEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration,
                                otLinkRawEnergyScanDone aCallback);

/**
 * Enable/Disable source match for AutoPend.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 * @param[in]  aEnable      Enable/disable source match for automatical pending.
 *
 */
void otLinkRawSrcMatchEnable(otInstance *aInstance, bool aEnable);

/**
 * Adding short address to the source match table.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aShortAddress    The short address to be added.
 *
 * @retval kThreadError_None     Successfully added short address to the source match table.
 * @retval kThreadError_NoBufs   No available entry in the source match table.
 *
 */
ThreadError otLinkRawSrcMatchAddShortEntry(otInstance *aInstance, const uint16_t aShortAddress);

/**
 * Adding extended address to the source match table.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aExtAddress      The extended address to be added.
 *
 * @retval kThreadError_None     Successfully added extended address to the source match table.
 * @retval kThreadError_NoBufs   No available entry in the source match table.
 *
 */
ThreadError otLinkRawSrcMatchAddExtEntry(otInstance *aInstance, const uint8_t *aExtAddress);

/**
 * Removing short address to the source match table.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aShortAddress    The short address to be removed.
 *
 * @retval kThreadError_None        Successfully removed short address from the source match table.
 * @retval kThreadError_NoAddress   The short address is not in source match table.
 *
 */
ThreadError otLinkRawSrcMatchClearShortEntry(otInstance *aInstance, const uint16_t aShortAddress);

/**
 * Removing extended address to the source match table of the radio.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aExtAddress      The extended address to be removed.
 *
 * @retval kThreadError_None        Successfully removed the extended address from the source match table.
 * @retval kThreadError_NoAddress   The extended address is not in source match table.
 *
 */
ThreadError otLinkRawSrcMatchClearExtEntry(otInstance *aInstance, const uint8_t *aExtAddress);

/**
 * Removing all the short addresses from the source match table.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 *
 */
void otLinkRawSrcMatchClearShortEntries(otInstance *aInstance);

/**
 * Removing all the extended addresses from the source match table.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 *
 */
void otLinkRawSrcMatchClearExtEntries(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LINK_RAW_H_
