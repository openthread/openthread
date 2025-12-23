/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions for the radio sample scheduler.
 */

#ifndef RADIO_SAMPLE_SCHEDULER_HPP_
#define RADIO_SAMPLE_SCHEDULER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "radio/radio.hpp"

namespace ot {

/**
 * @addtogroup core-mac
 *
 * @brief
 *   This module includes definitions for the radio sample scheduler.
 *
 * @{
 */

namespace Mac {

class RadioSampleScheduler : public InstanceLocator, private NonCopyable
{
    friend class SubMac;

public:
    explicit RadioSampleScheduler(Instance &aInstance);

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    /**
     * Starts the CSL sampling scheduler.
     *
     * @param[in]  aChannel      The channel to listen.
     * @param[in]  aDuration     The duration of the listen window, in microseconds.
     * @param[in]  aPeriod       The period of listining window, in microseconds.
     */
    void StartCslSample(uint8_t aChannel, uint32_t aDuration, uint32_t aPeriod)
    {
        Start(kCslSchedulerId, aChannel, aDuration, aPeriod);
    }

    /**
     * Stops the CSL sampling scheduler.
     */
    void StopCslSample(void) { Stop(kCslSchedulerId); }
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    /**
     * Starts the WED sampling scheduler.
     *
     * @param[in]  aChannel      The channel to listen.
     * @param[in]  aDuration     The duration of the listen window, in microseconds.
     * @param[in]  aPeriod       The period of listining window, in microseconds.
     */
    void StartWedSample(uint8_t aChannel, uint32_t aDuration, uint32_t aPeriod)
    {
        Start(kWedSchedulerId, aChannel, aDuration, aPeriod);
    }

    /**
     * Stops the WED sampling scheduler.
     */
    void StopWedSample(void) { Stop(kWedSchedulerId); }
#endif

#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
    /**
     * Starts the Enhanced CSL sampling scheduler.
     *
     * @param[in]  aChannel      The channel to listen.
     * @param[in]  aDuration     The duration of the listen window, in microseconds.
     * @param[in]  aPeriod       The period of listining window, in microseconds.
     */
    void StartECslSample(uint8_t aChannel, uint32_t aDuration, uint32_t aPeriod)
    {
        Start(kECslSchedulerId, aChannel, aDuration, aPeriod);
    }

    /**
     * Stops the Enhanced CSL sampling scheduler.
     */
    void StopECslSample(void) { Stop(kECslSchedulerId); }
#endif

    /**
     * Stops the all sampling schedulers.
     */
    void Stop(void);

    /**
     * Indicate whether any sampling shceduler has been enabled.
     *
     * @retval TRUE    At least one sampling scheduler has been enabled.
     * @retval FALSE   No sampling scheduler was enabled.
     */
    bool IsRadioSampleEnabled(void) const;

private:
    enum SchedulerId : uint8_t
    {
#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
        kECslSchedulerId,
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        kCslSchedulerId,
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
        kWedSchedulerId,
#endif
        kNumSchedulers,
    };

    struct Scheduler
    {
        static constexpr uint16_t       kInfoStringSize = 250;
        typedef String<kInfoStringSize> InfoString;

        InfoString ToString(void) const;

        TimeMicro mLocalStart;    // For debugging. TODO: remove it after the eCSL is supported.
        uint64_t  mRadioStart;    // For debugging. TODO: remove it after the eCSL is supported.
        TimeMicro mFireTime;      // The fire time of the next event, in microseconds.
        TimeMicro mLocalTime;     // The sample time of the current interval in local time, in microseconds.
        uint64_t  mRadioTime;     // The sample time of the current interval in radio time, in microseconds.
        uint32_t  mDuration;      // The duration of the listen window , in microseconds.
        uint32_t  mPeriod;        // The listen period, in microseconds.
        uint8_t   mChannel : 5;   // The listen channel.
        bool      mIsEnabled : 1; // Indicates whether the scheduler is enabled.
        bool mIsSampling : 1; // Indicates that the radio is requested to enter receive state using the sample channel
                              // for platforms not supporting `Radio::ReceiveAt()`.
    };

    void     UpdateTimer(void);
    void     HandleTimer(void);
    void     HandleRadioSample(Scheduler &aScheduler, uint32_t aTimeAhead, uint32_t aTimeAfter);
    void     HandleReceiveAt(Scheduler &aScheduler, uint32_t aTimeAhead, uint32_t aTimeAfter);
    void     HandleReceiveOrSleep(Scheduler &aScheduler, uint32_t aTimeAhead, uint32_t aTimeAfter);
    void     Start(SchedulerId aSchedulerId, uint8_t aChannel, uint32_t aDuration, uint32_t aPeriod);
    void     Stop(SchedulerId aSchedulerId);
    void     Stop(Scheduler &aScheduler);
    void     GetWindowEdges(const Scheduler &aScheduler, uint32_t &aAhead, uint32_t &aAfter);
    void     LogWindow(uint32_t aWinStart, uint32_t aWinDuration, uint8_t aChannel);
    uint32_t GetLocalTime(void) const;
    void     UpdateRadioSampleState(void);
    void     UpdateSampleTime(Scheduler &aScheduler);

    SchedulerId GetSchedulerId(const Scheduler &aScheduler) const;
    const char *GetSchedulerName(const Scheduler &aScheduler) const;

#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
    const Scheduler &GetECslScheduler(void) const { return mSchedulers[kECslSchedulerId]; }
    bool IsECslScheduler(const Scheduler &aScheduler) const { return GetSchedulerId(aScheduler) == kECslSchedulerId; }
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    const Scheduler &GetCslScheduler(void) const { return mSchedulers[kCslSchedulerId]; }
    bool IsCslScheduler(const Scheduler &aScheduler) const { return GetSchedulerId(aScheduler) == kCslSchedulerId; }
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    const Scheduler &GetWedScheduler(void) const { return mSchedulers[kWedSchedulerId]; }
    bool IsWedScheduler(const Scheduler &aScheduler) const { return GetSchedulerId(aScheduler) == kWedSchedulerId; }
#endif

    using SampleTimer = TimerMicroIn<RadioSampleScheduler, &RadioSampleScheduler::HandleTimer>;

    SampleTimer mTimer;
    Scheduler   mSchedulers[kNumSchedulers];
};
/**
 * @}
 */

} // namespace Mac
} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
#endif // RADIO_SAMPLE_SCHEDULER_HPP_
