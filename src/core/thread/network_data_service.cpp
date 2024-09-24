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
 */

#include "network_data_service.hpp"

#include "instance/instance.hpp"

namespace ot {
namespace NetworkData {
namespace Service {

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

Error Manager::AddService(const void *aServiceData,
                          uint8_t     aServiceDataLength,
                          const void *aServerData,
                          uint8_t     aServerDataLength)
{
    ServiceData serviceData;
    ServerData  serverData;

    serviceData.Init(aServiceData, aServiceDataLength);
    serverData.Init(aServerData, aServerDataLength);

    return Get<Local>().AddService(kThreadEnterpriseNumber, serviceData, /* aServerStable */ true, serverData);
}

Error Manager::RemoveService(const void *aServiceData, uint8_t aServiceDataLength)
{
    ServiceData serviceData;

    serviceData.Init(aServiceData, aServiceDataLength);

    return Get<Local>().RemoveService(kThreadEnterpriseNumber, serviceData);
}

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

Error Manager::GetServiceId(uint8_t aServiceNumber, uint8_t &aServiceId) const
{
    ServiceData serviceData;

    serviceData.InitFrom(aServiceNumber);

    return Get<Leader>().GetServiceId(kThreadEnterpriseNumber, serviceData, /* aServerStable */ true, aServiceId);
}

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

void Manager::GetBackboneRouterPrimary(ot::BackboneRouter::Config &aConfig) const
{
    const ServerTlv     *rvalServerTlv    = nullptr;
    const BbrServerData *rvalServerData   = nullptr;
    const ServiceTlv    *serviceTlv       = nullptr;
    uint8_t              bbrServiceNumber = kBackboneRouterServiceNumber;
    ServiceData          serviceData;

    serviceData.InitFrom(bbrServiceNumber);

    aConfig.mServer16 = Mle::kInvalidRloc16;

    while ((serviceTlv = Get<Leader>().FindNextThreadService(serviceTlv, serviceData,
                                                             NetworkData::kServicePrefixMatch)) != nullptr)
    {
        Iterator iterator;

        iterator.mServiceTlv = serviceTlv;

        while (IterateToNextServer(iterator) == kErrorNone)
        {
            ServerData           data;
            const BbrServerData *serverData;

            iterator.mServerSubTlv->GetServerData(data);

            if (data.GetLength() < sizeof(BbrServerData))
            {
                continue;
            }

            serverData = reinterpret_cast<const BbrServerData *>(data.GetBytes());

            if (rvalServerTlv == nullptr ||
                IsBackboneRouterPreferredTo(*iterator.mServerSubTlv, *serverData, *rvalServerTlv, *rvalServerData))
            {
                rvalServerTlv  = iterator.mServerSubTlv;
                rvalServerData = serverData;
            }
        }
    }

    VerifyOrExit(rvalServerTlv != nullptr);

    aConfig.mServer16            = rvalServerTlv->GetServer16();
    aConfig.mSequenceNumber      = rvalServerData->GetSequenceNumber();
    aConfig.mReregistrationDelay = rvalServerData->GetReregistrationDelay();
    aConfig.mMlrTimeout          = rvalServerData->GetMlrTimeout();

exit:
    return;
}

bool Manager::IsBackboneRouterPreferredTo(const ServerTlv     &aServerTlv,
                                          const BbrServerData &aServerData,
                                          const ServerTlv     &aOtherServerTlv,
                                          const BbrServerData &aOtherServerData) const
{
    bool     isPreferred;
    uint16_t leaderRloc16 = Get<Mle::MleRouter>().GetLeaderRloc16();

    VerifyOrExit(aServerTlv.GetServer16() != leaderRloc16, isPreferred = true);
    VerifyOrExit(aOtherServerTlv.GetServer16() != leaderRloc16, isPreferred = false);

    isPreferred = aServerData.GetSequenceNumber() > aOtherServerData.GetSequenceNumber() ||
                  (aServerData.GetSequenceNumber() == aOtherServerData.GetSequenceNumber() &&
                   aServerTlv.GetServer16() > aOtherServerTlv.GetServer16());
exit:
    return isPreferred;
}

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

Error Manager::GetNextDnsSrpAnycastInfo(Iterator &aIterator, DnsSrpAnycastInfo &aInfo) const
{
    Error   error         = kErrorNone;
    uint8_t serviceNumber = kDnsSrpAnycastServiceNumber;

    do
    {
        ServiceData serviceData;

        // Process the next Server sub-TLV in the current Service TLV.

        if (IterateToNextServer(aIterator) == kErrorNone)
        {
            aIterator.mServiceTlv->GetServiceData(serviceData);

            if (serviceData.GetLength() >= sizeof(DnsSrpAnycastServiceData))
            {
                const DnsSrpAnycastServiceData *dnsServiceData =
                    reinterpret_cast<const DnsSrpAnycastServiceData *>(serviceData.GetBytes());

                Get<Mle::Mle>().GetServiceAloc(aIterator.mServiceTlv->GetServiceId(), aInfo.mAnycastAddress);
                aInfo.mSequenceNumber = dnsServiceData->GetSequenceNumber();
                aInfo.mRloc16         = aIterator.mServerSubTlv->GetServer16();
                ExitNow();
            }
        }

        // Find the next matching Service TLV.

        serviceData.InitFrom(serviceNumber);
        aIterator.mServiceTlv =
            Get<Leader>().FindNextThreadService(aIterator.mServiceTlv, serviceData, NetworkData::kServicePrefixMatch);
        aIterator.mServerSubTlv = nullptr;

        // If we have a valid Service TLV, restart the loop
        // to process its Server sub-TLVs.

    } while (aIterator.mServiceTlv != nullptr);

    error = kErrorNotFound;

exit:
    return error;
}

Error Manager::FindPreferredDnsSrpAnycastInfo(DnsSrpAnycastInfo &aInfo) const
{
    Error             error = kErrorNotFound;
    Iterator          iterator;
    DnsSrpAnycastInfo info;
    DnsSrpAnycastInfo maxNumericalSeqNumInfo;

    // Determine the entry with largest seq number in two ways:
    // `aInfo` will track the largest using serial number arithmetic
    // comparison, while `maxNumericalSeqNumInfo` tracks the largest
    // using normal numerical comparison.

    while (GetNextDnsSrpAnycastInfo(iterator, info) == kErrorNone)
    {
        if (error == kErrorNotFound)
        {
            aInfo                  = info;
            maxNumericalSeqNumInfo = info;
            error                  = kErrorNone;
            continue;
        }

        if (SerialNumber::IsGreater(info.mSequenceNumber, aInfo.mSequenceNumber))
        {
            aInfo = info;
        }

        if (info.mSequenceNumber > maxNumericalSeqNumInfo.mSequenceNumber)
        {
            maxNumericalSeqNumInfo = info;
        }
    }

    SuccessOrExit(error);

    // Check that the largest seq number using serial number arithmetic is
    // well-defined (i.e., the seq number is larger than all other seq
    // numbers values). If it is not, we prefer `maxNumericalSeqNumInfo`.

    iterator.Reset();

    while (GetNextDnsSrpAnycastInfo(iterator, info) == kErrorNone)
    {
        constexpr uint8_t kMidValue = (NumericLimits<uint8_t>::kMax / 2) + 1;
        uint8_t           seqNumber = info.mSequenceNumber;
        uint8_t           diff;

        if (seqNumber == aInfo.mSequenceNumber)
        {
            continue;
        }

        diff = seqNumber - aInfo.mSequenceNumber;

        if ((diff == kMidValue) || !SerialNumber::IsGreater(aInfo.mSequenceNumber, seqNumber))
        {
            aInfo = maxNumericalSeqNumInfo;

            break;
        }
    }

exit:
    return error;
}

Error Manager::GetNextDnsSrpUnicastInfo(Iterator &aIterator, DnsSrpUnicastType aType, DnsSrpUnicastInfo &aInfo) const
{
    Error   error         = kErrorNone;
    uint8_t serviceNumber = kDnsSrpUnicastServiceNumber;

    do
    {
        ServiceData serviceData;

        // Process Server sub-TLVs in the current Service TLV.

        while (IterateToNextServer(aIterator) == kErrorNone)
        {
            ServerData data;

            if (aType == kAddrInServiceData)
            {
                const DnsSrpUnicast::ServiceData *dnsServiceData;

                if (aIterator.mServiceTlv->GetServiceDataLength() < sizeof(DnsSrpUnicast::ServiceData))
                {
                    // Break from `while(IterateToNextServer())` loop
                    // to skip over the Service TLV and all its
                    // sub-TLVs and go to the next one.
                    break;
                }

                aIterator.mServiceTlv->GetServiceData(serviceData);
                dnsServiceData = reinterpret_cast<const DnsSrpUnicast::ServiceData *>(serviceData.GetBytes());
                aInfo.mSockAddr.SetAddress(dnsServiceData->GetAddress());
                aInfo.mSockAddr.SetPort(dnsServiceData->GetPort());
                aInfo.mRloc16 = aIterator.mServerSubTlv->GetServer16();
                ExitNow();
            }

            // `aType` is `kAddrInServerData`.

            // Server sub-TLV can contain address and port info
            // (then we parse and return the info), or it can be
            // empty (then we skip over it).

            aIterator.mServerSubTlv->GetServerData(data);

            if (data.GetLength() >= sizeof(DnsSrpUnicast::ServerData))
            {
                const DnsSrpUnicast::ServerData *serverData =
                    reinterpret_cast<const DnsSrpUnicast::ServerData *>(data.GetBytes());

                aInfo.mSockAddr.SetAddress(serverData->GetAddress());
                aInfo.mSockAddr.SetPort(serverData->GetPort());
                aInfo.mRloc16 = aIterator.mServerSubTlv->GetServer16();
                ExitNow();
            }

            if (data.GetLength() == sizeof(uint16_t))
            {
                // Handle the case where the server TLV data only
                // contains a port number and use the RLOC as the
                // IPv6 address.
                aInfo.mSockAddr.GetAddress().SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(),
                                                                 aIterator.mServerSubTlv->GetServer16());
                aInfo.mSockAddr.SetPort(BigEndian::ReadUint16(data.GetBytes()));
                aInfo.mRloc16 = aIterator.mServerSubTlv->GetServer16();
                ExitNow();
            }
        }

        // Find the next matching Service TLV.

        serviceData.InitFrom(serviceNumber);
        aIterator.mServiceTlv =
            Get<Leader>().FindNextThreadService(aIterator.mServiceTlv, serviceData, NetworkData::kServicePrefixMatch);
        aIterator.mServerSubTlv = nullptr;

        // If we have a valid Service TLV, restart the loop
        // to process its Server sub-TLVs.

    } while (aIterator.mServiceTlv != nullptr);

    error = kErrorNotFound;

exit:
    return error;
}

Error Manager::IterateToNextServer(Iterator &aIterator) const
{
    Error error = kErrorNotFound;

    VerifyOrExit(aIterator.mServiceTlv != nullptr);

    aIterator.mServerSubTlv = NetworkDataTlv::Find<ServerTlv>(
        /* aStart */ (aIterator.mServerSubTlv != nullptr) ? aIterator.mServerSubTlv->GetNext()
                                                          : aIterator.mServiceTlv->GetSubTlvs(),
        /* aEnd */ aIterator.mServiceTlv->GetNext());

    if (aIterator.mServerSubTlv != nullptr)
    {
        error = kErrorNone;
    }

exit:
    return error;
}

} // namespace Service
} // namespace NetworkData
} // namespace ot
