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
 *   This file includes definitions for infrastructure DNS-SD (mDNS) platform.
 */

#ifndef DNSSD_HPP_
#define DNSSD_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE

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
 *   This module includes definitions for DNS-SD (mDNS) platform.
 *
 * @{
 *
 */

extern "C" void otPlatDnssdStateHandleStateChange(otInstance *aInstance);

/**
 * Represents DNS-SD (mDNS) platform.
 *
 */
class Dnssd : public InstanceLocator, private NonCopyable
{
    friend void otPlatDnssdStateHandleStateChange(otInstance *aInstance);

public:
    /**
     * Represents state of DNS-SD platform.
     *
     */
    enum State : uint8_t
    {
        kStopped = OT_PLAT_DNSSD_STOPPED, ///< Stopped and unable to register any service or host.
        kReady   = OT_PLAT_DNSSD_READY    ///< Running and ready to register service or host.
    };

    typedef otPlatDnssdRequestId        RequestId;        ///< A request ID.
    typedef otPlatDnssdRegisterCallback RegisterCallback; ///< The registration request callback

    class Host : public otPlatDnssdHost, public Clearable<Host> ///< Host information.
    {
    };

    class Service : public otPlatDnssdService, public Clearable<Service> ///< Service information.
    {
    };

    class Key : public otPlatDnssdKey, public Clearable<Key> ///< Key information
    {
    };

    /**
     * Represents a range of `RequestId` values.
     *
     * The range is stored using start and end ID values. The implementation handles the case when ID values roll over.
     *
     */
    struct RequestIdRange : public Clearable<RequestIdRange>
    {
        /**
         * Initializes a range as empty.
         *
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
         *
         */
        void Add(RequestId aId);

        /**
         * Removes a request ID from the range.
         *
         * @param[in] aId   The ID to remove from the range.
         *
         */
        void Remove(RequestId aId);

        /**
         * Indicates whether or not a given ID is contained within the range.
         *
         * @param[in] aId   The ID to check.
         *
         * @retval TRUE  The @p aID is contained within the range.
         * @retval FALSE The @p aId is not contained within the range.
         *
         */
        bool Contains(RequestId aId) const;

        /**
         * Indicates whether or not the range is empty.
         *
         * @retval TRUE  The range is empty.
         * @retval FALSE The range is not empty.
         *
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
     *
     */
    explicit Dnssd(Instance &aInstance);

    /**
     * Gets the current state of DNS-SD platform module.
     *
     * @returns The current state of DNS-SD platform.
     *
     */
    State GetState(void) const;

    /**
     * Indicates whether or not DNS-SD platform is ready (in `kReady` state).
     *
     * @retval TRUE   The DNS-SD platform is ready.
     * @retval FALSE  The DNS-SD platform is not ready.
     *
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
     *
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
     *
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
     *
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
     *
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
     *
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
     *
     */
    void UnregisterKey(const Key &aKey, RequestId aRequestId, RegisterCallback aCallback);

private:
    void HandleStateChange(void);
};

/**
 * @}
 *
 */

DefineMapEnum(otPlatDnssdState, Dnssd::State);
DefineCoreType(otPlatDnssdService, Dnssd::Service);
DefineCoreType(otPlatDnssdHost, Dnssd::Host);
DefineCoreType(otPlatDnssdKey, Dnssd::Key);

} // namespace ot

#endif // OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE

#endif // DNSSD_HPP_
