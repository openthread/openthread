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
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    , mRegisterPending(false)
#endif
    , mState(kStateStopped)
    , mTimer(aInstance)
{
}

void Manager::EnterState(State aState)
{
    State oldState;

    VerifyOrExit(mState != aState);

    oldState = mState;
    mState   = aState;

    if (oldState == kStateRegistering)
    {
        IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleResponse, this));
    }

    if (mState == kStateToRegisterAll)
    {
        // Clear "MlrRegistered" state on all addresses

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
            child.ClearAllAddressesMlrRegistrationState();
        }
#endif
    }

exit:
    return;
}

void Manager::UpdateState(void)
{
    // `UpdateState()` evaluates whether the MLR manager should be
    // running or not. It handles starting or stopping the manager.

    bool canRun = false;

    if (Get<Mle::Mle>().IsAttached() && Get<BackboneRouter::Leader>().HasPrimary())
    {
        canRun = Get<Mle::Mle>().IsFullThreadDevice() || Get<Mle::Mle>().GetParent().IsThreadVersion1p1();
    }

    if (!IsRunning())
    {
        VerifyOrExit(canRun);
        EnterState(kStateToRegisterAll);
        ScheduleTimerForReregistrationDelay();
    }
    else
    {
        VerifyOrExit(!canRun);
        EnterState(kStateStopped);
        mTimer.Stop();
    }

exit:
    return;
}

void Manager::HandleNotifierEvents(Events aEvents)
{
#if OPENTHREAD_CONFIG_MLR_ENABLE
    if (aEvents.Contains(kEventIp6MulticastSubscribed))
    {
        HandleEventIp6MulticastSubscribed();
    }
#endif

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        UpdateState();
    }
}

void Manager::HandleBackboneRouterPrimaryUpdate(BackboneRouter::PrimaryEvent aEvent)
{
    UpdateState();

    switch (aEvent)
    {
    case BackboneRouter::kPrimaryAdded:
    case BackboneRouter::kPrimaryRemoved:
        break;

    case BackboneRouter::kPrimaryUpdatedReregister:
        VerifyOrExit(IsRunning());
        EnterState(kStateToRegisterAll);
        ScheduleTimerForReregistrationDelay();
        break;

    case BackboneRouter::kPrimaryConfigParameterChanged:

        // When PBBR config parameters (like MLR Timeout) change, we
        // need to recalculate and update the renewal time for all
        // currently registered addresses to ensure they don't expire
        // prematurely.

        switch (GetState())
        {
        case kStateStopped:
        case kStateIdle:
        case kStateToRegisterAll:
        case kStateRegistering:
            break;

        case kStateRegistered:
            mTimer.Stop();
            OT_FALL_THROUGH;

        case kStateNewAddrToRegister:
            DetermineRenewTime();
            mTimer.FireAtIfEarlier(mRenewTime);
            break;
        }

        break;
    }

exit:
    return;
}

#if OPENTHREAD_CONFIG_MLR_ENABLE
void Manager::HandleEventIp6MulticastSubscribed(void)
{
    bool hasUnregistered = false;

    for (Ip6::Netif::MulticastAddress &addr : Get<ThreadNetif>().GetMulticastAddresses())
    {
        if (!addr.IsMlrCandidate())
        {
            continue;
        }

        if (!addr.IsMlrRegistered())
        {
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
            if (IsAddressRegisteredByAnyChild(addr.GetAddress()))
            {
                addr.SetMlrRegistered(true);
                continue;
            }
#endif

            hasUnregistered = true;
        }
    }

    if (hasUnregistered)
    {
        ScheduleNewAddrRegistration(0, kMaxNewNetifAddrRegistraionDelay);
    }
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

bool Manager::IsAddressRegisteredByAnyChild(const Ip6::Address &aAddress) const
{
    return IsAddressRegisteredByAnyChildExcept(aAddress, nullptr);
}

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

void Manager::UpdateChildRegistrations(Child &aChild)
{
    Child::Ip6AddressArray emptyArray;

    UpdateChildRegistrations(aChild, emptyArray);
}

void Manager::UpdateChildRegistrations(Child &aChild, const Child::Ip6AddressArray &aOldRegisteredAddresses)
{
    bool hasUnregistered = false;

    VerifyOrExit(aChild.IsStateValid());

    for (const Ip6::Address &addr : aChild.GetIp6Addresses())
    {
        bool isRegistered;

        if (!addr.IsMulticastLargerThanRealmLocal())
        {
            continue;
        }

        isRegistered = aOldRegisteredAddresses.ContainsMatching(addr);

#if OPENTHREAD_CONFIG_MLR_ENABLE
        isRegistered = isRegistered || IsAddressRegisteredByNetif(addr);
#endif
        isRegistered = isRegistered || IsAddressRegisteredByAnyChildExcept(addr, &aChild);

        aChild.SetAddressMlrRegistrationState(addr, isRegistered);

        if (!isRegistered)
        {
            hasUnregistered = true;
        }
    }

    if (hasUnregistered)
    {
        ScheduleNewAddrRegistration(kMinNewChildAddrRegistrationDelay, kMaxNewChildAddrRegistrationDelay);
    }

exit:
    return;
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

void Manager::ScheduleNewAddrRegistration(uint32_t aMinDelay, uint32_t aMaxDelay)
{
    uint32_t delay;

    switch (GetState())
    {
    case kStateIdle:
        EnterState(kStateToRegisterAll);
        break;

    case kStateRegistered:
    case kStateNewAddrToRegister:
        EnterState(kStateNewAddrToRegister);
        break;

    case kStateStopped:
    case kStateToRegisterAll:
    case kStateRegistering:
        ExitNow();
    }

    delay = Random::NonCrypto::GenerateInClosedRange(aMinDelay, aMaxDelay);

    // The timer may already be running for a periodic renewal or for a
    // previously scheduled new address registration. We ensure the
    // timer fires at the earliest requested time.

    mTimer.FireAtIfEarlier(TimerMilli::GetNow() + delay);

exit:
    return;
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
        for (const Ip6::Address &addr : child.GetIp6Addresses())
        {
            if (addr.IsMulticastLargerThanRealmLocal() && !child.HasMlrRegisteredAddress(addr))
            {
                SuccessOrExit(aAddresses.AddUnique(addr));
            }
        }
    }
#endif

exit:
    return;
}

void Manager::SendNextRequest(void)
{
    AddressArray addresses;

    IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleResponse, this));

    DetermineAddressesToRegister(addresses);

    if (addresses.IsEmpty())
    {
        // There are no (more) addresses to register. If we are still
        // in `kStateToRegisterAll`, it indicates there is no multicast
        // address to register at all, so we transition to `kStateIdle`.
        // Otherwise, we at least sent one registration, and all addresses
        // are now registered, so we transition to `kStateRegistered` and
        // schedule the next periodic renewal.

        switch (GetState())
        {
        case kStateToRegisterAll:
            EnterState(kStateIdle);
            break;

        case kStateRegistering:
        case kStateNewAddrToRegister:
            EnterState(kStateRegistered);
            mTimer.FireAt(mRenewTime);
            break;

        case kStateStopped:
        case kStateIdle:
        case kStateRegistered:
            break;
        }

        ExitNow();
    }

    if (SendMessage(addresses.GetArrayBuffer(), addresses.GetLength(), nullptr, HandleResponse) != kErrorNone)
    {
        mTimer.Start(kSendFailureRetryDelay);
        ExitNow();
    }

    // Transitioning from `kStateToRegisterAll` to `kStateRegistering`
    // indicates the start of a full registration cycle. We record the
    // start time and determine the renewal time. The start time is
    // tracked so we can recalculate the renewal time if PBBR config
    // parameters change later.

    if (GetState() == kStateToRegisterAll)
    {
        mStartTime = TimerMilli::GetNow();
        DetermineRenewTime();
    }

    EnterState(kStateRegistering);

    // Generally, a Thread 1.2 Router sends MLR.req on behalf of its
    // MTD children for multicast addresses. However, when a Thread
    // 1.2 MTD child attaches to a Thread 1.1 parent, the MTD child
    // must send the MLR.req to the PBBR itself. In this scenario, a
    // Thread 1.2 Sleepy End Device relies on fast data polls to
    // receive the response in a timely manner.

    if (!Get<Mle::Mle>().IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SendFastPolls();
    }

exit:
    return;
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
        Get<Mle::Mle>().ComposeServiceAloc(pbbrServiceId, destAddr);
    }
    else
    {
        Get<Mle::Mle>().ComposeRloc(Get<BackboneRouter::Leader>().GetServer16(), destAddr);
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
    VerifyOrExit(GetState() == kStateRegistering);
    ProcessResponse(aMsg, aResult);

exit:
    return;
}

void Manager::ProcessResponse(Coap::Msg *aMsg, Error aResult)
{
    Error                   error;
    uint8_t                 status;
    AddressArray            failedAddresses;
    AddressArray            registeredAddresses;
    OwnedPtr<Coap::Message> requestMsg;

    if (!Get<Mle::Mle>().IsRxOnWhenIdle())
    {
        Get<DataPollSender>().StopFastPolls();
    }

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
        for (const Ip6::Address &addr : child.GetIp6Addresses())
        {
            if (addr.IsMulticastLargerThanRealmLocal() && !child.HasMlrRegisteredAddress(addr) &&
                registeredAddresses.Contains(addr))
            {
                child.SetAddressMlrRegistrationState(addr, true);
            }
        }
    }
#endif

exit:
    if ((error == kErrorNone) && (status == kStatusSuccess))
    {
        SendNextRequest();
    }
    else
    {
        // If a registration attempt fails, retry it after a random
        // delay (same as re-registration delay).

        ScheduleTimerForReregistrationDelay();
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

void Manager::ScheduleTimerForReregistrationDelay(void)
{
    mTimer.Start(Time::SecToMsec(Get<BackboneRouter::Leader>().GetConfig().SelectRandomReregistrationDelay()));
}

void Manager::HandleTimer(void)
{
    switch (GetState())
    {
    case kStateNewAddrToRegister:
        if (mRenewTime > TimerMilli::GetNow())
        {
            break;
        }

        OT_FALL_THROUGH;

    case kStateRegistered:
        EnterState(kStateToRegisterAll);

        OT_FALL_THROUGH;

    case kStateToRegisterAll:
    case kStateRegistering:
        break;

    case kStateStopped:
    case kStateIdle:
        ExitNow();
    }

    SendNextRequest();

exit:
    return;
}

void Manager::DetermineRenewTime(void)
{
    // As per Thread spec, the renew delay is randomly chosen
    // between (0.5 * MLR-Timeout) and (MLR-Timeout - 9 seconds).
    // The `RenewGuardTime`(9 sec) allows time for transmission,
    // potential retransmissions, and acknowledgment before the
    // actual timeout.

    uint32_t timeout;

    timeout = Get<BackboneRouter::Leader>().GetConfig().GetMlrTimeout();
    timeout = Time::SecToMsec(Clamp<uint32_t>(timeout, BackboneRouter::kMinMlrTimeout, kLongRenewTimeout));

    mRenewTime =
        mStartTime + Random::NonCrypto::GenerateInClosedRange((timeout / 2) + 1, timeout - kRenewGuardTimeInMsec);
}

} // namespace Mlr
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE
