/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include <openthread/config.h>

#include "instance/instance.hpp"

namespace ot {

#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE

enum RadioState
{
    kRadioStateSleep,
    kRadioStateReceive,
};

static uint64_t   sNow                      = 10000000;
static RadioState sRadioState               = kRadioStateSleep;
static uint8_t    sReceiveChannel           = 0;
static uint32_t   sReceiveCallCount         = 0;
static uint32_t   sLocalRadioTimeDifference = 0;

static bool     sAlarmMicroOn = false;
static uint32_t sAlarmMicroT0 = 0;
static uint32_t sAlarmMicroDt = 0;

static bool     sAlarmMilliOn = false;
static uint32_t sAlarmMilliT0 = 0;
static uint32_t sAlarmMilliDt = 0;

extern "C" {

otRadioTime64 otPlatRadioGetNow(otInstance *) { return sNow + sLocalRadioTimeDifference; }

otError otPlatRadioReceive(otInstance *, uint8_t aChannel)
{
    sRadioState     = kRadioStateReceive;
    sReceiveChannel = aChannel;
    sReceiveCallCount++;
    return OT_ERROR_NONE;
}

otError otPlatRadioSleep(otInstance *)
{
    sRadioState = kRadioStateSleep;
    return OT_ERROR_NONE;
}

otRadioCaps otPlatRadioGetCaps(otInstance *) { return OT_RADIO_CAPS_ACK_TIMEOUT | OT_RADIO_CAPS_CSMA_BACKOFF; }

void otPlatAlarmMicroStop(otInstance *) { sAlarmMicroOn = false; }

void otPlatAlarmMicroStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmMicroOn = true;
    sAlarmMicroT0 = aT0;
    sAlarmMicroDt = aDt;
}

uint32_t otPlatAlarmMicroGetNow(void) { return static_cast<uint32_t>(sNow); }

void otPlatAlarmMilliStop(otInstance *) { sAlarmMilliOn = false; }

void otPlatAlarmMilliStartAt(otInstance *, uint32_t aT0, uint32_t aDt)
{
    sAlarmMilliOn = true;
    sAlarmMilliT0 = aT0;
    sAlarmMilliDt = aDt;
}

uint32_t otPlatAlarmMilliGetNow(void) { return static_cast<uint32_t>(sNow / 1000); }

} // extern "C"

static void ResetPlatformState(void)
{
    sNow              = 10000000;
    sRadioState       = kRadioStateSleep;
    sReceiveChannel   = 0;
    sReceiveCallCount = 0;
    sAlarmMicroOn     = false;
    sAlarmMicroT0     = 0;
    sAlarmMicroDt     = 0;
    sAlarmMilliOn     = false;
    sAlarmMilliT0     = 0;
    sAlarmMilliDt     = 0;
}

static void AdvanceTime(Instance &aInstance, uint32_t aDurationUs)
{
    uint64_t targetTime = sNow + aDurationUs;

    while (sNow < targetTime)
    {
        uint32_t step = static_cast<uint32_t>(targetTime - sNow);

        if (sAlarmMicroOn)
        {
            uint32_t alarmFireTime = sAlarmMicroT0 + sAlarmMicroDt;
            uint32_t timeToAlarm   = alarmFireTime - static_cast<uint32_t>(sNow);

            if (timeToAlarm <= step)
            {
                step = timeToAlarm;
            }
        }

        sNow += step;

        if (sAlarmMicroOn && (static_cast<uint32_t>(sNow) == sAlarmMicroT0 + sAlarmMicroDt))
        {
            sAlarmMicroOn = false;
            otPlatAlarmMicroFired(&aInstance);
        }

        if (sAlarmMilliOn && (static_cast<uint32_t>(sNow / 1000) == sAlarmMilliT0 + sAlarmMilliDt))
        {
            sAlarmMilliOn = false;
            otPlatAlarmMilliFired(&aInstance);
        }

        otTaskletsProcess(&aInstance);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TestSimpleReceiveAt(void)
{
    Instance    *instance;
    Mac::SubMac *subMac;
    uint64_t     startTime;
    uint32_t     duration;
    uint8_t      channel;

    printf("TestSimpleReceiveAt()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    VerifyOrQuit(sRadioState == kRadioStateSleep);

    // Schedule a simple future `ReceiveAt` window and verify that the
    // radio stays in sleep before the start time, transitions to receive
    // during the window, and returns to sleep when the window ends.

    startTime = otPlatRadioGetNow(instance) + 10000;
    duration  = 5000;
    channel   = 11;

    subMac->ReceiveAt(startTime, duration, channel);

    AdvanceTime(*instance, 9999);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should be in sleep before window start");

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should enter receive at window start");
    VerifyOrQuit(sReceiveChannel == channel, "Receive channel mismatch");

    AdvanceTime(*instance, 4999);
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should stay in receive during window");

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should return to sleep at window end");

    testFreeInstance(instance);
}

void TestPendingReceiveAtReplaced(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;

    printf("TestPendingReceiveAtReplaced()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, replace it before it starts.
    // Validate that the new `ReceiveAt` is executed.

    startTime1 = otPlatRadioGetNow(instance) + 100;
    subMac->ReceiveAt(startTime1, 50, 11);

    AdvanceTime(*instance, 20);

    startTime2 = otPlatRadioGetNow(instance) + 200;
    subMac->ReceiveAt(startTime2, 50, 12);

    AdvanceTime(*instance, 140);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should NOT enter receive for replaced window 1");

    AdvanceTime(*instance, static_cast<uint32_t>(startTime2 - otPlatRadioGetNow(instance)));

    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should enter receive for window 2");
    VerifyOrQuit(sReceiveChannel == 12, "Receive channel mismatch for window 2");

    AdvanceTime(*instance, 50);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should return to sleep after window 2");

    testFreeInstance(instance);
}

void TestReceiveAtAlreadyStarted(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime;
    uint32_t      duration;
    uint8_t       channel;

    printf("TestReceiveAtAlreadyStarted()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt` which is already started.

    startTime = otPlatRadioGetNow(instance) - 2000;
    duration  = 10000;
    channel   = 15;

    subMac->ReceiveAt(startTime, duration, channel);

    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should immediately enter receive for started window");
    VerifyOrQuit(sReceiveChannel == channel, "Receive channel mismatch");

    AdvanceTime(*instance, 7999);
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should remain in receive");

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should return to sleep when window ends");

    testFreeInstance(instance);
}

void TestReceiveAtAlreadyExpired(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime;
    uint32_t      duration;

    printf("TestReceiveAtAlreadyExpired()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt` which is already expired.

    startTime = otPlatRadioGetNow(instance) - 10000;
    duration  = 5000;

    subMac->ReceiveAt(startTime, duration, 11);

    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep for expired window");

    AdvanceTime(*instance, 10000);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");
    VerifyOrQuit(sReceiveCallCount == 0);

    testFreeInstance(instance);
}

void TestReceiveAtWhileActive(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;

    printf("TestReceiveAtWhileActive()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, wait for it to start,
    // then schedule a new one while the previous one is
    // active.

    startTime1 = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime1, 100, 11);

    AdvanceTime(*instance, 70);
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should be in receive for window 1");

    startTime2 = startTime1 + 200;
    subMac->ReceiveAt(startTime2, 50, 12);

    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should remain in receive for active window 1");
    VerifyOrQuit(sReceiveChannel == 11, "Radio should remain on channel 11");

    AdvanceTime(*instance, 80);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 1 ends");

    AdvanceTime(*instance, static_cast<uint32_t>(startTime2 - otPlatRadioGetNow(instance)));
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should enter receive for window 2");
    VerifyOrQuit(sReceiveChannel == 12, "Radio should switch to channel 12 for window 2");

    AdvanceTime(*instance, 50);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 2 ends");

    testFreeInstance(instance);
}

void TestSleepDuringActiveReceiveAt(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime;
    uint32_t      duration;
    uint8_t       channel;

    printf("TestSleepDuringActiveReceiveAt()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, wait for it to start, then while in the
    // middle of the active window call `Sleep()` multiple times.
    // Ensure that explicit calls to `Sleep()` do not interrupt the
    // active timed RX window and the radio remains in receive until
    // the window ends.

    startTime = otPlatRadioGetNow(instance) + 50;
    duration  = 50;
    channel   = 11;
    subMac->ReceiveAt(startTime, duration, channel);

    AdvanceTime(*instance, 50);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == channel);

    AdvanceTime(*instance, 20);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == channel);

    SuccessOrQuit(subMac->Sleep());
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Sleep() during active window should not stop timed-rx");
    VerifyOrQuit(sReceiveChannel == channel);

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == channel);

    SuccessOrQuit(subMac->Sleep());
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Subsequent Sleep() should not stop timed-rx");
    VerifyOrQuit(sReceiveChannel == channel);

    AdvanceTime(*instance, 19);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == channel);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window ends");

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    testFreeInstance(instance);
}

void TestOverlappingReceiveAtSameChannel(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;
    uint32_t      initialRxCallCount;

    printf("TestOverlappingReceiveAtSameChannel()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, wait for it to start, then schedule an
    // overlapping one on the same channel, which ends earlier than
    // the original one.

    startTime1 = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime1, 100, 11);

    AdvanceTime(*instance, 60);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    initialRxCallCount = sReceiveCallCount;

    startTime2 = startTime1 + 20;
    subMac->ReceiveAt(startTime2, 5, 11);

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);
    VerifyOrQuit(sReceiveCallCount == initialRxCallCount);

    AdvanceTime(*instance, 4);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);
    VerifyOrQuit(sReceiveCallCount == initialRxCallCount);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after overlapping window 2 ends");

    testFreeInstance(instance);
}

void TestOverlappingReceiveAtDifferentChannel(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;

    printf("TestOverlappingReceiveAtDifferentChannel()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, wait for it to start, then schedule an
    // overlapping one on a different channel, which ends earlier than
    // the original one.

    startTime1 = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime1, 100, 11);

    AdvanceTime(*instance, 60);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    startTime2 = startTime1 + 20;
    subMac->ReceiveAt(startTime2, 6, 15);

    AdvanceTime(*instance, 9);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15, "Radio should switch to channel 15 when window 2 starts");

    AdvanceTime(*instance, 5);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 2 ends");

    testFreeInstance(instance);
}

void TestOverlappingReceiveAtExtendingPastEnd(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;

    printf("TestOverlappingReceiveAtExtendingPastEnd()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt` window 1, wait for it to start, then
    // schedule an overlapping window 2 on a different channel that
    // starts before window 1 ends and extends past window 1's end.
    // Ensure channel switches when window 2 starts and reception
    // continues until window 2 ends.

    startTime1 = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime1, 50, 11);

    AdvanceTime(*instance, 50);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 20);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    startTime2 = startTime1 + 30;
    subMac->ReceiveAt(startTime2, 60, 15);

    AdvanceTime(*instance, 9);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15, "Radio should switch to channel 15 when window 2 starts");

    AdvanceTime(*instance, 19);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15, "Radio should remain in receive past window 1 end");

    AdvanceTime(*instance, 39);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 2 ends");

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    testFreeInstance(instance);
}

void TestAdjacentBackToBackReceiveAt(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;

    printf("TestAdjacentBackToBackReceiveAt()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt` window 1, then while it is active schedule
    // a back-to-back adjacent window 2 on a different channel (with zero
    // gap between windows). Ensure the radio switches channels at the
    // boundary tick without glitching into sleep, and concludes when
    // window 2 ends.

    startTime1 = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime1, 50, 11);

    AdvanceTime(*instance, 50);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 20);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    startTime2 = startTime1 + 50;
    subMac->ReceiveAt(startTime2, 40, 15);

    AdvanceTime(*instance, 29);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should switch to window 2 channel at boundary");
    VerifyOrQuit(sReceiveChannel == 15, "Receive channel mismatch for window 2");

    AdvanceTime(*instance, 39);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 2 ends");

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    testFreeInstance(instance);
}

void TestOverlappingReceiveAtSubsumingWindow(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;

    printf("TestOverlappingReceiveAtSubsumingWindow()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt` window 1, wait for it to start, then
    // schedule an overlapping window 2 on a different channel that
    // started earlier and ends later (fully subsuming window 1).
    // Ensure channel switches immediately to window 2 channel and
    // continues receiving until window 2 ends.

    startTime1 = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime1, 50, 11);

    AdvanceTime(*instance, 50);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 20);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    // Schedule window 2 ([t0 + 40, t0 + 120]) on channel 15 which
    // started earlier than current time (t0 + 70) and ends later
    // than window 1 (t0 + 100).
    startTime2 = startTime1 - 10;
    subMac->ReceiveAt(startTime2, 80, 15);

    // Radio should immediately switch to window 2 channel
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15, "Radio should switch to window 2 channel immediately");

    AdvanceTime(*instance, 29);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15);

    // Window 1 original end (t0 + 100); verify radio remains receiving on channel 15
    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15, "Radio should remain in receive past window 1 end");

    AdvanceTime(*instance, 19);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15);

    // Window 2 ends at t0 + 120; verify transition to sleep
    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 2 ends");

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    testFreeInstance(instance);
}

void TestFullyMissedReceiveAt(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime;
    uint8_t       rxChannel;

    printf("TestFullyMissedReceiveAt()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, before it can start call `Receive()` on
    // a different channel blocking timed-rx, wait until after the end
    // of the scheduled rx to call `Sleep()` to ensure the schedule is
    // fully missed.

    startTime = otPlatRadioGetNow(instance) + 60;
    subMac->ReceiveAt(startTime, 40, 11);

    AdvanceTime(*instance, 30);
    VerifyOrQuit(sRadioState == kRadioStateSleep);

    rxChannel = 16;
    SuccessOrQuit(subMac->Receive(rxChannel));
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    AdvanceTime(*instance, 70);

    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    SuccessOrQuit(subMac->Sleep());
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep since timed window was fully missed");

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    for (uint16_t i = 0; i < 100; i++)
    {
        AdvanceTime(*instance, 1);
        VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");
    }

    testFreeInstance(instance);
}

void TestInterruptedReceiveAt(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime;
    uint8_t       rxChannel;

    printf("TestInterruptedReceiveAt()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, before it can start call `Receive()` on
    // a different channel blocking timed-rx, then before the scheduled
    // rx window ends call `Sleep()`. Ensure the timed-rx is started
    // on its channel after `Sleep()` and then concludes properly when
    // the window ends.

    startTime = otPlatRadioGetNow(instance) + 60;
    subMac->ReceiveAt(startTime, 40, 11);

    AdvanceTime(*instance, 50);
    VerifyOrQuit(sRadioState == kRadioStateSleep);

    rxChannel = 18;
    SuccessOrQuit(subMac->Receive(rxChannel));
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    AdvanceTime(*instance, 25);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    SuccessOrQuit(subMac->Sleep());
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should switch to timed-rx channel");
    VerifyOrQuit(sReceiveChannel == 11, "Receive channel mismatch");

    AdvanceTime(*instance, 24);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after resumed window ends");

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    testFreeInstance(instance);
}

void TestInterruptedActiveReceiveAt(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime;
    uint8_t       rxChannel;

    printf("TestInterruptedActiveReceiveAt()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt`, wait for it to start, then interrupt it
    // by calling `Receive()` on a different channel, then before the
    // scheduled rx window ends call `Sleep()`. Ensure the timed-rx
    // is resumed on its channel and then concludes properly when the
    // window ends.

    startTime = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime, 50, 11);

    AdvanceTime(*instance, 49);
    VerifyOrQuit(sRadioState == kRadioStateSleep);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    rxChannel = 18;
    SuccessOrQuit(subMac->Receive(rxChannel));
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    AdvanceTime(*instance, 15);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    SuccessOrQuit(subMac->Sleep());
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should switch back to timed-rx channel");
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 24);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after resumed window ends");

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    testFreeInstance(instance);
}

void TestInterruptedReceiveAtWithSecondSchedule(void)
{
    Instance     *instance;
    Mac::SubMac  *subMac;
    Radio::Time64 startTime1;
    Radio::Time64 startTime2;
    uint8_t       rxChannel;

    printf("TestInterruptedReceiveAtWithSecondSchedule()\n");
    ResetPlatformState();

    instance = testInitInstance();
    subMac   = &instance->Get<Mac::SubMac>();

    SuccessOrQuit(subMac->Enable());
    SuccessOrQuit(subMac->Sleep());

    // Schedule a `ReceiveAt` window 1, before it starts block it by
    // calling `Receive()` on a different channel. Wait until window 1
    // starts and while blocked in receive state, schedule a future
    // `ReceiveAt` window 2. Advance time a bit (still within window 1),
    // then call `Sleep()`. Ensure window 1 is resumed on its channel,
    // ends properly, and window 2 starts and ends as scheduled.

    startTime1 = otPlatRadioGetNow(instance) + 50;
    subMac->ReceiveAt(startTime1, 50, 11);

    AdvanceTime(*instance, 30);
    VerifyOrQuit(sRadioState == kRadioStateSleep);

    rxChannel = 20;
    SuccessOrQuit(subMac->Receive(rxChannel));
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    AdvanceTime(*instance, 40);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    startTime2 = startTime1 + 200;
    subMac->ReceiveAt(startTime2, 7, 15);

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == rxChannel);

    SuccessOrQuit(subMac->Sleep());
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should switch back to window 1 channel");
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 19);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 11);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 1 ends");

    AdvanceTime(*instance, 149);
    VerifyOrQuit(sRadioState == kRadioStateSleep);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateReceive, "Radio should enter receive for window 2");
    VerifyOrQuit(sReceiveChannel == 15);

    AdvanceTime(*instance, 6);
    VerifyOrQuit(sRadioState == kRadioStateReceive);
    VerifyOrQuit(sReceiveChannel == 15);

    AdvanceTime(*instance, 1);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should sleep after window 2 ends");

    AdvanceTime(*instance, 10);
    VerifyOrQuit(sRadioState == kRadioStateSleep, "Radio should remain in sleep");

    testFreeInstance(instance);
}

#endif // OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE

} // namespace ot

int main(void)
{
#if OT_CONFIG_MAC_TARGET_TIME_RX_ENABLE
    const uint32_t kLocalRadioTimeDifferences[] = {0, 1000, 1000000};

    for (uint32_t diff : kLocalRadioTimeDifferences)
    {
        printf("------------------------------------------------------\n");
        printf("Setting sLocalRadioTimeDifference: %lu\n", ot::ToUlong(diff));

        ot::sLocalRadioTimeDifference = diff;

        ot::TestSimpleReceiveAt();
        ot::TestPendingReceiveAtReplaced();
        ot::TestReceiveAtAlreadyStarted();
        ot::TestReceiveAtAlreadyExpired();
        ot::TestReceiveAtWhileActive();
        ot::TestSleepDuringActiveReceiveAt();
        ot::TestOverlappingReceiveAtSameChannel();
        ot::TestOverlappingReceiveAtDifferentChannel();
        ot::TestOverlappingReceiveAtExtendingPastEnd();
        ot::TestAdjacentBackToBackReceiveAt();
        ot::TestOverlappingReceiveAtSubsumingWindow();
        ot::TestFullyMissedReceiveAt();
        ot::TestInterruptedReceiveAt();
        ot::TestInterruptedActiveReceiveAt();
        ot::TestInterruptedReceiveAtWithSecondSchedule();
    }

    printf("All tests passed!\n");
#else
    printf("Timed RX feature is not enabled\n");
#endif

    return 0;
}
