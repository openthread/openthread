/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for managing Multicast Listener Registration feature defined in Thread 1.2.
 */

#ifndef OT_CORE_THREAD_MLR_MANAGER_HPP_
#define OT_CORE_THREAD_MLR_MANAGER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)

#if OPENTHREAD_CONFIG_MLR_ENABLE && (OPENTHREAD_CONFIG_THREAD_VERSION < OT_THREAD_VERSION_1_2)
#error "Thread 1.2 or higher version is required for OPENTHREAD_CONFIG_MLR_ENABLE"
#endif

#include "backbone_router/bbr_leader.hpp"
#include "coap/coap_message.hpp"
#include "common/array.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "net/netif.hpp"
#include "thread/child.hpp"
#include "thread/mlr_types.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace Mlr {

/**
 * @addtogroup core-mlr
 *
 * @brief
 *   This module includes definitions for Multicast Listener Registration.
 *
 * @{
 *
 * @defgroup core-mlr Mlr
 *
 * @}
 */

/**
 * Implements MLR management.
 */
class Manager : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    typedef otIp6RegisterMulticastListenersCallback RegisterCallback;

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Manager(Instance &aInstance);

    /**
     * Notifies the `MlrManager` of a Primary Backbone Router event.
     *
     * @param[in]  aEvent   The Primary Backbone Router event.
     */
    void HandleBackboneRouterPrimaryUpdate(BackboneRouter::PrimaryEvent aEvent);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    /**
     * Updates the MLR registration status of a given child's addresses.
     *
     * @param[in]  aChild  The child to update.
     */
    void UpdateChildRegistrations(Child &aChild);

    /**
     * Updates the MLR registration status of a given child's addresses.
     *
     * @param[in]  aChild                    The child to update.
     * @param[in]  aOldRegisteredAddresses   Child's previously registered addresses.
     */
    void UpdateChildRegistrations(Child &aChild, const Child::Ip6AddressArray &aOldRegisteredAddresses);
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    /**
     * Registers Multicast Listeners to Primary Backbone Router.
     *
     * Note: only available when both `(OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)` and
     * `OPENTHREAD_CONFIG_COMMISSIONER_ENABLE` are enabled)
     *
     * @param aAddresses   A pointer to IPv6 multicast addresses to register.
     * @param aAddressNum  The number of IPv6 multicast addresses.
     * @param aTimeout     A pointer to the timeout (in seconds), or `nullptr` to use the default MLR timeout.
     *                     A timeout of 0 seconds removes the Multicast Listener addresses.
     * @param aCallback    A callback function.
     * @param aContext     A user context pointer.
     *
     * @retval kErrorNone          Successfully sent MLR.req. The @p aCallback will be called iff this method
     *                             returns kErrorNone.
     * @retval kErrorBusy          If a previous registration was ongoing.
     * @retval kErrorInvalidArgs   If one or more arguments are invalid.
     * @retval kErrorInvalidState  If the device was not in a valid state to send MLR.req (e.g. Commissioner not
     *                             started, Primary Backbone Router not found).
     * @retval kErrorNoBufs        If insufficient message buffers available.
     */
    Error RegisterMulticastListeners(const Ip6::Address *aAddresses,
                                     uint8_t             aAddressNum,
                                     const uint32_t     *aTimeout,
                                     RegisterCallback    aCallback,
                                     void               *aContext);
#endif

private:
    // Delays (in msec) applied before registration attempts when new
    // `Netif` or child multicast addresses are added. The longer
    // delay for child addresses allows the parent to aggregate
    // multiple address updates into a single MLR request.
    static constexpr uint32_t kMaxNewNetifAddrRegistraionDelay  = 100;
    static constexpr uint32_t kMinNewChildAddrRegistrationDelay = 750;
    static constexpr uint32_t kMaxNewChildAddrRegistrationDelay = 5000;

    static constexpr uint32_t kLongRenewTimeout      = 4 * Time::kOneHourInSec;    // `MLR_TIMEOUT_LONG` (in sec)
    static constexpr uint32_t kRenewGuardTimeInMsec  = 9 * Time::kOneSecondInMsec; // (in msec)
    static constexpr uint32_t kSendFailureRetryDelay = 1000;                       // (in msec)

    enum State : uint8_t
    {
        kStateStopped,           // Manager is stopped (e.g., no PBBR).
        kStateIdle,              // Started but has no multicast addresses to register.
        kStateToRegisterAll,     // Waiting to register (or re-register) all multicast addresses.
        kStateRegistering,       // MLR.req is sent, waiting for MLR.rsp, or waiting to retry.
        kStateRegistered,        // All addresses are registered, waiting for periodic renewal.
        kStateNewAddrToRegister, // All were registered, but new addresses are pending registration.
    };

    State GetState(void) const { return mState; }
    bool  IsRunning(void) const { return mState != kStateStopped; }
    void  EnterState(State aState);
    void  UpdateState(void);
    void  HandleNotifierEvents(Events aEvents);
    void  DetermineAddressesToRegister(AddressArray &aAddresses) const;
    void  SendNextRequest(void);
    void  DetermineRenewTime(void);
    void  ScheduleTimerForReregistrationDelay(void);
    void  ScheduleNewAddrRegistration(uint32_t aMinDelay, uint32_t aMaxDelay);
    void  ProcessResponse(Coap::Msg *aMsg, Error aResult);
    void  HandleTimer(void);
    Error SendMessage(const Ip6::Address   *aAddresses,
                      uint8_t               aAddressNum,
                      const uint32_t       *aTimeout,
                      Coap::ResponseHandler aResponseHandler);

    DeclareTmfResponseHandlerIn(Manager, HandleResponse);

    static Error ParseResponse(Error aResult, Coap::Msg *aMsg, uint8_t &aStatus, AddressArray &aFailedAddresses);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    DeclareTmfResponseHandlerIn(Manager, HandleRegisterResponse);
#endif

#if OPENTHREAD_CONFIG_MLR_ENABLE
    void HandleEventIp6MulticastSubscribed(void);
    bool IsAddressRegisteredByNetif(const Ip6::Address &aAddress) const;
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    bool IsAddressRegisteredByAnyChild(const Ip6::Address &aAddress) const;
    bool IsAddressRegisteredByAnyChildExcept(const Ip6::Address &aAddress, const Child *aExceptChild) const;
#endif

    using DelayTimer = TimerMilliIn<Manager, &Manager::HandleTimer>;

#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE) && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    Callback<RegisterCallback> mRegisterCallback;
    bool                       mRegisterPending;
#endif
    State      mState;
    TimeMilli  mStartTime;
    TimeMilli  mRenewTime;
    DelayTimer mTimer;
};

} // namespace Mlr
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
#endif // OT_CORE_THREAD_MLR_MANAGER_HPP_
