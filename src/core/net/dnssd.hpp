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
 *   This file includes definitions for DNS-SD module.
 */

#ifndef DNSSD_HPP_
#define DNSSD_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#if !OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
#error "Must enable either `PLATFORM_DNSSD_ENABLE` or `MULTICAST_DNS_ENABLE` and not both."
#endif
#else
#if !OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || !OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
#error "`PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION` requires both `PLATFORM_DNSSD_ENABLE` or `MULTICAST_DNS_ENABLE`.".
#endif
#endif // !OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION

#include <openthread/platform/dnssd.h>

#include "common/clearable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "net/ip6_address.hpp"

namespace ot {

/**
 * @addtogroup core-dns
 *
 * @brief
 *   This module includes definitions for DNS-SD (mDNS) APIs used by other modules in OT (e.g. advertising proxy).
 *
 *   The DNS-SD is implemented either using the native mDNS module in OpenThread or using `otPlatDnssd` platform
 *   APIs (delegating the DNS-SD implementation to platform layer).
 *
 * @{
 */

extern "C" void otPlatDnssdStateHandleStateChange(otInstance *aInstance);

/**
 * Represents DNS-SD module.
 */
class Dnssd : public InstanceLocator, private NonCopyable
{
    friend void otPlatDnssdStateHandleStateChange(otInstance *aInstance);

public:
    /**
     * Represents state of DNS-SD platform.
     */
    enum State : uint8_t
    {
        kStopped = OT_PLAT_DNSSD_STOPPED, ///< Stopped and unable to register any service or host.
        kReady   = OT_PLAT_DNSSD_READY    ///< Running and ready to register service or host.
    };

    typedef otPlatDnssdRequestId        RequestId;        ///< A request ID.
    typedef otPlatDnssdRegisterCallback RegisterCallback; ///< The registration request callback
    typedef otPlatDnssdBrowseCallback   BrowseCallback;   ///< Browser callback.
    typedef otPlatDnssdSrvCallback      SrvCallback;      ///< SRV callback.
    typedef otPlatDnssdTxtCallback      TxtCallback;      ///< TXT callback.
    typedef otPlatDnssdAddressCallback  AddressCallback;  ///< Address callback
    typedef otPlatDnssdBrowseResult     BrowseResult;     ///< Browser result.
    typedef otPlatDnssdSrvResult        SrvResult;        ///< SRV result.
    typedef otPlatDnssdTxtResult        TxtResult;        ///< TXT result.
    typedef otPlatDnssdAddressResult    AddressResult;    ///< Address result.
    typedef otPlatDnssdAddressAndTtl    AddressAndTtl;    ///< Address and TTL.

    class Host : public otPlatDnssdHost, public Clearable<Host> ///< Host information.
    {
    };

    class Service : public otPlatDnssdService, public Clearable<Service> ///< Service information.
    {
    };

    class Key : public otPlatDnssdKey, public Clearable<Key> ///< Key information
    {
    };

    class Browser : public otPlatDnssdBrowser, public Clearable<Browser> ///< Browser.
    {
    };

    class SrvResolver : public otPlatDnssdSrvResolver, public Clearable<SrvResolver> ///< SRV resolver.
    {
    };

    class TxtResolver : public otPlatDnssdTxtResolver, public Clearable<TxtResolver> ///< TXT resolver.
    {
    };

    class AddressResolver : public otPlatDnssdAddressResolver, public Clearable<AddressResolver> ///< Address resolver.
    {
    };

    /**
     * Represents a range of `RequestId` values.
     *
     * The range is stored using start and end ID values. The implementation handles the case when ID values roll over.
     */
    struct RequestIdRange : public Clearable<RequestIdRange>
    {
        /**
         * Initializes a range as empty.
         */
        RequestIdRange(void)
            : mStart(0)
            , mEnd(0)
        {
        }

        /**
         * Adds a request ID to the range.
         *
         * @param[in] aId   The ID to add to the range.
         */
        void Add(RequestId aId);

        /**
         * Removes a request ID from the range.
         *
         * @param[in] aId   The ID to remove from the range.
         */
        void Remove(RequestId aId);

        /**
         * Indicates whether or not a given ID is contained within the range.
         *
         * @param[in] aId   The ID to check.
         *
         * @retval TRUE  The @p aID is contained within the range.
         * @retval FALSE The @p aId is not contained within the range.
         */
        bool Contains(RequestId aId) const;

        /**
         * Indicates whether or not the range is empty.
         *
         * @retval TRUE  The range is empty.
         * @retval FALSE The range is not empty.
         */
        bool IsEmpty(void) const { return (mStart == mEnd); }

    private:
        // The range is represented as all `RequestId` values from
        // `mStart` up to, but not including, `mEnd`. It uses serial
        // number arithmetic logic when comparing `RequestId` values,
        // so `Contains()` and other methods work correctly even when
        // the ID value rolls over.

        RequestId mStart;
        RequestId mEnd;
    };

    /**
     * Initializes `Dnssd` object.
     *
     * @param[in]  aInstance  The OpenThread instance.
     */
    explicit Dnssd(Instance &aInstance);

    /**
     * Gets the current state of DNS-SD platform module.
     *
     * @returns The current state of DNS-SD platform.
     */
    State GetState(void) const;

    /**
     * Indicates whether or not DNS-SD platform is ready (in `kReady` state).
     *
     * @retval TRUE   The DNS-SD platform is ready.
     * @retval FALSE  The DNS-SD platform is not ready.
     */
    bool IsReady(void) const { return GetState() == kReady; }

    /**
     * Registers or updates a service on the infrastructure network's DNS-SD module.
     *
     * Refer to the documentation for `otPlatDnssdRegisterService()`, for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aService     Information about service to unregister.
     * @param[in] aRequestId   The ID associated with this request.
     * @param[in] aCallback    The callback function pointer to report the outcome (may be `nullptr`).
     */
    void RegisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Unregisters a service on the infrastructure network's DNS-SD module.
     *
     * Refer to the documentation for `otPlatDnssdUnregisterService()`, for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aService      Information about service to unregister.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (may be `nullptr`).
     */
    void UnregisterService(const Service &aService, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Registers or updates a host on the infrastructure network's DNS-SD module.
     *
     * Refer to the documentation for `otPlatDnssdRegisterHost()`, for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aHost         Information about host to register.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (may be `nullptr`).
     */
    void RegisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Unregisters a host on the infrastructure network's DNS-SD module.
     *
     * Refer to the documentation for `otPlatDnssdUnregisterHost()`, for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aHost         Information about the host to unregister.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
     */
    void UnregisterHost(const Host &aHost, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Registers or updates a key record on the infrastructure network's DNS-SD module.
     *
     * Refer to the documentation for `otPlatDnssdRegisterKey()`, for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aKey          Information about the key to register.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (may be `nullptr`).
     */
    void RegisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Unregisters a key record on the infrastructure network's DNS-SD module.
     *
     * Refer to the documentation for `otPlatDnssdUnregisterKey()`, for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aKey          Information about the key to unregister.
     * @param[in] aRequestId    The ID associated with this request.
     * @param[in] aCallback     The callback function pointer to report the outcome (may be NULL if no callback needed).
     */
    void UnregisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback);

    /**
     * Starts a service browser.
     *
     * Refer to the documentation for `otPlatDnssdStartBrowser()` for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aBrowser    The browser to be started.
     */
    void StartBrowser(const Browser &aBrowser);

    /**
     * Stops a service browser.
     *
     * Refer to the documentation for `otPlatDnssdStopBrowser()` for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aBrowser    The browser to stop.
     */
    void StopBrowser(const Browser &aBrowser);

    /**
     * Starts an SRV record resolver.
     *
     * Refer to the documentation for `otPlatDnssdStartSrvResolver()` for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aResolver    The resolver to be started.
     */
    void StartSrvResolver(const SrvResolver &aResolver);

    /**
     * Stops an SRV record resolver.
     *
     * Refer to the documentation for `otPlatDnssdStopSrvResolver()` for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aResolver    The resolver to stop.
     */
    void StopSrvResolver(const SrvResolver &aResolver);

    /**
     * Starts a TXT record resolver.
     *
     * Refer to the documentation for `otPlatDnssdStartTxtResolver()` for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aResolver    The resolver to be started.
     */
    void StartTxtResolver(const TxtResolver &aResolver);

    /**
     * Stops a TXT record resolver.
     *
     * Refer to the documentation for `otPlatDnssdStopTxtResolver()` for a more detailed description of the behavior
     * of this method.
     *
     * @param[in] aResolver    The resolver to stop.
     */
    void StopTxtResolver(const TxtResolver &aResolver);

    /**
     * Starts an IPv6 address resolver.
     *
     * Refer to the documentation for `otPlatDnssdStartIp6AddressResolver()` for a more detailed description of the
     * behavior of this method.
     *
     * @param[in] aResolver    The resolver to be started.
     */
    void StartIp6AddressResolver(const AddressResolver &aResolver);

    /**
     * Stops an IPv6 address resolver.
     *
     * Refer to the documentation for `otPlatDnssdStopIp6AddressResolver()` for a more detailed description of the
     * behavior of this method.
     *
     * @param[in] aResolver    The resolver to stop.
     */
    void StopIp6AddressResolver(const AddressResolver &aResolver);

    /**
     * Starts an IPv4 address resolver.
     *
     * Refer to the documentation for `otPlatDnssdStartIp4AddressResolver()` for a more detailed description of the
     * behavior of this method.
     *
     * @param[in] aResolver    The resolver to be started.
     */
    void StartIp4AddressResolver(const AddressResolver &aResolver);

    /**
     * Stops an IPv4 address resolver.
     *
     * Refer to the documentation for `otPlatDnssdStopIp4AddressResolver()` for a more detailed description of the
     * behavior of this method.
     *
     * @param[in] aResolver    The resolver to stop.
     */
    void StopIp4AddressResolver(const AddressResolver &aResolver);

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
    /**
     * Handles native mDNS state change.
     *
     * This is used to notify `Dnssd` when `Multicast::Dns::Core` gets enabled or disabled.
     */
    void HandleMdnsCoreStateChange(void);
#endif

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    /**
     * Selects whether to use the native mDNS or the platform `otPlatDnssd` APIs.
     *
     * @param[in] aUseMdns    TRUE to use the native mDNS module, FALSE to use platform APIs.
     */
    void SetUseNativeMdns(bool aUseMdns) { mUseNativeMdns = aUseMdns; }

    /**
     * Indicates whether the `Dnssd` is using the native mDNS or the platform `otPlatDnssd` APIs.
     *
     * @retval TRUE    `Dnssd` is using the native mDSN module.
     * @retval FALSE   `Dnssd` is using the platform `otPlatDnssd` APIs.
     */
    bool ShouldUseNativeMdns(void) const { return mUseNativeMdns; }
#endif

private:
    void HandleStateChange(void);

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ALLOW_RUN_TIME_SELECTION
    bool mUseNativeMdns;
#endif
};

/**
 * @}
 */

DefineMapEnum(otPlatDnssdState, Dnssd::State);
DefineCoreType(otPlatDnssdService, Dnssd::Service);
DefineCoreType(otPlatDnssdHost, Dnssd::Host);
DefineCoreType(otPlatDnssdKey, Dnssd::Key);

} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE || OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE

#endif // DNSSD_HPP_
