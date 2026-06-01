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

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("DuaManager");

DuaManager::DuaManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRegistrationTask(aInstance)
    , mIsDuaPending(false)
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    , mChildIndexDuaRegistering(Mle::kMaxChildren)
#endif
{
    mDelay.mValue = 0;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    mChildDuaMask.Clear();
    mChildDuaRegisteredMask.Clear();
#endif
}

void DuaManager::HandleDomainPrefixUpdate(BackboneRouter::DomainPrefixEvent aEvent)
{
    if ((aEvent == BackboneRouter::kDomainPrefixRemoved) || (aEvent == BackboneRouter::kDomainPrefixRefreshed))
    {
        if (mIsDuaPending)
        {
            IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleDuaResponse, this));
        }

        if (!mChildDuaMask.IsEmpty())
        {
            mChildDuaMask.Clear();
            mChildDuaRegisteredMask.Clear();
        }
    }
}

void DuaManager::UpdateReregistrationDelay(void)
{
    uint16_t delay;

    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary());

    delay = Get<BackboneRouter::Leader>().GetConfig().SelectRandomReregistrationDelay();

    if (mDelay.mFields.mReregistrationDelay == 0 || mDelay.mFields.mReregistrationDelay > delay)
    {
        mDelay.mFields.mReregistrationDelay = delay;
        UpdateTimeTickerRegistration();
        LogDebg("update reregdelay %d", mDelay.mFields.mReregistrationDelay);
    }

exit:
    return;
}

void DuaManager::UpdateCheckDelay(uint8_t aDelay)
{
    if (mDelay.mFields.mCheckDelay == 0 || mDelay.mFields.mCheckDelay > aDelay)
    {
        mDelay.mFields.mCheckDelay = aDelay;

        LogDebg("update checkdelay %d", mDelay.mFields.mCheckDelay);
        UpdateTimeTickerRegistration();
    }
}

void DuaManager::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(Get<Mle::Mle>().IsAttached(), mDelay.mValue = 0);

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        if (Get<Mle::Mle>().HasRestored())
        {
            UpdateReregistrationDelay();
        }
    }

exit:
    return;
}

void DuaManager::HandleBackboneRouterPrimaryUpdate(BackboneRouter::PrimaryEvent aEvent)
{
    if (aEvent == BackboneRouter::kPrimaryAdded || aEvent == BackboneRouter::kPrimaryUpdatedReregister)
    {
        UpdateReregistrationDelay();
    }
}

void DuaManager::HandleTimeTick(void)
{
    bool attempt = false;

    LogDebg("reregdelay %d, checkdelay %d", mDelay.mFields.mReregistrationDelay, mDelay.mFields.mCheckDelay);

    if ((mDelay.mFields.mCheckDelay > 0) && (--mDelay.mFields.mCheckDelay == 0))
    {
        attempt = true;
    }

    if ((mDelay.mFields.mReregistrationDelay > 0) && (--mDelay.mFields.mReregistrationDelay == 0))
    {
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
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
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;
    Ip6::Address   dua;
    Ip6::Address   destAddr;

    VerifyOrExit(Get<Mle::Mle>().IsAttached());
    VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary());

    // Only allow one outgoing DUA.req
    VerifyOrExit(!mIsDuaPending);

    VerifyOrExit(!mChildDuaMask.IsEmpty() && mChildDuaMask != mChildDuaRegisteredMask);

    // Prepare DUA.req
    message = Get<Tmf::Agent>().AllocateAndInitPriorityConfirmablePostMessage(kUriDuaRegistrationRequest);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    {
        uint32_t lastTransactionTime;
        Child   *child = nullptr;

        OT_ASSERT(mChildIndexDuaRegistering == Mle::kMaxChildren);

        for (Child &iter : Get<ChildTable>().Iterate(Child::kInStateValid))
        {
            uint16_t childIndex = Get<ChildTable>().GetChildIndex(iter);

            if (mChildDuaMask.Has(childIndex) && !mChildDuaRegisteredMask.Has(childIndex))
            {
                mChildIndexDuaRegistering = childIndex;
                break;
            }
        }

        child = Get<ChildTable>().GetChildAtIndex(mChildIndexDuaRegistering);
        SuccessOrAssert(child->GetDomainUnicastAddress(dua));

        SuccessOrExit(error = Tlv::Append<ThreadTargetTlv>(*message, dua));
        SuccessOrExit(error = Tlv::Append<ThreadMeshLocalEidTlv>(*message, child->GetMeshLocalIid()));

        lastTransactionTime = Time::MsecToSec(TimerMilli::GetNow() - child->GetLastHeard());
        SuccessOrExit(error = Tlv::Append<ThreadLastTransactionTimeTlv>(*message, lastTransactionTime));
    }

    destAddr.InitAsRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), Get<BackboneRouter::Leader>().GetServer16());

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageTo(*message, destAddr, HandleDuaResponse, this));

    mIsDuaPending   = true;
    mRegisteringDua = dua;
    mDelay.mValue   = 0;

    LogInfo("Sent %s for DUA %s", UriToString<kUriDuaRegistrationRequest>(), dua.ToString().AsCString());

exit:
    if (error == kErrorNoBufs)
    {
        UpdateCheckDelay(kNoBufDelay);
    }

    LogWarnOnError(error, "perform next registration");
    FreeMessageOnError(message, error);
}

void DuaManager::HandleDuaResponse(Coap::Msg *aMsg, Error aResult)
{
    Error error;

    mIsDuaPending = false;
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    mChildIndexDuaRegistering = Mle::kMaxChildren;
#endif

    if (aResult == kErrorResponseTimeout)
    {
        UpdateCheckDelay(KResponseTimeoutDelay);
        ExitNow(error = aResult);
    }

    VerifyOrExit(aResult == kErrorNone, error = kErrorParse);
    OT_ASSERT(aMsg != nullptr);

    VerifyOrExit(aMsg->GetCode() == Coap::kCodeChanged || aMsg->GetCode() >= Coap::kCodeBadRequest,
                 error = kErrorParse);

    error = ProcessDuaResponse(aMsg->mMessage);

exit:
    if (error != kErrorResponseTimeout)
    {
        mRegistrationTask.Post();
    }

    LogInfo("Received %s response: %s", UriToString<kUriDuaRegistrationRequest>(), ErrorToString(error));
}

template <> void DuaManager::HandleTmf<kUriDuaRegistrationNotify>(Coap::Msg &aMsg)
{
    Error error;

    if (aMsg.IsConfirmable() && Get<Tmf::Agent>().SendAckResponse(aMsg) == kErrorNone)
    {
        LogInfo("Sent %s ack", UriToString<kUriDuaRegistrationNotify>());
    }

    error = ProcessDuaResponse(aMsg.mMessage);

    OT_UNUSED_VARIABLE(error);
    LogInfo("Received %s: %s", UriToString<kUriDuaRegistrationNotify>(), ErrorToString(error));
}

Error DuaManager::ProcessDuaResponse(Coap::Message &aMessage)
{
    Error        error = kErrorNone;
    Ip6::Address target;
    uint8_t      status;

    if (aMessage.ReadCode() >= Coap::kCodeBadRequest)
    {
        status = kDuaGeneralFailure;
        target = mRegisteringDua;
    }
    else
    {
        SuccessOrExit(error = Tlv::Find<ThreadStatusTlv>(aMessage, status));
        SuccessOrExit(error = Tlv::Find<ThreadTargetTlv>(aMessage, target));
    }

    VerifyOrExit(Get<BackboneRouter::Leader>().IsDomainUnicast(target), error = kErrorDrop);

    {
        Child   *child = nullptr;
        uint16_t childIndex;

        for (Child &iter : Get<ChildTable>().Iterate(Child::kInStateValid))
        {
            if (iter.HasIp6Address(target))
            {
                child = &iter;
                break;
            }
        }

        VerifyOrExit(child != nullptr, error = kErrorNotFound);

        childIndex = Get<ChildTable>().GetChildIndex(*child);

        switch (status)
        {
        case kDuaSuccess:
            // Mark as Registered
            if (mChildDuaMask.Has(childIndex))
            {
                mChildDuaRegisteredMask.Add(childIndex);
            }
            break;
        case kDuaReRegister:
            // Parent stops registering for the Child's DUA until next Child Update Request
            mChildDuaMask.Remove(childIndex);
            mChildDuaRegisteredMask.Remove(childIndex);
            break;
        case kDuaInvalid:
        case kDuaDuplicate:
            IgnoreError(child->RemoveIp6Address(target));
            mChildDuaMask.Remove(childIndex);
            mChildDuaRegisteredMask.Remove(childIndex);
            break;
        case kDuaNoResources:
        case kDuaNotPrimary:
        case kDuaGeneralFailure:
            UpdateReregistrationDelay();
            break;
        }

        if (status != kDuaSuccess)
        {
            SendAddressNotification(target, static_cast<DuaStatus>(status), *child);
        }
    }

exit:
    UpdateTimeTickerRegistration();
    return error;
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
void DuaManager::SendAddressNotification(Ip6::Address &aAddress, DuaStatus aStatus, const Child &aChild)
{
    Coap::Message *message = nullptr;
    Error          error;

    message = Get<Tmf::Agent>().AllocateAndInitPriorityConfirmablePostMessage(kUriDuaRegistrationNotify);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<ThreadStatusTlv>(*message, aStatus));
    SuccessOrExit(error = Tlv::Append<ThreadTargetTlv>(*message, aAddress));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessageToRloc(*message, aChild.GetRloc16()));

    LogInfo("Sent %s for child %04x DUA %s", UriToString<kUriDuaRegistrationNotify>(), aChild.GetRloc16(),
            aAddress.ToString().AsCString());

exit:

    if (error != kErrorNone)
    {
        FreeMessage(message);

        // TODO: (DUA) (P4) may enhance to  guarantee the delivery of DUA.ntf
        LogWarn("Sent %s for child %04x DUA %s Error %s", UriToString<kUriDuaRegistrationNotify>(), aChild.GetRloc16(),
                aAddress.ToString().AsCString(), ErrorToString(error));
    }
}

void DuaManager::HandleChildDuaAddressEvent(const Child &aChild, ChildDuaAddressEvent aEvent)
{
    uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

    if ((aEvent == kAddressRemoved || aEvent == kAddressChanged) && mChildDuaMask.Has(childIndex))
    {
        // Abort on going proxy DUA.req for this child
        if (mChildIndexDuaRegistering == childIndex)
        {
            IgnoreError(Get<Tmf::Agent>().AbortTransaction(HandleDuaResponse, this));
        }

        mChildDuaMask.Remove(childIndex);
        mChildDuaRegisteredMask.Remove(childIndex);
    }

    if (aEvent == kAddressAdded || aEvent == kAddressChanged ||
        (aEvent == kAddressUnchanged && !mChildDuaMask.Has(childIndex)))
    {
        if (mChildDuaMask == mChildDuaRegisteredMask)
        {
            UpdateCheckDelay(
                Random::NonCrypto::GenerateInClosedRange<uint8_t>(1, BackboneRouter::kParentAggregateDelay));
        }

        mChildDuaMask.Add(childIndex);
        mChildDuaRegisteredMask.Remove(childIndex);
    }
}
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
