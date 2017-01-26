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

#include <platform/diag.h>
#include <platform/logging.h>
#include <platform/alarm.h>

#include <common/code_utils.hpp>

static void otPlatDiagProcessListen(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen);
static void otPlatDiagProcessID(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen);
static void otPlatDiagProcessTransmit(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen);
static ThreadError otPlatDiagParseLong(char *argv, long *value);
static void otPlatDiagAppendErrorResult(ThreadError aError, char *aOutput, size_t aOutputMaxLen);

struct DiagCommand
{
    const char *mName;
    void (*mCommand)(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen);
};

struct DiagMessage
{
    const char messageDescriptor[11];
    uint8_t channel;
    int16_t id;
    uint64_t cnt;
};

const struct DiagCommand sCommands[] =
{
        {"listen", &otPlatDiagProcessListen },
        {"transmit", &otPlatDiagProcessTransmit },
        {"id", &otPlatDiagProcessID }
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
static int32_t sTxCount = 1;
static int32_t sTxRequestedCount = 1;
static int16_t sID = -1;
static struct DiagMessage sDiagMessage = {.messageDescriptor = "DiagMessage", .channel = 0, .id = 0, .cnt = 0};

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

void otPlatDiagChannelSet(uint8_t aChannel){
    sChannel = aChannel;
}

void otPlatDiagTxPowerSet(int8_t aTxPower){
    sTxPower = aTxPower;
}

void otPlatDiagRadioReceived(otInstance *aInstance, RadioPacket *aFrame, ThreadError aError){
    (void) aInstance;

    if(sListen && (aError == kThreadError_None))
    {
        if(aFrame->mLength == sizeof(struct DiagMessage))
        {
            struct DiagMessage *message = (struct DiagMessage*)aFrame->mPsdu;
            if(strncmp(message->messageDescriptor, "DiagMessage", 11) == 0)
            {
                otPlatLog(kLogLevelDebg, kLogRegionNetDiag, "{\"Frame\": {"
                            "\"LocalChannel\": %d, "
                            "\"RemoteChannel\": %d, "
                            "\"CNT\": %d, \"LocalID\": %d, "
                            "\"RemoteID\": %d, "
                            "\"RSSI\": %d"
                            "}}\r\n",
                            (int)(aFrame->mChannel),
                            (int)(message->channel),
                            (int)(message->cnt),
                            (int)(sID),
                            (int)(message->id),
                            (int)(aFrame->mPower)
                        );
            }
        }
    }
}

void otPlatDiagAlarmCallback(otInstance *aInstance){
    if(sTransmitActive)
    {
        if((sTxCount > 0) || (sTxCount == -1))
        {
            RadioPacket *sTxPacket = otPlatRadioGetTransmitBuffer(aInstance);

            sTxPacket->mLength = sizeof(struct DiagMessage);
            sTxPacket->mChannel = sChannel;
            sTxPacket->mPower = sTxPower;

            sDiagMessage.channel = sTxPacket->mChannel;
            sDiagMessage.id = sID;

            memcpy(sTxPacket->mPsdu, &sDiagMessage, sizeof(struct DiagMessage));
            otPlatRadioTransmit(aInstance, sTxPacket);

            sDiagMessage.cnt++;

            if(sTxCount != -1)
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
            otPlatLog(kLogLevelDebg, kLogRegionNetDiag, "Transmit done");
        }
    }
}

static ThreadError otPlatDiagParseLong(char *argv, long *value)
{
    char *endptr;
    *value = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? kThreadError_None : kThreadError_Parse;
}

static void otPlatDiagAppendErrorResult(ThreadError aError, char *aOutput, size_t aOutputMaxLen)
{
    if (aError != kThreadError_None)
    {
        snprintf(aOutput, aOutputMaxLen, "failed\r\nstatus %#x\r\n", aError);
    }
}

static void otPlatDiagProcessListen(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    (void) aInstance;
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "listen: %s\r\n", sListen==true?"yes":"no");
    }
    else
    {
        long value;

        SuccessOrExit(error = otPlatDiagParseLong(argv[0], &value));
        sListen = (bool)(value);
        snprintf(aOutput, aOutputMaxLen, "set listen to %s\r\nstatus 0x%02x\r\n", sListen==true?"yes":"no", error);
    }

exit:
    otPlatDiagAppendErrorResult(error, aOutput, aOutputMaxLen);
}

static void otPlatDiagProcessID(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    (void) aInstance;

    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "ID: %d\r\n", (int)sID);
    }
    else
    {
        long value;

        SuccessOrExit(error = otPlatDiagParseLong(argv[0], &value));
        VerifyOrExit(value>=0, error = kThreadError_InvalidArgs);
        sID = (int16_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set ID to %d\r\nstatus 0x%02x\r\n", (int)sID, error);
    }

exit:
    otPlatDiagAppendErrorResult(error, aOutput, aOutputMaxLen);
}

static void otPlatDiagProcessTransmit(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "transmit will send %d diagnostic messages with %d ms interval\r\nstatus 0x%02x\r\n",
                (int)(sTxRequestedCount), (int)(sTxPeriod), error);
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
        snprintf(aOutput, aOutputMaxLen, "sending %d diagnostic messages with %d ms interval\r\nstatus 0x%02x\r\n",
                (int)(sTxRequestedCount), (int)(sTxPeriod), error);
    }
    else if (strcmp(argv[0], "interval") == 0)
    {
        long value;

        VerifyOrExit(argc == 2, error = kThreadError_InvalidArgs);

        SuccessOrExit(error = otPlatDiagParseLong(argv[1], &value));
        VerifyOrExit(value>0, error = kThreadError_InvalidArgs);
        sTxPeriod = (uint32_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set diagnostic messages interval to %d ms\r\nstatus 0x%02x\r\n", (int)(sTxPeriod), error);
    }
    else if (strcmp(argv[0], "count") == 0)
    {
        long value;

        VerifyOrExit(argc == 2, error = kThreadError_InvalidArgs);

        SuccessOrExit(error = otPlatDiagParseLong(argv[1], &value));
        VerifyOrExit((value > 0) || (value == -1), error = kThreadError_InvalidArgs);
        sTxRequestedCount = (uint32_t)(value);
        snprintf(aOutput, aOutputMaxLen, "set diagnostic messages count to %d\r\nstatus 0x%02x\r\n", (int)(sTxRequestedCount), error);
    }
exit:
    otPlatDiagAppendErrorResult(error, aOutput, aOutputMaxLen);
}
