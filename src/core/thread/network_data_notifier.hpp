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

#if OPENTHREAD_CONFIG_BORDER_ROUTER_LEADER_OVERRIDE_ENABLE && !OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
#error "OPENTHREAD_CONFIG_BORDER_ROUTER_LEADER_OVERRIDE_ENABLE requires BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL"
#endif

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#ifdef OPENTHREAD_NETDATA_NOTIFIER_USES_TIME_TICKER
#error "OPENTHREAD_NETDATA_NOTIFIER_USES_TIME_TICKER should not be defined directly, it is derived from other configs"
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
#define OPENTHREAD_NETDATA_NOTIFIER_USES_TIME_TICKER 1
#elif OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_LEADER_OVERRIDE_ENABLE
#define OPENTHREAD_NETDATA_NOTIFIER_USES_TIME_TICKER 1
#else
#define OPENTHREAD_NETDATA_NOTIFIER_USES_TIME_TICKER 0
#endif

#include <openthread/border_router.h>

#include "coap/coap.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/tasklet.hpp"
#include "common/time_ticker.hpp"

namespace ot {
namespace NetworkData {

class NetworkData;

/**
 * Implements the SVR_DATA.ntf transmission logic.
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
     * Posts a tasklet to sync new server data with leader so if there are multiple changes within the same
     * flow of execution (multiple calls to this method) they are all synchronized together and included in the same
     * message to the leader.
     *
     */
    void HandleServerDataUpdated(void);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_LEADER_OVERRIDE_ENABLE
    /**
     * Enables or disables the leader override mechanism.
     *
     * When enabled, device acting as a border router (BR) monitors the following trigger conditions to start leader
     * override:
     * - The BR's leader weight is higher than the current partition's weight (as indicated in the current Leader Data).
     * - The BR has pending local Network Data entries and has tried to register them with the leader at least 3 times,
     *   but failed each time.
     * - Each attempt consisted of sending a SRV_DATA.ntf message to the leader, which was acknowledged but not
     *   integrated into the Thread Network Data within `DATA_RESUBMIT_DELAY` seconds (300 seconds).
     * - The maximum size of the Thread Network Data has been such that the local Network Data entries would fit over
     *   the past period.
     *
     * If all of these conditions are met, the BR starts the leader override procedure by selecting a random delay
     * between 1 and 30 seconds. If the trigger conditions still hold after the random delay, the BR starts a new
     * partition as the leader.
     *
     * @param[in]  aEnabled     TRUE to enable leader override mechanism, FALSE to disable.
     *
     */
    void SetLeaderOverrideEnabled(bool aEnabled);

    /**
     * Indicates whether or not leader override mechanism is enabled.
     *
     * @retval TRUE  The leader override mechanism is enabled.
     * @retval FALSE The leader override mechanism is disabled.
     *
     * @sa SetLeaderOverrideEnabled
     *
     */
    bool IsLeaderOverrideEnabled(void) const { return mLeaderOverrideEnabled; }

#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    typedef otBorderRouterNetDataFullCallback NetDataCallback; ///< Network Data full callback.

    /**
     * Sets the callback to indicate when Network Data gets full.
     *
     * @param[in] aCallback   The callback.
     * @param[in] aContext    The context to use with @p aCallback.
     *
     */
    void SetNetDataFullCallback(NetDataCallback aCallback, void *aContext);

    /**
     * Signals that network data (local or leader) is getting full.
     *
     */
    void SignalNetworkDataFull(void) { mNetDataFullTask.Post(); }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    /**
     * Indicates whether the device as border router is eligible for router role upgrade request using the
     * status reason `kBorderRouterRequest`.
     *
     * Checks whether device is providing IP connectivity and that there are fewer than two border routers
     * in network data acting as router.  Device is considered to provide external IP connectivity if at least one of
     * the below conditions hold:
     *
     * - It has added at least one external route entry.
     * - It has added at least one prefix entry with default-route and on-mesh flags set.
     * - It has added at least one domain prefix (domain and on-mesh flags set).
     *
     * Does not check the current role of device.
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

    // Leader override constants
    static constexpr uint8_t kLeaderOverrideTriggerFailedAttempts = 3;
    static constexpr uint8_t kMinLeaderOverrideDelay              = 1;  // in sec
    static constexpr uint8_t kMaxLeaderOverrideDelay              = 30; // in sec

    void  SynchronizeServerData(void);
    Error SendServerDataNotification(uint16_t aOldRloc16, const NetworkData *aNetworkData = nullptr);
#if OPENTHREAD_FTD
    Error RemoveStaleChildEntries(void);
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    Error UpdateInconsistentData(void);
#endif

    void        HandleNotifierEvents(Events aEvents);
    void        HandleTimer(void);
    static void HandleCoapResponse(void                *aContext,
                                   otMessage           *aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   Error                aResult);
    void        HandleCoapResponse(Error aResult);

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    void HandleNetDataFull(void);
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    void ScheduleRouterRoleUpgradeIfEligible(void);
    bool HandleTimeTickForRoleUpgrade(void);
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_LEADER_OVERRIDE_ENABLE
    void ResetLeaderOverride(void);
    bool ShouldOverrideLeaderRole(void) const;
    bool HandleTimeTickForLeaderOverride(void);
#endif

#if OPENTHREAD_NETDATA_NOTIFIER_USES_TIME_TICKER
    void HandleTimeTick(void);
#endif

    using SynchronizeDataTask = TaskletIn<Notifier, &Notifier::SynchronizeServerData>;
    using DelayTimer          = TimerMilliIn<Notifier, &Notifier::HandleTimer>;
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    using NetDataFullTask = TaskletIn<Notifier, &Notifier::HandleNetDataFull>;
#endif

    DelayTimer          mTimer;
    SynchronizeDataTask mSynchronizeDataTask;
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    NetDataFullTask           mNetDataFullTask;
    Callback<NetDataCallback> mNetDataFullCallback;
#endif
    uint32_t mNextDelay;
    uint16_t mOldRloc;
    bool     mWaitingForResponse : 1;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    bool    mDidRequestRouterRoleUpgrade : 1;
    uint8_t mRouterRoleUpgradeTimeout;
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_LEADER_OVERRIDE_ENABLE
    bool    mLeaderOverrideEnabled;
    uint8_t mFailedAttempts;
    uint8_t mLeaderOverrideDelay;
#endif
};

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#endif // NETWORK_DATA_NOTIFIER_HPP_
