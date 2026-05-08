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
        HandleEventIp6MulticastSubscribed();
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
void Manager::HandleEventIp6MulticastSubscribed(void)
{
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.IsMlrCandidate() && !addr.IsMlrRegistered() && IsAddressRegisteredByAnyChild(addr.GetAddress()))
        {
            addr.SetMlrRegistered(true);
        }
    }
#endif

    ScheduleSend(0);
}

bool Manager::IsAddressRegisteredByNetif(const Ip6::Address &aAddress) const
{
    bool isRegistered = false;

    for (const Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.IsMlrRegistered() && (addr.GetAddress() == aAddress))
        {
            isRegistered = true;
            break;
        }
    }

    return isRegistered;
}

#endif // OPENTHREAD_CONFIG_MLR_ENABLE

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

bool Manager::IsAddressRegisteredByAnyChildExcept(const Ip6::Address &aAddress, const Child *aExceptChild) const
{
    bool isRegistered = false;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (&child != aExceptChild && child.HasMlrRegisteredAddress(aAddress))
        {
            isRegistered = true;
            break;
        }
    }

    return isRegistered;
}

void Manager::UpdateProxiedSubscriptions(Child &aChild, const ChildAddressArray &aOldRegisteredAddresses)
{
    bool hasUnregistered = false;

    VerifyOrExit(aChild.IsStateValid());

    for (Child::Ip6AddrEntry &addrEntry : aChild.GetIp6Addresses())
    {
        bool isRegistered;

        if (!addrEntry.IsMulticastLargerThanRealmLocal())
        {
            continue;
        }

        isRegistered = aOldRegisteredAddresses.Contains(addrEntry);

#if OPENTHREAD_CONFIG_MLR_ENABLE
        isRegistered = isRegistered || IsAddressRegisteredByNetif(addrEntry);
#endif
        isRegistered = isRegistered || IsAddressRegisteredByAnyChildExcept(addrEntry, &aChild);

        addrEntry.SetMlrRegistered(isRegistered, aChild);

        if (!isRegistered)
        {
            hasUnregistered = true;
        }
    }

    if (hasUnregistered)
    {
        ScheduleSend(Random::NonCrypto::GetUint16InRange(1, BackboneRouter::kParentAggregateDelay));
    }

exit:
    return;
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

void Manager::DetermineAddressesToRegister(AddressArray &aAddresses) const
{
    aAddresses.Clear();

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (const Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.IsMlrCandidate() && !addr.IsMlrRegistered())
        {
            SuccessOrExit(aAddresses.AddUnique(addr.GetAddress()));
        }
    }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (const Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (const Child::Ip6AddrEntry &addrEntry : child.GetIp6Addresses())
        {
            if (addrEntry.IsMulticastLargerThanRealmLocal() && !addrEntry.IsMlrRegistered(child))
            {
                SuccessOrExit(aAddresses.AddUnique(addrEntry));
            }
        }
    }
#endif

exit:
    return;
}

void Manager::Send(void)
{
    Error        error;
    AddressArray addresses;

    VerifyOrExit(!mPending, error = kErrorBusy);

    VerifyOrExit(Get<Mle::Mle>().IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(ShouldRegister(), error = kErrorInvalidState);

    DetermineAddressesToRegister(addresses);
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
    if (error == kErrorNoBufs)
    {
        ScheduleSend(1);
    }
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
    Error                   error;
    uint8_t                 status;
    AddressArray            failedAddresses;
    AddressArray            registeredAddresses;
    OwnedPtr<Coap::Message> requestMsg;

    mPending = false;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Parse the response message

    SuccessOrExit(error = ParseResponse(aResult, aMsg, status, failedAddresses));

    if (status != kStatusSuccess)
    {
        // If status is failure and the failed address list is empty,
        // all registrations failed. If a non-empty failed address
        // list is provided, only the addresses in the list failed
        // (if an address is not in the list, its registration was
        // successful).

        VerifyOrExit(!failedAddresses.IsEmpty());
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Retrieve the original TMF MLR request message and parse it
    // to find the list of addresses included in the request. Remove
    // addresses in `failedAddresses`.

    SuccessOrExit(error = Get<Tmf::Agent>().GetDispatchingRequest(requestMsg));
    SuccessOrExit(error = Ip6AddressesTlv::FindIn(*requestMsg, registeredAddresses));

    for (const Ip6::Address &addr : failedAddresses)
    {
        registeredAddresses.RemoveMatching(addr);
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Update the registration state of addresses on `ThreadNetif` and
    // `ChildTable`.

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.IsMlrCandidate() && !addr.IsMlrRegistered() && registeredAddresses.Contains(addr.GetAddress()))
        {
            addr.SetMlrRegistered(true);
        }
    }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (Child::Ip6AddrEntry &addrEntry : child.GetIp6Addresses())
        {
            if (addrEntry.IsMulticastLargerThanRealmLocal() && !addrEntry.IsMlrRegistered(child) &&
                registeredAddresses.Contains(addrEntry))
            {
                addrEntry.SetMlrRegistered(true, child);
            }
        }
    }
#endif

exit:
    if ((error == kErrorNone) && (status == kStatusSuccess))
    {
        // Send an MLR request for any remaining unregistered addresses.
        ScheduleSend(0);
    }
    else
    {
        // If a registration attempt fails, retry it after a random
        // delay (same as re-registration delay).

        BackboneRouter::Config config;

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
    LogInfo("Reregister");

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (addr.IsMlrCandidate())
        {
            addr.SetMlrRegistered(false);
        }
    }
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        child.ClearMlrRegisteredStateOnAllIp6Addresses();
    }
#endif

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

} // namespace Mlr
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE
