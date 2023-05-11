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

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/trickle_timer.hpp"

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

extern "C" {

void otPlatAlarmMilliStop(otInstance *)
{
    sTimerOn = false;
    sCallCount[kCallCountIndexAlarmStop]++;
}

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sTimerOn = true;
    sCallCount[kCallCountIndexAlarmStart]++;
    sPlatT0 = aT0;
    sPlatDt = aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

#if OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
void otPlatAlarmMicroStop(otInstance *)
{
    sTimerOn = false;
    sCallCount[kCallCountIndexAlarmStop]++;
}

void otPlatAlarmMicroStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sTimerOn = true;
    sCallCount[kCallCountIndexAlarmStart]++;
    sPlatT0 = aT0;
    sPlatDt = aDt;
}

uint32_t otPlatAlarmMicroGetNow(void) { return sNow; }
#endif

} // extern "C"

void InitCounters(void) { memset(sCallCount, 0, sizeof(sCallCount)); }

/**
 * `TestTrickleTimer` sub-classes `ot::TrickleTimer` and provides a handler and a counter to keep track of number of
 * times timer gets fired.
 */
class TestTrickleTimer : public ot::TrickleTimer
{
public:
    explicit TestTrickleTimer(ot::Instance &aInstance)
        : ot::TrickleTimer(aInstance, TestTrickleTimer::HandleTimerFired)
        , mFiredCounter(0)
    {
    }

    static void HandleTimerFired(ot::TrickleTimer &aTimer)
    {
        static_cast<TestTrickleTimer &>(aTimer).HandleTimerFired();
    }

    void HandleTimerFired(void)
    {
        sCallCount[kCallCountIndexTimerHandler]++;
        mFiredCounter++;
    }

    const uint32_t GetFiredCounter(void) { return mFiredCounter; }

    void ResetFiredCounter(void) { mFiredCounter = 0; }

    static void RemoveAll(ot::Instance &aInstance) { ot::TrickleTimer::RemoveAll(aInstance); }

private:
    uint32_t mFiredCounter; //< Number of times timer has been fired so far
};

void AlarmFired(otInstance *aInstance) { otPlatAlarmMilliFired(aInstance); }

/**
 * Test a single timer in plain mode
 */
int TestOneTrickleTimerPlainMode(void)
{
    const uint32_t   kTimeT0             = 1000;
    const uint32_t   kTimerMinInterval   = 2000;
    const uint32_t   kTimerMaxInterval   = 5000;
    const uint32_t   kRedundancyConstant = 0;
    ot::Instance    *instance            = testInitInstance();
    TestTrickleTimer timer(*instance);
    uint32_t         lastPlatDt;

    // Test one Timer plain mode operation.

    TestTrickleTimer::RemoveAll(*instance);
    InitCounters();

    printf("TestOneTrickleTimerPlainMode() ");

    // picks a random value between min and max before each start

    sNow = kTimeT0;
    timer.Start(ot::TrickleTimer::kModePlainTimer, kTimerMinInterval, kTimerMaxInterval, kRedundancyConstant);

    // validate that the timer is started with a random value between 2000 and 5000 milliseconds
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 1, "Start CallCount Failed.");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 0, "Stop CallCount Failed.");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "Handler CallCount Failed.");
    VerifyOrQuit(sPlatT0 == 1000 && sPlatDt >= 2000 && sPlatDt <= 5000, "Start params Failed");
    lastPlatDt = sPlatDt;

    sNow += sPlatDt; // advance to when the timer should fire

    AlarmFired(instance); // trigger the timer fire event

    // the plain mode trickle timer restarts with a new random value
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == 2, "Start CallCount Failed.");
    VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == 1, "Stop CallCount Failed.");
    VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 1, "Handler CallCount Failed.");
    VerifyOrQuit(sPlatT0 == 1000 + lastPlatDt && sPlatDt >= 2000 && sPlatDt <= 5000, "Start params Failed");
    VerifyOrQuit(timer.IsRunning() == true, "Timer running Failed.");
    VerifyOrQuit(sTimerOn == true, "Platform Timer State Failed.");

    printf(" --> PASSED\n");

    testFreeInstance(instance);

    return 0;
}

/**
 * Test a single timer in trickle mode
 */
int TestOneTrickleTimerTrickleMode(uint32_t aRedundancyConstant, uint32_t aConsistentCalls)
{
    const uint32_t   kTimeT0           = 1000;
    const uint32_t   kTimerMinInterval = 2000;
    const uint32_t   kTimerMaxInterval = 5000;
    ot::Instance    *instance          = testInitInstance();
    TestTrickleTimer timer(*instance);
    uint32_t         halfMinValue = 0;
    uint32_t         interval     = 0;
    size_t           iteration    = 0;
    uint32_t         timeBase     = kTimeT0;

    // Test one Timer trickle mode operation.

    TestTrickleTimer::RemoveAll(*instance);
    InitCounters();

    printf("TestOneTrickleTimerTrickleMode(%u, %u) ", aRedundancyConstant, aConsistentCalls);

    // Start The Timer
    sNow = kTimeT0;
    timer.Start(ot::TrickleTimer::kModeTrickle, kTimerMinInterval, kTimerMaxInterval, aRedundancyConstant);

    halfMinValue = kTimerMinInterval / 2;

    while (interval < kTimerMaxInterval && iteration < 10)
    {
        uint32_t counterBase        = iteration * 2;
        uint32_t beforeRandomPlatDt = 0;
        uint32_t afterRandomPlatDt  = 0;

        printf("Iteration %zu\n", iteration);

        // picks a random value between min & max, then takes half of this and picks a random value between the half and
        // max values

        // validate that the timer is started with a random value between halfMinValue and halfMaxValue milliseconds
        // timer is in kBeforeRandomTime phase
        beforeRandomPlatDt = sPlatDt;
        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == counterBase + 1, "Start CallCount Failed.");
        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == counterBase, "Stop CallCount Failed.");
        if (aConsistentCalls < aRedundancyConstant)
        {
            VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == iteration,
                         "Handler CallCount Failed (aConsistentCalls < aRedundancyConstant).");
        }
        else
        {
            VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "Handler CallCount Failed.");
        }
        VerifyOrQuit(sPlatT0 == timeBase && sPlatDt >= halfMinValue && sPlatDt <= kTimerMaxInterval,
                     "Start params Failed");

        for (size_t idx = 0; idx < aConsistentCalls; idx++)
        {
            timer.IndicateConsistent();
        }

        sNow += sPlatDt; // advance to when the timer should fire

        AlarmFired(instance); // trigger the timer fire event

        // the trickle timer restarts with the rest of the interval time
        afterRandomPlatDt = sPlatDt;
        interval          = beforeRandomPlatDt + afterRandomPlatDt;

        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == counterBase + 2, "Start CallCount Failed.");
        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == counterBase + 1, "Stop CallCount Failed.");

        if (aConsistentCalls < aRedundancyConstant)
        {
            VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == iteration + 1,
                         "Handler CallCount Failed (aConsistentCalls < aRedundancyConstant).");
        }
        else
        {
            VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "Handler CallCount Failed.");
        }

        VerifyOrQuit(sPlatT0 == timeBase + beforeRandomPlatDt && (sPlatDt + beforeRandomPlatDt) >= kTimerMinInterval &&
                         (sPlatDt + beforeRandomPlatDt) <= kTimerMaxInterval,
                     "Start params Failed.");
        VerifyOrQuit(interval >= kTimerMinInterval && interval <= kTimerMaxInterval, "Interval Invalid.");
        VerifyOrQuit(timer.IsRunning() == true, "Timer running Failed.");
        VerifyOrQuit(sTimerOn == true, "Platform Timer State Failed.");

        // Now if we fire again, we're in kAfterRandomTime and when it fires it will re-start the interval again with a
        // bigger mInterval value until mInteval = 5000
        sNow += sPlatDt; // advance to when the timer should fire

        AlarmFired(instance); // trigger the timer fire event

        // If the loop runs again, thise will be checked once more, but this catches the last iteration
        halfMinValue = interval / 2;
        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStart] == counterBase + 3, "Start CallCount Failed.");
        VerifyOrQuit(sCallCount[kCallCountIndexAlarmStop] == counterBase + 2, "Stop CallCount Failed.");

        if (aConsistentCalls < aRedundancyConstant)
        {
            VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == iteration + 1,
                         "Handler CallCount Failed (aConsistentCalls < aRedundancyConstant).");
        }
        else
        {
            VerifyOrQuit(sCallCount[kCallCountIndexTimerHandler] == 0, "Handler CallCount Failed.");
        }

        VerifyOrQuit(sPlatT0 == timeBase + interval && sPlatDt >= halfMinValue && sPlatDt <= kTimerMaxInterval,
                     "Start params Failed.");
        VerifyOrQuit(timer.IsRunning() == true, "Timer running Failed.");
        VerifyOrQuit(sTimerOn == true, "Platform Timer State Failed.");

        timeBase += interval; // Increase the time base by the interval
        iteration++;
    }

    VerifyOrQuit(iteration < 10, "Too many iterations");

    printf(" --> PASSED\n");

    testFreeInstance(instance);

    return 0;
}

int main(void)
{
    TestOneTrickleTimerPlainMode();
    TestOneTrickleTimerTrickleMode(5, 3); // validate with fewer consistency calls than needed to supress timer
    TestOneTrickleTimerTrickleMode(3, 3); // validate with sufficient consistency calls to suppress timer
    printf("All tests passed\n");
    return 0;
}
