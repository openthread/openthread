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
    bool rlocIn;
    bool rlocStable;

    RlocLookup(aRloc16, rlocIn, rlocStable, mTlvs, mLength, aMatchMode);
    VerifyOrExit(rlocIn);
    RemoveRloc(aRloc16, aMatchMode);
    IncrementVersions(rlocStable);

exit:
    return;
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

    VerifyOrExit(aMessageInfo.GetPeerAddr().IsRoutingLocator());

    switch (Tlv::ReadUint16Tlv(aMessage, ThreadTlv::kRloc16, rloc16))
    {
    case OT_ERROR_NONE:
        RemoveBorderRouter(rloc16, kMatchModeRloc16);
        break;
    case OT_ERROR_NOT_FOUND:
        break;
    default:
        ExitNow();
    }

    if (ThreadTlv::GetTlv(aMessage, ThreadTlv::kThreadNetworkData, sizeof(networkData), networkData) == OT_ERROR_NONE)
    {
        VerifyOrExit(networkData.IsValid());
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

    VerifyOrExit(length <= sizeof(tlvs));
    VerifyOrExit(Get<Mle::MleRouter>().IsLeader());

    aMessage.Read(offset, length, tlvs);

    // Session Id and Border Router Locator MUST NOT be set, but accept including unexpected or
    // unknown TLV as long as there is at least one valid TLV.
    cur = reinterpret_cast<MeshCoP::Tlv *>(tlvs);
    end = reinterpret_cast<MeshCoP::Tlv *>(tlvs + length);

    while (cur < end)
    {
        MeshCoP::Tlv::Type type;

        VerifyOrExit(((cur + 1) <= end) && !cur->IsExtended() && (cur->GetNext() <= end));

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

            VerifyOrExit(tlv->IsValid());
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
    VerifyOrExit(hasSessionId);

    // verify whether or not MGMT_COMM_SET.req includes at least one valid TLV
    VerifyOrExit(hasValidTlv);

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
                             static_cast<MeshCoP::CommissionerSessionIdTlv *>(cur)->GetCommissionerSessionId());
            }
            else if (cur->GetType() == MeshCoP::Tlv::kBorderAgentLocator)
            {
                VerifyOrExit(length + cur->GetSize() <= sizeof(tlvs));
                memcpy(tlvs + length, reinterpret_cast<uint8_t *>(cur), cur->GetSize());
                length += cur->GetSize();
            }
        }
    }

    SetCommissioningData(tlvs, static_cast<uint8_t>(length));

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

    SuccessOrExit(Tlv::GetValueOffset(aMessage, MeshCoP::Tlv::kGet, offset, length));
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
        message->SetLength(message->GetLength() - 1);
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

otError Leader::RlocLookup(uint16_t       aRloc16,
                           bool &         aIn,
                           bool &         aStable,
                           const uint8_t *aTlvs,
                           uint8_t        aTlvsLength,
                           MatchMode      aMatchMode,
                           bool           aAllowOtherEntries)
{
    otError               error = OT_ERROR_NONE;
    const NetworkDataTlv *end   = reinterpret_cast<const NetworkDataTlv *>(aTlvs + aTlvsLength);

    aIn     = false;
    aStable = false;

    for (const NetworkDataTlv *cur = reinterpret_cast<const NetworkDataTlv *>(aTlvs); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            const PrefixTlv *     prefix = static_cast<const PrefixTlv *>(cur);
            const NetworkDataTlv *subEnd;

            VerifyOrExit(prefix->IsValid(), error = OT_ERROR_PARSE);

            subEnd = prefix->GetNext();

            for (const NetworkDataTlv *subCur = prefix->GetSubTlvs(); subCur < subEnd; subCur = subCur->GetNext())
            {
                VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, error = OT_ERROR_PARSE);

                switch (subCur->GetType())
                {
                case NetworkDataTlv::kTypeBorderRouter:
                {
                    const BorderRouterTlv *borderRouter = static_cast<const BorderRouterTlv *>(subCur);

                    for (const BorderRouterEntry *borderRouterEntry = borderRouter->GetFirstEntry();
                         borderRouterEntry <= borderRouter->GetLastEntry();
                         borderRouterEntry = borderRouterEntry->GetNext())
                    {
                        if (RlocMatch(borderRouterEntry->GetRloc(), aRloc16, aMatchMode))
                        {
                            aIn = true;

                            if (borderRouter->IsStable())
                            {
                                aStable = true;
                            }
                        }
                        else
                        {
                            VerifyOrExit(aAllowOtherEntries, error = OT_ERROR_FAILED);
                        }
                    }

                    break;
                }

                case NetworkDataTlv::kTypeHasRoute:
                {
                    const HasRouteTlv *hasRoute = static_cast<const HasRouteTlv *>(subCur);

                    for (const HasRouteEntry *hasRouteEntry                       = hasRoute->GetFirstEntry();
                         hasRouteEntry <= hasRoute->GetLastEntry(); hasRouteEntry = hasRouteEntry->GetNext())
                    {
                        if (RlocMatch(hasRouteEntry->GetRloc(), aRloc16, aMatchMode))
                        {
                            aIn = true;

                            if (hasRoute->IsStable())
                            {
                                aStable = true;
                            }
                        }
                        else
                        {
                            VerifyOrExit(aAllowOtherEntries, error = OT_ERROR_FAILED);
                        }
                    }

                    break;
                }

                default:
                    break;
                }

                if (aIn && aStable && aAllowOtherEntries)
                {
                    ExitNow();
                }
            }
        }
        break;

        case NetworkDataTlv::kTypeService:
        {
            const ServiceTlv *    service = static_cast<const ServiceTlv *>(cur);
            const NetworkDataTlv *subEnd;

            VerifyOrExit(service->IsValid(), error = OT_ERROR_PARSE);

            subEnd = service->GetNext();

            for (const NetworkDataTlv *subCur = service->GetSubTlvs(); subCur < subEnd; subCur = subCur->GetNext())
            {
                VerifyOrExit((subCur + 1) <= subEnd && subCur->GetNext() <= subEnd, error = OT_ERROR_PARSE);

                switch (subCur->GetType())
                {
                case NetworkDataTlv::kTypeServer:
                {
                    const ServerTlv *server = static_cast<const ServerTlv *>(subCur);

                    VerifyOrExit(server->IsValid(), error = OT_ERROR_PARSE);

                    if (RlocMatch(server->GetServer16(), aRloc16, aMatchMode))
                    {
                        aIn = true;

                        if (server->IsStable())
                        {
                            aStable = true;
                        }
                    }
                    else
                    {
                        VerifyOrExit(aAllowOtherEntries, error = OT_ERROR_FAILED);
                    }

                    break;
                }

                default:
                    break;
                }

                if (aIn && aStable && aAllowOtherEntries)
                {
                    ExitNow();
                }
            }

            break;
        }

        default:
            break;
        }
    }

exit:
    return error;
}

bool Leader::IsStableUpdated(const uint8_t *aTlvs,
                             uint8_t        aTlvsLength,
                             const uint8_t *aTlvsBase,
                             uint8_t        aTlvsBaseLength)
{
    bool                  rval = false;
    const NetworkDataTlv *end  = reinterpret_cast<const NetworkDataTlv *>(aTlvs + aTlvsLength);

    for (const NetworkDataTlv *cur = reinterpret_cast<const NetworkDataTlv *>(aTlvs); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end);

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            const PrefixTlv *      prefix       = static_cast<const PrefixTlv *>(cur);
            const ContextTlv *     context      = FindContext(*prefix);
            const BorderRouterTlv *borderRouter = FindBorderRouter(*prefix, true);
            const HasRouteTlv *    hasRoute     = FindHasRoute(*prefix, true);

            if (cur->IsStable() && (!context || borderRouter))
            {
                const PrefixTlv *prefixBase =
                    FindPrefix(prefix->GetPrefix(), prefix->GetPrefixLength(), aTlvsBase, aTlvsBaseLength);

                if (!prefixBase)
                {
                    ExitNow(rval = true);
                }

                if (borderRouter)
                {
                    const BorderRouterTlv *borderRouterBase = FindBorderRouter(*prefixBase, true);

                    if (!borderRouterBase || (borderRouter->GetLength() != borderRouterBase->GetLength()) ||
                        (memcmp(borderRouter, borderRouterBase, borderRouter->GetLength()) != 0))
                    {
                        ExitNow(rval = true);
                    }
                }

                if (hasRoute)
                {
                    const HasRouteTlv *hasRouteBase = FindHasRoute(*prefixBase, true);

                    if (!hasRouteBase || (hasRoute->GetLength() != hasRouteBase->GetLength()) ||
                        (memcmp(hasRoute, hasRouteBase, hasRoute->GetLength()) != 0))
                    {
                        ExitNow(rval = true);
                    }
                }
            }

            break;
        }

        case NetworkDataTlv::kTypeService:
        {
            const ServiceTlv *service = static_cast<const ServiceTlv *>(cur);

            if (cur->IsStable())
            {
                const NetworkDataTlv *curInner;
                const NetworkDataTlv *endInner;

                const ServiceTlv *serviceBase =
                    FindService(service->GetEnterpriseNumber(), service->GetServiceData(),
                                service->GetServiceDataLength(), aTlvsBase, aTlvsBaseLength);

                if (!serviceBase || !serviceBase->IsStable())
                {
                    ExitNow(rval = true);
                }

                curInner = service->GetSubTlvs();
                endInner = service->GetNext();

                while (curInner < endInner)
                {
                    VerifyOrExit((curInner + 1) <= endInner && curInner->GetNext() <= endInner);

                    if (curInner->IsStable())
                    {
                        switch (curInner->GetType())
                        {
                        case NetworkDataTlv::kTypeServer:
                        {
                            bool             foundInBase = false;
                            const ServerTlv *server      = static_cast<const ServerTlv *>(curInner);

                            const NetworkDataTlv *curServerBase = serviceBase->GetSubTlvs();
                            const NetworkDataTlv *endServerBase = serviceBase->GetNext();

                            while (curServerBase < endServerBase)
                            {
                                const ServerTlv *serverBase = static_cast<const ServerTlv *>(curServerBase);

                                VerifyOrExit((curServerBase + 1) <= endServerBase &&
                                             curServerBase->GetNext() <= endServerBase);

                                if (curServerBase->IsStable() && (server->GetServer16() == serverBase->GetServer16()) &&
                                    (server->GetServerDataLength() == serverBase->GetServerDataLength()) &&
                                    (memcmp(server->GetServerData(), serverBase->GetServerData(),
                                            server->GetServerDataLength()) == 0))
                                {
                                    foundInBase = true;
                                    break;
                                }

                                curServerBase = curServerBase->GetNext();
                            }

                            if (!foundInBase)
                            {
                                ExitNow(rval = true);
                            }

                            break;
                        }

                        default:
                            break;
                        }
                    }

                    curInner = curInner->GetNext();
                }
            }

            break;
        }

        default:
            break;
        }
    }

exit:
    return rval;
}

otError Leader::RegisterNetworkData(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength)
{
    otError error = OT_ERROR_NONE;
    bool    rlocIn;
    bool    rlocStable;
    bool    unused;
    uint8_t oldTlvs[NetworkData::kMaxSize];
    uint8_t oldTlvsLength = NetworkData::kMaxSize;

    VerifyOrExit(Get<RouterTable>().IsAllocated(Mle::Mle::RouterIdFromRloc16(aRloc16)), error = OT_ERROR_NO_ROUTE);

    // Verify that `aTlvs` only contains entries matching `aRloc16`.
    SuccessOrExit(error = RlocLookup(aRloc16, rlocIn, rlocStable, aTlvs, aTlvsLength, kMatchModeRloc16,
                                     /* aAllowOtherEntries */ false));

    RlocLookup(aRloc16, rlocIn, unused, mTlvs, mLength, kMatchModeRloc16);

    if (rlocIn)
    {
        if (IsStableUpdated(aTlvs, aTlvsLength, mTlvs, mLength) || IsStableUpdated(mTlvs, mLength, aTlvs, aTlvsLength))
        {
            rlocStable = true;
        }

        // Store old Service IDs for given rloc16, so updates to server will reuse the same Service ID.
        SuccessOrExit(error = GetNetworkData(false, oldTlvs, oldTlvsLength));

        RemoveRloc(aRloc16, kMatchModeRloc16);
    }
    else
    {
        // No old data to be preserved, lets avoid memcpy() & FindService calls.
        oldTlvsLength = 0;
    }

    SuccessOrExit(error = AddNetworkData(aTlvs, aTlvsLength, oldTlvs, oldTlvsLength));
    IncrementVersions(rlocStable);

exit:
    return error;
}

otError Leader::AddNetworkData(uint8_t *aTlvs, uint8_t aTlvsLength, uint8_t *aOldTlvs, uint8_t aOldTlvsLength)
{
    otError         error = OT_ERROR_NONE;
    NetworkDataTlv *end   = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aTlvs); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            SuccessOrExit(error = AddPrefix(*static_cast<PrefixTlv *>(cur)));
            otDumpDebgNetData("add prefix done", mTlvs, mLength);
            break;

        case NetworkDataTlv::kTypeService:
            SuccessOrExit(error = AddService(*static_cast<ServiceTlv *>(cur), aOldTlvs, aOldTlvsLength));
            otDumpDebgNetData("add service done", mTlvs, mLength);
            break;

        default:
            break;
        }
    }

    otDumpDebgNetData("add done", mTlvs, mLength);

exit:
    return error;
}

otError Leader::AddPrefix(PrefixTlv &aPrefix)
{
    otError         error = OT_ERROR_NONE;
    NetworkDataTlv *end;

    VerifyOrExit(aPrefix.IsValid(), error = OT_ERROR_PARSE);
    end = aPrefix.GetNext();

    for (NetworkDataTlv *cur = aPrefix.GetSubTlvs(); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            SuccessOrExit(error = AddHasRoute(aPrefix, *static_cast<HasRouteTlv *>(cur)));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            SuccessOrExit(error = AddBorderRouter(aPrefix, *static_cast<BorderRouterTlv *>(cur)));
            break;

        default:
            break;
        }
    }

exit:
    return error;
}

otError Leader::AddService(ServiceTlv &aService, uint8_t *aOldTlvs, uint8_t aOldTlvsLength)
{
    otError         error = OT_ERROR_NONE;
    NetworkDataTlv *end;

    VerifyOrExit(aService.IsValid(), error = OT_ERROR_PARSE);
    end = aService.GetNext();

    for (NetworkDataTlv *cur = aService.GetSubTlvs(); cur < end; cur = cur->GetNext())
    {
        VerifyOrExit((cur + 1) <= end && cur->GetNext() <= end, error = OT_ERROR_PARSE);

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeServer:
            SuccessOrExit(error = AddServer(aService, *static_cast<ServerTlv *>(cur), aOldTlvs, aOldTlvsLength));
            break;

        default:
            break;
        }
    }

exit:
    return error;
}

otError Leader::AddHasRoute(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute)
{
    otError      error        = OT_ERROR_NONE;
    PrefixTlv *  dstPrefix    = NULL;
    HasRouteTlv *dstHasRoute  = NULL;
    uint16_t     appendLength = 0;

    VerifyOrExit(aHasRoute.GetNumEntries() > 0, error = OT_ERROR_PARSE);

    if ((dstPrefix = FindPrefix(aPrefix.GetPrefix(), aPrefix.GetPrefixLength())) != NULL)
    {
        dstHasRoute = FindHasRoute(*dstPrefix, aHasRoute.IsStable());
    }

    if (dstPrefix == NULL)
    {
        appendLength += sizeof(PrefixTlv) + BitVectorBytes(aPrefix.GetPrefixLength());
    }

    if (dstHasRoute == NULL)
    {
        appendLength += sizeof(HasRouteTlv);
    }

    appendLength += sizeof(HasRouteEntry);

    VerifyOrExit(mLength + appendLength <= sizeof(mTlvs), error = OT_ERROR_NO_BUFS);

    if (dstPrefix == NULL)
    {
        dstPrefix = static_cast<PrefixTlv *>(AppendTlv(sizeof(PrefixTlv) + BitVectorBytes(aPrefix.GetPrefixLength())));
        dstPrefix->Init(aPrefix.GetDomainId(), aPrefix.GetPrefixLength(), aPrefix.GetPrefix());
    }

    if (aHasRoute.IsStable())
    {
        dstPrefix->SetStable();
    }

    if (dstHasRoute == NULL)
    {
        dstHasRoute = static_cast<HasRouteTlv *>(dstPrefix->GetNext());
        Insert(dstHasRoute, sizeof(HasRouteTlv));
        dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(HasRouteTlv));
        dstHasRoute->Init();

        if (aHasRoute.IsStable())
        {
            dstHasRoute->SetStable();
        }
    }

    Insert(dstHasRoute->GetNext(), sizeof(HasRouteEntry));
    dstHasRoute->SetLength(dstHasRoute->GetLength() + sizeof(HasRouteEntry));
    dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(HasRouteEntry));
    memcpy(dstHasRoute->GetEntry(dstHasRoute->GetNumEntries() - 1), aHasRoute.GetEntry(0), sizeof(HasRouteEntry));

exit:
    return error;
}

otError Leader::AddServer(ServiceTlv &aService, ServerTlv &aServer, uint8_t *aOldTlvs, uint8_t aOldTlvsLength)
{
    otError     error          = OT_ERROR_NONE;
    ServiceTlv *dstService     = NULL;
    ServiceTlv *oldService     = NULL;
    ServerTlv * dstServer      = NULL;
    uint16_t    appendLength   = 0;
    uint8_t     serviceId      = 0;
    uint16_t    serviceTlvSize = ServiceTlv::GetSize(aService.GetEnterpriseNumber(), aService.GetServiceDataLength());

    dstService =
        FindService(aService.GetEnterpriseNumber(), aService.GetServiceData(), aService.GetServiceDataLength());

    if (dstService == NULL)
    {
        appendLength += serviceTlvSize;
    }

    appendLength += sizeof(ServerTlv) + aServer.GetServerDataLength();

    VerifyOrExit(mLength + appendLength <= sizeof(mTlvs), error = OT_ERROR_NO_BUFS);

    if (dstService == NULL)
    {
        // Try to preserve old Service ID, if existing
        oldService = FindService(aService.GetEnterpriseNumber(), aService.GetServiceData(),
                                 aService.GetServiceDataLength(), aOldTlvs, aOldTlvsLength);

        if (oldService != NULL)
        {
            // The same service is not found in current data, but was in old data. So, it had to be just removed by
            // RemoveRloc() Lets use the same ServiceId
            serviceId = oldService->GetServiceId();
        }
        else
        {
            uint8_t i;

            // This seems like completely new service. Lets try to find new ServiceId for it. If all are taken, error
            // out. Since we call FindServiceById() on mTlv, we need to execute this before Insert() call, otherwise
            // we'll find uninitialized service as well.
            for (i = Mle::kServiceMinId; i <= Mle::kServiceMaxId; i++)
            {
                if (FindServiceById(i) == NULL)
                {
                    serviceId = i;
                    break;
                }
            }

            otLogInfoNetData("Allocated Service ID = %d", i);

            VerifyOrExit(i <= Mle::kServiceMaxId, error = OT_ERROR_NO_BUFS);
        }

        dstService = static_cast<ServiceTlv *>(AppendTlv(static_cast<uint8_t>(serviceTlvSize)));

        dstService->Init(serviceId, aService.GetEnterpriseNumber(), aService.GetServiceData(),
                         aService.GetServiceDataLength());
    }

    dstServer = static_cast<ServerTlv *>(dstService->GetNext());

    Insert(dstServer, sizeof(ServerTlv) + aServer.GetServerDataLength());
    dstServer->Init();
    dstServer->SetServer16(aServer.GetServer16());
    dstServer->SetServerData(aServer.GetServerData(), aServer.GetServerDataLength());

    if (aServer.IsStable())
    {
        dstService->SetStable();
        dstServer->SetStable();
    }

    dstService->SetLength(dstService->GetLength() + sizeof(ServerTlv) + aServer.GetServerDataLength());

exit:
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

otError Leader::AddBorderRouter(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter)
{
    otError          error           = OT_ERROR_NONE;
    PrefixTlv *      dstPrefix       = NULL;
    ContextTlv *     dstContext      = NULL;
    BorderRouterTlv *dstBorderRouter = NULL;
    int              contextId       = -1;
    uint16_t         appendLength    = 0;

    VerifyOrExit(aBorderRouter.GetNumEntries() > 0, error = OT_ERROR_PARSE);

    if ((dstPrefix = FindPrefix(aPrefix.GetPrefix(), aPrefix.GetPrefixLength())) != NULL)
    {
        dstContext      = FindContext(*dstPrefix);
        dstBorderRouter = FindBorderRouter(*dstPrefix, aBorderRouter.IsStable());
    }

    if (dstPrefix == NULL)
    {
        appendLength += sizeof(PrefixTlv) + BitVectorBytes(aPrefix.GetPrefixLength());
    }

    if (dstContext == NULL)
    {
        appendLength += sizeof(ContextTlv);
    }

    if (dstBorderRouter == NULL)
    {
        appendLength += sizeof(BorderRouterTlv);
    }

    appendLength += sizeof(BorderRouterEntry);

    VerifyOrExit(mLength + appendLength <= sizeof(mTlvs), error = OT_ERROR_NO_BUFS);

    if (dstContext == NULL)
    {
        contextId = AllocateContext();
        VerifyOrExit(contextId >= 0, error = OT_ERROR_NO_BUFS);
    }

    if (dstPrefix == NULL)
    {
        dstPrefix = static_cast<PrefixTlv *>(AppendTlv(sizeof(PrefixTlv) + BitVectorBytes(aPrefix.GetPrefixLength())));
        dstPrefix->Init(aPrefix.GetDomainId(), aPrefix.GetPrefixLength(), aPrefix.GetPrefix());
    }

    if (dstContext == NULL)
    {
        dstContext = static_cast<ContextTlv *>(dstPrefix->GetNext());
        Insert(dstContext, sizeof(ContextTlv));
        dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(ContextTlv));
        dstContext->Init();
        dstContext->SetCompress();
        dstContext->SetContextId(static_cast<uint8_t>(contextId));
        dstContext->SetContextLength(aPrefix.GetPrefixLength());
    }

    dstContext->SetCompress();
    StopContextReuseTimer(dstContext->GetContextId());

    if (dstBorderRouter == NULL)
    {
        dstBorderRouter = static_cast<BorderRouterTlv *>(dstPrefix->GetNext());
        Insert(dstBorderRouter, sizeof(BorderRouterTlv));
        dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(BorderRouterTlv));
        dstBorderRouter->Init();
    }

    Insert(dstBorderRouter->GetNext(), sizeof(BorderRouterEntry));
    dstBorderRouter->SetLength(dstBorderRouter->GetLength() + sizeof(BorderRouterEntry));
    dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(BorderRouterEntry));
    memcpy(dstBorderRouter->GetEntry(dstBorderRouter->GetNumEntries() - 1), aBorderRouter.GetEntry(0),
           sizeof(BorderRouterEntry));

    if (aBorderRouter.IsStable())
    {
        dstPrefix->SetStable();
        dstContext->SetStable();
        dstBorderRouter->SetStable();
    }

exit:
    return error;
}

int Leader::AllocateContext(void)
{
    int rval = -1;

    for (int i = kMinContextId; i < kMinContextId + kNumContextIds; i++)
    {
        if ((mContextUsed & (1 << i)) == 0)
        {
            mContextUsed |= 1 << i;
            rval = i;
            otLogInfoNetData("Allocated Context ID = %d", rval);
            ExitNow();
        }
    }

exit:
    return rval;
}

void Leader::FreeContext(uint8_t aContextId)
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

void Leader::RemoveRloc(uint16_t aRloc16, MatchMode aMatchMode)
{
    NetworkDataTlv *cur = GetTlvsStart();

    while (cur < GetTlvsEnd())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            PrefixTlv *prefix = static_cast<PrefixTlv *>(cur);

            RemoveRloc(*prefix, aRloc16, aMatchMode);

            if (prefix->GetSubTlvsLength() == 0)
            {
                RemoveTlv(prefix);
                continue;
            }

            otDumpDebgNetData("remove prefix done", mTlvs, mLength);
            break;
        }

        case NetworkDataTlv::kTypeService:
        {
            ServiceTlv *service = static_cast<ServiceTlv *>(cur);

            RemoveRloc(*service, aRloc16, aMatchMode);

            if (service->GetSubTlvsLength() == 0)
            {
                RemoveTlv(service);
                continue;
            }

            otDumpDebgNetData("remove service done", mTlvs, mLength);

            break;
        }

        default:
            break;
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData("remove done", mTlvs, mLength);
}

void Leader::RemoveRloc(PrefixTlv &aPrefix, uint16_t aRloc16, MatchMode aMatchMode)
{
    NetworkDataTlv *cur = aPrefix.GetSubTlvs();
    ContextTlv *    context;

    while (cur < aPrefix.GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            RemoveRloc(aPrefix, *static_cast<HasRouteTlv *>(cur), aRloc16, aMatchMode);

            // remove has route tlv if empty
            if (cur->GetLength() == 0)
            {
                aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - sizeof(HasRouteTlv));
                RemoveTlv(cur);
                continue;
            }

            break;

        case NetworkDataTlv::kTypeBorderRouter:
            RemoveRloc(aPrefix, *static_cast<BorderRouterTlv *>(cur), aRloc16, aMatchMode);

            // remove border router tlv if empty
            if (cur->GetLength() == 0)
            {
                aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - sizeof(BorderRouterTlv));
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

void Leader::RemoveRloc(ServiceTlv &aService, uint16_t aRloc16, MatchMode aMatchMode)
{
    NetworkDataTlv *start = aService.GetSubTlvs();
    ServerTlv *     server;

    while ((server = FindTlv<ServerTlv>(start, aService.GetNext())) != NULL)
    {
        if (RlocMatch(server->GetServer16(), aRloc16, aMatchMode))
        {
            uint8_t subTlvSize = server->GetSize();
            RemoveTlv(server);
            aService.SetSubTlvsLength(aService.GetSubTlvsLength() - subTlvSize);
            continue;
        }

        start = server->GetNext();
    }
}

void Leader::RemoveRloc(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute, uint16_t aRloc16, MatchMode aMatchMode)
{
    HasRouteEntry *entry = aHasRoute.GetFirstEntry();

    while (entry <= aHasRoute.GetLastEntry())
    {
        if (RlocMatch(entry->GetRloc(), aRloc16, aMatchMode))
        {
            aHasRoute.SetLength(aHasRoute.GetLength() - sizeof(HasRouteEntry));
            aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - sizeof(HasRouteEntry));
            Remove(entry, sizeof(HasRouteEntry));
            continue;
        }

        entry = entry->GetNext();
    }
}

void Leader::RemoveRloc(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter, uint16_t aRloc16, MatchMode aMatchMode)
{
    BorderRouterEntry *entry = aBorderRouter.GetFirstEntry();

    while (entry <= aBorderRouter.GetLastEntry())
    {
        if (RlocMatch(entry->GetRloc(), aRloc16, aMatchMode))
        {
            aBorderRouter.SetLength(aBorderRouter.GetLength() - sizeof(BorderRouterEntry));
            aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - sizeof(BorderRouterEntry));
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

        if (prefix->GetSubTlvsLength() == 0)
        {
            RemoveTlv(prefix);
            continue;
        }

        start = prefix->GetNext();
    }

    otDumpDebgNetData("remove done", mTlvs, mLength);
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
            aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - subTlvSize);
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
            FreeContext(kMinContextId + i);
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

    VerifyOrExit(Get<Mle::MleRouter>().IsRouterOrLeader());

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
