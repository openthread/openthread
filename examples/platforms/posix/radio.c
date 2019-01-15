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

#include "platform-posix.h"

#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0

#include <openthread/dataset.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/random.h>
#include <openthread/platform/time.h>

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
    POSIX_RECEIVE_SENSITIVITY   = -100, // dBm
    POSIX_MAX_SRC_MATCH_ENTRIES = OPENTHREAD_CONFIG_MAX_CHILDREN,

    POSIX_HIGH_RSSI_SAMPLE               = -30, // dBm
    POSIX_LOW_RSSI_SAMPLE                = -98, // dBm
    POSIX_HIGH_RSSI_PROB_INC_PER_CHANNEL = 5,
};

OT_TOOL_PACKED_BEGIN
struct RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[OT_RADIO_FRAME_MAX_SIZE];
} OT_TOOL_PACKED_END;

static void radioTransmit(struct RadioMessage *msg, const struct otRadioFrame *pkt);
static void radioSendMessage(otInstance *aInstance);
static void radioSendAck(void);
static void radioProcessFrame(otInstance *aInstance);

static otRadioState        sState = OT_RADIO_STATE_DISABLED;
static struct RadioMessage sReceiveMessage;
static struct RadioMessage sTransmitMessage;
static struct RadioMessage sAckMessage;
static otRadioFrame        sReceiveFrame;
static otRadioFrame        sTransmitFrame;
static otRadioFrame        sAckFrame;

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
static otRadioIeInfo sTransmitIeInfo;
static otRadioIeInfo sReceivedIeInfo;
#endif

static uint8_t  sExtendedAddress[OT_EXT_ADDRESS_SIZE];
static uint16_t sShortAddress;
static uint16_t sPanid;
static uint16_t sPortOffset = 0;
static int      sSockFd;
static bool     sPromiscuous = false;
static bool     sAckWait     = false;
static int8_t   sTxPower     = 0;

static uint8_t      sShortAddressMatchTableCount = 0;
static uint8_t      sExtAddressMatchTableCount   = 0;
static uint16_t     sShortAddressMatchTable[POSIX_MAX_SRC_MATCH_ENTRIES];
static otExtAddress sExtAddressMatchTable[POSIX_MAX_SRC_MATCH_ENTRIES];
static bool         sSrcMatchEnabled = false;

static bool findShortAddress(uint16_t aShortAddress)
{
    uint8_t i;

    for (i = 0; i < sShortAddressMatchTableCount; ++i)
    {
        if (sShortAddressMatchTable[i] == aShortAddress)
        {
            break;
        }
    }

    return i < sShortAddressMatchTableCount;
}

static bool findExtAddress(const otExtAddress *aExtAddress)
{
    uint8_t i;

    for (i = 0; i < sExtAddressMatchTableCount; ++i)
    {
        if (!memcmp(&sExtAddressMatchTable[i], aExtAddress, sizeof(otExtAddress)))
        {
            break;
        }
    }

    return i < sExtAddressMatchTableCount;
}

static inline bool isFrameTypeAck(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_ACK;
}

static inline bool isFrameTypeMacCmd(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_FRAME_TYPE_MASK) == IEEE802154_FRAME_TYPE_MACCMD;
}

static inline bool isSecurityEnabled(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_SECURITY_ENABLED) != 0;
}

static inline bool isAckRequested(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_ACK_REQUEST) != 0;
}

static inline bool isPanIdCompressed(const uint8_t *frame)
{
    return (frame[0] & IEEE802154_PANID_COMPRESSION) != 0;
}

static inline bool isDataRequestAndHasFramePending(const uint8_t *frame)
{
    const uint8_t *cur = frame;
    uint8_t        securityControl;
    bool           isDataRequest   = false;
    bool           hasFramePending = false;

    // FCF + DSN
    cur += 2 + 1;

    otEXPECT(isFrameTypeMacCmd(frame));

    // Destination PAN + Address
    switch (frame[1] & IEEE802154_DST_ADDR_MASK)
    {
    case IEEE802154_DST_ADDR_SHORT:
        cur += sizeof(otPanId) + sizeof(otShortAddress);
        break;

    case IEEE802154_DST_ADDR_EXT:
        cur += sizeof(otPanId) + sizeof(otExtAddress);
        break;

    default:
        goto exit;
    }

    // Source PAN + Address
    switch (frame[1] & IEEE802154_SRC_ADDR_MASK)
    {
    case IEEE802154_SRC_ADDR_SHORT:
        if (!isPanIdCompressed(frame))
        {
            cur += sizeof(otPanId);
        }

        if (sSrcMatchEnabled)
        {
            hasFramePending = findShortAddress((uint16_t)(cur[1] << 8 | cur[0]));
        }

        cur += sizeof(otShortAddress);
        break;

    case IEEE802154_SRC_ADDR_EXT:
        if (!isPanIdCompressed(frame))
        {
            cur += sizeof(otPanId);
        }

        if (sSrcMatchEnabled)
        {
            hasFramePending = findExtAddress((const otExtAddress *)cur);
        }

        cur += sizeof(otExtAddress);
        break;

    default:
        goto exit;
    }

    // Security Control + Frame Counter + Key Identifier
    if (isSecurityEnabled(frame))
    {
        securityControl = *cur;

        if (securityControl & IEEE802154_SEC_LEVEL_MASK)
        {
            cur += 1 + 4;
        }

        switch (securityControl & IEEE802154_KEY_ID_MODE_MASK)
        {
        case IEEE802154_KEY_ID_MODE_0:
            cur += 0;
            break;

        case IEEE802154_KEY_ID_MODE_1:
            cur += 1;
            break;

        case IEEE802154_KEY_ID_MODE_2:
            cur += 5;
            break;

        case IEEE802154_KEY_ID_MODE_3:
            cur += 9;
            break;
        }
    }

    // Command ID
    isDataRequest = cur[0] == IEEE802154_MACCMD_DATA_REQ;

exit:
    return isDataRequest && hasFramePending;
}

static inline uint8_t getDsn(const uint8_t *frame)
{
    return frame[IEEE802154_DSN_OFFSET];
}

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

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
    OT_UNUSED_VARIABLE(aInstance);
    sPanid = panid;
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    for (size_t i = 0; i < sizeof(sExtendedAddress); i++)
    {
        sExtendedAddress[i] = aExtAddress->m8[sizeof(sExtendedAddress) - 1 - i];
    }
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t address)
{
    OT_UNUSED_VARIABLE(aInstance);
    sShortAddress = address;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    sPromiscuous = aEnable;
}

void platformRadioInit(void)
{
    struct sockaddr_in sockaddr;
    char *             offset;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;

    offset = getenv("PORT_OFFSET");

    if (offset)
    {
        char *endptr;

        sPortOffset = (uint16_t)strtol(offset, &endptr, 0);

        if (*endptr != '\0')
        {
            fprintf(stderr, "Invalid PORT_OFFSET: %s\n", offset);
            exit(EXIT_FAILURE);
        }

        sPortOffset *= WELLKNOWN_NODE_ID;
    }

    if (sPromiscuous)
    {
        sockaddr.sin_port = htons(9000 + sPortOffset + WELLKNOWN_NODE_ID);
    }
    else
    {
        sockaddr.sin_port = htons(9000 + sPortOffset + gNodeId);
    }

    sockaddr.sin_addr.s_addr = INADDR_ANY;

    sSockFd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sSockFd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(sSockFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    sReceiveFrame.mPsdu  = sReceiveMessage.mPsdu;
    sTransmitFrame.mPsdu = sTransmitMessage.mPsdu;
    sAckFrame.mPsdu      = sAckMessage.mPsdu;

#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
    sTransmitFrame.mIeInfo = &sTransmitIeInfo;
    sReceiveFrame.mIeInfo  = &sReceivedIeInfo;
#else
    sTransmitFrame.mIeInfo = NULL;
    sReceiveFrame.mIeInfo  = NULL;
#endif
}

void platformRadioDeinit(void)
{
#ifndef _WIN32
    close(sSockFd);
#else
    closesocket(sSockFd);
#endif
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (sState != OT_RADIO_STATE_DISABLED) ? true : false;
}

otError otPlatRadioEnable(otInstance *aInstance)
{
    if (!otPlatRadioIsEnabled(aInstance))
    {
        sState = OT_RADIO_STATE_SLEEP;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioDisable(otInstance *aInstance)
{
    if (otPlatRadioIsEnabled(aInstance))
    {
        sState = OT_RADIO_STATE_DISABLED;
    }

    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_SLEEP || sState == OT_RADIO_STATE_RECEIVE)
    {
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_SLEEP;
    }

    return error;
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState != OT_RADIO_STATE_DISABLED)
    {
        error                  = OT_ERROR_NONE;
        sState                 = OT_RADIO_STATE_RECEIVE;
        sAckWait               = false;
        sReceiveFrame.mChannel = aChannel;
    }

    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aRadio)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aRadio);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState == OT_RADIO_STATE_RECEIVE)
    {
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_TRANSMIT;
    }

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

    int8_t   rssi    = POSIX_LOW_RSSI_SAMPLE;
    uint8_t  channel = sReceiveFrame.mChannel;
    uint32_t probabilityThreshold;

    otEXPECT((OT_RADIO_CHANNEL_MIN <= channel) && channel <= (OT_RADIO_CHANNEL_MAX));

    // To emulate a simple interference model, we return either a high or
    // a low  RSSI value with a fixed probability per each channel. The
    // probability is increased per channel by a constant.

    probabilityThreshold = (channel - OT_RADIO_CHANNEL_MIN) * POSIX_HIGH_RSSI_PROB_INC_PER_CHANNEL;

    if ((otPlatRandomGet() & 0xffff) < (probabilityThreshold * 0xffff / 100))
    {
        rssi = POSIX_HIGH_RSSI_SAMPLE;
    }

exit:
    return rssi;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_RADIO_CAPS_NONE;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sPromiscuous;
}

void radioReceive(otInstance *aInstance)
{
    bool    isAck;
    ssize_t rval = recvfrom(sSockFd, (char *)&sReceiveMessage, sizeof(sReceiveMessage), 0, NULL, NULL);

    if (rval < 0)
    {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    if (otPlatRadioGetPromiscuous(aInstance))
    {
        // Timestamp
        sReceiveFrame.mInfo.mRxInfo.mMsec = otPlatAlarmMilliGetNow();
        sReceiveFrame.mInfo.mRxInfo.mUsec = 0; // Don't support microsecond timer for now.
    }

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    sReceiveFrame.mIeInfo->mTimestamp = otPlatTimeGet();
#endif

    sReceiveFrame.mLength = (uint8_t)(rval - 1);

    isAck = isFrameTypeAck(sReceiveFrame.mPsdu);

    if (sAckWait && sTransmitFrame.mChannel == sReceiveMessage.mChannel && isAck &&
        getDsn(sReceiveFrame.mPsdu) == getDsn(sTransmitFrame.mPsdu))
    {
        sState   = OT_RADIO_STATE_RECEIVE;
        sAckWait = false;

        otPlatRadioTxDone(aInstance, &sTransmitFrame, &sReceiveFrame, OT_ERROR_NONE);
    }
    else if ((sState == OT_RADIO_STATE_RECEIVE || sState == OT_RADIO_STATE_TRANSMIT) &&
             (sReceiveFrame.mChannel == sReceiveMessage.mChannel) && (!isAck || sPromiscuous))
    {
        radioProcessFrame(aInstance);
    }
}

void radioSendMessage(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
    bool notifyFrameUpdated = false;

#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
    if (sTransmitFrame.mIeInfo->mTimeIeOffset != 0)
    {
        uint8_t *timeIe = sTransmitFrame.mPsdu + sTransmitFrame.mIeInfo->mTimeIeOffset;
        uint64_t time   = (uint64_t)((int64_t)otPlatTimeGet() + sTransmitFrame.mIeInfo->mNetworkTimeOffset);

        *timeIe = sTransmitFrame.mIeInfo->mTimeSyncSeq;

        *(++timeIe) = (uint8_t)(time & 0xff);
        for (uint8_t i = 1; i < sizeof(uint64_t); i++)
        {
            time        = time >> 8;
            *(++timeIe) = (uint8_t)(time & 0xff);
        }

        notifyFrameUpdated = true;
    }
#endif // OPENTHREAD_CONFIG_ENABLE_TIME_SYNC

    if (notifyFrameUpdated)
    {
        otPlatRadioFrameUpdated(aInstance, &sTransmitFrame);
    }
#endif // OPENTHREAD_CONFIG_HEADER_IE_SUPPORT

    sTransmitMessage.mChannel = sTransmitFrame.mChannel;

    otPlatRadioTxStarted(aInstance, &sTransmitFrame);
    radioTransmit(&sTransmitMessage, &sTransmitFrame);

    sAckWait = isAckRequested(sTransmitFrame.mPsdu);

    if (!sAckWait)
    {
        sState = OT_RADIO_STATE_RECEIVE;

#if OPENTHREAD_ENABLE_DIAG

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NONE);
        }
        else
#endif
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_NONE);
        }
    }
}

void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
    if (aReadFdSet != NULL && (sState != OT_RADIO_STATE_TRANSMIT || sAckWait))
    {
        FD_SET(sSockFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }

    if (aWriteFdSet != NULL && sState == OT_RADIO_STATE_TRANSMIT && !sAckWait)
    {
        FD_SET(sSockFd, aWriteFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }
}

void platformRadioProcess(otInstance *aInstance)
{
    const int     flags  = POLLIN | POLLRDNORM | POLLERR | POLLNVAL | POLLHUP;
    struct pollfd pollfd = {sSockFd, flags, 0};

    if (POLL(&pollfd, 1, 0) > 0 && (pollfd.revents & flags) != 0)
    {
        radioReceive(aInstance);
    }

    if (sState == OT_RADIO_STATE_TRANSMIT && !sAckWait)
    {
        radioSendMessage(aInstance);
    }
}

static void radioComputeCrc(struct RadioMessage *aMessage, uint16_t aLength)
{
    uint16_t i;
    uint16_t crc        = 0;
    uint16_t crc_offset = aLength - sizeof(uint16_t);

    for (i = 0; i < crc_offset; i++)
    {
        crc = crc16_citt(crc, aMessage->mPsdu[i]);
    }

    aMessage->mPsdu[crc_offset]     = crc & 0xff;
    aMessage->mPsdu[crc_offset + 1] = crc >> 8;
}

void radioTransmit(struct RadioMessage *aMessage, const struct otRadioFrame *aFrame)
{
    uint32_t           i;
    struct sockaddr_in sockaddr;

    if (!sPromiscuous)
    {
        radioComputeCrc(aMessage, aFrame->mLength);
    }

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    for (i = 1; i <= WELLKNOWN_NODE_ID; i++)
    {
        ssize_t rval;

        if (gNodeId == i)
        {
            continue;
        }

        sockaddr.sin_port = htons(9000 + sPortOffset + i);
        rval = sendto(sSockFd, (const char *)aMessage, 1 + aFrame->mLength, 0, (struct sockaddr *)&sockaddr,
                      sizeof(sockaddr));

        if (rval < 0)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    }
}

void radioSendAck(void)
{
    sAckFrame.mLength    = IEEE802154_ACK_LENGTH;
    sAckMessage.mPsdu[0] = IEEE802154_FRAME_TYPE_ACK;

    if (isDataRequestAndHasFramePending(sReceiveFrame.mPsdu))
    {
        sAckMessage.mPsdu[0] |= IEEE802154_FRAME_PENDING;
    }

    sAckMessage.mPsdu[1] = 0;
    sAckMessage.mPsdu[2] = getDsn(sReceiveFrame.mPsdu);

    sAckMessage.mChannel = sReceiveFrame.mChannel;

    radioTransmit(&sAckMessage, &sAckFrame);
}

void radioProcessFrame(otInstance *aInstance)
{
    otError        error = OT_ERROR_NONE;
    otPanId        dstpan;
    otShortAddress short_address;
    otExtAddress   ext_address;

    otEXPECT_ACTION(sPromiscuous == false, error = OT_ERROR_NONE);

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

    sReceiveFrame.mInfo.mRxInfo.mRssi = -20;
    sReceiveFrame.mInfo.mRxInfo.mLqi  = OT_RADIO_LQI_NONE;

    // generate acknowledgment
    if (isAckRequested(sReceiveFrame.mPsdu))
    {
        radioSendAck();
    }

exit:

    if (error != OT_ERROR_ABORT)
    {
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
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    OT_UNUSED_VARIABLE(aInstance);

    sSrcMatchEnabled = aEnable;
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(sShortAddressMatchTableCount < sizeof(sShortAddressMatchTable) / sizeof(uint16_t),
                    error = OT_ERROR_NO_BUFS);

    for (uint8_t i = 0; i < sShortAddressMatchTableCount; ++i)
    {
        otEXPECT_ACTION(sShortAddressMatchTable[i] != aShortAddress, error = OT_ERROR_DUPLICATED);
    }

    sShortAddressMatchTable[sShortAddressMatchTableCount++] = aShortAddress;

exit:
    return error;
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sExtAddressMatchTableCount < sizeof(sExtAddressMatchTable) / sizeof(otExtAddress),
                    error = OT_ERROR_NO_BUFS);

    for (uint8_t i = 0; i < sExtAddressMatchTableCount; ++i)
    {
        otEXPECT_ACTION(memcmp(&sExtAddressMatchTable[i], aExtAddress, sizeof(otExtAddress)),
                        error = OT_ERROR_DUPLICATED);
    }

    sExtAddressMatchTable[sExtAddressMatchTableCount++] = *aExtAddress;

exit:
    return error;
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NOT_FOUND;
    otEXPECT(sShortAddressMatchTableCount > 0);

    for (uint8_t i = 0; i < sShortAddressMatchTableCount; ++i)
    {
        if (sShortAddressMatchTable[i] == aShortAddress)
        {
            sShortAddressMatchTable[i] = sShortAddressMatchTable[--sShortAddressMatchTableCount];
            error                      = OT_ERROR_NONE;
            goto exit;
        }
    }

exit:
    return error;
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NOT_FOUND;

    otEXPECT(sExtAddressMatchTableCount > 0);

    for (uint8_t i = 0; i < sExtAddressMatchTableCount; ++i)
    {
        if (!memcmp(&sExtAddressMatchTable[i], aExtAddress, sizeof(otExtAddress)))
        {
            sExtAddressMatchTable[i] = sExtAddressMatchTable[--sExtAddressMatchTableCount];
            error                    = OT_ERROR_NONE;
            goto exit;
        }
    }

exit:
    return error;
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sShortAddressMatchTableCount = 0;
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sExtAddressMatchTableCount = 0;
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
    OT_UNUSED_VARIABLE(aInstance);

    *aPower = sTxPower;

    return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    sTxPower = aPower;

    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return POSIX_RECEIVE_SENSITIVITY;
}

#endif // OPENTHREAD_POSIX_VIRTUAL_TIME == 0
