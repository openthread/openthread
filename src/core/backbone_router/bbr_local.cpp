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

#include "bbr_local.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "instance/instance.hpp"

namespace ot {

namespace BackboneRouter {

RegisterLogModule("BbrLocal");

Local::Local(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsServiceAdded(false)
    , mState(kStateDisabled)
    , mSequenceNumber(Random::NonCrypto::GenerateUpToExcluding<uint8_t>(127))
    , mRegistrationJitter(kDefaultRegistrationJitter)
    , mReregistrationDelay(kDefaultRegistrationDelay)
    , mRegistrationTimeout(0)
    , mMlrTimeout(kDefaultMlrTimeout)
{
    // Primary Backbone Router Aloc
    mBbrPrimaryAloc.InitAsThreadOriginMeshLocal();

    // All Network Backbone Routers Multicast Address.
    mAllNetworkBackboneRouters.Clear();

    mAllNetworkBackboneRouters.mFields.m8[0]  = 0xff; // Multicast
    mAllNetworkBackboneRouters.mFields.m8[1]  = 0x32; // Flags = 3, Scope = 2
    mAllNetworkBackboneRouters.mFields.m8[15] = 3;    // Group ID = 3
}

void Local::SetEnabled(bool aEnable)
{
    VerifyOrExit(aEnable != IsEnabled());

    if (aEnable)
    {
        SetState(kStateSecondary);
        IgnoreError(AddService(kDecideBasedOnState));
    }
    else
    {
        RemoveService();
        SetState(kStateDisabled);
    }

exit:
    return;
}

void Local::Reset(void)
{
    VerifyOrExit(mState != kStateDisabled);

    RemoveService();

    if (mState == kStatePrimary)
    {
        // Increase sequence number when changing from Primary to Secondary.
        IncrementSequenceNumber();
        Get<Notifier>().Signal(kEventThreadBackboneRouterLocalChanged);
        SetState(kStateSecondary);
    }

exit:
    return;
}

void Local::GetConfig(Config &aConfig) const
{
    aConfig.mSequenceNumber      = mSequenceNumber;
    aConfig.mReregistrationDelay = mReregistrationDelay;
    aConfig.mMlrTimeout          = mMlrTimeout;
}

Error Local::SetConfig(const Config &aConfig)
{
    Error error  = kErrorNone;
    bool  update = false;

#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    VerifyOrExit(aConfig.mMlrTimeout >= kMinMlrTimeout && aConfig.mMlrTimeout <= kMaxMlrTimeout,
                 error = kErrorInvalidArgs);
#endif
    // Validate configuration according to Thread 1.2.1 Specification 5.21.3.3:
    // "The Reregistration Delay in seconds MUST be lower than (0.5 * MLR Timeout). It MUST be at least 1."
    VerifyOrExit(aConfig.mReregistrationDelay >= 1, error = kErrorInvalidArgs);
    static_assert(sizeof(aConfig.mReregistrationDelay) < sizeof(aConfig.mMlrTimeout),
                  "the calculation below might overflow");
    VerifyOrExit(aConfig.mReregistrationDelay * 2 < aConfig.mMlrTimeout, error = kErrorInvalidArgs);

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
        Get<Notifier>().Signal(kEventThreadBackboneRouterLocalChanged);

        IgnoreError(AddService(kDecideBasedOnState));
    }

exit:
    LogService(kActionSet, error);
    return error;
}

Error Local::AddService(RegisterMode aMode)
{
    Error error = kErrorInvalidState;

    VerifyOrExit(mState != kStateDisabled && Get<Mle::Mle>().IsAttached());

    switch (aMode)
    {
    case kDecideBasedOnState:
        VerifyOrExit(!Get<BackboneRouter::Leader>().HasPrimary() ||
                     Get<BackboneRouter::Leader>().GetServer16() == Get<Mle::Mle>().GetRloc16());
        break;
    case kForceRegistration:
        break;
    }

    SuccessOrExit(error = Get<NetworkData::Service::Manager>().AddBackboneRouterService(
                      mSequenceNumber, mReregistrationDelay, mMlrTimeout));
    Get<NetworkData::Notifier>().HandleServerDataUpdated();

    mIsServiceAdded = true;

exit:
    LogService(kActionAdd, error);
    return error;
}

void Local::RemoveService(void)
{
    Error error;

    SuccessOrExit(error = Get<NetworkData::Service::Manager>().RemoveBackboneRouterService());
    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    mIsServiceAdded = false;

exit:
    LogService(kActionRemove, error);
}

void Local::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    switch (mState)
    {
    case kStateDisabled:
        // Update All Network Backbone Routers Multicast Address for both Secondary and Primary state.
        mAllNetworkBackboneRouters.SetMulticastNetworkPrefix(Get<Mle::Mle>().GetMeshLocalPrefix());
        break;
    case kStateSecondary:
        break;
    case kStatePrimary:
        Get<ThreadNetif>().RemoveUnicastAddress(mBbrPrimaryAloc);
        break;
    }

    if (aState == kStatePrimary)
    {
        // Add Primary Backbone Router ALOC for Primary Backbone Router.
        Get<Mle::Mle>().ComposeAloc(Mle::Aloc16::ForPrimaryBackboneRouter(), mBbrPrimaryAloc.GetAddress());
        Get<ThreadNetif>().AddUnicastAddress(mBbrPrimaryAloc);
    }

    mState = aState;

    Get<Notifier>().Signal(kEventThreadBackboneRouterStateChanged);

exit:
    return;
}

void Local::HandleBackboneRouterPrimaryUpdate(PrimaryEvent aEvent)
{
    OT_UNUSED_VARIABLE(aEvent);
    UpdateState();
}

void Local::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        UpdateState();
    }
}

void Local::UpdateState(void)
{
    VerifyOrExit(IsEnabled() && Get<Mle::Mle>().IsAttached());

    // Wait some jitter before trying to Register.
    if (!Get<Leader>().HasPrimary())
    {
        if (Get<Mle::Mle>().IsLeader())
        {
            mRegistrationTimeout = 0;
            Get<TimeTicker>().UnregisterReceiver(TimeTicker::kBbrLocal);
            IgnoreError(AddService(kDecideBasedOnState));
        }
        else
        {
            mRegistrationTimeout = Random::NonCrypto::GenerateInClosedRange<uint16_t>(1, mRegistrationJitter);
            Get<TimeTicker>().RegisterReceiver(TimeTicker::kBbrLocal);
        }
    }
    else if (!Get<Mle::Mle>().HasRloc16(Get<Leader>().GetConfig().GetServer16()))
    {
        Reset();
    }
    else if (!mIsServiceAdded)
    {
        // Here original PBBR restores its Backbone Router Service from Thread Network,
        // Intentionally skips the state update as PBBR will refresh its service.
        mSequenceNumber      = Get<Leader>().GetConfig().mSequenceNumber;
        mReregistrationDelay = Get<Leader>().GetConfig().mReregistrationDelay;
        mMlrTimeout          = Get<Leader>().GetConfig().mMlrTimeout;
        IncrementSequenceNumber();
        Get<Notifier>().Signal(kEventThreadBackboneRouterLocalChanged);
        IgnoreError(AddService(kForceRegistration));
    }
    else
    {
        SetState(kStatePrimary);
    }

exit:
    return;
}

void Local::HandleTimeTick(void)
{
    // Delay registration while router role transition is pending
    // (i.e., device may soon switch from REED to router role).

    VerifyOrExit(!Get<Mle::Mle>().IsRouterRoleTransitionPending());

    if (mRegistrationTimeout > 0)
    {
        mRegistrationTimeout--;

        if (mRegistrationTimeout == 0)
        {
            IgnoreError(AddService(kDecideBasedOnState));
        }
    }

exit:
    if (mRegistrationTimeout == 0)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kBbrLocal);
    }
}

void Local::ApplyNewMeshLocalPrefix(void)
{
    VerifyOrExit(IsEnabled());

    Get<BackboneTmfAgent>().UnsubscribeMulticast(mAllNetworkBackboneRouters);
    mAllNetworkBackboneRouters.SetMulticastNetworkPrefix(Get<Mle::Mle>().GetMeshLocalPrefix());
    Get<BackboneTmfAgent>().SubscribeMulticast(mAllNetworkBackboneRouters);

exit:
    return;
}

void Local::IncrementSequenceNumber(void)
{
    switch (mSequenceNumber)
    {
    case 126:
    case 127:
        mSequenceNumber = 0;
        break;
    case 254:
    case 255:
        mSequenceNumber = 128;
        break;
    default:
        mSequenceNumber++;
        break;
    }
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Local::ActionToString(Action aAction)
{
#define ActionMapList(_) \
    _(kActionSet, "Set") \
    _(kActionAdd, "Add") \
    _(kActionRemove, "Remove")

    DefineEnumStringArray(ActionMapList);

    return kStrings[aAction];
}

void Local::LogService(Action aAction, Error aError)
{
    LogInfo("%s BBR Service: seqno (%u), delay (%us), timeout (%lus), %s", ActionToString(aAction), mSequenceNumber,
            mReregistrationDelay, ToUlong(mMlrTimeout), ErrorToString(aError));
}

#endif

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
