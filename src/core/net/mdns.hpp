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

#ifndef MULTICAST_DNS_HPP_
#define MULTICAST_DNS_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#include <openthread/mdns.h>
#include <openthread/platform/mdns_socket.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/debug.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"
#include "common/heap_allocatable.hpp"
#include "common/heap_array.hpp"
#include "common/heap_data.hpp"
#include "common/heap_string.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/owned_ptr.hpp"
#include "common/owning_list.hpp"
#include "common/retain_ptr.hpp"
#include "common/timer.hpp"
#include "crypto/sha256.hpp"
#include "net/dns_types.hpp"

#if OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF && !OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
#error "OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF requires OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE"
#endif

/**
 * @file
 *   This file includes definitions for the Multicast DNS per RFC 6762.
 */

/**
 * Represents an opaque (and empty) type for an mDNS iterator.
 */
struct otMdnsIterator
{
};

namespace ot {
namespace Dns {
namespace Multicast {

extern "C" void otPlatMdnsHandleReceive(otInstance                  *aInstance,
                                        otMessage                   *aMessage,
                                        bool                         aIsUnicast,
                                        const otPlatMdnsAddressInfo *aAddress);

/**
 * Implements Multicast DNS (mDNS) core.
 */
class Core : public InstanceLocator, private NonCopyable
{
    friend void otPlatMdnsHandleReceive(otInstance                  *aInstance,
                                        otMessage                   *aMessage,
                                        bool                         aIsUnicast,
                                        const otPlatMdnsAddressInfo *aAddress);

public:
    /**
     * Initializes a `Core` instance.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit Core(Instance &aInstance);

    typedef otMdnsRequestId        RequestId;        ///< A request Identifier.
    typedef otMdnsRegisterCallback RegisterCallback; ///< Registration callback.
    typedef otMdnsConflictCallback ConflictCallback; ///< Conflict callback.
    typedef otMdnsEntryState       EntryState;       ///< Host/Service/Key entry state.
    typedef otMdnsHost             Host;             ///< Host information.
    typedef otMdnsService          Service;          ///< Service information.
    typedef otMdnsKey              Key;              ///< Key information.
    typedef otMdnsBrowser          Browser;          ///< Browser.
    typedef otMdnsBrowseCallback   BrowseCallback;   ///< Browser callback.
    typedef otMdnsBrowseResult     BrowseResult;     ///< Browser result.
    typedef otMdnsSrvResolver      SrvResolver;      ///< SRV resolver.
    typedef otMdnsSrvCallback      SrvCallback;      ///< SRV callback.
    typedef otMdnsSrvResult        SrvResult;        ///< SRV result.
    typedef otMdnsTxtResolver      TxtResolver;      ///< TXT resolver.
    typedef otMdnsTxtCallback      TxtCallback;      ///< TXT callback.
    typedef otMdnsTxtResult        TxtResult;        ///< TXT result.
    typedef otMdnsAddressResolver  AddressResolver;  ///< Address resolver.
    typedef otMdnsAddressCallback  AddressCallback;  ///< Address callback
    typedef otMdnsAddressResult    AddressResult;    ///< Address result.
    typedef otMdnsAddressAndTtl    AddressAndTtl;    ///< Address and TTL.
    typedef otMdnsIterator         Iterator;         ///< An entry iterator.
    typedef otMdnsCacheInfo        CacheInfo;        ///< Cache information.

    /**
     * Represents a socket address info.
     */
    class AddressInfo : public otPlatMdnsAddressInfo, public Clearable<AddressInfo>, public Equatable<AddressInfo>
    {
    public:
        /**
         * Initializes the `AddressInfo` clearing all the fields.
         */
        AddressInfo(void) { Clear(); }

        /**
         * Gets the IPv6 address.
         *
         * @returns the IPv6 address.
         */
        const Ip6::Address &GetAddress(void) const { return AsCoreType(&mAddress); }
    };

    /**
     * Enables or disables the mDNS module.
     *
     * mDNS module should be enabled before registration any host, service, or key entries. Disabling mDNS will
     * immediately stop all operations and any communication (multicast or unicast tx) and remove any previously
     * registered entries without sending any "goodbye" announcements or invoking their callback. When disabled,
     * all browsers and resolvers are stopped and all cached information is cleared.
     *
     * @param[in] aEnable       Whether to enable or disable.
     * @param[in] aInfraIfIndex The network interface index for mDNS operation. Value is ignored when disabling.
     *
     * @retval kErrorNone     Enabled or disabled the mDNS module successfully.
     * @retval kErrorAlready  mDNS is already enabled on an enable request, or is already disabled on a disable request.
     * @retval kErrorFailed   Failed to enable/disable mDNS.
     */
    Error SetEnabled(bool aEnable, uint32_t aInfraIfIndex);

    /**
     * Indicates whether or not mDNS module is enabled.
     *
     * @retval TRUE   The mDNS module is enabled.
     * @retval FALSE  The mDNS module is disabled.
     */
    bool IsEnabled(void) const { return mIsEnabled; }

#if OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF
    /**
     * Notifies `AdvertisingProxy` that `InfraIf` state changed.
     */
    void HandleInfraIfStateChanged(void);
#endif

    /**
     * Sets whether mDNS module is allowed to send questions requesting unicast responses referred to as "QU" questions.
     *
     * The "QU" question request unicast response in contrast to "QM" questions which request multicast responses.
     * When allowed, the first probe will be sent as a "QU" question.
     *
     * This can be used to address platform limitation where platform cannot accept unicast response received on mDNS
     * port.
     *
     * @param[in] aAllow        Indicates whether or not to allow "QU" questions.
     */
    void SetQuestionUnicastAllowed(bool aAllow) { mIsQuestionUnicastAllowed = aAllow; }

    /**
     * Indicates whether mDNS module is allowed to send "QU" questions requesting unicast response.
     *
     * @retval TRUE  The mDNS module is allowed to send "QU" questions.
     * @retval FALSE The mDNS module is not allowed to send "QU" questions.
     */
    bool IsQuestionUnicastAllowed(void) const { return mIsQuestionUnicastAllowed; }

    /**
     * Sets the conflict callback.
     *
     * @param[in] aCallback  The conflict callback. Can be `nullptr` is not needed.
     */
    void SetConflictCallback(ConflictCallback aCallback) { mConflictCallback = aCallback; }

    /**
     * Registers or updates a host.
     *
     * The fields in @p aHost follow these rules:
     *
     * - The `mHostName` field specifies the host name to register (e.g., "myhost"). MUST NOT contain the domain name.
     * - The `mAddresses` is array of IPv6 addresses to register with the host. `mAddressesLength` provides the number
     *   of entries in `mAddresses` array.
     * - The `mAddresses` array can be empty with zero `mAddressesLength`. In this case, mDNS will treat it as if host
     *   is unregistered and stop advertising any addresses for this the host name.
     * - The `mTtl` specifies the TTL if non-zero. If zero, the mDNS core will choose a default TTL to use.
     *
     * This method can be called again for the same `mHostName` to update a previously registered host entry, for
     * example, to change the list of addresses of the host. In this case, the mDNS module will send "goodbye"
     * announcements for any previously registered and now removed addresses and announce any newly added addresses.
     *
     * The outcome of the registration request is reported back by invoking the provided @p aCallback with
     * @p aRequestId as its input and one of the following `aError` inputs:
     *
     * - `kErrorNone`       indicates registration was successful
     * - `kErrorDuplicated` indicates a name conflict, i.e., the name is already claimed by another mDNS responder.
     *
     * For caller convenience, the OpenThread mDNS module guarantees that the callback will be invoked after this
     * method returns, even in cases of immediate registration success. The @p aCallback can be `nullptr` if caller
     * does not want to be notified of the outcome.
     *
     * @param[in] aHost         The host to register.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (can be `nullptr` if not needed).
     *
     * @retval kErrorNone          Successfully started registration. @p aCallback will report the outcome.
     * @retval kErrorInvalidState  mDNS module is not enabled.
     */
    Error RegisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Unregisters a host.
     *
     * The fields in @p aHost follow these rules:
     *
     * - The `mHostName` field specifies the host name to unregister (e.g., "myhost"). MUST NOT contain the domain name.
     * - The rest of the fields in @p aHost structure are ignored in an `UnregisterHost()` call.
     *
     * If there is no previously registered host with the same name, no action is performed.
     *
     * If there is a previously registered host with the same name, the mDNS module will send "goodbye" announcement
     * for all previously advertised address records.
     *
     * @param[in] aHost   The host to unregister.
     *
     * @retval kErrorNone           Successfully unregistered host.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     */
    Error UnregisterHost(const Host &aHost);

    /**
     * Registers or updates a service.
     *
     * The fields in @p aService follow these rules:
     *
     * - The `mServiceInstance` specifies the service instance label. It is treated as a single DNS label. It may
     *   contain dot `.` character which is allowed in a service instance label.
     * - The `mServiceType` specifies the service type (e.g., "_tst._udp"). It is treated as multiple dot `.` separated
     *   labels. It MUST NOT contain the domain name.
     * - The `mHostName` field specifies the host name of the service. MUST NOT contain the domain name.
     * - The `mSubTypeLabels` is an array of strings representing sub-types associated with the service. Each array
     *   entry is a sub-type label. The `mSubTypeLabels can be `nullptr` if there are no sub-types. Otherwise, the
     *   array length is specified by `mSubTypeLabelsLength`.
     * - The `mTxtData` and `mTxtDataLength` specify the encoded TXT data. The `mTxtData` can be `nullptr` or
     *   `mTxtDataLength` can be zero to specify an empty TXT data. In this case mDNS module will use a single zero
     *   byte `[ 0 ]` as empty TXT data.
     * - The `mPort`, `mWeight`, and `mPriority` specify the service's parameters (as specified in DNS SRV record).
     * - The `mTtl` specifies the TTL if non-zero. If zero, the mDNS module will use default TTL for service entry.
     *
     * This method can be called again for the same `mServiceInstance` and `mServiceType` to update a previously
     * registered service entry, for example, to change the sub-types list or update any parameter such as port, weight,
     * priority, TTL, or host name. The mDNS module will send announcements for any changed info, e.g., will send
     * "goodbye" announcements for any removed sub-types and announce any newly added sub-types.
     *
     * Regarding the invocation of the @p aCallback, this method behaves in the same way as described in
     * `RegisterHost()`.
     *
     * @param[in] aService      The service to register.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (can be `nullptr` if not needed).
     *
     * @retval kErrorNone           Successfully started registration. @p aCallback will report the outcome.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     */
    Error RegisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Unregisters a service.
     *
     * The fields in @p aService follow these rules:

     * - The `mServiceInstance` specifies the service instance label. It is treated as a single DNS label. It may
     *   contain dot `.` character which is allowed in a service instance label.
     * - The `mServiceType` specifies the service type (e.g., "_tst._udp"). It is treated as multiple dot `.` separated
     *   labels. It MUST NOT contain the domain name.
     * - The rest of the fields in @p aService structure are ignored in  a`otMdnsUnregisterService()` call.
     *
     * If there is no previously registered service with the same name, no action is performed.
     *
     * If there is a previously registered service with the same name, the mDNS module will send "goodbye"
     * announcements for all related records.
     *
     * @param[in] aService      The service to unregister.
     *
     * @retval kErrorNone            Successfully unregistered service.
     * @retval kErrorInvalidState    mDNS module is not enabled.
     */
    Error UnregisterService(const Service &aService);

    /**
     * Registers or updates a key record.
     *
     * The fields in @p aKey follow these rules:
     *
     * - If the key is associated with a host entry, the `mName` field specifies the host name and the `mServiceType`
     *    MUST be `nullptr`.
     * - If the key is associated with a service entry, the `mName` filed specifies the service instance label (always
     *   treated as a single label) and the `mServiceType` filed specifies the service type (e.g. "_tst._udp"). In this
     *   case the DNS name for key record is `<mName>.<mServiceTye>`.
     * - The `mKeyData` field contains the key record's data with `mKeyDataLength` as its length in byes.
     * - The `mTtl` specifies the TTL if non-zero. If zero, the mDNS module will use default TTL for the key entry.
     *
     * This method can be called again for the same name to updated a previously registered key entry, for example,
     * to change the key data or TTL.
     *
     * Regarding the invocation of the @p aCallback, this method behaves in the same way as described in
     * `RegisterHost()`.
     *
     * @param[in] aKey          The key record to register.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (can be `nullptr` if not needed).
     *
     * @retval kErrorNone            Successfully started registration. @p aCallback will report the outcome.
     * @retval kErrorInvalidState    mDNS module is not enabled.
     */
    Error RegisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Unregisters a key record on mDNS.
     *
     * The fields in @p aKey follow these rules:
     *
     * - If the key is associated with a host entry, the `mName` field specifies the host name and the `mServiceType`
     *    MUST be `nullptr`.
     * - If the key is associated with a service entry, the `mName` filed specifies the service instance label (always
     *   treated as a single label) and the `mServiceType` field specifies the service type (e.g. "_tst._udp"). In this
     *   case the DNS name for key record is `<mName>.<mServiceTye>`.
     * - The rest of the fields in @p aKey structure are ignored in  a`otMdnsUnregisterKey()` call.
     *
     * If there is no previously registered key with the same name, no action is performed.
     *
     * If there is a previously registered key with the same name, the mDNS module will send "goodbye" announcements
     * for the key record.
     *
     * @param[in] aKey          The key to unregister.
     *
     * @retval kErrorNone            Successfully unregistered key
     * @retval kErrorInvalidState    mDNS module is not enabled.
     */
    Error UnregisterKey(const Key &aKey);

    /**
     * Starts a service browser.
     *
     * Initiates a continuous search for the specified `mServiceType` in @p aBrowser. For sub-type services, use
     * `mSubTypeLabel` to define the sub-type, for base services, set `mSubTypeLabel` to NULL.
     *
     * Discovered services are reported through the `mCallback` function in @p aBrowser. Services that have been
     * removed are reported with a TTL value of zero. The callback may be invoked immediately with cached information
     * (if available) and potentially before this method returns. When cached results are used, the reported TTL value
     * will reflect the original TTL from the last received response.
     *
     * Multiple browsers can be started for the same service, provided they use different callback functions.
     *
     * @param[in] aBrowser    The browser to be started.
     *
     * @retval kErrorNone           Browser started successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     * @retval kErrorAlready        An identical browser (same service and callback) is already active.
     */
    Error StartBrowser(const Browser &aBrowser);

    /**
     * Stops a service browser.
     *
     * No action is performed if no matching browser with the same service and callback is currently active.
     *
     * @param[in] aBrowser    The browser to stop.
     *
     * @retval kErrorNone           Browser stopped successfully.
     * @retval kErrorInvalidSatet  mDNS module is not enabled.
     */
    Error StopBrowser(const Browser &aBrowser);

    /**
     * Starts an SRV record resolver.
     *
     * Initiates a continuous SRV record resolver for the specified service in @p aResolver.
     *
     * Discovered information is reported through the `mCallback` function in @p aResolver. When the service is removed
     * it is reported with a TTL value of zero. In this case, `mHostName` may be NULL and other result fields (such as
     * `mPort`) should be ignored.
     *
     * The callback may be invoked immediately with cached information (if available) and potentially before this
     * method returns. When cached result is used, the reported TTL value will reflect the original TTL from the last
     * received response.
     *
     * Multiple resolvers can be started for the same service, provided they use different callback functions.
     *
     * @param[in] aResolver    The resolver to be started.
     *
     * @retval kErrorNone           Resolver started successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     * @retval kErrorAlready        An identical resolver (same service and callback) is already active.
     */
    Error StartSrvResolver(const SrvResolver &aResolver);

    /**
     * Stops an SRV record resolver.
     *
     * No action is performed if no matching resolver with the same service and callback is currently active.
     *
     * @param[in] aResolver    The resolver to stop.
     *
     * @retval kErrorNone           Resolver stopped successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     */
    Error StopSrvResolver(const SrvResolver &aResolver);

    /**
     * Starts a TXT record resolver.
     *
     * Initiates a continuous TXT record resolver for the specified service in @p aResolver.
     *
     * Discovered information is reported through the `mCallback` function in @p aResolver. When the TXT record is
     * removed it is reported with a TTL value of zero. In this case, `mTxtData` may be NULL, and other result fields
     * (such as `mTxtDataLength`) should be ignored.
     *
     * The callback may be invoked immediately with cached information (if available) and potentially before this
     * method returns. When cached result is used, the reported TTL value will reflect the original TTL from the last
     * received response.
     *
     * Multiple resolvers can be started for the same service, provided they use different callback functions.
     *
     * @param[in] aResolver    The resolver to be started.
     *
     * @retval kErrorNone           Resolver started successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     * @retval kErrorAlready        An identical resolver (same service and callback) is already active.
     */
    Error StartTxtResolver(const TxtResolver &aResolver);

    /**
     * Stops a TXT record resolver.
     *
     * No action is performed if no matching resolver with the same service and callback is currently active.
     *
     * @param[in] aResolver    The resolver to stop.
     *
     * @retval kErrorNone           Resolver stopped successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     */
    Error StopTxtResolver(const TxtResolver &aResolver);

    /**
     * Starts an IPv6 address resolver.
     *
     * Initiates a continuous IPv6 address resolver for the specified host name in @p aResolver.
     *
     * Discovered addresses are reported through the `mCallback` function in @p aResolver. The callback is invoked
     * whenever addresses are added or removed, providing an updated list. If all addresses are removed, the callback
     * is invoked with an empty list (`mAddresses` will be NULL, and `mAddressesLength` will be zero).
     *
     * The callback may be invoked immediately with cached information (if available) and potentially before this
     * method returns. When cached result is used, the reported TTL values will reflect the original TTL from the last
     * received response.
     *
     * Multiple resolvers can be started for the same host name, provided they use different callback functions.
     *
     * @param[in] aResolver    The resolver to be started.
     *
     * @retval kErrorNone           Resolver started successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     * @retval kErrorAlready        An identical resolver (same host and callback) is already active.
     */
    Error StartIp6AddressResolver(const AddressResolver &aResolver);

    /**
     * Stops an IPv6 address resolver.
     *
     * No action is performed if no matching resolver with the same host name and callback is currently active.
     *
     * @param[in] aResolver    The resolver to stop.
     *
     * @retval kErrorNone           Resolver stopped successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     */
    Error StopIp6AddressResolver(const AddressResolver &aResolver);

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
     * The callback may be invoked immediately with cached information (if available) and potentially before this
     * method returns. When cached result is used, the reported TTL values will reflect the original TTL from the last
     * received response.
     *
     * Multiple resolvers can be started for the same host name, provided they use different callback functions.
     *
     * @param[in] aResolver    The resolver to be started.
     *
     * @retval kErrorNone           Resolver started successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     * @retval kErrorAlready        An identical resolver (same host and callback) is already active.
     */
    Error StartIp4AddressResolver(const AddressResolver &aResolver);

    /**
     * Stops an IPv4 address resolver.
     *
     * No action is performed if no matching resolver with the same host name and callback is currently active.
     *
     * @param[in] aResolver    The resolver to stop.
     *
     * @retval kErrorNone           Resolver stopped successfully.
     * @retval kErrorInvalidState   mDNS module is not enabled.
     */
    Error StopIp4AddressResolver(const AddressResolver &aResolver);

    /**
     * Sets the max size threshold for mDNS messages.
     *
     * This method is mainly intended for testing. The max size threshold is used to break larger messages.
     *
     * @param[in] aMaxSize  The max message size threshold.
     */
    void SetMaxMessageSize(uint16_t aMaxSize) { mMaxMessageSize = aMaxSize; }

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

    /**
     * Allocates a new iterator.
     *
     * @returns   A pointer to the newly allocated iterator or `nullptr` if it fails to allocate.
     */
    Iterator *AllocateIterator(void);

    /**
     * Frees a previously allocated iterator.
     *
     * @param[in] aIterator  The iterator to free.
     */
    void FreeIterator(Iterator &aIterator);

    /**
     * Iterates over registered host entries.
     *
     * On success, @p aHost is populated with information about the next host. Pointers within the `Host` structure
     * (like `mName`) remain valid until the next call to any OpenThread stack's public or platform API/callback.
     *
     * @param[in]  aIterator   The iterator to use.
     * @param[out] aHost       A `Host` to return the information about the next host entry.
     * @param[out] aState      An `EntryState` to return the entry state.
     *
     * @retval kErrorNone         @p aHost, @p aState, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextHost(Iterator &aIterator, Host &aHost, EntryState &aState) const;

    /**
     * Iterates over registered service entries.
     *
     * On success, @p aService is populated with information about the next service. Pointers within the `Service`
     * structure (like `mServiceType`) remain valid until the next call to any OpenThread stack's public or platform
     * API/callback.
     *
     * @param[out] aService    A `Service` to return the information about the next service entry.
     * @param[out] aState      An `EntryState` to return the entry state.
     *
     * @retval kErrorNone         @p aService, @p aState, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextService(Iterator &aIterator, Service &aService, EntryState &aState) const;

    /**
     * Iterates over registered key entries.
     *
     * On success, @p aKey is populated with information about the next key. Pointers within the `Key` structure
     * (like `mName`) remain valid until the next call to any OpenThread stack's public or platform API/callback.
     *
     * @param[out] aKey        A `Key` to return the information about the next key entry.
     * @param[out] aState      An `EntryState` to return the entry state.
     *
     * @retval kErrorNone         @p aKey, @p aState, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextKey(Iterator &aIterator, Key &aKey, EntryState &aState) const;

    /**
     * Iterates over browsers.
     *
     * On success, @p aBrowser is populated with information about the next browser. Pointers within the `Browser`
     * structure  remain valid until the next call to any OpenThread stack's public or platform API/callback.
     *
     * @param[in]  aIterator   The iterator to use.
     * @param[out] aBrowser    A `Browser` to return the information about the next browser.
     * @param[out] aInfo       A `CacheInfo` to return additional information.
     *
     * @retval kErrorNone         @p aBrowser, @p aInfo, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextBrowser(Iterator &aIterator, Browser &aBrowser, CacheInfo &aInfo) const;

    /**
     * Iterates over SRV resolvers.
     *
     * On success, @p aResolver is populated with information about the next resolver. Pointers within the `SrvResolver`
     * structure  remain valid until the next call to any OpenThread stack's public or platform API/callback.
     *
     * @param[in]  aIterator   The iterator to use.
     * @param[out] aResolver   An `SrvResolver` to return the information about the next resolver.
     * @param[out] aInfo       A `CacheInfo` to return additional information.
     *
     * @retval kErrorNone         @p aResolver, @p aInfo, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextSrvResolver(Iterator &aIterator, SrvResolver &aResolver, CacheInfo &aInfo) const;

    /**
     * Iterates over TXT resolvers.
     *
     * On success, @p aResolver is populated with information about the next resolver. Pointers within the `TxtResolver`
     * structure  remain valid until the next call to any OpenThread stack's public or platform API/callback.
     *
     * @param[in]  aIterator   The iterator to use.
     * @param[out] aResolver   A `TxtResolver` to return the information about the next resolver.
     * @param[out] aInfo       A `CacheInfo` to return additional information.
     *
     * @retval kErrorNone         @p aResolver, @p aInfo, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextTxtResolver(Iterator &aIterator, TxtResolver &aResolver, CacheInfo &aInfo) const;

    /**
     * Iterates over IPv6 address resolvers.
     *
     * On success, @p aResolver is populated with information about the next resolver. Pointers within the
     * `AddressResolver` structure  remain valid until the next call to any OpenThread stack's public or platform
     * API/callback.
     *
     * @param[in]  aIterator   The iterator to use.
     * @param[out] aResolver   An `AddressResolver to return the information about the next resolver.
     * @param[out] aInfo       A `CacheInfo` to return additional information.
     *
     * @retval kErrorNone         @p aResolver, @p aInfo, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextIp6AddressResolver(Iterator &aIterator, AddressResolver &aResolver, CacheInfo &aInfo) const;

    /**
     * Iterates over IPv4 address resolvers.
     *
     * On success, @p aResolver is populated with information about the next resolver. Pointers within the
     * `AddressResolver` structure  remain valid until the next call to any OpenThread stack's public or platform
     * API/callback.
     *
     * @param[in]  aIterator   The iterator to use.
     * @param[out] aResolver   An `AddressResolver to return the information about the next resolver.
     * @param[out] aInfo       A `CacheInfo` to return additional information.
     *
     * @retval kErrorNone         @p aResolver, @p aInfo, & @p aIterator are updated successfully.
     * @retval kErrorNotFound     Reached the end of the list.
     * @retval kErrorInvalidArg   @p aIterator is not valid.
     */
    Error GetNextIp4AddressResolver(Iterator &aIterator, AddressResolver &aResolver, CacheInfo &aInfo) const;

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

private:
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    static constexpr uint16_t kUdpPort = 5353;

    static constexpr bool kDefaultQuAllowed = OPENTHREAD_CONFIG_MULTICAST_DNS_DEFAULT_QUESTION_UNICAST_ALLOWED;

    static constexpr uint32_t kMaxMessageSize = 1200;

    static constexpr uint8_t  kNumberOfProbes = 3;
    static constexpr uint32_t kMinProbeDelay  = 20;  // In msec
    static constexpr uint32_t kMaxProbeDelay  = 250; // In msec
    static constexpr uint32_t kProbeWaitTime  = 250; // In msec

    static constexpr uint8_t  kNumberOfAnnounces = 3;
    static constexpr uint32_t kAnnounceInterval  = 1000; // In msec - time between first two announces

    static constexpr uint8_t  kNumberOfInitalQueries = 3;
    static constexpr uint32_t kInitialQueryInterval  = 1000; // In msec - time between first two queries

    static constexpr uint32_t kMinInitialQueryDelay     = 20;  // msec
    static constexpr uint32_t kMaxInitialQueryDelay     = 120; // msec
    static constexpr uint32_t kRandomDelayReuseInterval = 2;   // msec

    static constexpr uint32_t kUnspecifiedTtl       = 0;
    static constexpr uint32_t kDefaultTtl           = 120;
    static constexpr uint32_t kDefaultKeyTtl        = kDefaultTtl;
    static constexpr uint32_t kLegacyUnicastNsecTtl = 10;
    static constexpr uint32_t kNsecTtl              = 4500;
    static constexpr uint32_t kServicesPtrTtl       = 4500;

    static constexpr uint16_t kClassQuestionUnicastFlag = (1U << 15);
    static constexpr uint16_t kClassCacheFlushFlag      = (1U << 15);
    static constexpr uint16_t kClassMask                = (0x7fff);

    static constexpr uint16_t kUnspecifiedOffset = 0;

    static constexpr uint8_t kNumSections = 4;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    enum Section : uint8_t
    {
        kQuestionSection,
        kAnswerSection,
        kAuthoritySection,
        kAdditionalDataSection,
    };

    enum AppendOutcome : uint8_t
    {
        kAppendedFullNameAsCompressed,
        kAppendedLabels,
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Forward declarations

    class EntryTimerContext;
    class TxMessage;
    class RxMessage;
    class ServiceEntry;
    class ServiceType;
    class EntryIterator;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct EmptyChecker
    {
        // Used in `Matches()` to find empty entries (with no record) to remove and free.
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct ExpireChecker
    {
        // Used in `Matches()` to find expired entries in a list.

        explicit ExpireChecker(TimeMilli aNow) { mNow = aNow; }

        TimeMilli mNow;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class Callback : public Clearable<Callback>
    {
    public:
        Callback(void) { Clear(); }
        Callback(RequestId aRequestId, RegisterCallback aCallback);

        bool IsEmpty(void) const { return (mCallback == nullptr); }
        void InvokeAndClear(Instance &aInstance, Error aError);

    private:
        RequestId        mRequestId;
        RegisterCallback mCallback;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class RecordCounts : public Clearable<RecordCounts>
    {
    public:
        RecordCounts(void) { Clear(); }

        uint16_t GetFor(Section aSection) const { return mCounts[aSection]; }
        void     Increment(Section aSection) { mCounts[aSection]++; }
        void     ReadFrom(const Header &aHeader);
        void     WriteTo(Header &aHeader) const;
        bool     IsEmpty(void) const;

    private:
        uint16_t mCounts[kNumSections];
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct AnswerInfo
    {
        uint16_t  mQuestionRrType;
        TimeMilli mAnswerTime;
        bool      mIsProbe;
        bool      mUnicastResponse;
        bool      mLegacyUnicastResponse;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class AddressArray : public Heap::Array<Ip6::Address>
    {
    public:
        bool Matches(const Ip6::Address *aAddresses, uint16_t aNumAddresses) const;
        void SetFrom(const Ip6::Address *aAddresses, uint16_t aNumAddresses);
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class FireTime
    {
    public:
        FireTime(void) { ClearFireTime(); }
        void      ClearFireTime(void) { mHasFireTime = false; }
        bool      HasFireTime(void) const { return mHasFireTime; }
        TimeMilli GetFireTime(void) const { return mFireTime; }
        void      SetFireTime(TimeMilli aFireTime);

    protected:
        void ScheduleFireTimeOn(TimerMilli &aTimer);
        void UpdateNextFireTimeOn(NextFireTime &aNextFireTime) const;

    private:
        TimeMilli mFireTime;
        bool      mHasFireTime;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class RecordInfo : public Clearable<RecordInfo>, private NonCopyable
    {
    public:
        // Keeps track of record state and timings.

        static constexpr uint32_t kMaxLegacyUnicastTtl = 10; // seconds

        RecordInfo(void) { Clear(); }

        bool IsPresent(void) const { return mIsPresent; }

        template <typename UintType> void UpdateProperty(UintType &aProperty, UintType aValue);
        void UpdateProperty(AddressArray &aAddrProperty, const Ip6::Address *aAddrs, uint16_t aNumAddrs);
        void UpdateProperty(Heap::String &aStringProperty, const char *aString);
        void UpdateProperty(Heap::Data &aDataProperty, const uint8_t *aData, uint16_t aLength);

        uint32_t GetTtl(bool aIsLegacyUnicast = false) const;
        void     UpdateTtl(uint32_t aTtl);

        void     StartAnnouncing(void);
        bool     ShouldAppendTo(TxMessage &aResponse, TimeMilli aNow) const;
        bool     CanAnswer(void) const;
        void     ScheduleAnswer(const AnswerInfo &aInfo);
        void     UpdateStateAfterAnswer(const TxMessage &aResponse);
        void     UpdateFireTimeOn(FireTime &aFireTime);
        uint32_t GetDurationSinceLastMulticast(TimeMilli aTime) const;
        Error    GetLastMulticastTime(TimeMilli &aLastMulticastTime) const;

        // `AppendState` methods: Used to track whether the record
        // is appended in a message, or needs to be appended in
        // Additional Data section.

        void MarkAsNotAppended(void) { mAppendState = kNotAppended; }
        void MarkAsAppended(TxMessage &aTxMessage, Section aSection);
        void MarkToAppendInAdditionalData(void);
        bool IsAppended(void) const;
        bool CanAppend(void) const;
        bool ShouldAppendInAdditionalDataSection(void) const { return (mAppendState == kToAppendInAdditionalData); }

    private:
        enum AppendState : uint8_t
        {
            kNotAppended,
            kToAppendInAdditionalData,
            kAppendedInMulticastMsg,
            kAppendedInUnicastMsg,
        };

        static constexpr uint32_t kMinIntervalBetweenMulticast = 1000; // msec
        static constexpr uint32_t kLastMulticastTimeAge        = 10 * Time::kOneHourInMsec;

        static_assert(kNotAppended == 0, "kNotAppended MUST be zero, so `Clear()` works correctly");

        bool        mIsPresent : 1;
        bool        mMulticastAnswerPending : 1;
        bool        mUnicastAnswerPending : 1;
        bool        mIsLastMulticastValid : 1;
        uint8_t     mAnnounceCounter;
        AppendState mAppendState;
        Section     mAppendSection;
        uint32_t    mTtl;
        TimeMilli   mAnnounceTime;
        TimeMilli   mAnswerTime;
        TimeMilli   mLastMulticastTime;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class Entry : public InstanceLocatorInit, public FireTime, private NonCopyable
    {
        // Base class for `HostEntry` and `ServiceEntry`.

        friend class ServiceType;

    public:
        enum State : uint8_t
        {
            kProbing    = OT_MDNS_ENTRY_STATE_PROBING,
            kRegistered = OT_MDNS_ENTRY_STATE_REGISTERED,
            kConflict   = OT_MDNS_ENTRY_STATE_CONFLICT,
            kRemoving   = OT_MDNS_ENTRY_STATE_REMOVING,
        };

        State GetState(void) const { return mState; }
        bool  HasKeyRecord(void) const { return mKeyRecord.IsPresent(); }
        void  Register(const Key &aKey, const Callback &aCallback);
        void  Unregister(const Key &aKey);
        void  InvokeCallbacks(void);
        void  ClearAppendState(void);
        Error CopyKeyInfoTo(Key &aKey, EntryState &aState) const;

    protected:
        static constexpr uint32_t kMinIntervalProbeResponse = 250; // msec
        static constexpr uint8_t  kTypeArraySize            = 8;   // We can have SRV, TXT and KEY today.

        struct TypeArray : public Array<uint16_t, kTypeArraySize> // Array of record types for NSEC record
        {
            void Add(uint16_t aType) { SuccessOrAssert(PushBack(aType)); }
        };

        struct RecordAndType
        {
            RecordInfo &mRecord;
            uint16_t    mType;
        };

        typedef void (*NameAppender)(Entry &aEntry, TxMessage &aTxMessage, Section aSection);

        Entry(void);
        void Init(Instance &aInstance);
        void SetCallback(const Callback &aCallback);
        void ClearCallback(void) { mCallback.Clear(); }
        void MarkToInvokeCallbackUnconditionally(void);
        void StartProbing(void);
        void SetStateToConflict(void);
        void SetStateToRemoving(void);
        void UpdateRecordsState(const TxMessage &aResponse);
        void AppendQuestionTo(TxMessage &aTxMessage) const;
        void AppendKeyRecordTo(TxMessage &aTxMessage, Section aSection, NameAppender aNameAppender);
        void AppendNsecRecordTo(TxMessage       &aTxMessage,
                                Section          aSection,
                                const TypeArray &aTypes,
                                NameAppender     aNameAppender);
        bool ShouldAnswerNsec(TimeMilli aNow) const;
        void DetermineNextFireTime(void);
        void ScheduleTimer(void);
        void AnswerProbe(const AnswerInfo &aInfo, RecordAndType *aRecords, uint16_t aRecordsLength);
        void AnswerNonProbe(const AnswerInfo &aInfo, RecordAndType *aRecords, uint16_t aRecordsLength);
        void ScheduleNsecAnswer(const AnswerInfo &aInfo);

        template <typename EntryType> void HandleTimer(EntryTimerContext &aContext);

        RecordInfo mKeyRecord;

    private:
        void SetState(State aState);
        void ClearKey(void);
        void ScheduleCallbackTask(void);
        void CheckMessageSizeLimitToPrepareAgain(TxMessage &aTxMessage, bool &aPrepareAgain);

        State      mState;
        uint8_t    mProbeCount;
        bool       mMulticastNsecPending : 1;
        bool       mUnicastNsecPending : 1;
        bool       mAppendedNsec : 1;
        bool       mBypassCallbackStateCheck : 1;
        TimeMilli  mNsecAnswerTime;
        Heap::Data mKeyData;
        Callback   mCallback;
        Callback   mKeyCallback;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class HostEntry : public Entry, public LinkedListEntry<HostEntry>, public Heap::Allocatable<HostEntry>
    {
        friend class LinkedListEntry<HostEntry>;
        friend class Entry;
        friend class ServiceEntry;

    public:
        HostEntry(void);
        Error Init(Instance &aInstance, const Host &aHost) { return Init(aInstance, aHost.mHostName); }
        Error Init(Instance &aInstance, const Key &aKey) { return Init(aInstance, aKey.mName); }
        bool  IsEmpty(void) const;
        bool  Matches(const Name &aName) const;
        bool  Matches(const Host &aHost) const;
        bool  Matches(const Key &aKey) const;
        bool  Matches(const Heap::String &aName) const;
        bool  Matches(State aState) const { return GetState() == aState; }
        bool  Matches(const HostEntry &aEntry) const { return (this == &aEntry); }
        void  Register(const Host &aHost, const Callback &aCallback);
        void  Register(const Key &aKey, const Callback &aCallback);
        void  Unregister(const Host &aHost);
        void  Unregister(const Key &aKey);
        void  AnswerQuestion(const AnswerInfo &aInfo);
        void  HandleTimer(EntryTimerContext &aContext);
        void  ClearAppendState(void);
        void  PrepareResponse(TxMessage &aResponse, TimeMilli aNow);
        void  HandleConflict(void);
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        Error CopyInfoTo(Host &aHost, EntryState &aState) const;
        Error CopyInfoTo(Key &aKey, EntryState &aState) const;
#endif

    private:
        Error Init(Instance &aInstance, const char *aName);
        void  ClearHost(void);
        void  ScheduleToRemoveIfEmpty(void);
        void  PrepareProbe(TxMessage &aProbe);
        void  StartAnnouncing(void);
        void  PrepareResponseRecords(TxMessage &aResponse, TimeMilli aNow);
        void  UpdateRecordsState(const TxMessage &aResponse);
        void  DetermineNextFireTime(void);
        void  AppendAddressRecordsTo(TxMessage &aTxMessage, Section aSection);
        void  AppendKeyRecordTo(TxMessage &aTxMessage, Section aSection);
        void  AppendNsecRecordTo(TxMessage &aTxMessage, Section aSection);
        void  AppendNameTo(TxMessage &aTxMessage, Section aSection);

        static void AppendEntryName(Entry &aEntry, TxMessage &aTxMessage, Section aSection);

        HostEntry   *mNext;
        Heap::String mName;
        RecordInfo   mAddrRecord;
        AddressArray mAddresses;
        uint16_t     mNameOffset;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class ServiceEntry : public Entry, public LinkedListEntry<ServiceEntry>, public Heap::Allocatable<ServiceEntry>
    {
        friend class LinkedListEntry<ServiceEntry>;
        friend class Entry;
        friend class ServiceType;

    public:
        ServiceEntry(void);
        Error Init(Instance &aInstance, const Service &aService);
        Error Init(Instance &aInstance, const Key &aKey);
        bool  IsEmpty(void) const;
        bool  Matches(const Name &aFullName) const;
        bool  Matches(const Service &aService) const;
        bool  Matches(const Key &aKey) const;
        bool  Matches(State aState) const { return GetState() == aState; }
        bool  Matches(const ServiceEntry &aEntry) const { return (this == &aEntry); }
        bool  MatchesServiceType(const Name &aServiceType) const;
        bool  CanAnswerSubType(const char *aSubLabel) const;
        void  Register(const Service &aService, const Callback &aCallback);
        void  Register(const Key &aKey, const Callback &aCallback);
        void  Unregister(const Service &aService);
        void  Unregister(const Key &aKey);
        void  AnswerServiceNameQuestion(const AnswerInfo &aInfo);
        void  AnswerServiceTypeQuestion(const AnswerInfo &aInfo, const char *aSubLabel);
        bool  ShouldSuppressKnownAnswer(uint32_t aTtl, const char *aSubLabel) const;
        void  HandleTimer(EntryTimerContext &aContext);
        void  ClearAppendState(void);
        void  PrepareResponse(TxMessage &aResponse, TimeMilli aNow);
        void  HandleConflict(void);
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        Error CopyInfoTo(Service &aService, EntryState &aState, EntryIterator &aIterator) const;
        Error CopyInfoTo(Key &aKey, EntryState &aState) const;
#endif

    private:
        class SubType : public LinkedListEntry<SubType>, public Heap::Allocatable<SubType>, private ot::NonCopyable
        {
        public:
            Error Init(const char *aLabel);
            bool  Matches(const char *aLabel) const { return NameMatch(mLabel, aLabel); }
            bool  Matches(const EmptyChecker &aChecker) const;
            bool  IsContainedIn(const Service &aService) const;

            SubType     *mNext;
            Heap::String mLabel;
            RecordInfo   mPtrRecord;
            uint16_t     mSubServiceNameOffset;
        };

        Error Init(Instance &aInstance, const char *aServiceInstance, const char *aServiceType);
        void  ClearService(void);
        void  ScheduleToRemoveIfEmpty(void);
        void  PrepareProbe(TxMessage &aProbe);
        void  StartAnnouncing(void);
        void  PrepareResponseRecords(TxMessage &aResponse, TimeMilli aNow);
        void  UpdateRecordsState(const TxMessage &aResponse);
        void  DetermineNextFireTime(void);
        void  DiscoverOffsetsAndHost(HostEntry *&aHost);
        void  UpdateServiceTypes(void);
        void  AppendSrvRecordTo(TxMessage &aTxMessage, Section aSection);
        void  AppendTxtRecordTo(TxMessage &aTxMessage, Section aSection);
        void  AppendPtrRecordTo(TxMessage &aTxMessage, Section aSection, SubType *aSubType = nullptr);
        void  AppendKeyRecordTo(TxMessage &aTxMessage, Section aSection);
        void  AppendNsecRecordTo(TxMessage &aTxMessage, Section aSection);
        void  AppendServiceNameTo(TxMessage &TxMessage, Section aSection, bool aPerformNameCompression = true);
        void  AppendServiceTypeTo(TxMessage &aTxMessage, Section aSection);
        void  AppendSubServiceTypeTo(TxMessage &aTxMessage, Section aSection);
        void  AppendSubServiceNameTo(TxMessage &aTxMessage, Section aSection, SubType &aSubType);
        void  AppendHostNameTo(TxMessage &aTxMessage, Section aSection);

        static void AppendEntryName(Entry &aEntry, TxMessage &aTxMessage, Section aSection);

        static const uint8_t kEmptyTxtData[];

        ServiceEntry       *mNext;
        Heap::String        mServiceInstance;
        Heap::String        mServiceType;
        RecordInfo          mPtrRecord;
        RecordInfo          mSrvRecord;
        RecordInfo          mTxtRecord;
        OwningList<SubType> mSubTypes;
        Heap::String        mHostName;
        Heap::Data          mTxtData;
        uint16_t            mPriority;
        uint16_t            mWeight;
        uint16_t            mPort;
        uint16_t            mServiceNameOffset;
        uint16_t            mServiceTypeOffset;
        uint16_t            mSubServiceTypeOffset;
        uint16_t            mHostNameOffset;
        bool                mIsAddedInServiceTypes;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class ServiceType : public InstanceLocatorInit,
                        public FireTime,
                        public LinkedListEntry<ServiceType>,
                        public Heap::Allocatable<ServiceType>,
                        private NonCopyable
    {
        // Track a service type to answer to `_services._dns-sd._udp.local`
        // queries.

        friend class LinkedListEntry<ServiceType>;

    public:
        Error    Init(Instance &aInstance, const char *aServiceType);
        bool     Matches(const Name &aServiceTypeName) const;
        bool     Matches(const Heap::String &aServiceType) const;
        bool     Matches(const ServiceType &aServiceType) const { return (this == &aServiceType); }
        void     IncrementNumEntries(void) { mNumEntries++; }
        void     DecrementNumEntries(void) { mNumEntries--; }
        uint16_t GetNumEntries(void) const { return mNumEntries; }
        void     ClearAppendState(void);
        void     AnswerQuestion(const AnswerInfo &aInfo);
        bool     ShouldSuppressKnownAnswer(uint32_t aTtl) const;
        void     HandleTimer(EntryTimerContext &aContext);
        void     PrepareResponse(TxMessage &aResponse, TimeMilli aNow);

    private:
        void PrepareResponseRecords(TxMessage &aResponse, TimeMilli aNow);
        void AppendPtrRecordTo(TxMessage &aResponse, uint16_t aServiceTypeOffset);

        ServiceType *mNext;
        Heap::String mServiceType;
        RecordInfo   mServicesPtr;
        uint16_t     mNumEntries; // Number of service entries providing this service type.
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class TxMessage : public InstanceLocator
    {
    public:
        enum Type : uint8_t
        {
            kMulticastProbe,
            kMulticastQuery,
            kMulticastResponse,
            kUnicastResponse,
            kLegacyUnicastResponse,
        };

        TxMessage(Instance &aInstance, Type aType, uint16_t aQueryId = 0);
        TxMessage(Instance &aInstance, Type aType, const AddressInfo &aUnicastDest, uint16_t aQueryId = 0);
        Type          GetType(void) const { return mType; }
        Message      &SelectMessageFor(Section aSection);
        AppendOutcome AppendLabel(Section aSection, const char *aLabel, uint16_t &aCompressOffset);
        AppendOutcome AppendMultipleLabels(Section aSection, const char *aLabels, uint16_t &aCompressOffset);
        void          AppendServiceType(Section aSection, const char *aServiceType, uint16_t &aCompressOffset);
        void          AppendDomainName(Section aSection);
        void          AppendServicesDnssdName(Section aSection);
        void          AddQuestionFrom(const Message &aMessage);
        void          IncrementRecordCount(Section aSection) { mRecordCounts.Increment(aSection); }
        void          CheckSizeLimitToPrepareAgain(bool &aPrepareAgain);
        void          SaveCurrentState(void);
        void          RestoreToSavedState(void);
        void          Send(void);

    private:
        static constexpr bool kIsSingleLabel = true;

        void          Init(Type aType, uint16_t aMessageId = 0);
        void          Reinit(void);
        bool          IsOverSizeLimit(void) const;
        AppendOutcome AppendLabels(Section     aSection,
                                   const char *aLabels,
                                   bool        aIsSingleLabel,
                                   uint16_t   &aCompressOffset);
        bool          ShouldClearAppendStateOnReinit(const Entry &aEntry) const;

        static void SaveOffset(uint16_t &aCompressOffset, const Message &aMessage, Section aSection);

        RecordCounts      mRecordCounts;
        OwnedPtr<Message> mMsgPtr;
        OwnedPtr<Message> mExtraMsgPtr;
        RecordCounts      mSavedRecordCounts;
        uint16_t          mSavedMsgLength;
        uint16_t          mSavedExtraMsgLength;
        uint16_t          mDomainOffset;        // Offset for domain name `.local.` for name compression.
        uint16_t          mUdpOffset;           // Offset to `_udp.local.`
        uint16_t          mTcpOffset;           // Offset to `_tcp.local.`
        uint16_t          mServicesDnssdOffset; // Offset to `_services._dns-sd`
        AddressInfo       mUnicastDest;
        Type              mType;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class EntryTimerContext : public InstanceLocator // Used by `HandleEntryTimer`.
    {
    public:
        EntryTimerContext(Instance &aInstance);
        TimeMilli     GetNow(void) const { return mNextFireTime.GetNow(); }
        NextFireTime &GetNextFireTime(void) { return mNextFireTime; }
        TxMessage    &GetProbeMessage(void) { return mProbeMessage; }
        TxMessage    &GetResponseMessage(void) { return mResponseMessage; }

    private:
        NextFireTime mNextFireTime;
        TxMessage    mProbeMessage;
        TxMessage    mResponseMessage;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class RxMessage : public InstanceLocatorInit,
                      public Heap::Allocatable<RxMessage>,
                      public LinkedListEntry<RxMessage>,
                      private NonCopyable
    {
        friend class LinkedListEntry<RxMessage>;

    public:
        enum ProcessOutcome : uint8_t
        {
            kProcessed,
            kSaveAsMultiPacket,
        };

        Error               Init(Instance          &aInstance,
                                 OwnedPtr<Message> &aMessagePtr,
                                 bool               aIsUnicast,
                                 const AddressInfo &aSenderAddress);
        bool                IsQuery(void) const { return mIsQuery; }
        bool                IsTruncated(void) const { return mTruncated; }
        bool                IsSelfOriginating(void) const { return mIsSelfOriginating; }
        const RecordCounts &GetRecordCounts(void) const { return mRecordCounts; }
        const AddressInfo  &GetSenderAddress(void) const { return mSenderAddress; }
        void                ClearProcessState(void);
        ProcessOutcome      ProcessQuery(bool aShouldProcessTruncated);
        void                ProcessResponse(void);

    private:
        typedef void (RxMessage::*RecordProcessor)(const Name           &aName,
                                                   const ResourceRecord &aRecord,
                                                   uint16_t              aRecordOffset);

        struct Question : public Clearable<Question>
        {
            Question(void) { Clear(); }
            void ClearProcessState(void);

            Entry   *mEntry;                     // Entry which can provide answer (if any).
            uint16_t mNameOffset;                // Offset to start of question name.
            uint16_t mRrType;                    // The question record type.
            bool     mIsRrClassInternet : 1;     // Is the record class Internet or Any.
            bool     mIsProbe : 1;               // Is a probe (contains a matching record in Authority section).
            bool     mUnicastResponse : 1;       // Is QU flag set (requesting a unicast response).
            bool     mCanAnswer : 1;             // Can provide answer for this question
            bool     mIsUnique : 1;              // Is unique record (vs a shared record).
            bool     mIsForService : 1;          // Is for a `ServiceEntry` (vs a `HostEntry`).
            bool     mIsServiceType : 1;         // Is for service type or sub-type of a `ServiceEntry`.
            bool     mIsForAllServicesDnssd : 1; // Is for "_services._dns-sd._udp" (all service types).
        };

        static constexpr uint32_t kMinResponseDelay = 20;  // msec
        static constexpr uint32_t kMaxResponseDelay = 120; // msec

        void ProcessQuestion(Question &aQuestion);
        void AnswerQuestion(const Question &aQuestion, TimeMilli aAnswerTime);
        void AnswerServiceTypeQuestion(const Question &aQuestion, const AnswerInfo &aInfo, ServiceEntry &aFirstEntry);
        bool ShouldSuppressKnownAnswer(const Name         &aServiceType,
                                       const char         *aSubLabel,
                                       const ServiceEntry &aServiceEntry) const;
        bool ParseQuestionNameAsSubType(const Question    &aQuestion,
                                        Name::LabelBuffer &aSubLabel,
                                        Name              &aServiceType) const;
        void AnswerAllServicesQuestion(const Question &aQuestion, const AnswerInfo &aInfo);
        bool ShouldSuppressKnownAnswer(const Question &aQuestion, const ServiceType &aServiceType) const;
        void SendUnicastResponse(const AddressInfo &aUnicastDest);
        void IterateOnAllRecordsInResponse(RecordProcessor aRecordProcessor);
        void ProcessRecordForConflict(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset);
        void ProcessPtrRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset);
        void ProcessSrvRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset);
        void ProcessTxtRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset);
        void ProcessAaaaRecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset);
        void ProcessARecord(const Name &aName, const ResourceRecord &aRecord, uint16_t aRecordOffset);

        RxMessage            *mNext;
        OwnedPtr<Message>     mMessagePtr;
        Heap::Array<Question> mQuestions;
        AddressInfo           mSenderAddress;
        RecordCounts          mRecordCounts;
        uint16_t              mStartOffset[kNumSections];
        uint16_t              mQueryId;
        bool                  mIsQuery : 1;
        bool                  mIsUnicast : 1;
        bool                  mIsLegacyUnicast : 1;
        bool                  mTruncated : 1;
        bool                  mIsSelfOriginating : 1;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleMultiPacketTimer(void) { mMultiPacketRxMessages.HandleTimer(); }

    class MultiPacketRxMessages : public InstanceLocator
    {
    public:
        explicit MultiPacketRxMessages(Instance &aInstance);

        void AddToExisting(OwnedPtr<RxMessage> &aRxMessagePtr);
        void AddNew(OwnedPtr<RxMessage> &aRxMessagePtr);
        void HandleTimer(void);
        void Clear(void);

    private:
        static constexpr uint32_t kMinProcessDelay = 400; // msec
        static constexpr uint32_t kMaxProcessDelay = 500; // msec
        static constexpr uint16_t kMaxNumMessages  = 10;

        struct RxMsgEntry : public InstanceLocator,
                            public LinkedListEntry<RxMsgEntry>,
                            public Heap::Allocatable<RxMsgEntry>,
                            private NonCopyable
        {
            explicit RxMsgEntry(Instance &aInstance);

            bool Matches(const AddressInfo &aAddress) const;
            bool Matches(const ExpireChecker &aExpireChecker) const;
            void Add(OwnedPtr<RxMessage> &aRxMessagePtr);

            OwningList<RxMessage> mRxMessages;
            TimeMilli             mProcessTime;
            RxMsgEntry           *mNext;
        };

        using MultiPacketTimer = TimerMilliIn<Core, &Core::HandleMultiPacketTimer>;

        OwningList<RxMsgEntry> mRxMsgEntries;
        MultiPacketTimer       mTimer;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    void HandleTxMessageHistoryTimer(void) { mTxMessageHistory.HandleTimer(); }

    class TxMessageHistory : public InstanceLocator
    {
        // Keep track of messages sent by mDNS module to tell if
        // a received message is self originating.

    public:
        explicit TxMessageHistory(Instance &aInstance);
        void Clear(void);
        void Add(const Message &aMessage);
        bool Contains(const Message &aMessage) const;
        void HandleTimer(void);

    private:
        static constexpr uint32_t kExpireInterval = TimeMilli::SecToMsec(10); // in msec

        typedef Crypto::Sha256::Hash Hash;

        struct HashEntry : public LinkedListEntry<HashEntry>, public Heap::Allocatable<HashEntry>
        {
            bool Matches(const Hash &aHash) const { return aHash == mHash; }
            bool Matches(const ExpireChecker &aExpireChecker) const { return mExpireTime <= aExpireChecker.mNow; }

            HashEntry *mNext;
            Hash       mHash;
            TimeMilli  mExpireTime;
        };

        static void CalculateHash(const Message &aMessage, Hash &aHash);

        using TxMsgHistoryTimer = TimerMilliIn<Core, &Core::HandleTxMessageHistoryTimer>;

        OwningList<HashEntry> mHashEntries;
        TxMsgHistoryTimer     mTimer;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class CacheEntry;
    class TxtCache;

    class ResultCallback : public LinkedListEntry<ResultCallback>, public Heap::Allocatable<ResultCallback>
    {
        friend class Heap::Allocatable<ResultCallback>;
        friend class LinkedListEntry<ResultCallback>;
        friend class CacheEntry;

    public:
        ResultCallback(const ResultCallback &aResultCallback) = default;

        template <typename CallbackType>
        explicit ResultCallback(CallbackType aCallback)
            : mNext(nullptr)
            , mSharedCallback(aCallback)
        {
        }

        bool Matches(BrowseCallback aCallback) const { return mSharedCallback.mBrowse == aCallback; }
        bool Matches(SrvCallback aCallback) const { return mSharedCallback.mSrv == aCallback; }
        bool Matches(TxtCallback aCallback) const { return mSharedCallback.mTxt == aCallback; }
        bool Matches(AddressCallback aCallback) const { return mSharedCallback.mAddress == aCallback; }
        bool Matches(EmptyChecker) const { return (mSharedCallback.mSrv == nullptr); }

        void Invoke(Instance &aInstance, const BrowseResult &aResult) const;
        void Invoke(Instance &aInstance, const SrvResult &aResult) const;
        void Invoke(Instance &aInstance, const TxtResult &aResult) const;
        void Invoke(Instance &aInstance, const AddressResult &aResult) const;

        void ClearCallback(void) { mSharedCallback.Clear(); }

    private:
        union SharedCallback
        {
            explicit SharedCallback(BrowseCallback aCallback) { mBrowse = aCallback; }
            explicit SharedCallback(SrvCallback aCallback) { mSrv = aCallback; }
            explicit SharedCallback(TxtCallback aCallback) { mTxt = aCallback; }
            explicit SharedCallback(AddressCallback aCallback) { mAddress = aCallback; }

            void Clear(void) { mBrowse = nullptr; }

            BrowseCallback  mBrowse;
            SrvCallback     mSrv;
            TxtCallback     mTxt;
            AddressCallback mAddress;
        };

        ResultCallback *mNext;
        SharedCallback  mSharedCallback;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class CacheTimerContext : public InstanceLocator
    {
    public:
        CacheTimerContext(Instance &aInstance);
        TimeMilli     GetNow(void) const { return mNextFireTime.GetNow(); }
        NextFireTime &GetNextFireTime(void) { return mNextFireTime; }
        TxMessage    &GetQueryMessage(void) { return mQueryMessage; }

    private:
        NextFireTime mNextFireTime;
        TxMessage    mQueryMessage;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class CacheRecordInfo
    {
    public:
        CacheRecordInfo(void);

        bool     IsPresent(void) const { return (mTtl > 0); }
        uint32_t GetTtl(void) const { return mTtl; }
        bool     RefreshTtl(uint32_t aTtl);
        bool     ShouldExpire(TimeMilli aNow) const;
        void     UpdateStateAfterQuery(TimeMilli aNow);
        void     UpdateQueryAndFireTimeOn(CacheEntry &aCacheEntry);
        bool     LessThanHalfTtlRemains(TimeMilli aNow) const;
        uint32_t GetRemainingTtl(TimeMilli aNow) const;

    private:
        static constexpr uint32_t kMaxTtl            = (24 * 3600); // One day
        static constexpr uint8_t  kNumberOfQueries   = 4;
        static constexpr uint32_t kQueryTtlVariation = 1000 * 2 / 100; // 2%

        uint32_t  GetClampedTtl(void) const;
        TimeMilli GetExpireTime(void) const;
        TimeMilli GetQueryTime(uint8_t aAttemptIndex) const;

        uint32_t  mTtl;
        TimeMilli mLastRxTime;
        uint8_t   mQueryCount;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class CacheEntry : public FireTime, public InstanceLocatorInit, private NonCopyable
    {
        // Base class for cache entries: `BrowseCache`, `mSrvCache`,
        // `mTxtCache`, etc. Implements common behaviors: initial
        // queries, query/timer scheduling, callback tracking, entry
        // aging, and timer handling. Tracks entry type in `mType` and
        // invokes sub-class method for type-specific behaviors
        // (e.g., query message construction).

    public:
        void HandleTimer(CacheTimerContext &aContext);
        void ClearEmptyCallbacks(void);
        void ScheduleQuery(TimeMilli aQueryTime);

    protected:
        enum Type : uint8_t
        {
            kBrowseCache,
            kSrvCache,
            kTxtCache,
            kIp6AddrCache,
            kIp4AddrCache,
        };

        void  Init(Instance &aInstance, Type aType);
        bool  IsActive(void) const { return mIsActive; }
        bool  ShouldDelete(TimeMilli aNow) const;
        void  StartInitialQueries(void);
        void  StopInitialQueries(void) { mInitalQueries = kNumberOfInitalQueries; }
        Error Add(const ResultCallback &aCallback);
        void  Remove(const ResultCallback &aCallback);
        void  DetermineNextFireTime(void);
        void  ScheduleTimer(void);

        template <typename ResultType> void InvokeCallbacks(const ResultType &aResult);

    private:
        static constexpr uint32_t kMinIntervalBetweenQueries = 1000; // In msec
        static constexpr uint32_t kNonActiveDeleteTimeout    = 7 * Time::kOneMinuteInMsec;

        typedef OwningList<ResultCallback> CallbackList;

        void SetIsActive(bool aIsActive);
        bool ShouldQuery(TimeMilli aNow);
        void PrepareQuery(CacheTimerContext &aContext);
        void ProcessExpiredRecords(TimeMilli aNow);
        void DetermineNextInitialQueryTime(void);

        ResultCallback *FindCallbackMatching(const ResultCallback &aCallback);

        template <typename CacheType> CacheType       &As(void) { return *static_cast<CacheType *>(this); }
        template <typename CacheType> const CacheType &As(void) const { return *static_cast<const CacheType *>(this); }

        Type         mType;                   // Cache entry type.
        uint8_t      mInitalQueries;          // Number initial queries sent already.
        bool         mQueryPending : 1;       // Whether a query tx request is pending.
        bool         mLastQueryTimeValid : 1; // Whether `mLastQueryTime` is valid.
        bool         mIsActive : 1;           // Whether there is any active resolver/browser for this entry.
        TimeMilli    mNextQueryTime;          // The next query tx time when `mQueryPending`.
        TimeMilli    mLastQueryTime;          // The last query tx time or the upcoming tx time of first initial query.
        TimeMilli    mDeleteTime;             // The time to delete the entry when not `mIsActive`.
        CallbackList mCallbacks;              // Resolver/Browser callbacks.
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class BrowseCache : public CacheEntry, public LinkedListEntry<BrowseCache>, public Heap::Allocatable<BrowseCache>
    {
        friend class LinkedListEntry<BrowseCache>;
        friend class Heap::Allocatable<BrowseCache>;
        friend class CacheEntry;

    public:
        void  ClearCompressOffsets(void);
        bool  Matches(const Name &aFullName) const;
        bool  Matches(const char *aServiceType, const char *aSubTypeLabel) const;
        bool  Matches(const Browser &aBrowser) const;
        bool  Matches(const ExpireChecker &aExpireChecker) const;
        Error Add(const Browser &aBrowser);
        void  Remove(const Browser &aBrowser);
        void  ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset);
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        void CopyInfoTo(Browser &aBrowser, CacheInfo &aInfo) const;
#endif

    private:
        struct PtrEntry : public LinkedListEntry<PtrEntry>, public Heap::Allocatable<PtrEntry>
        {
            Error Init(const char *aServiceInstance);
            bool  Matches(const char *aServiceInstance) const { return NameMatch(mServiceInstance, aServiceInstance); }
            bool  Matches(const ExpireChecker &aExpireChecker) const;
            void  ConvertTo(BrowseResult &aResult, const BrowseCache &aBrowseCache) const;

            PtrEntry       *mNext;
            Heap::String    mServiceInstance;
            CacheRecordInfo mRecord;
        };

        // Called by base class `CacheEntry`
        void PreparePtrQuestion(TxMessage &aQuery, TimeMilli aNow);
        void UpdateRecordStateAfterQuery(TimeMilli aNow);
        void DetermineRecordFireTime(void);
        void ProcessExpiredRecords(TimeMilli aNow);
        void ReportResultsTo(ResultCallback &aCallback) const;

        Error Init(Instance &aInstance, const char *aServiceType, const char *aSubTypeLabel);
        Error Init(Instance &aInstance, const Browser &aBrowser);
        void  AppendServiceTypeOrSubTypeTo(TxMessage &aTxMessage, Section aSection);
        void  AppendKnownAnswer(TxMessage &aTxMessage, const PtrEntry &aPtrEntry, TimeMilli aNow);
        void  DiscoverCompressOffsets(void);

        BrowseCache         *mNext;
        Heap::String         mServiceType;
        Heap::String         mSubTypeLabel;
        OwningList<PtrEntry> mPtrEntries;
        uint16_t             mServiceTypeOffset;
        uint16_t             mSubServiceTypeOffset;
        uint16_t             mSubServiceNameOffset;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    struct ServiceName
    {
        ServiceName(const char *aServiceInstance, const char *aServiceType)
            : mServiceInstance(aServiceInstance)
            , mServiceType(aServiceType)
        {
        }

        const char *mServiceInstance;
        const char *mServiceType;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class ServiceCache : public CacheEntry
    {
        // Base class for `SrvCache` and `TxtCache`, tracking common info
        // shared between the two, e.g. service instance/type strings,
        // record info, and append state and compression offsets.

        friend class CacheEntry;

    public:
        void ClearCompressOffsets(void);

    protected:
        ServiceCache(void) = default;

        Error Init(Instance &aInstance, Type aType, const char *aServiceInstance, const char *aServiceType);
        bool  Matches(const Name &aFullName) const;
        bool  Matches(const char *aServiceInstance, const char *aServiceType) const;
        void  PrepareQueryQuestion(TxMessage &aQuery, uint16_t aRrType);
        void  AppendServiceNameTo(TxMessage &aTxMessage, Section aSection);
        void  UpdateRecordStateAfterQuery(TimeMilli aNow);
        void  DetermineRecordFireTime(void);
        bool  ShouldStartInitialQueries(void) const;

        CacheRecordInfo mRecord;
        Heap::String    mServiceInstance;
        Heap::String    mServiceType;
        uint16_t        mServiceNameOffset;
        uint16_t        mServiceTypeOffset;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class SrvCache : public ServiceCache, public LinkedListEntry<SrvCache>, public Heap::Allocatable<SrvCache>
    {
        friend class LinkedListEntry<SrvCache>;
        friend class Heap::Allocatable<SrvCache>;
        friend class CacheEntry;
        friend class TxtCache;
        friend class BrowseCache;

    public:
        bool  Matches(const Name &aFullName) const;
        bool  Matches(const SrvResolver &aResolver) const;
        bool  Matches(const ServiceName &aServiceName) const;
        bool  Matches(const ExpireChecker &aExpireChecker) const;
        Error Add(const SrvResolver &aResolver);
        void  Remove(const SrvResolver &aResolver);
        void  ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset);
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        void CopyInfoTo(SrvResolver &aResolver, CacheInfo &aInfo) const;
#endif

    private:
        Error Init(Instance &aInstance, const char *aServiceInstance, const char *aServiceType);
        Error Init(Instance &aInstance, const ServiceName &aServiceName);
        Error Init(Instance &aInstance, const SrvResolver &aResolver);
        void  PrepareSrvQuestion(TxMessage &aQuery);
        void  DiscoverCompressOffsets(void);
        void  ProcessExpiredRecords(TimeMilli aNow);
        void  ReportResultTo(ResultCallback &aCallback) const;
        void  ConvertTo(SrvResult &aResult) const;

        SrvCache    *mNext;
        Heap::String mHostName;
        uint16_t     mPort;
        uint16_t     mPriority;
        uint16_t     mWeight;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class TxtCache : public ServiceCache, public LinkedListEntry<TxtCache>, public Heap::Allocatable<TxtCache>
    {
        friend class LinkedListEntry<TxtCache>;
        friend class Heap::Allocatable<TxtCache>;
        friend class CacheEntry;
        friend class BrowseCache;

    public:
        bool  Matches(const Name &aFullName) const;
        bool  Matches(const TxtResolver &aResolver) const;
        bool  Matches(const ServiceName &aServiceName) const;
        bool  Matches(const ExpireChecker &aExpireChecker) const;
        Error Add(const TxtResolver &aResolver);
        void  Remove(const TxtResolver &aResolver);
        void  ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset);
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        void CopyInfoTo(TxtResolver &aResolver, CacheInfo &aInfo) const;
#endif

    private:
        Error Init(Instance &aInstance, const char *aServiceInstance, const char *aServiceType);
        Error Init(Instance &aInstance, const ServiceName &aServiceName);
        Error Init(Instance &aInstance, const TxtResolver &aResolver);
        void  PrepareTxtQuestion(TxMessage &aQuery);
        void  DiscoverCompressOffsets(void);
        void  ProcessExpiredRecords(TimeMilli aNow);
        void  ReportResultTo(ResultCallback &aCallback) const;
        void  ConvertTo(TxtResult &aResult) const;

        TxtCache  *mNext;
        Heap::Data mTxtData;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class AddrCache : public CacheEntry
    {
        // Base class for `Ip6AddrCache` and `Ip4AddrCache`, tracking common info
        // shared between the two.

        friend class CacheEntry;

    public:
        bool  Matches(const Name &aFullName) const;
        bool  Matches(const char *aName) const;
        bool  Matches(const AddressResolver &aResolver) const;
        bool  Matches(const ExpireChecker &aExpireChecker) const;
        Error Add(const AddressResolver &aResolver);
        void  Remove(const AddressResolver &aResolver);
        void  CommitNewResponseEntries(void);
#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
        void CopyInfoTo(AddressResolver &aResolver, CacheInfo &aInfo) const;
#endif

    protected:
        struct AddrEntry : public LinkedListEntry<AddrEntry>, public Heap::Allocatable<AddrEntry>
        {
            explicit AddrEntry(const Ip6::Address &aAddress);
            bool     Matches(const Ip6::Address &aAddress) const { return (mAddress == aAddress); }
            bool     Matches(const ExpireChecker &aExpireChecker) const;
            bool     Matches(EmptyChecker aChecker) const;
            uint32_t GetTtl(void) const { return mRecord.GetTtl(); }

            AddrEntry      *mNext;
            Ip6::Address    mAddress;
            CacheRecordInfo mRecord;
        };

        // Called by base class `CacheEntry`
        void PrepareQueryQuestion(TxMessage &aQuery, uint16_t aRrType);
        void UpdateRecordStateAfterQuery(TimeMilli aNow);
        void DetermineRecordFireTime(void);
        void ProcessExpiredRecords(TimeMilli aNow);
        void ReportResultsTo(ResultCallback &aCallback) const;
        bool ShouldStartInitialQueries(void) const;

        Error Init(Instance &aInstance, Type aType, const char *aHostName);
        Error Init(Instance &aInstance, Type aType, const AddressResolver &aResolver);
        void  AppendNameTo(TxMessage &aTxMessage, Section aSection);
        void  ConstructResult(AddressResult &aResult, Heap::Array<AddressAndTtl> &aAddrArray) const;
        void  AddNewResponseAddress(const Ip6::Address &aAddress, uint32_t aTtl, bool aCacheFlush);

        AddrCache            *mNext;
        Heap::String          mName;
        OwningList<AddrEntry> mCommittedEntries;
        OwningList<AddrEntry> mNewEntries;
        bool                  mShouldFlush;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class Ip6AddrCache : public AddrCache, public LinkedListEntry<Ip6AddrCache>, public Heap::Allocatable<Ip6AddrCache>
    {
        friend class CacheEntry;
        friend class LinkedListEntry<Ip6AddrCache>;
        friend class Heap::Allocatable<Ip6AddrCache>;

    public:
        void ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset);

    private:
        Error Init(Instance &aInstance, const char *aHostName);
        Error Init(Instance &aInstance, const AddressResolver &aResolver);
        void  PrepareAaaaQuestion(TxMessage &aQuery);
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    class Ip4AddrCache : public AddrCache, public LinkedListEntry<Ip4AddrCache>, public Heap::Allocatable<Ip4AddrCache>
    {
        friend class CacheEntry;
        friend class LinkedListEntry<Ip4AddrCache>;
        friend class Heap::Allocatable<Ip4AddrCache>;

    public:
        void ProcessResponseRecord(const Message &aMessage, uint16_t aRecordOffset);

    private:
        Error Init(Instance &aInstance, const char *aHostName);
        Error Init(Instance &aInstance, const AddressResolver &aResolver);
        void  PrepareAQuestion(TxMessage &aQuery);
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

    class EntryIterator : public Iterator, public InstanceLocator, public Heap::Allocatable<EntryIterator>
    {
        friend class Heap::Allocatable<EntryIterator>;
        friend class ServiceEntry;

    public:
        Error GetNextHost(Host &aHost, EntryState &aState);
        Error GetNextService(Service &aService, EntryState &aState);
        Error GetNextKey(Key &aKey, EntryState &aState);
        Error GetNextBrowser(Browser &aBrowser, CacheInfo &aInfo);
        Error GetNextSrvResolver(SrvResolver &aResolver, CacheInfo &aInfo);
        Error GetNextTxtResolver(TxtResolver &aResolver, CacheInfo &aInfo);
        Error GetNextIp6AddressResolver(AddressResolver &aResolver, CacheInfo &aInfo);
        Error GetNextIp4AddressResolver(AddressResolver &aResolver, CacheInfo &aInfo);

    private:
        static constexpr uint16_t kArrayCapacityIncrement = 32;

        enum Type : uint8_t
        {
            kUnspecified,
            kHost,
            kService,
            kHostKey,
            kServiceKey,
            kBrowser,
            kSrvResolver,
            kTxtResolver,
            kIp6AddrResolver,
            kIp4AddrResolver,
        };

        explicit EntryIterator(Instance &aInstance);

        Type mType;

        union
        {
            const HostEntry    *mHostEntry;
            const ServiceEntry *mServiceEntry;
            const BrowseCache  *mBrowseCache;
            const SrvCache     *mSrvCache;
            const TxtCache     *mTxtCache;
            const Ip6AddrCache *mIp6AddrCache;
            const Ip4AddrCache *mIp4AddrCache;
        };

        Heap::Array<const char *, kArrayCapacityIncrement> mSubTypeArray;
    };

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    template <typename EntryType> OwningList<EntryType> &GetEntryList(void);
    template <typename EntryType, typename ItemInfo>
    Error Register(const ItemInfo &aItemInfo, RequestId aRequestId, RegisterCallback aCallback);
    template <typename EntryType, typename ItemInfo> Error Unregister(const ItemInfo &aItemInfo);

    template <typename CacheType> OwningList<CacheType> &GetCacheList(void);
    template <typename CacheType, typename BrowserResolverType>
    Error Start(const BrowserResolverType &aBrowserOrResolver);
    template <typename CacheType, typename BrowserResolverType>
    Error Stop(const BrowserResolverType &aBrowserOrResolver);

    void      InvokeConflictCallback(const char *aName, const char *aServiceType);
    void      HandleMessage(Message &aMessage, bool aIsUnicast, const AddressInfo &aSenderAddress);
    void      AddPassiveSrvTxtCache(const char *aServiceInstance, const char *aServiceType);
    void      AddPassiveIp6AddrCache(const char *aHostName);
    TimeMilli RandomizeFirstProbeTxTime(void);
    TimeMilli RandomizeInitialQueryTxTime(void);
    void      RemoveEmptyEntries(void);
    void      HandleEntryTimer(void);
    void      HandleEntryTask(void);
    void      HandleCacheTimer(void);
    void      HandleCacheTask(void);

    static bool     IsKeyForService(const Key &aKey) { return aKey.mServiceType != nullptr; }
    static uint32_t DetermineTtl(uint32_t aTtl, uint32_t aDefaultTtl);
    static bool     NameMatch(const Heap::String &aHeapString, const char *aName);
    static bool     NameMatch(const Heap::String &aFirst, const Heap::String &aSecond);
    static void     UpdateCacheFlushFlagIn(ResourceRecord &aResourceRecord,
                                           Section         aSection,
                                           bool            aIsLegacyUnicast = false);
    static void     UpdateRecordLengthInMessage(ResourceRecord &aRecord, Message &aMessage, uint16_t aOffset);
    static void     UpdateCompressOffset(uint16_t &aOffset, uint16_t aNewOffse);
    static bool     QuestionMatches(uint16_t aQuestionRrType, uint16_t aRrType);
    static bool     RrClassIsInternetOrAny(uint16_t aRrClass);

    using EntryTimer = TimerMilliIn<Core, &Core::HandleEntryTimer>;
    using CacheTimer = TimerMilliIn<Core, &Core::HandleCacheTimer>;
    using EntryTask  = TaskletIn<Core, &Core::HandleEntryTask>;
    using CacheTask  = TaskletIn<Core, &Core::HandleCacheTask>;

    static const char kLocalDomain[];         // "local."
    static const char kUdpServiceLabel[];     // "_udp"
    static const char kTcpServiceLabel[];     // "_tcp"
    static const char kSubServiceLabel[];     // "_sub"
    static const char kServicesDnssdLabels[]; // "_services._dns-sd._udp"

    bool                     mIsEnabled;
    bool                     mIsQuestionUnicastAllowed;
    uint16_t                 mMaxMessageSize;
    uint32_t                 mInfraIfIndex;
    OwningList<HostEntry>    mHostEntries;
    OwningList<ServiceEntry> mServiceEntries;
    OwningList<ServiceType>  mServiceTypes;
    MultiPacketRxMessages    mMultiPacketRxMessages;
    TimeMilli                mNextProbeTxTime;
    EntryTimer               mEntryTimer;
    EntryTask                mEntryTask;
    TxMessageHistory         mTxMessageHistory;
    ConflictCallback         mConflictCallback;

    OwningList<BrowseCache>  mBrowseCacheList;
    OwningList<SrvCache>     mSrvCacheList;
    OwningList<TxtCache>     mTxtCacheList;
    OwningList<Ip6AddrCache> mIp6AddrCacheList;
    OwningList<Ip4AddrCache> mIp4AddrCacheList;
    TimeMilli                mNextQueryTxTime;
    CacheTimer               mCacheTimer;
    CacheTask                mCacheTask;
};

// Specializations of `Core::GetEntryList()` for `HostEntry` and `ServiceEntry`:

template <> inline OwningList<Core::HostEntry> &Core::GetEntryList<Core::HostEntry>(void) { return mHostEntries; }

template <> inline OwningList<Core::ServiceEntry> &Core::GetEntryList<Core::ServiceEntry>(void)
{
    return mServiceEntries;
}

// Specializations of `Core::GetCacheList()`:

template <> inline OwningList<Core::BrowseCache> &Core::GetCacheList<Core::BrowseCache>(void)
{
    return mBrowseCacheList;
}

template <> inline OwningList<Core::SrvCache> &Core::GetCacheList<Core::SrvCache>(void) { return mSrvCacheList; }

template <> inline OwningList<Core::TxtCache> &Core::GetCacheList<Core::TxtCache>(void) { return mTxtCacheList; }

template <> inline OwningList<Core::Ip6AddrCache> &Core::GetCacheList<Core::Ip6AddrCache>(void)
{
    return mIp6AddrCacheList;
}

template <> inline OwningList<Core::Ip4AddrCache> &Core::GetCacheList<Core::Ip4AddrCache>(void)
{
    return mIp4AddrCacheList;
}

} // namespace Multicast
} // namespace Dns

DefineCoreType(otPlatMdnsAddressInfo, Dns::Multicast::Core::AddressInfo);

} // namespace ot

#endif // OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#endif // MULTICAST_DNS_HPP_
