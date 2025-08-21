/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#ifndef DIAGNOSTIC_SERVER_HPP_
#define DIAGNOSTIC_SERVER_HPP_

#include <openthread/diag_server.h>

#include "common/array.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/owned_ptr.hpp"
#include "config/diagnostic_server.h"
#include "net/ip6_address.hpp"
#include "thread/diagnostic_server_types.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

#if OPENTHREAD_FTD
class Child;
class Router;
#endif

namespace DiagnosticServer {

class Client;

/**
 * Implements the Diagnostic Server server functionality for both routers and end devices.
 */
class Server : public InstanceLocator, private NonCopyable
{
private:
    friend class ot::TimeTicker;
    friend class Tmf::Agent;

    static constexpr uint16_t kCacheBuffersLimit    = OPENTHREAD_CONFIG_DIAG_SERVER_CACHE_BUFFERS_LIMIT;
    static constexpr uint32_t kRegistrationInterval = OPENTHREAD_CONFIG_DIAG_SERVER_REGISTRATION_INTERVAL;
    static constexpr uint32_t kUpdateBaseDelay      = OPENTHREAD_CONFIG_DIAG_SERVER_UPDATE_BASE_DELAY;
    static constexpr uint32_t kUpdateExtDelay       = OPENTHREAD_CONFIG_DIAG_SERVER_UPDATE_EXT_DELAY;

    /**
     * Delay between attempts to update child server state.
     *
     * In milliseconds.
     */
    static constexpr uint32_t kChildUpdateDelay = Time::kOneSecondInMsec;

    /**
     * Bitamsk of TLVs for which the extended delay duration should be applied.
     */
    static constexpr otDiagServerTlvSet kExtDelayTlvs{{
        {(1U << Tlv::kLastHeard) | (1U << Tlv::kConnectionTime) | (1U << Tlv::kLinkMarginIn),
         (1U << (Tlv::kMacLinkErrorRatesOut - 8U)), 0,
         (1U << (Tlv::kMacCounters - 24U)) | (1U << (Tlv::kMacLinkErrorRatesIn - 24U)) |
             (1U << (Tlv::kMleCounters - 24U)) | (1U << (Tlv::kLinkMarginOut - 24U))},
    }};

    /**
     * Bitamsk of TLVs which are static in neighbors and therefore do not need
     * to be sent in updates.
     */
    static constexpr otDiagServerTlvSet kNeighborStaticTlvs{{
        {(1U << Tlv::kMacAddress) | (1U << Tlv::kConnectionTime), 0U, (1U << (Tlv::kThreadSpecVersion - 16U)), 0U},
    }};

    bool HasExtDelayTlvs(const TlvSet &aTlvs)
    {
        return !aTlvs.Intersect(static_cast<const TlvSet &>(kExtDelayTlvs)).IsEmpty();
    }

    bool IsOnlyExtDelayTlvs(const TlvSet &aTlvs)
    {
        return aTlvs.Cut(static_cast<const TlvSet &>(kExtDelayTlvs)).IsEmpty();
    }

public:
#if OPENTHREAD_FTD
    /**
     * Provides per child state for the Diagnostic Server on routers.
     */
    class ChildInfo
    {
        friend class Server;

    private:
        /**
         * Represents the state of the Diagnostic Server on a child.
         */
        enum DiagState : uint8_t
        {
            kDiagServerStopped = 0,   ///< The diagnostic server is stopped
            kDiagServerActive,        ///< The diagnostic server is active
            kDiagServerStopPending,   ///< A stop command to the child is pending a response
            kDiagServerActivePending, ///< A active command to the child is pending a response
            kDiagServerUnknown,       ///< The last command to the child was not acked
        };

        /**
         * Marks the specified host provided TLV as having changed state.
         *
         * TLVs not provided by the host will be filtered out.
         */
        void MarkDiagsDirty(TlvSet aTlvs);

        /**
         * Marks the child as a full thread device.
         *
         * Note: This is duplaced from the child state itself since this class
         * has no access to the child class it belongs to. However since it only
         * requires a single bit and signifficantly improves the ability for
         * encapsulation it is done here anyways.
         */
        void SetDiagFtd(bool aFtd) { mDiagFtd = aFtd; }

        DiagState GetDiagServerState(void) const { return mState; }
        bool      IsDiagServerPending(void) const
        {
            return (mState == kDiagServerActivePending) || (mState == kDiagServerStopPending);
        }

        void SetDiagServerState(DiagState aState) { mState = aState; }

        bool IsAttachStateDirty(void) const { return mAttachStateDirty; }
        void SetAttachStateDirty(void) { mAttachStateDirty = true; }

        uint16_t GetUsedCacheBuffers(void) const { return mCacheBuffers; }

        /**
         * Prepares the diagnostic cache to send a update to clients.
         *
         * MUST be match with a later call to either `CommitDiagUpdate`
         * or `AbortDiagUpdate`.
         */
        void BeginDiagUpdate(void)
        {
            OT_ASSERT(!mDiagCacheLocked);
            mDiagCacheLocked = true;
        }
        void CommitDiagUpdate(void);
        void AbortDiagUpdate(void);

        bool ShouldSendDiagUpdate(void) const { return !mDirtySet.IsEmpty() || mAttachStateDirty; }

        TlvSet GetDirtyHostProvided(TlvSet aFilter) const;

        bool CanEvictCache(void) const { return (mCache != nullptr) && !mDiagCacheLocked; }

        void EvictDiagCache(void);
        void ResetDiagCache(void);

        /**
         * Updates the diag cache with a update message from the child.
         */
        Error UpdateDiagCache(const Message &aMessage, TlvSet aFilter);

        /**
         * Appends the current diag cache to the message.
         *
         * MUST be called within a diagnostic update block by first calling
         * `BeginDiagUpdate`.
         */
        Error AppendDiagCache(Message &aMessage);

        void SetDiagClean(void);

        TlvSet GetLostDiag(void) const { return mLostSet; }

        bool ShouldSendLostDiagQuery(void) const { return !mLostSet.IsEmpty() && !mLostQueryPending; }
        void SetLostDiagQueryPending(bool aPending) { mLostQueryPending = aPending; }

    private:
        DiagState mState : 3;
        bool      mDiagFtd : 1;     ///< When in state valid must be true if the child is a FTD
        bool mLostQueryPending : 1; ///< True if a query for lost diagnostic data is pending a response from the child
        bool mAttachStateDirty : 1; ///< True if the child state has changed since the last diagnostic update
        bool mDiagCacheLocked : 1;  ///< If true the diagnostic cache must not be evicted

        uint8_t mCacheBuffers; ///< The number of buffers used for the diag cache

        TlvSet            mDirtySet; ///< Includes both host dirty as well as cached diag
        TlvSet            mLostSet;  ///< Diag that was evicted from the cache
        OwnedPtr<Message> mCache;
    };
#endif

    explicit Server(Instance &aInstance);

    /**
     * Called when this thread device detaches. Including for router upgrades.
     */
    void HandleDetach(void) { StopServer(); }

    /**
     * Signals that some diagnostic type of this thread device has changed.
     *
     * @param[in]  aTlv  The diagnostic type that has changed state.
     */
    void MarkDiagDirty(Tlv::Type aTlv);

    /**
     * Signals that some collection of diagnostic types of this thread device
     * have changed.
     *
     * @param[in]  aTlvs  The set of diagnostic types that have changed state.
     */
    void MarkDiagDirty(TlvSet aTlvs);

#if OPENTHREAD_FTD
    void MarkChildDiagDirty(Child &aChild, Tlv::Type aTlv);

    void MarkChildDiagDirty(Child &aChild, TlvSet aTlvs);

    /**
     * Called when a new child has been added.
     *
     * @param[in]  aChild  The child that has been added.
     */
    void HandleChildAdded(Child &aChild);

    /**
     * Called when a child has been removed.
     *
     * @param[in]  aChild  The child that has been removed.
     */
    void HandleChildRemoved(Child &aChild);

    /**
     * Called when a router link has been added.
     *
     * @param[in]  aRouter  The router that has been added.
     */
    void HandleRouterAdded(Router &aRouter);

    /**
     * Called when a router link has been removed.
     *
     * @param[in]  aRouter  The router that has been removed.
     */
    void HandleRouterRemoved(Router &aRouter);

    /**
     * Attempts to evict diagnostic cache buffers to free up memory for messages.
     *
     * @param[in]  aOnlyRxOn  If true only caches from rx on when idle devices are considered.
     *
     * @retval kErrorNone      Successfully evicted at least one message buffer.
     * @retval kErrorNotFound  If no caches could be evicted.
     */
    Error EvictDiagCache(bool aOnlyRxOn);

    /**
     * Returns the total number of cache evictions from devices that are either
     * rx on when idle or csl synchronized.
     */
    uint32_t GetCacheSyncEvictions(void) const { return mCacheSyncEvictions; }

    /**
     * Returns the total number of cache evictions from devices that are rx off
     * when idle and not csl synchronized.
     */
    uint32_t GetCachePollEvictions(void) const { return mCachePollEvictions; }

    /**
     * Returns the total number of cases where a child update message failed to
     * be added to the diagnostic cache.
     */
    uint32_t GetCacheErrors(void) const { return mCacheErrors; }
#endif

    void HandleNotifierEvents(Events aEvents);

private:
    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Error SendChildResponse(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    bool  StartServerAsChild(const TlvSet &aTypes);

    void StopServer(void);

    Error SendUpdateAsChild(void);

    void HandleChildUpdateResponse(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult);
    static void HandleChildUpdateResponse(void                *aContext,
                                          otMessage           *aResponse,
                                          const otMessageInfo *aMessageInfo,
                                          otError              aResult);

#if OPENTHREAD_FTD
    Child *FindPeerAsChild(const Ip6::MessageInfo &aInfo);

    Error SendQueryResponse(const Ip6::Address &aPeerAddr);
    Error SendRegistrationResponse(const Ip6::Address &aPeerAddr);
    Error SendUpdateAsRouter(void);

    Error StartServerAsRouter(const TlvSet &aSelf, const TlvSet &aChild, const TlvSet &aNeighbor, bool aQuery);

    void UpdateChildServers(bool aMtdChanged, bool aFtdChanged, bool aQuery);
    void UpdateChildServer(Child &aChild, bool aQuery);

    Error SendChildStop(Child &aChild);
    Error SendChildStart(Child &aChild, const TlvSet &aTypes, bool aQuery);
    Error SendChildQuery(Child &aChild, const TlvSet &aTypes, bool aLost);

    void        HandleChildCommandResponse(Child                  &aChild,
                                           Coap::Message          *aResponse,
                                           const Ip6::MessageInfo *aMessageInfo,
                                           Error                   aResult);
    static void HandleChildCommandResponse(void                *aContext,
                                           otMessage           *aResponse,
                                           const otMessageInfo *aMessageInfo,
                                           otError              aResult);

    void        HandleChildLostQueryResponse(Child                  &aChild,
                                             Coap::Message          *aResponse,
                                             const Ip6::MessageInfo *aMessageInfo,
                                             Error                   aResult);
    static void HandleChildLostQueryResponse(void                *aContext,
                                             otMessage           *aResponse,
                                             const otMessageInfo *aMessageInfo,
                                             otError              aResult);
#endif

    // Error AppendStaticTlvs(Message &aMessage, StaticTlvSet aTlvs);
    Error AppendSelfTlvs(Message &aMessage, TlvSet aTlvs);
#if OPENTHREAD_FTD
    Error AppendHostContext(Message &aMessage, TlvSet aTlvs);
    Error AppendChildContextQuery(Message &aMessage, Child &aChild);
    Error AppendChildContextUpdate(Message &aMessage, Child &aChild);
    Error AppendChildTlvs(Message &aMessage, TlvSet aTlvs, const Child &aChild);
    Error AppendNeighborContextQuery(Message &aMessage, const Router &aRouter);
    Error AppendNeighborContextUpdate(Message &aMessage, uint8_t aId);
    Error AppendNeighborTlvs(Message &aMessage, TlvSet aTlvs, const Router &aNeighbor);

    void BeginDiagUpdate(void);
    void CommitDiagUpdate(void);
    void AbortDiagUpdate(void);
#endif

    bool FilterIp6Address(const Ip6::Address &aAddress);
    bool FilterAloc(const Ip6::Address &aAddress, uint8_t &aAloc);
    bool FilterIp6LinkLocalAddress(const Ip6::Address &aAddress);

    Error AppendSelfIp6AddressList(Message &aMessage);
    Error AppendSelfAlocList(Message &aMessage);
    Error AppendSelfIp6LinkLocalAddressList(Message &aMessage);

#if OPENTHREAD_FTD
    Error AppendChildIp6AddressList(Message &aMessage, const Child &aChild);
    Error AppendChildAlocList(Message &aMessage, const Child &aChild);
#endif

    void ScheduleUpdateTimer(uint32_t aDelay) { mUpdateTimer.FireAtIfEarlier(TimerMilli::GetNow() + aDelay); }
    void HandleUpdateTimer(void);

#if OPENTHREAD_FTD
    void UpdateIfCacheBufferLimit(void);

    void ScheduleChildTimer(void) { mChildTimer.FireAtIfEarlier(TimerMilli::GetNow() + kChildUpdateDelay); }
    void HandleChildTimer(void);
    void HandleRegistrationTimer(void);
#endif

    using UpdateTimer = TimerMilliIn<Server, &Server::HandleUpdateTimer>;
#if OPENTHREAD_FTD
    using ChildTimer        = TimerMilliIn<Server, &Server::HandleChildTimer>;
    using RegistrationTimer = TimerMilliIn<Server, &Server::HandleRegistrationTimer>;
#endif

    bool mActive : 1;
    bool mUpdateSent : 1; ///< On children set to true when a update message has been sent to the parent and not yet
                          ///< been acked
#if OPENTHREAD_FTD
    bool mMultiClient : 1;      ///< If true then there are more than 1 client receiving notifications.
    bool mMultiClientRenew : 1; ///< If true then there are more then 1 client receiving notifications after the next
                                ///< registration interval expires.
    uint16_t mClientRloc;       ///< Rloc16 of the client that last sent a registration request

    uint32_t mCacheSyncEvictions;
    uint32_t mCachePollEvictions;
    uint32_t mCacheErrors;

    uint64_t mRouterStateMask; ///< Bitmask of router ids which have changed link state.
#endif

    TlvSet mSelfEnabled; ///< The TLVs which are requested by clients for this device
    TlvSet
        mSelfRenewUpdate; ///< On routers the TLVs which are to be renewed for this device in the next registration
                          ///< interval. On children the set of TLVs which have been sent with the last update message.

#if OPENTHREAD_FTD
    TlvSet mChildEnabled; ///< The TLVs which are requested by clients for children
    TlvSet mChildRenew;   ///< The TLVs which are to be renewed in the next registration interval for children.

    TlvSet mNeighborEnabled; ///< The TLVs which are requested by clients for router neighbors
    TlvSet mNeighborRenew; ///< The TLVs which are to be renewed in the next registration interval for router neighbors.

    uint64_t mSequenceNumber; ///< The current sequence number used by the server

#endif

    TlvSet mSelfDirty; ///< The TLVs of this device which have changed since the last update

    UpdateTimer mUpdateTimer;
#if OPENTHREAD_FTD
    ChildTimer        mChildTimer;
    RegistrationTimer mRegistrationTimer;
#endif
};

DeclareTmfHandler(Server, kUriDiagnosticEndDeviceRequest);
#if OPENTHREAD_FTD
DeclareTmfHandler(Server, kUriDiagnosticEndDeviceUpdate);
DeclareTmfHandler(Server, kUriDiagnosticServerRequest);
#endif

/**
 * Implements Diagnostic Server client functionality.
 */
#if OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE
class Client : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;

    static constexpr uint32_t kRegistrationInterval =
        OPENTHREAD_CONFIG_DIAG_SERVER_REGISTRATION_INTERVAL -
        (OPENTHREAD_CONFIG_DIAG_CLIENT_REGISTRATION_JITTER * OPENTHREAD_CONFIG_DIAG_CLIENT_REGISTRATION_AHEAD);
    static constexpr uint32_t kRegistrationJitter = OPENTHREAD_CONFIG_DIAG_CLIENT_REGISTRATION_JITTER;

public:
    explicit Client(Instance &aInstance);

    /**
     * Starts the diagnostic server client and requests the provided TLVs from
     * all servers.
     *
     * @param[in]  aHost      The TLVs requested for Host Contexts. Can be nullptr.
     * @param[in]  aChild     The TLVs requested for Child Contexts. Can be nullptr.
     * @param[in]  aNeighbor  The TLVs requested for Neighbor Contexts. Can be nullptr.
     */
    void Start(const TlvSet              *aHost,
               const TlvSet              *aChild,
               const TlvSet              *aNeighbor,
               otDiagServerUpdateCallback aCallback,
               void                      *aContext);

    /**
     * Stops the diagnostic server client.
     *
     * Any registered callback will immediately stop receiving updates until
     * they are explicitly re-registered with another call to `Start`.
     */
    void Stop(void);

    /**
     * Implements `otDiagServerGetNextContext`.
     */
    static Error GetNextContext(const Coap::Message  &aMessage,
                                otDiagServerIterator &aIterator,
                                otDiagServerContext  &aContext);

    /**
     * Implements `otDiagServerGetNextTlv`.
     */
    static Error GetNextTlv(const Coap::Message &aMessage, otDiagServerContext &aContext, otDiagServerTlv &aTlv);

    /**
     * Implements `otDiagServerGetIp6Addresses`.
     */
    static Error GetIp6Addresses(const Coap::Message &aMessage,
                                 uint16_t             aDataOffset,
                                 uint16_t             aCount,
                                 otIp6Address        *aAddresses);

    /**
     * Implements `otDiagServerGetAlocs`.
     */
    static Error GetAlocs(const Coap::Message &aMessage, uint16_t aDataOffset, uint16_t aCount, uint8_t *aAlocs);

private:
    Error SendRegistration(bool aQuery);
    Error SendErrorQuery(uint16_t aRloc16);

    Error AppendContextTo(Message &aMessage, DeviceType aType, const TlvSet &aSet);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void ProcessUpdate(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void        HandleResponse(Coap::Message *aResponse, const Ip6::MessageInfo *aMessageInfo, Error aResult);
    static void HandleResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aResult);

    void ScheduleNextUpdate(void);
    void ScheduleRetry(void);

    void HandleUpdateTimer(void);

    using UpdateTimer = TimerMilliIn<Client, &Client::HandleUpdateTimer>;

    bool mActive : 1;
    bool mQueryPending : 1;

    TlvSet mHostSet;
    TlvSet mChildSet;
    TlvSet mNeighborSet;

    Array<uint64_t, Mle::kMaxRouterId + 1> mServerSeqNumbers; ///< The last received sequence number from a server

    UpdateTimer mTimer;

    otDiagServerUpdateCallback mCallback;
    void                      *mCallbackContext;
};

DeclareTmfHandler(Client, kUriDiagnosticServerUpdate);
#endif // OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE

} // namespace DiagnosticServer

} // namespace ot

#endif // DIAGNOSTIC_SERVER_HPP_
