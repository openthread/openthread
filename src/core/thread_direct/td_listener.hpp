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

/**
 * @file
 *   This file includes definitions for Thread Direct Listener.
 */

#ifndef OT_CORE_THREAD_DIRECT_TD_LISTENER_HPP_
#define OT_CORE_THREAD_DIRECT_TD_LISTENER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE

#include <openthread/thread_direct_listener.h>

#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "mac/sub_mac.hpp"
#include "radio/radio.hpp"
#include "radio/radio_types.hpp"
#include "thread_direct/td_types.hpp"

//----------------------------------------------------------------------------------------------------------------------

namespace ot {
namespace ThreadDirect {

/**
 * Implements the Thread Direct Wake Listener.
 */
class Listener : public InstanceLocator, private NonCopyable
{
    friend class ot::Mac::SubMac;

public:
    /**
     * Pointer to a callback function invoked when a valid Thread Direct Wake frame is received.
     */
    typedef otThreadDirectListenerWakeCallback WakeCallback;

    /**
     * Represents the configuration parameters for starting the `Listener`.
     */
    class Config : public otThreadDirectListenerConfig, public Clearable<Config>
    {
    public:
        static constexpr uint8_t  kDefaultChannel  = OT_THREAD_DIRECT_LISTENER_CHANNEL;  ///< Default listen channel.
        static constexpr uint32_t kDefaultInterval = OT_THREAD_DIRECT_LISTENER_INTERVAL; ///< Default interval (usec).
        static constexpr uint32_t kDefaultDuration = OT_THREAD_DIRECT_LISTENER_DURATION; ///< Default duration (usec).

        /**
         * Gets the effective radio channel (returning `kDefaultChannel` if `mChannel` is `0`).
         *
         * @returns The effective radio channel.
         */
        uint8_t GetChannel(void) const { return (mChannel == 0) ? kDefaultChannel : mChannel; }

        /**
         * Gets the effective listen duration in microseconds (returning `kDefaultDuration` if `mDuration` is `0`).
         *
         * @returns The effective listen duration in microseconds.
         */
        uint32_t GetDuration(void) const { return (mDuration == 0) ? kDefaultDuration : mDuration; }

        /**
         * Gets the effective listen interval in microseconds (returning `kDefaultInterval` if `mInterval` is `0`).
         *
         * @returns The effective listen interval in microseconds.
         */
        uint32_t GetInterval(void) const { return (mInterval == 0) ? kDefaultInterval : mInterval; }

        /**
         * Indicates whether the configuration parameters are valid.
         *
         * @retval TRUE   The configuration parameters are valid.
         * @retval FALSE  The configuration parameters are invalid.
         */
        bool IsValid(void) const;
    };

    /**
     * Represents information about a received Thread Direct Wake frame.
     */
    class WakeInfo : public otThreadDirectListenerWakeInfo, public Clearable<WakeInfo>
    {
    public:
        /**
         * Indicates whether the `WakeInfo` contains valid Wake frame information.
         *
         * @retval TRUE   The `WakeInfo` is valid.
         * @retval FALSE  The `WakeInfo` is not valid (empty/cleared).
         */
        bool IsValid(void) const { return mKey != nullptr; }

        /**
         * Gets the Extended MAC Address of the Wake Initiator peer.
         *
         * @returns The Extended MAC Address of the peer.
         */
        const Mac::ExtAddress &GetPeerAddr(void) const { return AsCoreType(&mPeerAddress); }

        /**
         * Gets the registered Thread Direct `Key` that authenticated the Wake frame.
         *
         * @returns A pointer to the `Key` that authenticated the Wake frame, or `nullptr` if not valid.
         */
        Key *GetKey(void) { return AsCoreTypePtr(mKey); }
    };

    /**
     * Initializes the `Listener` object.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    explicit Listener(Instance &aInstance);

    /**
     * Starts the Thread Direct Wake Listener with the given configuration.
     *
     * @param[in] aConfig  The listener configuration.
     *
     * @retval kErrorNone         Successfully started the listener.
     * @retval kErrorInvalidArgs  The configuration parameters in @p aConfig are invalid.
     * @retval kErrorBusy         The listener is already running.
     */
    Error Start(const Config &aConfig);

    /**
     * Stops the Thread Direct Wake Listener if it is currently running.
     */
    void Stop(void);

    /**
     * Indicates whether the Thread Direct Wake Listener is currently running.
     *
     * @retval TRUE   The listener is currently running.
     * @retval FALSE  The listener is not running.
     */
    bool IsRunning(void) const { return mState == kRunning; }

    /**
     * Gets the information for the most recently received Wake frame.
     *
     * @param[out] aWakeInfo  A reference to a `WakeInfo` structure to populate.
     *
     * @retval kErrorNone          Successfully retrieved the Wake frame information in @p aWakeInfo.
     * @retval kErrorInvalidState  The listener is not running.
     * @retval kErrorNotFound      No Wake frame has been received yet.
     */
    Error GetWakeInfo(WakeInfo &aWakeInfo) const;

private:
    typedef Mac::SubMac::TimedRx TimedRx;

    enum State : uint8_t
    {
        kStopped,
        kRunning,
    };

    const TimedRx &DetermineCurWindow(const Radio::SyncedTime &aNow);
    bool           ProcessRxFrame(Mac::RxFrame::ParseInfo &aFrameInfo);

    uint32_t               mInterval;
    TimedRx                mTimedRx;
    Callback<WakeCallback> mWakeCallback;
    State                  mState;
};

} // namespace ThreadDirect

DefineCoreType(otThreadDirectListenerConfig, ThreadDirect::Listener::Config);
DefineCoreType(otThreadDirectListenerWakeInfo, ThreadDirect::Listener::WakeInfo);

} // namespace ot

#endif // OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE

#endif // OT_CORE_THREAD_DIRECT_TD_LISTENER_HPP_
