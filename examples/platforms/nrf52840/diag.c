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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <openthread/platform/diag.h>
#include <openthread/platform/alarm.h>
#include <openthread/platform/radio.h>

#include <common/logging.hpp>
#include <utils/code_utils.h>

struct PlatformDiagCommand
{
    const char *mName;
    void (*mCommand)(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen);
};

struct PlatformDiagMessage
{
    const char mMessageDescriptor[11];
    uint8_t mChannel;
    int16_t mID;
    uint32_t mCnt;
};

/**
 * Diagnostics mode variables.
 *
 */
static bool sDiagMode = false;
static bool sListen = false;
static bool sTransmitActive = false;
static uint8_t sChannel = 20;
static int8_t sTxPower = 0;
static uint32_t sTxPeriod = 1;
static int32_t sTxCount = 0;
static int32_t sTxRequestedCount = 1;
static int16_t sID = -1;
static struct PlatformDiagMessage sDiagMessage = {.mMessageDescriptor = "DiagMessage", .mChannel = 0, .mID = 0, .mCnt = 0};

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

static void processListen(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    (void) aInstance;
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
        snprintf(aOutput, aOutputMaxLen, "set listen to %s\r\nstatus 0x%02x\r\n", sListen == true ? "yes" : "no", error);
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

static void processID(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    (void) aInstance;

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

static void processTransmit(otInstance *aInstance, int argc, char *argv[], char *aOutput,
                            size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "transmit will send %" PRId32 " diagnostic messages with %" PRIu32
                 " ms interval\r\nstatus 0x%02x\r\n",
                 sTxRequestedCount, sTxPeriod, error);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        otPlatAlarmStop(aInstance);
        snprintf(aOutput, aOutputMaxLen, "diagnostic message transmission is stopped\r\nstatus 0x%02x\r\n", error);
        sTransmitActive = false;
    }
    else if (strcmp(argv[0], "start") == 0)
    {
        otPlatAlarmStop(aInstance);
        sTransmitActive = true;
        sTxCount = sTxRequestedCount;
        uint32_t now = otPlatAlarmGetNow();
        otPlatAlarmStartAt(aInstance, now, sTxPeriod);
        snprintf(aOutput, aOutputMaxLen, "sending %" PRId32 " diagnostic messages with %" PRIu32
                 " ms interval\r\nstatus 0x%02x\r\n",
                 sTxRequestedCount, sTxPeriod, error);
    }
    else if (strcmp(argv[0], "interval") == 0)
    {
        long value;

        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);

        error = parseLong(argv[1], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION(value > 0, error = OT_ERROR_INVALID_ARGS);
        sTxPeriod = (uint32_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set diagnostic messages interval to %" PRIu32 " ms\r\nstatus 0x%02x\r\n", sTxPeriod,
                 error);
    }
    else if (strcmp(argv[0], "count") == 0)
    {
        long value;

        otEXPECT_ACTION(argc == 2, error = OT_ERROR_INVALID_ARGS);

        error = parseLong(argv[1], &value);
        otEXPECT(error == OT_ERROR_NONE);
        otEXPECT_ACTION((value > 0) || (value == -1), error = OT_ERROR_INVALID_ARGS);
        sTxRequestedCount = (uint32_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set diagnostic messages count to %" PRId32 "\r\nstatus 0x%02x\r\n", sTxRequestedCount,
                 error);
    }

exit:
    appendErrorResult(error, aOutput, aOutputMaxLen);
}

const struct PlatformDiagCommand sCommands[] =
{
    {"listen", &processListen },
    {"transmit", &processTransmit },
    {"id", &processID }
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
    (void) aInstance;

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
                          aFrame->mChannel,
                          message->mChannel,
                          message->mCnt,
                          sID,
                          message->mID,
                          aFrame->mPower
                         );
            }
        }
    }
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
    if (sTransmitActive)
    {
        if ((sTxCount > 0) || (sTxCount == -1))
        {
            otRadioFrame *sTxPacket = otPlatRadioGetTransmitBuffer(aInstance);

            sTxPacket->mLength = sizeof(struct PlatformDiagMessage);
            sTxPacket->mChannel = sChannel;
            sTxPacket->mPower = sTxPower;

            sDiagMessage.mChannel = sTxPacket->mChannel;
            sDiagMessage.mID = sID;

            memcpy(sTxPacket->mPsdu, &sDiagMessage, sizeof(struct PlatformDiagMessage));
            otPlatRadioTransmit(aInstance, sTxPacket);

            sDiagMessage.mCnt++;

            if (sTxCount != -1)
            {
                sTxCount--;
            }

            uint32_t now = otPlatAlarmGetNow();
            otPlatAlarmStartAt(aInstance, now, sTxPeriod);
        }
        else
        {
            sTransmitActive = false;
            otPlatAlarmStop(aInstance);
            otPlatLog(OT_LOG_LEVEL_DEBG, OT_LOG_REGION_PLATFORM, "Transmit done");
        }
    }
}
