/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes definitions for child supervision feature.
 */

#ifndef CHILD_SUPERVISION_HPP_
#define CHILD_SUPERVISION_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/child_supervision.h>

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "thread/child.hpp"

namespace ot {

class ThreadNetif;

/**
 *
 * Child supervision feature provides a mechanism for parent
 * to ensure that a message is sent to each sleepy child within
 * a fixed interval, namely the supervision interval. If there
 * is no transmission to the child within the supervision
 * interval, child supervisor enqueues and sends a supervision
 * message (a data message with empty payload) to the child.
 *
 * On the child side, this is used to check the connectivity
 * to the parent. If the child does not hear from its parent
 * for a pre-specified timeout interval it assumes that it may
 * be disconnected and tries to re-attach to the parent.
 *
 * The child supervision provides an alternative, more
 * energy-efficient solution compared to requiring the sleepy child
 * to periodically perform an MLE Child Update Request/Response
 * exchange with the parent (as a way of verifying that it
 * is still connected to the parent). The child supervision
 * solution puts the burden of message transmissions on the
 * parent instead of the typically more energy-constrained child.
 *
 * Note that most radios generate an auto-ack in hardware in
 * response to a received frame, so the child cannot solely rely
 * on the 15.4 acknowledgments it receives from parent as an
 * indicator that it is still connected and is in parent's
 * child table.
 */

#if OPENTHREAD_FTD

/**
 * Implements a child supervisor.
 */
class ChildSupervisor : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class ot::TimeTicker;

public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit ChildSupervisor(Instance &aInstance);

    /**
     * Returns the destination for a supervision message.
     *
     * @param[in] aMessage The message for which to get the destination.
     *
     * @returns  A pointer to the destination child of the message, or `nullptr` if @p aMessage is not of supervision
     *           type.
     */
    Child *GetDestination(const Message &aMessage) const;

    /**
     * Updates the supervision state for a child. It informs the child supervisor that a message was
     * successfully sent to the child.
     *
     * @param[in] aChild     The child to which a message was successfully sent.
     */
    void UpdateOnSend(Child &aChild);

private:
    static constexpr uint16_t kDefaultSupervisionInterval = OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL; // (seconds)

    void SendMessage(Child &aChild);
    void CheckState(void);
    void HandleTimeTick(void);
    void HandleNotifierEvents(Events aEvents);
};

#endif // #if OPENTHREAD_FTD

/**
 * Implements a child supervision listener.
 */
class SupervisionListener : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit SupervisionListener(Instance &aInstance);

    /**
     * Starts the supervision listener operation.
     */
    void Start(void);

    /**
     * Stops the supervision listener operation.
     */
    void Stop(void);

    /**
     * Sets the supervision interval.
     *
     * @param[in] aInterval If non-zero, the desired supervision interval (in seconds), zero to disable supervision.
     */
    void SetInterval(uint16_t aInterval);

    /**
     * Returns the supervision interval.
     *
     * @returns  The current supervision interval (seconds), or zero if supervision is disabled.
     */
    uint16_t GetInterval(void) const { return mInterval; }

    /**
     * Sets the supervision check timeout (in seconds).
     *
     * If the child does not hear from its parent within the given check timeout interval, it initiates the re-attach
     * process (MLE Child Update Request/Response exchange with its parent). Setting the timeout to zero, disables the
     * supervision check on the child.
     *
     * It is recommended to select a supervision check timeout value larger than the parent's child supervision
     * interval plus the maximum time between the child's data poll transmissions.
     *
     * @param[in]  aTimeout   The timeout interval (in seconds), zero to disable the supervision check on the child.
     */
    void SetTimeout(uint16_t aTimeout);

    /**
     * Returns the supervision check timeout interval (in seconds).
     *
     * @returns   The check timeout interval (in seconds) or zero if the supervision check on the child is disabled.
     */
    uint16_t GetTimeout(void) const { return mTimeout; }

    /**
     * Returns the value of supervision check timeout failure counter.
     *
     * The counter tracks the number of supervision check failures on the child. It is incremented when the child does
     * not hear from its parent within the specified check timeout interval.
     */
    uint16_t GetCounter(void) const { return mCounter; }

    /**
     * Reset the supervision check timeout failure counter.
     */
    void ResetCounter(void) { mCounter = 0; }

    /**
     * Updates the supervision listener state. It informs the listener of a received frame.
     *
     * @param[in]   aSourceAddress    The source MAC address of the received frame
     * @param[in]   aIsSecure         TRUE to indicate that the received frame is secure, FALSE otherwise.
     */
    void UpdateOnReceive(const Mac::Address &aSourceAddress, bool aIsSecure);

private:
    static constexpr uint16_t kDefaultTimeout  = OPENTHREAD_CONFIG_CHILD_SUPERVISION_CHECK_TIMEOUT; // (seconds)
    static constexpr uint16_t kDefaultInterval = OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL;      // (seconds)

    void RestartTimer(void);
    void HandleTimer(void);

    using ListenerTimer = TimerMilliIn<SupervisionListener, &SupervisionListener::HandleTimer>;

    uint16_t      mTimeout;
    uint16_t      mInterval;
    uint16_t      mCounter;
    ListenerTimer mTimer;
};

} // namespace ot

#endif // CHILD_SUPERVISION_HPP_
