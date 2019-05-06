/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#if OPENTHREAD_ENABLE_BLE_CONTROLLER
#include <openthread/platform/ble.h>
#include <openthread/platform/radio-ble.h>

#include "utils/code_utils.h"

/*
 * The timer accuracy of posix is less than microseconds. In order to guarantee that the sent message can hit
 * the receiving window, both the BLE_RADIO_TIFS_US and BLE_RADIO_RAMP_UP_US are set to a big value.
 *
 * The BLE_RADIO_TIFS_US should be set to 150us, BLE_RADIO_PREAMBLE_ADDR_US should be set to 40us and
 * BLE_RADIO_RAMP_UP_US should be set to the radio ramp-up time in the real BLE device.
 */
#define BLE_RADIO_TIFS_US 15000
#define BLE_RADIO_RAMP_UP_US 14000
#define BLE_RADIO_PREAMBLE_ADDR_US 14000

#define BLE_RADIO_TICKS_TO_US(n) (uint32_t)((uint64_t)(n)*1000000 / OPENTHREAD_CONFIG_BLE_BB_CLK_RATE_HZ)
#define BLE_RADIO_SOCKET_PORT_BASE 10000

enum
{
    kAccessAddressSize = 4,
    kCrcSize           = 3,
};

OT_TOOL_PACKED_BEGIN
struct BleRadioMessage
{
    uint8_t mChannel;
    uint8_t mPdu[kAccessAddressSize + OT_RADIO_BLE_FRAME_MAX_SIZE + kCrcSize];
} OT_TOOL_PACKED_END;

static otRadioBleFrame        sReceiveFrame;
static otRadioBleFrame        sTransmitFrame;
static struct BleRadioMessage sReceiveMessage;
static struct BleRadioMessage sTransmitMessage;

static int8_t   sTxPower    = 0;
static uint16_t sPortOffset = 0;
static int      sSockFd;

static otRadioBleSettings sTxSettings;
static otRadioBleTime     sTxTime;
static otRadioBleSettings sRxSettings;
static otRadioBleTime     sRxTime;

bool sTifsEnabled = false;
bool sTxAtTifs    = false;
bool sRxAtTifs    = false;

static uint8_t sState = OT_BLE_RADIO_STATE_IDLE;

static void bleRadioSendMessage(otInstance *aInstance);

static uint32_t bleTimeToUs(const otRadioBleTime *aTime)
{
    return BLE_RADIO_TICKS_TO_US(aTime->mTicks) + aTime->mOffsetUs;
}

static int32_t timeDiff(uint32_t aT0, uint32_t aT1)
{
    uint32_t diff = aT0 - aT1;
    return (diff & 0x80000000) ? (-1 * (int32_t)(aT1 - aT0)) : (int32_t)(aT0 - aT1);
}

static uint32_t bleRadioComputeCrc24(uint32_t aCrcInit, const uint8_t *aData, uint8_t aLength)
{
    uint32_t crc = aCrcInit;
    uint8_t  i, j, cur, next;

    for (i = 0; i < aLength; i++)
    {
        cur = aData[i];

        for (j = 0; j < 8; j++)
        {
            next = (crc ^ cur) & 1;

            cur >>= 1;
            crc >>= 1;

            if (next)
            {
                crc |= 1 << 23;
                crc ^= 0x5a6000;
            }
        }
    }

    return crc;
}

otError otPlatRadioBleEnable(otInstance *aInstance)
{
    if (sState != OT_BLE_RADIO_STATE_IDLE)
    {
        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatRadioBleDisable(otInstance *aInstance)
{
    if (sState != OT_BLE_RADIO_STATE_IDLE)
    {
        sState = OT_BLE_RADIO_STATE_DISABLED;
        platformBleAlarmMicroStop(aInstance);
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

uint32_t otPlatRadioBleGetTickNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return platformBleAlarmMicroGetNow();
}

int8_t otPlatRadioBleGetTransmitPower(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sTxPower;
}

otError otPlatRadioBleSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    sTxPower = aPower;
    return OT_ERROR_NONE;
}

void otPlatRadioBleEnableTifs(otInstance *aInstance)
{
    sTifsEnabled = true;
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioBleDisableTifs(otInstance *aInstance)
{
    sTifsEnabled = false;
    OT_UNUSED_VARIABLE(aInstance);
}

otRadioBleFrame *otPlatRadioBleGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sTransmitFrame;
}

uint32_t sTxTimeUs = 0;

otError otPlatRadioBleTransmitAtTime(otInstance *              aInstance,
                                     const otRadioBleSettings *aSettings,
                                     const otRadioBleTime *    aStartTime)
{
    otError  error = OT_ERROR_NONE;
    uint32_t now   = platformBleAlarmMicroGetNow();
    int32_t  dt;

    otEXPECT_ACTION((aSettings != NULL) && (aStartTime != NULL), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_IDLE, error = OT_ERROR_INVALID_STATE);

    sTxSettings = *aSettings;
    sTxTime     = *aStartTime;

    if ((dt = timeDiff(bleTimeToUs(aStartTime), now)) <= 0)
    {
        bleRadioSendMessage(aInstance);
    }
    else
    {
        sTxTimeUs = bleTimeToUs(aStartTime);

        sState = OT_BLE_RADIO_STATE_WAITING_TRANSMIT;
        platformBleAlarmMicroStartAt(aInstance, now, (uint32_t)(dt));
    }

exit:
    return error;
}

otError otPlatRadioBleTransmitAtTifs(otInstance *aInstance, const otRadioBleSettings *aSettings)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aSettings != NULL, error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS, error = OT_ERROR_INVALID_STATE);

    sTxAtTifs   = true;
    sTxSettings = *aSettings;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError otPlatRadioBleReceiveAtTime(otInstance *              aInstance,
                                    const otRadioBleSettings *aSettings,
                                    const otRadioBleTime *    aStartTime)
{
    otError  error = OT_ERROR_NONE;
    uint32_t now   = platformBleAlarmMicroGetNow();
    int32_t  dt    = timeDiff(bleTimeToUs(aStartTime), now);

    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_IDLE, error = OT_ERROR_INVALID_STATE);

    sRxSettings = *aSettings;
    sRxTime     = *aStartTime;

    if (dt <= BLE_RADIO_RAMP_UP_US)
    {
        uint32_t duration = sRxTime.mRxDurationUs + BLE_RADIO_PREAMBLE_ADDR_US;

        sState = OT_BLE_RADIO_STATE_RECEIVE;
        platformBleAlarmMicroStartAt(aInstance, platformBleAlarmMicroGetNow(), duration);
    }
    else
    {
        sState = OT_BLE_RADIO_STATE_WAITING_RECEIVE;
        platformBleAlarmMicroStartAt(aInstance, now, (uint32_t)(dt)-BLE_RADIO_RAMP_UP_US);
    }

exit:
    return error;
}

otError otPlatRadioBleReceiveAtTifs(otInstance *aInstance, const otRadioBleSettings *aSettings)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS, error = OT_ERROR_INVALID_STATE);

    sRxAtTifs   = true;
    sRxSettings = *aSettings;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError otPlatRadioBleCancelData(otInstance *aInstance)
{
    if ((sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT) || (sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE))
    {
        sState = OT_BLE_RADIO_STATE_IDLE;
        platformBleAlarmMicroStop(aInstance);
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatRadioBleCancelTifs(otInstance *aInstance)
{
    if ((sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS) || (sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS))
    {
        sTxAtTifs = false;
        sRxAtTifs = false;
        sState    = OT_BLE_RADIO_STATE_IDLE;
        platformBleAlarmMicroStop(aInstance);
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

uint8_t otPlatRadioBleGetXtalAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return 100;
}

void otPlatRadioBleGetPublicAddress(otInstance *aInstance, otPlatBleDeviceAddr *aAddress)
{
    memset(aAddress, 0, sizeof(otPlatBleDeviceAddr));

    aAddress->mAddrType            = OT_BLE_ADDRESS_TYPE_PUBLIC;
    *(uint32_t *)(aAddress->mAddr) = gNodeId;

    OT_UNUSED_VARIABLE(aInstance);

    return;
}

void platformBleAlarmMicroFired(otInstance *aInstance)
{
    switch (sState)
    {
    case OT_BLE_RADIO_STATE_WAITING_TRANSMIT:
        sState = OT_BLE_RADIO_STATE_TRANSMIT;
        bleRadioSendMessage(aInstance);
        break;

    case OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS:
        if (sTxAtTifs)
        {
            sTxAtTifs = false;
            sState    = OT_BLE_RADIO_STATE_TRANSMIT;
            bleRadioSendMessage(aInstance);
        }
        else
        {
            // transmit at TIFS timeout
            sState = OT_BLE_RADIO_STATE_IDLE;
            otPlatRadioBleTransmitDone(aInstance, OT_ERROR_FAILED);
        }

        break;

    case OT_BLE_RADIO_STATE_WAITING_RECEIVE:
    {
        uint32_t dt = sRxTime.mRxDurationUs + BLE_RADIO_PREAMBLE_ADDR_US;

        sState = OT_BLE_RADIO_STATE_RECEIVE;
        platformBleAlarmMicroStartAt(aInstance, platformBleAlarmMicroGetNow(), dt);
    }
    break;

    case OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS:
        sState = OT_BLE_RADIO_STATE_RECEIVE;
        platformBleAlarmMicroStartAt(aInstance, platformBleAlarmMicroGetNow(), BLE_RADIO_PREAMBLE_ADDR_US);
        break;

    case OT_BLE_RADIO_STATE_RECEIVE:
        // receive timeout
        sRxAtTifs = false;
        sState    = OT_BLE_RADIO_STATE_IDLE;
        otPlatRadioBleReceiveDone(aInstance, NULL, OT_ERROR_FAILED);
        break;

    default:
        break;
    }
}

void platformBleRadioInit(void)
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

    sockaddr.sin_port        = htons(BLE_RADIO_SOCKET_PORT_BASE + sPortOffset + gNodeId);
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

    sReceiveFrame.mPdu  = sReceiveMessage.mPdu + kAccessAddressSize;
    sTransmitFrame.mPdu = sTransmitMessage.mPdu + kAccessAddressSize;
    sState              = OT_BLE_RADIO_STATE_DISABLED;
}

static void bleRadioTransmit(struct BleRadioMessage *aMessage, const struct otRadioBleFrame *aFrame)
{
    uint32_t           i;
    struct sockaddr_in sockaddr;
    uint32_t           crc;
    uint16_t           length;

    aMessage->mChannel = sTxSettings.mChannel;

    // set access address field (little endian)
    aMessage->mPdu[0] = sTxSettings.mAccessAddress & 0x000000ff;
    aMessage->mPdu[1] = (sTxSettings.mAccessAddress >> 8) & 0x000000ff;
    aMessage->mPdu[2] = (sTxSettings.mAccessAddress >> 16) & 0x000000ff;
    aMessage->mPdu[3] = (sTxSettings.mAccessAddress >> 24) & 0x000000ff;

    crc = bleRadioComputeCrc24(sTxSettings.mCrcInit, aFrame->mPdu, aFrame->mLength);

    // set crc filed (little endian)
    aMessage->mPdu[kAccessAddressSize + aFrame->mLength]     = crc & 0x000000ff;
    aMessage->mPdu[kAccessAddressSize + aFrame->mLength + 1] = (crc >> 8) & 0x000000ff;
    aMessage->mPdu[kAccessAddressSize + aFrame->mLength + 2] = (crc >> 16) & 0x000000ff;

    // +1 for the size of channel
    length = 1 + kAccessAddressSize + aFrame->mLength + kCrcSize;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    for (i = 1; i <= WELLKNOWN_BLE_NODE_ID; i++)
    {
        ssize_t rval;

        if (gNodeId == i)
        {
            continue;
        }

        sockaddr.sin_port = htons(BLE_RADIO_SOCKET_PORT_BASE + sPortOffset + i);
        rval = sendto(sSockFd, (const char *)aMessage, length, 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

        if (rval < 0)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    }
}

static void bleRadioSendMessage(otInstance *aInstance)
{
    bool tifs_enabled = sTifsEnabled;

    bleRadioTransmit(&sTransmitMessage, &sTransmitFrame);

    if (tifs_enabled)
    {
        sState = OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS;
        platformBleAlarmMicroStartAt(aInstance, platformBleAlarmMicroGetNow(),
                                     BLE_RADIO_TIFS_US - BLE_RADIO_RAMP_UP_US);
    }
    else
    {
        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    otPlatRadioBleTransmitDone(aInstance, OT_ERROR_NONE);
}

static void bleRadioReceive(otInstance *aInstance)
{
    ssize_t rval = recvfrom(sSockFd, (char *)&sReceiveMessage, sizeof(sReceiveMessage), 0, NULL, NULL);

    if (rval < 0)
    {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    otEXPECT(rval > 1 + kAccessAddressSize + kCrcSize);
    sReceiveFrame.mLength = (uint8_t)(rval - 1 - kAccessAddressSize - kCrcSize);

    if ((sState == OT_BLE_RADIO_STATE_RECEIVE) && (sRxSettings.mChannel == sReceiveMessage.mChannel))
    {
        uint32_t access_address;
        uint32_t crc;

        access_address = sReceiveMessage.mPdu[0] | (sReceiveMessage.mPdu[1] << 8) | (sReceiveMessage.mPdu[2] << 16) |
                         (sReceiveMessage.mPdu[3] << 24);

        otEXPECT(sRxSettings.mAccessAddress == access_address);

        crc = sReceiveFrame.mPdu[sReceiveFrame.mLength] | (sReceiveFrame.mPdu[sReceiveFrame.mLength + 1] << 8) |
              (sReceiveFrame.mPdu[sReceiveFrame.mLength + 2] << 16);

        otEXPECT(crc == bleRadioComputeCrc24(sRxSettings.mCrcInit, sReceiveFrame.mPdu, sReceiveFrame.mLength));

        platformBleAlarmMicroStop(aInstance);

        if (sRxAtTifs)
        {
            sRxAtTifs = false;
        }

        if (sTifsEnabled)
        {
            sState = OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS;
            platformBleAlarmMicroStartAt(aInstance, platformBleAlarmMicroGetNow(),
                                         BLE_RADIO_TIFS_US - BLE_RADIO_RAMP_UP_US);
        }

        sReceiveFrame.mRxInfo.mRssi  = -20;
        sReceiveFrame.mRxInfo.mTicks = platformBleAlarmMicroGetNow();

        otPlatRadioBleReceiveDone(aInstance, &sReceiveFrame, OT_ERROR_NONE);
    }

exit:
    return;
}

void platformBleRadioUpdateFdSet(fd_set *aReadFdSet, fd_set *aWriteFdSet, int *aMaxFd)
{
    if (aReadFdSet != NULL && (sState != OT_BLE_RADIO_STATE_TRANSMIT))
    {
        FD_SET(sSockFd, aReadFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }

    if (aWriteFdSet != NULL && sState == OT_BLE_RADIO_STATE_TRANSMIT)
    {
        FD_SET(sSockFd, aWriteFdSet);

        if (aMaxFd != NULL && *aMaxFd < sSockFd)
        {
            *aMaxFd = sSockFd;
        }
    }
}

void platformBleRadioProcess(otInstance *aInstance)
{
    const int     flags  = POLLIN | POLLRDNORM | POLLERR | POLLNVAL | POLLHUP;
    struct pollfd pollfd = {sSockFd, flags, 0};

    if (POLL(&pollfd, 1, 0) > 0 && (pollfd.revents & flags) != 0)
    {
        bleRadioReceive(aInstance);
    }
}
#endif // OPENTHREAD_ENABLE_BLE_CONTROLLER
