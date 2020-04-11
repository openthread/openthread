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
 *   This file implements local Backbone Router service.
 */

#include "local.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "thread/mle.hpp"
#include "thread/mle_types.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

namespace BackboneRouter {

Local::Local(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(OT_BACKBONE_ROUTER_STATE_DISABLED)
    , mMlrTimeout(Mle::kMlrTimeoutDefault)
    , mReregistrationDelay(Mle::kRegistrationDelayDefault)
    , mSequenceNumber(Random::NonCrypto::GetUint8())
    , mRegistrationJitter(Mle::kBackboneRouterRegistrationJitter)
    , mIsServiceAdded(false)
{
    mDomainPrefixConfig.mPrefix.mLength = 0;
}

void Local::SetEnabled(bool aEnable)
{
    VerifyOrExit(aEnable == (mState == OT_BACKBONE_ROUTER_STATE_DISABLED));

    if (aEnable)
    {
        SetState(OT_BACKBONE_ROUTER_STATE_SECONDARY);
        AddDomainPrefixToNetworkData();
        AddService();
    }
    else
    {
        RemoveDomainPrefixFromNetworkData();
        RemoveService();
        SetState(OT_BACKBONE_ROUTER_STATE_DISABLED);
    }

    Get<NetworkData::Notifier>().HandleServerDataUpdated();

exit:
    return;
}

void Local::Reset(void)
{
    VerifyOrExit(mState != OT_BACKBONE_ROUTER_STATE_DISABLED);

    if (RemoveService() == OT_ERROR_NONE)
    {
        Get<NetworkData::Notifier>().HandleServerDataUpdated();
    }

    if (mState == OT_BACKBONE_ROUTER_STATE_PRIMARY)
    {
        // Increase sequence number when changing from Primary to Secondary.
        mSequenceNumber++;
        Get<Notifier>().Signal(OT_CHANGED_THREAD_BACKBONE_ROUTER_LOCAL);
        SetState(OT_BACKBONE_ROUTER_STATE_SECONDARY);
    }

exit:
    return;
}

void Local::GetConfig(BackboneRouterConfig &aConfig) const
{
    aConfig.mSequenceNumber      = mSequenceNumber;
    aConfig.mReregistrationDelay = mReregistrationDelay;
    aConfig.mMlrTimeout          = mMlrTimeout;
}

void Local::SetConfig(const BackboneRouterConfig &aConfig)
{
    bool update = false;

    if (aConfig.mReregistrationDelay != mReregistrationDelay)
    {
        mReregistrationDelay = aConfig.mReregistrationDelay;
        update               = true;
    }

    if (aConfig.mMlrTimeout != mMlrTimeout)
    {
        mMlrTimeout = aConfig.mMlrTimeout;
        update      = true;
    }

    if (aConfig.mSequenceNumber != mSequenceNumber)
    {
        mSequenceNumber = aConfig.mSequenceNumber;
        update          = true;
    }

    if (update)
    {
        Get<Notifier>().Signal(OT_CHANGED_THREAD_BACKBONE_ROUTER_LOCAL);

        if (AddService() == OT_ERROR_NONE)
        {
            Get<NetworkData::Notifier>().HandleServerDataUpdated();
        }
    }

    LogBackboneRouterService("Set", OT_ERROR_NONE);
}

otError Local::AddService(bool aForce)
{
    otError                               error       = OT_ERROR_INVALID_STATE;
    uint8_t                               serviceData = NetworkData::ServiceTlv::kServiceDataBackboneRouter;
    NetworkData::BackboneRouterServerData serverData;

    VerifyOrExit(mState != OT_BACKBONE_ROUTER_STATE_DISABLED && Get<Mle::Mle>().IsAttached());

    VerifyOrExit(aForce /* if register by force */ ||
                 !Get<BackboneRouter::Leader>().HasPrimary() /* if no available Backbone Router service */ ||
                 Get<BackboneRouter::Leader>().GetServer16() == Get<Mle::MleRouter>().GetRloc16()
                 /* If the device itself should be BBR. */
    );

    serverData.SetSequenceNumber(mSequenceNumber);
    serverData.SetReregistrationDelay(mReregistrationDelay);
    serverData.SetMlrTimeout(mMlrTimeout);

    SuccessOrExit(error = Get<NetworkData::Local>().AddService(
                      THREAD_ENTERPRISE_NUMBER, &serviceData, sizeof(serviceData), true,
                      reinterpret_cast<const uint8_t *>(&serverData), sizeof(serverData)));

    mIsServiceAdded = true;

exit:
    LogBackboneRouterService("Add", error);
    return error;
}

otError Local::RemoveService(void)
{
    otError error;
    uint8_t serviceData = NetworkData::ServiceTlv::kServiceDataBackboneRouter;

    SuccessOrExit(
        error = Get<NetworkData::Local>().RemoveService(THREAD_ENTERPRISE_NUMBER, &serviceData, sizeof(serviceData)));

    mIsServiceAdded = false;

exit:
    LogBackboneRouterService("Remove", error);
    return error;
}

void Local::SetState(BackboneRouterState aState)
{
    VerifyOrExit(mState != aState);

    mState = aState;
    Get<Notifier>().Signal(OT_CHANGED_THREAD_BACKBONE_ROUTER_STATE);

exit:
    return;
}

void Local::UpdateBackboneRouterPrimary(Leader::State aState, const BackboneRouterConfig &aConfig)
{
    OT_UNUSED_VARIABLE(aState);

    VerifyOrExit(mState != OT_BACKBONE_ROUTER_STATE_DISABLED && Get<Mle::MleRouter>().IsAttached());

    // Wait some jitter before trying to Register.
    if (aConfig.mServer16 == Mac::kShortAddrInvalid)
    {
        uint8_t delay = 1;

        if (!Get<Mle::MleRouter>().IsLeader())
        {
            delay += Random::NonCrypto::GetUint8InRange(0, mRegistrationJitter < 255 ? mRegistrationJitter + 1
                                                                                     : mRegistrationJitter);
        }

        // Here uses the timer resource in Mle.
        Get<Mle::MleRouter>().SetBackboneRouterRegistrationDelay(delay);
    }
    else if (aConfig.mServer16 != Get<Mle::MleRouter>().GetRloc16())
    {
        Reset();
    }
    else if (!mIsServiceAdded)
    {
        // Here original PBBR restores its Backbone Router Service from Thread Network,
        // Intentionally skips the state update as PBBR will refresh its service.
        mSequenceNumber      = aConfig.mSequenceNumber + 1;
        mReregistrationDelay = aConfig.mReregistrationDelay;
        mMlrTimeout          = aConfig.mMlrTimeout;
        Get<Notifier>().Signal(OT_CHANGED_THREAD_BACKBONE_ROUTER_LOCAL);
        if (AddService(true /* Force registration to refresh and restore Primary state */) == OT_ERROR_NONE)
        {
            Get<NetworkData::Notifier>().HandleServerDataUpdated();
        }
    }
    else
    {
        SetState(OT_BACKBONE_ROUTER_STATE_PRIMARY);
    }

exit:
    return;
}

otError Local::GetDomainPrefix(NetworkData::OnMeshPrefixConfig &aConfig)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mDomainPrefixConfig.mPrefix.mLength > 0, error = OT_ERROR_NOT_FOUND);

    aConfig = mDomainPrefixConfig;

exit:
    return error;
}

otError Local::RemoveDomainPrefix(const otIp6Prefix &aPrefix)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aPrefix.mLength > 0, error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(mDomainPrefixConfig.mPrefix.mLength == aPrefix.mLength, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit(Ip6::Address::PrefixMatch(mDomainPrefixConfig.mPrefix.mPrefix.mFields.m8, aPrefix.mPrefix.mFields.m8,
                                           BitVectorBytes(aPrefix.mLength)) >= aPrefix.mLength,
                 error = OT_ERROR_NOT_FOUND);

    if (IsEnabled())
    {
        RemoveDomainPrefixFromNetworkData();
    }

    mDomainPrefixConfig.mPrefix.mLength = 0;

exit:
    return error;
}

void Local::SetDomainPrefix(const NetworkData::OnMeshPrefixConfig &aConfig)
{
    if (IsEnabled())
    {
        RemoveDomainPrefixFromNetworkData();
    }

    mDomainPrefixConfig = aConfig;
    LogDomainPrefix("Set", OT_ERROR_NONE);

    if (IsEnabled())
    {
        AddDomainPrefixToNetworkData();
    }
}

void Local::RemoveDomainPrefixFromNetworkData(void)
{
    otError error = OT_ERROR_NOT_FOUND; // only used for logging.

    if (mDomainPrefixConfig.mPrefix.mLength > 0)
    {
        error = Get<NetworkData::Local>().RemoveOnMeshPrefix(mDomainPrefixConfig.mPrefix.mPrefix.mFields.m8,
                                                             mDomainPrefixConfig.mPrefix.mLength);
    }

    LogDomainPrefix("Remove", error);
}

void Local::AddDomainPrefixToNetworkData(void)
{
    otError error = OT_ERROR_NOT_FOUND; // only used for logging.

    if (mDomainPrefixConfig.mPrefix.mLength > 0)
    {
        error = Get<NetworkData::Local>().AddOnMeshPrefix(mDomainPrefixConfig);
    }

    LogDomainPrefix("Add", error);
}

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_NETDATA == 1)
void Local::LogDomainPrefix(const char *aAction, otError aError)
{
    otLogInfoNetData("%s Domain Prefix: %s/%d, %s", aAction,
                     mDomainPrefixConfig.mPrefix.mLength > 0
                         ? (*static_cast<Ip6::Address *>(&mDomainPrefixConfig.mPrefix.mPrefix)).ToString().AsCString()
                         : "",
                     mDomainPrefixConfig.mPrefix.mLength, otThreadErrorToString(aError));
}

void Local::LogBackboneRouterService(const char *aAction, otError aError)
{
    otLogInfoNetData("%s BBR Service: seqno (%d), delay (%ds), timeout (%ds), %s", aAction, mSequenceNumber,
                     mReregistrationDelay, mMlrTimeout, otThreadErrorToString(aError));
}
#endif

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
