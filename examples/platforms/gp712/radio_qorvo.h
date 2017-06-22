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
 *   This file includes the declarations of the radio functions from the Qorvo library.
 *
 */

#ifndef RADIO_QORVO_H_
#define RADIO_QORVO_H_

#include <stdint.h>
#include <openthread/types.h>
#include <openthread/platform/radio.h>

/* Radio Api functions */
void qorvoRadioInit(void);
void qorvoRadioReset(void);
void qorvoRadioProcess(otInstance *aInstance);
otError qorvoRadioEnergyScan(uint8_t aScanChannel, uint16_t aScanDuration);
void qorvoRadioSetCurrentChannel(uint8_t channel);
void qorvoRadioSetRxOnWhenIdle(bool rxOnWhenIdle);
void qorvoRadioGetIeeeEui64(uint8_t *aIeeeEui64);
otError qorvoRadioTransmit(otRadioFrame *aPacket);
void qorvoRadioSetPanId(uint16_t panid);
void qorvoRadioSetShortAddress(uint16_t address);
void qorvoRadioSetExtendedAddress(uint8_t *address);
void qorvoRadioEnableSrcMatch(bool aEnable);
void qorvoRadioClearSrcMatchEntries(void);
otError qorvoRadioAddSrcMatchShortEntry(const uint16_t aShortAddress, uint16_t panid);
otError qorvoRadioAddSrcMatchExtEntry(const uint8_t *aExtAddress, uint16_t panid);
otError qorvoRadioClearSrcMatchShortEntry(const uint16_t aShortAddress, uint16_t panid);
otError qorvoRadioClearSrcMatchExtEntry(const uint8_t *aExtAddress, uint16_t panid);

/* radio callbacks */
void cbQorvoRadioEnergyScanDone(int8_t aEnergyScanMaxRssi);
void cbQorvoRadioTransmitDone(otRadioFrame *aPacket, bool aFramePending, otError aError);
void cbQorvoRadioReceiveDone(otRadioFrame *aPacket, otError aError);

#endif  // RADIO_QORVO_H_
