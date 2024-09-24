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
#include "common/encoding.hpp"
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
struct DnsSrpAnycastInfo
{
    Ip6::Address mAnycastAddress; ///< The anycast address associated with the DNS/SRP servers.
    uint8_t      mSequenceNumber; ///< Sequence number used to notify SRP client if they need to re-register.
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
struct DnsSrpUnicastInfo
{
    Ip6::SockAddr mSockAddr; ///< The socket address (IPv6 address and port) of the DNS/SRP server.
    uint16_t      mRloc16;   ///< The BR RLOC16 adding the entry.
};

/**
 * Manages the Thread Service entries in Thread Network Data.
 */
class Manager : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Represents an iterator used to iterate through Network Data Service entries.
     */
    class Iterator : public Clearable<Iterator>
    {
        friend class Manager;

    public:
        /**
         * Initializes the iterator (as empty/clear).
         */
        Iterator(void)
            : mServiceTlv(nullptr)
            , mServerSubTlv(nullptr)
        {
        }

        /**
         * Resets the iterator to start from beginning.
         */
        void Reset(void)
        {
            mServiceTlv   = nullptr;
            mServerSubTlv = nullptr;
        }

    private:
        const ServiceTlv *mServiceTlv;
        const ServerTlv  *mServerSubTlv;
    };

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
     *
     * @retval kErrorNone     Successfully added the Service entry.
     * @retval kErrorNoBufs   Insufficient space to add the Service entry.
     */
    Error AddDnsSrpAnycastService(uint8_t aSequenceNumber)
    {
        return AddService(DnsSrpAnycastServiceData(aSequenceNumber));
    }

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
     *
     * @retval kErrorNone     Successfully added the Service entry.
     * @retval kErrorNoBufs   Insufficient space to add the Service entry.
     */
    Error AddDnsSrpUnicastServiceWithAddrInServiceData(const Ip6::Address &aAddress, uint16_t aPort)
    {
        return AddService(DnsSrpUnicast::ServiceData(aAddress, aPort));
    }

    /**
     * Removes a DNS/SRP Unicast Service entry with address in Service Data from the local Thread Network Data.
     *
     * @param[in] aAddress    The unicast address.
     * @param[in] aPort       The port number.
     *
     * @retval kErrorNone       Successfully removed the Service entry.
     * @retval kErrorNotFound   Could not find the Service entry.
     */
    Error RemoveDnsSrpUnicastServiceWithAddrInServiceData(const Ip6::Address &aAddress, uint16_t aPort)
    {
        return RemoveService(DnsSrpUnicast::ServiceData(aAddress, aPort));
    }

    /**
     * Adds a DNS/SRP Unicast Service entry with address in Server Data to the local Thread Network Data.
     *
     * @param[in] aAddress    The unicast address.
     * @param[in] aPort       The port number.
     *
     * @retval kErrorNone     Successfully added the Service entry.
     * @retval kErrorNoBufs   Insufficient space to add the Service entry.
     */
    Error AddDnsSrpUnicastServiceWithAddrInServerData(const Ip6::Address &aAddress, uint16_t aPort)
    {
        return AddService(kDnsSrpUnicastServiceNumber, DnsSrpUnicast::ServerData(aAddress, aPort));
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
        return AddService(kBackboneRouterServiceNumber,
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
     * Gets the next DNS/SRP info from the Thread Network Data "DNS/SRP Service Anycast Address" entries.
     *
     * To get the first entry, @p aIterator should be cleared (e.g., a new instance of `Iterator` or calling `Clear()`
     * method).
     *
     * @param[in,out] aIterator    A reference to an iterator.
     * @param[out]    aInfo        A reference to `DnsSrpAnycastInfo` to return the info.
     *
     * @retval kErrorNone       Successfully got the next info. @p aInfo and @p aIterator are updated.
     * @retval kErrorNotFound   No more matching entries in the Network Data.
     */
    Error GetNextDnsSrpAnycastInfo(Iterator &aIterator, DnsSrpAnycastInfo &aInfo) const;

    /**
     * Finds the preferred DNS/SRP info among all the Thread Network Data "DNS/SRP Service Anycast Address"
     * entries.
     *
     * The preferred entry is determined based on the sequence number value where a larger value (in the sense
     * specified by Serial Number Arithmetic logic in RFC-1982) is considered more recent and therefore preferred.
     *
     * @param[out] aInfo        A reference to `DnsSrpAnycastInfo` to return the info.
     *
     * @retval kErrorNone       Successfully found the preferred info. @p aInfo is updated.
     * @retval kErrorNotFound   No "DNS/SRP Service Anycast" entry in Network Data.
     */
    Error FindPreferredDnsSrpAnycastInfo(DnsSrpAnycastInfo &aInfo) const;

    /**
     * Gets the next DNS/SRP info from the Thread Network Data "DNS/SRP Service Unicast Address" entries.
     *
     * To get the first entry @p aIterator should be cleared (e.g., a new instance of `Iterator` or calling `Clear()`
     * method).
     *
     * @param[in,out] aIterator    A reference to an iterator.
     * @param[in]     aType        The entry type, `kAddrInServiceData` or `kAddrInServerData`
     * @param[out]    aInfo        A reference to `DnsSrpUnicastInfo` to return the info.
     *
     * @retval kErrorNone       Successfully got the next info. @p aInfo and @p aIterator are updated.
     * @retval kErrorNotFound   No more matching entries in the Network Data.
     */
    Error GetNextDnsSrpUnicastInfo(Iterator &aIterator, DnsSrpUnicastType aType, DnsSrpUnicastInfo &aInfo) const;

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

    private:
        uint8_t mServiceNumber;
        uint8_t mSequenceNumber;
    } OT_TOOL_PACKED_END;

    class DnsSrpUnicast
    {
    public:
        OT_TOOL_PACKED_BEGIN
        struct ServiceData
        {
        public:
            explicit ServiceData(const Ip6::Address &aAddress, uint16_t aPort)
                : mServiceNumber(kDnsSrpUnicastServiceNumber)
                , mAddress(aAddress)
                , mPort(BigEndian::HostSwap16(aPort))
            {
                OT_UNUSED_VARIABLE(mServiceNumber);
            }

            const Ip6::Address &GetAddress(void) const { return mAddress; }
            uint16_t            GetPort(void) const { return BigEndian::HostSwap16(mPort); }

        private:
            uint8_t      mServiceNumber;
            Ip6::Address mAddress;
            uint16_t     mPort;
        } OT_TOOL_PACKED_END;

        OT_TOOL_PACKED_BEGIN
        class ServerData
        {
        public:
            ServerData(const Ip6::Address &aAddress, uint16_t aPort)
                : mAddress(aAddress)
                , mPort(BigEndian::HostSwap16(aPort))
            {
            }

            const Ip6::Address &GetAddress(void) const { return mAddress; }
            uint16_t            GetPort(void) const { return BigEndian::HostSwap16(mPort); }

        private:
            Ip6::Address mAddress;
            uint16_t     mPort;
        } OT_TOOL_PACKED_END;

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

    private:
        uint8_t  mSequenceNumber;
        uint16_t mReregDelay;
        uint32_t mMlrTimeout;
    } OT_TOOL_PACKED_END;
#endif

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    template <typename ServiceDataType> Error AddService(const ServiceDataType &aServiceData)
    {
        return AddService(&aServiceData, sizeof(ServiceDataType), nullptr, 0);
    }

    template <typename ServerDataType> Error AddService(uint8_t aServiceNumber, const ServerDataType &aServerData)
    {
        return AddService(&aServiceNumber, sizeof(uint8_t), &aServerData, sizeof(ServerDataType));
    }

    Error AddService(const void *aServiceData,
                     uint8_t     aServiceDataLength,
                     const void *aServerData       = nullptr,
                     uint8_t     aServerDataLength = 0);

    template <typename ServiceDataType> Error RemoveService(const ServiceDataType &aServiceData)
    {
        return RemoveService(&aServiceData, sizeof(ServiceDataType));
    }

    Error RemoveService(uint8_t aServiceNumber) { return RemoveService(&aServiceNumber, sizeof(uint8_t)); }
    Error RemoveService(const void *aServiceData, uint8_t aServiceDataLength);
#endif

    Error GetServiceId(uint8_t aServiceNumber, uint8_t &aServiceId) const;
    Error IterateToNextServer(Iterator &aIterator) const;

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
