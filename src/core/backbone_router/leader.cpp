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

#include "leader.hpp"

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"

#include "thread/network_data_leader.hpp"
#include "thread/network_data_tlvs.hpp"
#include "thread/thread_netif.hpp"

namespace ot {
namespace BackboneRouter {

Leader::Leader(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    Reset();
}

void Leader::Reset(void)
{
    memset(&mConfig, 0, sizeof(mConfig));
    mConfig.mServer16 = Mac::kShortAddrInvalid;
}

otError Leader::GetConfig(BackboneRouterConfig &aConfig) const
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(HasPrimary(), error = OT_ERROR_NOT_FOUND);

    aConfig = mConfig;

exit:
    return error;
}

otError Leader::GetServiceId(uint8_t &aServiceId) const
{
    otError error       = OT_ERROR_NONE;
    uint8_t serviceData = NetworkData::ServiceTlv::kServiceDataBackboneRouter;

    VerifyOrExit(HasPrimary(), error = OT_ERROR_NOT_FOUND);

    error = Get<NetworkData::Leader>().GetServiceId(NetworkData::ServiceTlv::kThreadEnterpriseNumber, &serviceData,
                                                    sizeof(serviceData), true, aServiceId);

exit:
    return error;
}

void Leader::LogBackboneRouterPrimary(State aState, const BackboneRouterConfig &aConfig) const
{
    OT_UNUSED_VARIABLE(aConfig);

    otLogInfoNetData("BBR state %s", StateToString(aState));

    if (aState != kStateRemoved && aState != kStateNone)
    {
        otLogInfoNetData("Rloc16: 0x%4X, seqno: %d, delay: %d, timeout %d", aConfig.mServer16, aConfig.mSequenceNumber,
                         aConfig.mReregistrationDelay, aConfig.mMlrTimeout);
    }
}

const char *Leader::StateToString(State aState)
{
    const char *logString = "Unknown";

    switch (aState)
    {
    case kStateNone:
        logString = "PBBR: None";
        break;

    case kStateAdded:
        logString = "PBBR: Added";
        break;

    case kStateRemoved:
        logString = "PBBR: Removed";
        break;

    case kStateToTriggerRereg:
        logString = "PBBR: To trigger re-registration";
        break;

    case kStateRefreshed:
        logString = "PBBR: Refreshed";
        break;

    case kStateUnchanged:
        logString = "PBBR: Unchanged";
        break;

    default:
        break;
    }

    return logString;
}

void Leader::Update(void)
{
    BackboneRouterConfig config;
    State                state;

    Get<NetworkData::Leader>().GetBackboneRouterPrimary(config);

    if (config.mServer16 != mConfig.mServer16)
    {
        if (config.mServer16 == Mac::kShortAddrInvalid)
        {
            state = kStateRemoved;
        }
        else if (mConfig.mServer16 == Mac::kShortAddrInvalid)
        {
            state = kStateAdded;
        }
        else
        {
            // Short Address of PBBR changes.
            state = kStateToTriggerRereg;
        }
    }
    else if (config.mServer16 == Mac::kShortAddrInvalid)
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

    mConfig = config;
    LogBackboneRouterPrimary(state, mConfig);

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    Get<BackboneRouter::Local>().UpdateBackboneRouterPrimary(state, mConfig);
#endif
}

} // namespace BackboneRouter
} // namespace ot

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
