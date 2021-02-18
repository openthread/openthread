/*
 *  Copyright (c) 2017-2021, The OpenThread Authors.
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

#ifndef DNS_CLIENT_HPP_
#define DNS_CLIENT_HPP_

#include "openthread-core-config.h"

#include <openthread/dns_client.h>

#include "common/clearable.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "net/dns_headers.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"

/**
 * @file
 *   This file includes definitions for the DNS client.
 */

/**
 * This struct represents an opaque (and empty) type for a response to an address resolution DNS query.
 *
 */
struct otDnsAddressResponse
{
};

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

/**
 * This struct represents an opaque (and empty) type for a response to browse (service instance enumeration) DNS query.
 *
 */
struct otDnsBrowseResponse
{
};

/**
 * This struct represents an opaque (and empty) type for a response to service inst resolution DNS query.
 *
 */
struct otDnsServiceResponse
{
};

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

namespace ot {
namespace Dns {

/**
 * This class implements DNS client.
 *
 */
class Client : public InstanceLocator, private NonCopyable
{
    typedef Message Query; // `Message` is used to save `Query` related info.

public:
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    /**
     * This structure provides info for a DNS service instance.
     *
     */
    typedef otDnsServiceInfo ServiceInfo;
#endif

    /**
     * This class represents a DNS query response.
     *
     */
    class Response : public Clearable<Response>,
                     public otDnsAddressResponse
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        ,
                     public otDnsBrowseResponse,
                     public otDnsServiceResponse
#endif

    {
        friend class Client;

    protected:
        enum Section : uint8_t
        {
            kAnswerSection,
            kAdditionalDataSection,
        };

        Response(void) { Clear(); }

        otError GetName(char *aNameBuffer, uint16_t aNameBufferSize) const;
        void    SelectSection(Section aSection, uint16_t &aOffset, uint16_t &aNumRecord) const;
        otError FindHostAddress(Section       aSection,
                                const Name &  aHostName,
                                uint16_t      aIndex,
                                Ip6::Address &aAddress,
                                uint32_t &    aTtl) const;

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        otError FindServiceInfo(Section aSection, const Name &aName, ServiceInfo &aServiceInfo) const;
#endif

        Query *        mQuery;                 // The associated query.
        const Message *mMessage;               // The response message.
        uint16_t       mAnswerOffset;          // Answer section offset in `mMessage`.
        uint16_t       mAnswerRecordCount;     // Number of records in answer section.
        uint16_t       mAdditionalOffset;      // Additional data section offset in `mMessage`.
        uint16_t       mAdditionalRecordCount; // Number of records in additional data section.
    };

    /**
     * This type represents the function pointer callback which is called when a DNS response for an address resolution
     * query is received.
     *
     */
    typedef otDnsAddressCallback AddressCallback;

    /**
     * This type represents an address resolution query DNS response.
     *
     */
    class AddressResponse : public Response
    {
        friend class Client;

    public:
        /**
         * This method gets the host name associated with an address resolution DNS response.
         *
         * This method MUST only be used from `AddressCallback`.
         *
         * @param[out] aNameBuffer       A buffer to char array to output the host name.
         * @param[in]  aNameBufferSize   The size of @p aNameBuffer.
         *
         * @retval OT_ERROR_NONE     The host name was read successfully.
         * @retval OT_ERROR_NO_BUFS  The name does not fit in @p aNameBuffer.
         *
         */
        otError GetHostName(char *aNameBuffer, uint16_t aNameBufferSize) const
        {
            return GetName(aNameBuffer, aNameBufferSize);
        }

        /**
         * This method gets the IPv6 address associated with an address resolution DNS response.
         *
         * This method MUST only be used from `AddressCallback`.
         *
         * The response may include multiple IPv6 address records. @p aIndex can be used to iterate through the list of
         * addresses. Index zero gets the the first address and so on. When we reach end of the list, this method
         * returns `OT_ERROR_NOT_FOUND`.
         *
         * @param[in]  aIndex        The address record index to retrieve.
         * @param[out] aAddress      A reference to an IPv6 address to output the address.
         * @param[out] aTtl          A reference to a `uint32_t` to output TTL for the address.
         *
         * @retval OT_ERROR_NONE       The address was read successfully.
         * @retval OT_ERROR_NOT_FOUND  No address record at @p aIndex.
         * @retval OT_ERROR_PARSE      Could not parse the records.
         *
         */
        otError GetAddress(uint16_t aIndex, Ip6::Address &aAddress, uint32_t &aTtl) const;
    };

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

    /**
     * This type represents the function pointer callback which is called when a response for a browse (service
     * instance enumeration) DNS query is received.
     *
     */
    typedef otDnsBrowseCallback BrowseCallback;

    /**
     * This type represents a browse (service instance enumeration) DNS response.
     *
     */
    class BrowseResponse : public Response
    {
        friend class Client;

    public:
        /**
         * This method gets the service name associated with a DNS browse response.
         *
         * This method MUST only be used from `BrowseCallback`.
         *
         * @param[out] aNameBuffer       A buffer to char array to output the host name.
         * @param[in]  aNameBufferSize   The size of @p aNameBuffer.
         *
         * @retval OT_ERROR_NONE     The host name was read successfully.
         * @retval OT_ERROR_NO_BUFS  The name does not fit in @p aNameBuffer.
         *
         */
        otError GetServiceName(char *aNameBuffer, uint16_t aNameBufferSize) const
        {
            return GetName(aNameBuffer, aNameBufferSize);
        }

        /**
         * This method gets a service instance associated with a DNS browse (service instance enumeration) response.
         *
         * This method MUST only be used from `BrowseCallback`.
         *
         * A response may include multiple service instance records. @p aIndex can be used to iterate through the list.
         * Index zero gives the the first record. When we reach end of the list, `OT_ERROR_NOT_FOUND` is returned.
         *
         * Note that this method gets the service instance label and not the full service instance name which is of the
         * form `<Instance>.<Service>.<Domain>`.
         *
         * @param[in]  aResponse          A pointer to a response.
         * @param[in]  aIndex             The service instance record index to retrieve.
         * @param[out] aLabelBuffer       A char array to output the service instance label (MUST NOT be NULL).
         * @param[in]  aLabelBufferSize   The size of @p aLabelBuffer.
         *
         * @retval OT_ERROR_NONE          The service instance was read successfully.
         * @retval OT_ERROR_NO_BUFS       The name does not fit in @p aNameBuffer.
         * @retval OT_ERROR_NOT_FOUND     No service instance record at @p aIndex.
         * @retval OT_ERROR_PARSE         Could not parse the records.
         *
         */
        otError GetServiceInstance(uint16_t aIndex, char *aLabelBuffer, uint8_t aLabelBufferSize) const;

        /**
         * This method gets info for a service instance from a DNS browse (service instance enumeration) response.
         *
         * This method MUST only be used from `BrowseCallback`.
         *
         * A browse DNS response should include the SRV, TXT, and AAAA records for the service instances that are
         * enumerated (note that it is a SHOULD and not a MUST requirement). This method tries to retrieve this info
         * for a given service instance.
         *
         * - If no matching SRV record is found, `OT_ERROR_NOT_FOUND` is returned.
         * - If a matching SRV record is found, @p aServiceInfo is updated returning `OT_ERROR_NONE`.
         * - If no matching TXT record is found, `mTxtDataSize` in @p aServiceInfo is set to zero.
         * - If no matching AAAA record is found, `mHostAddress is set to all zero or unspecified address.
         * - If there are multiple AAAA records for the host name `mHostAddress` is set to the first one. The other
         *   addresses can be retrieved using `GetHostAddress()` method.
         *
         * @param[in]  aInstanceLabel     The service instance label (MUST NOT be `nullptr`).
         * @param[out] aServiceInfo       A `ServiceInfo` to output the service instance information.
         *
         * @retval OT_ERROR_NONE          The service instance info was read. @p aServiceInfo is updated.
         * @retval OT_ERROR_NOT_FOUND     Could not find a matching SRV record for @p aInstanceLabel.
         * @retval OT_ERROR_NO_BUFS       The host name and/or the TXT data could not fit in given buffers.
         * @retval OT_ERROR_PARSE         Could not parse the records.
         *
         */
        otError GetServiceInfo(const char *aInstanceLabel, ServiceInfo &aServiceInfo) const;

        /**
         * This method gets the host IPv6 address from a DNS browse (service instance enumeration) response.
         *
         * This method MUST only be used from `BrowseCallback`.
         *
         * The response can include zero or more IPv6 address records. @p aIndex can be used to iterate through the
         * list of addresses. Index zero gets the first address and so on. When we reach end of the list, this method
         * returns `OT_ERROR_NOT_FOUND`.
         *
         * @param[in]  aHostName     The host name to get the address (MUST NOT be `nullptr`).
         * @param[in]  aIndex        The address record index to retrieve.
         * @param[out] aAddress      A reference to an IPv6 address to output the address.
         * @param[out] aTtl          A reference to a `uint32_t` to output TTL for the address.
         *
         * @retval OT_ERROR_NONE       The address was read successfully.
         * @retval OT_ERROR_NOT_FOUND  No address record for @p aHostname at @p aIndex.
         * @retval OT_ERROR_PARSE      Could not parse the records.
         *
         */
        otError GetHostAddress(const char *aHostName, uint16_t aIndex, Ip6::Address &aAddress, uint32_t &aTtl) const;

    private:
        otError FindPtrRecord(const char *aInstanceLabel, Name &aInstanceName) const;
    };

    /**
     * This type represents the function pointer callback which is called when a response for a service instance
     * resolution DNS query is received.
     *
     */
    typedef otDnsServiceCallback ServiceCallback;

    /**
     * This type represents a service instance resolution DNS response.
     *
     */
    class ServiceResponse : public Response
    {
        friend class Client;

    public:
        /**
         * This method gets the service instance name associated with a DNS service instance resolution response.
         *
         * This method MUST only be used from `ServiceCallback`.
         *
         * @param[out] aLabelBuffer      A buffer to char array to output the service instance label (MUST NOT be NULL).
         * @param[in]  aLabelBufferSize  The size of @p aLabelBuffer.
         * @param[out] aNameBuffer       A buffer to char array to output the rest of service name (can be NULL if user
         *                               is not interested in getting the name).
         * @param[in]  aNameBufferSize   The size of @p aNameBuffer.
         *
         * @retval OT_ERROR_NONE     The service instance name was read successfully.
         * @retval OT_ERROR_NO_BUFS  Either the label or name does not fit in the given buffers.
         *
         */
        otError GetServiceName(char *   aLabelBuffer,
                               uint8_t  aLabelBufferSize,
                               char *   aNameBuffer,
                               uint16_t aNameBufferSize) const;

        /**
         * This method gets info for a service instance from a DNS service instance resolution response.
         *
         * This method MUST only be used from `ServiceCallback`.
         *
         * - If no matching SRV record is found, `OT_ERROR_NOT_FOUND` is returned.
         * - If a matching SRV record is found, @p aServiceInfo is updated and `OT_ERROR_NONE` is returned.
         * - If no matching TXT record is found, `mTxtDataSize` in @p aServiceInfo is set to zero.
         * - If no matching AAAA record is found, `mHostAddress is set to all zero or unspecified address.
         * - If there are multiple AAAA records for the host name, `mHostAddress` is set to the first one. The other
         *   addresses can be retrieved using `GetHostAddress()` method.
         *
         * @param[out] aServiceInfo       A `ServiceInfo` to output the service instance information
         *
         * @retval OT_ERROR_NONE          The service instance info was read. @p aServiceInfo is updated.
         * @retval OT_ERROR_NOT_FOUND     Could not find a matching SRV record.
         * @retval OT_ERROR_NO_BUFS       The host name and/or TXT data could not fit in the given buffers.
         * @retval OT_ERROR_PARSE         Could not parse the records in the @p aResponse.
         *
         */
        otError GetServiceInfo(ServiceInfo &aServiceInfo) const;

        /**
         * This method gets the host IPv6 address from a DNS service instance resolution response.
         *
         * This method MUST only be used from `ServiceCallback`.
         *
         * The response can include zero or more IPv6 address records. @p aIndex can be used to iterate through the
         * list of addresses. Index zero gets the first address and so on. When we reach end of the list, this method
         * returns `OT_ERROR_NOT_FOUND`.
         *
         * @param[in]  aHostName     The host name to get the address (MUST NOT be `nullptr`).
         * @param[in]  aIndex        The address record index to retrieve.
         * @param[out] aAddress      A reference to an IPv6 address to output the address.
         * @param[out] aTtl          A reference to a `uint32_t` to output TTL for the address.
         *
         * @retval OT_ERROR_NONE       The address was read successfully.
         * @retval OT_ERROR_NOT_FOUND  No address record for @p aHostname at @p aIndex.
         * @retval OT_ERROR_PARSE      Could not parse the records.
         *
         */
        otError GetHostAddress(const char *aHostName, uint16_t aIndex, Ip6::Address &aAddress, uint32_t &aTtl) const;
    };

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Client(Instance &aInstance);

    /**
     * This method starts the DNS client.
     *
     * @retval OT_ERROR_NONE     Successfully started the DNS client.
     * @retval OT_ERROR_ALREADY  The socket is already open.
     *
     */
    otError Start(void);

    /**
     * This method stops the DNS client.
     *
     */
    void Stop(void);

    /**
     * This method sends an address resolution DNS query for AAAA (IPv6) record for a given host name.
     *
     * @param[in]  aServerSockAddr  The server socket address.
     * @param[in]  aHostName        The host name for which to query the address (MUST NOT be `nullptr`).
     * @param[in]  aNoRecursion     Indicates whether name server can resolve the query recursively or not.
     * @param[in]  aCallback        A callback function pointer to report the result of query.
     * @param[in]  aContext         A pointer to arbitrary context information passed to @p aCallback.
     *
     * @retval OT_ERROR_NONE            Successfully sent DNS query.
     * @retval OT_ERROR_NO_BUFS         Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_ARGS    The host name is not valid format.
     * @retval OT_ERROR_INVALID_STATE   Cannot send query since Thread interface is not up.
     *
     */
    otError ResolveAddress(const Ip6::SockAddr &aServerSockAddr,
                           const char *         aHostName,
                           bool                 aNoRecursion,
                           AddressCallback      aCallback,
                           void *               aContext);

#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

    /**
     * This method sends a browse (service instance enumeration) DNS query for a given service name.
     *
     * @param[in]  aServerSockAddr  The server socket address.
     * @param[in]  aServiceName     The service name to query for (MUST NOT be `nullptr`).
     * @param[in]  aCallback        The callback to report the response or errors (such as time-out).
     * @param[in]  aContext         A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE        Query sent successfully. @p aCallback will be invoked to report the status.
     * @retval OT_ERROR_NO_BUFS     Insufficient buffer to prepare and send query.
     *
     */
    otError Browse(const Ip6::SockAddr &aServerSockAddr,
                   const char *         aServiceName,
                   BrowseCallback       aCallback,
                   void *               aContext);

    /**
     * This function sends a DNS service instance resolution query for a given service instance.
     *
     * @param[in]  aServerSockAddr  The server socket address.
     * @param[in]  aInstanceLabel   The service instance label.
     * @param[in]  aServiceName     The service name (together with @p aInstanceLabel form full instance name).
     * @param[in]  aCallback        A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext         A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE        Query sent successfully. @p aCallback will be invoked to report the status.
     * @retval OT_ERROR_NO_BUFS     Insufficient buffer to prepare and send query.
     *
     */
    otError ResolveService(const Ip6::SockAddr &aServerSockAddr,
                           const char *         aInstanceLabel,
                           const char *         aServiceName,
                           otDnsServiceCallback aCallback,
                           void *               aContext);

#endif // OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE

private:
    enum
    {
        kResponseTimeout = OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT, // in msec
        kMaxRetransmit   = OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT,
    };

    enum QueryType : uint8_t
    {
        kAddressQuery, // Address resolution.
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        kBrowseQuery,  // Browse (service instance enumeration).
        kServiceQuery, // Service instance resolution.
#endif
    };

    union Callback
    {
        AddressCallback mAddressCallback;
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
        BrowseCallback  mBrowseCallback;
        ServiceCallback mServiceCallback;
#endif
    };

    typedef MessageQueue QueryList; // List of queries.

    struct QueryInfo : public Clearable<QueryInfo> // Query related Info
    {
        void ReadFrom(const Query &aQuery) { IgnoreError(aQuery.Read(0, *this)); }

        QueryType     mQueryType;
        uint16_t      mMessageId;
        Ip6::SockAddr mServerSockAddr;
        Callback      mCallback;
        void *        mCallbackContext;
        TimeMilli     mRetransmissionTime;
        uint8_t       mRetransmissionCount;
        bool          mNoRecursion;
        // Followed by the name (service, host, instance) encoded as a `Dns::Name`.
    };

    enum : uint16_t
    {
        kNameOffsetInQuery = sizeof(QueryInfo),
    };

    otError     StartQuery(QueryInfo &          aInfo,
                           const Ip6::SockAddr &aServerSockAddr,
                           const char *         aLabel,
                           const char *         aName,
                           void *               aContext);
    otError     AllocateQuery(const QueryInfo &aInfo, const char *aLabel, const char *aName, Query *&aQuery);
    void        FreeQuery(Query &aQuery);
    void        UpdateQuery(Query &aQuery, const QueryInfo &aInfo) { aQuery.Write(0, aInfo); }
    void        SendQuery(Query &aQuery);
    void        SendQuery(Query &aQuery, QueryInfo &aInfo, bool aUpdateTimer);
    void        FinalizeQuery(Query &aQuery, otError aError);
    void        FinalizeQuery(Response &Response, QueryType aType, otError aError);
    static void GetCallback(const Query &aQuery, Callback &aCallback, void *&aContext);
    otError     AppendNameFromQuery(const Query &aQuery, Message &aMessage);
    Query *     FindQueryById(uint16_t aMessageId);
    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMsgInfo);
    void        ProcessResponse(const Message &aMessage);
    otError     ParseResponse(Response &aResponse, QueryType &aType, otError &aResponseError);
    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    static const uint8_t   kQuestionCount[];
    static const uint16_t *kQuestionRecordTypes[];

    static const uint16_t kAddressQueryRecordTypes[];
#if OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE
    static const uint16_t kBrowseQueryRecordTypes[];
    static const uint16_t kServiceQueryRecordTypes[];
#endif

    Ip6::Udp::Socket mSocket;
    QueryList        mQueries;
    TimerMilli       mTimer;
};

} // namespace Dns
} // namespace ot

#endif // DNS_CLIENT_HPP_
