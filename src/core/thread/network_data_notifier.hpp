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
 *   This file includes definitions for transmitting SVR_DATA.ntf messages.
 */

#ifndef NETWORK_DATA_NOTIFIER_HPP_
#define NETWORK_DATA_NOTIFIER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#include "coap/coap.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/tasklet.hpp"
#include "common/time_ticker.hpp"

namespace ot {
namespace NetworkData {

/**
 * This class implements the SVR_DATA.ntf transmission logic.
 *
 */
class Notifier : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class ot::TimeTicker;

public:
    /**
     * Constructor.
     *
     * @param[in] aInstance  The OpenThread instance.
     *
     */
    explicit Notifier(Instance &aInstance);

    /**
     * Call this method to inform the notifier that new server data is available.
     *
     * This method posts a tasklet to sync new server data with leader so if there are multiple changes within the same
     * flow of execution (multiple calls to this method) they are all synchronized together and included in the same
     * message to the leader.
     *
     */
    void HandleServerDataUpdated(void);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    /**
     * This method indicates whether the device as border router is eligible for router role upgrade request using the
     * status reason `kBorderRouterRequest`.
     *
     * This method checks whether device is providing IP connectivity and that there are fewer than two border routers
     * in network data acting as router.  Device is considered to provide external IP connectivity if at least one of
     * the below conditions hold:
     *
     * - It has added at least one external route entry.
     * - It has added at least one prefix entry with default-route and on-mesh flags set.
     * - It has added at least one domain prefix (domain and on-mesh flags set).
     *
     * This method does not check the current role of device.
     *
     * @retval TRUE    Device is eligible to request router role upgrade as a border router.
     * @retval FALSE   Device is not eligible to request router role upgrade as a border router.
     *
     */
    bool IsEligibleForRouterRoleUpgradeAsBorderRouter(void) const;
#endif

private:
    static constexpr uint32_t kDelayNoBufs                 = 1000;   // in msec
    static constexpr uint32_t kDelayRemoveStaleChildren    = 5000;   // in msec
    static constexpr uint32_t kDelaySynchronizeServerData  = 300000; // in msec
    static constexpr uint8_t  kRouterRoleUpgradeMaxTimeout = 10;     // in sec

    void HandleNotifierEvents(Events aEvents);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    static void HandleCoapResponse(void *               aContext,
                                   otMessage *          aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   Error                aResult);
    void        HandleCoapResponse(Error aResult);

    void SynchronizeServerData(void);
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    void ScheduleRouterRoleUpgradeIfEligible(void);
    void HandleTimeTick(void);
#endif

    using SynchronizeDataTask = TaskletIn<Notifier, &Notifier::SynchronizeServerData>;

    TimerMilli          mTimer;
    SynchronizeDataTask mSynchronizeDataTask;
    uint32_t            mNextDelay;
    bool                mWaitingForResponse : 1;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    bool    mDidRequestRouterRoleUpgrade : 1;
    uint8_t mRouterRoleUpgradeTimeout;
#endif
};

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#endif // NETWORK_DATA_NOTIFIER_HPP_
