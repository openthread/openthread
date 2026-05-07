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
#include "common/time_ticker.hpp"
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
    friend class ot::TimeTicker;

public:
    typedef otIp6RegisterMulticastListenersCallback RegisterCallback;

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Manager(Instance &aInstance);

    /**
     * Notifies Primary Backbone Router status.
     *
     * @param[in]  aState   The state or state change of Primary Backbone Router.
     * @param[in]  aConfig  The Primary Backbone Router service.
     */
    void HandleBackboneRouterPrimaryUpdate(BackboneRouter::Leader::State aState, const BackboneRouter::Config &aConfig);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    static constexpr uint16_t kMaxChildAddresses = OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD - 1; ///< Max MLR addresses

    typedef Array<Ip6::Address, kMaxChildAddresses> ChildAddressArray; ///< Registered MLR addresses array.

    /**
     * Updates the Multicast Subscription Table according to the Child information.
     *
     * @param[in]  aChild                       A reference to the child information.
     * @param[in]  aOldRegisteredAddresses   Array of the Child's previously registered IPv6 addresses.
     */
    void UpdateProxiedSubscriptions(Child &aChild, const ChildAddressArray &aOldRegisteredAddresses);
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
    static constexpr uint32_t kLongRenewTimeout = 4 * Time::kOneHourInSec; // `MLR_TIMEOUT_LONG` (in sec)
    static constexpr uint32_t kRenewGuardTime   = 9;                       // (in sec).

    enum RegistrationRequest : uint8_t
    {
        kReregister,
        kRenew,
    };

    class AddressArray : public Array<Ip6::Address, kMaxIp6Addresses>
    {
    public:
        Error AddUnique(const Ip6::Address &aAddress);
    };

    void  HandleNotifierEvents(Events aEvents);
    bool  ShouldRegister(void) const;
    void  Send(void);
    Error SendMessage(const Ip6::Address   *aAddresses,
                      uint8_t               aAddressNum,
                      const uint32_t       *aTimeout,
                      Coap::ResponseHandler aResponseHandler);

    DeclareTmfResponseHandlerIn(Manager, HandleResponse);

    static uint16_t DetermineReregistrationDelay(const BackboneRouter::Config &aConfig);
    static uint32_t DetermineRenewDelay(const BackboneRouter::Config &aConfig);

    static Error ParseResponse(Error aResult, Coap::Msg *aMsg, uint8_t &aStatus, AddressArray &aFailedAddresses);
    static bool  DidRegisterSuccessfully(const Ip6::Address &aAddress,
                                         bool                aSuccess,
                                         const AddressArray &aFailedAddresses);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    DeclareTmfResponseHandlerIn(Manager, HandleRegisterResponse);
#endif

#if OPENTHREAD_CONFIG_MLR_ENABLE
    void UpdateLocalSubscriptions(void);
    bool IsAddressRegisteredByNetif(const Ip6::Address &aAddress) const;
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    bool IsAddressRegisteredByAnyChild(const Ip6::Address &aAddress) const
    {
        return IsAddressRegisteredByAnyChildExcept(aAddress, nullptr);
    }

    bool IsAddressRegisteredByAnyChildExcept(const Ip6::Address &aAddress, const Child *aExceptChild) const;
#endif

    void SetMulticastAddressState(State aFromState, State aToState);
    void Finish(bool aSuccess, const AddressArray &aFailedAddresses);
    void ScheduleSend(uint16_t aDelay);
    void UpdateTimeTickerRegistration(void);
    void ScheduleNextRegistration(RegistrationRequest aRequest);
    void Reregister(void);
    void HandleTimeTick(void);
    void LogMulticastAddresses(void);

#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE) && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    Callback<RegisterCallback> mRegisterCallback;
#endif

    uint32_t mReregistrationDelay;
    uint16_t mSendDelay;

    bool mPending : 1;
#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE) && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    bool mRegisterPending : 1;
#endif
};

} // namespace Mlr
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
#endif // OT_CORE_THREAD_MLR_MANAGER_HPP_
