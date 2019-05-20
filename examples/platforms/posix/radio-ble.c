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
#include <openthread/platform/cordio/radio-ble.h>

#include "utils/code_utils.h"

/*
 * The timer accuracy of posix is less than microseconds. In order to guarantee that the sent message can hit
 * the receiving window, BLE_RADIO_TIFS_US, BLE_RADIO_RAMP_UP_US and BLE_RADIO_PREAMBLE_ADDR_US are set to a big value.
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
    kAccessAddressSize     = 4,
    kPduHeaderSize         = 2,
    kPduHeaderLengthOffset = 1,
    kCrcSize               = 3,
};

OT_TOOL_PACKED_BEGIN
struct BleRadioMessage
{
    uint8_t mChannel;
    uint8_t mPdu[kAccessAddressSize + OT_RADIO_BLE_FRAME_MAX_SIZE + kCrcSize];
} OT_TOOL_PACKED_END;

static struct BleRadioMessage sReceiveMessage;
static struct BleRadioMessage sTransmitMessage;

static int8_t   sTxPower    = 0;
static uint16_t sPortOffset = 0;
static int      sSockFd;

static otRadioBleTime          sRxTime;
static otRadioBleChannelParams sChannelParams;

static otRadioBleBufferDescriptor sBufferDescriptor = {NULL, 0};

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
    if (sState == OT_BLE_RADIO_STATE_DISABLED)
    {
        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

otError otPlatRadioBleDisable(otInstance *aInstance)
{
    if (sState != OT_BLE_RADIO_STATE_DISABLED)
    {
        platformBleAlarmMicroStop(aInstance);
        sState = OT_BLE_RADIO_STATE_DISABLED;
    }

    OT_UNUSED_VARIABLE(aInstance);
    return OT_ERROR_NONE;
}

void otPlatRadioBleEnableInterrupt(void)
{
}

void otPlatRadioBleDisableInterrupt(void)
{
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

otError otPlatRadioBleSetChannelParameters(otInstance *aInstance, const otRadioBleChannelParams *aChannelParams)
{
    sChannelParams = *aChannelParams;

    OT_UNUSED_VARIABLE(aInstance);
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

otError otPlatRadioBleTransmitAtTime(otInstance *                aInstance,
                                     otRadioBleBufferDescriptor *aBufferDescriptors,
                                     uint8_t                     aNumBufferDescriptors,
                                     const otRadioBleTime *      aStartTime)
{
    otError  error = OT_ERROR_NONE;
    uint32_t now   = platformBleAlarmMicroGetNow();
    uint8_t *txBuf = sTransmitMessage.mPdu + kAccessAddressSize;
    int32_t  dt;
    uint16_t offset;
    uint8_t  i;

    otEXPECT_ACTION((aBufferDescriptors != NULL) && (aStartTime != NULL), error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_IDLE, error = OT_ERROR_INVALID_STATE);

    for (i = 0, offset = 0; i < aNumBufferDescriptors; i++)
    {
        memcpy((uint8_t *)txBuf + offset, aBufferDescriptors[i].mBuffer, aBufferDescriptors[i].mLength);
        offset += aBufferDescriptors[i].mLength;
    }

    if ((dt = timeDiff(bleTimeToUs(aStartTime), now)) <= 0)
    {
        bleRadioSendMessage(aInstance);
    }
    else
    {
        sState = OT_BLE_RADIO_STATE_WAITING_TRANSMIT;
        platformBleAlarmMicroStartAt(aInstance, now, (uint32_t)(dt));
    }

exit:
    return error;
}

otError otPlatRadioBleTransmitAtTifs(otInstance *                aInstance,
                                     otRadioBleBufferDescriptor *aBufferDescriptors,
                                     uint8_t                     aNumBufferDescriptors)
{
    otError  error = OT_ERROR_NONE;
    uint8_t *txBuf = sTransmitMessage.mPdu + kAccessAddressSize;
    uint16_t offset;
    uint8_t  i;

    otEXPECT_ACTION(aBufferDescriptors != NULL, error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS, error = OT_ERROR_INVALID_STATE);

    sTxAtTifs = true;

    for (i = 0, offset = 0; i < aNumBufferDescriptors; i++)
    {
        memcpy((uint8_t *)txBuf + offset, aBufferDescriptors[i].mBuffer, aBufferDescriptors[i].mLength);
        offset += aBufferDescriptors[i].mLength;
    }

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

otError otPlatRadioBleReceiveAtTime(otInstance *                aInstance,
                                    otRadioBleBufferDescriptor *aBufferDescriptor,
                                    const otRadioBleTime *      aStartTime)
{
    otError  error = OT_ERROR_NONE;
    uint32_t now   = platformBleAlarmMicroGetNow();
    int32_t  dt    = timeDiff(bleTimeToUs(aStartTime), now);

    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_IDLE, error = OT_ERROR_INVALID_STATE);

    sBufferDescriptor = *aBufferDescriptor;
    sRxTime           = *aStartTime;

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

otError otPlatRadioBleReceiveAtTifs(otInstance *aInstance, otRadioBleBufferDescriptor *aBufferDescriptor)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS, error = OT_ERROR_INVALID_STATE);

    sBufferDescriptor = *aBufferDescriptor;
    sRxAtTifs         = true;

    OT_UNUSED_VARIABLE(aInstance);

exit:
    return error;
}

void otPlatRadioBleCancelData(otInstance *aInstance)
{
    if ((sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT) || (sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE))
    {
        platformBleAlarmMicroStop(aInstance);
        sState = OT_BLE_RADIO_STATE_IDLE;
    }

    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatRadioBleCancelTifs(otInstance *aInstance)
{
    if ((sState == OT_BLE_RADIO_STATE_WAITING_RECEIVE_TIFS) || (sState == OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS))
    {
        platformBleAlarmMicroStop(aInstance);

        sTxAtTifs = false;
        sRxAtTifs = false;
        sState    = OT_BLE_RADIO_STATE_IDLE;
    }

    OT_UNUSED_VARIABLE(aInstance);
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
        if (sRxAtTifs)
        {
            sRxAtTifs = false;
            sState    = OT_BLE_RADIO_STATE_RECEIVE;
            platformBleAlarmMicroStartAt(aInstance, platformBleAlarmMicroGetNow(), BLE_RADIO_PREAMBLE_ADDR_US);
        }
        else
        {
            sState = OT_BLE_RADIO_STATE_IDLE;
            otPlatRadioBleReceiveDone(aInstance, NULL, OT_BLE_RADIO_ERROR_FAILED);
        }

        break;

    case OT_BLE_RADIO_STATE_RECEIVE:
        sState = OT_BLE_RADIO_STATE_IDLE;
        otPlatRadioBleReceiveDone(aInstance, NULL, OT_BLE_RADIO_ERROR_RX_TIMEOUT);
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

    sState = OT_BLE_RADIO_STATE_DISABLED;
}

static void bleRadioTransmit(struct BleRadioMessage *aMessage)
{
    struct sockaddr_in sockaddr;
    uint16_t           length    = kPduHeaderSize + aMessage->mPdu[kAccessAddressSize + kPduHeaderLengthOffset];
    uint16_t           crcOffset = kAccessAddressSize + length;
    uint32_t           crc;

    aMessage->mChannel = sChannelParams.mChannel;

    // set access address field (little endian)
    aMessage->mPdu[0] = sChannelParams.mAccessAddress & 0x000000ff;
    aMessage->mPdu[1] = (sChannelParams.mAccessAddress >> 8) & 0x000000ff;
    aMessage->mPdu[2] = (sChannelParams.mAccessAddress >> 16) & 0x000000ff;
    aMessage->mPdu[3] = (sChannelParams.mAccessAddress >> 24) & 0x000000ff;

    // set crc filed (little endian)
    crc = bleRadioComputeCrc24(sChannelParams.mCrcInit, &aMessage->mPdu[kAccessAddressSize], length);
    aMessage->mPdu[crcOffset]     = crc & 0x000000ff;
    aMessage->mPdu[crcOffset + 1] = (crc >> 8) & 0x000000ff;
    aMessage->mPdu[crcOffset + 2] = (crc >> 16) & 0x000000ff;

    // +1 for the size of channel
    length = 1 + kAccessAddressSize + length + kCrcSize;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);

    for (uint8_t i = 1; i <= WELLKNOWN_BLE_NODE_ID; i++)
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

    bleRadioTransmit(&sTransmitMessage);

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
    ssize_t          rval  = recvfrom(sSockFd, (char *)&sReceiveMessage, sizeof(sReceiveMessage), 0, NULL, NULL);
    otRadioBleError  error = OT_BLE_RADIO_ERROR_NONE;
    otRadioBleRxInfo rxInfo;

    if (rval < 0)
    {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    otEXPECT(rval >= 1 + kAccessAddressSize + kPduHeaderSize + kCrcSize);

    if ((sState == OT_BLE_RADIO_STATE_RECEIVE) && (sChannelParams.mChannel == sReceiveMessage.mChannel))
    {
        uint32_t accessAddress;
        uint16_t length    = kPduHeaderSize + sReceiveMessage.mPdu[kAccessAddressSize + kPduHeaderLengthOffset];
        uint16_t crcOffset = kAccessAddressSize + length;
        uint32_t rxCrc;

        accessAddress = sReceiveMessage.mPdu[0] | (sReceiveMessage.mPdu[1] << 8) | (sReceiveMessage.mPdu[2] << 16) |
                        (sReceiveMessage.mPdu[3] << 24);

        otEXPECT(sChannelParams.mAccessAddress == accessAddress);

        rxCrc = sReceiveMessage.mPdu[crcOffset] | (sReceiveMessage.mPdu[crcOffset + 1] << 8) |
                (sReceiveMessage.mPdu[crcOffset + 2] << 16);

        if (rxCrc != bleRadioComputeCrc24(sChannelParams.mCrcInit, &sReceiveMessage.mPdu[kAccessAddressSize], length))
        {
            error = OT_BLE_RADIO_ERROR_CRC;
        }

        if (length > sBufferDescriptor.mLength)
        {
            error = OT_BLE_RADIO_ERROR_CRC;
        }
        else
        {
            memcpy(sBufferDescriptor.mBuffer, sReceiveMessage.mPdu + kAccessAddressSize, length);
        }

        rxInfo.mRssi  = -20;
        rxInfo.mTicks = platformBleAlarmMicroGetNow();

        platformBleAlarmMicroStop(aInstance);

        if (sTifsEnabled)
        {
            sState = OT_BLE_RADIO_STATE_WAITING_TRANSMIT_TIFS;
            platformBleAlarmMicroStartAt(aInstance, platformBleAlarmMicroGetNow(),
                                         BLE_RADIO_TIFS_US - BLE_RADIO_RAMP_UP_US);
        }

        otPlatRadioBleReceiveDone(aInstance, &rxInfo, error);
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
