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

#if OPENTHREAD_CONFIG_MLR_ENABLE

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
    if (aEvents.Contains(kEventIp6MulticastSubscribed))
    {
        ScheduleSend(0);
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
    OT_UNUSED_VARIABLE(aConfig);

    bool needRereg =
        aState == BackboneRouter::Leader::kStateAdded || aState == BackboneRouter::Leader::kStateToTriggerRereg;

    UpdateReregistrationDelay(needRereg);
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

    num = static_cast<uint8_t>(CountNetifMulticastAddressesToRegister());
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

    for (Ip6::ExternalNetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        if (addr.GetAddress().IsMulticastLargerThanRealmLocal() && addr.GetMlrState() == kMlrStateToRegister)
        {
            SuccessOrExit(error = message->Append(&addr.GetAddress(), sizeof(Ip6::Address)));
            addr.SetMlrState(kMlrStateRegistering);

            if (--num == 0)
            {
                break;
            }
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

        SetNetifMulticastAddressMlrState(kMlrStateRegistering, kMlrStateToRegister);

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

    if (status == ThreadStatusTlv::MlrStatus::kMlrSuccess)
    {
        SetNetifMulticastAddressMlrState(kMlrStateRegistering, kMlrStateRegistered);
        // keep sending until all multicast addresses are registered.
        ScheduleSend(0);
    }
    else
    {
        otBackboneRouterConfig config;
        uint16_t               reregDelay;

        SetNetifMulticastAddressMlrState(kMlrStateRegistering, kMlrStateToRegister);

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

    otLogInfoMlr("Receive MLR.rsp, result=%s, status=%d, error=%s", otThreadErrorToString(aResult), status,
                 otThreadErrorToString(error));
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
    SetNetifMulticastAddressMlrState(kMlrStateRegistered, kMlrStateToRegister);
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
#if OPENTHREAD_CONFIG_LOG_BBR && OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
    otLogDebgMlr("-------- Multicast Addresses --------");

    for (const Ip6::ExternalNetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        if (addr.GetAddress().IsMulticastLargerThanRealmLocal())
        {
            MlrState state = addr.GetMlrState();

            otLogDebgMlr("%-32s%c", addr.GetAddress().ToString().AsCString(), "-rR"[state]);
        }
    }
#endif
}

uint16_t MlrManager::CountNetifMulticastAddressesToRegister(void) const
{
    uint16_t count = 0;

    for (const Ip6::ExternalNetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        if (addr.GetAddress().IsMulticastLargerThanRealmLocal() && addr.GetMlrState() == kMlrStateToRegister)
        {
            count++;
        }
    }

    return count;
}

void MlrManager::SetNetifMulticastAddressMlrState(MlrState aFromState, MlrState aToState)
{
    for (Ip6::ExternalNetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
    {
        if (addr.GetAddress().IsMulticastLargerThanRealmLocal() && addr.GetMlrState() == aFromState)
        {
            addr.SetMlrState(aToState);
        }
    }
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MLR_ENABLE
