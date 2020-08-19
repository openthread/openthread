/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file implements the Notifier class.
 */

#include "notifier.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"

namespace ot {

Notifier::Notifier(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEventsToSignal()
    , mSignaledEvents()
    , mTask(aInstance, Notifier::EmitEvents, this)
{
    for (ExternalCallback &callback : mExternalCallbacks)
    {
        callback.mHandler = nullptr;
        callback.mContext = nullptr;
    }
}

otError Notifier::RegisterCallback(otStateChangedCallback aCallback, void *aContext)
{
    otError           error          = OT_ERROR_NONE;
    ExternalCallback *unusedCallback = nullptr;

    VerifyOrExit(aCallback != nullptr, OT_NOOP);

    for (ExternalCallback &callback : mExternalCallbacks)
    {
        if (callback.mHandler == nullptr)
        {
            if (unusedCallback == nullptr)
            {
                unusedCallback = &callback;
            }

            continue;
        }

        VerifyOrExit((callback.mHandler != aCallback) || (callback.mContext != aContext), error = OT_ERROR_ALREADY);
    }

    VerifyOrExit(unusedCallback != nullptr, error = OT_ERROR_NO_BUFS);

    unusedCallback->mHandler = aCallback;
    unusedCallback->mContext = aContext;

exit:
    return error;
}

void Notifier::RemoveCallback(otStateChangedCallback aCallback, void *aContext)
{
    VerifyOrExit(aCallback != nullptr, OT_NOOP);

    for (ExternalCallback &callback : mExternalCallbacks)
    {
        if ((callback.mHandler == aCallback) && (callback.mContext == aContext))
        {
            callback.mHandler = nullptr;
            callback.mContext = nullptr;
        }
    }

exit:
    return;
}

void Notifier::Signal(Event aEvent)
{
    mEventsToSignal.Add(aEvent);
    mSignaledEvents.Add(aEvent);
    mTask.Post();
}

void Notifier::SignalIfFirst(Event aEvent)
{
    if (!HasSignaled(aEvent))
    {
        Signal(aEvent);
    }
}

void Notifier::EmitEvents(Tasklet &aTasklet)
{
    aTasklet.GetOwner<Notifier>().EmitEvents();
}

void Notifier::EmitEvents(void)
{
    Events events;

    VerifyOrExit(!mEventsToSignal.IsEmpty(), OT_NOOP);

    // Note that the callbacks may signal new events, so we create a
    // copy of `mEventsToSignal` and then clear it.

    events = mEventsToSignal;
    mEventsToSignal.Clear();

    LogEvents(events);

    // Emit events to core internal modules

    Get<Mle::Mle>().HandleNotifierEvents(events);
    Get<EnergyScanServer>().HandleNotifierEvents(events);
#if OPENTHREAD_FTD
    Get<MeshCoP::JoinerRouter>().HandleNotifierEvents(events);
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Manager>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE
    Get<Utils::ChildSupervisor>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_CHANNEL_MANAGER_ENABLE
    Get<Utils::ChannelManager>().HandleNotifierEvents(events);
#endif
#endif // OPENTHREAD_FTD
#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    Get<NetworkData::Notifier>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
    Get<AnnounceSender>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    Get<MeshCoP::BorderAgent>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_MLR_ENABLE || OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    Get<MlrManager>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_DUA_ENABLE || OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    Get<DuaManager>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    Get<TimeSync>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
    Get<Utils::Slaac>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE
    Get<Utils::JamDetector>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().HandleNotifierEvents(events);
#endif
#if OPENTHREAD_ENABLE_VENDOR_EXTENSION
    Get<Extension::ExtensionBase>().HandleNotifierEvents(events);
#endif

    for (ExternalCallback &callback : mExternalCallbacks)
    {
        if (callback.mHandler != nullptr)
        {
            callback.mHandler(events.GetAsFlags(), callback.mContext);
        }
    }

exit:
    return;
}

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_CORE == 1)

void Notifier::LogEvents(Events aEvents) const
{
    Events::Flags                  flags    = aEvents.GetAsFlags();
    bool                           addSpace = false;
    bool                           didLog   = false;
    String<kFlagsStringBufferSize> string;

    for (uint8_t bit = 0; bit < sizeof(Events::Flags) * CHAR_BIT; bit++)
    {
        VerifyOrExit(flags != 0, OT_NOOP);

        if (flags & (1 << bit))
        {
            if (string.GetLength() >= kFlagsStringLineLimit)
            {
                otLogInfoCore("Notifier: StateChanged (0x%08x) %s%s ...", aEvents.GetAsFlags(), didLog ? "... " : "[",
                              string.AsCString());
                string.Clear();
                didLog   = true;
                addSpace = false;
            }

            IgnoreError(string.Append("%s%s", addSpace ? " " : "", EventToString(static_cast<Event>(1 << bit))));
            addSpace = true;

            flags ^= (1 << bit);
        }
    }

exit:
    otLogInfoCore("Notifier: StateChanged (0x%08x) %s%s] ", aEvents.GetAsFlags(), didLog ? "... " : "[",
                  string.AsCString());
}

const char *Notifier::EventToString(Event aEvent) const
{
    const char *retval = "(unknown)";

    // To ensure no clipping of flag names in the logs, the returned
    // strings from this method should have shorter length than
    // `kMaxFlagNameLength` value.

    switch (aEvent)
    {
    case kEventIp6AddressAdded:
        retval = "Ip6+";
        break;

    case kEventIp6AddressRemoved:
        retval = "Ip6-";
        break;

    case kEventThreadRoleChanged:
        retval = "Role";
        break;

    case kEventThreadLinkLocalAddrChanged:
        retval = "LLAddr";
        break;

    case kEventThreadMeshLocalAddrChanged:
        retval = "MLAddr";
        break;

    case kEventThreadRlocAdded:
        retval = "Rloc+";
        break;

    case kEventThreadRlocRemoved:
        retval = "Rloc-";
        break;

    case kEventThreadPartitionIdChanged:
        retval = "PartitionId";
        break;

    case kEventThreadKeySeqCounterChanged:
        retval = "KeySeqCntr";
        break;

    case kEventThreadNetdataChanged:
        retval = "NetData";
        break;

    case kEventThreadChildAdded:
        retval = "Child+";
        break;

    case kEventThreadChildRemoved:
        retval = "Child-";
        break;

    case kEventIp6MulticastSubscribed:
        retval = "Ip6Mult+";
        break;

    case kEventIp6MulticastUnsubscribed:
        retval = "Ip6Mult-";
        break;

    case kEventThreadChannelChanged:
        retval = "Channel";
        break;

    case kEventThreadPanIdChanged:
        retval = "PanId";
        break;

    case kEventThreadNetworkNameChanged:
        retval = "NetName";
        break;

    case kEventThreadExtPanIdChanged:
        retval = "ExtPanId";
        break;

    case kEventMasterKeyChanged:
        retval = "MstrKey";
        break;

    case kEventPskcChanged:
        retval = "PSKc";
        break;

    case kEventSecurityPolicyChanged:
        retval = "SecPolicy";
        break;

    case kEventChannelManagerNewChannelChanged:
        retval = "CMNewChan";
        break;

    case kEventSupportedChannelMaskChanged:
        retval = "ChanMask";
        break;

    case kEventCommissionerStateChanged:
        retval = "CommissionerState";
        break;

    case kEventThreadNetifStateChanged:
        retval = "NetifState";
        break;

    case kEventThreadBackboneRouterStateChanged:
        retval = "BbrState";
        break;

    case kEventThreadBackboneRouterLocalChanged:
        retval = "BbrLocal";
        break;

    case kEventJoinerStateChanged:
        retval = "JoinerState";
        break;
    }

    return retval;
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_CORE == 1)

void Notifier::LogEvents(Events) const
{
}

const char *Notifier::EventToString(Event) const
{
    return "";
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_CORE == 1)

// LCOV_EXCL_STOP

} // namespace ot
