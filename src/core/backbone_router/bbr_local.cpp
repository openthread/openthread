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

#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/random.hpp"
#include "instance/instance.hpp"
#include "thread/mle_types.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

namespace BackboneRouter {

RegisterLogModule("BbrLocal");

Local::Local(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsServiceAdded(false)
    , mState(kStateDisabled)
    , mSequenceNumber(Random::NonCrypto::GetUint8() % 127)
    , mRegistrationJitter(kDefaultRegistrationJitter)
    , mReregistrationDelay(kDefaultRegistrationDelay)
    , mRegistrationTimeout(0)
    , mMlrTimeout(kDefaultMlrTimeout)
{
    mDomainPrefixConfig.GetPrefix().SetLength(0);

    // Primary Backbone Router Aloc
    mBbrPrimaryAloc.InitAsThreadOriginMeshLocal();
    mBbrPrimaryAloc.GetAddress().GetIid().SetToLocator(Mle::kAloc16BackboneRouterPrimary);

    // All Network Backbone Routers Multicast Address.
    mAllNetworkBackboneRouters.Clear();

    mAllNetworkBackboneRouters.mFields.m8[0]  = 0xff; // Multicast
    mAllNetworkBackboneRouters.mFields.m8[1]  = 0x32; // Flags = 3, Scope = 2
    mAllNetworkBackboneRouters.mFields.m8[15] = 3;    // Group ID = 3

    // All Domain Backbone Routers Multicast Address.
    mAllDomainBackboneRouters.Clear();

    mAllDomainBackboneRouters.mFields.m8[0]  = 0xff; // Multicast
    mAllDomainBackboneRouters.mFields.m8[1]  = 0x32; // Flags = 3, Scope = 2
    mAllDomainBackboneRouters.mFields.m8[15] = 3;    // Group ID = 3
}

void Local::SetEnabled(bool aEnable)
{
    VerifyOrExit(aEnable != IsEnabled());

    if (aEnable)
    {
        SetState(kStateSecondary);
        AddDomainPrefixToNetworkData();
        IgnoreError(AddService(kDecideBasedOnState));
    }
    else
    {
        RemoveDomainPrefixFromNetworkData();
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
    Error                                            error = kErrorInvalidState;
    NetworkData::Service::BackboneRouter::ServerData serverData;

    VerifyOrExit(mState != kStateDisabled && Get<Mle::Mle>().IsAttached());

    switch (aMode)
    {
    case kDecideBasedOnState:
        VerifyOrExit(!Get<BackboneRouter::Leader>().HasPrimary() ||
                     Get<BackboneRouter::Leader>().GetServer16() == Get<Mle::MleRouter>().GetRloc16());
        break;
    case kForceRegistration:
        break;
    }

    serverData.SetSequenceNumber(mSequenceNumber);
    serverData.SetReregistrationDelay(mReregistrationDelay);
    serverData.SetMlrTimeout(mMlrTimeout);

    SuccessOrExit(error = Get<NetworkData::Service::Manager>().Add<NetworkData::Service::BackboneRouter>(serverData));
    Get<NetworkData::Notifier>().HandleServerDataUpdated();

    mIsServiceAdded = true;

exit:
    LogService(kActionAdd, error);
    return error;
}

void Local::RemoveService(void)
{
    Error error;

    SuccessOrExit(error = Get<NetworkData::Service::Manager>().Remove<NetworkData::Service::BackboneRouter>());
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
        mAllNetworkBackboneRouters.SetMulticastNetworkPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
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
        mBbrPrimaryAloc.GetAddress().SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
        Get<ThreadNetif>().AddUnicastAddress(mBbrPrimaryAloc);
    }

    mState = aState;

    Get<Notifier>().Signal(kEventThreadBackboneRouterStateChanged);

exit:
    return;
}

void Local::HandleBackboneRouterPrimaryUpdate(Leader::State aState, const Config &aConfig)
{
    OT_UNUSED_VARIABLE(aState);

    VerifyOrExit(IsEnabled() && Get<Mle::MleRouter>().IsAttached());

    // Wait some jitter before trying to Register.
    if (aConfig.mServer16 == Mac::kShortAddrInvalid)
    {
        mRegistrationTimeout = 1;

        if (!Get<Mle::MleRouter>().IsLeader())
        {
            mRegistrationTimeout +=
                Random::NonCrypto::GetUint16InRange(0, static_cast<uint16_t>(mRegistrationJitter) + 1);
        }

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kBbrLocal);
    }
    else if (aConfig.mServer16 != Get<Mle::MleRouter>().GetRloc16())
    {
        Reset();
    }
    else if (!mIsServiceAdded)
    {
        // Here original PBBR restores its Backbone Router Service from Thread Network,
        // Intentionally skips the state update as PBBR will refresh its service.
        mSequenceNumber      = aConfig.mSequenceNumber;
        mReregistrationDelay = aConfig.mReregistrationDelay;
        mMlrTimeout          = aConfig.mMlrTimeout;
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

    VerifyOrExit(!Get<Mle::MleRouter>().IsRouterRoleTransitionPending());

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

Error Local::GetDomainPrefix(NetworkData::OnMeshPrefixConfig &aConfig)
{
    Error error = kErrorNone;

    VerifyOrExit(mDomainPrefixConfig.GetPrefix().GetLength() > 0, error = kErrorNotFound);

    aConfig = mDomainPrefixConfig;

exit:
    return error;
}

Error Local::RemoveDomainPrefix(const Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(aPrefix.GetLength() > 0, error = kErrorInvalidArgs);
    VerifyOrExit(mDomainPrefixConfig.GetPrefix() == aPrefix, error = kErrorNotFound);

    if (IsEnabled())
    {
        RemoveDomainPrefixFromNetworkData();
    }

    mDomainPrefixConfig.GetPrefix().SetLength(0);

exit:
    return error;
}

Error Local::SetDomainPrefix(const NetworkData::OnMeshPrefixConfig &aConfig)
{
    Error error = kErrorNone;

    VerifyOrExit(aConfig.IsValid(GetInstance()), error = kErrorInvalidArgs);

    if (IsEnabled())
    {
        RemoveDomainPrefixFromNetworkData();
    }

    mDomainPrefixConfig = aConfig;
    LogDomainPrefix(kActionSet, kErrorNone);

    if (IsEnabled())
    {
        AddDomainPrefixToNetworkData();
    }

exit:
    return error;
}

void Local::ApplyNewMeshLocalPrefix(void)
{
    VerifyOrExit(IsEnabled());

    Get<BackboneTmfAgent>().UnsubscribeMulticast(mAllNetworkBackboneRouters);
    mAllNetworkBackboneRouters.SetMulticastNetworkPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
    Get<BackboneTmfAgent>().SubscribeMulticast(mAllNetworkBackboneRouters);

exit:
    return;
}

void Local::HandleDomainPrefixUpdate(DomainPrefixEvent aEvent)
{
    if (!IsEnabled())
    {
        ExitNow();
    }

    if (aEvent == kDomainPrefixRemoved || aEvent == kDomainPrefixRefreshed)
    {
        Get<BackboneTmfAgent>().UnsubscribeMulticast(mAllDomainBackboneRouters);
    }

    if (aEvent == kDomainPrefixAdded || aEvent == kDomainPrefixRefreshed)
    {
        mAllDomainBackboneRouters.SetMulticastNetworkPrefix(*Get<Leader>().GetDomainPrefix());
        Get<BackboneTmfAgent>().SubscribeMulticast(mAllDomainBackboneRouters);
    }

    if (aEvent != kDomainPrefixUnchanged)
    {
        mDomainPrefixCallback.InvokeIfSet(static_cast<otBackboneRouterDomainPrefixEvent>(aEvent),
                                          Get<Leader>().GetDomainPrefix());
    }

exit:
    return;
}

void Local::RemoveDomainPrefixFromNetworkData(void)
{
    Error error = kErrorNotFound; // only used for logging.

    if (mDomainPrefixConfig.mPrefix.mLength > 0)
    {
        error = Get<NetworkData::Local>().RemoveOnMeshPrefix(mDomainPrefixConfig.GetPrefix());
    }

    LogDomainPrefix(kActionRemove, error);
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

void Local::AddDomainPrefixToNetworkData(void)
{
    Error error = kErrorNotFound; // only used for logging.

    if (mDomainPrefixConfig.GetPrefix().GetLength() > 0)
    {
        error = Get<NetworkData::Local>().AddOnMeshPrefix(mDomainPrefixConfig);
    }

    LogDomainPrefix(kActionAdd, error);
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *Local::ActionToString(Action aAction)
{
    static const char *const kActionStrings[] = {
        "Set",    // (0) kActionSet
        "Add",    // (1) kActionAdd
        "Remove", // (2) kActionRemove
    };

    static_assert(0 == kActionSet, "kActionSet value is incorrect");
    static_assert(1 == kActionAdd, "kActionAdd value is incorrect");
    static_assert(2 == kActionRemove, "kActionRemove value is incorrect");

    return kActionStrings[aAction];
}

void Local::LogDomainPrefix(Action aAction, Error aError)
{
    LogInfo("%s Domain Prefix: %s, %s", ActionToString(aAction), mDomainPrefixConfig.GetPrefix().ToString().AsCString(),
            ErrorToString(aError));
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
