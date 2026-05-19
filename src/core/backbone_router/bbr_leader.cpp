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
 *   This file implements Primary Backbone Router service management in the Thread Network.
 */

#include "bbr_leader.hpp"

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#include "instance/instance.hpp"

namespace ot {
namespace BackboneRouter {

RegisterLogModule("BbrLeader");

//---------------------------------------------------------------------------------------------------------------------
// Config

void Config::AdjustMlrTimeout(void)
{
    uint32_t origTimeout;

    VerifyOrExit(IsPresent());

    origTimeout = GetMlrTimeout();
    mMlrTimeout = Clamp(mMlrTimeout, kMinMlrTimeout, kMaxMlrTimeout);

    VerifyOrExit(mMlrTimeout != origTimeout);
    LogNote("MLR timeout adjusted: %lu -> %lu", ToUlong(origTimeout), ToUlong(mMlrTimeout));

exit:
    return;
}

uint16_t Config::SelectRandomReregistrationDelay(void) const
{
    uint16_t delay = 1;

    VerifyOrExit(mReregistrationDelay > 1);
    delay = Random::NonCrypto::GetUint16InRange(1, mReregistrationDelay + 1);

exit:
    return delay;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void Config::Log(const char *aTitle) const
{
    if (IsPresent())
    {
        LogInfo("  %s: 0x%04x seqno:%u delay:%u timeout:%lu", aTitle, mServer16, mSequenceNumber, mReregistrationDelay,
                ToUlong(mMlrTimeout));
    }
    else
    {
        LogInfo("  %s: none", aTitle);
    }
}
#endif

//---------------------------------------------------------------------------------------------------------------------
// Leader

Leader::Leader(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    Reset();
}

void Leader::Reset(void)
{
    mConfig.MarkAsAbsent();

    // Domain Prefix Length 0 indicates no available Domain Prefix in the Thread network.
    mDomainPrefix.SetLength(0);
}

Error Leader::ReadConfig(Config &aConfig) const
{
    Error error = kErrorNone;

    VerifyOrExit(HasPrimary(), error = kErrorNotFound);

    aConfig = mConfig;

exit:
    return error;
}

Error Leader::GetServiceId(uint8_t &aServiceId) const
{
    Error error = kErrorNone;

    VerifyOrExit(HasPrimary(), error = kErrorNotFound);
    error = Get<NetworkData::Service::Manager>().GetBackboneRouterServiceId(aServiceId);

exit:
    return error;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Leader::StateToString(State aState)
{
#define StateMapList(_)                        \
    _(kStateNone, "None")                      \
    _(kStateAdded, "Added")                    \
    _(kStateRemoved, "Removed")                \
    _(kStateToTriggerRereg, "Rereg triggered") \
    _(kStateRefreshed, "Refreshed")            \
    _(kStateUnchanged, "Unchanged")

    DefineEnumStringArray(StateMapList);

    return kStrings[aState];
}

const char *Leader::DomainPrefixEventToString(DomainPrefixEvent aEvent)
{
#define DomainPrefixEventMapList(_)    \
    _(kDomainPrefixAdded, "Added")     \
    _(kDomainPrefixRemoved, "Removed") \
    _(kDomainPrefixRefreshed, "Refreshed")

    DefineEnumStringArray(DomainPrefixEventMapList);

    return kStrings[aEvent];
}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Leader::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadRoleChanged))
    {
        UpdateBackboneRouterPrimary();
        UpdateDomainPrefixConfig();
    }
}

void Leader::UpdateBackboneRouterPrimary(void)
{
    Config newConfig;
    State  state;

    Get<NetworkData::Service::Manager>().GetBackboneRouterPrimary(newConfig);

    newConfig.AdjustMlrTimeout();

    if (newConfig.GetServer16() != mConfig.GetServer16())
    {
        if (!newConfig.IsPresent())
        {
            state = kStateRemoved;
        }
        else if (!mConfig.IsPresent())
        {
            state = kStateAdded;
        }
        else
        {
            // Short Address of PBBR changes.
            state = kStateToTriggerRereg;
        }
    }
    else if (!newConfig.IsPresent())
    {
        // If no Primary all the time.
        state = kStateNone;
    }
    else if (newConfig.GetSequenceNumber() != mConfig.GetSequenceNumber())
    {
        state = kStateToTriggerRereg;
    }
    else if (newConfig.GetReregistrationDelay() != mConfig.GetReregistrationDelay() ||
             newConfig.GetMlrTimeout() != mConfig.GetMlrTimeout())
    {
        state = kStateRefreshed;
    }
    else
    {
        state = kStateUnchanged;
    }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    LogInfo("PBBR event: %s", StateToString(state));
    mConfig.Log("Old");
    newConfig.Log("New");
#endif

    mConfig = newConfig;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().HandleBackboneRouterPrimaryUpdate(state);
#endif

#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
    Get<Mlr::Manager>().HandleBackboneRouterPrimaryUpdate(state);
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE)
    Get<DuaManager>().HandleBackboneRouterPrimaryUpdate(state);
#endif

    OT_UNUSED_VARIABLE(state);
}

void Leader::UpdateDomainPrefixConfig(void)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    DomainPrefixEvent               event;
    bool                            found = false;

    while (Get<NetworkData::Leader>().GetNext(iterator, prefixConfig) == kErrorNone)
    {
        if (prefixConfig.mDp)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        VerifyOrExit(HasDomainPrefix());

        mDomainPrefix.Clear();
        event = kDomainPrefixRemoved;
    }
    else
    {
        VerifyOrExit(prefixConfig.GetPrefix() != mDomainPrefix);

        event         = HasDomainPrefix() ? kDomainPrefixRefreshed : kDomainPrefixAdded;
        mDomainPrefix = prefixConfig.GetPrefix();
    }

    LogInfo("%s domain Prefix: %s", DomainPrefixEventToString(event), mDomainPrefix.ToString().AsCString());

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<Local>().HandleDomainPrefixUpdate(event);
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    Get<NdProxyTable>().HandleDomainPrefixUpdate(event);
#endif
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE)
    Get<DuaManager>().HandleDomainPrefixUpdate(event);
#endif

exit:
    OT_UNUSED_VARIABLE(event);
}

bool Leader::IsDomainUnicast(const Ip6::Address &aAddress) const
{
    return HasDomainPrefix() && aAddress.MatchesPrefix(mDomainPrefix);
}

} // namespace BackboneRouter
} // namespace ot

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
