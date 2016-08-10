/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <coap/coap_header.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <common/message.hpp>
#include <common/timer.hpp>
#include <mac/mac_frame.hpp>
#include <platform/random.h>
#include <thread/mle_router.hpp>
#include <thread/network_data_leader.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {
namespace NetworkData {

Leader::Leader(ThreadNetif &aThreadNetif):
    mTimer(&HandleTimer, this),
    mServerData(OPENTHREAD_URI_SERVER_DATA, &HandleServerData, this),
    mCoapServer(aThreadNetif.GetCoapServer()),
    mNetif(aThreadNetif),
    mMle(aThreadNetif.GetMle())
{
    Reset();
}

void Leader::Reset(void)
{
    memset(mAddresses, 0, sizeof(mAddresses));
    memset(mContextLastUsed, 0, sizeof(mContextLastUsed));
    mVersion = otPlatRandomGet();
    mStableVersion = otPlatRandomGet();
    mLength = 0;
    mContextUsed = 0;
    mContextIdReuseDelay = kContextIdReuseDelay;
}

void Leader::Start(void)
{
    mCoapServer.AddResource(mServerData);
}

void Leader::Stop(void)
{
}

uint8_t Leader::GetVersion(void) const
{
    return mVersion;
}

void Leader::IncrementVersion(void)
{
    if (mMle.GetDeviceState() == Mle::kDeviceStateLeader)
    {
        mVersion++;
    }
}

uint8_t Leader::GetStableVersion(void) const
{
    return mStableVersion;
}

void Leader::IncrementStableVersion(void)
{
    if (mMle.GetDeviceState() == Mle::kDeviceStateLeader)
    {
        mStableVersion++;
    }
}

uint32_t Leader::GetContextIdReuseDelay(void) const
{
    return mContextIdReuseDelay;
}

ThreadError Leader::SetContextIdReuseDelay(uint32_t aDelay)
{
    mContextIdReuseDelay = aDelay;
    return kThreadError_None;
}

ThreadError Leader::GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext)
{
    PrefixTlv *prefix;
    ContextTlv *contextTlv;

    aContext.mPrefixLength = 0;

    if (PrefixMatch(mMle.GetMeshLocalPrefix(), aAddress.mFields.m8, 64) >= 0)
    {
        aContext.mPrefix = mMle.GetMeshLocalPrefix();
        aContext.mPrefixLength = 64;
        aContext.mContextId = 0;
    }

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = reinterpret_cast<PrefixTlv *>(cur);

        if (PrefixMatch(prefix->GetPrefix(), aAddress.mFields.m8, prefix->GetPrefixLength()) < 0)
        {
            continue;
        }

        contextTlv = FindContext(*prefix);

        if (contextTlv == NULL)
        {
            continue;
        }

        if (prefix->GetPrefixLength() > aContext.mPrefixLength)
        {
            aContext.mPrefix = prefix->GetPrefix();
            aContext.mPrefixLength = prefix->GetPrefixLength();
            aContext.mContextId = contextTlv->GetContextId();
        }
    }

    return (aContext.mPrefixLength > 0) ? kThreadError_None : kThreadError_Error;
}

ThreadError Leader::GetContext(uint8_t aContextId, Lowpan::Context &aContext)
{
    ThreadError error = kThreadError_Error;
    PrefixTlv *prefix;
    ContextTlv *contextTlv;

    if (aContextId == 0)
    {
        aContext.mPrefix = mMle.GetMeshLocalPrefix();
        aContext.mPrefixLength = 64;
        aContext.mContextId = 0;
        ExitNow(error = kThreadError_None);
    }

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = reinterpret_cast<PrefixTlv *>(cur);
        contextTlv = FindContext(*prefix);

        if (contextTlv == NULL)
        {
            continue;
        }

        if (contextTlv->GetContextId() != aContextId)
        {
            continue;
        }

        aContext.mPrefix = prefix->GetPrefix();
        aContext.mPrefixLength = prefix->GetPrefixLength();
        aContext.mContextId = contextTlv->GetContextId();
        ExitNow(error = kThreadError_None);
    }

exit:
    return error;
}

ThreadError Leader::ConfigureAddresses(void)
{
    PrefixTlv *prefix;

    // clear out addresses that are not on-mesh
    for (size_t i = 0; i < sizeof(mAddresses) / sizeof(mAddresses[0]); i++)
    {
        if (mAddresses[i].mValidLifetime == 0 ||
            IsOnMesh(mAddresses[i].GetAddress()))
        {
            continue;
        }

        mNetif.RemoveUnicastAddress(mAddresses[i]);
        mAddresses[i].mValidLifetime = 0;
    }

    // configure on-mesh addresses
    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = reinterpret_cast<PrefixTlv *>(cur);
        ConfigureAddress(*prefix);
    }

    return kThreadError_None;
}

ThreadError Leader::ConfigureAddress(PrefixTlv &aPrefix)
{
    BorderRouterTlv *borderRouter;
    BorderRouterEntry *entry;

    // look for Border Router TLV
    if ((borderRouter = FindBorderRouter(aPrefix)) == NULL)
    {
        ExitNow();
    }

    // check if SLAAC flag is set
    if ((entry = borderRouter->GetEntry(0)) == NULL ||
        entry->IsSlaac() == false)
    {
        ExitNow();
    }

    // check if address is already added for this prefix
    for (size_t i = 0; i < sizeof(mAddresses) / sizeof(mAddresses[0]); i++)
    {
        if (mAddresses[i].mValidLifetime != 0 &&
            mAddresses[i].mPrefixLength == aPrefix.GetPrefixLength() &&
            PrefixMatch(mAddresses[i].mAddress.mFields.m8, aPrefix.GetPrefix(), aPrefix.GetPrefixLength()) >= 0)
        {
            mAddresses[i].mPreferredLifetime = entry->IsPreferred() ? 0xffffffff : 0;
            ExitNow();
        }
    }

    // configure address for this prefix
    for (size_t i = 0; i < sizeof(mAddresses) / sizeof(mAddresses[0]); i++)
    {
        if (mAddresses[i].mValidLifetime != 0)
        {
            continue;
        }

        memset(&mAddresses[i], 0, sizeof(mAddresses[i]));
        memcpy(mAddresses[i].mAddress.mFields.m8, aPrefix.GetPrefix(), BitVectorBytes(aPrefix.GetPrefixLength()));

        for (size_t j = 8; j < sizeof(mAddresses[i].mAddress); j++)
        {
            mAddresses[i].mAddress.mFields.m8[j] = otPlatRandomGet();
        }

        mAddresses[i].mPrefixLength = aPrefix.GetPrefixLength();
        mAddresses[i].mPreferredLifetime = entry->IsPreferred() ? 0xffffffff : 0;
        mAddresses[i].mValidLifetime = 0xffffffff;
        mNetif.AddUnicastAddress(mAddresses[i]);
        break;
    }

exit:
    return kThreadError_None;
}

bool Leader::IsOnMesh(const Ip6::Address &aAddress)
{
    PrefixTlv *prefix;
    bool rval = false;

    if (memcmp(aAddress.mFields.m8, mMle.GetMeshLocalPrefix(), 8) == 0)
    {
        ExitNow(rval = true);
    }

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = reinterpret_cast<PrefixTlv *>(cur);

        if (PrefixMatch(prefix->GetPrefix(), aAddress.mFields.m8, prefix->GetPrefixLength()) < 0)
        {
            continue;
        }

        if (FindBorderRouter(*prefix) == NULL)
        {
            continue;
        }

        ExitNow(rval = true);
    }

exit:
    return rval;
}

ThreadError Leader::RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination,
                                uint8_t *aPrefixMatch, uint16_t *aRloc16)
{
    ThreadError error = kThreadError_NoRoute;
    PrefixTlv *prefix;

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = reinterpret_cast<PrefixTlv *>(cur);

        if (PrefixMatch(prefix->GetPrefix(), aSource.mFields.m8, prefix->GetPrefixLength()) >= 0)
        {
            if (ExternalRouteLookup(prefix->GetDomainId(), aDestination, aPrefixMatch, aRloc16) == kThreadError_None)
            {
                ExitNow(error = kThreadError_None);
            }

            if (DefaultRouteLookup(*prefix, aRloc16) == kThreadError_None)
            {
                if (aPrefixMatch)
                {
                    *aPrefixMatch = 0;
                }

                ExitNow(error = kThreadError_None);
            }
        }
    }

exit:
    return error;
}

ThreadError Leader::ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &aDestination,
                                        uint8_t *aPrefixMatch, uint16_t *aRloc16)
{
    ThreadError error = kThreadError_NoRoute;
    PrefixTlv *prefix;
    HasRouteTlv *hasRoute;
    HasRouteEntry *entry;
    HasRouteEntry *rvalRoute = NULL;
    int8_t rval_plen = 0;
    int8_t plen;
    NetworkDataTlv *cur;
    NetworkDataTlv *subCur;

    for (cur = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength);
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypePrefix)
        {
            continue;
        }

        prefix = reinterpret_cast<PrefixTlv *>(cur);

        if (prefix->GetDomainId() != aDomainId)
        {
            continue;
        }

        plen = PrefixMatch(prefix->GetPrefix(), aDestination.mFields.m8, prefix->GetPrefixLength());

        if (plen > rval_plen)
        {
            // select border router
            for (subCur = reinterpret_cast<NetworkDataTlv *>(prefix->GetSubTlvs());
                 subCur < reinterpret_cast<NetworkDataTlv *>(prefix->GetSubTlvs() + prefix->GetSubTlvsLength());
                 subCur = subCur->GetNext())
            {
                if (subCur->GetType() != NetworkDataTlv::kTypeHasRoute)
                {
                    continue;
                }

                hasRoute = reinterpret_cast<HasRouteTlv *>(subCur);

                for (int i = 0; i < hasRoute->GetNumEntries(); i++)
                {
                    entry = hasRoute->GetEntry(i);

                    if (rvalRoute == NULL ||
                        entry->GetPreference() > rvalRoute->GetPreference() ||
                        (entry->GetPreference() == rvalRoute->GetPreference() &&
                         mMle.GetRouteCost(entry->GetRloc()) < mMle.GetRouteCost(rvalRoute->GetRloc())))
                    {
                        rvalRoute = entry;
                        rval_plen = plen;
                    }
                }

            }
        }
    }

    if (rvalRoute != NULL)
    {
        if (aRloc16 != NULL)
        {
            *aRloc16 = rvalRoute->GetRloc();
        }

        if (aPrefixMatch != NULL)
        {
            *aPrefixMatch = rval_plen;
        }

        error = kThreadError_None;
    }

    return error;
}

ThreadError Leader::DefaultRouteLookup(PrefixTlv &aPrefix, uint16_t *aRloc16)
{
    ThreadError error = kThreadError_NoRoute;
    BorderRouterTlv *borderRouter;
    BorderRouterEntry *entry;
    BorderRouterEntry *route = NULL;

    for (NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
         cur < reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());
         cur = cur->GetNext())
    {
        if (cur->GetType() != NetworkDataTlv::kTypeBorderRouter)
        {
            continue;
        }

        borderRouter = reinterpret_cast<BorderRouterTlv *>(cur);

        for (int i = 0; i < borderRouter->GetNumEntries(); i++)
        {
            entry = borderRouter->GetEntry(i);

            if (entry->IsDefaultRoute() == false)
            {
                continue;
            }

            if (route == NULL ||
                entry->GetPreference() > route->GetPreference() ||
                (entry->GetPreference() == route->GetPreference() &&
                 mMle.GetRouteCost(entry->GetRloc()) < mMle.GetRouteCost(route->GetRloc())))
            {
                route = entry;
            }
        }
    }

    if (route != NULL)
    {
        if (aRloc16 != NULL)
        {
            *aRloc16 = route->GetRloc();
        }

        error = kThreadError_None;
    }

    return error;
}

void Leader::SetNetworkData(uint8_t aVersion, uint8_t aStableVersion, bool aStable,
                            const uint8_t *aData, uint8_t aDataLength)
{
    mVersion = aVersion;
    mStableVersion = aStableVersion;
    memcpy(mTlvs, aData, aDataLength);
    mLength = aDataLength;

    if (aStable)
    {
        RemoveTemporaryData(mTlvs, mLength);
    }

    otDumpDebgNetData("set network data", mTlvs, mLength);

    ConfigureAddresses();
    mMle.HandleNetworkDataUpdate();
}

void Leader::RemoveBorderRouter(uint16_t aRloc16)
{
    bool rlocIn = false;
    bool rlocStable = false;
    RlocLookup(aRloc16, rlocIn, rlocStable, mTlvs, mLength);

    if (rlocIn)
    {
        RemoveRloc(aRloc16);
        mVersion++;

        if (rlocStable)
        {
            mStableVersion++;
        }

        ConfigureAddresses();
    }

    mMle.HandleNetworkDataUpdate();
}

void Leader::HandleServerData(void *aContext, Coap::Header &aHeader, Message &aMessage,
                              const Ip6::MessageInfo &aMessageInfo)
{
    Leader *obj = reinterpret_cast<Leader *>(aContext);
    obj->HandleServerData(aHeader, aMessage, aMessageInfo);
}

void Leader::HandleServerData(Coap::Header &aHeader, Message &aMessage,
                              const Ip6::MessageInfo &aMessageInfo)
{
    ThreadNetworkDataTlv threadNetworkDataTlv;
    uint8_t tlvsLength;
    uint8_t tlvs[kMaxSize];
    uint16_t rloc16;

    otLogInfoNetData("Received network data registration\n");

    aMessage.Read(aMessage.GetOffset(), sizeof(threadNetworkDataTlv), &threadNetworkDataTlv);
    tlvsLength = threadNetworkDataTlv.GetLength();

    aMessage.Read(aMessage.GetOffset() + sizeof(threadNetworkDataTlv), tlvsLength, tlvs);
    rloc16 = HostSwap16(aMessageInfo.mPeerAddr.mFields.m16[7]);

    SendServerDataResponse(aHeader, aMessageInfo, NULL, 0);
    RegisterNetworkData(rloc16, tlvs, tlvsLength);
}

void Leader::SendServerDataResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                    const uint8_t *aTlvs, uint8_t aTlvsLength)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Message *message;

    VerifyOrExit((message = Ip6::Udp::NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    responseHeader.Init();
    responseHeader.SetVersion(1);
    responseHeader.SetType(Coap::Header::kTypeAcknowledgment);
    responseHeader.SetCode(Coap::Header::kCodeChanged);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());
    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());
    responseHeader.AppendContentFormatOption(Coap::Header::kApplicationOctetStream);
    responseHeader.Finalize();
    SuccessOrExit(error = message->Append(responseHeader.GetBytes(), responseHeader.GetLength()));
    SuccessOrExit(error = message->Append(aTlvs, aTlvsLength));

    SuccessOrExit(error = mCoapServer.SendMessage(*message, aMessageInfo));

    otLogInfoNetData("Sent network data registration acknowledgment\n");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        Message::Free(*message);
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
            prefix = reinterpret_cast<PrefixTlv *>(cur);
            subCur = reinterpret_cast<NetworkDataTlv *>(prefix->GetSubTlvs());
            subEnd = reinterpret_cast<NetworkDataTlv *>(prefix->GetSubTlvs() + prefix->GetSubTlvsLength());

            while (subCur < subEnd)
            {
                switch (subCur->GetType())
                {
                case NetworkDataTlv::kTypeBorderRouter:
                    borderRouter = FindBorderRouter(*prefix);

                    for (int i = 0; i < borderRouter->GetNumEntries(); i++)
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

                    for (int i = 0; i < hasRoute->GetNumEntries(); i++)
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
    PrefixTlv *prefix;
    PrefixTlv *prefixBase;
    BorderRouterTlv *borderRouter;
    HasRouteTlv *hasRoute;
    ContextTlv *context;

    while (cur < end)
    {
        if (cur->GetType() == NetworkDataTlv::kTypePrefix)
        {
            prefix = reinterpret_cast<PrefixTlv *>(cur);
            context = FindContext(*prefix);
            borderRouter = FindBorderRouter(*prefix);
            hasRoute = FindHasRoute(*prefix);

            if (cur->IsStable() && (!context || borderRouter))
            {
                prefixBase = FindPrefix(prefix->GetPrefix(), prefix->GetPrefixLength(), aTlvsBase, aTlvsBaseLength);

                if (!prefixBase)
                {
                    ExitNow(rval = true);
                }

                if (borderRouter && memcmp(borderRouter, FindBorderRouter(*prefixBase), borderRouter->GetLength()) != 0)
                {
                    ExitNow(rval = true);
                }

                if (hasRoute && (memcmp(hasRoute, FindHasRoute(*prefixBase), hasRoute->GetLength()) != 0))
                {
                    ExitNow(rval = true);
                }
            }
        }

        cur = cur->GetNext();
    }

exit:
    (void)aRloc16;
    return rval;
}

ThreadError Leader::RegisterNetworkData(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength)
{
    ThreadError error = kThreadError_None;
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

    ConfigureAddresses();
    mMle.HandleNetworkDataUpdate();

exit:
    return error;
}

ThreadError Leader::AddNetworkData(uint8_t *aTlvs, uint8_t aTlvsLength)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aTlvs);
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aTlvs + aTlvsLength);

    while (cur < end)
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypePrefix:
            AddPrefix(*reinterpret_cast<PrefixTlv *>(cur));
            otDumpDebgNetData("add prefix done", mTlvs, mLength);
            break;

        default:
            assert(false);
            break;
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData("add done", mTlvs, mLength);

    return kThreadError_None;
}

ThreadError Leader::AddPrefix(PrefixTlv &aPrefix)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

    while (cur < end)
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            AddHasRoute(aPrefix, *reinterpret_cast<HasRouteTlv *>(cur));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            AddBorderRouter(aPrefix, *reinterpret_cast<BorderRouterTlv *>(cur));
            break;

        default:
            assert(false);
            break;
        }

        cur = cur->GetNext();
    }

    return kThreadError_None;
}

ThreadError Leader::AddHasRoute(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute)
{
    ThreadError error = kThreadError_None;
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
        dstHasRoute = reinterpret_cast<HasRouteTlv *>(dstPrefix->GetNext());
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

ThreadError Leader::AddBorderRouter(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter)
{
    ThreadError error = kThreadError_None;
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

    if (aBorderRouter.IsStable())
    {
        dstPrefix->SetStable();

        if ((dstContext = FindContext(*dstPrefix)) != NULL)
        {
            dstContext->SetCompress();
        }
        else if ((contextId = AllocateContext()) >= 0)
        {
            dstContext = reinterpret_cast<ContextTlv *>(dstPrefix->GetNext());
            Insert(reinterpret_cast<uint8_t *>(dstContext), sizeof(ContextTlv));
            dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(ContextTlv));
            dstContext->Init();
            dstContext->SetStable();
            dstContext->SetCompress();
            dstContext->SetContextId(contextId);
            dstContext->SetContextLength(aPrefix.GetPrefixLength());
        }
        else
        {
            ExitNow(error = kThreadError_NoBufs);
        }

        mContextLastUsed[dstContext->GetContextId() - kMinContextId] = 0;
    }

    if ((dstBorderRouter = FindBorderRouter(*dstPrefix, aBorderRouter.IsStable())) == NULL)
    {
        dstBorderRouter = reinterpret_cast<BorderRouterTlv *>(dstPrefix->GetNext());
        Insert(reinterpret_cast<uint8_t *>(dstBorderRouter), sizeof(BorderRouterTlv));
        dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(BorderRouterTlv));
        dstBorderRouter->Init();

        if (aBorderRouter.IsStable())
        {
            dstBorderRouter->SetStable();
        }
    }

    Insert(reinterpret_cast<uint8_t *>(dstBorderRouter->GetNext()), sizeof(BorderRouterEntry));
    dstBorderRouter->SetLength(dstBorderRouter->GetLength() + sizeof(BorderRouterEntry));
    dstPrefix->SetLength(dstPrefix->GetLength() + sizeof(BorderRouterEntry));
    memcpy(dstBorderRouter->GetEntry(dstBorderRouter->GetNumEntries() - 1), aBorderRouter.GetEntry(0),
           sizeof(BorderRouterEntry));

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
            otLogInfoNetData("Allocated Context ID = %d\n", rval);
            ExitNow();
        }
    }

exit:
    return rval;
}

ThreadError Leader::FreeContext(uint8_t aContextId)
{
    otLogInfoNetData("Free Context Id = %d\n", aContextId);
    RemoveContext(aContextId);
    mContextUsed &= ~(1 << aContextId);
    mVersion++;
    mStableVersion++;
    mMle.HandleNetworkDataUpdate();
    return kThreadError_None;
}

ThreadError Leader::RemoveRloc(uint16_t aRloc16)
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
            prefix = reinterpret_cast<PrefixTlv *>(cur);
            RemoveRloc(*prefix, aRloc16);

            if (prefix->GetSubTlvsLength() == 0)
            {
                Remove(reinterpret_cast<uint8_t *>(prefix), sizeof(NetworkDataTlv) + prefix->GetLength());
                continue;
            }

            otDumpDebgNetData("remove prefix done", mTlvs, mLength);
            break;
        }

        default:
        {
            assert(false);
            break;
        }
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData("remove done", mTlvs, mLength);

    return kThreadError_None;
}

ThreadError Leader::RemoveRloc(PrefixTlv &prefix, uint16_t aRloc16)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(prefix.GetSubTlvs());
    NetworkDataTlv *end;
    ContextTlv *context;

    while (1)
    {
        end = reinterpret_cast<NetworkDataTlv *>(prefix.GetSubTlvs() + prefix.GetSubTlvsLength());

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            RemoveRloc(prefix, *reinterpret_cast<HasRouteTlv *>(cur), aRloc16);

            // remove has route tlv if empty
            if (cur->GetLength() == 0)
            {
                prefix.SetSubTlvsLength(prefix.GetSubTlvsLength() - sizeof(HasRouteTlv));
                Remove(reinterpret_cast<uint8_t *>(cur), sizeof(HasRouteTlv));
                continue;
            }

            break;

        case NetworkDataTlv::kTypeBorderRouter:
            RemoveRloc(prefix, *reinterpret_cast<BorderRouterTlv *>(cur), aRloc16);

            // remove border router tlv if empty
            if (cur->GetLength() == 0)
            {
                prefix.SetSubTlvsLength(prefix.GetSubTlvsLength() - sizeof(BorderRouterTlv));
                Remove(reinterpret_cast<uint8_t *>(cur), sizeof(BorderRouterTlv));
                continue;
            }

            break;

        case NetworkDataTlv::kTypeContext:
            break;

        default:
            assert(false);
            break;
        }

        cur = cur->GetNext();
    }

    if ((context = FindContext(prefix)) != NULL)
    {
        if (prefix.GetSubTlvsLength() == sizeof(ContextTlv))
        {
            context->ClearCompress();
            mContextLastUsed[context->GetContextId() - kMinContextId] = Timer::GetNow();

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

    return kThreadError_None;
}

ThreadError Leader::RemoveRloc(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute, uint16_t aRloc16)
{
    HasRouteEntry *entry;

    // remove rloc from has route tlv
    for (int i = 0; i < aHasRoute.GetNumEntries(); i++)
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

    return kThreadError_None;
}

ThreadError Leader::RemoveRloc(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter, uint16_t aRloc16)
{
    BorderRouterEntry *entry;

    // remove rloc from border router tlv
    for (int i = 0; i < aBorderRouter.GetNumEntries(); i++)
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

    return kThreadError_None;
}

ThreadError Leader::RemoveContext(uint8_t aContextId)
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
            prefix = reinterpret_cast<PrefixTlv *>(cur);
            RemoveContext(*prefix, aContextId);

            if (prefix->GetSubTlvsLength() == 0)
            {
                Remove(reinterpret_cast<uint8_t *>(prefix), sizeof(NetworkDataTlv) + prefix->GetLength());
                continue;
            }

            otDumpDebgNetData("remove prefix done", mTlvs, mLength);
            break;
        }

        default:
        {
            assert(false);
            break;
        }
        }

        cur = cur->GetNext();
    }

    otDumpDebgNetData("remove done", mTlvs, mLength);

    return kThreadError_None;
}

ThreadError Leader::RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId)
{
    NetworkDataTlv *cur = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs());
    NetworkDataTlv *end;
    ContextTlv *context;
    uint8_t length;

    while (1)
    {
        end = reinterpret_cast<NetworkDataTlv *>(aPrefix.GetSubTlvs() + aPrefix.GetSubTlvsLength());

        if (cur >= end)
        {
            break;
        }

        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeBorderRouter:
        {
            break;
        }

        case NetworkDataTlv::kTypeContext:
        {
            // remove context tlv
            context = reinterpret_cast<ContextTlv *>(cur);

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
        {
            assert(false);
            break;
        }
        }

        cur = cur->GetNext();
    }

    return kThreadError_None;
}

void Leader::HandleTimer(void *aContext)
{
    Leader *obj = reinterpret_cast<Leader *>(aContext);
    obj->HandleTimer();
}

void Leader::HandleTimer(void)
{
    bool contextsWaiting = false;

    for (int i = 0; i < kNumContextIds; i++)
    {
        if (mContextLastUsed[i] == 0)
        {
            continue;
        }

        if ((Timer::GetNow() - mContextLastUsed[i]) >= Timer::SecToMsec(mContextIdReuseDelay))
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

}  // namespace NetworkData
}  // namespace Thread
