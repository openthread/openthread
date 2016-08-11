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
 *   This file contains definitions for the diagnostics module.
 */

#ifndef DIAG_PROCESS_HPP_
#define DIAG_PROCESS_HPP_

#include <stdarg.h>
#include <platform/radio.h>
#include <platform/alarm.h>
#include <platform/diag.h>

namespace Thread {

namespace Diagnostics {

#define MAX_DIAG_OUTPUT 256

struct Command
{
    const char *mName;                         ///< A pointer to the command string.
    void (*mCommand)(int argc, char *argv[], char *aOutput);  ///< A function pointer to process the command.
};

struct DiagStats
{
    uint32_t received_packets;
    uint32_t sent_packets;
    int8_t first_rssi;
    uint8_t first_lqi;
};

class Diag
{
public:
    static void Init();
    static char *ProcessCmd(int argc, char *argv[]);
    static bool isEnabled();

    static void DiagTransmitDone(bool aRxPending, ThreadError aError);
    static void DiagReceiveDone(RadioPacket *aFrame, ThreadError aError);
    static void AlarmFired();

private:
    static void AppendErrorResult(ThreadError error, char *aOutput);
    static void ProcessSleep(int argc, char *argv[], char *aOutput);
    static void ProcessStart(int argc, char *argv[], char *aOutput);
    static void ProcessStop(int argc, char *argv[], char *aOutput);
    static void ProcessSend(int argc, char *argv[], char *aOutput);
    static void ProcessRepeat(int argc, char *argv[], char *aOutput);
    static void ProcessStats(int argc, char *argv[], char *aOutput);
    static void ProcessChannel(int argc, char *argv[], char *aOutput);
    static void ProcessPower(int argc, char *argv[], char *aOutput);
    static void TxPacket();
    static ThreadError ParseLong(char *aString, long &aLong);

    static char sDiagOutput[];
    static const struct Command sCommands[];
    static struct DiagStats sStats;
    static int8_t sTxPower;
    static uint8_t sChannel;
    static uint8_t sTxLen;
    static uint32_t sTxPeriod;
    static uint32_t sTxPackets;
};

}  // namespace Diagnostics
}  // namespace Thread

#endif  // CLI_HPP_
