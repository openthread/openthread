/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements function for managing Thread Network Data service/server entries.
 *
 */

#include "network_data_service.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "thread/network_data_local.hpp"

namespace ot {
namespace NetworkData {
namespace Service {

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

otError Manager::AddService(uint8_t     aServiceNumber,
                            bool        aServerStable,
                            const void *aServerData,
                            uint8_t     aServerDataLength)
{
    otError error;

    SuccessOrExit(error = Get<Local>().AddService(kThreadEnterpriseNumber, &aServiceNumber, sizeof(aServiceNumber),
                                                  aServerStable, reinterpret_cast<const uint8_t *>(aServerData),
                                                  aServerDataLength));

    Get<Notifier>().HandleServerDataUpdated();

exit:
    return error;
}

otError Manager::RemoveService(uint8_t aServiceNumber)
{
    otError error;

    SuccessOrExit(error = Get<Local>().RemoveService(kThreadEnterpriseNumber, &aServiceNumber, sizeof(aServiceNumber)));
    Get<Notifier>().HandleServerDataUpdated();

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

otError Manager::GetServiceId(uint8_t aServiceNumber, bool aServerStable, uint8_t &aServiceId) const
{
    return Get<Leader>().GetServiceId(kThreadEnterpriseNumber, &aServiceNumber, sizeof(aServiceNumber), aServerStable,
                                      aServiceId);
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

otError Manager::GetBackboneRouterPrimary(ot::BackboneRouter::BackboneRouterConfig &aConfig) const
{
    otError                           error          = OT_ERROR_NOT_FOUND;
    const uint8_t                     serviceData    = BackboneRouter::kServiceNumber;
    const ServerTlv *                 rvalServerTlv  = nullptr;
    const BackboneRouter::ServerData *rvalServerData = nullptr;
    Iterator                          iterator;

    iterator.mServiceTlv = Get<Leader>().FindService(kThreadEnterpriseNumber, &serviceData, sizeof(serviceData));

    VerifyOrExit(iterator.mServiceTlv != nullptr, aConfig.mServer16 = Mac::kShortAddrInvalid);

    while (IterateToNextServer(iterator) == OT_ERROR_NONE)
    {
        const BackboneRouter::ServerData *serverData;

        if (iterator.mServerSubTlv->GetServerDataLength() < sizeof(BackboneRouter::ServerData))
        {
            continue;
        }

        serverData = reinterpret_cast<const BackboneRouter::ServerData *>(iterator.mServerSubTlv->GetServerData());

        if (rvalServerTlv == nullptr ||
            (iterator.mServerSubTlv->GetServer16() ==
             Mle::Mle::Rloc16FromRouterId(Get<Mle::MleRouter>().GetLeaderId())) ||
            serverData->GetSequenceNumber() > rvalServerData->GetSequenceNumber() ||
            (serverData->GetSequenceNumber() == rvalServerData->GetSequenceNumber() &&
             iterator.mServerSubTlv->GetServer16() > rvalServerTlv->GetServer16()))
        {
            rvalServerTlv  = iterator.mServerSubTlv;
            rvalServerData = serverData;
        }
    }

    VerifyOrExit(rvalServerTlv != nullptr);

    aConfig.mServer16            = rvalServerTlv->GetServer16();
    aConfig.mSequenceNumber      = rvalServerData->GetSequenceNumber();
    aConfig.mReregistrationDelay = rvalServerData->GetReregistrationDelay();
    aConfig.mMlrTimeout          = rvalServerData->GetMlrTimeout();

    error = OT_ERROR_NONE;

exit:
    return error;
}

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

otError Manager::GetNextSrpServerInfo(Iterator &aIterator, SrpServer::Info &aInfo) const
{
    otError error = OT_ERROR_NOT_FOUND;

    if (aIterator.mServiceTlv == nullptr)
    {
        const uint8_t serviceData = SrpServer::kServiceNumber;

        aIterator.mServiceTlv = Get<Leader>().FindService(kThreadEnterpriseNumber, &serviceData, sizeof(serviceData));
        VerifyOrExit(aIterator.mServiceTlv != nullptr);
    }
    else
    {
        VerifyOrExit(aIterator.mServerSubTlv != nullptr);
    }

    while ((error = IterateToNextServer(aIterator)) == OT_ERROR_NONE)
    {
        const SrpServer::ServerData *serverData;

        if (aIterator.mServerSubTlv->GetServerDataLength() < sizeof(SrpServer::ServerData))
        {
            continue;
        }

        serverData = reinterpret_cast<const SrpServer::ServerData *>(aIterator.mServerSubTlv->GetServerData());

        aInfo.mRloc16 = aIterator.mServerSubTlv->GetServer16();
        aInfo.mSockAddr.GetAddress().SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), aInfo.mRloc16);
        aInfo.mSockAddr.SetPort(serverData->GetPort());

        break;
    }

exit:
    return error;
}

otError Manager::IterateToNextServer(Iterator &aIterator) const
{
    const NetworkDataTlv *start;

    start =
        (aIterator.mServerSubTlv != nullptr) ? aIterator.mServerSubTlv->GetNext() : aIterator.mServiceTlv->GetSubTlvs();
    aIterator.mServerSubTlv = NetworkData::FindTlv<ServerTlv>(start, aIterator.mServiceTlv->GetNext());

    return (aIterator.mServerSubTlv != nullptr) ? OT_ERROR_NONE : OT_ERROR_NOT_FOUND;
}

} // namespace Service
} // namespace NetworkData
} // namespace ot
