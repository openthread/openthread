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
     * @param[in]  aNetif  A reference to the Thread network interface.
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
     * @param[out]  aConfig  A reference whether to put local Backbone Router configuration.
     *
     */
    void GetConfig(BackboneRouterConfig &aConfig) const;

    /**
     * This method sets local Backbone Router configuration.
     *
     * @param[in]  aConfig              A reference of the configuration to set.
     *
     */
    void SetConfig(const BackboneRouterConfig &aConfig);

    /**
     * This method notifies the Primary Backbone Router update in existing Thread Network.
     *
     * @param[in]  aState               The Primary Backbone Router state or state change.
     * @param[in]  aConfig              A reference of the Primary Backbone Router information.
     *
     */
    void NotifyBackboneRouterPrimaryUpdate(Leader::State aState, const BackboneRouterConfig &aConfig);

    /**
     * This method registers Backbone Router Dataset to Leader.
     *
     * @param[in]  aForce True if to register forcely regardless of current BackboneRouterState.
     *                    False if allow to decide whether or not to register according to
     *                    current BackboneRouterState.
     *
     * @retval OT_ERROR_NONE             Successfully added the Service entry.
     * @retval OT_ERROR_INVALID_STATE    Not in the ready state to register.
     * @retval OT_ERROR_NO_BUFS          Insufficient space to add the Service entry.
     *
     */
    otError AddService(bool aForce = false);

private:
    otError RemoveService(void);
    otError AddBackboneRouterPrimaryAloc(void);

    void SetState(BackboneRouterState aState);

    // Indicates whether or not already add Backbone Router Service to local server data.
    // Used to check whether or not in restore stage after reset.
    bool mIsServiceAdded : 1;

    BackboneRouterState mState;
    uint8_t             mSequenceNumber;
    uint16_t            mReregistrationDelay;
    uint32_t            mMlrTimeout;

    Ip6::NetifUnicastAddress mBackboneRouterPrimaryAloc;
};

} // namespace BackboneRouter

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#endif // BACKBONE_ROUTER_LOCAL_HPP_
