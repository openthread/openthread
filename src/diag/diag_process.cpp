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

#include "diag_process.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <openthread/instance.h>

#include "common/code_utils.hpp"
#include "utils/wrap_string.h"

namespace ot {
namespace Diagnostics {

const struct Diag::Command Diag::sCommands[] = {
    {"start", &ProcessStart}, {"stop", &ProcessStop},     {"channel", &ProcessChannel}, {"power", &ProcessPower},
    {"send", &ProcessSend},   {"repeat", &ProcessRepeat}, {"stats", &ProcessStats},     {NULL, NULL},
};

struct Diag::DiagStats Diag::sStats;

int8_t        Diag::sTxPower;
uint8_t       Diag::sChannel;
uint8_t       Diag::sTxLen;
uint32_t      Diag::sTxPeriod;
uint32_t      Diag::sTxPackets;
otRadioFrame *Diag::sTxPacket;
bool          Diag::sRepeatActive;
otInstance *  Diag::sInstance;

void Diag::Init(otInstance *aInstance)
{
    sInstance     = aInstance;
    sChannel      = 20;
    sTxPower      = 0;
    sTxPeriod     = 0;
    sTxLen        = 0;
    sTxPackets    = 0;
    sRepeatActive = false;
    memset(&sStats, 0, sizeof(struct DiagStats));

    sTxPacket = otPlatRadioGetTransmitBuffer(sInstance);
    otPlatDiagChannelSet(sChannel);
    otPlatDiagTxPowerSet(sTxPower);
}

void Diag::ProcessCmd(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    if (aArgCount == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "diagnostics mode is %s\r\n", otPlatDiagModeGet() ? "enabled" : "disabled");
        ExitNow();
    }

    for (const Command *command = &sCommands[0]; command->mName != NULL; command++)
    {
        if (strcmp(aArgVector[0], command->mName) == 0)
        {
            command->mHandler(aArgCount - 1, (aArgCount > 1) ? &aArgVector[1] : NULL, aOutput, aOutputMaxLen);
            ExitNow();
        }
    }

    // more platform specific features will be processed under platform layer
    otPlatDiagProcess(sInstance, aArgCount, aArgVector, aOutput, aOutputMaxLen);

exit:
    return;
}

bool Diag::IsEnabled(void)
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

void Diag::ProcessStart(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgCount);
    OT_UNUSED_VARIABLE(aArgVector);

    otError error = OT_ERROR_NONE;

    otPlatRadioEnable(sInstance);
    otPlatRadioSetPromiscuous(sInstance, true);
    otPlatAlarmMilliStop(sInstance);
    SuccessOrExit(error = otPlatRadioReceive(sInstance, sChannel));
    otPlatDiagModeSet(true);
    memset(&sStats, 0, sizeof(struct DiagStats));
    snprintf(aOutput, aOutputMaxLen, "start diagnostics mode\r\nstatus 0x%02x\r\n", error);

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessStop(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgCount);
    OT_UNUSED_VARIABLE(aArgVector);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    otPlatAlarmMilliStop(sInstance);
    otPlatDiagModeSet(false);
    otPlatRadioSetPromiscuous(sInstance, false);

    snprintf(aOutput, aOutputMaxLen,
             "received packets: %d\r\nsent packets: %d\r\nfirst received packet: rssi=%d, lqi=%d\r\n"
             "\nstop diagnostics mode\r\nstatus 0x%02x\r\n",
             static_cast<int>(sStats.mReceivedPackets), static_cast<int>(sStats.mSentPackets),
             static_cast<int>(sStats.mFirstRssi), static_cast<int>(sStats.mFirstLqi), error);

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

otError Diag::ParseLong(char *aArgVector, long &aValue)
{
    char *endptr;
    aValue = strtol(aArgVector, &endptr, 0);
    return (*endptr == '\0') ? OT_ERROR_NONE : OT_ERROR_PARSE;
}

void Diag::TxPacket(void)
{
    sTxPacket->mLength  = sTxLen;
    sTxPacket->mChannel = sChannel;

    for (uint8_t i = 0; i < sTxLen; i++)
    {
        sTxPacket->mPsdu[i] = i;
    }

    otPlatRadioTransmit(sInstance, sTxPacket);
}

void Diag::ProcessChannel(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (aArgCount == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "channel: %d\r\n", sChannel);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgVector[0], value));
        VerifyOrExit(value >= OT_RADIO_CHANNEL_MIN && value <= OT_RADIO_CHANNEL_MAX, error = OT_ERROR_INVALID_ARGS);

        sChannel = static_cast<uint8_t>(value);
        otPlatRadioReceive(sInstance, sChannel);
        otPlatDiagChannelSet(sChannel);

        snprintf(aOutput, aOutputMaxLen, "set channel to %d\r\nstatus 0x%02x\r\n", sChannel, error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessPower(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    if (aArgCount == 0)
    {
        snprintf(aOutput, aOutputMaxLen, "tx power: %d dBm\r\n", sTxPower);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(aArgVector[0], value));

        sTxPower = static_cast<int8_t>(value);
        otPlatDiagTxPowerSet(sTxPower);

        snprintf(aOutput, aOutputMaxLen, "set tx power to %d dBm\r\nstatus 0x%02x\r\n", sTxPower, error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessSend(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;
    long    value;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aArgCount == 2, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = ParseLong(aArgVector[0], value));
    sTxPackets = static_cast<uint32_t>(value);

    SuccessOrExit(error = ParseLong(aArgVector[1], value));
    VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
    sTxLen = static_cast<uint8_t>(value);

    snprintf(aOutput, aOutputMaxLen, "sending %#x packet(s), length %#x\r\nstatus 0x%02x\r\n",
             static_cast<int>(sTxPackets), static_cast<int>(sTxLen), error);
    TxPacket();

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessRepeat(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(aArgCount > 0, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgVector[0], "stop") == 0)
    {
        otPlatAlarmMilliStop(sInstance);
        sRepeatActive = false;
        snprintf(aOutput, aOutputMaxLen, "repeated packet transmission is stopped\r\nstatus 0x%02x\r\n", error);
    }
    else
    {
        long value;

        VerifyOrExit(aArgCount == 2, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = ParseLong(aArgVector[0], value));
        sTxPeriod = static_cast<uint32_t>(value);

        SuccessOrExit(error = ParseLong(aArgVector[1], value));
        VerifyOrExit(value <= OT_RADIO_FRAME_MAX_SIZE, error = OT_ERROR_INVALID_ARGS);
        sTxLen = static_cast<uint8_t>(value);

        sRepeatActive = true;
        uint32_t now  = otPlatAlarmMilliGetNow();
        otPlatAlarmMilliStartAt(sInstance, now, sTxPeriod);
        snprintf(aOutput, aOutputMaxLen, "sending packets of length %#x at the delay of %#x ms\r\nstatus 0x%02x\r\n",
                 static_cast<int>(sTxLen), static_cast<int>(sTxPeriod), error);
    }

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::ProcessStats(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen)
{
    OT_UNUSED_VARIABLE(aArgCount);
    OT_UNUSED_VARIABLE(aArgVector);

    otError error = OT_ERROR_NONE;

    VerifyOrExit(otPlatDiagModeGet(), error = OT_ERROR_INVALID_STATE);

    snprintf(aOutput, aOutputMaxLen,
             "received packets: %d\r\nsent packets: %d\r\nfirst received packet: rssi=%d, lqi=%d\r\n",
             static_cast<int>(sStats.mReceivedPackets), static_cast<int>(sStats.mSentPackets),
             static_cast<int>(sStats.mFirstRssi), static_cast<int>(sStats.mFirstLqi));

exit:
    AppendErrorResult(error, aOutput, aOutputMaxLen);
}

void Diag::DiagTransmitDone(otInstance *aInstance, otError aError)
{
    VerifyOrExit(aInstance == sInstance);

    if (aError == OT_ERROR_NONE)
    {
        sStats.mSentPackets++;

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
    VerifyOrExit(aInstance == sInstance);

    if (aError == OT_ERROR_NONE)
    {
        // for sensitivity test, only record the rssi and lqi for the first packet
        if (sStats.mReceivedPackets == 0)
        {
            sStats.mFirstRssi = aFrame->mInfo.mRxInfo.mRssi;
            sStats.mFirstLqi  = aFrame->mInfo.mRxInfo.mLqi;
        }

        sStats.mReceivedPackets++;
    }
    otPlatDiagRadioReceived(aInstance, aFrame, aError);

exit:
    return;
}

void Diag::AlarmFired(otInstance *aInstance)
{
    VerifyOrExit(aInstance == sInstance);

    if (sRepeatActive)
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

} // namespace Diagnostics
} // namespace ot
