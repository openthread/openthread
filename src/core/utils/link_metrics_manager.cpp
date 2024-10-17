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

#include "instance/instance.hpp"

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

Error LinkMetricsManager::GetLinkMetricsValueByExtAddr(const Mac::ExtAddress      &aExtAddress,
                                                       LinkMetrics::MetricsValues &aMetricsValues)
{
    Error    error = kErrorNone;
    Subject *subject;

    subject = mSubjectList.FindMatching(aExtAddress);
    VerifyOrExit(subject != nullptr, error = kErrorNotFound);
    VerifyOrExit(subject->mState == kActive || subject->mState == kRenewing, error = kErrorInvalidState);

    aMetricsValues.mLinkMarginValue = subject->mData.mLinkMargin;
    aMetricsValues.mRssiValue       = subject->mData.mRssi;

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

    initiator.SetMgmtResponseCallback(nullptr, nullptr);
    initiator.SetEnhAckProbingCallback(nullptr, nullptr);

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

    while (Get<NeighborTable>().GetNextNeighborInfo(iterator, neighborInfo) == kErrorNone)
    {
        // if not in the subject list, allocate and add
        if (!mSubjectList.ContainsMatching(AsCoreType(&neighborInfo.mExtAddress)))
        {
            Subject *subject = mPool.Allocate();

            VerifyOrExit(subject != nullptr);

            subject->Clear();
            subject->mExtAddress = AsCoreType(&neighborInfo.mExtAddress);
            IgnoreError(mSubjectList.Add(*subject));
        }
    }

exit:
    return;
}

// This method updates the state and take corresponding actions for all subjects and removes stale subjects.
void LinkMetricsManager::UpdateLinkMetricsStates(void)
{
    LinkedList<Subject> staleSubjects;

    mSubjectList.RemoveAllMatching(*this, staleSubjects);

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
        IgnoreError(subject.UnregisterEap(GetInstance()));
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
}

// This special Match method is used for "iterating over list while removing some items"
bool LinkMetricsManager::Subject::Matches(const LinkMetricsManager &aLinkMetricsMgr)
{
    Error error = UpdateState(aLinkMetricsMgr.GetInstance());

    return error == kErrorUnknownNeighbor || error == kErrorNotCapable;
}

Error LinkMetricsManager::Subject::ConfigureEap(Instance &aInstance)
{
    Error                    error    = kErrorNone;
    Neighbor                *neighbor = aInstance.Get<NeighborTable>().FindNeighbor(mExtAddress);
    Ip6::Address             destination;
    LinkMetrics::EnhAckFlags enhAckFlags = LinkMetrics::kEnhAckRegister;
    LinkMetrics::Metrics     metricsFlags;

    VerifyOrExit(neighbor != nullptr, error = kErrorUnknownNeighbor);
    destination.SetToLinkLocalAddress(neighbor->GetExtAddress());

    metricsFlags.Clear();
    metricsFlags.mLinkMargin = 1;
    metricsFlags.mRssi       = 1;
    error =
        aInstance.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(destination, enhAckFlags, &metricsFlags);

exit:
    if (error == kErrorNone)
    {
        mState = (mState == SubjectState::kActive) ? SubjectState::kRenewing : SubjectState::kConfiguring;
        mAttempts++;
    }
    return error;
}

Error LinkMetricsManager::Subject::UnregisterEap(Instance &aInstance)
{
    Error                    error    = kErrorNone;
    Neighbor                *neighbor = aInstance.Get<NeighborTable>().FindNeighbor(mExtAddress);
    Ip6::Address             destination;
    LinkMetrics::EnhAckFlags enhAckFlags = LinkMetrics::kEnhAckClear;

    VerifyOrExit(neighbor != nullptr, error = kErrorUnknownNeighbor);
    destination.SetToLinkLocalAddress(neighbor->GetExtAddress());

    error = aInstance.Get<LinkMetrics::Initiator>().SendMgmtRequestEnhAckProbing(destination, enhAckFlags, nullptr);
exit:
    return error;
}

Error LinkMetricsManager::Subject::UpdateState(Instance &aInstance)
{
    bool     shouldConfigure = false;
    uint32_t pastTimeMs;
    Error    error = kErrorNone;

    switch (mState)
    {
    case kNotConfigured:
    case kConfiguring:
    case kRenewing:
        if (mAttempts >= kConfigureLinkMetricsMaxAttempts)
        {
            mState = kNotSupported;
        }
        else
        {
            shouldConfigure = true;
        }
        break;
    case kActive:
        pastTimeMs = TimerMilli::GetNow() - mLastUpdateTime;
        if (pastTimeMs >= kStateUpdateIntervalMilliSec)
        {
            shouldConfigure = true;
        }
        break;
    case kNotSupported:
        ExitNow(error = kErrorNotCapable);
        break;
    default:
        break;
    }

    if (shouldConfigure)
    {
        error = ConfigureEap(aInstance);
    }

exit:
    return error;
}

/**
 * @}
 */

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_LINK_METRICS_MANAGER_ENABLE
