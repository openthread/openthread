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
 *   This file implements managing DUA.
 */

#include "dua_manager.hpp"

#if OPENTHREAD_CONFIG_DUA_ENABLE || OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/settings.hpp"
#include "net/ip6_address.hpp"
#include "thread/mle_types.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"
#include "utils/slaac_address.hpp"

namespace ot {

DuaManager::DuaManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRegistrationTask(aInstance, DuaManager::HandleRegistrationTask, this)
    , mDuaNotification(UriPath::kDuaRegistrationNotify, &DuaManager::HandleDuaNotification, this)
    , mIsDuaPending(false)
#if OPENTHREAD_CONFIG_DUA_ENABLE
    , mDuaState(kNotExist)
    , mDadCounter(0)
    , mLastRegistrationTime(0)
#endif
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    , mChildIndexDuaRegistering(0)
    , mRegisterCurrentChildIndex(false)
#endif
{
    mDelay.mValue = 0;

#if OPENTHREAD_CONFIG_DUA_ENABLE
    mDomainUnicastAddress.InitAsThreadOriginGlobalScope();
    mFixedDuaInterfaceIdentifier.Clear();
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    mChildDuaMask.Clear();
    mChildDuaRegisteredMask.Clear();
#endif

    Get<Tmf::TmfAgent>().AddResource(mDuaNotification);
}

void DuaManager::HandleDomainPrefixUpdate(BackboneRouter::Leader::DomainPrefixState aState)
{
    if ((aState == BackboneRouter::Leader::kDomainPrefixRemoved) ||
        (aState == BackboneRouter::Leader::kDomainPrefixRefreshed))
    {
        if (mIsDuaPending)
        {
            IgnoreError(Get<Tmf::TmfAgent>().AbortTransaction(&DuaManager::HandleDuaResponse, this));
        }

#if OPENTHREAD_CONFIG_DUA_ENABLE
        RemoveDomainUnicastAddress();
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
        if (mChildDuaMask.HasAny())
        {
            mChildDuaMask.Clear();
            mChildDuaRegisteredMask.Clear();
            mRegisterCurrentChildIndex = false;
        }
#endif
    }

#if OPENTHREAD_CONFIG_DUA_ENABLE
    switch (aState)
    {
    case BackboneRouter::Leader::kDomainPrefixUnchanged:
        // In case removed for some reason e.g. the kDuaInvalid response from PBBR forcely
        VerifyOrExit(!Get<ThreadNetif>().HasUnicastAddress(GetDomainUnicastAddress()), OT_NOOP);

        // fall through
    case BackboneRouter::Leader::kDomainPrefixRefreshed:
    case BackboneRouter::Leader::kDomainPrefixAdded:
    {
        const Ip6::Prefix *prefix = Get<BackboneRouter::Leader>().GetDomainPrefix();
        OT_ASSERT(prefix != nullptr);
        mDomainUnicastAddress.mPrefixLength = prefix->GetLength();
        mDomainUnicastAddress.GetAddress().Clear();
        mDomainUnicastAddress.GetAddress().SetPrefix(*prefix);
    }
    break;
    default:
        ExitNow();
    }

    // Apply cached DUA Interface Identifier manually specified.
    if (IsFixedDuaInterfaceIdentifierSet())
    {
        mDomainUnicastAddress.GetAddress().SetIid(mFixedDuaInterfaceIdentifier);
    }
    else
    {
        SuccessOrExit(GenerateDomainUnicastAddressIid());
    }

    AddDomainUnicastAddress();

exit:
    return;
#endif
}

#if OPENTHREAD_CONFIG_DUA_ENABLE
otError DuaManager::GenerateDomainUnicastAddressIid(void)
{
    otError error;
    uint8_t dadCounter = mDadCounter;

    if ((error = Get<Utils::Slaac>().GenerateIid(mDomainUnicastAddress, nullptr, 0, &dadCounter)) == OT_ERROR_NONE)
    {
        if (dadCounter != mDadCounter)
        {
            mDadCounter = dadCounter;
            IgnoreError(Store());
        }

        otLogInfoDua("Generated DUA: %s", mDomainUnicastAddress.GetAddress().ToString().AsCString());
    }
    else
    {
        otLogWarnDua("Generate DUA: %s", otThreadErrorToString(error));
    }

    return error;
}

otError DuaManager::SetFixedDuaInterfaceIdentifier(const Ip6::InterfaceIdentifier &aIid)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!aIid.IsReserved(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(mFixedDuaInterfaceIdentifier.IsUnspecified() || mFixedDuaInterfaceIdentifier != aIid, OT_NOOP);

    mFixedDuaInterfaceIdentifier = aIid;
    otLogInfoDua("Set DUA IID: %s", mFixedDuaInterfaceIdentifier.ToString().AsCString());

    if (Get<ThreadNetif>().HasUnicastAddress(GetDomainUnicastAddress()))
    {
        RemoveDomainUnicastAddress();
        mDomainUnicastAddress.GetAddress().SetIid(mFixedDuaInterfaceIdentifier);
        AddDomainUnicastAddress();
    }

exit:
    return error;
}

void DuaManager::ClearFixedDuaInterfaceIdentifier(void)
{
    // Nothing to clear.
    VerifyOrExit(IsFixedDuaInterfaceIdentifierSet(), OT_NOOP);

    if (GetDomainUnicastAddress().GetIid() == mFixedDuaInterfaceIdentifier &&
        Get<ThreadNetif>().HasUnicastAddress(GetDomainUnicastAddress()))
    {
        RemoveDomainUnicastAddress();

        if (GenerateDomainUnicastAddressIid() == OT_ERROR_NONE)
        {
            AddDomainUnicastAddress();
        }
    }

    otLogInfoDua("Cleared DUA IID: %s", mFixedDuaInterfaceIdentifier.ToString().AsCString());
    mFixedDuaInterfaceIdentifier.Clear();

exit:
    return;
}

void DuaManager::Restore(void)
{
    Settings::DadInfo dadInfo;

    SuccessOrExit(Get<Settings>().ReadDadInfo(dadInfo));
    mDadCounter = dadInfo.GetDadCounter();

exit:
    return;
}

otError DuaManager::Store(void)
{
    Settings::DadInfo dadInfo;

    dadInfo.SetDadCounter(mDadCounter);
    return Get<Settings>().SaveDadInfo(dadInfo);
}

void DuaManager::AddDomainUnicastAddress(void)
{
    mDuaState             = kToRegister;
    mLastRegistrationTime = TimerMilli::GetNow();
    Get<ThreadNetif>().AddUnicastAddress(mDomainUnicastAddress);
}

void DuaManager::RemoveDomainUnicastAddress(void)
{
    if (mDuaState == kRegistering && mIsDuaPending)
    {
        IgnoreError(Get<Tmf::TmfAgent>().AbortTransaction(&DuaManager::HandleDuaResponse, this));
    }

    mDuaState                        = kNotExist;
    mDomainUnicastAddress.mPreferred = false;
    Get<ThreadNetif>().RemoveUnicastAddress(mDomainUnicastAddress);
}

void DuaManager::UpdateRegistrationDelay(uint8_t aDelay)
{
    if (mDelay.mFields.mRegistrationDelay == 0 || mDelay.mFields.mRegistrationDelay > aDelay)
    {
        mDelay.mFields.mRegistrationDelay = aDelay;

        otLogDebgDua("update regdelay %d", mDelay.mFields.mRegistrationDelay);
        UpdateTimeTickerRegistration();
    }
}
#endif

void DuaManager::UpdateReregistrationDelay(void)
{
    uint16_t               delay = 0;
    otBackboneRouterConfig config;

    VerifyOrExit(Get<BackboneRouter::Leader>().GetConfig(config) == OT_ERROR_NONE, OT_NOOP);

    delay = config.mReregistrationDelay > 1 ? Random::NonCrypto::GetUint16InRange(1, config.mReregistrationDelay) : 1;

    if (mDelay.mFields.mReregistrationDelay == 0 || mDelay.mFields.mReregistrationDelay > delay)
    {
        mDelay.mFields.mReregistrationDelay = delay;
        UpdateTimeTickerRegistration();
        otLogDebgDua("update reregdelay %d", mDelay.mFields.mReregistrationDelay);
    }

exit:
    return;
}

void DuaManager::UpdateCheckDelay(uint8_t aDelay)
{
    if (mDelay.mFields.mCheckDelay == 0 || mDelay.mFields.mCheckDelay > aDelay)
    {
        mDelay.mFields.mCheckDelay = aDelay;

        otLogDebgDua("update checkdelay %d", mDelay.mFields.mCheckDelay);
        UpdateTimeTickerRegistration();
    }
}

void DuaManager::HandleNotifierEvents(Events aEvents)
{
    Mle::MleRouter &mle = Get<Mle::MleRouter>();

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        if (mle.HasRestored())
        {
            UpdateReregistrationDelay();
        }
#if OPENTHREAD_CONFIG_DUA_ENABLE && OPENTHREAD_FTD
        else if (mle.IsRouter())
        {
            // Wait for link establishment with neighboring routers.
            UpdateRegistrationDelay(kNewRouterRegistrationDelay);
        }
        else if (mle.IsExpectedToBecomeRouter())
        {
            // Will check again in case the device decides to stay REED when jitter timeout expires.
            UpdateRegistrationDelay(mle.GetRouterSelectionJitterTimeout() + 1);
        }
#endif
    }

    if (aEvents.ContainsAny(kEventIp6AddressAdded))
    {
        mRegistrationTask.Post();
    }
}

void DuaManager::HandleBackboneRouterPrimaryUpdate(BackboneRouter::Leader::State               aState,
                                                   const BackboneRouter::BackboneRouterConfig &aConfig)
{
    OT_UNUSED_VARIABLE(aConfig);

    if (aState == BackboneRouter::Leader::kStateAdded || aState == BackboneRouter::Leader::kStateToTriggerRereg)
    {
        UpdateReregistrationDelay();
    }
}

void DuaManager::HandleTimeTick(void)
{
    bool attempt = false;

#if OPENTHREAD_CONFIG_DUA_ENABLE
    otLogDebgDua("regdelay %d, reregdelay %d, checkdelay %d", mDelay.mFields.mRegistrationDelay,
                 mDelay.mFields.mReregistrationDelay, mDelay.mFields.mCheckDelay);

    if (mDuaState != kNotExist && TimerMilli::GetNow() > mLastRegistrationTime + Mle::kDuaDadPeriod)
    {
        mDomainUnicastAddress.mPreferred = true;
    }

    if ((mDelay.mFields.mRegistrationDelay > 0) && (--mDelay.mFields.mRegistrationDelay == 0))
    {
        attempt = true;
    }
#else
    otLogDebgDua("reregdelay %d, checkdelay %d", mDelay.mFields.mReregistrationDelay, mDelay.mFields.mCheckDelay);
#endif

    if ((mDelay.mFields.mCheckDelay > 0) && (--mDelay.mFields.mCheckDelay == 0))
    {
        attempt = true;
    }

    if ((mDelay.mFields.mReregistrationDelay > 0) && (--mDelay.mFields.mReregistrationDelay == 0))
    {
#if OPENTHREAD_CONFIG_DUA_ENABLE
        if (mDuaState != kNotExist)
        {
            mDuaState = kToRegister;
        }
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
        mChildDuaRegisteredMask.Clear();
#endif
        attempt = true;
    }

    if (attempt)
    {
        mRegistrationTask.Post();
    }

    UpdateTimeTickerRegistration();
}

void DuaManager::UpdateTimeTickerRegistration(void)
{
    if (mDelay.mValue == 0)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kDuaManager);
    }
    else
    {
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kDuaManager);
    }
}

void DuaManager::PerformNextRegistration(void)
{
    otError          error   = OT_ERROR_NONE;
    Mle::MleRouter & mle     = Get<Mle::MleRouter>();
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;
    Ip6::Address     dua;

    VerifyOrExit(mle.IsAttached(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = OT_ERROR_INVALID_STATE);

    // Only allow one outgoing DUA.req
    VerifyOrExit(!mIsDuaPending, error = OT_ERROR_BUSY);

    // Only send DUA.req when necessary
#if OPENTHREAD_CONFIG_DUA_ENABLE
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    VerifyOrExit(mle.IsRouterOrLeader() || !mle.IsExpectedToBecomeRouter(), error = OT_ERROR_INVALID_STATE);
    VerifyOrExit((mDuaState == kToRegister && mDelay.mFields.mRegistrationDelay == 0) ||
                     (mChildDuaMask.HasAny() && mChildDuaMask != mChildDuaRegisteredMask),
                 error = OT_ERROR_NOT_FOUND);
#else
    VerifyOrExit(mDuaState == kToRegister && mDelay.mFields.mRegistrationDelay == 0, error = OT_ERROR_NOT_FOUND);
#endif // OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

    VerifyOrExit(mle.IsFullThreadDevice() || mle.GetParent().IsThreadVersion1p1(), error = OT_ERROR_INVALID_STATE);
#endif // OPENTHREAD_CONFIG_DUA_ENABLE

    // Prepare DUA.req
    VerifyOrExit((message = Get<Tmf::TmfAgent>().NewPriorityMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsConfirmablePost(UriPath::kDuaRegistrationRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

#if OPENTHREAD_CONFIG_DUA_ENABLE
    if (mDuaState == kToRegister && mDelay.mFields.mRegistrationDelay == 0)
    {
        dua = GetDomainUnicastAddress();
        SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, &dua, sizeof(dua)));
        SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kMeshLocalEid, &mle.GetMeshLocal64().GetIid(),
                                             sizeof(Ip6::InterfaceIdentifier)));
        mDuaState             = kRegistering;
        mLastRegistrationTime = TimerMilli::GetNow();
    }
    else
#endif // OPENTHREAD_CONFIG_DUA_ENABLE
    {
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
        uint32_t            lastTransactionTime;
        const Ip6::Address *duaPtr = nullptr;
        Child *             child  = nullptr;

        if (!mRegisterCurrentChildIndex)
        {
            for (Child &iter : Get<ChildTable>().Iterate(Child::kInStateValid))
            {
                uint16_t childIndex = Get<ChildTable>().GetChildIndex(iter);

                if (mChildDuaMask.Get(childIndex) && !mChildDuaRegisteredMask.Get(childIndex))
                {
                    mChildIndexDuaRegistering = childIndex;
                    break;
                }
            }
        }

        child  = Get<ChildTable>().GetChildAtIndex(mChildIndexDuaRegistering);
        duaPtr = child->GetDomainUnicastAddress();

        OT_ASSERT(duaPtr != nullptr);

        dua = *duaPtr;
        SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, &dua, sizeof(dua)));
        SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kMeshLocalEid, &child->GetMeshLocalIid(),
                                             sizeof(Ip6::InterfaceIdentifier)));

        lastTransactionTime = Time::MsecToSec(TimerMilli::GetNow() - child->GetLastHeard());
        SuccessOrExit(error = Tlv::AppendUint32Tlv(*message, ThreadTlv::kLastTransactionTime, lastTransactionTime));
#endif // OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
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

    messageInfo.SetPeerPort(Tmf::kUdpPort);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());

    SuccessOrExit(error =
                      Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo, &DuaManager::HandleDuaResponse, this));

    mIsDuaPending = true;

    // TODO: (DUA) need update when CSL is enabled.
    if (!Get<Mle::Mle>().IsRxOnWhenIdle())
    {
        Get<DataPollSender>().SendFastPolls();
    }

exit:
    if (error == OT_ERROR_NO_BUFS)
    {
        UpdateCheckDelay(Mle::kNoBufDelay);
    }

    FreeMessageOnError(message, error);
    otLogInfoDua("Sent DUA.req for DUA %s: %s", dua.ToString().AsCString(), otThreadErrorToString(error));
}

void DuaManager::HandleDuaResponse(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, otError aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    otError error;

    mIsDuaPending = false;

    if (aResult == OT_ERROR_RESPONSE_TIMEOUT)
    {
        UpdateCheckDelay(Mle::KResponseTimeoutDelay);
        ExitNow(error = aResult);
    }

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage.GetCode() == Coap::kCodeChanged, error = OT_ERROR_PARSE);

    error = ProcessDuaResponse(aMessage);

exit:
    if (error != OT_ERROR_RESPONSE_TIMEOUT)
    {
        mRegistrationTask.Post();
    }

    otLogInfoDua("Received DUA.req: %s", otThreadErrorToString(error));
}

void DuaManager::HandleDuaNotification(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    otError error;

    OT_UNUSED_VARIABLE(error);

    VerifyOrExit(aMessage.IsPostRequest(), error = OT_ERROR_PARSE);

    if (aMessage.IsConfirmable() && Get<Tmf::TmfAgent>().SendEmptyAck(aMessage, aMessageInfo) == OT_ERROR_NONE)
    {
        otLogInfoDua("Sent DUA.ntf acknowledgment");
    }

    error = ProcessDuaResponse(aMessage);

exit:
    otLogInfoDua("Received DUA.ntf: %d", otThreadErrorToString(error));
}

otError DuaManager::ProcessDuaResponse(Coap::Message &aMessage)
{
    otError      error = OT_ERROR_NONE;
    Ip6::Address target;
    uint8_t      status;

    SuccessOrExit(error = Tlv::FindUint8Tlv(aMessage, ThreadTlv::kStatus, status));
    SuccessOrExit(error = Tlv::FindTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));

#if OPENTHREAD_CONFIG_DUA_ENABLE
    if (Get<ThreadNetif>().HasUnicastAddress(target))
    {
        switch (static_cast<ThreadStatusTlv::DuaStatus>(status))
        {
        case ThreadStatusTlv::kDuaSuccess:
            mLastRegistrationTime = TimerMilli::GetNow();
            mDuaState             = kRegistered;
            break;
        case ThreadStatusTlv::kDuaReRegister:
            mDuaState                  = kToRegister;
            mDelay.mFields.mCheckDelay = Mle::kImmediateReRegisterDelay;
            break;
        case ThreadStatusTlv::kDuaInvalid:
            // Domain Prefix might be invalid.
            RemoveDomainUnicastAddress();
            break;
        case ThreadStatusTlv::kDuaDuplicate:
            RemoveDomainUnicastAddress();
            mDadCounter++;

            if (GenerateDomainUnicastAddressIid() == OT_ERROR_NONE)
            {
                AddDomainUnicastAddress();
            }

            break;
        case ThreadStatusTlv::kDuaNoResources:
        case ThreadStatusTlv::kDuaNotPrimary:
        case ThreadStatusTlv::kDuaGeneralFailure:
            UpdateReregistrationDelay();
            break;
        }
    }
    else
#endif
#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    {
        Child *child = Get<ChildTable>().GetChildAtIndex(mChildIndexDuaRegistering);

        VerifyOrExit(child != nullptr, error = OT_ERROR_NOT_FOUND);
        VerifyOrExit(child->HasIp6Address(target), error = OT_ERROR_NOT_FOUND);

        mRegisterCurrentChildIndex = false;

        switch (status)
        {
        case ThreadStatusTlv::kDuaSuccess:
            // Mark as Registered
            mChildDuaRegisteredMask.Set(mChildIndexDuaRegistering, true);
            break;
        case ThreadStatusTlv::kDuaReRegister:
            mRegisterCurrentChildIndex = true;
            mDelay.mFields.mCheckDelay = Mle::kImmediateReRegisterDelay;
            break;
        case ThreadStatusTlv::kDuaInvalid:
        case ThreadStatusTlv::kDuaDuplicate:
            SendAddressNotification(target, static_cast<ThreadStatusTlv::DuaStatus>(status), *child);
            IgnoreError(child->RemoveIp6Address(target));
            mChildDuaMask.Set(mChildIndexDuaRegistering, false);
            mChildDuaRegisteredMask.Set(mChildIndexDuaRegistering, false);
            break;
        case ThreadStatusTlv::kDuaNoResources:
        case ThreadStatusTlv::kDuaNotPrimary:
        case ThreadStatusTlv::kDuaGeneralFailure:
            UpdateReregistrationDelay();
            break;
        }
    }
#endif // OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

exit:
    UpdateTimeTickerRegistration();
    return error;
}

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
void DuaManager::SendAddressNotification(Ip6::Address &             aAddress,
                                         ThreadStatusTlv::DuaStatus aStatus,
                                         const Child &              aChild)
{
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;
    otError          error;

    VerifyOrExit((message = Get<Tmf::TmfAgent>().NewPriorityMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsConfirmablePost(UriPath::kDuaRegistrationNotify));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint8Tlv(*message, ThreadTlv::kStatus, static_cast<uint8_t>(aStatus)));
    SuccessOrExit(error = Tlv::AppendTlv(*message, ThreadTlv::kTarget, &aAddress, sizeof(aAddress)));

    messageInfo.GetPeerAddr().SetToRoutingLocator(Get<Mle::MleRouter>().GetMeshLocalPrefix(), aChild.GetRloc16());
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoDua("Sent ADDR_NTF for child %04x DUA %s", aChild.GetRloc16(), aAddress.ToString().AsCString());

exit:

    if (error != OT_ERROR_NONE)
    {
        FreeMessage(message);

        // TODO: (DUA) (P4) may enhance to  guarantee the delivery of DUA.ntf
        otLogWarnDua("Sent ADDR_NTF for child %04x DUA %s Error %s", aChild.GetRloc16(),
                     aAddress.ToString().AsCString(), otThreadErrorToString(error));
    }
}

void DuaManager::UpdateChildDomainUnicastAddress(const Child &aChild, Mle::ChildDuaState aState)
{
    uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

    if ((aState == Mle::ChildDuaState::kRemoved || aState == Mle::ChildDuaState::kChanged) &&
        mChildDuaMask.Get(childIndex))
    {
        // Abort on going proxy DUA.req for this child
#if OPENTHREAD_CONFIG_DUA_ENABLE
        if (mIsDuaPending && mDuaState != DuaState::kRegistering && mChildIndexDuaRegistering == childIndex)
#else
        if (mIsDuaPending && mChildIndexDuaRegistering == childIndex)
#endif
        {
            IgnoreError(Get<Tmf::TmfAgent>().AbortTransaction(&DuaManager::HandleDuaResponse, this));

            // Reset mRegisterCurrentChildIndex properly
            mRegisterCurrentChildIndex = mRegisterCurrentChildIndex && (aState == Mle::ChildDuaState::kRemoved);
        }

        mChildDuaMask.Set(childIndex, false);
        mChildDuaRegisteredMask.Set(childIndex, false);
    }

    if (aState == Mle::ChildDuaState::kAdded || aState == Mle::ChildDuaState::kChanged)
    {
        if (mChildDuaMask == mChildDuaRegisteredMask)
        {
            UpdateCheckDelay(Random::NonCrypto::GetUint8InRange(1, Mle::kParentAggregateDelay));
        }

        mChildDuaMask.Set(childIndex, true);
        mChildDuaRegisteredMask.Set(childIndex, false);
    }

    return;
}
#endif // OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

} // namespace ot

#endif // OPENTHREAD_CONFIG_DUA_ENABLE || OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
