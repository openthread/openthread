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
 *   This file implements the local Thread Network Data.
 */

#include "network_data_local.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "mac/mac_types.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

namespace ot {
namespace NetworkData {

Local::Local(Instance &aInstance)
    : NetworkData(aInstance, kTypeLocal)
    , mOldRloc(Mac::kShortAddrInvalid)
{
}

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
otError Local::AddOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, uint8_t aFlags, bool aStable)
{
    otError          error             = OT_ERROR_NONE;
    uint8_t          prefixLengthBytes = BitVectorBytes(aPrefixLength);
    uint8_t          appendLength;
    PrefixTlv *      prefixTlv;
    BorderRouterTlv *brTlv;

    VerifyOrExit(prefixLengthBytes <= sizeof(Ip6::Address), error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit((aPrf == OT_ROUTE_PREFERENCE_LOW) || (aPrf == OT_ROUTE_PREFERENCE_MED) ||
                     (aPrf == OT_ROUTE_PREFERENCE_HIGH),
                 error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(Ip6::Address::PrefixMatch(aPrefix, Get<Mle::MleRouter>().GetMeshLocalPrefix().m8, prefixLengthBytes) <
                     Ip6::Address::kMeshLocalPrefixLength,
                 error = OT_ERROR_INVALID_ARGS);

    RemoveOnMeshPrefix(aPrefix, aPrefixLength);

    appendLength = sizeof(PrefixTlv) + prefixLengthBytes + sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry);
    VerifyOrExit(mLength + appendLength <= sizeof(mTlvs), error = OT_ERROR_NO_BUFS);

    prefixTlv = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
    Insert(reinterpret_cast<uint8_t *>(prefixTlv), appendLength);
    prefixTlv->Init(0, aPrefixLength, aPrefix);
    prefixTlv->SetSubTlvsLength(sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry));

    brTlv = static_cast<BorderRouterTlv *>(prefixTlv->GetSubTlvs());
    brTlv->Init();
    brTlv->SetLength(brTlv->GetLength() + sizeof(BorderRouterEntry));
    brTlv->GetEntry(0)->Init();
    brTlv->GetEntry(0)->SetPreference(aPrf);
    brTlv->GetEntry(0)->SetFlags(aFlags);

    if (aStable)
    {
        prefixTlv->SetStable();
        brTlv->SetStable();
    }

    ClearResubmitDelayTimer();

    otDumpDebgNetData("add prefix done", mTlvs, mLength);

exit:
    return error;
}

otError Local::RemoveOnMeshPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    otError    error = OT_ERROR_NONE;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(FindBorderRouter(*tlv) != NULL, error = OT_ERROR_NOT_FOUND);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());
    ClearResubmitDelayTimer();

exit:
    otDumpDebgNetData("remove done", mTlvs, mLength);
    return error;
}

otError Local::AddHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, int8_t aPrf, bool aStable)
{
    otError      error             = OT_ERROR_NONE;
    uint8_t      prefixLengthBytes = BitVectorBytes(aPrefixLength);
    PrefixTlv *  prefixTlv;
    HasRouteTlv *hasRouteTlv;
    uint8_t      appendLength;

    VerifyOrExit(prefixLengthBytes <= sizeof(Ip6::Address), error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit((aPrf == OT_ROUTE_PREFERENCE_LOW) || (aPrf == OT_ROUTE_PREFERENCE_MED) ||
                     (aPrf == OT_ROUTE_PREFERENCE_HIGH),
                 error = OT_ERROR_INVALID_ARGS);

    RemoveHasRoutePrefix(aPrefix, aPrefixLength);

    appendLength = sizeof(PrefixTlv) + prefixLengthBytes + sizeof(HasRouteTlv) + sizeof(HasRouteEntry);
    VerifyOrExit(mLength + appendLength <= sizeof(mTlvs), error = OT_ERROR_NO_BUFS);

    prefixTlv = reinterpret_cast<PrefixTlv *>(mTlvs + mLength);
    Insert(reinterpret_cast<uint8_t *>(prefixTlv), appendLength);
    prefixTlv->Init(0, aPrefixLength, aPrefix);
    prefixTlv->SetSubTlvsLength(sizeof(HasRouteTlv) + sizeof(HasRouteEntry));

    hasRouteTlv = static_cast<HasRouteTlv *>(prefixTlv->GetSubTlvs());
    hasRouteTlv->Init();
    hasRouteTlv->SetLength(hasRouteTlv->GetLength() + sizeof(HasRouteEntry));
    hasRouteTlv->GetEntry(0)->Init();
    hasRouteTlv->GetEntry(0)->SetPreference(aPrf);

    if (aStable)
    {
        prefixTlv->SetStable();
        hasRouteTlv->SetStable();
    }

    ClearResubmitDelayTimer();

    otDumpDebgNetData("add route done", mTlvs, mLength);

exit:
    return error;
}

otError Local::RemoveHasRoutePrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    otError    error = OT_ERROR_NONE;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix, aPrefixLength)) != NULL, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(FindHasRoute(*tlv) != NULL, error = OT_ERROR_NOT_FOUND);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());
    ClearResubmitDelayTimer();

exit:
    otDumpDebgNetData("remove done", mTlvs, mLength);
    return error;
}

void Local::UpdateRloc(PrefixTlv &aPrefix)
{
    for (NetworkDataTlv *cur = aPrefix.GetSubTlvs(); cur < aPrefix.GetNext(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            UpdateRloc(*static_cast<HasRouteTlv *>(cur));
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            UpdateRloc(*static_cast<BorderRouterTlv *>(cur));
            break;

        default:
            assert(false);
            break;
        }
    }
}

void Local::UpdateRloc(HasRouteTlv &aHasRoute)
{
    HasRouteEntry *entry = aHasRoute.GetEntry(0);
    entry->SetRloc(Get<Mle::MleRouter>().GetRloc16());
}

void Local::UpdateRloc(BorderRouterTlv &aBorderRouter)
{
    BorderRouterEntry *entry = aBorderRouter.GetEntry(0);
    entry->SetRloc(Get<Mle::MleRouter>().GetRloc16());
}

bool Local::IsOnMeshPrefixConsistent(void)
{
    return (Get<Leader>().ContainsOnMeshPrefixes(*this, Get<Mle::MleRouter>().GetRloc16()) &&
            ContainsOnMeshPrefixes(Get<Leader>(), Get<Mle::MleRouter>().GetRloc16()));
}

bool Local::IsExternalRouteConsistent(void)
{
    return (Get<Leader>().ContainsExternalRoutes(*this, Get<Mle::MleRouter>().GetRloc16()) &&
            ContainsExternalRoutes(Get<Leader>(), Get<Mle::MleRouter>().GetRloc16()));
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
otError Local::AddService(uint32_t       aEnterpriseNumber,
                          const uint8_t *aServiceData,
                          uint8_t        aServiceDataLength,
                          bool           aServerStable,
                          const uint8_t *aServerData,
                          uint8_t        aServerDataLength)
{
    otError     error = OT_ERROR_NONE;
    ServiceTlv *serviceTlv;
    ServerTlv * serverTlv;
    size_t      serviceTlvLength =
        (sizeof(ServiceTlv) - sizeof(NetworkDataTlv)) + aServiceDataLength + sizeof(uint8_t) /*mServiceDataLength*/ +
        ServiceTlv::GetEnterpriseNumberFieldLength(aEnterpriseNumber) + aServerDataLength + sizeof(ServerTlv);

    RemoveService(aEnterpriseNumber, aServiceData, aServiceDataLength);

    VerifyOrExit(mLength + sizeof(NetworkDataTlv) + serviceTlvLength <= sizeof(mTlvs), error = OT_ERROR_NO_BUFS);

    serviceTlv = reinterpret_cast<ServiceTlv *>(mTlvs + mLength);
    Insert(reinterpret_cast<uint8_t *>(serviceTlv), static_cast<uint8_t>(serviceTlvLength) + sizeof(NetworkDataTlv));

    serviceTlv->Init();
    serviceTlv->SetEnterpriseNumber(aEnterpriseNumber);
    serviceTlv->SetServiceID(0);
    serviceTlv->SetServiceData(aServiceData, aServiceDataLength);
    serviceTlv->SetLength(static_cast<uint8_t>(serviceTlvLength));

    serverTlv = static_cast<ServerTlv *>(serviceTlv->GetSubTlvs());
    serverTlv->Init();

    // According to Thread spec 1.1.1, section 5.18.6 Service TLV:
    // "The Stable flag is set if any of the included sub-TLVs have their Stable flag set."
    // The meaning also seems to be 'if and only if'.
    if (aServerStable)
    {
        serviceTlv->SetStable();
        serverTlv->SetStable();
    }

    serverTlv->SetServer16(Get<Mle::MleRouter>().GetRloc16());
    serverTlv->SetServerData(aServerData, aServerDataLength);

    ClearResubmitDelayTimer();

    otDumpDebgNetData("add service done", mTlvs, mLength);

exit:
    return error;
}

otError Local::RemoveService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength)
{
    otError     error = OT_ERROR_NONE;
    ServiceTlv *tlv;

    VerifyOrExit((tlv = FindService(aEnterpriseNumber, aServiceData, aServiceDataLength)) != NULL,
                 error = OT_ERROR_NOT_FOUND);
    Remove(reinterpret_cast<uint8_t *>(tlv), sizeof(NetworkDataTlv) + tlv->GetLength());
    ClearResubmitDelayTimer();

exit:
    otDumpDebgNetData("remove service done", mTlvs, mLength);
    return error;
}

void Local::UpdateRloc(ServiceTlv &aService)
{
    for (NetworkDataTlv *cur = aService.GetSubTlvs(); cur < aService.GetNext(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeServer:
            UpdateRloc(*static_cast<ServerTlv *>(cur));
            break;

        default:
            assert(false);
            break;
        }
    }
}

void Local::UpdateRloc(ServerTlv &aServer)
{
    aServer.SetServer16(Get<Mle::MleRouter>().GetRloc16());
}

bool Local::IsServiceConsistent(void)
{
    return (Get<Leader>().ContainsServices(*this, Get<Mle::MleRouter>().GetRloc16()) &&
            ContainsServices(Get<Leader>(), Get<Mle::MleRouter>().GetRloc16()));
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

void Local::UpdateRloc(void)
{
    for (NetworkDataTlv *cur                                            = reinterpret_cast<NetworkDataTlv *>(mTlvs);
         cur < reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
        case NetworkDataTlv::kTypePrefix:
            UpdateRloc(*static_cast<PrefixTlv *>(cur));
            break;
#endif

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

        case NetworkDataTlv::kTypeService:
            UpdateRloc(*static_cast<ServiceTlv *>(cur));
            break;
#endif

        default:
            assert(false);
            break;
        }
    }

    ClearResubmitDelayTimer();
}

otError Local::SendServerDataNotification(void)
{
    otError  error        = OT_ERROR_NONE;
    uint16_t rloc         = Get<Mle::MleRouter>().GetRloc16();
    bool     isConsistent = true;

#if OPENTHREAD_FTD

    // Don't send this Server Data Notification if the device is going to upgrade to Router
    if (Get<Mle::MleRouter>().IsRouterEligible() && (Get<Mle::MleRouter>().GetRole() < OT_DEVICE_ROLE_ROUTER) &&
        (Get<RouterTable>().GetActiveRouterCount() < Get<Mle::MleRouter>().GetRouterUpgradeThreshold()))
    {
        ExitNow(error = OT_ERROR_INVALID_STATE);
    }

#endif

    UpdateRloc();

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    isConsistent = isConsistent && IsOnMeshPrefixConsistent() && IsExternalRouteConsistent();
#endif
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    isConsistent = isConsistent && IsServiceConsistent();
#endif

    VerifyOrExit(!isConsistent, ClearResubmitDelayTimer());

    if (mOldRloc == rloc)
    {
        mOldRloc = Mac::kShortAddrInvalid;
    }

    SuccessOrExit(error = NetworkData::SendServerDataNotification(mOldRloc));
    mOldRloc = rloc;

exit:
    return error;
}

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
