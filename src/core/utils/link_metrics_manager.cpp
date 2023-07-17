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

#include "link_metrics_manager.hpp"

#if OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE

#include "common/as_core_type.hpp"
#include "common/error.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/notifier.hpp"
#include "thread/mle.hpp"
#include "thread/neighbor_table.hpp"
#include "thread/topology.hpp"

namespace ot {
namespace Utils {

RegisterLogModule("LinkMetricsMgr");

/**
 * @addtogroup utils-link-metrics-manager
 *
 * @brief
 *   This module includes definitions for Link Metrics Manager.
 *
 * @{
 */

LinkMetricsManager::LinkMetricsManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
    , mEnabled(OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ON_BY_DEFAULT)
{
}

void LinkMetricsManager::SetEnabled(bool aEnable)
{
    VerifyOrExit(mEnabled != aEnable);
    mEnabled = aEnable;

    if (mEnabled)
    {
        Start();
    }
    else
    {
        Stop();
    }

exit:
    return;
}

Error LinkMetricsManager::GetLinkMetricsValueByExtAddr(const Mac::ExtAddress      *aExtAddress,
                                                       LinkMetrics::MetricsValues *aMetricsValues)
{
    Error    error = kErrorNone;
    Subject *subject;

    VerifyOrExit(aExtAddress != nullptr && aMetricsValues != nullptr, error = kErrorInvalidArgs);

    subject = mSubjectList.FindMatching(*aExtAddress);
    VerifyOrExit(subject != nullptr, error = kErrorNotFound);

    aMetricsValues->mLinkMarginValue = subject->mData.mLinkMargin;
    aMetricsValues->mRssiValue       = subject->mData.mRssi;

exit:
    return error;
}

void LinkMetricsManager::Start(void)
{
    LinkMetrics::Initiator &initiator = Get<LinkMetrics::Initiator>();

    VerifyOrExit(mEnabled && Get<Mle::Mle>().IsAttached());

    initiator.SetMgmtResponseCallback(HandleMgmtResponse, this);
    initiator.SetEnhAckProbingCallback(HandleEnhAckIe, this);

    mTimer.Start(kTimeBeforeStartMilliSec);
exit:
    return;
}

void LinkMetricsManager::Stop(void)
{
    LinkMetrics::Initiator &initiator = Get<LinkMetrics::Initiator>();

    mTimer.Stop();

    initiator.SetMgmtResponseCallback(nullptr, this);
    initiator.SetEnhAckProbingCallback(nullptr, this);

    UnregisterAllSubjects();
    ReleaseAllSubjects();
}

void LinkMetricsManager::Update(void)
{
    UpdateSubjects();
    UpdateLinkMetricsStates();
}

// This method updates the Link Metrics Subject in the subject list. It adds new neighbors to the list.
void LinkMetricsManager::UpdateSubjects(void)
{
    Neighbor::Info         neighborInfo;
    otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;

    while (Get<NeighborTable>().GetNextNeighborInfo(iterator, neighborInfo) == OT_ERROR_NONE)
    {
        // if not in the subject list, allocate and add
        if (!mSubjectList.ContainsMatching(AsCoreType(&neighborInfo.mExtAddress)))
        {
            Subject *subject = mPool.Allocate();
            if (subject != nullptr)
            {
                subject->Clear();
                subject->mExtAddress = AsCoreType(&neighborInfo.mExtAddress);
                IgnoreError(mSubjectList.Add(*subject));
            }
        }
    }
}

// This method updates the state and take corresponding actions for all subjects and removes stale subjects.
void LinkMetricsManager::UpdateLinkMetricsStates(void)
{
    ot::LinkedList<Subject> staleSubjects;

    Subject *entry;
    Subject *prev;
    Subject *next;

    for (prev = nullptr, entry = mSubjectList.GetHead(); entry != nullptr; entry = next)
    {
        Error error = UpdateStateForSingleSubject(*entry);
        next        = entry->GetNext();

        if (error == kErrorUnknownNeighbor || error == kErrorNotCapable)
        {
            mSubjectList.PopAfter(prev);
            IgnoreError(staleSubjects.Add(*entry));
        }
        else
        {
            prev = entry;
        }
    }

    while (!staleSubjects.IsEmpty())
    {
        Subject *subject = staleSubjects.Pop();
        mPool.Free(*subject);
    }
}

void LinkMetricsManager::UnregisterAllSubjects(void)
{
    for (Subject &subject : mSubjectList)
    {
        IgnoreError(UnregisterEapForSingleSubject(subject));
    }
}

void LinkMetricsManager::ReleaseAllSubjects(void)
{
    while (!mSubjectList.IsEmpty())
    {
        Subject *subject = mSubjectList.Pop();
        mPool.Free(*subject);
    }
}

Error LinkMetricsManager::UpdateStateForSingleSubject(Subject &aSubject)
{
    bool     shouldConfigure = false;
    uint32_t pastTimeMs;
    Error    error = kErrorNone;

    switch (aSubject.mState)
    {
    case SubjectState::kNotConfigured:
    case SubjectState::kConfiguring:
    case SubjectState::kRenewing:
        if (aSubject.mAttempts >= kConfigureLinkMetricsMaxAttempts)
        {
            aSubject.mState = kNotSupported;
        }
        else
        {
            shouldConfigure = true;
        }
        break;
    case SubjectState::kActive:
        pastTimeMs = TimerMilli::GetNow() - aSubject.mLastUpdateTime;
        if (pastTimeMs >= kStateUpdateIntervalMilliSec)
        {
            shouldConfigure = true;
        }
        break;
    case SubjectState::kNotSupported:
        ExitNow(error = kErrorNotCapable);
        break;
    default:
        break;
    }

    if (shouldConfigure)
    {
        error = ConfigureEapForSingleSubject(aSubject);
    }

exit:
    return error;
}

Error LinkMetricsManager::ConfigureEapForSingleSubject(Subject &aSubject)
{
    Error                    error    = kErrorNone;
    Neighbor                *neighbor = Get<NeighborTable>().FindNeighbor(aSubject.mExtAddress);
    Ip6::Address             destination;
    LinkMetrics::EnhAckFlags enhAckFlags = LinkMetrics::kEnhAckRegister;
    LinkMetrics::Metrics     metricsFlags;

    VerifyOrExit(neighbor != nullptr, error = kErrorUnknownNeighbor);
    destination.SetToLinkLocalAddress(neighbor->GetExtAddress());

    metricsFlags.Clear();
    metricsFlags.mLinkMargin = 1;
    metricsFlags.mRssi       = 1;
    error = Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(destination, enhAckFlags, &metricsFlags);

    aSubject.mAttempts++;
exit:
    if (error == kErrorNone)
    {
        if (aSubject.mState == SubjectState::kActive)
        {
            aSubject.mState = SubjectState::kRenewing;
        }
        else
        {
            aSubject.mState = SubjectState::kConfiguring;
        }
    }
    return error;
}

Error LinkMetricsManager::UnregisterEapForSingleSubject(Subject &aSubject)
{
    Error                    error    = kErrorNone;
    Neighbor                *neighbor = Get<NeighborTable>().FindNeighbor(aSubject.mExtAddress);
    Ip6::Address             destination;
    LinkMetrics::EnhAckFlags enhAckFlags = LinkMetrics::kEnhAckClear;

    VerifyOrExit(neighbor != nullptr, error = kErrorUnknownNeighbor);
    destination.SetToLinkLocalAddress(neighbor->GetExtAddress());

    error = Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(destination, enhAckFlags, nullptr);
exit:
    return error;
}

void LinkMetricsManager::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        if (Get<Mle::Mle>().IsAttached())
        {
            Start();
        }
        else
        {
            Stop();
        }
    }
}

void LinkMetricsManager::HandleTimer(void)
{
    if (Get<Mle::Mle>().IsAttached())
    {
        Update();
        mTimer.Start(kStateUpdateIntervalMilliSec);
    }
}

void LinkMetricsManager::HandleMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus, void *aContext)
{
    static_cast<LinkMetricsManager *>(aContext)->HandleMgmtResponse(aAddress, aStatus);
}

void LinkMetricsManager::HandleMgmtResponse(const otIp6Address *aAddress, otLinkMetricsStatus aStatus)
{
    Mac::ExtAddress extAddress;
    Subject        *subject;
    Neighbor       *neighbor;

    AsCoreType(aAddress).GetIid().ConvertToExtAddress(extAddress);
    neighbor = Get<NeighborTable>().FindNeighbor(extAddress);
    VerifyOrExit(neighbor != nullptr);

    subject = mSubjectList.FindMatching(extAddress);
    VerifyOrExit(subject != nullptr);

    switch (MapEnum(aStatus))
    {
    case LinkMetrics::Status::kStatusSuccess:
        subject->mState = SubjectState::kActive;
        break;
    default:
        subject->mState = SubjectState::kNotConfigured;
        break;
    }

exit:
    return;
}

void LinkMetricsManager::HandleEnhAckIe(otShortAddress             aShortAddress,
                                        const otExtAddress        *aExtAddress,
                                        const otLinkMetricsValues *aMetricsValues,
                                        void                      *aContext)
{
    static_cast<LinkMetricsManager *>(aContext)->HandleEnhAckIe(aShortAddress, aExtAddress, aMetricsValues);
}

void LinkMetricsManager::HandleEnhAckIe(otShortAddress             aShortAddress,
                                        const otExtAddress        *aExtAddress,
                                        const otLinkMetricsValues *aMetricsValues)
{
    OT_UNUSED_VARIABLE(aShortAddress);
    Error    error   = kErrorNone;
    Subject *subject = mSubjectList.FindMatching(AsCoreType(aExtAddress));
    VerifyOrExit(subject != nullptr, error = kErrorNotFound);

    VerifyOrExit(subject->mState == SubjectState::kActive || subject->mState == SubjectState::kRenewing);
    subject->mLastUpdateTime = TimerMilli::GetNow();

    VerifyOrExit(aMetricsValues->mMetrics.mRssi && aMetricsValues->mMetrics.mLinkMargin, error = kErrorInvalidArgs);

    subject->mData.mRssi       = aMetricsValues->mRssiValue;
    subject->mData.mLinkMargin = aMetricsValues->mLinkMarginValue;

exit:
    if (error == kErrorInvalidArgs)
    {
        LogWarn("Metrics received are unexpected!");
    }
    return;
}

/**
 * @}
 *
 */

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
