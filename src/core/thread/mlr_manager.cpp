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

#if (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_MLR_ENABLE

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
    , mTable(aInstance)
    , mTimer(aInstance, MlrManager::HandleTimer, this)
    , mReregistrationDelay(0)
    , mSendDelay(0)
    , mMlrPending(false)
{
}

void MlrManager::HandleNotifierEvents(Notifier::Receiver &aReceiver, Events aEvents)
{
    static_cast<MlrManager &>(aReceiver).HandleNotifierEvents(aEvents);
}

void MlrManager::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.ContainsAny(kEventIp6MulticastSubscribed | kEventIp6MulticastUnsubscribed))
    {
        UpdateLocalSubscriptions();
    }

    if (aEvents.Contains(kEventThreadRoleChanged) && Get<Mle::MleRouter>().IsChild())
    {
        // Reregistration after re-attach
        UpdateReregistrationDelay(true);
    }
}

void MlrManager::HandleBackboneRouterPrimaryUpdate(BackboneRouter::Leader::State               aState,
                                                   const BackboneRouter::BackboneRouterConfig &aConfig)
{
    bool needRereg =
        aState == BackboneRouter::Leader::kStateAdded || aState == BackboneRouter::Leader::kStateToTriggerRereg;

    OT_UNUSED_VARIABLE(aConfig);
    UpdateReregistrationDelay(needRereg);
}

void MlrManager::UpdateLocalSubscriptions(void)
{
    // Unsubscribe all multicast addresses that are not on Thread Netif
    for (MulticastListenerRegistrationTable::Iterator iter(GetInstance(),
                                                           MulticastListenerRegistrationTable::kFilterSubscribed);
         !iter.IsDone(); iter.Advance())
    {
        MulticastListenerRegistration &registration = iter.GetMulticastListenerRegistration();

        if (!Get<ThreadNetif>().IsMulticastSubscribed(registration.GetAddress()))
        {
            ot::MulticastListenerRegistrationTable::SetSubscribed(registration, false);
        }
    }

    // Subscribe all multicast addresses that are on Thread Netif
    for (const Ip6::NetifMulticastAddress *addr = Get<ThreadNetif>().GetMulticastAddresses(); addr != nullptr;
         addr                                   = addr->GetNext())
    {
        if (!addr->GetAddress().IsMulticastLargerThanRealmLocal())
        {
            continue;
        }

        mTable.SetSubscribed(addr->GetAddress(), true);
    }

    ScheduleSend(0);
    LogMulticastRegistrationTable();
}

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
    uint8_t          num;

    VerifyOrExit(!mMlrPending, error = OT_ERROR_BUSY);
    VerifyOrExit(mle.IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(mle.IsFullThreadDevice() || mle.GetParent().IsThreadVersion1p1(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = OT_ERROR_INVALID_STATE);

    num = mTable.Count(MulticastListenerRegistrationTable::kFilterNotRegistered);
    VerifyOrExit(num > 0, error = OT_ERROR_NOT_FOUND);
    if (num > kIPv6AddressesNumMax)
    {
        num = kIPv6AddressesNumMax;
    }

    VerifyOrExit((message = Get<Coap::Coap>().NewMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(message->SetToken(Coap::Message::kDefaultTokenLength));
    SuccessOrExit(message->AppendUriPathOptions(OT_URI_PATH_MLR));
    SuccessOrExit(message->SetPayloadMarker());

    addressesTlv.Init();
    addressesTlv.SetLength(sizeof(Ip6::Address) * num);
    SuccessOrExit(error = message->Append(&addressesTlv, sizeof(addressesTlv)));

    {
        MulticastListenerRegistrationTable::Iterator iter(GetInstance(),
                                                          MulticastListenerRegistrationTable::kFilterNotRegistered);
        for (uint8_t i = 0; i < num; i++, iter.Advance())
        {
            OT_ASSERT(!iter.IsDone());
            SuccessOrExit(
                error = message->Append(&iter.GetMulticastListenerRegistration().GetAddress(), sizeof(Ip6::Address)));
        }
    }

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

exit:
    if (error == OT_ERROR_NONE)
    {
        mMlrPending = true;
        mTable.SetRegistering(num);
    }
    else
    {
        if (message != nullptr)
        {
            message->Free();
        }

        if (error == OT_ERROR_NO_BUFS)
        {
            ScheduleSend(1);
        }
    }

    otLogInfoBbr("Send MLR.req: %s", otThreadErrorToString(error));
    LogMulticastRegistrationTable();
}

void MlrManager::HandleMulticastListenerRegistrationResponse(Coap::Message *         aMessage,
                                                             const Ip6::MessageInfo *aMessageInfo,
                                                             otError                 aResult)
{
    uint8_t status = ThreadStatusTlv::MlrStatus::kMlrGeneralFailure;
    otError error;

    OT_UNUSED_VARIABLE(aMessageInfo);

    mMlrPending = false;

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage != nullptr, error = OT_ERROR_PARSE);
    VerifyOrExit(aMessage->GetCode() == OT_COAP_CODE_CHANGED, error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::FindUint8Tlv(*aMessage, ThreadTlv::kStatus, status));

exit:

    if (status == ThreadStatusTlv::MlrStatus::kMlrSuccess)
    {
        mTable.FinishRegistering(true);
        // keep sending until all multicast addresses are registered.
        ScheduleSend(0);
    }
    else
    {
        // Conservatively schedule reregistration if failed.
        // This is required by Test Spec. MATN-TC-26
        // TODO (MLR): reregister only failed addresses.
        mTable.FinishRegistering(false);
        UpdateReregistrationDelay(true);
    }

    otLogInfoBbr("Receive MLR.rsp, result=%s, status=%d, error=%s", otThreadErrorToString(aResult), status,
                 otThreadErrorToString(error));
}

void MlrManager::HandleTimer(void)
{
    if (mSendDelay > 0)
    {
        if (--mSendDelay == 0)
        {
            SendMulticastListenerRegistration();
        }
    }

    if (mReregistrationDelay > 0)
    {
        if (--mReregistrationDelay == 0)
        {
            Reregistration();
        }
    }

    ResetTimer();
}

void MlrManager::Reregistration(void)
{
    mTable.SetAllToRegister();
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
        otBackboneRouterConfig config;
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

    otLogDebgBbr("MlrManager::UpdateReregistrationDelay: rereg=%d, needSendMlr=%d, ReregDelay=%lu", aRereg, needSendMlr,
                 mReregistrationDelay);
}

} // namespace ot

#endif // (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_MLR_ENABLE
