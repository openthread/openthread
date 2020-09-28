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
 *   This file implements Backbone Router management.
 */

#include "bbr_manager.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "thread/mle_types.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

namespace BackboneRouter {

Manager::Manager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMulticastListenerRegistration(UriPath::kMlr, Manager::HandleMulticastListenerRegistration, this)
    , mDuaRegistration(UriPath::kDuaRegistrationRequest, Manager::HandleDuaRegistration, this)
    , mNdProxyTable(aInstance)
    , mMulticastListenersTable(aInstance)
    , mTimer(aInstance, Manager::HandleTimer, this)
    , mBackboneTmfAgent(aInstance)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mDuaResponseStatus(ThreadStatusTlv::kDuaSuccess)
    , mMlrResponseStatus(ThreadStatusTlv::kMlrSuccess)
    , mDuaResponseIsSpecified(false)
    , mMlrResponseIsSpecified(false)
#endif
{
}

void Manager::HandleNotifierEvents(Events aEvents)
{
    otError error;

    if (aEvents.Contains(kEventThreadBackboneRouterStateChanged))
    {
        if (Get<BackboneRouter::Local>().GetState() == OT_BACKBONE_ROUTER_STATE_DISABLED)
        {
            Get<Tmf::TmfAgent>().RemoveResource(mMulticastListenerRegistration);
            Get<Tmf::TmfAgent>().RemoveResource(mDuaRegistration);
            mTimer.Stop();
            mMulticastListenersTable.Clear();

            error = mBackboneTmfAgent.Stop();

            if (error != OT_ERROR_NONE)
            {
                otLogWarnBbr("Stop Backbone TMF agent: %s", otThreadErrorToString(error));
            }
            else
            {
                otLogInfoBbr("Stop Backbone TMF agent: %s", otThreadErrorToString(error));
            }
        }
        else
        {
            Get<Tmf::TmfAgent>().AddResource(mMulticastListenerRegistration);
            Get<Tmf::TmfAgent>().AddResource(mDuaRegistration);
            if (!mTimer.IsRunning())
            {
                mTimer.Start(kTimerInterval);
            }

            error = mBackboneTmfAgent.Start();

            if (error != OT_ERROR_NONE)
            {
                otLogCritBbr("Start Backbone TMF agent: %s", otThreadErrorToString(error));
            }
            else
            {
                otLogInfoBbr("Start Backbone TMF agent: %s", otThreadErrorToString(error));
            }
        }
    }
}

void Manager::HandleTimer(void)
{
    mMulticastListenersTable.Expire();

    mTimer.Start(kTimerInterval);
}

void Manager::HandleMulticastListenerRegistration(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                    error     = OT_ERROR_NONE;
    bool                       isPrimary = Get<BackboneRouter::Local>().IsPrimary();
    ThreadStatusTlv::MlrStatus status    = ThreadStatusTlv::kMlrSuccess;
    BackboneRouterConfig       config;

    uint16_t     addressesOffset, addressesLength;
    Ip6::Address address;
    Ip6::Address failedAddresses[kIPv6AddressesNumMax];
    uint8_t      failedAddressNum = 0;
    TimeMilli    expireTime;

    uint32_t timeout;
    uint16_t commissionerSessionId;
    bool     hasCommissionerSessionIdTlv = false;
    bool     processTimeoutTlv           = false;

    VerifyOrExit(aMessage.IsConfirmablePostRequest(), error = OT_ERROR_PARSE);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    // Required by Test Specification 5.10.22 DUA-TC-26, only for certification purpose
    if (mMlrResponseIsSpecified)
    {
        mMlrResponseIsSpecified = false;
        ExitNow(status = mMlrResponseStatus);
    }
#endif

    VerifyOrExit(isPrimary, status = ThreadStatusTlv::kMlrBbrNotPrimary);

    // TODO: (MLR) send configured MLR response for Reference Device

    if (ThreadTlv::FindUint16Tlv(aMessage, ThreadTlv::kCommissionerSessionId, commissionerSessionId) == OT_ERROR_NONE)
    {
        const MeshCoP::CommissionerSessionIdTlv *commissionerSessionIdTlv =
            static_cast<const MeshCoP::CommissionerSessionIdTlv *>(
                Get<NetworkData::Leader>().GetCommissioningDataSubTlv(MeshCoP::Tlv::kCommissionerSessionId));

        VerifyOrExit(commissionerSessionIdTlv != nullptr &&
                         commissionerSessionIdTlv->GetCommissionerSessionId() == commissionerSessionId,
                     status = ThreadStatusTlv::kMlrGeneralFailure);

        hasCommissionerSessionIdTlv = true;
    }

    processTimeoutTlv = hasCommissionerSessionIdTlv &&
                        (ThreadTlv::FindUint32Tlv(aMessage, ThreadTlv::kTimeout, timeout) == OT_ERROR_NONE);

    VerifyOrExit(ThreadTlv::FindTlvValueOffset(aMessage, IPv6AddressesTlv::kIPv6Addresses, addressesOffset,
                                               addressesLength) == OT_ERROR_NONE,
                 error = OT_ERROR_PARSE);
    VerifyOrExit(addressesLength % sizeof(Ip6::Address) == 0, status = ThreadStatusTlv::kMlrGeneralFailure);
    VerifyOrExit(addressesLength / sizeof(Ip6::Address) <= kIPv6AddressesNumMax,
                 status = ThreadStatusTlv::kMlrGeneralFailure);

    if (!processTimeoutTlv)
    {
        IgnoreError(Get<BackboneRouter::Leader>().GetConfig(config));

        timeout = config.mMlrTimeout;
    }
    else
    {
        VerifyOrExit(timeout < UINT32_MAX, status = ThreadStatusTlv::kMlrNoPersistent);

        if (timeout != 0)
        {
            uint32_t origTimeout = timeout;

            timeout = OT_MIN(timeout, static_cast<uint32_t>(Mle::kMlrTimeoutMax));
            timeout = OT_MAX(timeout, static_cast<uint32_t>(Mle::kMlrTimeoutMin));

            if (timeout != origTimeout)
            {
                otLogNoteBbr("MLR.req: MLR timeout is normalized from %u to %u", origTimeout, timeout);
            }
        }
    }

    expireTime = TimerMilli::GetNow() + TimeMilli::SecToMsec(timeout);

    for (uint16_t offset = 0; offset < addressesLength; offset += sizeof(Ip6::Address))
    {
        IgnoreReturnValue(aMessage.Read(addressesOffset + offset, sizeof(Ip6::Address), &address));

        if (timeout == 0)
        {
            mMulticastListenersTable.Remove(address);
        }
        else
        {
            bool failed = true;

            switch (mMulticastListenersTable.Add(address, expireTime))
            {
            case OT_ERROR_NONE:
                failed = false;
                break;
            case OT_ERROR_INVALID_ARGS:
                if (status == ThreadStatusTlv::kMlrSuccess)
                {
                    status = ThreadStatusTlv::kMlrInvalid;
                }
                break;
            case OT_ERROR_NO_BUFS:
                if (status == ThreadStatusTlv::kMlrSuccess)
                {
                    status = ThreadStatusTlv::kMlrNoResources;
                }
                break;
            default:
                OT_ASSERT(false);
            }

            if (failed)
            {
                failedAddresses[failedAddressNum++] = address;
            }
        }
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        SendMulticastListenerRegistrationResponse(aMessage, aMessageInfo, status, failedAddresses, failedAddressNum);
        // TODO: (MLR) send BMLR.req
    }
}

void Manager::SendMulticastListenerRegistrationResponse(const Coap::Message &      aMessage,
                                                        const Ip6::MessageInfo &   aMessageInfo,
                                                        ThreadStatusTlv::MlrStatus aStatus,
                                                        Ip6::Address *             aFailedAddresses,
                                                        uint8_t                    aFailedAddressNum)
{
    otError        error   = OT_ERROR_NONE;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = Get<Tmf::TmfAgent>().NewMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(message->SetDefaultResponseHeader(aMessage));
    SuccessOrExit(message->SetPayloadMarker());

    SuccessOrExit(Tlv::AppendUint8Tlv(*message, ThreadTlv::kStatus, aStatus));

    if (aFailedAddressNum > 0)
    {
        IPv6AddressesTlv addressesTlv;

        addressesTlv.Init();
        addressesTlv.SetLength(sizeof(Ip6::Address) * aFailedAddressNum);
        SuccessOrExit(error = message->Append(&addressesTlv, sizeof(addressesTlv)));

        for (uint8_t i = 0; i < aFailedAddressNum; i++)
        {
            SuccessOrExit(error = message->Append(aFailedAddresses + i, sizeof(Ip6::Address)));
        }
    }

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, aMessageInfo));

exit:
    FreeMessageOnError(message, error);
    otLogInfoBbr("Sent MLR.rsp (status=%d): %s", aStatus, otThreadErrorToString(error));
}

void Manager::HandleDuaRegistration(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                    error     = OT_ERROR_NONE;
    ThreadStatusTlv::DuaStatus status    = ThreadStatusTlv::kDuaSuccess;
    bool                       isPrimary = Get<BackboneRouter::Local>().IsPrimary();
    uint32_t                   lastTransactionTime;
    bool                       hasLastTransactionTime;
    Ip6::Address               target;
    Ip6::InterfaceIdentifier   meshLocalIid;

    VerifyOrExit(aMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator(), error = OT_ERROR_DROP);
    VerifyOrExit(aMessage.IsConfirmablePostRequest(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::FindTlv(aMessage, ThreadTlv::kTarget, &target, sizeof(target)));
    SuccessOrExit(error = Tlv::FindTlv(aMessage, ThreadTlv::kMeshLocalEid, &meshLocalIid, sizeof(meshLocalIid)));

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (mDuaResponseIsSpecified && (mDuaResponseTargetMlIid.IsUnspecified() || mDuaResponseTargetMlIid == meshLocalIid))
    {
        mDuaResponseIsSpecified = false;
        ExitNow(status = mDuaResponseStatus);
    }
#endif

    VerifyOrExit(isPrimary, status = ThreadStatusTlv::kDuaNotPrimary);
    VerifyOrExit(Get<BackboneRouter::Leader>().IsDomainUnicast(target), status = ThreadStatusTlv::kDuaInvalid);

    hasLastTransactionTime =
        (Tlv::FindUint32Tlv(aMessage, ThreadTlv::kLastTransactionTime, lastTransactionTime) == OT_ERROR_NONE);

    switch (mNdProxyTable.Register(target.GetIid(), meshLocalIid, aMessageInfo.GetPeerAddr().GetIid().GetLocator(),
                                   hasLastTransactionTime ? &lastTransactionTime : nullptr))
    {
    case OT_ERROR_NONE:
        // TODO: update its EID-to-RLOC Map Cache based on the pair {DUA, RLOC16-source} which is gleaned from the
        // DUA.req packet according to Thread Spec. 5.23.3.6.2
        break;
    case OT_ERROR_DUPLICATED:
        status = ThreadStatusTlv::kDuaDuplicate;
        break;
    case OT_ERROR_NO_BUFS:
        status = ThreadStatusTlv::kDuaNoResources;
        break;
    default:
        status = ThreadStatusTlv::kDuaGeneralFailure;
        break;
    }

    // TODO: (DUA) Add DAD process
    // TODO: (DUA) Extended Address Query

exit:
    otLogInfoBbr("Received DUA.req on %s: %s", (isPrimary ? "PBBR" : "SBBR"), otThreadErrorToString(error));

    if (error == OT_ERROR_NONE)
    {
        SendDuaRegistrationResponse(aMessage, aMessageInfo, target, status);
    }
}

void Manager::SendDuaRegistrationResponse(const Coap::Message &      aMessage,
                                          const Ip6::MessageInfo &   aMessageInfo,
                                          const Ip6::Address &       aTarget,
                                          ThreadStatusTlv::DuaStatus aStatus)
{
    otError        error   = OT_ERROR_NONE;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = Get<Tmf::TmfAgent>().NewMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(message->SetDefaultResponseHeader(aMessage));
    SuccessOrExit(message->SetPayloadMarker());

    SuccessOrExit(Tlv::AppendUint8Tlv(*message, ThreadTlv::kStatus, aStatus));
    SuccessOrExit(Tlv::AppendTlv(*message, ThreadTlv::kTarget, &aTarget, sizeof(aTarget)));

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, aMessageInfo));

exit:
    FreeMessageOnError(message, error);
    otLogInfoBbr("Sent DUA.rsp for DUA %s, status %d %s", aTarget.ToString().AsCString(), aStatus,
                 otThreadErrorToString(error));
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void Manager::ConfigNextDuaRegistrationResponse(const Ip6::InterfaceIdentifier *aMlIid, uint8_t aStatus)
{
    mDuaResponseIsSpecified = true;

    if (aMlIid)
    {
        mDuaResponseTargetMlIid = *aMlIid;
    }
    else
    {
        mDuaResponseTargetMlIid.Clear();
    }

    mDuaResponseStatus = static_cast<ThreadStatusTlv::DuaStatus>(aStatus);
}

void Manager::ConfigNextMulticastListenerRegistrationResponse(ThreadStatusTlv::MlrStatus aStatus)
{
    mMlrResponseIsSpecified = true;
    mMlrResponseStatus      = aStatus;
}
#endif

NdProxyTable &Manager::GetNdProxyTable(void)
{
    return mNdProxyTable;
}

bool Manager::ShouldForwardDuaToBackbone(const Ip6::Address &aAddress)
{
    bool              forwardToBackbone = false;
    Mac::ShortAddress rloc16;
    otError           error;

    VerifyOrExit(Get<Local>().IsPrimary(), OT_NOOP);
    VerifyOrExit(Get<Leader>().IsDomainUnicast(aAddress), OT_NOOP);

    VerifyOrExit(!mNdProxyTable.IsRegistered(aAddress.GetIid()), OT_NOOP);

    error = Get<AddressResolver>().Resolve(aAddress, rloc16, /* aAllowAddressQuery */ false);
    VerifyOrExit(error != OT_ERROR_NONE || rloc16 == Get<Mle::MleRouter>().GetRloc16(), OT_NOOP);

    // TODO: check if the DUA is an address of any Child?
    forwardToBackbone = true;

exit:
    return forwardToBackbone;
}

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
