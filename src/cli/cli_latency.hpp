/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file contains definitions for a simple CLI for latency test.
 */

#ifndef CLI_LATENCY_HPP_
#define CLI_LATENCY_HPP_
// #include "cli.hpp"
#include <openthread/udp.h>
#include <openthread/types.h>

#include <openthread/platform/gpio.h>
#include <openthread/platform/random.h>

#include "common/timer.hpp"

#define MAX_PACKET_NUM 1000
#define MAX_PACKET_LEN 1500

#define MONITOR_PIN 3
#define CLEAR_PIN_INTERVAL 2

namespace ot {
namespace Cli {

class Interpreter;

/**
 * This class implements a CLI-based UDP example.
 *
 */
class LatencyTest
{
public:
    /**
     * Constructor
     *
     * @param[in]  aInterpreter  The CLI interpreter.
     *
     */
    LatencyTest(Interpreter &aInterpreter);

    /**
     * This method interprets a list of CLI arguments.
     *
     * @param[in]  argc  The number of elements in argv.
     * @param[in]  argv  A pointer to an array of command line arguments.
     *
     */
    otError Process(int argc, char *argv[]);

    static LatencyTest *sLatency;
    static uint16_t sCount;

    static uint32_t sSendTimestamp[MAX_PACKET_NUM];

    static void platGpioResponse(void *aContext);

private:
    struct Command
    {
        const char *mName;
        otError(LatencyTest::*mCommand)(int argc, char *argv[]);
    };

    otError ProcessHelp(int argc, char *argv[]);
    otError ProcessBind(int argc, char *argv[]);
    otError ProcessClose(int argc, char *argv[]);
    otError ProcessConnect(int argc, char *argv[]);
    otError ProcessOpen(int argc, char *argv[]);
    otError ProcessSend(int argc, char *argv[]);
    otError ProcessTest(int argc, char *argv[]);
    otError ProcessResult(int argc, char *argv[]);
    otError ProcessGpio(int argc, char *argv[]);
    otError ProcessStart(int argc, char *argv[]);
    
    static void s_HandlePingTimer(Timer &aTimer);
    void HandlePingTimer();

    static void s_HandleGpioTimer(Timer &aTimer);
    void HandleGpioTimer();

    static LatencyTest &GetOwner(OwnerLocator &aOwnerLocator);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(otMessage *aMessage, const otMessageInfo *aMessageInfo);

    otError SendUdpPacket(void);
    uint32_t GetAcceptedCount(otMessage *aMessage);
    uint32_t GetAcceptedTimestamp(otMessage *aMessage);
    void Init(void);

    static const Command sCommands[];
    Interpreter &mInterpreter;

    otUdpSocket mSocket;

    uint16_t mLength;
    uint16_t mCount;
    uint32_t mInterval;
    char mPayload[MAX_PACKET_LEN];
    otMessageInfo mMessageInfo;
    TimerMilli mPingTimer;
    TimerMilli mGpioTimer;
    uint32_t mTimeElapse;
    uint32_t mLatency;
    uint32_t mJitter;
    uint32_t mAcceptTimestamp;
    uint32_t mTotalCount;
    bool mIsRun;
    uint32_t mReceiveTimer[MAX_PACKET_NUM];
};

}  // namespace Cli
}  // namespace ot

#endif  // CLI_UDP_EXAMPLE_HPP_
