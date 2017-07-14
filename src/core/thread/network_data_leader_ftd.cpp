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


#define WPP_NAME "network_data_leader_ftd.tmh"
#include <openthread/config.h>

#include "network_data_leader.hpp"
#include <openthread/platform/random.h>

#include "openthread-instance.h"
#include "coap/coap_header.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/timer.hpp"
#include "mac/mac_frame.hpp"
#include "meshcop/meshcop.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace NetworkData {

Leader::Leader(ThreadNetif &aThreadNetif):
    LeaderBase(aThreadNetif),
    mTimer(aThreadNetif.GetInstance(), &Leader::HandleTimer, this),
    mServerData(OT_URI_PATH_SERVER_DATA, &Leader::HandleServerData, this),
    mCommissioningDataGet(OT_URI_PATH_COMMISSIONER_GET, &Leader::HandleCommissioningGet, this),
    mCommissioningDataSet(OT_URI_PATH_COMMISSIONER_SET, &Leader::HandleCommissioningSet, this)
{
    Reset();
}

void Leader::Reset(void)
{
    LeaderBase::Reset();

    memset(mContextLastUsed, 0, sizeof(mContextLastUsed));
    mContextUsed = 0;
    mContextIdReuseDelay = kContextIdReuseDelay;
}

void Leader::Start(void)
{
    Coap::Coap &coap = GetNetif().GetCoap();

    coap.AddResource(mServerData);
    coap.AddResource(mCommissioningDataGet);
    coap.AddResource(mCommissioningDataSet);
}

void Leader::Stop(void)
{
    Coap::Coap &coap = GetNetif().GetCoap();

    coap.RemoveResource(mServerData);
    coap.RemoveResource(mCommissioningDataGet);
    coap.RemoveResource(mCommissioningDataSet);
}

void Leader::IncrementVersion(void)
{
    ThreadNetif &netif = GetNetif();

    if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        mVersion++;
        netif.SetStateChangedFlags(OT_CHANGED_THREAD_NETDATA);
    }
}

void Leader::IncrementStableVersion(void)
{
    if (GetNetif().GetMle().GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        mStableVersion++;
    }
}

uint32_t Leader::GetContextIdReuseDelay(void) const
{
    return mContextIdReuseDelay;
}

otError Leader::SetContextIdReuseDelay(uint32_t aDelay)
{
    mContextIdReuseDelay = aDelay;
    return OT_ERROR_NONE;
}

void Leader::RemoveBorderRouter(uint16_t aRloc16)
{
    bool rlocIn = false;
    bool rlocStable = false;
    RlocLookup(aRloc16, rlocIn, rlocStable, mTlvs, mLength);

    VerifyOrExit(rlocIn);

    RemoveRloc(aRloc16);
    mVersion++;

    if (rlocStable)
    {
        mStableVersion++;
    }

    GetNetif().SetStateChangedFlags(OT_CHANGED_THREAD_NETDATA);

exit:
    return;
}

void Leader::HandleServerData(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                              const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleServerData(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleServerData(Coap::Header &aHeader, Message &aMessage,
                              const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetworkDataTlv networkData;
    ThreadRloc16Tlv rloc16;

    otLogInfoNetData(GetInstance(), "Received network data registration");

    if (ThreadTlv::GetTlv(aMessage, ThreadTlv::kRloc16, sizeof(rloc16), rloc16) == OT_ERROR_NONE)
    {
        VerifyOrExit(rloc16.IsValid());
        RemoveBorderRouter(rloc16.GetRloc16());
    }

    if (ThreadTlv::GetTlv(aMessage, ThreadTlv::kThreadNetworkData, sizeof(networkData), networkData) ==
        OT_ERROR_NONE)
    {
        RegisterNetworkData(HostSwap16(aMessageInfo.mPeerAddr.mFields.m16[7]),
                            networkData.GetTlvs(), networkData.GetLength());
    }

    SuccessOrExit(GetNetif().GetCoap().SendEmptyAck(aHeader, aMessageInfo));

    otLogInfoNetData(GetInstance(), "Sent network data registration acknowledgment");

exit:
    return;
}

void Leader::HandleCommissioningSet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                    const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleCommissioningSet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleCommissioningSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    uint16_t offset = aMessage.GetOffset();
    uint8_t length = static_cast<uint8_t>(aMessage.GetLength() - aMessage.GetOffset());
    uint8_t tlvs[NetworkData::kMaxSize];
    MeshCoP::StateTlv::State state = MeshCoP::StateTlv::kAccept;
    bool hasSessionId = false;
    bool hasValidTlv = false;
    uint16_t sessionId = 0;

    VerifyOrExit(GetNetif().GetMle().GetRole() == OT_DEVICE_ROLE_LEADER, state = MeshCoP::StateTlv::kReject);

    aMessage.Read(offset, length, tlvs);

    // Session Id and Border Router Locator MUST NOT be set, but accept including unexpected or
    // unknown TLV as long as there is at least one valid TLV.
    for (MeshCoP::Tlv *cur = reinterpret_cast<MeshCoP::Tlv *>(tlvs);
         cur < reinterpret_cast<MeshCoP::Tlv *>(tlvs + length);
         cur = cur->GetNext())
    {
        MeshCoP::Tlv::Type type = cur->GetType();

        if (type == MeshCoP::Tlv::kJoinerUdpPort || type == MeshCoP::Tlv::kSteeringData)
        {
            hasValidTlv = true;
        }
        else if (type == MeshCoP::Tlv::kBorderAgentLocator)
        {
            ExitNow(state = MeshCoP::StateTlv::kReject);
        }
        else if (type == MeshCoP::Tlv::kCommissionerSessionId)
        {
            hasSessionId = true;
            sessionId = static_cast<MeshCoP::CommissionerSessionIdTlv *>(cur)->GetCommissionerSessionId();
        }
        else
        {
            // do nothing for unexpected or unknown TLV
        }
    }

    // verify whether or not commissioner session id TLV is included
    VerifyOrExit(hasSessionId, state = MeshCoP::StateTlv::kReject);

    // verify whether or not MGMT_COMM_SET.req includes at least one valid TLV
    VerifyOrExit(hasValidTlv, state = MeshCoP::StateTlv::kReject);

    // Find Commissioning Data TLV
    for (NetworkDataTlv *netDataTlv = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         netDataTlv < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         netDataTlv = netDataTlv->GetNext())
    {
        if (netDataTlv->GetType() == NetworkDataTlv::kTypeCommissioningData)
        {
            // Iterate over MeshCoP TLVs and extract desired data
            for (MeshCoP::Tlv *cur = reinterpret_cast<MeshCoP::Tlv *>(netDataTlv->GetValue());
                 cur < reinterpret_cast<MeshCoP::Tlv *>(netDataTlv->GetValue() + netDataTlv->GetLength());
                 cur = cur->GetNext())
            {
                if (cur->GetType() == MeshCoP::Tlv::kCommissionerSessionId)
                {
                    VerifyOrExit(sessionId ==
                                 static_cast<MeshCoP::CommissionerSessionIdTlv *>(cur)->GetCommissionerSessionId(),
                                 state = MeshCoP::StateTlv::kReject);
                }
                else if (cur->GetType() == MeshCoP::Tlv::kBorderAgentLocator)
                {
                    memcpy(tlvs + length, reinterpret_cast<uint8_t *>(cur), cur->GetLength() + sizeof(MeshCoP::Tlv));
                    length += (cur->GetLength() + sizeof(MeshCoP::Tlv));
                }
            }
        }
    }

    SetCommissioningData(tlvs, length);

exit:

    if (GetNetif().GetMle().GetRole() == OT_DEVICE_ROLE_LEADER)
    {
        SendCommissioningSetResponse(aHeader, aMessageInfo, state);
    }

    return;
}

void Leader::HandleCommissioningGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                    const otMessageInfo *aMessageInfo)
{
    static_cast<Leader *>(aContext)->HandleCommissioningGet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Leader::HandleCommissioningGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    MeshCoP::Tlv tlv;
    uint16_t offset = aMessage.GetOffset();
    uint8_t tlvs[NetworkData::kMaxSize];
    uint8_t length = 0;

    while (offset < aMessage.GetLength())
    {
        aMessage.Read(offset, sizeof(tlv), &tlv);

        if (tlv.GetType() == MeshCoP::Tlv::kGet)
        {
            length = tlv.GetLength();
            aMessage.Read(offset + sizeof(MeshCoP::Tlv), length, tlvs);
            break;
        }

        offset += sizeof(tlv) + tlv.GetLength();
    }

    SendCommissioningGetResponse(aHeader, aMessageInfo, tlvs, length);
}

void Leader::SendCommissioningGetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                          uint8_t *aTlvs, uint8_t aLength)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    Message *message;
    uint8_t index;
    uint8_t *data = NULL;
    uint8_t length = 0;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL,
                 error = OT_ERROR_NO_BUFS);

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() == NetworkDataTlv::kTypeCommissioningData)
        {
            data = cur->GetValue();
            length = cur->GetLength();
            break;
        }
    }

    VerifyOrExit(data && length, error = OT_ERROR_DROP);

    if (aLength == 0)
    {
        SuccessOrExit(error = message->Append(data, length));
    }
    else
    {
        for (index = 0; index < aLength; index++)
        {
            for (MeshCoP::Tlv *cur = reinterpret_cast<MeshCoP::Tlv *>(data);
                 cur < reinterpret_cast<MeshCoP::Tlv *>(data + length);
                 cur = cur->GetNext())
            {
                if (cur->GetType() == aTlvs[index])
                {
                    SuccessOrExit(error = message->Append(cur, sizeof(NetworkDataTlv) + cur->GetLength()));
                    break;
                }
            }
        }
    }

    if (message->GetLength() == responseHeader.GetLength())
    {
        // no payload, remove coap payload marker
        message->SetLength(message->GetLength() - 1);
    }

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent commissioning dataset get response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void Leader::SendCommissioningSetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                          MeshCoP::StateTlv::State aState)
{
    ThreadNetif &netif = GetNetif();
    otError error = OT_ERROR_NONE;
    Coap::Header responseHeader;
    Message *message;
    MeshCoP::StateTlv state;

    responseHeader.SetDefaultResponseHeader(aRequestHeader);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = MeshCoP::NewMeshCoPMessage(netif.GetCoap(), responseHeader)) != NULL,
                 error = OT_ERROR_NO_BUFS);

    state.Init();
    state.SetState(aState);
    SuccessOrExit(error = message->Append(&state, sizeof(state)));

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent commissioning dataset set response");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void Leader::RlocLookup(uint16_t aRloc16, bool &aIn, bool &aStable, uint8_t *aTlvs, uint8_t aTlvsLength)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aTlvs);
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);
    NetworkDataTlv *subCur;
    NetworkDataTlv *subEnd;
    PrefixTlv *prefix;
    BorderRouterTlv *borderRouter;
    HasRouteTlv *hasRoute;
    BorderRouterEntry *borderRouterEntry;
    HasRouteEntry *hasRouteEntry;

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypePrefix)
        {
            prefix = static_cast<PrefixTlv *>(cur);
            subCur = prefix->GetSubTlvs();
            subEnd = prefix->GetNext();

            while (subCur < subEnd)
            {
                switch (subCur->GetType())
                {
                case NetworkDataTlv::kTypeBorderRouter:
                    borderRouter = FindBorderRouter(*prefix);

                    for (uint8_t i = 0; i < borderRouter->GetNumEntries(); i++)
                    {
                        borderRouterEntry = borderRouter->GetEntry(i);

                        if (borderRouterEntry->GetRloc() == aRloc16)
                        {
                            aIn = true;

                            if (borderRouter->IsStable())
                            {
                                aStable = true;
                            }
                        }
                    }

                    break;

                case NetworkDataTlv::kTypeHasRoute:
                    hasRoute = FindHasRoute(*prefix);

                    for (uint8_t i = 0; i < hasRoute->GetNumEntries(); i++)
                    {
                        hasRouteEntry = hasRoute->GetEntry(i);

                        if (hasRouteEntry->GetRloc() == aRloc16)
                        {
                            aIn = true;

                            if (hasRoute->IsStable())
                            {
                                aStable = true;
                            }
                        }
                    }

                    break;

                default:
                    break;
                }

                if (aIn && aStable)
                {
                    ExitNow();
                }

                subCur = subCur->GetNext();
            }
        }

        cur = cur->GetNext();
    }

exit:
    return;
}

bool Leader::IsStableUpdated(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength, uint8_t *aTlvsBase,
                             uint8_t aTlvsBaseLength)
{
    bool rval = false;
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aTlvs);
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypePrefix)
        {
            PrefixTlv *prefix = static_cast<PrefixTlv *>(cur);
            ContextTlv *context = FindContext(*prefix);
            BorderRouterTlv *borderRouter = FindBorderRouter(*prefix);
            HasRouteTlv *hasRoute = FindHasRoute(*prefix);

            if (cur->IsStable() && (!context || borderRouter))
            {
                PrefixTlv *prefixBase = FindPrefix(prefix->GetPrefix(), prefix->GetPrefixLength(),
                                                   aTlvsBase, aTlvsBaseLength);

                if (!prefixBase)
                {
                    ExitNow(rval = true);
                }

                if (borderRouter)
                {
                    BorderRouterTlv *borderRouterBase = FindBorderRouter(*prefixBase);

                    if (!borderRouterBase || memcmp(borderRouter, borderRouterBase, borderRouter->GetLength()) != 0)
                    {
                        ExitNow(rval = true);
                    }
                }

                if (hasRoute)
                {
                    HasRouteTlv *hasRouteBase = FindHasRoute(*prefixBase);

                    if (!hasRouteBase || (memcmp(hasRoute, hasRouteBase, hasRoute->GetLength()) != 0))
                    {
                        ExitNow(rval = true);
                    }
                }
            }
        }

        cur = cur->GetNext();
    }

exit:
    OT_UNUSED_VARIABLE(aRloc16);
    return rval;
}

otError Leader::RegisterNetworkData(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength)
{
    otError error = OT_ERROR_NONE;
    bool rlocIn = false;
    bool rlocStable = false;
    bool stableUpdated = false;

    RlocLookup(aRloc16, rlocIn, rlocStable, mTlvs, mLength);

    if (rlocIn)
    {
        if (IsStableUpdated(aRloc16, aTlvs, aTlvsLength, mTlvs, mLength) ||
            IsStableUpdated(aRloc16, mTlvs, mLength, aTlvs, aTlvsLength))
        {
            stableUpdated = true;
        }

        SuccessOrExit(error = RemoveRloc(aRloc16));
        SuccessOrExit(error = AddNetworkData(aTlvs, aTlvsLength));

        mVersion++;

        if (stableUpdated)
        {
            mStableVersion++;
        }
    }
    else
    {
        RlocLookup(aRloc16, rlocIn, rlocStable, aTlvs, aTlvsLength);
        SuccessOrExit(error = AddNetworkData(aTlvs, aTlvsLength));

        mVersion++;

        if (rlocStable)
        {
            mStableVersion++;
        }
    }

    GetNetif().SetStateChangedFlags(OT_CHANGED_THREAD_NETDATA);

exit:
    return error;
}

otError Leader::AddNetworkData(uint8_t *aTlvs, uint8_t aTlvsLength)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aTlvs);
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);

    while (cur < end)
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            AddPrefix(*static_cast<PrefixTlv *>(cur));
            otDumpDebgNetData(GetInstance(), "add prefix done", mTlvs, mLength);
            break;

        default:
            break;
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData(GetInstance(), "add done", mTlvs, mLength);

    return OT_ERROR_NONE;
}

otError Leader::AddPrefix(PrefixTlv &aPrefix)
{
    NetworkDataTlv *cur = aPrefix.GetSubTlvs();
    NetworkDataTlv *end = aPrefix.GetNext();

    while (cur < end)
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            AddHasRoute(aPrefix, *static_cast<HasRouteTlv *>(cur));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            AddBorderRouter(aPrefix, *static_cast<BorderRouterTlv *>(cur));
            break;

        default:
            break;
        }

        cur = cur->GetNext();
    }

    return OT_ERROR_NONE;
}

otError Leader::AddHasRoute(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute)
{
    otError error = OT_ERROR_NONE;
    PrefixTlv *dstPrefix;
    HasRouteTlv *dstHasRoute;

    if ((dstPrefix = FindPrefix(aPrefix.GetPrefix(), aPrefix.GetPrefixLength())) == NULL)
    {
        dstPrefix = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
        Insert(reinterpret_cast<uint8_t *>(dstPrefix), sizeof(PrefixTlv) + BitVectorBytes(aPrefix.GetPrefixLength()));
        dstPrefix->Init(aPrefix.GetDomainId(), aPrefix.GetPrefixLength(), aPrefix.GetPrefix());
    }

    if (aHasRoute.IsStable())
    {
        dstPrefix->SetStable();
    }

    if ((dstHasRoute = FindHasRoute(*dstPrefix, aHasRoute.IsStable())) == NULL)
    {
        dstHasRoute = static_cast<HasRouteTlv *>(dstPrefix->GetNext());
        Insert(reinterpret_cast<uint8_t *>(dstHasRoute), sizeof(HasRouteTlv));
        dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(HasRouteTlv));
        dstHasRoute->Init();

        if (aHasRoute.IsStable())
        {
            dstHasRoute->SetStable();
        }
    }

    Insert(reinterpret_cast<uint8_t *>(dstHasRoute->GetNext()), sizeof(HasRouteEntry));
    dstHasRoute->SetLength(dstHasRoute->GetLength() + sizeof(HasRouteEntry));
    dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(HasRouteEntry));
    memcpy(dstHasRoute->GetEntry(dstHasRoute->GetNumEntries() - 1), aHasRoute.GetEntry(0),
           sizeof(HasRouteEntry));

    return error;
}

otError Leader::AddBorderRouter(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter)
{
    otError error = OT_ERROR_NONE;
    PrefixTlv *dstPrefix;
    ContextTlv *dstContext;
    BorderRouterTlv *dstBorderRouter;
    int contextId;

    if ((dstPrefix = FindPrefix(aPrefix.GetPrefix(), aPrefix.GetPrefixLength())) == NULL)
    {
        dstPrefix = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
        Insert(reinterpret_cast<uint8_t *>(dstPrefix), sizeof(PrefixTlv) + BitVectorBytes(aPrefix.GetPrefixLength()));
        dstPrefix->Init(aPrefix.GetDomainId(), aPrefix.GetPrefixLength(), aPrefix.GetPrefix());
    }

    if ((dstContext = FindContext(*dstPrefix)) != NULL)
    {
        dstContext->SetCompress();
    }
    else if ((contextId = AllocateContext()) >= 0)
    {
        dstContext = static_cast<ContextTlv *>(dstPrefix->GetNext());
        Insert(reinterpret_cast<uint8_t *>(dstContext), sizeof(ContextTlv));
        dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(ContextTlv));
        dstContext->Init();
        dstContext->SetCompress();
        dstContext->SetContextId(static_cast<uint8_t>(contextId));
        dstContext->SetContextLength(aPrefix.GetPrefixLength());
    }

    VerifyOrExit(dstContext != NULL, error = OT_ERROR_NO_BUFS);
    mContextLastUsed[dstContext->GetContextId() - kMinContextId] = 0;


    if ((dstBorderRouter = FindBorderRouter(*dstPrefix, aBorderRouter.IsStable())) == NULL)
    {
        dstBorderRouter = static_cast<BorderRouterTlv *>(dstPrefix->GetNext());
        Insert(reinterpret_cast<uint8_t *>(dstBorderRouter), sizeof(BorderRouterTlv));
        dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(BorderRouterTlv));
        dstBorderRouter->Init();
    }

    Insert(reinterpret_cast<uint8_t *>(dstBorderRouter->GetNext()), sizeof(BorderRouterEntry));
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
            otLogInfoNetData(GetInstance(), "Allocated Context ID = %d", rval);
            ExitNow();
        }
    }

exit:
    return rval;
}

otError Leader::FreeContext(uint8_t aContextId)
{
    otLogInfoNetData(GetInstance(), "Free Context Id = %d", aContextId);
    RemoveContext(aContextId);
    mContextUsed &= ~(1 << aContextId);
    mVersion++;
    mStableVersion++;
    GetNetif().SetStateChangedFlags(OT_CHANGED_THREAD_NETDATA);
    return OT_ERROR_NONE;
}

otError Leader::SendServerDataNotification(uint16_t aRloc16)
{
    otError error = OT_ERROR_NONE;
    bool rlocIn = false;
    bool rlocStable = false;

    RlocLookup(aRloc16, rlocIn, rlocStable, mTlvs, mLength);

    VerifyOrExit(rlocIn, error = OT_ERROR_NOT_FOUND);

    SuccessOrExit(error = NetworkData::SendServerDataNotification(aRloc16));

exit:
    return error;
}

otError Leader::RemoveRloc(uint16_t aRloc16)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
    NetworkDataTlv *end;
    PrefixTlv *prefix;

    while (1)
    {
        end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            prefix = static_cast<PrefixTlv *>(cur);
            RemoveRloc(*prefix, aRloc16);

            if (prefix->GetSubTlvsLength() == 0)
            {
                Remove(reinterpret_cast<uint8_t *>(prefix), sizeof(NetworkDataTlv) + prefix->GetLength());
                continue;
            }

            otDumpDebgNetData(GetInstance(), "remove prefix done", mTlvs, mLength);
            break;
        }

        default:
            break;
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData(GetInstance(), "remove done", mTlvs, mLength);

    return OT_ERROR_NONE;
}

otError Leader::RemoveRloc(PrefixTlv &prefix, uint16_t aRloc16)
{
    NetworkDataTlv *cur = prefix.GetSubTlvs();
    NetworkDataTlv *end;
    ContextTlv *context;

    while (1)
    {
        end = prefix.GetNext();

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            RemoveRloc(prefix, *static_cast<HasRouteTlv *>(cur), aRloc16);

            // remove has route tlv if empty
            if (cur->GetLength() == 0)
            {
                prefix.SetSubTlvsLength(prefix.GetSubTlvsLength() - sizeof(HasRouteTlv));
                Remove(reinterpret_cast<uint8_t *>(cur), sizeof(HasRouteTlv));
                continue;
            }

            break;

        case NetworkDataTlv::kTypeBorderRouter:
            RemoveRloc(prefix, *static_cast<BorderRouterTlv *>(cur), aRloc16);

            // remove border router tlv if empty
            if (cur->GetLength() == 0)
            {
                prefix.SetSubTlvsLength(prefix.GetSubTlvsLength() - sizeof(BorderRouterTlv));
                Remove(reinterpret_cast<uint8_t *>(cur), sizeof(BorderRouterTlv));
                continue;
            }

            break;

        default:
            break;
        }

        cur = cur->GetNext();
    }

    if ((context = FindContext(prefix)) != NULL)
    {
        if (prefix.GetSubTlvsLength() == sizeof(ContextTlv))
        {
            context->ClearCompress();
            mContextLastUsed[context->GetContextId() - kMinContextId] = TimerMilli::GetNow();

            if (mContextLastUsed[context->GetContextId() - kMinContextId] == 0)
            {
                mContextLastUsed[context->GetContextId() - kMinContextId] = 1;
            }

            mTimer.Start(kStateUpdatePeriod);
        }
        else
        {
            context->SetCompress();
            mContextLastUsed[context->GetContextId() - kMinContextId] = 0;
        }
    }

    return OT_ERROR_NONE;
}

otError Leader::RemoveRloc(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute, uint16_t aRloc16)
{
    HasRouteEntry *entry;

    // remove rloc from has route tlv
    for (uint8_t i = 0; i < aHasRoute.GetNumEntries(); i++)
    {
        entry = aHasRoute.GetEntry(i);

        if (entry->GetRloc() != aRloc16)
        {
            continue;
        }

        aHasRoute.SetLength(aHasRoute.GetLength() - sizeof(HasRouteEntry));
        aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - sizeof(HasRouteEntry));
        Remove(reinterpret_cast<uint8_t *>(entry), sizeof(*entry));
        break;
    }

    return OT_ERROR_NONE;
}

otError Leader::RemoveRloc(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter, uint16_t aRloc16)
{
    BorderRouterEntry *entry;

    // remove rloc from border router tlv
    for (uint8_t i = 0; i < aBorderRouter.GetNumEntries(); i++)
    {
        entry = aBorderRouter.GetEntry(i);

        if (entry->GetRloc() != aRloc16)
        {
            continue;
        }

        aBorderRouter.SetLength(aBorderRouter.GetLength() - sizeof(BorderRouterEntry));
        aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - sizeof(BorderRouterEntry));
        Remove(reinterpret_cast<uint8_t *>(entry), sizeof(*entry));
        break;
    }

    return OT_ERROR_NONE;
}

otError Leader::RemoveContext(uint8_t aContextId)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
    NetworkDataTlv *end;
    PrefixTlv *prefix;

    while (1)
    {
        end = reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
        {
            prefix = static_cast<PrefixTlv *>(cur);
            RemoveContext(*prefix, aContextId);

            if (prefix->GetSubTlvsLength() == 0)
            {
                Remove(reinterpret_cast<uint8_t *>(prefix), sizeof(NetworkDataTlv) + prefix->GetLength());
                continue;
            }

            otDumpDebgNetData(GetInstance(), "remove prefix done", mTlvs, mLength);
            break;
        }

        default:
            break;
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData(GetInstance(), "remove done", mTlvs, mLength);

    return OT_ERROR_NONE;
}

otError Leader::RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId)
{
    NetworkDataTlv *cur = aPrefix.GetSubTlvs();
    NetworkDataTlv *end;
    ContextTlv *context;
    uint8_t length;

    while (1)
    {
        end = aPrefix.GetNext();

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeContext:
        {
            // remove context tlv
            context = static_cast<ContextTlv *>(cur);

            if (context->GetContextId() == aContextId)
            {
                length = sizeof(NetworkDataTlv) + context->GetLength();
                aPrefix.SetSubTlvsLength(aPrefix.GetSubTlvsLength() - length);
                Remove(reinterpret_cast<uint8_t *>(context), length);
                continue;
            }

            break;
        }

        default:
            break;
        }

        cur = cur->GetNext();
    }

    return OT_ERROR_NONE;
}

void Leader::HandleTimer(Timer &aTimer)
{
    GetOwner(aTimer).HandleTimer();
}

void Leader::HandleTimer(void)
{
    bool contextsWaiting = false;

    for (uint8_t i = 0; i < kNumContextIds; i++)
    {
        if (mContextLastUsed[i] == 0)
        {
            continue;
        }

        if ((TimerMilli::GetNow() - mContextLastUsed[i]) >= TimerMilli::SecToMsec(mContextIdReuseDelay))
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

Leader &Leader::GetOwner(const Context &aContext)
{
#if OPENTHREAD_ENABLE_MULTIPLE_INSTANCES
    Leader &leader = *static_cast<Leader *>(aContext.GetContext());
#else
    Leader &leader = otGetThreadNetif().GetNetworkDataLeader();
    OT_UNUSED_VARIABLE(aContext);
#endif
    return leader;
}

}  // namespace NetworkData
}  // namespace ot

#endif // OPENTHREAD_FTD

