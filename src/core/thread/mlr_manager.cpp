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

#if OPENTHREAD_CONFIG_MLR_ENABLE || OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "net/ip6_address.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"
#include "utils/slaac_address.hpp"

namespace ot {

MlrManager::MlrManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , Notifier::Receiver(aInstance, MlrManager::HandleNotifierEvents)
    , mTimer(aInstance, MlrManager::HandleTimer, this)
    , mReregistrationDelay(0)
    , mSendDelay(0)
    , mMlrPending(false)
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

void MlrManager::HandleBackboneRouterPrimaryUpdate(BackboneRouter::Leader::State               aState,
                                                   const BackboneRouter::BackboneRouterConfig &aConfig)
{
    OT_UNUSED_VARIABLE(aConfig);

    bool needRereg =
        aState == BackboneRouter::Leader::kStateAdded || aState == BackboneRouter::Leader::kStateToTriggerRereg;

    UpdateReregistrationDelay(needRereg);
}

#if OPENTHREAD_CONFIG_MLR_ENABLE
void MlrManager::UpdateLocalSubscriptions(void)
{
#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    // Check multicast addresses are newly listened against Children
    for (Ip6::ExternalNetifMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (addr.GetMlrState() == kMlrStateToRegister && IsAddressMlrRegisteredByAnyChild(addr.GetAddress()))
        {
            addr.SetMlrState(kMlrStateRegistered);
        }
    }
#endif

    ScheduleSend(0);
}

bool MlrManager::IsAddressMlrRegisteredByNetif(const Ip6::Address &aAddress) const
{
    bool ret = false;

    OT_ASSERT(aAddress.IsMulticastLargerThanRealmLocal());

    for (const Ip6::ExternalNetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        if (addr.GetAddress() == aAddress && addr.GetMlrState() == kMlrStateRegistered)
        {
            ExitNow(ret = true);
        }
    }

exit:
    return ret;
}

void MlrManager::SetNetifMulticastAddressMlrState(MlrState aFromState, MlrState aToState)
{
    for (Ip6::ExternalNetifMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (addr.GetMlrState() == aFromState)
        {
            addr.SetMlrState(aToState);
        }
    }
}
#endif // OPENTHREAD_CONFIG_MLR_ENABLE

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

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

void MlrManager::UpdateProxiedSubscriptions(Child &             aChild,
                                            const Ip6::Address *aOldMlrRegisteredAddresses,
                                            uint16_t            aOldMlrRegisteredAddressNum)
{
    VerifyOrExit(aChild.IsStateValid(), OT_NOOP);

    // Search the new multicast addresses and set its flag accordingly
    for (const Ip6::Address &address : aChild.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        bool isMlrRegistered = false;

        // Check if it's a new multicast address against old addresses
        for (size_t i = 0; i < aOldMlrRegisteredAddressNum; i++)
        {
            if (aOldMlrRegisteredAddresses[i] == address)
            {
                isMlrRegistered = true;
                break;
            }
        }

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

    if (aChild.HasAnyMlrToRegisterAddress())
    {
        ScheduleSend(Random::NonCrypto::GetUint16InRange(1, Mle::kParentAggregateDelay));
    }
}

void MlrManager::SetChildMulticastAddressMlrState(MlrState aFromState, MlrState aToState)
{
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
}
#endif // OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE

void MlrManager::ScheduleSend(uint16_t aDelay)
{
    OT_ASSERT(!mMlrPending || mSendDelay == 0);

    VerifyOrExit(!mMlrPending, OT_NOOP);

    if (aDelay == 0)
    {
        mSendDelay = 0;
        SendMulticastListenerRegistration();
    }
    else if (mSendDelay == 0 || mSendDelay > aDelay)
    {
        mSendDelay = aDelay;
    }

    ResetTimer();
exit:
    return;
}

void MlrManager::ResetTimer(void)
{
    if (mSendDelay == 0 && mReregistrationDelay == 0)
    {
        mTimer.Stop();
    }
    else if (!mTimer.IsRunning())
    {
        mTimer.Start(kTimerInterval);
    }
}

void MlrManager::SendMulticastListenerRegistration(void)
{
    otError          error   = OT_ERROR_NONE;
    Mle::MleRouter & mle     = Get<Mle::MleRouter>();
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;
    IPv6AddressesTlv addressesTlv;
    Ip6::Address     addresses[kIPv6AddressesNumMax];
    uint8_t          addressesNum = 0;

    VerifyOrExit(!mMlrPending, error = OT_ERROR_BUSY);
    VerifyOrExit(mle.IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mle.IsFullThreadDevice() || mle.GetParent().IsThreadVersion1p1(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = OT_ERROR_INVALID_STATE);

#if OPENTHREAD_CONFIG_MLR_ENABLE
    // Append Netif multicast addresses
    for (Ip6::ExternalNetifMulticastAddress &addr :
         Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
    {
        if (addressesNum >= kIPv6AddressesNumMax)
        {
            break;
        }

        if (addr.GetMlrState() == kMlrStateToRegister)
        {
            AppendToUniqueAddressList(addresses, addressesNum, addr.GetAddress());
            addr.SetMlrState(kMlrStateRegistering);
        }
    }
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    // Append Child multicast addresses
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        if (addressesNum >= kIPv6AddressesNumMax)
        {
            break;
        }

        if (!child.HasAnyMlrToRegisterAddress())
        {
            continue;
        }

        for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            if (addressesNum >= kIPv6AddressesNumMax)
            {
                break;
            }

            if (child.GetAddressMlrState(address) == kMlrStateToRegister)
            {
                AppendToUniqueAddressList(addresses, addressesNum, address);
                child.SetAddressMlrState(address, kMlrStateRegistering);
            }
        }
    }
#endif

    VerifyOrExit(addressesNum > 0, error = OT_ERROR_NOT_FOUND);

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(message->SetToken(Coap::Message::kDefaultTokenLength));
    SuccessOrExit(message->AppendUriPathOptions(OT_URI_PATH_MLR));
    SuccessOrExit(message->SetPayloadMarker());

    addressesTlv.Init();
    addressesTlv.SetLength(sizeof(Ip6::Address) * addressesNum);
    SuccessOrExit(error = message->Append(&addressesTlv, sizeof(addressesTlv)));
    SuccessOrExit(error = message->Append(&addresses, sizeof(Ip6::Address) * addressesNum));

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

    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetSockAddr(mle.GetMeshLocal16());

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(
                      *message, messageInfo, &MlrManager::HandleMulticastListenerRegistrationResponse, this));

    mMlrPending = true;

    // TODO: not enable fast polls for SSED
    if (!Get<Mle::Mle>().IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SendFastPolls(DataPollSender::kDefaultFastPolls);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        if (message != nullptr)
        {
            message->Free();
        }

        SetMulticastAddressMlrState(kMlrStateRegistering, kMlrStateToRegister);

        if (error == OT_ERROR_NO_BUFS)
        {
            ScheduleSend(1);
        }
    }

    otLogInfoMlr("Send MLR.req: %s", otThreadErrorToString(error));
    LogMulticastAddresses();
}

void MlrManager::HandleMulticastListenerRegistrationResponse(Coap::Message *         aMessage,
                                                             const Ip6::MessageInfo *aMessageInfo,
                                                             otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    uint8_t status = ThreadStatusTlv::MlrStatus::kMlrGeneralFailure;
    otError error;

    mMlrPending = false;

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage != nullptr, error = OT_ERROR_PARSE);
    VerifyOrExit(aMessage->GetCode() == OT_COAP_CODE_CHANGED, error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::FindUint8Tlv(*aMessage, ThreadTlv::kStatus, status));

exit:
    SetMulticastAddressMlrState(kMlrStateRegistering, status == ThreadStatusTlv::MlrStatus::kMlrSuccess
                                                          ? kMlrStateRegistered
                                                          : kMlrStateToRegister);
    otLogInfoBbr("Receive MLR.rsp, result=%s, status=%d, error=%s", otThreadErrorToString(aResult), status,
                 otThreadErrorToString(error));
    LogMulticastAddresses();

    if (status == ThreadStatusTlv::MlrStatus::kMlrSuccess)
    {
        // keep sending until all multicast addresses are registered.
        ScheduleSend(0);
    }
    else
    {
        otBackboneRouterConfig config;
        uint16_t               reregDelay;

        // The Device has just attempted a Multicast Listener Registration which failed, and it retries the same
        // registration with a random time delay chosen in the interval [0, Reregistration Delay].
        // This is required by Thread 1.2 Specification 5.24.2.3
        // TODO (MLR): reregister only failed addresses.
        if (Get<BackboneRouter::Leader>().GetConfig(config) == OT_ERROR_NONE)
        {
            reregDelay = config.mReregistrationDelay > 1
                             ? Random::NonCrypto::GetUint16InRange(1, config.mReregistrationDelay)
                             : 1;
            ScheduleSend(reregDelay);
        }
    }
}

void MlrManager::HandleTimer(void)
{
    if (mSendDelay > 0 && --mSendDelay == 0)
    {
        SendMulticastListenerRegistration();
    }

    if (mReregistrationDelay > 0 && --mReregistrationDelay == 0)
    {
        Reregister();
    }

    ResetTimer();
}

void MlrManager::Reregister(void)
{
    SetMulticastAddressMlrState(kMlrStateRegistered, kMlrStateToRegister);

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
        BackboneRouter::BackboneRouterConfig config;
        uint32_t                             reregDelay;
        uint32_t                             effectiveMlrTimeout;

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
            effectiveMlrTimeout = config.mMlrTimeout > Mle::kMlrTimeoutMin ? config.mMlrTimeout
                                                                           : static_cast<uint32_t>(Mle::kMlrTimeoutMin);
            reregDelay = Random::NonCrypto::GetUint32InRange((effectiveMlrTimeout >> 1u) + 1, effectiveMlrTimeout - 9);
        }

        if (mReregistrationDelay == 0 || mReregistrationDelay > reregDelay)
        {
            mReregistrationDelay = reregDelay;
        }
    }

    ResetTimer();

    otLogDebgMlr("MlrManager::UpdateReregistrationDelay: rereg=%d, needSendMlr=%d, ReregDelay=%lu", aRereg, needSendMlr,
                 mReregistrationDelay);
}

void MlrManager::LogMulticastAddresses(void)
{
#if OPENTHREAD_CONFIG_LOG_MLR && OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
    otLogDebgMlr("-------- Multicast Addresses --------");

#if OPENTHREAD_CONFIG_MLR_ENABLE
    for (const Ip6::ExternalNetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        otLogDebgMlr("%-32s%c", addr.GetAddress().ToString().AsCString(), "-rR"[addr.GetMlrState()]);
    }
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValid))
    {
        for (const Ip6::Address &address : child.IterateIp6Addresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal))
        {
            otLogDebgMlr("%-32s%c %04x", address.ToString().AsCString(), "-rR"[child.GetAddressMlrState(address)],
                         child.GetRloc16());
        }
    }
#endif

#endif // OPENTHREAD_CONFIG_LOG_MLR && OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
}

void MlrManager::AppendToUniqueAddressList(Ip6::Address (&aAddresses)[kIPv6AddressesNumMax],
                                           uint8_t &           aAddressNum,
                                           const Ip6::Address &aAddress)
{
#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
    for (uint8_t i = 0; i < aAddressNum; i++)
    {
        if (aAddresses[i] == aAddress)
        {
            ExitNow();
        }
    }
#endif

    aAddresses[aAddressNum++] = aAddress;

#if OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
exit:
#endif
    return;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE
