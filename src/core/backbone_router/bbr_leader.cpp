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
    return Random::NonCrypto::GenerateInClosedRange<uint16_t>(1, mReregistrationDelay);
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

void Leader::Reset(void) { mConfig.MarkAsAbsent(); }

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

const char *Leader::PrimaryEventToString(PrimaryEvent aEvent)
{
#define PrimaryEventMapList(_)              \
    _(kPrimaryAdded, "Added")               \
    _(kPrimaryRemoved, "Removed")           \
    _(kPrimaryUpdatedReregister, "Updated") \
    _(kPrimaryConfigParameterChanged, "ConfigChanged")

    DefineEnumStringArray(PrimaryEventMapList);

    return kStrings[aEvent];
}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Leader::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadRoleChanged))
    {
        UpdateBackboneRouterPrimary();
    }
}

void Leader::UpdateBackboneRouterPrimary(void)
{
    Config       newConfig;
    PrimaryEvent event;

    Get<NetworkData::Service::Manager>().GetBackboneRouterPrimary(newConfig);

    newConfig.AdjustMlrTimeout();

    if (!mConfig.IsPresent())
    {
        VerifyOrExit(newConfig.IsPresent());
        event = kPrimaryAdded;
    }
    else if (!newConfig.IsPresent())
    {
        event = kPrimaryRemoved;
    }
    else if (newConfig.GetServer16() != mConfig.GetServer16() ||
             newConfig.GetSequenceNumber() != mConfig.GetSequenceNumber())
    {
        event = kPrimaryUpdatedReregister;
    }
    else if (newConfig.GetReregistrationDelay() != mConfig.GetReregistrationDelay() ||
             newConfig.GetMlrTimeout() != mConfig.GetMlrTimeout())
    {
        event = kPrimaryConfigParameterChanged;
    }
    else
    {
        ExitNow(); // No changes
    }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    LogInfo("PrimaryEvent: %s", PrimaryEventToString(event));
    mConfig.Log("Old");
    newConfig.Log("New");
#endif

    mConfig = newConfig;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().HandleBackboneRouterPrimaryUpdate(event);
#endif

#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)
    Get<Mlr::Manager>().HandleBackboneRouterPrimaryUpdate(event);
#endif

exit:
    OT_UNUSED_VARIABLE(event);
}

} // namespace BackboneRouter
} // namespace ot

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
