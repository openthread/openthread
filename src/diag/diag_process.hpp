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
 *   This file contains definitions for the diagnostics module.
 */

#ifndef DIAG_PROCESS_HPP_
#define DIAG_PROCESS_HPP_

#include "openthread-core-config.h"

#include <stdarg.h>

#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>

namespace ot {

namespace Diagnostics {

class Diag
{
public:
    static void Init(otInstance *aInstance);
    static void ProcessCmd(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static bool IsEnabled(void);

    static void DiagTransmitDone(otInstance *aInstance, otError aError);
    static void DiagReceiveDone(otInstance *aInstance, otRadioFrame *aFrame, otError aError);
    static void AlarmFired(otInstance *aInstance);

private:
    struct DiagStats
    {
        uint32_t mReceivedPackets;
        uint32_t mSentPackets;
        int8_t   mFirstRssi;
        uint8_t  mFirstLqi;
    };

    struct Command
    {
        const char *mName;
        void (*mHandler)(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    };

    static void AppendErrorResult(otError aError, char *aOutput, size_t aOutputMaxLen);
    static void ProcessStart(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static void ProcessStop(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static void ProcessSend(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static void ProcessRepeat(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static void ProcessStats(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static void ProcessChannel(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static void ProcessPower(int aArgCount, char *aArgVector[], char *aOutput, size_t aOutputMaxLen);
    static void TxPacket(void);

    static otError ParseLong(char *aString, long &aLong);

    static const struct Command sCommands[];
    static struct DiagStats     sStats;

    static int8_t        sTxPower;
    static uint8_t       sChannel;
    static uint8_t       sTxLen;
    static uint32_t      sTxPeriod;
    static uint32_t      sTxPackets;
    static otRadioFrame *sTxPacket;
    static otInstance *  sInstance;
    static bool          sRepeatActive;
};

} // namespace Diagnostics
} // namespace ot

#endif // CLI_HPP_
