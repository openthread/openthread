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
 *   This file implements MLR management.
 */

#include "mlr_manager.hpp"

#if OPENTHREAD_CONFIG_MLR_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE)

#include "instance/instance.hpp"

namespace ot {
namespace Mlr {

RegisterLogModule("MlrManager");

Manager::Manager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mReregistrationDelay(0)
    , mSendDelay(0)
    , mPending(false)
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    , mRegisterPending(false)
#endif
{
}

void Manager::HandleNotifierEvents(Events aEvents)
{
#if OPENTHREAD_CONFIG_MLR_ENABLE
    if (aEvents.Contains(kEventIp6MulticastSubscribed))
    {
        UpdateLocalSubscriptions();
    }
#endif

    if (aEvents.Contains(kEventThreadRoleChanged) && Get<Mle::Mle>().IsChild())
    {
        ScheduleNextRegistration(kReregister);
    }
}

void Manager::HandleBackboneRouterPrimaryUpdate(BackboneRouter::Leader::State aState,
                                                const BackboneRouter::Config &aConfig)
{
    OT_UNUSED_VARIABLE(aConfig);

    RegistrationRequest request = kRenew;

    switch (aState)
    {
    case BackboneRouter::Leader::kStateAdded:
    case BackboneRouter::Leader::kStateToTriggerRereg:
        request = kReregister;
        break;
    default:
        break;
    }

    ScheduleNextRegistration(request);
}

#if OPENTHREAD_CONFIG_MLR_ENABLE
void Manager::UpdateLocalSubscriptions(void)
{
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    // Check multicast addresses are newly listened against Children
    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.Matches(kStateToRegister) && IsAddressRegisteredByAnyChild(addr.GetAddress()))
        {
            addr.SetMlrState(kStateRegistered);
        }
    }
#endif

    ScheduleSend(0);
}

bool Manager::IsAddressRegisteredByNetif(const Ip6::Address &aAddress) const
{
    bool ret = false;

    OT_ASSERT(aAddress.IsMulticastLargerThanRealmLocal());

    for (const Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.Matches(kStateRegistered) && (addr.GetAddress() == aAddress))
        {
            ret = true;
            break;
        }
    }

    return ret;
}

#endif // OPENTHREAD_CONFIG_MLR_ENABLE

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

bool Manager::IsAddressRegisteredByAnyChildExcept(const Ip6::Address &aAddress, const Child *aExceptChild) const
{
    bool ret = false;

    OT_ASSERT(aAddress.IsMulticastLargerThanRealmLocal());

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (&child != aExceptChild && child.HasMlrRegisteredAddress(aAddress))
        {
            ExitNow(ret = true);
        }
    }

exit:
    return ret;
}

void Manager::UpdateProxiedSubscriptions(Child &aChild, const ChildAddressArray &aOldRegisteredAddresses)
{
    VerifyOrExit(aChild.IsStateValid());

    // Search the new multicast addresses and set its flag accordingly
    for (Child::Ip6AddrEntry &addrEntry : aChild.GetIp6Addresses())
    {
        bool isRegistered;

        if (!addrEntry.IsMulticastLargerThanRealmLocal())
        {
            continue;
        }

        isRegistered = aOldRegisteredAddresses.Contains(addrEntry);

#if OPENTHREAD_CONFIG_MLR_ENABLE
        // Check if it's a new multicast address against parent Netif
        isRegistered = isRegistered || IsAddressRegisteredByNetif(addrEntry);
#endif
        // Check if it's a new multicast address against other Children
        isRegistered = isRegistered || IsAddressRegisteredByAnyChildExcept(addrEntry, &aChild);

        addrEntry.SetMlrState(isRegistered ? kStateRegistered : kStateToRegister, aChild);
    }

exit:
    LogMulticastAddresses();

    if (aChild.HasAnyMlrToRegisterAddress())
    {
        ScheduleSend(Random::NonCrypto::GetUint16InRange(1, BackboneRouter::kParentAggregateDelay));
    }
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

void Manager::ScheduleSend(uint16_t aDelay)
{
    OT_ASSERT(!mPending || mSendDelay == 0);

    VerifyOrExit(!mPending);

    if (aDelay == 0)
    {
        mSendDelay = 0;
        Send();
    }
    else if (mSendDelay == 0 || mSendDelay > aDelay)
    {
        mSendDelay = aDelay;
    }

    UpdateTimeTickerRegistration();
exit:
    return;
}

void Manager::UpdateTimeTickerRegistration(void)
{
    if (mSendDelay == 0 && mReregistrationDelay == 0)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMlrManager);
    }
    else
    {
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMlrManager);
    }
}

bool Manager::ShouldRegister(void) const
{
    bool shouldRegister = false;

    VerifyOrExit(Get<Mle::Mle>().IsFullThreadDevice() || Get<Mle::Mle>().GetParent().IsThreadVersion1p1());
    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary());

    shouldRegister = true;

exit:
    return shouldRegister;
}

void Manager::Send(void)
{
    Error        error;
    AddressArray addresses;

    VerifyOrExit(!mPending, error = kErrorBusy);

    VerifyOrExit(Get<Mle::Mle>().IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(ShouldRegister(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_MLR_ENABLE
    // Append Netif multicast addresses
    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addresses.IsFull())
        {
            break;
        }

        if (addr.Matches(kStateToRegister))
        {
            IgnoreError(addresses.AddUnique(addr.GetAddress()));
            addr.SetMlrState(kStateRegistering);
        }
    }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    // Append Child multicast addresses
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (addresses.IsFull())
        {
            break;
        }

        if (!child.HasAnyMlrToRegisterAddress())
        {
            continue;
        }

        for (Child::Ip6AddrEntry &addrEntry : child.GetIp6Addresses())
        {
            if (!addrEntry.IsMulticastLargerThanRealmLocal())
            {
                continue;
            }

            if (addresses.IsFull())
            {
                break;
            }

            if (addrEntry.GetMlrState(child) == kStateToRegister)
            {
                IgnoreError(addresses.AddUnique(addrEntry));
                addrEntry.SetMlrState(kStateRegistering, child);
            }
        }
    }
#endif

    VerifyOrExit(!addresses.IsEmpty(), error = kErrorNotFound);
    SuccessOrExit(error = SendMessage(addresses.GetArrayBuffer(), addresses.GetLength(), nullptr, HandleResponse));

    mPending = true;

    // Generally Thread 1.2 Router would send MLR.req on behalf for MA (scope >=4) subscribed by its MTD child.
    // When Thread 1.2 MTD attaches to Thread 1.1 parent, 1.2 MTD should send MLR.req to PBBR itself.
    // In this case, Thread 1.2 sleepy end device relies on fast data poll to fetch the response timely.
    if (!Get<Mle::Mle>().IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SendFastPolls();
    }

exit:
    if (error != kErrorNone)
    {
        SetMulticastAddressState(kStateRegistering, kStateToRegister);

        if (error == kErrorNoBufs)
        {
            ScheduleSend(1);
        }
    }

    LogMulticastAddresses();
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
Error Manager::RegisterMulticastListeners(const Ip6::Address *aAddresses,
                                          uint8_t             aAddressNum,
                                          const uint32_t     *aTimeout,
                                          RegisterCallback    aCallback,
                                          void               *aContext)
{
    Error error;

    VerifyOrExit(aAddresses != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(aAddressNum > 0 && aAddressNum <= kMaxIp6Addresses, error = kErrorInvalidArgs);
    VerifyOrExit(aContext == nullptr || aCallback != nullptr, error = kErrorInvalidArgs);
#if !OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    VerifyOrExit(Get<MeshCoP::Commissioner>().IsActive(), error = kErrorInvalidState);
#else
    if (!Get<MeshCoP::Commissioner>().IsActive())
    {
        LogWarn("MLR.req without active commissioner session for test.");
    }
#endif

    // Only allow one outstanding registration if callback is specified.
    VerifyOrExit(!mRegisterPending, error = kErrorBusy);

    SuccessOrExit(error = SendMessage(aAddresses, aAddressNum, aTimeout, HandleRegisterResponse));

    mRegisterPending = true;
    mRegisterCallback.Set(aCallback, aContext);

exit:
    return error;
}

void Manager::HandleRegisterResponse(Coap::Msg *aMsg, Error aResult)
{
    uint8_t      status;
    Error        error;
    AddressArray failedAddresses;

    mRegisterPending = false;

    error = ParseResponse(aResult, aMsg, status, failedAddresses);

    mRegisterCallback.InvokeAndClearIfSet(error, status, failedAddresses.GetArrayBuffer(), failedAddresses.GetLength());
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

Error Manager::SendMessage(const Ip6::Address         *aAddresses,
                           uint8_t                     aAddressNum,
                           const uint32_t             *aTimeout,
                           const Coap::ResponseHandler aResponseHandler)
{
    OT_UNUSED_VARIABLE(aTimeout);

    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;
    Ip6::Address   destAddr;

    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = kErrorInvalidState);

    message = Get<Tmf::Agent>().AllocateAndInitConfirmablePostMessage(kUriMlr);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Ip6AddressesTlv::AppendTo(*message, aAddresses, aAddressNum));

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    if (Get<MeshCoP::Commissioner>().IsActive())
    {
        SuccessOrExit(
            error = Tlv::Append<ThreadCommissionerSessionIdTlv>(*message, Get<MeshCoP::Commissioner>().GetSessionId()));
    }

    if (aTimeout != nullptr)
    {
        SuccessOrExit(error = Tlv::Append<ThreadTimeoutTlv>(*message, *aTimeout));
    }
#else
    OT_ASSERT(aTimeout == nullptr);
#endif

    if (!Get<Mle::Mle>().IsFullThreadDevice() && Get<Mle::Mle>().GetParent().IsThreadVersion1p1())
    {
        uint8_t pbbrServiceId;

        SuccessOrExit(error = Get<BackboneRouter::Leader>().GetServiceId(pbbrServiceId));
        Get<Mle::Mle>().GetServiceAloc(pbbrServiceId, destAddr);
    }
    else
    {
        destAddr.SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), Get<BackboneRouter::Leader>().GetServer16());
    }

    error = Get<Tmf::Agent>().SendMessageTo(*message, destAddr, aResponseHandler, this);

    LogInfo("Sent MLR.req: addressNum=%d", aAddressNum);

exit:
    LogInfoOnError(error, "SendMessage()");
    FreeMessageOnError(message, error);
    return error;
}

void Manager::HandleResponse(Coap::Msg *aMsg, Error aResult)
{
    uint8_t      status;
    Error        error;
    AddressArray failedAddresses;

    error = ParseResponse(aResult, aMsg, status, failedAddresses);

    Finish(error == kErrorNone && status == kStatusSuccess, failedAddresses);

    if (error == kErrorNone && status == kStatusSuccess)
    {
        // keep sending until all multicast addresses are registered.
        ScheduleSend(0);
    }
    else
    {
        BackboneRouter::Config config;

        // If a registration attempt fails, retry it after a random
        // delay (same as re-registration delay).

        if (Get<BackboneRouter::Leader>().GetConfig(config) == kErrorNone)
        {
            ScheduleSend(DetermineReregistrationDelay(config));
        }
    }
}

Error Manager::ParseResponse(Error aResult, Coap::Msg *aMsg, uint8_t &aStatus, AddressArray &aFailedAddresses)
{
    Error error = aResult;

    aStatus = kStatusGeneralFailure;
    aFailedAddresses.Clear();

    SuccessOrExit(error);
    VerifyOrExit(aMsg != nullptr, error = kErrorParse);
    VerifyOrExit(aMsg->GetCode() == Coap::kCodeChanged, error = kErrorParse);

    SuccessOrExit(error = Tlv::Find<ThreadStatusTlv>(aMsg->mMessage, aStatus));

    switch (error = Ip6AddressesTlv::FindIn(aMsg->mMessage, aFailedAddresses))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        error = kErrorNone;
        aFailedAddresses.Clear();
        break;
    default:
        ExitNow();
    }

    if (aStatus == kStatusSuccess)
    {
        VerifyOrExit(aFailedAddresses.IsEmpty(), error = kErrorParse);

        LogInfo("Receive MLR.rsp OK");
        ExitNow();
    }

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    LogWarn("Receive MLR.rsp: status=%u, failedAddressNum=%u", aStatus, aFailedAddresses.GetLength());

    for (const Ip6::Address &address : aFailedAddresses)
    {
        LogWarn("   %s", address.ToString().AsCString());
    }
#endif

exit:
    if (error != kErrorNone)
    {
        aFailedAddresses.Clear();
    }

    LogWarnOnError(error, "parse MLR.rsp");
    return error;
}

void Manager::SetMulticastAddressState(State aFromState, State aToState)
{
#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.Matches(aFromState))
        {
            addr.SetMlrState(aToState);
        }
    }
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (Child::Ip6AddrEntry &addrEntry : child.GetIp6Addresses())
        {
            if (!addrEntry.IsMulticastLargerThanRealmLocal())
            {
                continue;
            }

            if (addrEntry.GetMlrState(child) == aFromState)
            {
                addrEntry.SetMlrState(aToState, child);
            }
        }
    }
#endif
}

bool Manager::DidRegisterSuccessfully(const Ip6::Address &aAddress, bool aSuccess, const AddressArray &aFailedAddresses)
{
    // If the operation succeeded, all address registrations were successful.
    //
    // If it failed and the failed address list is empty, all registrations
    // failed.
    //
    // If it failed and a non-empty failed address list is provided, only the
    // addresses in the list failed (if an address is not in the list, its
    // registration was successful).

    bool didRegister;

    if (aSuccess)
    {
        didRegister = true;
    }
    else if (aFailedAddresses.IsEmpty())
    {
        didRegister = false;
    }
    else
    {
        didRegister = !aFailedAddresses.Contains(aAddress);
    }

    return didRegister;
}

void Manager::Finish(bool aSuccess, const AddressArray &aFailedAddresses)
{
    OT_ASSERT(mPending);

    mPending = false;

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.Matches(kStateRegistering))
        {
            bool didRegister = DidRegisterSuccessfully(addr.GetAddress(), aSuccess, aFailedAddresses);

            addr.SetMlrState(didRegister ? kStateRegistered : kStateToRegister);
        }
    }
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (Child::Ip6AddrEntry &addrEntry : child.GetIp6Addresses())
        {
            if (!addrEntry.IsMulticastLargerThanRealmLocal())
            {
                continue;
            }

            if (addrEntry.GetMlrState(child) == kStateRegistering)
            {
                bool didRegister = DidRegisterSuccessfully(addrEntry, aSuccess, aFailedAddresses);

                addrEntry.SetMlrState(didRegister ? kStateRegistered : kStateToRegister, child);
            }
        }
    }
#endif

    LogMulticastAddresses();
}

void Manager::HandleTimeTick(void)
{
    if (mSendDelay > 0 && --mSendDelay == 0)
    {
        Send();
    }

    if (mReregistrationDelay > 0 && --mReregistrationDelay == 0)
    {
        Reregister();
    }

    UpdateTimeTickerRegistration();
}

void Manager::Reregister(void)
{
    LogInfo("MLR Reregister!");

    SetMulticastAddressState(kStateRegistered, kStateToRegister);

    ScheduleSend(0);

    ScheduleNextRegistration(kRenew);
}

uint16_t Manager::DetermineReregistrationDelay(const BackboneRouter::Config &aConfig)
{
    uint16_t delay = 1;

    VerifyOrExit(aConfig.mReregistrationDelay > 1);
    delay = Random::NonCrypto::GetUint16InRange(1, aConfig.mReregistrationDelay);

exit:
    return delay;
}

uint32_t Manager::DetermineRenewDelay(const BackboneRouter::Config &aConfig)
{
    // As per Thread spec, the renew delay is randomly chosen
    // between (0.5 * MLR-Timeout) and (MLR-Timeout - 9 seconds).
    // The `kRenewGuardTime`(9 sec) allows time for transmission,
    // potential retransmissions, and acknowledgment before the
    // actual timeout.

    uint32_t timeout = Clamp<uint32_t>(aConfig.mMlrTimeout, BackboneRouter::kMinMlrTimeout, kLongRenewTimeout);

    return Random::NonCrypto::GetUint32InRange((timeout / 2) + 1, timeout - kRenewGuardTime);
}

void Manager::ScheduleNextRegistration(RegistrationRequest aRequest)
{
    uint32_t               delay;
    BackboneRouter::Config config;

    if (!ShouldRegister())
    {
        mReregistrationDelay = 0;
        ExitNow();
    }

    IgnoreError(Get<BackboneRouter::Leader>().GetConfig(config));

    switch (aRequest)
    {
    case kReregister:
        delay = DetermineReregistrationDelay(config);
        break;

    case kRenew:
        delay = DetermineRenewDelay(config);
        break;

    default:
        ExitNow();
    }

    if (mReregistrationDelay == 0 || mReregistrationDelay > delay)
    {
        mReregistrationDelay = delay;

        LogDebg("ScheduleNextRegistration() delay:%lu", ToUlong(delay));
    }

exit:
    UpdateTimeTickerRegistration();
}

void Manager::LogMulticastAddresses(void)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
    LogDebg("-------- Multicast Addresses --------");

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (const Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (!addr.IsMlrCandidate())
        {
            continue;
        }

        LogDebg("%-32s%c", addr.GetAddress().ToString().AsCString(), "-rR"[addr.GetMlrState()]);
    }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (const Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (const Child::Ip6AddrEntry &addrEntry : child.GetIp6Addresses())
        {
            if (!addrEntry.IsMulticastLargerThanRealmLocal())
            {
                continue;
            }

            LogDebg("%-32s%c %04x", addrEntry.ToString().AsCString(), "-rR"[addrEntry.GetMlrState(child)],
                    child.GetRloc16());
        }
    }
#endif

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
}

} // namespace Mlr
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE
