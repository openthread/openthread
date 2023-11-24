/*
 *  Copyright (c) 2023, The OpenThread Authors.
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

#ifndef LINK_METRICS_MANAGER_HPP_
#define LINK_METRICS_MANAGER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE == 0
#error \
    "OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE can only be used when OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE is true"
#endif

#include <openthread/link_metrics.h>

#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/pool.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "thread/link_metrics_types.hpp"

namespace ot {
class UnitTester;
namespace Utils {

/**
 * @addtogroup utils-link-metrics-manager
 *
 * @brief
 *   This module includes definitions for Link Metrics Manager.
 *
 * @{
 */

/**
 *
 * Link Metrics Manager feature utilizes the Enhanced-ACK Based
 * Probing (abbreviated as "EAP" below) to get the Link Metrics
 * data of neighboring devices. It is a user of the Link Metrics
 * feature.
 *
 * The feature works as follow:
 * - Start/Stop
 *   The switch `OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ON_BY_DEFAULT`
 *   controls enabling/disabling this feature by default. The feature
 *   will only start to work after it joins a Thread network.
 *
 *   A CLI interface is provided to enable/disable this feature.
 *
 *   Once enabled, it will regularly check current neighbors (all
 *   devices in the neighbor table, including 'Child' and 'Router')
 *   and configure the probing with them if haven't done that.
 *   If disabled, it will clear the configuration with its subjects
 *   and the local data.
 *
 * - Maintenance
 *   The manager will regularly check the status of each subject. If
 *   it finds that the link metrics data for one subject hasn't been
 *   updated for `kStateUpdateIntervalMilliSec`, it will configure
 *   EAP with the subject again.
 *   The manager may find that some subject (neighbor) no longer
 *   exist when trying to configure EAP. It will remove the stale
 *   subject then.
 *
 * - Show data
 *   An OT API is provided to get the link metrics data of any
 *   subject (neighbor) by its extended address. In production, this
 *   data may be fetched by some other means like RPC.
 *
 */

class LinkMetricsManager : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class ot::UnitTester;

    struct LinkMetricsData
    {
        uint8_t mLqi;        ///< Link Quality Indicator. Value range: [0, 255].
        int8_t  mRssi;       ///< Receive Signal Strength Indicator. Value range: [-128, 0].
        uint8_t mLinkMargin; ///< Link Margin. The relative signal strength is recorded as
                             ///< db above the local noise floor. Value range: [0, 130].
    };

    enum SubjectState : uint8_t
    {
        kNotConfigured = 0,
        kConfiguring,
        kActive,
        kRenewing,
        kNotSupported,
    };

    struct Subject : LinkedListEntry<Subject>, Clearable<Subject>
    {
        Mac::ExtAddress mExtAddress;     ///< Use the extended address to identify the neighbor.
        SubjectState    mState;          ///< Current State of the Subject
        uint8_t         mAttempts;       ///< The count of attempt that has been made to
                                         ///< configure EAP
        TimeMilli       mLastUpdateTime; ///< The time `mData` was updated last time
        LinkMetricsData mData;

        Subject *mNext;

        bool Matches(const Mac::ExtAddress &aExtAddress) const { return mExtAddress == aExtAddress; }
        bool Matches(const LinkMetricsManager &aLinkMetricsMgr);

        Error ConfigureEap(Instance &aInstance);
        Error UnregisterEap(Instance &aInstance);
        Error UpdateState(Instance &aInstance);
    };

public:
    /**
     * Initializes a `LinkMetricsManager` object.
     *
     * @param[in]   aInstance  A reference to the OpenThread instance.
     *
     */
    explicit LinkMetricsManager(Instance &aInstance);

    /**
     * Enable/Disable the LinkMetricsManager feature.
     *
     * @param[in]   aEnable  A boolean to indicate enable or disable.
     *
     */
    void SetEnabled(bool aEnable);

    /**
     * Get Link Metrics data of subject by the extended address.
     *
     * @param[in]  aExtAddress     A reference to the extended address of the subject.
     * @param[out] aMetricsValues  A reference to the MetricsValues object to place the result.
     *
     * @retval kErrorNone             Successfully got the metrics value.
     * @retval kErrorInvalidArgs      The arguments are invalid.
     * @retval kNotFound              No neighbor with the given extended address is found.
     *
     */
    Error GetLinkMetricsValueByExtAddr(const Mac::ExtAddress &aExtAddress, LinkMetrics::MetricsValues &aMetricsValues);

private:
    static constexpr uint16_t kTimeBeforeStartMilliSec         = 5000;
    static constexpr uint32_t kStateUpdateIntervalMilliSec     = 150000;
    static constexpr uint8_t  kConfigureLinkMetricsMaxAttempts = 3;
#if OPENTHREAD_FTD
    static constexpr uint8_t kMaximumSubjectToTrack = 128;
#elif OPENTHREAD_MTD
    static constexpr uint8_t kMaximumSubjectToTrack = 1;
#endif

    void Start(void);
    void Stop(void);
    void Update(void);
    void UpdateSubjects(void);
    void UpdateLinkMetricsStates(void);
    void UnregisterAllSubjects(void);
    void ReleaseAllSubjects(void);

    void        HandleNotifierEvents(Events aEvents);
    void        HandleTimer(void);
    static void HandleMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus, void *aContext);
    void        HandleMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus);
    static void HandleEnhAckIe(otShortAddress             aShortAddress,
                               const otExtAddress        *aExtAddress,
                               const otLinkMetricsValues *aMetricsValues,
                               void                      *aContext);
    void        HandleEnhAckIe(otShortAddress             aShortAddress,
                               const otExtAddress        *aExtAddress,
                               const otLinkMetricsValues *aMetricsValues);

    using LinkMetricsMgrTimer = TimerMilliIn<LinkMetricsManager, &LinkMetricsManager::HandleTimer>;

    Pool<Subject, kMaximumSubjectToTrack> mPool;
    LinkedList<Subject>                   mSubjectList;
    LinkMetricsMgrTimer                   mTimer;
    bool                                  mEnabled;
};

/**
 * @}
 *
 */

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE

#endif // LINK_METRICS_MANAGER_HPP_
