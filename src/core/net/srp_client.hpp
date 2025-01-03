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

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#include <openthread/srp_client.h>

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/numeric_limits.hpp"
#include "common/owned_ptr.hpp"
#include "common/timer.hpp"
#include "crypto/ecdsa.hpp"
#include "net/dns_types.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"
#include "net/udp6.hpp"
#include "thread/network_data_service.hpp"

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
 * Implements SRP client.
 */
class Client : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;
    friend class ot::Ip6::Netif;

    using DnsSrpUnicastInfo = NetworkData::Service::DnsSrpUnicastInfo;
    using DnsSrpUnicastType = NetworkData::Service::DnsSrpUnicastType;
    using DnsSrpAnycastInfo = NetworkData::Service::DnsSrpAnycastInfo;

public:
    /**
     * Types represents an SRP client item (service or host info) state.
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
     * Pointer type defines the callback used by SRP client to notify user of a changes/events/errors.
     *
     * Please see `otSrpClientCallback` for more details.
     */
    typedef otSrpClientCallback ClientCallback;

    /**
     * Represents an SRP client host info.
     */
    class HostInfo : public otSrpClientHostInfo, private Clearable<HostInfo>
    {
        friend class Client;
        friend class Clearable<HostInfo>;

    public:
        /**
         * Initializes the `HostInfo` object.
         */
        void Init(void);

        /**
         * Clears the `HostInfo` object.
         */
        void Clear(void);

        /**
         * Gets the host name (label) string.
         *
         * @returns The host name (label) string, or `nullptr` if not yet set.
         */
        const char *GetName(void) const { return mName; }

        /**
         * Indicates whether or not the host auto address mode is enabled.
         *
         * @retval TRUE  If the auto address mode is enabled.
         * @retval FALSE If the auto address mode is disabled.
         */
        bool IsAutoAddressEnabled(void) const { return mAutoAddress; }

        /**
         * Gets the number of host IPv6 addresses.
         *
         * @returns The number of host IPv6 addresses.
         */
        uint8_t GetNumAddresses(void) const { return mNumAddresses; }

        /**
         * Gets the host IPv6 address at a given index.
         *
         * @param[in] aIndex  The index to get (MUST be smaller than `GetNumAddresses()`).
         *
         * @returns  The host IPv6 address at index @p aIndex.
         */
        const Ip6::Address &GetAddress(uint8_t aIndex) const { return AsCoreType(&mAddresses[aIndex]); }

        /**
         * Gets the state of `HostInfo`.
         *
         * @returns The `HostInfo` state.
         */
        ItemState GetState(void) const { return static_cast<ItemState>(mState); }

    private:
        void SetName(const char *aName) { mName = aName; }
        bool SetState(ItemState aState);
        void SetAddresses(const Ip6::Address *aAddresses, uint8_t aNumAddresses);
        void EnableAutoAddress(void);
    };

    /**
     * Represents an SRP client service.
     */
    class Service : public otSrpClientService, public LinkedListEntry<Service>
    {
        friend class Client;
        friend class LinkedList<Service>;

    public:
        /**
         * Initializes and validates the `Service` object and its fields.
         *
         * @retval kErrorNone         Successfully initialized and validated the `Service` object.
         * @retval kErrorInvalidArgs  The info in `Service` object is not valid (e.g. null name or bad `TxtEntry`).
         */
        Error Init(void);

        /**
         * Gets the service name labels string.
         *
         * @returns The service name label string (e.g., "_chip._udp", not the full domain name).
         */
        const char *GetName(void) const { return mName; }

        /**
         * Gets the service instance name label (not the full name).
         *
         * @returns The service instance name label string.
         */
        const char *GetInstanceName(void) const { return mInstanceName; }

        /**
         * Indicates whether or not the service has any subtypes.
         *
         * @retval TRUE   The service has at least one subtype.
         * @retval FALSE  The service does not have any subtype.
         */
        bool HasSubType(void) const { return (mSubTypeLabels != nullptr); }

        /**
         * Gets the subtype label at a given index.
         *
         * MUST be used only after `HasSubType()` indicates that service has a subtype.
         *
         * @param[in] aIndex  The index into list of subtype labels.
         *
         * @returns A pointer to subtype label at @p aIndex, or `nullptr` if there is no label (@p aIndex is after the
         *          end of the subtype list).
         */
        const char *GetSubTypeLabelAt(uint16_t aIndex) const { return mSubTypeLabels[aIndex]; }

        /**
         * Gets the service port number.
         *
         * @returns The service port number.
         */
        uint16_t GetPort(void) const { return mPort; }

        /**
         * Gets the service priority.
         *
         * @returns The service priority.
         */
        uint16_t GetPriority(void) const { return mPriority; }

        /**
         * Gets the service weight.
         *
         * @returns The service weight.
         */
        uint16_t GetWeight(void) const { return mWeight; }

        /**
         * Gets the array of service TXT entries.
         *
         * @returns A pointer to an array of service TXT entries.
         */
        const Dns::TxtEntry *GetTxtEntries(void) const { return AsCoreTypePtr(mTxtEntries); }

        /**
         * Gets the number of entries in the service TXT entry array.
         *
         * @returns The number of entries in the service TXT entry array.
         */
        uint8_t GetNumTxtEntries(void) const { return mNumTxtEntries; }

        /**
         * Gets the state of service.
         *
         * @returns The service state.
         */
        ItemState GetState(void) const { return static_cast<ItemState>(mState); }

        /**
         * Gets the desired lease interval to request when registering this service.
         *
         * @returns The desired lease interval in sec. Zero indicates to use default.
         */
        uint32_t GetLease(void) const { return (mLease & kLeaseMask); }

        /**
         * Gets the desired key lease interval to request when registering this service.
         *
         * @returns The desired lease interval in sec. Zero indicates to use default.
         */
        uint32_t GetKeyLease(void) const { return mKeyLease; }

    private:
        // We use the high (MSB) bit of `mLease` as flag to indicate
        // whether or not the service is appended in the message.
        // This is then used when updating the service state. Note that
        // we guarantee that `mLease` is not greater than `kMaxLease`
        // which ensures that the last bit is unused.

        static constexpr uint32_t kAppendedInMsgFlag = (1U << 31);
        static constexpr uint32_t kLeaseMask         = ~kAppendedInMsgFlag;

        bool      SetState(ItemState aState);
        TimeMilli GetLeaseRenewTime(void) const { return TimeMilli(mData); }
        void      SetLeaseRenewTime(TimeMilli aTime) { mData = aTime.GetValue(); }
        bool      IsAppendedInMessage(void) const { return mLease & kAppendedInMsgFlag; }
        void      MarkAsAppendedInMessage(void) { mLease |= kAppendedInMsgFlag; }
        void      ClearAppendedInMessageFlag(void) { mLease &= ~kAppendedInMsgFlag; }
        bool      Matches(const Service &aOther) const;
        bool      Matches(ItemState aState) const { return GetState() == aState; }
    };

    /**
     * Initializes the SRP `Client` object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit Client(Instance &aInstance);

    /**
     * Starts the SRP client operation.
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
     * @retval kErrorNone     SRP client operation started successfully or it is already running with same server
     *                        socket address and callback.
     * @retval kErrorBusy     SRP client is busy running with a different socket address.
     * @retval kErrorFailed   Failed to open/connect the client's UDP socket.
     */
    Error Start(const Ip6::SockAddr &aServerSockAddr) { return Start(aServerSockAddr, kRequesterUser); }

    /**
     * Stops the SRP client operation.
     *
     * Stops any further interactions with the SRP server. Note that it does not remove or clear host info
     * and/or list of services. It marks all services to be added/removed again once the client is started again.
     *
     * If `OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE` (auto-start feature) is enabled, a call to this method
     * also disables the auto-start mode.
     */
    void Stop(void) { Stop(kRequesterUser, kResetRetryInterval); }

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    /**
     * Pointer type defines the callback used by SRP client to notify user when it is auto-started or
     * stopped.
     */
    typedef otSrpClientAutoStartCallback AutoStartCallback;

    /**
     * Enables the auto-start mode.
     *
     * Config option `OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_DEFAULT_MODE` specifies the default auto-start mode
     * (whether it is enabled or disabled at the start of OT stack).
     *
     * When auto-start is enabled, the SRP client will monitor the Thread Network Data to discover SRP servers and
     * select the preferred server and automatically start and stop the client when an SRP server is detected.
     *
     * There are three categories of Network Data entries indicating presence of SRP sever. They are preferred in the
     * following order:
     *
     *   1) Preferred unicast entries where server address is included in the service data. If there are multiple
     *      options, the one with numerically lowest IPv6 address is preferred.
     *
     *   2) Anycast entries each having a seq number. A larger sequence number in the sense specified by Serial Number
     *      Arithmetic logic in RFC-1982 is considered more recent and therefore preferred. The largest seq number
     *      using serial number arithmetic is preferred if it is well-defined (i.e., the seq number is larger than all
     *      other seq numbers). If it is not well-defined, then the numerically largest seq number is preferred.
     *
     *   3) Unicast entries where the server address info is included in server data. If there are multiple options,
     *      the one with numerically lowest IPv6 address is preferred.
     *
     * When there is a change in the Network Data entries, client will check that the currently selected server is
     * still present in the Network Data and is still the preferred one. Otherwise the client will switch to the new
     * preferred server or stop if there is none.
     *
     * When the SRP client is explicitly started through a successful call to `Start()`, the given SRP server address
     * in `Start()` will continue to be used regardless of the state of auto-start mode and whether the same SRP
     * server address is discovered or not in the Thread Network Data. In this case, only an explicit `Stop()` call
     * will stop the client.
     *
     * @param[in] aCallback   A callback to notify when client is auto-started/stopped. Can be `nullptr` if not needed.
     * @param[in] aContext    A context to be passed when invoking @p aCallback.
     */
    void EnableAutoStartMode(AutoStartCallback aCallback, void *aContext);

    /**
     * Disables the auto-start mode.
     *
     * Disabling the auto-start mode will not stop the client if it is already running but the client stops monitoring
     * the Thread Network Data to verify that the selected SRP server is still present in it.
     *
     * Note that a call to `Stop()` will also disable the auto-start mode.
     */
    void DisableAutoStartMode(void) { mAutoStart.SetState(AutoStart::kDisabled); }

    /**
     * Indicates the current state of auto-start mode (enabled or disabled).
     *
     * @returns TRUE if the auto-start mode is enabled, FALSE otherwise.
     */
    bool IsAutoStartModeEnabled(void) const { return mAutoStart.GetState() != AutoStart::kDisabled; }

    /**
     * Indicates whether or not the current SRP server's address is selected by auto-start.
     *
     * @returns TRUE if the SRP server's address is selected by auto-start, FALSE otherwise.
     */
    bool IsServerSelectedByAutoStart(void) const { return mAutoStart.HasSelectedServer(); }
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

    /**
     * Indicates whether the SRP client is running or not.
     *
     * @returns TRUE if the SRP client is running, FALSE otherwise.
     */
    bool IsRunning(void) const { return (mState != kStateStopped); }

    /**
     * Gets the socket address (IPv6 address and port number) of the SRP server which is being used by SRP
     * client.
     *
     * If the client is not running, the address is unspecified (all zero) with zero port number.
     *
     * @returns The SRP server's socket address.
     */
    const Ip6::SockAddr &GetServerAddress(void) const { return mSocket.GetPeerName(); }

    /**
     * Sets the callback used to notify caller of events/changes.
     *
     * The SRP client allows a single callback to be registered. So consecutive calls to this method will overwrite any
     * previously set callback functions.
     *
     * @param[in] aCallback        The callback to notify of events and changes. Can be `nullptr` if not needed.
     * @param[in] aContext         An arbitrary context used with @p aCallback.
     */
    void SetCallback(ClientCallback aCallback, void *aContext) { mCallback.Set(aCallback, aContext); }

    /**
     * Gets the TTL used in SRP update requests.
     *
     * Note that this is the TTL requested by the SRP client. The server may choose to accept a different TTL.
     *
     * By default, the TTL will equal the lease interval. Passing 0 or a value larger than the lease interval via
     * `otSrpClientSetTtl()` will also cause the TTL to equal the lease interval.
     *
     * @returns The TTL (in seconds).
     */
    uint32_t GetTtl(void) const { return mTtl; }

    /**
     * Sets the TTL used in SRP update requests.
     *
     * Changing the TTL does not impact the TTL of already registered services/host-info.
     * It only changes any future SRP update messages (i.e adding new services and/or refreshes of existing services).
     *
     * @param[in] aTtl  The TTL (in seconds). If value is zero or greater than lease interval, the TTL is set to the
     *                  lease interval.
     */
    void SetTtl(uint32_t aTtl) { mTtl = aTtl; }

    /**
     * Gets the lease interval used in SRP update requests.
     *
     * Note that this is lease duration that would be requested by the SRP client. Server may choose to accept a
     * different lease interval.
     *
     * @returns The lease interval (in seconds).
     */
    uint32_t GetLeaseInterval(void) const { return mDefaultLease; }

    /**
     * Sets the lease interval used in SRP update requests.
     *
     * Changing the lease interval does not impact the accepted lease interval of already registered services/host-info.
     * It only changes any future SRP update messages (i.e adding new services and/or refreshes of existing services).
     *
     * @param[in] aInterval  The lease interval (in seconds). If zero, the default value `kDefaultLease` would be used.
     */
    void SetLeaseInterval(uint32_t aInterval) { mDefaultLease = DetermineLeaseInterval(aInterval, kDefaultLease); }

    /**
     * Gets the key lease interval used in SRP update requests.
     *
     * @returns The key lease interval (in seconds).
     */
    uint32_t GetKeyLeaseInterval(void) const { return mDefaultKeyLease; }

    /**
     * Sets the key lease interval used in SRP update requests.
     *
     * Changing the lease interval does not impact the accepted lease interval of already registered services/host-info.
     * It only changes any future SRP update messages (i.e adding new services and/or refreshes of existing services).
     *
     * @param[in] aInterval The key lease interval (in seconds). If zero, the default value `kDefaultKeyLease` would be
     *                      used.
     */
    void SetKeyLeaseInterval(uint32_t aInterval)
    {
        mDefaultKeyLease = DetermineLeaseInterval(aInterval, kDefaultKeyLease);
    }

    /**
     * Gets the host info.
     *
     * @returns A reference to host info structure.
     */
    const HostInfo &GetHostInfo(void) const { return mHostInfo; }

    /**
     * Sets the host name label.
     *
     * After a successful call to this method, `Callback` will be called to report the status of host info
     *  registration with SRP server.
     *
     * The host name can be set before client is started or after start but before host info is registered with server
     * (host info should be in either `kToAdd` or `kRemoved`).
     *
     * @param[in] aName       A pointer to host name label string (MUST NOT be NULL). Pointer the string buffer MUST
     *                        persist and remain valid and constant after return from this method.
     *
     * @retval kErrorNone           The host name label was set successfully.
     * @retval kErrorInvalidArgs    The @p aName is NULL.
     * @retval kErrorInvalidState   The host name is already set and registered with the server.
     */
    Error SetHostName(const char *aName);

    /**
     * Enables auto host address mode.
     *
     * When enabled host IPv6 addresses are automatically set by SRP client using all the unicast addresses on Thread
     * netif excluding the link-local and mesh-local addresses. If there is no valid address, then Mesh Local EID
     * address is added. The SRP client will automatically re-register when/if addresses on Thread netif are updated
     * (new addresses are added or existing addresses are removed).
     *
     * The auto host address mode can be enabled before start or during operation of SRP client except when the host
     * info is being removed (client is busy handling a remove request from an call to `RemoveHostAndServices()` and
     * host info still being in  either `kStateToRemove` or `kStateRemoving` states).
     *
     * After auto host address mode is enabled, it can be disabled by a call to `SetHostAddresses()` which then
     * explicitly sets the host addresses.
     *
     * @retval kErrorNone          Successfully enabled auto host address mode.
     * @retval kErrorInvalidState  Host is being removed and therefore cannot enable auto host address mode.
     */
    Error EnableAutoHostAddress(void);

    /**
     * Sets/updates the list of host IPv6 address.
     *
     * Host IPv6 addresses can be set/changed before start or even during operation of SRP client (e.g. to add/remove
     * or change a previously registered host address), except when the host info is being removed (client is busy
     * handling a remove request from an earlier call to `RemoveHostAndServices()` and host info still being in either
     * `kStateToRemove` or `kStateRemoving` states).
     *
     * After a successful call to this method, `Callback` will be called to report the status of the address
     * registration with SRP server.
     *
     * Calling this method disables auto host address mode if it was previously enabled from a successful call to
     * `EnableAutoHostAddress()`.
     *
     * @param[in] aAddresses          A pointer to the an array containing the host IPv6 addresses.
     * @param[in] aNumAddresses       The number of addresses in the @p aAddresses array.
     *
     * @retval kErrorNone           The host IPv6 address list change started successfully. The `Callback` will be
     *                              called to report the status of registering addresses with server.
     * @retval kErrorInvalidArgs    The address list is invalid (e.g., must contain at least one address).
     * @retval kErrorInvalidState   Host is being removed and therefore cannot change host address.
     */
    Error SetHostAddresses(const Ip6::Address *aAddresses, uint8_t aNumAddresses);

    /**
     * Adds a service to be registered with server.
     *
     * After a successful call to this method, `Callback` will be called to report the status of the service
     * addition/registration with SRP server.
     *
     * @param[in] aService         A `Service` to add (the instance must persist and remain unchanged after
     *                             successful return from this method).
     *
     * @retval kErrorNone          The addition of service started successfully. The `Callback` will be called to
     *                             report the status.
     * @retval kErrorAlready       A service with the same service and instance names is already in the list.
     * @retval kErrorInvalidArgs   The service structure is invalid (e.g., bad service name or `TxEntry`).
     */
    Error AddService(Service &aService);

    /**
     * Removes a service to be unregistered with server.
     *
     * @param[in] aService         A `Service` to remove (the instance must persist and remain unchanged after
     *                             successful return from this method).
     *
     * @retval kErrorNone      The removal of service started successfully. The `Callback` will be called to report
     *                         the status.
     * @retval kErrorNotFound  The service could not be found in the list.
     */

    Error RemoveService(Service &aService);

    /**
     * Clears a service, immediately removing it from the client service list.
     *
     * Unlike `RemoveService()` which sends an update message to the server to remove the service, this method clears
     * the service from the client's service list without any interaction with the server. On a successful call
     * to this method, the `Callback` will NOT be called and the @p aService entry can be reclaimed and re-used by the
     * caller immediately.
     *
     * @param[in] aService     A service to delete from the list.
     *
     * @retval kErrorNone      The @p aService is cleared successfully. It can be reclaimed and re-used immediately.
     * @retval kErrorNotFound  The service could not be found in the list.
     */
    Error ClearService(Service &aService);

    /**
     * Gets the list of services being managed by client.
     *
     * @returns The list of services.
     */
    const LinkedList<Service> &GetServices(void) const { return mServices; }

    /**
     * Starts the remove process of the host info and all services.
     *
     * After returning from this method, `Callback` will be called to report the status of remove request with
     * SRP server.
     *
     * If the host info is to be permanently removed from server, @p aRemoveKeyLease should be set to `true` which
     * removes the key lease associated with host on server. Otherwise, the key lease record is kept as before, which
     * ensures that the server holds the host name in reserve for when the client once again able to provide and
     * register its service(s).
     *
     * The @p aSendUnregToServer determines the behavior when the host info is not yet registered with the server. If
     * @p aSendUnregToServer is set to `false` (which is the default/expected value) then the SRP client will
     * immediately remove the host info and services without sending an update message to server (no need to update the
     * server if nothing is yet registered with it). If @p aSendUnregToServer is set to `true` then the SRP client will
     * send an update message to the server. Note that if the host info is registered then the value of
     * @p aSendUnregToServer does not matter and the SRP client will always send an update message to server requesting
     * removal of all info.
     *
     * One situation where @p aSendUnregToServer can be useful is on a device reset/reboot, caller may want to remove
     * any previously registered services with the server. In this case, caller can `SetHostName()` and then request
     * `RemoveHostAndServices()` with `aSendUnregToServer` as `true`.
     *
     * @param[in] aShouldRemoveKeyLease  A boolean indicating whether or not the host key lease should also be removed.
     * @param[in] aSendUnregToServer     A boolean indicating whether to send update to server when host info is not
     *                                   registered.
     *
     * @retval kErrorNone      The removal of host and services started successfully. The `Callback` will be called
     *                         to report the status.
     * @retval kErrorAlready   The host is already removed.
     */
    Error RemoveHostAndServices(bool aShouldRemoveKeyLease, bool aSendUnregToServer = false);

    /**
     * Clears all host info and all the services.
     *
     * Unlike `RemoveHostAndServices()` which sends an update message to the server to remove all the info, this method
     * clears all the info immediately without any interaction with the server.
     */
    void ClearHostAndServices(void);

#if OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE
    /**
     * Gets the domain name being used by SRP client.
     *
     * If domain name is not set, "default.service.arpa" will be used.
     *
     * @returns The domain name string.
     */
    const char *GetDomainName(void) const { return mDomainName; }

    /**
     * Sets the domain name to be used by SRP client.
     *
     * This is an optional method. If not set "default.service.arpa" will be used.
     *
     * The domain name can be set before client is started or after start but before host info is registered with server
     * (host info should be in either `kToAdd` or `kToRemove`).
     *
     * @param[in] aName      A pointer to the domain name string. If NULL sets it to default "default.service.arpa".
     *
     * @retval kErrorNone           The domain name label was set successfully.
     * @retval kErrorInvalidState   The host info is already registered with server.
     */
    Error SetDomainName(const char *aName);
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_DOMAIN_NAME_API_ENABLE

    /**
     * Converts a `ItemState` to a string.
     *
     * @param[in] aState   An `ItemState`.
     *
     * @returns A string representation of @p aState.
     */
    static const char *ItemStateToString(ItemState aState);

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    /**
     * Enables/disables "service key record inclusion" mode.
     *
     * When enabled, SRP client will include KEY record in Service Description Instructions in the SRP update messages
     * that it sends.
     *
     * @note KEY record is optional in Service Description Instruction (it is required and always included in the Host
     * Description Instruction). The default behavior of SRP client is to not include it. This method is added under
     * `REFERENCE_DEVICE` config and is intended to override the default behavior for testing only.
     *
     * @param[in] aEnabled   TRUE to enable, FALSE to disable the "service key record inclusion" mode.
     */
    void SetServiceKeyRecordEnabled(bool aEnabled) { mServiceKeyRecordEnabled = aEnabled; }

    /**
     * Indicates whether the "service key record inclusion" mode is enabled or disabled.
     *
     * @returns TRUE if "service key record inclusion" mode is enabled, FALSE otherwise.
     */
    bool IsServiceKeyRecordEnabled(void) const { return mServiceKeyRecordEnabled; }

    /**
     * Enables/disables "use short Update Lease Option" behavior.
     *
     * When enabled, the SRP client will use the short variant format of Update Lease Option in its message. The short
     * format only includes the lease interval.
     *
     * Is added under `REFERENCE_DEVICE` config and is intended to override the default behavior for
     * testing only.
     *
     * @param[in] aUseShort    TRUE to enable, FALSE to disable the "use short Update Lease Option" mode.
     */
    void SetUseShortLeaseOption(bool aUseShort) { mUseShortLeaseOption = aUseShort; }

    /**
     * Gets the current "use short Update Lease Option" mode.
     *
     * @returns TRUE if "use short Update Lease Option" mode is enabled, FALSE otherwise.
     */
    bool GetUseShortLeaseOption(void) const { return mUseShortLeaseOption; }

    /**
     * Set the next DNS message ID for client to use.
     *
     * This is intended for testing only.
     *
     * @pram[in] aMessageId  A message ID.
     */
    void SetNextMessageId(uint16_t aMessageId) { mNextMessageId = aMessageId; }

#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

private:
    // Number of fast data polls after SRP Update tx (11x 188ms = ~2 seconds)
    static constexpr uint8_t kFastPollsAfterUpdateTx = 11;

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    static constexpr uint32_t kSrpEcdsaKeyRef = Crypto::Storage::kEcdsaRef;
#endif

#if OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
    static constexpr uint8_t kMaxTimeoutFailuresToSwitchServer =
        OPENTHREAD_CONFIG_SRP_CLIENT_MAX_TIMEOUT_FAILURES_TO_SWITCH_SERVER;
#endif

    static constexpr uint16_t kUdpPayloadSize = Ip6::kMaxDatagramLength - sizeof(Ip6::Udp::Header);

    // -------------------------------
    // Lease related constants

    static constexpr uint32_t kDefaultLease    = OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_LEASE;     // in seconds.
    static constexpr uint32_t kDefaultKeyLease = OPENTHREAD_CONFIG_SRP_CLIENT_DEFAULT_KEY_LEASE; // in seconds.

    // The guard interval determines how much earlier (relative to
    // the lease expiration time) the client will send an update
    // to renew the lease. Value is in seconds.
    static constexpr uint32_t kLeaseRenewGuardInterval = OPENTHREAD_CONFIG_SRP_CLIENT_LEASE_RENEW_GUARD_INTERVAL;

    // Lease renew time jitter (in msec).
    static constexpr uint16_t kLeaseRenewJitter = 15 * 1000; // 15 second

    // Max allowed lease time to avoid timer roll-over (~24.8 days).
    static constexpr uint32_t kMaxLease = (Timer::kMaxDelay / 1000) - 1;

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

    static constexpr uint32_t kEarlyLeaseRenewFactorNumerator =
        OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_NUMERATOR;
    static constexpr uint32_t kEarlyLeaseRenewFactorDenominator =
        OPENTHREAD_CONFIG_SRP_CLIENT_EARLY_LEASE_RENEW_FACTOR_DENOMINATOR;

    // -------------------------------
    // TX jitter constants
    //
    // When changes trigger a new SRP update message transmission a random
    // jitter delay is applied before sending the update message to server.
    // This can occur due to changes in client services or host info,
    // or `AutoStart` selecting a server for the first time or switching
    // to a new server thus requiring re-registration.
    //
    // The constants below specify jitter ranges applied based on
    // different trigger reasons. All values are in milliseconds.
    // Also see `TxJitter` class.

    static constexpr uint32_t kMinTxJitter                  = 10;
    static constexpr uint32_t kMaxTxJitterDefault           = 500;
    static constexpr uint32_t kMaxTxJitterOnDeviceReboot    = 700;
    static constexpr uint32_t kMaxTxJitterOnServerStart     = 10 * Time::kOneSecondInMsec;
    static constexpr uint32_t kMaxTxJitterOnServerRestart   = 10 * Time::kOneSecondInMsec;
    static constexpr uint32_t kMaxTxJitterOnServerSwitch    = 10 * Time::kOneSecondInMsec;
    static constexpr uint32_t kMaxTxJitterOnSlaacAddrAdd    = 10 * Time::kOneSecondInMsec;
    static constexpr uint32_t kMaxTxJitterOnSlaacAddrRemove = 10 * Time::kOneSecondInMsec;

    static constexpr uint32_t kGuardTimeAfterAttachToUseShorterTxJitter = 1000;

    // -------------------------------
    // Retry related constants
    //
    // If the preparation or transmission of an SRP update message
    // fails (e.g., no buffer to allocate the message), SRP client
    // will retry after a short interval `kTxFailureRetryInterval`
    // up to `kMaxTxFailureRetries` attempts. After this, the retry
    // wait interval will be used (which keeps growing on each failure
    // - please see below).
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

    static constexpr uint32_t kTxFailureRetryInterval = 250; // in ms
    static constexpr uint32_t kMaxTxFailureRetries    = 8;   // num of quick retries after tx failure
    static constexpr uint32_t kMinRetryWaitInterval   = OPENTHREAD_CONFIG_SRP_CLIENT_MIN_RETRY_WAIT_INTERVAL; // in ms
    static constexpr uint32_t kMaxRetryWaitInterval   = OPENTHREAD_CONFIG_SRP_CLIENT_MAX_RETRY_WAIT_INTERVAL; // in ms
    static constexpr uint32_t kRetryIntervalGrowthFactorNumerator =
        OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_NUMERATOR;
    static constexpr uint32_t kRetryIntervalGrowthFactorDenominator =
        OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_INTERVAL_GROWTH_FACTOR_DENOMINATOR;

    static constexpr uint16_t kTxFailureRetryJitter = 10;                                                      // in ms
    static constexpr uint16_t kRetryIntervalJitter  = OPENTHREAD_CONFIG_SRP_CLIENT_RETRY_WAIT_INTERVAL_JITTER; // in ms

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

    static constexpr bool kAutoStartDefaultMode = OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_DEFAULT_MODE;
    static constexpr bool kDisallowSwitchOnRegisteredHost =
        OPENTHREAD_CONFIG_SRP_CLIENT_DISALLOW_SERVER_SWITCH_WITH_REGISTERED_HOST;

    // Port number to use when server is discovered using "network data anycast service".
    static constexpr uint16_t kAnycastServerPort = 53;

    static constexpr uint32_t kUnspecifiedInterval = 0; // Used for lease/key-lease intervals.

    // This enumeration type is used by the private `Start()` and
    // `Stop()` methods to indicate whether it is being requested by the
    // user or by the auto-start feature.
    enum Requester : uint8_t
    {
        kRequesterUser,
#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
        kRequesterAuto,
#endif
    };

    // This enumeration is used as an input to private `Stop()` to
    // indicate whether to reset the retry interval or keep it as is.
    enum StopMode : uint8_t
    {
        kResetRetryInterval,
        kKeepRetryInterval,
    };

    // Used in `ChangeHostAndServiceStates()`
    enum ServiceStateChangeMode : uint8_t
    {
        kForAllServices,
        kForServicesAppendedInMessage,
    };

#if OPENTHREAD_CONFIG_PLATFORM_KEY_REFERENCES_ENABLE
    typedef Crypto::Ecdsa::P256::KeyPairAsRef KeyInfo;
#else
    typedef Crypto::Ecdsa::P256::KeyPair KeyInfo;
#endif

    class TxJitter : public Clearable<TxJitter>
    {
        // Manages the random TX jitter to use when sending SRP update
        // messages.

    public:
        enum Reason
        {
            kOnDeviceReboot,
            kOnServerStart,
            kOnServerRestart,
            kOnServerSwitch,
            kOnSlaacAddrAdd,
            kOnSlaacAddrRemove,
        };

        TxJitter(void) { Clear(); }
        void     Request(Reason aReason);
        uint32_t DetermineDelay(void);

    private:
        static const uint32_t kMaxJitters[];
#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
        static const char *ReasonToString(Reason aReason);
#endif

        uint32_t  mRequestedMax;
        TimeMilli mRequestTime;
    };

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    class AutoStart : public Clearable<AutoStart>
    {
    public:
        enum State : uint8_t{
            kDisabled,                 // Disabled.
            kFirstTimeSelecting,       // Trying to select a server for the first time since AutoStart was enabled.
            kReselecting,              // Trying to select a server again (previously selected server was removed).
            kSelectedUnicastPreferred, // Has selected a preferred unicast entry (address in service data).
            kSelectedAnycast,          // Has selected an anycast entry with `mAnycastSeqNum`.
            kSelectedUnicast,          // Has selected a unicast entry (address in server data).
        };

        AutoStart(void);
        bool    HasSelectedServer(void) const;
        State   GetState(void) const { return mState; }
        void    SetState(State aState);
        uint8_t GetAnycastSeqNum(void) const { return mAnycastSeqNum; }
        void    SetAnycastSeqNum(uint8_t aAnycastSeqNum) { mAnycastSeqNum = aAnycastSeqNum; }
        void    SetCallback(AutoStartCallback aCallback, void *aContext) { mCallback.Set(aCallback, aContext); }
        void    InvokeCallback(const Ip6::SockAddr *aServerSockAddr) const;

#if OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
        uint8_t GetTimeoutFailureCount(void) const { return mTimeoutFailureCount; }
        void    ResetTimeoutFailureCount(void) { mTimeoutFailureCount = 0; }
        void    IncrementTimeoutFailureCount(void)
        {
            if (mTimeoutFailureCount < NumericLimits<uint8_t>::kMax)
            {
                mTimeoutFailureCount++;
            }
        }
#endif

    private:
        static constexpr bool kDefaultMode = OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_DEFAULT_MODE;

        static const char *StateToString(State aState);

        Callback<AutoStartCallback> mCallback;
        State                       mState;
        uint8_t                     mAnycastSeqNum;
#if OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
        uint8_t mTimeoutFailureCount; // Number of no-response timeout failures with the currently selected server.
#endif
    };
#endif // OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE

    struct MsgInfo
    {
        static constexpr uint16_t kUnknownOffset = 0;

        OwnedPtr<Message> mMessage;
        uint16_t          mDomainNameOffset;
        uint16_t          mHostNameOffset;
        uint16_t          mRecordCount;
        KeyInfo           mKeyInfo;
    };

    Error        Start(const Ip6::SockAddr &aServerSockAddr, Requester aRequester);
    void         Stop(Requester aRequester, StopMode aMode);
    void         Resume(void);
    void         Pause(void);
    void         HandleNotifierEvents(Events aEvents);
    void         HandleRoleChanged(void);
    void         HandleUnicastAddressEvent(Ip6::Netif::AddressEvent aEvent, const Ip6::Netif::UnicastAddress &aAddress);
    bool         ShouldUpdateHostAutoAddresses(void) const;
    bool         ShouldHostAutoAddressRegister(const Ip6::Netif::UnicastAddress &aUnicastAddress) const;
    Error        UpdateHostInfoStateOnAddressChange(void);
    void         UpdateServiceStateToRemove(Service &aService);
    State        GetState(void) const { return mState; }
    void         SetState(State aState);
    bool         ChangeHostAndServiceStates(const ItemState *aNewStates, ServiceStateChangeMode aMode);
    void         InvokeCallback(Error aError) const;
    void         InvokeCallback(Error aError, const HostInfo &aHostInfo, const Service *aRemovedServices) const;
    void         HandleHostInfoOrServiceChange(void);
    void         SendUpdate(void);
    Error        PrepareUpdateMessage(MsgInfo &aInfo);
    Error        ReadOrGenerateKey(KeyInfo &aKeyInfo);
    Error        AppendServiceInstructions(MsgInfo &aInfo);
    bool         CanAppendService(const Service &aService);
    Error        AppendServiceInstruction(Service &aService, MsgInfo &aInfo);
    Error        AppendHostDescriptionInstruction(MsgInfo &aInfo);
    Error        AppendKeyRecord(MsgInfo &aInfo) const;
    Error        AppendDeleteAllRrsets(MsgInfo &aInfo) const;
    Error        AppendHostName(MsgInfo &aInfo, bool aDoNotCompress = false) const;
    Error        AppendAaaaRecord(const Ip6::Address &aAddress, MsgInfo &aInfo) const;
    Error        AppendUpdateLeaseOptRecord(MsgInfo &aInfo);
    Error        AppendSignature(MsgInfo &aInfo);
    void         UpdateRecordLengthInMessage(Dns::ResourceRecord &aRecord, uint16_t aOffset, Message &aMessage) const;
    void         HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void         ProcessResponse(Message &aMessage);
    bool         IsResponseMessageIdValid(uint16_t aId) const;
    void         HandleUpdateDone(void);
    void         GetRemovedServices(LinkedList<Service> &aRemovedServices);
    static Error ReadResourceRecord(const Message &aMessage, uint16_t &aOffset, Dns::ResourceRecord &aRecord);
    Error        ProcessOptRecord(const Message &aMessage, uint16_t aOffset, const Dns::OptRecord &aOptRecord);
    void         UpdateState(void);
    uint32_t     GetRetryWaitInterval(void) const { return mRetryWaitInterval; }
    void         ResetRetryWaitInterval(void) { mRetryWaitInterval = kMinRetryWaitInterval; }
    void         GrowRetryWaitInterval(void);
    uint32_t     DetermineLeaseInterval(uint32_t aInterval, uint32_t aDefaultInterval) const;
    uint32_t     DetermineTtl(void) const;
    bool         ShouldRenewEarly(const Service &aService) const;
    void         HandleTimer(void);
#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    void  ApplyAutoStartGuardOnAttach(void);
    void  ProcessAutoStart(void);
    Error SelectUnicastEntry(DnsSrpUnicastType aType, DnsSrpUnicastInfo &aInfo) const;
    void  HandleGuardTimer(void) {}
#if OPENTHREAD_CONFIG_SRP_CLIENT_SWITCH_SERVER_ON_FAILURE
    void SelectNextServer(bool aDisallowSwitchOnRegisteredHost);
#endif
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    static const char *StateToString(State aState);
    void               LogRetryWaitInterval(void) const;
#else
    void                                 LogRetryWaitInterval(void) const {}
#endif

    static const char kDefaultDomainName[];

    static_assert(kMaxTxFailureRetries < 16, "kMaxTxFailureRetries exceed the range of mTxFailureRetryCount (4-bit)");

    using DelayTimer   = TimerMilliIn<Client, &Client::HandleTimer>;
    using ClientSocket = Ip6::Udp::SocketIn<Client, &Client::HandleUdpReceive>;

#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    using GuardTimer = TimerMilliIn<Client, &Client::HandleGuardTimer>;
#endif

    State   mState;
    uint8_t mTxFailureRetryCount : 4;
    bool    mShouldRemoveKeyLease : 1;
    bool    mSingleServiceMode : 1;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    bool mServiceKeyRecordEnabled : 1;
    bool mUseShortLeaseOption : 1;
#endif

    uint16_t mNextMessageId;
    uint16_t mResponseMessageId;
    uint16_t mAutoHostAddressCount;
    uint32_t mRetryWaitInterval;

    TimeMilli mLeaseRenewTime;
    uint32_t  mTtl;
    uint32_t  mLease;
    uint32_t  mKeyLease;
    uint32_t  mDefaultLease;
    uint32_t  mDefaultKeyLease;
    TxJitter  mTxJitter;

    ClientSocket mSocket;

    Callback<ClientCallback> mCallback;
    const char              *mDomainName;
    HostInfo                 mHostInfo;
    LinkedList<Service>      mServices;
    DelayTimer               mTimer;
#if OPENTHREAD_CONFIG_SRP_CLIENT_AUTO_START_API_ENABLE
    GuardTimer mGuardTimer;
    AutoStart  mAutoStart;
#endif
};

} // namespace Srp

DefineCoreType(otSrpClientHostInfo, Srp::Client::HostInfo);
DefineCoreType(otSrpClientService, Srp::Client::Service);
DefineMapEnum(otSrpClientItemState, Srp::Client::ItemState);

} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE

#endif // SRP_CLIENT_HPP_
