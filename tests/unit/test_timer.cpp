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

#include "test_util.h"
#include <openthread.h>
#include <common/debug.hpp>
#include <common/timer.hpp>
#include <platform/alarm.h>
#include <string.h>

enum
{
    kCallCountIndexAlarmStop = 0,
    kCallCountIndexAlarmStart,
    kCallCountIndexTimerHandler,

    kCallCountIndexMax
};

static Thread::TimerScheduler sTimerScheduler;
extern uint32_t sNow;
extern uint32_t sPlatT0;
extern uint32_t sPlatDt;
extern bool     sTimerOn;
extern uint32_t sCallCount[kCallCountIndexMax];

void InitCounters(void)
{
    memset(sCallCount, 0, sizeof(sCallCount));
}

void TestTimerHandler(void *aContext)
{
    sCallCount[kCallCountIndexTimerHandler]++;

    if (aContext)
    {
        (*static_cast<uint32_t *>(aContext))++;
    }
}

/**
 * Test the TimerScheduler's behavior of one timer started and fired.
 */
int TestOneTimer(void)
{
    const uint32_t kTimeT0 = 1000;
    const uint32_t kTimerInterval = 10;
    Thread::Timer timer(sTimerScheduler, TestTimerHandler, NULL);

    // Test one Timer basic operation.

    InitCounters();

    sNow = kTimeT0;
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 1000 && sPlatDt == 10,              "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(),                             "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn,                                      "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    otPlatAlarmFired(NULL);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false,                    "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false,                             "TestOneTimer: Platform Timer State Failed.\n");

    // Test one Timer that spans the 32-bit wrap.

    InitCounters();

    sNow = 0 - (kTimerInterval - 2);
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 1,         "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 0,         "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 0,         "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 0 - (kTimerInterval - 2) && sPlatDt == 10,  "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(),                                     "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn,                                              "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    otPlatAlarmFired(NULL);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false,                    "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false,                             "TestOneTimer: Platform Timer State Failed.\n");

    // Test one Timer that is late by several msec

    InitCounters();

    sNow = kTimeT0;
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 1000 && sPlatDt == 10,              "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(),                             "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn,                                      "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval + 5;

    otPlatAlarmFired(NULL);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false,                    "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false,                             "TestOneTimer: Platform Timer State Failed.\n");

    // Test one Timer that is early by several msec

    InitCounters();

    sNow = kTimeT0;
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 1000 && sPlatDt == 10,              "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(),                             "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn,                                      "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval - 2;

    otPlatAlarmFired(NULL);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 2, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == true,                     "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == true,                              "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    otPlatAlarmFired(NULL);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 2, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false,                    "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false,                             "TestOneTimer: Platform Timer State Failed.\n");

    return 0;
}

/**
 * Test the TimerScheduler's behavior of ten timers started and fired.
 */
int TestTenTimers(void)
{
    const uint32_t kNumTimers = 10;
    const uint32_t kNumTriggers = 7;
    const uint32_t kTimeT0[kNumTimers] =
    {
        1000,
        1000,
        1001,
        1002,
        1003,
        1004,
        1005,
        1006,
        1007,
        1008
    };
    const uint32_t kTimerInterval[kNumTimers] =
    {
        20,
        100,
        (0 - kTimeT0[2]),
        100000,
        1000000,
        10,
        (1000 - kTimeT0[6]),
        200,
        200,
        200
    };
    // Expected timer fire order
    // timer #    Trigger time
    //   5         1014
    //   0         1020
    //   1         1100
    //   7         1206
    //   8         1207
    //   9         1208
    //   3       101002
    //   4      1001003
    //   2            0 <timer wrapped>
    //   6         1000 <timer wrapped>
    const uint32_t kTriggerTimes[kNumTriggers] =
    {
        1014,
        1020,
        1100,
        1207,
        101004,
        /* timer wrap here */
        2,
        1000
    };
    // Expected timers fired by each kTriggerTimes[] value
    //  Trigger #    Timers Fired
    //    0             5
    //    1             0
    //    2             1
    //    3             7, 8
    //    4             9, 3
    //    5             4, 2
    //    6             6
    const bool kTimerStateAfterTrigger [kNumTriggers][kNumTimers] =
    {
        {  true,  true,  true,  true,  true, false, true,   true,  true,  true},  // 5
        { false,  true,  true,  true,  true, false, true,   true,  true,  true},  // 0
        { false, false,  true,  true,  true, false, true,   true,  true,  true},  // 1
        { false, false,  true,  true,  true, false, true,  false, false,  true},  // 7, 8
        { false, false,  true, false,  true, false, true,  false, false, false},  // 9, 3
        { false, false, false, false, false, false, true,  false, false, false},  // 4, 2
        { false, false, false, false, false, false, false, false, false, false}   // 6
    };

    const bool kSchedulerStateAfterTrigger[kNumTriggers] =
    {
        true,
        true,
        true,
        true,
        true,
        true,
        false
    };

    const uint32_t kTimerHandlerCountAfterTrigger[kNumTriggers] =
    {
        1,
        2,
        3,
        5,
        7,
        9,
        10
    };

    const uint32_t kTimerStopCountAfterTrigger[kNumTriggers] =
    {
        0,
        0,
        0,
        0,
        0,
        0,
        1
    };

    const uint32_t kTimerStartCountAfterTrigger[kNumTriggers] =
    {
        3,
        4,
        5,
        7,
        9,
        11,
        11
    };

    uint32_t timerContextHandleCounter[kNumTimers] = {0};
    Thread::Timer timer0(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[0]);
    Thread::Timer timer1(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[1]);
    Thread::Timer timer2(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[2]);
    Thread::Timer timer3(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[3]);
    Thread::Timer timer4(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[4]);
    Thread::Timer timer5(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[5]);
    Thread::Timer timer6(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[6]);
    Thread::Timer timer7(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[7]);
    Thread::Timer timer8(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[8]);
    Thread::Timer timer9(sTimerScheduler, TestTimerHandler, &timerContextHandleCounter[9]);
    Thread::Timer *timers[kNumTimers] = {&timer0, &timer1, &timer2, &timer3, &timer4, &timer5, &timer6, &timer7, &timer8, &timer9};
    size_t i;

    // Start the Ten timers.

    InitCounters();

    for (i = 0; i < kNumTimers ; i++)
    {
        sNow = kTimeT0[i];
        timers[i]->Start(kTimerInterval[i]);
    }

    // given the order in which timers are started, the TimerScheduler should call otPlatAlarmStartAt 2 times.
    // one for timer[0] and one for timer[5] which will supercede timer[0].
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == 2,         "TestTenTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == 0,         "TestTenTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == 0,         "TestTenTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == kTimeT0[5] && sPlatDt == kTimerInterval[5], "TestTenTimer: Start params Failed.\n");
    VerifyOrQuit(sTimerOn,                                              "TestTenTimer: Platform Timer State Failed.\n");

    for (i = 0 ; i < kNumTimers ; i++)
    {
        VerifyOrQuit(timers[i]->IsRunning(), "TestTenTimer: Timer running Failed.\n");
    }

    // Issue the triggers and test the State after each trigger.

    for (size_t trigger = 0 ; trigger < kNumTriggers ; trigger++)
    {
        sNow = kTriggerTimes[trigger];

        do
        {
            // By design, each call to otPlatAlarmFired() can result in 0 or 1 calls to a timer handler.
            // For some combinations of sNow and Timers queued, it is necessary to call otPlatAlarmFired()
            // multiple times in order to handle all the expired timers.  It can be determined that another
            // timer is ready to be triggered by examining the aDt arg passed into otPlatAlarmStartAt().  If
            // that value is 0, then otPlatAlarmFired should be fired immediately. This loop calls otPlatAlarmFired()
            // the requisite number of times based on the aDt argument.
            otPlatAlarmFired(NULL);
        }
        while (sPlatDt == 0);

        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart]    == kTimerStartCountAfterTrigger[trigger],
                     "TestTenTimer: Start CallCount Failed.\n");
        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop]     == kTimerStopCountAfterTrigger[trigger],
                     "TestTenTimer: Stop CallCount Failed.\n");
        VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler]  == kTimerHandlerCountAfterTrigger[trigger],
                     "TestTenTimer: Handler CallCount Failed.\n");
        VerifyOrQuit(sTimerOn                                 == kSchedulerStateAfterTrigger[trigger],
                     "TestTenTimer: Platform Timer State Failed.\n");

        for (i = 0 ; i < kNumTimers ; i++)
        {
            VerifyOrQuit(timers[i]->IsRunning() == kTimerStateAfterTrigger[trigger][i], "TestTenTimer: Timer running Failed.\n");
        }
    }

    for (i = 0 ; i < kNumTimers ; i++)
    {
        VerifyOrQuit(timerContextHandleCounter[i] == 1, "TestTenTimer: Timer context counter Failed.\n");
    }

    return 0;
}

void RunTimerTests(void)
{
    TestOneTimer();
    TestTenTimers();
}

#ifndef _WIN32
int main(void)
{
    RunTimerTests();
    printf("All tests passed\n");
    return 0;
}
#endif
