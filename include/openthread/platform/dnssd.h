/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file includes the platform abstraction for the DNS-SD (e.g., mDNS) on infrastructure network.
 *
 */

#ifndef OPENTHREAD_PLATFORM_DNSSD_H_
#define OPENTHREAD_PLATFORM_DNSSD_H_

#include <stdint.h>

#include <openthread/dns.h>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-dns-sd
 *
 * @brief
 *   This module includes the platform abstraction for the DNS-SD (e.g., mDNS) on infrastructure network.
 *
 * @{
 *
 * The DNS-SD platform API are used only when `OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE` is enabled.
 *
 */

/**
 * Represents state of DNS-SD platform.
 *
 */
typedef enum otPlatDnssdState
{
    OT_PLAT_DNSSD_STOPPED, ///< Stopped and unable to register any service or host, or start any browser/resolver.
    OT_PLAT_DNSSD_READY,   ///< Running and ready to register service or host.
} otPlatDnssdState;

/**
 * Represents an event from infrastructure DNS-SD module.
 *
 * This is used in callbacks `otPlatDnssdHandleServiceBrowseResult()`, `otPlatDnssdHandleIp6AddressResolveResult()`, or
 * `otPlatDnssdHandleIp4AddressResolveResult()`.
 *
 */
typedef enum otPlatDnssdEvent
{
    OT_PLAT_DNSSD_EVENT_ENTRY_ADDED,   ///< Entry (service instance, or IPv6/IPv4 address) is added.
    OT_PLAT_DNSSD_EVENT_ENTRY_REMOVED, ///< Entry (service instance, or IPv6/IPv6 address) is removed.
} otPlatDnssdEvent;

/**
 * Represents a request ID (for registering/unregistering a service or host).
 *
 */
typedef uint32_t otPlatDnssdRequestId;

/**
 * Represents callback function used when registering/unregistering a host or service.
 *
 * See `otPlatDnssdRegisterService()`, `otPlatDnssdUnregisterService()`, `otPlatDnssdRegisterHost()`, and
 * `otPlatDnssdUnregisterHost()` for more details about when to invoke the callback and the `aError` values that can
 * be returned in each case.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aRequestId    The request ID.
 * @param[in] aError        Error indicating the outcome of request.
 *
 */
typedef void (*otPlatDnssdRegisterCallback)(otInstance *aInstance, otPlatDnssdRequestId aRequestId, otError aError);

/**
 * Represents a service instance.
 *
 * This type is used
 * - to report service browser result in `otPlatDnssdHandleServiceBrowseResult()` callback, or
 * - to start or stop a service resolver for a service instance in `otPlatDnssd{Start/Stop}ServiceResolver()`.
 *
 */
typedef struct otPlatDnssdServiceInstance
{
    const char *mServiceType;     ///< Service type or sub-type (e.g., "_mt._udp", "_s1._sub._mt._udp")
    const char *mServiceInstance; ///< Service instance label.
    uint32_t    mTtl;             ///< TTL in seconds.
    uint32_t    mInfraIfIndex;    ///< The infrastructure network interface index.
} otPlatDnssdServiceInstance;

/**
 * Represent a DNS-SD service.
 *
 * This type is used
 * - to register or unregister a service (`otPlatDnssdRegisterService()` and `otPlatDnssdRegisterService()`),
 * - to report result of service resolver (`otPlatDnssdHandleServiceResolveResult()` callback).
 *
 * See the the description of each function/callback for more details on how different fields are used in each case.
 *
 */
typedef struct otPlatDnssdService
{
    const char        *mHostName;            ///< The host name (does not include domain name).
    const char        *mServiceInstance;     ///< The service instance name label (not the full name).
    const char        *mServiceType;         ///< The service type (e.g., "_mt._udp", does not include domain name).
    const char *const *mSubTypeLabels;       ///< Array of sub-type labels (can be NULL if no label).
    uint16_t           mSubTypeLabelsLength; ///< Length of array of sub-type labels.
    const uint8_t     *mTxtData;             ///< Encoded TXT data bytes.
    uint16_t           mTxtDataLength;       ///< Length of TXT data.
    uint16_t           mPort;                ///< The service port number.
    uint16_t           mPriority;            ///< The service priority.
    uint16_t           mWeight;              ///< The service weight.
    uint32_t           mTtl;                 ///< The service TTL in seconds.
    uint32_t           mInfraIfIndex;        ///< The infrastructure network interface index.
} otPlatDnssdService;

/**
 * Represent a DNS-SD host.
 *
 * This type is used
 * - to register or unregister a host (`otPlatDnssdRegisterHost()` and `otPlatDnssdUnregisterHost()`),
 * - to report result of an address resolver `otPlatDnssdHandleIp6AddressResolveResult()` or
 *   `otPlatDnssdHandleIp4AddressResolveResult()` callbacks).
 *
 * See the the description of each function/callback for more details on how different fields are used in each case.
 *
 */
typedef struct otPlatDnssdHost
{
    const char         *mHostName;     ///< The host name (does not include domain name).
    const otIp6Address *mAddresses;    ///< Array of IPv6 host addresses.
    uint16_t            mNumAddresses; ///< Number of entries in @p mAddresses array.
    uint32_t            mTtl;          ///< The host TTL in seconds.
    uint32_t            mInfraIfIndex; ///< The infrastructure network interface index.
} otPlatDnssdHost;

/**
 * Represent a DNS-SD key record.
 *
 * See `otPlatDnssdRegisterKey()`, `otPlatDnssdUnregisterKey()` for more details about fields in each case.
 *
 */
typedef struct otPlatDnssdKey
{
    const char    *mName;          ///< A host or a service instance name (does not include domain name).
    const char    *mServiceType;   ///< The service type if key is for a service (does not include domain name).
    const uint8_t *mKeyData;       ///< Byte array containing the key record data.
    uint16_t       mKeyDataLength; ///< Length of @p mKeyData in bytes.
    uint16_t       mClass;         ///< The resource record class.
    uint32_t       mTtl;           ///< The TTL in seconds.
    uint32_t       mInfraIfIndex;  ///< The infrastructure network interface index.
} otPlatDnssdKey;

/**
 * Callback from platform to notify OpenThread of state changes to DNS-SD module.
 *
 * OpenThread stack will call `otPlatDnssdGetState()` (from this callback or later) to get the new state. The platform
 * MUST therefore ensure that the returned state from `otPlatDnssdGetState()` is updated before calling this.
 *
 * @param[in] aInstance The OpenThread instance structure.
 *
 */
extern void otPlatDnssdStateHandleStateChange(otInstance *aInstance);

/**
 * Gets the current state of DNS-SD module.
 *
 * Platform MUST notify OpenThread whenever its state gets changed by invoking `otPlatDnssdStateHandleStateChange()`.
 *
 * @param[in] aInstance The OpenThread instance.
 *
 * @returns The current state of DNS-SD module.
 *
 */
otPlatDnssdState otPlatDnssdGetState(otInstance *aInstance);

/**
 * Registers or updates a service on the infrastructure network's DNS-SD module.
 *
 * The @p aService and all its contained information (strings and buffers) are only valid during this call. The
 * platform MUST save a copy of the information if it wants to retain the information after returning from this
 * function.
 *
 * The fields in @p aService follow these rules:
 *
 * - The `mServiceInstance` and `mServiceType` fields specify the service instance label and service type name,
 *   respectively. They are never NULL.
 * - The `mHostName` field specifies the host name of the service if it is not NULL. Otherwise, if it is NULL, it
 *   indicates that this service is for the device itself and leaves the host name selection to the DNS-SD platform.
 * - The `mSubTypeLabels` is an array of strings representing sub-types associated with the service. It can be NULL
 *   if there are no sub-types. Otherwise, the array length is specified by `mSubTypeLabelsLength`.
 * - The `mTxtData` and `mTxtDataLength` specify the encoded TXT data.
 * - The `mPort`, `mWeight`, and `mPriority` specify the service's parameters (as specified in DNS SRV record).
 * - The `mTtl` specifies the TTL if non-zero. If zero, the platform can choose the TTL to use.
 * - The `mInfraIfIndex`, if non-zero, specifies the infrastructure network interface index to use for this request. If
 *   zero, the platform implementation can decided the interface.
 *
 * When the `mHostName` field in @p aService is not NULL (indicating that this registration is on behalf of another
 * host), the OpenThread stack will ensure that `otPlatDnssdRegisterHost()` is also called for the same host before any
 * service registration requests for the same host.
 *
 * Once the registration request is finished, either successfully or failed, the platform reports the outcome by
 * invoking the @p aCallback and passing the same @p aRequestId in the callback. The @p aCallback function pointer can
 * be NULL, which indicates that the OpenThread stack does not need to be notified of the outcome of the request.
 * If the outcome is determined, the platform implementation may invoke the @p aCallback before returning from this
 * function. OpenThread stack will ensure to handle such a situation.
 *
 * On success, the @p aCallback MUST be called (if non-NULL) with `OT_ERROR_NONE` as the `aError` input parameter. If
 * the registration causes a name conflict on the DNS-SD domain (the service instance name is already claimed by another
 * host), the `OT_ERROR_DUPLICATED` error MUST be used. The platform implementation can use other `OT_ERROR` types for
 * other types of errors.
 *
 * The platform implementation MUST not assume that the @p aRequestId used in subsequent requests will be different.
 * OpenThread may reuse the same request ID again for a different request.
 *
 * OpenThread stack will not register the same service (with no changes) that was registered successfully earlier.
 * Therefore, the platform implementation does not need to check for duplicate/same service and can assume that calls
 * to this function are either registering a new entry or changing some parameter in a previously registered item. As
 * a result, these changes always need to be synced on the infrastructure DNS-SD module.
 *
 * OpenThread stack does not require the platform implementation to always invoke the @p aCallback function. OpenThread
 * stack has its own mechanism to time out an aged request with no response. This relaxes the requirement on platform
 * implementations.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aService      Information about the service to register.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
 *
 */
void otPlatDnssdRegisterService(otInstance                 *aInstance,
                                const otPlatDnssdService   *aService,
                                otPlatDnssdRequestId        aRequestId,
                                otPlatDnssdRegisterCallback aCallback);

/**
 * Unregisters a service on the infrastructure network's DNS-SD module.
 *
 * The @p aService and all its contained information (strings and buffers) are only valid during this call. The
 * platform MUST save a copy of the information if it wants to retain the information after returning from this
 * function.
 *
 * The fields in @p aService follow these rules:
 *
 * - The `mServiceInstance` and `mServiceType` fields specify the service instance label and service type name,
 *   respectively. They are never NULL.
 * - The `mHostName` field specifies the host name of the service if it is not NULL. Otherwise, if it is NULL, it
 *   indicates that this service is for the device itself and leaves the host name selection to the DNS-SD platform.
 * - The `mInfraIfIndex`, if non-zero, specifies the infrastructure network interface index to use for this request. If
 *   zero, the platform implementation can decided the interface.
 * - The rest of the fields in @p aService structure MUST be ignored in `otPlatDnssdUnregisterService()` call and may
 *   be set to zero by OpenThread stack.
 *
 * Regarding the invocation of the @p aCallback and the reuse of the @p aRequestId, this function follows the same
 * rules as described in `otPlatDnssdRegisterService()`.
 *
 * OpenThread stack may request the unregistration of a service that was not previously registered, and the platform
 * implementation MUST handle this case. In such a case, the platform can use either `OT_ERROR_NOT_FOUND` to indicate
 * that there was no such registration, or `OT_ERROR_NONE` when invoking the @p aCallback function. OpenThread stack
 * will handle either case correctly.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aService      Information about the service to unregister.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
 *
 */
void otPlatDnssdUnregisterService(otInstance                 *aInstance,
                                  const otPlatDnssdService   *aService,
                                  otPlatDnssdRequestId        aRequestId,
                                  otPlatDnssdRegisterCallback aCallback);

/**
 * Registers or updates a host on the infrastructure network's DNS-SD module.
 *
 * The @p aHost and all its contained information (strings and arrays) are only valid during this call. The
 * platform MUST save a copy of the information if it wants to retain the information after returning from this
 * function.
 *
 * The fields in @p aHost follow these rules:
 *
 * - The `mHostName` field specifies the host name to register. It is never NULL.
 * - The `mAddresses` is array of IPv6 addresses to register with the host. `mNumAddresses` provides the number of
 *   entries in `mAddresses` array. OpenThread stack will ensure that the given addresses are externally reachable
 *   (link-local or mesh-local addresses associated the host are not included in `mAddresses` array). The `mAddresses`
 *   array can be empty with zero `mNumAddresses`. In such a case, the platform MUST stop advertising any addresses
 *   for this the host name on infrastructure DNS-SD.
 * - The `mTtl` specifies the TTL if non-zero. If zero, the platform can choose the TTL to use.
 * - The `mInfraIfIndex`, if non-zero, specifies the infrastructure network interface index to use for this request. If
 *   zero, the platform implementation can decided the interface.
 *
 * Regarding the invocation of the @p aCallback and the reuse of the @p aRequestId, this function follows the same
 * rules as described in `otPlatDnssdRegisterService()`.
 *
 * OpenThread stack will not register the same host (with no changes) that was registered successfully earlier.
 * Therefore, the platform implementation does not need to check for duplicate/same host and can assume that calls
 * to this function are either registering a new entry or changing some parameter in a previously registered item. As
 * a result, these changes always need to be synced on the infrastructure DNS-SD module.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aHost         Information about the host to register.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
 *
 */
void otPlatDnssdRegisterHost(otInstance                 *aInstance,
                             const otPlatDnssdHost      *aHost,
                             otPlatDnssdRequestId        aRequestId,
                             otPlatDnssdRegisterCallback aCallback);

/**
 * Unregisters a host on the infrastructure network's DNS-SD module.
 *
 * The @p aHost and all its contained information (strings and arrays) are only valid during this call. The
 * platform MUST save a copy of the information if it wants to retain the information after returning from this
 * function.
 *
 * The fields in @p aHost follow these rules:
 *
 * - The `mHostName` field specifies the host name to unregister. It is never NULL.
 * - The `mInfraIfIndex`, if non-zero, specifies the infrastructure network interface index to use for this request. If
 *   zero, the platform implementation can decided the interface.
 * - The rest of the fields in @p aHost structure MUST be ignored in `otPlatDnssdUnregisterHost()` call and may
 *   be set to zero by OpenThread stack.
 *
 * Regarding the invocation of the @p aCallback and the reuse of the @p aRequestId, this function follows the same
 * rules as described in `otPlatDnssdRegisterService()`.
 *
 * OpenThread stack may request the unregistration of a host that was not previously registered, and the platform
 * implementation MUST handle this case. In such a case, the platform can use either `OT_ERROR_NOT_FOUND` to indicate
 * that there was no such registration, or `OT_ERROR_NONE` when invoking the @p aCallback function. OpenThread stack
 * will handle either case correctly.
 *
 * When unregistering a host, the OpenThread stack will also unregister any previously registered services
 * associated with the same host (by calling `otPlatDnssdUnregisterService()`). However, the platform implementation
 * MAY assume that unregistering a host also unregisters all its associated services.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aHost         Information about the host to unregister.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
 *
 */
void otPlatDnssdUnregisterHost(otInstance                 *aInstance,
                               const otPlatDnssdHost      *aHost,
                               otPlatDnssdRequestId        aRequestId,
                               otPlatDnssdRegisterCallback aCallback);

/**
 * Registers or updates a key record on the infrastructure network's DNS-SD module.
 *
 * The @p aKey and all its contained information (strings and arrays) are only valid during this call. The
 * platform MUST save a copy of the information if it wants to retain the information after returning from this
 * function.
 *
 * The fields in @p aKey follow these rules:
 *
 * - If the key is associated with a host, `mName` specifies the host name and `mServcieType` will be NULL.
 * - If the key is associated with a service, `mName` specifies the service instance label and `mServiceType` specifies
 *   the service type. In this case the DNS name for key record is `{mName}.{mServiceTye}`.
 * - The `mKeyData` field contains the key record's data with `mKeyDataLength` as its length in byes. It is never NULL.
 * - The `mClass` fields specifies the resource record class to use when registering key record.
 * - The `mTtl` specifies the TTL if non-zero. If zero, the platform can choose the TTL to use.
 * - The `mInfraIfIndex`, if non-zero, specifies the infrastructure network interface index to use for this request. If
 *   zero, the platform implementation can decided the interface.
 *
 * Regarding the invocation of the @p aCallback and the reuse of the @p aRequestId, this function follows the same
 * rules as described in `otPlatDnssdRegisterService()`.
 *
 * OpenThread stack will not register the same key (with no changes) that was registered successfully earlier.
 * Therefore, the platform implementation does not need to check for duplicate/same name and can assume that calls
 * to this function are either registering a new key or changing the key data in a previously registered one. As
 * a result, these changes always need to be synced on the infrastructure DNS-SD module.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aHost         Information about the key record to register.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
 *
 */
void otPlatDnssdRegisterKey(otInstance                 *aInstance,
                            const otPlatDnssdKey       *aKey,
                            otPlatDnssdRequestId        aRequestId,
                            otPlatDnssdRegisterCallback aCallback);

/**
 * Unregisters a key record on the infrastructure network's DNS-SD module.
 *
 * The @p aKey and all its contained information (strings and arrays) are only valid during this call. The
 * platform MUST save a copy of the information if it wants to retain the information after returning from this
 * function.
 *
 * The fields in @p aKey follow these rules:
 *
 * - If the key is associated with a host, `mName` specifies the host name and `mServcieType` will be NULL.
 * - If the key is associated with a service, `mName` specifies the service instance label and `mServiceType` specifies
 *   the service type. In this case the DNS name for key record is `{mName}.{mServiceTye}`.
 * - The `mInfraIfIndex`, if non-zero, specifies the infrastructure network interface index to use for this request. If
 *   zero, the platform implementation can decided the interface.
 * - The rest of the fields in @p aKey structure MUST be ignored in `otPlatDnssdUnregisterKey()` call and may
 *   be set to zero by OpenThread stack.
 *
 * Regarding the invocation of the @p aCallback and the reuse of the @p aRequestId, this function follows the same
 * rules as described in `otPlatDnssdRegisterService()`.
 *
 * OpenThread stack may request the unregistration of a key that was not previously registered, and the platform
 * implementation MUST handle this case. In such a case, the platform can use either `OT_ERROR_NOT_FOUND` to indicate
 * that there was no such registration, or `OT_ERROR_NONE` when invoking the @p aCallback function. OpenThread stack
 * will handle either case correctly.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aKey          Information about the key to unregister.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
 *
 */
void otPlatDnssdUnregisterKey(otInstance                 *aInstance,
                              const otPlatDnssdKey       *aKey,
                              otPlatDnssdRequestId        aRequestId,
                              otPlatDnssdRegisterCallback aCallback);

/**
 * Starts a service browser for a service type or sub-type on the infrastructure network's DNS-SD module.
 *
 * The @p aServiceType string is only valid during this call. Platform MUST save a copy of the string if it wants
 * to retain the information after returning from this function.
 *
 * Platform uses the `otPlatDnssdHandleServiceBrowseResult()` callback to report updates to the discovered service
 * instances matching the browser service type. Until the browser is stopped, it must continue to browse for the given
 * service type and can invoke the callback multiple times. The callback should be called with an "added" event for a
 * newly discovered service instance, and with a "removed" event when a service instance is removed.
 *
 * If some results are already available, the platform implementation may invoke the callback before returning from
 * this function. The OpenThread stack will ensure to handle such a situation.
 *
 * Platform implementation must treat browsers with different service types and/or different infrastructure network
 * interface indices as separate and unrelated browsers. In particular, two browsers for the same service type but on
 * different infrastructure network interfaces should be considered independent of each other and each one can be
 * stopped separately. The OpenThread stack will not start a browser for the same service type and on the same
 * infrastructure network interface that was started earlier and is already running. However, if this function is
 * called in this way, the platform implementation can ignore the new request and do not need to restart the active
 * browser.
 *
 * If the platform signals a state change to `OT_PLAT_DNSSD_STOPPED` using `otPlatDnssdStateHandleStateChange()`
 * callback, all active browsers and resolvers are considered to be stopped.
 *
 * @param[in] aInstance       The OpenThread instance.
 * @param[in] aServiceType    The service type or sub-type (e.g., "_mt._udp", "_s1._sub._mt._udp") - does not include
 *                            the domain name.
 * @param[in] aInfraIfIndex   The infrastructure network interface index for the service browser.
 *                            If zero, the platform implementation can decide the interface.
 *
 */
void otPlatDnssdStartServiceBrowser(otInstance *aInstance, const char *aServiceType, uint32_t aInfraIfIndex);

/**
 * Stops a service browser for a given service type or sub-type.
 *
 * The @p aServiceType string is only valid during this call. Platform MUST save a copy of the string if it wants
 * to retain the information after returning from this function.
 *
 * Platform implementation must ignore a stop request if there are no active browser for the given service type and
 * infrastructure network interface index.
 *
 * If the platform signals a state change to `OT_PLAT_DNSSD_STOPPED` using `otPlatDnssdStateHandleStateChange()`
 * callback, all active browsers and resolvers are considered to be stopped. In this case, OpenThread stack will not
 * call this function to stop the browser.
 *
 * @param[in] aInstance       The OpenThread instance.
 * @param[in] aServiceType    The service type or sub-type (e.g., "_mt._udp", "_s1._sub._mt._udp") - does not include
 *                            the domain name.
 * @param[in] aInfraIfIndex   The infrastructure network interface index for the service browser.
 *                            If zero, the platform implementation can decide the interface.
 *
 */
void otPlatDnssdStopServiceBrowser(otInstance *aInstance, const char *aServiceType, uint32_t aInfraIfIndex);

/**
 * Callback from platform to notify OpenThread a service browse result.
 *
 * Platform uses the `otPlatDnssdHandleServiceBrowseResult()` callback to report updates to the discovered service
 * instances for all active service browsers. See `otPlatDnssdStartServiceBrowser()`.
 *
 * The fields in @p aServiceInstance follow these rules:
 *
 * - The `mServiceType` specifies the service type or sub-type associated with the service browser. MUST not be NULL.
 * - The `mServiceInstance` specifies the service instance label (can include dot `.` character). MUST not be NULL.
 * - The mTtl` specifies the TTL associated with discovered instance. It can be zero for a removed instance.
 * - The `mInfraIfIndex` specifies the interface index on which the the service browser is active.
 *
 * @param[in] aInstance          The OpenThread instance.
 * @param[in] aEvent             The event to report, i.e., if this service is added or removed.
 * @param[in] aServiceInstance   The service instance information.
 *
 */
extern void otPlatDnssdHandleServiceBrowseResult(otInstance                       *aInstance,
                                                 otPlatDnssdEvent                  aEvent,
                                                 const otPlatDnssdServiceInstance *aServiceInstance);

/**
 * Starts a service resolver for a service instance on the infrastructure network's DNS-SD module.
 *
 * The @p aServiceInstance and all its contained information (strings and buffers) are only valid during this call. The
 * platform MUST save a copy of the information if it wants to retain the information after returning from this
 * function.
 *
 * The fields in @p aServiceInstance follow these rules:
 *
 * - The `mServiceType` specifies the service type (e.g., "_mt._udp")
 * - The `mServiceInstance` specifies the service instance label (may include dot `.` character).
 * - The `mInfraIfIndex` specifies the interface index on which the the service resolver should run.
 * - The `mTtl` is not used and should be ignored.
 *
 * Platform uses the `otPlatDnssdHandleServiceResolveResult()` callback to report the result.
 *
 * OpenThread stack uses service resolver as a one-shot operation, that is, after the callback is invoked, the platform
 * implementation does not need to continue to monitor the service instance and report changes (e.g., if any of service
 * parameter or TXT data changes).
 *
 * If the result is already available, the platform implementation may invoke the callback before returning from
 * this function. The OpenThread stack will ensure to handle such a situation.
 *
 * Similar to service browsers, service resolvers for different services instances and/or on different infrastructure
 * network interfaces should be considered separate entities. Platform implementation can ignore a call to this
 * function to start a service resolver when one with the exact same parameters is active.
 *
 * @param[in] aInstance         The OpenThread instance.
 * @param[in] aServiceInstance  The service instance information.
 *
 */
void otPlatDnssdStartServiceResolver(otInstance *aInstance, const otPlatDnssdServiceInstance *aServiceInstance);

/**
 * Stops a service resolver for a given service instance.
 *
 * The @p aServiceInstance fields follow the same rules as in `otPlatDnssdStartServiceResolver()`
 *
 * Platform implementation must ignore a stop request if there are no active service resolver matching the given
 * service instance.
 *
 * If the platform signals a state change to `OT_PLAT_DNSSD_STOPPED` using `otPlatDnssdStateHandleStateChange()`
 * callback, all active browsers and resolvers are considered to be stopped. In this case, OpenThread stack will not
 * call this function to stop the resolver.
 *
 * @param[in] aInstance         The OpenThread instance.
 * @param[in] aServiceInstance  The service instance information.
 *
 */
void otPlatDnssdStopServiceResolver(otInstance *aInstance, const otPlatDnssdServiceInstance *aServiceInstance);

/**
 * Callback from platform to notify OpenThread of the result from a service resolver.
 *
 * The fields in @p aService follow these rules:
 *
 * - The `mServiceType` specifies the service type (e.g., "_mt._udp")
 * - The `mServiceInstance` specifies the service instance label (may include dot `.` character).
 * - The `mHostName` field specifies the host name of the service. MUST not be NULL.
 * - The `mTxtData` and `mTxtDataLength` specify the encoded TXT data. MUST not be NULL.
 * - The `mPort`, `mWeight`, and `mPriority` specify the service's parameters (as specified in DNS SRV record).
 * - The `mTtl` specifies the TTL in seconds.
 * - The `mInfraIfIndex` specifies the infrastructure network interface index of service resolver.
 * - The other fields (e.g., `mSubTypeLabels`) are not used and are ignored (can be set to NULL).
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aSevice       The service information.
 *
 */
extern void otPlatDnssdHandleServiceResolveResult(otInstance *aInstance, const otPlatDnssdService *aService);

/**
 * Starts an IPv6 address resolver for a given host name on the infrastructure network's DNS-SD module.
 *
 * The @p aHostName string is only valid during this call. Platform MUST save a copy of the string if it wants to
 * retain the information after returning from this function.
 *
 * Platform uses the `otPlatDnssdHandleIp6AddressResolveResult()` callback to report updates to IPv6 addresses of the
 * @p aHostName. Until the address resolver is stopped, it must continue to monitor for changes to addresses of the
 * host and can invoke the callback multiple times. The callback should be called with an "added" event for a
 * newly discovered/added address, and with a "removed" event when an address is removed.
 *
 * If some results are already available, the platform implementation may invoke the callback before returning from
 * this function. The OpenThread stack will ensure to handle such a situation.
 *
 * Similar to service browsers and resolvers, address resolvers for different host names and/or on different
 * infrastructure network interfaces should be considered separate entities. Platform implementation can ignore a call
 * to this function to start an address resolver when one for same host and same network interface index is active.
 *
 * If the platform signals a state change to `OT_PLAT_DNSSD_STOPPED` using `otPlatDnssdStateHandleStateChange()`
 * callback, all active browsers and resolvers are considered to be stopped.
 *
 * @param[in] aInstance       The OpenThread instance.
 * @param[in] aHostName       The host name - does not include the domain name.
 * @param[in] aInfraIfIndex   The infrastructure network interface index for the address resolver.
 *                            If zero, the platform implementation can decide the interface.
 *
 */
void otPlatDnssdStartIp6AddressResolver(otInstance *aInstance, const char *aHostName, uint32_t aInfraIfIndex);

/**
 * Stops an IPv6 address resolver for a given host name.
 *
 * The @p aHostName fields follow the same rules as in `otPlatDnssdStartIp6AddressResolver()`
 *
 * Platform implementation must ignore a stop request if there are no active address resolver for the given host name.
 *
 * If the platform signals a state change to `OT_PLAT_DNSSD_STOPPED` using `otPlatDnssdStateHandleStateChange()`
 * callback, all active browsers and resolvers are considered to be stopped. In this case, OpenThread stack will not
 * call this function to stop the resolver.
 *
 * @param[in] aInstance       The OpenThread instance.
 * @param[in] aHostName       The host name - does not include the domain name.
 * @param[in] aInfraIfIndex   The infrastructure network interface index for address browser.
 *                            If zero, the platform implementation can decide the interface.
 *
 */
void otPlatDnssdStopIp6AddressResolver(otInstance *aInstance, const char *aHostName, uint32_t aInfraIfIndex);

/**
 * Callback from platform to notify OpenThread of the result from an IPv6 address resolver.
 *
 * The fields in @p aHost follow these rules:
 *
 * - The `mHostName` field specifies the host name from the address resolver.
 * - The `mAddresses` field point to an array of IPv6 addresses of host.
 * - The `mNumAddresses` field specifies the number of entries in `mAddresses` array.
 * - The `mTtl` specifies the TTL in seconds.
 * - The `mInfraIfIndex` specifies the infrastructure network interface index of address resolver.
 *
 * @p aEvent applies to all addresses in `mAddresses`, i.e. all are added or removed.
 *
 * Platform implementation MUST not filter any addresses when reporting the result. In particular link-local IPv6
 * addresses must be included. OpenThread stack will filter the result according to how the result is used.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aEvent      The event to report, i.e., if added or removed.
 * @param[in] aHost       The host information.
 *
 */
extern void otPlatDnssdHandleIp6AddressResolveResult(otInstance            *aInstance,
                                                     otPlatDnssdEvent       aEvent,
                                                     const otPlatDnssdHost *aHost);

/**
 * Starts an IPv4 address resolver for a given host name on the infrastructure network's DNS-SD module.
 *
 * This function behaves similarly to `otPlatDnssdStartIp6AddressResolver()` and follows the same rules except that
 * it is for IPv4 addresses and `otPlatDnssdHandleIp4AddressResolveResult()` is used to report the result.
 *
 * @param[in] aInstance       The OpenThread instance.
 * @param[in] aHostName       The host name - does not include the domain name.
 * @param[in] aInfraIfIndex   The infrastructure network interface index for the address resolver.
 *                            If zero, the platform implementation can decide the interface.
 *
 */
void otPlatDnssdStartIp4AddressResolver(otInstance *aInstance, const char *aHostName, uint32_t aInfraIfIndex);

/**
 * Stops an IPv4 address resolver for a given host name on the infrastructure network's DNS-SD module.
 *
 * This function behaves similarly to `otPlatDnssdStopIp6AddressResolver()` and follows the same rules.
 *
 * @param[in] aInstance       The OpenThread instance.
 * @param[in] aHostName       The host name - does not include the domain name.
 * @param[in] aInfraIfIndex   The infrastructure network interface index for the address resolver.
 *                            If zero, the platform implementation can decide the interface.
 *
 */
void otPlatDnssdStopIp4AddressResolver(otInstance *aInstance, const char *aHostName, uint32_t aInfraIfIndex);

/**
 * Callback from platform to notify OpenThread of the result from an Ipv4 address resolver.
 *
 * This callback is similar to `otPlatDnssdHandleIp6AddressResolveResult()` and follows the same rules except that
 * addresses in `mAddresses` fields use the "IPv4-mapped IPv6 addresses" format, i.e.,
 *
 * - `mAddresses` field in @p aHost is an array of `otIp6Address` entries.
 * - The `mNumAddresses` field specifies the number of entries in `mAddresses` array.
 * - The `mAddresses` entries use "IPv4-mapped IPv6 address" format (e.g. `::ffff:192.0.2.128`).
 *
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aEvent      The event to report, i.e., if added or removed.
 * @param[in] aHost       The host information.
 *
 */
extern void otPlatDnssdHandleIp4AddressResolveResult(otInstance            *aInstance,
                                                     otPlatDnssdEvent       aEvent,
                                                     const otPlatDnssdHost *aHost);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_DNSSD_H_
