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

#include "test_platform.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/timer.hpp"

enum
{
    kCallCountIndexAlarmStop = 0,
    kCallCountIndexAlarmStart,
    kCallCountIndexTimerHandler,

    kCallCountIndexMax
};

uint32_t sNow;
uint32_t sPlatT0;
uint32_t sPlatDt;
bool     sTimerOn;
uint32_t sCallCount[kCallCountIndexMax];

void testTimerAlarmStop(otInstance *)
{
    sTimerOn = false;
    sCallCount[kCallCountIndexAlarmStop]++;
}

void testTimerAlarmStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sTimerOn = true;
    sCallCount[kCallCountIndexAlarmStart]++;
    sPlatT0 = aT0;
    sPlatDt = aDt;
}

uint32_t testTimerAlarmGetNow(void)
{
    return sNow;
}

void InitTestTimer(void)
{
    g_testPlatAlarmStop    = testTimerAlarmStop;
    g_testPlatAlarmStartAt = testTimerAlarmStartAt;
    g_testPlatAlarmGetNow  = testTimerAlarmGetNow;
}

void InitCounters(void)
{
    memset(sCallCount, 0, sizeof(sCallCount));
}

/**
 * `TestTimer` sub-classes `ot::TimerMilli` and provides a handler and a counter to keep track of number of times timer
 * gets fired.
 */
class TestTimer : public ot::TimerMilli
{
public:
    TestTimer(ot::Instance &aInstance)
        : ot::TimerMilli(aInstance, TestTimer::HandleTimerFired, NULL)
        , mFiredCounter(0)
    {
    }

    static void HandleTimerFired(ot::Timer &aTimer) { static_cast<TestTimer &>(aTimer).HandleTimerFired(); }

    void HandleTimerFired(void)
    {
        sCallCount[kCallCountIndexTimerHandler]++;
        mFiredCounter++;
    }

    uint32_t GetFiredCounter(void) { return mFiredCounter; }

    void ResetFiredCounter(void) { mFiredCounter = 0; }

private:
    uint32_t mFiredCounter; //< Number of times timer has been fired so far
};

/**
 * Test the TimerScheduler's behavior of one timer started and fired.
 */
int TestOneTimer(void)
{
    const uint32_t kTimeT0        = 1000;
    const uint32_t kTimerInterval = 10;
    ot::Instance * instance       = testInitInstance();
    TestTimer      timer(*instance);

    // Test one Timer basic operation.

    InitTestTimer();
    InitCounters();

    printf("TestOneTimer() ");

    sNow = kTimeT0;
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 1000 && sPlatDt == 10, "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(), "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false, "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false, "TestOneTimer: Platform Timer State Failed.\n");

    // Test one Timer that spans the 32-bit wrap.

    InitCounters();

    sNow = 0 - (kTimerInterval - 2);
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 0 - (kTimerInterval - 2) && sPlatDt == 10, "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(), "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false, "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false, "TestOneTimer: Platform Timer State Failed.\n");

    // Test one Timer that is late by several msec

    InitCounters();

    sNow = kTimeT0;
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 1000 && sPlatDt == 10, "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(), "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval + 5;

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false, "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false, "TestOneTimer: Platform Timer State Failed.\n");

    // Test one Timer that is early by several msec

    InitCounters();

    sNow = kTimeT0;
    timer.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == 1000 && sPlatDt == 10, "TestOneTimer: Start params Failed.\n");
    VerifyOrQuit(timer.IsRunning(), "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval - 2;

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == true, "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == true, "TestOneTimer: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "TestOneTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "TestOneTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "TestOneTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(timer.IsRunning() == false, "TestOneTimer: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false, "TestOneTimer: Platform Timer State Failed.\n");

    printf(" --> PASSED\n");

    testFreeInstance(instance);

    return 0;
}

/**
 * Test the TimerScheduler's behavior of two timers started and fired.
 */
int TestTwoTimers(void)
{
    const uint32_t kTimeT0        = 1000;
    const uint32_t kTimerInterval = 10;
    ot::Instance * instance       = testInitInstance();
    TestTimer      timer1(*instance);
    TestTimer      timer2(*instance);

    InitTestTimer();
    printf("TestTwoTimers() ");

    // Test when second timer stars at the fire time of first timer (before alarm callback).

    InitCounters();

    sNow = kTimeT0;
    timer1.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == kTimeT0 && sPlatDt == kTimerInterval, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(timer1.IsRunning(), "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestTwoTimers: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    timer2.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == kTimeT0 && sPlatDt == kTimerInterval, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(timer1.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestTwoTimers: Platform Timer State Failed.\n");

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer1.GetFiredCounter() == 1, "TestTwoTimers: Fire Counter failed.\n");
    VerifyOrQuit(sPlatT0 == sNow && sPlatDt == kTimerInterval, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(timer1.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == true, "TestTwoTimers: Platform Timer State Failed.\n");

    sNow += kTimerInterval;
    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 2, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer2.GetFiredCounter() == 1, "TestTwoTimers: Fire Counter failed.\n");
    VerifyOrQuit(timer1.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false, "TestTwoTimers: Platform Timer State Failed.\n");

    // Test when second timer starts at the fire time of first timer (before otPlatAlarmMilliFired()) and its fire time
    // is before the first timer. Ensure that the second timer handler is invoked before the first one.

    InitCounters();
    timer1.ResetFiredCounter();
    timer2.ResetFiredCounter();

    sNow = kTimeT0;
    timer1.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == kTimeT0 && sPlatDt == kTimerInterval, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(timer1.IsRunning(), "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestTwoTimers: Platform Timer State Failed.\n");

    sNow += kTimerInterval;

    timer2.StartAt(kTimeT0, kTimerInterval - 2); // Timer 2 is even before timer 1

    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer1.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestTwoTimers: Platform Timer State Failed.\n");

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer2.GetFiredCounter() == 1, "TestTwoTimers: Fire Counter failed.\n");
    VerifyOrQuit(sPlatT0 == sNow && sPlatDt == 0, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(timer1.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == true, "TestTwoTimers: Platform Timer State Failed.\n");

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 2, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer1.GetFiredCounter() == 1, "TestTwoTimers: Fire Counter failed.\n");
    VerifyOrQuit(timer1.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false, "TestTwoTimers: Platform Timer State Failed.\n");

    // Timer 1 fire callback is late by some ticks/ms, and second timer is scheduled (before call to
    // otPlatAlarmMilliFired) with a maximum interval. This is to test (corner-case) scenario where the fire time of two
    // timers spanning over the maximum interval.

    InitCounters();
    timer1.ResetFiredCounter();
    timer2.ResetFiredCounter();

    sNow = kTimeT0;
    timer1.Start(kTimerInterval);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == kTimeT0 && sPlatDt == kTimerInterval, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(timer1.IsRunning(), "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestTwoTimers: Platform Timer State Failed.\n");

    sNow += kTimerInterval + 5;

    timer2.Start(ot::Timer::kMaxDt);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer1.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn, "TestTwoTimers: Platform Timer State Failed.\n");

    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer1.GetFiredCounter() == 1, "TestTwoTimers: Fire Counter failed.\n");
    VerifyOrQuit(sPlatT0 == sNow, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(sPlatDt == ot::Timer::kMaxDt, "TestTwoTimers: Start params Failed.\n");
    VerifyOrQuit(timer1.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == true, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == true, "TestTwoTimers: Platform Timer State Failed.\n");

    sNow += ot::Timer::kMaxDt;
    otPlatAlarmMilliFired(instance);

    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "TestTwoTimers: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "TestTwoTimers: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 2, "TestTwoTimers: Handler CallCount Failed.\n");
    VerifyOrQuit(timer2.GetFiredCounter() == 1, "TestTwoTimers: Fire Counter failed.\n");
    VerifyOrQuit(timer1.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(timer2.IsRunning() == false, "TestTwoTimers: Timer running Failed.\n");
    VerifyOrQuit(sTimerOn == false, "TestTwoTimers: Platform Timer State Failed.\n");

    printf(" --> PASSED\n");

    testFreeInstance(instance);

    return 0;
}

/**
 * Test the TimerScheduler's behavior of ten timers started and fired.
 *
 * `aTimeShift` is added to the t0 and trigger times for all timers. It can be used to check the ten timer behavior
 * at different start time (e.g., around a 32-bit wrap).
 */
static void TenTimers(uint32_t aTimeShift)
{
    const uint32_t kNumTimers                 = 10;
    const uint32_t kNumTriggers               = 7;
    const uint32_t kTimeT0[kNumTimers]        = {1000, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008};
    const uint32_t kTimerInterval[kNumTimers] = {
        20, 100, (ot::Timer::kMaxDt - kTimeT0[2]), 100000, 1000000, 10, ot::Timer::kMaxDt, 200, 200, 200};
    // Expected timer fire order
    // timer #     Trigger time
    //   5            1014
    //   0            1020
    //   1            1100
    //   7            1206
    //   8            1207
    //   9            1208
    //   3          101002
    //   4         1001003
    //   2          kMaxDt
    //   6   kMaxDt + 1005
    const uint32_t kTriggerTimes[kNumTriggers] = {
        1014, 1020, 1100, 1207, 101004, ot::Timer::kMaxDt, ot::Timer::kMaxDt + kTimeT0[6]};
    // Expected timers fired by each kTriggerTimes[] value
    //  Trigger #    Timers Fired
    //    0             5
    //    1             0
    //    2             1
    //    3             7, 8
    //    4             9, 3
    //    5             4, 2
    //    6             6
    const bool kTimerStateAfterTrigger[kNumTriggers][kNumTimers] = {
        {true, true, true, true, true, false, true, true, true, true},         // 5
        {false, true, true, true, true, false, true, true, true, true},        // 0
        {false, false, true, true, true, false, true, true, true, true},       // 1
        {false, false, true, true, true, false, true, false, false, true},     // 7, 8
        {false, false, true, false, true, false, true, false, false, false},   // 9, 3
        {false, false, false, false, false, false, true, false, false, false}, // 4, 2
        {false, false, false, false, false, false, false, false, false, false} // 6
    };

    const bool kSchedulerStateAfterTrigger[kNumTriggers] = {true, true, true, true, true, true, false};

    const uint32_t kTimerHandlerCountAfterTrigger[kNumTriggers] = {1, 2, 3, 5, 7, 9, 10};

    const uint32_t kTimerStopCountAfterTrigger[kNumTriggers] = {0, 0, 0, 0, 0, 0, 1};

    const uint32_t kTimerStartCountAfterTrigger[kNumTriggers] = {3, 4, 5, 7, 9, 11, 11};

    ot::Instance *instance = testInitInstance();

    TestTimer  timer0(*instance);
    TestTimer  timer1(*instance);
    TestTimer  timer2(*instance);
    TestTimer  timer3(*instance);
    TestTimer  timer4(*instance);
    TestTimer  timer5(*instance);
    TestTimer  timer6(*instance);
    TestTimer  timer7(*instance);
    TestTimer  timer8(*instance);
    TestTimer  timer9(*instance);
    TestTimer *timers[kNumTimers] = {&timer0, &timer1, &timer2, &timer3, &timer4,
                                     &timer5, &timer6, &timer7, &timer8, &timer9};
    size_t     i;

    printf("TestTenTimer() with aTimeShift=%-10u ", aTimeShift);

    // Start the Ten timers.

    InitTestTimer();
    InitCounters();

    for (i = 0; i < kNumTimers; i++)
    {
        sNow = kTimeT0[i] + aTimeShift;
        timers[i]->Start(kTimerInterval[i]);
    }

    // given the order in which timers are started, the TimerScheduler should call otPlatAlarmMilliStartAt 2 times.
    // one for timer[0] and one for timer[5] which will supercede timer[0].
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "TestTenTimer: Start CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "TestTenTimer: Stop CallCount Failed.\n");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "TestTenTimer: Handler CallCount Failed.\n");
    VerifyOrQuit(sPlatT0 == kTimeT0[5] + aTimeShift, "TestTenTimer: Start params Failed.\n");
    VerifyOrQuit(sPlatDt == kTimerInterval[5], "TestTenTimer: Start params Failed.\n");
    VerifyOrQuit(sTimerOn, "TestTenTimer: Platform Timer State Failed.\n");

    for (i = 0; i < kNumTimers; i++)
    {
        VerifyOrQuit(timers[i]->IsRunning(), "TestTenTimer: Timer running Failed.\n");
    }

    // Issue the triggers and test the State after each trigger.

    for (size_t trigger = 0; trigger < kNumTriggers; trigger++)
    {
        sNow = kTriggerTimes[trigger] + aTimeShift;

        do
        {
            // By design, each call to otPlatAlarmMilliFired() can result in 0 or 1 calls to a timer handler.
            // For some combinations of sNow and Timers queued, it is necessary to call otPlatAlarmMilliFired()
            // multiple times in order to handle all the expired timers.  It can be determined that another
            // timer is ready to be triggered by examining the aDt arg passed into otPlatAlarmMilliStartAt().  If
            // that value is 0, then otPlatAlarmMilliFired should be fired immediately. This loop calls
            // otPlatAlarmMilliFired() the requisite number of times based on the aDt argument.
            otPlatAlarmMilliFired(instance);
        } while (sPlatDt == 0);

        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == kTimerStartCountAfterTrigger[trigger],
                     "TestTenTimer: Start CallCount Failed.\n");
        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == kTimerStopCountAfterTrigger[trigger],
                     "TestTenTimer: Stop CallCount Failed.\n");
        VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == kTimerHandlerCountAfterTrigger[trigger],
                     "TestTenTimer: Handler CallCount Failed.\n");
        VerifyOrQuit(sTimerOn == kSchedulerStateAfterTrigger[trigger], "TestTenTimer: Platform Timer State Failed.\n");

        for (i = 0; i < kNumTimers; i++)
        {
            VerifyOrQuit(timers[i]->IsRunning() == kTimerStateAfterTrigger[trigger][i],
                         "TestTenTimer: Timer running Failed.\n");
        }
    }

    for (i = 0; i < kNumTimers; i++)
    {
        VerifyOrQuit(timers[i]->GetFiredCounter() == 1, "TestTenTimer: Timer fired counter Failed.\n");
    }

    printf("--> PASSED\n");

    testFreeInstance(instance);
}

int TestTenTimers(void)
{
    // Time shift to change the start/fire time of ten timers.
    const uint32_t kTimeShift[] = {
        0, 100000U, 0U - 1U, 0U - 1100U, ot::Timer::kMaxDt, ot::Timer::kMaxDt + 1020U,
    };

    size_t i;

    for (i = 0; i < OT_ARRAY_LENGTH(kTimeShift); i++)
    {
        TenTimers(kTimeShift[i]);
    }

    return 0;
}

void RunTimerTests(void)
{
    TestOneTimer();
    TestTwoTimers();
    TestTenTimers();
}

#ifdef ENABLE_TEST_MAIN
int main(void)
{
    RunTimerTests();
    printf("All tests passed\n");
    return 0;
}
#endif
