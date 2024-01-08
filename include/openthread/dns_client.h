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

/**
 * @file
 * @brief
 *  This file defines the top-level DNS functions for the OpenThread library.
 */

#ifndef OPENTHREAD_DNS_CLIENT_H_
#define OPENTHREAD_DNS_CLIENT_H_

#include <openthread/dns.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-dns
 *
 * @brief
 *   This module includes functions that control DNS communication.
 *
 *   The functions in this module are available only if feature `OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE` is enabled.
 *
 * @{
 *
 */

/**
 * Type represents the "Recursion Desired" (RD) flag in an `otDnsQueryConfig`.
 *
 */
typedef enum
{
    OT_DNS_FLAG_UNSPECIFIED       = 0, ///< Indicates the flag is not specified.
    OT_DNS_FLAG_RECURSION_DESIRED = 1, ///< Indicates DNS name server can resolve the query recursively.
    OT_DNS_FLAG_NO_RECURSION      = 2, ///< Indicates DNS name server can not resolve the query recursively.
} otDnsRecursionFlag;

/**
 * Type represents the NAT64 mode in an `otDnsQueryConfig`.
 *
 * The NAT64 mode indicates whether to allow or disallow NAT64 address translation during DNS client address resolution.
 * This mode is only used when `OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE` is enabled.
 *
 */
typedef enum
{
    OT_DNS_NAT64_UNSPECIFIED = 0, ///< NAT64 mode is not specified. Use default NAT64 mode.
    OT_DNS_NAT64_ALLOW       = 1, ///< Allow NAT64 address translation during DNS client address resolution.
    OT_DNS_NAT64_DISALLOW    = 2, ///< Do not allow NAT64 address translation during DNS client address resolution.
} otDnsNat64Mode;

/**
 * Type represents the service resolution mode in an `otDnsQueryConfig`.
 *
 * This is only used during DNS client service resolution `otDnsClientResolveService()`. It determines which
 * record types to query.
 *
 */
typedef enum
{
    OT_DNS_SERVICE_MODE_UNSPECIFIED      = 0, ///< Mode is not specified. Use default service mode.
    OT_DNS_SERVICE_MODE_SRV              = 1, ///< Query for SRV record only.
    OT_DNS_SERVICE_MODE_TXT              = 2, ///< Query for TXT record only.
    OT_DNS_SERVICE_MODE_SRV_TXT          = 3, ///< Query for both SRV and TXT records in same message.
    OT_DNS_SERVICE_MODE_SRV_TXT_SEPARATE = 4, ///< Query in parallel for SRV and TXT using separate messages.
    OT_DNS_SERVICE_MODE_SRV_TXT_OPTIMIZE = 5, ///< Query for TXT/SRV together first, if fails then query separately.
} otDnsServiceMode;

/**
 * Type represents the DNS transport protocol in an `otDnsQueryConfig`.
 *
 * This `OT_DNS_TRANSPORT_TCP` is only supported when `OPENTHREAD_CONFIG_DNS_CLIENT_OVER_TCP_ENABLE` is enabled.
 *
 */
typedef enum
{
    OT_DNS_TRANSPORT_UNSPECIFIED = 0, /// DNS transport is unspecified.
    OT_DNS_TRANSPORT_UDP         = 1, /// DNS query should be sent via UDP.
    OT_DNS_TRANSPORT_TCP         = 2, /// DNS query should be sent via TCP.
} otDnsTransportProto;

/**
 * Represents a DNS query configuration.
 *
 * Any of the fields in this structure can be set to zero to indicate that it is not specified. How the unspecified
 * fields are treated is determined by the function which uses the instance of `otDnsQueryConfig`.
 *
 */
typedef struct otDnsQueryConfig
{
    otSockAddr          mServerSockAddr;  ///< Server address (IPv6 addr/port). All zero or zero port for unspecified.
    uint32_t            mResponseTimeout; ///< Wait time (in msec) to rx response. Zero indicates unspecified value.
    uint8_t             mMaxTxAttempts;   ///< Maximum tx attempts before reporting failure. Zero for unspecified value.
    otDnsRecursionFlag  mRecursionFlag;   ///< Indicates whether the server can resolve the query recursively or not.
    otDnsNat64Mode      mNat64Mode;       ///< Allow/Disallow NAT64 address translation during address resolution.
    otDnsServiceMode    mServiceMode;     ///< Determines which records to query during service resolution.
    otDnsTransportProto mTransportProto;  ///< Select default transport protocol.
} otDnsQueryConfig;

/**
 * Gets the current default query config used by DNS client.
 *
 * When OpenThread stack starts, the default DNS query config is determined from a set of OT config options such as
 * `OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_IP6_ADDRESS`, `_DEFAULT_SERVER_PORT`, `_DEFAULT_RESPONSE_TIMEOUT`, etc.
 * (see `config/dns_client.h` for all related config options).
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 *
 * @returns A pointer to the current default config being used by DNS client.
 *
 */
const otDnsQueryConfig *otDnsClientGetDefaultConfig(otInstance *aInstance);

/**
 * Sets the default query config on DNS client.
 *
 * @note Any ongoing query will continue to use the config from when it was started. The new default config will be
 * used for any future DNS queries.
 *
 * The @p aConfig can be NULL. In this case the default config will be set to the defaults from OT config options
 * `OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_{}`. This resets the default query config back to to the config when the
 * OpenThread stack starts.
 *
 * In a non-NULL @p aConfig, caller can choose to leave some of the fields in `otDnsQueryConfig` instance unspecified
 * (value zero). The unspecified fields are replaced by the corresponding OT config option definitions
 * `OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_{}` to form the default query config.
 *
 * When `OPENTHREAD_CONFIG_DNS_CLIENT_DEFAULT_SERVER_ADDRESS_AUTO_SET_ENABLE` is enabled, the server's IPv6 address in
 * the default config is automatically set and updated by DNS client. This is done only when user does not explicitly
 * set or specify it. This behavior requires SRP client and its auto-start feature to be enabled. SRP client will then
 * monitor the Thread Network Data for DNS/SRP Service entries to select an SRP server. The selected SRP server address
 * is also set as the DNS server address in the default config.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aConfig     A pointer to the new query config to use as default.
 *
 */
void otDnsClientSetDefaultConfig(otInstance *aInstance, const otDnsQueryConfig *aConfig);

/**
 * An opaque representation of a response to an address resolution DNS query.
 *
 * Pointers to instance of this type are provided from callback `otDnsAddressCallback`.
 *
 */
typedef struct otDnsAddressResponse otDnsAddressResponse;

/**
 * Pointer is called when a DNS response is received for an address resolution query.
 *
 * Within this callback the user can use `otDnsAddressResponseGet{Item}()` functions along with the @p aResponse
 * pointer to get more info about the response.
 *
 * The @p aResponse pointer can only be used within this callback and after returning from this function it will not
 * stay valid, so the user MUST NOT retain the @p aResponse pointer for later use.
 *
 * @param[in]  aError     The result of the DNS transaction.
 * @param[in]  aResponse  A pointer to the response (it is always non-NULL).
 * @param[in]  aContext   A pointer to application-specific context.
 *
 * The @p aError can have the following:
 *
 *  - OT_ERROR_NONE              A response was received successfully.
 *  - OT_ERROR_ABORT             A DNS transaction was aborted by stack.
 *  - OT_ERROR_RESPONSE_TIMEOUT  No DNS response has been received within timeout.
 *
 * If the server rejects the address resolution request the error code from server is mapped as follow:
 *
 *  - (0)  NOERROR   Success (no error condition)                    -> OT_ERROR_NONE
 *  - (1)  FORMERR   Server unable to interpret due to format error  -> OT_ERROR_PARSE
 *  - (2)  SERVFAIL  Server encountered an internal failure          -> OT_ERROR_FAILED
 *  - (3)  NXDOMAIN  Name that ought to exist, does not exist        -> OT_ERROR_NOT_FOUND
 *  - (4)  NOTIMP    Server does not support the query type (OpCode) -> OT_ERROR_NOT_IMPLEMENTED
 *  - (5)  REFUSED   Server refused for policy/security reasons      -> OT_ERROR_SECURITY
 *  - (6)  YXDOMAIN  Some name that ought not to exist, does exist   -> OT_ERROR_DUPLICATED
 *  - (7)  YXRRSET   Some RRset that ought not to exist, does exist  -> OT_ERROR_DUPLICATED
 *  - (8)  NXRRSET   Some RRset that ought to exist, does not exist  -> OT_ERROR_NOT_FOUND
 *  - (9)  NOTAUTH   Service is not authoritative for zone           -> OT_ERROR_SECURITY
 *  - (10) NOTZONE   A name is not in the zone                       -> OT_ERROR_PARSE
 *  - (20) BADNAME   Bad name                                        -> OT_ERROR_PARSE
 *  - (21) BADALG    Bad algorithm                                   -> OT_ERROR_SECURITY
 *  - (22) BADTRUN   Bad truncation                                  -> OT_ERROR_PARSE
 *  - Other response codes                                           -> OT_ERROR_FAILED
 *
 */
typedef void (*otDnsAddressCallback)(otError aError, const otDnsAddressResponse *aResponse, void *aContext);

/**
 * Sends an address resolution DNS query for AAAA (IPv6) record(s) for a given host name.
 *
 * The @p aConfig can be NULL. In this case the default config (from `otDnsClientGetDefaultConfig()`) will be used as
 * the config for this query. In a non-NULL @p aConfig, some of the fields can be left unspecified (value zero). The
 * unspecified fields are then replaced by the values from the default config.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aHostName        The host name for which to query the address (MUST NOT be NULL).
 * @param[in]  aCallback        A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext         A pointer to arbitrary context information.
 * @param[in]  aConfig          A pointer to the config to use for this query.
 *
 * @retval OT_ERROR_NONE          Query sent successfully. @p aCallback will be invoked to report the status.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer to prepare and send query.
 * @retval OT_ERROR_INVALID_ARGS  The host name is not valid format.
 * @retval OT_ERROR_INVALID_STATE Cannot send query since Thread interface is not up.
 *
 */
otError otDnsClientResolveAddress(otInstance             *aInstance,
                                  const char             *aHostName,
                                  otDnsAddressCallback    aCallback,
                                  void                   *aContext,
                                  const otDnsQueryConfig *aConfig);

/**
 * Sends an address resolution DNS query for A (IPv4) record(s) for a given host name.
 *
 * Requires and is available when `OPENTHREAD_CONFIG_DNS_CLIENT_NAT64_ENABLE` is enabled.
 *
 * When a successful response is received, the addresses are returned from @p aCallback as NAT64 IPv6 translated
 * versions of the IPv4 addresses from the query response.
 *
 * The @p aConfig can be NULL. In this case the default config (from `otDnsClientGetDefaultConfig()`) will be used as
 * the config for this query. In a non-NULL @p aConfig, some of the fields can be left unspecified (value zero). The
 * unspecified fields are then replaced by the values from the default config.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aHostName        The host name for which to query the address (MUST NOT be NULL).
 * @param[in]  aCallback        A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext         A pointer to arbitrary context information.
 * @param[in]  aConfig          A pointer to the config to use for this query.
 *
 * @retval OT_ERROR_NONE          Query sent successfully. @p aCallback will be invoked to report the status.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer to prepare and send query.
 * @retval OT_ERROR_INVALID_ARGS  The host name is not valid format or NAT64 is not enabled in config.
 * @retval OT_ERROR_INVALID_STATE Cannot send query since Thread interface is not up.
 *
 */
otError otDnsClientResolveIp4Address(otInstance             *aInstance,
                                     const char             *aHostName,
                                     otDnsAddressCallback    aCallback,
                                     void                   *aContext,
                                     const otDnsQueryConfig *aConfig);

/**
 * Gets the full host name associated with an address resolution DNS response.
 *
 * MUST only be used from `otDnsAddressCallback`.
 *
 * @param[in]  aResponse         A pointer to the response.
 * @param[out] aNameBuffer       A buffer to char array to output the full host name (MUST NOT be NULL).
 * @param[in]  aNameBufferSize   The size of @p aNameBuffer.
 *
 * @retval OT_ERROR_NONE     The full host name was read successfully.
 * @retval OT_ERROR_NO_BUFS  The name does not fit in @p aNameBuffer.
 *
 */
otError otDnsAddressResponseGetHostName(const otDnsAddressResponse *aResponse,
                                        char                       *aNameBuffer,
                                        uint16_t                    aNameBufferSize);

/**
 * Gets an IPv6 address associated with an address resolution DNS response.
 *
 * MUST only be used from `otDnsAddressCallback`.
 *
 * The response may include multiple IPv6 address records. @p aIndex can be used to iterate through the list of
 * addresses. Index zero gets the first address and so on. When we reach end of the list, `OT_ERROR_NOT_FOUND` is
 * returned.
 *
 * @param[in]  aResponse     A pointer to the response.
 * @param[in]  aIndex        The address record index to retrieve.
 * @param[out] aAddress      A pointer to a IPv6 address to output the address (MUST NOT be NULL).
 * @param[out] aTtl          A pointer to an `uint32_t` to output TTL for the address. It can be NULL if caller does not
 *                           want to get the TTL.
 *
 * @retval OT_ERROR_NONE           The address was read successfully.
 * @retval OT_ERROR_NOT_FOUND      No address record in @p aResponse at @p aIndex.
 * @retval OT_ERROR_PARSE          Could not parse the records in the @p aResponse.
 * @retval OT_ERROR_INVALID_STATE  No NAT64 prefix (applicable only when NAT64 is allowed).
 *
 */
otError otDnsAddressResponseGetAddress(const otDnsAddressResponse *aResponse,
                                       uint16_t                    aIndex,
                                       otIp6Address               *aAddress,
                                       uint32_t                   *aTtl);

/**
 * An opaque representation of a response to a browse (service instance enumeration) DNS query.
 *
 * Pointers to instance of this type are provided from callback `otDnsBrowseCallback`.
 *
 */
typedef struct otDnsBrowseResponse otDnsBrowseResponse;

/**
 * Pointer is called when a DNS response is received for a browse (service instance enumeration) query.
 *
 * Within this callback the user can use `otDnsBrowseResponseGet{Item}()` functions along with the @p aResponse
 * pointer to get more info about the response.
 *
 * The @p aResponse pointer can only be used within this callback and after returning from this function it will not
 * stay valid, so the user MUST NOT retain the @p aResponse pointer for later use.
 *
 * @param[in]  aError     The result of the DNS transaction.
 * @param[in]  aResponse  A pointer to the response (it is always non-NULL).
 * @param[in]  aContext   A pointer to application-specific context.
 *
 * For the full list of possible values for @p aError, please see `otDnsAddressCallback()`.
 *
 */
typedef void (*otDnsBrowseCallback)(otError aError, const otDnsBrowseResponse *aResponse, void *aContext);

/**
 * Provides info for a DNS service instance.
 *
 */
typedef struct otDnsServiceInfo
{
    uint32_t     mTtl;                ///< Service record TTL (in seconds).
    uint16_t     mPort;               ///< Service port number.
    uint16_t     mPriority;           ///< Service priority.
    uint16_t     mWeight;             ///< Service weight.
    char        *mHostNameBuffer;     ///< Buffer to output the service host name (can be NULL if not needed).
    uint16_t     mHostNameBufferSize; ///< Size of `mHostNameBuffer`.
    otIp6Address mHostAddress;        ///< The host IPv6 address. Set to all zero if not available.
    uint32_t     mHostAddressTtl;     ///< The host address TTL.
    uint8_t     *mTxtData;            ///< Buffer to output TXT data (can be NULL if not needed).
    uint16_t     mTxtDataSize;        ///< On input, size of `mTxtData` buffer. On output number bytes written.
    bool         mTxtDataTruncated;   ///< Indicates if TXT data could not fit in `mTxtDataSize` and was truncated.
    uint32_t     mTxtDataTtl;         ///< The TXT data TTL.
} otDnsServiceInfo;

/**
 * Sends a DNS browse (service instance enumeration) query for a given service name.
 *
 * Is available when `OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE` is enabled.
 *
 * The @p aConfig can be NULL. In this case the default config (from `otDnsClientGetDefaultConfig()`) will be used as
 * the config for this query. In a non-NULL @p aConfig, some of the fields can be left unspecified (value zero). The
 * unspecified fields are then replaced by the values from the default config.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aServiceName     The service name to query for (MUST NOT be NULL).
 * @param[in]  aCallback        A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext         A pointer to arbitrary context information.
 * @param[in]  aConfig          A pointer to the config to use for this query.
 *
 * @retval OT_ERROR_NONE        Query sent successfully. @p aCallback will be invoked to report the status.
 * @retval OT_ERROR_NO_BUFS     Insufficient buffer to prepare and send query.
 *
 */
otError otDnsClientBrowse(otInstance             *aInstance,
                          const char             *aServiceName,
                          otDnsBrowseCallback     aCallback,
                          void                   *aContext,
                          const otDnsQueryConfig *aConfig);

/**
 * Gets the service name associated with a DNS browse (service instance enumeration) response.
 *
 * MUST only be used from `otDnsBrowseCallback`.
 *
 * @param[in]  aResponse         A pointer to the response.
 * @param[out] aNameBuffer       A buffer to char array to output the service name (MUST NOT be NULL).
 * @param[in]  aNameBufferSize   The size of @p aNameBuffer.
 *
 * @retval OT_ERROR_NONE     The service name was read successfully.
 * @retval OT_ERROR_NO_BUFS  The name does not fit in @p aNameBuffer.
 *
 */
otError otDnsBrowseResponseGetServiceName(const otDnsBrowseResponse *aResponse,
                                          char                      *aNameBuffer,
                                          uint16_t                   aNameBufferSize);

/**
 * Gets a service instance associated with a DNS browse (service instance enumeration) response.
 *
 * MUST only be used from `otDnsBrowseCallback`.
 *
 * The response may include multiple service instance records. @p aIndex can be used to iterate through the list. Index
 * zero gives the first record. When we reach end of the list, `OT_ERROR_NOT_FOUND` is returned.
 *
 * Note that this function gets the service instance label and not the full service instance name which is of the form
 * `<Instance>.<Service>.<Domain>`.
 *
 * @param[in]  aResponse          A pointer to the response.
 * @param[in]  aIndex             The service instance record index to retrieve.
 * @param[out] aLabelBuffer       A buffer to char array to output the service instance label (MUST NOT be NULL).
 * @param[in]  aLabelBufferSize   The size of @p aLabelBuffer.
 *
 * @retval OT_ERROR_NONE          The service instance was read successfully.
 * @retval OT_ERROR_NO_BUFS       The name does not fit in @p aNameBuffer.
 * @retval OT_ERROR_NOT_FOUND     No service instance record in @p aResponse at @p aIndex.
 * @retval OT_ERROR_PARSE         Could not parse the records in the @p aResponse.
 *
 */
otError otDnsBrowseResponseGetServiceInstance(const otDnsBrowseResponse *aResponse,
                                              uint16_t                   aIndex,
                                              char                      *aLabelBuffer,
                                              uint8_t                    aLabelBufferSize);

/**
 * Gets info for a service instance from a DNS browse (service instance enumeration) response.
 *
 * MUST only be used from `otDnsBrowseCallback`.
 *
 * A browse DNS response can include SRV, TXT, and AAAA records for the service instances that are enumerated. This is
 * a SHOULD and not a MUST requirement, and servers/resolvers are not required to provide this. This function attempts
 * to retrieve this info for a given service instance when available.
 *
 * - If no matching SRV record is found in @p aResponse, `OT_ERROR_NOT_FOUND` is returned. In this case, no additional
 *   records (no TXT and/or AAAA) are read.
 * - If a matching SRV record is found in @p aResponse, @p aServiceInfo is updated and `OT_ERROR_NONE` is returned.
 * - If no matching TXT record is found in @p aResponse, `mTxtDataSize` in @p aServiceInfo is set to zero.
 * - If TXT data length is greater than `mTxtDataSize`, it is read partially and `mTxtDataTruncated` is set to true.
 * - If no matching AAAA record is found in @p aResponse, `mHostAddress is set to all zero or unspecified address.
 * - If there are multiple AAAA records for the host name in @p aResponse, `mHostAddress` is set to the first one. The
 *   other addresses can be retrieved using `otDnsBrowseResponseGetHostAddress()`.
 *
 * @param[in]  aResponse          A pointer to the response.
 * @param[in]  aInstanceLabel     The service instance label (MUST NOT be NULL).
 * @param[out] aServiceInfo       A `ServiceInfo` to output the service instance information (MUST NOT be NULL).
 *
 * @retval OT_ERROR_NONE          The service instance info was read. @p aServiceInfo is updated.
 * @retval OT_ERROR_NOT_FOUND     Could not find a matching SRV record for @p aInstanceLabel.
 * @retval OT_ERROR_NO_BUFS       The host name and/or TXT data could not fit in the given buffers.
 * @retval OT_ERROR_PARSE         Could not parse the records in the @p aResponse.
 *
 */
otError otDnsBrowseResponseGetServiceInfo(const otDnsBrowseResponse *aResponse,
                                          const char                *aInstanceLabel,
                                          otDnsServiceInfo          *aServiceInfo);

/**
 * Gets the host IPv6 address from a DNS browse (service instance enumeration) response.
 *
 * MUST only be used from `otDnsBrowseCallback`.
 *
 * The response can include zero or more IPv6 address records. @p aIndex can be used to iterate through the list of
 * addresses. Index zero gets the first address and so on. When we reach end of the list, `OT_ERROR_NOT_FOUND` is
 * returned.
 *
 * @param[in]  aResponse     A pointer to the response.
 * @param[in]  aHostName     The host name to get the address (MUST NOT be NULL).
 * @param[in]  aIndex        The address record index to retrieve.
 * @param[out] aAddress      A pointer to a IPv6 address to output the address (MUST NOT be NULL).
 * @param[out] aTtl          A pointer to an `uint32_t` to output TTL for the address. It can be NULL if caller does
 *                           not want to get the TTL.
 *
 * @retval OT_ERROR_NONE       The address was read successfully.
 * @retval OT_ERROR_NOT_FOUND  No address record for @p aHostname in @p aResponse at @p aIndex.
 * @retval OT_ERROR_PARSE      Could not parse the records in the @p aResponse.
 *
 */
otError otDnsBrowseResponseGetHostAddress(const otDnsBrowseResponse *aResponse,
                                          const char                *aHostName,
                                          uint16_t                   aIndex,
                                          otIp6Address              *aAddress,
                                          uint32_t                  *aTtl);

/**
 * An opaque representation of a response to a service instance resolution DNS query.
 *
 * Pointers to instance of this type are provided from callback `otDnsAddressCallback`.
 *
 */
typedef struct otDnsServiceResponse otDnsServiceResponse;

/**
 * Pointer is called when a DNS response is received for a service instance resolution query.
 *
 * Within this callback the user can use `otDnsServiceResponseGet{Item}()` functions along with the @p aResponse
 * pointer to get more info about the response.
 *
 * The @p aResponse pointer can only be used within this callback and after returning from this function it will not
 * stay valid, so the user MUST NOT retain the @p aResponse pointer for later use.
 *
 * @param[in]  aError     The result of the DNS transaction.
 * @param[in]  aResponse  A pointer to the response (it is always non-NULL).
 * @param[in]  aContext   A pointer to application-specific context.
 *
 * For the full list of possible values for @p aError, please see `otDnsAddressCallback()`.
 *
 */
typedef void (*otDnsServiceCallback)(otError aError, const otDnsServiceResponse *aResponse, void *aContext);

/**
 * Starts a DNS service instance resolution for a given service instance.
 *
 * Is available when `OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE` is enabled.
 *
 * The @p aConfig can be NULL. In this case the default config (from `otDnsClientGetDefaultConfig()`) will be used as
 * the config for this query. In a non-NULL @p aConfig, some of the fields can be left unspecified (value zero). The
 * unspecified fields are then replaced by the values from the default config.
 *
 * The function sends queries for SRV and/or TXT records for the given service instance. The `mServiceMode` field in
 * `otDnsQueryConfig` determines which records to query (SRV only, TXT only, or both SRV and TXT) and how to perform
 * the query (together in the same message, separately in parallel, or in optimized mode where client will try in the
 * same message first and then separately if it fails to get a response).
 *
 * The SRV record provides information about service port, priority, and weight along with the host name associated
 * with the service instance. This function DOES NOT perform address resolution for the host name discovered from SRV
 * record. The server/resolver may provide AAAA/A record(s) for the host name in the Additional Data section of the
 * response to SRV/TXT query and this information can be retrieved using `otDnsServiceResponseGetServiceInfo()` in
 * `otDnsServiceCallback`. Users of this API MUST NOT assume that host address will always be available from
 * `otDnsServiceResponseGetServiceInfo()`.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aInstanceLabel     The service instance label.
 * @param[in]  aServiceName       The service name (together with @p aInstanceLabel form full instance name).
 * @param[in]  aCallback          A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext           A pointer to arbitrary context information.
 * @param[in]  aConfig            A pointer to the config to use for this query.
 *
 * @retval OT_ERROR_NONE          Query sent successfully. @p aCallback will be invoked to report the status.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer to prepare and send query.
 * @retval OT_ERROR_INVALID_ARGS  @p aInstanceLabel is NULL.
 *
 */
otError otDnsClientResolveService(otInstance             *aInstance,
                                  const char             *aInstanceLabel,
                                  const char             *aServiceName,
                                  otDnsServiceCallback    aCallback,
                                  void                   *aContext,
                                  const otDnsQueryConfig *aConfig);

/**
 * Starts a DNS service instance resolution for a given service instance, with a potential follow-up
 * address resolution for the host name discovered for the service instance.
 *
 * Is available when `OPENTHREAD_CONFIG_DNS_CLIENT_SERVICE_DISCOVERY_ENABLE` is enabled.
 *
 * The @p aConfig can be NULL. In this case the default config (from `otDnsClientGetDefaultConfig()`) will be used as
 * the config for this query. In a non-NULL @p aConfig, some of the fields can be left unspecified (value zero). The
 * unspecified fields are then replaced by the values from the default config. This function cannot be used with
 * `mServiceMode` in DNS config set to `OT_DNS_SERVICE_MODE_TXT` (i.e., querying for TXT record only) and will return
 * `OT_ERROR_INVALID_ARGS`.
 *
 * Behaves similarly to `otDnsClientResolveService()` sending queries for SRV and TXT records. However,
 * if the server/resolver does not provide AAAA/A records for the host name in the response to SRV query (in the
 * Additional Data section), it will perform host name resolution (sending an AAAA query) for the discovered host name
 * from the SRV record. The callback @p aCallback is invoked when responses for all queries are received (i.e., both
 * service and host address resolutions are finished).
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aInstanceLabel     The service instance label.
 * @param[in]  aServiceName       The service name (together with @p aInstanceLabel form full instance name).
 * @param[in]  aCallback          A function pointer that shall be called on response reception or time-out.
 * @param[in]  aContext           A pointer to arbitrary context information.
 * @param[in]  aConfig            A pointer to the config to use for this query.
 *
 * @retval OT_ERROR_NONE          Query sent successfully. @p aCallback will be invoked to report the status.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer to prepare and send query.
 * @retval OT_ERROR_INVALID_ARGS  @p aInstanceLabel is NULL, or @p aConfig is invalid.
 *
 */
otError otDnsClientResolveServiceAndHostAddress(otInstance             *aInstance,
                                                const char             *aInstanceLabel,
                                                const char             *aServiceName,
                                                otDnsServiceCallback    aCallback,
                                                void                   *aContext,
                                                const otDnsQueryConfig *aConfig);

/**
 * Gets the service instance name associated with a DNS service instance resolution response.
 *
 * MUST only be used from `otDnsServiceCallback`.
 *
 * @param[in]  aResponse         A pointer to the response.
 * @param[out] aLabelBuffer      A buffer to char array to output the service instance label (MUST NOT be NULL).
 * @param[in]  aLabelBufferSize  The size of @p aLabelBuffer.
 * @param[out] aNameBuffer       A buffer to char array to output the rest of service name (can be NULL if user is
 *                               not interested in getting the name.
 * @param[in]  aNameBufferSize   The size of @p aNameBuffer.
 *
 * @retval OT_ERROR_NONE     The service name was read successfully.
 * @retval OT_ERROR_NO_BUFS  Either the label or name does not fit in the given buffers.
 *
 */
otError otDnsServiceResponseGetServiceName(const otDnsServiceResponse *aResponse,
                                           char                       *aLabelBuffer,
                                           uint8_t                     aLabelBufferSize,
                                           char                       *aNameBuffer,
                                           uint16_t                    aNameBufferSize);

/**
 * Gets info for a service instance from a DNS service instance resolution response.
 *
 * MUST only be used from a `otDnsServiceCallback` triggered from `otDnsClientResolveService()` or
 * `otDnsClientResolveServiceAndHostAddress()`.
 *
 * When this is is used from a `otDnsClientResolveService()` callback, the DNS response from server/resolver may
 * include AAAA records in its Additional Data section for the host name associated with the service instance that is
 * resolved. This is a SHOULD and not a MUST requirement so servers/resolvers are not required to provide this. This
 * function attempts to parse AAAA record(s) if included in the response. If it is not included `mHostAddress` is set
 * to all zeros (unspecified address). To also resolve the host address, user can use the DNS client API function
 * `otDnsClientResolveServiceAndHostAddress()` which will perform service resolution followed up by a host name
 * address resolution query (when AAAA records are not provided by server/resolver in the SRV query response).
 *
 * - If a matching SRV record is found in @p aResponse, @p aServiceInfo is updated.
 * - If no matching SRV record is found, `OT_ERROR_NOT_FOUND` is returned unless the query config for this query
 *   used `OT_DNS_SERVICE_MODE_TXT` for `mServiceMode` (meaning the request was only for TXT record). In this case, we
 *   still try to parse the SRV record from Additional Data Section of response (in case server provided the info).
 * - If no matching TXT record is found in @p aResponse, `mTxtDataSize` in @p aServiceInfo is set to zero.
 * - If TXT data length is greater than `mTxtDataSize`, it is read partially and `mTxtDataTruncated` is set to true.
 * - If no matching AAAA record is found in @p aResponse, `mHostAddress is set to all zero or unspecified address.
 * - If there are multiple AAAA records for the host name in @p aResponse, `mHostAddress` is set to the first one. The
 *   other addresses can be retrieved using `otDnsServiceResponseGetHostAddress()`.
 *
 * @param[in]  aResponse          A pointer to the response.
 * @param[out] aServiceInfo       A `ServiceInfo` to output the service instance information (MUST NOT be NULL).
 *
 * @retval OT_ERROR_NONE          The service instance info was read. @p aServiceInfo is updated.
 * @retval OT_ERROR_NOT_FOUND     Could not find a required record in @p aResponse.
 * @retval OT_ERROR_NO_BUFS       The host name and/or TXT data could not fit in the given buffers.
 * @retval OT_ERROR_PARSE         Could not parse the records in the @p aResponse.
 *
 */
otError otDnsServiceResponseGetServiceInfo(const otDnsServiceResponse *aResponse, otDnsServiceInfo *aServiceInfo);

/**
 * Gets the host IPv6 address from a DNS service instance resolution response.
 *
 * MUST only be used from `otDnsServiceCallback`.
 *
 * The response can include zero or more IPv6 address records. @p aIndex can be used to iterate through the list of
 * addresses. Index zero gets the first address and so on. When we reach end of the list, `OT_ERROR_NOT_FOUND` is
 * returned.
 *
 * @param[in]  aResponse     A pointer to the response.
 * @param[in]  aHostName     The host name to get the address (MUST NOT be NULL).
 * @param[in]  aIndex        The address record index to retrieve.
 * @param[out] aAddress      A pointer to a IPv6 address to output the address (MUST NOT be NULL).
 * @param[out] aTtl          A pointer to an `uint32_t` to output TTL for the address. It can be NULL if caller does
 *                           not want to get the TTL.
 *
 * @retval OT_ERROR_NONE       The address was read successfully.
 * @retval OT_ERROR_NOT_FOUND  No address record for @p aHostname in @p aResponse at @p aIndex.
 * @retval OT_ERROR_PARSE      Could not parse the records in the @p aResponse.
 *
 */
otError otDnsServiceResponseGetHostAddress(const otDnsServiceResponse *aResponse,
                                           const char                 *aHostName,
                                           uint16_t                    aIndex,
                                           otIp6Address               *aAddress,
                                           uint32_t                   *aTtl);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DNS_CLIENT_H_
