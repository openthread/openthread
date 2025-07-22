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
 *   This file includes definitions related to Thread Network Data service/server entries.
 */

#ifndef NETWORK_DATA_SERVICE_HPP_
#define NETWORK_DATA_SERVICE_HPP_

#include "openthread-core-config.h"

#include <openthread/netdata.h>

#include "backbone_router/bbr_leader.hpp"
#include "common/clearable.hpp"
#include "common/encoding.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/serial_number.hpp"
#include "net/socket.hpp"
#include "thread/network_data_tlvs.hpp"

namespace ot {
namespace NetworkData {
namespace Service {

const uint32_t kThreadEnterpriseNumber = ServiceTlv::kThreadEnterpriseNumber; ///< Thread enterprise number.

/**
 * Represents information about an DNS/SRP server parsed from related Network Data service entries.
 */
struct DnsSrpAnycastInfo : public Clearable<DnsSrpAnycastInfo>, public Equatable<DnsSrpAnycastInfo>
{
    Ip6::Address mAnycastAddress; ///< The anycast address associated with the DNS/SRP servers.
    uint8_t      mSequenceNumber; ///< Sequence number used to notify SRP client if they need to re-register.
    uint8_t      mVersion;        ///< Version number.
    uint16_t     mRloc16;         ///< The RLOC16 of the entry.
};

/**
 * Represents the `DnsSrpUnicast` entry type.
 */
enum DnsSrpUnicastType : uint8_t
{
    kAddrInServiceData, ///< Socket address is from service data.
    kAddrInServerData,  ///< Socket address is from server data.
};

/**
 * Represents information about an DNS/SRP server parsed from related Network Data service entries.
 */
struct DnsSrpUnicastInfo : public Clearable<DnsSrpUnicastInfo>, public Equatable<DnsSrpUnicastInfo>
{
    Ip6::SockAddr mSockAddr; ///< The socket address (IPv6 address and port) of the DNS/SRP server.
    uint8_t       mVersion;  ///< Version number.
    uint16_t      mRloc16;   ///< The BR RLOC16 adding the entry.
};

class Manager;

/**
 * Represents an iterator to iterate over service entries in a Network Data.
 */
class Iterator : public InstanceLocator, private NonCopyable
{
    friend class Manager;

public:
    /**
     * Initializes the `Iterator` for iterating over service entries in the Leader Network Data
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit Iterator(Instance &aInstance);

    /**
     * Initializes the `Iterator` for iterating over service entries in a given Network Data.
     *
     * @param[in] aInstance     The OpenThread instance.
     * @param[in] aNetworkData  The `NetworkData` to use with this iterator.
     */
    Iterator(Instance &aInstance, const NetworkData &aNetworkData);

    /**
     * Resets the `Iterator` to start over.
     */
    void Reset(void);

    /**
     * Gets the next DNS/SRP info from the Thread Network Data "DNS/SRP Service Anycast Address" entries.
     *
     * To start from the first entry, ensure the iterator is reset (e.g., by creating a new `Iterator` instance, or by
     * calling `Reset()`).
     *
     * @param[out] aInfo        A reference to `DnsSrpAnycastInfo` to return the info.
     *
     * @retval kErrorNone       Successfully got the next info. @p aInfo is updated.
     * @retval kErrorNotFound   No more matching entries in the Network Data.
     */
    Error GetNextDnsSrpAnycastInfo(DnsSrpAnycastInfo &aInfo);

    /**
     * Gets the next DNS/SRP info from the Thread Network Data "DNS/SRP Service Unicast Address" entries.
     *
     * To start from the first entry, ensure the iterator is reset (e.g., by creating a new `Iterator` instance, or by
     * calling `Reset()`).
     *
     * @param[in]  aType        The entry type, `kAddrInServiceData` or `kAddrInServerData`
     * @param[out] aInfo        A reference to `DnsSrpUnicastInfo` to return the info.
     *
     * @retval kErrorNone       Successfully got the next info. @p aInfo is updated.
     * @retval kErrorNotFound   No more matching entries in the Network Data.
     */
    Error GetNextDnsSrpUnicastInfo(DnsSrpUnicastType aType, DnsSrpUnicastInfo &aInfo);

private:
    Error AdvanceToNextServer(void);

    const NetworkData &mNetworkData;
    const ServiceTlv  *mServiceTlv;
    const ServerTlv   *mServerSubTlv;
};

/**
 * Manages the Thread Service entries in Thread Network Data.
 */
class Manager : public InstanceLocator, private NonCopyable
{
    friend class Iterator;

public:
    /**
     * Initializes the `Manager` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Manager(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
    }

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    /**
     * Adds a DNS/SRP Anycast Service entry to the local Thread Network Data.
     *
     * @param[in] aSequenceNumber  The anycast sequence number.
     * @param[in] aVersion         The version number
     *
     * @retval kErrorNone     Successfully added the Service entry.
     * @retval kErrorNoBufs   Insufficient space to add the Service entry.
     */
    Error AddDnsSrpAnycastService(uint8_t aSequenceNumber, uint8_t aVersion);

    /**
     * Removes a DNS/SRP Anycast Service entry from local Thread Network Data.
     *
     * @param[in] aSequenceNumber  The anycast sequence number.
     *
     * @retval kErrorNone       Successfully removed the Service entry.
     * @retval kErrorNotFound   Could not find the Service entry.
     */
    Error RemoveDnsSrpAnycastService(uint8_t aSequenceNumber)
    {
        return RemoveService(DnsSrpAnycastServiceData(aSequenceNumber));
    }

    /**
     * Adds a DNS/SRP Unicast Service entry with address in Service Data to the local Thread Network Data.
     *
     * @param[in] aAddress    The unicast address.
     * @param[in] aPort       The port number.
     * @param[in] aVersion    The version.
     *
     * @retval kErrorNone     Successfully added the Service entry.
     * @retval kErrorNoBufs   Insufficient space to add the Service entry.
     */
    Error AddDnsSrpUnicastServiceWithAddrInServiceData(const Ip6::Address &aAddress, uint16_t aPort, uint8_t aVersion)
    {
        return AddService(DnsSrpUnicast::ServiceData(aAddress, aPort, aVersion));
    }

    /**
     * Removes a DNS/SRP Unicast Service entry with address in Service Data from the local Thread Network Data.
     *
     * @param[in] aAddress    The unicast address.
     * @param[in] aPort       The port number.
     * @param[in] aVersion    The version.
     *
     * @retval kErrorNone       Successfully removed the Service entry.
     * @retval kErrorNotFound   Could not find the Service entry.
     */
    Error RemoveDnsSrpUnicastServiceWithAddrInServiceData(const Ip6::Address &aAddress,
                                                          uint16_t            aPort,
                                                          uint8_t             aVersion)
    {
        return RemoveService(DnsSrpUnicast::ServiceData(aAddress, aPort, aVersion));
    }

    /**
     * Adds a DNS/SRP Unicast Service entry with address in Server Data to the local Thread Network Data.
     *
     * @param[in] aAddress    The unicast address.
     * @param[in] aPort       The port number.
     * @param[in] aVersion    The version.
     *
     * @retval kErrorNone     Successfully added the Service entry.
     * @retval kErrorNoBufs   Insufficient space to add the Service entry.
     */
    Error AddDnsSrpUnicastServiceWithAddrInServerData(const Ip6::Address &aAddress, uint16_t aPort, uint8_t aVersion)
    {
        return AddServiceWithNumber(kDnsSrpUnicastServiceNumber, DnsSrpUnicast::ServerData(aAddress, aPort, aVersion));
    }

    /**
     * Removes a DNS/SRP Unicast Service entry with address in Server Data from the local Thread Network Data.
     *
     * @retval kErrorNone       Successfully removed the Service entry.
     * @retval kErrorNotFound   Could not find the Service entry.
     */
    Error RemoveDnsSrpUnicastServiceWithAddrInServerData(void) { return RemoveService(kDnsSrpUnicastServiceNumber); }

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    /**
     * Adds a Backbone Router Service entry to the local Thread Network Data.
     *
     * @param[in]  aSequenceNumber       The sequence number of Backbone Router.
     * @param[in]  aReregistrationDelay  The Registration Delay (in seconds) of Backbone Router.
     * @param[in]  aMlrTimeout           The multicast listener report timeout (in seconds) of Backbone Router.
     *
     * @retval kErrorNone     Successfully added the Service entry.
     * @retval kErrorNoBufs   Insufficient space to add the Service entry.
     */
    Error AddBackboneRouterService(uint8_t aSequenceNumber, uint16_t aReregistrationDelay, uint32_t aMlrTimeout)
    {
        return AddServiceWithNumber(kBackboneRouterServiceNumber,
                                    BbrServerData(aSequenceNumber, aReregistrationDelay, aMlrTimeout));
    }

    /**
     * Removes the Backbone Router Service entry from the local Thread Network Data.
     *
     * @retval kErrorNone       Successfully removed the Service entry.
     * @retval kErrorNotFound   Could not find the Service entry.
     */
    Error RemoveBackboneRouterService(void) { return RemoveService(kBackboneRouterServiceNumber); }
#endif

#endif // OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    /**
     * Gets the Primary Backbone Router (PBBR) in the Thread Network Data.
     *
     * @param[out]  aConfig      The Primary Backbone Router configuration.
     */
    void GetBackboneRouterPrimary(ot::BackboneRouter::Config &aConfig) const;

    /**
     * Gets the Service ID of Backbone Router service from Thread Network Data.
     *
     * @param[out] aServiceId      A reference where to put the Service ID.
     *
     * @retval kErrorNone       Successfully got the Service ID.
     * @retval kErrorNotFound   The specified service was not found.
     */
    Error GetBackboneRouterServiceId(uint8_t &aServiceId) const
    {
        return GetServiceId(kBackboneRouterServiceNumber, aServiceId);
    }
#endif

    /**
     * Finds the preferred DNS/SRP info among all the Thread Network Data "DNS/SRP Service Anycast Address"
     * entries.
     *
     * The preferred entry is determined based on the sequence number value where a larger value (in the sense
     * specified by Serial Number Arithmetic logic in RFC-1982) is considered more recent and therefore preferred.
     *
     * When successfully found, the `aInfo.mVersion` is set to the minimum version among all the entries matching the
     * same sequence number as the selected `aInfo.mSequenceNumber`.
     *
     * @param[out] aInfo        A reference to `DnsSrpAnycastInfo` to return the info.
     *
     * @retval kErrorNone       Successfully found the preferred info. @p aInfo is updated.
     * @retval kErrorNotFound   No "DNS/SRP Service Anycast" entry in Network Data.
     */
    Error FindPreferredDnsSrpAnycastInfo(DnsSrpAnycastInfo &aInfo) const;

private:
    static constexpr uint8_t kBackboneRouterServiceNumber = 0x01;
    static constexpr uint8_t kDnsSrpAnycastServiceNumber  = 0x5c;
    static constexpr uint8_t kDnsSrpUnicastServiceNumber  = 0x5d;

    OT_TOOL_PACKED_BEGIN
    class DnsSrpAnycastServiceData
    {
    public:
        explicit DnsSrpAnycastServiceData(uint8_t aSequenceNumber)
            : mServiceNumber(kDnsSrpAnycastServiceNumber)
            , mSequenceNumber(aSequenceNumber)
        {
            OT_UNUSED_VARIABLE(mServiceNumber);
        }

        uint8_t GetSequenceNumber(void) const { return mSequenceNumber; }
        uint8_t GetLength(void) const { return sizeof(DnsSrpAnycastServiceData); }

    private:
        uint8_t mServiceNumber;
        uint8_t mSequenceNumber;
    } OT_TOOL_PACKED_END;

    class DnsSrpUnicast
    {
    public:
        OT_TOOL_PACKED_BEGIN
        class AddrData
        {
        public:
            static constexpr uint8_t kMinLength = sizeof(Ip6::Address) + sizeof(uint16_t); // Address and port.

            AddrData(const Ip6::Address &aAddress, uint16_t aPort, uint8_t aVersion)
                : mAddress(aAddress)
                , mPort(BigEndian::HostSwap16(aPort))
                , mVersion(aVersion)
            {
            }

            uint8_t             GetLength(void) const { return (mVersion == 0) ? kMinLength : sizeof(AddrData); }
            const Ip6::Address &GetAddress(void) const { return mAddress; }
            uint16_t            GetPort(void) const { return BigEndian::HostSwap16(mPort); }
            uint8_t             GetVersion(void) const { return mVersion; }

            static Error ParseFrom(const uint8_t *aData, uint8_t aLength, DnsSrpUnicastInfo &aInfo);

        private:
            Ip6::Address mAddress;
            uint16_t     mPort;
            uint8_t      mVersion;
        } OT_TOOL_PACKED_END;

        static_assert(AddrData::kMinLength + sizeof(uint8_t) == sizeof(AddrData),
                      "Update all methods/constants if adding new (optional) fields to `AddrData`.");

        OT_TOOL_PACKED_BEGIN
        class ServiceData
        {
        public:
            static constexpr uint8_t kMinLength = sizeof(uint8_t) + AddrData::kMinLength;

            ServiceData(const Ip6::Address &aAddress, uint16_t aPort, uint8_t aVersion)
                : mServiceNumber(kDnsSrpUnicastServiceNumber)
                , mAddrData(aAddress, aPort, aVersion)
            {
                OT_UNUSED_VARIABLE(mServiceNumber);
            }

            uint8_t GetLength(void) const { return sizeof(uint8_t) + mAddrData.GetLength(); }

            static Error ParseFrom(const ServiceTlv &aServiceTlv, DnsSrpUnicastInfo &aInfo)
            {
                // Skip over `mServiceNumber` field (`uint8_t`)`
                return AddrData::ParseFrom(aServiceTlv.GetServiceData() + sizeof(uint8_t),
                                           aServiceTlv.GetServiceDataLength() - sizeof(uint8_t), aInfo);
            }

        private:
            uint8_t  mServiceNumber;
            AddrData mAddrData;
        } OT_TOOL_PACKED_END;

        static_assert(ServiceData::kMinLength + sizeof(uint8_t) == sizeof(ServiceData),
                      "Update all methods/constants if adding new (optional) fields to `ServiceData`.");

        OT_TOOL_PACKED_BEGIN
        class ServerData
        {
        public:
            static constexpr uint8_t kMinLength = AddrData::kMinLength;

            ServerData(const Ip6::Address &aAddress, uint16_t aPort, uint8_t aVersion)
                : mAddrData(aAddress, aPort, aVersion)
            {
            }

            uint8_t GetLength(void) const { return mAddrData.GetLength(); }

            static Error ParseFrom(const ServerTlv &aServerTlv, DnsSrpUnicastInfo &aInfo)
            {
                return AddrData::ParseFrom(aServerTlv.GetServerData(), aServerTlv.GetServerDataLength(), aInfo);
            }

        private:
            AddrData mAddrData;
        } OT_TOOL_PACKED_END;

        static_assert(ServerData::kMinLength + sizeof(uint8_t) == sizeof(ServerData),
                      "Update all methods/constants if adding new (optional) fields to `ServerData`.");

        DnsSrpUnicast(void) = delete;
    };

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    OT_TOOL_PACKED_BEGIN
    class BbrServerData
    {
    public:
        BbrServerData(uint8_t aSequenceNumber, uint16_t aReregDelay, uint32_t aMlrTimeout)
            : mSequenceNumber(aSequenceNumber)
            , mReregDelay(BigEndian::HostSwap16(aReregDelay))
            , mMlrTimeout(BigEndian::HostSwap32(aMlrTimeout))
        {
        }

        uint8_t  GetSequenceNumber(void) const { return mSequenceNumber; }
        uint16_t GetReregistrationDelay(void) const { return BigEndian::HostSwap16(mReregDelay); }
        uint32_t GetMlrTimeout(void) const { return BigEndian::HostSwap32(mMlrTimeout); }
        uint8_t  GetLength(void) const { return sizeof(BbrServerData); }

    private:
        uint8_t  mSequenceNumber;
        uint16_t mReregDelay;
        uint32_t mMlrTimeout;
    } OT_TOOL_PACKED_END;
#endif

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    template <typename ServiceDataType> Error AddService(const ServiceDataType &aServiceData)
    {
        return AddService(&aServiceData, aServiceData.GetLength(), nullptr, 0);
    }

    template <typename ServerDataType>
    Error AddServiceWithNumber(uint8_t aServiceNumber, const ServerDataType &aServerData)
    {
        return AddService(&aServiceNumber, sizeof(uint8_t), &aServerData, aServerData.GetLength());
    }

    template <typename ServiceDataType, typename ServerDataType>
    Error AddService(const ServiceDataType &aServiceData, const ServerDataType &aServerData)
    {
        return AddService(&aServiceData, aServiceData.GetLength(), &aServerData, sizeof(ServerDataType));
    }

    Error AddService(const void *aServiceData,
                     uint8_t     aServiceDataLength,
                     const void *aServerData,
                     uint8_t     aServerDataLength);

    template <typename ServiceDataType> Error RemoveService(const ServiceDataType &aServiceData)
    {
        return RemoveService(&aServiceData, aServiceData.GetLength());
    }

    Error RemoveService(uint8_t aServiceNumber) { return RemoveService(&aServiceNumber, sizeof(uint8_t)); }
    Error RemoveService(const void *aServiceData, uint8_t aServiceDataLength);
#endif

    Error GetServiceId(uint8_t aServiceNumber, uint8_t &aServiceId) const;

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    bool IsBackboneRouterPreferredTo(const ServerTlv     &aServerTlv,
                                     const BbrServerData &aServerData,
                                     const ServerTlv     &aOtherServerTlv,
                                     const BbrServerData &aOtherServerData) const;
#endif
};

} // namespace Service
} // namespace NetworkData
} // namespace ot

#endif // NETWORK_DATA_SERVICE_HPP_
