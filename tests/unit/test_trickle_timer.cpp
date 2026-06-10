/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
#include "common/num_utils.hpp"
#include "common/trickle_timer.hpp"
#include "instance/instance.hpp"

namespace ot {

static Instance *sInstance;

static uint32_t sNow = 0;
static uint32_t sAlarmTime;
static bool     sAlarmOn = false;

extern "C" {

void otPlatAlarmMilliStop(otInstance *) { sAlarmOn = false; }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmOn   = true;
    sAlarmTime = aT0 + aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return sNow; }

} // extern "C"

void AdvanceTime(uint32_t aDuration)
{
    uint32_t time = sNow + aDuration;

    while (TimeMilli(sAlarmTime) <= TimeMilli(time))
    {
        sNow = sAlarmTime;
        otPlatAlarmMilliFired(sInstance);
    }

    sNow = time;
}

class TrickleTimerTester : public TrickleTimer
{
public:
    explicit TrickleTimerTester(Instance &aInstance)
        : TrickleTimer(aInstance, HandleTimerFired)
        , mDidFire(false)
    {
    }

    Time     GetFireTime(void) const { return TimerMilli::GetFireTime(); }
    uint32_t GetInterval(void) const { return TrickleTimer::mInterval; }
    uint32_t GetTimeInInterval(void) const { return TrickleTimer::mTimeInInterval; }

    void VerifyTimerDidFire(void)
    {
        VerifyOrQuit(mDidFire);
        mDidFire = false;
    }

    void VerifyTimerDidNotFire(void) const { VerifyOrQuit(!mDidFire); }

    static void RemoveAll(Instance &aInstance) { TimerMilli::RemoveAll(aInstance); }

private:
    static void HandleTimerFired(TrickleTimer &aTimer) { static_cast<TrickleTimerTester &>(aTimer).HandleTimerFired(); }
    void        HandleTimerFired(void) { mDidFire = true; }

    bool mDidFire;
};

void AlarmFired(otInstance *aInstance) { otPlatAlarmMilliFired(aInstance); }

void TestTrickleTimerPlainMode(void)
{
    static constexpr uint32_t kMinInterval = 2000;
    static constexpr uint32_t kMaxInterval = 5000;

    Instance          *instance = testInitInstance();
    TrickleTimerTester timer(*instance);
    uint32_t           interval;

    sInstance = instance;
    TrickleTimerTester::RemoveAll(*instance);

    printf("TestTrickleTimerPlainMode() ");

    // Validate that timer picks a random interval between min and max
    // on start.

    sNow = 1000;
    timer.Start(TrickleTimer::kModePlainTimer, kMinInterval, kMaxInterval, 0);

    VerifyOrQuit(timer.IsRunning());
    VerifyOrQuit(timer.GetIntervalMax() == kMaxInterval);
    VerifyOrQuit(timer.GetIntervalMin() == kMinInterval);

    interval = timer.GetInterval();
    VerifyOrQuit((interval >= kMinInterval) && (interval <= kMaxInterval));

    for (uint8_t iter = 0; iter <= 10; iter++)
    {
        AdvanceTime(interval);

        timer.VerifyTimerDidFire();

        // The plain mode trickle timer restarts with a new random
        // interval between min and max.

        VerifyOrQuit(timer.IsRunning());
        interval = timer.GetInterval();
        VerifyOrQuit((interval >= kMinInterval) && (interval <= kMaxInterval));
    }

    printf(" --> PASSED\n");

    testFreeInstance(instance);
}

void TestTrickleTimerTrickleMode(uint32_t aRedundancyConstant, uint32_t aConsistentCalls)
{
    static constexpr uint32_t kMinInterval = 1000;
    static constexpr uint32_t kMaxInterval = 9000;

    Instance          *instance = testInitInstance();
    TrickleTimerTester timer(*instance);
    uint32_t           interval;
    uint32_t           t;

    sInstance = instance;
    TrickleTimerTester::RemoveAll(*instance);

    printf("TestTrickleTimerTrickleMode(aRedundancyConstant:%u, aConsistentCalls:%u) ", aRedundancyConstant,
           aConsistentCalls);

    sNow = 1000;
    timer.Start(TrickleTimer::kModeTrickle, kMinInterval, kMaxInterval, aRedundancyConstant);

    // Validate that trickle timer starts with random interval between
    // min/max.

    VerifyOrQuit(timer.IsRunning());
    VerifyOrQuit(timer.GetIntervalMax() == kMaxInterval);
    VerifyOrQuit(timer.GetIntervalMin() == kMinInterval);

    interval = timer.GetInterval();
    VerifyOrQuit((kMinInterval <= interval) && (interval <= kMaxInterval));
    t = timer.GetTimeInInterval();
    VerifyOrQuit((interval / 2 <= t) && (t <= interval));

    // After `IndicateInconsistent()` should go back to min
    // interval.

    timer.IndicateInconsistent();

    VerifyOrQuit(timer.IsRunning());
    interval = timer.GetInterval();
    VerifyOrQuit(interval == kMinInterval);
    t = timer.GetTimeInInterval();
    VerifyOrQuit((interval / 2 <= t) && (t <= interval));

    for (uint8_t iter = 0; iter < 10; iter++)
    {
        for (uint32_t index = 0; index < aConsistentCalls; index++)
        {
            timer.IndicateConsistent();
        }

        AdvanceTime(t);

        if (aConsistentCalls < aRedundancyConstant)
        {
            timer.VerifyTimerDidFire();
        }
        else
        {
            timer.VerifyTimerDidNotFire();
        }

        AdvanceTime(interval - t);

        // Verify that interval is doubling each time up
        // to max interval.

        VerifyOrQuit(timer.IsRunning());
        VerifyOrQuit(timer.GetInterval() == Min(interval * 2, kMaxInterval));

        interval = timer.GetInterval();
        t        = timer.GetTimeInInterval();
        VerifyOrQuit((interval / 2 <= t) && (t <= interval));
    }

    AdvanceTime(t);

    timer.IndicateInconsistent();

    VerifyOrQuit(timer.IsRunning());
    interval = timer.GetInterval();
    VerifyOrQuit(interval == kMinInterval);

    printf(" --> PASSED\n");

    testFreeInstance(instance);
}

void TestTrickleTimerMinMaxIntervalChange(void)
{
    Instance          *instance = testInitInstance();
    TrickleTimerTester timer(*instance);
    TimeMilli          fireTime;
    uint32_t           interval;
    uint32_t           t;

    sInstance = instance;
    TrickleTimerTester::RemoveAll(*instance);

    printf("TestTrickleTimerMinMaxIntervalChange()");

    sNow = 1000;
    timer.Start(TrickleTimer::kModeTrickle, 2000, 4000);

    VerifyOrQuit(timer.IsRunning());
    VerifyOrQuit(timer.GetIntervalMin() == 2000);
    VerifyOrQuit(timer.GetIntervalMax() == 4000);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that `SetIntervalMin()` to a larger value than
    // previously set does not impact the current interval.

    timer.IndicateInconsistent();
    interval = timer.GetInterval();
    t        = timer.GetTimeInInterval();
    fireTime = timer.GetFireTime();

    VerifyOrQuit(interval == 2000);
    VerifyOrQuit((interval / 2 <= t) && (t < interval));

    // Change `IntervalMin` before time `t`.

    timer.SetIntervalMin(3000);

    VerifyOrQuit(timer.IsRunning());
    VerifyOrQuit(timer.GetIntervalMin() == 3000);
    VerifyOrQuit(timer.GetIntervalMax() == 4000);

    VerifyOrQuit(interval == timer.GetInterval());
    VerifyOrQuit(t == timer.GetTimeInInterval());
    VerifyOrQuit(fireTime == timer.GetFireTime());

    AdvanceTime(t);
    timer.VerifyTimerDidFire();
    fireTime = timer.GetFireTime();

    // Change `IntervalMin` after time `t`.

    timer.SetIntervalMin(3500);

    VerifyOrQuit(timer.IsRunning());
    VerifyOrQuit(timer.GetIntervalMin() == 3500);
    VerifyOrQuit(timer.GetIntervalMax() == 4000);

    VerifyOrQuit(interval == timer.GetInterval());
    VerifyOrQuit(t == timer.GetTimeInInterval());
    VerifyOrQuit(fireTime == timer.GetFireTime());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that `SetIntervalMin()` to a smaller value
    // also does not impact the current interval.

    timer.IndicateInconsistent();

    interval = timer.GetInterval();
    t        = timer.GetTimeInInterval();
    fireTime = timer.GetFireTime();

    VerifyOrQuit(interval == 3500);
    VerifyOrQuit((interval / 2 <= t) && (t < interval));

    // Change `IntervalMin` before time `t`.

    timer.SetIntervalMin(3000);

    VerifyOrQuit(timer.IsRunning());
    VerifyOrQuit(timer.GetIntervalMin() == 3000);
    VerifyOrQuit(timer.GetIntervalMax() == 4000);

    VerifyOrQuit(interval == timer.GetInterval());
    VerifyOrQuit(t == timer.GetTimeInInterval());
    VerifyOrQuit(fireTime == timer.GetFireTime());

    AdvanceTime(t);
    timer.VerifyTimerDidFire();
    fireTime = timer.GetFireTime();

    // Change `IntervalMin` after time `t`.

    timer.SetIntervalMin(2000);

    VerifyOrQuit(timer.IsRunning());
    VerifyOrQuit(timer.GetIntervalMin() == 2000);
    VerifyOrQuit(timer.GetIntervalMax() == 4000);

    VerifyOrQuit(interval == timer.GetInterval());
    VerifyOrQuit(t == timer.GetTimeInInterval());
    VerifyOrQuit(fireTime == timer.GetFireTime());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Validate that changing `IntervalMax` to a larger value
    // than the current interval being used by timer, does not
    // impact the current internal.

    timer.IndicateInconsistent();

    interval = timer.GetInterval();
    t        = timer.GetTimeInInterval();
    fireTime = timer.GetFireTime();

    VerifyOrQuit(interval == 2000);
    VerifyOrQuit((interval / 2 <= t) && (t < interval));

    // Change `IntervalMax` before time `t`.

    timer.SetIntervalMax(2500);

    VerifyOrQuit(timer.GetIntervalMax() == 2500);
    VerifyOrQuit(timer.IsRunning());

    VerifyOrQuit(interval == timer.GetInterval());
    VerifyOrQuit(t == timer.GetTimeInInterval());
    VerifyOrQuit(fireTime == timer.GetFireTime());

    AdvanceTime(t);

    timer.VerifyTimerDidFire();

    fireTime = timer.GetFireTime();

    // Change `IntervalMax` after time `t`.

    timer.SetIntervalMax(3000);

    VerifyOrQuit(interval == timer.GetInterval());
    VerifyOrQuit(t == timer.GetTimeInInterval());
    VerifyOrQuit(fireTime == timer.GetFireTime());

    timer.Stop();
    VerifyOrQuit(!timer.IsRunning());

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Check behavior when the new `IntervalMax` is smaller
    // than the current interval being used by timer.

    // New `Imax` is smaller than `t` and before now.
    //
    //   |<---- interval --^-------------------------------->|
    //   |<---- t ---------^------------------>|             |
    //   |<---- new Imax --^--->|              |             |
    //   |                now   |              |             |

    timer.Start(TrickleTimer::kModeTrickle, 2000, 2000);
    interval = timer.GetInterval();
    t        = timer.GetTimeInInterval();
    fireTime = timer.GetFireTime();

    VerifyOrQuit(interval == 2000);
    VerifyOrQuit((interval / 2 <= t) && (t < interval));
    timer.SetIntervalMin(500);

    AdvanceTime(100);
    timer.VerifyTimerDidNotFire();

    timer.SetIntervalMax(500);

    VerifyOrQuit(timer.GetInterval() == 500);
    VerifyOrQuit(timer.GetTimeInInterval() == 500);
    VerifyOrQuit(timer.GetFireTime() != fireTime);
    timer.VerifyTimerDidNotFire();

    AdvanceTime(400);
    timer.VerifyTimerDidFire();

    // New `Imax` is smaller than `t` and after now.
    //
    //   |<---- interval --------------^-------------------->|
    //   |<---- t ---------------------^------>|             |
    //   |<---- new Imax ------>|      ^       |             |
    //   |                      |     now      |             |

    timer.Start(TrickleTimer::kModeTrickle, 2000, 2000);
    interval = timer.GetInterval();
    t        = timer.GetTimeInInterval();
    fireTime = timer.GetFireTime();

    VerifyOrQuit(interval == 2000);
    VerifyOrQuit((interval / 2 <= t) && (t < interval));
    timer.SetIntervalMin(500);

    AdvanceTime(800);
    timer.VerifyTimerDidNotFire();

    timer.SetIntervalMax(500);

    VerifyOrQuit(timer.GetInterval() == 500);
    VerifyOrQuit(timer.GetTimeInInterval() == 500);
    VerifyOrQuit(timer.GetFireTime() != fireTime);
    timer.VerifyTimerDidNotFire();

    AdvanceTime(0);
    timer.VerifyTimerDidFire();

    // New `Imax` is larger than `t` and before now.
    //
    //   |<---- interval --------------------------------^-->|
    //   |<---- t ---------------------------->|         ^   |
    //   |<---- new Imax --------------------------->|   ^   |
    //   |                                     |     |  now  |

    timer.Start(TrickleTimer::kModeTrickle, 2000, 2000);

    interval = timer.GetInterval();
    t        = timer.GetTimeInInterval();

    VerifyOrQuit(interval == 2000);
    VerifyOrQuit((interval / 2 <= t) && (t < interval));
    timer.SetIntervalMin(500);

    AdvanceTime(1999);
    timer.VerifyTimerDidFire();

    timer.SetIntervalMax(t + 1);

    VerifyOrQuit(timer.GetInterval() == t + 1);
    fireTime = timer.GetFireTime();

    // Check that new interval is started immediately.
    AdvanceTime(0);
    timer.VerifyTimerDidNotFire();
    VerifyOrQuit(fireTime != timer.GetFireTime());
    VerifyOrQuit(timer.GetInterval() == timer.GetIntervalMax());

    // New `Imax` is larger than `t` and after now.
    //
    //   |<---- interval -------------------------^--------->|
    //   |<---- t ---------------------------->|  ^          |
    //   |<---- new Imax -------------------------^->|       |
    //   |                                     | now |       |

    timer.Start(TrickleTimer::kModeTrickle, 2000, 2000);

    interval = timer.GetInterval();
    t        = timer.GetTimeInInterval();

    VerifyOrQuit(interval == 2000);
    VerifyOrQuit((interval / 2 <= t) && (t < interval));
    timer.SetIntervalMin(500);

    AdvanceTime(t);
    timer.VerifyTimerDidFire();

    timer.SetIntervalMax(t + 1);

    VerifyOrQuit(timer.GetInterval() == t + 1);
    fireTime = timer.GetFireTime();

    AdvanceTime(1);
    timer.VerifyTimerDidNotFire();
    VerifyOrQuit(fireTime != timer.GetFireTime());
    VerifyOrQuit(timer.GetInterval() == timer.GetIntervalMax());

    printf(" --> PASSED\n");

    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestTrickleTimerPlainMode();
    ot::TestTrickleTimerTrickleMode(/* aRedundancyConstant */ 5, /* aConsistentCalls */ 3);
    ot::TestTrickleTimerTrickleMode(/* aRedundancyConstant */ 3, /* aConsistentCalls */ 3);
    ot::TestTrickleTimerMinMaxIntervalChange();

    printf("All tests passed\n");
    return 0;
}
