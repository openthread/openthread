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

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "instance/instance.hpp"
#include "net/ip6_address.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"
#include "utils/slaac_address.hpp"

namespace ot {

RegisterLogModule("MlrManager");

MlrManager::MlrManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mReregistrationDelay(0)
    , mSendDelay(0)
    , mMlrPending(false)
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
    , mRegisterPending(false)
#endif
{
}

void MlrManager::HandleNotifierEvents(Events aEvents)
{
#if OPENTHREAD_CONFIG_MLR_ENABLE
    if (aEvents.Contains(kEventIp6MulticastSubscribed))
    {
        UpdateLocalSubscriptions();
    }
#endif

    if (aEvents.Contains(kEventThreadRoleChanged) && Get<Mle::MleRouter>().IsChild())
    {
        // Reregistration after re-attach
        UpdateReregistrationDelay(true);
    }
}

void MlrManager::HandleBackboneRouterPrimaryUpdate(BackboneRouter::Leader::State aState,
                                                   const BackboneRouter::Config &aConfig)
{
    OT_UNUSED_VARIABLE(aConfig);

    bool needRereg =
        aState == BackboneRouter::Leader::kStateAdded || aState == BackboneRouter::Leader::kStateToTriggerRereg;

    UpdateReregistrationDelay(needRereg);
}

#if OPENTHREAD_CONFIG_MLR_ENABLE
void MlrManager::UpdateLocalSubscriptions(void)
{
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    // Check multicast addresses are newly listened against Children
    for (Ip6::Netif::ExternalMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (addr.GetMlrState() == kMlrStateToRegister && IsAddressMlrRegisteredByAnyChild(addr.GetAddress()))
        {
            addr.SetMlrState(kMlrStateRegistered);
        }
    }
#endif

    CheckInvariants();
    ScheduleSend(0);
}

bool MlrManager::IsAddressMlrRegisteredByNetif(const Ip6::Address &aAddress) const
{
    bool ret = false;

    OT_ASSERT(aAddress.IsMulticastLargerThanRealmLocal());

    for (const Ip6::Netif::ExternalMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        if (addr.GetAddress() == aAddress && addr.GetMlrState() == kMlrStateRegistered)
        {
            ExitNow(ret = true);
        }
    }

exit:
    return ret;
}

#endif // OPENTHREAD_CONFIG_MLR_ENABLE

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

bool MlrManager::IsAddressMlrRegisteredByAnyChildExcept(const Ip6::Address &aAddress, const Child *aExceptChild) const
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

void MlrManager::UpdateProxiedSubscriptions(Child &aChild, const MlrAddressArray &aOldMlrRegisteredAddresses)
{
    VerifyOrExit(aChild.IsStateValid());

    // Search the new multicast addresses and set its flag accordingly
    for (const Ip6::Address &address : aChild.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        bool isMlrRegistered = aOldMlrRegisteredAddresses.Contains(address);

#if OPENTHREAD_CONFIG_MLR_ENABLE
        // Check if it's a new multicast address against parent Netif
        isMlrRegistered = isMlrRegistered || IsAddressMlrRegisteredByNetif(address);
#endif
        // Check if it's a new multicast address against other Children
        isMlrRegistered = isMlrRegistered || IsAddressMlrRegisteredByAnyChildExcept(address, &aChild);

        aChild.SetAddressMlrState(address, isMlrRegistered ? kMlrStateRegistered : kMlrStateToRegister);
    }

exit:
    LogMulticastAddresses();
    CheckInvariants();

    if (aChild.HasAnyMlrToRegisterAddress())
    {
        ScheduleSend(Random::NonCrypto::GetUint16InRange(1, BackboneRouter::kParentAggregateDelay));
    }
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

void MlrManager::ScheduleSend(uint16_t aDelay)
{
    OT_ASSERT(!mMlrPending || mSendDelay == 0);

    VerifyOrExit(!mMlrPending);

    if (aDelay == 0)
    {
        mSendDelay = 0;
        SendMlr();
    }
    else if (mSendDelay == 0 || mSendDelay > aDelay)
    {
        mSendDelay = aDelay;
    }

    UpdateTimeTickerRegistration();
exit:
    return;
}

void MlrManager::UpdateTimeTickerRegistration(void)
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

void MlrManager::SendMlr(void)
{
    Error           error;
    Mle::MleRouter &mle = Get<Mle::MleRouter>();
    AddressArray    addresses;

    VerifyOrExit(!mMlrPending, error = kErrorBusy);
    VerifyOrExit(mle.IsAttached(), error = kErrorInvalidState);
    VerifyOrExit(mle.IsFullThreadDevice() || mle.GetParent().IsThreadVersion1p1(), error = kErrorInvalidState);
    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = kErrorInvalidState);

#if OPENTHREAD_CONFIG_MLR_ENABLE
    // Append Netif multicast addresses
    for (Ip6::Netif::ExternalMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (addresses.IsFull())
        {
            break;
        }

        if (addr.GetMlrState() == kMlrStateToRegister)
        {
            addresses.AddUnique(addr.GetAddress());
            addr.SetMlrState(kMlrStateRegistering);
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

        for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            if (addresses.IsFull())
            {
                break;
            }

            if (child.GetAddressMlrState(address) == kMlrStateToRegister)
            {
                addresses.AddUnique(address);
                child.SetAddressMlrState(address, kMlrStateRegistering);
            }
        }
    }
#endif

    VerifyOrExit(!addresses.IsEmpty(), error = kErrorNotFound);
    SuccessOrExit(
        error = SendMlrMessage(addresses.GetArrayBuffer(), addresses.GetLength(), nullptr, HandleMlrResponse, this));

    mMlrPending = true;

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
        SetMulticastAddressMlrState(kMlrStateRegistering, kMlrStateToRegister);

        if (error == kErrorNoBufs)
        {
            ScheduleSend(1);
        }
    }

    LogMulticastAddresses();
    CheckInvariants();
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
Error MlrManager::RegisterMulticastListeners(const Ip6::Address *aAddresses,
                                             uint8_t             aAddressNum,
                                             const uint32_t     *aTimeout,
                                             MlrCallback         aCallback,
                                             void               *aContext)
{
    Error error;

    VerifyOrExit(aAddresses != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit(aAddressNum > 0 && aAddressNum <= Ip6AddressesTlv::kMaxAddresses, error = kErrorInvalidArgs);
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

    SuccessOrExit(error = SendMlrMessage(aAddresses, aAddressNum, aTimeout, HandleRegisterResponse, this));

    mRegisterPending = true;
    mRegisterCallback.Set(aCallback, aContext);

exit:
    return error;
}

void MlrManager::HandleRegisterResponse(void                *aContext,
                                        otMessage           *aMessage,
                                        const otMessageInfo *aMessageInfo,
                                        Error                aResult)
{
    static_cast<MlrManager *>(aContext)->HandleRegisterResponse(AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo),
                                                                aResult);
}

void MlrManager::HandleRegisterResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    uint8_t               status;
    Error                 error;
    AddressArray          failedAddresses;
    Callback<MlrCallback> callbackCopy = mRegisterCallback;

    mRegisterPending = false;
    mRegisterCallback.Clear();

    error = ParseMlrResponse(aResult, AsCoapMessagePtr(aMessage), status, failedAddresses);

    callbackCopy.InvokeIfSet(error, status, failedAddresses.GetArrayBuffer(), failedAddresses.GetLength());
}

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE

Error MlrManager::SendMlrMessage(const Ip6::Address   *aAddresses,
                                 uint8_t               aAddressNum,
                                 const uint32_t       *aTimeout,
                                 Coap::ResponseHandler aResponseHandler,
                                 void                 *aResponseContext)
{
    OT_UNUSED_VARIABLE(aTimeout);

    Error            error   = kErrorNone;
    Mle::MleRouter  &mle     = Get<Mle::MleRouter>();
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    Ip6AddressesTlv  addressesTlv;

    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = kErrorInvalidState);

    message = Get<Tmf::Agent>().NewConfirmablePostMessage(kUriMlr);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    addressesTlv.Init();
    addressesTlv.SetLength(sizeof(Ip6::Address) * aAddressNum);
    SuccessOrExit(error = message->Append(addressesTlv));
    SuccessOrExit(error = message->AppendBytes(aAddresses, sizeof(Ip6::Address) * aAddressNum));

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

    if (!mle.IsFullThreadDevice() && mle.GetParent().IsThreadVersion1p1())
    {
        uint8_t pbbrServiceId;

        SuccessOrExit(error = Get<BackboneRouter::Leader>().GetServiceId(pbbrServiceId));
        SuccessOrExit(error = mle.GetServiceAloc(pbbrServiceId, messageInfo.GetPeerAddr()));
    }
    else
    {
        messageInfo.GetPeerAddr().SetToRoutingLocator(mle.GetMeshLocalPrefix(),
                                                      Get<BackboneRouter::Leader>().GetServer16());
    }

    messageInfo.SetSockAddrToRloc();

    error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, aResponseHandler, aResponseContext);

    LogInfo("Sent MLR.req: addressNum=%d", aAddressNum);

exit:
    LogInfo("SendMlrMessage(): %s", ErrorToString(error));
    FreeMessageOnError(message, error);
    return error;
}

void MlrManager::HandleMlrResponse(void                *aContext,
                                   otMessage           *aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   Error                aResult)
{
    static_cast<MlrManager *>(aContext)->HandleMlrResponse(AsCoapMessagePtr(aMessage), AsCoreTypePtr(aMessageInfo),
                                                           aResult);
}

void MlrManager::HandleMlrResponse(Coap::Message *aMessage, const Ip6::MessageInfo *aMessageInfo, Error aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    uint8_t      status;
    Error        error;
    AddressArray failedAddresses;

    error = ParseMlrResponse(aResult, aMessage, status, failedAddresses);

    FinishMlr(error == kErrorNone && status == ThreadStatusTlv::kMlrSuccess, failedAddresses);

    if (error == kErrorNone && status == ThreadStatusTlv::kMlrSuccess)
    {
        // keep sending until all multicast addresses are registered.
        ScheduleSend(0);
    }
    else
    {
        BackboneRouter::Config config;
        uint16_t               reregDelay;

        // The Device has just attempted a Multicast Listener Registration which failed, and it retries the same
        // registration with a random time delay chosen in the interval [0, Reregistration Delay].
        // This is required by Thread 1.2 Specification 5.24.2.3
        if (Get<BackboneRouter::Leader>().GetConfig(config) == kErrorNone)
        {
            reregDelay = config.mReregistrationDelay > 1
                             ? Random::NonCrypto::GetUint16InRange(1, config.mReregistrationDelay)
                             : 1;
            ScheduleSend(reregDelay);
        }
    }
}

Error MlrManager::ParseMlrResponse(Error          aResult,
                                   Coap::Message *aMessage,
                                   uint8_t       &aStatus,
                                   AddressArray  &aFailedAddresses)
{
    Error    error;
    uint16_t addressesOffset, addressesLength;

    aStatus = ThreadStatusTlv::kMlrGeneralFailure;

    VerifyOrExit(aResult == kErrorNone && aMessage != nullptr, error = kErrorParse);
    VerifyOrExit(aMessage->GetCode() == Coap::kCodeChanged, error = kErrorParse);

    SuccessOrExit(error = Tlv::Find<ThreadStatusTlv>(*aMessage, aStatus));

    if (ThreadTlv::FindTlvValueOffset(*aMessage, Ip6AddressesTlv::kIp6Addresses, addressesOffset, addressesLength) ==
        kErrorNone)
    {
        VerifyOrExit(addressesLength % sizeof(Ip6::Address) == 0, error = kErrorParse);
        VerifyOrExit(addressesLength / sizeof(Ip6::Address) <= Ip6AddressesTlv::kMaxAddresses, error = kErrorParse);

        for (uint16_t offset = 0; offset < addressesLength; offset += sizeof(Ip6::Address))
        {
            IgnoreError(aMessage->Read(addressesOffset + offset, *aFailedAddresses.PushBack()));
        }
    }

    VerifyOrExit(aFailedAddresses.IsEmpty() || aStatus != ThreadStatusTlv::kMlrSuccess, error = kErrorParse);

exit:
    LogMlrResponse(aResult, error, aStatus, aFailedAddresses);
    return aResult != kErrorNone ? aResult : error;
}

void MlrManager::SetMulticastAddressMlrState(MlrState aFromState, MlrState aToState)
{
#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (Ip6::Netif::ExternalMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (addr.GetMlrState() == aFromState)
        {
            addr.SetMlrState(aToState);
        }
    }
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            if (child.GetAddressMlrState(address) == aFromState)
            {
                child.SetAddressMlrState(address, aToState);
            }
        }
    }
#endif
}

void MlrManager::FinishMlr(bool aSuccess, const AddressArray &aFailedAddresses)
{
    OT_ASSERT(mMlrPending);

    mMlrPending = false;

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (Ip6::Netif::ExternalMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (addr.GetMlrState() == kMlrStateRegistering)
        {
            bool success = aSuccess || !aFailedAddresses.IsEmptyOrContains(addr.GetAddress());

            addr.SetMlrState(success ? kMlrStateRegistered : kMlrStateToRegister);
        }
    }
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            if (child.GetAddressMlrState(address) == kMlrStateRegistering)
            {
                bool success = aSuccess || !aFailedAddresses.IsEmptyOrContains(address);

                child.SetAddressMlrState(address, success ? kMlrStateRegistered : kMlrStateToRegister);
            }
        }
    }
#endif

    LogMulticastAddresses();
    CheckInvariants();
}

void MlrManager::HandleTimeTick(void)
{
    if (mSendDelay > 0 && --mSendDelay == 0)
    {
        SendMlr();
    }

    if (mReregistrationDelay > 0 && --mReregistrationDelay == 0)
    {
        Reregister();
    }

    UpdateTimeTickerRegistration();
}

void MlrManager::Reregister(void)
{
    LogInfo("MLR Reregister!");

    SetMulticastAddressMlrState(kMlrStateRegistered, kMlrStateToRegister);
    CheckInvariants();

    ScheduleSend(0);

    // Schedule for the next renewing.
    UpdateReregistrationDelay(false);
}

void MlrManager::UpdateReregistrationDelay(bool aRereg)
{
    Mle::MleRouter &mle = Get<Mle::MleRouter>();

    bool needSendMlr = (mle.IsFullThreadDevice() || mle.GetParent().IsThreadVersion1p1()) &&
                       Get<BackboneRouter::Leader>().HasPrimary();

    if (!needSendMlr)
    {
        mReregistrationDelay = 0;
    }
    else
    {
        BackboneRouter::Config config;
        uint32_t               reregDelay;
        uint32_t               effectiveMlrTimeout;

        IgnoreError(Get<BackboneRouter::Leader>().GetConfig(config));

        if (aRereg)
        {
            reregDelay = config.mReregistrationDelay > 1
                             ? Random::NonCrypto::GetUint16InRange(1, config.mReregistrationDelay)
                             : 1;
        }
        else
        {
            // Calculate renewing period according to Thread Spec. 5.24.2.3.2
            // The random time t SHOULD be chosen such that (0.5* MLR-Timeout) < t < (MLR-Timeout – 9 seconds).
            effectiveMlrTimeout = Max(config.mMlrTimeout, BackboneRouter::kMinMlrTimeout);
            reregDelay = Random::NonCrypto::GetUint32InRange((effectiveMlrTimeout >> 1u) + 1, effectiveMlrTimeout - 9);
        }

        if (mReregistrationDelay == 0 || mReregistrationDelay > reregDelay)
        {
            mReregistrationDelay = reregDelay;
        }
    }

    UpdateTimeTickerRegistration();

    LogDebg("MlrManager::UpdateReregistrationDelay: rereg=%d, needSendMlr=%d, ReregDelay=%lu", aRereg, needSendMlr,
            ToUlong(mReregistrationDelay));
}

void MlrManager::LogMulticastAddresses(void)
{
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
    LogDebg("-------- Multicast Addresses --------");

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (const Ip6::Netif::ExternalMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        LogDebg("%-32s%c", addr.GetAddress().ToString().AsCString(), "-rR"[addr.GetMlrState()]);
    }
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            LogDebg("%-32s%c %04x", address.ToString().AsCString(), "-rR"[child.GetAddressMlrState(address)],
                    child.GetRloc16());
        }
    }
#endif

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_DEBG)
}

void MlrManager::AddressArray::AddUnique(const Ip6::Address &aAddress)
{
    if (!Contains(aAddress))
    {
        IgnoreError(PushBack(aAddress));
    }
}

void MlrManager::LogMlrResponse(Error aResult, Error aError, uint8_t aStatus, const AddressArray &aFailedAddresses)
{
    OT_UNUSED_VARIABLE(aResult);
    OT_UNUSED_VARIABLE(aError);
    OT_UNUSED_VARIABLE(aStatus);
    OT_UNUSED_VARIABLE(aFailedAddresses);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_WARN)
    if (aResult == kErrorNone && aError == kErrorNone && aStatus == ThreadStatusTlv::kMlrSuccess)
    {
        LogInfo("Receive MLR.rsp OK");
    }
    else
    {
        LogWarn("Receive MLR.rsp: result=%s, error=%s, status=%d, failedAddressNum=%d", ErrorToString(aResult),
                ErrorToString(aError), aStatus, aFailedAddresses.GetLength());

        for (const Ip6::Address &address : aFailedAddresses)
        {
            LogWarn("MA failed: %s", address.ToString().AsCString());
        }
    }
#endif
}

void MlrManager::CheckInvariants(void) const
{
#if OPENTHREAD_EXAMPLES_SIMULATION && OPENTHREAD_CONFIG_ASSERT_ENABLE
    uint16_t registeringNum = 0;

    OT_ASSERT(!mMlrPending || mSendDelay == 0);

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (Ip6::Netif::ExternalMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        registeringNum += (addr.GetMlrState() == kMlrStateRegistering);
    }
#endif
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            registeringNum += (child.GetAddressMlrState(address) == kMlrStateRegistering);
        }
    }
#endif

    OT_ASSERT(registeringNum == 0 || mMlrPending);
#endif // OPENTHREAD_EXAMPLES_SIMULATION
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE
