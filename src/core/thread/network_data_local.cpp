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
#include "thread/mle_types.hpp"
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
otError Local::AddOnMeshPrefix(const OnMeshPrefixConfig &aConfig)
{
    uint16_t flags = 0;

    if (aConfig.mPreferred)
    {
        flags |= BorderRouterEntry::kPreferredFlag;
    }

    if (aConfig.mSlaac)
    {
        flags |= BorderRouterEntry::kSlaacFlag;
    }

    if (aConfig.mDhcp)
    {
        flags |= BorderRouterEntry::kDhcpFlag;
    }

    if (aConfig.mConfigure)
    {
        flags |= BorderRouterEntry::kConfigureFlag;
    }

    if (aConfig.mDefaultRoute)
    {
        flags |= BorderRouterEntry::kDefaultRouteFlag;
    }

    if (aConfig.mOnMesh)
    {
        flags |= BorderRouterEntry::kOnMeshFlag;
    }

    if (aConfig.mNdDns)
    {
        flags |= BorderRouterEntry::kNdDnsFlag;
    }

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    if (aConfig.mDp)
    {
        flags |= BorderRouterEntry::kDpFlag;
    }
#endif

    return AddPrefix(aConfig.GetPrefix(), NetworkDataTlv::kTypeBorderRouter, aConfig.mPreference, flags,
                     aConfig.mStable);
}

otError Local::RemoveOnMeshPrefix(const Ip6::Prefix &aPrefix)
{
    return RemovePrefix(aPrefix, NetworkDataTlv::kTypeBorderRouter);
}

otError Local::AddHasRoutePrefix(const ExternalRouteConfig &aConfig)
{
    return AddPrefix(aConfig.GetPrefix(), NetworkDataTlv::kTypeHasRoute, aConfig.mPreference, /* aFlags */ 0,
                     aConfig.mStable);
}

otError Local::RemoveHasRoutePrefix(const Ip6::Prefix &aPrefix)
{
    return RemovePrefix(aPrefix, NetworkDataTlv::kTypeHasRoute);
}

otError Local::AddPrefix(const Ip6::Prefix &  aPrefix,
                         NetworkDataTlv::Type aSubTlvType,
                         int8_t               aPrf,
                         uint16_t             aFlags,
                         bool                 aStable)
{
    otError    error = OT_ERROR_NONE;
    uint8_t    subTlvLength;
    PrefixTlv *prefixTlv;

    VerifyOrExit((aPrefix.GetLength() > 0) && aPrefix.IsValid(), error = OT_ERROR_INVALID_ARGS);

    switch (aPrf)
    {
    case OT_ROUTE_PREFERENCE_LOW:
    case OT_ROUTE_PREFERENCE_MED:
    case OT_ROUTE_PREFERENCE_HIGH:
        break;
    default:
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    VerifyOrExit(!aPrefix.ContainsPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix()), error = OT_ERROR_INVALID_ARGS);

    IgnoreError(RemovePrefix(aPrefix, aSubTlvType));

    subTlvLength = (aSubTlvType == NetworkDataTlv::kTypeBorderRouter)
                       ? sizeof(BorderRouterTlv) + sizeof(BorderRouterEntry)
                       : sizeof(HasRouteTlv) + sizeof(HasRouteEntry);

    prefixTlv = static_cast<PrefixTlv *>(AppendTlv(sizeof(PrefixTlv) + aPrefix.GetBytesSize() + subTlvLength));
    VerifyOrExit(prefixTlv != nullptr, error = OT_ERROR_NO_BUFS);

    prefixTlv->Init(0, aPrefix);
    prefixTlv->SetSubTlvsLength(subTlvLength);

    if (aSubTlvType == NetworkDataTlv::kTypeBorderRouter)
    {
        BorderRouterTlv *brTlv = static_cast<BorderRouterTlv *>(prefixTlv->GetSubTlvs());
        brTlv->Init();
        brTlv->SetLength(brTlv->GetLength() + sizeof(BorderRouterEntry));
        brTlv->GetEntry(0)->Init();
        brTlv->GetEntry(0)->SetPreference(aPrf);
        brTlv->GetEntry(0)->SetFlags(aFlags);
    }
    else // aSubTlvType is NetworkDataTlv::kTypeHasRoute
    {
        HasRouteTlv *hasRouteTlv = static_cast<HasRouteTlv *>(prefixTlv->GetSubTlvs());
        hasRouteTlv->Init();
        hasRouteTlv->SetLength(hasRouteTlv->GetLength() + sizeof(HasRouteEntry));
        hasRouteTlv->GetEntry(0)->Init();
        hasRouteTlv->GetEntry(0)->SetPreference(aPrf);
    }

    if (aStable)
    {
        prefixTlv->SetStable();
        prefixTlv->GetSubTlvs()->SetStable();
    }

    otDumpDebgNetData("add prefix done", mTlvs, mLength);

exit:
    return error;
}

otError Local::RemovePrefix(const Ip6::Prefix &aPrefix, NetworkDataTlv::Type aSubTlvType)
{
    otError    error = OT_ERROR_NONE;
    PrefixTlv *tlv;

    VerifyOrExit((tlv = FindPrefix(aPrefix)) != nullptr, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(FindTlv(tlv->GetSubTlvs(), tlv->GetNext(), aSubTlvType) != nullptr, error = OT_ERROR_NOT_FOUND);
    RemoveTlv(tlv);

exit:
    otDumpDebgNetData("remove done", mTlvs, mLength);
    return error;
}

void Local::UpdateRloc(PrefixTlv &aPrefixTlv)
{
    uint16_t rloc16 = Get<Mle::MleRouter>().GetRloc16();

    for (NetworkDataTlv *cur = aPrefixTlv.GetSubTlvs(); cur < aPrefixTlv.GetNext(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeHasRoute:
            static_cast<HasRouteTlv *>(cur)->GetEntry(0)->SetRloc(rloc16);
            break;

        case NetworkDataTlv::kTypeBorderRouter:
            static_cast<BorderRouterTlv *>(cur)->GetEntry(0)->SetRloc(rloc16);
            break;

        default:
            OT_ASSERT(false);
            OT_UNREACHABLE_CODE(break);
        }
    }
}

bool Local::IsOnMeshPrefixConsistent(void) const
{
    return (Get<Leader>().ContainsOnMeshPrefixes(*this, Get<Mle::MleRouter>().GetRloc16()) &&
            ContainsOnMeshPrefixes(Get<Leader>(), Get<Mle::MleRouter>().GetRloc16()));
}

bool Local::IsExternalRouteConsistent(void) const
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
    uint16_t    serviceTlvSize =
        ServiceTlv::CalculateSize(aEnterpriseNumber, aServiceDataLength) + sizeof(ServerTlv) + aServerDataLength;

    IgnoreError(RemoveService(aEnterpriseNumber, aServiceData, aServiceDataLength));

    VerifyOrExit(serviceTlvSize <= kMaxSize, error = OT_ERROR_NO_BUFS);

    serviceTlv = static_cast<ServiceTlv *>(AppendTlv(serviceTlvSize));
    VerifyOrExit(serviceTlv != nullptr, error = OT_ERROR_NO_BUFS);

    serviceTlv->Init(/* aServiceId */ 0, aEnterpriseNumber, aServiceData, aServiceDataLength);
    serviceTlv->SetSubTlvsLength(sizeof(ServerTlv) + aServerDataLength);

    serverTlv = static_cast<ServerTlv *>(serviceTlv->GetSubTlvs());

    serverTlv->Init(Get<Mle::MleRouter>().GetRloc16(), aServerData, aServerDataLength);

    // According to Thread spec 1.1.1, section 5.18.6 Service TLV:
    // "The Stable flag is set if any of the included sub-TLVs have their Stable flag set."
    // The meaning also seems to be 'if and only if'.
    if (aServerStable)
    {
        serviceTlv->SetStable();
        serverTlv->SetStable();
    }

    otDumpDebgNetData("add service done", mTlvs, mLength);

exit:
    return error;
}

otError Local::RemoveService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength)
{
    otError     error = OT_ERROR_NONE;
    ServiceTlv *tlv;

    VerifyOrExit((tlv = FindService(aEnterpriseNumber, aServiceData, aServiceDataLength)) != nullptr,
                 error = OT_ERROR_NOT_FOUND);
    RemoveTlv(tlv);

exit:
    otDumpDebgNetData("remove service done", mTlvs, mLength);
    return error;
}

void Local::UpdateRloc(ServiceTlv &aService)
{
    uint16_t rloc16 = Get<Mle::MleRouter>().GetRloc16();

    for (NetworkDataTlv *cur = aService.GetSubTlvs(); cur < aService.GetNext(); cur = cur->GetNext())
    {
        switch (cur->GetType())
        {
        case NetworkDataTlv::kTypeServer:
            static_cast<ServerTlv *>(cur)->SetServer16(rloc16);
            break;

        default:
            OT_ASSERT(false);
            OT_UNREACHABLE_CODE(break);
        }
    }
}

bool Local::IsServiceConsistent(void) const
{
    return (Get<Leader>().ContainsServices(*this, Get<Mle::MleRouter>().GetRloc16()) &&
            ContainsServices(Get<Leader>(), Get<Mle::MleRouter>().GetRloc16()));
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

void Local::UpdateRloc(void)
{
    for (NetworkDataTlv *cur = GetTlvsStart(); cur < GetTlvsEnd(); cur = cur->GetNext())
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
            OT_ASSERT(false);
            OT_UNREACHABLE_CODE(break);
        }
    }
}

otError Local::UpdateInconsistentServerData(Coap::ResponseHandler aHandler, void *aContext)
{
    otError  error        = OT_ERROR_NONE;
    uint16_t rloc         = Get<Mle::MleRouter>().GetRloc16();
    bool     isConsistent = true;

#if OPENTHREAD_FTD

    // Don't send this Server Data Notification if the device is going to upgrade to Router
    if (Get<Mle::MleRouter>().IsExpectedToBecomeRouter())
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

    VerifyOrExit(!isConsistent, error = OT_ERROR_NOT_FOUND);

    if (mOldRloc == rloc)
    {
        mOldRloc = Mac::kShortAddrInvalid;
    }

    SuccessOrExit(error = SendServerDataNotification(mOldRloc, aHandler, aContext));
    mOldRloc = rloc;

exit:
    return error;
}

} // namespace NetworkData
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE || OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
