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
#include "thread/mle_constants.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

namespace BackboneRouter {

Local::Local(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsServiceAdded(false)
    , mState(OT_BACKBONE_ROUTER_STATE_DISABLED)
    , mSequenceNumber(Random::NonCrypto::GetUint8())
    , mReregistrationDelay(Mle::kRegistrationDelayDefault)
    , mMlrTimeout(Mle::kMlrTimeoutDefault)
{
    // Primary Backbone Router Aloc
    memset(&mBackboneRouterPrimaryAloc, 0, sizeof(mBackboneRouterPrimaryAloc));

    mBackboneRouterPrimaryAloc.mPrefixLength       = 128;
    mBackboneRouterPrimaryAloc.mPreferred          = true;
    mBackboneRouterPrimaryAloc.mValid              = true;
    mBackboneRouterPrimaryAloc.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mBackboneRouterPrimaryAloc.mScopeOverrideValid = true;
}

void Local::SetEnabled(bool aEnable)
{
    VerifyOrExit(aEnable == (mState == OT_BACKBONE_ROUTER_STATE_DISABLED));

    if (aEnable)
    {
        AddService();
        SetState(OT_BACKBONE_ROUTER_STATE_SECONDARY);
    }
    else if (mState == OT_BACKBONE_ROUTER_STATE_PRIMARY)
    {
        RemoveService();
        Get<ThreadNetif>().RemoveUnicastAddress(mBackboneRouterPrimaryAloc);
        SetState(OT_BACKBONE_ROUTER_STATE_DISABLED);
    }

exit:
    return;
}

void Local::Reset(void)
{
    if (mState == OT_BACKBONE_ROUTER_STATE_PRIMARY)
    {
        RemoveService();
        Get<ThreadNetif>().RemoveUnicastAddress(mBackboneRouterPrimaryAloc);
        SetState(OT_BACKBONE_ROUTER_STATE_SECONDARY);
    }
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
        IgnoreReturnValue(AddService());
    }

    otLogDebgNetData("BBR local: seqno (%d), delay (%ds), timeout (%ds)", mSequenceNumber, mReregistrationDelay,
                     mMlrTimeout);
}

otError Local::AddService(bool aForce)
{
    otError                               error       = OT_ERROR_INVALID_STATE;
    uint8_t                               serviceData = NetworkData::ServiceTlv::kServiceDataBackboneRouter;
    NetworkData::BackboneRouterServerData serverData;

    VerifyOrExit(mState != OT_BACKBONE_ROUTER_STATE_DISABLED && Get<Mle::Mle>().IsAttached());

    // If not by force, not register unless 1) there is no avilable Backbone Router service, or 2) it is PBBR itself
    // refresh its dataset.
    VerifyOrExit(aForce || !Get<BackboneRouter::Leader>().HasPrimary() || mState == OT_BACKBONE_ROUTER_STATE_PRIMARY);

    serverData.SetSequenceNumber(mSequenceNumber);
    serverData.SetReregistrationDelay(mReregistrationDelay);
    serverData.SetMlrTimeout(mMlrTimeout);

    error = Get<NetworkData::Local>().AddService(THREAD_ENTERPRISE_NUMBER, &serviceData, sizeof(serviceData), true,
                                                 reinterpret_cast<const uint8_t *>(&serverData), sizeof(serverData));
exit:
    if (error == OT_ERROR_NONE)
    {
        mIsServiceAdded = true;
        otLogInfoNetData("BBR Service added: seqno (%d), delay (%ds), timeout (%ds)", mSequenceNumber,
                         mReregistrationDelay, mMlrTimeout);
        error = Get<NetworkData::Local>().SendServerDataNotification();
    }

    otLogInfoNetData("Add BBR Service: %s", otThreadErrorToString(error));
    return error;
}

otError Local::RemoveService(void)
{
    otError error       = OT_ERROR_NONE;
    uint8_t serviceData = NetworkData::ServiceTlv::kServiceDataBackboneRouter;

    error = Get<NetworkData::Local>().RemoveService(THREAD_ENTERPRISE_NUMBER, &serviceData, sizeof(serviceData));
    SuccessOrExit(error);

    error = Get<NetworkData::Local>().SendServerDataNotification();

exit:
    otLogInfoNetData("Remove BBR Service %s", otThreadErrorToString(error));
    return error;
}

otError Local::AddBackboneRouterPrimaryAloc(void)
{
    memcpy(&mBackboneRouterPrimaryAloc.GetAddress(), &Get<Mle::MleRouter>().GetMeshLocal16(), 14);
    mBackboneRouterPrimaryAloc.GetAddress().mFields.m16[7] = HostSwap16(Mle::kAloc16BackboneRouterPrimary);
    return Get<ThreadNetif>().AddUnicastAddress(mBackboneRouterPrimaryAloc);
}

void Local::NotifyBackboneRouterPrimaryUpdate(Leader::State aState, const BackboneRouterConfig &aConfig)
{
    OT_UNUSED_VARIABLE(aConfig);

    switch (aState)
    {
    case Leader::kStateToTriggerRereg:
        // fall through
    case Leader::kStateAdded:
        if (Get<BackboneRouter::Leader>().GetServer16() == Get<Mle::MleRouter>().GetRloc16())
        {
            if (mState == OT_BACKBONE_ROUTER_STATE_SECONDARY)
            {
                if (!mIsServiceAdded)
                {
                    // Here original PBBR restores its Backbone Rouetr Service from Thread Network,
                    // Intentionally skips the state update as PBBR will refresh its service.
                    mSequenceNumber      = aConfig.mSequenceNumber + 1;
                    mReregistrationDelay = aConfig.mReregistrationDelay;
                    mMlrTimeout          = aConfig.mMlrTimeout;
                    AddService(true /* Force registration to refresh and restore Primary state */);
                }
                else
                {
                    AddBackboneRouterPrimaryAloc();
                    SetState(OT_BACKBONE_ROUTER_STATE_PRIMARY);
                }
            }
        }
        else if (mState == OT_BACKBONE_ROUTER_STATE_PRIMARY)
        {
            Reset();
            // Here increases sequence number if becomes Secondary from Primary.
            mSequenceNumber++;
        }
        break;
    default:
        break;
    }
}

void Local::SetState(BackboneRouterState aState)
{
    VerifyOrExit(mState != aState);

    mState = aState;
    Get<Notifier>().Signal(OT_CHANGED_THREAD_BACKBONE_ROUTER_STATE);

exit:
    return;
}

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
