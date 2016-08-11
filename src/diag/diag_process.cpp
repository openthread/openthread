/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/code_utils.hpp>

#include "diag_process.hpp"

namespace Thread {
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
otContext *Diag::sContext;

void Diag::Init(otContext *aContext)
{
    sContext = aContext;
    sChannel = 20;
    sTxPower = 0;
    sTxPeriod = 0;
    sTxLen = 0;
    sTxPackets = 0;
    memset(&sStats, 0, sizeof(struct DiagStats));
}

char *Diag::ProcessCmd(int argc, char *argv[])
{
    unsigned int i;

    if (argc == 0)
    {
        sprintf(sDiagOutput, "diagnostics mode is %s\r\n", otPlatDiagModeGet() ? "enabled" : "disabled");
    }
    else
    {
        for (i = 0; i < sizeof(sCommands) / sizeof(sCommands[0]); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                sCommands[i].mCommand(argc - 1, &argv[1], sDiagOutput);
                break;
            }
        }

        // more platform specific features will be processed under platform layer
        if (i == sizeof(sCommands) / sizeof(sCommands[0]))
        {
            otPlatDiagProcess(argc, argv, sDiagOutput);
        }
    }

    return sDiagOutput;
}

bool Diag::isEnabled()
{
    return otPlatDiagModeGet();
}

void Diag::AppendErrorResult(ThreadError aError, char *aOutput)
{
    if (aError != kThreadError_None)
    {
        sprintf(aOutput, "failed\r\nstatus %#x\r\n", aError);
    }
}

void Diag::ProcessStart(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;

    // enable radio
    otPlatRadioEnable(sContext);

    // enable promiscuous mode
    otPlatRadioSetPromiscuous(sContext, true);

    // stop timer
    otPlatAlarmStop(sContext);

    // start to listen on the default channel
    SuccessOrExit(error = otPlatRadioReceive(sContext, sChannel));

    // enable diagnostics mode
    otPlatDiagModeSet(true);

    memset(&sStats, 0, sizeof(struct DiagStats));

    sprintf(aOutput, "start diagnostics mode\r\nstatus 0x%02x\r\n", error);

exit:
    (void)argc;
    (void)argv;
    AppendErrorResult(error, aOutput);
}

void Diag::ProcessStop(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    otPlatAlarmStop(sContext);
    otPlatDiagModeSet(false);
    otPlatRadioSetPromiscuous(sContext, false);

    sprintf(aOutput, "received packets: %d\r\nsent packets: %d\r\nfirst received packet: rssi=%d, lqi=%d\r\n\nstop diagnostics mode\r\nstatus 0x%02x\r\n",
            static_cast<int>(sStats.received_packets), static_cast<int>(sStats.sent_packets), static_cast<int>(sStats.first_rssi),
            static_cast<int>(sStats.first_lqi), error);

exit:
    (void)argc;
    (void)argv;
    AppendErrorResult(error, aOutput);
}

ThreadError Diag::ParseLong(char *argv, long &value)
{
    char *endptr;
    value = strtol(argv, &endptr, 0);
    return (*endptr == '\0') ? kThreadError_None : kThreadError_Parse;
}

void Diag::TxPacket()
{
    RadioPacket *packet = otPlatRadioGetTransmitBuffer(sContext);

    if (sTxPackets > 0)
    {
        sTxPackets--;
    }

    packet->mLength = sTxLen;
    packet->mChannel = sChannel;
    packet->mPower = sTxPower;

    for (uint8_t i = 0; i < sTxLen; i++)
    {
        packet->mPsdu[i] = i;
    }

    otPlatRadioTransmit(sContext);
}

void Diag::ProcessChannel(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    if (argc == 0)
    {
        sprintf(aOutput, "channel: %d\r\n", sChannel);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(argv[0], value));
        VerifyOrExit(value >= kPhyMinChannel && value <= kPhyMaxChannel, error = kThreadError_InvalidArgs);
        sChannel = static_cast<uint8_t>(value);

        // listen on the set channel immediately
        otPlatRadioReceive(sContext, sChannel);
        sprintf(aOutput, "set channel to %d\r\nstatus 0x%02x\r\n", sChannel, error);
    }

exit:
    AppendErrorResult(error, aOutput);
}

void Diag::ProcessPower(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    if (argc == 0)
    {
        sprintf(aOutput, "tx power: %d dBm\r\n", sTxPower);
    }
    else
    {
        long value;

        SuccessOrExit(error = ParseLong(argv[0], value));
        sTxPower = static_cast<int8_t>(value);
        sprintf(aOutput, "set tx power to %d dBm\r\nstatus 0x%02x\r\n", sTxPower, error);
    }

exit:
    AppendErrorResult(error, aOutput);
}

void Diag::ProcessSend(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;
    long value;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);
    VerifyOrExit(argc == 2, error = kThreadError_InvalidArgs);

    SuccessOrExit(error = ParseLong(argv[0], value));
    sTxPackets = static_cast<uint32_t>(value);

    SuccessOrExit(error = ParseLong(argv[1], value));
    VerifyOrExit(value <= kMaxPHYPacketSize, error = kThreadError_InvalidArgs);
    sTxLen = static_cast<uint8_t>(value);

    sprintf(aOutput, "sending %#x packet(s), length %#x\r\nstatus 0x%02x\r\n", static_cast<int>(sTxPackets), static_cast<int>(sTxLen), error);
    TxPacket();

exit:
    AppendErrorResult(error, aOutput);
}

void Diag::ProcessRepeat(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);
    VerifyOrExit(argc > 0, error = kThreadError_InvalidArgs);

    if (strcmp(argv[0], "stop") == 0)
    {
        otPlatAlarmStop(sContext);
        sprintf(aOutput, "repeated packet transmission is stopped\r\nstatus 0x%02x\r\n", error);
    }
    else
    {
        long value;

        VerifyOrExit(argc == 2, error = kThreadError_InvalidArgs);

        SuccessOrExit(error = ParseLong(argv[0], value));
        sTxPeriod = static_cast<uint32_t>(value);

        SuccessOrExit(error = ParseLong(argv[1], value));
        VerifyOrExit(value <= kMaxPHYPacketSize, error = kThreadError_InvalidArgs);
        sTxLen = static_cast<uint8_t>(value);

        uint32_t now = otPlatAlarmGetNow();
        otPlatAlarmStartAt(sContext, now, sTxPeriod);
        sprintf(aOutput, "sending packets of length %#x at the delay of %#x ms\r\nstatus 0x%02x\r\n", static_cast<int>(sTxLen), static_cast<int>(sTxPeriod), error);
    }

exit:
    AppendErrorResult(error, aOutput);
}

void Diag::ProcessSleep(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    otPlatRadioSleep(sContext);
    sprintf(aOutput, "sleeping now...\r\n");

exit:
    (void)argc;
    (void)argv;
    AppendErrorResult(error, aOutput);
}

void Diag::ProcessStats(int argc, char *argv[], char *aOutput)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(otPlatDiagModeGet(), error = kThreadError_InvalidState);

    sprintf(aOutput, "received packets: %d\r\nsent packets: %d\r\nfirst received packet: rssi=%d, lqi=%d\r\n",
            static_cast<int>(sStats.received_packets), static_cast<int>(sStats.sent_packets),
            static_cast<int>(sStats.first_rssi), static_cast<int>(sStats.first_lqi));

exit:
    (void)argc;
    (void)argv;
    AppendErrorResult(error, aOutput);
}

void Diag::DiagTransmitDone(otContext *aContext, bool aRxPending, ThreadError aError)
{
    (void)aContext;
    if (!aRxPending && aError == kThreadError_None)
    {
        sStats.sent_packets++;

        if (sTxPackets > 0)
        {
            TxPacket();
        }
    }
}

void Diag::DiagReceiveDone(otContext *aContext, RadioPacket *aFrame, ThreadError aError)
{
    (void)aContext;
    if (aError == kThreadError_None)
    {
        // for sensitivity test, only record the rssi and lqi for the first packet
        if (sStats.received_packets == 0)
        {
            sStats.first_rssi = aFrame->mPower;
            sStats.first_lqi = aFrame->mLqi;
        }

        sStats.received_packets++;
    }
}

void Diag::AlarmFired(otContext *aContext)
{
    uint32_t now = otPlatAlarmGetNow();

    TxPacket();
    otPlatAlarmStartAt(aContext, now, sTxPeriod);
}

extern "C" void otPlatDiagAlarmFired(otContext *aContext)
{
    Diag::AlarmFired(aContext);
}

extern "C" void otPlatDiagRadioTransmitDone(otContext *aContext, bool aRxPending, ThreadError aError)
{
    Diag::DiagTransmitDone(aContext, aRxPending, aError);
}

extern "C" void otPlatDiagRadioReceiveDone(otContext *aContext, RadioPacket *aFrame, ThreadError aError)
{
    Diag::DiagReceiveDone(aContext, aFrame, aError);
}

}  // namespace Diagnostics
}  // namespace Thread
