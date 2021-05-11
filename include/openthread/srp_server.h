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
 *
 */

/**
 * This opaque type represents a SRP service host.
 *
 */
typedef void otSrpServerHost;

/**
 * This opaque type represents a SRP service.
 *
 */
typedef void otSrpServerService;

/**
 * The ID of a SRP service update transaction on the SRP Server.
 *
 */
typedef uint32_t otSrpServerServiceUpdateId;

/**
 * This structure includes SRP server LEASE and KEY-LEASE configurations.
 *
 */
typedef struct otSrpServerLeaseConfig
{
    uint32_t mMinLease;    ///< The minimum LEASE interval in seconds.
    uint32_t mMaxLease;    ///< The maximum LEASE interval in seconds.
    uint32_t mMinKeyLease; ///< The minimum KEY-LEASE interval in seconds.
    uint32_t mMaxKeyLease; ///< The maximum KEY-LEASE interval in seconds.
} otSrpServerLeaseConfig;

/**
 * This function returns the domain authorized to the SRP server.
 *
 * If the domain if not set by SetDomain, "default.service.arpa." will be returned.
 * A trailing dot is always appended even if the domain is set without it.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns A pointer to the dot-joined domain string.
 *
 */
const char *otSrpServerGetDomain(otInstance *aInstance);

/**
 * This function sets the domain on the SRP server.
 *
 * A trailing dot will be appended to @p aDomain if it is not already there.
 * This function should only be called before the SRP server is enabled.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDomain    The domain to be set. MUST NOT be NULL.
 *
 * @retval  OT_ERROR_NONE           Successfully set the domain to @p aDomain.
 * @retval  OT_ERROR_INVALID_STATE  The SRP server is already enabled and the Domain cannot be changed.
 * @retval  OT_ERROR_INVALID_ARGS   The argument @p aDomain is not a valid DNS domain name.
 * @retval  OT_ERROR_NO_BUFS        There is no memory to store content of @p aDomain.
 *
 */
otError otSrpServerSetDomain(otInstance *aInstance, const char *aDomain);

/**
 * This function enables/disables the SRP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aEnabled   A boolean to enable/disable the SRP server.
 *
 */
void otSrpServerSetEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function returns SRP server LEASE and KEY-LEASE configurations.
 *
 * @param[in]   aInstance     A pointer to an OpenThread instance.
 * @param[out]  aLeaseConfig  A pointer to an `otSrpServerLeaseConfig` instance.
 *
 */
void otSrpServerGetLeaseConfig(otInstance *aInstance, otSrpServerLeaseConfig *aLeaseConfig);

/**
 * This function sets SRP server LEASE and KEY-LEASE configurations.
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
 *
 */
otError otSrpServerSetLeaseConfig(otInstance *aInstance, const otSrpServerLeaseConfig *aLeaseConfig);

/**
 * This function handles SRP service updates.
 *
 * This function is called by the SRP server to notify that a SRP host and possibly SRP services
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
 *
 */
typedef void (*otSrpServerServiceUpdateHandler)(otSrpServerServiceUpdateId aId,
                                                const otSrpServerHost *    aHost,
                                                uint32_t                   aTimeout,
                                                void *                     aContext);

/**
 * This function sets the SRP service updates handler on SRP server.
 *
 * @param[in]  aInstance        A pointer to an OpenThread instance.
 * @param[in]  aServiceHandler  A pointer to a service handler. Use NULL to remove the handler.
 * @param[in]  aContext         A pointer to arbitrary context information.
 *                              May be NULL if not used.
 *
 */
void otSrpServerSetServiceUpdateHandler(otInstance *                    aInstance,
                                        otSrpServerServiceUpdateHandler aServiceHandler,
                                        void *                          aContext);

/**
 * This function reports the result of processing a SRP update to the SRP server.
 *
 * The Service Update Handler should call this function to return the result of its
 * processing of a SRP update.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aId        The service update transaction ID. This should be the same ID
 *                        provided via `otSrpServerServiceUpdateHandler`.
 * @param[in]  aError     An error to be returned to the SRP server. Use OT_ERROR_DUPLICATED
 *                        to represent DNS name conflicts.
 *
 */
void otSrpServerHandleServiceUpdateResult(otInstance *aInstance, otSrpServerServiceUpdateId aId, otError aError);

/**
 * This function returns the next registered host on the SRP server.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aHost      A pointer to current host; use NULL to get the first host.
 *
 * @returns  A pointer to the registered host. NULL, if no more hosts can be found.
 *
 */
const otSrpServerHost *otSrpServerGetNextHost(otInstance *aInstance, const otSrpServerHost *aHost);

/**
 * This function tells if the SRP service host has been deleted.
 *
 * A SRP service host can be deleted but retains its name for future uses.
 * In this case, the host instance is not removed from the SRP server/registry.
 *
 * @param[in]  aHost  A pointer to the SRP service host.
 *
 * @returns  TRUE if the host has been deleted, FALSE if not.
 *
 */
bool otSrpServerHostIsDeleted(const otSrpServerHost *aHost);

/**
 * This function returns the full name of the host.
 *
 * @param[in]  aHost  A pointer to the SRP service host.
 *
 * @returns  A pointer to the null-terminated host name string.
 *
 */
const char *otSrpServerHostGetFullName(const otSrpServerHost *aHost);

/**
 * This function returns the addresses of given host.
 *
 * @param[in]   aHost          A pointer to the SRP service host.
 * @param[out]  aAddressesNum  A pointer to where we should output the number of the addresses to.
 *
 * @returns  A pointer to the array of IPv6 Address.
 *
 */
const otIp6Address *otSrpServerHostGetAddresses(const otSrpServerHost *aHost, uint8_t *aAddressesNum);

/**
 * This function returns the next service of given host.
 *
 * @param[in]  aHost     A pointer to the SRP service host.
 * @param[in]  aService  A pointer to current SRP service instance; use NULL to get the first service.
 *
 * @returns  A pointer to the next service or NULL if there is no more services.
 *
 */
const otSrpServerService *otSrpServerHostGetNextService(const otSrpServerHost *   aHost,
                                                        const otSrpServerService *aService);

/**
 * This function tells if the SRP service has been deleted.
 *
 * A SRP service can be deleted but retains its name for future uses.
 * In this case, the service instance is not removed from the SRP server/registry.
 * It is guaranteed that all services are deleted if the host is deleted.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  TRUE if the service has been deleted, FALSE if not.
 *
 */
bool otSrpServerServiceIsDeleted(const otSrpServerService *aService);

/**
 * This function returns the full name of the service.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  A pointer to the null-terminated service name string.
 *
 */
const char *otSrpServerServiceGetFullName(const otSrpServerService *aService);

/**
 * This function returns the port of the service instance.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  The port of the service.
 *
 */
uint16_t otSrpServerServiceGetPort(const otSrpServerService *aService);

/**
 * This function returns the weight of the service instance.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  The weight of the service.
 *
 */
uint16_t otSrpServerServiceGetWeight(const otSrpServerService *aService);

/**
 * This function returns the priority of the service instance.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  The priority of the service.
 *
 */
uint16_t otSrpServerServiceGetPriority(const otSrpServerService *aService);

/**
 * This function returns the TXT record data of the service instance.
 *
 * @param[in]  aService        A pointer to the SRP service.
 * @param[out] aDataLength     A pointer to return the TXT record data length. MUST NOT be NULL.
 *
 * @returns A pointer to the buffer containing the TXT record data (the TXT data length is returned in @p aDataLength).
 *
 */
const uint8_t *otSrpServerServiceGetTxtData(const otSrpServerService *aService, uint16_t *aDataLength);

/**
 * This function returns the host which the service instance reside on.
 *
 * @param[in]  aService  A pointer to the SRP service.
 *
 * @returns  A pointer to the host instance.
 *
 */
const otSrpServerHost *otSrpServerServiceGetHost(const otSrpServerService *aService);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_SRP_SERVER_H_
