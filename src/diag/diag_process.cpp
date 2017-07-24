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

/**
 * @file
 *   This file implements the diagnostics module.
 */

#include <openthread/config.h>

#include <stdio.h>
#include <stdlib.h>

#include <openthread/instance.h>

#include "diag_process.hpp"
#include "common/code_utils.hpp"
#include "utils/wrap_string.h"

namespace ot {
namespace Diagnostics {

const struct Command Diag::sCommands[] =
{
    { "start", &ProcessStart },
    { "stop", &ProcessStop },
    { "channel", &ProcessChannel },
    { "power", &ProcessPower },
    { "send", &ProcessSend },
    { "repeat", &ProcessRepeat },
    { "sleep", &ProcessSleep },
    { "stats", &ProcessStats },
};

char Diag::sDiagOutput[MAX_DIAG_OUTPUT];
struct DiagStats Diag::sStats;

int8_t Diag::sTxPower;
uint8_t Diag::sChannel;
uint8_t Diag::sTxLen;
uint32_t Diag::sTxPeriod;
uint32_t Diag::sTxPackets;
otRadioFrame * Diag::sTxPacket;
bool Diag::sRepeatActive;

otInstance *Diag::sContext;

void Diag::Init(otInstance *aInstance)
{
    sContext = aInstance;
    sChannel = 20;
    sTxPower = 0;
    sTxPeriod = 0;
    sTxLen = 0;
    sTxPackets = 0;
    sRepeatActive = false;
    memset(&sStats, 0, sizeof(struct DiagStats));
    sTxPacket = otPlatRadioGetTransmitBuffer(sContext);
    otPlatDiagChannelSet(sChannel);
    otPlatDiagTxPowerSet(sTxPower);
}

char *Diag::ProcessCmd(int argc, char *argv[])
{
    unsigned int i;

    if (argc == 0)
    {
        snprintf(sDiagOutput, sizeof(sDiagOutput), "diagnostics mode is %s\r\n", otPlatDiagModeGet() ? "enabled" : "disabled");
    }
    else
    {
        for (i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                sCommands[i].mCommand(argc - 1, argc > 1 ? &argv[1] : NULL, sDiagOutput, sizeof(sDiagOutput));
                break;
            }
        }

        // more platform specific features will be processed under platform layer
        if (i == sizeof(sCommands) / sizeof(sCommands[0]))
        {
            otPlatDiagProcess(sContext, argc, argv, sDiagOutput, sizeof(sDiagOutput));
        }
    }

    return sDiagOutput;
}

bool Diag::isEnabled()
{
    return otPlatDiagModeGet();
}

void Diag::AppendErrorResult(otError aError, char *aOutput, size_t aOutputMaxLen)
{
    if (aError != OT_ERROR_NONE)
    {
        snprintf(aOutput, aOutputMaxLen, "failed\r\nstatus %#x\r\n", aError);
    }
}

void Diag::ProcessStart(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    // enable radio
    otPlatRadioEnable(sContext);

    // enable promiscuous mode
    otPlatRadioSetPromiscuous(sContext, true);

    // stop timer
    otPlatAlarmMilliStop(sContext);

    // start to listen on the default channel
    SuccessOrExit(error = otPlatRadioReceive(sContext, sChannel));

    // enable diagnostics mode
    otPlatDiagModeSet(true);

    memset(&sStats, 0, sizeof(struct DiagStats));

    snprintf(aOutput, aOutputMaxLen, "start diagnostics mode\r\nstatus 0x%02x\r\n", error);

exit:
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessStop(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    otPlatAlarmMilliStop(sContext);
    otPlatDiagModeSet(false);
    otPlatRadioSetPromiscuous(sContext, false);

    snprintf(aOutput, aOutputMaxLen, "received packets: %d\r\nsent packets: %d\r\nfirst received packet: rssi=%d, lqi=%d\r\n\nstop diagnostics mode\r\nstatus 0x%02x\r\n",
            static_cast<int>(sStats.received_packets), static_cast<int>(sStats.sent_packets), static_cast<int>(sStats.first_rssi),
            static_cast<int>(sStats.first_lqi), error);

exit:
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

otError Diag::ParseLong(char *argv, long &value)
{
    char *endptr;
    value = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

void Diag::TxPacket()
{
    sTxPacket->mLength = sTxLen;
    sTxPacket->mChannel = sChannel;
    sTxPacket->mPower = sTxPower;

    for (uint8_t i = 0; i < sTxLen; i++)
    {
        sTxPacket->mPsdu[i] = i;
    }
    otPlatRadioTransmit(sContext, sTxPacket);
}

void Diag::ProcessChannel(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "channel: %d\r\n", sChannel);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(argv[0], value));
        VerifyOrExit(value >= OT_RADIO_CHANNEL_MIN && value <= OT_RADIO_CHANNEL_MAX, error = OT_ERROR_INVALID_ARGS);
        sChannel = static_cast<uint8_t>(value);

        // listen on the set channel immediately
        otPlatRadioReceive(sContext, sChannel);
        otPlatDiagChannelSet(sChannel);
        snprintf(aOutput, aOutputMaxLen, "set channel to %d\r\nstatus 0x%02x\r\n", sChannel, error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessPower(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (argc == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "tx power: %d dBm\r\n", sTxPower);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(argv[0], value));
        sTxPower = static_cast<int8_t>(value);
        otPlatDiagTxPowerSet(sTxPower);
        snprintf(aOutput, aOutputMaxLen, "set tx power to %d dBm\r\nstatus 0x%02x\r\n", sTxPower, error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessSend(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;
    long value;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(argv[0], value));
    sTxPackets = static_cast<uint32_t>(value);

    SuccessOrExit(error = ParseLong(argv[1], value));
    VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
    sTxLen = static_cast<uint8_t>(value);

    snprintf(aOutput, aOutputMaxLen, "sending %#x packet(s), length %#x\r\nstatus 0x%02x\r\n", static_cast<int>(sTxPackets), static_cast<int>(sTxLen), error);
    TxPacket();

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessRepeat(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(argc > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(argv[0], "stop") == 0)
    {
        otPlatAlarmMilliStop(sContext);
        sRepeatActive = false;
        snprintf(aOutput, aOutputMaxLen, "repeated packet transmission is stopped\r\nstatus 0x%02x\r\n", error);
    }
    else
    {
        long value;

        VerifyOrExit(argc == 2, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(argv[0], value));
        sTxPeriod = static_cast<uint32_t>(value);

        SuccessOrExit(error = ParseLong(argv[1], value));
        VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
        sTxLen = static_cast<uint8_t>(value);

        sRepeatActive = true;
        uint32_t now = otPlatAlarmMilliGetNow();
        otPlatAlarmMilliStartAt(sContext, now, sTxPeriod);
        snprintf(aOutput, aOutputMaxLen, "sending packets of length %#x at the delay of %#x ms\r\nstatus 0x%02x\r\n", static_cast<int>(sTxLen), static_cast<int>(sTxPeriod), error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessSleep(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    otPlatRadioSleep(sContext);
    snprintf(aOutput, aOutputMaxLen, "sleeping now...\r\n");

exit:
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessStats(int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    snprintf(aOutput, aOutputMaxLen, "received packets: %d\r\nsent packets: %d\r\nfirst received packet: rssi=%d, lqi=%d\r\n",
            static_cast<int>(sStats.received_packets), static_cast<int>(sStats.sent_packets),
            static_cast<int>(sStats.first_rssi), static_cast<int>(sStats.first_lqi));

exit:
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::DiagTransmitDone(otInstance *aInstance, otError aError)
{
    VerifyOrExit(aInstance == sContext);

    if (aError == OT_ERROR_NONE)
    {
        sStats.sent_packets++;

        if (sTxPackets > 1)
        {
            sTxPackets--;
            TxPacket();
        }
    }
    else
    {
        TxPacket();
    }

exit:
    return;
}

void Diag::DiagReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    VerifyOrExit(aInstance == sContext);

    if (aError == OT_ERROR_NONE)
    {
        // for sensitivity test, only record the rssi and lqi for the first packet
        if (sStats.received_packets == 0)
        {
            sStats.first_rssi = aFrame->mPower;
            sStats.first_lqi = aFrame->mLqi;
        }

        sStats.received_packets++;
    }
    otPlatDiagRadioReceived(aInstance, aFrame, aError);
    otPlatRadioReceive(aInstance, sChannel);

exit:
    return;
}

void Diag::AlarmFired(otInstance *aInstance)
{
    VerifyOrExit(aInstance == sContext);

    if(sRepeatActive)
    {
        uint32_t now = otPlatAlarmMilliGetNow();

        TxPacket();
        otPlatAlarmMilliStartAt(aInstance, now, sTxPeriod);
    }
    else
    {
        otPlatDiagAlarmCallback(aInstance);
    }

exit:
    return;
}

extern "C" void otPlatDiagAlarmFired(otInstance *aInstance)
{
    Diag::AlarmFired(aInstance);
}

extern "C" void otPlatDiagRadioTransmitDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aFrame);

    Diag::DiagTransmitDone(aInstance, aError);
}

extern "C" void otPlatDiagRadioReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    Diag::DiagReceiveDone(aInstance, aFrame, aError);
}

}  // namespace Diagnostics
}  // namespace ot
