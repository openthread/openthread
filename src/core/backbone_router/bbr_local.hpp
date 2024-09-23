/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for local Backbone Router service.
 */

#ifndef BACKBONE_ROUTER_LOCAL_HPP_
#define BACKBONE_ROUTER_LOCAL_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#if (OPENTHREAD_CONFIG_THREAD_VERSION < OT_THREAD_VERSION_1_2)
#error "Thread 1.2 or higher version is required for OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE."
#endif

#if !OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#error "OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE is required for OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE."
#endif

#if !OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
#error "OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE is required for OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE."
#endif

#include <openthread/backbone_router.h>
#include <openthread/backbone_router_ftd.h>

#include "backbone_router/bbr_leader.hpp"
#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/time_ticker.hpp"
#include "net/netif.hpp"
#include "thread/network_data.hpp"

namespace ot {

namespace BackboneRouter {

/**
 * Implements the definitions for local Backbone Router service.
 */
class Local : public InstanceLocator, private NonCopyable
{
    friend class ot::TimeTicker;

public:
    typedef otBackboneRouterDomainPrefixCallback DomainPrefixCallback; ///< Domain Prefix callback.

    /**
     * Represents Backbone Router state.
     */
    enum State : uint8_t
    {
        kStateDisabled  = OT_BACKBONE_ROUTER_STATE_DISABLED,  ///< Backbone function is disabled.
        kStateSecondary = OT_BACKBONE_ROUTER_STATE_SECONDARY, ///< Secondary Backbone Router.
        kStatePrimary   = OT_BACKBONE_ROUTER_STATE_PRIMARY,   ///< The Primary Backbone Router.
    };

    /**
     * Represents registration mode used as input to `AddService()` method.
     */
    enum RegisterMode : uint8_t
    {
        kDecideBasedOnState, ///< Decide based on current state.
        kForceRegistration,  ///< Force registration regardless of current state.
    };

    /**
     * Initializes the local Backbone Router.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    explicit Local(Instance &aInstance);

    /**
     * Enables/disables Backbone function.
     *
     * @param[in]  aEnable  TRUE to enable the backbone function, FALSE otherwise.
     */
    void SetEnabled(bool aEnable);

    /**
     * Retrieves the Backbone Router state.
     *
     *
     * @returns The current state of Backbone Router.
     */
    State GetState(void) const { return mState; }

    /**
     * Resets the local Thread Network Data.
     */
    void Reset(void);

    /**
     * Gets local Backbone Router configuration.
     *
     * @param[out]  aConfig  The local Backbone Router configuration.
     */
    void GetConfig(Config &aConfig) const;

    /**
     * Sets local Backbone Router configuration.
     *
     * @param[in]  aConfig  The configuration to set.
     *
     * @retval kErrorNone         Successfully updated configuration.
     * @retval kErrorInvalidArgs  The configuration in @p aConfig is invalid.
     */
    Error SetConfig(const Config &aConfig);

    /**
     * Registers Backbone Router Dataset to Leader.
     *
     * @param[in]  aMode  The registration mode to use (decide based on current state or force registration).
     *
     * @retval kErrorNone            Successfully added the Service entry.
     * @retval kErrorInvalidState    Not in the ready state to register.
     * @retval kErrorNoBufs          Insufficient space to add the Service entry.
     */
    Error AddService(RegisterMode aMode);

    /**
     * Indicates whether or not the Backbone Router is Primary.
     *
     * @retval  True  if the Backbone Router is Primary.
     * @retval  False if the Backbone Router is not Primary.
     */
    bool IsPrimary(void) const { return mState == kStatePrimary; }

    /**
     * Indicates whether or not the Backbone Router is enabled.
     *
     * @retval  True  if the Backbone Router is enabled.
     * @retval  False if the Backbone Router is not enabled.
     */
    bool IsEnabled(void) const { return mState != kStateDisabled; }

    /**
     * Sets the Backbone Router registration jitter value.
     *
     * @param[in]  aRegistrationJitter the Backbone Router registration jitter value to set.
     */
    void SetRegistrationJitter(uint8_t aRegistrationJitter) { mRegistrationJitter = aRegistrationJitter; }

    /**
     * Returns the Backbone Router registration jitter value.
     *
     * @returns The Backbone Router registration jitter value.
     */
    uint8_t GetRegistrationJitter(void) const { return mRegistrationJitter; }

    /**
     * Notifies Primary Backbone Router status.
     *
     * @param[in]  aState   The state or state change of Primary Backbone Router.
     * @param[in]  aConfig  The Primary Backbone Router service.
     */
    void HandleBackboneRouterPrimaryUpdate(Leader::State aState, const Config &aConfig);

    /**
     * Gets the Domain Prefix configuration.
     *
     * @param[out]  aConfig  A reference to the Domain Prefix configuration.
     *
     * @retval kErrorNone      Successfully got the Domain Prefix configuration.
     * @retval kErrorNotFound  No Domain Prefix was configured.
     */
    Error GetDomainPrefix(NetworkData::OnMeshPrefixConfig &aConfig);

    /**
     * Removes the local Domain Prefix configuration.
     *
     * @param[in]  aPrefix A reference to the IPv6 Domain Prefix.
     *
     * @retval kErrorNone         Successfully removed the Domain Prefix.
     * @retval kErrorInvalidArgs  @p aPrefix is invalid.
     * @retval kErrorNotFound     No Domain Prefix was configured or @p aPrefix doesn't match.
     */
    Error RemoveDomainPrefix(const Ip6::Prefix &aPrefix);

    /**
     * Sets the local Domain Prefix configuration.
     *
     * @param[in]  aConfig A reference to the Domain Prefix configuration.
     *
     * @returns kErrorNone          Successfully set the local Domain Prefix.
     * @returns kErrorInvalidArgs   @p aConfig is invalid.
     */
    Error SetDomainPrefix(const NetworkData::OnMeshPrefixConfig &aConfig);

    /**
     * Returns a reference to the All Network Backbone Routers Multicast Address.
     *
     * @returns A reference to the All Network Backbone Routers Multicast Address.
     */
    const Ip6::Address &GetAllNetworkBackboneRoutersAddress(void) const { return mAllNetworkBackboneRouters; }

    /**
     * Returns a reference to the All Domain Backbone Routers Multicast Address.
     *
     * @returns A reference to the All Domain Backbone Routers Multicast Address.
     */
    const Ip6::Address &GetAllDomainBackboneRoutersAddress(void) const { return mAllDomainBackboneRouters; }

    /**
     * Applies the Mesh Local Prefix.
     */
    void ApplyNewMeshLocalPrefix(void);

    /**
     * Updates the subscription of All Domain Backbone Routers Multicast Address.
     *
     * @param[in]  aEvent  The Domain Prefix event.
     */
    void HandleDomainPrefixUpdate(DomainPrefixEvent aEvent);

    /**
     * Sets the Domain Prefix callback.
     *
     * @param[in] aCallback  The callback function.
     * @param[in] aContext   A user context pointer.
     */
    void SetDomainPrefixCallback(DomainPrefixCallback aCallback, void *aContext)
    {
        mDomainPrefixCallback.Set(aCallback, aContext);
    }

private:
    enum Action : uint8_t
    {
        kActionSet,
        kActionAdd,
        kActionRemove,
    };

    void SetState(State aState);
    void RemoveService(void);
    void HandleTimeTick(void);
    void AddDomainPrefixToNetworkData(void);
    void RemoveDomainPrefixFromNetworkData(void);
    void IncrementSequenceNumber(void);
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *ActionToString(Action aAction);
    void               LogService(Action aAction, Error aError);
    void               LogDomainPrefix(Action aAction, Error aError);
#else
    void LogService(Action, Error) {}
    void LogDomainPrefix(Action, Error) {}
#endif

    // Indicates whether or not already add Backbone Router Service to local server data.
    // Used to check whether or not in restore stage after reset or whether to remove
    // Backbone Router service for Secondary Backbone Router if it was added by force.
    bool                            mIsServiceAdded;
    State                           mState;
    uint8_t                         mSequenceNumber;
    uint8_t                         mRegistrationJitter;
    uint16_t                        mReregistrationDelay;
    uint16_t                        mRegistrationTimeout;
    uint32_t                        mMlrTimeout;
    NetworkData::OnMeshPrefixConfig mDomainPrefixConfig;
    Ip6::Netif::UnicastAddress      mBbrPrimaryAloc;
    Ip6::Address                    mAllNetworkBackboneRouters;
    Ip6::Address                    mAllDomainBackboneRouters;
    Callback<DomainPrefixCallback>  mDomainPrefixCallback;
};

} // namespace BackboneRouter

DefineMapEnum(otBackboneRouterState, BackboneRouter::Local::State);

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#endif // BACKBONE_ROUTER_LOCAL_HPP_
