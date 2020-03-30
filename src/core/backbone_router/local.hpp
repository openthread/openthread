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
#include <openthread/backbone_router.h>
#include <openthread/backbone_router_ftd.h>

#include "backbone_router/leader.hpp"
#include "net/netif.hpp"

namespace ot {

namespace BackboneRouter {

/**
 * This class implements the definitions for local Backbone Router service.
 *
 */
class Local : public InstanceLocator
{
public:
    typedef otBackboneRouterState BackboneRouterState;

    /**
     * This constructor initializes the local Backbone Router.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     *
     */
    explicit Local(Instance &aInstance);

    /**
     * This methhod enables/disables Backbone function.
     *
     * @param[in]  aEnable  TRUE to enable the backbone function, FALSE otherwise.
     *
     */
    void SetEnabled(bool aEnable);

    /**
     * This methhod retrieves the Backbone Router state.
     *
     *
     * @retval OT_BACKBONE_ROUTER_STATE_DISABLED   Backbone function is disabled.
     * @retval OT_BACKBONE_ROUTER_STATE_SECONDARY  Secondary Backbone Router.
     * @retval OT_BACKBONE_ROUTER_STATE_PRIMARY    Primary Backbone Router.
     *
     */
    BackboneRouterState GetState(void) const { return mState; }

    /**
     * This method resets the local Thread Network Data.
     *
     */
    void Reset(void);

    /**
     * This method gets local Backbone Router configuration.
     *
     * @param[out]  aConfig  The local Backbone Router configuration.
     *
     */
    void GetConfig(BackboneRouterConfig &aConfig) const;

    /**
     * This method sets local Backbone Router configuration.
     *
     * @param[in]  aConfig  The configuration to set.
     *
     */
    void SetConfig(const BackboneRouterConfig &aConfig);

    /**
     * This method registers Backbone Router Dataset to Leader.
     *
     * @param[in]  aForce True to force registration regardless of current BackboneRouterState.
     *                    False to decide based on current BackboneRouterState.
     *
     *
     * @retval OT_ERROR_NONE             Successfully added the Service entry.
     * @retval OT_ERROR_INVALID_STATE    Not in the ready state to register.
     * @retval OT_ERROR_NO_BUFS          Insufficient space to add the Service entry.
     *
     */
    otError AddService(bool aForce = false);

    /**
     * This method indicates whether or not the Backbone Router is Primary.
     *
     * @retval  True  if the Backbone Router is Primary.
     * @retval  False if the Backbone Router is not Primary.
     *
     */
    bool IsPrimary(void) const { return mState == OT_BACKBONE_ROUTER_STATE_PRIMARY; }

    /**
     * This method indicates whether or not the Backbone Router is enabled.
     *
     * @retval  True  if the Backbone Router is enabled.
     * @retval  False if the Backbone Router is not enabled.
     *
     */
    bool IsEnabled(void) const { return mState != OT_BACKBONE_ROUTER_STATE_DISABLED; }

    /**
     * This method sets the Backbone Router registration jitter value.
     *
     * @param[in]  aRegistrationJitter the Backbone Router registration jitter value to set.
     *
     */
    void SetRegistrationJitter(uint8_t aRegistrationJitter) { mRegistrationJitter = aRegistrationJitter; }

    /**
     * This method returns the Backbone Router registration jitter value.
     *
     * @returns The Backbone Router registration jitter value.
     *
     */
    uint8_t GetRegistrationJitter(void) const { return mRegistrationJitter; }

    /**
     * This method notifies Primary Backbone Router status.
     *
     * @param[in]  aState   The state or state change of Primary Backbone Router.
     * @param[in]  aConfig  The Primary Backbone Router service.
     *
     */
    void UpdateBackboneRouterPrimary(Leader::State aState, const BackboneRouterConfig &aConfig);

private:
    void    SetState(BackboneRouterState aState);
    otError RemoveService(void);

    BackboneRouterState mState;
    uint32_t            mMlrTimeout;
    uint16_t            mReregistrationDelay;
    uint8_t             mSequenceNumber;
    uint8_t             mRegistrationJitter;

    // Indicates whether or not already add Backbone Router Service to local server data.
    // Used to check whether or not in restore stage after reset or whether to remove
    // Backbone Router service for Secondary Backbone Router if it was added by force.
    bool mIsServiceAdded;
};

} // namespace BackboneRouter

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#endif // BACKBONE_ROUTER_LOCAL_HPP_
