/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements the OpenThread platform abstraction for radio communication.
 *
 */

#include <string.h>

#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "utils/code_utils.h"

#include "radio_qorvo.h"

enum
{
    GP712_RECEIVE_SENSITIVITY = -100, // dBm
};

enum
{
    IEEE802154_MIN_LENGTH      = 5,
    IEEE802154_MAX_LENGTH      = 127,
    IEEE802154_ACK_LENGTH      = 5,
    IEEE802154_FRAME_TYPE_MASK = 0x7,
    IEEE802154_FRAME_TYPE_ACK  = 0x2,
    IEEE802154_FRAME_PENDING   = 1 << 4,
    IEEE802154_ACK_REQUEST     = 1 << 5,
    IEEE802154_DSN_OFFSET      = 2,
};

enum
{
    QORVO_RSSI_OFFSET  = 73,
    QORVO_CRC_BIT_MASK = 0x80,
    QORVO_LQI_BIT_MASK = 0x7f,
};

extern otRadioFrame sTransmitFrame;

static otRadioState sState;
static otInstance * pQorvoInstance;

typedef struct otCachedSettings_s
{
    uint16_t panid;
} otCachedSettings_t;

static otCachedSettings_t otCachedSettings;

static uint8_t sScanstate         = 0;
static int8_t  sLastReceivedPower = 127;

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    qorvoRadioGetIeeeEui64(aIeeeEui64);
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    OT_UNUSED_VARIABLE(aInstance);

    qorvoRadioSetPanId(panid);
    otCachedSettings.panid = panid;
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *address)
{
    OT_UNUSED_VARIABLE(aInstance);

    qorvoRadioSetExtendedAddress(address->m8);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    OT_UNUSED_VARIABLE(aInstance);

    qorvoRadioSetShortAddress(address);
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (sState != OT_RADIO_STATE_DISABLED);
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    pQorvoInstance = aInstance;
    memset(&otCachedSettings, 0x00, sizeof(otCachedSettings_t));

    if (!otPlatRadioIsEnabled(aInstance))
    {
        sState = OT_RADIO_STATE_SLEEP;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otEXPECT(otPlatRadioIsEnabled(aInstance));

    if (sState == OT_RADIO_STATE_RECEIVE)
    {
        qorvoRadioSetRxOnWhenIdle(false);
    }

    sState = OT_RADIO_STATE_DISABLED;

exit:
    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_RECEIVE)
    {
        qorvoRadioSetRxOnWhenIdle(false);
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_SLEEP;
    }

    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    otError error = OT_ERROR_INVALID_STATE;

    pQorvoInstance = aInstance;

    if ((sState != OT_RADIO_STATE_DISABLED) && (sScanstate == 0))
    {
        qorvoRadioSetCurrentChannel(aChannel);
        error = OT_ERROR_NONE;
    }

    if (sState == OT_RADIO_STATE_SLEEP)
    {
        qorvoRadioSetRxOnWhenIdle(true);
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_RECEIVE;
    }

    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aPacket)
{
    otError err = OT_ERROR_NONE;

    pQorvoInstance = aInstance;

    otEXPECT_ACTION(sState != OT_RADIO_STATE_DISABLED, err = OT_ERROR_INVALID_STATE);

    err = qorvoRadioTransmit(aPacket);

exit:
    return err;
}

void cbQorvoRadioTransmitDone(otRadioFrame *aPacket, bool aFramePending, otError aError)
{
    // TODO: pass received ACK frame instead of generating one.
    otRadioFrame ackFrame;
    uint8_t      psdu[IEEE802154_ACK_LENGTH];

    ackFrame.mPsdu    = psdu;
    ackFrame.mLength  = IEEE802154_ACK_LENGTH;
    ackFrame.mPsdu[0] = IEEE802154_FRAME_TYPE_ACK;

    if (aFramePending)
    {
        ackFrame.mPsdu[0] |= IEEE802154_FRAME_PENDING;
    }

    ackFrame.mPsdu[1] = 0;
    ackFrame.mPsdu[2] = aPacket->mPsdu[IEEE802154_DSN_OFFSET];

    otPlatRadioTxDone(pQorvoInstance, aPacket, &ackFrame, aError);
}

void cbQorvoRadioReceiveDone(otRadioFrame *aPacket, otError aError)
{
    if (aError == OT_ERROR_NONE)
    {
        sLastReceivedPower = aPacket->mInfo.mRxInfo.mRssi;
    }

    otPlatRadioReceiveDone(pQorvoInstance, aPacket, aError);
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sLastReceivedPower;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_ENERGY_SCAN | OT_RADIO_CAPS_TRANSMIT_RETRIES;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return false;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aEnable);
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    qorvoRadioEnableSrcMatch(aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return qorvoRadioAddSrcMatchShortEntry(aShortAddress, otCachedSettings.panid);
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return qorvoRadioAddSrcMatchExtEntry(aExtAddress->m8, otCachedSettings.panid);
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return qorvoRadioClearSrcMatchShortEntry(aShortAddress, otCachedSettings.panid);
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return qorvoRadioClearSrcMatchExtEntry(aExtAddress->m8, otCachedSettings.panid);
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    /* clear both short and extended addresses here */
    qorvoRadioClearSrcMatchEntries();
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    /* not implemented */
    /* assumes clearing of short and extended entries is done simultaniously by the openthread stack */
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);

    sScanstate = 1;
    return qorvoRadioEnergyScan(aScanChannel, aScanDuration);
}

void cbQorvoRadioEnergyScanDone(int8_t aEnergyScanMaxRssi)
{
    sScanstate = 0;
    otPlatRadioEnergyScanDone(pQorvoInstance, aEnergyScanMaxRssi);
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    // TODO: Create a proper implementation for this driver.
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPower);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    // TODO: Create a proper implementation for this driver.
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aPower);

    return OT_ERROR_NOT_IMPLEMENTED;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return GP712_RECEIVE_SENSITIVITY;
}
