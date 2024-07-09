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

#include "instance/instance.hpp"

namespace ot {

namespace BackboneRouter {

RegisterLogModule("BbrManager");

Manager::Manager(Instance &aInstance)
    : InstanceLocator(aInstance)
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    , mNdProxyTable(aInstance)
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    , mMulticastListenersTable(aInstance)
#endif
    , mTimer(aInstance)
    , mBackboneTmfAgent(aInstance)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    , mDuaResponseStatus(ThreadStatusTlv::kDuaSuccess)
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    , mMlrResponseStatus(ThreadStatusTlv::kMlrSuccess)
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    , mDuaResponseIsSpecified(false)
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    , mMlrResponseIsSpecified(false)
#endif
#endif
{
}

void Manager::HandleNotifierEvents(Events aEvents)
{
    Error error;

    if (aEvents.Contains(kEventThreadBackboneRouterStateChanged))
    {
        if (!Get<Local>().IsEnabled())
        {
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
            mMulticastListenersTable.Clear();
#endif
            mTimer.Stop();

            error = mBackboneTmfAgent.Stop();

            if (error != kErrorNone)
            {
                LogWarn("Stop Backbone TMF agent: %s", ErrorToString(error));
            }
            else
            {
                LogInfo("Stop Backbone TMF agent: %s", ErrorToString(error));
            }
        }
        else
        {
            if (!mTimer.IsRunning())
            {
                mTimer.Start(kTimerInterval);
            }

            error = mBackboneTmfAgent.Start();

            LogError("Start Backbone TMF agent", error);
        }
    }
}

void Manager::HandleTimer(void)
{
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
    mMulticastListenersTable.Expire();
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
    mNdProxyTable.HandleTimer();
#endif

    mTimer.Start(kTimerInterval);
}

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
template <> void Manager::HandleTmf<kUriMlr>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(Get<Local>().IsEnabled());
    HandleMulticastListenerRegistration(aMessage, aMessageInfo);

exit:
    return;
}

void Manager::HandleMulticastListenerRegistration(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                      error     = kErrorNone;
    bool                       isPrimary = Get<Local>().IsPrimary();
    ThreadStatusTlv::MlrStatus status    = ThreadStatusTlv::kMlrSuccess;
    Config                     config;

    OffsetRange  offsetRange;
    Ip6::Address address;
    Ip6::Address addresses[Ip6AddressesTlv::kMaxAddresses];
    uint8_t      failedAddressNum  = 0;
    uint8_t      successAddressNum = 0;
    TimeMilli    expireTime;
    uint32_t     timeout;
    uint16_t     commissionerSessionId;
    bool         hasCommissionerSessionIdTlv = false;
    bool         processTimeoutTlv           = false;

    VerifyOrExit(aMessage.IsConfirmablePostRequest(), error = kErrorParse);

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

    if (Tlv::Find<ThreadCommissionerSessionIdTlv>(aMessage, commissionerSessionId) == kErrorNone)
    {
        uint16_t localSessionId;

        VerifyOrExit((Get<NetworkData::Leader>().FindCommissioningSessionId(localSessionId) == kErrorNone) &&
                         (localSessionId == commissionerSessionId),
                     status = ThreadStatusTlv::kMlrGeneralFailure);

        hasCommissionerSessionIdTlv = true;
    }

    processTimeoutTlv = hasCommissionerSessionIdTlv && (Tlv::Find<ThreadTimeoutTlv>(aMessage, timeout) == kErrorNone);

    VerifyOrExit(Tlv::FindTlvValueOffsetRange(aMessage, Ip6AddressesTlv::kIp6Addresses, offsetRange) == kErrorNone,
                 error = kErrorParse);
    VerifyOrExit(offsetRange.GetLength() % sizeof(Ip6::Address) == 0, status = ThreadStatusTlv::kMlrGeneralFailure);
    VerifyOrExit(offsetRange.GetLength() / sizeof(Ip6::Address) <= Ip6AddressesTlv::kMaxAddresses,
                 status = ThreadStatusTlv::kMlrGeneralFailure);

    if (!processTimeoutTlv)
    {
        IgnoreError(Get<Leader>().GetConfig(config));

        timeout = config.mMlrTimeout;
    }
    else
    {
        VerifyOrExit(timeout < NumericLimits<uint32_t>::kMax, status = ThreadStatusTlv::kMlrNoPersistent);

        if (timeout != 0)
        {
            uint32_t origTimeout = timeout;

            timeout = Min(timeout, kMaxMlrTimeout);

            if (timeout != origTimeout)
            {
                LogNote("MLR.req: MLR timeout is normalized from %lu to %lu", ToUlong(origTimeout), ToUlong(timeout));
            }
        }
    }

    expireTime = TimerMilli::GetNow() + TimeMilli::SecToMsec(timeout);

    while (!offsetRange.IsEmpty())
    {
        IgnoreError(aMessage.Read(offsetRange, address));
        offsetRange.AdvanceOffset(sizeof(Ip6::Address));

        if (timeout == 0)
        {
            mMulticastListenersTable.Remove(address);

            // Put successfully de-registered addresses at the end of `addresses`.
            addresses[Ip6AddressesTlv::kMaxAddresses - (++successAddressNum)] = address;
        }
        else
        {
            bool failed = true;

            switch (mMulticastListenersTable.Add(address, expireTime))
            {
            case kErrorNone:
                failed = false;
                break;
            case kErrorInvalidArgs:
                if (status == ThreadStatusTlv::kMlrSuccess)
                {
                    status = ThreadStatusTlv::kMlrInvalid;
                }
                break;
            case kErrorNoBufs:
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
                addresses[failedAddressNum++] = address;
            }
            else
            {
                // Put successfully registered addresses at the end of `addresses`.
                addresses[Ip6AddressesTlv::kMaxAddresses - (++successAddressNum)] = address;
            }
        }
    }

exit:
    if (error == kErrorNone)
    {
        SendMulticastListenerRegistrationResponse(aMessage, aMessageInfo, status, addresses, failedAddressNum);
    }

    if (successAddressNum > 0)
    {
        SendBackboneMulticastListenerRegistration(&addresses[Ip6AddressesTlv::kMaxAddresses - successAddressNum],
                                                  successAddressNum, timeout);
    }
}

void Manager::SendMulticastListenerRegistrationResponse(const Coap::Message       &aMessage,
                                                        const Ip6::MessageInfo    &aMessageInfo,
                                                        ThreadStatusTlv::MlrStatus aStatus,
                                                        Ip6::Address              *aFailedAddresses,
                                                        uint8_t                    aFailedAddressNum)
{
    Error          error = kErrorNone;
    Coap::Message *message;

    message = Get<Tmf::Agent>().NewResponseMessage(aMessage);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(Tlv::Append<ThreadStatusTlv>(*message, aStatus));

    if (aFailedAddressNum > 0)
    {
        Ip6AddressesTlv addressesTlv;

        addressesTlv.Init();
        addressesTlv.SetLength(sizeof(Ip6::Address) * aFailedAddressNum);
        SuccessOrExit(error = message->Append(addressesTlv));

        for (uint8_t i = 0; i < aFailedAddressNum; i++)
        {
            SuccessOrExit(error = message->Append(aFailedAddresses[i]));
        }
    }

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));

exit:
    FreeMessageOnError(message, error);
    LogInfo("Sent MLR.rsp (status=%d): %s", aStatus, ErrorToString(error));
}

void Manager::SendBackboneMulticastListenerRegistration(const Ip6::Address *aAddresses,
                                                        uint8_t             aAddressNum,
                                                        uint32_t            aTimeout)
{
    Error             error   = kErrorNone;
    Coap::Message    *message = nullptr;
    Ip6::MessageInfo  messageInfo;
    Ip6AddressesTlv   addressesTlv;
    BackboneTmfAgent &backboneTmf = Get<BackboneRouter::BackboneTmfAgent>();

    OT_ASSERT(aAddressNum >= Ip6AddressesTlv::kMinAddresses && aAddressNum <= Ip6AddressesTlv::kMaxAddresses);

    message = backboneTmf.NewNonConfirmablePostMessage(kUriBackboneMlr);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    addressesTlv.Init();
    addressesTlv.SetLength(sizeof(Ip6::Address) * aAddressNum);
    SuccessOrExit(error = message->Append(addressesTlv));
    SuccessOrExit(error = message->AppendBytes(aAddresses, sizeof(Ip6::Address) * aAddressNum));

    SuccessOrExit(error = Tlv::Append<ThreadTimeoutTlv>(*message, aTimeout));

    messageInfo.SetPeerAddr(Get<Local>().GetAllNetworkBackboneRoutersAddress());
    messageInfo.SetPeerPort(BackboneRouter::kBackboneUdpPort); // TODO: Provide API for configuring Backbone COAP port.

    messageInfo.SetHopLimit(kDefaultHoplimit);
    messageInfo.SetIsHostInterface(true);

    SuccessOrExit(error = backboneTmf.SendMessage(*message, messageInfo));

exit:
    FreeMessageOnError(message, error);
    LogInfo("Sent BMLR.ntf: %s", ErrorToString(error));
}
#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE

template <>
void Manager::HandleTmf<kUriDuaRegistrationRequest>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(Get<Local>().IsEnabled());
    HandleDuaRegistration(aMessage, aMessageInfo);

exit:
    return;
}

void Manager::HandleDuaRegistration(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                      error     = kErrorNone;
    ThreadStatusTlv::DuaStatus status    = ThreadStatusTlv::kDuaSuccess;
    bool                       isPrimary = Get<Local>().IsPrimary();
    uint32_t                   lastTransactionTime;
    bool                       hasLastTransactionTime;
    Ip6::Address               target;
    Ip6::InterfaceIdentifier   meshLocalIid;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    Coap::Code duaRespCoapCode = Coap::kCodeEmpty;
#endif

    VerifyOrExit(aMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator(), error = kErrorDrop);
    VerifyOrExit(aMessage.IsConfirmablePostRequest(), error = kErrorParse);

    SuccessOrExit(error = Tlv::Find<ThreadTargetTlv>(aMessage, target));
    SuccessOrExit(error = Tlv::Find<ThreadMeshLocalEidTlv>(aMessage, meshLocalIid));

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (mDuaResponseIsSpecified && (mDuaResponseTargetMlIid.IsUnspecified() || mDuaResponseTargetMlIid == meshLocalIid))
    {
        mDuaResponseIsSpecified = false;
        if (mDuaResponseStatus >= Coap::kCodeResponseMin)
        {
            duaRespCoapCode = static_cast<Coap::Code>(mDuaResponseStatus);
        }
        else
        {
            status = static_cast<ThreadStatusTlv::DuaStatus>(mDuaResponseStatus);
        }
        ExitNow();
    }
#endif

    VerifyOrExit(isPrimary, status = ThreadStatusTlv::kDuaNotPrimary);
    VerifyOrExit(Get<Leader>().HasDomainPrefix(), status = ThreadStatusTlv::kDuaGeneralFailure);
    VerifyOrExit(Get<Leader>().IsDomainUnicast(target), status = ThreadStatusTlv::kDuaInvalid);

    hasLastTransactionTime = (Tlv::Find<ThreadLastTransactionTimeTlv>(aMessage, lastTransactionTime) == kErrorNone);

    switch (mNdProxyTable.Register(target.GetIid(), meshLocalIid, aMessageInfo.GetPeerAddr().GetIid().GetLocator(),
                                   hasLastTransactionTime ? &lastTransactionTime : nullptr))
    {
    case kErrorNone:
        // TODO: update its EID-to-RLOC Map Cache based on the pair {DUA, RLOC16-source} which is gleaned from the
        // DUA.req packet according to Thread Spec. 5.23.3.6.2
        break;
    case kErrorDuplicated:
        status = ThreadStatusTlv::kDuaDuplicate;
        break;
    case kErrorNoBufs:
        status = ThreadStatusTlv::kDuaNoResources;
        break;
    default:
        status = ThreadStatusTlv::kDuaGeneralFailure;
        break;
    }

exit:
    LogInfo("Received DUA.req on %s: %s", (isPrimary ? "PBBR" : "SBBR"), ErrorToString(error));

    if (error == kErrorNone)
    {
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        if (duaRespCoapCode != Coap::kCodeEmpty)
        {
            IgnoreError(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo, duaRespCoapCode));
        }
        else
#endif
        {
            SendDuaRegistrationResponse(aMessage, aMessageInfo, target, status);
        }
    }
}

void Manager::SendDuaRegistrationResponse(const Coap::Message       &aMessage,
                                          const Ip6::MessageInfo    &aMessageInfo,
                                          const Ip6::Address        &aTarget,
                                          ThreadStatusTlv::DuaStatus aStatus)
{
    Error          error = kErrorNone;
    Coap::Message *message;

    message = Get<Tmf::Agent>().NewResponseMessage(aMessage);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(Tlv::Append<ThreadStatusTlv>(*message, aStatus));
    SuccessOrExit(Tlv::Append<ThreadTargetTlv>(*message, aTarget));

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));

exit:
    FreeMessageOnError(message, error);
    LogInfo("Sent DUA.rsp for DUA %s, status %d %s", aTarget.ToString().AsCString(), aStatus, ErrorToString(error));
}
#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
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

    mDuaResponseStatus = aStatus;
}
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
void Manager::ConfigNextMulticastListenerRegistrationResponse(ThreadStatusTlv::MlrStatus aStatus)
{
    mMlrResponseIsSpecified = true;
    mMlrResponseStatus      = aStatus;
}
#endif
#endif

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
NdProxyTable &Manager::GetNdProxyTable(void) { return mNdProxyTable; }

bool Manager::ShouldForwardDuaToBackbone(const Ip6::Address &aAddress)
{
    bool forwardToBackbone = false;

    VerifyOrExit(Get<Local>().IsPrimary());
    VerifyOrExit(Get<Leader>().IsDomainUnicast(aAddress));

    // Do not forward to Backbone if the DUA is registered on PBBR
    VerifyOrExit(!mNdProxyTable.IsRegistered(aAddress.GetIid()));
    // Do not forward to Backbone if the DUA belongs to a MTD Child (which may have failed in DUA registration)
    VerifyOrExit(Get<NeighborTable>().FindNeighbor(aAddress) == nullptr);
    // Forward to Backbone only if the DUA is resolved to the PBBR's RLOC16
    VerifyOrExit(Get<AddressResolver>().LookUp(aAddress) == Get<Mle::MleRouter>().GetRloc16());

    forwardToBackbone = true;

exit:
    return forwardToBackbone;
}

Error Manager::SendBackboneQuery(const Ip6::Address &aDua, uint16_t aRloc16)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Ip6::MessageInfo messageInfo;

    VerifyOrExit(Get<Local>().IsPrimary(), error = kErrorInvalidState);

    message = mBackboneTmfAgent.NewPriorityNonConfirmablePostMessage(kUriBackboneQuery);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<ThreadTargetTlv>(*message, aDua));

    if (aRloc16 != Mle::kInvalidRloc16)
    {
        SuccessOrExit(error = Tlv::Append<ThreadRloc16Tlv>(*message, aRloc16));
    }

    messageInfo.SetPeerAddr(Get<Local>().GetAllDomainBackboneRoutersAddress());
    messageInfo.SetPeerPort(BackboneRouter::kBackboneUdpPort);

    messageInfo.SetHopLimit(kDefaultHoplimit);
    messageInfo.SetIsHostInterface(true);

    error = mBackboneTmfAgent.SendMessage(*message, messageInfo);

exit:
    LogInfo("SendBackboneQuery for %s (rloc16=%04x): %s", aDua.ToString().AsCString(), aRloc16, ErrorToString(error));
    FreeMessageOnError(message, error);
    return error;
}

template <> void Manager::HandleTmf<kUriBackboneQuery>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                  error = kErrorNone;
    Ip6::Address           dua;
    uint16_t               rloc16 = Mle::kInvalidRloc16;
    NdProxyTable::NdProxy *ndProxy;

    VerifyOrExit(aMessageInfo.IsHostInterface(), error = kErrorDrop);

    VerifyOrExit(Get<Local>().IsPrimary(), error = kErrorInvalidState);
    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), error = kErrorParse);

    SuccessOrExit(error = Tlv::Find<ThreadTargetTlv>(aMessage, dua));

    error = Tlv::Find<ThreadRloc16Tlv>(aMessage, rloc16);
    VerifyOrExit(error == kErrorNone || error == kErrorNotFound);

    LogInfo("Received BB.qry from %s for %s (rloc16=%04x)", aMessageInfo.GetPeerAddr().ToString().AsCString(),
            dua.ToString().AsCString(), rloc16);

    ndProxy = mNdProxyTable.ResolveDua(dua);
    VerifyOrExit(ndProxy != nullptr && !ndProxy->GetDadFlag(), error = kErrorNotFound);

    error = SendBackboneAnswer(aMessageInfo, dua, rloc16, *ndProxy);

exit:
    LogInfo("HandleBackboneQuery: %s", ErrorToString(error));
}

template <> void Manager::HandleTmf<kUriBackboneAnswer>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error                    error = kErrorNone;
    bool                     proactive;
    Ip6::Address             dua;
    Ip6::InterfaceIdentifier meshLocalIid;
    OffsetRange              offsetRange;
    uint32_t                 timeSinceLastTransaction;
    uint16_t                 srcRloc16 = Mle::kInvalidRloc16;

    VerifyOrExit(aMessageInfo.IsHostInterface(), error = kErrorDrop);

    VerifyOrExit(Get<Local>().IsPrimary(), error = kErrorInvalidState);
    VerifyOrExit(aMessage.IsPostRequest(), error = kErrorParse);

    proactive = !aMessage.IsConfirmable();

    SuccessOrExit(error = Tlv::Find<ThreadTargetTlv>(aMessage, dua));
    SuccessOrExit(error = Tlv::Find<ThreadMeshLocalEidTlv>(aMessage, meshLocalIid));
    SuccessOrExit(error = Tlv::Find<ThreadLastTransactionTimeTlv>(aMessage, timeSinceLastTransaction));
    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, ThreadTlv::kNetworkName, offsetRange));

    error = Tlv::Find<ThreadRloc16Tlv>(aMessage, srcRloc16);
    VerifyOrExit(error == kErrorNone || error == kErrorNotFound);

    if (proactive)
    {
        HandleProactiveBackboneNotification(dua, meshLocalIid, timeSinceLastTransaction);
    }
    else if (srcRloc16 == Mle::kInvalidRloc16)
    {
        HandleDadBackboneAnswer(dua, meshLocalIid);
    }
    else
    {
        HandleExtendedBackboneAnswer(dua, meshLocalIid, timeSinceLastTransaction, srcRloc16);
    }

    SuccessOrExit(error = mBackboneTmfAgent.SendEmptyAck(aMessage, aMessageInfo));

exit:
    LogInfo("HandleBackboneAnswer: %s", ErrorToString(error));
}

Error Manager::SendProactiveBackboneNotification(const Ip6::Address             &aDua,
                                                 const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                                 uint32_t                        aTimeSinceLastTransaction)
{
    return SendBackboneAnswer(Get<Local>().GetAllDomainBackboneRoutersAddress(), aDua, aMeshLocalIid,
                              aTimeSinceLastTransaction, Mle::kInvalidRloc16);
}

Error Manager::SendBackboneAnswer(const Ip6::MessageInfo      &aQueryMessageInfo,
                                  const Ip6::Address          &aDua,
                                  uint16_t                     aSrcRloc16,
                                  const NdProxyTable::NdProxy &aNdProxy)
{
    return SendBackboneAnswer(aQueryMessageInfo.GetPeerAddr(), aDua, aNdProxy.GetMeshLocalIid(),
                              aNdProxy.GetTimeSinceLastTransaction(), aSrcRloc16);
}

Error Manager::SendBackboneAnswer(const Ip6::Address             &aDstAddr,
                                  const Ip6::Address             &aDua,
                                  const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                  uint32_t                        aTimeSinceLastTransaction,
                                  uint16_t                        aSrcRloc16)
{
    Error            error   = kErrorNone;
    Coap::Message   *message = nullptr;
    Ip6::MessageInfo messageInfo;
    bool             proactive = aDstAddr.IsMulticast();

    VerifyOrExit((message = mBackboneTmfAgent.NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->Init(proactive ? Coap::kTypeNonConfirmable : Coap::kTypeConfirmable, Coap::kCodePost,
                                        kUriBackboneAnswer));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::Append<ThreadTargetTlv>(*message, aDua));

    SuccessOrExit(error = Tlv::Append<ThreadMeshLocalEidTlv>(*message, aMeshLocalIid));

    SuccessOrExit(error = Tlv::Append<ThreadLastTransactionTimeTlv>(*message, aTimeSinceLastTransaction));

    SuccessOrExit(error = Tlv::Append<ThreadNetworkNameTlv>(
                      *message, Get<MeshCoP::NetworkNameManager>().GetNetworkName().GetAsCString()));

    if (aSrcRloc16 != Mle::kInvalidRloc16)
    {
        SuccessOrExit(Tlv::Append<ThreadRloc16Tlv>(*message, aSrcRloc16));
    }

    messageInfo.SetPeerAddr(aDstAddr);
    messageInfo.SetPeerPort(BackboneRouter::kBackboneUdpPort);

    messageInfo.SetHopLimit(kDefaultHoplimit);
    messageInfo.SetIsHostInterface(true);

    error = mBackboneTmfAgent.SendMessage(*message, messageInfo);

exit:
    LogInfo("Send %s for %s (rloc16=%04x): %s", proactive ? "PRO_BB.ntf" : "BB.ans", aDua.ToString().AsCString(),
            aSrcRloc16, ErrorToString(error));

    FreeMessageOnError(message, error);
    return error;
}

void Manager::HandleDadBackboneAnswer(const Ip6::Address &aDua, const Ip6::InterfaceIdentifier &aMeshLocalIid)
{
    Error                  error     = kErrorNone;
    NdProxyTable::NdProxy *ndProxy   = mNdProxyTable.ResolveDua(aDua);
    bool                   duplicate = false;

    OT_UNUSED_VARIABLE(error);

    VerifyOrExit(ndProxy != nullptr, error = kErrorNotFound);

    duplicate = ndProxy->GetMeshLocalIid() != aMeshLocalIid;

    if (duplicate)
    {
        Ip6::Address dest;

        dest.SetToRoutingLocator(Get<Mle::MleRouter>().GetMeshLocalPrefix(), ndProxy->GetRloc16());
        Get<AddressResolver>().SendAddressError(aDua, aMeshLocalIid, &dest);
    }

    ot::BackboneRouter::NdProxyTable::NotifyDadComplete(*ndProxy, duplicate);

exit:
    LogInfo("HandleDadBackboneAnswer: %s, target=%s, mliid=%s, duplicate=%s", ErrorToString(error),
            aDua.ToString().AsCString(), aMeshLocalIid.ToString().AsCString(), duplicate ? "Y" : "N");
}

void Manager::HandleExtendedBackboneAnswer(const Ip6::Address             &aDua,
                                           const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                           uint32_t                        aTimeSinceLastTransaction,
                                           uint16_t                        aSrcRloc16)
{
    Ip6::Address dest;

    dest.SetToRoutingLocator(Get<Mle::MleRouter>().GetMeshLocalPrefix(), aSrcRloc16);
    Get<AddressResolver>().SendAddressQueryResponse(aDua, aMeshLocalIid, &aTimeSinceLastTransaction, dest);

    LogInfo("HandleExtendedBackboneAnswer: target=%s, mliid=%s, LTT=%lus, rloc16=%04x", aDua.ToString().AsCString(),
            aMeshLocalIid.ToString().AsCString(), ToUlong(aTimeSinceLastTransaction), aSrcRloc16);
}

void Manager::HandleProactiveBackboneNotification(const Ip6::Address             &aDua,
                                                  const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                                  uint32_t                        aTimeSinceLastTransaction)
{
    Error                  error   = kErrorNone;
    NdProxyTable::NdProxy *ndProxy = mNdProxyTable.ResolveDua(aDua);

    OT_UNUSED_VARIABLE(error);

    VerifyOrExit(ndProxy != nullptr, error = kErrorNotFound);

    if (ndProxy->GetMeshLocalIid() == aMeshLocalIid)
    {
        uint32_t localTimeSinceLastTransaction = ndProxy->GetTimeSinceLastTransaction();

        if (aTimeSinceLastTransaction <= localTimeSinceLastTransaction)
        {
            BackboneRouter::NdProxyTable::Erase(*ndProxy);
        }
        else
        {
            IgnoreError(SendProactiveBackboneNotification(aDua, ndProxy->GetMeshLocalIid(),
                                                          ndProxy->GetTimeSinceLastTransaction()));
        }
    }
    else
    {
        // Duplicated address detected, send ADDR_ERR.ntf to ff03::2 in the Thread network
        BackboneRouter::NdProxyTable::Erase(*ndProxy);
        Get<AddressResolver>().SendAddressError(aDua, aMeshLocalIid, nullptr);
    }

exit:
    LogInfo("HandleProactiveBackboneNotification: %s, target=%s, mliid=%s, LTT=%lus", ErrorToString(error),
            aDua.ToString().AsCString(), aMeshLocalIid.ToString().AsCString(), ToUlong(aTimeSinceLastTransaction));
}
#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE

void Manager::LogError(const char *aText, Error aError) const
{
    OT_UNUSED_VARIABLE(aText);

    if (aError == kErrorNone)
    {
        LogInfo("%s: %s", aText, ErrorToString(aError));
    }
    else
    {
        LogWarn("%s: %s", aText, ErrorToString(aError));
    }
}

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
