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

Iterator::Iterator(Instance &aInstance)
    : Iterator(aInstance, aInstance.Get<Leader>())
{
}

Iterator::Iterator(Instance &aInstance, const NetworkData &aNetworkData)
    : InstanceLocator(aInstance)
    , mNetworkData(aNetworkData)
    , mServiceTlv(nullptr)
    , mServerSubTlv(nullptr)
{
}

void Iterator::Reset(void)
{
    mServiceTlv   = nullptr;
    mServerSubTlv = nullptr;
}

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

Error Manager::AddDnsSrpAnycastService(uint8_t aSequenceNumber, uint8_t aVersion)
{
    DnsSrpAnycastServiceData anycastData(aSequenceNumber);

    return (aVersion == 0) ? AddService(anycastData) : AddService(anycastData, aVersion);
}

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
        Iterator iterator(GetInstance());

        iterator.mServiceTlv = serviceTlv;

        while (iterator.AdvanceToNextServer() == kErrorNone)
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
    uint16_t leaderRloc16 = Get<Mle::Mle>().GetLeaderRloc16();

    VerifyOrExit(aServerTlv.GetServer16() != leaderRloc16, isPreferred = true);
    VerifyOrExit(aOtherServerTlv.GetServer16() != leaderRloc16, isPreferred = false);

    isPreferred = aServerData.GetSequenceNumber() > aOtherServerData.GetSequenceNumber() ||
                  (aServerData.GetSequenceNumber() == aOtherServerData.GetSequenceNumber() &&
                   aServerTlv.GetServer16() > aOtherServerTlv.GetServer16());
exit:
    return isPreferred;
}

#endif // (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

Error Iterator::GetNextDnsSrpAnycastInfo(DnsSrpAnycastInfo &aInfo)
{
    Error   error         = kErrorNone;
    uint8_t serviceNumber = Manager::kDnsSrpAnycastServiceNumber;

    aInfo.Clear();

    do
    {
        ServiceData serviceData;

        // Process the next Server sub-TLV in the current Service TLV.

        if (AdvanceToNextServer() == kErrorNone)
        {
            uint8_t dataLength = mServiceTlv->GetServiceDataLength();

            if (dataLength >= sizeof(Manager::DnsSrpAnycastServiceData))
            {
                const Manager::DnsSrpAnycastServiceData *anycastData =
                    reinterpret_cast<const Manager::DnsSrpAnycastServiceData *>(mServiceTlv->GetServiceData());

                Get<Mle::Mle>().GetServiceAloc(mServiceTlv->GetServiceId(), aInfo.mAnycastAddress);
                aInfo.mSequenceNumber = anycastData->GetSequenceNumber();
                aInfo.mRloc16         = mServerSubTlv->GetServer16();
                aInfo.mVersion =
                    (mServerSubTlv->GetServerDataLength() >= sizeof(uint8_t)) ? *mServerSubTlv->GetServerData() : 0;
                ExitNow();
            }
        }

        // Find the next matching Service TLV.

        serviceData.InitFrom(serviceNumber);
        mServiceTlv   = mNetworkData.FindNextThreadService(mServiceTlv, serviceData, NetworkData::kServicePrefixMatch);
        mServerSubTlv = nullptr;

        // If we have a valid Service TLV, restart the loop
        // to process its Server sub-TLVs.

    } while (mServiceTlv != nullptr);

    error = kErrorNotFound;

exit:
    return error;
}

Error Manager::FindPreferredDnsSrpAnycastInfo(DnsSrpAnycastInfo &aInfo) const
{
    Error             error = kErrorNotFound;
    Iterator          iterator(GetInstance());
    DnsSrpAnycastInfo info;
    DnsSrpAnycastInfo maxNumericalSeqNumInfo;

    aInfo.Clear();

    // Determine the entry with largest seq number in two ways:
    // `aInfo` will track the largest using serial number arithmetic
    // comparison, while `maxNumericalSeqNumInfo` tracks the largest
    // using normal numerical comparison.

    while (iterator.GetNextDnsSrpAnycastInfo(info) == kErrorNone)
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

    while (iterator.GetNextDnsSrpAnycastInfo(info) == kErrorNone)
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

    // Determine the minimum version supported among all entries
    // matching the selected `aInfo.mSequenceNumber`.

    iterator.Reset();

    while (iterator.GetNextDnsSrpAnycastInfo(info) == kErrorNone)
    {
        if (info.mSequenceNumber == aInfo.mSequenceNumber)
        {
            aInfo.mVersion = Min(aInfo.mVersion, info.mVersion);
        }
    }

exit:
    return error;
}

Error Manager::DnsSrpUnicast::AddrData::ParseFrom(const uint8_t *aData, uint8_t aLength, DnsSrpUnicastInfo &aInfo)
{
    Error           error    = kErrorNone;
    const AddrData *addrData = reinterpret_cast<const AddrData *>(aData);

    VerifyOrExit(aLength >= kMinLength, error = kErrorParse);

    aInfo.mSockAddr.SetAddress(addrData->GetAddress());
    aInfo.mSockAddr.SetPort(addrData->GetPort());
    aInfo.mVersion = (aLength >= sizeof(AddrData)) ? addrData->GetVersion() : 0;

exit:
    return error;
}

Error Iterator::GetNextDnsSrpUnicastInfo(DnsSrpUnicastType aType, DnsSrpUnicastInfo &aInfo)
{
    Error   error         = kErrorNone;
    uint8_t serviceNumber = Manager::kDnsSrpUnicastServiceNumber;

    aInfo.Clear();

    do
    {
        ServiceData serviceData;

        // Process Server sub-TLVs in the current Service TLV.

        while (AdvanceToNextServer() == kErrorNone)
        {
            aInfo.mRloc16 = mServerSubTlv->GetServer16();

            if (aType == kAddrInServiceData)
            {
                if (Manager::DnsSrpUnicast::ServiceData::ParseFrom(*mServiceTlv, aInfo) == kErrorNone)
                {
                    ExitNow();
                }

                // If Service Data does not contain address info, we
                // break from `while (IterateToNextServer())` loop
                // to skip over the entire Service TLV and all its
                // sub-TLVs and go to the next one.

                break;
            }

            // `aType` is `kAddrInServerData`.

            // Server sub-TLV can contain address and port info
            // (then we parse and return the info), or it can be
            // empty (then we skip over it).

            if (Manager::DnsSrpUnicast::ServerData::ParseFrom(*mServerSubTlv, aInfo) == kErrorNone)
            {
                ExitNow();
            }

            if (mServerSubTlv->GetServerDataLength() == sizeof(uint16_t))
            {
                // Handle the case where the server TLV data only
                // contains a port number and use the RLOC as the
                // IPv6 address.

                aInfo.mSockAddr.GetAddress().SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(),
                                                                 mServerSubTlv->GetServer16());
                aInfo.mSockAddr.SetPort(BigEndian::ReadUint16(mServerSubTlv->GetServerData()));
                aInfo.mVersion = 0;
                ExitNow();
            }
        }

        // Find the next matching Service TLV.

        serviceData.InitFrom(serviceNumber);
        mServiceTlv   = mNetworkData.FindNextThreadService(mServiceTlv, serviceData, NetworkData::kServicePrefixMatch);
        mServerSubTlv = nullptr;

        // If we have a valid Service TLV, restart the loop
        // to process its Server sub-TLVs.

    } while (mServiceTlv != nullptr);

    error = kErrorNotFound;

exit:
    return error;
}

Error Iterator::AdvanceToNextServer(void)
{
    Error                 error = kErrorNotFound;
    const NetworkDataTlv *start;
    const NetworkDataTlv *end;

    VerifyOrExit(mServiceTlv != nullptr);

    start = (mServerSubTlv != nullptr) ? mServerSubTlv->GetNext() : mServiceTlv->GetSubTlvs();
    end   = mServiceTlv->GetNext();

    mServerSubTlv = NetworkDataTlv::Find<ServerTlv>(start, end);

    if (mServerSubTlv != nullptr)
    {
        error = kErrorNone;
    }

exit:
    return error;
}

} // namespace Service
} // namespace NetworkData
} // namespace ot
