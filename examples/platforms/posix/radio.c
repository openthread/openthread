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

#include "platform-posix.h"

#include <errno.h>

#include <openthread/dataset.h>
#include <openthread/random_noncrypto.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "utils/code_utils.h"
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
    POSIX_RECEIVE_SENSITIVITY = -100, // dBm

    POSIX_HIGH_RSSI_SAMPLE               = -30, // dBm
    POSIX_LOW_RSSI_SAMPLE                = -98, // dBm
    POSIX_HIGH_RSSI_PROB_INC_PER_CHANNEL = 5,
};

#if OPENTHREAD_POSIX_VIRTUAL_TIME
extern int      sSockFd;
extern uint16_t sPortOffset;
#else
static int      sTxFd       = -1;
static int      sRxFd       = -1;
static uint16_t sPortOffset = 0;
static uint16_t sPort       = 0;
#endif

enum
{
    POSIX_RADIO_CHANNEL_MIN = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MIN,
    POSIX_RADIO_CHANNEL_MAX = OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MAX,
};

OT_TOOL_PACKED_BEGIN
struct RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[OT_RADIO_FRAME_MAX_SIZE];
} OT_TOOL_PACKED_END;

static void radioTransmit(struct RadioMessage *aMessage, const struct otRadioFrame *aFrame);
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

static bool sSrcMatchEnabled = false;

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
static bool sRadioCoexEnabled = true;
#endif

static void ReverseExtAddress(otExtAddress *aReversed, const otExtAddress *aOrigin)
{
    for (size_t i = 0; i < sizeof(*aReversed); i++)
    {
        aReversed->m8[i] = aOrigin->m8[sizeof(*aOrigin) - 1 - i];
    }
}

static bool isDataRequestAndHasFramePending(const otRadioFrame *aFrame)
{
    bool         rval = false;
    otMacAddress src;

    otEXPECT(otMacFrameIsDataRequest(aFrame));
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
    assert(aInstance != NULL);

    sPanid = aPanid;
    utilsSoftSrcMatchSetPanId(aPanid);
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aExtAddress)
{
    assert(aInstance != NULL);

    ReverseExtAddress(&sExtAddress, aExtAddress);
}

void otPlatRadioSetShortAddress(otInstance *aInstance, otShortAddress aAddress)
{
    assert(aInstance != NULL);

    sShortAddress = aAddress;
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    assert(aInstance != NULL);

    sPromiscuous = aEnable;
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0
static void initFds(void)
{
    int                fd;
    int                one = 1;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));

    otEXPECT_ACTION((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != -1, perror("socket(sTxFd)"));

    sPort                    = (uint16_t)(9000 + sPortOffset + gNodeId);
    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons(sPort);
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &sockaddr.sin_addr, sizeof(sockaddr.sin_addr)) != -1,
                    perror("setsockopt(sTxFd, IP_MULTICAST_IF)"));

    otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &one, sizeof(one)) != -1,
                    perror("setsockopt(sRxFd, IP_MULTICAST_LOOP)"));

    otEXPECT_ACTION(bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != -1, perror("bind(sTxFd)"));

    // Tx fd is successfully initialized.
    sTxFd = fd;

    otEXPECT_ACTION((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != -1, perror("socket(sRxFd)"));

    otEXPECT_ACTION(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) != -1,
                    perror("setsockopt(sRxFd, SO_REUSEADDR)"));
    otEXPECT_ACTION(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) != -1,
                    perror("setsockopt(sRxFd, SO_REUSEPORT)"));

    {
        struct ip_mreqn mreq;

        memset(&mreq, 0, sizeof(mreq));
        inet_pton(AF_INET, OT_RADIO_GROUP, &mreq.imr_multiaddr);

        // Always use loopback device to send simulation packets.
        mreq.imr_address.s_addr = inet_addr("127.0.0.1");

        otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq.imr_address, sizeof(mreq.imr_address)) != -1,
                        perror("setsockopt(sRxFd, IP_MULTICAST_IF)"));
        otEXPECT_ACTION(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1,
                        perror("setsockopt(sRxFd, IP_ADD_MEMBERSHIP)"));
    }

    sockaddr.sin_family      = AF_INET;
    sockaddr.sin_port        = htons((uint16_t)(9000 + sPortOffset + WELLKNOWN_NODE_ID));
    sockaddr.sin_addr.s_addr = inet_addr(OT_RADIO_GROUP);

    otEXPECT_ACTION(bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != -1, perror("bind(sRxFd)"));

    // Rx fd is successfully initialized.
    sRxFd = fd;

exit:
    if (sRxFd == -1 || sTxFd == -1)
    {
        exit(EXIT_FAILURE);
    }
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME == 0

void platformRadioInit(void)
{
#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0
    char *offset;

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

    initFds();
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME == 0

    sReceiveFrame.mPsdu  = sReceiveMessage.mPsdu;
    sTransmitFrame.mPsdu = sTransmitMessage.mPsdu;
    sAckFrame.mPsdu      = sAckMessage.mPsdu;

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    sTransmitFrame.mInfo.mTxInfo.mIeInfo = &sTransmitIeInfo;
#else
    sTransmitFrame.mInfo.mTxInfo.mIeInfo = NULL;
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
    otError error = OT_ERROR_NONE;

    otEXPECT(otPlatRadioIsEnabled(aInstance));
    otEXPECT_ACTION(sState == OT_RADIO_STATE_SLEEP, error = OT_ERROR_INVALID_STATE);

    sState = OT_RADIO_STATE_DISABLED;

exit:
    return error;
}

otError otPlatRadioSleep(otInstance *aInstance)
{
    assert(aInstance != NULL);

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
    assert(aInstance != NULL);

    otError error = OT_ERROR_INVALID_STATE;

    if (sState != OT_RADIO_STATE_DISABLED)
    {
        error                  = OT_ERROR_NONE;
        sState                 = OT_RADIO_STATE_RECEIVE;
        sTxWait                = false;
        sReceiveFrame.mChannel = aChannel;
    }

    return error;
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aRadio)
{
    assert(aInstance != NULL);
    assert(aRadio != NULL);

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
    assert(aInstance != NULL);

    return &sTransmitFrame;
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    assert(aInstance != NULL);

    int8_t   rssi    = POSIX_LOW_RSSI_SAMPLE;
    uint8_t  channel = sReceiveFrame.mChannel;
    uint32_t probabilityThreshold;

    otEXPECT((POSIX_RADIO_CHANNEL_MIN <= channel) && channel <= (POSIX_RADIO_CHANNEL_MAX));

    // To emulate a simple interference model, we return either a high or
    // a low  RSSI value with a fixed probability per each channel. The
    // probability is increased per channel by a constant.

    probabilityThreshold = (channel - POSIX_RADIO_CHANNEL_MIN) * POSIX_HIGH_RSSI_PROB_INC_PER_CHANNEL;

    if (otRandomNonCryptoGetUint16() < (probabilityThreshold * 0xffff / 100))
    {
        rssi = POSIX_HIGH_RSSI_SAMPLE;
    }

exit:
    return rssi;
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    assert(aInstance != NULL);

    return OT_RADIO_CAPS_NONE;
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    assert(aInstance != NULL);

    return sPromiscuous;
}

static void radioReceive(otInstance *aInstance)
{
    bool isTxDone = false;
    bool isAck    = otMacFrameIsAck(&sReceiveFrame);

    otEXPECT(sReceiveFrame.mChannel == sReceiveMessage.mChannel);
    otEXPECT(sState == OT_RADIO_STATE_RECEIVE || sState == OT_RADIO_STATE_TRANSMIT);

    // Unable to simulate SFD, so use the rx done timestamp instead.
    sReceiveFrame.mInfo.mRxInfo.mTimestamp = otPlatTimeGet();

    if (sTxWait)
    {
        if (otMacFrameIsAckRequested(&sTransmitFrame))
        {
            isTxDone = isAck && otMacFrameGetSequence(&sReceiveFrame) == otMacFrameGetSequence(&sTransmitFrame);
        }
#if OPENTHREAD_POSIX_VIRTUAL_TIME
        // Simulate tx done when receiving the echo frame.
        else
        {
            isTxDone = !isAck && sTransmitFrame.mLength == sReceiveFrame.mLength &&
                       memcmp(sTransmitFrame.mPsdu, sReceiveFrame.mPsdu, sTransmitFrame.mLength) == 0;
        }
#endif
    }

    if (isTxDone)
    {
        sState  = OT_RADIO_STATE_RECEIVE;
        sTxWait = false;

#if OPENTHREAD_CONFIG_DIAG_ENABLE

        if (otPlatDiagModeGet())
        {
            otPlatDiagRadioTransmitDone(aInstance, &sTransmitFrame, OT_ERROR_NONE);
        }
        else
#endif
        {
            otPlatRadioTxDone(aInstance, &sTransmitFrame, (isAck ? &sReceiveFrame : NULL), OT_ERROR_NONE);
        }
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

void radioSendMessage(otInstance *aInstance)
{
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    bool notifyFrameUpdated = false;

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
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

        notifyFrameUpdated = true;
    }
#endif // OPENTHREAD_CONFIG_TIME_SYNC_ENABLE

    if (notifyFrameUpdated)
    {
        otMacFrameProcessTransmitAesCcm(&sTransmitFrame, &sExtAddress);
    }
#endif // OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT

    sTransmitMessage.mChannel = sTransmitFrame.mChannel;

    otPlatRadioTxStarted(aInstance, &sTransmitFrame);
    radioComputeCrc(&sTransmitMessage, sTransmitFrame.mLength);
    radioTransmit(&sTransmitMessage, &sTransmitFrame);

#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0
    sTxWait = otMacFrameIsAckRequested(&sTransmitFrame);

    if (!sTxWait)
    {
        sState = OT_RADIO_STATE_RECEIVE;

#if OPENTHREAD_CONFIG_DIAG_ENABLE

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
#else
    // Wait for echo radio in virtual time mode.
    sTxWait = true;
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME
}

bool platformRadioIsTransmitPending(void)
{
    return sState == OT_RADIO_STATE_TRANSMIT && !sTxWait;
}

#if OPENTHREAD_POSIX_VIRTUAL_TIME
void platformRadioReceive(otInstance *aInstance, uint8_t *aBuf, uint16_t aBufLength)
{
    assert(sizeof(sReceiveMessage) >= aBufLength);

    memcpy(&sReceiveMessage, aBuf, aBufLength);

    sReceiveFrame.mLength = (uint8_t)(aBufLength - 1);

    radioReceive(aInstance);
}
#else
void platformRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
    if (aReadFdSet != NULL && (sState != OT_RADIO_STATE_TRANSMIT || sTxWait))
    {
        FD_SET(sRxFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sRxFd)
        {
            *aMaxFd = sRxFd;
        }
    }

    if (aWriteFdSet != NULL && platformRadioIsTransmitPending())
    {
        FD_SET(sTxFd, aWriteFdSet);

        if (aMaxFd != NULL && *aMaxFd < sTxFd)
        {
            *aMaxFd = sTxFd;
        }
    }
}

// no need to close in virtual time mode.
void platformRadioDeinit(void)
{
    if (sRxFd != -1)
    {
        close(sRxFd);
    }

    if (sTxFd != -1)
    {
        close(sTxFd);
    }
}
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME

void platformRadioProcess(otInstance *aInstance, const fd_set *aReadFdSet, const fd_set *aWriteFdSet)
{
    OT_UNUSED_VARIABLE(aReadFdSet);
    OT_UNUSED_VARIABLE(aWriteFdSet);

#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0
    if (FD_ISSET(sRxFd, aReadFdSet))
    {
        struct sockaddr_in sockaddr;
        socklen_t          len = sizeof(sockaddr);
        ssize_t            rval;

        memset(&sockaddr, 0, sizeof(sockaddr));
        rval =
            recvfrom(sRxFd, (char *)&sReceiveMessage, sizeof(sReceiveMessage), 0, (struct sockaddr *)&sockaddr, &len);

        if (rval > 0)
        {
            if (sockaddr.sin_port != htons(sPort))
            {
                sReceiveFrame.mLength = (uint16_t)(rval - 1);

                radioReceive(aInstance);
            }
        }
        else if (rval == 0)
        {
            // socket is closed, which should not happen
            assert(false);
        }
        else if (errno != EINTR && errno != EAGAIN)
        {
            perror("recvfrom(sRxFd)");
            exit(EXIT_FAILURE);
        }
    }
#endif

    if (platformRadioIsTransmitPending())
    {
        radioSendMessage(aInstance);
    }
}

void radioTransmit(struct RadioMessage *aMessage, const struct otRadioFrame *aFrame)
{
#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0
    ssize_t            rval;
    struct sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, OT_RADIO_GROUP, &sockaddr.sin_addr);

    sockaddr.sin_port = htons((uint16_t)(9000 + sPortOffset + WELLKNOWN_NODE_ID));
    rval =
        sendto(sTxFd, (const char *)aMessage, 1 + aFrame->mLength, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    if (rval < 0)
    {
        perror("sendto(sTxFd)");
        exit(EXIT_FAILURE);
    }
#else  // OPENTHREAD_POSIX_VIRTUAL_TIME == 0
    struct Event event;

    event.mDelay      = 1; // 1us for now
    event.mEvent      = OT_SIM_EVENT_RADIO_RECEIVED;
    event.mDataLength = 1 + aFrame->mLength; // include channel in first byte
    memcpy(event.mData, aMessage, event.mDataLength);

    otSimSendEvent(&event);
#endif // OPENTHREAD_POSIX_VIRTUAL_TIME == 0
}

void radioSendAck(void)
{
    sAckFrame.mLength    = IEEE802154_ACK_LENGTH;
    sAckMessage.mPsdu[0] = IEEE802154_FRAME_TYPE_ACK;

    if (isDataRequestAndHasFramePending(&sReceiveFrame))
    {
        sAckMessage.mPsdu[0] |= IEEE802154_FRAME_PENDING;
        sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending = true;
    }

    sAckMessage.mPsdu[1] = 0;
    sAckMessage.mPsdu[2] = otMacFrameGetSequence(&sReceiveFrame);

    sAckMessage.mChannel = sReceiveFrame.mChannel;

    radioComputeCrc(&sAckMessage, sAckFrame.mLength);
    radioTransmit(&sAckMessage, &sAckFrame);
}

void radioProcessFrame(otInstance *aInstance)
{
    otError error = OT_ERROR_NONE;

    sReceiveFrame.mInfo.mRxInfo.mRssi = -20;
    sReceiveFrame.mInfo.mRxInfo.mLqi  = OT_RADIO_LQI_NONE;

    sReceiveFrame.mInfo.mRxInfo.mAckedWithFramePending = false;

    otEXPECT(sPromiscuous == false);

    otEXPECT_ACTION(otMacFrameDoesAddrMatch(&sReceiveFrame, sPanid, sShortAddress, &sExtAddress),
                    error = OT_ERROR_ABORT);

    // generate acknowledgment
    if (otMacFrameIsAckRequested(&sReceiveFrame))
    {
        radioSendAck();
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
    assert(aInstance != NULL);

    sSrcMatchEnabled = aEnable;
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    assert(aInstance != NULL);
    assert(aScanChannel >= POSIX_RADIO_CHANNEL_MIN && aScanChannel <= POSIX_RADIO_CHANNEL_MAX);
    assert(aScanDuration > 0);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    assert(aInstance != NULL);

    *aPower = sTxPower;

    return OT_ERROR_NONE;
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    assert(aInstance != NULL);

    sTxPower = aPower;

    return OT_ERROR_NONE;
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    assert(aInstance != NULL);

    *aThreshold = sCcaEdThresh;

    return OT_ERROR_NONE;
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    assert(aInstance != NULL);

    sCcaEdThresh = aThreshold;

    return OT_ERROR_NONE;
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    assert(aInstance != NULL);

    return POSIX_RECEIVE_SENSITIVITY;
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sState;
}

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
otError otPlatRadioSetCoexEnabled(otInstance *aInstance, bool aEnabled)
{
    assert(aInstance != NULL);

    sRadioCoexEnabled = aEnabled;
    return OT_ERROR_NONE;
}

bool otPlatRadioIsCoexEnabled(otInstance *aInstance)
{
    assert(aInstance != NULL);

    return sRadioCoexEnabled;
}

otError otPlatRadioGetCoexMetrics(otInstance *aInstance, otRadioCoexMetrics *aCoexMetrics)
{
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
