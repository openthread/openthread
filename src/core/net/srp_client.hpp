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

#ifndef SRP_CLIENT_HPP_
#define SRP_CLIENT_HPP_

#include "openthread-core-config.h"

#include <openthread/srp_client.h>

#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "crypto/ecdsa.hpp"
#include "net/dns_headers.hpp"
#include "net/ip6.hpp"
#include "net/udp6.hpp"

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

/**
 * @file
 *   This file includes definitions for the SRP (Service Registration Protocol) client.
 */

namespace ot {
namespace Srp {

#if !OPENTHREAD_CONFIG_ECDSA_ENABLE
#error "SRP Client feature requires ECDSA support (OPENTHREAD_CONFIG_ECDSA_ENABLE)."
#endif

/**
 * This class implements SRP client.
 *
 */
class Client : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    /**
     * This enumeration types represents an SRP client item (service or host info) state.
     *
     */
    enum ItemState : uint8_t
    {
        kToAdd      = OT_SRP_CLIENT_ITEM_STATE_TO_ADD,     ///< Item to be added/registered.
        kAdding     = OT_SRP_CLIENT_ITEM_STATE_ADDING,     ///< Item is being added/registered.
        kToRefresh  = OT_SRP_CLIENT_ITEM_STATE_TO_REFRESH, ///< Item to be refreshed (renew lease).
        kRefreshing = OT_SRP_CLIENT_ITEM_STATE_REFRESHING, ///< Item is being refreshed.
        kToRemove   = OT_SRP_CLIENT_ITEM_STATE_TO_REMOVE,  ///< Item to be removed.
        kRemoving   = OT_SRP_CLIENT_ITEM_STATE_REMOVING,   ///< Item is being removed.
        kRegistered = OT_SRP_CLIENT_ITEM_STATE_REGISTERED, ///< Item is registered with server.
        kRemoved    = OT_SRP_CLIENT_ITEM_STATE_REMOVED,    ///< Item is removed.
    };

    /**
     * This function pointer type defines the callback used by SRP client to notify user of a changes/events/errors.
     *
     * Please see `otSrpClientCallback` for more details.
     *
     */
    typedef otSrpClientCallback Callback;

    /**
     * This type represents an SRP client host info.
     *
     */
    class HostInfo : public otSrpClientHostInfo, public Clearable<HostInfo>
    {
        friend class Client;

    public:
        /**
         * This method initializes the `HostInfo` object.
         *
         */
        void Init(void);

        /**
         * This method clears the `HostInfo` object.
         *
         */
        void Clear(void);

        /**
         * This method gets the host name (label) string.
         *
         * @returns The host name (label) string, or nullptr if not yet set.
         *
         */
        const char *GetName(void) const { return mName; }

        /**
         * This method gets the number of host IPv6 addresses.
         *
         * @returns The number of host IPv6 addresses.
         *
         */
        uint8_t GetNumAddresses(void) const { return mNumAddresses; }

        /**
         * This method gets the host IPv6 address at a given index.
         *
         * @param[in] aIndex  The index to get (MUST be smaller than `GetNumAddresses()`).
         *
         * @returns  The host IPv6 address at index @p aIndex.
         *
         */
        const Ip6::Address &GetAddress(uint8_t aIndex) const
        {
            return static_cast<const Ip6::Address &>(mAddresses[aIndex]);
        }

        /**
         * This method gets the state of `HostInfo`.
         *
         * @returns The `HostInfo` state.
         *
         */
        ItemState GetState(void) const { return static_cast<ItemState>(mState); }

    private:
        void SetName(const char *aName) { mName = aName; }
        void SetState(ItemState aState);
        void SetAddresses(const Ip6::Address *aAddresses, uint8_t aNumAddresses);
    };

    /**
     * This type represents an SRP client service.
     *
     */
    class Service : public otSrpClientService, public LinkedListEntry<Service>
    {
        friend class Client;

    public:
        /**
         * This method initializes and validates the `Service` object and its fields.
         *
         * @retval OT_ERROR_NONE          Successfully initialized and validated the `Service` object.
         * @retval OT_ERROR_INVALID_ARGS  The info in `Service` object is not valid (e.g. null name or bad `TxtEntry`).
         *
         */
        otError Init(void);

        /**
         * This method gets the service name labels string.
         *
         * @returns The service name label string (e.g., "_chip._udp", not the full domain name).
         *
         */
        const char *GetName(void) const { return mName; }

        /**
         * This method gets the service instance name label (not the full name).
         *
         * @returns The service instance name label string.
         *
         */
        const char *GetInstanceName(void) const { return mInstanceName; }

        /**
         * This method gets the service port number.
         *
         * @returns The service port number.
         *
         */
        uint16_t GetPort(void) const { return mPort; }

        /**
         * This method gets the service priority.
         *
         * @returns The service priority.
         *
         */
        uint16_t GetPriority(void) const { return mPriority; }

        /**
         * This method gets the service weight.
         *
         * @returns The service weight.
         *
         */
        uint16_t GetWeight(void) const { return mWeight; }

        /**
         * This method gets the array of service TXT entries.
         *
         * @returns A pointer to an array of service TXT entries.
         *
         */
        const Dns::TxtEntry *GetTxtEntries(void) const { return static_cast<const Dns::TxtEntry *>(mTxtEntries); }

        /**
         * This method gets the number of entries in the service TXT entry array.
         *
         * @returns The number of entries in the service TXT entry array.
         *
         */
        uint8_t GetNumTxtEntries(void) const { return mNumTxtEntries; }

        /**
         * This method get the state of service.
         *
         * @returns The service state.
         *
         */
        ItemState GetState(void) const { return static_cast<ItemState>(mState); }

    private:
        void      SetState(ItemState aState);
        TimeMilli GetLeaseRenewTime(void) const { return TimeMilli(mData); }
        void      SetLeaseRenewTime(TimeMilli aTime) { mData = aTime.GetValue(); }
    };

    /**
     * This constructor initializes the SRP `Client` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit Client(Instance &aInstance);

    /**
     * This method starts the SRP client operation.
     *
     * SRP client will prepare and send "SRP Update" message to the SRP server once all the following conditions are
     * met:
     *
     *  - The SRP client is started - `Start()` is called
     *  - Host name is set - `SetHostName()` is called.
     *  - At least one host IPv6 address is set - `SetHostAddresses()` is called.
     *  - At least one service is added - `AddService()` is called.
     *
     * It does not matter in which order these methods are called. When all conditions are met, the SRP client will
     * wait for a short delay before preparing an "SRP Update" message and sending it to server. This delay allows for
     * user to add multiple services and/or IPv6 addresses before the first SRP Update message is sent (ensuring a
     * single SRP Update is sent containing all the info).
     *
     * @param[in] aServerSockAddr  The socket address (IPv6 address and port number) of the SRP server.
     *
     * @retval OT_ERROR_NONE     SRP client operation started successfully or it is already running with same server
     *                           socket address and callback.
     * @retval OT_ERROR_BUSY     SRP client is busy running with a different socket address.
     * @retval OT_ERROR_FAILED   Failed to open/connect the client's UDP socket.
     *
     */
    otError Start(const Ip6::SockAddr &aServerSockAddr);

    /**
     * This method stops the SRP client operation.
     *
     * This method stops any further interactions with the SRP server. Note that it does not remove or clear host info
     * and/or list of services. It marks all services to be added/removed again once the client is started again.
     *
     */
    void Stop(void);

    /**
     * This method indicates whether the SRP client is running or not.
     *
     * @returns TRUE if the SRP client is running, FALSE otherwise.
     *
     */
    bool IsRunning(void) const { return (mState != kStateStopped); }

    /**
     * This method gets the socket address (IPv6 address and port number) of the SRP server which is being used by SRP
     * client.
     *
     * If the client is not running, the address is unspecified (all zero) with zero port number.
     *
     * @returns The SRP server's socket address.
     *
     */
    const Ip6::SockAddr &GetServerAddress(void) const { return mSocket.GetPeerName(); }

    /**
     * This method sets the callback used to notify caller of events/changes.
     *
     * The SRP client allows a single callback to be registered. So consecutive calls to this method will overwrite any
     * previously set callback functions.
     *
     * @param[in] aCallback        The callback to notify of events and changes. Can be nullptr if not needed.
     * @param[in] aContext         An arbitrary context used with @p aCallback.
     *
     */
    void SetCallback(Callback aCallback, void *aContext);

    /**
     * This method gets the lease interval used in SRP update requests.
     *
     * Note that this is lease duration that would be requested by the SRP client. Server may choose to accept a
     * different lease interval.
     *
     * @returns The lease interval (in seconds).
     *
     */
    uint32_t GetLeaseInterval(void) const { return mLeaseInterval; }

    /**
     * This method sets the lease interval used in SRP update requests.
     *
     * Changing the lease interval does not impact the accepted lease interval of already registered services/host-info.
     * It only changes any future SRP update messages (i.e adding new services and/or refreshes of existing services).
     *
     * @param[in] The lease interval (in seconds). If zero, the default value `kDefaultLease` would be used.
     *
     */
    void SetLeaseInterval(uint32_t aInterval) { mLeaseInterval = GetBoundedLeaseInterval(aInterval, kDefaultLease); }

    /**
     * This method gets the key lease interval used in SRP update requests.
     *
     * @returns The key lease interval (in seconds).
     *
     */
    uint32_t GetKeyLeaseInterval(void) const { return mKeyLeaseInterval; }

    /**
     * This method sets the key lease interval used in SRP update requests.
     *
     * Changing the lease interval does not impact the accepted lease interval of already registered services/host-info.
     * It only changes any future SRP update messages (i.e adding new services and/or refreshes of existing services).
     *
     * @param[in] The key lease interval (in seconds). If zero, the default value `kDefaultKeyLease` would be used.
     *
     */
    void SetKeyLeaseInterval(uint32_t aInterval)
    {
        mKeyLeaseInterval = GetBoundedLeaseInterval(aInterval, kDefaultKeyLease);
    }

    /**
     * This method gets the host info.
     *
     * @returns A reference to host info structure.
     *
     */
    const HostInfo &GetHostInfo(void) const { return mHostInfo; }

    /**
     * This function sets the host name label.
     *
     * After a successful call to this function, `Callback` will be called to report the status of host info
     *  registration with SRP server.
     *
     * The host name can be set before client is started or after start but before host info is registered with server
     * (host info should be in either `kToAdd` or `kRemoved`).
     *
     * @param[in] aName       A pointer to host name label string (MUST NOT be NULL). Pointer the string buffer MUST
     *                        persist and remain valid and constant after return from this function.
     *
     * @retval OT_ERROR_NONE            The host name label was set successfully.
     * @retval OT_ERROR_INVALID_ARGS    The @p aName is NULL.
     * @retval OT_ERROR_INVALID_STATE   The host name is already set and registered with the server.
     *
     */
    otError SetHostName(const char *aName);

    /**
     * This method sets/updates the list of host IPv6 address.
     *
     * Host IPv6 addresses can be set/changed before start or even during operation of SRP client (e.g. to add/remove
     * or change a previously registered host address), except when the host info is being removed (client is busy
     * handling a remove request from an earlier call to `RemoveHostAndServices()` and host info still being in either
     * `kStateToRemove` or `kStateRemoving` states).
     *
     * After a successful call to this method, `Callback` will be called to report the status of the address
     * registration with SRP server.
     *
     * @param[in] aAddresses          A pointer to the an array containing the host IPv6 addresses.
     * @param[in] aNumAddresses       The number of addresses in the @p aAddresses array.
     *
     * @retval OT_ERROR_NONE            The host IPv6 address list change started successfully. The `Callback`
     *                                  will be called to report the status of registering addresses with server.
     * @retval OT_ERROR_INVALID_ARGS    The address list is invalid (e.g., must contain at least one address).
     * @retval OT_ERROR_INVALID_STATE   Host is being removed and therefore cannot change host address.
     *
     */
    otError SetHostAddresses(const Ip6::Address *aAddresses, uint8_t aNumAddresses);

    /**
     * This method adds a service to be registered with server.
     *
     * After a successful call to this method, `Callback` will be called to report the status of the service
     * addition/registration with SRP server.
     *
     * @param[in] aService         A `Service` to add (the instance must persist and remain unchanged after
     *                             successful return from this method).
     *
     * @retval OT_ERROR_NONE          The addition of service started successfully. The `Callback` will be
     *                                called to report the status.
     * @retval OT_ERROR_ALREADY       The same service is already in the list.
     * @retval OT_ERROR_INVALID_ARGS  The service structure is invalid (e.g., bad service name or `TxEntry`).
     *
     */
    otError AddService(Service &aService);

    /**
     * This method removes a service to be unregistered with server.
     *
     * @param[in] aService         A `Service` to remove (the instance must persist and remain unchanged after
     *                             successful return from this method).
     *
     * @retval OT_ERROR_NONE       The removal of service started successfully. The `Callback` will be called to
     *                             report the status.
     * @retval OT_ERROR_NOT_FOUND  The service could not be found in the list.
     *
     */

    otError RemoveService(Service &aService);

    /**
     * This method gets the list of services being managed by client.
     *
     * @returns The list of services.
     *
     */
    const LinkedList<Service> &GetServices(void) const { return mServices; }

    /**
     * This method starts the remove process of the host info and all services.
     *
     * After retuning from this method, `Callback` will be called to report the status of remove request with
     * SRP server.
     *
     * If the host info is to be permanently removed from server, @p aRemoveKeyLease should be set to `true` which
     * removes the key lease associated with host on server. Otherwise, the key lease record is kept as before, which
     * ensures that the server holds the host name in reserve for when the client once again able to provide and
     * register its service(s).
     *
     * @param[in] aRemoveKeyLease  A boolean indicating whether or not the host key lease should also be removed.
     *
     * @retval OT_ERROR_NONE      The removal of host and services started successfully. The `Callback` will be called
     *                            to report the status.
     * @retval OT_ERROR_ALREADY   The host is already removed.
     *
     */
    otError RemoveHostAndServices(bool aShouldRemoveKeyLease);

    /**
     * This method clears all host info and all the services.
     *
     * Unlike `RemoveHostAndServices()` which sends an update message to server to remove/unregister all the info, this
     * method clears all the info immediately without any interaction with server.
     *
     */
    void ClearHostAndServices(void);

#if OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE
    /**
     * This method gets the domain name being used by SRP client.
     *
     * If domain name is not set, "default.service.arpa" will be used.
     *
     * @returns The domain name string.
     *
     */
    const char *GetDomainName(void) const { return mDomainName; }

    /**
     * This method sets the domain name to be used by SRP client.
     *
     * This is an optional method. If not set "default.service.arpa" will be used.
     *
     * The domain name can be set before client is started or after start but before host info is registered with server
     * (host info should be in either `kToAdd` or `kToRemove`).
     *
     * @param[in] aName      A pointer to the domain name string. If NULL sets it to default "default.service.arpa".
     *
     * @retval OT_ERROR_NONE            The domain name label was set successfully.
     * @retval OT_ERROR_INVALID_STATE   The host info is already registered with server.
     *
     */
    otError SetDomainName(const char *aName);
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE

    /**
     * This static method converts a `ItemState` to a string.
     *
     * @param[in] aState   An `ItemState`.
     *
     * @returns A string representation of @p aState.
     *
     */
    static const char *ItemStateToString(ItemState aState);

private:
    enum : uint8_t
    {
        kFastPollsAfterUpdateTx = 11, // Number of fast data polls after SRP Update tx (11x 188ms = ~2 seconds)
    };

    enum : uint16_t
    {
        kUdpPayloadSize = Ip6::Ip6::kMaxDatagramLength - sizeof(Ip6::Udp::Header), // Max UDP payload size
    };

    enum : uint32_t
    {
        // -------------------------------
        // Lease related constants

        kDefaultLease    = OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_LEASE,     // in seconds.
        kDefaultKeyLease = OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_KEY_LEASE, // in seconds.

        // The guard interval determines how much earlier (relative to
        // the lease expiration time) the client will send an update
        // to renew the lease.
        kLeaseRenewGuardInterval = OPENTHREAD_CONFIG_SRP_CLIENT_LEASE_RENEW_GUARD_INTERVAL, // in seconds.

        // Max allowed lease time to avoid timer roll-over (~24.8 days).
        kMaxLease = (Timer::kMaxDelay / 1000) - 1,

        // Opportunistic early refresh: When sending an SRP update, the
        // services that are not yet expired but are close, are allowed
        // to refresh early and are included in the SRP update. This
        // helps place more services on the same lease refresh schedule
        // reducing number of messages sent to the SRP server. The
        // "early lease renewal interval" is used to determine if a
        // service can renew early. The interval is calculated by
        // multiplying the accepted lease interval by the"early lease
        // renewal factor" which is given as a fraction (numerator and
        // denominator).
        //
        // If the factor is set to zero (numerator=0, denominator=1),
        // the opportunistic early refresh behavior is disabled. If
        // denominator is set to zero (the factor is set to infinity),
        // then all services (including previously registered ones)
        // are always included in SRP update message.

        kEarlyLeaseRenewFactorNumerator   = OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_NUMERATOR,
        kEarlyLeaseRenewFactorDenominator = OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_DENOMINATOR,

        // -------------------------------
        // When there is a change (e.g., a new service is added/removed)
        // that requires an update, the SRP client will wait for a short
        // delay as specified by `kUpdateTxDelay` before sending an SRP
        // update to server. This allows the user to provide more change
        // that are then all sent in same update message.
        kUpdateTxDelay = OPENTHREAD_CONFIG_SRP_CLIENT_UPDATE_TX_DELAY, // in msec.

        // -------------------------------
        // Retry related constants
        //
        // If the preparation or transmission of an SRP update message
        // fails (e.g., no buffer to allocate the message), SRP client
        // will retry after a short interval `kTxFailureRetryInterval`
        // up to `kMaxTxFailureRetries` attempts. After this, the retry
        // wait interval will be used (which keeps growing on each failure
        // - please see bellow).
        //
        // If the update message is sent successfully but there is no
        // response from server or if server rejects the update, the
        // client will retransmit the update message after some wait
        // interval. The wait interval starts from the minimum value and
        // is increased by the growth factor on back-to-back failures up
        // to the max value. The growth factor is given as a fraction
        // (e.g., for 1.5, we can use 15 as the numerator and 10 as the
        // denominator). A random jitter is added to the retry interval.
        // If the current wait interval value is smaller than the jitter
        // interval, then wait interval value itself is used as the
        // jitter value. For example, with jitter interval of 2 seconds
        // if the current retry interval is 800ms, then a random wait
        // interval in [0,2*800] ms will be used.

        kTxFailureRetryInterval               = 250, // in ms
        kMaxTxFailureRetries                  = 8,   // num of quick retries after tx failure
        kMinRetryWaitInterval                 = OPENTHREAD_CONFIG_SRP_CLIENT_MIN_RETRY_WAIT_INTERVAL, // in ms
        kMaxRetryWaitInterval                 = OPENTHREAD_CONFIG_SRP_CLIENT_MAX_RETRY_WAIT_INTERVAL, // in ms
        kRetryIntervalGrowthFactorNumerator   = OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_NUMERATOR,
        kRetryIntervalGrowthFactorDenominator = OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_DENOMINATOR,
    };

    enum : uint16_t
    {
        kTxFailureRetryJitter = 10,                                                      // in ms
        kRetryIntervalJitter  = OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_WAIT_INTERVAL_JITTER, // in ms
    };

    static_assert(kDefaultLease <= static_cast<uint32_t>(kMaxLease), "kDefaultLease is larger than max");
    static_assert(kDefaultKeyLease <= static_cast<uint32_t>(kMaxLease), "kDefaultKeyLease is larger than max");

    enum State : uint8_t
    {
        kStateStopped,  // Client is stopped.
        kStatePaused,   // Client is paused (due to device being detached).
        kStateToUpdate, // Waiting to send SRP update
        kStateUpdating, // SRP update is sent, waiting for response from server.
        kStateUpdated,  // SRP update response received from server.
        kStateToRetry,  // SRP update tx failed, waiting to retry.
    };

    struct Info : public Clearable<Info>
    {
        enum : uint16_t
        {
            kUnknownOffset = 0, // Unknown offset value (used when offset is not yet set).
        };

        uint16_t                     mDomainNameOffset; // Offset of domain name serialization
        uint16_t                     mHostNameOffset;   // Offset of host name serialization.
        uint16_t                     mRecordCount;      // Number of resource records in Update section.
        Crypto::Ecdsa::P256::KeyPair mKeyPair;          // The ECDSA key pair.
    };

    void           Resume(void);
    void           Pause(void);
    void           HandleNotifierEvents(Events aEvents);
    void           UpdateServiceStateToRemove(Service &aService);
    State          GetState(void) const { return mState; }
    void           SetState(State aState);
    void           ChangeHostAndServiceStates(const ItemState *aNewStates);
    void           InvokeCallback(otError aError) const;
    void           InvokeCallback(otError aError, const HostInfo &aHostInfo, const Service *aRemovedServices) const;
    void           ClearHostInfoAndServices(void);
    void           HandleHostInfoOrServiceChange(void);
    void           SendUpdate(void);
    otError        PrepareUpdateMessage(Message &aMessage);
    otError        ReadOrGenerateKey(Crypto::Ecdsa::P256::KeyPair &aKeyPair);
    otError        AppendServiceInstructions(Service &aService, Message &aMessage, Info &aInfo);
    otError        AppendHostDescriptionInstruction(Message &aMessage, Info &aInfo) const;
    otError        AppendDeleteAllRrsets(Message &aMessage) const;
    otError        AppendHostName(Message &aMessage, Info &aInfo, bool aDoNotCompress = false) const;
    otError        AppendUpdateLeaseOptRecord(Message &aMessage) const;
    otError        AppendSignature(Message &aMessage, Info &aInfo);
    void           UpdateRecordLengthInMessage(Dns::ResourceRecord &aRecord, uint16_t aOffset, Message &aMessage) const;
    static void    HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void           ProcessResponse(Message &aMessage);
    void           HandleUpdateDone(void);
    void           GetRemovedServices(LinkedList<Service> &aRemovedServices);
    static otError ReadResourceRecord(const Message &aMessage, uint16_t &aOffset, Dns::ResourceRecord &aRecord);
    otError        ProcessOptRecord(const Message &aMessage, uint16_t aOffset, const Dns::OptRecord &aOptRecord);
    void           UpdateState(void);
    uint32_t       GetRetryWaitInterval(void) const { return mRetryWaitInterval; }
    void           ResetRetryWaitInterval(void) { mRetryWaitInterval = kMinRetryWaitInterval; }
    void           GrowRetryWaitInterval(void);
    uint32_t       GetBoundedLeaseInterval(uint32_t aInterval, uint32_t aDefaultInterval) const;
    bool           ShouldRenewEarly(const Service &aService) const;
    static void    HandleTimer(Timer &aTimer);
    void           HandleTimer(void);

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_SRP == 1)
    static const char *StateToString(State aState);
    void               LogRetryWaitInterval(void) const;
#else
    void LogRetryWaitInterval(void) const {}
#endif

    static const char kDefaultDomainName[];

    static_assert(kMaxTxFailureRetries < 128, "kMaxTxFailureRetries exceed the range of mTxFailureRetryCount (7-bit)");

    State   mState;
    uint8_t mTxFailureRetryCount : 7;
    bool    mShouldRemoveKeyLease : 1;

    uint16_t mUpdateMessageId;
    uint32_t mRetryWaitInterval;

    TimeMilli mLeaseRenewTime;
    uint32_t  mAcceptedLeaseInterval;
    uint32_t  mLeaseInterval;
    uint32_t  mKeyLeaseInterval;

    Ip6::Udp::Socket mSocket;

    Callback            mCallback;
    void *              mCallbackContext;
    const char *        mDomainName;
    HostInfo            mHostInfo;
    LinkedList<Service> mServices;
    TimerMilli          mTimer;
};

} // namespace Srp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#endif // SRP_CLIENT_HPP_
