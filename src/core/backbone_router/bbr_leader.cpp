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

Leader::Leader(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    Reset();
}

void Leader::Reset(void)
{
    // Invalid server short address indicates no available Backbone Router service in the Thread Network.
    mConfig.mServer16 = Mle::kInvalidRloc16;

    // Domain Prefix Length 0 indicates no available Domain Prefix in the Thread network.
    mDomainPrefix.SetLength(0);
}

Error Leader::GetConfig(Config &aConfig) const
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

void Leader::LogBackboneRouterPrimary(State aState, const Config &aConfig) const
{
    OT_UNUSED_VARIABLE(aConfig);

    LogInfo("PBBR state: %s", StateToString(aState));

    if (aState != kStateRemoved && aState != kStateNone)
    {
        LogInfo("Rloc16:0x%4x, seqno:%u, delay:%u, timeout:%lu", aConfig.mServer16, aConfig.mSequenceNumber,
                aConfig.mReregistrationDelay, ToUlong(aConfig.mMlrTimeout));
    }
}

const char *Leader::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "None",            //  (0) kStateNone
        "Added",           //  (1) kStateAdded
        "Removed",         //  (2) kStateRemoved
        "Rereg triggered", //  (3) kStateToTriggerRereg
        "Refreshed",       //  (4) kStateRefreshed
        "Unchanged",       //  (5) kStateUnchanged
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateNone);
        ValidateNextEnum(kStateAdded);
        ValidateNextEnum(kStateRemoved);
        ValidateNextEnum(kStateToTriggerRereg);
        ValidateNextEnum(kStateRefreshed);
        ValidateNextEnum(kStateUnchanged);
    };

    return kStateStrings[aState];
}

const char *Leader::DomainPrefixEventToString(DomainPrefixEvent aEvent)
{
    static const char *const kEventStrings[] = {
        "Added",     // (0) kDomainPrefixAdded
        "Removed",   // (1) kDomainPrefixRemoved
        "Refreshed", // (2) kDomainPrefixRefreshed
        "Unchanged", // (3) kDomainPrefixUnchanged
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kDomainPrefixAdded);
        ValidateNextEnum(kDomainPrefixRemoved);
        ValidateNextEnum(kDomainPrefixRefreshed);
        ValidateNextEnum(kDomainPrefixUnchanged);
    };

    return kEventStrings[aEvent];
}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Leader::Update(void)
{
    UpdateBackboneRouterPrimary();
    UpdateDomainPrefixConfig();
}

void Leader::UpdateBackboneRouterPrimary(void)
{
    Config config;
    State  state;

    Get<NetworkData::Service::Manager>().GetBackboneRouterPrimary(config);

    if (config.mServer16 != mConfig.mServer16)
    {
        if (config.mServer16 == Mle::kInvalidRloc16)
        {
            state = kStateRemoved;
        }
        else if (mConfig.mServer16 == Mle::kInvalidRloc16)
        {
            state = kStateAdded;
        }
        else
        {
            // Short Address of PBBR changes.
            state = kStateToTriggerRereg;
        }
    }
    else if (config.mServer16 == Mle::kInvalidRloc16)
    {
        // If no Primary all the time.
        state = kStateNone;
    }
    else if (config.mSequenceNumber != mConfig.mSequenceNumber)
    {
        state = kStateToTriggerRereg;
    }
    else if (config.mReregistrationDelay != mConfig.mReregistrationDelay || config.mMlrTimeout != mConfig.mMlrTimeout)
    {
        state = kStateRefreshed;
    }
    else
    {
        state = kStateUnchanged;
    }

    // Restrain the range of MLR timeout to be always valid
    if (config.mServer16 != Mle::kInvalidRloc16)
    {
        uint32_t origTimeout = config.mMlrTimeout;

        config.mMlrTimeout = Clamp(config.mMlrTimeout, kMinMlrTimeout, kMaxMlrTimeout);

        if (config.mMlrTimeout != origTimeout)
        {
            LogNote("Leader MLR Timeout is normalized from %lu to %lu", ToUlong(origTimeout),
                    ToUlong(config.mMlrTimeout));
        }
    }

    mConfig = config;
    LogBackboneRouterPrimary(state, mConfig);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().HandleBackboneRouterPrimaryUpdate(state, mConfig);
#endif

#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
    Get<MlrManager>().HandleBackboneRouterPrimaryUpdate(state, mConfig);
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE)
    Get<DuaManager>().HandleBackboneRouterPrimaryUpdate(state, mConfig);
#endif
}

void Leader::UpdateDomainPrefixConfig(void)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig config;
    DomainPrefixEvent               event;
    bool                            found = false;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == kErrorNone)
    {
        if (config.mDp)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        VerifyOrExit(HasDomainPrefix());

        // Domain Prefix does not exist any more.
        mDomainPrefix.Clear();
        event = kDomainPrefixRemoved;
    }
    else if (config.GetPrefix() == mDomainPrefix)
    {
        event = kDomainPrefixUnchanged;
    }
    else
    {
        event         = HasDomainPrefix() ? kDomainPrefixRefreshed : kDomainPrefixAdded;
        mDomainPrefix = config.GetPrefix();
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
#else
    OT_UNUSED_VARIABLE(event);
#endif

exit:
    return;
}

bool Leader::IsDomainUnicast(const Ip6::Address &aAddress) const
{
    return HasDomainPrefix() && aAddress.MatchesPrefix(mDomainPrefix);
}

} // namespace BackboneRouter
} // namespace ot

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
