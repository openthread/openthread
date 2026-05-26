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
 *   This file includes definitions for managing Domain Unicast Address feature defined in Thread 1.2.
 */

#ifndef OT_CORE_THREAD_DUA_MANAGER_HPP_
#define OT_CORE_THREAD_DUA_MANAGER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

#include "backbone_router/bbr_leader.hpp"
#include "coap/coap_message.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/tasklet.hpp"
#include "common/time.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "net/netif.hpp"
#include "thread/child.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/tmf.hpp"

namespace ot {

/**
 * @addtogroup core-dua
 *
 * @brief
 *   This module includes definitions for generating, managing, registering Domain Unicast Address.
 *
 * @{
 *
 * @defgroup core-dua Dua
 *
 * @}
 */

/**
 * Domain Unicast Address (DUA) Registration Status values
 */
enum DuaStatus : uint8_t
{
    kDuaSuccess        = 0, ///< Successful registration.
    kDuaReRegister     = 1, ///< Registration was accepted but immediate reregistration is required to solve.
    kDuaInvalid        = 2, ///< Registration rejected (Fatal): Target EID is not a valid DUA.
    kDuaDuplicate      = 3, ///< Registration rejected (Fatal): DUA is already in use by another device.
    kDuaNoResources    = 4, ///< Registration rejected (Non-fatal): Backbone Router Resource shortage.
    kDuaNotPrimary     = 5, ///< Registration rejected (Non-fatal): Backbone Router is not primary at this moment.
    kDuaGeneralFailure = 6, ///< Registration failure (Non-fatal): Reason(s) not further specified.
};

/**
 * Implements managing DUA.
 */
class DuaManager : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class ot::TimeTicker;
    friend class Tmf::Agent;

public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit DuaManager(Instance &aInstance);

    /**
     * Notifies Domain Prefix changes.
     *
     * @param[in]  aEvent  The Domain Prefix event.
     */
    void HandleDomainPrefixUpdate(BackboneRouter::DomainPrefixEvent aEvent);

    /**
     * Notifies the `DuaManager` of a Primary Backbone Router event.
     *
     * @param[in]  aEvent   The Primary Backbone Router event.
     */
    void HandleBackboneRouterPrimaryUpdate(BackboneRouter::PrimaryEvent aEvent);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    /**
     * Events related to a Child DUA address.
     */
    enum ChildDuaAddressEvent : uint8_t
    {
        kAddressAdded,     ///< A new DUA registered by the Child via Address Registration.
        kAddressChanged,   ///< A different DUA registered by the Child via Address Registration.
        kAddressRemoved,   ///< DUA registered by the Child is removed and not in Address Registration.
        kAddressUnchanged, ///< The Child registers the same DUA again.
    };

    /**
     * Handles Child DUA address event.
     *
     * @param[in] aChild   A child.
     * @param[in] aEvent   The DUA address event for @p aChild.
     */
    void HandleChildDuaAddressEvent(const Child &aChild, ChildDuaAddressEvent aEvent);
#endif

private:
    static constexpr uint8_t kNoBufDelay           = 5;  // In sec.
    static constexpr uint8_t KResponseTimeoutDelay = 30; // In sec.

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    void SendAddressNotification(Ip6::Address &aAddress, DuaStatus aStatus, const Child &aChild);
#endif

    void HandleNotifierEvents(Events aEvents);

    void HandleTimeTick(void);

    void UpdateTimeTickerRegistration(void);

    DeclareTmfResponseHandlerIn(DuaManager, HandleDuaResponse);

    template <Uri kUri> void HandleTmf(Coap::Msg &aMsg);

    Error ProcessDuaResponse(Coap::Message &aMessage);

    void PerformNextRegistration(void);
    void UpdateReregistrationDelay(void);
    void UpdateCheckDelay(uint8_t aDelay);

    using RegistrationTask = TaskletIn<DuaManager, &DuaManager::PerformNextRegistration>;

    RegistrationTask mRegistrationTask;
    Ip6::Address     mRegisteringDua;
    bool             mIsDuaPending : 1;

    union
    {
        struct
        {
            uint16_t mReregistrationDelay; // Delay (in seconds) for DUA re-registration.
            uint8_t  mCheckDelay;          // Delay (in seconds) for checking whether or not registration is required.
        } mFields;
        uint32_t mValue; // Non-zero indicates timer should start.
    } mDelay;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    // TODO: (DUA) may re-evaluate the alternative option of distributing the flags into the child table:
    //       - Child class itself have some padding - may save some RAM
    //       - Avoid cross reference between a bit-vector and the child entry
    ChildMask mChildDuaMask;             // Child Mask for child who registers DUA via Child Update Request.
    ChildMask mChildDuaRegisteredMask;   // Child Mask for child's DUA that was registered by the parent on behalf.
    uint16_t  mChildIndexDuaRegistering; // Child Index of the DUA being registered.
#endif
};

DeclareTmfHandler(DuaManager, kUriDuaRegistrationNotify);

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
#endif // OT_CORE_THREAD_DUA_MANAGER_HPP_
