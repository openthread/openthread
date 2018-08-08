/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes the declarations of the radio functions from the Qorvo library.
 *
 */

#ifndef RADIO_QORVO_H_
#define RADIO_QORVO_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/platform/radio.h>

/**
 * This function initializes the radio.
 *
 */
void qorvoRadioInit(void);

/**
 * This function resets the radio.
 *
 */
void qorvoRadioReset(void);

/**
 * This function processes event to/from the radio.
 *
 */
void qorvoRadioProcess(void);

/**
 * This function starts an ED scan.
 *
 * @param[in]  aScanChannel  The channel which needs to be scanned.
 * @param[in]  aScanDuration The amount of time in ms which needs to be scanned.
 *
 */
otError qorvoRadioEnergyScan(uint8_t aScanChannel, uint16_t aScanDuration);

/**
 * This function sets the current channel.
 *
 * @param[in]  channel   The channel index.
 *
 */
void qorvoRadioSetCurrentChannel(uint8_t channel);

/**
 * This function sets the idle behaviour of the radio.
 *
 * @param[in]  rxOnWhenIdle  If true, the radio remains on which not transmitting.
 *
 */
void qorvoRadioSetRxOnWhenIdle(bool rxOnWhenIdle);

/**
 * This function retrieves the MAC address of the radio.
 *
 * @param[out]  aIeeeEui64  The MAC address of the radio.
 *
 */
void qorvoRadioGetIeeeEui64(uint8_t *aIeeeEui64);

/**
 * This function transmits a frame.
 *
 * @param[in]  aPacket  The frame which needs to be transmitted.
 *
 */
otError qorvoRadioTransmit(otRadioFrame *aPacket);

/**
 * This function sets the PanId.
 *
 * @param[in]  panid  The panId.
 *
 */
void qorvoRadioSetPanId(uint16_t panid);

/**
 * This function sets the short address.
 *
 * @param[in]  address  The short address.
 *
 */
void qorvoRadioSetShortAddress(uint16_t address);

/**
 * This function sets the extended address.
 *
 * @param[in]  address  The extended address.
 *
 */
void qorvoRadioSetExtendedAddress(const uint8_t *address);

/**
 * This function enables source address matching for indirect transmit.
 *
 * @param[in]  aEnable  if True will enable source address matching, false will disable.
 *
 */
void qorvoRadioEnableSrcMatch(bool aEnable);

/**
 * This function clears all entries from the source address match list.
 *
 */
void qorvoRadioClearSrcMatchEntries(void);

/**
 * This function adds an short address plus panid to the source address match list.
 *
 * @param[in]  aShortAddress  The short address which should be added.
 * @param[in]  panid          The panid.
 *
 */
otError qorvoRadioAddSrcMatchShortEntry(const uint16_t aShortAddress, uint16_t panid);

/**
 * This function adds an extended address plus panid to the source address match list.
 *
 * @param[in]  aExtAddress    The extended address which should be added.
 * @param[in]  panid          The panid.
 *
 */
otError qorvoRadioAddSrcMatchExtEntry(const uint8_t *aExtAddress, uint16_t panid);

/**
 * This function removes an short address plus panid from the source address match list.
 *
 * @param[in]  aShortAddress  The short address which should be removed.
 * @param[in]  panid          The panid.
 *
 */
otError qorvoRadioClearSrcMatchShortEntry(const uint16_t aShortAddress, uint16_t panid);

/**
 * This function removes an extended address plus panid from the source address match list.
 *
 * @param[in]  aExtAddress    The extended address which should be removed.
 * @param[in]  panid          The panid.
 *
 */
otError qorvoRadioClearSrcMatchExtEntry(const uint8_t *aExtAddress, uint16_t panid);

/**
 * This callback is called when the energy scan is finished.
 *
 * @param[in]  aEnergyScanMaxRssi  The amount of energy detected during the ED scan.
 *
 */
void cbQorvoRadioEnergyScanDone(int8_t aEnergyScanMaxRssi);

/**
 * This callback is called after a transmission is completed (and if required an ACK is received).
 *
 * @param[in]  aPacket        The packet which was transmitted.
 * @param[in]  aFramePending  Indicates if the FP bit was set in the ACK frame or not.
 * @param[in]  aError         Indicates if an error occurred during transmission.
 *
 */
void cbQorvoRadioTransmitDone(otRadioFrame *aPacket, bool aFramePending, otError aError);

/**
 * This callback is called after a frame is received.
 *
 * @param[in]  aPacket  The packet which was received.
 * @param[in]  aError   Any error which occurred during reception of the packet.
 *
 */
void cbQorvoRadioReceiveDone(otRadioFrame *aPacket, otError aError);

#endif  // RADIO_QORVO_H_
