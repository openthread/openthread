/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#ifndef OT_CORE_THREAD_MESH_MONITOR_HPP_
#define OT_CORE_THREAD_MESH_MONITOR_HPP_

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/owned_ptr.hpp"
#include "config/mesh_monitor.h"
#include "net/ip6_address.hpp"
#include "thread/mesh_monitor_types.hpp"
#include "thread/tmf.hpp"
#include "thread/uri_paths.hpp"

namespace ot {

#if OPENTHREAD_FTD
class Child;
class Router;
#endif

namespace MeshMonitor {

#if OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE
/**
 * Implements the Mesh Monitor server functionality for both routers and end devices.
 */
class Server : public InstanceLocator, private NonCopyable
{
private:
    friend class ot::TimeTicker;
    friend class Tmf::Agent;

    static constexpr uint16_t kCacheBuffersLimit    = OPENTHREAD_CONFIG_MESH_MONITOR_CACHE_BUFFERS_LIMIT;
    static constexpr uint32_t kRegistrationInterval = OPENTHREAD_CONFIG_MESH_MONITOR_REGISTRATION_INTERVAL;
    static constexpr uint32_t kUpdateBaseDelay      = OPENTHREAD_CONFIG_MESH_MONITOR_UPDATE_BASE_DELAY;
    static constexpr uint32_t kUpdateExtDelay       = OPENTHREAD_CONFIG_MESH_MONITOR_UPDATE_EXT_DELAY;
    static constexpr uint32_t kUpdateJitter         = OPENTHREAD_CONFIG_MESH_MONITOR_UPDATE_JITTER;

    /**
     * Hard limit for SU message length to avoid oversized updates when many children exist.
     * IPv6 minimum MTU is 1280 bytes, subtracting UDP(8) + IPv6(40) headers leaves 1232 bytes for the CoAP message.
     */
    static constexpr uint16_t kMaxUpdateMessageLength = 1232;

    /**
     * Maximum number of consecutive SU (Server Update) retry attempts when ACKs fail.
     * Used for exponential backoff in HandleServerUpdateAck().
     */
    static constexpr uint8_t kMaxUpdateRetries = 5;

    /**
     * Maximum backoff delay for SU retries (in milliseconds).
     * Caps the exponential backoff at 320 seconds (5.3 minutes).
     */
    static constexpr uint32_t kMaxUpdateBackoff = 320000;

    /**
     * Delay between attempts to update child server state.
     *
     * In milliseconds.
     */
    static constexpr uint32_t kChildUpdateDelay = Time::kOneSecondInMsec;

    /**
     * Bitmask of TLVs for which the extended delay duration should be applied.
     */
    static const TlvSet kExtDelayTlvMask;

    /**
     * Bitmask of TLVs which are static in neighbors and therefore do not need
     * to be sent in updates.
     */
    static const TlvSet kStaticNeighborTlvMask;

    /**
     * Bitmask of child-provided TLVs to skip when a child attaches/re-attaches.
     *
     * When a child changes parent, we avoid re-querying these TLVs immediately
     * to reduce traffic. They will still be sent:
     * - On initial client registration (baseline query)
     * - Periodically via normal update cycles
     *
     */
    static const TlvSet kChildAttachSkipTlvMask;

    /**
     * Determines whether the given TLV set has any TLVs that require extended delay.
     *
     * @param[in]  aTlvs  The TLV set to check.
     * @returns TRUE if any TLVs in `aTlvs` require extended delay, FALSE otherwise.
     */
    bool HasExtDelayTlvs(const TlvSet &aTlvs)
    {
        return !aTlvs.Intersect(static_cast<const TlvSet &>(kExtDelayTlvMask)).IsEmpty();
    }

    /**
     * Determines whether the given TLV set contains only TLVs that require extended delay.
     *
     * @param[in]  aTlvs  The TLV set to check.
     * @returns TRUE if all TLVs in `aTlvs` require extended delay, FALSE otherwise.
     */
    bool IsOnlyExtDelayTlvs(const TlvSet &aTlvs)
    {
        return aTlvs.Cut(static_cast<const TlvSet &>(kExtDelayTlvMask)).IsEmpty();
    }

    /**
     * Gets the intersection of the given TLV set with the extended delay TLVs.
     *
     * @param[in]  aTlvs  The TLV set to intersect.
     * @returns A TLV set containing only the TLVs from `aTlvs` that require extended delay.
     */
    TlvSet GetExtDelayTlvs(const TlvSet &aTlvs)
    {
        return aTlvs.Intersect(static_cast<const TlvSet &>(kExtDelayTlvMask));
    }

public:
#if OPENTHREAD_FTD
    /**
     * Provides per child state for the diagnostic server on routers.
     */
    class ChildInfo
    {
        friend class Server;

    private:
        enum DiagState : uint8_t
        {
            kDiagServerStopped = 0,   ///< The diagnostic server is stopped
            kDiagServerActive,        ///< The diagnostic server is active
            kDiagServerStopPending,   ///< A stop command to the child is pending a response
            kDiagServerActivePending, ///< An active command to the child is pending a response
            kDiagServerUnknown,       ///< The last command to the child was not acked
        };

        void MarkHostProvidedTlvsChanged(TlvSet aTlvs);

        void SetChildIsFtd(bool aFtd) { mIsFtd = aFtd; }

        DiagState GetDiagServerState(void) const { return static_cast<DiagState>(mState); }

        bool IsDiagServerPending(void) const
        {
            return (mState == kDiagServerActivePending) || (mState == kDiagServerStopPending);
        }

        void SetDiagServerState(DiagState aState) { mState = aState; }

        bool IsAttachStateDirty(void) const { return mAttachStateDirty; }
        void SetAttachStateDirty(void) { mAttachStateDirty = true; }

        uint16_t GetUsedCacheBuffers(void) const { return mCacheBuffers; }

        void LockCache(void)
        {
            OT_ASSERT(!mDiagCacheLocked);
            mDiagCacheLocked = true;
        }
        void CommitCacheUpdate(void);
        void AbortCacheUpdate(void);

        bool IsDiagCacheLocked(void) const { return mDiagCacheLocked; }

        bool ShouldSendDiagUpdate(void) const { return !mDirtySet.IsEmpty() || mAttachStateDirty; }

        TlvSet GetDirtyHostProvided(TlvSet aFilter) const;

        bool CanEvictCache(void) const { return (mCache != nullptr) && !mDiagCacheLocked; }

        void EvictCache(void);
        void ClearCache(void);

        Error UpdateCache(const Message &aMessage, TlvSet aFilter);

        Error RemoveCachedTlv(uint8_t aType);

        Error AppendCachedTlvs(Message &aMessage);

        TlvSet GetLostDiag(void) const { return mLostSet; }

        bool ShouldSendLostDiagQuery(void) const { return !mLostSet.IsEmpty() && !mLostQueryPending; }
        void SetLostDiagQueryPending(bool aPending) { mLostQueryPending = aPending; }

    private:
        uint8_t mState : 4;
        bool    mIsFtd : 1;         ///< When in state valid must be true if the child is a FTD
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
    Error EvictCache(bool aOnlyRxOn);

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
    template <Uri kUri> void HandleTmf(Coap::Msg &aMsg);

    bool ConfigureAsEndDevice(const TlvSet &aTypes);

    void StopServer(void);

    Error SendEndDeviceUpdate(void);

    DeclareTmfResponseHandlerIn(Server, HandleEndDeviceUpdateAck);

#if OPENTHREAD_FTD
    Error SendFullServerUpdate(const Ip6::Address &aClientAddr);

    Error SendEmptyServerUpdate(const Ip6::Address &aClientAddr);

    Error SendServerUpdate(void);

    Error ConfigureAsRouter(const TlvSet &aSelf, const TlvSet &aChild, const TlvSet &aNeighbor, bool aQuery);

    void SyncAllChildDiagStates(bool aMtdChanged, bool aFtdChanged, bool aQuery);

    void SyncChildDiagState(Child &aChild, bool aQuery);

    Error SendEndDeviceRequestStop(Child &aChild);

    Error SendEndDeviceRequestStart(Child &aChild, const TlvSet &aTypes, bool aQuery);

    Error SendEndDeviceRecoveryQuery(Child &aChild, const TlvSet &aTypes);

    void        HandleEndDeviceRequestAck(Child &aChild, Coap::Msg *aMsg, Error aResult);
    static void HandleEndDeviceRequestAck(void *aContext, Coap::Msg *aMsg, Error aResult);

    void        HandleEndDeviceRecoveryAck(Child &aChild, Coap::Msg *aMsg, Error aResult);
    static void HandleEndDeviceRecoveryAck(void *aContext, Coap::Msg *aMsg, Error aResult);

    DeclareTmfResponseHandlerIn(Server, HandleServerUpdateAck);
#endif

    Error AppendHostTlvs(Message &aMessage, TlvSet aTlvs);
#if OPENTHREAD_FTD
    Error AppendHostContext(Message &aMessage, TlvSet aTlvs);

    Error AppendChildContext(Message &aMessage, Child &aChild);

    Error AppendChildTlvs(Message &aMessage, TlvSet aTlvs, const Child &aChild);

    Error AppendNeighborContextBaseline(Message &aMessage, const Router &aRouter);

    Error AppendNeighborContextUpdate(Message &aMessage, uint8_t aId);

    Error AppendNeighborTlvs(Message &aMessage, TlvSet aTlvs, const Router &aNeighbor);

    bool AppendChildContextBatch(Message &aMessage, bool &aNeedsMore);
    bool AppendNeighborContextBatch(Message &aMessage, bool &aNeedsMore);

    void LockChildCaches(void);

    void CommitChildCacheUpdates(void);

    void UnlockChildCaches(void);
#endif

    bool ShouldIncludeIp6Address(const Ip6::Address &aAddress);

    bool ShouldIncludeAloc(const Ip6::Address &aAddress, uint8_t &aAloc);

    bool ShouldIncludeLinkLocalAddress(const Ip6::Address &aAddress);

    Error AppendHostIp6AddressList(Message &aMessage);

    Error AppendHostAlocList(Message &aMessage);

    Error AppendHostLinkLocalAddressList(Message &aMessage);

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
    bool     mSendFullUpdate : 1;
    bool     mSendChildBaseline : 1;    ///< When set, next SU will include baseline host-provided child TLVs
    bool     mSendNeighborBaseline : 1; ///< When set, next SU will include baseline neighbor TLVs
    bool     mUpdatePending : 1;
    uint16_t mChildResumeIndex;    ///< Index to resume child iteration from after size-limited SU
    uint8_t  mNeighborResumeIndex; ///< Router ID to resume neighbor iteration from after size-limited SU

    uint16_t mClientRloc; ///< Rloc16 of the client that last sent a registration request

    uint8_t mUpdateRetryCount; ///< Number of consecutive ACK failures (for exponential backoff)

    uint32_t mCacheSyncEvictions;
    uint32_t mCachePollEvictions;
    uint32_t mCacheErrors;

    uint64_t mRouterStateMask; ///< Bitmask of router ids which have changed link state.
#endif

    TlvSet mSelfEnabled;       ///< The TLVs which are requested by clients for this device
    TlvSet mSelfPendingUpdate; ///< On children: TLVs sent in last EU, pending ACK (for retry on failure)

#if OPENTHREAD_FTD
    TlvSet mChildEnabled;    ///< The TLVs which are requested by clients for children
    TlvSet mNeighborEnabled; ///< The TLVs which are requested by clients for router neighbors

    bool mClientRegistered; ///< True if client sent registration this interval

    uint64_t mSequenceNumber; ///< The current sequence number used by the server
#endif

    TlvSet mSelfDirty; ///< The TLVs of this device which have changed since the last update

    UpdateTimer mUpdateTimer;
#if OPENTHREAD_FTD
    ChildTimer        mChildTimer;
    RegistrationTimer mRegistrationTimer;
#endif
};

DeclareTmfHandler(Server, kUriMeshMonEndDeviceRequest);

#if OPENTHREAD_FTD
DeclareTmfHandler(Server, kUriMeshMonEndDeviceUpdate);
DeclareTmfHandler(Server, kUriMeshMonServerRequest);
#endif // OPENTHREAD_FTD

#endif // OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE

#if OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE
/**
 * Implements Mesh Monitor client functionality.
 */
class Client;
#endif // OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE

} // namespace MeshMonitor

} // namespace ot

#endif // OT_CORE_THREAD_MESH_MONITOR_HPP_
