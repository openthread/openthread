/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements the Thread Network Data managed by the Thread Leader.
 */

#include "network_data_leader.hpp"

#if OPENTHREAD_FTD

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "instance/instance.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace NetworkData {

RegisterLogModule("NetworkData");

void Leader::Start(Mle::LeaderStartMode aStartMode)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    OT_ASSERT(!mIsClone);
#endif

    mWaitingForNetDataSync = (aStartMode == Mle::kRestoringLeaderRoleAfterReset);

    if (mWaitingForNetDataSync)
    {
        mTimer.Start(kMaxNetDataSyncWait);
    }
}

void Leader::IncrementVersion(void)
{
    if (Get<Mle::MleRouter>().IsLeader())
    {
        IncrementVersions(/* aIncludeStable */ false);
    }
}

void Leader::IncrementVersionAndStableVersion(void)
{
    if (Get<Mle::MleRouter>().IsLeader())
    {
        IncrementVersions(/* aIncludeStable */ true);
    }
}

void Leader::IncrementVersions(const ChangedFlags &aFlags)
{
    if (aFlags.DidChange())
    {
        IncrementVersions(aFlags.DidStableChange());
    }
}

void Leader::IncrementVersions(bool aIncludeStable)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    VerifyOrExit(!mIsClone);
#endif

    if (aIncludeStable)
    {
        mStableVersion++;
    }

    mVersion++;
    SignalNetDataChanged();
    ExitNow();

exit:
    return;
}

void Leader::RemoveBorderRouter(uint16_t aRloc16, MatchMode aMatchMode)
{
    ChangedFlags flags;

    RemoveRloc(aRloc16, aMatchMode, flags);

    IncrementVersions(flags);
}

template <> void Leader::HandleTmf<kUriServerData>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetworkDataTlv networkDataTlv;
    uint16_t             rloc16;

    VerifyOrExit(Get<Mle::Mle>().IsLeader() && !mWaitingForNetDataSync);

    LogInfo("Received %s", UriToString<kUriServerData>());

    VerifyOrExit(aMessageInfo.GetPeerAddr().GetIid().IsRoutingLocator());

    switch (Tlv::Find<ThreadRloc16Tlv>(aMessage, rloc16))
    {
    case kErrorNone:
        RemoveBorderRouter(rloc16, kMatchModeRloc16);
        break;
    case kErrorNotFound:
        break;
    default:
        ExitNow();
    }

    if (Tlv::FindTlv(aMessage, networkDataTlv) == kErrorNone)
    {
        VerifyOrExit(networkDataTlv.IsValid());

        {
            NetworkData networkData(GetInstance(), networkDataTlv.GetTlvs(), networkDataTlv.GetLength());

            RegisterNetworkData(aMessageInfo.GetPeerAddr().GetIid().GetLocator(), networkData);
        }
    }

    SuccessOrExit(Get<Tmf::Agent>().SendEmptyAck(aMessage, aMessageInfo));

    LogInfo("Sent %s ack", UriToString<kUriServerData>());

exit:
    return;
}

template <> void Leader::HandleTmf<kUriCommissionerSet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::StateTlv::State state = MeshCoP::StateTlv::kReject;
    uint16_t                 borderAgentRloc;
    uint16_t                 sessionId;
    uint16_t                 localSessionId;

    VerifyOrExit(Get<Mle::Mle>().IsLeader() && !mWaitingForNetDataSync);

    // Validate that there is no Border Agent Locator TLV. This also
    // validates that all included TLVs are properly formatted.

    VerifyOrExit(Tlv::Find<MeshCoP::BorderAgentLocatorTlv>(aMessage, borderAgentRloc) == kErrorNotFound);

    SuccessOrExit(Tlv::Find<MeshCoP::CommissionerSessionIdTlv>(aMessage, sessionId));

    if (FindCommissioningSessionId(localSessionId) == kErrorNone)
    {
        VerifyOrExit(sessionId == localSessionId);
    }

    // Add the Border Agent RLOC TLV from Network Data.

    if (FindBorderAgentRloc(borderAgentRloc) == kErrorNone)
    {
        SuccessOrExit(Tlv::Append<MeshCoP::BorderAgentLocatorTlv>(aMessage, borderAgentRloc));
    }

    SuccessOrExit(SetCommissioningData(aMessage));

    state = MeshCoP::StateTlv::kAccept;

exit:
    if (Get<Mle::MleRouter>().IsLeader())
    {
        SendCommissioningSetResponse(aMessage, aMessageInfo, state);
    }
}

template <> void Leader::HandleTmf<kUriCommissionerGet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t       length;
    uint16_t       offset;
    Coap::Message *response = nullptr;

    VerifyOrExit(Get<Mle::Mle>().IsLeader() && !mWaitingForNetDataSync);

    response = Get<Tmf::Agent>().NewPriorityResponseMessage(aMessage);
    VerifyOrExit(response != nullptr);

    if (Tlv::FindTlvValueOffset(aMessage, MeshCoP::Tlv::kGet, offset, length) == kErrorNone)
    {
        // Append the requested sub-TLV types given in Get TLV.

        for (; length > 0; offset++, length--)
        {
            uint8_t             type;
            const MeshCoP::Tlv *subTlv;

            IgnoreError(aMessage.Read(offset, type));

            subTlv = FindCommissioningDataSubTlv(type);

            if (subTlv != nullptr)
            {
                SuccessOrExit(subTlv->AppendTo(*response));
            }
        }
    }
    else
    {
        // Append all sub-TLVs in the Commissioning Data.

        CommissioningDataTlv *dataTlv = FindCommissioningData();

        if (dataTlv != nullptr)
        {
            SuccessOrExit(response->AppendBytes(dataTlv->GetValue(), dataTlv->GetLength()));
        }
    }

    SuccessOrExit(Get<Tmf::Agent>().SendMessage(*response, aMessageInfo));
    response = nullptr; // `SendMessage` takes ownership on success

    LogInfo("Sent %s response", UriToString<kUriCommissionerGet>());

exit:
    FreeMessage(response);
}

void Leader::SendCommissioningSetResponse(const Coap::Message     &aRequest,
                                          const Ip6::MessageInfo  &aMessageInfo,
                                          MeshCoP::StateTlv::State aState)
{
    Coap::Message *message = Get<Tmf::Agent>().NewPriorityResponseMessage(aRequest);

    VerifyOrExit(message != nullptr);
    SuccessOrExit(Tlv::Append<MeshCoP::StateTlv>(*message, aState));

    SuccessOrExit(Get<Tmf::Agent>().SendMessage(*message, aMessageInfo));
    message = nullptr; // `SendMessage` takes ownership on success

    LogInfo("Sent %s response", UriToString<kUriCommissionerSet>());

exit:
    FreeMessage(message);
}

bool Leader::RlocMatch(uint16_t aFirstRloc16, uint16_t aSecondRloc16, MatchMode aMatchMode)
{
    bool matched = false;

    switch (aMatchMode)
    {
    case kMatchModeRloc16:
        matched = (aFirstRloc16 == aSecondRloc16);
        break;

    case kMatchModeRouterId:
        matched = Mle::RouterIdMatch(aFirstRloc16, aSecondRloc16);
        break;
    }

    return matched;
}

Error Leader::Validate(const NetworkData &aNetworkData, uint16_t aRloc16)
{
    // Validate that the `aTlvs` contains well-formed TLVs, sub-TLVs,
    // and entries all matching `aRloc16` (no other entry for other
    // RLOCs and no duplicates TLVs).

    Error                 error = kErrorNone;
    const NetworkDataTlv *end   = aNetworkData.GetTlvsEnd();

    for (const NetworkDataTlv *cur = aNetworkData.GetTlvsStart(); cur < end; cur = cur->GetNext())
    {
        NetworkData validatedSegment(aNetworkData.GetInstance(), aNetworkData.GetTlvsStart(), cur);

        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = kErrorParse);

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            const PrefixTlv *prefix = As<PrefixTlv>(cur);

            VerifyOrExit(prefix->IsValid(), error = kErrorParse);

            // Ensure there is no duplicate Prefix TLVs with same prefix.
            VerifyOrExit(validatedSegment.FindPrefix(prefix->GetPrefix(), prefix->GetPrefixLength()) == nullptr,
                         error = kErrorParse);

            SuccessOrExit(error = ValidatePrefix(*prefix, aRloc16));
            break;
        }

        case NetworkDataTlv::kTypeService:
        {
            const ServiceTlv *service = As<ServiceTlv>(cur);
            ServiceData       serviceData;

            VerifyOrExit(service->IsValid(), error = kErrorParse);

            service->GetServiceData(serviceData);

            // Ensure there is no duplicate Service TLV with same
            // Enterprise Number and Service Data.
            VerifyOrExit(validatedSegment.FindService(service->GetEnterpriseNumber(), serviceData,
                                                      kServiceExactMatch) == nullptr,
                         error = kErrorParse);

            SuccessOrExit(error = ValidateService(*service, aRloc16));
            break;
        }

        default:
            break;
        }
    }

exit:
    return error;
}

Error Leader::ValidatePrefix(const PrefixTlv &aPrefix, uint16_t aRloc16)
{
    // Validate that `aPrefix` TLV contains well-formed sub-TLVs and
    // and entries all matching `aRloc16` (no other entry for other
    // RLOCs).

    Error                 error                   = kErrorParse;
    const NetworkDataTlv *subEnd                  = aPrefix.GetNext();
    bool                  foundTempHasRoute       = false;
    bool                  foundStableHasRoute     = false;
    bool                  foundTempBorderRouter   = false;
    bool                  foundStableBorderRouter = false;

    for (const NetworkDataTlv *subCur = aPrefix.GetSubTlvs(); subCur < subEnd; subCur = subCur->GetNext())
    {
        VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd);

        switch (subCur->GetType())
        {
        case NetworkDataTlv::kTypeBorderRouter:
        {
            const BorderRouterTlv *borderRouter = As<BorderRouterTlv>(subCur);

            // Ensure Prefix TLV contains at most one stable and one
            // temporary Border Router sub-TLV and the sub-TLVs have
            // a single entry.

            if (borderRouter->IsStable())
            {
                VerifyOrExit(!foundStableBorderRouter);
                foundStableBorderRouter = true;
            }
            else
            {
                VerifyOrExit(!foundTempBorderRouter);
                foundTempBorderRouter = true;
            }

            VerifyOrExit(borderRouter->GetFirstEntry() == borderRouter->GetLastEntry());
            VerifyOrExit(borderRouter->GetFirstEntry()->GetRloc() == aRloc16);
            break;
        }

        case NetworkDataTlv::kTypeHasRoute:
        {
            const HasRouteTlv *hasRoute = As<HasRouteTlv>(subCur);

            // Ensure Prefix TLV contains at most one stable and one
            // temporary Has Route sub-TLV and the sub-TLVs have a
            // single entry.

            if (hasRoute->IsStable())
            {
                VerifyOrExit(!foundStableHasRoute);
                foundStableHasRoute = true;
            }
            else
            {
                VerifyOrExit(!foundTempHasRoute);
                foundTempHasRoute = true;
            }

            VerifyOrExit(hasRoute->GetFirstEntry() == hasRoute->GetLastEntry());
            VerifyOrExit(hasRoute->GetFirstEntry()->GetRloc() == aRloc16);
            break;
        }

        default:
            break;
        }
    }

    if (foundStableBorderRouter || foundTempBorderRouter || foundStableHasRoute || foundTempHasRoute)
    {
        error = kErrorNone;
    }

exit:
    return error;
}

Error Leader::ValidateService(const ServiceTlv &aService, uint16_t aRloc16)
{
    // Validate that `aService` TLV contains a single well-formed
    // Server sub-TLV associated with `aRloc16`.

    Error                 error       = kErrorParse;
    const NetworkDataTlv *subEnd      = aService.GetNext();
    bool                  foundServer = false;

    for (const NetworkDataTlv *subCur = aService.GetSubTlvs(); subCur < subEnd; subCur = subCur->GetNext())
    {
        VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd);

        switch (subCur->GetType())
        {
        case NetworkDataTlv::kTypeServer:
        {
            const ServerTlv *server = As<ServerTlv>(subCur);

            VerifyOrExit(!foundServer);
            foundServer = true;

            VerifyOrExit(server->IsValid() && server->GetServer16() == aRloc16);
            break;
        }

        default:
            break;
        }
    }

    if (foundServer)
    {
        error = kErrorNone;
    }

exit:
    return error;
}

bool Leader::ContainsMatchingEntry(const PrefixTlv *aPrefix, bool aStable, const HasRouteEntry &aEntry)
{
    // Check whether `aPrefix` has a Has Route sub-TLV with stable
    // flag `aStable` containing a matching entry to `aEntry`.

    return (aPrefix == nullptr) ? false : ContainsMatchingEntry(aPrefix->FindSubTlv<HasRouteTlv>(aStable), aEntry);
}

bool Leader::ContainsMatchingEntry(const HasRouteTlv *aHasRoute, const HasRouteEntry &aEntry)
{
    // Check whether `aHasRoute` has a matching entry to `aEntry`.

    bool contains = false;

    VerifyOrExit(aHasRoute != nullptr);

    for (const HasRouteEntry *entry = aHasRoute->GetFirstEntry(); entry <= aHasRoute->GetLastEntry(); entry++)
    {
        if (*entry == aEntry)
        {
            contains = true;
            break;
        }
    }

exit:
    return contains;
}

bool Leader::ContainsMatchingEntry(const PrefixTlv *aPrefix, bool aStable, const BorderRouterEntry &aEntry)
{
    // Check whether `aPrefix` has a Border Router sub-TLV with stable
    // flag `aStable` containing a matching entry to `aEntry`.

    return (aPrefix == nullptr) ? false : ContainsMatchingEntry(aPrefix->FindSubTlv<BorderRouterTlv>(aStable), aEntry);
}

bool Leader::ContainsMatchingEntry(const BorderRouterTlv *aBorderRouter, const BorderRouterEntry &aEntry)
{
    // Check whether `aBorderRouter` has a matching entry to `aEntry`.

    bool contains = false;

    VerifyOrExit(aBorderRouter != nullptr);

    for (const BorderRouterEntry *entry = aBorderRouter->GetFirstEntry(); entry <= aBorderRouter->GetLastEntry();
         entry++)
    {
        if (*entry == aEntry)
        {
            contains = true;
            break;
        }
    }

exit:
    return contains;
}

bool Leader::ContainsMatchingServer(const ServiceTlv *aService, const ServerTlv &aServer)
{
    // Check whether the `aService` has a matching Server sub-TLV
    // same as `aServer`.

    bool contains = false;

    if (aService != nullptr)
    {
        const ServerTlv *server;
        TlvIterator      subTlvIterator(*aService);

        while ((server = subTlvIterator.Iterate<ServerTlv>(aServer.IsStable())) != nullptr)
        {
            if (*server == aServer)
            {
                contains = true;
                break;
            }
        }
    }

    return contains;
}

Leader::UpdateStatus Leader::UpdatePrefix(PrefixTlv &aPrefix) { return UpdateTlv(aPrefix, aPrefix.GetSubTlvs()); }

Leader::UpdateStatus Leader::UpdateService(ServiceTlv &aService) { return UpdateTlv(aService, aService.GetSubTlvs()); }

Leader::UpdateStatus Leader::UpdateTlv(NetworkDataTlv &aTlv, const NetworkDataTlv *aSubTlvs)
{
    // If `aTlv` contains no sub-TLVs, remove it from Network Data,
    // otherwise update its stable flag based on its sub-TLVs.

    UpdateStatus status = kTlvUpdated;

    if (aSubTlvs == aTlv.GetNext())
    {
        RemoveTlv(&aTlv);
        ExitNow(status = kTlvRemoved);
    }

    for (const NetworkDataTlv *subCur = aSubTlvs; subCur < aTlv.GetNext(); subCur = subCur->GetNext())
    {
        if (subCur->IsStable())
        {
            aTlv.SetStable();
            ExitNow();
        }
    }

    aTlv.ClearStable();

exit:
    return status;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL

void Leader::CheckForNetDataGettingFull(const NetworkData &aNetworkData, uint16_t aOldRloc16)
{
    // Determines whether there is still room in Network Data to register
    // `aNetworkData` entries. The `aNetworkData` MUST follow the format of
    // local Network Data (e.g., all entries associated with the RLOC16 of
    // this device). Network data getting full is signaled by invoking the
    // `Get<Notifier>().SignalNetworkDataFull()` method.
    //
    // Input `aOldRloc16` can be used to indicate the old RLOC16 of the
    // device. If provided, then entries matching old RLOC16 are first
    // removed, before checking if new entries from @p aNetworkData can fit.

    if (!Get<Mle::MleRouter>().IsLeader())
    {
        // Create a clone of the leader's network data, and try to register
        // `aNetworkData` into the copy (as if this device itself is the
        // leader). `mIsClone` flag is used to mark the clone and ensure
        // that the cloned instance does interact with other OT modules,
        // e.g., does not start timer, or does not signal version change
        // using `Get<ot::Notifier>().Signal()`, or allocate service or
        // context ID.

        Leader leaderClone(GetInstance());

        leaderClone.MarkAsClone();
        SuccessOrAssert(CopyNetworkData(kFullSet, leaderClone));

        if (aOldRloc16 != Mac::kShortAddrInvalid)
        {
            leaderClone.RemoveBorderRouter(aOldRloc16, kMatchModeRloc16);
        }

        leaderClone.RegisterNetworkData(Get<Mle::Mle>().GetRloc16(), aNetworkData);
    }
}

void Leader::MarkAsClone(void)
{
    mIsClone = true;
    mContextIds.MarkAsClone();
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL

void Leader::RegisterNetworkData(uint16_t aRloc16, const NetworkData &aNetworkData)
{
    Error        error = kErrorNone;
    ChangedFlags flags;

    VerifyOrExit(Get<RouterTable>().IsAllocated(Mle::RouterIdFromRloc16(aRloc16)), error = kErrorNoRoute);

    // Validate that the `aNetworkData` contains well-formed TLVs, sub-TLVs,
    // and entries all matching `aRloc16` (no other RLOCs).
    SuccessOrExit(error = Validate(aNetworkData, aRloc16));

    // Remove all entries matching `aRloc16` excluding entries that are
    // present in `aNetworkData`
    RemoveRloc(aRloc16, kMatchModeRloc16, aNetworkData, flags);

    // Now add all new entries in `aTlvs` to Network Data.
    for (const NetworkDataTlv *cur = aNetworkData.GetTlvsStart(); cur < aNetworkData.GetTlvsEnd(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            SuccessOrExit(error = AddPrefix(*As<PrefixTlv>(cur), flags));
            break;

        case NetworkDataTlv::kTypeService:
            SuccessOrExit(error = AddService(*As<ServiceTlv>(cur), flags));
            break;

        default:
            break;
        }
    }

    DumpDebg("Register", GetBytes(), GetLength());

exit:
    IncrementVersions(flags);

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    if (error == kErrorNoBufs)
    {
        Get<Notifier>().SignalNetworkDataFull();
    }

    if (!mIsClone)
#endif
    {
        if (error != kErrorNone)
        {
            LogNote("Failed to register network data: %s", ErrorToString(error));
        }
    }
}

Error Leader::AddPrefix(const PrefixTlv &aPrefix, ChangedFlags &aChangedFlags)
{
    Error      error     = kErrorNone;
    PrefixTlv *dstPrefix = FindPrefix(aPrefix.GetPrefix(), aPrefix.GetPrefixLength());

    if (dstPrefix == nullptr)
    {
        dstPrefix = As<PrefixTlv>(AppendTlv(PrefixTlv::CalculateSize(aPrefix.GetPrefixLength())));
        VerifyOrExit(dstPrefix != nullptr, error = kErrorNoBufs);

        dstPrefix->Init(aPrefix.GetDomainId(), aPrefix.GetPrefixLength(), aPrefix.GetPrefix());
    }

    for (const NetworkDataTlv *subCur = aPrefix.GetSubTlvs(); subCur < aPrefix.GetNext(); subCur = subCur->GetNext())
    {
        switch (subCur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            SuccessOrExit(error = AddHasRoute(*As<HasRouteTlv>(subCur), *dstPrefix, aChangedFlags));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            SuccessOrExit(error = AddBorderRouter(*As<BorderRouterTlv>(subCur), *dstPrefix, aChangedFlags));
            break;

        default:
            break;
        }
    }

exit:
    if (dstPrefix != nullptr)
    {
        // `UpdatePrefix()` updates the TLV's stable flag based on
        // its sub-TLVs, or removes the TLV if it contains no sub-TLV.
        // This is called at `exit` to ensure that if appending
        // sub-TLVs fail (e.g., out of space in network data), we
        // remove an empty Prefix TLV.

        IgnoreReturnValue(UpdatePrefix(*dstPrefix));
    }

    return error;
}

Error Leader::AddService(const ServiceTlv &aService, ChangedFlags &aChangedFlags)
{
    Error            error = kErrorNone;
    ServiceTlv      *dstService;
    ServiceData      serviceData;
    const ServerTlv *server;

    aService.GetServiceData(serviceData);
    dstService = FindService(aService.GetEnterpriseNumber(), serviceData, kServiceExactMatch);

    if (dstService == nullptr)
    {
        uint8_t serviceId;

        SuccessOrExit(error = AllocateServiceId(serviceId));

        dstService = As<ServiceTlv>(
            AppendTlv(ServiceTlv::CalculateSize(aService.GetEnterpriseNumber(), serviceData.GetLength())));
        VerifyOrExit(dstService != nullptr, error = kErrorNoBufs);

        dstService->Init(serviceId, aService.GetEnterpriseNumber(), serviceData);
    }

    server = NetworkDataTlv::Find<ServerTlv>(aService.GetSubTlvs(), aService.GetNext());
    OT_ASSERT(server != nullptr);

    SuccessOrExit(error = AddServer(*server, *dstService, aChangedFlags));

exit:
    if (dstService != nullptr)
    {
        // `UpdateService()` updates the TLV's stable flag based on
        // its sub-TLVs, or removes the TLV if it contains no sub-TLV.
        // This is called at `exit` to ensure that if appending
        // sub-TLVs fail (e.g., out of space in network data), we
        // remove an empty Service TLV.

        IgnoreReturnValue(UpdateService(*dstService));
    }

    return error;
}

Error Leader::AddHasRoute(const HasRouteTlv &aHasRoute, PrefixTlv &aDstPrefix, ChangedFlags &aChangedFlags)
{
    Error                error       = kErrorNone;
    HasRouteTlv         *dstHasRoute = aDstPrefix.FindSubTlv<HasRouteTlv>(aHasRoute.IsStable());
    const HasRouteEntry *entry       = aHasRoute.GetFirstEntry();

    if (dstHasRoute == nullptr)
    {
        // Ensure there is space for `HasRouteTlv` and a single entry.
        VerifyOrExit(CanInsert(sizeof(HasRouteTlv) + sizeof(HasRouteEntry)), error = kErrorNoBufs);

        dstHasRoute = As<HasRouteTlv>(aDstPrefix.GetNext());
        Insert(dstHasRoute, sizeof(HasRouteTlv));
        aDstPrefix.IncreaseLength(sizeof(HasRouteTlv));
        dstHasRoute->Init();

        if (aHasRoute.IsStable())
        {
            dstHasRoute->SetStable();
        }
    }

    VerifyOrExit(!ContainsMatchingEntry(dstHasRoute, *entry));

    VerifyOrExit(CanInsert(sizeof(HasRouteEntry)), error = kErrorNoBufs);

    Insert(dstHasRoute->GetNext(), sizeof(HasRouteEntry));
    dstHasRoute->IncreaseLength(sizeof(HasRouteEntry));
    aDstPrefix.IncreaseLength(sizeof(HasRouteEntry));

    *dstHasRoute->GetLastEntry() = *entry;
    aChangedFlags.Update(*dstHasRoute);

exit:
    return error;
}

Error Leader::AddBorderRouter(const BorderRouterTlv &aBorderRouter, PrefixTlv &aDstPrefix, ChangedFlags &aChangedFlags)
{
    Error                    error           = kErrorNone;
    BorderRouterTlv         *dstBorderRouter = aDstPrefix.FindSubTlv<BorderRouterTlv>(aBorderRouter.IsStable());
    ContextTlv              *dstContext      = aDstPrefix.FindSubTlv<ContextTlv>();
    uint8_t                  contextId       = 0;
    const BorderRouterEntry *entry           = aBorderRouter.GetFirstEntry();

    if (dstContext == nullptr)
    {
        // Get a new Context ID first. This ensure that if we cannot
        // get new Context ID, we fail and exit before potentially
        // inserting a Border Router sub-TLV.
        SuccessOrExit(error = mContextIds.GetUnallocatedId(contextId));
    }

    if (dstBorderRouter == nullptr)
    {
        // Ensure there is space for `BorderRouterTlv` with a single entry
        // and a `ContextTlv` (if not already present).
        VerifyOrExit(CanInsert(sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry) +
                               ((dstContext == nullptr) ? sizeof(ContextTlv) : 0)),
                     error = kErrorNoBufs);

        dstBorderRouter = As<BorderRouterTlv>(aDstPrefix.GetNext());
        Insert(dstBorderRouter, sizeof(BorderRouterTlv));
        aDstPrefix.IncreaseLength(sizeof(BorderRouterTlv));
        dstBorderRouter->Init();

        if (aBorderRouter.IsStable())
        {
            dstBorderRouter->SetStable();
        }
    }

    if (dstContext == nullptr)
    {
        // Ensure there is space for a `ContextTlv` and a single entry.
        VerifyOrExit(CanInsert(sizeof(BorderRouterEntry) + sizeof(ContextTlv)), error = kErrorNoBufs);

        dstContext = As<ContextTlv>(aDstPrefix.GetNext());
        Insert(dstContext, sizeof(ContextTlv));
        aDstPrefix.IncreaseLength(sizeof(ContextTlv));
        dstContext->Init(static_cast<uint8_t>(contextId), aDstPrefix.GetPrefixLength());
    }

    if (aBorderRouter.IsStable())
    {
        dstContext->SetStable();
    }

    dstContext->SetCompress();
    mContextIds.MarkAsInUse(dstContext->GetContextId());

    VerifyOrExit(!ContainsMatchingEntry(dstBorderRouter, *entry));

    VerifyOrExit(CanInsert(sizeof(BorderRouterEntry)), error = kErrorNoBufs);

    Insert(dstBorderRouter->GetNext(), sizeof(BorderRouterEntry));
    dstBorderRouter->IncreaseLength(sizeof(BorderRouterEntry));
    aDstPrefix.IncreaseLength(sizeof(BorderRouterEntry));
    *dstBorderRouter->GetLastEntry() = *entry;
    aChangedFlags.Update(*dstBorderRouter);

exit:
    return error;
}

Error Leader::AddServer(const ServerTlv &aServer, ServiceTlv &aDstService, ChangedFlags &aChangedFlags)
{
    Error      error = kErrorNone;
    ServerTlv *dstServer;
    ServerData serverData;
    uint8_t    tlvSize = aServer.GetSize();

    VerifyOrExit(!ContainsMatchingServer(&aDstService, aServer));

    VerifyOrExit(CanInsert(tlvSize), error = kErrorNoBufs);

    aServer.GetServerData(serverData);

    dstServer = As<ServerTlv>(aDstService.GetNext());
    Insert(dstServer, tlvSize);
    dstServer->Init(aServer.GetServer16(), serverData);

    if (aServer.IsStable())
    {
        dstServer->SetStable();
    }

    aDstService.IncreaseLength(tlvSize);
    aChangedFlags.Update(*dstServer);

exit:
    return error;
}

Error Leader::AllocateServiceId(uint8_t &aServiceId) const
{
    Error   error = kErrorNotFound;
    uint8_t serviceId;

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    if (mIsClone)
    {
        aServiceId = kMinServiceId;
        error      = kErrorNone;
        ExitNow();
    }
#endif

    for (serviceId = kMinServiceId; serviceId <= kMaxServiceId; serviceId++)
    {
        if (FindServiceById(serviceId) == nullptr)
        {
            aServiceId = serviceId;
            error      = kErrorNone;
            LogInfo("Allocated Service ID = %d", serviceId);
            ExitNow();
        }
    }

exit:
    return error;
}

const ServiceTlv *Leader::FindServiceById(uint8_t aServiceId) const
{
    const ServiceTlv *service;
    TlvIterator       tlvIterator(GetTlvsStart(), GetTlvsEnd());

    while ((service = tlvIterator.Iterate<ServiceTlv>()) != nullptr)
    {
        if (service->GetServiceId() == aServiceId)
        {
            break;
        }
    }

    return service;
}

void Leader::RemoveRloc(uint16_t aRloc16, MatchMode aMatchMode, ChangedFlags &aChangedFlags)
{
    NetworkData excludeNetworkData(GetInstance()); // Empty network data.

    RemoveRloc(aRloc16, aMatchMode, excludeNetworkData, aChangedFlags);
}

void Leader::RemoveRloc(uint16_t           aRloc16,
                        MatchMode          aMatchMode,
                        const NetworkData &aExcludeNetworkData,
                        ChangedFlags      &aChangedFlags)
{
    // Remove entries from Network Data matching `aRloc16` (using
    // `aMatchMode` to determine the match) but exclude any entries
    // that are present in `aExcludeNetworkData`. As entries are
    // removed update `aChangedFlags` to indicate if Network Data
    // (stable or not) got changed.

    NetworkDataTlv *cur = GetTlvsStart();

    while (cur < GetTlvsEnd())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            PrefixTlv       *prefix = As<PrefixTlv>(cur);
            const PrefixTlv *excludePrefix =
                aExcludeNetworkData.FindPrefix(prefix->GetPrefix(), prefix->GetPrefixLength());

            RemoveRlocInPrefix(*prefix, aRloc16, aMatchMode, excludePrefix, aChangedFlags);

            if (UpdatePrefix(*prefix) == kTlvRemoved)
            {
                // Do not update `cur` when TLV is removed.
                continue;
            }

            break;
        }

        case NetworkDataTlv::kTypeService:
        {
            ServiceTlv       *service = As<ServiceTlv>(cur);
            ServiceData       serviceData;
            const ServiceTlv *excludeService;

            service->GetServiceData(serviceData);

            excludeService =
                aExcludeNetworkData.FindService(service->GetEnterpriseNumber(), serviceData, kServiceExactMatch);

            RemoveRlocInService(*service, aRloc16, aMatchMode, excludeService, aChangedFlags);

            if (UpdateService(*service) == kTlvRemoved)
            {
                // Do not update `cur` when TLV is removed.
                continue;
            }

            break;
        }

        default:
            break;
        }

        cur = cur->GetNext();
    }
}

void Leader::RemoveRlocInPrefix(PrefixTlv       &aPrefix,
                                uint16_t         aRloc16,
                                MatchMode        aMatchMode,
                                const PrefixTlv *aExcludePrefix,
                                ChangedFlags    &aChangedFlags)
{
    // Remove entries in `aPrefix` TLV matching the given `aRloc16`
    // excluding any entries that are present in `aExcludePrefix`.

    NetworkDataTlv *cur = aPrefix.GetSubTlvs();
    ContextTlv     *context;

    while (cur < aPrefix.GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            RemoveRlocInHasRoute(aPrefix, *As<HasRouteTlv>(cur), aRloc16, aMatchMode, aExcludePrefix, aChangedFlags);

            if (cur->GetLength() == 0)
            {
                aPrefix.DecreaseLength(sizeof(HasRouteTlv));
                RemoveTlv(cur);
                continue;
            }

            break;

        case NetworkDataTlv::kTypeBorderRouter:
            RemoveRlocInBorderRouter(aPrefix, *As<BorderRouterTlv>(cur), aRloc16, aMatchMode, aExcludePrefix,
                                     aChangedFlags);

            if (cur->GetLength() == 0)
            {
                aPrefix.DecreaseLength(sizeof(BorderRouterTlv));
                RemoveTlv(cur);
                continue;
            }

            break;

        default:
            break;
        }

        cur = cur->GetNext();
    }

    if ((context = aPrefix.FindSubTlv<ContextTlv>()) != nullptr)
    {
        if (aPrefix.FindSubTlv<BorderRouterTlv>() == nullptr)
        {
            context->ClearCompress();
            mContextIds.ScheduleToRemove(context->GetContextId());
        }
        else
        {
            context->SetCompress();
            mContextIds.MarkAsInUse(context->GetContextId());
        }
    }
}

void Leader::RemoveRlocInService(ServiceTlv       &aService,
                                 uint16_t          aRloc16,
                                 MatchMode         aMatchMode,
                                 const ServiceTlv *aExcludeService,
                                 ChangedFlags     &aChangedFlags)
{
    // Remove entries in `aService` TLV matching the given `aRloc16`
    // excluding any entries that are present in `aExcludeService`.

    NetworkDataTlv *start = aService.GetSubTlvs();
    ServerTlv      *server;

    while ((server = NetworkDataTlv::Find<ServerTlv>(start, aService.GetNext())) != nullptr)
    {
        if (RlocMatch(server->GetServer16(), aRloc16, aMatchMode) && !ContainsMatchingServer(aExcludeService, *server))
        {
            uint8_t subTlvSize = server->GetSize();

            aChangedFlags.Update(*server);
            RemoveTlv(server);
            aService.DecreaseLength(subTlvSize);
            continue;
        }

        start = server->GetNext();
    }
}

void Leader::RemoveRlocInHasRoute(PrefixTlv       &aPrefix,
                                  HasRouteTlv     &aHasRoute,
                                  uint16_t         aRloc16,
                                  MatchMode        aMatchMode,
                                  const PrefixTlv *aExcludePrefix,
                                  ChangedFlags    &aChangedFlags)
{
    // Remove entries in `aHasRoute` (a sub-TLV of `aPrefix` TLV)
    // matching the given `aRloc16` excluding entries that are present
    // in `aExcludePrefix`.

    HasRouteEntry *entry = aHasRoute.GetFirstEntry();

    while (entry <= aHasRoute.GetLastEntry())
    {
        if (RlocMatch(entry->GetRloc(), aRloc16, aMatchMode) &&
            !ContainsMatchingEntry(aExcludePrefix, aHasRoute.IsStable(), *entry))
        {
            aChangedFlags.Update(aHasRoute);
            aHasRoute.DecreaseLength(sizeof(HasRouteEntry));
            aPrefix.DecreaseLength(sizeof(HasRouteEntry));
            Remove(entry, sizeof(HasRouteEntry));
            continue;
        }

        entry = entry->GetNext();
    }
}

void Leader::RemoveRlocInBorderRouter(PrefixTlv       &aPrefix,
                                      BorderRouterTlv &aBorderRouter,
                                      uint16_t         aRloc16,
                                      MatchMode        aMatchMode,
                                      const PrefixTlv *aExcludePrefix,
                                      ChangedFlags    &aChangedFlags)
{
    // Remove entries in `aBorderRouter` (a sub-TLV of `aPrefix` TLV)
    // matching the given `aRloc16` excluding entries that are present
    // in `aExcludePrefix`.

    BorderRouterEntry *entry = aBorderRouter.GetFirstEntry();

    while (entry <= aBorderRouter.GetLastEntry())
    {
        if (RlocMatch(entry->GetRloc(), aRloc16, aMatchMode) &&
            !ContainsMatchingEntry(aExcludePrefix, aBorderRouter.IsStable(), *entry))
        {
            aChangedFlags.Update(aBorderRouter);
            aBorderRouter.DecreaseLength(sizeof(BorderRouterEntry));
            aPrefix.DecreaseLength(sizeof(BorderRouterEntry));
            Remove(entry, sizeof(*entry));
            continue;
        }

        entry = entry->GetNext();
    }
}

void Leader::RemoveContext(uint8_t aContextId)
{
    NetworkDataTlv *start = GetTlvsStart();
    PrefixTlv      *prefix;

    while ((prefix = NetworkDataTlv::Find<PrefixTlv>(start, GetTlvsEnd())) != nullptr)
    {
        RemoveContext(*prefix, aContextId);

        if (UpdatePrefix(*prefix) == kTlvRemoved)
        {
            // Do not update `start` when TLV is removed.
            continue;
        }

        start = prefix->GetNext();
    }

    IncrementVersions(/* aIncludeStable */ true);
}

void Leader::RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId)
{
    NetworkDataTlv *start = aPrefix.GetSubTlvs();
    ContextTlv     *context;

    while ((context = NetworkDataTlv::Find<ContextTlv>(start, aPrefix.GetNext())) != nullptr)
    {
        if (context->GetContextId() == aContextId)
        {
            uint8_t subTlvSize = context->GetSize();
            RemoveTlv(context);
            aPrefix.DecreaseLength(subTlvSize);
            continue;
        }

        start = context->GetNext();
    }
}

void Leader::HandleNetworkDataRestoredAfterReset(void)
{
    const PrefixTlv *prefix;
    TlvIterator      tlvIterator(GetTlvsStart(), GetTlvsEnd());
    Iterator         iterator = kIteratorInit;
    ChangedFlags     flags;
    uint16_t         rloc16;
    uint16_t         sessionId;

    mWaitingForNetDataSync = false;

    // Remove entries in Network Data from any un-allocated Router ID.
    // This acts as a safeguard against an edge case where the leader
    // is reset at an inopportune time, such as right after it removed
    // an allocated router ID and sent MLE advertisement but before it
    // got the chance to send the updated Network Data to other
    // routers.

    while (GetNextServer(iterator, rloc16) == kErrorNone)
    {
        if (!Get<RouterTable>().IsAllocated(Mle::RouterIdFromRloc16(rloc16)))
        {
            // After we `RemoveRloc()` the Network Data gets changed
            // and the `iterator` will not be valid anymore. So we set
            // it to `kIteratorInit` to restart the loop.

            RemoveRloc(rloc16, kMatchModeRouterId, flags);
            iterator = kIteratorInit;
        }
    }

    IncrementVersions(flags);

    // Synchronize internal 6LoWPAN Context ID Set with the
    // recently obtained Network Data.

    while ((prefix = tlvIterator.Iterate<PrefixTlv>()) != nullptr)
    {
        const ContextTlv *context = prefix->FindSubTlv<ContextTlv>();

        if (context == nullptr)
        {
            continue;
        }

        mContextIds.MarkAsInUse(context->GetContextId());

        if (!context->IsCompress())
        {
            mContextIds.ScheduleToRemove(context->GetContextId());
        }
    }

    // Update Commissioning Data. We adopt the same session ID
    // (if any) and resign active commissioner (if any) by
    // clearing the Commissioning Data.

    if (FindCommissioningSessionId(sessionId) == kErrorNone)
    {
        Get<MeshCoP::Leader>().SetSessionId(sessionId);
    }

    if (FindBorderAgentRloc(rloc16) == kErrorNone)
    {
        Get<MeshCoP::Leader>().SetEmptyCommissionerData();
    }
}

Error Leader::UpdateCommissioningData(uint16_t aDataLength, CommissioningDataTlv *&aDataTlv)
{
    // First determine whether or not we can add Commissioning Data
    // TLV with the given `aDataLength`, taking into account that we
    // would remove the current Commissioning Data TLV. Then remove
    // the current TLV and append a new TLV with proper size which is
    // returned in `aDataTlv`.

    Error                 error   = kErrorNone;
    CommissioningDataTlv *dataTlv = FindCommissioningData();
    uint16_t              insertLength;

    if (dataTlv != nullptr)
    {
        insertLength = (aDataLength <= dataTlv->GetLength()) ? 0 : aDataLength - dataTlv->GetLength();
    }
    else
    {
        insertLength = sizeof(CommissioningDataTlv) + aDataLength;
    }

    VerifyOrExit(CanInsert(insertLength), error = kErrorNoBufs);

    if (dataTlv != nullptr)
    {
        RemoveTlv(dataTlv);
    }

    aDataTlv = As<CommissioningDataTlv>(AppendTlv(sizeof(CommissioningDataTlv) + aDataLength));

    OT_ASSERT(aDataTlv != nullptr);

    aDataTlv->Init();
    aDataTlv->SetLength(static_cast<uint8_t>(aDataLength));

    // The caller would fill the `aDataTlv` value.

    mVersion++;
    SignalNetDataChanged();

exit:
    return error;
}

Error Leader::SetCommissioningData(const void *aData, uint8_t aDataLength)
{
    Error                 error = kErrorNone;
    CommissioningDataTlv *dataTlv;

    SuccessOrExit(error = UpdateCommissioningData(aDataLength, dataTlv));
    memcpy(dataTlv->GetValue(), aData, aDataLength);

exit:
    return error;
}

Error Leader::SetCommissioningData(const Message &aMessage)
{
    Error                 error      = kErrorNone;
    uint16_t              dataLength = aMessage.GetLength() - aMessage.GetOffset();
    CommissioningDataTlv *dataTlv;

    SuccessOrExit(error = UpdateCommissioningData(dataLength, dataTlv));
    aMessage.ReadBytes(aMessage.GetOffset(), dataTlv->GetValue(), dataLength);

exit:
    return error;
}

void Leader::HandleTimer(void)
{
    if (mWaitingForNetDataSync)
    {
        LogInfo("Timed out waiting for netdata on restoring leader role after reset");
        IgnoreError(Get<Mle::MleRouter>().BecomeDetached());
    }
    else
    {
        mContextIds.HandleTimer();
    }
}

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
bool Leader::ContainsOmrPrefix(const Ip6::Prefix &aPrefix)
{
    PrefixTlv *prefixTlv;
    bool       contains = false;

    VerifyOrExit(BorderRouter::RoutingManager::IsValidOmrPrefix(aPrefix));

    prefixTlv = FindPrefix(aPrefix);
    VerifyOrExit(prefixTlv != nullptr);

    for (int i = 0; i < 2; i++)
    {
        const BorderRouterTlv *borderRouter = prefixTlv->FindSubTlv<BorderRouterTlv>(/* aStable */ (i == 0));

        if (borderRouter == nullptr)
        {
            continue;
        }

        for (const BorderRouterEntry *entry = borderRouter->GetFirstEntry(); entry <= borderRouter->GetLastEntry();
             entry                          = entry->GetNext())
        {
            OnMeshPrefixConfig config;

            config.SetFrom(*prefixTlv, *borderRouter, *entry);

            if (BorderRouter::RoutingManager::IsValidOmrPrefix(config))
            {
                ExitNow(contains = true);
            }
        }
    }

exit:
    return contains;
}
#endif

//---------------------------------------------------------------------------------------------------------------------
// Leader::ContextIds

Leader::ContextIds::ContextIds(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mReuseDelay(kReuseDelay)
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    , mIsClone(false)
#endif
{
}

void Leader::ContextIds::Clear(void)
{
    for (uint8_t id = kMinId; id <= kMaxId; id++)
    {
        MarkAsUnallocated(id);
    }
}

Error Leader::ContextIds::GetUnallocatedId(uint8_t &aId)
{
    Error error = kErrorNotFound;

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    if (mIsClone)
    {
        aId   = kMinId;
        error = kErrorNone;
        ExitNow();
    }
#endif

    for (uint8_t id = kMinId; id <= kMaxId; id++)
    {
        if (IsUnallocated(id))
        {
            aId   = id;
            error = kErrorNone;
            ExitNow();
        }
    }

exit:
    return error;
}

void Leader::ContextIds::ScheduleToRemove(uint8_t aId)
{
#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    VerifyOrExit(!mIsClone);
#endif

    VerifyOrExit(IsInUse(aId));

    SetRemoveTime(aId, TimerMilli::GetNow() + Time::SecToMsec(mReuseDelay));
    Get<Leader>().mTimer.FireAtIfEarlier(GetRemoveTime(aId));

exit:
    return;
}

void Leader::ContextIds::SetRemoveTime(uint8_t aId, TimeMilli aTime)
{
    uint32_t time = aTime.GetValue();

    while ((time == kUnallocated) || (time == kInUse))
    {
        time++;
    }

    mRemoveTimes[aId - kMinId].SetValue(time);
}

void Leader::ContextIds::HandleTimer(void)
{
    TimeMilli now      = TimerMilli::GetNow();
    TimeMilli nextTime = now.GetDistantFuture();

#if OPENTHREAD_CONFIG_BORDER_ROUTER_SIGNAL_NETWORK_DATA_FULL
    OT_ASSERT(!mIsClone);
#endif

    for (uint8_t id = kMinId; id <= kMaxId; id++)
    {
        if (IsUnallocated(id) || IsInUse(id))
        {
            continue;
        }

        if (now >= GetRemoveTime(id))
        {
            MarkAsUnallocated(id);
            Get<Leader>().RemoveContext(id);
        }
        else
        {
            nextTime = Min(nextTime, GetRemoveTime(id));
        }
    }

    if (nextTime != now.GetDistantFuture())
    {
        Get<Leader>().mTimer.FireAt(nextTime);
    }
}

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_FTD
