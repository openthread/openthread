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
 *   This file implements transmissions of SVR_DATA.ntf messages to the Leader.
 */

#include "network_data_notifier.hpp"

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace NetworkData {

RegisterLogModule("NetworkData");

Notifier::Notifier(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
    , mSynchronizeDataTask(aInstance)
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    , mNetDataFullTask(aInstance)
#endif
    , mNextDelay(0)
    , mOldRloc(Mle::kInvalidRloc16)
    , mWaitingForResponse(false)
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    , mDidRequestRouterRoleUpgrade(false)
    , mRouterRoleUpgradeTimeout(0)
#endif
{
}

void Notifier::HandleServerDataUpdated(void)
{
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    mDidRequestRouterRoleUpgrade = false;
    ScheduleRouterRoleUpgradeIfEligible();
#endif

    mNextDelay = 0;
    mSynchronizeDataTask.Post();
}

void Notifier::SynchronizeServerData(void)
{
    Error error = kErrorNotFound;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached() && !mWaitingForResponse);

    VerifyOrExit((mNextDelay == 0) || !mTimer.IsRunning());

#if OPENTHREAD_FTD
    mNextDelay = kDelayRemoveStaleChildren;
    error      = RemoveStaleChildEntries();
    VerifyOrExit(error == kErrorNotFound);
#endif

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    mNextDelay = kDelaySynchronizeServerData;
    error      = UpdateInconsistentData();
    VerifyOrExit(error == kErrorNotFound);
#endif

exit:
    switch (error)
    {
    case kErrorNone:
        mWaitingForResponse = true;
        break;
    case kErrorNoBufs:
        mTimer.Start(kDelayNoBufs);
        break;
#if OPENTHREAD_FTD
    case kErrorInvalidState:
        mTimer.Start(Time::SecToMsec(Get<Mle::MleRouter>().GetRouterRoleTransitionTimeout() + 1));
        break;
#endif
    case kErrorNotFound:
        break;
    default:
        OT_ASSERT(false);
    }
}

#if OPENTHREAD_FTD
Error Notifier::RemoveStaleChildEntries(void)
{
    // Check if there is any stale child entry in network data and send
    // a "Server Data" notification to leader to remove it.
    //
    // - `kErrorNone` when a stale child entry was found and successfully
    //    sent a "Server Data" notification to leader.
    // - `kErrorNoBufs` if could not allocate message to send message.
    // - `kErrorNotFound` if no stale child entries were found.

    Error error = kErrorNotFound;
    Rlocs rlocs;

    VerifyOrExit(Get<Mle::MleRouter>().IsRouterOrLeader());

    Get<Leader>().FindRlocs(kAnyBrOrServer, kAnyRole, rlocs);

    for (uint16_t rloc16 : rlocs)
    {
        if (Mle::IsChildRloc16(rloc16) && Get<Mle::Mle>().HasMatchingRouterIdWith(rloc16) &&
            Get<ChildTable>().FindChild(rloc16, Child::kInStateValid) == nullptr)
        {
            error = SendServerDataNotification(rloc16);
            ExitNow();
        }
    }

exit:
    return error;
}
#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
Error Notifier::UpdateInconsistentData(void)
{
    Error    error      = kErrorNone;
    uint16_t deviceRloc = Get<Mle::MleRouter>().GetRloc16();

#if OPENTHREAD_FTD
    // Don't send this Server Data Notification if the device is going
    // to upgrade to Router.

    if (Get<Mle::MleRouter>().IsExpectedToBecomeRouterSoon())
    {
        ExitNow(error = kErrorInvalidState);
    }
#endif

    Get<Local>().UpdateRloc();

    if (Get<Leader>().ContainsEntriesFrom(Get<Local>(), deviceRloc) &&
        Get<Local>().ContainsEntriesFrom(Get<Leader>(), deviceRloc))
    {
        ExitNow(error = kErrorNotFound);
    }

    if (mOldRloc == deviceRloc)
    {
        mOldRloc = Mle::kInvalidRloc16;
    }

    SuccessOrExit(error = SendServerDataNotification(mOldRloc, &Get<Local>()));
    mOldRloc = deviceRloc;

exit:
    return error;
}
#endif // #if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

Error Notifier::SendServerDataNotification(uint16_t aOldRloc16, const NetworkData *aNetworkData)
{
    Error            error = kErrorNone;
    Coap::Message   *message;
    Tmf::MessageInfo messageInfo(GetInstance());

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(kUriServerData);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    if (aNetworkData != nullptr)
    {
        ThreadTlv tlv;

        tlv.SetType(ThreadTlv::kThreadNetworkData);
        tlv.SetLength(aNetworkData->GetLength());
        SuccessOrExit(error = message->Append(tlv));
        SuccessOrExit(error = message->AppendBytes(aNetworkData->GetBytes(), aNetworkData->GetLength()));

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
        Get<Leader>().CheckForNetDataGettingFull(*aNetworkData, aOldRloc16);
#endif
    }

    if (aOldRloc16 != Mle::kInvalidRloc16)
    {
        SuccessOrExit(error = Tlv::Append<ThreadRloc16Tlv>(*message, aOldRloc16));
    }

    messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc();
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleCoapResponse, this));

    LogInfo("Sent %s", UriToString<kUriServerData>());

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Notifier::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.ContainsAny(kEventThreadRoleChanged | kEventThreadChildRemoved))
    {
        mNextDelay = 0;
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
    if (aEvents.Contains(kEventThreadPartitionIdChanged))
    {
        mDidRequestRouterRoleUpgrade = false;
    }

    if (aEvents.ContainsAny(kEventThreadRoleChanged | kEventThreadNetdataChanged | kEventThreadPartitionIdChanged))
    {
        ScheduleRouterRoleUpgradeIfEligible();
    }
#endif

    if (aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadRoleChanged | kEventThreadChildRemoved))
    {
        SynchronizeServerData();
    }
}

void Notifier::HandleTimer(void) { SynchronizeServerData(); }

void Notifier::HandleCoapResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    static_cast<Notifier *>(aContext)->HandleCoapResponse(aResult);
}

void Notifier::HandleCoapResponse(Error aResult)
{
    mWaitingForResponse = false;

    switch (aResult)
    {
    case kErrorNone:
        mTimer.Start(mNextDelay + 1);
        break;

    case kErrorResponseTimeout:
    case kErrorAbort:
        SynchronizeServerData();
        break;

    default:
        OT_ASSERT(false);
    }
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
void Notifier::SetNetDataFullCallback(NetDataCallback aCallback, void *aContext)
{
    mNetDataFullCallback.Set(aCallback, aContext);
}

void Notifier::HandleNetDataFull(void) { mNetDataFullCallback.InvokeIfSet(); }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE

bool Notifier::IsEligibleForRouterRoleUpgradeAsBorderRouter(void) const
{
    bool     isEligible = false;
    uint16_t rloc16     = Get<Mle::Mle>().GetRloc16();
    uint8_t  activeRouterCount;

    VerifyOrExit(Get<Mle::MleRouter>().IsRouterEligible());

    // RouterUpgradeThreshold can be explicitly set to zero in some of
    // cert tests to disallow device to become router.

    VerifyOrExit(Get<Mle::MleRouter>().GetRouterUpgradeThreshold() != 0);

    // Check that we are a border router providing IP connectivity and already
    // in the leader's network data and therefore eligible to request router
    // role upgrade with `kBorderRouterRequest` status.

    VerifyOrExit(Get<Local>().ContainsBorderRouterWithRloc(rloc16) &&
                 Get<Leader>().ContainsBorderRouterWithRloc(rloc16));

    activeRouterCount = Get<RouterTable>().GetActiveRouterCount();
    VerifyOrExit((activeRouterCount >= Get<Mle::MleRouter>().GetRouterUpgradeThreshold()) &&
                 (activeRouterCount < Mle::kMaxRouters));

    VerifyOrExit(Get<Leader>().CountBorderRouters(kRouterRoleOnly) < Mle::kRouterUpgradeBorderRouterRequestThreshold);
    isEligible = true;

exit:
    return isEligible;
}

void Notifier::ScheduleRouterRoleUpgradeIfEligible(void)
{
    // We allow device to request router role upgrade using status
    // reason `kBorderRouterRequest` once while its local network data
    // remains unchanged. This ensures if the leader is running an
    // older version of Thread stack which does not support
    // `kBorderRouterRequest` reason, we do not keep trying (on no
    // response). The boolean `mDidRequestRouterRoleUpgrade` tracks
    // this. It is set to `false` when local network data gets changed
    // or when partition ID gets changed (indicating a potential
    // leader change).

    VerifyOrExit(!mDidRequestRouterRoleUpgrade);

    VerifyOrExit(Get<Mle::MleRouter>().IsChild());
    VerifyOrExit(IsEligibleForRouterRoleUpgradeAsBorderRouter() && (mRouterRoleUpgradeTimeout == 0));

    mRouterRoleUpgradeTimeout = Random::NonCrypto::GetUint8InRange(1, kRouterRoleUpgradeMaxTimeout + 1);
    Get<TimeTicker>().RegisterReceiver(TimeTicker::kNetworkDataNotifier);

exit:
    return;
}

void Notifier::HandleTimeTick(void)
{
    VerifyOrExit(mRouterRoleUpgradeTimeout > 0);

    mRouterRoleUpgradeTimeout--;

    if (mRouterRoleUpgradeTimeout == 0)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kNetworkDataNotifier);

        // Check that we are still eligible for requesting router role
        // upgrade (note that state can change since the last time we
        // checked and registered to receive time ticks).

        if (Get<Mle::MleRouter>().IsChild() && IsEligibleForRouterRoleUpgradeAsBorderRouter())
        {
            LogInfo("Requesting router role as BR");
            mDidRequestRouterRoleUpgrade = true;
            IgnoreError(Get<Mle::MleRouter>().BecomeRouter(ThreadStatusTlv::kBorderRouterRequest));
        }
    }
exit:
    return;
}
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE &&
       // OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
