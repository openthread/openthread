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
 *   This file implements Thread NdProxy Table management.
 */

#include "ndproxy_table.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "common/locator-getters.hpp"
#include "common/logging.hpp"

namespace ot {

namespace BackboneRouter {

void NdProxyTable::NdProxy::Init(const Ip6::InterfaceIdentifier &aAddressIid,
                                 const Ip6::InterfaceIdentifier &aMeshLocalIid,
                                 uint16_t                        aRloc16,
                                 uint32_t                        aTimeSinceLastTransaction)
{
    OT_ASSERT(!mValid);

    mValid        = true;
    mAddressIid   = aAddressIid;
    mMeshLocalIid = aMeshLocalIid;
    Update(aRloc16, aTimeSinceLastTransaction);
}

void NdProxyTable::NdProxy::Update(uint16_t aRloc16, uint32_t aTimeSinceLastTransaction)
{
    OT_ASSERT(mValid);

    mRloc16 = aRloc16;
    aTimeSinceLastTransaction =
        OT_MIN(aTimeSinceLastTransaction, static_cast<uint32_t>(Mle::kTimeSinceLastTransactionMax));
    mLastRegistrationTime = TimerMilli::GetNow() - TimeMilli::SecToMsec(aTimeSinceLastTransaction);
}

bool NdProxyTable::MatchesFilter(const NdProxy &aProxy, Filter aFilter)
{
    bool rval = false;

    switch (aFilter)
    {
    case kFilterInvalid:
        rval = !aProxy.mValid;
        break;
    case kFilterValid:
        rval = aProxy.mValid;
        break;
    }

    return rval;
}

NdProxyTable::Iterator::Iterator(Instance &aInstance, Filter aFilter)
    : InstanceLocator(aInstance)
    , mFilter(aFilter)
{
    NdProxyTable &table = GetInstance().Get<BackboneRouter::NdProxyTable>();

    mCurrent = &table.mProxies[0];

    if (!MatchesFilter(*mCurrent, mFilter))
    {
        Advance();
    }
}

NdProxyTable::Iterator::Iterator(Instance &aInstance, NdProxyTable::Iterator::IteratorType)
    : InstanceLocator(aInstance)
{
    NdProxyTable &table = GetInstance().Get<BackboneRouter::NdProxyTable>();
    mCurrent            = OT_ARRAY_END(table.mProxies);
}

void NdProxyTable::Iterator::Advance(void)
{
    NdProxyTable &table = GetInstance().Get<BackboneRouter::NdProxyTable>();

    do
    {
        mCurrent++;
    } while (mCurrent < OT_ARRAY_END(table.mProxies) && !MatchesFilter(*mCurrent, mFilter));
}

void NdProxyTable::Erase(NdProxy &aProxy)
{
    aProxy.Clear();
}

void NdProxyTable::HandleDomainPrefixUpdate(Leader::DomainPrefixState aState)
{
    if (aState == Leader::kDomainPrefixAdded || aState == Leader::kDomainPrefixRemoved ||
        aState == Leader::kDomainPrefixRefreshed)
    {
        Clear();
    }
}

void NdProxyTable::Clear(void)
{
    for (NdProxy &proxy : mProxies)
    {
        proxy.Clear();
    }

    otLogNoteBbr("NdProxyTable::Clear!");
}

otError NdProxyTable::Register(const Ip6::InterfaceIdentifier &aAddressIid,
                               const Ip6::InterfaceIdentifier &aMeshLocalIid,
                               uint16_t                        aRloc16,
                               const uint32_t *                aTimeSinceLastTransaction)
{
    otError  error                    = OT_ERROR_NONE;
    NdProxy *proxy                    = FindByAddressIid(aAddressIid);
    uint32_t timeSinceLastTransaction = aTimeSinceLastTransaction == nullptr ? 0 : *aTimeSinceLastTransaction;

    if (proxy != nullptr)
    {
        VerifyOrExit(proxy->mMeshLocalIid == aMeshLocalIid, error = OT_ERROR_DUPLICATED);

        proxy->Update(aRloc16, timeSinceLastTransaction);
        ExitNow();
    }

    proxy = FindByMeshLocalIid(aMeshLocalIid);
    if (proxy != nullptr)
    {
        Erase(*proxy);
    }
    else
    {
        proxy = FindInvalid();

        // TODO: evict stale DUA entries to have room for this new DUA.
        VerifyOrExit(proxy != nullptr, error = OT_ERROR_NO_BUFS);
    }

    proxy->Init(aAddressIid, aMeshLocalIid, aRloc16, timeSinceLastTransaction);

exit:
    otLogInfoBbr("NdProxyTable::Register %s MLIID %s RLOC16 %04x LTT %u => %s", aAddressIid.ToString().AsCString(),
                 aMeshLocalIid.ToString().AsCString(), aRloc16, timeSinceLastTransaction, otThreadErrorToString(error));
    return error;
}

NdProxyTable::NdProxy *NdProxyTable::FindByAddressIid(const Ip6::InterfaceIdentifier &aAddressIid)
{
    NdProxy *found = nullptr;

    for (NdProxy &proxy : Iterate(kFilterValid))
    {
        if (proxy.mAddressIid == aAddressIid)
        {
            ExitNow(found = &proxy);
        }
    }

exit:
    otLogDebgBbr("NdProxyTable::FindByAddressIid(%s) => %s", aAddressIid.ToString().AsCString(),
                 found ? found->mMeshLocalIid.ToString().AsCString() : "NOT_FOUND");
    return found;
}

NdProxyTable::NdProxy *NdProxyTable::FindByMeshLocalIid(const Ip6::InterfaceIdentifier &aMeshLocalIid)
{
    NdProxy *found = nullptr;

    for (NdProxy &proxy : Iterate(kFilterValid))
    {
        if (proxy.mMeshLocalIid == aMeshLocalIid)
        {
            ExitNow(found = &proxy);
        }
    }

exit:
    otLogDebgBbr("NdProxyTable::FindByMeshLocalIid(%s) => %s", aMeshLocalIid.ToString().AsCString(),
                 found ? found->mAddressIid.ToString().AsCString() : "NOT_FOUND");
    return found;
}

NdProxyTable::NdProxy *NdProxyTable::FindInvalid(void)
{
    NdProxy *found = nullptr;

    for (NdProxy &proxy : Iterate(kFilterInvalid))
    {
        ExitNow(found = &proxy);
    }

exit:
    otLogDebgBbr("NdProxyTable::FindInvalid() => %s", found ? "OK" : "NOT_FOUND");
    return found;
}

} // namespace BackboneRouter

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
