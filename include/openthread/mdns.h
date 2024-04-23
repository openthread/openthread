/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes the mDNS related APIs.
 *
 */

#ifndef OPENTHREAD_MULTICAST_DNS_H_
#define OPENTHREAD_MULTICAST_DNS_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/platform/dnssd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-mdns
 *
 * @brief
 *   This module includes APIs for Multicast DNS (mDNS).
 *
 * @{
 *
 * The mDNS APIs are available when the mDNS support `OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE` is enabled and the
 * `OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE` is also enabled.
 *
 */

/**
 * Represents a request ID (`uint32_t` value) for registering a host, a service, or a key service.
 *
 */
typedef otPlatDnssdRequestId otMdnsRequestId;

/**
 * Represents the callback function to report the outcome of a host, service, or key registration request.
 *
 * The outcome of a registration request is reported back by invoking this callback with one of the following `aError`
 * inputs:
 *
 * - `OT_ERROR_NONE` indicates registration was successful.
 * - `OT_ERROR_DUPLICATED` indicates a name conflict while probing, i.e., name is claimed by another mDNS responder.
 *
 * See `otMdnsRegisterHost()`, `otMdnsRegisterService()`, and `otMdnsRegisterKey()` for more details about when
 * the callback will be invoked.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aRequestId    The request ID.
 * @param[in] aError        Error indicating the outcome of request.
 *
 */
typedef otPlatDnssdRegisterCallback otMdnsRegisterCallback;

/**
 * Represents the callback function to report a detected name conflict after successful registration of an entry.
 *
 * If a conflict is detected while registering an entry, it is reported through the provided `otMdnsRegisterCallback`.
 * The `otMdnsConflictCallback` is used only when a name conflict is detected after an entry has been successfully
 * registered.
 *
 * A non-NULL @p aServiceType indicates that conflict is for a service entry. In this case @p aName specifies the
 * service instance label (treated as as a single DNS label and can potentially include dot `.` character).
 *
 * A NULL @p aServiceType indicates that conflict is for a host entry. In this case @p Name specifies the host name. It
 * does not include the domain name.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aName          The host name or the service instance label.
 * @param[in] aServiceType   The service type (e.g., `_tst._udp`).
 *
 */
typedef void (*otMdnsConflictCallback)(otInstance *aInstance, const char *aName, const char *aServiceType);

/**
 * Represents an mDNS host.
 *
 * This type is used to register or unregister a host (`otMdnsRegisterHost()` and `otMdnsUnregisterHost()`).
 *
 * See the description of each function for more details on how different fields are used in each case.
 *
 */
typedef otPlatDnssdHost otMdnsHost;

/**
 * Represents an mDNS service.
 *
 * This type is used to register or unregister a service (`otMdnsRegisterService()` and `otMdnsUnregisterService()`).
 *
 * See the description of each function for more details on how different fields are used in each case.
 *
 */
typedef otPlatDnssdService otMdnsService;

/**
 * Represents an mDNS key record.
 *
 * See `otMdnsRegisterKey()`, `otMdnsUnregisterKey()` for more details about fields in each case.
 *
 */
typedef otPlatDnssdKey otMdnsKey;

/**
 * Represents an mDNS entry iterator.
 *
 */
typedef struct otMdnsIterator otMdnsIterator;

/**
 * Represents a host/service/key entry state.
 *
 */
typedef enum otMdnsEntryState
{
    OT_MDNS_ENTRY_STATE_PROBING,    ///< Probing to claim the name.
    OT_MDNS_ENTRY_STATE_REGISTERED, ///< Entry is successfully registered.
    OT_MDNS_ENTRY_STATE_CONFLICT,   ///< Name conflict was detected.
    OT_MDNS_ENTRY_STATE_REMOVING,   ///< Entry is being removed (sending "goodbye" announcements).
} otMdnsEntryState;

/**
 * Enables or disables the mDNS module.
 *
 * The mDNS module should be enabled before registration any host, service, or key entries. Disabling mDNS will
 * immediately stop all operations and any communication (multicast or unicast tx) and remove any previously registered
 * entries without sending any "goodbye" announcements or invoking their callback. Once disabled, all currently active
 * browsers and resolvers are stopped.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aEnable        Boolean to indicate whether to enable (on `TRUE`) or disable (on `FALSE`).
 * @param[in] aInfraIfIndex  The network interface index for mDNS operation. Value is ignored when disabling
 *
 * @retval OT_ERROR_NONE     Enabled or disabled the mDNS module successfully.
 * @retval OT_ERROR_ALREADY  mDNS is already enabled on an enable request or is already disabled on a disable request.
 *
 */
otError otMdnsSetEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex);

/**
 * Indicates whether the mDNS module is enabled.
 *
 * @param[in] aInstance     The OpenThread instance.
 *
 * @retval TRUE    The mDNS module is enabled
 * @retval FALSE   The mDNS module is disabled.
 *
 */
bool otMdnsIsEnabled(otInstance *aInstance);

/**
 * Sets whether the mDNS module is allowed to send questions requesting unicast responses referred to as "QU" questions.
 *
 * The "QU" questions request unicast responses, in contrast to "QM" questions which request multicast responses.
 *
 * When allowed, the first probe will be sent as a "QU" question. This API can be used to address platform limitation
 * where platform socket cannot accept unicast response received on mDNS port (due to it being already bound).
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aAllow        Indicates whether or not to allow "QU" questions.
 *
 */
void otMdnsSetQuestionUnicastAllowed(otInstance *aInstance, bool aAllow);

/**
 * Indicates whether mDNS module is allowed to send "QU" questions requesting unicast response.
 *
 * @retval TRUE  The mDNS module is allowed to send "QU" questions.
 * @retval FALSE The mDNS module is not allowed to send "QU" questions.
 *
 */
bool otMdnsIsQuestionUnicastAllowed(otInstance *aInstance);

/**
 * Sets the post-registration conflict callback.
 *
 * If a conflict is detected while registering an entry, it is reported through the provided `otMdnsRegisterCallback`.
 * The `otMdnsConflictCallback` is used only when a name conflict is detected after an entry has been successfully
 * registered.
 *
 * @p aCallback can be set to `NULL` if not needed. Subsequent calls will replace any previously set callback.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aCallback     The conflict callback.
 *
 */
void otMdnsSetConflictCallback(otInstance *aInstance, otMdnsConflictCallback aCallback);

/**
 * Registers or updates a host on mDNS.
 *
 * The fields in @p aHost follow these rules:
 *
 * - The `mHostName` field specifies the host name to register (e.g., "myhost"). MUST NOT contain the domain name.
 * - The `mAddresses` is array of IPv6 addresses to register with the host. `mAddressesLength` provides the number of
 *   entries in `mAddresses` array.
 * - The `mAddresses` array can be empty with zero `mAddressesLength`. In this case, mDNS will treat it as if host is
 *   unregistered and stops advertising any addresses for this the host name.
 * - The `mTtl` specifies the TTL if non-zero. If zero, the mDNS core will choose the default TTL of 120 seconds.
 * - Other fields in @p aHost structure are ignored in an `otMdnsRegisterHost()` call.
 *
 * This function can be called again for the same `mHostName` to update a previously registered host entry, for example,
 * to change the list of addresses of the host. In this case, the mDNS module will send "goodbye" announcements for any
 * previously registered and now removed addresses and announce any newly added addresses.
 *
 * The outcome of the registration request is reported back by invoking the provided @p aCallback with @p aRequestId
 * as its input and one of the following `aError` inputs:
 *
 * - `OT_ERROR_NONE` indicates registration was successful.
 * - `OT_ERROR_DULICATED` indicates a name conflict while probing, i.e., name is claimed by another mDNS responder.
 *
 * For caller convenience, the OpenThread mDNS module guarantees that the callback will be invoked after this function
 * returns, even in cases of immediate registration success. The @p aCallback can be `NULL` if caller does not want to
 * be notified of the outcome.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aHost         Information about the host to register.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (can be NULL if not needed).
 *
 * @retval OT_ERROR_NONE            Successfully started registration. @p aCallback will report the outcome.
 * @retval OT_ERROR_INVALID_STATE   mDNS module is not enabled.
 *
 */
otError otMdnsRegisterHost(otInstance            *aInstance,
                           const otMdnsHost      *aHost,
                           otMdnsRequestId        aRequestId,
                           otMdnsRegisterCallback aCallback);

/**
 * Unregisters a host on mDNS.
 *
 * The fields in @p aHost follow these rules:
 *
 * - The `mHostName` field specifies the host name to unregister (e.g., "myhost"). MUST NOT contain the domain name.
 * - Other fields in @p aHost structure are ignored in an `otMdnsUnregisterHost()` call.
 *
 * If there is no previously registered host with the same name, no action is performed.
 *
 * If there is a previously registered host with the same name, the mDNS module will send "goodbye" announcement for
 * all previously advertised address records.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aHost         Information about the host to unregister.
 *
 * @retval OT_ERROR_NONE            Successfully unregistered host.
 * @retval OT_ERROR_INVALID_STATE   mDNS module is not enabled.
 *
 */
otError otMdnsUnregisterHost(otInstance *aInstance, const otMdnsHost *aHost);

/**
 * Registers or updates a service on mDNS.
 *
 * The fields in @p aService follow these rules:
 *
 * - The `mServiceInstance` specifies the service instance label. It is treated as a single DNS name label. It may
 *   contain dot `.` character which is allowed in a service instance label.
 * - The `mServiceType` specifies the service type (e.g., "_tst._udp"). It is treated as multiple dot `.` separated
 *   labels. It MUST NOT contain the domain name.
 * - The `mHostName` field specifies the host name of the service. MUST NOT contain the domain name.
 * - The `mSubTypeLabels` is an array of strings representing sub-types associated with the service. Each array entry
 *   is a sub-type label. The `mSubTypeLabels can be NULL if there is no sub-type. Otherwise, the array length is
 *   specified by `mSubTypeLabelsLength`.
 * - The `mTxtData` and `mTxtDataLength` specify the encoded TXT data. The `mTxtData` can be NULL or `mTxtDataLength`
 *   can be zero to specify an empty TXT data. In this case mDNS module will use a single zero byte `[ 0 ]` as the
 *   TXT data.
 * - The `mPort`, `mWeight`, and `mPriority` specify the service's parameters as specified in DNS SRV record.
 * - The `mTtl` specifies the TTL if non-zero. If zero, the mDNS module will use the default TTL of 120 seconds.
 * - Other fields in @p aService structure are ignored in an `otMdnsRegisterService()` call.
 *
 * This function can be called again for the same `mServiceInstance` and `mServiceType` to update a previously
 * registered service entry, for example, to change the sub-types list, or update any parameter such as port, weight,
 * priority, TTL, or host name. The mDNS module will send announcements for any changed info, e.g., will send "goodbye"
 * announcements for any removed sub-types and announce any newly added sub-types.
 *
 * Regarding the invocation of the @p aCallback, this function behaves in the same way as described in
 * `otMdnsRegisterHost()`.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aService      Information about the service to register.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (can be NULL if not needed).
 *
 * @retval OT_ERROR_NONE            Successfully started registration. @p aCallback will report the outcome.
 * @retval OT_ERROR_INVALID_STATE   mDNS module is not enabled.
 *
 */
otError otMdnsRegisterService(otInstance            *aInstance,
                              const otMdnsService   *aService,
                              otMdnsRequestId        aRequestId,
                              otMdnsRegisterCallback aCallback);

/**
 * Unregisters a service on mDNS module.
 *
 * The fields in @p aService follow these rules:

 * - The `mServiceInstance` specifies the service instance label. It is treated as a single DNS name label. It may
 *   contain dot `.` character which is allowed in a service instance label.
 * - The `mServiceType` specifies the service type (e.g., "_tst._udp"). It is treated as multiple dot `.` separated
 *   labels. It MUST NOT contain the domain name.
 * - Other fields in @p aService structure are ignored in an `otMdnsUnregisterService()` call.
 *
 * If there is no previously registered service with the same name, no action is performed.
 *
 * If there is a previously registered service with the same name, the mDNS module will send "goodbye" announcements
 * for all related records.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aService      Information about the service to unregister.
 *
 * @retval OT_ERROR_NONE            Successfully unregistered service.
 * @retval OT_ERROR_INVALID_STATE   mDNS module is not enabled.
 *
 */
otError otMdnsUnregisterService(otInstance *aInstance, const otMdnsService *aService);

/**
 * Registers or updates a key record on mDNS module.
 *
 * The fields in @p aKey follow these rules:
 *
 * - If the key is associated with a host entry, the `mName` field specifies the host name and the `mServiceType` MUST
 *   be NULL.
 * - If the key is associated with a service entry, the `mName` filed specifies the service instance label (always
 *   treated as a single label) and the `mServiceType` filed specifies the service type (e.g., "_tst._udp"). In this
 *   case the DNS name for key record is `<mName>.<mServiceTye>`.
 * - The `mKeyData` field contains the key record's data with `mKeyDataLength` as its length in byes.
 * - The `mTtl` specifies the TTL if non-zero. If zero, the mDNS module will use the default TTL of 120 seconds.
 * - Other fields in @p aKey structure are ignored in an `otMdnsRegisterKey()` call.
 *
 * This function can be called again for the same name to updated a previously registered key entry, for example, to
 * change the key data or TTL.
 *
 * Regarding the invocation of the @p aCallback, this function behaves in the same way as described in
 * `otMdnsRegisterHost()`.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aKey          Information about the key record to register.
 * @param[in] aRequestId    The ID associated with this request.
 * @param[in] aCallback     The callback function pointer to report the outcome (can be NULL if not needed).
 *
 * @retval OT_ERROR_NONE            Successfully started registration. @p aCallback will report the outcome.
 * @retval OT_ERROR_INVALID_STATE   mDNS module is not enabled.
 *
 */
otError otMdnsRegisterKey(otInstance            *aInstance,
                          const otMdnsKey       *aKey,
                          otMdnsRequestId        aRequestId,
                          otMdnsRegisterCallback aCallback);

/**
 * Unregisters a key record on mDNS.
 *
 * The fields in @p aKey follow these rules:
 *
 * - If the key is associated with a host entry, the `mName` field specifies the host name and the `mServiceType` MUST
 *   be NULL.
 * - If the key is associated with a service entry, the `mName` filed specifies the service instance label (always
 *   treated as a single label) and the `mServiceType` filed specifies the service type (e.g., "_tst._udp"). In this
 *   case the DNS name for key record is `<mName>.<mServiceTye>`.
 * - Other fields in @p aKey structure are ignored in an `otMdnsUnregisterKey()` call.
 *
 * If there is no previously registered key with the same name, no action is performed.
 *
 * If there is a previously registered key with the same name, the mDNS module will send "goodbye" announcements for
 * the key record.
 *
 * @param[in] aInstance     The OpenThread instance.
 * @param[in] aKey          Information about the key to unregister.
 *
 * @retval OT_ERROR_NONE            Successfully unregistered key
 * @retval OT_ERROR_INVALID_STATE   mDNS module is not enabled.
 *
 */
otError otMdnsUnregisterKey(otInstance *aInstance, const otMdnsKey *aKey);

/**
 * Allocates a new iterator.
 *
 * An allocated iterator must be freed by the caller using `otMdnsFreeIterator()`.
 *
 * @param[in] aInstance    The OpenThread instance.
 *
 * @returns A pointer to the allocated iterator, or `NULL` if it fails to allocate.
 *
 */
otMdnsIterator *otMdnsAllocateIterator(otInstance *aInstance);

/**
 * Frees a previously allocated iterator.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aIterator    The iterator to free.
 *
 */
void otMdnsFreeIterator(otInstance *aInstance, otMdnsIterator *aIterator);

/**
 * Iterates over registered host entries.
 *
 * On success, @p aHost is populated with information about the next host. Pointers within the `otMdnsHost` structure
 * (like `mName`) remain valid until the next call to any OpenThread stack's public or platform API/callback.
 *
 * @param[in]  aInstance   The OpenThread instance.
 * @param[in]  aIterator   Pointer to the iterator.
 * @param[out] aHost       Pointer to an `otMdnsHost` to return the information about the next host entry.
 * @param[out] aState      Pointer to an `otMdnsEntryState` to return the entry state.
 *
 * @retval OT_ERROR_NONE         @p aHost, @p aState, & @p aIterator are updated successfully.
 * @retval OT_ERROR_NOT_FOUND    Reached the end of the list.
 * @retval OT_ERROR_INVALID_ARG  @p aIterator is not valid.
 *
 */
otError otMdnsGetNextHost(otInstance       *aInstance,
                          otMdnsIterator   *aIterator,
                          otMdnsHost       *aHost,
                          otMdnsEntryState *aState);

/**
 * Iterates over registered service entries.
 *
 * On success, @p aService is populated with information about the next service . Pointers within the `otMdnsService`
 * structure (like `mServiceType`, `mSubTypeLabels`) remain valid until the next call to any OpenThread stack's public
 * or platform API/callback.
 *
 * @param[in]  aInstance    The OpenThread instance.
 * @param[in]  aIterator    Pointer to the iterator to use.
 * @param[out] aService     Pointer to an `otMdnsService` to return the information about the next service entry.
 * @param[out] aState       Pointer to an `otMdnsEntryState` to return the entry state.
 *
 * @retval OT_ERROR_NONE         @p aService, @p aState, & @p aIterator are updated successfully.
 * @retval OT_ERROR_NOT_FOUND    Reached the end of the list.
 * @retval OT_ERROR_INVALID_ARG  @p aIterator is not valid.
 *
 */
otError otMdnsGetNextService(otInstance       *aInstance,
                             otMdnsIterator   *aIterator,
                             otMdnsService    *aService,
                             otMdnsEntryState *aState);

/**
 * Iterates over registered key entries.
 *
 * On success, @p aKey is populated with information about the next key.  Pointers within the `otMdnsKey` structure
 * (like `mName`) remain valid until the next call to any OpenThread stack's public or platform API/callback.
 *
 * @param[in]  aInstance    The OpenThread instance.
 * @param[in]  aIterator    Pointer to the iterator to use.
 * @param[out] aKey         Pointer to an `otMdnsKey` to return the information about the next key entry.
 * @param[out] aState       Pointer to an `otMdnsEntryState` to return the entry state.
 *
 * @retval OT_ERROR_NONE         @p aKey, @p aState, & @p aIterator are updated successfully.
 * @retval OT_ERROR_NOT_FOUND    Reached the end of the list.
 * @retval OT_ERROR_INVALID_ARG  Iterator is not valid.
 *
 */
otError otMdnsGetNextKey(otInstance *aInstance, otMdnsIterator *aIterator, otMdnsKey *aKey, otMdnsEntryState *aState);

typedef struct otMdnsBrowseResult  otMdnsBrowseResult;
typedef struct otMdnsSrvResult     otMdnsSrvResult;
typedef struct otMdnsTxtResult     otMdnsTxtResult;
typedef struct otMdnsAddressResult otMdnsAddressResult;

/**
 * Represents the callback function used to report a browse result.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResult      The browse result.
 *
 */
typedef void (*otMdnsBrowseCallback)(otInstance *aInstance, const otMdnsBrowseResult *aResult);

/**
 * Represents the callback function used to report an SRV resolve result.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResult      The SRV resolve result.
 *
 */
typedef void (*otMdnsSrvCallback)(otInstance *aInstance, const otMdnsSrvResult *aResult);

/**
 * Represents the callback function used to report a TXT resolve result.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResult      The TXT resolve result.
 *
 */
typedef void (*otMdnsTxtCallback)(otInstance *aInstance, const otMdnsTxtResult *aResult);

/**
 * Represents the callback function use to report a IPv6/IPv4 address resolve result.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResult      The address resolve result.
 *
 */
typedef void (*otMdnsAddressCallback)(otInstance *aInstance, const otMdnsAddressResult *aResult);

/**
 * Represents a service browser.
 *
 */
typedef struct otMdnsBrowser
{
    const char          *mServiceType;  ///< The service type (e.g., "_mt._udp"). MUST NOT include domain name.
    const char          *mSubTypeLabel; ///< The sub-type label if browsing for sub-type, NULL otherwise.
    uint32_t             mInfraIfIndex; ///< The infrastructure network interface index.
    otMdnsBrowseCallback mCallback;     ///< The callback to report result.
} otMdnsBrowser;

/**
 * Represents a browse result.
 *
 */
struct otMdnsBrowseResult
{
    const char *mServiceType;     ///< The service type (e.g., "_mt._udp").
    const char *mSubTypeLabel;    ///< The sub-type label if browsing for sub-type, NULL otherwise.
    const char *mServiceInstance; ///< Service instance label.
    uint32_t    mTtl;             ///< TTL in seconds. Zero TTL indicates that service is removed.
    uint32_t    mInfraIfIndex;    ///< The infrastructure network interface index.
};

/**
 * Represents an SRV service resolver.
 *
 */
typedef struct otMdnsSrvResolver
{
    const char       *mServiceInstance; ///< The service instance label.
    const char       *mServiceType;     ///< The service type.
    uint32_t          mInfraIfIndex;    ///< The infrastructure network interface index.
    otMdnsSrvCallback mCallback;        ///< The callback to report result.
} otMdnsSrvResolver;

/**
 * Represents an SRV resolver result.
 *
 */
struct otMdnsSrvResult
{
    const char *mServiceInstance; ///< The service instance name label.
    const char *mServiceType;     ///< The service type.
    const char *mHostName;        ///< The host name (e.g., "myhost"). Can be NULL when `mTtl` is zero.
    uint16_t    mPort;            ///< The service port number.
    uint16_t    mPriority;        ///< The service priority.
    uint16_t    mWeight;          ///< The service weight.
    uint32_t    mTtl;             ///< The service TTL in seconds. Zero TTL indicates SRV record is removed.
    uint32_t    mInfraIfIndex;    ///< The infrastructure network interface index.
};

/**
 * Represents a TXT service resolver.
 *
 */
typedef struct otMdnsTxtResolver
{
    const char       *mServiceInstance; ///< Service instance label.
    const char       *mServiceType;     ///< Service type.
    uint32_t          mInfraIfIndex;    ///< The infrastructure network interface index.
    otMdnsTxtCallback mCallback;
} otMdnsTxtResolver;

/**
 * Represents a TXT resolver result.
 *
 */
struct otMdnsTxtResult
{
    const char    *mServiceInstance; ///< The service instance name label.
    const char    *mServiceType;     ///< The service type.
    const uint8_t *mTxtData;         ///< Encoded TXT data bytes. Can be NULL when `mTtl` is zero.
    uint16_t       mTxtDataLength;   ///< Length of TXT data.
    uint32_t       mTtl;             ///< The TXT data TTL in seconds. Zero TTL indicates record is removed.
    uint32_t       mInfraIfIndex;    ///< The infrastructure network interface index.
};

/**
 * Represents an address resolver.
 *
 */
typedef struct otMdnsAddressResolver
{
    const char           *mHostName;     ///< The host name (e.g., "myhost"). MUST NOT contain domain name.
    uint32_t              mInfraIfIndex; ///< The infrastructure network interface index.
    otMdnsAddressCallback mCallback;     ///< The callback to report result.
} otMdnsAddressResolver;

/**
 * Represents a discovered host address and its TTL.
 *
 */
typedef struct otMdnsAddressAndTtl
{
    otIp6Address mAddress; ///< The IPv6 address. For IPv4 address the IPv4-mapped IPv6 address format is used.
    uint32_t     mTtl;     ///< The TTL in seconds.
} otMdnsAddressAndTtl;

/**
 * Represents address resolver result.
 *
 */
struct otMdnsAddressResult
{
    const char                *mHostName;        ///< The host name.
    const otMdnsAddressAndTtl *mAddresses;       ///< Array of host addresses and their TTL. Can be NULL if empty.
    uint16_t                   mAddressesLength; ///< Number of entries in `mAddresses` array.
    uint32_t                   mInfraIfIndex;    ///< The infrastructure network interface index.
};

/**
 * Starts a service browser.
 *
 * Initiates a continuous search for the specified `mServiceType` in @p aBrowser. For sub-type services, use
 * `mSubTypeLabel` to define the sub-type, for base services, set `mSubTypeLabel` to NULL.
 *
 * Discovered services are reported through the `mCallback` function in @p aBrowser. Services that have been removed
 * are reported with a TTL value of zero. The callback may be invoked immediately with cached information (if available)
 * and potentially before this function returns. When cached results are used, the reported TTL value will reflect
 * the original TTL from the last received response.
 *
 * Multiple browsers can be started for the same service, provided they use different callback functions.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aBrowser    The browser to be started.
 *
 * @retval OT_ERROR_NONE           Browser started successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 * @retval OT_ERROR_ALREADY        An identical browser (same service and callback) is already active.
 *
 */
otError otMdnsStartBrowser(otInstance *aInstance, const otMdnsBrowser *aBrowser);

/**
 * Stops a service browser.
 *
 * No action is performed if no matching browser with the same service and callback is currently active.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aBrowser    The browser to stop.
 *
 * @retval OT_ERROR_NONE           Browser stopped successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 *
 */
otError otMdnsStopBrowser(otInstance *aInstance, const otMdnsBrowser *aBroswer);

/**
 * Starts an SRV record resolver.
 *
 * Initiates a continuous SRV record resolver for the specified service in @p aResolver.
 *
 * Discovered information is reported through the `mCallback` function in @p aResolver. When the service is removed
 * it is reported with a TTL value of zero. In this case, `mHostName` may be NULL and other result fields (such as
 * `mPort`) should be ignored.
 *
 * The callback may be invoked immediately with cached information (if available) and potentially before this function
 * returns. When cached result is used, the reported TTL value will reflect the original TTL from the last received
 * response.
 *
 * Multiple resolvers can be started for the same service, provided they use different callback functions.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to be started.
 *
 * @retval OT_ERROR_NONE           Resolver started successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 * @retval OT_ERROR_ALREADY        An identical resolver (same service and callback) is already active.
 *
 */
otError otMdnsStartSrvResolver(otInstance *aInstance, const otMdnsSrvResolver *aResolver);

/**
 * Stops an SRV record resolver.
 *
 * No action is performed if no matching resolver with the same service and callback is currently active.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to stop.
 *
 * @retval OT_ERROR_NONE           Resolver stopped successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 *
 */
otError otMdnsStopSrvResolver(otInstance *aInstance, const otMdnsSrvResolver *aResolver);

/**
 * Starts a TXT record resolver.
 *
 * Initiates a continuous TXT record resolver for the specified service in @p aResolver.
 *
 * Discovered information is reported through the `mCallback` function in @p aResolver. When the TXT record is removed
 * it is reported with a TTL value of zero. In this case, `mTxtData` may be NULL, and other result fields (such as
 * `mTxtDataLength`) should be ignored.
 *
 * The callback may be invoked immediately with cached information (if available) and potentially before this function
 * returns. When cached result is used, the reported TTL value will reflect the original TTL from the last received
 * response.
 *
 * Multiple resolvers can be started for the same service, provided they use different callback functions.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to be started.
 *
 * @retval OT_ERROR_NONE           Resolver started successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 * @retval OT_ERROR_ALREADY        An identical resolver (same service and callback) is already active.
 *
 */
otError otMdnsStartTxtResolver(otInstance *aInstance, const otMdnsTxtResolver *aResolver);

/**
 * Stops a TXT record resolver.
 *
 * No action is performed if no matching resolver with the same service and callback is currently active.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to stop.
 *
 * @retval OT_ERROR_NONE           Resolver stopped successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 *
 */
otError otMdnsStopTxtResolver(otInstance *aInstance, const otMdnsTxtResolver *aResolver);

/**
 * Starts an IPv6 address resolver.
 *
 * Initiates a continuous IPv6 address resolver for the specified host name in @p aResolver.
 *
 * Discovered addresses are reported through the `mCallback` function in @p aResolver. The callback is invoked
 * whenever addresses are added or removed, providing an updated list. If all addresses are removed, the callback is
 * invoked with an empty list (`mAddresses` will be NULL, and `mAddressesLength` will be zero).
 *
 * The callback may be invoked immediately with cached information (if available) and potentially before this function
 * returns. When cached result is used, the reported TTL values will reflect the original TTL from the last received
 * response.
 *
 * Multiple resolvers can be started for the same host name, provided they use different callback functions.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to be started.
 *
 * @retval OT_ERROR_NONE           Resolver started successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 * @retval OT_ERROR_ALREADY        An identical resolver (same host and callback) is already active.
 *
 */
otError otMdnsStartIp6AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver);

/**
 * Stops an IPv6 address resolver.
 *
 * No action is performed if no matching resolver with the same host name and callback is currently active.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to stop.
 *
 * @retval OT_ERROR_NONE           Resolver stopped successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 *
 */
otError otMdnsStopIp6AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver);

/**
 * Starts an IPv4 address resolver.
 *
 * Initiates a continuous IPv4 address resolver for the specified host name in @p aResolver.
 *
 * Discovered addresses are reported through the `mCallback` function in @p aResolver. The IPv4 addresses are
 * represented using the IPv4-mapped IPv6 address format in `mAddresses` array.  The callback is invoked  whenever
 * addresses are added or removed, providing an updated list. If all addresses are removed, the callback is invoked
 * with an empty list (`mAddresses` will be NULL, and `mAddressesLength` will be zero).
 *
 * The callback may be invoked immediately with cached information (if available) and potentially before this function
 * returns. When cached result is used, the reported TTL values will reflect the original TTL from the last received
 * response.
 *
 * Multiple resolvers can be started for the same host name, provided they use different callback functions.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to be started.
 *
 * @retval OT_ERROR_NONE           Resolver started successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 * @retval OT_ERROR_ALREADY        An identical resolver (same host and callback) is already active.
 *
 */
otError otMdnsStartIp4AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver);

/**
 * Stops an IPv4 address resolver.
 *
 * No action is performed if no matching resolver with the same host name and callback is currently active.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aResolver    The resolver to stop.
 *
 * @retval OT_ERROR_NONE           Resolver stopped successfully.
 * @retval OT_ERROR_INVALID_STATE  mDNS module is not enabled.
 *
 */
otError otMdnsStopIp4AddressResolver(otInstance *aInstance, const otMdnsAddressResolver *aResolver);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_MULTICAST_DNS_H_
