/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *  This file defines the API for server of the Service Registration Protocol (SRP).
 */

#ifndef OPENTHREAD_SRP_SERVER_H_
#define OPENTHREAD_SRP_SERVER_H_

#include <stdint.h>

#include <openthread/dns.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-srp
 *
 * @brief
 *  This module includes functions of the Service Registration Protocol.
 *
 * @{
 */

/**
 * This opaque type represents a SRP service host.
 */
typedef struct otSrpServerHost otSrpServerHost;

/**
 * This opaque type represents a SRP service.
 */
typedef struct otSrpServerService otSrpServerService;

/**
 * The ID of a SRP service update transaction on the SRP Server.
 */
typedef uint32_t otSrpServerServiceUpdateId;

/**
 * Represents the state of the SRP server.
 */
typedef enum
{
    OT_SRP_SERVER_STATE_DISABLED = 0, ///< The SRP server is disabled.
    OT_SRP_SERVER_STATE_RUNNING  = 1, ///< The SRP server is enabled and running.
    OT_SRP_SERVER_STATE_STOPPED  = 2, ///< The SRP server is enabled but stopped.
} otSrpServerState;

/**
 * Represents the address mode used by the SRP server.
 *
 * Address mode specifies how the address and port number are determined by the SRP server and how this info is
 * published in the Thread Network Data.
 */
typedef enum otSrpServerAddressMode
{
    OT_SRP_SERVER_ADDRESS_MODE_UNICAST = 0, ///< Unicast address mode.
    OT_SRP_SERVER_ADDRESS_MODE_ANYCAST = 1, ///< Anycast address mode.
} otSrpServerAddressMode;

/**
 * Includes SRP server TTL configurations.
 */
typedef struct otSrpServerTtlConfig
{
    uint32_t mMinTtl; ///< The minimum TTL in seconds.
    uint32_t mMaxTtl; ///< The maximum TTL in seconds.
} otSrpServerTtlConfig;

/**
 * Includes SRP server LEASE and KEY-LEASE configurations.
 */
typedef struct otSrpServerLeaseConfig
{
    uint32_t mMinLease;    ///< The minimum LEASE interval in seconds.
    uint32_t mMaxLease;    ///< The maximum LEASE interval in seconds.
    uint32_t mMinKeyLease; ///< The minimum KEY-LEASE interval in seconds.
    uint32_t mMaxKeyLease; ///< The maximum KEY-LEASE interval in seconds.
} otSrpServerLeaseConfig;

/**
 * Includes SRP server lease information of a host/service.
 */
typedef struct otSrpServerLeaseInfo
{
    uint32_t mLease;             ///< The lease time of a host/service in milliseconds.
    uint32_t mKeyLease;          ///< The key lease time of a host/service in milliseconds.
    uint32_t mRemainingLease;    ///< The remaining lease time of the host/service in milliseconds.
    uint32_t mRemainingKeyLease; ///< The remaining key lease time of a host/service in milliseconds.
} otSrpServerLeaseInfo;

/**
 * Includes the statistics of SRP server responses.
 */
typedef struct otSrpServerResponseCounters
{
    uint32_t mSuccess;       ///< The number of successful responses.
    uint32_t mServerFailure; ///< The number of server failure responses.
    uint32_t mFormatError;   ///< The number of format error responses.
    uint32_t mNameExists;    ///< The number of 'name exists' responses.
    uint32_t mRefused;       ///< The number of refused responses.
    uint32_t mOther;         ///< The number of other responses.
} otSrpServerResponseCounters;

/**
 * Returns the domain authorized to the SRP server.
 *
 * If the domain if not set by SetDomain, "default.service.arpa." will be returned.
 * A trailing dot is always appended even if the domain is set without it.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the dot-joined domain string.
 */
const char *otSrpServerGetDomain(otInstance *aInstance);

/**
 * Sets the domain on the SRP server.
 *
 * A trailing dot will be appended to @p aDomain if it is not already there.
 * Should only be called before the SRP server is enabled.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDomain    The domain to be set. MUST NOT be NULL.
 *
 * @retval  OT_ERROR_NONE           Successfully set the domain to @p aDomain.
 * @retval  OT_ERROR_INVALID_STATE  The SRP server is already enabled and the Domain cannot be changed.
 * @retval  OT_ERROR_INVALID_ARGS   The argument @p aDomain is not a valid DNS domain name.
 * @retval  OT_ERROR_NO_BUFS        There is no memory to store content of @p aDomain.
 */
otError otSrpServerSetDomain(otInstance *aInstance, const char *aDomain);

/**
 * Returns the state of the SRP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns The current state of the SRP server.
 */
otSrpServerState otSrpServerGetState(otInstance *aInstance);

/**
 * Returns the port the SRP server is listening to.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns  The port of the SRP server. It returns 0 if the server is not running.
 */
uint16_t otSrpServerGetPort(otInstance *aInstance);

/**
 * Returns the address mode being used by the SRP server.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @returns The SRP server's address mode.
 */
otSrpServerAddressMode otSrpServerGetAddressMode(otInstance *aInstance);

/**
 * Sets the address mode to be used by the SRP server.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aMode      The address mode to use.
 *
 * @retval OT_ERROR_NONE           Successfully set the address mode.
 * @retval OT_ERROR_INVALID_STATE  The SRP server is enabled and the address mode cannot be changed.
 */
otError otSrpServerSetAddressMode(otInstance *aInstance, otSrpServerAddressMode aMode);

/**
 * Returns the sequence number used with anycast address mode.
 *
 * The sequence number is included in "DNS/SRP Service Anycast Address" entry published in the Network Data.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 *
 * @returns The anycast sequence number.
 */
uint8_t otSrpServerGetAnycastModeSequenceNumber(otInstance *aInstance);

/**
 * Sets the sequence number used with anycast address mode.
 *
 * @param[in] aInstance        A pointer to an OpenThread instance.
 * @param[in] aSequenceNumber  The sequence number to use.
 *
 * @retval OT_ERROR_NONE            Successfully set the address mode.
 * @retval OT_ERROR_INVALID_STATE   The SRP server is enabled and the sequence number cannot be changed.
 */
otError otSrpServerSetAnycastModeSequenceNumber(otInstance *aInstance, uint8_t aSequenceNumber);

/**
 * Enables/disables the SRP server.
 *
 * On a Border Router, it is recommended to use `otSrpServerSetAutoEnableMode()` instead.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aEnabled   A boolean to enable/disable the SRP server.
 */
void otSrpServerSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * Enables/disables the auto-enable mode on SRP server.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` feature.
 *
 * When this mode is enabled, the Border Routing Manager controls if/when to enable or disable the SRP server.
 * SRP sever is auto-enabled if/when Border Routing is started and it is done with the initial prefix and route
 * configurations (when the OMR and on-link prefixes are determined, advertised in emitted Router Advertisement message
 * on infrastructure side and published in the Thread Network Data). The SRP server is auto-disabled if/when BR is
 * stopped (e.g., if the infrastructure network interface is brought down or if BR gets detached).
 *
 * This mode can be disabled by a `otSrpServerSetAutoEnableMode()` call with @p aEnabled set to `false` or if the SRP
 * server is explicitly enabled or disabled by a call to `otSrpServerSetEnabled()` function. Disabling auto-enable mode
 * using `otSrpServerSetAutoEnableMode(false)` will not change the current state of SRP sever (e.g., if it is enabled
 * it stays enabled).
 *
 * @param[in] aInstance   A pointer to an OpenThread instance.
 * @param[in] aEnabled    A boolean to enable/disable the auto-enable mode.
 */
void otSrpServerSetAutoEnableMode(otInstance *aInstance, bool aEnabled);

/**
 * Indicates whether the auto-enable mode is enabled or disabled.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` feature.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval TRUE   The auto-enable mode is enabled.
 * @retval FALSE  The auto-enable mode is disabled.
 */
bool otSrpServerIsAutoEnableMode(otInstance *aInstance);

/**
 * Returns SRP server TTL configuration.
 *
 * @param[in]   aInstance   A pointer to an OpenThread instance.
 * @param[out]  aTtlConfig  A pointer to an `otSrpServerTtlConfig` instance.
 */
void otSrpServerGetTtlConfig(otInstance *aInstance, otSrpServerTtlConfig *aTtlConfig);

/**
 * Sets SRP server TTL configuration.
 *
 * The granted TTL will always be no greater than the max lease interval configured via `otSrpServerSetLeaseConfig()`,
 * regardless of the minimum and maximum TTL configuration.
 *
 * @param[in]  aInstance   A pointer to an OpenThread instance.
 * @param[in]  aTtlConfig  A pointer to an `otSrpServerTtlConfig` instance.
 *
 * @retval  OT_ERROR_NONE          Successfully set the TTL configuration.
 * @retval  OT_ERROR_INVALID_ARGS  The TTL configuration is not valid.
 */
otError otSrpServerSetTtlConfig(otInstance *aInstance, const otSrpServerTtlConfig *aTtlConfig);

/**
 * Returns SRP server LEASE and KEY-LEASE configurations.
 *
 * @param[in]   aInstance     A pointer to an OpenThread instance.
 * @param[out]  aLeaseConfig  A pointer to an `otSrpServerLeaseConfig` instance.
 */
void otSrpServerGetLeaseConfig(otInstance *aInstance, otSrpServerLeaseConfig *aLeaseConfig);

/**
 * Sets SRP server LEASE and KEY-LEASE configurations.
 *
 * When a non-zero LEASE time is requested from a client, the granted value will be
 * limited in range [aMinLease, aMaxLease]; and a non-zero KEY-LEASE will be granted
 * in range [aMinKeyLease, aMaxKeyLease]. For zero LEASE or KEY-LEASE time, zero will
 * be granted.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aLeaseConfig  A pointer to an `otSrpServerLeaseConfig` instance.
 *
 * @retval  OT_ERROR_NONE          Successfully set the LEASE and KEY-LEASE ranges.
 * @retval  OT_ERROR_INVALID_ARGS  The LEASE or KEY-LEASE range is not valid.
 */
otError otSrpServerSetLeaseConfig(otInstance *aInstance, const otSrpServerLeaseConfig *aLeaseConfig);

/**
 * Handles SRP service updates.
 *
 * Is called by the SRP server to notify that a SRP host and possibly SRP services
 * are being updated. It is important that the SRP updates are not committed until the handler
 * returns the result by calling otSrpServerHandleServiceUpdateResult or times out after @p aTimeout.
 *
 * A SRP service observer should always call otSrpServerHandleServiceUpdateResult with error code
 * OT_ERROR_NONE immediately after receiving the update events.
 *
 * A more generic handler may perform validations on the SRP host/services and rejects the SRP updates
 * if any validation fails. For example, an Advertising Proxy should advertise (or remove) the host and
 * services on a multicast-capable link and returns specific error code if any failure occurs.
 *
 * @param[in]  aId       The service update transaction ID. This ID must be passed back with
 *                       `otSrpServerHandleServiceUpdateResult`.
 * @param[in]  aHost     A pointer to the otSrpServerHost object which contains the SRP updates. The
 *                       handler should publish/un-publish the host and each service points to this
 *                       host with below rules:
 *                         1. If the host is not deleted (indicated by `otSrpServerHostIsDeleted`),
 *                            then it should be published or updated with mDNS. Otherwise, the host
 *                            should be un-published (remove AAAA RRs).
 *                         2. For each service points to this host, it must be un-published if the host
 *                            is to be un-published. Otherwise, the handler should publish or update the
 *                            service when it is not deleted (indicated by `otSrpServerServiceIsDeleted`)
 *                            and un-publish it when deleted.
 * @param[in]  aTimeout  The maximum time in milliseconds for the handler to process the service event.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 * @sa otSrpServerSetServiceUpdateHandler
 * @sa otSrpServerHandleServiceUpdateResult
 */
typedef void (*otSrpServerServiceUpdateHandler)(otSrpServerServiceUpdateId aId,
                                                const otSrpServerHost     *aHost,
                                                uint32_t                   aTimeout,
                                                void                      *aContext);

/**
 * Sets the SRP service updates handler on SRP server.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aServiceHandler  A pointer to a service handler. Use NULL to remove the handler.
 * @param[in]  aContext         A pointer to arbitrary context information.
 *                              May be NULL if not used.
 */
void otSrpServerSetServiceUpdateHandler(otInstance                     *aInstance,
                                        otSrpServerServiceUpdateHandler aServiceHandler,
                                        void                           *aContext);

/**
 * Reports the result of processing a SRP update to the SRP server.
 *
 * The Service Update Handler should call this function to return the result of its
 * processing of a SRP update.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aId        The service update transaction ID. This should be the same ID
 *                        provided via `otSrpServerServiceUpdateHandler`.
 * @param[in]  aError     An error to be returned to the SRP server. Use OT_ERROR_DUPLICATED
 *                        to represent DNS name conflicts.
 */
void otSrpServerHandleServiceUpdateResult(otInstance *aInstance, otSrpServerServiceUpdateId aId, otError aError);

/**
 * Returns the next registered host on the SRP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aHost      A pointer to current host; use NULL to get the first host.
 *
 * @returns  A pointer to the registered host. NULL, if no more hosts can be found.
 */
const otSrpServerHost *otSrpServerGetNextHost(otInstance *aInstance, const otSrpServerHost *aHost);

/**
 * Returns the response counters of the SRP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns  A pointer to the response counters of the SRP server.
 */
const otSrpServerResponseCounters *otSrpServerGetResponseCounters(otInstance *aInstance);

/**
 * Tells if the SRP service host has been deleted.
 *
 * A SRP service host can be deleted but retains its name for future uses.
 * In this case, the host instance is not removed from the SRP server/registry.
 *
 * @param[in]  aHost  A pointer to the SRP service host.
 *
 * @returns  TRUE if the host has been deleted, FALSE if not.
 */
bool otSrpServerHostIsDeleted(const otSrpServerHost *aHost);

/**
 * Returns the full name of the host.
 *
 * @param[in]  aHost  A pointer to the SRP service host.
 *
 * @returns  A pointer to the null-terminated host name string.
 */
const char *otSrpServerHostGetFullName(const otSrpServerHost *aHost);

/**
 * Indicates whether the host matches a given host name.
 *
 * DNS name matches are performed using a case-insensitive string comparison (i.e., "Abc" and "aBc" are considered to
 * be the same).
 *
 * @param[in]  aHost       A pointer to the SRP service host.
 * @param[in]  aFullName   A full host name.
 *
 * @retval  TRUE   If host matches the host name.
 * @retval  FALSE  If host does not match the host name.
 */
bool otSrpServerHostMatchesFullName(const otSrpServerHost *aHost, const char *aFullName);

/**
 * Returns the addresses of given host.
 *
 * @param[in]   aHost          A pointer to the SRP service host.
 * @param[out]  aAddressesNum  A pointer to where we should output the number of the addresses to.
 *
 * @returns  A pointer to the array of IPv6 Address.
 */
const otIp6Address *otSrpServerHostGetAddresses(const otSrpServerHost *aHost, uint8_t *aAddressesNum);

/**
 * Returns the LEASE and KEY-LEASE information of a given host.
 *
 * @param[in]   aHost       A pointer to the SRP server host.
 * @param[out]  aLeaseInfo  A pointer to where to output the LEASE and KEY-LEASE information.
 */
void otSrpServerHostGetLeaseInfo(const otSrpServerHost *aHost, otSrpServerLeaseInfo *aLeaseInfo);

/**
 * Returns the next service of given host.
 *
 * @param[in]  aHost     A pointer to the SRP service host.
 * @param[in]  aService  A pointer to current SRP service instance; use NULL to get the first service.
 *
 * @returns  A pointer to the next service or NULL if there is no more services.
 */
const otSrpServerService *otSrpServerHostGetNextService(const otSrpServerHost    *aHost,
                                                        const otSrpServerService *aService);

/**
 * Indicates whether or not the SRP service has been deleted.
 *
 * A SRP service can be deleted but retains its name for future uses.
 * In this case, the service instance is not removed from the SRP server/registry.
 * It is guaranteed that all services are deleted if the host is deleted.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  TRUE if the service has been deleted, FALSE if not.
 */
bool otSrpServerServiceIsDeleted(const otSrpServerService *aService);

/**
 * Returns the full service instance name of the service.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  A pointer to the null-terminated service instance name string.
 */
const char *otSrpServerServiceGetInstanceName(const otSrpServerService *aService);

/**
 * Indicates whether this service matches a given service instance name.
 *
 * DNS name matches are performed using a case-insensitive string comparison (i.e., "Abc" and "aBc" are considered to
 * be the same).
 *
 * @param[in]  aService       A pointer to the SRP service.
 * @param[in]  aInstanceName  The service instance name.
 *
 * @retval  TRUE   If service matches the service instance name.
 * @retval  FALSE  If service does not match the service instance name.
 */
bool otSrpServerServiceMatchesInstanceName(const otSrpServerService *aService, const char *aInstanceName);

/**
 * Returns the service instance label (first label in instance name) of the service.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  A pointer to the null-terminated service instance label string..
 */
const char *otSrpServerServiceGetInstanceLabel(const otSrpServerService *aService);

/**
 * Returns the full service name of the service.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  A pointer to the null-terminated service name string.
 */
const char *otSrpServerServiceGetServiceName(const otSrpServerService *aService);

/**
 * Indicates whether this service matches a given service name.
 *
 * DNS name matches are performed using a case-insensitive string comparison (i.e., "Abc" and "aBc" are considered to
 * be the same).
 *
 * @param[in]  aService       A pointer to the SRP service.
 * @param[in]  aServiceName  The service  name.
 *
 * @retval  TRUE   If service matches the service name.
 * @retval  FALSE  If service does not match the service name.
 */
bool otSrpServerServiceMatchesServiceName(const otSrpServerService *aService, const char *aServiceName);

/**
 * Gets the number of sub-types of the service.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns The number of sub-types of @p aService.
 */
uint16_t otSrpServerServiceGetNumberOfSubTypes(const otSrpServerService *aService);

/**
 * Gets the sub-type service name (full name) of the service at a given index
 *
 * The full service name for a sub-type service follows "<sub-label>._sub.<service-labels>.<domain>.".
 *
 * @param[in]  aService  A pointer to the SRP service.
 * @param[in] aIndex     The index to get.
 *
 * @returns A pointer to sub-type service name at @p aIndex, or `NULL` if no sub-type at this index.
 */
const char *otSrpServerServiceGetSubTypeServiceNameAt(const otSrpServerService *aService, uint16_t aIndex);

/**
 * Indicates whether or not the service has a given sub-type.
 *
 * DNS name matches are performed using a case-insensitive string comparison (i.e., "Abc" and "aBc" are considered to
 * be the same).
 *
 * @param[in] aService             A pointer to the SRP service.
 * @param[in] aSubTypeServiceName  The sub-type service name (full name) to check.
 *
 * @retval TRUE   Service contains the sub-type @p aSubTypeServiceName.
 * @retval FALSE  Service does not contain the sub-type @p aSubTypeServiceName.
 */
bool otSrpServerServiceHasSubTypeServiceName(const otSrpServerService *aService, const char *aSubTypeServiceName);

/**
 * Parses a sub-type service name (full name) and extracts the sub-type label.
 *
 * The full service name for a sub-type service follows "<sub-label>._sub.<service-labels>.<domain>.".
 *
 * @param[in]  aSubTypeServiceName  A sub-type service name (full name).
 * @param[out] aLabel               A pointer to a buffer to copy the extracted sub-type label.
 * @param[in]  aLabelSize           Maximum size of @p aLabel buffer.
 *
 * @retval OT_ERROR_NONE          Name was successfully parsed and @p aLabel was updated.
 * @retval OT_ERROR_NO_BUFS       The sub-type label could not fit in @p aLabel buffer (number of chars from label
 *                                that could fit are copied in @p aLabel ensuring it is null-terminated).
 * @retval OT_ERROR_INVALID_ARGS  @p aSubTypeServiceName is not a valid sub-type format.
 */
otError otSrpServerParseSubTypeServiceName(const char *aSubTypeServiceName, char *aLabel, uint8_t aLabelSize);

/**
 * Returns the port of the service instance.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  The port of the service.
 */
uint16_t otSrpServerServiceGetPort(const otSrpServerService *aService);

/**
 * Returns the weight of the service instance.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  The weight of the service.
 */
uint16_t otSrpServerServiceGetWeight(const otSrpServerService *aService);

/**
 * Returns the priority of the service instance.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  The priority of the service.
 */
uint16_t otSrpServerServiceGetPriority(const otSrpServerService *aService);

/**
 * Returns the TTL of the service instance.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  The TTL of the service instance..
 */
uint32_t otSrpServerServiceGetTtl(const otSrpServerService *aService);

/**
 * Returns the TXT record data of the service instance.
 *
 * @param[in]  aService        A pointer to the SRP service.
 * @param[out] aDataLength     A pointer to return the TXT record data length. MUST NOT be NULL.
 *
 * @returns A pointer to the buffer containing the TXT record data (the TXT data length is returned in @p aDataLength).
 */
const uint8_t *otSrpServerServiceGetTxtData(const otSrpServerService *aService, uint16_t *aDataLength);

/**
 * Returns the host which the service instance reside on.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  A pointer to the host instance.
 */
const otSrpServerHost *otSrpServerServiceGetHost(const otSrpServerService *aService);

/**
 * Returns the LEASE and KEY-LEASE information of a given service.
 *
 * @param[in]   aService    A pointer to the SRP server service.
 * @param[out]  aLeaseInfo  A pointer to where to output the LEASE and KEY-LEASE information.
 */
void otSrpServerServiceGetLeaseInfo(const otSrpServerService *aService, otSrpServerLeaseInfo *aLeaseInfo);
/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_SRP_SERVER_H_
