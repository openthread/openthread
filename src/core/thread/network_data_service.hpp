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
#include "net/socket.hpp"
#include "thread/network_data_tlvs.hpp"

namespace ot {
namespace NetworkData {
namespace Service {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

enum : uint32_t
{
    kThreadEnterpriseNumber = ServiceTlv::kThreadEnterpriseNumber, ///< Thread enterprise number.
};

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

/**
 * This type implements Thread Network Data "Backbone Router Service" server data generation and parsing.
 *
 */
class BackboneRouter
{
public:
    enum : uint8_t
    {
        kServiceNumber = 0x01, ///< Backbone Router service data number (THREAD_SERVICE_DATA_BBR).
    };

    /**
     * This class implements the generation and parsing of "Backbone Router Service" server data.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class ServerData
    {
    public:
        /**
         * This method returns the length (in bytes) of server data.
         *
         * @returns The server data length in bytes.
         *
         */
        uint8_t GetLength(void) const { return sizeof(ServerData); }

        /**
         * This method returns the sequence number of Backbone Router.
         *
         * @returns  The sequence number of the Backbone Router.
         *
         */
        uint8_t GetSequenceNumber(void) const { return mSequenceNumber; }

        /**
         * This method sets the sequence number of Backbone Router.
         *
         * @param[in]  aSequenceNumber  The sequence number of Backbone Router.
         *
         */
        void SetSequenceNumber(uint8_t aSequenceNumber) { mSequenceNumber = aSequenceNumber; }

        /**
         * This method returns the Registration Delay (in seconds) of Backbone Router.
         *
         * @returns The BBR Registration Delay (in seconds) of Backbone Router.
         *
         */
        uint16_t GetReregistrationDelay(void) const { return HostSwap16(mReregistrationDelay); }

        /**
         * This method sets the Registration Delay (in seconds) of Backbone Router.
         *
         * @param[in]  aReregistrationDelay  The Registration Delay (in seconds) of Backbone Router.
         *
         */
        void SetReregistrationDelay(uint16_t aReregistrationDelay)
        {
            mReregistrationDelay = HostSwap16(aReregistrationDelay);
        }

        /**
         * This method returns the multicast listener report timeout (in seconds) of Backbone Router.
         *
         * @returns The multicast listener report timeout (in seconds) of Backbone Router.
         *
         */
        uint32_t GetMlrTimeout(void) const { return HostSwap32(mMlrTimeout); }

        /**
         * This method sets multicast listener report timeout (in seconds) of Backbone Router.
         *
         * @param[in]  aMlrTimeout  The multicast listener report timeout (in seconds) of Backbone Router.
         *
         */
        void SetMlrTimeout(uint32_t aMlrTimeout) { mMlrTimeout = HostSwap32(aMlrTimeout); }

    private:
        uint8_t  mSequenceNumber;
        uint16_t mReregistrationDelay;
        uint32_t mMlrTimeout;
    } OT_TOOL_PACKED_END;

    BackboneRouter(void) = delete;
};

#endif // #if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)

/**
 * This type implements Thread Network Data "SRP Server Service" server data generation and parsing.
 *
 */
class SrpServer
{
public:
    enum : uint8_t
    {
        kServiceNumber = OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_NUMBER, ///< SRP Sever Service number
    };

    /**
     * This structure represents information about an SRP server (from "SRP Server Service" Server entries).
     *
     */
    struct Info
    {
        Ip6::SockAddr mSockAddr; ///< The SRP server address (IPv6 address and port number).
        uint16_t      mRloc16;   ///< The RLOC16 of SRP server.
    };

    /**
     * This class implements generation and parsing of "SRP Server Service" server data.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class ServerData
    {
    public:
        /** This method returns the length (in bytes) of server data.
         *
         * @returns The server data length in bytes.
         *
         */
        uint8_t GetLength(void) const { return sizeof(ServerData); }

        /**
         * This method returns the port number being used by the SRP server.
         *
         * @return The port number of SPR server.
         *
         */
        uint16_t GetPort(void) const { return HostSwap16(mPort); }

        /**
         * This method sets the SRP port number in `ServerData`.
         *
         * @param[in] aPort   The port number of SRP server.
         *
         */
        void SetPort(uint16_t aPort) { mPort = HostSwap16(aPort); }

    private:
        uint16_t mPort;
    } OT_TOOL_PACKED_END;

    SrpServer(void) = delete;
};

/**
 * This class manages the Thread Service entries in Thread Network Data.
 *
 */
class Manager : public InstanceLocator, private NonCopyable
{
public:
    /**
     * This class represents an iterator used to iterate through Network Data Service entries.
     *
     */
    class Iterator : public Clearable<Iterator>
    {
        friend class Manager;

    public:
        /**
         * This constructor initializes the iterator (as empty/clear).
         *
         */
        Iterator(void)
            : mServiceTlv(nullptr)
            , mServerSubTlv(nullptr)
        {
        }

    private:
        const ServiceTlv *mServiceTlv;
        const ServerTlv * mServerSubTlv;
    };

    /**
     * This constructor initializes the `Manager` object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Manager(Instance &aInstance)
        : InstanceLocator(aInstance)
    {
    }

#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    /**
     * This method adds a Thread Service entry to the local Thread Network Data.
     *
     * When successfully added, this method also invokes `Notifier::HandleServerDataUpdated()` to register the changes
     * in local Network Data with leader.
     *
     * The template type `ServiceType` has the following requirements:
     *   - It MUST have a constant `ServiceType::kServiceNumber` specifying the service number.
     *   - It MUST define nested type `ServiceType::ServerData` representing the server data (and its format).
     *   - The `ServiceType::ServerData` MUST provide `GetLength()` method returning the length of server data.
     *
     * @tparam    ServiceType    The service type to be added.
     *
     * @param[in] aServerData    The server data.
     * @param[in] aServerStable  The Stable flag value for Server TLV.
     *
     * @retval OT_ERROR_NONE     Successfully added the Service entry.
     * @retval OT_ERROR_NO_BUFS  Insufficient space to add the Service entry.
     *
     */
    template <typename ServiceType>
    otError Add(const typename ServiceType::ServerData &aServerData, bool aServerStable = true)
    {
        return AddService(ServiceType::kServiceNumber, aServerStable, &aServerData, aServerData.GetLength());
    }

    /**
     * This method removed a Thread Service entry to the local Thread Network Data.
     *
     * When successfully removed, this method also invokes `Notifier::HandleServerDataUpdated()` to register the
     * changes in local Network Data with leader.
     *
     * The template type `ServiceType` has the following requirements:
     *   - It MUST have a constant `ServiceType::kServiceNumber` specifying the service number.
     *
     * @tparam   ServiceType       The service type to be removed.
     *
     * @retval OT_ERROR_NONE       Successfully removed the Service entry.
     * @retval OT_ERROR_NOT_FOUND  Could not find the Service entry.
     *
     */
    template <typename ServiceType> otError Remove(void) { return RemoveService(ServiceType::kServiceNumber); }

#endif

    /**
     * This method gets the Service ID for the specified service from Thread Network Data.
     *
     * The template type `ServiceType` has the following requirements:
     *   - It MUST have a constant `ServiceType::kServiceNumber` specifying the service number.
     *
     * @tparam     ServiceType     The service type to be added.
     *
     * @param[in]  aServerStable   The Stable flag value for Server TLV
     * @param[out] aServiceId      A reference where to put the Service ID.
     *
     * @retval OT_ERROR_NONE       Successfully got the Service ID.
     * @retval OT_ERROR_NOT_FOUND  The specified service was not found.
     *
     */
    template <typename ServiceType> otError GetServiceId(bool aServerStable, uint8_t &aServiceId) const
    {
        return GetServiceId(ServiceType::kServiceNumber, aServerStable, aServiceId);
    }

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
    /**
     * This method gets the Primary Backbone Router (PBBR) in the Thread Network Data.
     *
     * @param[out]  aConfig      The Primary Backbone Router configuration.
     *
     * @retval OT_ERROR_NONE       Successfully got the Primary Backbone Router configuration.
     * @retval OT_ERROR_NOT_FOUND  No Backbone Router Service in the Thread Network.
     *
     */
    otError GetBackboneRouterPrimary(ot::BackboneRouter::BackboneRouterConfig &aConfig) const;
#endif

    /**
     * This method gets the next SRP server info from the Thread Network Data "SRP Server Service" entries.
     *
     * This method allows caller to iterate through all server entries for Network Data "SRP Server Service". To get
     * the first entry @p aIterator should be cleared (e.g., a new instance of `Iterator` or calling `Clear()` method).
     *
     * @param[inout] aIterator     A reference to an iterator.
     * @param[out]   aInfo         A reference to `SrpServer::Info` to return the next SRP server info.
     *
     * @retval OT_ERROR_NONE       Successfully got the next SRP server info. @p aInfo and @p aIterator are updated.
     * @retval OT_ERROR_NOT_FOUND  No more SRP server entries in Network Data.
     *
     */
    otError GetNextSrpServerInfo(Iterator &aIterator, SrpServer::Info &aInfo) const;

private:
#if OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
    otError AddService(uint8_t aServiceNumber, bool aServerStable, const void *aServerData, uint8_t aServerDataLength);
    otError RemoveService(uint8_t aServiceNumber);
#endif

    otError GetServiceId(uint8_t aServiceNumber, bool aServerStable, uint8_t &aServiceId) const;
    otError IterateToNextServer(Iterator &aIterator) const;
};

} // namespace Service
} // namespace NetworkData
} // namespace ot

#endif // NETWORK_DATA_SERVICE_HPP_
