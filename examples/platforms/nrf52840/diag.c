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

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform-nrf5.h"

#include <hal/nrf_gpio.h>

#include <openthread/cli.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

#include <common/logging.hpp>
#include <drivers/radio/nrf_802154.h>
#include <utils/code_utils.h>

typedef enum
{
    kDiagTransmitModeIdle,
    kDiagTransmitModePackets,
    kDiagTransmitModeCarrier
} DiagTrasmitMode;

struct PlatformDiagCommand
{
    const char *mName;
    void (*mCommand)(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen);
};

struct PlatformDiagMessage
{
    const char mMessageDescriptor[11];
    uint8_t    mChannel;
    int16_t    mID;
    uint32_t   mCnt;
};

/**
 * Diagnostics mode variables.
 *
 */
static bool                       sDiagMode         = false;
static bool                       sListen           = false;
static DiagTrasmitMode            sTransmitMode     = kDiagTransmitModeIdle;
static uint8_t                    sChannel          = 20;
static int8_t                     sTxPower          = 0;
static uint32_t                   sTxPeriod         = 1;
static int32_t                    sTxCount          = 0;
static int32_t                    sTxRequestedCount = 1;
static int16_t                    sID               = -1;
static struct PlatformDiagMessage sDiagMessage      = {.mMessageDescriptor = "DiagMessage",
                                                  .mChannel           = 0,
                                                  .mID                = 0,
                                                  .mCnt               = 0};

static otError parseLong(char *argv, long *aValue)
{
    char *endptr;
    *aValue = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

static void appendErrorResult(otError aError, char *aOutput, size_t aOutputMaxLen)
{
    if (aError != OT_ERROR_NONE)
    {
        snprintf(aOutput, aOutputMaxLen, "failed\r\nstatus %#x\r\n", aError);
    }
}

static bool startCarrierTransmision(void)
{
    nrf_802154_channel_set(sChannel);
    nrf_802154_tx_power_set(sTxPower);

    return nrf_802154_continuous_carrier();
}

static void processListen(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "listen: %s\r\n", sListen == true ? "yes" : "no");
    }
    else
    {
        long value;

        error = parseLong(argv[0], &value);
        otEXPECT(error == OT_ERROR_NONE);
        sListen = (bool)(value);
        snprintf(aOutput, aOutputMaxLen, "set listen to %s\r\nstatus 0x%02x\r\n", sListen == true ? "yes" : "no",
                 error);
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

static void processID(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "ID: %" PRId16 "\r\n", sID);
    }
    else
    {
        long value;

        error = parseLong(argv[0], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value >= 0, error = OT_ERROR_INVALID_ARGS);
        sID = (int16_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set ID to %" PRId16 "\r\nstatus 0x%02x\r\n", sID, error);
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

static void processTransmit(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen,
                 "transmit will send %" PRId32 " diagnostic messages with %" PRIu32 " ms interval\r\nstatus 0x%02x\r\n",
                 sTxRequestedCount, sTxPeriod, error);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        otEXPECT_ACTION(sTransmitMode != kDiagTransmitModeIdle, error = OT_ERROR_INVALID_STATE);

        otPlatAlarmMilliStop(aInstance);
        snprintf(aOutput, aOutputMaxLen, "diagnostic message transmission is stopped\r\nstatus 0x%02x\r\n", error);
        sTransmitMode = kDiagTransmitModeIdle;
        otPlatRadioReceive(aInstance, sChannel);
    }
    else if (strcmp(argv[0], "start") == 0)
    {
        otEXPECT_ACTION(sTransmitMode == kDiagTransmitModeIdle, error = OT_ERROR_INVALID_STATE);

        otPlatAlarmMilliStop(aInstance);
        sTransmitMode = kDiagTransmitModePackets;
        sTxCount      = sTxRequestedCount;
        uint32_t now  = otPlatAlarmMilliGetNow();
        otPlatAlarmMilliStartAt(aInstance, now, sTxPeriod);
        snprintf(aOutput, aOutputMaxLen,
                 "sending %" PRId32 " diagnostic messages with %" PRIu32 " ms interval\r\nstatus 0x%02x\r\n",
                 sTxRequestedCount, sTxPeriod, error);
    }
    else if (strcmp(argv[0], "carrier") == 0)
    {
        otEXPECT_ACTION(sTransmitMode == kDiagTransmitModeIdle, error = OT_ERROR_INVALID_STATE);

        otEXPECT_ACTION(startCarrierTransmision(), error = OT_ERROR_FAILED);

        sTransmitMode = kDiagTransmitModeCarrier;

        snprintf(aOutput, aOutputMaxLen, "sending carrier on channel %d with tx power %d\r\nstatus 0x%02x\r\n",
                 sChannel, sTxPower, error);
    }
    else if (strcmp(argv[0], "interval") == 0)
    {
        long value;

        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);

        error = parseLong(argv[1], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value > 0, error = OT_ERROR_INVALID_ARGS);
        sTxPeriod = (uint32_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set diagnostic messages interval to %" PRIu32 " ms\r\nstatus 0x%02x\r\n",
                 sTxPeriod, error);
    }
    else if (strcmp(argv[0], "count") == 0)
    {
        long value;

        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);

        error = parseLong(argv[1], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION((value > 0) || (value == -1), error = OT_ERROR_INVALID_ARGS);
        sTxRequestedCount = (uint32_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set diagnostic messages count to %" PRId32 "\r\nstatus 0x%02x\r\n",
                 sTxRequestedCount, error);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

static void processGpio(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);

    long    pinnum;
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 1)
    {
        uint32_t value;

        error = parseLong(argv[0], &pinnum);
        otEXPECT(error == OT_ERROR_NONE);

        value = nrf_gpio_pin_read(pinnum);

        snprintf(aOutput, aOutputMaxLen, "gpio %d = %d\r\n", (uint8_t)pinnum, (uint8_t)value);
    }
    else if (strcmp(argv[0], "set") == 0)
    {
        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);
        error = parseLong(argv[1], &pinnum);
        otEXPECT(error == OT_ERROR_NONE);

        nrf_gpio_pin_set(pinnum);

        snprintf(aOutput, aOutputMaxLen, "gpio %d = 1\r\n", (uint8_t)pinnum);
    }
    else if (strcmp(argv[0], "clr") == 0)
    {
        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);
        error = parseLong(argv[1], &pinnum);
        otEXPECT(error == OT_ERROR_NONE);

        nrf_gpio_pin_clear(pinnum);

        snprintf(aOutput, aOutputMaxLen, "gpio %d = 0\r\n", (uint8_t)pinnum);
    }
    else if (strcmp(argv[0], "out") == 0)
    {
        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);
        error = parseLong(argv[1], &pinnum);
        otEXPECT(error == OT_ERROR_NONE);

        nrf_gpio_cfg_output(pinnum);

        snprintf(aOutput, aOutputMaxLen, "gpio %d: out\r\n", (uint8_t)pinnum);
    }
    else if (strcmp(argv[0], "in") == 0)
    {
        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);
        error = parseLong(argv[1], &pinnum);
        otEXPECT(error == OT_ERROR_NONE);

        nrf_gpio_cfg_input(pinnum, NRF_GPIO_PIN_NOPULL);

        snprintf(aOutput, aOutputMaxLen, "gpio %d: in no pull\r\n", (uint8_t)pinnum);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

static void processTemp(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(argv);

    otError error = OT_ERROR_NONE;
    int32_t temperature;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    otEXPECT_ACTION(argc == 0, error = OT_ERROR_INVALID_ARGS);

    temperature = nrf5TempGet();

    // Measurement resolution is 0.25 degrees Celsius
    // Convert the temperature measurement to a decimal value, in degrees Celsius
    snprintf(aOutput, aOutputMaxLen, "%" PRId32 ".%02" PRId32 "\r\n", temperature / 4, 25 * (temperature % 4));

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

static void processCcaThreshold(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError              error = OT_ERROR_NONE;
    nrf_802154_cca_cfg_t ccaConfig;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 0)
    {
        nrf_802154_cca_cfg_get(&ccaConfig);

        snprintf(aOutput, aOutputMaxLen, "cca threshold: %u\r\n", ccaConfig.ed_threshold);
    }
    else
    {
        long value;
        error = parseLong(argv[0], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value >= 0 && value <= 0xFF, error = OT_ERROR_INVALID_ARGS);

        memset(&ccaConfig, 0, sizeof(ccaConfig));
        ccaConfig.mode         = NRF_RADIO_CCA_MODE_ED;
        ccaConfig.ed_threshold = (uint8_t)value;

        nrf_802154_cca_cfg_set(&ccaConfig);
        snprintf(aOutput, aOutputMaxLen, "set cca threshold to %u\r\nstatus 0x%02x\r\n", ccaConfig.ed_threshold, error);
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

const struct PlatformDiagCommand sCommands[] = {
    {"ccathreshold", &processCcaThreshold},
    {"gpio", &processGpio},
    {"id", &processID},
    {"listen", &processListen},
    {"temp", &processTemp},
    {"transmit", &processTransmit},
};

void otPlatDiagProcess(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    uint32_t i;

    for (i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
    {
        if (strcmp(argv[0], sCommands[i].mName) == 0)
        {
            sCommands[i].mCommand(aInstance, argc - 1, argc > 1 ? &argv[1] : NULL, aOutput, aOutputMaxLen);
            break;
        }
    }

    if (i == sizeof(sCommands) / sizeof(sCommands[0]))
    {
        snprintf(aOutput, aOutputMaxLen, "diag feature '%s' is not supported\r\n", argv[0]);
    }
}

void otPlatDiagModeSet(bool aMode)
{
    sDiagMode = aMode;

    if (!sDiagMode)
    {
        otPlatRadioReceive(NULL, sChannel);
        otPlatRadioSleep(NULL);
    }
    else
    {
        // Reinit
        sTransmitMode = kDiagTransmitModeIdle;
    }
}

bool otPlatDiagModeGet()
{
    return sDiagMode;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
    sChannel = aChannel;
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
    sTxPower = aTxPower;
}

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sListen && (aError == OT_ERROR_NONE))
    {
        if (aFrame->mLength == sizeof(struct PlatformDiagMessage))
        {
            struct PlatformDiagMessage *message = (struct PlatformDiagMessage *)aFrame->mPsdu;

            if (strncmp(message->mMessageDescriptor, "DiagMessage", 11) == 0)
            {
                otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_PLATFORM,
                          "{\"Frame\":{"
                          "\"LocalChannel\":%u ,"
                          "\"RemoteChannel\":%u,"
                          "\"CNT\":%" PRIu32 ","
                          "\"LocalID\":%" PRId16 ","
                          "\"RemoteID\":%" PRId16 ","
                          "\"RSSI\":%d"
                          "}}\r\n",
                          aFrame->mChannel, message->mChannel, message->mCnt, sID, message->mID,
                          aFrame->mInfo.mRxInfo.mRssi);
            }
        }
    }
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
    if (sTransmitMode == kDiagTransmitModePackets)
    {
        if ((sTxCount > 0) || (sTxCount == -1))
        {
            otRadioFrame *sTxPacket = otPlatRadioGetTransmitBuffer(aInstance);

            sTxPacket->mLength  = sizeof(struct PlatformDiagMessage);
            sTxPacket->mChannel = sChannel;

            sDiagMessage.mChannel = sTxPacket->mChannel;
            sDiagMessage.mID      = sID;

            memcpy(sTxPacket->mPsdu, &sDiagMessage, sizeof(struct PlatformDiagMessage));
            otPlatRadioTransmit(aInstance, sTxPacket);

            sDiagMessage.mCnt++;

            if (sTxCount != -1)
            {
                sTxCount--;
            }

            uint32_t now = otPlatAlarmMilliGetNow();
            otPlatAlarmMilliStartAt(aInstance, now, sTxPeriod);
        }
        else
        {
            sTransmitMode = kDiagTransmitModeIdle;
            otPlatAlarmMilliStop(aInstance);
            otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_PLATFORM, "Transmit done");
        }
    }
}
