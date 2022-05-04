/*
 *  Copyright (c) 2016-2019, The OpenThread Authors.
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

#include "platform-simulation.h"

#if OPENTHREAD_SIMULATION_VIRTUAL_TIME

#include <errno.h>
#include <stdio.h>

#include <openthread/dataset.h>
#include <openthread/link.h>
#include <openthread/random_noncrypto.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "utils/code_utils.h"
#include "utils/link_metrics.h"
#include "utils/mac_frame.h"
#include "utils/soft_source_match_table.h"

// The IPv4 group for receiving packets of radio simulation
#define OT_RADIO_GROUP "224.0.0.116"

enum
{
    IEEE802154_ACK_LENGTH = 5,

    IEEE802154_FRAME_TYPE_ACK = 2 << 0,

    IEEE802154_FRAME_PENDING = 1 << 4,
};

enum
{
    SIM_RECEIVE_SENSITIVITY = -100, // dBm

    SIM_HIGH_RSSI_SAMPLE               = -30, // dBm
    SIM_LOW_RSSI_SAMPLE                = -98, // dBm
    SIM_HIGH_RSSI_PROB_INC_PER_CHANNEL = 5,
};

enum
{
    SIM_RADIO_CHANNEL_MIN = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN,
    SIM_RADIO_CHANNEL_MAX = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX,
};

enum
{
    CCA_DURATION_US                 = 8 * (OT_US_PER_TEN_SYMBOLS / 10),
    RADIO_STATE_TRANSITION_DELAY_US = 400,
};

extern int          sSockFd;
extern uint16_t     sPortOffset;
static otRadioState sLastReportedState   = OT_RADIO_STATE_DISABLED;
static uint8_t      sLastReportedChannel = 0;
static bool         sRadioTransmitting   = false;
static bool         sAckTxDonePending    = false;
static uint64_t     sTransmittingUntil   = 0;

#if OPENTHREAD_SIMULATION_CCA
static bool sCcaPending;
#endif

OT_TOOL_PACKED_BEGIN
struct RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[OT_RADIO_FRAME_MAX_SIZE];
} OT_TOOL_PACKED_END;

static void radioTransmitToOtns(struct RadioMessage *aMessage, const struct otRadioFrame *aFrame);
static void radioSendMessage(otInstance *aInstance);
static void radioSendAck(void);
static void radioProcessFrame(otInstance *aInstance);
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
static uint8_t generateAckIeData(uint8_t *aLinkMetricsIeData, uint8_t aLinkMetricsIeDataLen);
#endif

static otRadioState        sState = OT_RADIO_STATE_DISABLED;
static struct RadioMessage sReceiveMessage;
static struct RadioMessage sTransmitMessage;
static struct RadioMessage sAckMessage;
static otRadioFrame        sReceiveFrame;
static otRadioFrame        sTransmitFrame;
static otRadioFrame        sAckFrame;

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
static otRadioIeInfo sTransmitIeInfo;
#endif

static otExtAddress   sExtAddress;
static otShortAddress sShortAddress;
static otPanId        sPanid;
static bool           sPromiscuous = false;
static bool           sTxWait      = false;
static int8_t         sTxPower     = 0;
static int8_t         sCcaEdThresh = -74;
static int8_t         sLnaGain     = 0;
static uint16_t       sRegionCode  = 0;

enum
{
    kMinChannel = 11,
    kMaxChannel = 26,
};
static int8_t  sChannelMaxTransmitPower[kMaxChannel - kMinChannel + 1];
static uint8_t sCurrentChannel = kMinChannel;

static bool sSrcMatchEnabled = false;

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
static uint8_t sAckIeData[OT_ACK_IE_MAX_SIZE];
static uint8_t sAckIeDataLength = 0;
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint32_t sCslSampleTime;
static uint32_t sCslPeriod;
#endif

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
static bool sRadioCoexEnabled = true;
#endif

otRadioCaps gRadioCaps =
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    OT_RADIO_CAPS_TRANSMIT_SEC;
#else
    OT_RADIO_CAPS_NONE;
#endif

static uint32_t         sMacFrameCounter;
static uint8_t          sKeyId;
static otMacKeyMaterial sPrevKey;
static otMacKeyMaterial sCurrKey;
static otMacKeyMaterial sNextKey;
static otRadioKeyType   sKeyType;

static void ReverseExtAddress(otExtAddress *aReversed, const otExtAddress *aOrigin)
{
    for (size_t i = 0; i < sizeof(*aReversed); i++)
    {
        aReversed->m8[i] = aOrigin->m8[sizeof(*aOrigin) - 1 - i];
    }
}

static bool hasFramePending(const otRadioFrame *aFrame)
{
    bool         rval = false;
    otMacAddress src;

    otEXPECT_ACTION(sSrcMatchEnabled, rval = true);
    otEXPECT(otMacFrameGetSrcAddr(aFrame, &src) == OT_ERROR_NONE);

    switch (src.mType)
    {
    case OT_MAC_ADDRESS_TYPE_SHORT:
        rval = utilsSoftSrcMatchShortFindEntry(src.mAddress.mShortAddress) >= 0;
        break;
    case OT_MAC_ADDRESS_TYPE_EXTENDED:
    {
        otExtAddress extAddr;

        ReverseExtAddress(&extAddr, &src.mAddress.mExtAddress);
        rval = utilsSoftSrcMatchExtFindEntry(&extAddr) >= 0;
        break;
    }
    default:
        break;
    }

exit:
    return rval;
}

static uint16_t crc16_citt(uint16_t aFcs, uint8_t aByte)
{
    // CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-CCITT
    // width=16 poly=0x1021 init=0x0000 refin=true refout=true xorout=0x0000 check=0x2189 name="KERMIT"
    // http://reveng.sourceforge.net/crc-catalogue/16.htm#crc.cat.kermit
    static const uint16_t sFcsTable[256] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5,
        0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52,
        0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3,
        0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9,
        0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e,
        0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 0x6306, 0x728f,
        0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862,
        0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb,
        0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948,
        0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226,
        0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497,
        0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704,
        0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb,
        0x0e70, 0x1ff9, 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c,
        0x3de3, 0x2c6a, 0x1ef1, 0x0f78};
    return (aFcs >> 8) ^ sFcsTable[(aFcs ^ aByte) & 0xff];
}

const char *radioStateToString(otRadioState aState)
{
    char const *ans;

    switch (aState)
    {
    case OT_RADIO_STATE_RECEIVE:
        ans = "rx";
        break;
    case OT_RADIO_STATE_TRANSMIT:
        ans = "tx";
        break;
    case OT_RADIO_STATE_DISABLED:
        ans = "off";
        break;
    case OT_RADIO_STATE_SLEEP:
        ans = "sleep";
        break;
    default:
        assert(false);
    }
    return ans;
}

void reportRadioStatusToOtns(otRadioState aState)
{
    int          n;
    struct Event event;

    if (sLastReportedState != aState || sLastReportedChannel != sCurrentChannel)
    {
        sLastReportedState   = aState;
        sLastReportedChannel = sCurrentChannel;
        event.mDelay         = 0;
        event.mEvent         = OT_SIM_EVENT_OTNS_STATUS_PUSH;

        n = snprintf((char *)event.mData, sizeof(event.mData), "radio_state=%s,%d", radioStateToString(aState),
                     sCurrentChannel);

        assert(n > 0);

        event.mDataLength = (uint16_t)n;
        otSimSendEvent(&event);
    }
}

void requestCcaToOtns(uint8_t channel)
{
    struct Event event;

    event.mDelay   = CCA_DURATION_US;
    event.mEvent   = OT_SIM_EVENT_CHANNEL_ACTIVITY;
    event.mData[0] = channel;

    event.mDataLength = 1;
    otSimSendEvent(&event);
}

void setRadioState(otRadioState aState)
{
    if (!sRadioTransmitting && aState != OT_RADIO_STATE_TRANSMIT)
    {
        // Transmit state can only be reported by the radio driver, not by higher layers request.
        reportRadioStatusToOtns(aState);
    }
    sState = aState;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    aIeeeEui64[0] = 0x18;
    aIeeeEui64[1] = 0xb4;
    aIeeeEui64[2] = 0x30;
    aIeeeEui64[3] = 0x00;
    aIeeeEui64[4] = (gNodeId >> 24) & 0xff;
    aIeeeEui64[5] = (gNodeId >> 16) & 0xff;
    aIeeeEui64[6] = (gNodeId >> 8) & 0xff;
    aIeeeEui64[7] = gNodeId & 0xff;
}

void otPlatRadioSetPanId(otInstance *aInstance, otPanId aPanid)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sPanid = aPanid;
    utilsSoftSrcMatchSetPanId(aPanid);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    ReverseExtAddress(&sExtAddress, aExtAddress);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, otShortAddress aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sShortAddress = aShortAddress;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sPromiscuous = aEnable;
}

void platformRadioInit(void)
{
    sReceiveFrame.mPsdu  = sReceiveMessage.mPsdu;
    sTransmitFrame.mPsdu = sTransmitMessage.mPsdu;
    sAckFrame.mPsdu      = sAckMessage.mPsdu;

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    sTransmitFrame.mInfo.mTxInfo.mIeInfo = &sTransmitIeInfo;
#else
    sTransmitFrame.mInfo.mTxInfo.mIeInfo = NULL;
#endif

    for (size_t i = 0; i <= kMaxChannel - kMinChannel; i++)
    {
        sChannelMaxTransmitPower[i] = OT_RADIO_POWER_INVALID;
    }

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    otLinkMetricsInit(SIM_RECEIVE_SENSITIVITY);
#endif
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint16_t getCslPhase(void)
{
    uint32_t curTime       = otPlatAlarmMicroGetNow();
    uint32_t cslPeriodInUs = sCslPeriod * OT_US_PER_TEN_SYMBOLS;
    uint32_t diff = ((sCslSampleTime % cslPeriodInUs) - (curTime % cslPeriodInUs) + cslPeriodInUs) % cslPeriodInUs;

    return (uint16_t)(diff / OT_US_PER_TEN_SYMBOLS);
}
#endif

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (sState != OT_RADIO_STATE_DISABLED) ? true : false;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    if (!otPlatRadioIsEnabled(aInstance))
    {
        setRadioState(OT_RADIO_STATE_SLEEP);
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    otEXPECT(otPlatRadioIsEnabled(aInstance));
    otEXPECT_ACTION(sState == OT_RADIO_STATE_SLEEP, error = OT_ERROR_INVALID_STATE);

    setRadioState(OT_RADIO_STATE_DISABLED);

exit:
    return error;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_SLEEP || sState == OT_RADIO_STATE_RECEIVE)
    {
        error = OT_ERROR_NONE;
        setRadioState(OT_RADIO_STATE_SLEEP);
    }

    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState != OT_RADIO_STATE_DISABLED)
    {
        error                  = OT_ERROR_NONE;
        sTxWait                = false;
        sReceiveFrame.mChannel = aChannel;
        sCurrentChannel        = aChannel;
        setRadioState(OT_RADIO_STATE_RECEIVE); // Keep this call after sCurrentChannel is set.
    }

    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);

    assert(aInstance != NULL);
    assert(aFrame != NULL);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_RECEIVE)
    {
        error           = OT_ERROR_NONE;
        sCurrentChannel = aFrame->mChannel;
        setRadioState(OT_RADIO_STATE_TRANSMIT); // Keep this call after sCurrentChannel is set.
    }

    return error;
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    int8_t   rssi    = SIM_LOW_RSSI_SAMPLE;
    uint8_t  channel = sReceiveFrame.mChannel;
    uint32_t probabilityThreshold;

    otEXPECT((SIM_RADIO_CHANNEL_MIN <= channel) && channel <= (SIM_RADIO_CHANNEL_MAX));

    // To emulate a simple interference model, we return either a high or
    // a low  RSSI value with a fixed probability per each channel. The
    // probability is increased per channel by a constant.

    probabilityThreshold = (channel - SIM_RADIO_CHANNEL_MIN) * SIM_HIGH_RSSI_PROB_INC_PER_CHANNEL;

    if (otRandomNonCryptoGetUint16() < (probabilityThreshold * 0xffff / 100))
    {
        rssi = SIM_HIGH_RSSI_SAMPLE;
    }

exit:
    return rssi;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    return gRadioCaps;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    return sPromiscuous;
}

void platformTransmitReturn(otInstance *aInstance, otRadioFrame *frame, otRadioFrame *ack, otError error)
{
    sTxWait = false;
    setRadioState(OT_RADIO_STATE_RECEIVE);
#if OPENTHREAD_CONFIG_DIAG_ENABLE
    if (otPlatDiagModeGet())
    {
        otPlatDiagRadioTransmitDone(aInstance, frame, error);
    }
    else
#endif
    {
        otPlatRadioTxDone(aInstance, frame, ack, error);
    }
}

static void radioReceive(otInstance *aInstance)
{
    bool isTxDone = false;
    bool isAck    = otMacFrameIsAck(&sReceiveFrame);

    otEXPECT(sReceiveFrame.mChannel == sReceiveMessage.mChannel);
    otEXPECT(sState == OT_RADIO_STATE_RECEIVE || sState == OT_RADIO_STATE_TRANSMIT);

    // Unable to simulate SFD, so use the rx done timestamp instead.
    sReceiveFrame.mInfo.mRxInfo.mTimestamp = otPlatTimeGet();

    if (sTxWait && otMacFrameIsAckRequested(&sTransmitFrame))
    {
        isTxDone = isAck && otMacFrameGetSequence(&sReceiveFrame) == otMacFrameGetSequence(&sTransmitFrame);
    }

    if (isTxDone)
    {
        platformTransmitReturn(aInstance, &sTransmitFrame, (isAck ? &sReceiveFrame : NULL), OT_ERROR_NONE);
    }
    else if (!isAck || sPromiscuous)
    {
        radioProcessFrame(aInstance);
    }

exit:
    return;
}

static void radioComputeCrc(struct RadioMessage *aMessage, uint16_t aLength)
{
    uint16_t crc        = 0;
    uint16_t crc_offset = aLength - sizeof(uint16_t);

    for (uint16_t i = 0; i < crc_offset; i++)
    {
        crc = crc16_citt(crc, aMessage->mPsdu[i]);
    }

    aMessage->mPsdu[crc_offset]     = crc & 0xff;
    aMessage->mPsdu[crc_offset + 1] = crc >> 8;
}

void radioTransmitToOtns(struct RadioMessage *aMessage, const struct otRadioFrame *aFrame)
{
    radioComputeCrc(aMessage, aFrame->mLength);

    struct Event event;

    // Radio sState keeps in OT_RADIO_STATE_RECEIVE even when sending an ack.
    // For energy accuracy, we make a report without changing the sState.
    reportRadioStatusToOtns(OT_RADIO_STATE_TRANSMIT);

    event.mEvent       = OT_SIM_EVENT_RADIO_COMM;
    event.mDataLength  = 1 + aFrame->mLength; // include channel in first byte
    sRadioTransmitting = true;

    // 4 bytes of preamble + 1 SOF + 1 PHY header @250kbps + 400us of radio state transision time
    event.mDelay       = ((aFrame->mLength + 6) * 8 * 1000) / 250 + RADIO_STATE_TRANSITION_DELAY_US;
    sTransmittingUntil = otPlatTimeGet() + event.mDelay;

    memcpy(event.mData, aMessage, event.mDataLength);

    otSimSendEvent(&event);
}

void platformRadioReceive(otInstance *aInstance, uint8_t *aBuf, uint16_t aBufLength)
{
    assert(sizeof(sReceiveMessage) >= aBufLength);

    memcpy(&sReceiveMessage, aBuf, aBufLength);

    sReceiveFrame.mLength = (uint8_t)(aBufLength - 1);

    radioReceive(aInstance);
}

static otError radioProcessTransmitSecurity(otRadioFrame *aFrame)
{
    otError error = OT_ERROR_NONE;
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    otMacKeyMaterial *key = NULL;
    uint8_t           keyId;

    otEXPECT(otMacFrameIsSecurityEnabled(aFrame) && otMacFrameIsKeyIdMode1(aFrame) &&
             !aFrame->mInfo.mTxInfo.mIsSecurityProcessed);

    if (otMacFrameIsAck(aFrame))
    {
        keyId = otMacFrameGetKeyId(aFrame);

        otEXPECT_ACTION(keyId != 0, error = OT_ERROR_FAILED);

        if (keyId == sKeyId)
        {
            key = &sCurrKey;
        }
        else if (keyId == sKeyId - 1)
        {
            key = &sPrevKey;
        }
        else if (keyId == sKeyId + 1)
        {
            key = &sNextKey;
        }
        else
        {
            error = OT_ERROR_SECURITY;
            otEXPECT(false);
        }
    }
    else
    {
        key   = &sCurrKey;
        keyId = sKeyId;
    }

    aFrame->mInfo.mTxInfo.mAesKey = key;

    if (!aFrame->mInfo.mTxInfo.mIsHeaderUpdated)
    {
        otMacFrameSetKeyId(aFrame, keyId);
        otMacFrameSetFrameCounter(aFrame, sMacFrameCounter++);
    }
#else
    otEXPECT(!aFrame->mInfo.mTxInfo.mIsSecurityProcessed);
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

    otMacFrameProcessTransmitAesCcm(aFrame, &sExtAddress);

exit:
    return error;
}

#if OPENTHREAD_SIMULATION_CCA
void platformChannelActivity(otInstance *aInstance, uint8_t channel, int8_t value)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(channel); // For future use of scanning

    if (sCcaPending)
    {
        sCcaPending = false;
        if (value <= sCcaEdThresh)
        {
            radioSendMessage(aInstance);
        }
        else
        {
            platformTransmitReturn(aInstance, &sTransmitFrame, NULL, OT_ERROR_CHANNEL_ACCESS_FAILURE);
        }
    }
}

void simulateCca(otInstance *aInstance, uint8_t channel)
{
    OT_UNUSED_VARIABLE(aInstance);

    sCcaPending = true;
    requestCcaToOtns(channel);
    otPlatAlarmMicroStartAt(aInstance, otPlatAlarmMicroGetNow(), CCA_DURATION_US);
}
#endif

void radioSendMessage(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset != 0)
    {
        uint8_t *timeIe = sTransmitFrame.mPsdu + sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeIeOffset;
        uint64_t time = (uint64_t)((int64_t)otPlatTimeGet() + sTransmitFrame.mInfo.mTxInfo.mIeInfo->mNetworkTimeOffset);

        *timeIe = sTransmitFrame.mInfo.mTxInfo.mIeInfo->mTimeSyncSeq;

        *(++timeIe) = (uint8_t)(time & 0xff);
        for (uint8_t i = 1; i < sizeof(uint64_t); i++)
        {
            time        = time >> 8;
            *(++timeIe) = (uint8_t)(time & 0xff);
        }
    }
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT && OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (sCslPeriod > 0 && !sTransmitFrame.mInfo.mTxInfo.mIsHeaderUpdated)
    {
        otMacFrameSetCslIe(&sTransmitFrame, (uint16_t)sCslPeriod, getCslPhase());
    }
#endif

    sTransmitMessage.mChannel = sTransmitFrame.mChannel;

    otEXPECT(radioProcessTransmitSecurity(&sTransmitFrame) == OT_ERROR_NONE);
    otPlatRadioTxStarted(aInstance, &sTransmitFrame);
    radioTransmitToOtns(&sTransmitMessage, &sTransmitFrame);

    // Wait for simulator confirmation signal of radio transmission in virtual time mode.
    sTxWait = true;
exit:
    return;
}

void setupTransmission(otInstance *aInstance)
{
    otError  error = OT_ERROR_NONE;
    uint32_t delay;

    otEXPECT_ACTION(!sRadioTransmitting, error = OT_ERROR_ALREADY);

#if OPENTHREAD_SIMULATION_CCA
    simulateCca(aInstance, sTransmitFrame.mChannel);
#else
    radioSendMessage(aInstance);
#endif

exit:
    if (error == OT_ERROR_ALREADY)
    {
        // Radio is already transmiting, wait for it to end
        if (sTransmittingUntil > otPlatTimeGet())
        {
            delay = (uint32_t)(sTransmittingUntil - otPlatTimeGet()) + 1;
        }
        else
        {
            delay = 10; // Arbitrary number to give simulater enough time to send confirmation signal;
        }

        otPlatAlarmMicroStartAt(aInstance, otPlatAlarmMicroGetNow(), delay);
    }

    return;
}

bool platformRadioIsTransmitPending(void)
{
    return sState == OT_RADIO_STATE_TRANSMIT && !sTxWait;
}

bool platformRadioTaskPending(void)
{
#if OPENTHREAD_SIMULATION_CCA
    return sAckTxDonePending || sCcaPending;
#else
    return sAckTxDonePending;
#endif
}

void platformRadioTxDone(otInstance *aInstance, uint8_t pktSeq)
{
    OT_UNUSED_VARIABLE(aInstance);
    otEXPECT(sLastReportedState == OT_RADIO_STATE_TRANSMIT);

    sRadioTransmitting = false;
    sTransmittingUntil = 0;

    if (sAckTxDonePending)
    {
        sAckTxDonePending = false;
        setRadioState(sState);
    }
    else if (otMacFrameGetSequence(&sTransmitFrame) == pktSeq)
    {
        // Radio sState keeps in OT_RADIO_STATE_RECEIVE even when sending an ack.
        // For energy accuracy, we make a report without changing the sState.
        if (otMacFrameIsAckRequested(&sTransmitFrame))
        {
            reportRadioStatusToOtns(OT_RADIO_STATE_RECEIVE);
        }
        else
        {
            platformTransmitReturn(aInstance, &sTransmitFrame, NULL, OT_ERROR_NONE);
        }
    }

exit:
    return;
}

void platformRadioProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aReadFdSet);
    OT_UNUSED_VARIABLE(aWriteFdSet);

    // Do not send if radio is transmitting an ack nor waiting for CCA.
#if OPENTHREAD_SIMULATION_CCA
    if (platformRadioIsTransmitPending() && !sAckTxDonePending && !sCcaPending)
#else
    if (platformRadioIsTransmitPending() && !sAckTxDonePending)
#endif
    {
        setupTransmission(aInstance);
    }
}

void radioSendAck(void)
{
    if (
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        // Determine if frame pending should be set
        ((otMacFrameIsVersion2015(&sReceiveFrame) && otMacFrameIsCommand(&sReceiveFrame)) ||
         otMacFrameIsData(&sReceiveFrame) || otMacFrameIsDataRequest(&sReceiveFrame))
#else
        otMacFrameIsDataRequest(&sReceiveFrame)
#endif
        && hasFramePending(&sReceiveFrame))
    {
        sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending = true;
    }

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    // Use enh-ack for 802.15.4-2015 frames
    if (otMacFrameIsVersion2015(&sReceiveFrame))
    {
        uint8_t  linkMetricsDataLen = 0;
        uint8_t *dataPtr            = NULL;

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
        uint8_t      linkMetricsData[OT_ENH_PROBING_IE_DATA_MAX_SIZE];
        otMacAddress macAddress;

        otEXPECT(otMacFrameGetSrcAddr(&sReceiveFrame, &macAddress) == OT_ERROR_NONE);

        linkMetricsDataLen = otLinkMetricsEnhAckGenData(&macAddress, sReceiveFrame.mInfo.mRxInfo.mLqi,
                                                        sReceiveFrame.mInfo.mRxInfo.mRssi, linkMetricsData);

        if (linkMetricsDataLen > 0)
        {
            dataPtr = linkMetricsData;
        }
#endif

        sAckIeDataLength = generateAckIeData(dataPtr, linkMetricsDataLen);

        otEXPECT(otMacFrameGenerateEnhAck(&sReceiveFrame, sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending,
                                          sAckIeData, sAckIeDataLength, &sAckFrame) == OT_ERROR_NONE);
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        if (sCslPeriod > 0)
        {
            otMacFrameSetCslIe(&sAckFrame, (uint16_t)sCslPeriod, getCslPhase());
        }
#endif
        if (otMacFrameIsSecurityEnabled(&sAckFrame))
        {
            otEXPECT(radioProcessTransmitSecurity(&sAckFrame) == OT_ERROR_NONE);
        }
    }
    else
#endif
    {
        otMacFrameGenerateImmAck(&sReceiveFrame, sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending, &sAckFrame);
    }

    sAckMessage.mChannel = sReceiveFrame.mChannel;

    radioTransmitToOtns(&sAckMessage, &sAckFrame);
    sAckTxDonePending = true;

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
exit:
#endif
    return;
}

void radioProcessFrame(otInstance *aInstance)
{
    otError      error = OT_ERROR_NONE;
    otMacAddress macAddress;
    OT_UNUSED_VARIABLE(macAddress);

    sReceiveFrame.mInfo.mRxInfo.mRssi = -20;
    sReceiveFrame.mInfo.mRxInfo.mLqi  = OT_RADIO_LQI_NONE;

    sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending = false;
    sReceiveFrame.mInfo.mRxInfo.mAckedWithSecEnhAck    = false;

    otEXPECT(sPromiscuous == false);

    otEXPECT_ACTION(otMacFrameDoesAddrMatch(&sReceiveFrame, sPanid, sShortAddress, &sExtAddress),
                    error = OT_ERROR_ABORT);

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    otEXPECT_ACTION(otMacFrameGetSrcAddr(&sReceiveFrame, &macAddress) == OT_ERROR_NONE, error = OT_ERROR_PARSE);
#endif

    // generate acknowledgment
    if (otMacFrameIsAckRequested(&sReceiveFrame))
    {
        radioSendAck();

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
        if (otMacFrameIsSecurityEnabled(&sAckFrame))
        {
            sReceiveFrame.mInfo.mRxInfo.mAckedWithSecEnhAck = true;
            sReceiveFrame.mInfo.mRxInfo.mAckFrameCounter    = otMacFrameGetFrameCounter(&sAckFrame);
        }
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    }

exit:

    if (error != OT_ERROR_ABORT)
    {
#if OPENTHREAD_CONFIG_DIAG_ENABLE
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
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sSrcMatchEnabled = aEnable;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aScanChannel);
    OT_UNUSED_VARIABLE(aScanDuration);

    assert(aInstance != NULL);
    assert(aScanChannel >= SIM_RADIO_CHANNEL_MIN && aScanChannel <= SIM_RADIO_CHANNEL_MAX);
    assert(aScanDuration > 0);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    int8_t maxPower = sChannelMaxTransmitPower[sCurrentChannel - kMinChannel];

    assert(aInstance != NULL);

    *aPower = sTxPower < maxPower ? sTxPower : maxPower;

    return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sTxPower = aPower;

    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    *aThreshold = sCcaEdThresh;

    return OT_ERROR_NONE;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sCcaEdThresh = aThreshold;

    return OT_ERROR_NONE;
}

otError otPlatRadioGetFemLnaGain(otInstance *aInstance, int8_t *aGain)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL && aGain != NULL);

    *aGain = sLnaGain;

    return OT_ERROR_NONE;
}

otError otPlatRadioSetFemLnaGain(otInstance *aInstance, int8_t aGain)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sLnaGain = aGain;

    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    return SIM_RECEIVE_SENSITIVITY;
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sState;
}

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
otError otPlatRadioSetCoexEnabled(otInstance *aInstance, bool aEnabled)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    sRadioCoexEnabled = aEnabled;
    return OT_ERROR_NONE;
}

bool otPlatRadioIsCoexEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    return sRadioCoexEnabled;
}

otError otPlatRadioGetCoexMetrics(otInstance *aInstance, otRadioCoexMetrics *aCoexMetrics)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    assert(aInstance != NULL);
    otEXPECT_ACTION(aCoexMetrics != NULL, error = OT_ERROR_INVALID_ARGS);

    memset(aCoexMetrics, 0, sizeof(otRadioCoexMetrics));

    aCoexMetrics->mStopped                            = false;
    aCoexMetrics->mNumGrantGlitch                     = 1;
    aCoexMetrics->mNumTxRequest                       = 2;
    aCoexMetrics->mNumTxGrantImmediate                = 3;
    aCoexMetrics->mNumTxGrantWait                     = 4;
    aCoexMetrics->mNumTxGrantWaitActivated            = 5;
    aCoexMetrics->mNumTxGrantWaitTimeout              = 6;
    aCoexMetrics->mNumTxGrantDeactivatedDuringRequest = 7;
    aCoexMetrics->mNumTxDelayedGrant                  = 8;
    aCoexMetrics->mAvgTxRequestToGrantTime            = 9;
    aCoexMetrics->mNumRxRequest                       = 10;
    aCoexMetrics->mNumRxGrantImmediate                = 11;
    aCoexMetrics->mNumRxGrantWait                     = 12;
    aCoexMetrics->mNumRxGrantWaitActivated            = 13;
    aCoexMetrics->mNumRxGrantWaitTimeout              = 14;
    aCoexMetrics->mNumRxGrantDeactivatedDuringRequest = 15;
    aCoexMetrics->mNumRxDelayedGrant                  = 16;
    aCoexMetrics->mAvgRxRequestToGrantTime            = 17;
    aCoexMetrics->mNumRxGrantNone                     = 18;

exit:
    return error;
}
#endif

uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return otPlatTimeGet();
}

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
static uint8_t generateAckIeData(uint8_t *aLinkMetricsIeData, uint8_t aLinkMetricsIeDataLen)
{
    OT_UNUSED_VARIABLE(aLinkMetricsIeData);
    OT_UNUSED_VARIABLE(aLinkMetricsIeDataLen);

    uint8_t offset = 0;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (sCslPeriod > 0)
    {
        offset += otMacFrameGenerateCslIeTemplate(sAckIeData);
    }
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    if (aLinkMetricsIeData != NULL && aLinkMetricsIeDataLen > 0)
    {
        offset += otMacFrameGenerateEnhAckProbingIe(sAckIeData, aLinkMetricsIeData, aLinkMetricsIeDataLen);
    }
#endif

    return offset;
}
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError otPlatRadioEnableCsl(otInstance *        aInstance,
                             uint32_t            aCslPeriod,
                             otShortAddress      aShortAddr,
                             const otExtAddress *aExtAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddr);
    OT_UNUSED_VARIABLE(aExtAddr);

    otError error = OT_ERROR_NONE;

    sCslPeriod = aCslPeriod;

    return error;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
    OT_UNUSED_VARIABLE(aInstance);

    sCslSampleTime = aCslSampleTime;
}

uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}
#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

void otPlatRadioSetMacKey(otInstance *            aInstance,
                          uint8_t                 aKeyIdMode,
                          uint8_t                 aKeyId,
                          const otMacKeyMaterial *aPrevKey,
                          const otMacKeyMaterial *aCurrKey,
                          const otMacKeyMaterial *aNextKey,
                          otRadioKeyType          aKeyType)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKeyIdMode);

    otEXPECT(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

    sKeyId   = aKeyId;
    sKeyType = aKeyType;
    memcpy(&sPrevKey, aPrevKey, sizeof(otMacKeyMaterial));
    memcpy(&sCurrKey, aCurrKey, sizeof(otMacKeyMaterial));
    memcpy(&sNextKey, aNextKey, sizeof(otMacKeyMaterial));

exit:
    return;
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMacFrameCounter = aMacFrameCounter;
}

otError otPlatRadioSetChannelMaxTransmitPower(otInstance *aInstance, uint8_t aChannel, int8_t aMaxPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aChannel >= kMinChannel && aChannel <= kMaxChannel, error = OT_ERROR_INVALID_ARGS);
    sChannelMaxTransmitPower[aChannel - kMinChannel] = aMaxPower;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
otError otPlatRadioConfigureEnhAckProbing(otInstance *         aInstance,
                                          otLinkMetrics        aLinkMetrics,
                                          const otShortAddress aShortAddress,
                                          const otExtAddress * aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return otLinkMetricsConfigureEnhAckProbing(aShortAddress, aExtAddress, aLinkMetrics);
}
#endif

otError otPlatRadioSetRegion(otInstance *aInstance, uint16_t aRegionCode)
{
    OT_UNUSED_VARIABLE(aInstance);

    sRegionCode = aRegionCode;
    return OT_ERROR_NONE;
}

otError otPlatRadioGetRegion(otInstance *aInstance, uint16_t *aRegionCode)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aRegionCode != NULL, error = OT_ERROR_INVALID_ARGS);

    *aRegionCode = sRegionCode;
exit:
    return error;
}

#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME
