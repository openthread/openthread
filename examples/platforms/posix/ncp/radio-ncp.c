/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include "platform-posix.h"

#if OPENTHREAD_ENABLE_POSIX_RADIO_NCP
#include <stdlib.h>

#include <common/logging.hpp>

#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

#include "ncp.h"
#include "utils/code_utils.h"

enum
{
    IEEE802154_MIN_LENGTH = 5,
    IEEE802154_MAX_LENGTH = 127,
    IEEE802154_ACK_LENGTH = 5,

    IEEE802154_BROADCAST = 0xffff,

    IEEE802154_FRAME_TYPE_ACK    = 2 << 0,
    IEEE802154_FRAME_TYPE_MACCMD = 3 << 0,
    IEEE802154_FRAME_TYPE_MASK   = 7 << 0,

    IEEE802154_SECURITY_ENABLED  = 1 << 3,
    IEEE802154_FRAME_PENDING     = 1 << 4,
    IEEE802154_ACK_REQUEST       = 1 << 5,
    IEEE802154_PANID_COMPRESSION = 1 << 6,

    IEEE802154_DST_ADDR_NONE  = 0 << 2,
    IEEE802154_DST_ADDR_SHORT = 2 << 2,
    IEEE802154_DST_ADDR_EXT   = 3 << 2,
    IEEE802154_DST_ADDR_MASK  = 3 << 2,

    IEEE802154_SRC_ADDR_NONE  = 0 << 6,
    IEEE802154_SRC_ADDR_SHORT = 2 << 6,
    IEEE802154_SRC_ADDR_EXT   = 3 << 6,
    IEEE802154_SRC_ADDR_MASK  = 3 << 6,

    IEEE802154_DSN_OFFSET     = 2,
    IEEE802154_DSTPAN_OFFSET  = 3,
    IEEE802154_DSTADDR_OFFSET = 5,

    IEEE802154_SEC_LEVEL_MASK = 7 << 0,

    IEEE802154_KEY_ID_MODE_0    = 0 << 3,
    IEEE802154_KEY_ID_MODE_1    = 1 << 3,
    IEEE802154_KEY_ID_MODE_2    = 2 << 3,
    IEEE802154_KEY_ID_MODE_3    = 3 << 3,
    IEEE802154_KEY_ID_MODE_MASK = 3 << 3,

    IEEE802154_MACCMD_DATA_REQ = 4,
};

enum
{
    kIdle,
    kSent,
    kDone,
};

static uint8_t sReceivePsdu[OT_RADIO_FRAME_MAX_SIZE];
static uint8_t sTransmitPsdu[OT_RADIO_FRAME_MAX_SIZE];
static uint8_t sAckPsdu[OT_RADIO_FRAME_MAX_SIZE];

static otRadioFrame sReceiveFrame;
static otRadioFrame sTransmitFrame;
static otRadioFrame sAckFrame;

static int      sSockFd = -1;
static uint8_t  sExtendedAddress[OT_EXT_ADDRESS_SIZE];
static uint16_t sShortAddress;
static uint16_t sPanid;
static uint8_t  sChannel;
static int8_t   sReceiveSensitivity = 0;
static bool     sPromiscuous        = false;
static bool     sAckWait            = false;
static uint8_t  sTxState            = kIdle;

static otError      sLastTransmitError;
static otRadioState sState = OT_RADIO_STATE_DISABLED;

static void radioTransmit(otInstance *aInstance);
const char *dump_hex(const uint8_t *buf, uint16_t len);

static inline otPanId getDstPan(const uint8_t *frame)
{
    return (otPanId)((frame[IEEE802154_DSTPAN_OFFSET + 1] << 8) | frame[IEEE802154_DSTPAN_OFFSET]);
}

static inline otShortAddress getShortAddress(const uint8_t *frame)
{
    return (otShortAddress)((frame[IEEE802154_DSTADDR_OFFSET + 1] << 8) | frame[IEEE802154_DSTADDR_OFFSET]);
}

static inline void getExtAddress(const uint8_t *frame, otExtAddress *address)
{
    size_t i;

    for (i = 0; i < sizeof(otExtAddress); i++)
    {
        address->m8[i] = frame[IEEE802154_DSTADDR_OFFSET + (sizeof(otExtAddress) - 1 - i)];
    }
}

static inline bool isAckRequested(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_ACK_REQUEST) != 0;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    otError error = ncpGet(SPINEL_PROP_HWADDR, SPINEL_DATATYPE_EUI64_S, aIeeeEui64);
    assert(error == OT_ERROR_NONE);
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    otError error = ncpSet(SPINEL_PROP_MAC_15_4_PANID, SPINEL_DATATYPE_UINT16_S, panid);
    assert(error == OT_ERROR_NONE);
    sPanid = panid;

    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *address)
{
    otError error;

    for (size_t i = 0; i < sizeof(sExtendedAddress); i++)
    {
        sExtendedAddress[i] = address->m8[sizeof(sExtendedAddress) - 1 - i];
    }

    error = ncpSet(SPINEL_PROP_MAC_15_4_LADDR, SPINEL_DATATYPE_EUI64_S, sExtendedAddress);
    // TODO assert should be changed to runtime checks.
    assert(error == OT_ERROR_NONE);

    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    otError error = ncpSet(SPINEL_PROP_MAC_15_4_SADDR, SPINEL_DATATYPE_UINT16_S, address);
    assert(error == OT_ERROR_NONE);
    sShortAddress = address;

    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    uint8_t mode  = (aEnable ? SPINEL_MAC_PROMISCUOUS_MODE_NETWORK : SPINEL_MAC_PROMISCUOUS_MODE_OFF);
    otError error = ncpSet(SPINEL_PROP_MAC_PROMISCUOUS_MODE, SPINEL_DATATYPE_UINT8_S, mode);
    assert(error == OT_ERROR_NONE);
    sPromiscuous = aEnable;
    OT_UNUSED_VARIABLE(aInstance);
}

void platformRadioInit(void)
{
    sSockFd = ncpOpen();
    assert(sSockFd != -1);

    sReceiveFrame.mPsdu  = sReceivePsdu;
    sTransmitFrame.mPsdu = sTransmitPsdu;
    sAckFrame.mPsdu      = sAckPsdu;
}

void platformRadioDeinit(void)
{
    ncpClose();
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sState != OT_RADIO_STATE_DISABLED;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    if (!otPlatRadioIsEnabled(aInstance))
    {
        error = ncpEnable(aInstance, radioProcessFrame, radioTransmitDone);
        otEXPECT(error == OT_ERROR_NONE);

        error = ncpGet(SPINEL_PROP_PHY_RX_SENSITIVITY, SPINEL_DATATYPE_INT8_S, &sReceiveSensitivity);
        otEXPECT(error == OT_ERROR_NONE);

        sState = OT_RADIO_STATE_SLEEP;
    }

exit:
    assert(error == OT_ERROR_NONE);
    return error;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    if (otPlatRadioIsEnabled(aInstance))
    {
        error = ncpDisable();
        otEXPECT(error == OT_ERROR_NONE);
        sState = OT_RADIO_STATE_DISABLED;
    }

exit:
    assert(error == OT_ERROR_NONE);
    return error;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aInstance);
    switch (sState)
    {
    case OT_RADIO_STATE_RECEIVE:
        error = ncpSet(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, false);
        otEXPECT(error == OT_ERROR_NONE);

        sState = OT_RADIO_STATE_SLEEP;
        break;

    case OT_RADIO_STATE_SLEEP:
        break;

    default:
        error = OT_ERROR_INVALID_STATE;
        break;
    }

exit:
    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sState != OT_RADIO_STATE_DISABLED, error = OT_ERROR_INVALID_STATE);

    if (sChannel != aChannel)
    {
        error = ncpSet(SPINEL_PROP_PHY_CHAN, SPINEL_DATATYPE_UINT8_S, aChannel);
        otEXPECT(error == OT_ERROR_NONE);
        sChannel = aChannel;
    }

    if (sState == OT_RADIO_STATE_SLEEP)
    {
        error = ncpSet(SPINEL_PROP_MAC_RAW_STREAM_ENABLED, SPINEL_DATATYPE_BOOL_S, true);
        otEXPECT(error == OT_ERROR_NONE);
    }

    sTxState = kIdle;
    sState   = OT_RADIO_STATE_RECEIVE;

exit:
    assert(error == OT_ERROR_NONE);
    OT_UNUSED_VARIABLE(aInstance);
    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    otError error = OT_ERROR_INVALID_STATE;

    otEXPECT(sState == OT_RADIO_STATE_RECEIVE);
    sState = OT_RADIO_STATE_TRANSMIT;
    error  = OT_ERROR_NONE;

exit:
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return 0;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_TRANSMIT_RETRIES | OT_RADIO_CAPS_CSMA_BACKOFF;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sPromiscuous;
}

void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd, struct timeval *aTimeout)
{
    if (aReadFdSet != NULL && (sState != OT_RADIO_STATE_TRANSMIT || sTxState == kSent))
    {
        FD_SET(sSockFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }

    if (aWriteFdSet != NULL && sState == OT_RADIO_STATE_TRANSMIT && sTxState == kIdle)
    {
        FD_SET(sSockFd, aWriteFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }

    if (aTimeout != NULL && ncpIsFrameCached())
    {
        aTimeout->tv_sec  = 0;
        aTimeout->tv_usec = 0;
    }
}

void platformRadioProcess(otInstance *aInstance, fd_set *aReadFdSet, fd_set *aWriteFdSet)
{
    if (FD_ISSET(sSockFd, aReadFdSet) || ncpIsFrameCached())
    {
        ncpProcess(&sReceiveFrame, FD_ISSET(sSockFd, aReadFdSet));

        if (sState == OT_RADIO_STATE_TRANSMIT && sTxState == kDone)
        {
            sState = OT_RADIO_STATE_RECEIVE;
            otPlatRadioTxDone(aInstance, &sTransmitFrame, (sAckWait ? &sAckFrame : NULL), sLastTransmitError);
        }
    }

    if (FD_ISSET(sSockFd, aWriteFdSet))
    {
        if (sState == OT_RADIO_STATE_TRANSMIT && sTxState == kIdle)
        {
            radioTransmit(aInstance);
        }
    }
}

void radioTransmit(otInstance *aInstance)
{
    otError error;
    otPlatRadioTxStarted(aInstance, &sTransmitFrame);
    assert(sTxState == kIdle);

    sAckWait = isAckRequested(sTransmitFrame.mPsdu);
    error    = ncpTransmit(&sTransmitFrame, (sAckWait ? &sAckFrame : NULL));

    if (error)
    {
        sState = OT_RADIO_STATE_RECEIVE;

#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, error);
        }
        else
#endif
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, error);
        }

        sTxState = kIdle;
    }
    else
    {
        sTxState = kSent;
    }
}

void radioTransmitDone(otInstance *aInstance, otError aError)
{
    sTxState           = kDone;
    sLastTransmitError = aError;
    OT_UNUSED_VARIABLE(aInstance);
}

void radioProcessFrame(otInstance *aInstance)
{
    otError        error = OT_ERROR_NONE;
    otPanId        dstpan;
    otShortAddress short_address;
    otExtAddress   ext_address;

    otEXPECT_ACTION(sPromiscuous == false, error = OT_ERROR_NONE);
    otEXPECT_ACTION((sState == OT_RADIO_STATE_RECEIVE || sState == OT_RADIO_STATE_TRANSMIT), error = OT_ERROR_DROP);

    switch (sReceiveFrame.mPsdu[1] & IEEE802154_DST_ADDR_MASK)
    {
    case IEEE802154_DST_ADDR_NONE:
        break;

    case IEEE802154_DST_ADDR_SHORT:
        dstpan        = getDstPan(sReceiveFrame.mPsdu);
        short_address = getShortAddress(sReceiveFrame.mPsdu);
        otEXPECT_ACTION((dstpan == IEEE802154_BROADCAST || dstpan == sPanid) &&
                            (short_address == IEEE802154_BROADCAST || short_address == sShortAddress),
                        error = OT_ERROR_ABORT);
        break;

    case IEEE802154_DST_ADDR_EXT:
        dstpan = getDstPan(sReceiveFrame.mPsdu);
        getExtAddress(sReceiveFrame.mPsdu, &ext_address);
        otEXPECT_ACTION((dstpan == IEEE802154_BROADCAST || dstpan == sPanid) &&
                            memcmp(&ext_address, sExtendedAddress, sizeof(ext_address)) == 0,
                        error = OT_ERROR_ABORT);
        break;

    default:
        error = OT_ERROR_ABORT;
        goto exit;
    }

exit:

#if OPENTHREAD_ENABLE_DIAG

    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioReceiveDone(aInstance, error == OT_ERROR_NONE ? &sReceiveFrame : NULL, error);
    }
    else
#endif
    {
        otPlatRadioReceiveDone(aInstance, error == OT_ERROR_NONE ? &sReceiveFrame : NULL, error);
    }
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);
    ncpSet(SPINEL_PROP_MAC_SRC_MATCH_ENABLED, SPINEL_DATATYPE_BOOL_S, aEnable);
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return ncpInsert(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, aShortAddress);
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otExtAddress addr;
    OT_UNUSED_VARIABLE(aInstance);

    for (size_t i = 0; i < sizeof(sExtendedAddress); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(sExtendedAddress) - 1 - i];
    }

    return ncpInsert(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, addr.m8);
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, const uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    return ncpRemove(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, SPINEL_DATATYPE_UINT16_S, aShortAddress);
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    otExtAddress addr;
    OT_UNUSED_VARIABLE(aInstance);

    for (size_t i = 0; i < sizeof(sExtendedAddress); i++)
    {
        addr.m8[i] = aExtAddress->m8[sizeof(sExtendedAddress) - 1 - i];
    }

    return ncpRemove(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, SPINEL_DATATYPE_EUI64_S, addr.m8);
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    otError error;
    OT_UNUSED_VARIABLE(aInstance);

    error = ncpSet(SPINEL_PROP_MAC_SRC_MATCH_SHORT_ADDRESSES, NULL);

    if (OT_ERROR_NONE != error)
    {
        otLogCritPlat(aInstance, "Failed to clear short entries!", NULL);
        abort();
    }
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    otError error;
    OT_UNUSED_VARIABLE(aInstance);

    error = ncpSet(SPINEL_PROP_MAC_SRC_MATCH_EXTENDED_ADDRESSES, NULL);

    if (OT_ERROR_NONE != error)
    {
        otLogCritPlat(aInstance, "Failed to clear short entries!", NULL);
        abort();
    }
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aScanChannel);
    OT_UNUSED_VARIABLE(aScanDuration);
    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    otError error = OT_ERROR_NONE;
    OT_UNUSED_VARIABLE(aInstance);
    error = ncpGet(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, aPower);

    if (OT_ERROR_NONE != error)
    {
        otLogCritPlat(aInstance, "Failed to get short entries!", NULL);
        abort();
    }

    return error;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (OT_ERROR_NONE != ncpSet(SPINEL_PROP_PHY_TX_POWER, SPINEL_DATATYPE_INT8_S, aPower))
    {
        otLogCritPlat(aInstance, "Failed to clear short entries!", NULL);
        abort();
    }

    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sReceiveSensitivity;
}

#endif // OPENTHREAD_ENABLE_POSIX_RADIO_NCP
