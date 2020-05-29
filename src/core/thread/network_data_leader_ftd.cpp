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

#if OPENTHREAD_FTD

#include "network_data_leader.hpp"

#include "coap/coap_message.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/meshcop.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

namespace ot {
namespace NetworkData {

Leader::Leader(Instance &aInstance)
    : LeaderBase(aInstance)
    , mTimer(aInstance, &Leader::HandleTimer, this)
    , mServerData(OT_URI_PATH_SERVER_DATA, &Leader::HandleServerData, this)
    , mCommissioningDataGet(OT_URI_PATH_COMMISSIONER_GET, &Leader::HandleCommissioningGet, this)
    , mCommissioningDataSet(OT_URI_PATH_COMMISSIONER_SET, &Leader::HandleCommissioningSet, this)
{
    Reset();
}

void Leader::Reset(void)
{
    LeaderBase::Reset();

    memset(mContextLastUsed, 0, sizeof(mContextLastUsed));
    mContextUsed         = 0;
    mContextIdReuseDelay = kContextIdReuseDelay;
}

void Leader::Start(void)
{
    Get<Coap::Coap>().AddResource(mServerData);
    Get<Coap::Coap>().AddResource(mCommissioningDataGet);
    Get<Coap::Coap>().AddResource(mCommissioningDataSet);
}

void Leader::Stop(void)
{
    Get<Coap::Coap>().RemoveResource(mServerData);
    Get<Coap::Coap>().RemoveResource(mCommissioningDataGet);
    Get<Coap::Coap>().RemoveResource(mCommissioningDataSet);
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
    if (aIncludeStable)
    {
        mStableVersion++;
    }

    mVersion++;
    Get<ot::Notifier>().Signal(OT_CHANGED_THREAD_NETDATA);
}

void Leader::RemoveBorderRouter(uint16_t aRloc16, MatchMode aMatchMode)
{
    ChangedFlags flags;

    RemoveRloc(aRloc16, aMatchMode, flags);
    IncrementVersions(flags);
}

void Leader::HandleServerData(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleServerData(*static_cast<Coap::Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleServerData(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetworkDataTlv networkData;
    uint16_t             rloc16;

    otLogInfoNetData("Received network data registration");

    VerifyOrExit(aMessageInfo.GetPeerAddr().IsIidRoutingLocator(), OT_NOOP);

    switch (Tlv::FindUint16Tlv(aMessage, ThreadTlv::kRloc16, rloc16))
    {
    case OT_ERROR_NONE:
        RemoveBorderRouter(rloc16, kMatchModeRloc16);
        break;
    case OT_ERROR_NOT_FOUND:
        break;
    default:
        ExitNow();
    }

    if (ThreadTlv::FindTlv(aMessage, ThreadTlv::kThreadNetworkData, sizeof(networkData), networkData) == OT_ERROR_NONE)
    {
        VerifyOrExit(networkData.IsValid(), OT_NOOP);
        RegisterNetworkData(aMessageInfo.GetPeerAddr().GetLocator(), networkData.GetTlvs(), networkData.GetLength());
    }

    SuccessOrExit(Get<Coap::Coap>().SendEmptyAck(aMessage, aMessageInfo));

    otLogInfoNetData("Sent network data registration acknowledgment");

exit:
    return;
}

void Leader::HandleCommissioningSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleCommissioningSet(*static_cast<Coap::Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleCommissioningSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t                 offset = aMessage.GetOffset();
    uint16_t                 length = aMessage.GetLength() - aMessage.GetOffset();
    uint8_t                  tlvs[NetworkData::kMaxSize];
    MeshCoP::StateTlv::State state        = MeshCoP::StateTlv::kReject;
    bool                     hasSessionId = false;
    bool                     hasValidTlv  = false;
    uint16_t                 sessionId    = 0;
    CommissioningDataTlv *   commDataTlv;

    MeshCoP::Tlv *cur;
    MeshCoP::Tlv *end;

    VerifyOrExit(length <= sizeof(tlvs), OT_NOOP);
    VerifyOrExit(Get<Mle::MleRouter>().IsLeader(), OT_NOOP);

    aMessage.Read(offset, length, tlvs);

    // Session Id and Border Router Locator MUST NOT be set, but accept including unexpected or
    // unknown TLV as long as there is at least one valid TLV.
    cur = reinterpret_cast<MeshCoP::Tlv *>(tlvs);
    end = reinterpret_cast<MeshCoP::Tlv *>(tlvs + length);

    while (cur < end)
    {
        MeshCoP::Tlv::Type type;

        VerifyOrExit(((cur + 1) <= end) && !cur->IsExtended() && (cur->GetNext() <= end), OT_NOOP);

        type = cur->GetType();

        if (type == MeshCoP::Tlv::kJoinerUdpPort || type == MeshCoP::Tlv::kSteeringData)
        {
            hasValidTlv = true;
        }
        else if (type == MeshCoP::Tlv::kBorderAgentLocator)
        {
            ExitNow();
        }
        else if (type == MeshCoP::Tlv::kCommissionerSessionId)
        {
            MeshCoP::CommissionerSessionIdTlv *tlv = static_cast<MeshCoP::CommissionerSessionIdTlv *>(cur);

            VerifyOrExit(tlv->IsValid(), OT_NOOP);
            sessionId    = tlv->GetCommissionerSessionId();
            hasSessionId = true;
        }
        else
        {
            // do nothing for unexpected or unknown TLV
        }

        cur = cur->GetNext();
    }

    // verify whether or not commissioner session id TLV is included
    VerifyOrExit(hasSessionId, OT_NOOP);

    // verify whether or not MGMT_COMM_SET.req includes at least one valid TLV
    VerifyOrExit(hasValidTlv, OT_NOOP);

    // Find Commissioning Data TLV
    commDataTlv = GetCommissioningData();

    if (commDataTlv != NULL)
    {
        // Iterate over MeshCoP TLVs and extract desired data
        for (cur = reinterpret_cast<MeshCoP::Tlv *>(commDataTlv->GetValue());
             cur < reinterpret_cast<MeshCoP::Tlv *>(commDataTlv->GetValue() + commDataTlv->GetLength());
             cur = cur->GetNext())
        {
            if (cur->GetType() == MeshCoP::Tlv::kCommissionerSessionId)
            {
                VerifyOrExit(sessionId ==
                                 static_cast<MeshCoP::CommissionerSessionIdTlv *>(cur)->GetCommissionerSessionId(),
                             OT_NOOP);
            }
            else if (cur->GetType() == MeshCoP::Tlv::kBorderAgentLocator)
            {
                VerifyOrExit(length + cur->GetSize() <= sizeof(tlvs), OT_NOOP);
                memcpy(tlvs + length, reinterpret_cast<uint8_t *>(cur), cur->GetSize());
                length += cur->GetSize();
            }
        }
    }

    IgnoreError(SetCommissioningData(tlvs, static_cast<uint8_t>(length)));

    state = MeshCoP::StateTlv::kAccept;

exit:

    if (Get<Mle::MleRouter>().IsLeader())
    {
        SendCommissioningSetResponse(aMessage, aMessageInfo, state);
    }
}

void Leader::HandleCommissioningGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleCommissioningGet(*static_cast<Coap::Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleCommissioningGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t length = 0;
    uint16_t offset;

    SuccessOrExit(Tlv::FindTlvValueOffset(aMessage, MeshCoP::Tlv::kGet, offset, length));
    aMessage.SetOffset(offset);

exit:
    SendCommissioningGetResponse(aMessage, length, aMessageInfo);
}

void Leader::SendCommissioningGetResponse(const Coap::Message &   aRequest,
                                          uint16_t                aLength,
                                          const Ip6::MessageInfo &aMessageInfo)
{
    otError               error = OT_ERROR_NONE;
    Coap::Message *       message;
    CommissioningDataTlv *commDataTlv;
    uint8_t *             data   = NULL;
    uint8_t               length = 0;

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    commDataTlv = GetCommissioningData();

    if (commDataTlv != NULL)
    {
        data   = commDataTlv->GetValue();
        length = commDataTlv->GetLength();
    }

    VerifyOrExit(data && length, error = OT_ERROR_DROP);

    if (aLength == 0)
    {
        SuccessOrExit(error = message->Append(data, length));
    }
    else
    {
        for (uint16_t index = 0; index < aLength; index++)
        {
            uint8_t type;

            aRequest.Read(aRequest.GetOffset() + index, sizeof(type), &type);

            for (MeshCoP::Tlv *cur                                          = reinterpret_cast<MeshCoP::Tlv *>(data);
                 cur < reinterpret_cast<MeshCoP::Tlv *>(data + length); cur = cur->GetNext())
            {
                if (cur->GetType() == type)
                {
                    SuccessOrExit(error = cur->AppendTo(*message));
                    break;
                }
            }
        }
    }

    if (message->GetLength() == message->GetOffset())
    {
        // no payload, remove coap payload marker
        IgnoreError(message->SetLength(message->GetLength() - 1));
    }

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent commissioning dataset get response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void Leader::SendCommissioningSetResponse(const Coap::Message &    aRequest,
                                          const Ip6::MessageInfo & aMessageInfo,
                                          MeshCoP::StateTlv::State aState)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint8Tlv(*message, MeshCoP::Tlv::kState, static_cast<uint8_t>(aState)));

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP("sent commissioning dataset set response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
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
        matched = Mle::Mle::RouterIdMatch(aFirstRloc16, aSecondRloc16);
        break;
    }

    return matched;
}

otError Leader::Validate(const uint8_t *aTlvs, uint8_t aTlvsLength, uint16_t aRloc16)
{
    // Validate that the `aTlvs` contains well-formed TLVs, sub-TLVs,
    // and entries all matching `aRloc16` (no other entry for other
    // RLOCs and no duplicates TLVs).

    otError               error = OT_ERROR_NONE;
    const NetworkDataTlv *end   = reinterpret_cast<const NetworkDataTlv *>(aTlvs + aTlvsLength);

    for (const NetworkDataTlv *cur = reinterpret_cast<const NetworkDataTlv *>(aTlvs); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            const PrefixTlv *prefix = static_cast<const PrefixTlv *>(cur);

            VerifyOrExit(prefix->IsValid(), error = OT_ERROR_PARSE);

            // Ensure there is no duplicate Prefix TLVs with same prefix.
            VerifyOrExit(prefix == FindPrefix(prefix->GetPrefix(), prefix->GetPrefixLength(), aTlvs, aTlvsLength),
                         error = OT_ERROR_PARSE);

            SuccessOrExit(error = ValidatePrefix(*prefix, aRloc16));
            break;
        }

        case NetworkDataTlv::kTypeService:
        {
            const ServiceTlv *service = static_cast<const ServiceTlv *>(cur);

            VerifyOrExit(service->IsValid(), error = OT_ERROR_PARSE);

            // Ensure there is no duplicate Service TLV with same
            // Enterprise Number and Service Data.
            VerifyOrExit(service == FindService(service->GetEnterpriseNumber(), service->GetServiceData(),
                                                service->GetServiceDataLength(), aTlvs, aTlvsLength),
                         error = OT_ERROR_PARSE);

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

otError Leader::ValidatePrefix(const PrefixTlv &aPrefix, uint16_t aRloc16)
{
    // Validate that `aPrefix` TLV contains well-formed sub-TLVs and
    // and entries all matching `aRloc16` (no other entry for other
    // RLOCs).

    otError               error                   = OT_ERROR_PARSE;
    const NetworkDataTlv *subEnd                  = aPrefix.GetNext();
    bool                  foundTempHasRoute       = false;
    bool                  foundStableHasRoute     = false;
    bool                  foundTempBorderRouter   = false;
    bool                  foundStableBorderRouter = false;

    for (const NetworkDataTlv *subCur = aPrefix.GetSubTlvs(); subCur < subEnd; subCur = subCur->GetNext())
    {
        VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, OT_NOOP);

        switch (subCur->GetType())
        {
        case NetworkDataTlv::kTypeBorderRouter:
        {
            const BorderRouterTlv *borderRouter = static_cast<const BorderRouterTlv *>(subCur);

            // Ensure Prefix TLV contains at most one stable and one
            // temporary Border Router sub-TLV and the sub-TLVs have
            // a single entry.

            if (borderRouter->IsStable())
            {
                VerifyOrExit(!foundStableBorderRouter, OT_NOOP);
                foundStableBorderRouter = true;
            }
            else
            {
                VerifyOrExit(!foundTempBorderRouter, OT_NOOP);
                foundTempBorderRouter = true;
            }

            VerifyOrExit(borderRouter->GetFirstEntry() == borderRouter->GetLastEntry(), OT_NOOP);
            VerifyOrExit(borderRouter->GetFirstEntry()->GetRloc() == aRloc16, OT_NOOP);
            break;
        }

        case NetworkDataTlv::kTypeHasRoute:
        {
            const HasRouteTlv *hasRoute = static_cast<const HasRouteTlv *>(subCur);

            // Ensure Prefix TLV contains at most one stable and one
            // temporary Has Route sub-TLV and the sub-TLVs have a
            // single entry.

            if (hasRoute->IsStable())
            {
                VerifyOrExit(!foundStableHasRoute, OT_NOOP);
                foundStableHasRoute = true;
            }
            else
            {
                VerifyOrExit(!foundTempHasRoute, OT_NOOP);
                foundTempHasRoute = true;
            }

            VerifyOrExit(hasRoute->GetFirstEntry() == hasRoute->GetLastEntry(), OT_NOOP);
            VerifyOrExit(hasRoute->GetFirstEntry()->GetRloc() == aRloc16, OT_NOOP);
            break;
        }

        default:
            break;
        }
    }

    if (foundStableBorderRouter || foundTempBorderRouter || foundStableHasRoute || foundTempHasRoute)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

otError Leader::ValidateService(const ServiceTlv &aService, uint16_t aRloc16)
{
    // Validate that `aService` TLV contains a single well-formed
    // Server sub-TLV associated with `aRloc16`.

    otError               error       = OT_ERROR_PARSE;
    const NetworkDataTlv *subEnd      = aService.GetNext();
    bool                  foundServer = false;

    for (const NetworkDataTlv *subCur = aService.GetSubTlvs(); subCur < subEnd; subCur = subCur->GetNext())
    {
        VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, OT_NOOP);

        switch (subCur->GetType())
        {
        case NetworkDataTlv::kTypeServer:
        {
            const ServerTlv *server = static_cast<const ServerTlv *>(subCur);

            VerifyOrExit(!foundServer, OT_NOOP);
            foundServer = true;

            VerifyOrExit(server->IsValid() && server->GetServer16() == aRloc16, OT_NOOP);
            break;
        }

        default:
            break;
        }
    }

    if (foundServer)
    {
        error = OT_ERROR_NONE;
    }

exit:
    return error;
}

bool Leader::ContainsMatchingEntry(const PrefixTlv *aPrefix, bool aStable, const HasRouteEntry &aEntry)
{
    // Check whether `aPrefix` has a Has Route sub-TLV with stable
    // flag `aStable` containing a matching entry to `aEntry`.

    return (aPrefix == NULL) ? false : ContainsMatchingEntry(FindHasRoute(*aPrefix, aStable), aEntry);
}

bool Leader::ContainsMatchingEntry(const HasRouteTlv *aHasRoute, const HasRouteEntry &aEntry)
{
    // Check whether `aHasRoute` has a matching entry to `aEntry`.

    bool contains = false;

    VerifyOrExit(aHasRoute != NULL, OT_NOOP);

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

    return (aPrefix == NULL) ? false : ContainsMatchingEntry(FindBorderRouter(*aPrefix, aStable), aEntry);
}

bool Leader::ContainsMatchingEntry(const BorderRouterTlv *aBorderRouter, const BorderRouterEntry &aEntry)
{
    // Check whether `aBorderRouter` has a matching entry to `aEntry`.

    bool contains = false;

    VerifyOrExit(aBorderRouter != NULL, OT_NOOP);

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

    bool             contains = false;
    const ServerTlv *server;

    VerifyOrExit(aService != NULL, OT_NOOP);

    for (const NetworkDataTlv *start = aService->GetSubTlvs();
         (server = FindTlv<ServerTlv>(start, aService->GetNext(), aServer.IsStable())) != NULL;
         start = server->GetNext())
    {
        if (*server == aServer)
        {
            ExitNow(contains = true);
        }
    }

exit:
    return contains;
}

Leader::UpdateStatus Leader::UpdatePrefix(PrefixTlv &aPrefix)
{
    return UpdateTlv(aPrefix, aPrefix.GetSubTlvs());
}

Leader::UpdateStatus Leader::UpdateService(ServiceTlv &aService)
{
    return UpdateTlv(aService, aService.GetSubTlvs());
}

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

void Leader::RegisterNetworkData(uint16_t aRloc16, const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    otError               error = OT_ERROR_NONE;
    const NetworkDataTlv *end   = reinterpret_cast<const NetworkDataTlv *>(aTlvs + aTlvsLength);
    ChangedFlags          flags;

    VerifyOrExit(Get<RouterTable>().IsAllocated(Mle::Mle::RouterIdFromRloc16(aRloc16)), error = OT_ERROR_NO_ROUTE);

    // Validate that the `aTlvs` contains well-formed TLVs, sub-TLVs,
    // and entries all matching `aRloc16` (no other RLOCs).
    SuccessOrExit(error = Validate(aTlvs, aTlvsLength, aRloc16));

    // Remove all entries matching `aRloc16` excluding entries that are
    // present in `aTlvs`
    RemoveRloc(aRloc16, kMatchModeRloc16, aTlvs, aTlvsLength, flags);

    // Now add all new entries in `aTlvs` to Network Data.
    for (const NetworkDataTlv *cur = reinterpret_cast<const NetworkDataTlv *>(aTlvs); cur < end; cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            SuccessOrExit(error = AddPrefix(*static_cast<const PrefixTlv *>(cur), flags));
            break;

        case NetworkDataTlv::kTypeService:
            SuccessOrExit(error = AddService(*static_cast<const ServiceTlv *>(cur), flags));
            break;

        default:
            break;
        }
    }

    IncrementVersions(flags);

    otDumpDebgNetData("add done", mTlvs, mLength);

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogNoteNetData("Failed to register network data: %s", otThreadErrorToString(error));
    }
}

otError Leader::AddPrefix(const PrefixTlv &aPrefix, ChangedFlags &aChangedFlags)
{
    otError    error     = OT_ERROR_NONE;
    PrefixTlv *dstPrefix = FindPrefix(aPrefix.GetPrefix(), aPrefix.GetPrefixLength());

    if (dstPrefix == NULL)
    {
        dstPrefix = static_cast<PrefixTlv *>(AppendTlv(PrefixTlv::CalculateSize(aPrefix.GetPrefixLength())));
        VerifyOrExit(dstPrefix != NULL, error = OT_ERROR_NO_BUFS);

        dstPrefix->Init(aPrefix.GetDomainId(), aPrefix.GetPrefixLength(), aPrefix.GetPrefix());
    }

    for (const NetworkDataTlv *subCur = aPrefix.GetSubTlvs(); subCur < aPrefix.GetNext(); subCur = subCur->GetNext())
    {
        switch (subCur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            SuccessOrExit(error = AddHasRoute(*static_cast<const HasRouteTlv *>(subCur), *dstPrefix, aChangedFlags));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            SuccessOrExit(
                error = AddBorderRouter(*static_cast<const BorderRouterTlv *>(subCur), *dstPrefix, aChangedFlags));
            break;

        default:
            break;
        }
    }

exit:
    if (dstPrefix != NULL)
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

otError Leader::AddService(const ServiceTlv &aService, ChangedFlags &aChangedFlags)
{
    otError     error = OT_ERROR_NONE;
    ServiceTlv *dstService =
        FindService(aService.GetEnterpriseNumber(), aService.GetServiceData(), aService.GetServiceDataLength());
    const ServerTlv *server;

    if (dstService == NULL)
    {
        uint8_t serviceId;

        SuccessOrExit(error = AllocateServiceId(serviceId));

        dstService = static_cast<ServiceTlv *>(
            AppendTlv(ServiceTlv::CalculateSize(aService.GetEnterpriseNumber(), aService.GetServiceDataLength())));
        VerifyOrExit(dstService != NULL, error = OT_ERROR_NO_BUFS);

        dstService->Init(serviceId, aService.GetEnterpriseNumber(), aService.GetServiceData(),
                         aService.GetServiceDataLength());
    }

    server = FindTlv<ServerTlv>(aService.GetSubTlvs(), aService.GetNext());
    OT_ASSERT(server != NULL);

    SuccessOrExit(error = AddServer(*server, *dstService, aChangedFlags));

exit:
    if (dstService != NULL)
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

otError Leader::AddHasRoute(const HasRouteTlv &aHasRoute, PrefixTlv &aDstPrefix, ChangedFlags &aChangedFlags)
{
    otError              error       = OT_ERROR_NONE;
    HasRouteTlv *        dstHasRoute = FindHasRoute(aDstPrefix, aHasRoute.IsStable());
    const HasRouteEntry *entry       = aHasRoute.GetFirstEntry();

    if (dstHasRoute == NULL)
    {
        // Ensure there is space for `HasRouteTlv` and a single entry.
        VerifyOrExit(CanInsert(sizeof(HasRouteTlv) + sizeof(HasRouteEntry)), error = OT_ERROR_NO_BUFS);

        dstHasRoute = static_cast<HasRouteTlv *>(aDstPrefix.GetNext());
        Insert(dstHasRoute, sizeof(HasRouteTlv));
        aDstPrefix.IncreaseLength(sizeof(HasRouteTlv));
        dstHasRoute->Init();

        if (aHasRoute.IsStable())
        {
            dstHasRoute->SetStable();
        }
    }

    VerifyOrExit(!ContainsMatchingEntry(dstHasRoute, *entry), OT_NOOP);

    VerifyOrExit(CanInsert(sizeof(HasRouteEntry)), error = OT_ERROR_NO_BUFS);

    Insert(dstHasRoute->GetNext(), sizeof(HasRouteEntry));
    dstHasRoute->IncreaseLength(sizeof(HasRouteEntry));
    aDstPrefix.IncreaseLength(sizeof(HasRouteEntry));

    *dstHasRoute->GetLastEntry() = *entry;
    aChangedFlags.Update(*dstHasRoute);

exit:
    return error;
}

otError Leader::AddBorderRouter(const BorderRouterTlv &aBorderRouter,
                                PrefixTlv &            aDstPrefix,
                                ChangedFlags &         aChangedFlags)
{
    otError                  error           = OT_ERROR_NONE;
    BorderRouterTlv *        dstBorderRouter = FindBorderRouter(aDstPrefix, aBorderRouter.IsStable());
    ContextTlv *             dstContext      = FindContext(aDstPrefix);
    uint8_t                  contextId       = 0;
    const BorderRouterEntry *entry           = aBorderRouter.GetFirstEntry();

    if (dstContext == NULL)
    {
        // Allocate a Context ID first. This ensure that if we cannot
        // allocate, we fail and exit before potentially inserting a
        // Border Router sub-TLV.
        SuccessOrExit(error = AllocateContextId(contextId));
    }

    if (dstBorderRouter == NULL)
    {
        // Ensure there is space for `BorderRouterTlv` with a single entry
        // and a `ContextTlv` (if not already present).
        VerifyOrExit(CanInsert(sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry) +
                               ((dstContext == NULL) ? sizeof(ContextTlv) : 0)),
                     error = OT_ERROR_NO_BUFS);

        dstBorderRouter = static_cast<BorderRouterTlv *>(aDstPrefix.GetNext());
        Insert(dstBorderRouter, sizeof(BorderRouterTlv));
        aDstPrefix.IncreaseLength(sizeof(BorderRouterTlv));
        dstBorderRouter->Init();

        if (aBorderRouter.IsStable())
        {
            dstBorderRouter->SetStable();
        }
    }

    if (dstContext == NULL)
    {
        // Ensure there is space for a `ContextTlv` and a single entry.
        VerifyOrExit(CanInsert(sizeof(BorderRouterEntry) + sizeof(ContextTlv)), error = OT_ERROR_NO_BUFS);

        dstContext = static_cast<ContextTlv *>(aDstPrefix.GetNext());
        Insert(dstContext, sizeof(ContextTlv));
        aDstPrefix.IncreaseLength(sizeof(ContextTlv));
        dstContext->Init(static_cast<uint8_t>(contextId), aDstPrefix.GetPrefixLength());
    }

    if (aBorderRouter.IsStable())
    {
        dstContext->SetStable();
    }

    dstContext->SetCompress();
    StopContextReuseTimer(dstContext->GetContextId());

    VerifyOrExit(!ContainsMatchingEntry(dstBorderRouter, *entry), OT_NOOP);

    VerifyOrExit(CanInsert(sizeof(BorderRouterEntry)), error = OT_ERROR_NO_BUFS);

    Insert(dstBorderRouter->GetNext(), sizeof(BorderRouterEntry));
    dstBorderRouter->IncreaseLength(sizeof(BorderRouterEntry));
    aDstPrefix.IncreaseLength(sizeof(BorderRouterEntry));
    *dstBorderRouter->GetLastEntry() = *entry;
    aChangedFlags.Update(*dstBorderRouter);

exit:
    return error;
}

otError Leader::AddServer(const ServerTlv &aServer, ServiceTlv &aDstService, ChangedFlags &aChangedFlags)
{
    otError    error = OT_ERROR_NONE;
    ServerTlv *dstServer;
    uint8_t    tlvSize = aServer.GetSize();

    VerifyOrExit(!ContainsMatchingServer(&aDstService, aServer), OT_NOOP);

    VerifyOrExit(CanInsert(tlvSize), error = OT_ERROR_NO_BUFS);

    dstServer = static_cast<ServerTlv *>(aDstService.GetNext());
    Insert(dstServer, tlvSize);
    dstServer->Init(aServer.GetServer16(), aServer.GetServerData(), aServer.GetServerDataLength());

    if (aServer.IsStable())
    {
        dstServer->SetStable();
    }

    aDstService.IncreaseLength(tlvSize);
    aChangedFlags.Update(*dstServer);

exit:
    return error;
}

otError Leader::AllocateServiceId(uint8_t &aServiceId)
{
    otError error = OT_ERROR_NOT_FOUND;
    uint8_t serviceId;

    for (serviceId = Mle::kServiceMinId; serviceId <= Mle::kServiceMaxId; serviceId++)
    {
        if (FindServiceById(serviceId) == NULL)
        {
            aServiceId = serviceId;
            error      = OT_ERROR_NONE;
            otLogInfoNetData("Allocated Service ID = %d", serviceId);
            break;
        }
    }

    return error;
}

const ServiceTlv *Leader::FindServiceById(uint8_t aServiceId) const
{
    const NetworkDataTlv *start = GetTlvsStart();
    const ServiceTlv *    service;

    while ((service = FindTlv<ServiceTlv>(start, GetTlvsEnd())) != NULL)
    {
        if (service->GetServiceId() == aServiceId)
        {
            ExitNow();
        }

        start = service->GetNext();
    }

exit:
    return service;
}

otError Leader::AllocateContextId(uint8_t &aContextId)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (uint8_t contextId = kMinContextId; contextId < kMinContextId + kNumContextIds; contextId++)
    {
        if ((mContextUsed & (1 << contextId)) == 0)
        {
            mContextUsed |= (1 << contextId);
            aContextId = contextId;
            error      = OT_ERROR_NONE;
            otLogInfoNetData("Allocated Context ID = %d", contextId);
            break;
        }
    }

    return error;
}

void Leader::FreeContextId(uint8_t aContextId)
{
    otLogInfoNetData("Free Context Id = %d", aContextId);
    RemoveContext(aContextId);
    mContextUsed &= ~(1 << aContextId);
    IncrementVersions(/* aIncludeStable */ true);
}

void Leader::StartContextReuseTimer(uint8_t aContextId)
{
    mContextLastUsed[aContextId - kMinContextId] = TimerMilli::GetNow();

    if (mContextLastUsed[aContextId - kMinContextId].GetValue() == 0)
    {
        mContextLastUsed[aContextId - kMinContextId].SetValue(1);
    }

    mTimer.Start(kStateUpdatePeriod);
}

void Leader::StopContextReuseTimer(uint8_t aContextId)
{
    mContextLastUsed[aContextId - kMinContextId].SetValue(0);
}

void Leader::RemoveRloc(uint16_t aRloc16, MatchMode aMatchMode, ChangedFlags &aChangedFlags)
{
    RemoveRloc(aRloc16, aMatchMode, NULL, 0, aChangedFlags);
}

void Leader::RemoveRloc(uint16_t       aRloc16,
                        MatchMode      aMatchMode,
                        const uint8_t *aExcludeTlvs,
                        uint8_t        aExcludeTlvsLength,
                        ChangedFlags & aChangedFlags)
{
    // Remove entries from Network Data matching `aRloc16` (using
    // `aMatchMode` to determine the match) but exclude any entries
    // that are present in `aExcludeTlvs`. As entries are removed
    // update `aChangedFlags` to indicate if Network Data (stable or
    // not) got changed.

    NetworkDataTlv *cur = GetTlvsStart();

    while (cur < GetTlvsEnd())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            PrefixTlv *      prefix = static_cast<PrefixTlv *>(cur);
            const PrefixTlv *excludePrefix =
                FindPrefix(prefix->GetPrefix(), prefix->GetPrefixLength(), aExcludeTlvs, aExcludeTlvsLength);

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
            ServiceTlv *      service = static_cast<ServiceTlv *>(cur);
            const ServiceTlv *excludeService =
                FindService(service->GetEnterpriseNumber(), service->GetServiceData(), service->GetServiceDataLength(),
                            aExcludeTlvs, aExcludeTlvsLength);

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

    otDumpDebgNetData("remove done", mTlvs, mLength);
}

void Leader::RemoveRlocInPrefix(PrefixTlv &      aPrefix,
                                uint16_t         aRloc16,
                                MatchMode        aMatchMode,
                                const PrefixTlv *aExcludePrefix,
                                ChangedFlags &   aChangedFlags)
{
    // Remove entries in `aPrefix` TLV matching the given `aRloc16`
    // excluding any entries that are present in `aExcludePrefix`.

    NetworkDataTlv *cur = aPrefix.GetSubTlvs();
    ContextTlv *    context;

    while (cur < aPrefix.GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            RemoveRlocInHasRoute(aPrefix, *static_cast<HasRouteTlv *>(cur), aRloc16, aMatchMode, aExcludePrefix,
                                 aChangedFlags);

            if (cur->GetLength() == 0)
            {
                aPrefix.DecreaseLength(sizeof(HasRouteTlv));
                RemoveTlv(cur);
                continue;
            }

            break;

        case NetworkDataTlv::kTypeBorderRouter:
            RemoveRlocInBorderRouter(aPrefix, *static_cast<BorderRouterTlv *>(cur), aRloc16, aMatchMode, aExcludePrefix,
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

    if ((context = FindContext(aPrefix)) != NULL)
    {
        if (aPrefix.GetSubTlvsLength() == sizeof(ContextTlv))
        {
            context->ClearCompress();
            StartContextReuseTimer(context->GetContextId());
        }
        else
        {
            context->SetCompress();
            StopContextReuseTimer(context->GetContextId());
        }
    }
}

void Leader::RemoveRlocInService(ServiceTlv &      aService,
                                 uint16_t          aRloc16,
                                 MatchMode         aMatchMode,
                                 const ServiceTlv *aExcludeService,
                                 ChangedFlags &    aChangedFlags)
{
    // Remove entries in `aService` TLV matching the given `aRloc16`
    // excluding any entries that are present in `aExcludeService`.

    NetworkDataTlv *start = aService.GetSubTlvs();
    ServerTlv *     server;

    while ((server = FindTlv<ServerTlv>(start, aService.GetNext())) != NULL)
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

void Leader::RemoveRlocInHasRoute(PrefixTlv &      aPrefix,
                                  HasRouteTlv &    aHasRoute,
                                  uint16_t         aRloc16,
                                  MatchMode        aMatchMode,
                                  const PrefixTlv *aExcludePrefix,
                                  ChangedFlags &   aChangedFlags)
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

void Leader::RemoveRlocInBorderRouter(PrefixTlv &      aPrefix,
                                      BorderRouterTlv &aBorderRouter,
                                      uint16_t         aRloc16,
                                      MatchMode        aMatchMode,
                                      const PrefixTlv *aExcludePrefix,
                                      ChangedFlags &   aChangedFlags)
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
    PrefixTlv *     prefix;

    while ((prefix = FindTlv<PrefixTlv>(start, GetTlvsEnd())) != NULL)
    {
        RemoveContext(*prefix, aContextId);

        if (UpdatePrefix(*prefix) == kTlvRemoved)
        {
            // Do not update `start` when TLV is removed.
            continue;
        }

        start = prefix->GetNext();
    }
}

void Leader::RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId)
{
    NetworkDataTlv *start = aPrefix.GetSubTlvs();
    ContextTlv *    context;

    while ((context = FindTlv<ContextTlv>(start, aPrefix.GetNext())) != NULL)
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

void Leader::UpdateContextsAfterReset(void)
{
    NetworkDataTlv *start;
    PrefixTlv *     prefix;

    for (start = GetTlvsStart(); (prefix = FindTlv<PrefixTlv>(start, GetTlvsEnd())) != NULL; start = prefix->GetNext())
    {
        ContextTlv *context = FindContext(*prefix);

        if (context == NULL)
        {
            continue;
        }

        mContextUsed |= 1 << context->GetContextId();

        if (context->IsCompress())
        {
            StopContextReuseTimer(context->GetContextId());
        }
        else
        {
            StartContextReuseTimer(context->GetContextId());
        }
    }
}

void Leader::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<Leader>().HandleTimer();
}

void Leader::HandleTimer(void)
{
    bool contextsWaiting = false;

    for (uint8_t i = 0; i < kNumContextIds; i++)
    {
        if (mContextLastUsed[i].GetValue() == 0)
        {
            continue;
        }

        if (TimerMilli::GetNow() - mContextLastUsed[i] >= Time::SecToMsec(mContextIdReuseDelay))
        {
            FreeContextId(kMinContextId + i);
        }
        else
        {
            contextsWaiting = true;
        }
    }

    if (contextsWaiting)
    {
        mTimer.Start(kStateUpdatePeriod);
    }
}

otError Leader::RemoveStaleChildEntries(Coap::ResponseHandler aHandler, void *aContext)
{
    otError  error    = OT_ERROR_NOT_FOUND;
    Iterator iterator = kIteratorInit;
    uint16_t rloc16;

    VerifyOrExit(Get<Mle::MleRouter>().IsRouterOrLeader(), OT_NOOP);

    while (GetNextServer(iterator, rloc16) == OT_ERROR_NONE)
    {
        if (!Mle::Mle::IsActiveRouter(rloc16) && Mle::Mle::RouterIdMatch(Get<Mle::MleRouter>().GetRloc16(), rloc16) &&
            Get<ChildTable>().FindChild(rloc16, Child::kInStateValid) == NULL)
        {
            // In Thread 1.1 Specification 5.15.6.1, only one RLOC16 TLV entry may appear in SRV_DATA.ntf.
            error = NetworkData::SendServerDataNotification(rloc16, aHandler, aContext);
            ExitNow();
        }
    }

exit:
    return error;
}

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_FTD
