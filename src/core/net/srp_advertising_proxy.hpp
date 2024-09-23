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
 *   This file includes definitions for Advertising Proxy.
 */

#ifndef SRP_ADVERTISING_PROXY_HPP_
#define SRP_ADVERTISING_PROXY_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE

#if !OPENTHREAD_CONFIG_PLATFORM_DNSSD_ENABLE && !OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
#error "OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE requires PLATFORM_DNSSD_ENABLE or MULTICAST_DNS_ENABLE"
#endif

#if !OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
#error "OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE requires OPENTHREAD_CONFIG_SRP_SERVER_ENABLE"
#endif

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
#error "OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE requires OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE"
#endif

#include "common/clearable.hpp"
#include "common/heap_allocatable.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/owning_list.hpp"
#include "common/tasklet.hpp"
#include "common/timer.hpp"
#include "net/dnssd.hpp"
#include "net/srp_server.hpp"

namespace ot {
namespace Srp {

/**
 * Implements SRP Advertising Proxy.
 */
class AdvertisingProxy : public InstanceLocator, private NonCopyable
{
public:
    typedef Server::Host    Host;    ///< An SRP server host registration.
    typedef Server::Service Service; ///< An SRP server service registration.

    /**
     * Represents counters for Advertising Proxy.
     */
    struct Counters : public Clearable<Counters>
    {
        uint32_t mAdvTotal;           ///< Total number of advertisement requests, i.e., calls to `Advertise()`.
        uint32_t mAdvReplaced;        ///< Number of advertisements that were replaced by a newer one.
        uint32_t mAdvSkipped;         ///< Number of advertisement that were skipped (DNS-SD platform not yet ready).
        uint32_t mAdvSuccessful;      ///< Number of successful adv (all requests registered successfully).
        uint32_t mAdvRejected;        ///< Number of rejected adv (at least one request was rejected by DNS-SD plat).
        uint32_t mAdvTimeout;         ///< Number of advertisements that timed out (no response from DNS-SD platform).
        uint32_t mAdvHostRemovals;    ///< Number of host removal adv, i.e., calls to `AdvertiseRemovalOf(Host &)`
        uint32_t mAdvServiceRemovals; ///< Number of service removal adv, i.e., calls to `AdvertiseRemovalOf(Service &)`
        uint32_t mStateChanges;       ///< Number of state changes of Advertising Proxy.
    };

    /**
     * Initializes the `AdvertisingProxy` object.
     *
     * @param[in] aInstance  The OpenThread instance
     */
    explicit AdvertisingProxy(Instance &aInstance);

    /**
     * Indicates whether or not the Advertising Proxy is running.
     *
     * @retval TRUE   The Advertising Proxy is running.
     * @retval FALSE  The Advertising Proxy is not running (it is stopped).
     */
    bool IsRunning(void) const { return mState == kStateRunning; }

    /**
     * Requests advertisement of a newly received SRP Update message.
     *
     * Once advertisement is completed, `AdvertisingProxy` notifies server by invoking `Server::CommitSrpUpdate()`
     * using the same `aHost` and `aMetadata` as input parameters along with an `Error` indicating the outcome of the
     * advertisement.
     *
     * The `aHost` instance ownership is passed to `AdvertisingProxy` until it is passed back to the `Server` in the
     * `CommitSrpUpdate()` call. The call to `CommitSrpUpdate()` may happen before this method returns, for example,
     * if the proxy is not running and therefore the advertisement is skipped.
     *
     * @param[in] aHost     The `aHost` instance constructed from processing a newly received SRP Update message.
     * @param[in] aMetadata The `MessageMetadata` associated with the received SRP Update message by server.
     */
    void Advertise(Host &aHost, const Server::MessageMetadata &aMetadata);

    /**
     * Requests advertisement of removal of an already committed host and all its services, for example, due to its
     * lease expiration.
     *
     * The removal does not use any callback to notify the SRP server since the server always immediately commits the
     * removed entries.
     *
     * If there is an outstanding advertisement request (an earlier call to `Advertise()` that has not yet completed)
     * that is registering the same host name as @p aHost that is being removed, the outstanding advertisement is
     * rejected using the `kErrorAbort` error. This situation can happen if the client tries to refresh or update its
     * registration close to its lease expiration time. By rejecting any outstanding advertisements, we ensure that an
     * expired host is not re-added by mistake due to a delay in registration by the DNS-SD platform. The error is
     * passed back to the client, triggering it to retry its registration.
     *
     * @param[in] aHost  The host which is being removed.
     */
    void AdvertiseRemovalOf(Host &aHost);

    /**
     * Requests advertisement of removal of an already committed service, for example, due to its lease expiration.
     *
     * The removal does not use any callback to notify the SRP server since the server always immediately commits the
     * removed services.
     *
     * If there is an outstanding advertisement request (an earlier call to `Advertise()` that has not yet completed)
     * that is registering the same service as @p aService that is being removed, we skip the advertisement of service
     * removal (do not unregister the service on infrastructure DNS-SD). This ensures that when the outstanding
     * advertisement is completed, the service is re-added successfully (and it is still being advertised by proxy).
     * This behavior is different from `AdvertiseRemovalOf(Host &)`, where the outstanding advertisement is rejected
     * because service removals are individual, compared to when removing a host where the host and all its associated
     * services are removed.
     *
     * @param[in] aHost  The host which is being removed.
     */
    void AdvertiseRemovalOf(Service &aService);

    /**
     * Gets the set of counters.
     *
     * @returns The `AdvertisingProxy` counter.
     */
    const Counters &GetCounters(void) const { return mCounters; }

    /**
     * Resets the counters
     */
    void ResetCounters(void) { mCounters.Clear(); }

    /**
     * Gets the advertisement timeout (in msec).
     *
     * The default value of `OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_UPDATE_TIMEOUT` is used when not explicitly set.
     *
     * @returns The advertisement timeout (in msec).
     */
    uint32_t GetAdvTimeout(void) const { return mAdvTimeout; }

    /**
     * Sets the advertisement timeout.
     *
     * Changing the timeout is intended for testing purposes only. This allows tests to use a long timeout to validate
     * the behavior of `AdvertisingProxy` when new `Advertise()` requests replace entries in earlier requests.
     *
     * @param[in] aTimeout   The advertisement timeout (in msec).
     */
    void SetAdvTimeout(uint32_t aTimeout) { mAdvTimeout = Max(aTimeout, kAdvTimeout); }

    /**
     * Notifies `AdvertisingProxy` that SRP sever state changed.
     */
    void HandleServerStateChange(void) { UpdateState(); }

    /**
     * Notifies `AdvertisingProxy` that DND-SD platform state changed.
     */
    void HandleDnssdPlatformStateChange(void) { UpdateState(); }

    /**
     * Notifies `AdvertisingProxy` that `InfraIf` state changed.
     */
    void HandleInfraIfStateChanged(void) { UpdateState(); }

private:
    typedef Dnssd::RequestId RequestId;
    typedef char             DnsName[Dns::Name::kMaxNameSize];

    static constexpr RequestId kInvalidRequestId = Server::kInvalidRequestId;

    static constexpr uint32_t kAdvTimeout = OPENTHREAD_CONFIG_SRP_SERVER_SERVICE_UPDATE_TIMEOUT; // in msec

    enum State : uint8_t
    {
        kStateStopped,
        kStateRunning,
    };

    struct AdvInfo : public Heap::Allocatable<AdvInfo>, public LinkedListEntry<AdvInfo>, public GetProvider<AdvInfo>
    {
        struct CompletionChecker
        {
            // Used in `Matches()` to check if advertisement is
            // completed (successfully or failed).
        };

        struct ExpirationChecker
        {
            explicit ExpirationChecker(TimeMilli aNow)
                : mNow(aNow)
            {
            }

            TimeMilli mNow;
        };

        AdvInfo(Host &aHost, const Server::MessageMetadata &aMetadata, uint32_t aTimeout);
        void      SignalServerToCommit(void);
        bool      IsCompleted(void) const;
        bool      Matches(const CompletionChecker &) const { return IsCompleted(); }
        bool      Matches(const ExpirationChecker &aChecker) const { return (mExpireTime <= aChecker.mNow); }
        Instance &GetInstance(void) const { return mHost.GetInstance(); }

        AdvInfo                *mNext;
        AdvInfo                *mBlockingAdv;
        Host                   &mHost;
        TimeMilli               mExpireTime;
        Server::MessageMetadata mMessageMetadata;
        Ip6::MessageInfo        mMessageInfo;
        Error                   mError;
    };

    template <typename Entry> void UpdateAdvIdRangeOn(Entry &aEntry);
    template <typename Entry> bool IsRegisteredOrRegistering(const Entry &aEntry) const;
    template <typename Entry> bool IsKeyRegisteredOrRegistering(const Entry &aEntry) const;
    template <typename Entry> void DecideToAdvertise(Entry &aEntry, bool aUnregisterEntry, bool aUnregisterKey);
    template <typename Entry> void UpdateKeyRegistrationStatus(Entry &aEntry, const Entry &aExistingEntry);
    template <typename Entry> bool CompareAndUpdate(Entry &aEntry, Entry &aExistingEntry);
    template <typename Entry> bool EntriesMatch(const Entry &aFirstEntry, const Entry &aSecondEntry);

    void        Start(void);
    void        Stop(void);
    void        UpdateState(void);
    RequestId   AllocateNextRequestId(void);
    void        Advertise(Host &aHost);
    void        UnregisterHostAndItsServicesAndKeys(Host &aHost);
    bool        CompareAndUpdateHostAndServices(Host &aHost, Host &aExistingHost);
    bool        CompareAndUpdateHost(Host &aHost, Host &aExistingHost);
    bool        CompareAndUpdateService(Service &aService, Service &aExistingService);
    void        RegisterHost(Host &aHost);
    void        UnregisterHost(Host &aHost);
    void        RegisterService(Service &aService);
    void        UnregisterService(Service &aService);
    void        RegisterKey(Host &aHost);
    void        RegisterKey(Service &aService);
    void        RegisterKey(const char      *aName,
                            const char      *aServiceType,
                            const Host::Key &aKey,
                            RequestId        aRequestId,
                            uint32_t         aTtl);
    void        UnregisterKey(Service &aService);
    void        UnregisterKey(Host &aHost);
    void        UnregisterKey(const char *aName, const char *aServiceType);
    void        CopyNameAndRemoveDomain(DnsName &aName, const char *aFullName);
    static void HandleRegistered(otInstance *aInstance, otPlatDnssdRequestId aRequestId, otError aError);
    void        HandleRegistered(RequestId aRequestId, Error aError);
    bool        HandleRegisteredRequestIdOn(Host &aHost, RequestId aRequestId, Error aError);
    void        HandleTimer(void);
    void        HandleTasklet(void);
    void        SignalAdvCompleted(AdvInfo &aAdvInfo);

    using AdvTimer   = TimerMilliIn<AdvertisingProxy, &AdvertisingProxy::HandleTimer>;
    using AdvTasklet = TaskletIn<AdvertisingProxy, &AdvertisingProxy::HandleTasklet>;

    State               mState;
    RequestId           mCurrentRequestId;
    uint32_t            mAdvTimeout;
    OwningList<AdvInfo> mAdvInfoList;
    AdvTimer            mTimer;
    AdvTasklet          mTasklet;
    Counters            mCounters;
};

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ADVERTISING_PROXY_ENABLE

#endif // SRP_ADVERTISING_PROXY_HPP_
